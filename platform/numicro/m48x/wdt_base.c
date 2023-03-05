/*
 * wdt_base.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the MIT License
 */

#include <halm/platform/numicro/clocking.h>
#include <halm/platform/numicro/system.h>
#include <halm/platform/numicro/wdt_base.h>
#include <halm/platform/numicro/wdt_defs.h>
/*----------------------------------------------------------------------------*/
static bool setInstance(struct WdtBase *);

static enum Result wdtInit(void *, const void *);
/*----------------------------------------------------------------------------*/
const struct EntityClass * const WdtBase = &(const struct EntityClass){
    .size = 0, /* Abstract class */
    .init = wdtInit,
    .deinit = 0 /* Default destructor */
};
/*----------------------------------------------------------------------------*/
static struct WdtBase *instance = 0;
/*----------------------------------------------------------------------------*/
static bool setInstance(struct WdtBase *object)
{
  if (!instance)
  {
    instance = object;
    return true;
  }
  else
    return false;
}
/*----------------------------------------------------------------------------*/
void WDT_ISR(void)
{
  instance->handler(instance);
}
/*----------------------------------------------------------------------------*/
uint32_t wdtGetClock(const struct WdtBase *timer __attribute__((unused)))
{
  return clockFrequency(WdtClock);
}
/*----------------------------------------------------------------------------*/
static enum Result wdtInit(void *object, const void *configBase
    __attribute__((unused)))
{
  struct WdtBase * const timer = object;

  if (setInstance(timer))
  {
    /* Enable clock to peripheral */
    sysClockEnable(CLK_WDT);

    timer->handler = 0;
    timer->irq = WDT_IRQ;
    return E_OK;
  }
  else
    return E_BUSY;
}