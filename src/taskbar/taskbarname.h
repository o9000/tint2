/**************************************************************************
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
*
* taskbarname
*
**************************************************************************/

#ifndef TASKBARNAME_H
#define TASKBARNAME_H
#include "area.h"


extern int taskbarname_enabled;

typedef struct {
	// always start with area
	Area area;

	int desktop;

} Taskbarname;


// default global data
void default_taskbarname();

// freed memory
void cleanup_taskbarname();

void init_taskbarname();
void init_taskbarname_panel(void *p);

void draw_taskbarname(void *obj, cairo_t *c);

int  resize_taskbarname(void *obj);


#endif

