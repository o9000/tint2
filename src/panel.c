/**************************************************************************
*
* Copyright (C) 2008 Pål Staurland (staura@gmail.com)
* Modified (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <pango/pangocairo.h>

#include "server.h"
#include "config.h"
#include "window.h"
#include "task.h"
#include "panel.h"
#include "tooltip.h"


int signal_pending;
// --------------------------------------------------
// mouse events
int mouse_left;
int mouse_middle;
int mouse_right;
int mouse_scroll_up;
int mouse_scroll_down;
int mouse_tilt_left;
int mouse_tilt_right;

int panel_mode;
int wm_menu;
int panel_dock;
int panel_layer;
int panel_position;
int panel_horizontal;
int panel_refresh;
int task_dragged;
char *panel_window_name = NULL;

int panel_autohide;
int panel_autohide_show_timeout;
int panel_autohide_hide_timeout;
int panel_autohide_height;
int panel_strut_policy;
char *panel_items_order;

int  max_tick_urgent;

// panel's initial config
Panel panel_config;
// panels (one panel per monitor)
Panel *panel1;
int  nb_panel;

GArray* backgrounds;

Imlib_Image default_icon;

void default_panel()
{
	panel1 = 0;
	nb_panel = 0;
	default_icon = NULL;
	task_dragged = 0;
	panel_horizontal = 1;
	panel_position = CENTER;
	panel_items_order = 0;
	panel_autohide = 0;
	panel_autohide_show_timeout = 0;
	panel_autohide_hide_timeout = 0;
	panel_autohide_height = 5;  // for vertical panels this is of course the width
	panel_strut_policy = STRUT_FOLLOW_SIZE;
	panel_dock = 0;  // default not in the dock
	panel_layer = BOTTOM_LAYER;  // default is bottom layer
	panel_window_name = strdup("tint2");
	wm_menu = 0;
	max_tick_urgent = 14;
	mouse_left = TOGGLE_ICONIFY;
	backgrounds = g_array_new(0, 0, sizeof(Background));

	memset(&panel_config, 0, sizeof(Panel));

	// append full transparency background
	Background transparent_bg;
	memset(&transparent_bg, 0, sizeof(Background));
	g_array_append_val(backgrounds, transparent_bg);
}

void cleanup_panel()
{
	if (!panel1) return;

	cleanup_taskbar();
	// taskbarname_font_desc freed here because cleanup_taskbarname() called on _NET_NUMBER_OF_DESKTOPS
	if (taskbarname_font_desc)	pango_font_description_free(taskbarname_font_desc);

	int i;
	Panel *p;
	for (i=0 ; i < nb_panel ; i++) {
		p = &panel1[i];

		free_area(&p->area);
		if (p->temp_pmap) XFreePixmap(server.dsp, p->temp_pmap);
		if (p->hidden_pixmap) XFreePixmap(server.dsp, p->hidden_pixmap);
		if (p->main_win) XDestroyWindow(server.dsp, p->main_win);
	}

	if (panel_items_order) g_free(panel_items_order);
	free(panel_window_name);
	if (panel1) free(panel1);
	if (backgrounds)
		g_array_free(backgrounds, 1);
	if (panel_config.g_task.font_desc) pango_font_description_free(panel_config.g_task.font_desc);
}

void init_panel()
{
	int i, k;
	Panel *p;

	if (panel_config.monitor > (server.nb_monitor-1)) {
		// server.nb_monitor minimum value is 1 (see get_monitors())
		fprintf(stderr, "warning : monitor not found. tint2 default to all monitors.\n");
		panel_config.monitor = 0;
	}

	init_tooltip();
	init_systray();
	init_launcher();
	init_clock();
#ifdef ENABLE_BATTERY
	init_battery();
#endif
	init_taskbar();

	// number of panels (one monitor or 'all' monitors)
	if (panel_config.monitor >= 0)
		nb_panel = 1;
	else
		nb_panel = server.nb_monitor;

	panel1 = malloc(nb_panel * sizeof(Panel));
	for (i=0 ; i < nb_panel ; i++) {
		memcpy(&panel1[i], &panel_config, sizeof(Panel));
	}

	fprintf(stderr, "tint2 : nb monitor %d, nb monitor used %d, nb desktop %d\n", server.nb_monitor, nb_panel, server.nb_desktop);
	for (i=0 ; i < nb_panel ; i++) {
		p = &panel1[i];

		if (panel_config.monitor < 0)
			p->monitor = i;
		if ( p->area.bg == 0 )
			p->area.bg = &g_array_index(backgrounds, Background, 0);
		p->area.parent = p;
		p->area.panel = p;
		p->area.on_screen = 1;
		p->area.resize = 1;
		p->area.size_mode = SIZE_BY_LAYOUT;
		p->area._resize = resize_panel;
		init_panel_size_and_position(p);
		// add childs according to panel_items
		for (k=0 ; k < strlen(panel_items_order) ; k++) {
			if (panel_items_order[k] == 'L') 
				init_launcher_panel(p);
			if (panel_items_order[k] == 'T')
				init_taskbar_panel(p);
#ifdef ENABLE_BATTERY
			if (panel_items_order[k] == 'B')
				init_battery_panel(p);
#endif
			if (panel_items_order[k] == 'S' && systray_on_monitor(i, nb_panel)) {
				init_systray_panel(p);
				refresh_systray = 1;
			}
			if (panel_items_order[k] == 'C')
				init_clock_panel(p);
		}
		set_panel_items_order(p);

		// catch some events
		XSetWindowAttributes att = { .colormap=server.colormap, .background_pixel=0, .border_pixel=0 };
		unsigned long mask = CWEventMask|CWColormap|CWBackPixel|CWBorderPixel;
		p->main_win = XCreateWindow(server.dsp, server.root_win, p->posx, p->posy, p->area.width, p->area.height, 0, server.depth, InputOutput, server.visual, mask, &att);

		long event_mask = ExposureMask|ButtonPressMask|ButtonReleaseMask|ButtonMotionMask;
		if (p->g_task.tooltip_enabled || p->clock.area._get_tooltip_text || (launcher_enabled && launcher_tooltip_enabled))
			event_mask |= PointerMotionMask|LeaveWindowMask;
		if (panel_autohide)
			event_mask |= LeaveWindowMask|EnterWindowMask;
		XChangeWindowAttributes(server.dsp, p->main_win, CWEventMask, &(XSetWindowAttributes){.event_mask=event_mask});

		if (!server.gc) {
			XGCValues  gcv;
			server.gc = XCreateGC(server.dsp, p->main_win, 0, &gcv);
		}
		//printf("panel %d : %d, %d, %d, %d\n", i, p->posx, p->posy, p->area.width, p->area.height);
		set_panel_properties(p);
		set_panel_background(p);
		if (snapshot_path == 0) {
			// if we are not in 'snapshot' mode then map new panel
			XMapWindow (server.dsp, p->main_win);
		}

		if (panel_autohide)
			add_timeout(panel_autohide_hide_timeout, 0, autohide_hide, p);
		
		visible_taskbar(p);
	}

	task_refresh_tasklist();
	active_task();
}


void init_panel_size_and_position(Panel *panel)
{
	// detect panel size
	if (panel_horizontal) {
		if (panel->pourcentx)
			panel->area.width = (float)server.monitor[panel->monitor].width * panel->area.width / 100;
		if (panel->pourcenty)
			panel->area.height = (float)server.monitor[panel->monitor].height * panel->area.height / 100;
		if (panel->area.width + panel->marginx > server.monitor[panel->monitor].width)
			panel->area.width = server.monitor[panel->monitor].width - panel->marginx;
		if (panel->area.bg->border.rounded > panel->area.height/2) {
			printf("panel_background_id rounded is too big... please fix your tint2rc\n");
			g_array_append_val(backgrounds, *panel->area.bg);
			panel->area.bg = &g_array_index(backgrounds, Background, backgrounds->len-1);
			panel->area.bg->border.rounded = panel->area.height/2;
		}
	}
	else {
		int old_panel_height = panel->area.height;
		if (panel->pourcentx)
			panel->area.height = (float)server.monitor[panel->monitor].height * panel->area.width / 100;
		else
			panel->area.height = panel->area.width;
		if (panel->pourcenty)
			panel->area.width = (float)server.monitor[panel->monitor].width * old_panel_height / 100;
		else
			panel->area.width = old_panel_height;
		if (panel->area.height + panel->marginy > server.monitor[panel->monitor].height)
			panel->area.height = server.monitor[panel->monitor].height - panel->marginy;
		if (panel->area.bg->border.rounded > panel->area.width/2) {
			printf("panel_background_id rounded is too big... please fix your tint2rc\n");
			g_array_append_val(backgrounds, *panel->area.bg);
			panel->area.bg = &g_array_index(backgrounds, Background, backgrounds->len-1);
			panel->area.bg->border.rounded = panel->area.width/2;
		}
	}

	// panel position determined here
	if (panel_position & LEFT) {
		panel->posx = server.monitor[panel->monitor].x + panel->marginx;
	}
	else {
		if (panel_position & RIGHT) {
			panel->posx = server.monitor[panel->monitor].x + server.monitor[panel->monitor].width - panel->area.width - panel->marginx;
		}
		else {
			if (panel_horizontal)
				panel->posx = server.monitor[panel->monitor].x + ((server.monitor[panel->monitor].width - panel->area.width) / 2);
			else
				panel->posx = server.monitor[panel->monitor].x + panel->marginx;
		}
	}
	if (panel_position & TOP) {
		panel->posy = server.monitor[panel->monitor].y + panel->marginy;
	}
	else {
		if (panel_position & BOTTOM) {
			panel->posy = server.monitor[panel->monitor].y + server.monitor[panel->monitor].height - panel->area.height - panel->marginy;
		}
		else {
			panel->posy = server.monitor[panel->monitor].y + ((server.monitor[panel->monitor].height - panel->area.height) / 2);
		}
	}

	// autohide or strut_policy=minimum
	int diff = (panel_horizontal ? panel->area.height : panel->area.width) - panel_autohide_height;
	if (panel_horizontal) {
		panel->hidden_width = panel->area.width;
		panel->hidden_height = panel->area.height - diff;
	}
	else {
		panel->hidden_width = panel->area.width - diff;
		panel->hidden_height = panel->area.height;
	}
	// printf("panel : posx %d, posy %d, width %d, height %d\n", panel->posx, panel->posy, panel->area.width, panel->area.height);
}


int resize_panel(void *obj)
{
	resize_by_layout(obj, 0);
	
	//printf("resize_panel\n");
	if (panel_mode != MULTI_DESKTOP && taskbar_enabled) {
		// propagate width/height on hidden taskbar
		int i, width, height;
		Panel *panel = (Panel*)obj;
		width = panel->taskbar[server.desktop].area.width;
		height = panel->taskbar[server.desktop].area.height;
		for (i=0 ; i < panel->nb_desktop ; i++) {
			panel->taskbar[i].area.width = width;
			panel->taskbar[i].area.height = height;
			panel->taskbar[i].area.resize = 1;
		}
	}
	if (panel_mode == MULTI_DESKTOP && taskbar_enabled && taskbar_distribute_size) {
		// Distribute the available space between taskbars
		Panel *panel = (Panel*)obj;

		// Compute the total available size, and the total size requested by the taskbars
		int total_size = 0;
		int total_name_size = 0;
		int total_items = 0;
		int i;
		for (i = 0; i < panel->nb_desktop; i++) {
			if (panel_horizontal) {
				total_size += panel->taskbar[i].area.width;
			} else {
				total_size += panel->taskbar[i].area.height;
			}

			Taskbar *taskbar = &panel->taskbar[i];
			GSList *l;
			for (l = taskbar->area.list; l; l = l->next) {
				Area *child = l->data;
				if (!child->on_screen)
					continue;
				total_items++;
			}
			if (taskbarname_enabled) {
				if (taskbar->area.list) {
					total_items--;
					Area *name = taskbar->area.list->data;
					if (panel_horizontal) {
						total_name_size += name->width;
					} else {
						total_name_size += name->height;
					}
				}
			}
		}
		// Distribute the space proportionally to the requested size (that is, to the
		// number of tasks in each taskbar)
		if (total_items) {
			int actual_name_size;
			if (total_name_size <= total_size) {
				actual_name_size = total_name_size / panel->nb_desktop;
			} else {
				actual_name_size = total_size / panel->nb_desktop;
			}
			total_size -= total_name_size;

			for (i = 0; i < panel->nb_desktop; i++) {
				Taskbar *taskbar = &panel->taskbar[i];

				int requested_size = (2 * taskbar->area.bg->border.width) + (2 * taskbar->area.paddingxlr);
				int items = 0;
				GSList *l = taskbar->area.list;
				if (taskbarname_enabled)
					l = l->next;
				for (; l; l = l->next) {
					Area *child = l->data;
					if (!child->on_screen)
						continue;
					items++;
					if (panel_horizontal) {
						requested_size += child->width + taskbar->area.paddingy;
					} else {
						requested_size += child->height + taskbar->area.paddingx;
					}
				}
				if (panel_horizontal) {
					requested_size -= taskbar->area.paddingy;
				} else {
					requested_size -= taskbar->area.paddingx;
				}

				if (panel_horizontal) {
					taskbar->area.width = actual_name_size + items / (float)total_items * total_size;
				} else {
					taskbar->area.height = actual_name_size + items / (float)total_items * total_size;
				}
				taskbar->area.resize = 1;
			}
		}
	}
	return 0;
}


void update_strut(Panel* p)
{
	if (panel_strut_policy == STRUT_NONE) {
		XDeleteProperty(server.dsp, p->main_win, server.atom._NET_WM_STRUT);
		XDeleteProperty(server.dsp, p->main_win, server.atom._NET_WM_STRUT_PARTIAL);
		return;
	}

	// Reserved space
	unsigned int d1, screen_width, screen_height;
	Window d2;
	int d3;
	XGetGeometry(server.dsp, server.root_win, &d2, &d3, &d3, &screen_width, &screen_height, &d1, &d1);
	Monitor monitor = server.monitor[p->monitor];
	long   struts [12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	if (panel_horizontal) {
		int height = p->area.height + p->marginy;
		if (panel_strut_policy == STRUT_MINIMUM || (panel_strut_policy == STRUT_FOLLOW_SIZE && p->is_hidden))
			height = p->hidden_height;
		if (panel_position & TOP) {
			struts[2] = height + monitor.y;
			struts[8] = p->posx;
			// p->area.width - 1 allowed full screen on monitor 2
			struts[9] = p->posx + p->area.width - 1;
		}
		else {
			struts[3] = height + screen_height - monitor.y - monitor.height;
			struts[10] = p->posx;
			// p->area.width - 1 allowed full screen on monitor 2
			struts[11] = p->posx + p->area.width - 1;
		}
	}
	else {
		int width = p->area.width + p->marginx;
		if (panel_strut_policy == STRUT_MINIMUM || (panel_strut_policy == STRUT_FOLLOW_SIZE && p->is_hidden))
			width = p->hidden_width;
		if (panel_position & LEFT) {
			struts[0] = width + monitor.x;
			struts[4] = p->posy;
			// p->area.width - 1 allowed full screen on monitor 2
			struts[5] = p->posy + p->area.height - 1;
		}
		else {
			struts[1] = width + screen_width - monitor.x - monitor.width;
			struts[6] = p->posy;
			// p->area.width - 1 allowed full screen on monitor 2
			struts[7] = p->posy + p->area.height - 1;
		}
	}
	// Old specification : fluxbox need _NET_WM_STRUT.
	XChangeProperty (server.dsp, p->main_win, server.atom._NET_WM_STRUT, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &struts, 4);
	XChangeProperty (server.dsp, p->main_win, server.atom._NET_WM_STRUT_PARTIAL, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &struts, 12);
}


void set_panel_items_order(Panel *p)
{
	int k, j;
	
	if (p->area.list) {
		g_slist_free(p->area.list);
		p->area.list = 0;
	}

	for (k=0 ; k < strlen(panel_items_order) ; k++) {
		if (panel_items_order[k] == 'L') {
			p->area.list = g_slist_append(p->area.list, &p->launcher);
			p->launcher.area.resize = 1;
		}
		if (panel_items_order[k] == 'T') {
			for (j=0 ; j < p->nb_desktop ; j++)
				p->area.list = g_slist_append(p->area.list, &p->taskbar[j]);
		}
#ifdef ENABLE_BATTERY
		if (panel_items_order[k] == 'B') 
			p->area.list = g_slist_append(p->area.list, &p->battery);
#endif
		int i = p - panel1;
		if (panel_items_order[k] == 'S' && systray_on_monitor(i, nb_panel)) {
			p->area.list = g_slist_append(p->area.list, &systray);
		}
		if (panel_items_order[k] == 'C')
			p->area.list = g_slist_append(p->area.list, &p->clock);
	}
	init_rendering(&p->area, 0);
}


void set_panel_properties(Panel *p)
{
	XStoreName (server.dsp, p->main_win, panel_window_name);
	XSetIconName (server.dsp, p->main_win, panel_window_name);

	gsize len;
	gchar *name = g_locale_to_utf8(panel_window_name, -1, NULL, &len, NULL);
	if (name != NULL) {
		XChangeProperty(server.dsp, p->main_win, server.atom._NET_WM_NAME, server.atom.UTF8_STRING, 8, PropModeReplace, (unsigned char *) name, (int) len);
		XChangeProperty(server.dsp, p->main_win, server.atom._NET_WM_ICON_NAME, server.atom.UTF8_STRING, 8, PropModeReplace, (unsigned char *) name, (int) len);
		g_free(name);
	}

	// Dock
	long val = panel_dock ? server.atom._NET_WM_WINDOW_TYPE_DOCK : server.atom._NET_WM_WINDOW_TYPE_SPLASH;
	XChangeProperty (server.dsp, p->main_win, server.atom._NET_WM_WINDOW_TYPE, XA_ATOM, 32, PropModeReplace, (unsigned char *) &val, 1);

	val = ALLDESKTOP;
	XChangeProperty (server.dsp, p->main_win, server.atom._NET_WM_DESKTOP, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &val, 1);

	Atom state[4];
	state[0] = server.atom._NET_WM_STATE_SKIP_PAGER;
	state[1] = server.atom._NET_WM_STATE_SKIP_TASKBAR;
	state[2] = server.atom._NET_WM_STATE_STICKY;
	state[3] = panel_layer == BOTTOM_LAYER ? server.atom._NET_WM_STATE_BELOW : server.atom._NET_WM_STATE_ABOVE;
	int nb_atoms = panel_layer == NORMAL_LAYER ? 3 : 4;
	XChangeProperty (server.dsp, p->main_win, server.atom._NET_WM_STATE, XA_ATOM, 32, PropModeReplace, (unsigned char *) state, nb_atoms);

	XWMHints wmhints;
	memset(&wmhints, 0, sizeof(wmhints));
	if (panel_dock) {
		// Necessary for placing the panel into the dock on Openbox and Fluxbox.
		// See https://code.google.com/p/tint2/issues/detail?id=465
		wmhints.icon_window = wmhints.window_group = p->main_win;
		wmhints.flags = StateHint | IconWindowHint;
		wmhints.initial_state = WithdrawnState;
	}
	// We do not need keyboard input focus.
	wmhints.flags |= InputHint;
	wmhints.input = False;
	XSetWMHints(server.dsp, p->main_win, &wmhints);

	// Undecorated
	long prop[5] = { 2, 0, 0, 0, 0 };
	XChangeProperty(server.dsp, p->main_win, server.atom._MOTIF_WM_HINTS, server.atom._MOTIF_WM_HINTS, 32, PropModeReplace, (unsigned char *) prop, 5);

	// XdndAware - Register for Xdnd events
	Atom version=4;
	XChangeProperty(server.dsp, p->main_win, server.atom.XdndAware, XA_ATOM, 32, PropModeReplace, (unsigned char*)&version, 1);

	update_strut(p);

	// Fixed position and non-resizable window
	// Allow panel move and resize when tint2 reload config file
	int minwidth = panel_autohide ? p->hidden_width : p->area.width;
	int minheight = panel_autohide ? p->hidden_height : p->area.height;
	XSizeHints size_hints;
	size_hints.flags = PPosition|PMinSize|PMaxSize;
	size_hints.min_width = minwidth;
	size_hints.max_width = p->area.width;
	size_hints.min_height = minheight;
	size_hints.max_height = p->area.height;
	XSetWMNormalHints(server.dsp, p->main_win, &size_hints);

	// Set WM_CLASS
	XClassHint* classhint = XAllocClassHint();
	classhint->res_name = "tint2";
	classhint->res_class = "Tint2";
	XSetClassHint(server.dsp, p->main_win, classhint);
	XFree(classhint);
}


void set_panel_background(Panel *p)
{
	if (p->area.pix) XFreePixmap (server.dsp, p->area.pix);
	p->area.pix = XCreatePixmap (server.dsp, server.root_win, p->area.width, p->area.height, server.depth);

	int xoff=0, yoff=0;
	if (panel_horizontal && panel_position & BOTTOM)
		yoff = p->area.height-p->hidden_height;
	else if (!panel_horizontal && panel_position & RIGHT)
		xoff = p->area.width-p->hidden_width;

	if (server.real_transparency) {
		clear_pixmap(p->area.pix, 0, 0, p->area.width, p->area.height);
	}
	else {
		get_root_pixmap();
		// copy background (server.root_pmap) in panel.area.pix
		Window dummy;
		int  x, y;
		XTranslateCoordinates(server.dsp, p->main_win, server.root_win, 0, 0, &x, &y, &dummy);
		if (panel_autohide && p->is_hidden) {
			x -= xoff;
			y -= yoff;
		}
		XSetTSOrigin(server.dsp, server.gc, -x, -y);
		XFillRectangle(server.dsp, p->area.pix, server.gc, 0, 0, p->area.width, p->area.height);
	}

	// draw background panel
	cairo_surface_t *cs;
	cairo_t *c;
	cs = cairo_xlib_surface_create (server.dsp, p->area.pix, server.visual, p->area.width, p->area.height);
	c = cairo_create (cs);
	draw_background(&p->area, c);
	cairo_destroy (c);
	cairo_surface_destroy (cs);

	if (panel_autohide) {
		if (p->hidden_pixmap) XFreePixmap(server.dsp, p->hidden_pixmap);
		p->hidden_pixmap = XCreatePixmap(server.dsp, server.root_win, p->hidden_width, p->hidden_height, server.depth);
		XCopyArea(server.dsp, p->area.pix, p->hidden_pixmap, server.gc, xoff, yoff, p->hidden_width, p->hidden_height, 0, 0);
	}

	// redraw panel's object
	GSList *l0;
	Area *a;
	for (l0 = p->area.list; l0 ; l0 = l0->next) {
		a = l0->data;
		set_redraw(a);
	}
	
	// reset task/taskbar 'state_pix'
	int i, k;
	Taskbar *tskbar;
	for (i=0 ; i < p->nb_desktop ; i++) {
		tskbar = &p->taskbar[i];
		for (k=0; k<TASKBAR_STATE_COUNT; ++k) {
			if (tskbar->state_pix[k]) XFreePixmap(server.dsp, tskbar->state_pix[k]);
			tskbar->state_pix[k] = 0;
			if (tskbar->bar_name.state_pix[k]) XFreePixmap(server.dsp, tskbar->bar_name.state_pix[k]);
			tskbar->bar_name.state_pix[k] = 0;
		}
		tskbar->area.pix = 0;
		tskbar->bar_name.area.pix = 0;
		l0 = tskbar->area.list;
		if (taskbarname_enabled) l0 = l0->next;
		for (; l0 ; l0 = l0->next) {
			set_task_redraw((Task *)l0->data);
		}
	}
}


Panel *get_panel(Window win)
{
	int i;
	for (i=0 ; i < nb_panel ; i++) {
		if (panel1[i].main_win == win) {
			return &panel1[i];
		}
	}
	return 0;
}


Taskbar *click_taskbar (Panel *panel, int x, int y)
{
	Taskbar *tskbar;
	int i;

	if (panel_horizontal) {
		for (i=0; i < panel->nb_desktop ; i++) {
			tskbar = &panel->taskbar[i];
			if (tskbar->area.on_screen && x >= tskbar->area.posx && x <= (tskbar->area.posx + tskbar->area.width))
				return tskbar;
		}
	}
	else {
		for (i=0; i < panel->nb_desktop ; i++) {
			tskbar = &panel->taskbar[i];
			if (tskbar->area.on_screen && y >= tskbar->area.posy && y <= (tskbar->area.posy + tskbar->area.height))
				return tskbar;
		}
	}
	return NULL;
}


Task *click_task (Panel *panel, int x, int y)
{
	GSList *l0;
	Taskbar *tskbar;

	if ( (tskbar = click_taskbar(panel, x, y)) ) {
		if (panel_horizontal) {
			Task *tsk;
			l0 = tskbar->area.list;
			if (taskbarname_enabled) l0 = l0->next;
			for (; l0 ; l0 = l0->next) {
				tsk = l0->data;
				if (tsk->area.on_screen && x >= tsk->area.posx && x <= (tsk->area.posx + tsk->area.width)) {
					return tsk;
				}
			}
		}
		else {
			Task *tsk;
			l0 = tskbar->area.list;
			if (taskbarname_enabled) l0 = l0->next;
			for (; l0 ; l0 = l0->next) {
				tsk = l0->data;
				if (tsk->area.on_screen && y >= tsk->area.posy && y <= (tsk->area.posy + tsk->area.height)) {
					return tsk;
				}
			}
		}
	}
	return NULL;
}


Launcher *click_launcher (Panel *panel, int x, int y)
{
	Launcher *launcher = &panel->launcher;
	
	if (panel_horizontal) {
		if (launcher->area.on_screen && x >= launcher->area.posx && x <= (launcher->area.posx + launcher->area.width))
			return launcher;
	}
	else {
		if (launcher->area.on_screen && y >= launcher->area.posy && y <= (launcher->area.posy + launcher->area.height))
			return launcher;
	}
	return NULL;
}


LauncherIcon *click_launcher_icon (Panel *panel, int x, int y)
{
	GSList *l0;
	Launcher *launcher;

	//printf("Click x=%d y=%d\n", x, y);
	if ( (launcher = click_launcher(panel, x, y)) ) {
		LauncherIcon *icon;
		for (l0 = launcher->list_icons; l0 ; l0 = l0->next) {
			icon = l0->data;
			if (x >= (launcher->area.posx + icon->x) && x <= (launcher->area.posx + icon->x + icon->icon_size) &&
				y >= (launcher->area.posy + icon->y) && y <= (launcher->area.posy + icon->y + icon->icon_size)) {
				//printf("Hit rect x=%d y=%d xmax=%d ymax=%d\n", launcher->area.posx + icon->x, launcher->area.posy + icon->y, launcher->area.posx + icon->x + icon->width, launcher->area.posy + icon->y + icon->height);
				return icon;
			}
		}
	}
	return NULL;
}


int click_padding(Panel *panel, int x, int y)
{
	if (panel_horizontal) {
		if (x < panel->area.paddingxlr || x > panel->area.width-panel->area.paddingxlr)
		return 1;
	}
	else {
		if (y < panel->area.paddingxlr || y > panel->area.height-panel->area.paddingxlr)
		return 1;
	}
	return 0;
}


int click_clock(Panel *panel, int x, int y)
{
	Clock clk = panel->clock;
	if (panel_horizontal) {
		if (clk.area.on_screen && x >= clk.area.posx && x <= (clk.area.posx + clk.area.width))
			return TRUE;
	} else {
		if (clk.area.on_screen && y >= clk.area.posy && y <= (clk.area.posy + clk.area.height))
			return TRUE;
	}
	return FALSE;
}


Area* click_area(Panel *panel, int x, int y)
{
	Area* result = &panel->area;
	Area* new_result = result;
	do {
		result = new_result;
		GSList* it = result->list;
		while (it) {
			Area* a = it->data;
			if (a->on_screen && x >= a->posx && x <= (a->posx + a->width)
					&& y >= a->posy && y <= (a->posy + a->height)) {
				new_result = a;
				break;
			}
			it = it->next;
		}
	} while (new_result != result);
	return result;
}


void stop_autohide_timeout(Panel* p)
{
	if (p->autohide_timeout) {
		stop_timeout(p->autohide_timeout);
		p->autohide_timeout = 0;
	}
}


void autohide_show(void* p)
{
	Panel* panel = p;
	stop_autohide_timeout(panel);
	panel->is_hidden = 0;
	if (panel_strut_policy == STRUT_FOLLOW_SIZE)
		update_strut(p);

	XMapSubwindows(server.dsp, panel->main_win);  // systray windows
	if (panel_horizontal) {
		if (panel_position & TOP)
			XResizeWindow(server.dsp, panel->main_win, panel->area.width, panel->area.height);
		else
			XMoveResizeWindow(server.dsp, panel->main_win, panel->posx, panel->posy, panel->area.width, panel->area.height);
	}
	else {
		if (panel_position & LEFT)
			XResizeWindow(server.dsp, panel->main_win, panel->area.width, panel->area.height);
		else
			XMoveResizeWindow(server.dsp, panel->main_win, panel->posx, panel->posy, panel->area.width, panel->area.height);
	}
	refresh_systray = 1;   // ugly hack, because we actually only need to call XSetBackgroundPixmap
	panel_refresh = 1;
}


void autohide_hide(void* p)
{
	Panel* panel = p;
	stop_autohide_timeout(panel);
	panel->is_hidden = 1;
	if (panel_strut_policy == STRUT_FOLLOW_SIZE)
		update_strut(p);

	XUnmapSubwindows(server.dsp, panel->main_win);  // systray windows
	int diff = (panel_horizontal ? panel->area.height : panel->area.width) - panel_autohide_height;
	//printf("autohide_hide : diff %d, w %d, h %d\n", diff, panel->hidden_width, panel->hidden_height);
	if (panel_horizontal) {
		if (panel_position & TOP)
			XResizeWindow(server.dsp, panel->main_win, panel->hidden_width, panel->hidden_height);
		else
			XMoveResizeWindow(server.dsp, panel->main_win, panel->posx, panel->posy+diff, panel->hidden_width, panel->hidden_height);
	}
	else {
		if (panel_position & LEFT)
			XResizeWindow(server.dsp, panel->main_win, panel->hidden_width, panel->hidden_height);
		else
			XMoveResizeWindow(server.dsp, panel->main_win, panel->posx+diff, panel->posy, panel->hidden_width, panel->hidden_height);
	}
	panel_refresh = 1;
}


void autohide_trigger_show(Panel* p)
{
	if (!p)
		return;
	if (p->autohide_timeout)
		change_timeout(p->autohide_timeout, panel_autohide_show_timeout, 0, autohide_show, p);
	else
		p->autohide_timeout = add_timeout(panel_autohide_show_timeout, 0, autohide_show, p);
}


void autohide_trigger_hide(Panel* p)
{
	if (!p)
		return;

	Window root, child;
	int xr, yr, xw, yw;
	unsigned int mask;
	if (XQueryPointer(server.dsp, p->main_win, &root, &child, &xr, &yr, &xw, &yw, &mask))
		if (child) return;  // mouse over one of the system tray icons

	if (p->autohide_timeout)
		change_timeout(p->autohide_timeout, panel_autohide_hide_timeout, 0, autohide_hide, p);
	else
		p->autohide_timeout = add_timeout(panel_autohide_hide_timeout, 0, autohide_hide, p);
}
