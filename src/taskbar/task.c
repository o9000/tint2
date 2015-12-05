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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "panel.h"
#include "server.h"
#include "task.h"
#include "taskbar.h"
#include "timer.h"
#include "tooltip.h"
#include "window.h"

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
		return NULL;
	if (window_is_hidden(win)) {
		// fprintf(stderr, "%s %d: win = %ld not adding task: window hidden\n", __FUNCTION__, __LINE__, win);
		return NULL;
	}

	XSelectInput(server.dsp, win, PropertyChangeMask | StructureNotifyMask);
	XFlush(server.dsp);

	int monitor = 0;
	if (num_panels > 1) {
		monitor = get_window_monitor(win);
		if (monitor >= num_panels)
			monitor = 0;
	}

	// TODO why do we add the task only to the panel for the current monitor, without checking hide_task_diff_monitor?

	Task task_template;
	memset(&task_template, 0, sizeof(task_template));
	task_template.area.has_mouse_over_effect = TRUE;
	task_template.area.has_mouse_press_effect = TRUE;
	task_template.win = win;
	task_template.desktop = get_window_desktop(win);
	task_template.area.panel = &panels[monitor];
	task_template.current_state = window_is_iconified(win) ? TASK_ICONIFIED : TASK_NORMAL;
	get_window_coordinates(win, &task_template.win_x, &task_template.win_y, &task_template.win_w, &task_template.win_h);

	// allocate only one title and one icon
	// even with task_on_all_desktop and with task_on_all_panel
	task_template.title = NULL;
	for (int k = 0; k < TASK_STATE_COUNT; ++k) {
		task_template.icon[k] = NULL;
		task_template.state_pix[k] = 0;
	}
	get_title(&task_template);
	get_icon(&task_template);

	// fprintf(stderr, "%s %d: win = %ld, task = %s\n", __FUNCTION__, __LINE__, win, task_template.title ? task_template.title : "??");
	// fprintf(stderr, "new task %s win %u: desktop %d, monitor %d\n", new_task.title, win, new_task.desktop, monitor);

	GPtrArray *task_group = g_ptr_array_new();
	for (int j = 0; j < panels[monitor].num_desktops; j++) {
		if (task_template.desktop != ALL_DESKTOPS && task_template.desktop != j)
			continue;

		Taskbar *taskbar = &panels[monitor].taskbar[j];
		Task *task_instance = calloc(1, sizeof(Task));
		memcpy(&task_instance->area, &panels[monitor].g_task.area, sizeof(Area));
		task_instance->area.has_mouse_over_effect = TRUE;
		task_instance->area.has_mouse_press_effect = TRUE;
		task_instance->win = task_template.win;
		task_instance->desktop = task_template.desktop;
		task_instance->win_x = task_template.win_x;
		task_instance->win_y = task_template.win_y;
		task_instance->win_w = task_template.win_w;
		task_instance->win_h = task_template.win_h;
		task_instance->current_state = -1; // to update the current state later in set_task_state...
		if (task_instance->desktop == ALL_DESKTOPS && server.desktop != j) {
			// fprintf(stderr, "%s %d: win = %ld hiding task: another desktop\n", __FUNCTION__, __LINE__, win);
			task_instance->area.on_screen = FALSE;
		}
		task_instance->title = task_template.title;
		if (panels[monitor].g_task.tooltip_enabled)
			task_instance->area._get_tooltip_text = task_get_tooltip;
		for (int k = 0; k < TASK_STATE_COUNT; ++k) {
			task_instance->icon[k] = task_template.icon[k];
			task_instance->icon_hover[k] = task_template.icon_hover[k];
			task_instance->icon_press[k] = task_template.icon_press[k];
			task_instance->state_pix[k] = 0;
		}
		task_instance->icon_width = task_template.icon_width;
		task_instance->icon_height = task_template.icon_height;

		add_area(&task_instance->area, &taskbar->area);
		g_ptr_array_add(task_group, task_instance);
	}
	Window *key = calloc(1, sizeof(Window));
	*key = task_template.win;
	g_hash_table_insert(win_to_task, key, task_group);

	set_task_state((Task*)g_ptr_array_index(task_group, 0), task_template.current_state);

	sort_taskbar_for_win(win);

	if (taskbar_mode == MULTI_DESKTOP) {
		Panel *panel = (Panel*)task_template.area.panel;
		panel->area.resize_needed = TRUE;
	}

	if (window_is_urgent(win)) {
		add_urgent((Task*)g_ptr_array_index(task_group, 0));
	}

	return (Task*)g_ptr_array_index(task_group, 0);
}

void remove_task(Task *task)
{
	if (!task)
		return;

	// fprintf(stderr, "%s %d: win = %ld, task = %s\n", __FUNCTION__, __LINE__, task->win, task->title ? task->title : "??");

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
	for (int k = 0; k < TASK_STATE_COUNT; ++k) {
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

	GPtrArray *task_group = g_hash_table_lookup(win_to_task, &win);
	for (int i = 0; i < task_group->len; ++i) {
		Task *task2 = g_ptr_array_index(task_group, i);
		if (task2 == active_task)
			active_task = 0;
		if (task2 == task_drag)
			task_drag = 0;
		if (g_slist_find(urgent_list, task2))
			del_urgent(task2);
		remove_area((Area *)task2);
		free(task2);
	}
	g_hash_table_remove(win_to_task, &win);
}

gboolean get_title(Task *task)
{
	Panel *panel = task->area.panel;

	if (!panel->g_task.text && !panel->g_task.tooltip_enabled && taskbar_sort_method != TASKBAR_SORT_TITLE)
		return FALSE;

	char *name = server_get_property(task->win, server.atom._NET_WM_VISIBLE_NAME, server.atom.UTF8_STRING, 0);
	if (!name || !strlen(name)) {
		name = server_get_property(task->win, server.atom._NET_WM_NAME, server.atom.UTF8_STRING, 0);
		if (!name || !strlen(name)) {
			name = server_get_property(task->win, server.atom.WM_NAME, XA_STRING, 0);
		}
	}

	char *title;
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
			return FALSE;
		} else {
			free(task->title);
		}
	}

	task->title = title;
	GPtrArray *task_group = task_get_tasks(task->win);
	if (task_group) {
		for (int i = 0; i < task_group->len; ++i) {
			Task *task2 = g_ptr_array_index(task_group, i);
			task2->title = task->title;
			set_task_redraw(task2);
		}
	}
	return TRUE;
}

void get_icon(Task *task)
{
	Panel *panel = task->area.panel;
	if (!panel->g_task.icon)
		return;

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

	int i;
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
	int w = imlib_image_get_width();
	int h = imlib_image_get_height();
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
	} else {
		pos_x = panel->g_task.area.paddingxlr + task->area.bg->border.width;
	}

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
	Task *task = (Task *)obj;
	Panel *panel = (Panel *)task->area.panel;

	if (!panel_config.mouse_effects)
		task->state_pix[task->current_state] = task->area.pix;

	int text_width = 0;
	if (panel->g_task.text) {
		PangoLayout *layout = pango_cairo_create_layout(c);
		pango_layout_set_font_description(layout, panel->g_task.font_desc);
		pango_layout_set_text(layout, task->title, -1);

		pango_layout_set_width(layout, ((Taskbar *)task->area.parent)->text_width * PANGO_SCALE);
		pango_layout_set_height(layout, panel->g_task.text_height * PANGO_SCALE);
		pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
		pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

		if (panel->g_task.centered)
			pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
		else
			pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);

		int text_height;
		pango_layout_get_pixel_size(layout, &text_width, &text_height);
		double text_posy = (panel->g_task.area.height - text_height) / 2.0;

		Color *config_text = &panel->g_task.font[task->current_state];
		draw_text(layout, c, panel->g_task.text_posx, text_posy, config_text, panel->font_shadow);

		g_object_unref(layout);
	}

	if (panel->g_task.icon) {
		draw_task_icon(task, text_width);
	}
}

void on_change_task(void *obj)
{
	Task *task = (Task *)obj;
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

Task *find_active_task(Task *current_task)
{
	if (active_task == NULL)
		return current_task;

	Taskbar *taskbar = (Taskbar *)current_task->area.parent;

	GList *l0 = taskbar->area.children;
	if (taskbarname_enabled)
		l0 = l0->next;
	for (; l0; l0 = l0->next) {
		Task *task = (Task *)l0->data;
		if (task->win == active_task->win)
			return task;
	}

	return current_task;
}

Task *next_task(Task *task)
{
	if (!task)
		return NULL;

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

	return NULL;
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

	return NULL;
}

void reset_active_task()
{
	if (active_task) {
		set_task_state(active_task, window_is_iconified(active_task->win) ? TASK_ICONIFIED : TASK_NORMAL);
		active_task = NULL;
	}

	Window w1 = get_active_window();
	// printf("Change active task %ld\n", w1);

	if (w1) {
		if (!task_get_tasks(w1)) {
			Window w2;
			while (XGetTransientForHint(server.dsp, w1, &w2))
				w1 = w2;
		}
		set_task_state((active_task = task_get_task(w1)), TASK_ACTIVE);
	}
}

void set_task_state(Task *task, TaskState state)
{
	if (!task || state < 0 || state >= TASK_STATE_COUNT)
		return;

	if (state == TASK_ACTIVE && task->current_state != state) {
		clock_gettime(CLOCK_MONOTONIC, &task->last_activation_time);
		if (taskbar_sort_method == TASKBAR_SORT_LRU || taskbar_sort_method == TASKBAR_SORT_MRU) {
			GPtrArray *task_group = task_get_tasks(task->win);
			if (task_group) {
				for (int i = 0; i < task_group->len; ++i) {
					Task *task1 = g_ptr_array_index(task_group, i);
					Taskbar *taskbar = (Taskbar *)task1->area.parent;
					sort_tasks(taskbar);
				}
			}
		}
	}

	if (task->current_state != state || hide_task_diff_monitor) {
		GPtrArray *task_group = task_get_tasks(task->win);
		if (task_group) {
			for (int i = 0; i < task_group->len; ++i) {
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
				gboolean hide = FALSE;
				Taskbar *taskbar = (Taskbar *)task1->area.parent;
				if (task->desktop == ALL_DESKTOPS && server.desktop != taskbar->desktop) {
					// Hide ALL_DESKTOPS task on non-current desktop
					hide = TRUE;
				}
				if (hide_inactive_tasks) {
					// Show only the active task
					if (state != TASK_ACTIVE) {
						hide = TRUE;
					}
				}
				if (get_window_monitor(task->win) != ((Panel *)task->area.panel)->monitor &&
					(hide_task_diff_monitor || num_panels > 1)) {
					hide = TRUE;
				}
				if ((!hide) != task1->area.on_screen) {
					task1->area.on_screen = !hide;
					set_task_redraw(task1);
					Panel *p = (Panel *)task->area.panel;
					task->area.resize_needed = TRUE;
					p->taskbar->area.resize_needed = TRUE;
					p->area.resize_needed = TRUE;
				}
			}
			panel_refresh = TRUE;
		}
	}
}

void set_task_redraw(Task *task)
{
	for (int k = 0; k < TASK_STATE_COUNT; ++k) {
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
	if (active_task && active_task->win == task->win)
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
