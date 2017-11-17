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
#include "tooltip.h"

GHashTable *win_to_task;

Task *active_task;
Task *task_drag;
gboolean taskbar_enabled;
gboolean taskbar_distribute_size;
gboolean hide_task_diff_desktop;
gboolean hide_inactive_tasks;
gboolean hide_task_diff_monitor;
gboolean hide_taskbar_if_empty;
gboolean always_show_all_desktop_tasks;
TaskbarSortMethod taskbar_sort_method;
Alignment taskbar_alignment;
static timeout *thumbnail_update_timer_all;
static timeout *thumbnail_update_timer_active;
static timeout *thumbnail_update_timer_tooltip;

static GList *taskbar_task_orderings = NULL;
static GList *taskbar_thumbnail_jobs_done = NULL;

void taskbar_init_fonts();
int taskbar_compute_desired_size(void *obj);

// Removes the task with &win = key. The other args are ignored.
void taskbar_remove_task(Window *win);

void taskbar_update_thumbnails(void *arg);

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
    hide_task_diff_desktop = FALSE;
    hide_inactive_tasks = FALSE;
    hide_task_diff_monitor = FALSE;
    hide_taskbar_if_empty = FALSE;
    always_show_all_desktop_tasks = FALSE;
    thumbnail_update_timer_all = NULL;
    thumbnail_update_timer_active = NULL;
    thumbnail_update_timer_tooltip = NULL;
    taskbar_thumbnail_jobs_done = NULL;
    taskbar_sort_method = TASKBAR_NOSORT;
    taskbar_alignment = ALIGN_LEFT;
    default_taskbarname();
}

void taskbar_clear_orderings()
{
    if (!taskbar_task_orderings)
        return;
    for (GList *order = taskbar_task_orderings; order; order = order->next) {
        g_list_free_full((GList *)order->data, free);
    }
    g_list_free(taskbar_task_orderings);
    taskbar_task_orderings = NULL;
}

void taskbar_save_orderings()
{
    taskbar_clear_orderings();
    taskbar_task_orderings = NULL;
    for (int i = 0; i < num_panels; i++) {
        Panel *panel = &panels[i];
        for (int j = 0; j < panel->num_desktops; j++) {
            Taskbar *taskbar = &panel->taskbar[j];
            GList *task_order = NULL;
            for (GList *c = (taskbar->area.children && taskbarname_enabled) ? taskbar->area.children->next
                                                                            : taskbar->area.children;
                 c;
                 c = c->next) {
                Task *t = (Task *)c->data;
                Window *window = calloc(1, sizeof(Window));
                *window = t->win;
                task_order = g_list_append(task_order, window);
            }
            taskbar_task_orderings = g_list_append(taskbar_task_orderings, task_order);
        }
    }
}

void cleanup_taskbar()
{
    stop_timeout(thumbnail_update_timer_all);
    stop_timeout(thumbnail_update_timer_active);
    stop_timeout(thumbnail_update_timer_tooltip);
    g_list_free(taskbar_thumbnail_jobs_done);
    taskbar_save_orderings();
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
    cleanup_taskbarname();
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

    for (int state = 0; state < TASK_STATE_COUNT; state++) {
        g_list_free(panel_config.g_task.gradient[state]);
    }

    for (int state = 0; state < TASKBAR_STATE_COUNT; state++) {
        g_list_free(panel_config.g_taskbar.gradient[state]);
        g_list_free(panel_config.g_taskbar.gradient_name[state]);
    }
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
    snprintf(panel->g_taskbar.area_name.name, sizeof(panel->g_taskbar.area_name.name), "Taskbarname");
    panel->g_taskbar.area_name.size_mode = LAYOUT_FIXED;
    panel->g_taskbar.area_name._resize = resize_taskbarname;
    panel->g_taskbar.area_name._is_under_mouse = full_width_area_is_under_mouse;
    panel->g_taskbar.area_name._draw_foreground = draw_taskbarname;
    panel->g_taskbar.area_name._on_change_layout = 0;
    panel->g_taskbar.area_name.resize_needed = 1;
    panel->g_taskbar.area_name.on_screen = TRUE;

    // taskbar
    panel->g_taskbar.area.parent = panel;
    panel->g_taskbar.area.panel = panel;
    snprintf(panel->g_taskbar.area.name, sizeof(panel->g_taskbar.area.name), "Taskbar");
    panel->g_taskbar.area.size_mode = LAYOUT_DYNAMIC;
    panel->g_taskbar.area.alignment = taskbar_alignment;
    panel->g_taskbar.area._resize = resize_taskbar;
    panel->g_taskbar.area._compute_desired_size = taskbar_compute_desired_size;
    panel->g_taskbar.area._is_under_mouse = full_width_area_is_under_mouse;
    panel->g_taskbar.area.resize_needed = 1;
    panel->g_taskbar.area.on_screen = TRUE;
    if (panel_horizontal) {
        panel->g_taskbar.area.posy = top_border_width(&panel->area) + panel->area.paddingy;
        panel->g_taskbar.area.height =
            panel->area.height - top_bottom_border_width(&panel->area) - 2 * panel->area.paddingy;
        panel->g_taskbar.area_name.posy = panel->g_taskbar.area.posy;
        panel->g_taskbar.area_name.height = panel->g_taskbar.area.height;
    } else {
        panel->g_taskbar.area.posx = left_border_width(&panel->area) + panel->area.paddingy;
        panel->g_taskbar.area.width =
            panel->area.width - left_right_border_width(&panel->area) - 2 * panel->area.paddingy;
        panel->g_taskbar.area_name.posx = panel->g_taskbar.area.posx;
        panel->g_taskbar.area_name.width = panel->g_taskbar.area.width;
    }

    // task
    panel->g_task.area.panel = panel;
    snprintf(panel->g_task.area.name, sizeof(panel->g_task.area.name), "Task");
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

    if (!panel->g_task.maximum_width || !panel_horizontal)
        panel->g_task.maximum_width = server.monitors[panel->monitor].width;
    if (!panel->g_task.maximum_height || panel_horizontal)
        panel->g_task.maximum_height = server.monitors[panel->monitor].height;

    if (panel_horizontal) {
        panel->g_task.area.posy = panel->g_taskbar.area.posy +
                                  top_bg_border_width(panel->g_taskbar.background[TASKBAR_NORMAL]) +
                                  panel->g_taskbar.area.paddingy;
        panel->g_task.area.width = panel->g_task.maximum_width;
        panel->g_task.area.height = panel->g_taskbar.area.height -
                                    top_bottom_bg_border_width(panel->g_taskbar.background[TASKBAR_NORMAL]) -
                                    2 * panel->g_taskbar.area.paddingy;
    } else {
        panel->g_task.area.posx = panel->g_taskbar.area.posx +
                                  left_bg_border_width(panel->g_taskbar.background[TASKBAR_NORMAL]) +
                                  panel->g_taskbar.area.paddingy;
        panel->g_task.area.width = panel->g_taskbar.area.width -
                                   left_right_bg_border_width(panel->g_taskbar.background[TASKBAR_NORMAL]) -
                                   2 * panel->g_taskbar.area.paddingy;
        panel->g_task.area.height = panel->g_task.maximum_height;
    }

    for (int j = 0; j < TASK_STATE_COUNT; ++j) {
        if (!panel->g_task.background[j])
            panel->g_task.background[j] = &g_array_index(backgrounds, Background, 0);
        if (panel->g_task.background[j]->border.radius > panel->g_task.area.height / 2) {
            fprintf(stderr,
                    "tint2: task%sbackground_id has a too large rounded value. Please fix your tint2rc\n",
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

    panel->g_task.text_posx = left_bg_border_width(panel->g_task.background[0]) + panel->g_task.area.paddingxlr;
    panel->g_task.text_height =
        panel->g_task.area.height - (2 * panel->g_task.area.paddingy) - top_bottom_border_width(&panel->g_task.area);
    if (panel->g_task.has_icon) {
        panel->g_task.icon_size1 = MIN(MIN(panel->g_task.maximum_width, panel->g_task.maximum_height),
                                       MIN(panel->g_task.area.width, panel->g_task.area.height)) -
                                   2 * panel->g_task.area.paddingy - MAX(left_right_border_width(&panel->g_task.area),
                                                                         top_bottom_border_width(&panel->g_task.area));
        panel->g_task.text_posx += panel->g_task.icon_size1 + panel->g_task.area.paddingx;
        panel->g_task.icon_posy = (panel->g_task.area.height - panel->g_task.icon_size1) / 2;
    }

    Taskbar *taskbar;
    panel->num_desktops = server.num_desktops;
    panel->taskbar = calloc(server.num_desktops, sizeof(Taskbar));
    for (int j = 0; j < panel->num_desktops; j++) {
        taskbar = &panel->taskbar[j];
        memcpy(&taskbar->area, &panel->g_taskbar.area, sizeof(Area));
        taskbar->desktop = j;
        if (j == server.desktop) {
            taskbar->area.bg = panel->g_taskbar.background[TASKBAR_ACTIVE];
            free_area_gradient_instances(&taskbar->area);
            instantiate_area_gradients(&taskbar->area);
        } else {
            taskbar->area.bg = panel->g_taskbar.background[TASKBAR_NORMAL];
            free_area_gradient_instances(&taskbar->area);
            instantiate_area_gradients(&taskbar->area);
        }
    }
    init_taskbarname_panel(panel);
    taskbar_start_thumbnail_timer(THUMB_MODE_ALL);
}

void taskbar_start_thumbnail_timer(ThumbnailUpdateMode mode)
{
    if (!panel_config.g_task.thumbnail_enabled)
        return;
    if (debug_thumbnails)
        fprintf(stderr, BLUE "tint2: taskbar_start_thumbnail_timer %s" RESET "\n", mode == THUMB_MODE_ACTIVE_WINDOW ? "active" : mode == THUMB_MODE_TOOLTIP_WINDOW ? "tooltip" : "all");
    change_timeout(mode == THUMB_MODE_ALL ? &thumbnail_update_timer_all :
                                            mode == THUMB_MODE_ACTIVE_WINDOW ? &thumbnail_update_timer_active : &thumbnail_update_timer_tooltip,
                   mode == THUMB_MODE_TOOLTIP_WINDOW ? 1000 : 500,
                   mode == THUMB_MODE_ALL ? 10 * 1000 : 0,
                   taskbar_update_thumbnails,
                   (void *)(long)mode);
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
    schedule_panel_redraw();
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

static Window *sort_windows = NULL;

int compare_windows(const void *a, const void *b)
{
    if (!sort_windows)
        return 0;

    int ia = *(int *)a;
    int ib = *(int *)b;

    Window wina = sort_windows[ia];
    Window winb = sort_windows[ib];

    for (GList *order = taskbar_task_orderings; order; order = order->next) {
        int posa = -1;
        int posb = -1;
        int pos = 0;
        for (GList *item = (GList *)order->data; item; item = item->next, pos++) {
            Window win = *(Window *)item->data;
            if (win == wina)
                posa = pos;
            if (win == winb)
                posb = pos;
        }
        if (posa >= 0 && posb >= 0) {
            return posa - posb;
        }
    }

    return ia - ib;
}

void sort_win_list(Window *windows, int count)
{
    int *indices = (int *)calloc(count, sizeof(int));
    for (int i = 0; i < count; i++)
        indices[i] = i;
    sort_windows = windows;
    qsort(indices, count, sizeof(int), compare_windows);
    Window *result = (Window *)calloc(count, sizeof(Window));
    for (int i = 0; i < count; i++)
        result[i] = windows[indices[i]];
    memcpy(windows, result, count * sizeof(Window));
    free(result);
    free(indices);
    sort_windows = NULL;
}

void taskbar_refresh_tasklist()
{
    if (!taskbar_enabled)
        return;

    int num_results;
    Window *win = server_get_property(server.root_win, server.atom._NET_CLIENT_LIST, XA_WINDOW, &num_results);
    Window *sorted = (Window *)calloc(num_results, sizeof(Window));
    memcpy(sorted, win, num_results * sizeof(Window));
    if (taskbar_task_orderings) {
        sort_win_list(sorted, num_results);
        taskbar_clear_orderings();
    }
    if (!win)
        return;

    GList *win_list = g_hash_table_get_keys(win_to_task);
    for (GList *it = win_list; it; it = it->next) {
        int i;
        for (i = 0; i < num_results; i++)
            if (*((Window *)it->data) == sorted[i])
                break;
        if (i == num_results)
            taskbar_remove_task(it->data);
    }
    g_list_free(win_list);

    // Add any new
    for (int i = 0; i < num_results; i++)
        if (!get_task(sorted[i]))
            add_task(sorted[i]);

    XFree(win);
    free(sorted);
}

int taskbar_compute_desired_size(void *obj)
{
    Taskbar *taskbar = (Taskbar *)obj;
    Panel *panel = (Panel *)taskbar->area.panel;

    if (taskbar_mode == MULTI_DESKTOP && !taskbar_distribute_size) {
        int result = 0;
        for (int i = 0; i < panel->num_desktops; i++) {
            Taskbar *t = &panel->taskbar[i];
            result = MAX(result, container_compute_desired_size(&t->area));
        }
        return result;
    }
    return container_compute_desired_size(&taskbar->area);
}

gboolean resize_taskbar(void *obj)
{
    Taskbar *taskbar = (Taskbar *)obj;
    Panel *panel = (Panel *)taskbar->area.panel;

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
        taskbar->text_width = text_width - panel->g_task.text_posx - right_border_width(&panel->g_task.area) -
                              panel->g_task.area.paddingxlr;
    } else {
        relayout_with_constraint(&taskbar->area, panel->g_task.maximum_height);

        taskbar->text_width = taskbar->area.width - (2 * panel->g_taskbar.area.paddingy) - panel->g_task.text_posx -
                              right_border_width(&panel->g_task.area) - panel->g_task.area.paddingxlr;
    }
    return FALSE;
}

gboolean taskbar_is_empty(Taskbar *taskbar)
{
    GList *l = taskbar->area.children;
    if (taskbarname_enabled)
        l = l->next;
    for (; l != NULL; l = l->next) {
        if (((Task *)l->data)->area.on_screen) {
            return FALSE;
        }
    }
    return TRUE;
}

void update_taskbar_visibility(Taskbar *taskbar)
{
    if (taskbar->desktop == server.desktop) {
        // Taskbar for current desktop is always shown
        show(&taskbar->area);
    } else if (taskbar_mode == MULTI_DESKTOP) {
        if (hide_taskbar_if_empty) {
            if (taskbar_is_empty(taskbar)) {
                hide(&taskbar->area);
            } else {
                show(&taskbar->area);
            }
        } else {
            show(&taskbar->area);
        }
    } else {
        hide(&taskbar->area);
    }
}

void update_all_taskbars_visibility()
{
    for (int i = 0; i < num_panels; i++) {
        Panel *panel = &panels[i];
        for (int j = 0; j < panel->num_desktops; j++) {
            update_taskbar_visibility(&panel->taskbar[j]);
        }
    }
}

void set_taskbar_state(Taskbar *taskbar, TaskbarState state)
{
    taskbar->area.bg = panels[0].g_taskbar.background[state];
    free_area_gradient_instances(&taskbar->area);
    instantiate_area_gradients(&taskbar->area);

    if (taskbarname_enabled) {
        taskbar->bar_name.area.bg = panels[0].g_taskbar.background_name[state];
        free_area_gradient_instances(&taskbar->bar_name.area);
        instantiate_area_gradients(&taskbar->bar_name.area);
    }

    update_taskbar_visibility(taskbar);

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
        if (taskbar_mode == MULTI_DESKTOP && hide_task_diff_desktop) {
            GList *l = taskbar->area.children;
            if (taskbarname_enabled)
                l = l->next;
            for (; l; l = l->next) {
                Task *task = (Task *)l->data;
                set_task_state(task, task->current_state);
            }
        }
    }
    schedule_panel_redraw();
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
    schedule_panel_redraw();
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

void update_minimized_icon_positions(void *p)
{
    Panel *panel = (Panel *)p;
    for (int i = 0; i < panel->num_desktops; i++) {
        Taskbar *taskbar = &panel->taskbar[i];
        if (!taskbar->area.on_screen)
            continue;
        for (GList *c = taskbar->area.children; c; c = c->next) {
            Area *area = (Area *)c->data;
            if (area->_on_change_layout)
                area->_on_change_layout(area);
        }
    }
}

void taskbar_update_thumbnails(void *arg)
{
    if (!panel_config.g_task.thumbnail_enabled)
        return;
    ThumbnailUpdateMode mode = (ThumbnailUpdateMode)(long)arg;
    if (debug_thumbnails)
        fprintf(stderr, BLUE "tint2: taskbar_update_thumbnails %s" RESET "\n", mode == THUMB_MODE_ACTIVE_WINDOW ? "active" : mode == THUMB_MODE_TOOLTIP_WINDOW ? "tooltip" : "all");
    double start_time = get_time();
    for (int i = 0; i < num_panels; i++) {
        Panel *panel = &panels[i];
        for (int j = 0; j < panel->num_desktops; j++) {
            Taskbar *taskbar = &panel->taskbar[j];
            for (GList *c = (taskbar->area.children && taskbarname_enabled) ? taskbar->area.children->next
                                                                            : taskbar->area.children;
                 c;
                 c = c->next) {
                Task *t = (Task *)c->data;
                if ((mode == THUMB_MODE_ALL && t->current_state == TASK_ACTIVE && !g_list_find(taskbar_thumbnail_jobs_done, t)) || (mode == THUMB_MODE_ACTIVE_WINDOW && t->current_state == TASK_ACTIVE) ||
                    (mode == THUMB_MODE_TOOLTIP_WINDOW && g_tooltip.mapped && g_tooltip.area == &t->area)) {
                    task_refresh_thumbnail(t);
                    if (mode == THUMB_MODE_ALL)
                        taskbar_thumbnail_jobs_done = g_list_append(taskbar_thumbnail_jobs_done, t);
                    if (t->thumbnail && mode == THUMB_MODE_TOOLTIP_WINDOW) {
                        taskbar_start_thumbnail_timer(THUMB_MODE_TOOLTIP_WINDOW);
                    }
                }
                if (mode == THUMB_MODE_ALL) {
                    double now = get_time();
                    if (now - start_time > 0.030) {
                        change_timeout(&thumbnail_update_timer_all, 50, 10 * 1000, taskbar_update_thumbnails, arg);
                        return;
                    }
                }
            }
        }
    }
    if (mode == THUMB_MODE_ALL) {
        if (taskbar_thumbnail_jobs_done) {
            g_list_free(taskbar_thumbnail_jobs_done);
            taskbar_thumbnail_jobs_done = NULL;
            change_timeout(&thumbnail_update_timer_all, 10 * 1000, 10 * 1000, taskbar_update_thumbnails, arg);
        }
    }
}
