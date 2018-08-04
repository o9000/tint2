/**************************************************************************
*
* Tint2 : backtrace
*
* Copyright (C) 2007 Pål Staurland (staura@gmail.com)
* Modified (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "bt.h"
#include "bool.h"

#if defined(HAS_BACKTRACE) || defined(HAS_LIBUNWIND) || defined(HAS_EXECINFO)

static void bt_add_frame(struct backtrace *bt, const char *fname)
{
    if (bt->frame_count >= BT_MAX_FRAMES)
        return;
    struct backtrace_frame *frame = &bt->frames[bt->frame_count];
    if (fname && *fname) {
        strncpy(frame->name, fname, BT_FRAME_SIZE);
    } else {
        strncpy(frame->name, "??", BT_FRAME_SIZE);
    }
    bt->frame_count++;
}

#endif

#ifdef HAS_BACKTRACE

#include <backtrace.h>

static const char *get_exe()
{
    static char buf[256] = {0};
    ssize_t ret = readlink("/proc/self/exe", buf, sizeof(buf)-1);
    if (ret > 0)
        return buf;
    ret = readlink("/proc/curproc/file", buf, sizeof(buf)-1);
    if (ret > 0)
        return buf;
    return buf;
}

static void bt_error_callback(void *data, const char *msg, int errnum)
{
}

static int bt_full_callback(void *data, uintptr_t pc,
                     const char *filename, int lineno,
                     const char *function)
{
    struct backtrace *bt = (struct backtrace *)data;
    bt_add_frame(bt, function);
    return 0;
}

void get_backtrace(struct backtrace *bt, int skip)
{
    static struct backtrace_state *state = NULL;

    if (!state) {
        const char *exe = get_exe();
        if (exe)
            state = backtrace_create_state(exe, 1, bt_error_callback, NULL);
    }
    bzero(bt, sizeof(*bt));
    if (state) {
        backtrace_full(state, skip + 1, bt_full_callback, bt_error_callback, bt);
    }
}

#elif HAS_LIBUNWIND

#define UNW_LOCAL_ONLY
#include <libunwind.h>

struct bt_mapping {
    unw_word_t ip;
    char fname[BT_FRAME_SIZE];
};

#define BT_BUCKET_SIZE 3
struct bt_bucket {
    struct bt_mapping mappings[BT_BUCKET_SIZE];
};

struct bt_cache {
    struct bt_bucket *buckets;
    size_t count;
    size_t size;
};

static unsigned oat_hash(void *key, int len)
{
    unsigned char *p = key;
    unsigned h = 0;
    int i;

    for (i = 0; i < len; i++)
    {
        h += p[i];
        h += (h << 10);
        h ^= (h >> 6);
    }

    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);

    return h;
}

static struct bt_bucket *cached_proc_name_bucket(struct bt_cache *cache, unw_word_t ip)
{
    if (!cache->size)
        return NULL;
    unsigned h = oat_hash(&ip, sizeof(ip));
    return &cache->buckets[h % cache->size];
}

static void bt_cache_init(struct bt_cache *cache)
{
    if (!cache->size) {
        cache->size = 119;
        cache->buckets = calloc(cache->size, sizeof(*cache->buckets));
        if (!cache->buckets)
            cache->size = 0;
        cache->count = 0;
    }
}

static void cached_proc_name_store(struct bt_cache *cache, unw_word_t ip, const char *fname);

static void bt_cache_rebalance(struct bt_cache *cache)
{
    struct bt_cache bigger = {};
    bigger.size = cache->size * 2 + 1337;
    bigger.buckets = calloc(bigger.size, sizeof(*bigger.buckets));
    bigger.count = 0;
    for (size_t b = 0; b < cache->size; b++) {
        for (size_t i = 0; i < BT_BUCKET_SIZE; i++) {
            struct bt_mapping *map = &cache->buckets[b].mappings[i];
            if (map->ip) {
                cached_proc_name_store(&bigger, map->ip, map->fname);
            }
        }
    }
    free(cache->buckets);
    *cache = bigger;
}

static void cached_proc_name_store(struct bt_cache *cache, unw_word_t ip, const char *fname)
{
    bt_cache_init(cache);
    if (!cache->size)
        return;
    struct bt_bucket *bucket = cached_proc_name_bucket(cache, ip);
    bool stored = false;
    for (size_t i = 0; i < BT_BUCKET_SIZE; i++) {
        if (bucket->mappings[i].ip == ip)
            return;
        if (bucket->mappings[i].ip != 0)
            continue;
        bucket->mappings[i].ip = ip;
        strncpy(bucket->mappings[i].fname, fname, sizeof(bucket->mappings[i].fname));
        cache->count++;
        stored = true;
        break;
    }
    if (cache->count > cache->size / 4 || !stored) {
        bt_cache_rebalance(cache);
        fprintf(stderr, "tint2: proc_name cache: ratio %f, count %lu, size %lu, (%lu bytes)\n",
                cache->count / (double)cache->size,
                cache->count, (size_t)cache->size, cache->size * sizeof(*cache->buckets));
    }
}

const char *cached_proc_name_get(struct bt_cache *cache, unw_word_t ip)
{
    struct bt_bucket *bucket = cached_proc_name_bucket(cache, ip);
    if (!bucket)
        return NULL;
    for (size_t i = 0; i < BT_BUCKET_SIZE; i++) {
        if (bucket->mappings[i].ip != ip)
            continue;
        return bucket->mappings[i].fname;
    }
    return NULL;
}

void get_backtrace(struct backtrace *bt, int skip)
{
    static struct bt_cache bt_cache = {};
    bzero(bt, sizeof(*bt));

    unw_cursor_t cursor;
    unw_context_t context;
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    while (unw_step(&cursor) > 0) {
        if (skip > 0) {
            skip--;
            continue;
        }
        unw_word_t offset;
        char fname[BT_FRAME_SIZE] = {0};
        unw_word_t ip;
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        const char *fname_cached = cached_proc_name_get(&bt_cache, ip);
        if (fname_cached) {
            bt_add_frame(bt, fname_cached);
        } else {
            unw_get_proc_name(&cursor, fname, sizeof(fname), &offset);
            cached_proc_name_store(&bt_cache, ip, fname);
            bt_add_frame(bt, fname);
        }
    }
}

#elif HAS_EXECINFO

#include <execinfo.h>

void get_backtrace(struct backtrace *bt, int skip)
{
    bzero(bt, sizeof(*bt));

    void *array[BT_MAX_FRAMES];
    int size = backtrace(array, BT_MAX_FRAMES);
    char **strings = backtrace_symbols(array, size);

    for (int i = 0; i < size; i++) {
        bt_add_frame(bt, strings[i]);
    }

    free(strings);
}


#else

void get_backtrace(struct backtrace *bt, int skip)
{
    bzero(bt, sizeof(*bt));
}

#endif
