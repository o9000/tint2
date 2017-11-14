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

typedef struct {
    Area area;
    gchar *name;
    int posy;
} TaskbarName;

typedef struct {
    Area area;
    int desktop;
    TaskbarName bar_name;
    int text_width;
} Taskbar;

typedef struct GlobalTaskbar {
    Area area;
    Area area_name;
    Background *background[TASKBAR_STATE_COUNT];
    Background *background_name[TASKBAR_STATE_COUNT];
    GList *gradient[TASKBAR_STATE_COUNT];
    GList *gradient_name[TASKBAR_STATE_COUNT];
} GlobalTaskbar;

extern gboolean taskbar_enabled;
extern gboolean taskbar_distribute_size;
extern gboolean hide_task_diff_desktop;
extern gboolean hide_inactive_tasks;
extern gboolean hide_task_diff_monitor;
extern gboolean hide_taskbar_if_empty;
extern gboolean always_show_all_desktop_tasks;
extern TaskbarSortMethod taskbar_sort_method;
extern Alignment taskbar_alignment;

// win_to_task holds for every Window an array of tasks. Usually the array contains only one
// element. However for omnipresent windows (windows which are visible in every taskbar) the array
// contains to every Task* on each panel a pointer (i.e. GPtrArray.len == server.num_desktops)
extern GHashTable *win_to_task;

extern Task *active_task;
extern Task *task_drag;

void default_taskbar();
void cleanup_taskbar();
void init_taskbar();
void init_taskbar_panel(void *p);

gboolean resize_taskbar(void *obj);
void taskbar_default_font_changed();
void taskbar_start_thumbnail_timer();

// Reloads the entire list of tasks from the window manager and recreates the task buttons.
void taskbar_refresh_tasklist();

// Returns the task button for this window. If there are multiple buttons, returns the first one.
Task *get_task(Window win);

// Returns the task buttons for this window, usually having only one element.
// However for windows shown on all desktops, there are multiple buttons, one for each taskbar.
GPtrArray *get_task_buttons(Window win);

// Change state of a taskbar (ACTIVE or NORMAL)
void set_taskbar_state(Taskbar *taskbar, TaskbarState state);

// Updates the visibility of all taskbars
void update_all_taskbars_visibility();

void update_minimized_icon_positions(void *p);

// Sorts the taskbar(s) on which the window is present.
void sort_taskbar_for_win(Window win);

void sort_tasks(Taskbar *taskbar);

gboolean taskbar_is_under_mouse(void *obj, int x, int y);

#endif
