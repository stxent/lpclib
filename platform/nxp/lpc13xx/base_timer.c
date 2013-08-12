/*
 * base_timer.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include "threading/mutex.h"
#include "platform/nxp/system.h"
#include "platform/nxp/lpc13xx/base_timer.h"
#include "platform/nxp/lpc13xx/base_timer_defs.h"
#include "platform/nxp/lpc13xx/power.h"
/*----------------------------------------------------------------------------*/
#define DEFAULT_DIV_VALUE   1
#define DEFAULT_PRIORITY    255 /* Lowest interrupt priority in Cortex-M3 */
/*----------------------------------------------------------------------------*/
static void interruptHandler(void *);
static enum result setDescriptor(uint8_t, struct BaseTimer *);
/*----------------------------------------------------------------------------*/
static enum result btInit(void *, const void *);
static void btDeinit(void *);
static void btSetCallback(void *, void (*)(void *), void *);
static void btSetEnabled(void *, bool);
static void btSetFrequency(void *, uint32_t);
static void btSetOverflow(void *, uint32_t);
/*----------------------------------------------------------------------------*/
static const struct TimerClass timerTable = {
    .size = sizeof(struct BaseTimer),
    .init = btInit,
    .deinit = btDeinit,

    .setCallback = btSetCallback,
    .setEnabled = btSetEnabled,
    .setFrequency = btSetFrequency,
    .setOverflow = btSetOverflow
};
/*----------------------------------------------------------------------------*/
const struct TimerClass *BaseTimer = &timerTable;
/*----------------------------------------------------------------------------*/
static struct BaseTimer *descriptors[] = {0, 0, 0, 0};
static Mutex lock = MUTEX_UNLOCKED;
/*----------------------------------------------------------------------------*/
static void interruptHandler(void *object)
{
  struct BaseTimer *device = object;

  if (device->reg->IR & IR_MATCH_INTERRUPT(0)) /* Match 0 */
  {
    if (device->callback)
      device->callback(device->callbackArgument);
    device->reg->IR = IR_MATCH_INTERRUPT(0); /* Clear flag */
  }
}
/*----------------------------------------------------------------------------*/
enum result setDescriptor(uint8_t channel, struct BaseTimer *device)
{
  enum result res = E_ERROR;

  assert(channel < sizeof(descriptors));

  mutexLock(&lock);
  if (!descriptors[channel])
  {
    descriptors[channel] = device;
    res = E_OK;
  }
  mutexUnlock(&lock);
  return res;
}
/*----------------------------------------------------------------------------*/
void TIMER16B0_ISR(void)
{
  if (descriptors[0])
    descriptors[0]->handler(descriptors[0]);
}
/*----------------------------------------------------------------------------*/
void TIMER16B1_ISR(void)
{
  if (descriptors[1])
    descriptors[1]->handler(descriptors[1]);
}
/*----------------------------------------------------------------------------*/
void TIMER32B0_ISR(void)
{
  if (descriptors[2])
    descriptors[2]->handler(descriptors[2]);
}
/*----------------------------------------------------------------------------*/
void TIMER32B1_ISR(void)
{
  if (descriptors[3])
    descriptors[3]->handler(descriptors[3]);
}
/*----------------------------------------------------------------------------*/
static enum result btInit(void *object, const void *configPtr)
{
  /* Set pointer to device configuration data */
  const struct BaseTimerConfig * const config = configPtr;
  struct BaseTimer *device = object;

  /* Check device configuration data */
  assert(config);

  /* Try to set peripheral descriptor */
  device->channel = config->channel;
  if (setDescriptor(device->channel, device) != E_OK)
    return E_ERROR;

  /* Set hardware interrupt handler to default handler */
  device->handler = interruptHandler;

  switch (device->channel)
  {
    case 0:
      sysClockEnable(CLK_CT16B0);
      device->reg = LPC_TMR16B0;
      device->irq = TIMER16B0_IRQ;
      break;
    case 1:
      sysClockEnable(CLK_CT16B1);
      device->reg = LPC_TMR16B1;
      device->irq = TIMER16B1_IRQ;
      break;
    case 2:
      sysClockEnable(CLK_CT32B0);
      device->reg = LPC_TMR32B0;
      device->irq = TIMER32B0_IRQ;
      break;
    case 3:
      sysClockEnable(CLK_CT32B1);
      device->reg = LPC_TMR32B1;
      device->irq = TIMER32B1_IRQ;
      break;
  }

  device->reg->PR = (sysCoreClock / DEFAULT_DIV_VALUE)
      / config->frequency - 1;
  /* Reset control registers */
  device->reg->MCR = 0;
  /* Reset internal counters */
  device->reg->PC = 0;
  device->reg->TC = 0;
  /* Enable timer/counter */
  device->reg->TCR = TCR_CEN;

  /* Clear pending interrupts */
  device->reg->IR = IR_MASK;
  /* Enable interrupt */
  nvicEnable(device->irq);
  /* Set interrupt priority, lowest by default */
  nvicSetPriority(device->irq, DEFAULT_PRIORITY);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void btDeinit(void *object)
{
  struct BaseTimer *device = object;

  /* Disable interrupt */
  nvicDisable(device->irq);
  /* Disable Timer clock */
  switch (device->channel)
  {
    case 0:
      sysClockDisable(CLK_CT16B0);
      break;
    case 1:
      sysClockDisable(CLK_CT16B1);
      break;
    case 2:
      sysClockDisable(CLK_CT32B0);
      break;
    case 3:
      sysClockDisable(CLK_CT32B1);
      break;
  }
  /* Release external clock pin when used*/
  if (gpioGetKey(&device->input))
    gpioDeinit(&device->input);
  /* Reset Timer descriptor */
  setDescriptor(device->channel, 0);
}
/*----------------------------------------------------------------------------*/
static void btSetCallback(void *object, void (*callback)(void *),
    void *argument)
{
  struct BaseTimer *device = object;

  device->callback = callback;
  device->callbackArgument = argument;
  /* Enable or disable Match interrupt and counter reset after each interrupt */
  if (callback)
  {
    device->reg->IR = IR_MATCH_INTERRUPT(0); /* Clear pending interrupt flag */
    device->reg->MCR |= MCR_INTERRUPT(0) | MCR_RESET(0);
  }
  else
    device->reg->MCR &= ~(MCR_INTERRUPT(0) | MCR_RESET(0));
}
/*----------------------------------------------------------------------------*/
static void btSetEnabled(void *object, bool state)
{
  struct BaseTimer *device = object;

  if (state)
  {
    device->reg->TCR &= ~TCR_CRES;
  }
  else
  {
    device->reg->TCR |= TCR_CRES;
    while (device->reg->TC || device->reg->PC);
  }
}
/*----------------------------------------------------------------------------*/
static void btSetFrequency(void *object, uint32_t frequency)
{
  struct BaseTimer *device = object;

  device->reg->PR = (sysCoreClock / DEFAULT_DIV_VALUE) / frequency - 1;
}
/*----------------------------------------------------------------------------*/
static void btSetOverflow(void *object, uint32_t overflow)
{
  struct BaseTimer *device = object;

  device->reg->MR0 = overflow;
  /* Synchronously reset prescaler and counter registers */
  device->reg->TCR |= TCR_CRES;
  while (device->reg->TC || device->reg->PC);
  device->reg->TCR &= ~TCR_CRES;
}