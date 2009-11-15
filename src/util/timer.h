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

extern GSList* timer_list;


struct timer {
	int id;
	void (*_callback)();
};


// timer functions
/** installs a timer with the first timeout 'value_sec' and then a periodic timeout with interval_sec
	* '_callback' is the callback function when the timer reaches the timeout.
	* If 'value_sec' == 0 then the timer is disabled (but not uninstalled)
	* If 'interval_sec' == 0 the timer is a single shot timer, ie. no periodic timeout occur
	* returns the 'id' of the timer, which is needed for uninstalling the timer **/
int install_timer(int value_sec, int value_nsec, int interval_sec, int interval_nsec, void (*_callback)());

/** resets a timer to the new values. If 'id' does not exist nothing happens.
	* If value_sec == 0 the timer is stopped **/
void reset_timer(int id, int value_sec, int value_nsec, int interval_sec, int interval_nsec);

/** uninstalls a timer with the given 'id'. If no timer is installed with this id nothing happens **/
void uninstall_timer(int id);

#endif // TIMER_H
