/*
 * startup.c
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <bits.h>
#include <platform/nxp/lpc17xx/system.h>
#include <platform/nxp/lpc17xx/system_defs.h>
/*----------------------------------------------------------------------------*/
void platformStartup(void);
/*----------------------------------------------------------------------------*/
void platformStartup(void)
{
  static const enum sysPowerDevice peripheralsToDisable[] = {
      PWR_TIM0,
      PWR_TIM1,
      PWR_UART0,
      PWR_UART1,
      PWR_PWM1,
      PWR_I2C0,
      PWR_SPI,
      PWR_SSP1,
      PWR_I2C1,
      PWR_SSP0,
      PWR_I2C2
  };

  /*
   * PWR_RTC is left untouched. PWR_GPIO is enabled by default.
   * Other peripherals are disabled by default.
   */

  for (uint8_t index = 0; index < ARRAY_SIZE(peripheralsToDisable); ++index)
    sysPowerDisable(peripheralsToDisable[index]);
}