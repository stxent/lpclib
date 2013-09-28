/*
 * spinlock.h
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef SPINLOCK_H_
#define SPINLOCK_H_
/*----------------------------------------------------------------------------*/
#include <stdbool.h>
#include "error.h"
/*----------------------------------------------------------------------------*/
enum
{
  SPIN_UNLOCKED = 0,
  SPIN_LOCKED
};
/*----------------------------------------------------------------------------*/
typedef volatile unsigned char spinlock_t;
/*----------------------------------------------------------------------------*/
void spinLock(spinlock_t *);
bool spinTryLock(spinlock_t *);
void spinUnlock(spinlock_t *);
/*----------------------------------------------------------------------------*/
#endif /* SPINLOCK_H_ */