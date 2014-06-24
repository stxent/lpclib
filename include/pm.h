/*
 * pm.h
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

/**
 * @file
 * Power Management interface for embedded systems.
 */

#ifndef PM_H_
#define PM_H_
/*----------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
/*----------------------------------------------------------------------------*/
enum pmState
{
  PM_ACTIVE = 0,
  PM_SLEEP,
  PM_SUSPEND,
  PM_POWERDOWN
};
/*----------------------------------------------------------------------------*/
typedef enum result (*PmCallback)(void *, enum pmState);
/*----------------------------------------------------------------------------*/
/**
 * Change the system state.
 * Two processor-specific functions should be defined externally:
 * @b pmCoreChangeState for core-dependent code and
 * @b pmPlatformChangeState for platform-dependent code.
 * When the low power mode is selected, function returns after resuming from
 * that mode to the active state.
 * @param state Next system state.
 * @return @b E_OK on success or error otherwise.
 */
enum result pmChangeState(enum pmState state);
/*----------------------------------------------------------------------------*/
/**
 * Register a callback function.
 * @param object Pointer to an object used as function argument.
 * @param callback Callback function.
 * @return @b E_OK on success.
 */
enum result pmRegister(void *object, PmCallback callback);
/*----------------------------------------------------------------------------*/
/**
 * Unregister the callback function.
 * @param object Pointer to an object to be deleted from the list.
 */
void pmUnregister(const void *object);
/*----------------------------------------------------------------------------*/
#endif /* PM_H_ */