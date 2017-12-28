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
#include <string.h>

#include "timer.h"
#include "test.h"

GSList *timeout_list;
struct timeval next_timeout;
GHashTable *multi_timeouts;

// functions and structs for multi timeouts
typedef struct {
    int current_count;
    int count_to_expiration;
} multi_timeout;

typedef struct {
    GSList *timeout_list;
    timeout *parent_timeout;
} multi_timeout_handler;

struct _timeout {
    int interval_msec;
    struct timespec timeout_expires;
    void (*_callback)(void *);
    void *arg;
    multi_timeout *multi_timeout;
    timeout **self;
    gboolean expired;
    // timer has been restarted from its own callback function
    gboolean reactivated;
};

void add_timeout_intern(int value_msec, int interval_msec, void (*_callback)(void *), void *arg, timeout *t);
gint compare_timeouts(gconstpointer t1, gconstpointer t2);
int timespec_subtract(struct timespec *result, struct timespec *x, struct timespec *y);

int align_with_existing_timeouts(timeout *t);
void create_multi_timeout(timeout *t1, timeout *t2);
void append_multi_timeout(timeout *t1, timeout *t2);
int calc_multi_timeout_interval(multi_timeout_handler *mth);
void update_multi_timeout_values(multi_timeout_handler *mth);
void callback_multi_timeout(void *mth);
void remove_from_multi_timeout(timeout *t);
void stop_multi_timeout(timeout *t);

void default_timeout()
{
    timeout_list = NULL;
    multi_timeouts = NULL;
}

void cleanup_timeout()
{
    while (timeout_list) {
        timeout *t = timeout_list->data;
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

static struct timespec mock_time = { 0, 0 };
void set_mock_time(struct timespec *tp)
{
    mock_time = *tp;
}

void set_mock_time_ms(u_int64_t ms)
{
    struct timespec t;
    t.tv_sec = ms / 1000;
    t.tv_nsec = (ms % 1000) * 1000 * 1000;
    set_mock_time(&t);
}

int gettime(struct timespec *tp)
{
    if (mock_time.tv_sec || mock_time.tv_nsec) {
        *tp = mock_time;
        return 0;
    }
    // CLOCK_BOOTTIME under Linux is the same as CLOCK_MONOTONIC under *BSD.
#ifdef CLOCK_BOOTTIME
    return clock_gettime(CLOCK_BOOTTIME, tp);
#else
    return clock_gettime(CLOCK_MONOTONIC, tp);
#endif
}

// Implementation notes for timeouts
//
// The timeouts are kept in a GSList sorted by their expiration time.
// That means that update_next_timeout() only have to consider the first timeout in the list,
// and callback_timeout_expired() only have to consider the timeouts as long as the expiration time
// is in the past to the current time.
// As time measurement we use clock_gettime(CLOCK_MONOTONIC) because this refers to a timer, which
// reference point lies somewhere in the past and cannot be changed, but just queried.
// If a single shot timer is installed it will be automatically deleted. I.e. the returned value
// of add_timeout will not be valid anymore. You do not need to call stop_timeout for these timeouts,
// however it's save to call it.

timeout *add_timeout(int value_msec, int interval_msec, void (*_callback)(void *), void *arg, timeout **self)
{
    if (self && *self && !(*self)->expired)
        return *self;
    timeout *t = calloc(1, sizeof(timeout));
    t->self = self;
    add_timeout_intern(value_msec, interval_msec, _callback, arg, t);
    return t;
}

void change_timeout(timeout **t, int value_msec, int interval_msec, void (*_callback)(), void *arg)
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

struct timeval *get_next_timeout()
{
    if (timeout_list) {
        timeout *t = timeout_list->data;
        struct timespec cur_time;
        struct timespec next_timeout2 = {.tv_sec = next_timeout.tv_sec, .tv_nsec = next_timeout.tv_usec * 1000};
        gettime(&cur_time);
        if (timespec_subtract(&next_timeout2, &t->timeout_expires, &cur_time)) {
            next_timeout.tv_sec = 0;
            next_timeout.tv_usec = 0;
        } else {
            next_timeout.tv_sec = next_timeout2.tv_sec;
            next_timeout.tv_usec = next_timeout2.tv_nsec / 1000;
        }
    } else
        next_timeout.tv_sec = -1;
    return (next_timeout.tv_sec >= 0 && next_timeout.tv_usec >= 0) ? &next_timeout : NULL;
}

void handle_expired_timers()
{
    struct timespec cur_time;
    timeout *t;
    while (timeout_list) {
        gettime(&cur_time);
        t = timeout_list->data;
        if (compare_timespecs(&t->timeout_expires, &cur_time) <= 0) {
            // it's time for the callback function
            t->expired = t->interval_msec == 0;
            t->reactivated = FALSE;
            t->_callback(t->arg);
            // If _callback() calls stop_timeout(t) the timer 't' was freed and is not in the timeout_list
            if (g_slist_find(timeout_list, t)) {
                // Timer still exists
                if (!t->reactivated) {
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
            }
        } else {
            return;
        }
    }
}

void stop_timeout(timeout *t)
{
    if (!t)
        return;
    // if not in the list, it was deleted in callback_timeout_expired
    if ((timeout_list && g_slist_find(timeout_list, t)) || (multi_timeouts && g_hash_table_lookup(multi_timeouts, t))) {
        if (multi_timeouts && t->multi_timeout)
            remove_from_multi_timeout(t);
        if (timeout_list)
            timeout_list = g_slist_remove(timeout_list, t);
        if (t->self)
            *t->self = NULL;
        free(t);
    }
}

void add_timeout_intern(int value_msec, int interval_msec, void (*_callback)(), void *arg, timeout *t)
{
    t->interval_msec = interval_msec;
    t->_callback = _callback;
    t->arg = arg;
    struct timespec cur_time;
    gettime(&cur_time);
    t->timeout_expires = add_msec_to_timespec(cur_time, value_msec);

    int can_align = 0;
    if (interval_msec > 0 && !t->multi_timeout)
        can_align = align_with_existing_timeouts(t);
    if (!can_align)
        timeout_list = g_slist_insert_sorted(timeout_list, t, compare_timeouts);
    t->reactivated = TRUE;
}

gint compare_timeouts(gconstpointer t1, gconstpointer t2)
{
    return compare_timespecs(&((const timeout *)t1)->timeout_expires, &((const timeout *)t2)->timeout_expires);
}

gint compare_timespecs(const struct timespec *t1, const struct timespec *t2)
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
    } else
        return 1;
}

int timespec_subtract(struct timespec *result, struct timespec *x, struct timespec *y)
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
    ts.tv_nsec += (msec % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) { // 10^9
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }
    return ts;
}

int align_with_existing_timeouts(timeout *t)
{
    GSList *it = timeout_list;
    while (it) {
        timeout *t2 = it->data;
        if (t2->interval_msec > 0) {
            if (t->interval_msec % t2->interval_msec == 0 || t2->interval_msec % t->interval_msec == 0) {
                if (!multi_timeouts)
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

int calc_multi_timeout_interval(multi_timeout_handler *mth)
{
    GSList *it = mth->timeout_list;
    timeout *t = it->data;
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

void create_multi_timeout(timeout *t1, timeout *t2)
{
    multi_timeout *mt1 = calloc(1, sizeof(multi_timeout));
    multi_timeout *mt2 = calloc(1, sizeof(multi_timeout));
    multi_timeout_handler *mth = calloc(1, sizeof(multi_timeout_handler));
    timeout *real_timeout = calloc(1, sizeof(timeout));

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
    real_timeout->multi_timeout = (void *)real_timeout;

    timeout_list = g_slist_remove(timeout_list, t1);
    timeout_list = g_slist_remove(timeout_list, t2);

    update_multi_timeout_values(mth);
}

void append_multi_timeout(timeout *t1, timeout *t2)
{
    if (t2->multi_timeout) {
        // swap t1 and t2 such that t1 is the multi timeout
        timeout *tmp = t2;
        t2 = t1;
        t1 = tmp;
    }

    multi_timeout *mt = calloc(1, sizeof(multi_timeout));
    multi_timeout_handler *mth = g_hash_table_lookup(multi_timeouts, t1);

    mth->timeout_list = g_slist_prepend(mth->timeout_list, t2);
    g_hash_table_insert(multi_timeouts, t2, mth);

    t2->multi_timeout = mt;

    update_multi_timeout_values(mth);
}

void update_multi_timeout_values(multi_timeout_handler *mth)
{
    int interval = calc_multi_timeout_interval(mth);
    int next_timeout_msec = interval;

    struct timespec cur_time;
    gettime(&cur_time);

    GSList *it = mth->timeout_list;
    struct timespec diff_time;
    while (it) {
        timeout *t = it->data;
        t->multi_timeout->count_to_expiration = t->interval_msec / interval;
        timespec_subtract(&diff_time, &t->timeout_expires, &cur_time);
        int msec_to_expiration = diff_time.tv_sec * 1000 + diff_time.tv_nsec / 1000000;
        int count_left = msec_to_expiration / interval + (msec_to_expiration % interval != 0);
        t->multi_timeout->current_count = t->multi_timeout->count_to_expiration - count_left;
        if (msec_to_expiration < next_timeout_msec)
            next_timeout_msec = msec_to_expiration;
        it = it->next;
    }

    mth->parent_timeout->interval_msec = interval;
    timeout_list = g_slist_remove(timeout_list, mth->parent_timeout);
    add_timeout_intern(next_timeout_msec, interval, callback_multi_timeout, mth, mth->parent_timeout);
}

void callback_multi_timeout(void *arg)
{
    multi_timeout_handler *mth = arg;
    struct timespec cur_time;
    gettime(&cur_time);
    GSList *it = mth->timeout_list;
    while (it) {
        GSList *next = it->next;
        timeout *t = it->data;
        if (t->multi_timeout && ++t->multi_timeout->current_count >= t->multi_timeout->count_to_expiration) {
            t->_callback(t->arg);
            if (multi_timeouts && g_hash_table_lookup(multi_timeouts, t)) {
                // Timer still exists
                t->multi_timeout->current_count = 0;
                t->timeout_expires = add_msec_to_timespec(cur_time, t->interval_msec);
            } else {
                return;
            }
        }
        it = next;
    }
}

void remove_from_multi_timeout(timeout *t)
{
    multi_timeout_handler *mth = g_hash_table_lookup(multi_timeouts, t);
    g_hash_table_remove(multi_timeouts, t);

    mth->timeout_list = g_slist_remove(mth->timeout_list, t);
    free(t->multi_timeout);
    t->multi_timeout = 0;

    if (g_slist_length(mth->timeout_list) == 1) {
        timeout *last_timeout = mth->timeout_list->data;
        mth->timeout_list = g_slist_remove(mth->timeout_list, last_timeout);
        free(last_timeout->multi_timeout);
        last_timeout->multi_timeout = 0;
        g_hash_table_remove(multi_timeouts, last_timeout);
        g_hash_table_remove(multi_timeouts, mth->parent_timeout);
        mth->parent_timeout->multi_timeout = 0;
        stop_timeout(mth->parent_timeout);
        free(mth);

        struct timespec cur_time, diff_time;
        gettime(&cur_time);
        timespec_subtract(&diff_time, &t->timeout_expires, &cur_time);
        int msec_to_expiration = diff_time.tv_sec * 1000 + diff_time.tv_nsec / 1000000;
        add_timeout_intern(msec_to_expiration,
                           last_timeout->interval_msec,
                           last_timeout->_callback,
                           last_timeout->arg,
                           last_timeout);
    } else
        update_multi_timeout_values(mth);
}

void stop_multi_timeout(timeout *t)
{
    multi_timeout_handler *mth = g_hash_table_lookup(multi_timeouts, t);
    g_hash_table_remove(multi_timeouts, mth->parent_timeout);
    while (mth->timeout_list) {
        timeout *t1 = mth->timeout_list->data;
        mth->timeout_list = g_slist_remove(mth->timeout_list, t1);
        g_hash_table_remove(multi_timeouts, t1);
        free(t1->multi_timeout);
        free(t1);
    }
    free(mth);
}

double profiling_get_time_old_time = 0;

double get_time()
{
    struct timespec cur_time;
    gettime(&cur_time);
    return cur_time.tv_sec + cur_time.tv_nsec * 1.0e-9;
}

double profiling_get_time()
{
    double t = get_time();
    if (profiling_get_time_old_time == 0)
        profiling_get_time_old_time = t;
    double delta = t - profiling_get_time_old_time;
    profiling_get_time_old_time = t;
    return delta;
}

TEST(mock_time) {
    struct timespec t1 = {1000, 2};
    struct timespec t2 = {0, 0};
    struct timespec t3 = {2000, 3};
    int ret;
    set_mock_time(&t1);
    ret = gettime(&t2);
    ASSERT_EQUAL(ret, 0);
    ASSERT_EQUAL(t1.tv_sec, t2.tv_sec);
    ASSERT_EQUAL(t1.tv_nsec, t2.tv_nsec);

    set_mock_time(&t3);
    ret = gettime(&t2);
    ASSERT_EQUAL(ret, 0);
    ASSERT_EQUAL(t3.tv_sec, t2.tv_sec);
    ASSERT_EQUAL(t3.tv_nsec, t2.tv_nsec);
}

TEST(mock_time_ms) {
    struct timespec t1 = {1000, 2 * 1000 * 1000};
    struct timespec t2 = {0, 0};
    struct timespec t3 = {2000, 3 * 1000 * 1000};
    int ret;
    set_mock_time_ms(1000 * 1000 + 2);
    ret = gettime(&t2);
    ASSERT_EQUAL(ret, 0);
    ASSERT_EQUAL(t1.tv_sec, t2.tv_sec);
    ASSERT_EQUAL(t1.tv_nsec, t2.tv_nsec);

    set_mock_time_ms(2000 * 1000 + 3);
    ret = gettime(&t2);
    ASSERT_EQUAL(ret, 0);
    ASSERT_EQUAL(t3.tv_sec, t2.tv_sec);
    ASSERT_EQUAL(t3.tv_nsec, t2.tv_nsec);
}

void trigger_callback(void *arg)
{
    int *triggered = (int*) arg;
    *triggered += 1;
}

typedef struct {
    timeout *timeout;
    int triggered;
    bool stop;
    bool change;
    int change_value_ms;
    int change_interval_ms;
    bool add;
    int add_value_ms;
    int add_interval_ms;
    bool stop_other;
    bool change_other;
    int change_other_value_ms;
    int change_other_interval_ms;
    timeout *other;
} TimeoutContainer;

void container_callback(void *arg) {
    TimeoutContainer *container = (TimeoutContainer *)arg;
    container->triggered += 1;
    if (container->stop)
        stop_timeout(container->timeout);
    else if (container->change)
        change_timeout(&container->timeout, container->change_value_ms, container->change_interval_ms, container_callback, arg);
    if (container->add) {
        add_timeout(container->add_value_ms, container->add_interval_ms, container_callback, arg, NULL);
        container->add = false;
    }
    if (container->stop_other)
        stop_timeout(container->other);
    else if (container->change_other) {
        change_timeout(&container->other, container->change_other_value_ms, container->change_other_interval_ms, container_callback, arg);
        container->change_other = false;
    }
}

TEST(add_timeout_simple) {
    u_int64_t origin = 2134523;
    int triggered = 0;

    set_mock_time_ms(origin + 0);
    add_timeout(200, 0, trigger_callback, &triggered, NULL);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 190);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 500);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);
}

TEST(add_timeout_simple_two) {
    u_int64_t origin = 2134523;
    int triggered = 0;

    set_mock_time_ms(origin + 0);
    add_timeout(100, 0, trigger_callback, &triggered, NULL);
    add_timeout(200, 0, trigger_callback, &triggered, NULL);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 190);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 500);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);
}

TEST(add_timeout_simple_two_reversed) {
    u_int64_t origin = 2134523;
    int triggered = 0;

    set_mock_time_ms(origin + 0);
    add_timeout(200, 0, trigger_callback, &triggered, NULL);
    add_timeout(100, 0, trigger_callback, &triggered, NULL);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 190);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 500);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);
}

TEST(add_timeout_simple_inside_callback) {
    u_int64_t origin = 2134523;
    TimeoutContainer container;
    bzero(&container, sizeof(container));

    set_mock_time_ms(origin + 0);

    container.add = true;
    container.add_value_ms = 100;
    container.timeout = add_timeout(200, 0, container_callback, &container, NULL);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 0);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);

    set_mock_time_ms(origin + 250);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 2);

    set_mock_time_ms(origin + 500);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 2);
}

TEST(stop_timeout_simple_two) {
    u_int64_t origin = 2134523;
    int triggered = 0;

    set_mock_time_ms(origin + 0);
    timeout *t1 = add_timeout(100, 0, trigger_callback, &triggered, NULL);
    add_timeout(200, 0, trigger_callback, &triggered, NULL);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    stop_timeout(t1);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 190);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 500);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);
}

TEST(stop_timeout_simple_two_reversed) {
    u_int64_t origin = 2134523;
    int triggered = 0;

    set_mock_time_ms(origin + 0);
    add_timeout(100, 0, trigger_callback, &triggered, NULL);
    timeout *t2 = add_timeout(200, 0, trigger_callback, &triggered, NULL);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    stop_timeout(t2);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 190);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 500);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);
}

TEST(stop_timeout_simple_inside_callback) {
    u_int64_t origin = 2134523;
    TimeoutContainer container;
    bzero(&container, sizeof(container));

    set_mock_time_ms(origin + 0);

    container.stop = true;
    container.timeout = add_timeout(200, 0, container_callback, &container, NULL);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 0);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);

    set_mock_time_ms(origin + 250);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);

    set_mock_time_ms(origin + 500);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);
}

TEST(stop_timeout_simple_other_inside_callback) {
    u_int64_t origin = 2134523;
    TimeoutContainer container;
    bzero(&container, sizeof(container));
    int triggered_other = 0;

    set_mock_time_ms(origin + 0);

    container.stop_other = true;
    container.timeout = add_timeout(100, 0, container_callback, &container, NULL);
    container.other = add_timeout(200, 0, trigger_callback, &triggered_other, NULL);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 0);
    ASSERT_EQUAL(triggered_other, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);
    ASSERT_EQUAL(triggered_other, 0);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);
    ASSERT_EQUAL(triggered_other, 0);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);
    ASSERT_EQUAL(triggered_other, 0);
}

TEST(change_timeout_simple) {
    u_int64_t origin = 2134523;
    int triggered = 0;

    set_mock_time_ms(origin + 0);
    timeout *t1 = add_timeout(200, 0, trigger_callback, &triggered, NULL);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    change_timeout(&t1, 200, 0, trigger_callback, &triggered);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 250);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 500);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);
}

TEST(change_timeout_simple_two) {
    u_int64_t origin = 2134523;
    int triggered = 0;

    set_mock_time_ms(origin + 0);
    timeout *t1 = add_timeout(200, 0, trigger_callback, &triggered, NULL);
    add_timeout(250, 0, trigger_callback, &triggered, NULL);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    change_timeout(&t1, 200, 0, trigger_callback, &triggered);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 250);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 500);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);
}

TEST(change_timeout_simple_inside_callback) {
    u_int64_t origin = 2134523;
    TimeoutContainer container;
    bzero(&container, sizeof(container));

    set_mock_time_ms(origin + 0);

    container.change = true;
    container.change_value_ms = 100;
    container.timeout = add_timeout(200, 0, container_callback, &container, NULL);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 0);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);

    set_mock_time_ms(origin + 250);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 2);

    set_mock_time_ms(origin + 350);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 2);

    set_mock_time_ms(origin + 400);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 3);
}

TEST(change_timeout_simple_other_inside_callback) {
    u_int64_t origin = 2134523;
    TimeoutContainer container;
    bzero(&container, sizeof(container));
    int triggered_other = 0;

    set_mock_time_ms(origin + 0);

    container.change_other = true;
    container.change_other_value_ms = 100;
    container.timeout = add_timeout(100, 0, container_callback, &container, NULL);
    container.other = add_timeout(1000, 0, trigger_callback, &triggered_other, NULL);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 0);
    ASSERT_EQUAL(triggered_other, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);
    ASSERT_EQUAL(triggered_other, 0);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 2);
    ASSERT_EQUAL(triggered_other, 0);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 2);
    ASSERT_EQUAL(triggered_other, 0);
}

TEST(add_change_two_timeout_simple_inside_callback) {
    u_int64_t origin = 2134523;
    TimeoutContainer container;
    bzero(&container, sizeof(container));

    set_mock_time_ms(origin + 0);

    container.add = true;
    container.add_value_ms = 100;
    container.change = true;
    container.change_value_ms = 100;
    container.timeout = add_timeout(200, 0, container_callback, &container, NULL);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 0);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);

    // Now we have two running timers, one changing itself to expire after 100 ms when triggered,
    // the other firing once after 100 ms
    set_mock_time_ms(origin + 250);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 3);

    set_mock_time_ms(origin + 350);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 3);

    set_mock_time_ms(origin + 400);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 4);
}

int64_t timeval_to_ms(struct timeval *v)
{
    if (!v)
        return -1;
    return (int64_t)(v->tv_sec * 1000 + v->tv_usec / 1000);
}

TEST(get_next_timeout_simple) {
    u_int64_t origin = 2134523;
    int triggered = 0;

    set_mock_time_ms(origin + 0);
    timeout *t1 = add_timeout(200, 0, trigger_callback, &triggered, NULL);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);
    ASSERT_EQUAL(timeval_to_ms(get_next_timeout()), 200);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);
    ASSERT_EQUAL(timeval_to_ms(get_next_timeout()), 100);

    change_timeout(&t1, 200, 0, trigger_callback, &triggered);
    ASSERT_EQUAL(timeval_to_ms(get_next_timeout()), 200);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);
    ASSERT_EQUAL(timeval_to_ms(get_next_timeout()), 100);

    set_mock_time_ms(origin + 250);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);
    ASSERT_EQUAL(timeval_to_ms(get_next_timeout()), 50);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);
    ASSERT_EQUAL(timeval_to_ms(get_next_timeout()), -1);

    set_mock_time_ms(origin + 500);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);
    ASSERT_EQUAL(timeval_to_ms(get_next_timeout()), -1);
}

TEST(cleanup_timeout_simple) {
    u_int64_t origin = 2134523;
    int triggered = 0;

    set_mock_time_ms(origin + 0);
    add_timeout(100, 0, trigger_callback, &triggered, NULL);
    add_timeout(200, 0, trigger_callback, &triggered, NULL);
    add_timeout(300, 0, trigger_callback, &triggered, NULL);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 150);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    cleanup_timeout();
    ASSERT_EQUAL(timeval_to_ms(get_next_timeout()), -1);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 500);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);
}
