/**************************************************************************
*
* Copyright (C) 2009 Andreas.Fink (Andreas.Fink85@gmail.com)
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************/


#ifndef TIMER_H
#define TIMER_H

#include <glib.h>

extern struct timeval next_timeout;


typedef struct _timeout timeout;


// timer functions
/**
  * Single shot timers (i.e. timers with interval_msec == 0) are deleted automatically as soon as they expire,
  * i.e. you do not need to stop them, however it is safe to call stop_timeout for these timers.
  * You can pass the address of the variable storing the pointer to the timer as 'self' in add_timeout, in which
  * case it is used to clear the pointer if the timer is destroyed automatically. This enforces the timeout pointers
  * to be either valid or NULL.
  * Periodic timeouts are aligned to each other whenever possible, i.e. one interval_msec is an
  * integral multiple of the other.
**/

/** Initializes default global data. **/
void default_timeout();

/** Cleans up: stops all timers and frees memory. **/
void cleanup_timeout();

/** Installs a timer with the first timeout after 'value_msec' and then an optional periodic timeout every
  * 'interval_msec' (set it to 0 to prevent periodic timeouts).
  * '_callback' is the function called when the timer reaches the timeout.
  * 'arg' is the argument passed to the callback function.
  * 'self' is an optional pointer to a timeout* variable. If non-NULL, the variable is set to NULL when the timer
  * is destroyed (with stop_timeout, cleanup_timeout or when the timer expires and it is single-shot).
  * Returns a pointer to the timer, which is needed for stopping/changing it.
**/
timeout* add_timeout(int value_msec, int interval_msec, void (*_callback)(void*), void* arg, timeout **self);

/** Changes timer 't'. If it does not exist, a new timer is created, with self set to 't'. **/
void change_timeout(timeout** t, int value_msec, int interval_msec, void (*_callback)(void*), void* arg);

/** stops the timeout 't' **/
void stop_timeout(timeout* t);

/** update_next_timeout updates next_timeout to the value, when the next installed timeout will expire **/
void update_next_timeout();

/** Callback of all expired timeouts **/
void callback_timeout_expired();

#endif // TIMER_H
