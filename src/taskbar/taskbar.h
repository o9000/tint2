/**************************************************************************
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
*
* taskbar
*
**************************************************************************/

#ifndef TASKBAR_H
#define TASKBAR_H

#include "task.h"

extern GHashTable* win_to_task_table;
extern Task *task_active;
extern Task *task_drag;

// tint2 use one taskbar per desktop.
typedef struct {
	// always start with area
	Area area;

	int desktop;

	// task parameters
	int task_width;
	int task_modulo;
	int text_width;
} Taskbar;


typedef struct {
	//always start with area
	Area area;
	Background* bg;
	Background* bg_active;
	int use_active;
} Global_taskbar;


// default global data
void default_taskbar();

// freed memory
void cleanup_taskbar();

void init_taskbar();

Task *task_get_task (Window win);
GPtrArray* task_get_tasks(Window win);
void task_refresh_tasklist ();

void resize_taskbar(void *obj);


#endif

