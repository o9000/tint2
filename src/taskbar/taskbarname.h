/**************************************************************************
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
**************************************************************************/

#ifndef TASKBARNAME_H
#define TASKBARNAME_H

#include "common.h"
#include "area.h"

extern int taskbarname_enabled;
extern PangoFontDescription *taskbarname_font_desc;
extern Color taskbarname_font;
extern Color taskbarname_active_font;

void default_taskbarname();
void cleanup_taskbarname();

void init_taskbarname_panel(void *p);

void draw_taskbarname(void *obj, cairo_t *c);

int  resize_taskbarname(void *obj);


#endif

