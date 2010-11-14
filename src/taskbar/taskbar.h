/**************************************************************************
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
*
* taskbar
*
**************************************************************************/

#ifndef TASKBAR_H
#define TASKBAR_H

#include "task.h"
#include "taskbarname.h"

enum { TASKBAR_NORMAL, TASKBAR_ACTIVE, TASKBAR_STATE_COUNT };
extern GHashTable* win_to_task_table;
extern Task *task_active;
extern Task *task_drag;
extern int taskbar_enabled;


typedef struct {
	// always start with area
	Area area;
	Pixmap state_pix[TASKBAR_STATE_COUNT];

	char *name;
	int  posy;
} Taskbarname;

// tint2 use one taskbar per desktop.
typedef struct {
	// always start with area
	Area area;

	int desktop;
	Pixmap state_pix[TASKBAR_STATE_COUNT];

	Taskbarname bar_name;
	
	// task parameters
	int text_width;
} Taskbar;

typedef struct {
	//always start with area
	Area area;
	Area area_name;
	Background* background[TASKBAR_STATE_COUNT];
	Background* background_name[TASKBAR_STATE_COUNT];
} Global_taskbar;


// default global data
void default_taskbar();

// freed memory
void cleanup_taskbar();

void init_taskbar();
void init_taskbar_panel(void *p);

void draw_taskbar (void *obj, cairo_t *c);
void taskbar_remove_task(gpointer key, gpointer value, gpointer user_data);
Task *task_get_task (Window win);
GPtrArray* task_get_tasks(Window win);
void task_refresh_tasklist ();

int  resize_taskbar(void *obj);
void on_change_taskbar (void *obj);
void set_taskbar_state(Taskbar *tskbar, int state);

// show/hide taskbar according to current desktop
void visible_taskbar(void *p);


#endif

