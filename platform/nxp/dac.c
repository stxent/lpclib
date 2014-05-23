/*
 * dac.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <platform/nxp/dac.h>
#include <platform/nxp/dac_defs.h>
/*----------------------------------------------------------------------------*/
static enum result dacInit(void *, const void *);
static void dacDeinit(void *);
static enum result dacCallback(void *, void (*)(void *), void *);
static enum result dacGet(void *, enum ifOption, void *);
static enum result dacSet(void *, enum ifOption, const void *);
static uint32_t dacWrite(void *, const uint8_t *, uint32_t);
/*----------------------------------------------------------------------------*/
static const struct InterfaceClass dacTable = {
    .size = sizeof(struct Dac),
    .init = dacInit,
    .deinit = dacDeinit,

    .callback = dacCallback,
    .get = dacGet,
    .set = dacSet,
    .read = 0,
    .write = dacWrite
};
/*----------------------------------------------------------------------------*/
const struct InterfaceClass *Dac = &dacTable;
/*----------------------------------------------------------------------------*/
static enum result dacInit(void *object, const void *configPtr)
{
  const struct DacConfig * const config = configPtr;
  const struct DacBaseConfig parentConfig = {
      .pin = config->pin
  };
  struct Dac *interface = object;
  enum result res;

  /* Call base class constructor */
  if ((res = DacBase->init(object, &parentConfig)) != E_OK)
    return res;

  LPC_DAC_Type *reg = interface->parent.reg;

  reg->CR = (config->value & CR_OUTPUT_MASK) | CR_BIAS;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void dacDeinit(void *object)
{
  DacBase->deinit(object);
}
/*----------------------------------------------------------------------------*/
static enum result dacCallback(void *object __attribute__((unused)),
    void (*callback)(void *) __attribute__((unused)),
    void *argument __attribute__((unused)))
{
  return E_ERROR;
}
/*----------------------------------------------------------------------------*/
static enum result dacGet(void *object __attribute__((unused)),
    enum ifOption option __attribute__((unused)),
    void *data __attribute__((unused)))
{
  return E_ERROR;
}
/*----------------------------------------------------------------------------*/
static enum result dacSet(void *object __attribute__((unused)),
    enum ifOption option __attribute__((unused)),
    const void *data __attribute__((unused)))
{
  return E_ERROR;
}
/*----------------------------------------------------------------------------*/
static uint32_t dacWrite(void *object, const uint8_t *buffer, uint32_t length)
{
  struct Dac *interface = object;
  LPC_DAC_Type *reg = interface->parent.reg;

  /* Check buffer alignment */
  assert(!(length & MASK(1))); //FIXME Hardcoded

  if (!length)
    return 0;

  uint16_t value = *(uint16_t *)buffer;
  reg->CR = (value & CR_OUTPUT_MASK) | CR_BIAS;

  return 2;
}