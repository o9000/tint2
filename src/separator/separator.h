// Tint2 : Separator
// Author: Oskari Rauta <oskari.rauta@gmail.com>

#ifndef SEPARATOR_H
#define SEPARATOR_H

#include "common.h"
#include "area.h"

typedef struct Separator {
	Area area;
	int style;
	Color color;
	double empty_thickness;
	double thickness;
	double len;
} Separator;

Separator *create_separator();
void destroy_separator(void *obj);
void init_separator();
void init_separator_panel(void *p);
void cleanup_separator();
gboolean resize_separator(void *obj);
void draw_separator(void *obj, cairo_t *c);

#endif
