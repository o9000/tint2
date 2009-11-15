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

#include <sys/timerfd.h>
#include <stdlib.h>
#include <unistd.h>

#include "timer.h"

GSList* timer_list = 0;

int install_timer(int value_sec, int value_nsec, int interval_sec, int interval_nsec, void (*_callback)())
{
	if ( value_sec < 0 || interval_sec < 0 )
		return -1;

	int timer_fd;
	struct itimerspec timer;
	timer.it_value = (struct timespec){ .tv_sec=value_sec, .tv_nsec=value_nsec };
	timer.it_interval = (struct timespec){ .tv_sec=interval_sec, .tv_nsec=interval_nsec };
	timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	timerfd_settime(timer_fd, 0, &timer, 0);
	struct timer* t = malloc(sizeof(struct timer));
	t->id=timer_fd;
	t->_callback = _callback;
	timer_list = g_slist_prepend(timer_list, t);
	return timer_fd;
}


void reset_timer(int id, int value_sec, int value_nsec, int interval_sec, int interval_nsec)
{
	struct itimerspec timer;
	timer.it_value = (struct timespec){ .tv_sec=value_sec, .tv_nsec=value_nsec };
	timer.it_interval = (struct timespec){ .tv_sec=interval_sec, .tv_nsec=interval_nsec };
	timerfd_settime(id, 0, &timer, 0);
}


void uninstall_timer(int id)
{
	close(id);
	GSList* timer_iter = timer_list;
	while (timer_iter) {
		struct timer* t = timer_iter->data;
		if (t->id == id) {
			timer_list = g_slist_remove(timer_list, t);
			free(t);
			return;
		}
		timer_iter = timer_iter->next;
	}
}
