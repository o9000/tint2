/**************************************************************************
*
* Tint2 : taskbar
*
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
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
#include <Imlib2.h>

#include "task.h"
#include "taskbar.h"
#include "server.h"
#include "window.h"
#include "panel.h"
#include "strnatcmp.h"

GHashTable *win_to_task;

Task *active_task;
Task *task_drag;
gboolean taskbar_enabled;
gboolean taskbar_distribute_size;
gboolean hide_inactive_tasks;
gboolean hide_task_diff_monitor;
TaskbarSortMethod taskbar_sort_method;
Alignment taskbar_alignment;

void taskbar_init_fonts();

// Removes the task with &win = key. The other args are ignored.
void taskbar_remove_task(Window *win);

guint win_hash(gconstpointer key)
{
	return *((const Window *)key);
}

gboolean win_compare(gconstpointer a, gconstpointer b)
{
	return (*((const Window *)a) == *((const Window *)b));
}

void free_ptr_array(gpointer data)
{
	g_ptr_array_free(data, 1);
}

void default_taskbar()
{
	win_to_task = NULL;
	urgent_timeout = NULL;
	urgent_list = NULL;
	taskbar_enabled = FALSE;
	taskbar_distribute_size = FALSE;
	hide_inactive_tasks = FALSE;
	hide_task_diff_monitor = FALSE;
	taskbar_sort_method = TASKBAR_NOSORT;
	taskbar_alignment = ALIGN_LEFT;
	default_taskbarname();
}

void cleanup_taskbar()
{
	cleanup_taskbarname();
	if (win_to_task) {
		while (g_hash_table_size(win_to_task)) {
			GHashTableIter iter;
			gpointer key, value;

			g_hash_table_iter_init(&iter, win_to_task);
			if (g_hash_table_iter_next(&iter, &key, &value)) {
				taskbar_remove_task(key);
			}
		}
		g_hash_table_destroy(win_to_task);
		win_to_task = NULL;
	}
	for (int i = 0; i < num_panels; i++) {
		Panel *panel = &panels[i];
		for (int j = 0; j < panel->num_desktops; j++) {
			Taskbar *taskbar = &panel->taskbar[j];
			free_area(&taskbar->area);
			// remove taskbar from the panel
			remove_area((Area *)taskbar);
		}
		if (panel->taskbar) {
			free(panel->taskbar);
			panel->taskbar = NULL;
		}
	}

	g_slist_free(urgent_list);
	urgent_list = NULL;

	stop_timeout(urgent_timeout);
}

void init_taskbar()
{
	if (!panel_config.g_task.has_text && !panel_config.g_task.has_icon) {
		panel_config.g_task.has_text = panel_config.g_task.has_icon = 1;
	}

	if (!win_to_task)
		win_to_task = g_hash_table_new_full(win_hash, win_compare, free, free_ptr_array);

	active_task = 0;
	task_drag = 0;
}

void init_taskbar_panel(void *p)
{
	Panel *panel = (Panel *)p;

	if (!panel->g_taskbar.background[TASKBAR_NORMAL]) {
		panel->g_taskbar.background[TASKBAR_NORMAL] = &g_array_index(backgrounds, Background, 0);
		panel->g_taskbar.background[TASKBAR_ACTIVE] = &g_array_index(backgrounds, Background, 0);
	}
	if (!panel->g_taskbar.background_name[TASKBAR_NORMAL]) {
		panel->g_taskbar.background_name[TASKBAR_NORMAL] = &g_array_index(backgrounds, Background, 0);
		panel->g_taskbar.background_name[TASKBAR_ACTIVE] = &g_array_index(backgrounds, Background, 0);
	}
	if (!panel->g_task.area.bg)
		panel->g_task.area.bg = &g_array_index(backgrounds, Background, 0);
	taskbar_init_fonts();

	// taskbar name
	panel->g_taskbar.area_name.panel = panel;
	panel->g_taskbar.area_name.size_mode = LAYOUT_FIXED;
	panel->g_taskbar.area_name._resize = resize_taskbarname;
	panel->g_taskbar.area_name._draw_foreground = draw_taskbarname;
	panel->g_taskbar.area_name._on_change_layout = 0;
	panel->g_taskbar.area_name.resize_needed = 1;
	panel->g_taskbar.area_name.on_screen = TRUE;

	// taskbar
	panel->g_taskbar.area.parent = panel;
	panel->g_taskbar.area.panel = panel;
	panel->g_taskbar.area.size_mode = LAYOUT_DYNAMIC;
	panel->g_taskbar.area.alignment = taskbar_alignment;
	panel->g_taskbar.area._resize = resize_taskbar;
	panel->g_taskbar.area.resize_needed = 1;
	panel->g_taskbar.area.on_screen = TRUE;
	if (panel_horizontal) {
		panel->g_taskbar.area.posy = panel->area.bg->border.width + panel->area.paddingy;
		panel->g_taskbar.area.height = panel->area.height - (2 * panel->g_taskbar.area.posy);
		panel->g_taskbar.area_name.posy = panel->g_taskbar.area.posy;
		panel->g_taskbar.area_name.height = panel->g_taskbar.area.height;
	} else {
		panel->g_taskbar.area.posx = panel->area.bg->border.width + panel->area.paddingy;
		panel->g_taskbar.area.width = panel->area.width - (2 * panel->g_taskbar.area.posx);
		panel->g_taskbar.area_name.posx = panel->g_taskbar.area.posx;
		panel->g_taskbar.area_name.width = panel->g_taskbar.area.width;
	}

	// task
	panel->g_task.area.panel = panel;
	panel->g_task.area.size_mode = LAYOUT_DYNAMIC;
	panel->g_task.area._draw_foreground = draw_task;
	panel->g_task.area._on_change_layout = on_change_task;
	panel->g_task.area.resize_needed = 1;
	panel->g_task.area.on_screen = TRUE;
	if ((panel->g_task.config_asb_mask & (1 << TASK_NORMAL)) == 0) {
		panel->g_task.alpha[TASK_NORMAL] = 100;
		panel->g_task.saturation[TASK_NORMAL] = 0;
		panel->g_task.brightness[TASK_NORMAL] = 0;
	}
	if ((panel->g_task.config_asb_mask & (1 << TASK_ACTIVE)) == 0) {
		panel->g_task.alpha[TASK_ACTIVE] = panel->g_task.alpha[TASK_NORMAL];
		panel->g_task.saturation[TASK_ACTIVE] = panel->g_task.saturation[TASK_NORMAL];
		panel->g_task.brightness[TASK_ACTIVE] = panel->g_task.brightness[TASK_NORMAL];
	}
	if ((panel->g_task.config_asb_mask & (1 << TASK_ICONIFIED)) == 0) {
		panel->g_task.alpha[TASK_ICONIFIED] = panel->g_task.alpha[TASK_NORMAL];
		panel->g_task.saturation[TASK_ICONIFIED] = panel->g_task.saturation[TASK_NORMAL];
		panel->g_task.brightness[TASK_ICONIFIED] = panel->g_task.brightness[TASK_NORMAL];
	}
	if ((panel->g_task.config_asb_mask & (1 << TASK_URGENT)) == 0) {
		panel->g_task.alpha[TASK_URGENT] = panel->g_task.alpha[TASK_ACTIVE];
		panel->g_task.saturation[TASK_URGENT] = panel->g_task.saturation[TASK_ACTIVE];
		panel->g_task.brightness[TASK_URGENT] = panel->g_task.brightness[TASK_ACTIVE];
	}
	if ((panel->g_task.config_font_mask & (1 << TASK_NORMAL)) == 0)
		panel->g_task.font[TASK_NORMAL] = (Color){{1, 1, 1}, 1};
	if ((panel->g_task.config_font_mask & (1 << TASK_ACTIVE)) == 0)
		panel->g_task.font[TASK_ACTIVE] = panel->g_task.font[TASK_NORMAL];
	if ((panel->g_task.config_font_mask & (1 << TASK_ICONIFIED)) == 0)
		panel->g_task.font[TASK_ICONIFIED] = panel->g_task.font[TASK_NORMAL];
	if ((panel->g_task.config_font_mask & (1 << TASK_URGENT)) == 0)
		panel->g_task.font[TASK_URGENT] = panel->g_task.font[TASK_ACTIVE];
	if ((panel->g_task.config_background_mask & (1 << TASK_NORMAL)) == 0)
		panel->g_task.background[TASK_NORMAL] = &g_array_index(backgrounds, Background, 0);
	if ((panel->g_task.config_background_mask & (1 << TASK_ACTIVE)) == 0)
		panel->g_task.background[TASK_ACTIVE] = panel->g_task.background[TASK_NORMAL];
	if ((panel->g_task.config_background_mask & (1 << TASK_ICONIFIED)) == 0)
		panel->g_task.background[TASK_ICONIFIED] = panel->g_task.background[TASK_NORMAL];
	if ((panel->g_task.config_background_mask & (1 << TASK_URGENT)) == 0)
		panel->g_task.background[TASK_URGENT] = panel->g_task.background[TASK_ACTIVE];

	if (panel_horizontal) {
		panel->g_task.area.posy = panel->g_taskbar.area.posy +
								  panel->g_taskbar.background[TASKBAR_NORMAL]->border.width +
								  panel->g_taskbar.area.paddingy;
		panel->g_task.area.height = panel->area.height - (2 * panel->g_task.area.posy);
	} else {
		panel->g_task.area.posx = panel->g_taskbar.area.posx +
								  panel->g_taskbar.background[TASKBAR_NORMAL]->border.width +
								  panel->g_taskbar.area.paddingy;
		panel->g_task.area.width = panel->area.width - (2 * panel->g_task.area.posx);
		panel->g_task.area.height = panel->g_task.maximum_height;
	}

	for (int j = 0; j < TASK_STATE_COUNT; ++j) {
		if (!panel->g_task.background[j])
			panel->g_task.background[j] = &g_array_index(backgrounds, Background, 0);
		if (panel->g_task.background[j]->border.radius > panel->g_task.area.height / 2) {
			printf("task%sbackground_id has a too large rounded value. Please fix your tint2rc\n",
				   j == 0 ? "_" : j == 1 ? "_active_" : j == 2 ? "_iconified_" : "_urgent_");
			g_array_append_val(backgrounds, *panel->g_task.background[j]);
			panel->g_task.background[j] = &g_array_index(backgrounds, Background, backgrounds->len - 1);
			panel->g_task.background[j]->border.radius = panel->g_task.area.height / 2;
		}
	}

	// compute vertical position : text and icon
	int height_ink, height, width;
	get_text_size2(panel->g_task.font_desc,
				   &height_ink,
				   &height,
				   &width,
				   panel->area.height,
				   panel->area.width,
				   "TAjpg",
				   5,
				   PANGO_WRAP_WORD_CHAR,
				   PANGO_ELLIPSIZE_END,
				   FALSE);

	if (!panel->g_task.maximum_width && panel_horizontal)
		panel->g_task.maximum_width = server.monitors[panel->monitor].width;

	panel->g_task.text_posx = panel->g_task.background[0]->border.width + panel->g_task.area.paddingxlr;
	panel->g_task.text_height = panel->g_task.area.height - (2 * panel->g_task.area.paddingy);
	if (panel->g_task.has_icon) {
		panel->g_task.icon_size1 = panel->g_task.area.height - (2 * panel->g_task.area.paddingy);
		panel->g_task.text_posx += panel->g_task.icon_size1 + panel->g_task.area.paddingx;
		panel->g_task.icon_posy = (panel->g_task.area.height - panel->g_task.icon_size1) / 2;
	}
	// printf("monitor %d, task_maximum_width %d\n", panel->monitor, panel->g_task.maximum_width);

	Taskbar *taskbar;
	panel->num_desktops = server.num_desktops;
	panel->taskbar = calloc(server.num_desktops, sizeof(Taskbar));
	for (int j = 0; j < panel->num_desktops; j++) {
		taskbar = &panel->taskbar[j];
		memcpy(&taskbar->area, &panel->g_taskbar.area, sizeof(Area));
		taskbar->desktop = j;
		if (j == server.desktop)
			taskbar->area.bg = panel->g_taskbar.background[TASKBAR_ACTIVE];
		else
			taskbar->area.bg = panel->g_taskbar.background[TASKBAR_NORMAL];
	}
	init_taskbarname_panel(panel);
}

void taskbar_init_fonts()
{
	for (int i = 0; i < num_panels; i++) {
		if (!panels[i].g_task.font_desc) {
			panels[i].g_task.font_desc = pango_font_description_from_string(get_default_font());
			pango_font_description_set_size(panels[i].g_task.font_desc,
			                                pango_font_description_get_size(panels[i].g_task.font_desc) - PANGO_SCALE);
		}
	}
}

void taskbar_default_font_changed()
{
	if (!taskbar_enabled)
		return;

	gboolean needs_update = FALSE;
	for (int i = 0; i < num_panels; i++) {
		if (!panels[i].g_task.has_font) {
			pango_font_description_free(panels[i].g_task.font_desc);
			panels[i].g_task.font_desc = NULL;
			needs_update = TRUE;
		}
	}
	if (!needs_update)
		return;
	taskbar_init_fonts();
	for (int i = 0; i < num_panels; i++) {
		for (int j = 0; j < panels[i].num_desktops; j++) {
			Taskbar *taskbar = &panels[i].taskbar[j];
			for (GList *c = taskbar->area.children; c; c = c->next) {
				Task *t = c->data;
				t->area.resize_needed = TRUE;
				schedule_redraw(&t->area);
			}
		}
	}
	panel_refresh = TRUE;
}

void taskbar_remove_task(Window *win)
{
	remove_task(get_task(*win));
}

Task *get_task(Window win)
{
	GPtrArray *task_buttons = get_task_buttons(win);
	if (task_buttons)
		return g_ptr_array_index(task_buttons, 0);
	return NULL;
}

GPtrArray *get_task_buttons(Window win)
{
	if (win_to_task && taskbar_enabled)
		return g_hash_table_lookup(win_to_task, &win);
	return NULL;
}

void taskbar_refresh_tasklist()
{
	if (!taskbar_enabled)
		return;
	// fprintf(stderr, "%s %d:\n", __FUNCTION__, __LINE__);

	int num_results;
	Window *win = server_get_property(server.root_win, server.atom._NET_CLIENT_LIST, XA_WINDOW, &num_results);
	if (!win)
		return;

	GList *win_list = g_hash_table_get_keys(win_to_task);
	for (GList *it = win_list; it; it = it->next) {
		int i;
		for (i = 0; i < num_results; i++)
			if (*((Window *)it->data) == win[i])
				break;
		if (i == num_results)
			taskbar_remove_task(it->data);
	}
	g_list_free(win_list);

	// Add any new
	for (int i = 0; i < num_results; i++)
		if (!get_task(win[i]))
			add_task(win[i]);

	XFree(win);
}

gboolean resize_taskbar(void *obj)
{
	Taskbar *taskbar = (Taskbar *)obj;
	Panel *panel = (Panel *)taskbar->area.panel;

	// printf("resize_taskbar %d %d\n", taskbar->area.posx, taskbar->area.posy);
	if (panel_horizontal) {
		relayout_with_constraint(&taskbar->area, panel->g_task.maximum_width);

		int text_width = panel->g_task.maximum_width;
		GList *l = taskbar->area.children;
		if (taskbarname_enabled)
			l = l->next;
		for (; l != NULL; l = l->next) {
			if (((Task *)l->data)->area.on_screen) {
				text_width = ((Task *)l->data)->area.width;
				break;
			}
		}
		taskbar->text_width =
			text_width - panel->g_task.text_posx - panel->g_task.area.bg->border.width - panel->g_task.area.paddingxlr;
	} else {
		relayout_with_constraint(&taskbar->area, panel->g_task.maximum_height);

		taskbar->text_width = taskbar->area.width - (2 * panel->g_taskbar.area.paddingy) - panel->g_task.text_posx -
							  panel->g_task.area.bg->border.width - panel->g_task.area.paddingxlr;
	}
	return FALSE;
}

void set_taskbar_state(Taskbar *taskbar, TaskbarState state)
{
	taskbar->area.bg = panels[0].g_taskbar.background[state];
	if (taskbarname_enabled) {
		taskbar->bar_name.area.bg = panels[0].g_taskbar.background_name[state];
	}
	if (taskbar_mode != MULTI_DESKTOP) {
		if (state == TASKBAR_NORMAL)
			taskbar->area.on_screen = FALSE;
		else
			taskbar->area.on_screen = TRUE;
	}
	if (taskbar->area.on_screen) {
		schedule_redraw(&taskbar->area);
		if (taskbarname_enabled) {
			schedule_redraw(&taskbar->bar_name.area);
		}
		if (taskbar_mode == MULTI_DESKTOP &&
			panels[0].g_taskbar.background[TASKBAR_NORMAL] != panels[0].g_taskbar.background[TASKBAR_ACTIVE]) {
			GList *l = taskbar->area.children;
			if (taskbarname_enabled)
				l = l->next;
			for (; l; l = l->next)
				schedule_redraw((Area *)l->data);
		}
	}
	panel_refresh = TRUE;
}

void update_taskbar_visibility(void *p)
{
	Panel *panel = (Panel *)p;

	for (int j = 0; j < panel->num_desktops; j++) {
		Taskbar *taskbar = &panel->taskbar[j];
		if (taskbar_mode != MULTI_DESKTOP && taskbar->desktop != server.desktop) {
			// SINGLE_DESKTOP and not current desktop
			taskbar->area.on_screen = FALSE;
		} else {
			taskbar->area.on_screen = TRUE;
		}
	}
	panel_refresh = TRUE;
}

#define NONTRIVIAL 2
gint compare_tasks_trivial(Task *a, Task *b, Taskbar *taskbar)
{
	if (a == b)
		return 0;
	if (taskbarname_enabled) {
		if (a == taskbar->area.children->data)
			return -1;
		if (b == taskbar->area.children->data)
			return 1;
	}
	return NONTRIVIAL;
}

gboolean contained_within(Task *a, Task *b)
{
	if ((a->win_x <= b->win_x) && (a->win_y <= b->win_y) && (a->win_x + a->win_w >= b->win_x + b->win_w) &&
		(a->win_y + a->win_h >= b->win_y + b->win_h)) {
		return TRUE;
	}
	return FALSE;
}

gint compare_task_centers(Task *a, Task *b, Taskbar *taskbar)
{
	int trivial = compare_tasks_trivial(a, b, taskbar);
	if (trivial != NONTRIVIAL)
		return trivial;

	// If a window has the same coordinates and size as the other,
	// they are considered to be equal in the comparison.
	if ((a->win_x == b->win_x) && (a->win_y == b->win_y) && (a->win_w == b->win_w) && (a->win_h == b->win_h)) {
		return 0;
	}

	// If a window is completely contained in another,
	// then it is considered to come after (to the right/bottom) of the other.
	if (contained_within(a, b))
		return -1;
	if (contained_within(b, a))
		return 1;

	// Compare centers
	int a_horiz_c = a->win_x + a->win_w / 2;
	int b_horiz_c = b->win_x + b->win_w / 2;
	int a_vert_c = a->win_y + a->win_h / 2;
	int b_vert_c = b->win_y + b->win_h / 2;
	if (panel_horizontal) {
		if (a_horiz_c != b_horiz_c) {
			return a_horiz_c - b_horiz_c;
		}
		return a_vert_c - b_vert_c;
	} else {
		if (a_vert_c != b_vert_c) {
			return a_vert_c - b_vert_c;
		}
		return a_horiz_c - b_horiz_c;
	}
}

gint compare_task_titles(Task *a, Task *b, Taskbar *taskbar)
{
	int trivial = compare_tasks_trivial(a, b, taskbar);
	if (trivial != NONTRIVIAL)
		return trivial;
	return strnatcasecmp(a->title ? a->title : "", b->title ? b->title : "");
}

gint compare_tasks(Task *a, Task *b, Taskbar *taskbar)
{
	int trivial = compare_tasks_trivial(a, b, taskbar);
	if (trivial != NONTRIVIAL)
		return trivial;
	if (taskbar_sort_method == TASKBAR_NOSORT) {
		return 0;
	} else if (taskbar_sort_method == TASKBAR_SORT_CENTER) {
		return compare_task_centers(a, b, taskbar);
	} else if (taskbar_sort_method == TASKBAR_SORT_TITLE) {
		return compare_task_titles(a, b, taskbar);
	} else if (taskbar_sort_method == TASKBAR_SORT_LRU) {
		return compare_timespecs(&a->last_activation_time, &b->last_activation_time);
	} else if (taskbar_sort_method == TASKBAR_SORT_MRU) {
		return -compare_timespecs(&a->last_activation_time, &b->last_activation_time);
	}
	return 0;
}

gboolean taskbar_needs_sort(Taskbar *taskbar)
{
	if (taskbar_sort_method == TASKBAR_NOSORT)
		return FALSE;

	for (GList *i = taskbar->area.children, *j = i ? i->next : NULL; i && j; i = i->next, j = j->next) {
		if (compare_tasks(i->data, j->data, taskbar) > 0) {
			return TRUE;
		}
	}

	return FALSE;
}

void sort_tasks(Taskbar *taskbar)
{
	if (!taskbar)
		return;
	if (!taskbar_needs_sort(taskbar))
		return;

	taskbar->area.children = g_list_sort_with_data(taskbar->area.children, (GCompareDataFunc)compare_tasks, taskbar);
	taskbar->area.resize_needed = TRUE;
	panel_refresh = TRUE;
	((Panel *)taskbar->area.panel)->area.resize_needed = TRUE;
}

void sort_taskbar_for_win(Window win)
{
	if (taskbar_sort_method == TASKBAR_NOSORT)
		return;

	GPtrArray *task_buttons = get_task_buttons(win);
	if (task_buttons) {
		Task *task0 = g_ptr_array_index(task_buttons, 0);
		if (task0) {
			get_window_coordinates(win, &task0->win_x, &task0->win_y, &task0->win_w, &task0->win_h);
		}
		for (int i = 0; i < task_buttons->len; ++i) {
			Task *task = g_ptr_array_index(task_buttons, i);
			task->win_x = task0->win_x;
			task->win_y = task0->win_y;
			task->win_w = task0->win_w;
			task->win_h = task0->win_h;
			sort_tasks(task->area.parent);
		}
	}
}
