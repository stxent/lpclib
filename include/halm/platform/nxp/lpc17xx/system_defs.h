/*
 * halm/platform/nxp/lpc17xx/system_defs.h
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_LPC17XX_SYSTEM_DEFS_H_
#define HALM_PLATFORM_NXP_LPC17XX_SYSTEM_DEFS_H_
/*----------------------------------------------------------------------------*/
#include <xcore/bits.h>
/*------------------Flash Accelerator Configuration register------------------*/
#define FLASHCFG_FLASHTIM_MASK          BIT_FIELD(MASK(4), 12)
#define FLASHCFG_FLASHTIM(value)        BIT_FIELD((value), 12)
/*------------------Power Mode Control register-------------------------------*/
#define PCON_PM_SLEEP                   0
#define PCON_PM_POWERDOWN               1
#define PCON_PM_DEEP_POWERDOWN          3
#define PCON_PM_MASK                    BIT_FIELD(MASK(4), 0)
#define PCON_PM(value)                  BIT_FIELD((value), 0)
#define PCON_SMFLAG                     BIT(8)
#define PCON_DSFLAG                     BIT(9)
#define PCON_PDFLAG                     BIT(10)
#define PCON_DPDFLAG                    BIT(11)
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NXP_LPC17XX_SYSTEM_DEFS_H_ */