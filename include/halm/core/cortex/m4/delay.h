/*
 * halm/core/cortex/m4/delay.h
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_CORE_CORTEX_M4_DELAY_H_
#define HALM_CORE_CORTEX_M4_DELAY_H_
/*----------------------------------------------------------------------------*/
#include <stdint.h>
/*----------------------------------------------------------------------------*/
static inline void __delay(uint32_t count)
{
  __asm__ volatile (
      "1:\n"
      "    NOP\n"
      "    SUBS.W %[count], %[count], #1\n"
      "    BCS 1b"
      : [count] "=r" (count)
      : "0" (count)
      : "r3"
  );
}
/*----------------------------------------------------------------------------*/
static inline void delayTicks(uint32_t count)
{
  __delay(count >> 2);
}
/*----------------------------------------------------------------------------*/
static inline void mdelay(uint32_t period)
{
  extern uint32_t ticksPerSecond;

  while (period)
  {
    uint32_t count = period > (1 << 12) ? (1 << 12) : period;

    period -= count;
    count = (ticksPerSecond * count) >> 2;
    __delay(count);
  }
}
/*----------------------------------------------------------------------------*/
static inline void udelay(uint32_t period)
{
  extern uint32_t ticksPerSecond;

  while (period)
  {
    uint32_t count = period > (1 << 12) ? (1 << 12) : period;

    period -= count;
    count = (ticksPerSecond * count) / 4000;
    __delay(count);
  }
}
/*----------------------------------------------------------------------------*/
#endif /* HALM_CORE_CORTEX_M4_DELAY_H_ */