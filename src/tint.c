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

#include <sys/stat.h>
#include <unistd.h>
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

#include <version.h>
#include "server.h"
#include "window.h"
#include "config.h"
#include "task.h"
#include "taskbar.h"
#include "systraybar.h"
#include "launcher.h"
#include "panel.h"
#include "tooltip.h"
#include "timer.h"
#include "xsettings-client.h"


void signal_handler(int sig)
{
	// signal handler is light as it should be
	signal_pending = sig;
}


void init (int argc, char *argv[])
{
	int i;

	// set global data
	default_config();
	default_timeout();
	default_systray();
	memset(&server, 0, sizeof(Server_global));
#ifdef ENABLE_BATTERY
	default_battery();
#endif
	default_clock();
	default_launcher();
	default_taskbar();
	default_tooltip();
	default_panel();

	// read options
	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))	{
			printf("Usage: tint2 [-c] <config_file>\n");
			exit(0);
		}
		if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))	{
			printf("tint2 version %s\n", VERSION_STRING);
			exit(0);
		}
		if (!strcmp(argv[i], "-c")) {
			i++;
			if (i < argc)
				config_path = strdup(argv[i]);
		}
		if (!strcmp(argv[i], "-s"))	{
			i++;
			if (i < argc)
				snapshot_path = strdup(argv[i]);
		}
	}
	// Set signal handler
	signal_pending = 0;
	struct sigaction sa = { .sa_handler = signal_handler };
	struct sigaction sa_chld = { .sa_handler = SIG_DFL, .sa_flags = SA_NOCLDWAIT };
	sigaction(SIGUSR1, &sa, 0);
	sigaction(SIGINT, &sa, 0);
	sigaction(SIGTERM, &sa, 0);
	sigaction(SIGHUP, &sa, 0);
	sigaction(SIGCHLD, &sa_chld, 0);

	// BSD does not support pselect(), therefore we have to use select and hope that we do not
	// end up in a race condition there (see 'man select()' on a linux machine for more information)
	// block all signals, such that no race conditions occur before pselect in our main loop
//	sigset_t block_mask;
//	sigaddset(&block_mask, SIGINT);
//	sigaddset(&block_mask, SIGTERM);
//	sigaddset(&block_mask, SIGHUP);
//	sigaddset(&block_mask, SIGUSR1);
//	sigprocmask(SIG_BLOCK, &block_mask, 0);
}

void init_X11()
{
	server.dsp = XOpenDisplay (NULL);
	if (!server.dsp) {
		fprintf(stderr, "tint2 exit : could not open display.\n");
		exit(0);
	}
	server_init_atoms ();
	server.screen = DefaultScreen (server.dsp);
	server.root_win = RootWindow(server.dsp, server.screen);
	server.desktop = server_get_current_desktop ();
	server_init_visual();
	XSetErrorHandler ((XErrorHandler) server_catch_error);

	imlib_context_set_display (server.dsp);
	imlib_context_set_visual (server.visual);
	imlib_context_set_colormap (server.colormap);

	/* Catch events */
	XSelectInput (server.dsp, server.root_win, PropertyChangeMask|StructureNotifyMask);

	setlocale (LC_ALL, "");
	// config file use '.' as decimal separator
	setlocale(LC_NUMERIC, "POSIX");
	
	// load default icon
	gchar *path;
	const gchar * const *data_dirs;
	data_dirs = g_get_system_data_dirs ();
	int i;
	for (i = 0; data_dirs[i] != NULL; i++)	{
		path = g_build_filename(data_dirs[i], "tint2", "default_icon.png", NULL);
		if (g_file_test (path, G_FILE_TEST_EXISTS))
			default_icon = imlib_load_image(path);
		g_free(path);
	}

	// get monitor and desktop config
	get_monitors();
	get_desktops();
}


void cleanup()
{
	cleanup_systray();
	cleanup_tooltip();
	cleanup_clock();
	cleanup_launcher();
#ifdef ENABLE_BATTERY
	cleanup_battery();
#endif
	cleanup_panel();
	cleanup_config();

	if (default_icon) {
		imlib_context_set_image(default_icon);
		imlib_free_image();
	}
	imlib_context_disconnect_display();

	cleanup_server();
	cleanup_timeout();
	if (server.dsp) XCloseDisplay(server.dsp);
}


void get_snapshot(const char *path)
{
	Panel *panel = &panel1[0];

	if (panel->area.width > server.monitor[0].width)
		panel->area.width = server.monitor[0].width;

	panel->temp_pmap = XCreatePixmap(server.dsp, server.root_win, panel->area.width, panel->area.height, server.depth);
	rendering(panel);

	Imlib_Image img = NULL;
	imlib_context_set_drawable(panel->temp_pmap);
	img = imlib_create_image_from_drawable(0, 0, 0, panel->area.width, panel->area.height, 0);

	imlib_context_set_image(img);
	if (!panel_horizontal) {
		// rotate 90° vertical panel
		imlib_image_flip_horizontal();
		imlib_image_flip_diagonal();
	}
	imlib_save_image(path);
	imlib_free_image();
}


void window_action (Task *tsk, int action)
{
	if (!tsk) return;
	int desk;
	switch (action) {
		case CLOSE:
			set_close (tsk->win);
			break;
		case TOGGLE:
			set_active(tsk->win);
			break;
		case ICONIFY:
			XIconifyWindow (server.dsp, tsk->win, server.screen);
			break;
		case TOGGLE_ICONIFY:
			if (task_active && tsk->win == task_active->win)
				XIconifyWindow (server.dsp, tsk->win, server.screen);
			else
				set_active (tsk->win);
			break;
		case SHADE:
			window_toggle_shade (tsk->win);
			break;
		case MAXIMIZE_RESTORE:
			window_maximize_restore (tsk->win);
			break;
		case MAXIMIZE:
			window_maximize_restore (tsk->win);
			break;
		case RESTORE:
			window_maximize_restore (tsk->win);
			break;
		case DESKTOP_LEFT:
			if ( tsk->desktop == 0 ) break;
			desk = tsk->desktop - 1;
			windows_set_desktop(tsk->win, desk);
			if (desk == server.desktop)
				set_active(tsk->win);
			break;
		case DESKTOP_RIGHT:
			if (tsk->desktop == server.nb_desktop ) break;
			desk = tsk->desktop + 1;
			windows_set_desktop(tsk->win, desk);
			if (desk == server.desktop)
				set_active(tsk->win);
			break;
		case NEXT_TASK:
			if (task_active) {
				Task *tsk1;
				tsk1 = next_task(task_active);
				set_active(tsk1->win);
			}
			break;
		case PREV_TASK:
			if (task_active) {
				Task *tsk1;
				tsk1 = prev_task(task_active);
				set_active(tsk1->win);
			}
	}
}


int tint2_handles_click(Panel* panel, XButtonEvent* e)
{
	Task* task = click_task(panel, e->x, e->y);
	if (task) {
		if(   (e->button == 1)
			 || (e->button == 2 && mouse_middle != 0)
			 || (e->button == 3 && mouse_right != 0)
			 || (e->button == 4 && mouse_scroll_up != 0)
			 || (e->button == 5 && mouse_scroll_down !=0) )
		{
			return 1;
		}
		else
			return 0;
	}
	LauncherIcon *icon = click_launcher_icon(panel, e->x, e->y);
	if (icon) {
		if (e->button == 1) {
			return 1;
		} else {
			return 0;
		}
	}
	// no launcher/task clicked --> check if taskbar clicked
	Taskbar *tskbar = click_taskbar(panel, e->x, e->y);
	if (tskbar && e->button == 1 && panel_mode == MULTI_DESKTOP)
		return 1;
	if (click_clock(panel, e->x, e->y)) {
		if ( (e->button == 1 && clock_lclick_command) || (e->button == 3 && clock_rclick_command) )
			return 1;
		else
			return 0;
	}
	return 0;
}


void forward_click(XEvent* e)
{
	// forward the click to the desktop window (thanks conky)
	XUngrabPointer(server.dsp, e->xbutton.time);
	e->xbutton.window = server.root_win;
	// icewm doesn't open under the mouse.
	// and xfce doesn't open at all.
	e->xbutton.x = e->xbutton.x_root;
	e->xbutton.y = e->xbutton.y_root;
	//printf("**** %d, %d\n", e->xbutton.x, e->xbutton.y);
	//XSetInputFocus(server.dsp, e->xbutton.window, RevertToParent, e->xbutton.time);
	XSendEvent(server.dsp, e->xbutton.window, False, ButtonPressMask, e);
}


void event_button_press (XEvent *e)
{
	Panel *panel = get_panel(e->xany.window);
	if (!panel) return;


	if (wm_menu && !tint2_handles_click(panel, &e->xbutton) ) {
		forward_click(e);
		return;
	}
	task_drag = click_task(panel, e->xbutton.x, e->xbutton.y);

	if (panel_layer == BOTTOM_LAYER)
		XLowerWindow (server.dsp, panel->main_win);
}

void event_button_motion_notify (XEvent *e)
{
	Panel * panel = get_panel(e->xany.window);
	if(!panel || !task_drag)
		return;

	// Find the taskbar on the event's location
	Taskbar * event_taskbar = click_taskbar(panel, e->xbutton.x, e->xbutton.y);
	if(event_taskbar == NULL)
		return;

	// Find the task on the event's location
	Task * event_task = click_task(panel, e->xbutton.x, e->xbutton.y);

	// If the event takes place on the same taskbar as the task being dragged
	if(event_taskbar == task_drag->area.parent)	{
		// Swap the task_drag with the task on the event's location (if they differ)
		if(event_task && event_task != task_drag) {
			GSList * drag_iter = g_slist_find(event_taskbar->area.list, task_drag);
			GSList * task_iter = g_slist_find(event_taskbar->area.list, event_task);
			if(drag_iter && task_iter) {
				gpointer temp = task_iter->data;
				task_iter->data = drag_iter->data;
				drag_iter->data = temp;
				event_taskbar->area.resize = 1;
				panel_refresh = 1;
				task_dragged = 1;
			}
		}
	}
	else { // The event is on another taskbar than the task being dragged
		if(task_drag->desktop == ALLDESKTOP || panel_mode != MULTI_DESKTOP)
			return;

		Taskbar * drag_taskbar = (Taskbar*)task_drag->area.parent;
		drag_taskbar->area.list = g_slist_remove(drag_taskbar->area.list, task_drag);

		if(event_taskbar->area.posx > drag_taskbar->area.posx || event_taskbar->area.posy > drag_taskbar->area.posy) {
			int i = (taskbarname_enabled) ? 1 : 0;
			event_taskbar->area.list = g_slist_insert(event_taskbar->area.list, task_drag, i);
		}
		else
			event_taskbar->area.list = g_slist_append(event_taskbar->area.list, task_drag);

		// Move task to other desktop (but avoid the 'Window desktop changed' code in 'event_property_notify')
		task_drag->area.parent = event_taskbar;
		task_drag->desktop = event_taskbar->desktop;

		windows_set_desktop(task_drag->win, event_taskbar->desktop);

		event_taskbar->area.resize = 1;
		drag_taskbar->area.resize = 1;
		task_dragged = 1;
		panel_refresh = 1;
	}
}

void event_button_release (XEvent *e)
{
	Panel *panel = get_panel(e->xany.window);
	if (!panel) return;

	if (wm_menu && !tint2_handles_click(panel, &e->xbutton)) {
		forward_click(e);
		if (panel_layer == BOTTOM_LAYER)
			XLowerWindow (server.dsp, panel->main_win);
		task_drag = 0;
		return;
	}

	int action = TOGGLE_ICONIFY;
	switch (e->xbutton.button) {
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

	if ( click_clock(panel, e->xbutton.x, e->xbutton.y)) {
		clock_action(e->xbutton.button);
		if (panel_layer == BOTTOM_LAYER)
			XLowerWindow (server.dsp, panel->main_win);
		task_drag = 0;
		return;
	}

	if ( click_launcher(panel, e->xbutton.x, e->xbutton.y)) {
		LauncherIcon *icon = click_launcher_icon(panel, e->xbutton.x, e->xbutton.y);
		if (icon) {
			launcher_action(icon);
		}
		task_drag = 0;
		return;
	}

	Taskbar *tskbar;
	if ( !(tskbar = click_taskbar(panel, e->xbutton.x, e->xbutton.y)) ) {
		// TODO: check better solution to keep window below
		if (panel_layer == BOTTOM_LAYER)
			XLowerWindow (server.dsp, panel->main_win);
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
	if (panel_mode == MULTI_DESKTOP) {
		if (tskbar->desktop != server.desktop && action != CLOSE && action != DESKTOP_LEFT && action != DESKTOP_RIGHT)
			set_desktop (tskbar->desktop);
	}

	// action on task
	window_action( click_task(panel, e->xbutton.x, e->xbutton.y), action);

	// to keep window below
	if (panel_layer == BOTTOM_LAYER)
		XLowerWindow (server.dsp, panel->main_win);
}


void event_property_notify (XEvent *e)
{
	int i;
	Task *tsk;
	Window win = e->xproperty.window;
	Atom at = e->xproperty.atom;

	if (xsettings_client)
		xsettings_client_process_event(xsettings_client, e);
	if (win == server.root_win) {
		if (!server.got_root_win) {
			XSelectInput (server.dsp, server.root_win, PropertyChangeMask|StructureNotifyMask);
			server.got_root_win = 1;
		}

		// Change name of desktops
		else if (at == server.atom._NET_DESKTOP_NAMES) {
			if (!taskbarname_enabled) return;
			GSList *l, *list = server_get_name_of_desktop();
			int j;
			gchar *name;
			Taskbar *tskbar;
			for (i=0 ; i < nb_panel ; i++) {
				for (j=0, l=list ; j < panel1[i].nb_desktop ; j++) {
					if (l) {
						name = g_strdup(l->data);
						l = l->next;
					}
					else
						name = g_strdup_printf("%d", j+1);
					tskbar = &panel1[i].taskbar[j];
					if (strcmp(name, tskbar->bar_name.name) != 0) {
						g_free(tskbar->bar_name.name);
						tskbar->bar_name.name = name;
						tskbar->bar_name.area.resize = 1;
					}
					else
						g_free(name);
				}
			}
			for (l=list ; l ; l = l->next)
				g_free(l->data);
			g_slist_free(list);
			panel_refresh = 1;
		}
		// Change number of desktops
		else if (at == server.atom._NET_NUMBER_OF_DESKTOPS) {
			if (!taskbar_enabled) return;
			server.nb_desktop = server_get_number_of_desktop ();
			cleanup_taskbar();
			init_taskbar();
			for (i=0 ; i < nb_panel ; i++) {
				init_taskbar_panel(&panel1[i]);
				set_panel_items_order(&panel1[i]);
				visible_taskbar(&panel1[i]);
				panel1[i].area.resize = 1;
			}
			task_refresh_tasklist();
			active_task();
			panel_refresh = 1;
		}
		// Change desktop
		else if (at == server.atom._NET_CURRENT_DESKTOP) {
			if (!taskbar_enabled) return;
			int old_desktop = server.desktop;
			server.desktop = server_get_current_desktop ();
			for (i=0 ; i < nb_panel ; i++) {
				Panel *panel = &panel1[i];
				set_taskbar_state(&panel->taskbar[old_desktop], TASKBAR_NORMAL);
				set_taskbar_state(&panel->taskbar[server.desktop], TASKBAR_ACTIVE);
				// check ALLDESKTOP task => resize taskbar
				Taskbar *tskbar;
				Task *tsk;
				GSList *l;
				if (server.nb_desktop > old_desktop) {
					tskbar = &panel->taskbar[old_desktop];
					l = tskbar->area.list;
					if (taskbarname_enabled) l = l->next;
					for (; l ; l = l->next) {
						tsk = l->data;
						if (tsk->desktop == ALLDESKTOP) {
							tsk->area.on_screen = 0;
							tskbar->area.resize = 1;
							panel_refresh = 1;
						}
					}
				}
				tskbar = &panel->taskbar[server.desktop];
				l = tskbar->area.list;
				if (taskbarname_enabled) l = l->next;
				for (; l ; l = l->next) {
					tsk = l->data;
					if (tsk->desktop == ALLDESKTOP) {
						tsk->area.on_screen = 1;
						tskbar->area.resize = 1;
					}
				}
			}
		}
		// Window list
		else if (at == server.atom._NET_CLIENT_LIST) {
			task_refresh_tasklist();
			panel_refresh = 1;
		}
		// Change active
		else if (at == server.atom._NET_ACTIVE_WINDOW) {
			active_task();
			panel_refresh = 1;
		}
		else if (at == server.atom._XROOTPMAP_ID) {
			// change Wallpaper
			for (i=0 ; i < nb_panel ; i++) {
				set_panel_background(&panel1[i]);
			}
			panel_refresh = 1;
		}
	}
	else {
		tsk = task_get_task (win);
		if (!tsk) {
			if (at != server.atom._NET_WM_STATE)
				return;
			else {
				// xfce4 sends _NET_WM_STATE after minimized to tray, so we need to check if window is mapped
				// if it is mapped and not set as skip_taskbar, we must add it to our task list
				XWindowAttributes wa;
				XGetWindowAttributes(server.dsp, win, &wa);
				if (wa.map_state == IsViewable && !window_is_skip_taskbar(win)) {
					if ( (tsk = add_task(win)) )
						panel_refresh = 1;
					else
						return;
				}
				else
					return;
			}
		}
		//printf("atom root_win = %s, %s\n", XGetAtomName(server.dsp, at), tsk->title);

		// Window title changed
		if (at == server.atom._NET_WM_VISIBLE_NAME || at == server.atom._NET_WM_NAME || at == server.atom.WM_NAME) {
			if (get_title(tsk)) {
				if (g_tooltip.mapped && (g_tooltip.area == (Area*)tsk)) {
					tooltip_copy_text((Area*)tsk);
					tooltip_update();
				}
				panel_refresh = 1;
			}
		}
		// Demand attention
		else if (at == server.atom._NET_WM_STATE) {
			if (window_is_urgent (win)) {
				add_urgent(tsk);
			}
			if (window_is_skip_taskbar(win)) {
				remove_task( tsk );
				panel_refresh = 1;
			}
		}
		else if (at == server.atom.WM_STATE) {
			// Iconic state
			int state = (task_active && tsk->win == task_active->win ? TASK_ACTIVE : TASK_NORMAL);
			if (window_is_iconified(win))
				state = TASK_ICONIFIED;
			set_task_state(tsk, state);
			panel_refresh = 1;
		}
		// Window icon changed
		else if (at == server.atom._NET_WM_ICON) {
			get_icon(tsk);
			panel_refresh = 1;
		}
		// Window desktop changed
		else if (at == server.atom._NET_WM_DESKTOP) {
			int desktop = window_get_desktop (win);
			//printf("  Window desktop changed %d, %d\n", tsk->desktop, desktop);
			// bug in windowmaker : send unecessary 'desktop changed' when focus changed
			if (desktop != tsk->desktop) {
				remove_task (tsk);
				tsk = add_task (win);
				active_task();
				panel_refresh = 1;
			}
		}
		else if (at == server.atom.WM_HINTS) {
			XWMHints* wmhints = XGetWMHints(server.dsp, win);
			if (wmhints && wmhints->flags & XUrgencyHint) {
				add_urgent(tsk);
			}
			XFree(wmhints);
		}

		if (!server.got_root_win) server.root_win = RootWindow (server.dsp, server.screen);
	}
}


void event_expose (XEvent *e)
{
	Panel *panel;
	panel = get_panel(e->xany.window);
	if (!panel) return;
	// TODO : one panel_refresh per panel ?
	panel_refresh = 1;
}


void event_configure_notify (Window win)
{
	// change in root window (xrandr)
	if (win == server.root_win) {
		signal_pending = SIGUSR1;
		return;
	}

	// 'win' is a trayer icon
	TrayWindow *traywin;
	GSList *l;
	for (l = systray.list_icons; l ; l = l->next) {
		traywin = (TrayWindow*)l->data;
		if (traywin->tray_id == win) {
			//printf("move tray %d\n", traywin->x);
			XMoveResizeWindow(server.dsp, traywin->id, traywin->x, traywin->y, traywin->width, traywin->height);
			XResizeWindow(server.dsp, traywin->tray_id, traywin->width, traywin->height);
			panel_refresh = 1;
			return;
		}
	}

	// 'win' move in another monitor
	if (nb_panel == 1) return;
	Task *tsk = task_get_task (win);
	if (!tsk) return;

	Panel *p = tsk->area.panel;
	if (p->monitor != window_get_monitor (win)) {
		remove_task (tsk);
		tsk = add_task (win);
		if (win == window_get_active ()) {
			set_task_state(tsk, TASK_ACTIVE);
			task_active = tsk;
		}
		panel_refresh = 1;
	}
}


void dnd_message(XClientMessageEvent *e)
{
	Panel *panel = get_panel(e->window);
	int x, y, mapX, mapY;
	Window child;
	x = (e->data.l[2] >> 16) & 0xFFFF;
	y = e->data.l[2] & 0xFFFF;
	XTranslateCoordinates(server.dsp, server.root_win, e->window, x, y, &mapX, &mapY, &child);
	Task* task = click_task(panel, mapX, mapY);
	if (task) {
		if (task->desktop != server.desktop )
			set_desktop (task->desktop);
		window_action(task, TOGGLE);
	}

	// send XdndStatus event to get more XdndPosition events
	XClientMessageEvent se;
	se.type = ClientMessage;
	se.window = e->data.l[0];
	se.message_type = server.atom.XdndStatus;
	se.format = 32;
	se.data.l[0] = e->window;  // XID of the target window
	se.data.l[1] = 0;          // bit 0: accept drop    bit 1: send XdndPosition events if inside rectangle
	se.data.l[2] = 0;          // Rectangle x,y for which no more XdndPosition events
	se.data.l[3] = (1 << 16) | 1;  // Rectangle w,h for which no more XdndPosition events
	se.data.l[4] = None;       // None = drop will not be accepted
	XSendEvent(server.dsp, e->data.l[0], False, NoEventMask, (XEvent*)&se);
}


int main (int argc, char *argv[])
{
	XEvent e;
	XClientMessageEvent *ev;
	fd_set fdset;
	int x11_fd, i;
	Panel *panel;
	GSList *it;
	struct timeval* timeout;
	int hidden_dnd = 0;

start:
	init (argc, argv);
	init_X11();

	i = 0;
	if (config_path)
		i = config_read_file (config_path);
	else
		i = config_read ();
	if (!i) {
		fprintf(stderr, "usage: tint2 [-c] <config_file>\n");
		cleanup();
		exit(1);
	}

	init_panel();
	if (snapshot_path) {
		get_snapshot(snapshot_path);
		cleanup();
		exit(0);
	}

	int damage_event, damage_error;
	XDamageQueryExtension(server.dsp, &damage_event, &damage_error);
	x11_fd = ConnectionNumber(server.dsp);
	XSync(server.dsp, False);

//	sigset_t empty_mask;
//	sigemptyset(&empty_mask);

	while (1) {
		if (panel_refresh) {
			panel_refresh = 0;

			for (i=0 ; i < nb_panel ; i++) {
				panel = &panel1[i];

				if (panel->is_hidden) {
					XCopyArea(server.dsp, panel->hidden_pixmap, panel->main_win, server.gc, 0, 0, panel->hidden_width, panel->hidden_height, 0, 0);
					XSetWindowBackgroundPixmap(server.dsp, panel->main_win, panel->hidden_pixmap);
				}
				else {
					if (panel->temp_pmap) XFreePixmap(server.dsp, panel->temp_pmap);
					panel->temp_pmap = XCreatePixmap(server.dsp, server.root_win, panel->area.width, panel->area.height, server.depth);
					rendering(panel);
					XCopyArea(server.dsp, panel->temp_pmap, panel->main_win, server.gc, 0, 0, panel->area.width, panel->area.height, 0, 0);
				}
			}
			XFlush (server.dsp);

			panel = (Panel*)systray.area.panel;
			if (refresh_systray && panel && !panel->is_hidden) {
				refresh_systray = 0;
				// tint2 doen't draw systray icons. it just redraw background.
				XSetWindowBackgroundPixmap (server.dsp, panel->main_win, panel->temp_pmap);
				// force icon's refresh
				refresh_systray_icon();
			}
		}

		// thanks to AngryLlama for the timer
		// Create a File Description Set containing x11_fd
		FD_ZERO (&fdset);
		FD_SET (x11_fd, &fdset);
		update_next_timeout();
		if (next_timeout.tv_sec >= 0 && next_timeout.tv_usec >= 0)
			timeout = &next_timeout;
		else
			timeout = 0;

		// Wait for X Event or a Timer
		if (select(x11_fd+1, &fdset, 0, 0, timeout) > 0) {
			while (XPending (server.dsp)) {
				XNextEvent(server.dsp, &e);

				panel = get_panel(e.xany.window);
				if (panel && panel_autohide) {
					if (e.type == EnterNotify)
						autohide_trigger_show(panel);
					else if (e.type == LeaveNotify)
						autohide_trigger_hide(panel);
					if (panel->is_hidden) {
						if (e.type == ClientMessage && e.xclient.message_type == server.atom.XdndPosition) {
							hidden_dnd = 1;
							autohide_show(panel);
						}
						else
							continue;   // discard further processing of this event because the panel is not visible yet
					}
					else if (hidden_dnd && e.type == ClientMessage && e.xclient.message_type == server.atom.XdndLeave) {
						hidden_dnd = 0;
						autohide_hide(panel);
					}
				}

				switch (e.type) {
					case ButtonPress:
						tooltip_hide(0);
						event_button_press (&e);
						break;

					case ButtonRelease:
						event_button_release(&e);
						break;

					case MotionNotify: {
						unsigned int button_mask = Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask;
						if (e.xmotion.state & button_mask)
							event_button_motion_notify (&e);

						if (!g_tooltip.enabled) break;
						Panel* panel = get_panel(e.xmotion.window);
						Area* area = click_area(panel, e.xmotion.x, e.xmotion.y);
						if (area->_get_tooltip_text)
							tooltip_trigger_show(area, panel, &e);
						else
							tooltip_trigger_hide();
						break;
					}

					case LeaveNotify:
						if (g_tooltip.enabled)
							tooltip_trigger_hide();
						break;

					case Expose:
						event_expose(&e);
						break;

					case PropertyNotify:
						event_property_notify(&e);
						break;

					case ConfigureNotify:
						event_configure_notify (e.xconfigure.window);
						break;

					case ReparentNotify:
						if (!systray_enabled)
							break;
						panel = (Panel*)systray.area.panel;
						if (e.xany.window == panel->main_win) // reparented to us
							break;
						// FIXME: 'reparent to us' badly detected => disabled
						break;
					case UnmapNotify:
					case DestroyNotify:
						if (e.xany.window == server.composite_manager) {
							// Stop real_transparency
							signal_pending = SIGUSR1;
							break;
						}
						if (e.xany.window == g_tooltip.window || !systray_enabled)
							break;
						for (it = systray.list_icons; it; it = g_slist_next(it)) {
							if (((TrayWindow*)it->data)->tray_id == e.xany.window) {
								remove_icon((TrayWindow*)it->data);
								break;
							}
						}
						break;

					case ClientMessage:
						ev = &e.xclient;
						if (ev->data.l[1] == server.atom._NET_WM_CM_S0) {
							if (ev->data.l[2] == None)
								// Stop real_transparency
								signal_pending = SIGUSR1;
							else
								// Start real_transparency
								signal_pending = SIGUSR1;
						}
						if (systray_enabled && e.xclient.message_type == server.atom._NET_SYSTEM_TRAY_OPCODE && e.xclient.format == 32 && e.xclient.window == net_sel_win) {
							net_message(&e.xclient);
						}
						else if (e.xclient.message_type == server.atom.XdndPosition) {
							dnd_message(&e.xclient);
						}
						break;

					default:
						if (e.type == XDamageNotify+damage_event) {
							// union needed to avoid strict-aliasing warnings by gcc
							union { XEvent e; XDamageNotifyEvent de; } event_union = {.e=e};
							TrayWindow *traywin;
							GSList *l;
							XDamageNotifyEvent* de = &event_union.de;
							for (l = systray.list_icons; l ; l = l->next) {
								traywin = (TrayWindow*)l->data;
								if ( traywin->id == de->drawable ) {
									systray_render_icon(traywin);
									break;
								}
							}
						}
				}
			}
		}

		callback_timeout_expired();

		if (signal_pending) {
			cleanup();
			if (signal_pending == SIGUSR1) {
				// restart tint2
				// SIGUSR1 used when : user's signal, composite manager stop/start or xrandr
				FD_CLR (x11_fd, &fdset); // not sure if needed
				goto start;
			}
			else {
				// SIGINT, SIGTERM, SIGHUP
				return 0;
			}
		}
	}
}


