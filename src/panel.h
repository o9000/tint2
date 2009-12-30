/**************************************************************************
* Copyright (C) 2008 Pål Staurland (staura@gmail.com)
* Modified (C) 2008/2009 thierry lorthiois (lorthiois@bbsoft.fr)
*
* panel :
* - draw panel and all objects according to panel_layout
*
*
**************************************************************************/

#ifndef PANEL_H
#define PANEL_H

#include <pango/pangocairo.h>
#include <sys/time.h>

#include "common.h"
#include "clock.h"
#include "task.h"
#include "taskbar.h"
#include "systraybar.h"

#ifdef ENABLE_BATTERY
#include "battery.h"
#endif


extern int signal_pending;
// --------------------------------------------------
// mouse events
extern int mouse_middle;
extern int mouse_right;
extern int mouse_scroll_up;
extern int mouse_scroll_down;
extern int mouse_tilt_left;
extern int mouse_tilt_right;

//panel mode
enum { SINGLE_DESKTOP=0, MULTI_DESKTOP };
enum { BOTTOM_LAYER, NORMAL_LAYER, TOP_LAYER };
extern int panel_mode;
extern int wm_menu;
extern int panel_dock;
extern int panel_layer;

//panel position
enum { LEFT=0x01, RIGHT=0x02, CENTER=0X04, TOP=0X08, BOTTOM=0x10 };
extern int panel_position;
extern int panel_horizontal;

extern int panel_refresh;

extern Task *task_active;
extern Task *task_drag;
extern int  max_tick_urgent;

extern Imlib_Image default_icon;


// tint2 use one panel per monitor and one taskbar per desktop.
typedef struct {
	// always start with area
	// area.list own all objects of the panel according to config file
	Area area;

	// --------------------------------------------------
	// panel
	Window main_win;
	Pixmap temp_pmap;

	// position relative to root window
	int posx, posy;
	int marginx, marginy;
	int pourcentx, pourcenty;
	// location of the panel (monitor number)
	int monitor;

	// --------------------------------------------------
	// task and taskbar parameter per panel
	Area g_taskbar;
	Global_task g_task;

	// --------------------------------------------------
	// taskbar point to the first taskbar in panel.area.list.
	// number of tasbar == nb_desktop. taskbar[i] is for desktop(i).
	// taskbar[i] is used to loop over taskbar,
	// while panel->area.list is used to loop over all panel's objects
	Taskbar *taskbar;
	int  nb_desktop;

	// --------------------------------------------------
	// clock
	Clock clock;

	// --------------------------------------------------
	// battery
#ifdef ENABLE_BATTERY
	Battery battery;
#endif
} Panel;


extern Panel panel_config;
extern Panel *panel1;
extern int  nb_panel;

// realloc panels according to number of monitor
// use panel_config as default value
void init_panel();

void init_panel_size_and_position(Panel *panel);
void cleanup_panel();
void resize_panel(void *obj);

void set_panel_properties(Panel *p);
void visible_object();

// draw background panel
void set_panel_background(Panel *p);

// detect witch panel
Panel *get_panel(Window win);

Taskbar *click_taskbar (Panel *panel, int x, int y);
Task *click_task (Panel *panel, int x, int y);
int click_padding(Panel *panel, int x, int y);
int click_clock(Panel *panel, int x, int y);
Area* click_area(Panel *panel, int x, int y);

#endif
