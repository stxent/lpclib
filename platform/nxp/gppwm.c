/*
 * gppwm.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <platform/nxp/gppwm.h>
#include <platform/nxp/gppwm_defs.h>
/*----------------------------------------------------------------------------*/
#define UNPACK_FUNCTION(value)  ((value) & 0x0F)
/* Unpack match output */
#define UNPACK_CHANNEL(value)   (((value) >> 4) & 0x0F)
/*----------------------------------------------------------------------------*/
static inline uint32_t *calcMatchChannel(LPC_PWM_Type *, uint8_t);
/*----------------------------------------------------------------------------*/
static enum result unitInit(void *, const void *);
static void unitDeinit(void *);
/*----------------------------------------------------------------------------*/
static uint32_t channelGetResolution(void *);
static void channelSetEnabled(void *, bool);
/*----------------------------------------------------------------------------*/
static enum result singleEdgeInit(void *, const void *);
static void singleEdgeDeinit(void *);
static void singleEdgeSetDuration(void *, uint32_t);
static void singleEdgeSetEdges(void *, uint32_t, uint32_t);
/*----------------------------------------------------------------------------*/
static enum result doubleEdgeInit(void *, const void *);
static void doubleEdgeDeinit(void *);
static void doubleEdgeSetDuration(void *, uint32_t);
static void doubleEdgeSetEdges(void *, uint32_t, uint32_t);
/*----------------------------------------------------------------------------*/
static const struct EntityClass unitTable = {
    .size = sizeof(struct GpPwmUnit),
    .init = unitInit,
    .deinit = unitDeinit
};
/*----------------------------------------------------------------------------*/
static const struct PwmClass singleEdgeTable = {
    .size = sizeof(struct GpPwm),
    .init = singleEdgeInit,
    .deinit = singleEdgeDeinit,

    .getResolution = channelGetResolution,
    .setDuration = singleEdgeSetDuration,
    .setEdges = singleEdgeSetEdges,
    .setEnabled = channelSetEnabled
};
/*----------------------------------------------------------------------------*/
static const struct PwmClass doubleEdgeTable = {
    .size = sizeof(struct GpPwmDoubleEdge),
    .init = doubleEdgeInit,
    .deinit = doubleEdgeDeinit,

    .getResolution = channelGetResolution,
    .setDuration = doubleEdgeSetDuration,
    .setEdges = doubleEdgeSetEdges,
    .setEnabled = channelSetEnabled
};
/*----------------------------------------------------------------------------*/
extern const struct GpioDescriptor gpPwmPins[];
const struct EntityClass *GpPwmUnit = &unitTable;
const struct PwmClass *GpPwm = &singleEdgeTable;
const struct PwmClass *GpPwmDoubleEdge = &doubleEdgeTable;
/*----------------------------------------------------------------------------*/
static inline uint32_t *calcMatchChannel(LPC_PWM_Type *device, uint8_t channel)
{
  if (!channel || channel > 6)
    return 0;

  if (channel > 3)
    return (uint32_t *)&device->MR4 + (channel - 4);
  else
    return (uint32_t *)&device->MR1 + (channel - 1);
}
/*----------------------------------------------------------------------------*/
static enum result unitInit(void *object, const void *configPtr)
{
  const struct GpPwmUnitConfig * const config = configPtr;
  const struct GpPwmUnitBaseConfig parentConfig = {
      .channel = config->channel
  };
  struct GpPwmUnit *unit = object;
  enum result res;

  const uint32_t clockFrequency = gpPwmGetClock(object);
  const uint32_t timerFrequency = config->frequency * config->resolution;
  assert(timerFrequency && timerFrequency < clockFrequency);

  /* Call base class constructor */
  if ((res = GpPwmUnitBase->init(object, &parentConfig)) != E_OK)
    return res;

  unit->resolution = config->resolution;
  unit->matches = 0;

  LPC_PWM_Type *reg = unit->parent.reg;

  reg->MCR = 0;
  reg->PC = reg->TC = 0;
  reg->CCR = 0;
  reg->CTCR = 0;
  reg->PCR = 0;
  reg->IR = IR_MASK;

  /* Configure prescaler */
  reg->PR = clockFrequency / timerFrequency - 1;
  reg->MR0 = config->resolution;
  reg->MCR = MCR_RESET(0);

  /* Enable timer */
  reg->TCR = TCR_CEN | TCR_PWM_ENABLE;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void unitDeinit(void *object)
{
  struct GpPwmUnit *unit = object;

  ((LPC_PWM_Type *)unit->parent.reg)->TCR &= ~(TCR_CEN | TCR_PWM_ENABLE);
  GpPwmUnitBase->deinit(object);
}
/*----------------------------------------------------------------------------*/
static uint32_t channelGetResolution(void *object)
{
  return ((struct GpPwm *)object)->unit->resolution;
}
/*----------------------------------------------------------------------------*/
static void channelSetEnabled(void *object, bool state)
{
  struct GpPwm *pwm = object;
  LPC_PWM_Type *reg = pwm->unit->parent.reg;

  if (!state)
    reg->PCR &= ~PCR_OUTPUT_ENABLED(pwm->channel);
  else
    reg->PCR |= PCR_OUTPUT_ENABLED(pwm->channel);
}
/*----------------------------------------------------------------------------*/
static enum result singleEdgeInit(void *object, const void *configPtr)
{
  const struct GpPwmConfig * const config = configPtr;
  const struct GpioDescriptor *pinDescriptor;
  struct GpPwm *pwm = object;

  pinDescriptor = gpioFind(gpPwmPins, config->pin,
      config->parent->parent.channel);
  if (!pinDescriptor)
    return E_VALUE;

  pwm->channel = UNPACK_CHANNEL(pinDescriptor->value);
  /* Check if channel is free */
  if (config->parent->matches & pwm->channel)
    return E_BUSY;

  pwm->channel = UNPACK_CHANNEL(pinDescriptor->value);
  pwm->unit = config->parent;
  pwm->unit->matches |= 1 << pwm->channel;

  /* Initialize match output pin */
  struct Gpio pin = gpioInit(config->pin);
  gpioOutput(pin, 0);
  gpioSetFunction(pin, UNPACK_FUNCTION(pinDescriptor->value));

  LPC_PWM_Type *reg = pwm->unit->parent.reg;

  /* Calculate pointer to match register for fast access */
  pwm->value = calcMatchChannel(reg, pwm->channel);
  /* Call function directly because of unfinished object construction */
  singleEdgeSetDuration(pwm, config->duration);
  /* Enable PWM channel */
  reg->PCR |= PCR_OUTPUT_ENABLED(pwm->channel);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void singleEdgeDeinit(void *object)
{
  struct GpPwm *pwm = object;
  LPC_PWM_Type *reg = pwm->unit->parent.reg;

  reg->PCR &= ~PCR_OUTPUT_ENABLED(pwm->channel);
}
/*----------------------------------------------------------------------------*/
static void singleEdgeSetDuration(void *object, uint32_t duration)
{
  struct GpPwm *pwm = object;
  LPC_PWM_Type *reg = pwm->unit->parent.reg;
  uint32_t value;

  if (duration >= pwm->unit->resolution)
  {
    /*
     * If match register is set to a value greater than resolution, than
     * output stays high during all cycle.
     */
    value = pwm->unit->resolution + 1;
  }
  else
    value = duration;

  *pwm->value = value;
  reg->LER |= LER_ENABLE_LATCH(pwm->channel);
}
/*----------------------------------------------------------------------------*/
static void singleEdgeSetEdges(void *object,
    uint32_t leading __attribute__((unused)), uint32_t trailing)
{
  struct GpPwm *pwm = object;
  LPC_PWM_Type *reg = pwm->unit->parent.reg;
  uint32_t value;

  assert(!leading); /* Leading edge time is constant in single edge mode */

  if (trailing >= pwm->unit->resolution)
    value = pwm->unit->resolution + 1;
  else
    value = trailing;

  *pwm->value = value;
  reg->LER |= LER_ENABLE_LATCH(pwm->channel);
}
/*----------------------------------------------------------------------------*/
static enum result doubleEdgeInit(void *object, const void *configPtr)
{
  const struct GpPwmDoubleEdgeConfig * const config = configPtr;
  const struct GpioDescriptor *pinDescriptor;
  struct GpPwmDoubleEdge *pwm = object;

  pinDescriptor = gpioFind(gpPwmPins, config->pin,
      config->parent->parent.channel);
  if (!pinDescriptor)
    return E_VALUE;

//  pwm->channel = UNPACK_CHANNEL(pinDescriptor->value);
//  /* Check if channel is free */
//  if (config->parent->matches & pwm->channel)
//    return E_BUSY;
//
//  pwm->channel = UNPACK_CHANNEL(pinDescriptor->value);
//  pwm->unit = config->parent;
//  pwm->unit->matches |= 1 << pwm->channel;
//
//  /* Initialize match output pin */
//  struct Gpio pin = gpioInit(config->pin);
//  gpioOutput(pin, 0);
//  gpioSetFunction(pin, UNPACK_FUNCTION(pinDescriptor->value));
//
//  LPC_PWM_Type *reg = pwm->unit->parent.reg;
//
//  /* Calculate pointer to match register for fast access */
//  pwm->value = calcMatchChannel(reg, pwm->channel);
//  /* Call function directly because of unfinished object construction */
//  singleEdgeSetDuration(pwm, config->duration);
//  /* Enable PWM channel */
//  reg->PCR |= PCR_OUTPUT_ENABLED(pwm->channel);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void doubleEdgeDeinit(void *object)
{
  struct GpPwm *pwm = object;
  LPC_PWM_Type *reg = pwm->unit->parent.reg;

  //FIXME
//  reg->PCR &= ~PCR_OUTPUT_ENABLED(pwm->channel);
}
/*----------------------------------------------------------------------------*/
static void doubleEdgeSetDuration(void *object, uint32_t duration)
{
  struct GpPwm *pwm = object;

  //TODO
}
/*----------------------------------------------------------------------------*/
static void doubleEdgeSetEdges(void *object, uint32_t leading,
    uint32_t trailing)
{
  struct GpPwm *pwm = object;

  //TODO
}
/*----------------------------------------------------------------------------*/
/**
 * Create single edge PWM channel.
 * @param unit Pointer to GpPwmUnit object.
 * @param pin pin used as output for pulse width modulated signal.
 * @param duration Initial duration in timer ticks.
 * @return Pointer to new Pwm object on success or zero on error.
 */
void *gpPwmCreate(void *unit, gpio_t pin, uint32_t duration)
{
  const struct GpPwmConfig channelConfig = {
      .parent = unit,
      .pin = pin,
      .duration = duration
  };

  return init(GpPwm, &channelConfig);
}
/*----------------------------------------------------------------------------*/
/**
 * Create double edge PWM channel.
 * @param unit Pointer to GpPwmUnit object.
 * @param pin pin used as output for pulse width modulated signal.
 * @param duration Initial duration in timer ticks.
 * @return Pointer to new Pwm object on success or zero on error.
 */
void *gpPwmCreateDoubleEdge(void *unit, gpio_t pin, uint32_t leading,
    uint32_t trailing)
{
  const struct GpPwmDoubleEdgeConfig channelConfig = {
      .parent = unit,
      .pin = pin,
      .leading = leading,
      .trailing = trailing
  };

  return init(GpPwmDoubleEdge, &channelConfig);
}