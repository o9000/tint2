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
extern struct timespec next_timeout;


struct timeout {
	int interval_msec;
	struct timespec timeout_expires;
	void (*_callback)();
};


// timer functions
/** installs a timeout with the first timeout of 'value_msec' and then a periodic timeout with
	* 'interval_msec'. '_callback' is the callback function when the timer reaches the timeout.
	* returns a pointer to the timeout, which is needed for stopping it again **/
const struct timeout* add_timeout(int value_msec, int interval_msec, void (*_callback)());

void change_timeout(const struct timeout* t, int value_msec, int interval_msec, void (*_callback)());

/** stops the timeout 't' **/
void stop_timeout(const struct timeout* t);

/** stops all timeouts **/
void stop_all_timeouts();

void update_next_timeout();
void callback_timeout_expired();

#endif // TIMER_H
