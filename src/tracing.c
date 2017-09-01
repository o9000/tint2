#include "timer.h"

#ifdef HAVE_TRACING

#ifdef ENABLE_EXECINFO
#include <execinfo.h>
#endif
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RED "\033[1;31m"
#define BLUE "\033[1;34m"
#define RESET "\033[0m"

static GList *tracing_events = NULL;
static sig_atomic_t tracing = FALSE;

typedef struct TracingEvent {
    void *address;
    void *caller;
    double time;
    gboolean enter;
} TracingEvent;

void __attribute__ ((constructor)) init_tracing()
{
    tracing_events = NULL;
    tracing = FALSE;
}

void cleanup_tracing()
{
    g_list_free_full(tracing_events, free);
    tracing_events = NULL;
    tracing = FALSE;
}

char *addr2name(void *func)
{
#ifdef ENABLE_EXECINFO
    void *array[1];
    array[0] = func;
    char **strings = backtrace_symbols(array, 1);
    char *result = strdup(strings[0] ? strings[0] : "??");
    free(strings);
    return result;
#else
    char *result = (char*) calloc(32, 1);
    sprintf(result, "%p", func);
    return result;
#endif
}

void add_tracing_event(void *func, void *caller, gboolean enter)
{
    TracingEvent *entry = (TracingEvent *)calloc(sizeof(TracingEvent), 1);
    entry->address = func;
    entry->caller = caller;
    entry->time = get_time();
    entry->enter = enter;
    tracing_events = g_list_append(tracing_events, entry);
}

void start_tracing(void *root)
{
    if (tracing_events)
        cleanup_tracing();
    add_tracing_event(root, NULL, TRUE);
    tracing = TRUE;
}

void stop_tracing()
{
    tracing = FALSE;
}

void __cyg_profile_func_enter(void *func, void *caller)
{
    if (tracing)
        add_tracing_event(func, caller, TRUE);
}

void __cyg_profile_func_exit(void *func, void *caller)
{
    if (tracing)
        add_tracing_event(func, caller, FALSE);
}

void print_tracing_events()
{
    GList *stack = NULL;
    int depth = 0;
    double now = get_time();
    for (GList *i = tracing_events; i; i = i->next) {
        TracingEvent *e = (TracingEvent *)i->data;
        if (e->enter) {
            // Push a new function on the stack
            for (int d = 0; d < depth; d++)
                fprintf(stderr, "tint2:  ");
            char *name = addr2name(e->address);
            char *caller = addr2name(e->caller);
            fprintf(stderr,
                    "%s called from %s\n",
                    name,
                    caller);
            stack = g_list_append(stack, e);
            depth++;
        } else {
            // Pop a function from the stack, if matching, and print
            if (stack) {
                TracingEvent *old = (TracingEvent *)g_list_last(stack)->data;
                if (old->address == e->address) {
                    depth--;
                    for (int d = 0; d < depth; d++)
                        fprintf(stderr, "tint2:  ");
                    char *name = addr2name(e->address);
                    double duration = (e->time - old->time) * 1.0e3;
                    fprintf(stderr,
                            "-- %s exited after %.1f ms",
                            name,
                            duration);
                    if (duration >= 1.0) {
                        fprintf(stderr, YELLOW "tint2:  ");
                        for (int d = 0; d < duration; d++) {
                            fprintf(stderr, "tint2: #");
                        }
                        fprintf(stderr, RESET);
                    }
                    fprintf(stderr, "tint2: \n");
                    free(name);
                    stack = g_list_delete_link(stack, g_list_last(stack));
                }
            }
        }
    }
    while (stack) {
        TracingEvent *old = (TracingEvent *)g_list_last(stack)->data;
        depth--;
        for (int d = 0; d < depth; d++)
            fprintf(stderr, "tint2:  ");
        char *name = addr2name(old->address);
        double duration = (now - old->time) * 1.0e3;
        fprintf(stderr,
                "-- %s exited after %.1f ms",
                name,
                duration);
        if (duration >= 1.0) {
            fprintf(stderr, YELLOW "tint2:  ");
            for (int d = 0; d < duration; d++) {
                fprintf(stderr, "tint2: #");
            }
            fprintf(stderr, RESET);
        }
        fprintf(stderr, "tint2: \n");
        free(name);
        stack = g_list_delete_link(stack, g_list_last(stack));
    }
}

#endif
