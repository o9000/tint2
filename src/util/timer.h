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

extern GSList* timeout_list;
extern struct timeval next_timeout;


typedef struct _timeout timeout;


// timer functions
/**
  * Single shot timer (i.e. timer with interval_msec == 0) are deleted automatically as soon as they expire
  * i.e. you do not need to stop them, however it is safe to call stop_timeout for these timers.
  * Periodic timeouts are aligned to each other whenever possible, i.e. one interval_msec is an
  * integral multiple of the other.
**/

/** default global data **/
void default_timeout();

/** freed memory : stops all timeouts **/
void cleanup_timeout();

/** installs a timeout with the first timeout of 'value_msec' and then a periodic timeout with
  * 'interval_msec'. '_callback' is the callback function when the timer reaches the timeout.
  * returns a pointer to the timeout, which is needed for stopping it again
**/
timeout* add_timeout(int value_msec, int interval_msec, void (*_callback)(void*), void* arg);

/** changes timeout 't'. If timeout 't' does not exist, nothing happens **/
void change_timeout(timeout* t, int value_msec, int interval_msec, void (*_callback)(void*), void* arg);

/** stops the timeout 't' **/
void stop_timeout(timeout* t);

/** update_next_timeout updates next_timeout to the value, when the next installed timeout will expire **/
void update_next_timeout();

/** Callback of all expired timeouts **/
void callback_timeout_expired();

#endif // TIMER_H
