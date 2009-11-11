/**************************************************************************
*
* Tint2 panel
*
* Copyright (C) 2007 Pål Staurland (staura@gmail.com)
* Modified (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
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
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>
#include <Imlib2.h>
#include <signal.h>

#include "server.h"
#include "window.h"
#include "config.h"
#include "task.h"
#include "taskbar.h"
#include "systraybar.h"
#include "panel.h"
#include "tooltip.h"


void signal_handler(int sig)
{
	// signal handler is light as it should be
	signal_pending = sig;
}


void init (int argc, char *argv[])
{
	int i;

	// read options
	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))	{
			printf("Usage: tint2 [-c] <config_file>\n");
			exit(0);
		}
		if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))	{
			printf("tint2 version 0.7.svn\n");
			exit(0);
		}
		if (!strcmp(argv[i], "-c"))	{
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
	signal(SIGUSR1, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGCHLD, SIG_IGN);		// don't have to wait() after fork()
	signal(SIGALRM, tooltip_sighandler);

	// set global data
	memset(&server, 0, sizeof(Server_global));
	memset(&systray, 0, sizeof(Systraybar));

	server.dsp = XOpenDisplay (NULL);
	if (!server.dsp) {
		fprintf(stderr, "tint2 exit : could not open display.\n");
		exit(0);
	}
	server_init_atoms ();
	server.screen = DefaultScreen (server.dsp);
	server.root_win = RootWindow(server.dsp, server.screen);
	server.depth = DefaultDepth (server.dsp, server.screen);
	server.visual = DefaultVisual (server.dsp, server.screen);
	server.desktop = server_get_current_desktop ();
	XGCValues  gcv;
	server.gc = XCreateGC (server.dsp, server.root_win, (unsigned long)0, &gcv);

	XSetErrorHandler ((XErrorHandler) server_catch_error);

	imlib_context_set_display (server.dsp);
	imlib_context_set_visual (server.visual);
	imlib_context_set_colormap (DefaultColormap (server.dsp, server.screen));

	/* Catch events */
	XSelectInput (server.dsp, server.root_win, PropertyChangeMask|StructureNotifyMask);

	setlocale (LC_ALL, "");

	// load default icon
	char *path;
	const gchar * const *data_dirs;
	data_dirs = g_get_system_data_dirs ();
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
	stop_net();
	cleanup_panel();
	cleanup_tooltip();
	cleanup_clock();
#ifdef ENABLE_BATTERY
	cleanup_battery();
#endif

	if (default_icon) {
		imlib_context_set_image(default_icon);
		imlib_free_image();
	}
	if (config_path) g_free(config_path);
	if (snapshot_path) g_free(snapshot_path);

	if (server.monitor) free(server.monitor);
	XFreeGC(server.dsp, server.gc);
	XCloseDisplay(server.dsp);
}


void get_snapshot(const char *path)
{
	Panel *panel = &panel1[0];

	if (panel->temp_pmap) XFreePixmap(server.dsp, panel->temp_pmap);
	panel->temp_pmap = XCreatePixmap(server.dsp, server.root_win, panel->area.width, panel->area.height, server.depth);

	refresh(&panel->area);

	Imlib_Image img = NULL;
	imlib_context_set_drawable(panel->temp_pmap);
	img = imlib_create_image_from_drawable(0, 0, 0, panel->area.width, panel->area.height, 0);

	imlib_context_set_image(img);
	imlib_save_image(path);
	imlib_free_image();
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
			for (l0 = tskbar->area.list; l0 ; l0 = l0->next) {
				tsk = l0->data;
				if (tsk->area.on_screen && x >= tsk->area.posx && x <= (tsk->area.posx + tsk->area.width)) {
					return tsk;
				}
			}
		}
		else {
			Task *tsk;
			for (l0 = tskbar->area.list; l0 ; l0 = l0->next) {
				tsk = l0->data;
				if (tsk->area.on_screen && y >= tsk->area.posy && y <= (tsk->area.posy + tsk->area.height)) {
					return tsk;
				}
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
	}
}


void event_button_press (XEvent *e)
{
	Panel *panel = get_panel(e->xany.window);
	if (!panel) return;

	task_drag = click_task(panel, e->xbutton.x, e->xbutton.y);

	if (wm_menu && !task_drag && !click_clock(panel, e->xbutton.x, e->xbutton.y) && (e->xbutton.button != 1) ) {
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
		return;
	}

	XLowerWindow (server.dsp, panel->main_win);
}


void event_button_release (XEvent *e)
{
	Panel *panel = get_panel(e->xany.window);
	if (!panel) return;

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
		XLowerWindow (server.dsp, panel->main_win);
		task_drag = 0;
		return;
	}

	Taskbar *tskbar;
	if ( !(tskbar = click_taskbar(panel, e->xbutton.x, e->xbutton.y)) ) {
		// TODO: check better solution to keep window below
		XLowerWindow (server.dsp, panel->main_win);
		task_drag = 0;
		return;
	}

	// drag and drop task
	if (task_drag) {
		if (tskbar != task_drag->area.parent && action == TOGGLE_ICONIFY) {
			if (task_drag->desktop != ALLDESKTOP && panel_mode == MULTI_DESKTOP) {
				windows_set_desktop(task_drag->win, tskbar->desktop);
				if (tskbar->desktop == server.desktop)
					set_active(task_drag->win);
				task_drag = 0;
			}
			return;
		}
		else task_drag = 0;
	}

	// switch desktop
	if (panel_mode == MULTI_DESKTOP) {
		if (tskbar->desktop != server.desktop && action != CLOSE && action != DESKTOP_LEFT && action != DESKTOP_RIGHT)
			set_desktop (tskbar->desktop);
	}

	// action on task
	window_action( click_task(panel, e->xbutton.x, e->xbutton.y), action);

	// to keep window below
	XLowerWindow (server.dsp, panel->main_win);
}


void event_property_notify (XEvent *e)
{
	int i, j;
	Task *tsk;
	Window win = e->xproperty.window;
	Atom at = e->xproperty.atom;

	if (win == server.root_win) {
		if (!server.got_root_win) {
			XSelectInput (server.dsp, server.root_win, PropertyChangeMask|StructureNotifyMask);
			server.got_root_win = 1;
		}

		// Change number of desktops
		else if (at == server.atom._NET_NUMBER_OF_DESKTOPS) {
			server.nb_desktop = server_get_number_of_desktop ();
			cleanup_taskbar();
			init_taskbar();
			visible_object();
			for (i=0 ; i < nb_panel ; i++) {
				panel1[i].area.resize = 1;
			}
			task_refresh_tasklist();
			panel_refresh = 1;
		}
		// Change desktop
		else if (at == server.atom._NET_CURRENT_DESKTOP) {
			int old_desktop = server.desktop;
			server.desktop = server_get_current_desktop ();
			for (i=0 ; i < nb_panel ; i++) {
				Panel *panel = &panel1[i];
				if (panel_mode == MULTI_DESKTOP && panel->g_taskbar.use_active) {
					// redraw both taskbar
					panel->taskbar[old_desktop].area.is_active = 0;
					panel->taskbar[old_desktop].area.resize = 1;
					panel->taskbar[server.desktop].area.is_active = 1;
					panel->taskbar[server.desktop].area.resize = 1;
					panel_refresh = 1;
				}
				// check ALLDESKTOP task => resize taskbar
				Taskbar *tskbar;
				Task *tsk;
				GSList *l;
				tskbar = &panel->taskbar[old_desktop];
				for (l = tskbar->area.list; l ; l = l->next) {
					tsk = l->data;
					if (tsk->desktop == ALLDESKTOP) {
						tsk->area.on_screen = 0;
						tskbar->area.resize = 1;
						panel_refresh = 1;
					}
				}
				tskbar = &panel->taskbar[server.desktop];
				for (l = tskbar->area.list; l ; l = l->next) {
					tsk = l->data;
					if (tsk->desktop == ALLDESKTOP) {
						tsk->area.on_screen = 1;
						tskbar->area.resize = 1;
					}
				}
			}
			if (panel_mode != MULTI_DESKTOP) {
				visible_object();
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
			// some stupid wm send _NET_WM_STATE after the window was minimized to tray???
			if (at != server.atom._NET_WM_STATE)
				return;
			else if (!window_is_skip_taskbar(win)) {
				if (tsk = add_task(win))
					panel_refresh = 1;
				else
					return;
			}
		}
		//printf("atom root_win = %s, %s\n", XGetAtomName(server.dsp, at), tsk->title);

		// Window title changed
		if (at == server.atom._NET_WM_VISIBLE_NAME || at == server.atom._NET_WM_NAME || at == server.atom.WM_NAME) {
			Task *tsk2;
			GSList *l0;
			get_title(tsk);
			// changed other tsk->title
			for (i=0 ; i < nb_panel ; i++) {
				for (j=0 ; j < panel1[i].nb_desktop ; j++) {
					for (l0 = panel1[i].taskbar[j].area.list; l0 ; l0 = l0->next) {
						tsk2 = l0->data;
						if (tsk->win == tsk2->win && tsk != tsk2) {
							tsk2->title = tsk->title;
							tsk2->area.redraw = 1;
						}
					}
				}
			}
			panel_refresh = 1;
		}
		// Demand attention
		else if (at == server.atom._NET_WM_STATE) {
			if (window_is_urgent (win)) {
				task_urgent = tsk;
				tick_urgent = 0;
				time_precision = 1;
			}
			if (window_is_skip_taskbar(win)) {
				remove_task( tsk );
				panel_refresh = 1;
			}
		}
// We do not check for the iconified state, since it only unsets our active window
// but in openbox a shaded window is considered iconified. So we would loose the active window
// property on unshading it again (commented 01.10.2009)
//		else if (at == server.atom.WM_STATE) {
//			// Iconic state
//			// TODO : try to delete following code
//			if (window_is_iconified (win)) {
//				if (task_active) {
//					if (task_active->win == tsk->win) {
//						Task *tsk2;
//						GSList *l0;
//						for (i=0 ; i < nb_panel ; i++) {
//							for (j=0 ; j < panel1[i].nb_desktop ; j++) {
//								for (l0 = panel1[i].taskbar[j].area.list; l0 ; l0 = l0->next) {
//									tsk2 = l0->data;
//									tsk2->area.is_active = 0;
//								}
//							}
//						}
//						task_active = 0;
//					}
//				}
//			}
//		}
		// Window icon changed
		else if (at == server.atom._NET_WM_ICON) {
			get_icon(tsk);
			Task *tsk2;
			GSList *l0;
			for (i=0 ; i < nb_panel ; i++) {
				for (j=0 ; j < panel1[i].nb_desktop ; j++) {
					for (l0 = panel1[i].taskbar[j].area.list; l0 ; l0 = l0->next) {
						tsk2 = l0->data;
						if (tsk->win == tsk2->win && tsk != tsk2) {
							tsk2->icon_width = tsk->icon_width;
							tsk2->icon_height = tsk->icon_height;
							tsk2->icon = tsk->icon;
							tsk2->icon_active = tsk->icon_active;
							tsk2->area.redraw = 1;
						}
					}
				}
			}
			panel_refresh = 1;
		}
		// Window desktop changed
		else if (at == server.atom._NET_WM_DESKTOP) {
			int desktop = window_get_desktop (win);
			int active = tsk->area.is_active;
			//printf("  Window desktop changed %d, %d\n", tsk->desktop, desktop);
			// bug in windowmaker : send unecessary 'desktop changed' when focus changed
			if (desktop != tsk->desktop) {
				remove_task (tsk);
				tsk = add_task (win);
				if (tsk && active) {
					tsk->area.is_active = 1;
					task_active = tsk;
				}
				panel_refresh = 1;
			}
		}
		else if (at == server.atom.WM_HINTS) {
			XWMHints* wmhints = XGetWMHints(server.dsp, win);
			if (wmhints && wmhints->flags & XUrgencyHint) {
				task_urgent = tsk;
				tick_urgent = 0;
				time_precision = 1;
			}
			XFree(wmhints);
		}

		if (!server.got_root_win) server.root_win = RootWindow (server.dsp, server.screen);
	}
}


void event_expose (XEvent *e)
{
	if (e->xany.window == g_tooltip.window)
		tooltip_update();
	else {
		Panel *panel;
		panel = get_panel(e->xany.window);
		if (!panel) return;
		// TODO : one panel_refresh per panel ?
		panel_refresh = 1;
	}
}


void event_configure_notify (Window win)
{
	// change in root window (xrandr)
	if (win == server.root_win) {
		get_monitors();
		init_config();
		config_read_file (config_path);
		init_panel();
		cleanup_config();
		return;
	}

	// 'win' is a trayer icon
	TrayWindow *traywin;
	GSList *l;
	for (l = systray.list_icons; l ; l = l->next) {
		traywin = (TrayWindow*)l->data;
		if (traywin->id == win) {
			//printf("move tray %d\n", traywin->x);
			XMoveResizeWindow(server.dsp, traywin->id, traywin->x, traywin->y, traywin->width, traywin->height);
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
		add_task (win);
		if (win == window_get_active ()) {
			Task *tsk = task_get_task (win);
			tsk->area.is_active = 1;
			task_active = tsk;
		}
		panel_refresh = 1;
	}
}


void event_timer()
{
	struct timeval stv;
	int i;

	if (gettimeofday(&stv, 0)) return;

	if (abs(stv.tv_sec - time_clock.tv_sec) < time_precision) return;
	time_clock.tv_sec = stv.tv_sec;
	time_clock.tv_sec -= time_clock.tv_sec % time_precision;

	// urgent task
	if (task_urgent) {
		if (tick_urgent < max_tick_urgent) {
			task_urgent->area.is_active = !task_urgent->area.is_active;
			task_urgent->area.redraw = 1;
			tick_urgent++;
		}
	}

	// update battery
#ifdef ENABLE_BATTERY
	if (battery_enabled) {
		update_battery();
		for (i=0 ; i < nb_panel ; i++)
			panel1[i].battery.area.resize = 1;
	}
#endif

	// update clock
	if (time1_format) {
		for (i=0 ; i < nb_panel ; i++)
			panel1[i].clock.area.resize = 1;
	}
	panel_refresh = 1;
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
	fd_set fd;
	int x11_fd, i;
	struct timeval tv;
	Panel *panel;
	GSList *it;

	init (argc, argv);

	i = 0;
	init_config();
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
	cleanup_config();
	if (snapshot_path) {
		get_snapshot(snapshot_path);
		cleanup();
		exit(0);
	}

	x11_fd = ConnectionNumber(server.dsp);
	XSync(server.dsp, False);

	while (1) {
		// thanks to AngryLlama for the timer
		// Create a File Description Set containing x11_fd
		FD_ZERO (&fd);
		FD_SET (x11_fd, &fd);

		tv.tv_usec = 500000;
		tv.tv_sec = 0;

		// Wait for X Event or a Timer
		if (select(x11_fd+1, &fd, 0, 0, &tv)) {
			while (XPending (server.dsp)) {
				XNextEvent(server.dsp, &e);

				switch (e.type) {
					case ButtonPress:
						tooltip_hide();
						event_button_press (&e);
						break;

					case ButtonRelease:
						event_button_release(&e);
						break;

					case MotionNotify: {
						if (!g_tooltip.enabled) break;
						Panel* panel = get_panel(e.xmotion.window);
						Task* task = click_task(panel, e.xmotion.x, e.xmotion.y);
						if (task)
							tooltip_trigger_show(task, e.xmotion.x_root, e.xmotion.y_root);
						else
							tooltip_trigger_hide();
						break;
					}

					case LeaveNotify:
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
						if (!systray.area.on_screen)
							break;
						for (it = systray.list_icons; it; it = g_slist_next(it)) {
							if (((TrayWindow*)it->data)->id == e.xany.window) {
								remove_icon((TrayWindow*)it->data);
								break;
							}
						}
					break;

					case ClientMessage:
						if (!systray.area.on_screen) break;
						if (e.xclient.message_type == server.atom._NET_SYSTEM_TRAY_OPCODE && e.xclient.format == 32 && e.xclient.window == net_sel_win) {
							net_message(&e.xclient);
						}
						else if (e.xclient.message_type == server.atom.XdndPosition) {
							dnd_message(&e.xclient);
						}
						break;
				}
			}
		}
		event_timer();

		switch (signal_pending) {
			case SIGUSR1: // reload config file
				signal_pending = 0;
				init_config();
				config_read_file (config_path);
				init_panel();
				cleanup_config();
				break;
			case SIGINT:
			case SIGTERM:
			case SIGHUP:
				cleanup ();
				return 0;
		}

		if (panel_refresh) {
			panel_refresh = 0;

			if (refresh_systray) {
				panel = (Panel*)systray.area.panel;
				XSetWindowBackgroundPixmap (server.dsp, panel->main_win, None);
			}
			for (i=0 ; i < nb_panel ; i++) {
				panel = &panel1[i];

				if (panel->temp_pmap) XFreePixmap(server.dsp, panel->temp_pmap);
				panel->temp_pmap = XCreatePixmap(server.dsp, server.root_win, panel->area.width, panel->area.height, server.depth);

				refresh(&panel->area);
				XCopyArea(server.dsp, panel->temp_pmap, panel->main_win, server.gc, 0, 0, panel->area.width, panel->area.height, 0, 0);
			}
			XFlush (server.dsp);

			if (refresh_systray) {
				refresh_systray = 0;
				panel = (Panel*)systray.area.panel;
				// tint2 doen't draw systray icons. it just redraw background.
				XSetWindowBackgroundPixmap (server.dsp, panel->main_win, panel->temp_pmap);
				// force icon's refresh
				refresh_systray_icon();
			}
		}
	}
}


