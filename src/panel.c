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

#include <glib.h>
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

MouseAction mouse_left;
MouseAction mouse_middle;
MouseAction mouse_right;
MouseAction mouse_scroll_up;
MouseAction mouse_scroll_down;
MouseAction mouse_tilt_left;
MouseAction mouse_tilt_right;

TaskbarMode taskbar_mode;
gboolean wm_menu;
gboolean panel_dock;
Layer panel_layer;
PanelPosition panel_position;
gboolean panel_horizontal;
gboolean panel_refresh;
gboolean task_dragged;
char *panel_window_name = NULL;

gboolean panel_autohide;
int panel_autohide_show_timeout;
int panel_autohide_hide_timeout;
int panel_autohide_height;
Strut panel_strut_policy;
char *panel_items_order;

int max_tick_urgent;

// panel's initial config
Panel panel_config;
// panels (one panel per monitor)
Panel *panels;
int num_panels;

GArray *backgrounds;

Imlib_Image default_icon;
char *default_font = NULL;

void default_panel()
{
	panels = NULL;
	num_panels = 0;
	default_icon = NULL;
	task_dragged = FALSE;
	panel_horizontal = TRUE;
	panel_position = CENTER;
	panel_items_order = NULL;
	panel_autohide = FALSE;
	panel_autohide_show_timeout = 0;
	panel_autohide_hide_timeout = 0;
	panel_autohide_height = 5; // for vertical panels this is of course the width
	panel_strut_policy = STRUT_FOLLOW_SIZE;
	panel_dock = FALSE;         // default not in the dock
	panel_layer = BOTTOM_LAYER; // default is bottom layer
	panel_window_name = strdup("tint2");
	wm_menu = FALSE;
	max_tick_urgent = 14;
	mouse_left = TOGGLE_ICONIFY;
	backgrounds = g_array_new(0, 0, sizeof(Background));

	memset(&panel_config, 0, sizeof(Panel));
	panel_config.mouse_over_alpha = 100;
	panel_config.mouse_over_saturation = 0;
	panel_config.mouse_over_brightness = 10;
	panel_config.mouse_pressed_alpha = 100;
	panel_config.mouse_pressed_saturation = 0;
	panel_config.mouse_pressed_brightness = 0;

	// First background is always fully transparent
	Background transparent_bg;
	init_background(&transparent_bg);
	g_array_append_val(backgrounds, transparent_bg);
}

void cleanup_panel()
{
	if (!panels)
		return;

	cleanup_taskbar();

	for (int i = 0; i < num_panels; i++) {
		Panel *p = &panels[i];

		free_area(&p->area);
		if (p->temp_pmap)
			XFreePixmap(server.dsp, p->temp_pmap);
		p->temp_pmap = 0;
		if (p->hidden_pixmap)
			XFreePixmap(server.dsp, p->hidden_pixmap);
		p->hidden_pixmap = 0;
		if (p->main_win)
			XDestroyWindow(server.dsp, p->main_win);
		p->main_win = 0;
		stop_timeout(p->autohide_timeout);
	}

	free(panel_items_order);
	panel_items_order = NULL;
	free(panel_window_name);
	panel_window_name = NULL;
	free(panels);
	panels = NULL;
	if (backgrounds)
		g_array_free(backgrounds, 1);
	backgrounds = NULL;
	pango_font_description_free(panel_config.g_task.font_desc);
	panel_config.g_task.font_desc = NULL;
	pango_font_description_free(panel_config.taskbarname_font_desc);
	panel_config.taskbarname_font_desc = NULL;
}

void init_panel()
{
	if (panel_config.monitor > (server.num_monitors - 1)) {
		// server.num_monitors minimum value is 1 (see get_monitors())
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
	init_execp();

	// number of panels (one monitor or 'all' monitors)
	if (panel_config.monitor >= 0)
		num_panels = 1;
	else
		num_panels = server.num_monitors;

	panels = calloc(num_panels, sizeof(Panel));
	for (int i = 0; i < num_panels; i++) {
		memcpy(&panels[i], &panel_config, sizeof(Panel));
	}

	fprintf(stderr,
			"tint2 : nb monitor %d, nb monitor used %d, nb desktop %d\n",
			server.num_monitors,
			num_panels,
			server.num_desktops);
	for (int i = 0; i < num_panels; i++) {
		Panel *p = &panels[i];

		if (panel_config.monitor < 0)
			p->monitor = i;
		if (!p->area.bg)
			p->area.bg = &g_array_index(backgrounds, Background, 0);
		p->area.parent = p;
		p->area.panel = p;
		p->area.on_screen = TRUE;
		p->area.resize_needed = 1;
		p->area.size_mode = LAYOUT_DYNAMIC;
		p->area._resize = resize_panel;
		init_panel_size_and_position(p);
		// add children according to panel_items
		for (int k = 0; k < strlen(panel_items_order); k++) {
			if (panel_items_order[k] == 'L')
				init_launcher_panel(p);
			if (panel_items_order[k] == 'T')
				init_taskbar_panel(p);
#ifdef ENABLE_BATTERY
			if (panel_items_order[k] == 'B')
				init_battery_panel(p);
#endif
			if (panel_items_order[k] == 'S' && systray_on_monitor(i, num_panels)) {
				init_systray_panel(p);
				refresh_systray = 1;
			}
			if (panel_items_order[k] == 'C')
				init_clock_panel(p);
			if (panel_items_order[k] == 'F' && !strstr(panel_items_order, "T"))
				init_freespace_panel(p);
			if (panel_items_order[k] == 'E')
				init_execp_panel(p);
		}
		set_panel_items_order(p);

		// catch some events
		XSetWindowAttributes att = {.colormap = server.colormap, .background_pixel = 0, .border_pixel = 0};
		unsigned long mask = CWEventMask | CWColormap | CWBackPixel | CWBorderPixel;
		p->main_win = XCreateWindow(server.dsp,
									server.root_win,
									p->posx,
									p->posy,
									p->area.width,
									p->area.height,
									0,
									server.depth,
									InputOutput,
									server.visual,
									mask,
									&att);

		long event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;
		if (p->mouse_effects || p->g_task.tooltip_enabled || p->clock.area._get_tooltip_text ||
			(launcher_enabled && launcher_tooltip_enabled))
			event_mask |= PointerMotionMask | LeaveWindowMask;
		if (panel_autohide)
			event_mask |= LeaveWindowMask | EnterWindowMask;
		XChangeWindowAttributes(server.dsp, p->main_win, CWEventMask, &(XSetWindowAttributes){.event_mask = event_mask});

		if (!server.gc) {
			XGCValues gcv;
			server.gc = XCreateGC(server.dsp, p->main_win, 0, &gcv);
		}
		// printf("panel %d : %d, %d, %d, %d\n", i, p->posx, p->posy, p->area.width, p->area.height);
		set_panel_properties(p);
		set_panel_background(p);
		if (!snapshot_path) {
			// if we are not in 'snapshot' mode then map new panel
			XMapWindow(server.dsp, p->main_win);
		}

		if (panel_autohide)
			autohide_trigger_hide(p);

		visible_taskbar(p);
	}

	task_refresh_tasklist();
	reset_active_task();
}

void init_panel_size_and_position(Panel *panel)
{
	// detect panel size
	if (panel_horizontal) {
		if (panel->fractional_width)
			panel->area.width = (float)server.monitor[panel->monitor].width * panel->area.width / 100;
		if (panel->fractional_height)
			panel->area.height = (float)server.monitor[panel->monitor].height * panel->area.height / 100;
		if (panel->area.width + panel->marginx > server.monitor[panel->monitor].width)
			panel->area.width = server.monitor[panel->monitor].width - panel->marginx;
		if (panel->area.bg->border.radius > panel->area.height / 2) {
			printf("panel_background_id rounded is too big... please fix your tint2rc\n");
			g_array_append_val(backgrounds, *panel->area.bg);
			panel->area.bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
			panel->area.bg->border.radius = panel->area.height / 2;
		}
	} else {
		int old_panel_height = panel->area.height;
		if (panel->fractional_width)
			panel->area.height = (float)server.monitor[panel->monitor].height * panel->area.width / 100;
		else
			panel->area.height = panel->area.width;

		if (panel->fractional_height)
			panel->area.width = (float)server.monitor[panel->monitor].width * old_panel_height / 100;
		else
			panel->area.width = old_panel_height;

		if (panel->area.height + panel->marginy > server.monitor[panel->monitor].height)
			panel->area.height = server.monitor[panel->monitor].height - panel->marginy;

		if (panel->area.bg->border.radius > panel->area.width / 2) {
			printf("panel_background_id rounded is too big... please fix your tint2rc\n");
			g_array_append_val(backgrounds, *panel->area.bg);
			panel->area.bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
			panel->area.bg->border.radius = panel->area.width / 2;
		}
	}

	// panel position determined here
	if (panel_position & LEFT) {
		panel->posx = server.monitor[panel->monitor].x + panel->marginx;
	} else {
		if (panel_position & RIGHT) {
			panel->posx = server.monitor[panel->monitor].x + server.monitor[panel->monitor].width - panel->area.width -
						  panel->marginx;
		} else {
			if (panel_horizontal)
				panel->posx =
				server.monitor[panel->monitor].x + ((server.monitor[panel->monitor].width - panel->area.width) / 2);
			else
				panel->posx = server.monitor[panel->monitor].x + panel->marginx;
		}
	}
	if (panel_position & TOP) {
		panel->posy = server.monitor[panel->monitor].y + panel->marginy;
	} else {
		if (panel_position & BOTTOM) {
			panel->posy = server.monitor[panel->monitor].y + server.monitor[panel->monitor].height -
						  panel->area.height - panel->marginy;
		} else {
			panel->posy =
			server.monitor[panel->monitor].y + ((server.monitor[panel->monitor].height - panel->area.height) / 2);
		}
	}

	// autohide or strut_policy=minimum
	int diff = (panel_horizontal ? panel->area.height : panel->area.width) - panel_autohide_height;
	if (panel_horizontal) {
		panel->hidden_width = panel->area.width;
		panel->hidden_height = panel->area.height - diff;
	} else {
		panel->hidden_width = panel->area.width - diff;
		panel->hidden_height = panel->area.height;
	}
	// printf("panel : posx %d, posy %d, width %d, height %d\n", panel->posx, panel->posy, panel->area.width,
	// panel->area.height);
}

gboolean resize_panel(void *obj)
{
	Panel *panel = (Panel *)obj;
	relayout_with_constraint(&panel->area, 0);

	// printf("resize_panel\n");
	if (taskbar_mode != MULTI_DESKTOP && taskbar_enabled) {
		// propagate width/height on hidden taskbar
		int width = panel->taskbar[server.desktop].area.width;
		int height = panel->taskbar[server.desktop].area.height;
		for (int i = 0; i < panel->num_desktops; i++) {
			panel->taskbar[i].area.width = width;
			panel->taskbar[i].area.height = height;
			panel->taskbar[i].area.resize_needed = 1;
		}
	}
	if (taskbar_mode == MULTI_DESKTOP && taskbar_enabled && taskbar_distribute_size) {
		// Distribute the available space between taskbars

		// Compute the total available size, and the total size requested by the taskbars
		int total_size = 0;
		int total_name_size = 0;
		int total_items = 0;
		for (int i = 0; i < panel->num_desktops; i++) {
			if (panel_horizontal) {
				total_size += panel->taskbar[i].area.width;
			} else {
				total_size += panel->taskbar[i].area.height;
			}

			Taskbar *taskbar = &panel->taskbar[i];
			GList *l;
			for (l = taskbar->area.children; l; l = l->next) {
				Area *child = (Area *)l->data;
				if (!child->on_screen)
					continue;
				total_items++;
			}
			if (taskbarname_enabled) {
				if (taskbar->area.children) {
					total_items--;
					Area *name = (Area *)taskbar->area.children->data;
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
				actual_name_size = total_name_size / panel->num_desktops;
			} else {
				actual_name_size = total_size / panel->num_desktops;
			}
			total_size -= total_name_size;

			for (int i = 0; i < panel->num_desktops; i++) {
				Taskbar *taskbar = &panel->taskbar[i];

				int requested_size = (2 * taskbar->area.bg->border.width) + (2 * taskbar->area.paddingxlr);
				int items = 0;
				GList *l = taskbar->area.children;
				if (taskbarname_enabled)
					l = l->next;
				for (; l; l = l->next) {
					Area *child = (Area *)l->data;
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
				taskbar->area.resize_needed = 1;
			}
		}
	}
	if (panel->freespace.area.on_screen)
		resize_freespace(&panel->freespace);
	return FALSE;
}

void update_strut(Panel *p)
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
	long struts[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	if (panel_horizontal) {
		int height = p->area.height + p->marginy;
		if (panel_strut_policy == STRUT_MINIMUM || (panel_strut_policy == STRUT_FOLLOW_SIZE && p->is_hidden))
			height = p->hidden_height;
		if (panel_position & TOP) {
			struts[2] = height + monitor.y;
			struts[8] = p->posx;
			// p->area.width - 1 allowed full screen on monitor 2
			struts[9] = p->posx + p->area.width - 1;
		} else {
			struts[3] = height + screen_height - monitor.y - monitor.height;
			struts[10] = p->posx;
			// p->area.width - 1 allowed full screen on monitor 2
			struts[11] = p->posx + p->area.width - 1;
		}
	} else {
		int width = p->area.width + p->marginx;
		if (panel_strut_policy == STRUT_MINIMUM || (panel_strut_policy == STRUT_FOLLOW_SIZE && p->is_hidden))
			width = p->hidden_width;
		if (panel_position & LEFT) {
			struts[0] = width + monitor.x;
			struts[4] = p->posy;
			// p->area.width - 1 allowed full screen on monitor 2
			struts[5] = p->posy + p->area.height - 1;
		} else {
			struts[1] = width + screen_width - monitor.x - monitor.width;
			struts[6] = p->posy;
			// p->area.width - 1 allowed full screen on monitor 2
			struts[7] = p->posy + p->area.height - 1;
		}
	}
	// Old specification : fluxbox need _NET_WM_STRUT.
	XChangeProperty(server.dsp,
					p->main_win,
					server.atom._NET_WM_STRUT,
					XA_CARDINAL,
					32,
					PropModeReplace,
					(unsigned char *)&struts,
					4);
	XChangeProperty(server.dsp,
					p->main_win,
					server.atom._NET_WM_STRUT_PARTIAL,
					XA_CARDINAL,
					32,
					PropModeReplace,
					(unsigned char *)&struts,
					12);
}

void set_panel_items_order(Panel *p)
{
	if (p->area.children) {
		g_list_free(p->area.children);
		p->area.children = 0;
	}

	int i_execp = 0;
	for (int k = 0; k < strlen(panel_items_order); k++) {
		if (panel_items_order[k] == 'L') {
			p->area.children = g_list_append(p->area.children, &p->launcher);
			p->launcher.area.resize_needed = 1;
		}
		if (panel_items_order[k] == 'T') {
			for (int j = 0; j < p->num_desktops; j++)
				p->area.children = g_list_append(p->area.children, &p->taskbar[j]);
		}
#ifdef ENABLE_BATTERY
		if (panel_items_order[k] == 'B')
			p->area.children = g_list_append(p->area.children, &p->battery);
#endif
		int i = p - panels;
		if (panel_items_order[k] == 'S' && systray_on_monitor(i, num_panels)) {
			p->area.children = g_list_append(p->area.children, &systray);
		}
		if (panel_items_order[k] == 'C')
			p->area.children = g_list_append(p->area.children, &p->clock);
		if (panel_items_order[k] == 'F')
			p->area.children = g_list_append(p->area.children, &p->freespace);
		if (panel_items_order[k] == 'E') {
			GList *item = g_list_nth(p->execp_list, i_execp);
			i_execp++;
			if (item)
				p->area.children = g_list_append(p->area.children, (Area*)item->data);
		}
	}
	initialize_positions(&p->area, 0);
}

void set_panel_properties(Panel *p)
{
	XStoreName(server.dsp, p->main_win, panel_window_name);
	XSetIconName(server.dsp, p->main_win, panel_window_name);

	gsize len;
	gchar *name = g_locale_to_utf8(panel_window_name, -1, NULL, &len, NULL);
	if (name != NULL) {
		XChangeProperty(server.dsp,
						p->main_win,
						server.atom._NET_WM_NAME,
						server.atom.UTF8_STRING,
						8,
						PropModeReplace,
						(unsigned char *)name,
						(int)len);
		XChangeProperty(server.dsp,
						p->main_win,
						server.atom._NET_WM_ICON_NAME,
						server.atom.UTF8_STRING,
						8,
						PropModeReplace,
						(unsigned char *)name,
						(int)len);
		g_free(name);
	}

	// Dock
	long val = (!panel_dock && panel_layer == NORMAL_LAYER) ? server.atom._NET_WM_WINDOW_TYPE_SPLASH
															: server.atom._NET_WM_WINDOW_TYPE_DOCK;
	XChangeProperty(server.dsp,
					p->main_win,
					server.atom._NET_WM_WINDOW_TYPE,
					XA_ATOM,
					32,
					PropModeReplace,
					(unsigned char *)&val,
					1);

	val = ALL_DESKTOPS;
	XChangeProperty(server.dsp,
					p->main_win,
					server.atom._NET_WM_DESKTOP,
					XA_CARDINAL,
					32,
					PropModeReplace,
					(unsigned char *)&val,
					1);

	Atom state[4];
	state[0] = server.atom._NET_WM_STATE_SKIP_PAGER;
	state[1] = server.atom._NET_WM_STATE_SKIP_TASKBAR;
	state[2] = server.atom._NET_WM_STATE_STICKY;
	state[3] = panel_layer == BOTTOM_LAYER ? server.atom._NET_WM_STATE_BELOW : server.atom._NET_WM_STATE_ABOVE;
	int num_atoms = panel_layer == NORMAL_LAYER ? 3 : 4;
	XChangeProperty(server.dsp,
					p->main_win,
					server.atom._NET_WM_STATE,
					XA_ATOM,
					32,
					PropModeReplace,
					(unsigned char *)state,
					num_atoms);

	XWMHints wmhints;
	memset(&wmhints, 0, sizeof(wmhints));
	if (panel_dock) {
		// Necessary for placing the panel into the dock on Openbox and Fluxbox.
		// See https://gitlab.com/o9000/tint2/issues/465
		wmhints.flags = IconWindowHint | WindowGroupHint | StateHint;
		wmhints.icon_window = wmhints.window_group = p->main_win;
		wmhints.initial_state = WithdrawnState;
	}
	// We do not need keyboard input focus.
	wmhints.flags |= InputHint;
	wmhints.input = False;
	XSetWMHints(server.dsp, p->main_win, &wmhints);

	// Undecorated
	long prop[5] = {2, 0, 0, 0, 0};
	XChangeProperty(server.dsp,
					p->main_win,
					server.atom._MOTIF_WM_HINTS,
					server.atom._MOTIF_WM_HINTS,
					32,
					PropModeReplace,
					(unsigned char *)prop,
					5);

	// XdndAware - Register for Xdnd events
	Atom version = 4;
	XChangeProperty(server.dsp,
					p->main_win,
					server.atom.XdndAware,
					XA_ATOM,
					32,
					PropModeReplace,
					(unsigned char *)&version,
					1);

	update_strut(p);

	// Fixed position and non-resizable window
	// Allow panel move and resize when tint2 reload config file
	int minwidth = panel_autohide ? p->hidden_width : p->area.width;
	int minheight = panel_autohide ? p->hidden_height : p->area.height;
	XSizeHints size_hints;
	size_hints.flags = PPosition | PMinSize | PMaxSize;
	size_hints.min_width = minwidth;
	size_hints.max_width = p->area.width;
	size_hints.min_height = minheight;
	size_hints.max_height = p->area.height;
	XSetWMNormalHints(server.dsp, p->main_win, &size_hints);

	// Set WM_CLASS
	XClassHint *classhint = XAllocClassHint();
	classhint->res_name = (char *)"tint2";
	classhint->res_class = (char *)"Tint2";
	XSetClassHint(server.dsp, p->main_win, classhint);
	XFree(classhint);
}

void set_panel_background(Panel *p)
{
	if (p->area.pix)
		XFreePixmap(server.dsp, p->area.pix);
	p->area.pix = XCreatePixmap(server.dsp, server.root_win, p->area.width, p->area.height, server.depth);

	int xoff = 0, yoff = 0;
	if (panel_horizontal && panel_position & BOTTOM)
		yoff = p->area.height - p->hidden_height;
	else if (!panel_horizontal && panel_position & RIGHT)
		xoff = p->area.width - p->hidden_width;

	if (server.real_transparency) {
		clear_pixmap(p->area.pix, 0, 0, p->area.width, p->area.height);
	} else {
		get_root_pixmap();
		// copy background (server.root_pmap) in panel.area.pix
		Window dummy;
		int x, y;
		XTranslateCoordinates(server.dsp, p->main_win, server.root_win, 0, 0, &x, &y, &dummy);
		if (panel_autohide && p->is_hidden) {
			x -= xoff;
			y -= yoff;
		}
		XSetTSOrigin(server.dsp, server.gc, -x, -y);
		XFillRectangle(server.dsp, p->area.pix, server.gc, 0, 0, p->area.width, p->area.height);
	}

	// draw background panel
	cairo_surface_t *cs = cairo_xlib_surface_create(server.dsp, p->area.pix, server.visual, p->area.width, p->area.height);
	cairo_t *c = cairo_create(cs);
	draw_background(&p->area, c);
	cairo_destroy(c);
	cairo_surface_destroy(cs);

	if (panel_autohide) {
		if (p->hidden_pixmap)
			XFreePixmap(server.dsp, p->hidden_pixmap);
		p->hidden_pixmap = XCreatePixmap(server.dsp, server.root_win, p->hidden_width, p->hidden_height, server.depth);
		XCopyArea(server.dsp,
				  p->area.pix,
				  p->hidden_pixmap,
				  server.gc,
				  xoff,
				  yoff,
				  p->hidden_width,
				  p->hidden_height,
				  0,
				  0);
	}

	// redraw panel's object
	for (GList *l = p->area.children; l; l = l->next) {
		Area *a = (Area *)l->data;
		schedule_redraw(a);
	}

	// reset task/taskbar 'state_pix'
	Taskbar *taskbar;
	for (int i = 0; i < p->num_desktops; i++) {
		taskbar = &p->taskbar[i];
		for (int k = 0; k < TASKBAR_STATE_COUNT; ++k) {
			if (taskbar->state_pix[k])
				XFreePixmap(server.dsp, taskbar->state_pix[k]);
			taskbar->state_pix[k] = 0;
			if (taskbar->bar_name.state_pix[k])
				XFreePixmap(server.dsp, taskbar->bar_name.state_pix[k]);
			taskbar->bar_name.state_pix[k] = 0;
		}
		taskbar->area.pix = 0;
		taskbar->bar_name.area.pix = 0;
		GList *l = taskbar->area.children;
		if (taskbarname_enabled)
			l = l->next;
		for (; l; l = l->next) {
			set_task_redraw((Task *)l->data);
		}
	}
}

Panel *get_panel(Window win)
{
	for (int i = 0; i < num_panels; i++) {
		if (panels[i].main_win == win) {
			return &panels[i];
		}
	}
	return 0;
}

Taskbar *click_taskbar(Panel *panel, int x, int y)
{
	if (panel_horizontal) {
		for (int i = 0; i < panel->num_desktops; i++) {
			Taskbar *taskbar = &panel->taskbar[i];
			if (taskbar->area.on_screen && x >= taskbar->area.posx && x <= (taskbar->area.posx + taskbar->area.width))
				return taskbar;
		}
	} else {
		for (int i = 0; i < panel->num_desktops; i++) {
			Taskbar *taskbar = &panel->taskbar[i];
			if (taskbar->area.on_screen && y >= taskbar->area.posy && y <= (taskbar->area.posy + taskbar->area.height))
				return taskbar;
		}
	}
	return NULL;
}

Task *click_task(Panel *panel, int x, int y)
{
	Taskbar *taskbar = click_taskbar(panel, x, y);
	if (taskbar) {
		if (panel_horizontal) {
			GList *l = taskbar->area.children;
			if (taskbarname_enabled)
				l = l->next;
			for (; l; l = l->next) {
				Task *task = (Task *)l->data;
				if (task->area.on_screen && x >= task->area.posx && x <= (task->area.posx + task->area.width)) {
					return task;
				}
			}
		} else {
			GList *l = taskbar->area.children;
			if (taskbarname_enabled)
				l = l->next;
			for (; l; l = l->next) {
				Task *task = (Task *)l->data;
				if (task->area.on_screen && y >= task->area.posy && y <= (task->area.posy + task->area.height)) {
					return task;
				}
			}
		}
	}
	return NULL;
}

Launcher *click_launcher(Panel *panel, int x, int y)
{
	Launcher *launcher = &panel->launcher;

	if (panel_horizontal) {
		if (launcher->area.on_screen && x >= launcher->area.posx && x <= (launcher->area.posx + launcher->area.width))
			return launcher;
	} else {
		if (launcher->area.on_screen && y >= launcher->area.posy && y <= (launcher->area.posy + launcher->area.height))
			return launcher;
	}
	return NULL;
}

LauncherIcon *click_launcher_icon(Panel *panel, int x, int y)
{
	Launcher *launcher = click_launcher(panel, x, y);

	if (launcher) {
		for (GSList *l = launcher->list_icons; l; l = l->next) {
			LauncherIcon *icon = (LauncherIcon *)l->data;
			if (x >= (launcher->area.posx + icon->x) && x <= (launcher->area.posx + icon->x + icon->icon_size) &&
				y >= (launcher->area.posy + icon->y) && y <= (launcher->area.posy + icon->y + icon->icon_size)) {
				return icon;
			}
		}
	}
	return NULL;
}

gboolean click_padding(Panel *panel, int x, int y)
{
	if (panel_horizontal) {
		if (x < panel->area.paddingxlr || x > panel->area.width - panel->area.paddingxlr)
			return TRUE;
	} else {
		if (y < panel->area.paddingxlr || y > panel->area.height - panel->area.paddingxlr)
			return TRUE;
	}
	return FALSE;
}

int click_clock(Panel *panel, int x, int y)
{
	Clock *clock = &panel->clock;
	if (panel_horizontal) {
		if (clock->area.on_screen && x >= clock->area.posx && x <= (clock->area.posx + clock->area.width))
			return TRUE;
	} else {
		if (clock->area.on_screen && y >= clock->area.posy && y <= (clock->area.posy + clock->area.height))
			return TRUE;
	}
	return FALSE;
}

#ifdef ENABLE_BATTERY
int click_battery(Panel *panel, int x, int y)
{
	Battery *bat = &panel->battery;
	if (panel_horizontal) {
		if (bat->area.on_screen && x >= bat->area.posx && x <= (bat->area.posx + bat->area.width))
			return TRUE;
	} else {
		if (bat->area.on_screen && y >= bat->area.posy && y <= (bat->area.posy + bat->area.height))
			return TRUE;
	}
	return FALSE;
}
#endif

Execp *click_execp(Panel *panel, int x, int y)
{
	GList *l;
	for (l = panel->execp_list; l; l = l->next) {
		Execp *execp = (Execp *)l->data;
		if (panel_horizontal) {
			if (execp->area.on_screen && x >= execp->area.posx && x <= (execp->area.posx + execp->area.width))
				return execp;
		} else {
			if (execp->area.on_screen && y >= execp->area.posy && y <= (execp->area.posy + execp->area.height))
				return execp;
		}
	}
	return NULL;
}

Area *click_area(Panel *panel, int x, int y)
{
	Area *result = &panel->area;
	Area *new_result = result;
	do {
		result = new_result;
		GList *it = result->children;
		while (it) {
			Area *a = (Area *)it->data;
			if (a->on_screen && x >= a->posx && x <= (a->posx + a->width) && y >= a->posy &&
				y <= (a->posy + a->height)) {
				new_result = a;
				break;
			}
			it = it->next;
		}
	} while (new_result != result);
	return result;
}

void stop_autohide_timeout(Panel *p)
{
	stop_timeout(p->autohide_timeout);
}

void autohide_show(void *p)
{
	Panel *panel = (Panel *)p;
	stop_autohide_timeout(panel);
	panel->is_hidden = 0;

	XMapSubwindows(server.dsp, panel->main_win); // systray windows
	if (panel_horizontal) {
		if (panel_position & TOP)
			XResizeWindow(server.dsp, panel->main_win, panel->area.width, panel->area.height);
		else
			XMoveResizeWindow(server.dsp,
							  panel->main_win,
							  panel->posx,
							  panel->posy,
							  panel->area.width,
							  panel->area.height);
	} else {
		if (panel_position & LEFT)
			XResizeWindow(server.dsp, panel->main_win, panel->area.width, panel->area.height);
		else
			XMoveResizeWindow(server.dsp,
							  panel->main_win,
							  panel->posx,
							  panel->posy,
							  panel->area.width,
							  panel->area.height);
	}
	if (panel_strut_policy == STRUT_FOLLOW_SIZE)
		update_strut(panel);
	refresh_systray = TRUE; // ugly hack, because we actually only need to call XSetBackgroundPixmap
	panel_refresh = TRUE;
}

void autohide_hide(void *p)
{
	Panel *panel = (Panel *)p;
	stop_autohide_timeout(panel);
	panel->is_hidden = TRUE;
	if (panel_strut_policy == STRUT_FOLLOW_SIZE)
		update_strut(panel);

	XUnmapSubwindows(server.dsp, panel->main_win); // systray windows
	int diff = (panel_horizontal ? panel->area.height : panel->area.width) - panel_autohide_height;
	// printf("autohide_hide : diff %d, w %d, h %d\n", diff, panel->hidden_width, panel->hidden_height);
	if (panel_horizontal) {
		if (panel_position & TOP)
			XResizeWindow(server.dsp, panel->main_win, panel->hidden_width, panel->hidden_height);
		else
			XMoveResizeWindow(server.dsp,
							  panel->main_win,
							  panel->posx,
							  panel->posy + diff,
							  panel->hidden_width,
							  panel->hidden_height);
	} else {
		if (panel_position & LEFT)
			XResizeWindow(server.dsp, panel->main_win, panel->hidden_width, panel->hidden_height);
		else
			XMoveResizeWindow(server.dsp,
							  panel->main_win,
							  panel->posx + diff,
							  panel->posy,
							  panel->hidden_width,
							  panel->hidden_height);
	}
	panel_refresh = TRUE;
}

void autohide_trigger_show(Panel *p)
{
	if (!p)
		return;
	change_timeout(&p->autohide_timeout, panel_autohide_show_timeout, 0, autohide_show, p);
}

void autohide_trigger_hide(Panel *p)
{
	if (!p)
		return;

	Window root, child;
	int xr, yr, xw, yw;
	unsigned int mask;
	if (XQueryPointer(server.dsp, p->main_win, &root, &child, &xr, &yr, &xw, &yw, &mask))
		if (child)
			return; // mouse over one of the system tray icons

	change_timeout(&p->autohide_timeout, panel_autohide_hide_timeout, 0, autohide_hide, p);
}

void render_panel(Panel *panel)
{
	relayout(&panel->area);
	draw_tree(&panel->area);
}

const char *get_default_font()
{
	if (default_font)
		return default_font;
	return DEFAULT_FONT;
}

void default_icon_theme_changed()
{
	launcher_default_icon_theme_changed();
}

void default_font_changed()
{
#ifdef ENABLE_BATTERY
	battery_default_font_changed();
#endif
	clock_default_font_changed();
	execp_default_font_changed();
	taskbar_default_font_changed();
	taskbarname_default_font_changed();
	tooltip_default_font_changed();
}
