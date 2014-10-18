/*
 * gpdma_base.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <string.h>
#include <irq.h>
#include <memory.h>
#include <platform/platform_defs.h>
#include <platform/nxp/gpdma_base.h>
#include <platform/nxp/gpdma_defs.h>
#include <platform/nxp/lpc43xx/system.h>
/*----------------------------------------------------------------------------*/
struct DmaHandler
{
  struct Entity parent;

  /* Channel descriptors currently in use */
  struct GpDmaBase *descriptors[8];
  /* Initialized descriptors count */
  uint16_t instances;
  /* Peripheral connection statuses */
  uint8_t connections[16];
};
/*----------------------------------------------------------------------------*/
static inline void *calcPeripheral(uint8_t);
/*----------------------------------------------------------------------------*/
static uint8_t dmaHandlerAllocate(struct GpDmaBase *, enum gpDmaEvent);
static void dmaHandlerAttach();
static void dmaHandlerDetach();
static void dmaHandlerFree(struct GpDmaBase *);
static void dmaHandlerInstantiate();
static enum result dmaHandlerInit(void *, const void *);
/*----------------------------------------------------------------------------*/
static enum result channelInit(void *, const void *);
static void channelDeinit(void *);
/*----------------------------------------------------------------------------*/
static const struct EntityClass handlerTable = {
    .size = sizeof(struct DmaHandler),
    .init = dmaHandlerInit,
    .deinit = 0
};
/*----------------------------------------------------------------------------*/
static const struct EntityClass channelTable = {
    .size = sizeof(struct GpDmaBase),
    .init = channelInit,
    .deinit = channelDeinit
};
/*----------------------------------------------------------------------------*/
static const enum gpDmaEvent eventMap[16][4] = {
    {GPDMA_SPIFI,   GPDMA_SCT_OUT2,   GPDMA_SGPIO14,    GPDMA_MAT3_1},
    {GPDMA_MAT0_0,  GPDMA_UART0_TX,   GPDMA_EVENT_END,  GPDMA_EVENT_END},
    {GPDMA_MAT0_1,  GPDMA_UART0_RX,   GPDMA_EVENT_END,  GPDMA_EVENT_END},
    {GPDMA_MAT1_0,  GPDMA_UART1_TX,   GPDMA_I2S1_REQ1,  GPDMA_SSP1_TX},
    {GPDMA_MAT1_1,  GPDMA_UART1_RX,   GPDMA_I2S1_REQ2,  GPDMA_SSP1_RX},
    {GPDMA_MAT2_0,  GPDMA_UART2_TX,   GPDMA_SSP1_TX,    GPDMA_SGPIO15},
    {GPDMA_MAT2_1,  GPDMA_UART2_RX,   GPDMA_SSP1_RX,    GPDMA_SGPIO14},
    {GPDMA_MAT3_0,  GPDMA_UART3_TX,   GPDMA_SCT_REQ0,   GPDMA_ADCHS_WRITE},
    {GPDMA_MAT3_1,  GPDMA_UART3_RX,   GPDMA_SCT_REQ1,   GPDMA_ADCHS_READ},
    {GPDMA_SSP0_RX, GPDMA_I2S0_REQ1,  GPDMA_SCT_REQ1,   GPDMA_EVENT_END},
    {GPDMA_SSP0_TX, GPDMA_I2S0_REQ2,  GPDMA_SCT_REQ0,   GPDMA_EVENT_END},
    {GPDMA_SSP1_RX, GPDMA_SGPIO14,    GPDMA_UART0_TX,   GPDMA_EVENT_END},
    {GPDMA_SSP1_TX, GPDMA_SGPIO15,    GPDMA_UART0_RX,   GPDMA_EVENT_END},
    {GPDMA_ADC0,    GPDMA_EVENT_END,  GPDMA_SSP1_RX,    GPDMA_UART3_RX},
    {GPDMA_ADC1,    GPDMA_EVENT_END,  GPDMA_SSP1_TX,    GPDMA_UART3_TX},
    {GPDMA_DAC,     GPDMA_SCT_OUT3,   GPDMA_SGPIO15,    GPDMA_MAT3_0}
};
/*----------------------------------------------------------------------------*/
static const struct EntityClass * const DmaHandler = &handlerTable;
const struct EntityClass * const GpDmaBase = &channelTable;
static struct DmaHandler *dmaHandler = 0;
/*----------------------------------------------------------------------------*/
static inline void *calcPeripheral(uint8_t channel)
{
  return (void *)((uint32_t)LPC_GPDMACH0 + ((uint32_t)LPC_GPDMACH1
      - (uint32_t)LPC_GPDMACH0) * channel);
}
/*----------------------------------------------------------------------------*/
const struct GpDmaBase *gpDmaGetDescriptor(uint8_t channel)
{
  assert(channel < GPDMA_CHANNEL_COUNT);

  return dmaHandler->descriptors[channel];
}
/*----------------------------------------------------------------------------*/
enum result gpDmaSetDescriptor(uint8_t channel, struct GpDmaBase *descriptor)
{
  assert(descriptor);
  assert(channel < GPDMA_CHANNEL_COUNT);

  /* Descriptor reset is performed in interrupt handler */
  return compareExchangePointer((void **)(dmaHandler->descriptors + channel),
      0, descriptor) ? E_OK : E_BUSY;
}
/*----------------------------------------------------------------------------*/
void gpDmaSetupMux(struct GpDmaBase *descriptor)
{
  LPC_CREG->DMAMUX = (LPC_CREG->DMAMUX & descriptor->mux.mask)
      | descriptor->mux.value;
}
/*----------------------------------------------------------------------------*/
void GPDMA_ISR(void)
{
  const uint8_t errorStatus = LPC_GPDMA->INTERRSTAT;
  const uint8_t terminalStatus = LPC_GPDMA->INTTCSTAT;
  const uint8_t intStatus = errorStatus | terminalStatus;

  LPC_GPDMA->INTERRCLEAR = errorStatus;
  LPC_GPDMA->INTTCCLEAR = terminalStatus;

  for (uint8_t index = 0; index < GPDMA_CHANNEL_COUNT; ++index)
  {
    const uint8_t mask = 1 << index;

    if (intStatus & mask)
    {
      struct GpDmaBase * const descriptor = dmaHandler->descriptors[index];
      LPC_GPDMACH_Type * const reg = descriptor->reg;
      enum result res = E_OK;

      if (!(reg->CONFIG & CONFIG_ENABLE))
      {
        /* Clear descriptor when channel is stopped or transfer is completed */
        dmaHandler->descriptors[index] = 0;
      }
      else
        res = E_BUSY;

      if (errorStatus & mask)
        res = E_ERROR;

      descriptor->handler(descriptor, res);
    }
  }
}
/*----------------------------------------------------------------------------*/
static uint8_t dmaHandlerAllocate(struct GpDmaBase *channel,
    enum gpDmaEvent event)
{
  uint8_t minIndex = 0, minValue = 0;
  bool found = false;

  assert(event < GPDMA_MEMORY);

  dmaHandlerInstantiate();

  for (uint8_t index = 0; index < 16; ++index)
  {
    bool allowed = false;

    for (uint8_t entry = 0; entry < 4; ++entry)
    {
      if (eventMap[index][entry] == event)
        allowed = true;
    }

    if (allowed && (!found || minValue < dmaHandler->connections[index]))
    {
      found = true;
      minIndex = index;
      minValue = dmaHandler->connections[index];
    }
  }

  assert(found);

  ++dmaHandler->connections[minIndex];
  channel->mux.mask &= ~(0x03 << (minIndex << 1));
  channel->mux.value = minValue << (minIndex << 1);
  return minIndex;
}
/*----------------------------------------------------------------------------*/
static void dmaHandlerAttach()
{
  dmaHandlerInstantiate();

  if (!dmaHandler->instances++)
  {
    sysClockEnable(CLK_M4_GPDMA);
    sysResetEnable(RST_GPDMA);
    LPC_GPDMA->CONFIG |= DMA_ENABLE;
    irqEnable(GPDMA_IRQ);
  }
}
/*----------------------------------------------------------------------------*/
static void dmaHandlerDetach()
{
  /* Disable peripheral when no active descriptors exist */
  if (!--dmaHandler->instances)
  {
    irqDisable(GPDMA_IRQ);
    LPC_GPDMA->CONFIG &= ~DMA_ENABLE;
    sysClockDisable(CLK_M4_GPDMA);
  }
}
/*----------------------------------------------------------------------------*/
static void dmaHandlerFree(struct GpDmaBase *channel)
{
  uint32_t mask = ~channel->mux.mask;
  uint8_t index = 0;

  /* DMA Handler is already initialized */
  while (mask)
  {
    if (mask & 0x03)
      --dmaHandler->connections[index];

    ++index;
    mask >>= 2;
  }
}
/*----------------------------------------------------------------------------*/
static void dmaHandlerInstantiate()
{
  if (!dmaHandler)
    dmaHandler = init(DmaHandler, 0);

  assert(dmaHandler);
}
/*----------------------------------------------------------------------------*/
static enum result dmaHandlerInit(void *object,
    const void *configBase __attribute__((unused)))
{
  struct DmaHandler * const handler = object;

  /* TODO Add priority configuration for GPDMA interrupt */
  for (uint8_t index = 0; index < GPDMA_CHANNEL_COUNT; ++index)
    handler->descriptors[index] = 0;

  memset(handler->connections, 0, sizeof(handler->connections));

  handler->instances = 0;
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum result channelInit(void *object, const void *configBase)
{
  const struct GpDmaBaseConfig * const config = configBase;
  struct GpDmaBase * const channel = object;

  assert(config->channel < GPDMA_CHANNEL_COUNT);

  channel->config = 0;
  channel->control = 0;
  channel->handler = 0;
  channel->number = config->channel;
  channel->reg = calcPeripheral(channel->number);

  /* Reset multiplexer mask and value */
  channel->mux.mask = 0xFFFFFFFF;
  channel->mux.value = 0;

  if (config->type != GPDMA_TYPE_M2M)
  {
    const uint8_t peripheral = dmaHandlerAllocate(channel, config->event);

    /* Only AHB master 1 can access a peripheral */
    switch (config->type)
    {
      case GPDMA_TYPE_M2P:
        channel->config |= CONFIG_DST_PERIPH(peripheral);
        channel->control |= CONTROL_DST_MASTER(1);
        break;

      case GPDMA_TYPE_P2M:
        channel->config |= CONFIG_SRC_PERIPH(peripheral);
        channel->control |= CONTROL_SRC_MASTER(1);
        break;

      default:
        break;
    }
  }

  /* Register new descriptor */
  dmaHandlerAttach();

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void channelDeinit(void *object)
{
  dmaHandlerFree(object);
  dmaHandlerDetach();
}