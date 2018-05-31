#define _GNU_SOURCE
#include <dlfcn.h>
#include <endian.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>
#include <signal.h>

#include "bt.h"
#include "bool.h"

#define UNUSED(x) ((void)x)

void ERR(const char *s)
{
    if (!s) {
        ERR("(null)");
        return;
    }
    ssize_t ret = write(STDERR_FILENO, s, strlen(s));
    UNUSED(ret);
    ret = fsync(STDERR_FILENO);
    UNUSED(ret);
}

void utoa(unsigned long n, char *s)
{
    if (n == 0) {
        *s++ = '0';
        *s = 0;
        return;
    }
    char buffer[128] = {0};
    char *digit;
    for (digit = buffer; n; digit++, n/=10) {
        *digit = '0' + (n % 10);
    }
    digit--;
    while (digit >= buffer) {
        *s++ = *digit--;
    }
    *s = 0;
}

unsigned long uabs(long n)
{
    if (n == LONG_MIN)
        return ((unsigned long)LONG_MAX) + 1;
    return (unsigned long)labs(n);
}

void itoa(long n, char *s)
{
    if (n < 0) {
        *s++ = '-';
    }
    utoa(uabs(n), s);
}

__attribute__((noreturn))
static void crash(char *file, int line, char *msg)
{
    ERR(file);
    ERR(":");
    char buf[256];
    itoa(line, buf);
    ERR(buf);
    ERR(" ");
    ERR(msg);
    _exit(1);
}

#define ASSERT_OK(call) if (0 != (call)) crash(__FILE__, __LINE__, #call)
#define ASSERT_FD(fd) if ((fd) == -1) crash(__FILE__, __LINE__, "bad file descriptor")
#define ASSERT_PID(pid) if ((pid) == -1) crash(__FILE__, __LINE__, "bad PID")
#define ASSERT(b) if (!(b)) crash(__FILE__, __LINE__, "assert failed")

void *return_null(void *x, void *y)
{
    return 0;
}

void load_func_or_crash(void **result, const char *name)
{
    static int inside_loader = 0;
    if (inside_loader) {
        *result = (void*)return_null;
        return;
    } else if (*result == (void*)return_null) {
        *result = 0;
    }
    if (*result)
        return;
    dlerror();
    inside_loader++;
    *result = dlsym(RTLD_NEXT, name);
    inside_loader--;
    char *err = dlerror();
    if (err) {
        ERR("Failed to load ");
        ERR(name);
        ERR(" error: ");
        ERR(err);
        ERR("\n");
        exit(1);
    }
}

static int fd = -1;
static u_int64_t tstart = 0;
static bool stop_alloc_log = false;

static void write_char(char c)
{
    static char buffer[4096] = {0};
    static size_t count = 0;
    if (c) {
        buffer[count++] = c;
    }
    if (!count)
        return;
    if (c == '\n' || c == 0 || count >= sizeof(buffer)) {
        ssize_t ret = write(fd, buffer, count);
        ASSERT(ret > 0);
        count = 0;
    }
}

static void write_string(const char *s)
{
    for (; *s; s++)
        write_char(*s);
}

static void write_word(const char *s)
{
    write_string(s);
    write_string(" ");
}

static char hex(u_int8_t value)
{
    if (value < 10)
        return '0' + (char)value;
    return 'a' + (char)(value - 10);
}

static void write_hex(u_int64_t v)
{
    write_string("0x");
    if (!v) {
        write_string("0 ");
        return;
    }
    v = htobe64(v);
    int leading_zero = 1;
    while (v) {
        u_int8_t byte = v & 0xff;
        if (byte)
            leading_zero = 0;
        if (!leading_zero) {
            write_char(hex(byte >> 4));
            write_char(hex(byte & 0xf));
        }
        v = v >> 8;
    }
    write_string(" ");
}

static void write_backtrace(int skip)
{
    struct backtrace bt;
    get_backtrace(&bt, skip + 1);
    for (size_t i = 0; i < bt.frame_count; i++) {
        write_word(bt.frames[i].name);
    }
}

static void log_alloc_finish(void);

static u_int64_t current_time_ms()
{
    struct timespec t = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &t);
    u_int64_t result = t.tv_sec * 1000 + t.tv_nsec / 1000 / 1000;
    return result;
}

static void log_alloc_init(u_int64_t t)
{
    if (stop_alloc_log)
        return;
    if (fd == -1) {
        int pfd[2] = {-1, -1};
        ASSERT_OK(pipe(pfd));
        pid_t child = fork();
        ASSERT_PID(child);
        if (child == 0) {
            // child
            close(pfd[1]);
            ASSERT_FD(dup2(pfd[0], STDIN_FILENO));
            ASSERT_OK(close(pfd[0]));
            int out = open("mem.log.gz", O_APPEND | O_CLOEXEC | O_CREAT | O_WRONLY | O_TRUNC, 0600);
            ASSERT_FD(out);
            ASSERT_FD(dup2(out, STDOUT_FILENO));
            sigset_t mask;
            sigfillset(&mask);
            sigprocmask(SIG_SETMASK, &mask, NULL);
            ASSERT_OK(execlp("gzip", "gzip", "-c", NULL));
            _exit(1);
        } else {
            // parent
            close(pfd[0]);
            fd = pfd[1];
        }
        atexit(log_alloc_finish);
        tstart = t;
        write_string("# function time_ms result ptr size count backtrace\n");
    }
}

static void log_alloc_locked(const char *func_name, void *result, void *ptr, size_t size, size_t count)
{
    if (stop_alloc_log)
        return;
    u_int64_t t = current_time_ms();
    if (fd == -1)
        log_alloc_init(t);
    if (fd == -1)
        return;
    if (func_name) {
        write_word(func_name);
        write_hex((u_int64_t)(t - tstart));
        write_hex((u_int64_t)result);
        write_hex((u_int64_t)ptr);
        write_hex((u_int64_t)size);
        write_hex((u_int64_t)count);
        write_backtrace(2);
        write_string("\n");
    } else {
        write_string("# done\n");
        close(fd);
        fd = -1;
        stop_alloc_log = true;
    }
}

static int pid = -1;
static void log_alloc(const char *func_name, void *result, void *ptr, size_t size, size_t count)
{
    static pthread_mutex_t mutex_global = PTHREAD_MUTEX_INITIALIZER;
    static pthread_mutex_t mutex_recursive;
    static pthread_mutex_t mutex_nonrecursive = PTHREAD_MUTEX_INITIALIZER;
    static bool mutexes_initialized = false;

    pthread_mutex_lock(&mutex_global);
    {
        if (!mutexes_initialized) {
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&mutex_recursive, &attr);
            mutexes_initialized = true;
            pid = getpid();
        }
    }
    pthread_mutex_unlock(&mutex_global);

    // Do not log from forked processes.
    if (pid != getpid())
        return;

    pthread_mutex_lock(&mutex_recursive);
    int ret = pthread_mutex_trylock(&mutex_nonrecursive);
    if (ret == 0) {
        log_alloc_locked(func_name, result, ptr, size, count);
        pthread_mutex_unlock(&mutex_nonrecursive);
    }
    pthread_mutex_unlock(&mutex_recursive);
}

static void log_alloc_finish()
{
    log_alloc(0, 0, 0, 0, 0);
}

void *malloc(size_t size)
{
    static void *(*original)(size_t size) = 0;
    load_func_or_crash((void *)&original, __FUNCTION__);
    void *result = original(size);
    log_alloc(__FUNCTION__, result, 0, size, 0);
    return result;
}

void *realloc(void *ptr, size_t size)
{
    static void *(*original)(void *p, size_t size) = 0;
    load_func_or_crash((void *)&original, __FUNCTION__);
    void *result = original(ptr, size);
    log_alloc(__FUNCTION__, result, ptr, size, 0);
    return result;
}

void *calloc(size_t nmemb, size_t size)
{
    static void *(*original)(size_t nmemb, size_t size) = 0;
    load_func_or_crash((void *)&original, __FUNCTION__);
    void *result = original(nmemb, size);
    log_alloc(__FUNCTION__, result, 0, size, nmemb);
    return result;
}

void free(void *ptr)
{
    static void *(*original)(void *p) = 0;
    load_func_or_crash((void *)&original, __FUNCTION__);
    if (!original) {
        return;
    }
    original(ptr);
    log_alloc(__FUNCTION__, 0, ptr, 0, 0);
}
