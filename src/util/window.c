/**************************************************************************
*
* Tint2 : common windows function
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

#include <Imlib2.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include "common.h"
#include "window.h"
#include "server.h"
#include "panel.h"
#include "taskbar.h"

void activate_window(Window win)
{
	send_event32(win, server.atom._NET_ACTIVE_WINDOW, 2, CurrentTime, 0);
}

void change_window_desktop(Window win, int desktop)
{
	send_event32(win, server.atom._NET_WM_DESKTOP, desktop, 2, 0);
}

void close_window(Window win)
{
	send_event32(win, server.atom._NET_CLOSE_WINDOW, 0, 2, 0);
}

void toggle_window_shade(Window win)
{
	send_event32(win, server.atom._NET_WM_STATE, 2, server.atom._NET_WM_STATE_SHADED, 0);
}

void toggle_window_maximized(Window win)
{
	send_event32(win, server.atom._NET_WM_STATE, 2, server.atom._NET_WM_STATE_MAXIMIZED_VERT, 0);
	send_event32(win, server.atom._NET_WM_STATE, 2, server.atom._NET_WM_STATE_MAXIMIZED_HORZ, 0);
}

gboolean window_is_hidden(Window win)
{
	Window window;
	int count;

	Atom *at = server_get_property(win, server.atom._NET_WM_STATE, XA_ATOM, &count);
	for (int i = 0; i < count; i++) {
		if (at[i] == server.atom._NET_WM_STATE_SKIP_TASKBAR) {
			XFree(at);
			return TRUE;
		}
		// do not add transient_for windows if the transient window is already in the taskbar
		window = win;
		while (XGetTransientForHint(server.display, window, &window)) {
			if (task_get_tasks(window)) {
				XFree(at);
				return TRUE;
			}
		}
	}
	XFree(at);

	at = server_get_property(win, server.atom._NET_WM_WINDOW_TYPE, XA_ATOM, &count);
	for (int i = 0; i < count; i++) {
		if (at[i] == server.atom._NET_WM_WINDOW_TYPE_DOCK || at[i] == server.atom._NET_WM_WINDOW_TYPE_DESKTOP ||
			at[i] == server.atom._NET_WM_WINDOW_TYPE_TOOLBAR || at[i] == server.atom._NET_WM_WINDOW_TYPE_MENU ||
			at[i] == server.atom._NET_WM_WINDOW_TYPE_SPLASH) {
			XFree(at);
			return TRUE;
		}
	}
	XFree(at);

	for (int i = 0; i < num_panels; i++) {
		if (panels[i].main_win == win) {
			return TRUE;
		}
	}

	// specification
	// Windows with neither _NET_WM_WINDOW_TYPE nor WM_TRANSIENT_FOR set
	// MUST be taken as top-level window.
	return FALSE;
}

int get_window_desktop(Window win)
{
	if (!server.viewports)
		return get_property32(win, server.atom._NET_WM_DESKTOP, XA_CARDINAL);

	int x, y, w, h;
	get_window_coordinates(win, &x, &y, &w, &h);

	int desktop = MIN(get_current_desktop(), server.num_desktops - 1);
	// Window coordinates are relative to the current viewport, make them absolute
	x += server.viewports[desktop].x;
	y += server.viewports[desktop].y;

	if (x < 0 || y < 0) {
		int num_results;
		long *x_screen_size = server_get_property(server.root_win, server.atom._NET_DESKTOP_GEOMETRY, XA_CARDINAL, &num_results);
		if (!x_screen_size)
			return 0;
		int x_screen_width = x_screen_size[0];
		int x_screen_height = x_screen_size[1];
		XFree(x_screen_size);

		// Wrap
		if (x < 0)
			x += x_screen_width;
		if (y < 0)
			y += x_screen_height;
	}

	int best_match = -1;
	int match_right = 0;
	int match_bottom = 0;
	// There is an ambiguity when a window is right on the edge between viewports.
	// In that case, prefer the viewports which is on the right and bottom of the window's top-left corner.
	for (int i = 0; i < server.num_desktops; i++) {
		if (x >= server.viewports[i].x && x <= (server.viewports[i].x + server.viewports[i].width) &&
			y >= server.viewports[i].y && y <= (server.viewports[i].y + server.viewports[i].height)) {
			int current_right = x < (server.viewports[i].x + server.viewports[i].width);
			int current_bottom = y < (server.viewports[i].y + server.viewports[i].height);
			if (best_match < 0 || (!match_right && current_right) || (!match_bottom && current_bottom)) {
				best_match = i;
			}
		}
	}

	if (best_match < 0)
		best_match = 0;
	// printf("window %lx : viewport %d, (%d, %d)\n", win, best_match+1, x, y);
	return best_match;
}

int get_window_monitor(Window win)
{
	int i, x, y;
	Window src;

	XTranslateCoordinates(server.display, win, server.root_win, 0, 0, &x, &y, &src);
	int best_match = -1;
	int match_right = 0;
	int match_bottom = 0;
	// There is an ambiguity when a window is right on the edge between screens.
	// In that case, prefer the monitor which is on the right and bottom of the window's top-left corner.
	for (i = 0; i < server.num_monitors; i++) {
		if (x >= server.monitors[i].x && x <= (server.monitors[i].x + server.monitors[i].width) &&
			y >= server.monitors[i].y && y <= (server.monitors[i].y + server.monitors[i].height)) {
			int current_right = x < (server.monitors[i].x + server.monitors[i].width);
			int current_bottom = y < (server.monitors[i].y + server.monitors[i].height);
			if (best_match < 0 || (!match_right && current_right) || (!match_bottom && current_bottom)) {
				best_match = i;
			}
		}
	}

	if (best_match < 0)
		best_match = 0;
	// printf("window %lx : ecran %d, (%d, %d)\n", win, best_match+1, x, y);
	return best_match;
}

void get_window_coordinates(Window win, int *x, int *y, int *w, int *h)
{
	int dummy_int;
	unsigned ww, wh, bw, bh;
	Window src;
	XTranslateCoordinates(server.display, win, server.root_win, 0, 0, x, y, &src);
	XGetGeometry(server.display, win, &src, &dummy_int, &dummy_int, &ww, &wh, &bw, &bh);
	*w = ww + bw;
	*h = wh + bh;
}

gboolean window_is_iconified(Window win)
{
	// EWMH specification : minimization of windows use _NET_WM_STATE_HIDDEN.
	// WM_STATE is not accurate for shaded window and in multi_desktop mode.
	int count;
	Atom *at = server_get_property(win, server.atom._NET_WM_STATE, XA_ATOM, &count);
	for (int i = 0; i < count; i++) {
		if (at[i] == server.atom._NET_WM_STATE_HIDDEN) {
			XFree(at);
			return TRUE;
		}
	}
	XFree(at);
	return FALSE;
}

gboolean window_is_urgent(Window win)
{
	int count;

	Atom *at = server_get_property(win, server.atom._NET_WM_STATE, XA_ATOM, &count);
	for (int i = 0; i < count; i++) {
		if (at[i] == server.atom._NET_WM_STATE_DEMANDS_ATTENTION) {
			XFree(at);
			return TRUE;
		}
	}
	XFree(at);
	return FALSE;
}

gboolean window_is_skip_taskbar(Window win)
{
	int count;

	Atom *at = server_get_property(win, server.atom._NET_WM_STATE, XA_ATOM, &count);
	for (int i = 0; i < count; i++) {
		if (at[i] == server.atom._NET_WM_STATE_SKIP_TASKBAR) {
			XFree(at);
			return 1;
		}
	}
	XFree(at);
	return FALSE;
}

Window get_active_window()
{
	return get_property32(server.root_win, server.atom._NET_ACTIVE_WINDOW, XA_WINDOW);
}

gboolean window_is_active(Window win)
{
	return (win == get_property32(server.root_win, server.atom._NET_ACTIVE_WINDOW, XA_WINDOW));
}

int get_icon_count(gulong *data, int num)
{
	int count, pos, w, h;

	count = 0;
	pos = 0;
	while (pos + 2 < num) {
		w = data[pos++];
		h = data[pos++];
		pos += w * h;
		if (pos > num || w <= 0 || h <= 0)
			break;
		count++;
	}

	return count;
}

gulong *get_best_icon(gulong *data, int icon_count, int num, int *iw, int *ih, int best_icon_size)
{
	int width[icon_count], height[icon_count], pos, i, w, h;
	gulong *icon_data[icon_count];

	/* List up icons */
	pos = 0;
	i = icon_count;
	while (i--) {
		w = data[pos++];
		h = data[pos++];
		if (pos + w * h > num)
			break;

		width[i] = w;
		height[i] = h;
		icon_data[i] = &data[pos];

		pos += w * h;
	}

	/* Try to find exact size */
	int icon_num = -1;
	for (i = 0; i < icon_count; i++) {
		if (width[i] == best_icon_size) {
			icon_num = i;
			break;
		}
	}

	/* Take the biggest or whatever */
	if (icon_num < 0) {
		int highest = 0;
		for (i = 0; i < icon_count; i++) {
			if (width[i] > highest) {
				icon_num = i;
				highest = width[i];
			}
		}
	}

	*iw = width[icon_num];
	*ih = height[icon_num];
	return icon_data[icon_num];
}
