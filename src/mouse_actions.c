/**************************************************************************
* Copyright (C) 2017 tint2 authors
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

#include "drag_and_drop.h"
#include "panel.h"
#include "server.h"
#include "task.h"
#include "window.h"

gboolean tint2_handles_click(Panel *panel, XButtonEvent *e)
{
    Task *task = click_task(panel, e->x, e->y);
    if (task) {
        if ((e->button == 1 && mouse_left != 0) || (e->button == 2 && mouse_middle != 0) ||
            (e->button == 3 && mouse_right != 0) || (e->button == 4 && mouse_scroll_up != 0) ||
            (e->button == 5 && mouse_scroll_down != 0)) {
            return TRUE;
        } else
            return FALSE;
    }
    LauncherIcon *icon = click_launcher_icon(panel, e->x, e->y);
    if (icon) {
        if (e->button == 1) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
    // no launcher/task clicked --> check if taskbar clicked
    Taskbar *taskbar = click_taskbar(panel, e->x, e->y);
    if (taskbar && e->button == 1 && taskbar_mode == MULTI_DESKTOP)
        return TRUE;
    if (click_clock(panel, e->x, e->y)) {
        if ((e->button == 1 && clock_lclick_command) || (e->button == 2 && clock_mclick_command) ||
            (e->button == 3 && clock_rclick_command) || (e->button == 4 && clock_uwheel_command) ||
            (e->button == 5 && clock_dwheel_command))
            return TRUE;
        else
            return FALSE;
    }
#ifdef ENABLE_BATTERY
    if (click_battery(panel, e->x, e->y)) {
        if ((e->button == 1 && battery_lclick_command) || (e->button == 2 && battery_mclick_command) ||
            (e->button == 3 && battery_rclick_command) || (e->button == 4 && battery_uwheel_command) ||
            (e->button == 5 && battery_dwheel_command))
            return TRUE;
        else
            return FALSE;
    }
#endif
    if (click_execp(panel, e->x, e->y))
        return TRUE;
    if (click_button(panel, e->x, e->y))
        return TRUE;
    return FALSE;
}

void handle_mouse_press_event(XEvent *e)
{
    Panel *panel = get_panel(e->xany.window);
    if (!panel)
        return;

    if (wm_menu && !tint2_handles_click(panel, &e->xbutton)) {
        forward_click(e);
        return;
    }
    task_drag = click_task(panel, e->xbutton.x, e->xbutton.y);

    if (panel_layer == BOTTOM_LAYER)
        XLowerWindow(server.display, panel->main_win);
}

void handle_mouse_move_event(XEvent *e)
{
    Panel *panel = get_panel(e->xany.window);
    if (!panel || !task_drag)
        return;

    // Find the taskbar on the event's location
    Taskbar *event_taskbar = click_taskbar(panel, e->xbutton.x, e->xbutton.y);
    if (event_taskbar == NULL)
        return;

    // Find the task on the event's location
    Task *event_task = click_task(panel, e->xbutton.x, e->xbutton.y);

    // If the event takes place on the same taskbar as the task being dragged
    if (&event_taskbar->area == task_drag->area.parent) {
        if (taskbar_sort_method != TASKBAR_NOSORT) {
            sort_tasks(event_taskbar);
        } else {
            // Swap the task_drag with the task on the event's location (if they differ)
            if (event_task && event_task != task_drag) {
                GList *drag_iter = g_list_find(event_taskbar->area.children, task_drag);
                GList *task_iter = g_list_find(event_taskbar->area.children, event_task);
                if (drag_iter && task_iter) {
                    gpointer temp = task_iter->data;
                    task_iter->data = drag_iter->data;
                    drag_iter->data = temp;
                    event_taskbar->area.resize_needed = 1;
                    schedule_panel_redraw();
                    task_dragged = 1;
                }
            }
        }
    } else { // The event is on another taskbar than the task being dragged
        if (task_drag->desktop == ALL_DESKTOPS || taskbar_mode != MULTI_DESKTOP)
            return;

        Taskbar *drag_taskbar = (Taskbar *)task_drag->area.parent;
        remove_area((Area *)task_drag);

        if (event_taskbar->area.posx > drag_taskbar->area.posx || event_taskbar->area.posy > drag_taskbar->area.posy) {
            int i = (taskbarname_enabled) ? 1 : 0;
            event_taskbar->area.children = g_list_insert(event_taskbar->area.children, task_drag, i);
        } else
            event_taskbar->area.children = g_list_append(event_taskbar->area.children, task_drag);

        // Move task to other desktop (but avoid the 'Window desktop changed' code in 'event_property_notify')
        task_drag->area.parent = &event_taskbar->area;
        task_drag->desktop = event_taskbar->desktop;

        change_window_desktop(task_drag->win, event_taskbar->desktop);
        if (hide_task_diff_desktop)
            change_desktop(event_taskbar->desktop);

        if (taskbar_sort_method != TASKBAR_NOSORT) {
            sort_tasks(event_taskbar);
        }

        event_taskbar->area.resize_needed = 1;
        drag_taskbar->area.resize_needed = 1;
        task_dragged = 1;
        schedule_panel_redraw();
        panel->area.resize_needed = 1;
    }
}

void handle_mouse_release_event(XEvent *e)
{
    Panel *panel = get_panel(e->xany.window);
    if (!panel)
        return;

    if (wm_menu && !tint2_handles_click(panel, &e->xbutton)) {
        forward_click(e);
        if (panel_layer == BOTTOM_LAYER)
            XLowerWindow(server.display, panel->main_win);
        task_drag = 0;
        return;
    }

    MouseAction action = TOGGLE_ICONIFY;
    switch (e->xbutton.button) {
    case 1:
        action = mouse_left;
        break;
    case 2:
        action = mouse_middle;
        break;
    case 3:
        action = mouse_right;
        break;
    case 4:
        action = mouse_scroll_up;
        break;
    case 5:
        action = mouse_scroll_down;
        break;
    case 6:
        action = mouse_tilt_left;
        break;
    case 7:
        action = mouse_tilt_right;
        break;
    }

    Clock *clock = click_clock(panel, e->xbutton.x, e->xbutton.y);
    if (clock) {
        clock_action(clock, e->xbutton.button, e->xbutton.x - clock->area.posx, e->xbutton.y - clock->area.posy, e->xbutton.time);
        if (panel_layer == BOTTOM_LAYER)
            XLowerWindow(server.display, panel->main_win);
        task_drag = 0;
        return;
    }

#ifdef ENABLE_BATTERY
    Battery *battery = click_battery(panel, e->xbutton.x, e->xbutton.y);
    if (battery) {
        battery_action(battery, e->xbutton.button, e->xbutton.x - battery->area.posx, e->xbutton.y - battery->area.posy, e->xbutton.time);
        if (panel_layer == BOTTOM_LAYER)
            XLowerWindow(server.display, panel->main_win);
        task_drag = 0;
        return;
    }
#endif

    Execp *execp = click_execp(panel, e->xbutton.x, e->xbutton.y);
    if (execp) {
        execp_action(execp, e->xbutton.button, e->xbutton.x - execp->area.posx, e->xbutton.y - execp->area.posy, e->xbutton.time);
        if (panel_layer == BOTTOM_LAYER)
            XLowerWindow(server.display, panel->main_win);
        task_drag = 0;
        return;
    }

    Button *button = click_button(panel, e->xbutton.x, e->xbutton.y);
    if (button) {
        button_action(button, e->xbutton.button, e->xbutton.x - button->area.posx, e->xbutton.y - button->area.posy, e->xbutton.time);
        if (panel_layer == BOTTOM_LAYER)
            XLowerWindow(server.display, panel->main_win);
        task_drag = 0;
        return;
    }

    if (e->xbutton.button == 1 && click_launcher(panel, e->xbutton.x, e->xbutton.y)) {
        LauncherIcon *icon = click_launcher_icon(panel, e->xbutton.x, e->xbutton.y);
        if (icon) {
            launcher_action(icon, e, e->xbutton.x - icon->area.posx, e->xbutton.y - icon->area.posy);
        }
        task_drag = 0;
        return;
    }

    Taskbar *taskbar;
    if (!(taskbar = click_taskbar(panel, e->xbutton.x, e->xbutton.y))) {
        // TODO: check better solution to keep window below
        if (panel_layer == BOTTOM_LAYER)
            XLowerWindow(server.display, panel->main_win);
        task_drag = 0;
        return;
    }

    // drag and drop task
    if (task_dragged) {
        task_drag = 0;
        task_dragged = 0;
        return;
    }

    // switch desktop
    if (taskbar_mode == MULTI_DESKTOP) {
        gboolean diff_desktop = FALSE;
        if (taskbar->desktop != server.desktop && action != CLOSE && action != DESKTOP_LEFT &&
            action != DESKTOP_RIGHT) {
            diff_desktop = TRUE;
            change_desktop(taskbar->desktop);
        }
        Task *task = click_task(panel, e->xbutton.x, e->xbutton.y);
        if (task) {
            if (diff_desktop) {
                if (action == TOGGLE_ICONIFY) {
                    if (!window_is_active(task->win))
                        activate_window(task->win);
                } else {
                    task_handle_mouse_event(task, action);
                }
            } else {
                task_handle_mouse_event(task, action);
            }
        }
    } else {
        task_handle_mouse_event(click_task(panel, e->xbutton.x, e->xbutton.y), action);
    }

    // to keep window below
    if (panel_layer == BOTTOM_LAYER)
        XLowerWindow(server.display, panel->main_win);
}
