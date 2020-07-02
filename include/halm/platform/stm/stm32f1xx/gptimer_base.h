/*
 * halm/platform/stm/stm32f1xx/gptimer_base.h
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_STM_STM32F1XX_GPTIMER_BASE_H_
#define HALM_PLATFORM_STM_STM32F1XX_GPTIMER_BASE_H_
/*----------------------------------------------------------------------------*/
/** Symbolic names for advanced-control and general-purpose timers. */
enum
{
  TIM1,
  TIM2,
  TIM3,
  TIM4,
  TIM5,

  /*
   * TIM6 and TIM7 are available on high-density and XL-density
   * STM32F101 and STM32F103 devices and connectivity line devices only.
   */
  TIM6,
  TIM7,

  /* TIM8 available on high-density and XL-density STM32F103 devices only */
  TIM8,

  /* TIM9 through TIM14 are available on XL-density devices only */
  TIM9,
  TIM10,
  TIM11,
  TIM12,
  TIM13,
  TIM14
} __attribute__((packed));
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_STM_STM32F1XX_GPTIMER_BASE_H_ */