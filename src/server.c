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

#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xrandr.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server.h"
#include "config.h"
#include "window.h"

Server server;

gboolean primary_monitor_first = FALSE;

void server_catch_error(Display *d, XErrorEvent *ev)
{
}

void server_init_atoms()
{
	server.atom._XROOTPMAP_ID = XInternAtom(server.display, "_XROOTPMAP_ID", False);
	server.atom._XROOTMAP_ID = XInternAtom(server.display, "_XROOTMAP_ID", False);
	server.atom._NET_CURRENT_DESKTOP = XInternAtom(server.display, "_NET_CURRENT_DESKTOP", False);
	server.atom._NET_NUMBER_OF_DESKTOPS = XInternAtom(server.display, "_NET_NUMBER_OF_DESKTOPS", False);
	server.atom._NET_DESKTOP_NAMES = XInternAtom(server.display, "_NET_DESKTOP_NAMES", False);
	server.atom._NET_DESKTOP_GEOMETRY = XInternAtom(server.display, "_NET_DESKTOP_GEOMETRY", False);
	server.atom._NET_DESKTOP_VIEWPORT = XInternAtom(server.display, "_NET_DESKTOP_VIEWPORT", False);
	server.atom._NET_WORKAREA = XInternAtom(server.display, "_NET_WORKAREA", False);
	server.atom._NET_ACTIVE_WINDOW = XInternAtom(server.display, "_NET_ACTIVE_WINDOW", False);
	server.atom._NET_WM_WINDOW_TYPE = XInternAtom(server.display, "_NET_WM_WINDOW_TYPE", False);
	server.atom._NET_WM_STATE_SKIP_PAGER = XInternAtom(server.display, "_NET_WM_STATE_SKIP_PAGER", False);
	server.atom._NET_WM_STATE_SKIP_TASKBAR = XInternAtom(server.display, "_NET_WM_STATE_SKIP_TASKBAR", False);
	server.atom._NET_WM_STATE_STICKY = XInternAtom(server.display, "_NET_WM_STATE_STICKY", False);
	server.atom._NET_WM_STATE_DEMANDS_ATTENTION = XInternAtom(server.display, "_NET_WM_STATE_DEMANDS_ATTENTION", False);
	server.atom._NET_WM_WINDOW_TYPE_DOCK = XInternAtom(server.display, "_NET_WM_WINDOW_TYPE_DOCK", False);
	server.atom._NET_WM_WINDOW_TYPE_DESKTOP = XInternAtom(server.display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
	server.atom._NET_WM_WINDOW_TYPE_TOOLBAR = XInternAtom(server.display, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
	server.atom._NET_WM_WINDOW_TYPE_MENU = XInternAtom(server.display, "_NET_WM_WINDOW_TYPE_MENU", False);
	server.atom._NET_WM_WINDOW_TYPE_SPLASH = XInternAtom(server.display, "_NET_WM_WINDOW_TYPE_SPLASH", False);
	server.atom._NET_WM_WINDOW_TYPE_DIALOG = XInternAtom(server.display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	server.atom._NET_WM_WINDOW_TYPE_NORMAL = XInternAtom(server.display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
	server.atom._NET_WM_DESKTOP = XInternAtom(server.display, "_NET_WM_DESKTOP", False);
	server.atom.WM_STATE = XInternAtom(server.display, "WM_STATE", False);
	server.atom._NET_WM_STATE = XInternAtom(server.display, "_NET_WM_STATE", False);
	server.atom._NET_WM_STATE_MAXIMIZED_VERT = XInternAtom(server.display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
	server.atom._NET_WM_STATE_MAXIMIZED_HORZ = XInternAtom(server.display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
	server.atom._NET_WM_STATE_SHADED = XInternAtom(server.display, "_NET_WM_STATE_SHADED", False);
	server.atom._NET_WM_STATE_HIDDEN = XInternAtom(server.display, "_NET_WM_STATE_HIDDEN", False);
	server.atom._NET_WM_STATE_BELOW = XInternAtom(server.display, "_NET_WM_STATE_BELOW", False);
	server.atom._NET_WM_STATE_ABOVE = XInternAtom(server.display, "_NET_WM_STATE_ABOVE", False);
	server.atom._NET_WM_STATE_MODAL = XInternAtom(server.display, "_NET_WM_STATE_MODAL", False);
	server.atom._NET_CLIENT_LIST = XInternAtom(server.display, "_NET_CLIENT_LIST", False);
	server.atom._NET_WM_VISIBLE_NAME = XInternAtom(server.display, "_NET_WM_VISIBLE_NAME", False);
	server.atom._NET_WM_NAME = XInternAtom(server.display, "_NET_WM_NAME", False);
	server.atom._NET_WM_STRUT = XInternAtom(server.display, "_NET_WM_STRUT", False);
	server.atom._NET_WM_ICON = XInternAtom(server.display, "_NET_WM_ICON", False);
	server.atom._NET_WM_ICON_GEOMETRY = XInternAtom(server.display, "_NET_WM_ICON_GEOMETRY", False);
	server.atom._NET_WM_ICON_NAME = XInternAtom(server.display, "_NET_WM_ICON_NAME", False);
	server.atom._NET_CLOSE_WINDOW = XInternAtom(server.display, "_NET_CLOSE_WINDOW", False);
	server.atom.UTF8_STRING = XInternAtom(server.display, "UTF8_STRING", False);
	server.atom._NET_SUPPORTING_WM_CHECK = XInternAtom(server.display, "_NET_SUPPORTING_WM_CHECK", False);
	server.atom._NET_WM_CM_S0 = XInternAtom(server.display, "_NET_WM_CM_S0", False);
	server.atom._NET_SUPPORTING_WM_CHECK = XInternAtom(server.display, "_NET_WM_NAME", False);
	server.atom._NET_WM_STRUT_PARTIAL = XInternAtom(server.display, "_NET_WM_STRUT_PARTIAL", False);
	server.atom.WM_NAME = XInternAtom(server.display, "WM_NAME", False);
	server.atom.__SWM_VROOT = XInternAtom(server.display, "__SWM_VROOT", False);
	server.atom._MOTIF_WM_HINTS = XInternAtom(server.display, "_MOTIF_WM_HINTS", False);
	server.atom.WM_HINTS = XInternAtom(server.display, "WM_HINTS", False);
	gchar *name = g_strdup_printf("_XSETTINGS_S%d", DefaultScreen(server.display));
	server.atom._XSETTINGS_SCREEN = XInternAtom(server.display, name, False);
	g_free(name);
	server.atom._XSETTINGS_SETTINGS = XInternAtom(server.display, "_XSETTINGS_SETTINGS", False);

	// systray protocol
	name = g_strdup_printf("_NET_SYSTEM_TRAY_S%d", DefaultScreen(server.display));
	server.atom._NET_SYSTEM_TRAY_SCREEN = XInternAtom(server.display, name, False);
	g_free(name);
	server.atom._NET_SYSTEM_TRAY_OPCODE = XInternAtom(server.display, "_NET_SYSTEM_TRAY_OPCODE", False);
	server.atom.MANAGER = XInternAtom(server.display, "MANAGER", False);
	server.atom._NET_SYSTEM_TRAY_MESSAGE_DATA = XInternAtom(server.display, "_NET_SYSTEM_TRAY_MESSAGE_DATA", False);
	server.atom._NET_SYSTEM_TRAY_ORIENTATION = XInternAtom(server.display, "_NET_SYSTEM_TRAY_ORIENTATION", False);
	server.atom._NET_SYSTEM_TRAY_ICON_SIZE = XInternAtom(server.display, "_NET_SYSTEM_TRAY_ICON_SIZE", False);
	server.atom._NET_SYSTEM_TRAY_PADDING = XInternAtom(server.display, "_NET_SYSTEM_TRAY_PADDING", False);
	server.atom._XEMBED = XInternAtom(server.display, "_XEMBED", False);
	server.atom._XEMBED_INFO = XInternAtom(server.display, "_XEMBED_INFO", False);
	server.atom._NET_WM_PID = XInternAtom(server.display, "_NET_WM_PID", True);

	// drag 'n' drop
	server.atom.XdndAware = XInternAtom(server.display, "XdndAware", False);
	server.atom.XdndEnter = XInternAtom(server.display, "XdndEnter", False);
	server.atom.XdndPosition = XInternAtom(server.display, "XdndPosition", False);
	server.atom.XdndStatus = XInternAtom(server.display, "XdndStatus", False);
	server.atom.XdndDrop = XInternAtom(server.display, "XdndDrop", False);
	server.atom.XdndLeave = XInternAtom(server.display, "XdndLeave", False);
	server.atom.XdndSelection = XInternAtom(server.display, "XdndSelection", False);
	server.atom.XdndTypeList = XInternAtom(server.display, "XdndTypeList", False);
	server.atom.XdndActionCopy = XInternAtom(server.display, "XdndActionCopy", False);
	server.atom.XdndFinished = XInternAtom(server.display, "XdndFinished", False);
	server.atom.TARGETS = XInternAtom(server.display, "TARGETS", False);
}

void cleanup_server()
{
	if (server.colormap)
		XFreeColormap(server.display, server.colormap);
	server.colormap = 0;
	if (server.colormap32)
		XFreeColormap(server.display, server.colormap32);
	server.colormap32 = 0;
	if (server.monitors) {
		for (int i = 0; i < server.num_monitors; ++i) {
			g_strfreev(server.monitors[i].names);
			server.monitors[i].names = NULL;
		}
		free(server.monitors);
		server.monitors = NULL;
	}
	if (server.gc)
		XFreeGC(server.display, server.gc);
	server.gc = NULL;
	server.disable_transparency = FALSE;
#ifdef HAVE_SN
	if (server.pids)
		g_tree_destroy(server.pids);
	server.pids = NULL;
#endif
}

void send_event32(Window win, Atom at, long data1, long data2, long data3)
{
	XEvent event;

	event.xclient.type = ClientMessage;
	event.xclient.serial = 0;
	event.xclient.send_event = True;
	event.xclient.display = server.display;
	event.xclient.window = win;
	event.xclient.message_type = at;

	event.xclient.format = 32;
	event.xclient.data.l[0] = data1;
	event.xclient.data.l[1] = data2;
	event.xclient.data.l[2] = data3;
	event.xclient.data.l[3] = 0;
	event.xclient.data.l[4] = 0;

	XSendEvent(server.display, server.root_win, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

int get_property32(Window win, Atom at, Atom type)
{
	Atom type_ret;
	int format_ret = 0, data = 0;
	unsigned long nitems_ret = 0;
	unsigned long bafter_ret = 0;
	unsigned char *prop_value = 0;
	int result;

	if (!win)
		return 0;

	result = XGetWindowProperty(server.display,
								win,
								at,
								0,
								0x7fffffff,
								False,
								type,
								&type_ret,
								&format_ret,
								&nitems_ret,
								&bafter_ret,
								&prop_value);

	if (result == Success && prop_value) {
		data = ((gulong *)prop_value)[0];
		XFree(prop_value);
	}
	return data;
}

void *server_get_property(Window win, Atom at, Atom type, int *num_results)
{
	Atom type_ret;
	int format_ret = 0;
	unsigned long nitems_ret = 0;
	unsigned long bafter_ret = 0;
	unsigned char *prop_value;

	if (!win)
		return NULL;

	int result = XGetWindowProperty(server.display,
									win,
									at,
									0,
									0x7fffffff,
									False,
									type,
									&type_ret,
									&format_ret,
									&nitems_ret,
									&bafter_ret,
									&prop_value);

	// Send fill_color resultcount
	if (num_results)
		*num_results = (int)nitems_ret;

	if (result == Success && prop_value)
		return prop_value;
	else
		return NULL;
}

void get_root_pixmap()
{
	Pixmap ret = None;

	Atom pixmap_atoms[] = {server.atom._XROOTPMAP_ID, server.atom._XROOTMAP_ID};
	for (int i = 0; i < sizeof(pixmap_atoms) / sizeof(Atom); ++i) {
		unsigned long *res = (unsigned long *)server_get_property(server.root_win, pixmap_atoms[i], XA_PIXMAP, NULL);
		if (res) {
			ret = *((Pixmap *)res);
			XFree(res);
			break;
		}
	}
	server.root_pmap = ret;

	if (server.root_pmap == None) {
		fprintf(stderr, "tint2 : pixmap background detection failed\n");
	} else {
		XGCValues gcv;
		gcv.ts_x_origin = 0;
		gcv.ts_y_origin = 0;
		gcv.fill_style = FillTiled;
		uint mask = GCTileStipXOrigin | GCTileStipYOrigin | GCFillStyle | GCTile;

		gcv.tile = server.root_pmap;
		XChangeGC(server.display, server.gc, mask, &gcv);
	}
}

int compare_monitor_pos(const void *monitor1, const void *monitor2)
{
	const Monitor *m1 = (const Monitor *)monitor1;
	const Monitor *m2 = (const Monitor *)monitor2;

	if (primary_monitor_first) {
		if (m1->primary && !m2->primary)
			return -1;
		if (!m1->primary && m2->primary)
			return 1;
	}

	if (m1->x < m2->x) {
		return -1;
	} else if (m1->x > m2->x) {
		return 1;
	} else if (m1->y < m2->y) {
		return -1;
	} else if (m1->y > m2->y) {
		return 1;
	} else {
		return 0;
	}
}

int monitor_includes_monitor(const void *monitor1, const void *monitor2)
{
	const Monitor *m1 = (const Monitor *)monitor1;
	const Monitor *m2 = (const Monitor *)monitor2;

	if (m1->x >= m2->x && m1->y >= m2->y && (m1->x + m1->width) <= (m2->x + m2->width) &&
		(m1->y + m1->height) <= (m2->y + m2->height)) {
		// m1 included inside m2
		return 1;
	} else {
		return -1;
	}
}

void sort_monitors()
{
	qsort(server.monitors, server.num_monitors, sizeof(Monitor), compare_monitor_pos);
}

void get_monitors()
{
	if (XineramaIsActive(server.display)) {
		int num_monitors;
		XineramaScreenInfo *info = XineramaQueryScreens(server.display, &num_monitors);
		XRRScreenResources *res = XRRGetScreenResourcesCurrent(server.display, server.root_win);
		RROutput primary_output = XRRGetOutputPrimary(server.display, server.root_win);

		if (res && res->ncrtc >= num_monitors) {
			// use xrandr to identify monitors (does not work with proprietery nvidia drivers)
			printf("xRandr: Found crtc's: %d\n", res->ncrtc);
			server.monitors = calloc(res->ncrtc, sizeof(Monitor));
			num_monitors = 0;
			for (int i = 0; i < res->ncrtc; ++i) {
				XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(server.display, res, res->crtcs[i]);
				// Ignore empty crtc
				if (!crtc_info->width || !crtc_info->height) {
					printf("xRandr: crtc %d seems disabled\n", i);
					XRRFreeCrtcInfo(crtc_info);
					continue;
				}
				int i_monitor = num_monitors;
				num_monitors++;
				server.monitors[i_monitor].x = crtc_info->x;
				server.monitors[i_monitor].y = crtc_info->y;
				server.monitors[i_monitor].width = crtc_info->width;
				server.monitors[i_monitor].height = crtc_info->height;
				server.monitors[i_monitor].names = calloc((crtc_info->noutput + 1), sizeof(gchar *));
				for (int j = 0; j < crtc_info->noutput; ++j) {
					XRROutputInfo *output_info = XRRGetOutputInfo(server.display, res, crtc_info->outputs[j]);
					printf("xRandr: Linking output %s with crtc %d\n", output_info->name, i);
					server.monitors[i_monitor].names[j] = g_strdup(output_info->name);
					XRRFreeOutputInfo(output_info);
					server.monitors[i_monitor].primary = crtc_info->outputs[j] == primary_output;
				}
				server.monitors[i_monitor].names[crtc_info->noutput] = NULL;
				XRRFreeCrtcInfo(crtc_info);
			}
		} else if (info && num_monitors > 0) {
			server.monitors = calloc(num_monitors, sizeof(Monitor));
			for (int i = 0; i < num_monitors; i++) {
				server.monitors[i].x = info[i].x_org;
				server.monitors[i].y = info[i].y_org;
				server.monitors[i].width = info[i].width;
				server.monitors[i].height = info[i].height;
				server.monitors[i].names = NULL;
			}
		}

		// Sort monitors by inclusion
		qsort(server.monitors, num_monitors, sizeof(Monitor), monitor_includes_monitor);

		// Remove monitors included in other ones
		int i = 0;
		while (i < num_monitors) {
			for (int j = 0; j < i; j++) {
				if (monitor_includes_monitor(&server.monitors[i], &server.monitors[j]) > 0) {
					goto next;
				}
			}
			i++;
		}
	next:
		for (int j = i; j < num_monitors; ++j)
			if (server.monitors[j].names)
				g_strfreev(server.monitors[j].names);
		server.num_monitors = i;
		server.monitors = realloc(server.monitors, server.num_monitors * sizeof(Monitor));
		qsort(server.monitors, server.num_monitors, sizeof(Monitor), compare_monitor_pos);

		if (res)
			XRRFreeScreenResources(res);
		XFree(info);
	}

	if (!server.num_monitors) {
		server.num_monitors = 1;
		server.monitors = calloc(1, sizeof(Monitor));
		server.monitors[0].x = server.monitors[0].y = 0;
		server.monitors[0].width = DisplayWidth(server.display, server.screen);
		server.monitors[0].height = DisplayHeight(server.display, server.screen);
		server.monitors[0].names = 0;
	}
}

void print_monitors()
{
	fprintf(stderr, "Number of monitors: %d\n", server.num_monitors);
	for (int i = 0; i < server.num_monitors; i++) {
		fprintf(stderr,
				"Monitor %d: x = %d, y = %d, w = %d, h = %d\n",
				i + 1,
				server.monitors[i].x,
				server.monitors[i].y,
				server.monitors[i].width,
				server.monitors[i].height);
	}
}

void server_get_number_of_desktops()
{
	if (server.viewports) {
		free(server.viewports);
		server.viewports = NULL;
	}

	server.num_desktops = get_property32(server.root_win, server.atom._NET_NUMBER_OF_DESKTOPS, XA_CARDINAL);
	if (server.num_desktops > 1)
		return;

	int num_results;
	long *work_area_size = server_get_property(server.root_win, server.atom._NET_WORKAREA, XA_CARDINAL, &num_results);
	if (!work_area_size)
		return;
	int work_area_width = work_area_size[0] + work_area_size[2];
	int work_area_height = work_area_size[1] + work_area_size[3];
	XFree(work_area_size);

	long *x_screen_size =
		server_get_property(server.root_win, server.atom._NET_DESKTOP_GEOMETRY, XA_CARDINAL, &num_results);
	if (!x_screen_size)
		return;
	int x_screen_width = x_screen_size[0];
	int x_screen_height = x_screen_size[1];
	XFree(x_screen_size);

	int num_viewports = MAX(x_screen_width / work_area_width, 1) * MAX(x_screen_height / work_area_height, 1);
	if (num_viewports <= 1) {
		server.num_desktops = 1;
		return;
	}

	server.viewports = calloc(num_viewports, sizeof(Viewport));
	int k = 0;
	for (int i = 0; i < MAX(x_screen_height / work_area_height, 1); i++) {
		for (int j = 0; j < MAX(x_screen_width / work_area_width, 1); j++) {
			server.viewports[k].x = j * work_area_width;
			server.viewports[k].y = i * work_area_height;
			server.viewports[k].width = work_area_width;
			server.viewports[k].height = work_area_height;
			k++;
		}
	}

	server.num_desktops = num_viewports;
}

GSList *get_desktop_names()
{
	if (server.viewports) {
		GSList *list = NULL;
		for (int j = 0; j < server.num_desktops; j++) {
			list = g_slist_append(list, g_strdup_printf("%d", j + 1));
		}
		return list;
	}

	int count;
	GSList *list = NULL;
	gchar *data_ptr =
		server_get_property(server.root_win, server.atom._NET_DESKTOP_NAMES, server.atom.UTF8_STRING, &count);
	if (data_ptr) {
		list = g_slist_append(list, g_strdup(data_ptr));
		for (int j = 0; j < count - 1; j++) {
			if (*(data_ptr + j) == '\0') {
				gchar *ptr = (gchar *)data_ptr + j + 1;
				list = g_slist_append(list, g_strdup(ptr));
			}
		}
		XFree(data_ptr);
	}
	return list;
}

int get_current_desktop()
{
	if (!server.viewports) {
		return MAX(0,
				   MIN(server.num_desktops - 1,
					   get_property32(server.root_win, server.atom._NET_CURRENT_DESKTOP, XA_CARDINAL)));
	}

	int num_results;
	long *work_area_size = server_get_property(server.root_win, server.atom._NET_WORKAREA, XA_CARDINAL, &num_results);
	if (!work_area_size)
		return 0;
	int work_area_width = work_area_size[0] + work_area_size[2];
	int work_area_height = work_area_size[1] + work_area_size[3];
	XFree(work_area_size);

	if (work_area_width <= 0 || work_area_height <= 0)
		return 0;

	long *viewport = server_get_property(server.root_win, server.atom._NET_DESKTOP_VIEWPORT, XA_CARDINAL, &num_results);
	if (!viewport)
		return 0;
	int viewport_x = viewport[0];
	int viewport_y = viewport[1];
	XFree(viewport);

	long *x_screen_size =
		server_get_property(server.root_win, server.atom._NET_DESKTOP_GEOMETRY, XA_CARDINAL, &num_results);
	if (!x_screen_size)
		return 0;
	int x_screen_width = x_screen_size[0];
	XFree(x_screen_size);

	int ncols = x_screen_width / work_area_width;

	//	fprintf(stderr, "\n");
	//	fprintf(stderr, "Work area size: %d x %d\n", work_area_width, work_area_height);
	//	fprintf(stderr, "Viewport pos: %d x %d\n", viewport_x, viewport_y);
	//	fprintf(stderr, "Viewport i: %d\n", (viewport_y / work_area_height) * ncols + viewport_x / work_area_width);

	int result = (viewport_y / work_area_height) * ncols + viewport_x / work_area_width;
	return MAX(0, MIN(server.num_desktops - 1, result));
}

void change_desktop(int desktop)
{
	if (!server.viewports) {
		send_event32(server.root_win, server.atom._NET_CURRENT_DESKTOP, desktop, 0, 0);
	} else {
		send_event32(server.root_win,
					 server.atom._NET_DESKTOP_VIEWPORT,
					 server.viewports[desktop].x,
					 server.viewports[desktop].y,
					 0);
	}
}

void get_desktops()
{
	// detect number of desktops
	// wait 15s to leave some time for window manager startup
	for (int i = 0; i < 15; i++) {
		server_get_number_of_desktops();
		if (server.num_desktops > 0)
			break;
		sleep(1);
	}
	if (server.num_desktops == 0) {
		server.num_desktops = 1;
		fprintf(stderr, "warning : WM doesn't respect NETWM specs. tint2 default to 1 desktop.\n");
	}
}

void server_init_visual()
{
	// inspired by freedesktops fdclock ;)
	XVisualInfo templ = {.screen = server.screen, .depth = 32, .class = TrueColor};
	int nvi;
	XVisualInfo *xvi =
		XGetVisualInfo(server.display, VisualScreenMask | VisualDepthMask | VisualClassMask, &templ, &nvi);

	Visual *visual = NULL;
	if (xvi) {
		XRenderPictFormat *format;
		for (int i = 0; i < nvi; i++) {
			format = XRenderFindVisualFormat(server.display, xvi[i].visual);
			if (format->type == PictTypeDirect && format->direct.alphaMask) {
				visual = xvi[i].visual;
				break;
			}
		}
	}
	XFree(xvi);

	// check composite manager
	server.composite_manager = XGetSelectionOwner(server.display, server.atom._NET_WM_CM_S0);
	if (server.colormap)
		XFreeColormap(server.display, server.colormap);
	if (server.colormap32)
		XFreeColormap(server.display, server.colormap32);

	if (visual) {
		server.visual32 = visual;
		server.colormap32 = XCreateColormap(server.display, server.root_win, visual, AllocNone);
	}

	if (!server.disable_transparency && visual && server.composite_manager != None && !snapshot_path) {
		XSetWindowAttributes attrs;
		attrs.event_mask = StructureNotifyMask;
		XChangeWindowAttributes(server.display, server.composite_manager, CWEventMask, &attrs);

		server.real_transparency = TRUE;
		server.depth = 32;
		printf("real transparency on... depth: %d\n", server.depth);
		server.colormap = XCreateColormap(server.display, server.root_win, visual, AllocNone);
		server.visual = visual;
	} else {
		// no composite manager or snapshot mode => fake transparency
		server.real_transparency = FALSE;
		server.depth = DefaultDepth(server.display, server.screen);
		printf("real transparency off.... depth: %d\n", server.depth);
		server.colormap = DefaultColormap(server.display, server.screen);
		server.visual = DefaultVisual(server.display, server.screen);
	}
}
