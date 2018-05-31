#ifndef BT_H
#define BT_H

#include <sys/types.h>

#define BT_FRAME_SIZE 64
#define BT_MAX_FRAMES 64

struct backtrace_frame {
    char name[BT_FRAME_SIZE];
};

struct backtrace {
    struct backtrace_frame frames[BT_MAX_FRAMES];
    size_t frame_count;
};

void get_backtrace(struct backtrace *bt, int skip);

#endif // BT_H
