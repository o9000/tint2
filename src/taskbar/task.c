/**************************************************************************
*
* Tint2 : task
*
* Copyright (C) 2007 Pål Staurland (staura@gmail.com)
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <unistd.h>

#include "window.h"
#include "task.h"
#include "taskbar.h"
#include "server.h"
#include "panel.h"
#include "tooltip.h"
#include "timer.h"

timeout *urgent_timeout;
GSList *urgent_list;

char *task_get_tooltip(void *obj)
{
	Task *t = (Task *)obj;
	return strdup(t->title);
}

Task *add_task(Window win)
{
	if (!win)
		return 0;
	if (window_is_hidden(win))
		return 0;

	XSelectInput(server.dsp, win, PropertyChangeMask | StructureNotifyMask);
	XFlush(server.dsp);

	int monitor;
	if (num_panels > 1) {
		monitor = get_window_monitor(win);
		if (monitor >= num_panels)
			monitor = 0;
	} else
		monitor = 0;

	Task new_task;
	memset(&new_task, 0, sizeof(new_task));
	new_task.area.has_mouse_over_effect = 1;
	new_task.area.has_mouse_press_effect = 1;
	new_task.win = win;
	new_task.desktop = get_window_desktop(win);
	new_task.area.panel = &panels[monitor];
	new_task.current_state = window_is_iconified(win) ? TASK_ICONIFIED : TASK_NORMAL;
	get_window_coordinates(win, &new_task.win_x, &new_task.win_y, &new_task.win_w, &new_task.win_h);

	// allocate only one title and one icon
	// even with task_on_all_desktop and with task_on_all_panel
	new_task.title = 0;
	int k;
	for (k = 0; k < TASK_STATE_COUNT; ++k) {
		new_task.icon[k] = 0;
		new_task.state_pix[k] = 0;
	}
	get_title(&new_task);
	get_icon(&new_task);

	// printf("new task %s win %u: desktop %d, monitor %d\n", new_task.title, win, new_task.desktop, monitor);

	GPtrArray *task_group = g_ptr_array_new();
	Taskbar *taskbar;
	Task *new_task2 = 0;
	int j;
	for (j = 0; j < panels[monitor].num_desktops; j++) {
		if (new_task.desktop != ALLDESKTOP && new_task.desktop != j)
			continue;

		taskbar = &panels[monitor].taskbar[j];
		new_task2 = calloc(1, sizeof(Task));
		memcpy(&new_task2->area, &panels[monitor].g_task.area, sizeof(Area));
		new_task2->area.parent = taskbar;
		new_task2->area.has_mouse_over_effect = 1;
		new_task2->area.has_mouse_press_effect = 1;
		new_task2->win = new_task.win;
		new_task2->desktop = new_task.desktop;
		new_task2->win_x = new_task.win_x;
		new_task2->win_y = new_task.win_y;
		new_task2->win_w = new_task.win_w;
		new_task2->win_h = new_task.win_h;
		new_task2->current_state = -1; // to update the current state later in set_task_state...
		if (new_task2->desktop == ALLDESKTOP && server.desktop != j) {
			// hide ALLDESKTOP task on non-current desktop
			new_task2->area.on_screen = FALSE;
		}
		new_task2->title = new_task.title;
		if (panels[monitor].g_task.tooltip_enabled)
			new_task2->area._get_tooltip_text = task_get_tooltip;
		for (k = 0; k < TASK_STATE_COUNT; ++k) {
			new_task2->icon[k] = new_task.icon[k];
			new_task2->icon_hover[k] = new_task.icon_hover[k];
			new_task2->icon_press[k] = new_task.icon_press[k];
			new_task2->state_pix[k] = 0;
		}
		new_task2->icon_width = new_task.icon_width;
		new_task2->icon_height = new_task.icon_height;
		taskbar->area.children = g_list_append(taskbar->area.children, new_task2);
		taskbar->area.resize_needed = 1;
		g_ptr_array_add(task_group, new_task2);
		// printf("add_task panel %d, desktop %d, task %s\n", i, j, new_task2->title);
	}
	Window *key = calloc(1, sizeof(Window));
	*key = new_task.win;
	g_hash_table_insert(win_to_task_table, key, task_group);
	set_task_state(new_task2, new_task.current_state);

	sort_taskbar_for_win(win);

	if (taskbar_mode == MULTI_DESKTOP) {
		Panel *panel = new_task2->area.panel;
		panel->area.resize_needed = 1;
	}

	if (window_is_urgent(win)) {
		add_urgent(new_task2);
	}

	return new_task2;
}

void remove_task(Task *task)
{
	if (!task)
		return;

	if (taskbar_mode == MULTI_DESKTOP) {
		Panel *panel = task->area.panel;
		panel->area.resize_needed = 1;
	}

	Window win = task->win;

	// free title and icon just for the first task
	// even with task_on_all_desktop and with task_on_all_panel
	// printf("remove_task %s %d\n", task->title, task->desktop);
	if (task->title)
		free(task->title);
	int k;
	for (k = 0; k < TASK_STATE_COUNT; ++k) {
		if (task->icon[k]) {
			imlib_context_set_image(task->icon[k]);
			imlib_free_image();
			task->icon[k] = 0;
		}
		if (task->icon_hover[k]) {
			imlib_context_set_image(task->icon_hover[k]);
			imlib_free_image();
			task->icon_hover[k] = 0;
		}
		if (task->icon_press[k]) {
			imlib_context_set_image(task->icon_press[k]);
			imlib_free_image();
			task->icon_press[k] = 0;
		}
		if (task->state_pix[k])
			XFreePixmap(server.dsp, task->state_pix[k]);
	}

	int i;
	Task *task2;
	GPtrArray *task_group = g_hash_table_lookup(win_to_task_table, &win);
	for (i = 0; i < task_group->len; ++i) {
		task2 = g_ptr_array_index(task_group, i);
		if (task2 == task_active)
			task_active = 0;
		if (task2 == task_drag)
			task_drag = 0;
		if (g_slist_find(urgent_list, task2))
			del_urgent(task2);
		remove_area((Area *)task2);
		free(task2);
	}
	g_hash_table_remove(win_to_task_table, &win);
}

gboolean get_title(Task *task)
{
	Panel *panel = task->area.panel;
	char *title, *name;

	if (!panel->g_task.text && !panel->g_task.tooltip_enabled && taskbar_sort_method != TASKBAR_SORT_TITLE)
		return 0;

	name = server_get_property(task->win, server.atom._NET_WM_VISIBLE_NAME, server.atom.UTF8_STRING, 0);
	if (!name || !strlen(name)) {
		name = server_get_property(task->win, server.atom._NET_WM_NAME, server.atom.UTF8_STRING, 0);
		if (!name || !strlen(name)) {
			name = server_get_property(task->win, server.atom.WM_NAME, XA_STRING, 0);
		}
	}

	if (name && strlen(name)) {
		title = strdup(name);
	} else {
		title = strdup("Untitled");
	}
	if (name)
		XFree(name);

	if (task->title) {
		// check unecessary title change
		if (strcmp(task->title, title) == 0) {
			free(title);
			return 0;
		} else
			free(task->title);
	}

	task->title = title;
	GPtrArray *task_group = task_get_tasks(task->win);
	if (task_group) {
		int i;
		for (i = 0; i < task_group->len; ++i) {
			Task *task2 = g_ptr_array_index(task_group, i);
			task2->title = task->title;
			set_task_redraw(task2);
		}
	}
	return 1;
}

void get_icon(Task *task)
{
	Panel *panel = task->area.panel;
	if (!panel->g_task.icon)
		return;
	int i;
	Imlib_Image img = NULL;
	XWMHints *hints = 0;
	gulong *data = 0;

	for (int k = 0; k < TASK_STATE_COUNT; ++k) {
		if (task->icon[k]) {
			imlib_context_set_image(task->icon[k]);
			imlib_free_image();
			task->icon[k] = 0;
		}
	}

	data = server_get_property(task->win, server.atom._NET_WM_ICON, XA_CARDINAL, &i);
	if (data) {
		// get ARGB icon
		int w, h;
		gulong *tmp_data;

		tmp_data = get_best_icon(data, get_icon_count(data, i), i, &w, &h, panel->g_task.icon_size1);
#ifdef __x86_64__
		DATA32 icon_data[w * h];
		int length = w * h;
		for (i = 0; i < length; ++i)
			icon_data[i] = tmp_data[i];
		img = imlib_create_image_using_copied_data(w, h, icon_data);
#else
		img = imlib_create_image_using_data(w, h, (DATA32 *)tmp_data);
#endif
	} else {
		// get Pixmap icon
		hints = XGetWMHints(server.dsp, task->win);
		if (hints) {
			if (hints->flags & IconPixmapHint && hints->icon_pixmap != 0) {
				// get width, height and depth for the pixmap
				Window root;
				int icon_x, icon_y;
				uint border_width, bpp;
				uint w, h;

				// printf("  get pixmap\n");
				XGetGeometry(server.dsp, hints->icon_pixmap, &root, &icon_x, &icon_y, &w, &h, &border_width, &bpp);
				imlib_context_set_drawable(hints->icon_pixmap);
				img = imlib_create_image_from_drawable(hints->icon_mask, 0, 0, w, h, 0);
			}
		}
	}
	if (img == NULL) {
		imlib_context_set_image(default_icon);
		img = imlib_clone_image();
	}

	// transform icons
	imlib_context_set_image(img);
	imlib_image_set_has_alpha(1);
	int w, h;
	w = imlib_image_get_width();
	h = imlib_image_get_height();
	Imlib_Image orig_image =
	imlib_create_cropped_scaled_image(0, 0, w, h, panel->g_task.icon_size1, panel->g_task.icon_size1);
	imlib_free_image();

	imlib_context_set_image(orig_image);
	task->icon_width = imlib_image_get_width();
	task->icon_height = imlib_image_get_height();
	for (int k = 0; k < TASK_STATE_COUNT; ++k) {
		imlib_context_set_image(orig_image);
		task->icon[k] = imlib_clone_image();
		imlib_context_set_image(task->icon[k]);
		DATA32 *data32;
		if (panel->g_task.alpha[k] != 100 || panel->g_task.saturation[k] != 0 || panel->g_task.brightness[k] != 0) {
			data32 = imlib_image_get_data();
			adjust_asb(data32,
					   task->icon_width,
					   task->icon_height,
					   panel->g_task.alpha[k],
					   (float)panel->g_task.saturation[k] / 100,
					   (float)panel->g_task.brightness[k] / 100);
			imlib_image_put_back_data(data32);
		}
		if (panel_config.mouse_effects) {
			task->icon_hover[k] = adjust_icon(task->icon[k],
											 panel_config.mouse_over_alpha,
											 panel_config.mouse_over_saturation,
											 panel_config.mouse_over_brightness);
			task->icon_press[k] = adjust_icon(task->icon[k],
											 panel_config.mouse_pressed_alpha,
											 panel_config.mouse_pressed_saturation,
											 panel_config.mouse_pressed_brightness);
		}
	}
	imlib_context_set_image(orig_image);
	imlib_free_image();

	if (hints)
		XFree(hints);
	if (data)
		XFree(data);

	GPtrArray *task_group = task_get_tasks(task->win);
	if (task_group) {
		for (i = 0; i < task_group->len; ++i) {
			Task *task2 = g_ptr_array_index(task_group, i);
			task2->icon_width = task->icon_width;
			task2->icon_height = task->icon_height;
			for (int k = 0; k < TASK_STATE_COUNT; ++k) {
				task2->icon[k] = task->icon[k];
				task2->icon_hover[k] = task->icon_hover[k];
				task2->icon_press[k] = task->icon_press[k];
			}
			set_task_redraw(task2);
		}
	}
}

// TODO icons look too large when the panel is large
void draw_task_icon(Task *task, int text_width)
{
	if (!task->icon[task->current_state])
		return;

	// Find pos
	int pos_x;
	Panel *panel = (Panel *)task->area.panel;
	if (panel->g_task.centered) {
		if (panel->g_task.text)
			pos_x = (task->area.width - text_width - panel->g_task.icon_size1) / 2;
		else
			pos_x = (task->area.width - panel->g_task.icon_size1) / 2;
	} else
		pos_x = panel->g_task.area.paddingxlr + task->area.bg->border.width;

	// Render

	Imlib_Image image;
	// Render
	if (panel_config.mouse_effects) {
		if (task->area.mouse_state == MOUSE_OVER)
			image = task->icon_hover[task->current_state];
		else if (task->area.mouse_state == MOUSE_DOWN)
			image = task->icon_press[task->current_state];
		else
			image = task->icon[task->current_state];
	} else {
		image = task->icon[task->current_state];
	}

	imlib_context_set_image(image);
	render_image(task->area.pix, pos_x, panel->g_task.icon_posy);
}

void draw_task(void *obj, cairo_t *c)
{
	Task *task = obj;
	if (!panel_config.mouse_effects)
		task->state_pix[task->current_state] = task->area.pix;
	PangoLayout *layout;
	Color *config_text;
	int width = 0, height;
	Panel *panel = (Panel *)task->area.panel;
	// printf("draw_task %d %d\n", task->area.posx, task->area.posy);

	if (panel->g_task.text) {
		/* Layout */
		layout = pango_cairo_create_layout(c);
		pango_layout_set_font_description(layout, panel->g_task.font_desc);
		pango_layout_set_text(layout, task->title, -1);

		/* Drawing width and Cut text */
		// pango use U+22EF or U+2026
		pango_layout_set_width(layout, ((Taskbar *)task->area.parent)->text_width * PANGO_SCALE);
		pango_layout_set_height(layout, panel->g_task.text_height * PANGO_SCALE);
		pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
		pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

		/* Center text */
		if (panel->g_task.centered)
			pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
		else
			pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);

		pango_layout_get_pixel_size(layout, &width, &height);

		config_text = &panel->g_task.font[task->current_state];

		double text_posy = (panel->g_task.area.height - height) / 2.0;

		draw_text(layout, c, panel->g_task.text_posx, text_posy, config_text, panel->font_shadow);

		g_object_unref(layout);
	}

	if (panel->g_task.icon) {
		draw_task_icon(task, width);
	}
}

void on_change_task(void *obj)
{
	Task *task = obj;
	Panel *panel = (Panel *)task->area.panel;

	long value[] = {panel->posx + task->area.posx, panel->posy + task->area.posy, task->area.width, task->area.height};
	XChangeProperty(server.dsp,
					task->win,
					server.atom._NET_WM_ICON_GEOMETRY,
					XA_CARDINAL,
					32,
					PropModeReplace,
					(unsigned char *)value,
					4);

	// reset Pixmap when position/size changed
	set_task_redraw(task);
}

// Given a pointer to the active task (active_task) and a pointer
// to the task that is currently under the mouse (current_task),
// returns a pointer to the active task.
Task *find_active_task(Task *current_task, Task *active_task)
{
	if (active_task == NULL)
		return current_task;

	Taskbar *taskbar = current_task->area.parent;

	GList *l0 = taskbar->area.children;
	if (taskbarname_enabled)
		l0 = l0->next;
	for (; l0; l0 = l0->next) {
		Task *task = l0->data;
		if (task->win == active_task->win)
			return task;
	}

	return current_task;
}

Task *next_task(Task *task)
{
	if (!task)
		return 0;

	Taskbar *taskbar = task->area.parent;

	GList *l0 = taskbar->area.children;
	if (taskbarname_enabled)
		l0 = l0->next;
	GList *lfirst_task = l0;
	for (; l0; l0 = l0->next) {
		Task *task1 = l0->data;
		if (task1 == task) {
			l0 = l0->next ? l0->next : lfirst_task;
			return l0->data;
		}
	}

	return 0;
}

Task *prev_task(Task *task)
{
	if (!task)
		return 0;

	Taskbar *taskbar = task->area.parent;

	Task *task2 = NULL;
	GList *l0 = taskbar->area.children;
	if (taskbarname_enabled)
		l0 = l0->next;
	GList *lfirst_task = l0;
	for (; l0; l0 = l0->next) {
		Task *task1 = l0->data;
		if (task1 == task) {
			if (l0 == lfirst_task) {
				l0 = g_list_last(l0);
				task2 = l0->data;
			}
			return task2;
		}
		task2 = task1;
	}

	return 0;
}

void active_task()
{
	if (task_active) {
		set_task_state(task_active, window_is_iconified(task_active->win) ? TASK_ICONIFIED : TASK_NORMAL);
		task_active = 0;
	}

	Window w1 = get_active_window();
	// printf("Change active task %ld\n", w1);

	if (w1) {
		if (!task_get_tasks(w1)) {
			Window w2;
			while (XGetTransientForHint(server.dsp, w1, &w2))
				w1 = w2;
		}
		set_task_state((task_active = task_get_task(w1)), TASK_ACTIVE);
	}
}

void set_task_state(Task *task, TaskState state)
{
	if (!task || state < 0 || state >= TASK_STATE_COUNT)
		return;

	if (task->current_state != state || hide_task_diff_monitor) {
		GPtrArray *task_group = task_get_tasks(task->win);
		if (task_group) {
			int i;
			for (i = 0; i < task_group->len; ++i) {
				Task *task1 = g_ptr_array_index(task_group, i);
				task1->current_state = state;
				task1->area.bg = panels[0].g_task.background[state];
				if (!panel_config.mouse_effects) {
					task1->area.pix = task1->state_pix[state];
					if (!task1->area.pix)
						task1->area.redraw_needed = TRUE;
				} else {
					task1->area.redraw_needed = TRUE;
				}
				if (state == TASK_ACTIVE && g_slist_find(urgent_list, task1))
					del_urgent(task1);
				int hide = 0;
				Taskbar *taskbar = (Taskbar *)task1->area.parent;
				if (task->desktop == ALLDESKTOP && server.desktop != taskbar->desktop) {
					// Hide ALLDESKTOP task on non-current desktop
					hide = 1;
				}
				if (hide_inactive_tasks) {
					// Show only the active task
					if (state != TASK_ACTIVE) {
						hide = 1;
					}
				}
				if (get_window_monitor(task->win) != ((Panel *)task->area.panel)->monitor &&
					(hide_task_diff_monitor || num_panels > 1)) {
					hide = 1;
				}
				if (1 - hide != task1->area.on_screen) {
					task1->area.on_screen = TRUE - hide;
					set_task_redraw(task1);
					Panel *p = (Panel *)task->area.panel;
					task->area.resize_needed = 1;
					p->taskbar->area.resize_needed = 1;
					p->area.resize_needed = 1;
				}
			}
			panel_refresh = TRUE;
		}
	}
}

void set_task_redraw(Task *task)
{
	int k;
	for (k = 0; k < TASK_STATE_COUNT; ++k) {
		if (task->state_pix[k])
			XFreePixmap(server.dsp, task->state_pix[k]);
		task->state_pix[k] = 0;
	}
	task->area.pix = 0;
	task->area.redraw_needed = TRUE;
}

void blink_urgent(void *arg)
{
	GSList *urgent_task = urgent_list;
	while (urgent_task) {
		Task *t = urgent_task->data;
		if (t->urgent_tick < max_tick_urgent) {
			if (t->urgent_tick++ % 2)
				set_task_state(t, TASK_URGENT);
			else
				set_task_state(t, window_is_iconified(t->win) ? TASK_ICONIFIED : TASK_NORMAL);
		}
		urgent_task = urgent_task->next;
	}
	panel_refresh = TRUE;
}

void add_urgent(Task *task)
{
	if (!task)
		return;

	// some programs set urgency hint although they are active
	if (task_active && task_active->win == task->win)
		return;

	task = task_get_task(task->win); // always add the first task for a task group (omnipresent windows)
	task->urgent_tick = 0;
	if (g_slist_find(urgent_list, task))
		return;

	// not yet in the list, so we have to add it
	urgent_list = g_slist_prepend(urgent_list, task);

	if (!urgent_timeout)
		urgent_timeout = add_timeout(10, 1000, blink_urgent, 0, &urgent_timeout);

	Panel *panel = task->area.panel;
	if (panel->is_hidden)
		autohide_show(panel);
}

void del_urgent(Task *task)
{
	urgent_list = g_slist_remove(urgent_list, task);
	if (!urgent_list) {
		stop_timeout(urgent_timeout);
		urgent_timeout = NULL;
	}
}
