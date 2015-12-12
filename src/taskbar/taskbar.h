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

typedef enum TaskbarState {
	TASKBAR_NORMAL = 0,
	TASKBAR_ACTIVE,
	TASKBAR_STATE_COUNT,
} TaskbarState;

typedef enum TaskbarSortMethod {
	TASKBAR_NOSORT = 0,
	TASKBAR_SORT_CENTER,
	TASKBAR_SORT_TITLE,
	TASKBAR_SORT_LRU,
	TASKBAR_SORT_MRU,
} TaskbarSortMethod;

extern GHashTable *win_to_task;
extern Task *active_task;
extern Task *task_drag;
extern gboolean taskbar_enabled;
extern gboolean taskbar_distribute_size;
extern gboolean hide_inactive_tasks;
extern gboolean hide_task_diff_monitor;
extern TaskbarSortMethod taskbar_sort_method;
extern Alignment taskbar_alignment;

typedef struct {
	Area area;
	Pixmap state_pix[TASKBAR_STATE_COUNT];

	gchar *name;
	int posy;
} Taskbarname;

typedef struct {
	Area area;
	Pixmap state_pix[TASKBAR_STATE_COUNT];
	int desktop;
	Taskbarname bar_name;
	int text_width;
} Taskbar;

typedef struct GlobalTaskbar {
	// always start with area
	Area area;
	Area area_name;
	Background *background[TASKBAR_STATE_COUNT];
	Background *background_name[TASKBAR_STATE_COUNT];
} GlobalTaskbar;

// default global data
void default_taskbar();

// freed memory
void cleanup_taskbar();

void init_taskbar();
void init_taskbar_panel(void *p);

void draw_taskbar(void *obj, cairo_t *c);
void taskbar_remove_task(gpointer key, gpointer value, gpointer user_data);
Task *task_get_task(Window win);
GPtrArray *task_get_tasks(Window win);
void task_refresh_tasklist();

gboolean resize_taskbar(void *obj);
void on_change_taskbar(void *obj);
void set_taskbar_state(Taskbar *taskbar, TaskbarState state);

// show/hide taskbar according to current desktop
void visible_taskbar(void *p);

void sort_taskbar_for_win(Window win);
void sort_tasks(Taskbar *taskbar);

void taskbar_default_font_changed();

#endif
