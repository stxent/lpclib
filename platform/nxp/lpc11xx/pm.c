/*
 * pm.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <pm.h>
#include <platform/platform_defs.h>
#include <platform/nxp/lpc11xx/system_defs.h>
/*----------------------------------------------------------------------------*/
enum result pmPlatformChangeState(enum pmState);
/*----------------------------------------------------------------------------*/
enum result pmPlatformChangeState(enum pmState state)
{
  switch (state)
  {
    case PM_SLEEP:
      LPC_PMU->PCON = LPC_PMU->PCON & ~(PCON_DPDEN | PCON_SLEEPFLAG);
      break;

    case PM_SUSPEND:
      LPC_PMU->PCON = (LPC_PMU->PCON & ~PCON_DPDFLAG) | PCON_DPDEN;
      break;

    default:
      break;
  }

  return E_OK;
}