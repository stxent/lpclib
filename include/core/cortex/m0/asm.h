/*
 * core/cortex/m0/asm.h
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef ASM_H_
#define ASM_H_
/*----------------------------------------------------------------------------*/
#include <stdint.h>
/*----------------------------------------------------------------------------*/
static inline void __dmb(void)
{
  __asm__ volatile ("DMB");
}
/*----------------------------------------------------------------------------*/
static inline void __irqDisable(void)
{
  __asm__ volatile ("CPSID i");
}
/*----------------------------------------------------------------------------*/
static inline void __irqEnable(void)
{
  __asm__ volatile ("CPSIE i");
}
/*----------------------------------------------------------------------------*/
static inline uint32_t __rev(uint32_t value)
{
  uint32_t result;

  __asm__ volatile (
      "REV %[result], %[value]"
      : [result] "=r" (result)
      : [value] "r" (value)
  );
  return result;
}
/*----------------------------------------------------------------------------*/
static inline uint16_t __rev16(uint16_t value)
{
  uint16_t result;

  __asm__ volatile (
      "REV16 %[result], %[value]"
      : [result] "=r" (result)
      : [value] "r" (value)
  );
  return result;
}
/*----------------------------------------------------------------------------*/
static inline void __wfi(void)
{
  __asm__ volatile ("WFI");
}
/*----------------------------------------------------------------------------*/
#endif /* ASM_H_ */