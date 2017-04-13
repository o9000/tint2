// Tint2 : Separator
// Author: Oskari Rauta <oskari.rauta@gmail.com>

#ifndef SEPARATOR_H
#define SEPARATOR_H

#include "common.h"
#include "area.h"

typedef enum SeparatorStyle { SEPARATOR_EMPTY = 0, SEPARATOR_LINE, SEPARATOR_DOTS } SeparatorStyle;

typedef struct Separator {
    Area area;
    SeparatorStyle style;
    Color color;
    int thickness;
    int length;
} Separator;

Separator *create_separator();
void destroy_separator(void *obj);
void init_separator();
void init_separator_panel(void *p);
void cleanup_separator();
gboolean resize_separator(void *obj);
void draw_separator(void *obj, cairo_t *c);

#endif
