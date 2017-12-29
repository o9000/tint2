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

#include "colors.h"
#include "timer.h"
#include "test.h"

bool debug_timers = false;
#define MOCK_ORIGIN 1000000

// All active timers
static GList *timers = NULL;

long long get_time_ms();

void default_timers()
{
    timers = NULL;
}

void cleanup_timers()
{
    if (debug_timers)
        fprintf(stderr, "tint2: timers: %s\n", __FUNCTION__);
    g_list_free(timers);
    timers = NULL;
}

void init_timer(Timer *timer, const char *name)
{
    if (debug_timers)
        fprintf(stderr, "tint2: timers: %s: %s, %p\n", __FUNCTION__, name, (void *)timer);
    bzero(timer, sizeof(*timer));
    strncpy(timer->name_, name, sizeof(timer->name_));
    if (!g_list_find(timers, timer)) {
        timers = g_list_append(timers, timer);
    }
}

void destroy_timer(Timer *timer)
{
    if (!g_list_find(timers, timer)) {
        fprintf(stderr, RED "tint2: Attempt to destroy nonexisting timer: %s" RESET "\n", timer->name_);
        return;
    }
    if (debug_timers)
        fprintf(stderr, "tint2: timers: %s: %s, %p\n", __FUNCTION__, timer->name_, (void *)timer);
    timers = g_list_remove(timers, timer);
}

void change_timer(Timer *timer, bool enabled, int delay_ms, int period_ms, TimerCallback *callback, void *arg)
{
    if (!g_list_find(timers, timer)) {
        fprintf(stderr, RED "tint2: Attempt to change unknown timer" RESET "\n");
        init_timer(timer, "unknown");
    }
    timer->enabled_ = enabled;
    timer->expiration_time_ms_ = get_time_ms() + delay_ms;
    timer->period_ms_ = period_ms;
    timer->callback_ = callback;
    timer->arg_ = arg;
    if (debug_timers)
        fprintf(stderr,
                "tint2: timers: %s: %s, %p: %s, expires %lld, period %d\n",
                __FUNCTION__,
                timer->name_,
                (void *)timer,
                timer->enabled_ ? "on" : "off",
                timer->expiration_time_ms_,
                timer->period_ms_);
}

void stop_timer(Timer *timer)
{
    change_timer(timer, false, 0, 0, NULL, NULL);
}

struct timeval *get_duration_to_next_timer_expiration()
{
    static struct timeval result = {0, 0};
    long long min_expiration_time = -1;
    Timer *next_timer = NULL;
    for (GList *it = timers; it; it = it->next) {
        Timer *timer = (Timer *)it->data;
        if (!timer->enabled_)
            continue;
        if (min_expiration_time < 0 || timer->expiration_time_ms_ < min_expiration_time) {
            min_expiration_time = timer->expiration_time_ms_;
            next_timer = timer;
        }
    }
    if (min_expiration_time < 0) {
        if (debug_timers)
            fprintf(stderr,
                    "tint2: timers: %s: no active timer\n",
                    __FUNCTION__);
        return NULL;
    }
    long long now = get_time_ms();
    long long duration = min_expiration_time - now;
    if (debug_timers)
        fprintf(stderr,
                "tint2: timers: %s: t=%lld, %lld to next timer: %s, %p: %s, expires %lld, period %d\n",
                __FUNCTION__,
                now,
                duration,
                next_timer->name_,
                (void *)next_timer,
                next_timer->enabled_ ? "on" : "off",
                next_timer->expiration_time_ms_,
                next_timer->period_ms_);
    result.tv_sec = duration / 1000;
    result.tv_usec = 1000 * (duration - result.tv_sec);
    return &result;
}

void handle_expired_timers()
{
    long long now = get_time_ms();
    bool expired_timers = false;
    for (GList *it = timers; it; it = it->next) {
        Timer *timer = (Timer *)it->data;
        if (timer->enabled_ && timer->callback_ && timer->expiration_time_ms_ <= now)
            expired_timers = true;
    }
    if (!expired_timers)
        return;

    // We make a copy of the timer list, as the callbacks may create new timers,
    // which we don't want to trigger in this event loop iteration,
    // to prevent infinite loops.
    GList *current_timers = g_list_copy(timers);

    // Mark all timers as not handled.
    // This is to ensure not triggering a timer more than once per event loop iteration,
    // in case it is modified from a callback.
    for (GList *it = current_timers; it; it = it->next) {
        Timer *timer = (Timer *)it->data;
        timer->handled_ = false;
    }

    bool triggered;
    do {
        triggered = false;
        for (GList *it = current_timers; it; it = it->next) {
            Timer *timer = (Timer *)it->data;
            // Check that it is still registered.
            if (!g_list_find(timers, timer))
                continue;
            if (!timer->enabled_ || timer->handled_ || !timer->callback_ || timer->expiration_time_ms_ > now)
                continue;
            timer->handled_ = true;
            if (timer->period_ms_ == 0) {
                // One shot timer, turn it off.
                timer->enabled_ = false;
            } else {
                // Periodic timer, reschedule.
                timer->expiration_time_ms_ = now + timer->period_ms_;
            }
            if (debug_timers)
                fprintf(stderr,
                        "tint2: timers: %s: t=%lld, triggering %s, %p: %s, expires %lld, period %d\n",
                        __FUNCTION__,
                        now,
                        timer->name_,
                        (void *)timer,
                        timer->enabled_ ? "on" : "off",
                        timer->expiration_time_ms_,
                        timer->period_ms_);
            timer->callback_(timer->arg_);
            // The callback may have modified timers, so start from scratch
            triggered = true;
            break;
        }
    } while (triggered);

    g_list_free(current_timers);
}

// Time helper functions

static struct timespec mock_time = {0, 0};
void set_mock_time(struct timespec *tp)
{
    mock_time = *tp;
    if (debug_timers)
        fprintf(stderr,
                "tint2: timers: %s: t=%lld\n",
                __FUNCTION__,
                get_time_ms());
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

static double profiling_get_time_old_time = 0;

double get_time()
{
    struct timespec cur_time;
    gettime(&cur_time);
    return cur_time.tv_sec + cur_time.tv_nsec * 1.0e-9;
}

long long get_time_ms()
{
    struct timespec cur_time;
    gettime(&cur_time);
    return cur_time.tv_sec * 1000LL + cur_time.tv_nsec / 1000000LL;
}

double profiling_get_time()
{
    double t = get_time();
    if (profiling_get_time_old_time <= 0)
        profiling_get_time_old_time = t;
    double delta = t - profiling_get_time_old_time;
    profiling_get_time_old_time = t;
    return delta;
}

///////////////////////////////////////////////////////////////////////////////
// Tests start here

static int64_t timeval_to_ms(struct timeval *v)
{
    if (!v)
        return -1;
    return (int64_t)(v->tv_sec * 1000 + v->tv_usec / 1000);
}

static void trigger_callback(void *arg)
{
    int *triggered = (int *)arg;
    *triggered += 1;
}

typedef struct {
    Timer *timer;
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
    Timer *other;
} TimeoutContainer;

static void container_callback(void *arg)
{
    TimeoutContainer *container = (TimeoutContainer *)arg;
    container->triggered += 1;
    if (container->stop)
        stop_timer(container->timer);
    else if (container->change) {
        change_timer(container->timer,
                     true,
                     container->change_value_ms,
                     container->change_interval_ms,
                     container_callback,
                     arg);
        if (container->change_interval_ms)
            container->change = false;
    }
    if (container->add) {
        Timer *timer = calloc(1, sizeof(Timer));
        init_timer(timer, "container_callback");
        change_timer(timer, true, container->add_value_ms, container->add_interval_ms, container_callback, arg);
        container->add = false;
    }
    if (container->stop_other)
        stop_timer(container->other);
    else if (container->change_other) {
        change_timer(container->other,
                     true,
                     container->change_other_value_ms,
                     container->change_other_interval_ms,
                     container_callback,
                     arg);
        container->change_other = false;
    }
}

TEST(mock_time)
{
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

TEST(mock_time_ms)
{
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

TEST(change_timer_simple)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer timer;
    init_timer(&timer, __FUNCTION__);

    set_mock_time_ms(origin + 0);
    change_timer(&timer, true, 200, 0, trigger_callback, &triggered);
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

TEST(change_timer_simple_two)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t2, "t2");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 100, 0, trigger_callback, &triggered);
    change_timer(&t2, true, 200, 0, trigger_callback, &triggered);
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

TEST(change_timer_simple_two_reversed)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t2, "t2");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 200, 0, trigger_callback, &triggered);
    change_timer(&t2, true, 100, 0, trigger_callback, &triggered);
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

TEST(change_timer_simple_two_overlap)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t2, "t2");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 100, 0, trigger_callback, &triggered);
    change_timer(&t2, true, 100, 0, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);
}

TEST(change_timer_simple_inside_callback)
{
    u_int64_t origin = MOCK_ORIGIN;
    TimeoutContainer container;
    bzero(&container, sizeof(container));
    Timer timer;
    init_timer(&timer, __FUNCTION__);

    set_mock_time_ms(origin + 0);

    container.add = true;
    container.add_value_ms = 100;
    container.timer = &timer;
    change_timer(container.timer, true, 200, 0, container_callback, &container);
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

TEST(change_timer_multi)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer timer;
    init_timer(&timer, __FUNCTION__);

    set_mock_time_ms(origin + 0);
    change_timer(&timer, true, 200, 100, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 150);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 250);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 400);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 3);

    set_mock_time_ms(origin + 500);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 4);
}

TEST(change_timer_multi_two)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t2, "t2");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 100, 100, trigger_callback, &triggered);
    change_timer(&t2, true, 200, 200, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 3);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 4);

    set_mock_time_ms(origin + 400);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 6);
}

TEST(change_timer_multi_two_overlap)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t2, "t2");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 100, 100, trigger_callback, &triggered);
    change_timer(&t2, true, 100, 100, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 150);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 4);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 6);

    set_mock_time_ms(origin + 400);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 8);
}

TEST(change_timer_simple_multi_two)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t2, "t2");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 100, 100, trigger_callback, &triggered);
    change_timer(&t2, true, 200, 0, trigger_callback, &triggered);
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

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 3);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 4);

    set_mock_time_ms(origin + 400);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 5);
}

TEST(change_timer_simple_multi_two_overlap)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t2, "t2");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 100, 100, trigger_callback, &triggered);
    change_timer(&t2, true, 100, 0, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 150);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 3);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 4);

    set_mock_time_ms(origin + 400);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 5);
}

TEST(stop_timer_simple_two)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t2, "t2");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 100, 0, trigger_callback, &triggered);
    change_timer(&t2, true, 200, 0, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    stop_timer(&t1);

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

TEST(stop_timer_simple_two_reversed)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t2, "t2");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 100, 0, trigger_callback, &triggered);
    change_timer(&t2, true, 200, 0, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    stop_timer(&t2);

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

TEST(stop_timer_simple_inside_callback)
{
    u_int64_t origin = MOCK_ORIGIN;
    TimeoutContainer container;
    bzero(&container, sizeof(container));
    Timer t1;
    init_timer(&t1, "t1");

    set_mock_time_ms(origin + 0);

    container.stop = true;
    container.timer = &t1;
    change_timer(&t1, true, 200, 0, container_callback, &container);
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

TEST(stop_timer_simple_other_inside_callback)
{
    u_int64_t origin = MOCK_ORIGIN;
    TimeoutContainer container;
    bzero(&container, sizeof(container));
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t1, "t2");
    int triggered_other = 0;

    set_mock_time_ms(origin + 0);

    container.stop_other = true;
    container.timer = &t1;
    change_timer(&t1, true, 100, 0, container_callback, &container);
    container.other = &t2;
    change_timer(&t2, true, 200, 0, trigger_callback, &triggered_other);
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

TEST(stop_timer_multi)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 100, 100, trigger_callback, &triggered);
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

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    stop_timer(&t1);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 400);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);
}

TEST(stop_timer_multi_two)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t2, "t2");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 100, 100, trigger_callback, &triggered);
    change_timer(&t2, true, 100, 100, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 150);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 4);

    stop_timer(&t1);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 5);

    set_mock_time_ms(origin + 400);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 6);
}

TEST(stop_timer_multi_inside_callback)
{
    u_int64_t origin = MOCK_ORIGIN;
    TimeoutContainer container;
    bzero(&container, sizeof(container));
    Timer t1;
    init_timer(&t1, "t1");

    set_mock_time_ms(origin + 0);

    container.stop = true;
    container.timer = &t1;
    change_timer(&t1, true, 100, 100, container_callback, &container);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 0);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);

    set_mock_time_ms(origin + 150);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 1);
}

TEST(stop_timer_multi_other_inside_callback)
{
    u_int64_t origin = MOCK_ORIGIN;
    TimeoutContainer container;
    bzero(&container, sizeof(container));
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t2, "t2");
    int triggered_other = 0;

    set_mock_time_ms(origin + 0);

    container.stop_other = true;
    container.timer = &t1;
    change_timer(&t1, true, 100, 100, container_callback, &container);
    container.other = &t2;
    change_timer(&t2, true, 200, 10, trigger_callback, &triggered_other);
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

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 3);
    ASSERT_EQUAL(triggered_other, 0);
}

TEST(change_timer_simple_again)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 200, 0, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    change_timer(&t1, true, 200, 0, trigger_callback, &triggered);

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

TEST(change_timer_simple_two_again)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t2, "t2");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 200, 0, trigger_callback, &triggered);
    change_timer(&t2, true, 250, 0, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    change_timer(&t1, true, 200, 0, trigger_callback, &triggered);

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

TEST(change_timer_simple_inside_callback_again)
{
    u_int64_t origin = MOCK_ORIGIN;
    TimeoutContainer container;
    bzero(&container, sizeof(container));
    Timer t1;
    init_timer(&t1, "t1");

    set_mock_time_ms(origin + 0);

    container.change = true;
    container.change_value_ms = 100;
    container.timer = &t1;
    change_timer(&t1, true, 200, 0, container_callback, &container);
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

TEST(change_timer_simple_other_inside_callback)
{
    u_int64_t origin = MOCK_ORIGIN;
    TimeoutContainer container;
    bzero(&container, sizeof(container));
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t2, "t2");
    int triggered_other = 0;

    set_mock_time_ms(origin + 0);

    container.change_other = true;
    container.change_other_value_ms = 100;
    container.timer = &t1;
    change_timer(&t1, true, 100, 0, container_callback, &container);
    container.other = &t2;
    change_timer(&t2, true, 1000, 0, trigger_callback, &triggered_other);
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

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 2);
    ASSERT_EQUAL(triggered_other, 0);
}

TEST(add_change_two_timeout_simple_inside_callback)
{
    u_int64_t origin = MOCK_ORIGIN;
    TimeoutContainer container;
    bzero(&container, sizeof(container));
    Timer t1;
    init_timer(&t1, "t1");

    set_mock_time_ms(origin + 0);

    container.add = true;
    container.add_value_ms = 100;
    container.change = true;
    container.change_value_ms = 100;
    container.timer = &t1;
    change_timer(&t1, true, 200, 0, container_callback, &container);
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

TEST(change_timer_multi_again)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 100, 50, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 150);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 3);

    change_timer(&t1, true, 100, 100, trigger_callback, &triggered);

    set_mock_time_ms(origin + 250);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 3);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 4);

    set_mock_time_ms(origin + 350);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 4);

    set_mock_time_ms(origin + 400);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 5);
}

TEST(change_timer_simple_multi)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 100, 0, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    change_timer(&t1, true, 100, 100, trigger_callback, &triggered);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);

    set_mock_time_ms(origin + 150);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);

    set_mock_time_ms(origin + 250);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);

    change_timer(&t1, true, 50, 0, trigger_callback, &triggered);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 3);

    set_mock_time_ms(origin + 350);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 3);

    set_mock_time_ms(origin + 400);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 3);
}

TEST(change_timer_multi_inside_callback)
{
    u_int64_t origin = MOCK_ORIGIN;
    TimeoutContainer container;
    bzero(&container, sizeof(container));
    Timer t1;
    init_timer(&t1, "t1");

    set_mock_time_ms(origin + 0);

    container.change = true;
    container.change_value_ms = 100;
    container.change_interval_ms = 100;
    container.timer = &t1;
    change_timer(&t1, true, 200, 200, container_callback, &container);
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

TEST(change_timer_multi_other_inside_callback)
{
    u_int64_t origin = MOCK_ORIGIN;
    TimeoutContainer container;
    bzero(&container, sizeof(container));
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t2, "t2");
    int triggered_other = 0;

    set_mock_time_ms(origin + 0);

    container.change_other = true;
    container.change_other_value_ms = 100;
    container.change_other_interval_ms = 100;
    container.timer = &t1;
    change_timer(&t1, true, 100, 0, container_callback, &container);
    container.other = &t2;
    change_timer(&t2, true, 1000, 0, trigger_callback, &triggered_other);
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

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(container.triggered, 3);
    ASSERT_EQUAL(triggered_other, 0);
}

TEST(add_change_two_timeout_multi_inside_callback)
{
    u_int64_t origin = MOCK_ORIGIN;
    TimeoutContainer container;
    bzero(&container, sizeof(container));
    Timer t1;
    init_timer(&t1, "t1");

    set_mock_time_ms(origin + 0);

    container.add = true;
    container.add_value_ms = 100;
    container.add_interval_ms = 100;
    container.change = true;
    container.change_value_ms = 100;
    container.change_interval_ms = 100;
    container.timer = &t1;
    change_timer(&t1, true, 200, 200, container_callback, &container);
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
    ASSERT_EQUAL(container.triggered, 5);
}

TEST(get_duration_to_next_timer_expiration_simple)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 200, 0, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 200);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 100);

    change_timer(&t1, true, 200, 0, trigger_callback, &triggered);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 200);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 100);

    set_mock_time_ms(origin + 250);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 50);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), -1);

    set_mock_time_ms(origin + 500);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), -1);
}

TEST(get_duration_to_next_timer_expiration_multi)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 100, 200, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 100);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 50);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 200);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 100);

    set_mock_time_ms(origin + 300);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 200);

    change_timer(&t1, true, 100, 0, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 100);

    change_timer(&t1, true, 100, 300, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 100);

    set_mock_time_ms(origin + 400);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 3);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 300);

    set_mock_time_ms(origin + 700);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 4);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 300);
}

TEST(get_duration_to_next_timer_expiration_simple_multi)
{
    u_int64_t origin = MOCK_ORIGIN;
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t2, "t2");
    int triggered = 0;

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 100, 0, trigger_callback, &triggered);
    change_timer(&t2, true, 200, 50, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 100);

    set_mock_time_ms(origin + 50);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 0);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 50);

    set_mock_time_ms(origin + 100);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 100);

    set_mock_time_ms(origin + 150);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 1);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 50);

    set_mock_time_ms(origin + 200);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 50);

    change_timer(&t1, true, 10, 0, trigger_callback, &triggered);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 2);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 10);

    set_mock_time_ms(origin + 210);
    handle_expired_timers();
    ASSERT_EQUAL(triggered, 3);
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), 40);
}

TEST(cleanup_timeout_simple)
{
    u_int64_t origin = MOCK_ORIGIN;
    int triggered = 0;
    Timer t1;
    init_timer(&t1, "t1");
    Timer t2;
    init_timer(&t1, "t2");
    Timer t3;
    init_timer(&t1, "t3");

    set_mock_time_ms(origin + 0);
    change_timer(&t1, true, 100, 0, trigger_callback, &triggered);
    change_timer(&t2, true, 200, 0, trigger_callback, &triggered);
    change_timer(&t3, true, 300, 0, trigger_callback, &triggered);
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

    cleanup_timers();
    ASSERT_EQUAL(timeval_to_ms(get_duration_to_next_timer_expiration()), -1);

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
