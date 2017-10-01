/**************************************************************************
*
* Tint2 panel
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

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>
#include <X11/extensions/Xdamage.h>
#include <Imlib2.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>

#ifdef HAVE_SN
#include <libsn/sn.h>
#endif

#include "config.h"
#include "drag_and_drop.h"
#include "fps_distribution.h"
#include "init.h"
#include "launcher.h"
#include "mouse_actions.h"
#include "panel.h"
#include "server.h"
#include "signals.h"
#include "systraybar.h"
#include "task.h"
#include "taskbar.h"
#include "tooltip.h"
#include "timer.h"
#include "tracing.h"
#include "uevent.h"
#include "version.h"
#include "window.h"
#include "xsettings-client.h"

// Global process state variables

XSettingsClient *xsettings_client = NULL;

gboolean debug_fps = FALSE;
gboolean debug_frames = FALSE;
static int frame = 0;
double tracing_fps_threshold = 60;
static double ts_event_read;
static double ts_event_processed;
static double ts_render_finished;
static double ts_flush_finished;

static gboolean first_render;

void handle_event_property_notify(XEvent *e)
{
    gboolean debug = FALSE;

    Window win = e->xproperty.window;
    Atom at = e->xproperty.atom;

    if (xsettings_client)
        xsettings_client_process_event(xsettings_client, e);
    for (int i = 0; i < num_panels; i++) {
        Panel *p = &panels[i];
        if (win == p->main_win) {
            if (at == server.atom._NET_WM_DESKTOP && get_window_desktop(p->main_win) != ALL_DESKTOPS)
                replace_panel_all_desktops(p);
            return;
        }
    }
    if (win == server.root_win) {
        if (!server.got_root_win) {
            XSelectInput(server.display, server.root_win, PropertyChangeMask | StructureNotifyMask);
            server.got_root_win = TRUE;
        }

        // Change name of desktops
        else if (at == server.atom._NET_DESKTOP_NAMES) {
            if (debug)
                fprintf(stderr, "tint2: %s %d: win = root, atom = _NET_DESKTOP_NAMES\n", __func__, __LINE__);
            update_desktop_names();
        }
        // Change desktops
        else if (at == server.atom._NET_NUMBER_OF_DESKTOPS || at == server.atom._NET_DESKTOP_GEOMETRY ||
                 at == server.atom._NET_DESKTOP_VIEWPORT || at == server.atom._NET_WORKAREA ||
                 at == server.atom._NET_CURRENT_DESKTOP) {
            if (debug)
                fprintf(stderr, "tint2: %s %d: win = root, atom = ?? desktops changed\n", __func__, __LINE__);
            if (!taskbar_enabled)
                return;
            int old_num_desktops = server.num_desktops;
            int old_desktop = server.desktop;
            server_get_number_of_desktops();
            server.desktop = get_current_desktop();
            if (old_num_desktops != server.num_desktops) { // If number of desktop changed
                if (server.num_desktops <= server.desktop) {
                    server.desktop = server.num_desktops - 1;
                }
                cleanup_taskbar();
                init_taskbar();
                for (int i = 0; i < num_panels; i++) {
                    init_taskbar_panel(&panels[i]);
                    set_panel_items_order(&panels[i]);
                    panels[i].area.resize_needed = 1;
                }
                taskbar_refresh_tasklist();
                reset_active_task();
                update_all_taskbars_visibility();
                if (old_desktop != server.desktop)
                    tooltip_trigger_hide();
                schedule_panel_redraw();
            } else if (old_desktop != server.desktop) {
                tooltip_trigger_hide();
                for (int i = 0; i < num_panels; i++) {
                    Panel *panel = &panels[i];
                    set_taskbar_state(&panel->taskbar[old_desktop], TASKBAR_NORMAL);
                    set_taskbar_state(&panel->taskbar[server.desktop], TASKBAR_ACTIVE);
                    // check ALL_DESKTOPS task => resize taskbar
                    Taskbar *taskbar;
                    if (server.num_desktops > old_desktop) {
                        taskbar = &panel->taskbar[old_desktop];
                        GList *l = taskbar->area.children;
                        if (taskbarname_enabled)
                            l = l->next;
                        for (; l; l = l->next) {
                            Task *task = l->data;
                            if (task->desktop == ALL_DESKTOPS) {
                                task->area.on_screen = always_show_all_desktop_tasks;
                                taskbar->area.resize_needed = 1;
                                schedule_panel_redraw();
                                if (taskbar_mode == MULTI_DESKTOP)
                                    panel->area.resize_needed = 1;
                            }
                        }
                    }
                    taskbar = &panel->taskbar[server.desktop];
                    GList *l = taskbar->area.children;
                    if (taskbarname_enabled)
                        l = l->next;
                    for (; l; l = l->next) {
                        Task *task = l->data;
                        if (task->desktop == ALL_DESKTOPS) {
                            task->area.on_screen = TRUE;
                            taskbar->area.resize_needed = 1;
                            if (taskbar_mode == MULTI_DESKTOP)
                                panel->area.resize_needed = 1;
                        }
                    }

                    if (server.viewports) {
                        GList *need_update = NULL;

                        GHashTableIter iter;
                        gpointer key, value;

                        g_hash_table_iter_init(&iter, win_to_task);
                        while (g_hash_table_iter_next(&iter, &key, &value)) {
                            Window task_win = *(Window *)key;
                            Task *task = get_task(task_win);
                            if (task) {
                                int desktop = get_window_desktop(task_win);
                                if (desktop != task->desktop) {
                                    need_update = g_list_append(need_update, task);
                                }
                            }
                        }
                        for (l = need_update; l; l = l->next) {
                            Task *task = l->data;
                            task_update_desktop(task);
                        }
                    }
                }
            }
        }
        // Window list
        else if (at == server.atom._NET_CLIENT_LIST) {
            if (debug)
                fprintf(stderr, "tint2: %s %d: win = root, atom = _NET_CLIENT_LIST\n", __func__, __LINE__);
            taskbar_refresh_tasklist();
            update_all_taskbars_visibility();
            schedule_panel_redraw();
        }
        // Change active
        else if (at == server.atom._NET_ACTIVE_WINDOW) {
            if (debug)
                fprintf(stderr, "tint2: %s %d: win = root, atom = _NET_ACTIVE_WINDOW\n", __func__, __LINE__);
            reset_active_task();
            schedule_panel_redraw();
        } else if (at == server.atom._XROOTPMAP_ID || at == server.atom._XROOTMAP_ID) {
            if (debug)
                fprintf(stderr, "tint2: %s %d: win = root, atom = _XROOTPMAP_ID\n", __func__, __LINE__);
            // change Wallpaper
            for (int i = 0; i < num_panels; i++) {
                set_panel_background(&panels[i]);
            }
            schedule_panel_redraw();
        }
    } else {
        TrayWindow *traywin = systray_find_icon(win);
        if (traywin) {
            systray_property_notify(traywin, e);
            return;
        }

        Task *task = get_task(win);
        if (debug) {
            char *atom_name = XGetAtomName(server.display, at);
            fprintf(stderr,
                    "%s %d: win = %ld, task = %s, atom = %s\n",
                    __func__,
                    __LINE__,
                    win,
                    task ? (task->title ? task->title : "??") : "null",
                    atom_name);
            XFree(atom_name);
        }
        if (!task) {
            if (debug)
                fprintf(stderr, "tint2: %s %d\n", __func__, __LINE__);
            if (at == server.atom._NET_WM_STATE) {
                // xfce4 sends _NET_WM_STATE after minimized to tray, so we need to check if window is mapped
                // if it is mapped and not set as skip_taskbar, we must add it to our task list
                XWindowAttributes wa;
                XGetWindowAttributes(server.display, win, &wa);
                if (wa.map_state == IsViewable && !window_is_skip_taskbar(win)) {
                    if ((task = add_task(win)))
                        schedule_panel_redraw();
                }
            }
            return;
        }
        // fprintf(stderr, "tint2: atom root_win = %s, %s\n", XGetAtomName(server.display, at), task->title);

        // Window title changed
        if (at == server.atom._NET_WM_VISIBLE_NAME || at == server.atom._NET_WM_NAME || at == server.atom.WM_NAME) {
            if (task_update_title(task)) {
                if (g_tooltip.mapped && (g_tooltip.area == (Area *)task)) {
                    tooltip_copy_text((Area *)task);
                    tooltip_update();
                }
                if (taskbar_sort_method == TASKBAR_SORT_TITLE)
                    sort_taskbar_for_win(win);
                schedule_panel_redraw();
            }
        }
        // Demand attention
        else if (at == server.atom._NET_WM_STATE) {
            if (debug) {
                int count;
                Atom *atom_state = server_get_property(win, server.atom._NET_WM_STATE, XA_ATOM, &count);
                for (int j = 0; j < count; j++) {
                    char *atom_state_name = XGetAtomName(server.display, atom_state[j]);
                    fprintf(stderr, "tint2: %s %d: _NET_WM_STATE = %s\n", __func__, __LINE__, atom_state_name);
                    XFree(atom_state_name);
                }
                XFree(atom_state);
            }
            if (window_is_urgent(win)) {
                add_urgent(task);
            }
            if (window_is_skip_taskbar(win)) {
                remove_task(task);
                schedule_panel_redraw();
            }
        } else if (at == server.atom.WM_STATE) {
            // Iconic state
            TaskState state = (active_task && task->win == active_task->win ? TASK_ACTIVE : TASK_NORMAL);
            if (window_is_iconified(win))
                state = TASK_ICONIFIED;
            set_task_state(task, state);
            schedule_panel_redraw();
        }
        // Window icon changed
        else if (at == server.atom._NET_WM_ICON) {
            task_update_icon(task);
            schedule_panel_redraw();
        }
        // Window desktop changed
        else if (at == server.atom._NET_WM_DESKTOP) {
            int desktop = get_window_desktop(win);
            // fprintf(stderr, "tint2:   Window desktop changed %d, %d\n", task->desktop, desktop);
            // bug in windowmaker : send unecessary 'desktop changed' when focus changed
            if (desktop != task->desktop) {
                task_update_desktop(task);
            }
        } else if (at == server.atom.WM_HINTS) {
            XWMHints *wmhints = XGetWMHints(server.display, win);
            if (wmhints && wmhints->flags & XUrgencyHint) {
                add_urgent(task);
            }
            XFree(wmhints);
            task_update_icon(task);
            schedule_panel_redraw();
        }

        if (!server.got_root_win)
            server.root_win = RootWindow(server.display, server.screen);
    }
}

void handle_event_expose(XEvent *e)
{
    Panel *panel;
    panel = get_panel(e->xany.window);
    if (!panel)
        return;
    // TODO : one panel_refresh per panel ?
    schedule_panel_redraw();
}

void handle_event_configure_notify(XEvent *e)
{
    Window win = e->xconfigure.window;

    // change in root window (xrandr)
    if (win == server.root_win) {
        emit_self_restart("configuration change in the root window");
        return;
    }

    TrayWindow *traywin = systray_find_icon(win);
    if (traywin) {
        systray_reconfigure_event(traywin, e);
        return;
    }

    // 'win' move in another monitor
    if (num_panels > 1 || hide_task_diff_monitor) {
        Task *task = get_task(win);
        if (task) {
            Panel *p = task->area.panel;
            int monitor = get_window_monitor(win);
            if ((hide_task_diff_monitor && p->monitor != monitor && task->area.on_screen) ||
                (hide_task_diff_monitor && p->monitor == monitor && !task->area.on_screen) ||
                (p->monitor != monitor && num_panels > 1)) {
                remove_task(task);
                task = add_task(win);
                if (win == get_active_window()) {
                    set_task_state(task, TASK_ACTIVE);
                    active_task = task;
                }
                schedule_panel_redraw();
            }
        }
    }

    if (server.viewports) {
        Task *task = get_task(win);
        if (task) {
            int desktop = get_window_desktop(win);
            if (task->desktop != desktop) {
                task_update_desktop(task);
            }
        }
    }

    sort_taskbar_for_win(win);
}

gboolean handle_x_event_autohide(XEvent *e)
{
    Panel *panel = get_panel(e->xany.window);
    if (panel && panel_autohide) {
        if (e->type == EnterNotify)
            autohide_trigger_show(panel);
        else if (e->type == LeaveNotify)
            autohide_trigger_hide(panel);
        if (panel->is_hidden) {
            if (e->type == ClientMessage && e->xclient.message_type == server.atom.XdndPosition) {
                hidden_panel_shown_for_dnd = TRUE;
                autohide_show(panel);
            } else {
                // discard further processing of this event because the panel is not visible yet
                return TRUE;
            }
        } else if (hidden_panel_shown_for_dnd && e->type == ClientMessage &&
                   e->xclient.message_type == server.atom.XdndLeave) {
            hidden_panel_shown_for_dnd = FALSE;
            autohide_hide(panel);
        }
    }
    return FALSE;
}

void handle_x_event(XEvent *e)
{
#if HAVE_SN
    if (startup_notifications)
        sn_display_process_event(server.sn_display, e);
#endif // HAVE_SN

    if (handle_x_event_autohide(e))
        return;

    Panel *panel = get_panel(e->xany.window);
    switch (e->type) {
    case ButtonPress: {
        tooltip_hide(0);
        handle_mouse_press_event(e);
        Area *area = find_area_under_mouse(panel, e->xbutton.x, e->xbutton.y);
        if (panel_config.mouse_effects)
            mouse_over(area, TRUE);
        break;
    }

    case ButtonRelease: {
        handle_mouse_release_event(e);
        Area *area = find_area_under_mouse(panel, e->xbutton.x, e->xbutton.y);
        if (panel_config.mouse_effects)
            mouse_over(area, FALSE);
        break;
    }

    case MotionNotify: {
        unsigned int button_mask = Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask;
        if (e->xmotion.state & button_mask)
            handle_mouse_move_event(e);

        Area *area = find_area_under_mouse(panel, e->xmotion.x, e->xmotion.y);
        if (area->_get_tooltip_text)
            tooltip_trigger_show(area, panel, e);
        else
            tooltip_trigger_hide();
        if (panel_config.mouse_effects)
            mouse_over(area, e->xmotion.state & button_mask);
        break;
    }

    case LeaveNotify: {
        tooltip_trigger_hide();
        if (panel_config.mouse_effects)
            mouse_out();
        break;
    }

    case Expose:
        handle_event_expose(e);
        break;

    case PropertyNotify:
        handle_event_property_notify(e);
        break;

    case ConfigureNotify:
        handle_event_configure_notify(e);
        break;

    case ConfigureRequest: {
        TrayWindow *traywin = systray_find_icon(e->xany.window);
        if (traywin)
            systray_reconfigure_event(traywin, e);
        break;
    }

    case ResizeRequest: {
        TrayWindow *traywin = systray_find_icon(e->xany.window);
        if (traywin)
            systray_resize_request_event(traywin, e);
        break;
    }

    case ReparentNotify: {
        if (!systray_enabled)
            break;
        Panel *systray_panel = (Panel *)systray.area.panel;
        if (e->xany.window == systray_panel->main_win) // don't care
            break;
        TrayWindow *traywin = systray_find_icon(e->xreparent.window);
        if (traywin) {
            if (traywin->win == e->xreparent.window) {
                if (traywin->parent == e->xreparent.parent) {
                    embed_icon(traywin);
                } else {
                    remove_icon(traywin);
                }
                break;
            }
        }
        break;
    }

    case UnmapNotify:
        break;

    case DestroyNotify:
        if (e->xany.window == server.composite_manager) {
            // Stop real_transparency
            emit_self_restart("compositor shutdown");
            break;
        }
        if (e->xany.window == g_tooltip.window || !systray_enabled)
            break;
        for (GSList *it = systray.list_icons; it; it = g_slist_next(it)) {
            if (((TrayWindow *)it->data)->win == e->xany.window) {
                systray_destroy_event((TrayWindow *)it->data);
                break;
            }
        }
        break;

    case ClientMessage: {
        XClientMessageEvent *ev = &e->xclient;
        if (ev->data.l[1] == server.atom._NET_WM_CM_S0) {
            if (ev->data.l[2] == None) {
                // Stop real_transparency
                emit_self_restart("compositor changed");
            } else {
                // Start real_transparency
                emit_self_restart("compositor changed");
            }
        }
        if (systray_enabled && e->xclient.message_type == server.atom._NET_SYSTEM_TRAY_OPCODE &&
            e->xclient.format == 32 && e->xclient.window == net_sel_win) {
            handle_systray_event(&e->xclient);
        } else if (e->xclient.message_type == server.atom.XdndEnter) {
            handle_dnd_enter(&e->xclient);
        } else if (e->xclient.message_type == server.atom.XdndPosition) {
            handle_dnd_position(&e->xclient);
        } else if (e->xclient.message_type == server.atom.XdndDrop) {
            handle_dnd_drop(&e->xclient);
        }
        break;
    }

    case SelectionNotify: {
        handle_dnd_selection_notify(&e->xselection);
        break;
    }

    default:
        if (e->type == server.xdamage_event_type) {
            XDamageNotifyEvent *de = (XDamageNotifyEvent *)e;
            TrayWindow *traywin = systray_find_icon(de->drawable);
            if (traywin)
                systray_render_icon(traywin);
        }
    }
}

void handle_x_events()
{
    if (XPending(server.display) > 0) {
        XEvent e;
        XNextEvent(server.display, &e);
        if (debug_fps)
            ts_event_read = get_time();

        handle_x_event(&e);
    }
}

void prepare_fd_set(fd_set *set, int *max_fd)
{
    FD_ZERO(set);
    FD_SET(server.x11_fd, set);
    *max_fd = server.x11_fd;
    if (sigchild_pipe_valid) {
        FD_SET(sigchild_pipe[0], set);
        *max_fd = MAX(*max_fd, sigchild_pipe[0]);
    }
    for (GList *l = panel_config.execp_list; l; l = l->next) {
        Execp *execp = (Execp *)l->data;
        int fd = execp->backend->child_pipe_stdout;
        if (fd > 0) {
            FD_SET(fd, set);
            *max_fd = MAX(*max_fd, fd);
        }
        fd = execp->backend->child_pipe_stderr;
        if (fd > 0) {
            FD_SET(fd, set);
            *max_fd = MAX(*max_fd, fd);
        }
    }
    if (uevent_fd > 0) {
        FD_SET(uevent_fd, set);
        *max_fd = MAX(*max_fd, uevent_fd);
    }
}

void handle_panel_refresh()
{
    if (debug_fps)
        ts_event_processed = get_time();
    panel_refresh = FALSE;

    for (int i = 0; i < num_panels; i++) {
        Panel *panel = &panels[i];
        if (!first_render)
            shrink_panel(panel);

        if (!panel->is_hidden || panel->area.resize_needed) {
            if (panel->temp_pmap)
                XFreePixmap(server.display, panel->temp_pmap);
            panel->temp_pmap = XCreatePixmap(server.display,
                                             server.root_win,
                                             panel->area.width,
                                             panel->area.height,
                                             server.depth);
            render_panel(panel);
        }

        if (panel->is_hidden) {
            if (!panel->hidden_pixmap) {
                panel->hidden_pixmap = XCreatePixmap(server.display,
                                                     server.root_win,
                                                     panel->hidden_width,
                                                     panel->hidden_height,
                                                     server.depth);
                int xoff = 0, yoff = 0;
                if (panel_horizontal && panel_position & BOTTOM)
                    yoff = panel->area.height - panel->hidden_height;
                else if (!panel_horizontal && panel_position & RIGHT)
                    xoff = panel->area.width - panel->hidden_width;
                XCopyArea(server.display,
                          panel->area.pix,
                          panel->hidden_pixmap,
                          server.gc,
                          xoff,
                          yoff,
                          panel->hidden_width,
                          panel->hidden_height,
                          0,
                          0);
            }
            XCopyArea(server.display,
                      panel->hidden_pixmap,
                      panel->main_win,
                      server.gc,
                      0,
                      0,
                      panel->hidden_width,
                      panel->hidden_height,
                      0,
                      0);
            XSetWindowBackgroundPixmap(server.display, panel->main_win, panel->hidden_pixmap);
        } else {
            XCopyArea(server.display,
                      panel->temp_pmap,
                      panel->main_win,
                      server.gc,
                      0,
                      0,
                      panel->area.width,
                      panel->area.height,
                      0,
                      0);
            if (panel == (Panel *)systray.area.panel) {
                if (refresh_systray && panel && !panel->is_hidden) {
                    refresh_systray = FALSE;
                    XSetWindowBackgroundPixmap(server.display, panel->main_win, panel->temp_pmap);
                    refresh_systray_icons();
                }
            }
        }
    }
    if (first_render) {
        first_render = FALSE;
        if (panel_shrink)
            schedule_panel_redraw();
    }

    if (debug_fps)
        ts_render_finished = get_time();
    XFlush(server.display);

    if (debug_fps && ts_event_read > 0) {
        ts_flush_finished = get_time();
        double period = ts_flush_finished - ts_event_read;
        double fps = 1.0 / period;
        sample_fps(fps);
        double proc_ratio = (ts_event_processed - ts_event_read) / period;
        double render_ratio = (ts_render_finished - ts_event_processed) / period;
        double flush_ratio = (ts_flush_finished - ts_render_finished) / period;
        double fps_low, fps_median, fps_high, fps_samples;
        fps_compute_stats(&fps_low, &fps_median, &fps_high, &fps_samples);
        fprintf(stderr,
                BLUE "frame %d: fps = %.0f (low %.0f, med %.0f, high %.0f, samples %.0f) : processing %.0f%%, "
                     "rendering %.0f%%, "
                     "flushing %.0f%%" RESET "\n",
                frame,
                fps,
                fps_low,
                fps_median,
                fps_high,
                fps_samples,
                proc_ratio * 100,
                render_ratio * 100,
                flush_ratio * 100);
#ifdef HAVE_TRACING
        stop_tracing();
        if (fps <= tracing_fps_threshold) {
            print_tracing_events();
        }
#endif
    }
    if (debug_frames) {
        for (int i = 0; i < num_panels; i++) {
            char path[256];
            sprintf(path, "tint2-%d-panel-%d-frame-%d.png", getpid(), i, frame);
            save_panel_screenshot(&panels[i], path);
        }
    }
    frame++;
}

void run_tint2_event_loop()
{
    ts_event_read = 0;
    ts_event_processed = 0;
    ts_render_finished = 0;
    ts_flush_finished = 0;
    first_render = TRUE;

    while (!get_signal_pending()) {
        if (panel_refresh)
            handle_panel_refresh();

        fd_set fds;
        int max_fd;
        prepare_fd_set(&fds, &max_fd);

        // Wait for an event and handle it
        ts_event_read = 0;
        if (XPending(server.display) > 0 || select(max_fd + 1, &fds, 0, 0, get_next_timeout()) >= 0) {
#ifdef HAVE_TRACING
            start_tracing((void*)run_tint2_event_loop);
#endif
            uevent_handler();
            handle_sigchld_events();
            handle_execp_events();
            handle_x_events();
        }

        handle_expired_timers();
    }
}

void tint2(int argc, char **argv, gboolean *restart)
{
    init(argc, argv);

    if (snapshot_path) {
        save_screenshot(snapshot_path);
        cleanup();
        return;
    }

    dnd_init();
    uevent_init();
    run_tint2_event_loop();

    if (get_signal_pending()) {
        cleanup();
        if (get_signal_pending() == SIGUSR1) {
            fprintf(stderr, YELLOW "tint2: %s %d: restarting tint2..." RESET "\n", __FILE__, __LINE__);
            *restart = TRUE;
            return;
        } else if (get_signal_pending() == SIGUSR2) {
            fprintf(stderr, YELLOW "tint2: %s %d: reexecuting tint2..." RESET "\n", __FILE__, __LINE__);
            if (execvp(argv[0], argv) == -1) {
                fprintf(stderr, RED "tint2: %s %d: failed!" RESET "\n", __FILE__, __LINE__);
                return;
            }
        } else {
            // SIGINT, SIGTERM, SIGHUP etc.
            return;
        }
    }
}

int main(int argc, char **argv)
{
    gboolean restart;
    do {
        restart = FALSE;
        tint2(argc, argv, &restart);
    } while(restart);
    return 0;
}
