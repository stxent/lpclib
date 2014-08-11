/*
 * platform/nxp/lpc11exx/clocking_defs.h
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef CLOCKING_DEFS_H_
#define CLOCKING_DEFS_H_
/*----------------------------------------------------------------------------*/
#include <bits.h>
/*------------------Main Clock Source Select register-------------------------*/
#define MAINCLKSEL_MASK                 BIT_FIELD(MASK(2), 0)
#define MAINCLKSEL_IRC                  BIT_FIELD(0, 0)
#define MAINCLKSEL_PLL_INPUT            BIT_FIELD(1, 0)
#define MAINCLKSEL_WDT                  BIT_FIELD(2, 0)
#define MAINCLKSEL_PLL_OUTPUT           BIT_FIELD(3, 0)
/*------------------Main Clock Source Update register-------------------------*/
#define MAINCLKUEN_ENA                  BIT(0)
/*------------------System Oscillator Control register------------------------*/
#define SYSOSCCTRL_BYPASS               BIT(0)
#define SYSOSCCTRL_FREQRANGE            BIT(1) /* Set for 15 - 25 MHz range */
/*------------------PLL Clock Source Select registers-------------------------*/
#define PLLCLKSEL_MASK                  BIT_FIELD(MASK(2), 0)
#define PLLCLKSEL_IRC                   BIT_FIELD(0, 0)
#define PLLCLKSEL_SYSOSC                BIT_FIELD(1, 0)
/*------------------PLL Control registers-------------------------------------*/
#define PLLCTRL_MSEL_MASK               BIT_FIELD(MASK(5), 0)
#define PLLCTRL_MSEL(value)             BIT_FIELD((value), 0)
#define PLLCTRL_PSEL_MASK               BIT_FIELD(MASK(2), 5)
#define PLLCTRL_PSEL(value)             BIT_FIELD((value), 5)
/*------------------PLL Status registers--------------------------------------*/
#define PLLSTAT_LOCK                    BIT(0)
/*----------------------------------------------------------------------------*/
#endif /* CLOCKING_DEFS_H_ */