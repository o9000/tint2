/**************************************************************************
* Tint2 : systraybar
*
* Copyright (C) 2009 thierry lorthiois (lorthiois@bbsoft.fr)
* based on 'docker-1.5' from Ben Jansens.
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

#include "systraybar.h"
#include "server.h"
#include "panel.h"

GSList *icons;

/* defined in the systray spec */
#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

// selection window
Window net_sel_win = None, hint_win = None;

// freedesktop specification doesn't allow multi systray
Systraybar systray;
int refresh_systray;


void init_systray()
{
	Panel *panel = &panel1[0];

	if (systray.area.on_screen)
		systray.area.on_screen = init_net();

	if (!systray.area.on_screen)
		return;

	systray.area.parent = panel;
	systray.area.panel = panel;
	systray.area._draw_foreground = draw_systray;
	systray.area._resize = resize_systray;
	systray.area.resize = 1;
	systray.area.redraw = 1;
	refresh_systray = 0;

	// configure systray
	// draw only one systray (even with multi panel)
	if (panel_horizontal) {
		systray.area.posy = panel->area.pix.border.width + panel->area.paddingy;
		systray.area.height = panel->area.height - (2 * systray.area.posy);
	}
	else {
		systray.area.posx = panel->area.pix.border.width + panel->area.paddingy;
		systray.area.width = panel->area.width - (2 * panel->area.pix.border.width) - (2 * panel->area.paddingy);
	}
}


void cleanup_systray()
{
	if (systray.list_icons) {
		// remove_icon change systray.list_icons
		while(systray.list_icons)
			remove_icon((TrayWindow*)systray.list_icons->data);

		g_slist_free(systray.list_icons);
		systray.list_icons = 0;
	}

	free_area(&systray.area);
	cleanup_net();
}


void draw_systray(void *obj, cairo_t *c, int active)
{
	// tint2 don't draw systray icons. just the background.
	refresh_systray = 1;
}


void resize_systray(void *obj)
{
	Systraybar *sysbar = obj;
	Panel *panel = sysbar->area.panel;
	TrayWindow *traywin;
	GSList *l;
	int count, posx, posy;
	int icon_size;

	if (panel_horizontal)
		icon_size = sysbar->area.height;
	else
		icon_size = sysbar->area.width;
	icon_size = icon_size - (2 * sysbar->area.pix.border.width) - (2 * sysbar->area.paddingy);
	count = g_slist_length(systray.list_icons);

	if (panel_horizontal) {
		if (!count) systray.area.width = 0;
		else systray.area.width = (2 * systray.area.pix.border.width) + (2 * systray.area.paddingxlr) + (icon_size * count) + ((count-1) * systray.area.paddingx);

		systray.area.posx = panel->area.width - panel->area.pix.border.width - panel->area.paddingxlr - systray.area.width;
		if (panel->clock.area.on_screen)
			systray.area.posx -= (panel->clock.area.width + panel->area.paddingx);
#ifdef ENABLE_BATTERY
		if (panel->battery.area.on_screen)
			systray.area.posx -= (panel->battery.area.width + panel->area.paddingx);
#endif
	}
	else {
		if (!count) systray.area.height = 0;
		else systray.area.height = (2 * systray.area.pix.border.width) + (2 * systray.area.paddingxlr) + (icon_size * count) + ((count-1) * systray.area.paddingx);

		systray.area.posy = panel->area.pix.border.width + panel->area.paddingxlr;
		if (panel->clock.area.on_screen)
			systray.area.posy += (panel->clock.area.height + panel->area.paddingx);
#ifdef ENABLE_BATTERY
		if (panel->battery.area.on_screen)
			systray.area.posy += (panel->battery.area.height + panel->area.paddingx);
#endif
	}

	if (panel_horizontal) {
		posy = panel->area.pix.border.width + panel->area.paddingy + systray.area.pix.border.width + systray.area.paddingy;
		posx = systray.area.posx + systray.area.pix.border.width + systray.area.paddingxlr;
	}
	else {
		posx = panel->area.pix.border.width + panel->area.paddingy + systray.area.pix.border.width + systray.area.paddingy;
		posy = systray.area.posy + systray.area.pix.border.width + systray.area.paddingxlr;
	}
	for (l = systray.list_icons; l ; l = l->next) {
		traywin = (TrayWindow*)l->data;

		traywin->y = posy;
		traywin->x = posx;
		traywin->width = icon_size;
		traywin->height = icon_size;
		if (panel_horizontal)
			posx += (icon_size + systray.area.paddingx);
		else
			posy += (icon_size + systray.area.paddingx);

		// position and size the icon window
		XMoveResizeWindow(server.dsp, traywin->id, traywin->x, traywin->y, icon_size, icon_size);
	}
}


// ***********************************************
// systray protocol

int init_net()
{
	Window win = XGetSelectionOwner(server.dsp, server.atom._NET_SYSTEM_TRAY_SCREEN);

	// freedesktop systray specification
	if (win != None) {
		// search pid
		Atom _NET_WM_PID, actual_type;
		int actual_format;
		unsigned long nitems;
		unsigned long bytes_after;
		unsigned char *prop = 0;
		int pid;

		_NET_WM_PID = XInternAtom(server.dsp, "_NET_WM_PID", True);
		//atom_name = XGetAtomName (dpy,atom);

		int ret = XGetWindowProperty(server.dsp, win, _NET_WM_PID, 0, 1024, False, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after, &prop);

		fprintf(stderr, "tint2 : another systray is running");
		if (ret == Success && prop) {
			pid = prop[1] * 256;
			pid += prop[0];
			fprintf(stderr, " pid=%d", pid);
		}
		fprintf(stderr, "\n");
		return 0;
	}

	// init systray protocol
	net_sel_win = XCreateSimpleWindow(server.dsp, server.root_win, -1, -1, 1, 1, 0, 0, 0);

	// v0.2 trayer specification. tint2 always orizontal.
	// TODO : vertical panel ??
	int orient = 0;
	XChangeProperty(server.dsp, net_sel_win, server.atom._NET_SYSTEM_TRAY_ORIENTATION, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &orient, 1);

	XSetSelectionOwner(server.dsp, server.atom._NET_SYSTEM_TRAY_SCREEN, net_sel_win, CurrentTime);
	if (XGetSelectionOwner(server.dsp, server.atom._NET_SYSTEM_TRAY_SCREEN) != net_sel_win) {
		fprintf(stderr, "tint2 : can't get systray manager\n");
		return 0;
	}

	XClientMessageEvent ev;
	ev.type = ClientMessage;
	ev.window = server.root_win;
	ev.message_type = server.atom.MANAGER;
	ev.format = 32;
	ev.data.l[0] = CurrentTime;
	ev.data.l[1] = server.atom._NET_SYSTEM_TRAY_SCREEN;
	ev.data.l[2] = net_sel_win;
	ev.data.l[3] = 0;
	ev.data.l[4] = 0;
	XSendEvent(server.dsp, server.root_win, False, StructureNotifyMask, (XEvent*)&ev);
	return 1;
}


void cleanup_net()
{
	if (net_sel_win != None) {
		XDestroyWindow(server.dsp, net_sel_win);
		net_sel_win = None;
	}
}


gboolean error;
int window_error_handler(Display *d, XErrorEvent *e)
{
	d=d;e=e;
	error = TRUE;
	if (e->error_code != BadWindow) {
		printf("error_handler %d\n", e->error_code);
	}
	return 0;
}


static gint compare_traywindows(gconstpointer a, gconstpointer b)
{
	const TrayWindow * traywin_a = (TrayWindow*)a;
	const TrayWindow * traywin_b = (TrayWindow*)b;
	XTextProperty name_a, name_b;

	if(XGetWMName(server.dsp, traywin_a->id, &name_a) == 0) {
		return -1;
	}
	else if(XGetWMName(server.dsp, traywin_b->id, &name_b) == 0) {
		XFree(name_a.value);
		return 1;
	}
	else {
		gint retval = g_ascii_strncasecmp((char*)name_a.value, (char*)name_b.value, -1) * systray.sort;
		XFree(name_a.value);
		XFree(name_b.value);
		return retval;
	}
}


gboolean add_icon(Window id)
{
	TrayWindow *traywin;
	XErrorHandler old;
	Panel *panel = systray.area.panel;

	error = FALSE;
	old = XSetErrorHandler(window_error_handler);
	XReparentWindow(server.dsp, id, panel->main_win, 0, 0);
	XSync(server.dsp, False);
	XSetErrorHandler(old);
	if (error != FALSE) {
		fprintf(stderr, "tint2 : not icon_swallow\n");
		return FALSE;
	}

	{
		Atom acttype;
		int actfmt;
		unsigned long nbitem, bytes;
		unsigned char *data = 0;
		int ret;

		ret = XGetWindowProperty(server.dsp, id, server.atom._XEMBED_INFO, 0, 2, False, server.atom._XEMBED_INFO, &acttype, &actfmt, &nbitem, &bytes, &data);
		if (data) XFree(data);
		if (ret != Success) {
			fprintf(stderr, "tint2 : xembed error\n");
			return FALSE;
		}
	}
	{
		XEvent e;
		e.xclient.type = ClientMessage;
		e.xclient.serial = 0;
		e.xclient.send_event = True;
		e.xclient.message_type = server.atom._XEMBED;
		e.xclient.window = id;
		e.xclient.format = 32;
		e.xclient.data.l[0] = CurrentTime;
		e.xclient.data.l[1] = XEMBED_EMBEDDED_NOTIFY;
		e.xclient.data.l[2] = 0;
		e.xclient.data.l[3] = panel->main_win;
		e.xclient.data.l[4] = 0;
		XSendEvent(server.dsp, id, False, 0xFFFFFF, &e);
	}

	traywin = g_new0(TrayWindow, 1);
	traywin->id = id;

	if (systray.sort == 3)
		systray.list_icons = g_slist_prepend(systray.list_icons, traywin);
	else if (systray.sort == 2)
		systray.list_icons = g_slist_append(systray.list_icons, traywin);
	else
		systray.list_icons = g_slist_insert_sorted(systray.list_icons, traywin, compare_traywindows);
	systray.area.resize = 1;
	systray.area.redraw = 1;
	//printf("add_icon id %lx, %d\n", id, g_slist_length(systray.list_icons));

	// watch for the icon trying to resize itself!
	XSelectInput(server.dsp, traywin->id, StructureNotifyMask);

	// show the window
	XMapRaised(server.dsp, traywin->id);

	// changed in systray force resize on panel
	panel->area.resize = 1;
	panel_refresh = 1;
	return TRUE;
}


void remove_icon(TrayWindow *traywin)
{
	XErrorHandler old;
	Window id = traywin->id;

	// remove from our list
	systray.list_icons = g_slist_remove(systray.list_icons, traywin);
	g_free(traywin);
	systray.area.resize = 1;
	systray.area.redraw = 1;
	//printf("remove_icon id %lx, %d\n", traywin->id);

	XSelectInput(server.dsp, id, NoEventMask);

	// reparent to root
	error = FALSE;
	old = XSetErrorHandler(window_error_handler);
	XUnmapWindow(server.dsp, id);
	XReparentWindow(server.dsp, id, server.root_win, 0, 0);
	XSync(server.dsp, False);
	XSetErrorHandler(old);

	// changed in systray force resize on panel
	Panel *panel = systray.area.panel;
	panel->area.resize = 1;
	panel_refresh = 1;
}


void net_message(XClientMessageEvent *e)
{
	unsigned long opcode;
	Window id;

	opcode = e->data.l[1];
	switch (opcode) {
		case SYSTEM_TRAY_REQUEST_DOCK:
			id = e->data.l[2];
			if (id) add_icon(id);
			break;

		case SYSTEM_TRAY_BEGIN_MESSAGE:
		case SYSTEM_TRAY_CANCEL_MESSAGE:
			// we don't show baloons messages.
			break;

		default:
			if (opcode == server.atom._NET_SYSTEM_TRAY_MESSAGE_DATA)
				printf("message from dockapp: %s\n", e->data.b);
			else
				fprintf(stderr, "SYSTEM_TRAY : unknown message type\n");
			break;
	}
}


void refresh_systray_icon()
{
	TrayWindow *traywin;
	GSList *l;
	for (l = systray.list_icons; l ; l = l->next) {
		traywin = (TrayWindow*)l->data;
		XClearArea(server.dsp, traywin->id, 0, 0, traywin->width, traywin->height, True);
	}
}


