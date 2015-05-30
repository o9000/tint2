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

#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

#include "timer.h"

GSList* timeout_list;
struct timeval next_timeout;
GHashTable* multi_timeouts;


// functions and structs for multi timeouts
typedef struct {
	int current_count;
	int count_to_expiration;
} multi_timeout;

typedef struct {
	GSList* timeout_list;
	timeout* parent_timeout;
} multi_timeout_handler;

struct _timeout {
	int interval_msec;
	struct timespec timeout_expires;
	void (*_callback)(void*);
	void* arg;
	multi_timeout* multi_timeout;
	timeout **self;
};

void add_timeout_intern(int value_msec, int interval_msec, void(*_callback)(void*), void* arg, timeout* t);
gint compare_timeouts(gconstpointer t1, gconstpointer t2);
gint compare_timespecs(const struct timespec* t1, const struct timespec* t2);
int timespec_subtract(struct timespec* result, struct timespec* x, struct timespec* y);
struct timespec add_msec_to_timespec(struct timespec ts, int msec);


int align_with_existing_timeouts(timeout* t);
void create_multi_timeout(timeout* t1, timeout* t2);
void append_multi_timeout(timeout* t1, timeout* t2);
int calc_multi_timeout_interval(multi_timeout_handler* mth);
void update_multi_timeout_values(multi_timeout_handler* mth);
void callback_multi_timeout(void* mth);
void remove_from_multi_timeout(timeout* t);
void stop_multi_timeout(timeout* t);

void default_timeout()
{
	timeout_list = NULL;
	multi_timeouts = NULL;
}

void cleanup_timeout()
{
	while (timeout_list) {
		timeout* t = timeout_list->data;
		if (t->multi_timeout)
			stop_multi_timeout(t);
		if (t->self)
			*t->self = NULL;
		free(t);
		timeout_list = g_slist_remove(timeout_list, t);
	}
	if (multi_timeouts) {
		g_hash_table_destroy(multi_timeouts);
		multi_timeouts = NULL;
	}
}

/** Implementation notes for timeouts: The timeouts are kept in a GSList sorted by their
	* expiration time.
	* That means that update_next_timeout() only have to consider the first timeout in the list,
	* and callback_timeout_expired() only have to consider the timeouts as long as the expiration time
	* is in the past to the current time.
	* As time measurement we use clock_gettime(CLOCK_MONOTONIC) because this refers to a timer, which
	* reference point lies somewhere in the past and cannot be changed, but just queried.
	* If a single shot timer is installed it will be automatically deleted. I.e. the returned value
	* of add_timeout will not be valid anymore. You do not need to call stop_timeout for these timeouts,
	* however it's save to call it.
**/

timeout* add_timeout(int value_msec, int interval_msec, void (*_callback)(void*), void* arg, timeout **self)
{
	timeout* t = calloc(1, sizeof(timeout));
	t->self = self;
	add_timeout_intern(value_msec, interval_msec, _callback, arg, t);
	return t;
}


void change_timeout(timeout **t, int value_msec, int interval_msec, void(*_callback)(), void* arg)
{
	if (!((timeout_list && g_slist_find(timeout_list, *t)) ||
		  (multi_timeouts && g_hash_table_lookup(multi_timeouts, *t))))
		*t = add_timeout(value_msec, interval_msec, _callback, arg, t);
	else {
		if ((*t)->multi_timeout)
			remove_from_multi_timeout(*t);
		else
			timeout_list = g_slist_remove(timeout_list, *t);
		add_timeout_intern(value_msec, interval_msec, _callback, arg, *t);
	}
}


void update_next_timeout()
{
	if (timeout_list) {
		timeout* t = timeout_list->data;
		struct timespec cur_time;
		struct timespec next_timeout2 = { .tv_sec=next_timeout.tv_sec, .tv_nsec=next_timeout.tv_usec*1000 };
		clock_gettime(CLOCK_MONOTONIC, &cur_time);
		if (timespec_subtract(&next_timeout2, &t->timeout_expires, &cur_time)) {
			next_timeout.tv_sec = 0;
			next_timeout.tv_usec = 0;
		}
		else {
			next_timeout.tv_sec = next_timeout2.tv_sec;
			next_timeout.tv_usec = next_timeout2.tv_nsec/1000;
		}
	}
	else
		next_timeout.tv_sec = -1;
}


void callback_timeout_expired()
{
	struct timespec cur_time;
	timeout* t;
	while (timeout_list) {
		clock_gettime(CLOCK_MONOTONIC, &cur_time);
		t = timeout_list->data;
		if (compare_timespecs(&t->timeout_expires, &cur_time) <= 0) {
			// it's time for the callback function
			t->_callback(t->arg);
			// If _callback() calls stop_timeout(t) the timer 't' was freed and is not in the timeout_list
			if (g_slist_find(timeout_list, t)) {
				// Timer still exists
				timeout_list = g_slist_remove(timeout_list, t);
				if (t->interval_msec > 0) {
					add_timeout_intern(t->interval_msec, t->interval_msec, t->_callback, t->arg, t);
				} else {
					// Destroy single-shot timer
					if (t->self)
						*t->self = NULL;
					free(t);
				}
			}
		} else {
			return;
		}
	}
}


void stop_timeout(timeout* t)
{
	if (!t)
		return;
	// if not in the list, it was deleted in callback_timeout_expired
	if ((timeout_list && g_slist_find(timeout_list, t)) ||
		(multi_timeouts && g_hash_table_lookup(multi_timeouts, t))) {
		if (multi_timeouts && t->multi_timeout)
			remove_from_multi_timeout(t);
		if (timeout_list)
			timeout_list = g_slist_remove(timeout_list, t);
		if (t->self)
			*t->self = NULL;
		free(t);
	}
}


void add_timeout_intern(int value_msec, int interval_msec, void(*_callback)(), void* arg, timeout *t)
{
	t->interval_msec = interval_msec;
	t->_callback = _callback;
	t->arg = arg;
	struct timespec cur_time;
	clock_gettime(CLOCK_MONOTONIC, &cur_time);
	t->timeout_expires = add_msec_to_timespec(cur_time, value_msec);

	int can_align = 0;
	if (interval_msec > 0 && !t->multi_timeout)
		can_align = align_with_existing_timeouts(t);
	if (!can_align)
		timeout_list = g_slist_insert_sorted(timeout_list, t, compare_timeouts);
}


gint compare_timeouts(gconstpointer t1, gconstpointer t2)
{
	return compare_timespecs(&((timeout*)t1)->timeout_expires,
							 &((timeout*)t2)->timeout_expires);
}


gint compare_timespecs(const struct timespec* t1, const struct timespec* t2)
{
	if (t1->tv_sec < t2->tv_sec)
		return -1;
	else if (t1->tv_sec == t2->tv_sec) {
		if (t1->tv_nsec < t2->tv_nsec)
			return -1;
		else if (t1->tv_nsec == t2->tv_nsec)
			return 0;
		else
			return 1;
	}
	else
		return 1;
}

int timespec_subtract(struct timespec* result, struct timespec* x, struct timespec* y)
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_nsec < y->tv_nsec) {
		int nsec = (y->tv_nsec - x->tv_nsec) / 1000000000 + 1;
		y->tv_nsec -= 1000000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_nsec - y->tv_nsec > 1000000000) {
		int nsec = (x->tv_nsec - y->tv_nsec) / 1000000000;
		y->tv_nsec += 1000000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait. tv_nsec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_nsec = x->tv_nsec - y->tv_nsec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}


struct timespec add_msec_to_timespec(struct timespec ts, int msec)
{
	ts.tv_sec += msec / 1000;
	ts.tv_nsec += (msec % 1000)*1000000;
	if (ts.tv_nsec >= 1000000000) {  // 10^9
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000;
	}
	return ts;
}


int align_with_existing_timeouts(timeout *t)
{
	GSList* it = timeout_list;
	while (it) {
		timeout* t2 = it->data;
		if (t2->interval_msec > 0) {
			if (t->interval_msec % t2->interval_msec == 0 || t2->interval_msec % t->interval_msec == 0) {
				if (multi_timeouts == 0)
					multi_timeouts = g_hash_table_new(0, 0);
				if (!t->multi_timeout && !t2->multi_timeout) {
					// both timeouts can be aligned, but there is no multi timeout for them
					create_multi_timeout(t, t2);
				} else {
					// there is already a multi timeout, so we append the new timeout to the multi timeout
					append_multi_timeout(t, t2);
				}
				return 1;
			}
		}
		it = it->next;
	}
	return 0;
}


int calc_multi_timeout_interval(multi_timeout_handler* mth)
{
	GSList* it = mth->timeout_list;
	timeout* t = it->data;
	int min_interval = t->interval_msec;
	it = it->next;
	while (it) {
		t = it->data;
		if (t->interval_msec < min_interval)
			min_interval = t->interval_msec;
		it = it->next;
	}
	return min_interval;
}


void create_multi_timeout(timeout* t1, timeout* t2)
{
	multi_timeout* mt1 = calloc(1, sizeof(multi_timeout));
	multi_timeout* mt2 = calloc(1, sizeof(multi_timeout));
	multi_timeout_handler* mth = calloc(1, sizeof(multi_timeout_handler));
	timeout* real_timeout = calloc(1, sizeof(timeout));

	mth->timeout_list = 0;
	mth->timeout_list = g_slist_prepend(mth->timeout_list, t1);
	mth->timeout_list = g_slist_prepend(mth->timeout_list, t2);
	mth->parent_timeout = real_timeout;

	g_hash_table_insert(multi_timeouts, t1, mth);
	g_hash_table_insert(multi_timeouts, t2, mth);
	g_hash_table_insert(multi_timeouts, real_timeout, mth);

	t1->multi_timeout = mt1;
	t2->multi_timeout = mt2;
	// set real_timeout->multi_timeout to something, such that we see in add_timeout_intern that
	// it is already a multi_timeout (we never use it, except of checking for 0 ptr)
	real_timeout->multi_timeout = (void*)real_timeout;

	timeout_list = g_slist_remove(timeout_list, t1);
	timeout_list = g_slist_remove(timeout_list, t2);

	update_multi_timeout_values(mth);
}


void append_multi_timeout(timeout* t1, timeout* t2)
{
	if (t2->multi_timeout) {
		// swap t1 and t2 such that t1 is the multi timeout
		timeout* tmp = t2;
		t2 = t1;
		t1 = tmp;
	}

	multi_timeout* mt = calloc(1, sizeof(multi_timeout));
	multi_timeout_handler* mth = g_hash_table_lookup(multi_timeouts, t1);

	mth->timeout_list = g_slist_prepend(mth->timeout_list, t2);
	g_hash_table_insert(multi_timeouts, t2, mth);

	t2->multi_timeout = mt;

	update_multi_timeout_values(mth);
}


void update_multi_timeout_values(multi_timeout_handler* mth)
{
	int interval = calc_multi_timeout_interval(mth);
	int next_timeout_msec = interval;

	struct timespec cur_time;
	clock_gettime(CLOCK_MONOTONIC, &cur_time);

	GSList* it = mth->timeout_list;
	struct timespec diff_time;
	while (it) {
		timeout* t = it->data;
		t->multi_timeout->count_to_expiration = t->interval_msec / interval;
		timespec_subtract(&diff_time, &t->timeout_expires, &cur_time);
		int msec_to_expiration = diff_time.tv_sec*1000 + diff_time.tv_nsec/1000000;
		int count_left = msec_to_expiration / interval + (msec_to_expiration%interval != 0);
		t->multi_timeout->current_count = t->multi_timeout->count_to_expiration - count_left;
		if (msec_to_expiration < next_timeout_msec)
			next_timeout_msec = msec_to_expiration;
		it = it->next;
	}

	mth->parent_timeout->interval_msec = interval;
	timeout_list = g_slist_remove(timeout_list, mth->parent_timeout);
	add_timeout_intern(next_timeout_msec, interval, callback_multi_timeout, mth, mth->parent_timeout);
}


void callback_multi_timeout(void* arg)
{
	multi_timeout_handler* mth = arg;
	struct timespec cur_time;
	clock_gettime(CLOCK_MONOTONIC, &cur_time);
	GSList* it = mth->timeout_list;
	while (it) {
		timeout* t = it->data;
		if (++t->multi_timeout->current_count >= t->multi_timeout->count_to_expiration) {
			t->_callback(t->arg);
			t->multi_timeout->current_count = 0;
			t->timeout_expires = add_msec_to_timespec(cur_time, t->interval_msec);
		}
		it = it->next;
	}
}


void remove_from_multi_timeout(timeout* t)
{
	multi_timeout_handler* mth = g_hash_table_lookup(multi_timeouts, t);
	g_hash_table_remove(multi_timeouts, t);

	mth->timeout_list = g_slist_remove(mth->timeout_list, t);
	free(t->multi_timeout);
	t->multi_timeout = 0;

	if (g_slist_length(mth->timeout_list) == 1) {
		timeout* last_timeout = mth->timeout_list->data;
		mth->timeout_list = g_slist_remove(mth->timeout_list, last_timeout);
		free(last_timeout->multi_timeout);
		last_timeout->multi_timeout = 0;
		g_hash_table_remove(multi_timeouts, last_timeout);
		g_hash_table_remove(multi_timeouts, mth->parent_timeout);
		mth->parent_timeout->multi_timeout = 0;
		stop_timeout(mth->parent_timeout);
		free(mth);

		struct timespec cur_time, diff_time;
		clock_gettime(CLOCK_MONOTONIC, &cur_time);
		timespec_subtract(&diff_time, &t->timeout_expires, &cur_time);
		int msec_to_expiration = diff_time.tv_sec*1000 + diff_time.tv_nsec/1000000;
		add_timeout_intern(msec_to_expiration, last_timeout->interval_msec, last_timeout->_callback, last_timeout->arg, last_timeout);
	}
	else
		update_multi_timeout_values(mth);
}


void stop_multi_timeout(timeout* t)
{
	multi_timeout_handler* mth = g_hash_table_lookup(multi_timeouts, t);
	g_hash_table_remove(multi_timeouts, mth->parent_timeout);
	while (mth->timeout_list) {
		timeout* t1 = mth->timeout_list->data;
		mth->timeout_list = g_slist_remove(mth->timeout_list, t1);
		g_hash_table_remove(multi_timeouts, t1);
		free(t1->multi_timeout);
		free(t1);
	}
	free(mth);
}
