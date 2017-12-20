#ifndef TRACING_H
#define TRACING_H

#ifdef HAVE_TRACING

void init_tracing();
void cleanup_tracing();

void start_tracing(void *root);
void stop_tracing();
void print_tracing_events();

#endif

#endif
