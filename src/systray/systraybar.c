/**************************************************************************
* Tint2 : systraybar
*
* Copyright (C) 2009 thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
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
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>

#include "systraybar.h"
#include "server.h"
#include "panel.h"

GSList *icons;

/* defined in the systray spec */
#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

// selection window
Window net_sel_win = None;

// freedesktop specification doesn't allow multi systray
Systraybar systray;
int refresh_systray;
int systray_enabled;
int systray_max_icon_size;
int systray_monitor;
int chrono;
int systray_composited;
int systray_profile;
// background pixmap if we render ourselves the icons
static Pixmap render_background;

const int min_refresh_period = 50;
const int max_fast_refreshes = 5;
const int resize_period_threshold = 1000;
const int fast_resize_period = 50;
const int slow_resize_period = 5000;
const int min_bad_resize_events = 3;
const int max_bad_resize_events = 10;

void default_systray()
{
	systray_enabled = 0;
	memset(&systray, 0, sizeof(Systraybar));
	render_background = 0;
	chrono = 0;
	systray.alpha = 100;
	systray.sort = SYSTRAY_SORT_LEFT2RIGHT;
	systray.area._draw_foreground = draw_systray;
	systray.area._on_change_layout = on_change_systray;
	systray.area.size_mode = LAYOUT_FIXED;
	systray.area._resize = resize_systray;
	systray_profile = getenv("SYSTRAY_PROFILING") != NULL;
}

void cleanup_systray()
{
	stop_net();
	systray_enabled = 0;
	systray_max_icon_size = 0;
	systray_monitor = 0;
	systray.area.on_screen = FALSE;
	free_area(&systray.area);
	if (render_background) {
		XFreePixmap(server.display, render_background);
		render_background = 0;
	}
}

void init_systray()
{
	if (!systray_enabled)
		return;

	systray_composited = !server.disable_transparency && server.visual32 && server.colormap32;
	fprintf(stderr, "Systray composited rendering %s\n", systray_composited ? "on" : "off");

	if (!systray_composited) {
		fprintf(stderr, "systray_asb forced to 100 0 0\n");
		systray.alpha = 100;
		systray.brightness = systray.saturation = 0;
	}
}

void init_systray_panel(void *p)
{
	Panel *panel = (Panel *)p;
	systray.area.parent = panel;
	systray.area.panel = panel;
	if (!systray.area.bg)
		systray.area.bg = &g_array_index(backgrounds, Background, 0);
	show(&systray.area);
	systray.area.resize_needed = 1;
	panel->area.resize_needed = 1;
	schedule_redraw(&systray.area);
	panel_refresh = TRUE;
	refresh_systray = 1;
}

gboolean resize_systray(void *obj)
{
	if (systray_profile)
		fprintf(stderr, "[%f] %s:%d\n", profiling_get_time(), __FUNCTION__, __LINE__);
	Systraybar *sysbar = obj;

	if (panel_horizontal)
		sysbar->icon_size = sysbar->area.height;
	else
		sysbar->icon_size = sysbar->area.width;
	sysbar->icon_size = sysbar->icon_size - (2 * sysbar->area.bg->border.width) - (2 * sysbar->area.paddingy);
	if (systray_max_icon_size > 0 && sysbar->icon_size > systray_max_icon_size)
		sysbar->icon_size = systray_max_icon_size;

	if (systray.icon_size > 0) {
		long icon_size = systray.icon_size;
		XChangeProperty(server.display,
		                net_sel_win,
		                server.atom._NET_SYSTEM_TRAY_ICON_SIZE,
		                XA_CARDINAL,
		                32,
		                PropModeReplace,
		                (unsigned char *)&icon_size,
		                1);
	}

	int count = 0;
	for (GSList *l = systray.list_icons; l; l = l->next) {
		if (((TrayWindow *)l->data)->hide)
			continue;
		count++;
	}
	if (systray_profile)
		fprintf(stderr, BLUE "%s:%d number of icons = %d\n" RESET, __FUNCTION__, __LINE__, count);

	if (panel_horizontal) {
		int height = sysbar->area.height - 2 * sysbar->area.bg->border.width - 2 * sysbar->area.paddingy;
		// here icons_per_column always higher than 0
		sysbar->icons_per_column = (height + sysbar->area.paddingx) / (sysbar->icon_size + sysbar->area.paddingx);
		sysbar->margin =
		    height - (sysbar->icons_per_column - 1) * (sysbar->icon_size + sysbar->area.paddingx) - sysbar->icon_size;
		sysbar->icons_per_row = count / sysbar->icons_per_column + (count % sysbar->icons_per_column != 0);
		systray.area.width = (2 * systray.area.bg->border.width) + (2 * systray.area.paddingxlr) +
		                     (sysbar->icon_size * sysbar->icons_per_row) +
		                     ((sysbar->icons_per_row - 1) * systray.area.paddingx);
	} else {
		int width = sysbar->area.width - 2 * sysbar->area.bg->border.width - 2 * sysbar->area.paddingy;
		// here icons_per_row always higher than 0
		sysbar->icons_per_row = (width + sysbar->area.paddingx) / (sysbar->icon_size + sysbar->area.paddingx);
		sysbar->margin =
		    width - (sysbar->icons_per_row - 1) * (sysbar->icon_size + sysbar->area.paddingx) - sysbar->icon_size;
		sysbar->icons_per_column = count / sysbar->icons_per_row + (count % sysbar->icons_per_row != 0);
		systray.area.height = (2 * systray.area.bg->border.width) + (2 * systray.area.paddingxlr) +
		                      (sysbar->icon_size * sysbar->icons_per_column) +
		                      ((sysbar->icons_per_column - 1) * systray.area.paddingx);
	}

	if (net_sel_win == None) {
		start_net();
	}

	return TRUE;
}

void draw_systray(void *obj, cairo_t *c)
{
	if (systray_profile)
		fprintf(stderr, BLUE "[%f] %s:%d\n" RESET, profiling_get_time(), __FUNCTION__, __LINE__);
	if (systray_composited) {
		if (render_background)
			XFreePixmap(server.display, render_background);
		render_background =
			XCreatePixmap(server.display, server.root_win, systray.area.width, systray.area.height, server.depth);
		XCopyArea(server.display,
		          systray.area.pix,
		          render_background,
		          server.gc,
		          0,
		          0,
		          systray.area.width,
		          systray.area.height,
		          0,
		          0);
	}

	refresh_systray = TRUE;
}

void on_change_systray(void *obj)
{
	if (systray_profile)
		fprintf(stderr, "[%f] %s:%d\n", profiling_get_time(), __FUNCTION__, __LINE__);
	// here, systray.area.posx/posy are defined by rendering engine. so we can calculate position of tray icon.
	Systraybar *sysbar = obj;
	if (sysbar->icons_per_column == 0 || sysbar->icons_per_row == 0)
		return;

	Panel *panel = sysbar->area.panel;
	int posx, posy;
	int start = panel->area.bg->border.width + panel->area.paddingy + systray.area.bg->border.width +
	            systray.area.paddingy + sysbar->margin / 2;
	if (panel_horizontal) {
		posy = start;
		posx = systray.area.posx + systray.area.bg->border.width + systray.area.paddingxlr;
	} else {
		posx = start;
		posy = systray.area.posy + systray.area.bg->border.width + systray.area.paddingxlr;
	}

	TrayWindow *traywin;
	GSList *l;
	int i;
	for (i = 1, l = systray.list_icons; l; i++, l = l->next) {
		traywin = (TrayWindow *)l->data;
		if (traywin->hide)
			continue;

		traywin->y = posy;
		traywin->x = posx;
		if (systray_profile)
			fprintf(stderr,
			        "%s:%d win = %lu (%s), parent = %lu, x = %d, y = %d\n",
			        __FUNCTION__,
			        __LINE__,
			        traywin->win,
			        traywin->name,
			        traywin->parent,
			        posx,
			        posy);
		traywin->width = sysbar->icon_size;
		traywin->height = sysbar->icon_size;
		if (panel_horizontal) {
			if (i % sysbar->icons_per_column) {
				posy += sysbar->icon_size + sysbar->area.paddingx;
			} else {
				posy = start;
				posx += (sysbar->icon_size + systray.area.paddingx);
			}
		} else {
			if (i % sysbar->icons_per_row) {
				posx += sysbar->icon_size + systray.area.paddingx;
			} else {
				posx = start;
				posy += (sysbar->icon_size + systray.area.paddingx);
			}
		}

		// position and size the icon window
		unsigned int border_width;
		int xpos, ypos;
		unsigned int width, height, depth;
		Window root;
		if (!XGetGeometry(server.display, traywin->parent, &root, &xpos, &ypos, &width, &height, &border_width, &depth)) {
			fprintf(stderr, RED "Couldn't get geometry of window!\n" RESET);
		}
		if (width != traywin->width || height != traywin->height || xpos != traywin->x || ypos != traywin->y) {
			if (systray_profile)
				fprintf(stderr,
						"XMoveResizeWindow(server.display, traywin->parent = %ld, traywin->x = %d, traywin->y = %d, "
				        "traywin->width = %d, traywin->height = %d)\n",
				        traywin->parent,
				        traywin->x,
				        traywin->y,
				        traywin->width,
				        traywin->height);
			XMoveResizeWindow(server.display, traywin->parent, traywin->x, traywin->y, traywin->width, traywin->height);
		}
		if (!traywin->reparented)
			reparent_icon(traywin);
	}
	refresh_systray = TRUE;
}

// ***********************************************
// systray protocol

void start_net()
{
	if (systray_profile)
		fprintf(stderr, "[%f] %s:%d\n", profiling_get_time(), __FUNCTION__, __LINE__);
	if (net_sel_win) {
		// protocol already started
		if (!systray_enabled)
			stop_net();
		return;
	} else {
		if (!systray_enabled)
			return;
	}

	Window win = XGetSelectionOwner(server.display, server.atom._NET_SYSTEM_TRAY_SCREEN);

	// freedesktop systray specification
	if (win != None) {
		// search pid
		Atom _NET_WM_PID, actual_type;
		int actual_format;
		unsigned long nitems;
		unsigned long bytes_after;
		unsigned char *prop = 0;
		int pid;

		_NET_WM_PID = XInternAtom(server.display, "_NET_WM_PID", True);
		int ret = XGetWindowProperty(server.display,
		                             win,
		                             _NET_WM_PID,
		                             0,
		                             1024,
		                             False,
		                             AnyPropertyType,
		                             &actual_type,
		                             &actual_format,
		                             &nitems,
		                             &bytes_after,
		                             &prop);

		fprintf(stderr, RED "tint2 : another systray is running" RESET);
		if (ret == Success && prop) {
			pid = prop[1] * 256;
			pid += prop[0];
			fprintf(stderr, " pid=%d", pid);
		}
		fprintf(stderr, "\n" RESET);
		return;
	}

	// init systray protocol
	net_sel_win = XCreateSimpleWindow(server.display, server.root_win, -1, -1, 1, 1, 0, 0, 0);
	fprintf(stderr, "systray window %ld\n", net_sel_win);

	// v0.3 trayer specification. tint2 always horizontal.
	// Vertical panel will draw the systray horizontal.
	long orientation = 0;
	XChangeProperty(server.display,
	                net_sel_win,
	                server.atom._NET_SYSTEM_TRAY_ORIENTATION,
	                XA_CARDINAL,
	                32,
	                PropModeReplace,
	                (unsigned char *)&orientation,
	                1);
	if (systray.icon_size > 0) {
		long icon_size = systray.icon_size;
		XChangeProperty(server.display,
		                net_sel_win,
		                server.atom._NET_SYSTEM_TRAY_ICON_SIZE,
		                XA_CARDINAL,
		                32,
		                PropModeReplace,
		                (unsigned char *)&icon_size,
		                1);
	}
	long padding = 0;
	XChangeProperty(server.display,
	                net_sel_win,
	                server.atom._NET_SYSTEM_TRAY_PADDING,
	                XA_CARDINAL,
	                32,
	                PropModeReplace,
	                (unsigned char *)&padding,
	                1);

	VisualID vid;
	if (systray_composited)
		vid = XVisualIDFromVisual(server.visual32);
	else
		vid = XVisualIDFromVisual(server.visual);
	XChangeProperty(server.display,
	                net_sel_win,
					XInternAtom(server.display, "_NET_SYSTEM_TRAY_VISUAL", False),
	                XA_VISUALID,
	                32,
	                PropModeReplace,
	                (unsigned char *)&vid,
	                1);

	XSetSelectionOwner(server.display, server.atom._NET_SYSTEM_TRAY_SCREEN, net_sel_win, CurrentTime);
	if (XGetSelectionOwner(server.display, server.atom._NET_SYSTEM_TRAY_SCREEN) != net_sel_win) {
		stop_net();
		fprintf(stderr, RED "tint2 : can't get systray manager\n" RESET);
		return;
	}

	fprintf(stderr, GREEN "tint2 : systray started\n" RESET);
	if (systray_profile)
		fprintf(stderr, "[%f] %s:%d\n", profiling_get_time(), __FUNCTION__, __LINE__);
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
	XSendEvent(server.display, server.root_win, False, StructureNotifyMask, (XEvent *)&ev);
}

void net_message(XClientMessageEvent *e)
{
	if (systray_profile)
		fprintf(stderr, "[%f] %s:%d\n", profiling_get_time(), __FUNCTION__, __LINE__);

	Window win;
	unsigned long opcode = e->data.l[1];
	switch (opcode) {
	case SYSTEM_TRAY_REQUEST_DOCK:
		win = e->data.l[2];
		if (win)
			add_icon(win);
		break;

	case SYSTEM_TRAY_BEGIN_MESSAGE:
	case SYSTEM_TRAY_CANCEL_MESSAGE:
		// we don't show baloons messages.
		break;

	default:
		if (opcode == server.atom._NET_SYSTEM_TRAY_MESSAGE_DATA)
			fprintf(stderr, "message from dockapp: %s\n", e->data.b);
		else
			fprintf(stderr, RED "SYSTEM_TRAY : unknown message type\n" RESET);
		break;
	}
}

void stop_net()
{
	if (systray_profile)
		fprintf(stderr, "[%f] %s:%d\n", profiling_get_time(), __FUNCTION__, __LINE__);
	if (systray.list_icons) {
		// remove_icon change systray.list_icons
		while (systray.list_icons)
			remove_icon((TrayWindow *)systray.list_icons->data);

		g_slist_free(systray.list_icons);
		systray.list_icons = NULL;
	}

	if (net_sel_win != None) {
		XDestroyWindow(server.display, net_sel_win);
		net_sel_win = None;
	}
}

gboolean error;
int window_error_handler(Display *d, XErrorEvent *e)
{
	if (systray_profile)
		fprintf(stderr, RED "[%f] %s:%d\n" RESET, profiling_get_time(), __FUNCTION__, __LINE__);
	error = TRUE;
	if (e->error_code != BadWindow) {
		fprintf(stderr, RED "systray: error code %d\n" RESET, e->error_code);
	}
	return 0;
}

static gint compare_traywindows(gconstpointer a, gconstpointer b)
{
	const TrayWindow *traywin_a = (const TrayWindow *)a;
	const TrayWindow *traywin_b = (const TrayWindow *)b;

#if 0
	// This breaks pygtk2 StatusIcon with blinking activated
	if (traywin_a->empty && !traywin_b->empty)
		return 1 * (systray.sort == SYSTRAY_SORT_RIGHT2LEFT ? -1 : 1);
	if (!traywin_a->empty && traywin_b->empty)
		return -1 * (systray.sort == SYSTRAY_SORT_RIGHT2LEFT ? -1 : 1);
#endif

	if (systray.sort == SYSTRAY_SORT_ASCENDING || systray.sort == SYSTRAY_SORT_DESCENDING) {
		return g_ascii_strncasecmp(traywin_a->name, traywin_b->name, -1) *
		       (systray.sort == SYSTRAY_SORT_ASCENDING ? 1 : -1);
	}

	if (systray.sort == SYSTRAY_SORT_LEFT2RIGHT || systray.sort == SYSTRAY_SORT_RIGHT2LEFT) {
		return (traywin_a->chrono - traywin_b->chrono) * (systray.sort == SYSTRAY_SORT_LEFT2RIGHT ? 1 : -1);
	}

	return 0;
}

gboolean add_icon(Window win)
{
	XTextProperty xname;
	char *name;
	if (XGetWMName(server.display, win, &xname)) {
		name = strdup((char *)xname.value);
		XFree(xname.value);
	} else {
		name = strdup("");
	}

	if (systray_profile)
		fprintf(stderr, "[%f] %s:%d win = %lu (%s)\n", profiling_get_time(), __FUNCTION__, __LINE__, win, name);
	Panel *panel = systray.area.panel;
	int hide = 0;

	// Get the process ID of the application that created the window
	int pid = 0;
	{
		Atom actual_type;
		int actual_format;
		unsigned long nitems;
		unsigned long bytes_after;
		unsigned char *prop = 0;
		int ret = XGetWindowProperty(server.display,
		                             win,
		                             server.atom._NET_WM_PID,
		                             0,
		                             1024,
		                             False,
		                             AnyPropertyType,
		                             &actual_type,
		                             &actual_format,
		                             &nitems,
		                             &bytes_after,
		                             &prop);
		if (ret == Success && prop) {
			pid = prop[1] * 256;
			pid += prop[0];
		}
	}

	// Check if the application leaves behind empty icons
	int num_empty_same_pid = 0;
	for (GSList *l = systray.list_icons; l; l = l->next) {
		TrayWindow *other = (TrayWindow *)l->data;
		if (other->win == win) {
			free(name);
			return FALSE;
		}
		if (!systray_composited) {
			// Empty icon detection: we compare the contents of the icon with the contents of the panel pixmap.
			// If any pixel is different, the icon is not empty.
			imlib_context_set_visual(server.visual);
			imlib_context_set_colormap(server.colormap);
			imlib_context_set_drawable(other->win);
			Imlib_Image image = imlib_create_image_from_drawable(0, 0, 0, other->width, other->height, 1);
			if (image) {
				imlib_context_set_drawable(panel->temp_pmap);
				Imlib_Image bg =
				    imlib_create_image_from_drawable(0, other->x, other->y, other->width, other->height, 1);
				imlib_context_set_image(bg);
				DATA32 *data_bg = imlib_image_get_data_for_reading_only();
				imlib_context_set_image(image);
				imlib_image_set_has_alpha(other->depth > 24);
				DATA32 *data = imlib_image_get_data_for_reading_only();
				int x, y;
				gboolean empty = TRUE;
				for (x = 0; x < other->width && empty; x++) {
					for (y = 0; y < other->height && empty; y++) {
						DATA32 pixel = data[y * other->width + x];
						DATA32 a = (pixel >> 24) & 0xff;
						if (a == 0)
							continue;

						DATA32 rgb = pixel & 0xffFFff;
						DATA32 pixel_bg = data_bg[y * other->width + x];
						DATA32 rgb_bg = pixel_bg & 0xffFFff;
						if (rgb != rgb_bg) {
							if (systray_profile)
								fprintf(stderr, "Pixel: %x different from bg %x at pos %d %d\n", pixel, pixel_bg, x, y);
							empty = FALSE;
						}
					}
				}
				other->empty = empty;
				imlib_free_image_and_decache();
				imlib_context_set_image(bg);
				imlib_free_image_and_decache();
				if (systray_profile)
					fprintf(stderr,
					        "[%f] %s:%d win = %lu (%s) empty = %d\n",
					        profiling_get_time(),
					        __FUNCTION__,
					        __LINE__,
					        other->win,
					        other->name,
					        other->empty);
			}
		}
		if (pid && other->pid == pid) {
			if (other->empty)
				num_empty_same_pid++;
		}
	}

	// Remove empty icons if the application leaves behind more than 1
	const int max_num_empty_same_pid = 0;
	if (num_empty_same_pid > max_num_empty_same_pid) {
		for (GSList *l = systray.list_icons; l; l = l->next) {
			if (pid && ((TrayWindow *)l->data)->pid == pid && ((TrayWindow *)l->data)->empty) {
				num_empty_same_pid++;
				fprintf(stderr,
				        RED
				        "Removing tray icon %lu (%s) from misbehaving application with pid=%d (too many icons)\n" RESET,
				        ((TrayWindow *)l->data)->win,
				        ((TrayWindow *)l->data)->name,
				        pid);
				remove_icon((TrayWindow *)l->data);
				break;
			}
		}
	}

	// Create the parent window that will embed the icon
	XWindowAttributes attr;
	if (systray_profile)
		fprintf(stderr, "XGetWindowAttributes(server.display, win = %ld, &attr)\n", win);
	if (XGetWindowAttributes(server.display, win, &attr) == False) {
		free(name);
		return FALSE;
	}
	unsigned long mask = 0;
	XSetWindowAttributes set_attr;
	Visual *visual = server.visual;
	fprintf(stderr,
	        GREEN "add_icon: %lu (%s), pid %d, %d, visual %p, colormap %lu, depth %d, width %d, height %d\n" RESET,
	        win,
	        name,
	        pid,
	        num_empty_same_pid,
	        attr.visual,
	        attr.colormap,
	        attr.depth,
	        attr.width,
	        attr.height);
	if (server.disable_transparency) {
		set_attr.background_pixmap = ParentRelative;
		mask = CWBackPixmap;
		if (systray_composited || attr.depth != server.depth) {
			visual = attr.visual;
			set_attr.colormap = attr.colormap;
			mask |= CWColormap;
		}
	} else {
		if (systray_composited || attr.depth != server.depth) {
			visual = attr.visual;
			set_attr.background_pixel = 0;
			set_attr.border_pixel = 0;
			set_attr.colormap = attr.colormap;
			mask = CWColormap | CWBackPixel | CWBorderPixel;
		} else {
			set_attr.background_pixmap = ParentRelative;
			mask = CWBackPixmap;
		}
	}
	if (systray_profile)
		fprintf(stderr, "XCreateWindow(...)\n");
	Window parent = XCreateWindow(server.display,
	                              panel->main_win,
	                              0,
	                              0,
	                              systray.icon_size,
	                              systray.icon_size,
	                              0,
	                              attr.depth,
	                              InputOutput,
	                              visual,
	                              mask,
	                              &set_attr);

	// Add the icon to the list
	TrayWindow *traywin = g_new0(TrayWindow, 1);
	traywin->parent = parent;
	traywin->win = win;
	traywin->hide = hide;
	traywin->depth = attr.depth;
	// Reparenting is done at the first paint event when the window is positioned correctly over its empty background,
	// to prevent graphical corruptions in icons with fake transparency
	traywin->pid = pid;
	traywin->name = name;
	traywin->chrono = chrono;
	chrono++;

	if (systray.area.on_screen == 0)
		show(&systray.area);

	if (systray.sort == SYSTRAY_SORT_RIGHT2LEFT)
		systray.list_icons = g_slist_prepend(systray.list_icons, traywin);
	else
		systray.list_icons = g_slist_append(systray.list_icons, traywin);
	systray.list_icons = g_slist_sort(systray.list_icons, compare_traywindows);

	if (!traywin->hide && !panel->is_hidden) {
		if (systray_profile)
			fprintf(stderr, "XMapRaised(server.display, traywin->parent)\n");
		XMapRaised(server.display, traywin->parent);
	}

	if (systray_profile)
		fprintf(stderr, "[%f] %s:%d\n", profiling_get_time(), __FUNCTION__, __LINE__);

	// Resize and redraw the systray
	if (systray_profile)
		fprintf(stderr, BLUE "[%f] %s:%d trigger resize & redraw\n" RESET, profiling_get_time(), __FUNCTION__, __LINE__);
	systray.area.resize_needed = TRUE;
	schedule_redraw(&systray.area);
	panel->area.resize_needed = TRUE;
	panel_refresh = TRUE;
	refresh_systray = TRUE;
	return TRUE;
}

gboolean reparent_icon(TrayWindow *traywin)
{
	if (systray_profile)
		fprintf(stderr,
		        "[%f] %s:%d win = %lu (%s)\n",
		        profiling_get_time(),
		        __FUNCTION__,
		        __LINE__,
		        traywin->win,
		        traywin->name);
	if (traywin->reparented)
		return TRUE;

	// Watch for the icon trying to resize itself / closing again
	XSync(server.display, False);
	error = FALSE;
	XErrorHandler old = XSetErrorHandler(window_error_handler);
	if (systray_profile)
		fprintf(stderr, "XSelectInput(server.display, traywin->win, ...)\n");
	XSelectInput(server.display, traywin->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
	XWithdrawWindow(server.display, traywin->win, server.screen);
	XReparentWindow(server.display, traywin->win, traywin->parent, 0, 0);

	if (systray_profile)
		fprintf(stderr,
				"XMoveResizeWindow(server.display, traywin->win = %ld, 0, 0, traywin->width = %d, traywin->height = %d)\n",
		        traywin->win,
		        traywin->width,
		        traywin->height);
	XMoveResizeWindow(server.display, traywin->win, 0, 0, traywin->width, traywin->height);

	// Embed into parent
	{
		XEvent e;
		e.xclient.type = ClientMessage;
		e.xclient.serial = 0;
		e.xclient.send_event = True;
		e.xclient.message_type = server.atom._XEMBED;
		e.xclient.window = traywin->win;
		e.xclient.format = 32;
		e.xclient.data.l[0] = CurrentTime;
		e.xclient.data.l[1] = XEMBED_EMBEDDED_NOTIFY;
		e.xclient.data.l[2] = 0;
		e.xclient.data.l[3] = traywin->parent;
		e.xclient.data.l[4] = 0;
		if (systray_profile)
			fprintf(stderr, "XSendEvent(server.display, traywin->win, False, NoEventMask, &e)\n");
		XSendEvent(server.display, traywin->win, False, NoEventMask, &e);
	}

	XSync(server.display, False);
	XSetErrorHandler(old);
	if (error != FALSE) {
		fprintf(stderr,
		        RED "systray %d: cannot embed icon for window %lu (%s) parent %lu pid %d\n" RESET,
		        __LINE__,
		        traywin->win,
		        traywin->name,
		        traywin->parent,
		        traywin->pid);
		remove_icon(traywin);
		return FALSE;
	}

	traywin->reparented = TRUE;

	if (systray_profile)
		fprintf(stderr,
		        "[%f] %s:%d win = %lu (%s)\n",
		        profiling_get_time(),
		        __FUNCTION__,
		        __LINE__,
		        traywin->win,
		        traywin->name);

	return TRUE;
}

gboolean embed_icon(TrayWindow *traywin)
{
	if (systray_profile)
		fprintf(stderr,
		        "[%f] %s:%d win = %lu (%s)\n",
		        profiling_get_time(),
		        __FUNCTION__,
		        __LINE__,
		        traywin->win,
		        traywin->name);
	if (traywin->embedded)
		return TRUE;

	Panel *panel = systray.area.panel;

	XSync(server.display, False);
	error = FALSE;
	XErrorHandler old = XSetErrorHandler(window_error_handler);

	if (0) {
		Atom acttype;
		int actfmt;
		unsigned long nbitem, bytes;
		unsigned long *data = 0;
		int ret;

		if (systray_profile)
			fprintf(stderr,
					"XGetWindowProperty(server.display, traywin->win, server.atom._XEMBED_INFO, 0, 2, False, "
			        "server.atom._XEMBED_INFO, &acttype, &actfmt, &nbitem, &bytes, &data)\n");
		ret = XGetWindowProperty(server.display,
		                         traywin->win,
		                         server.atom._XEMBED_INFO,
		                         0,
		                         2,
		                         False,
		                         server.atom._XEMBED_INFO,
		                         &acttype,
		                         &actfmt,
		                         &nbitem,
		                         &bytes,
		                         (unsigned char **)&data);
		if (ret == Success) {
			if (data) {
				if (nbitem >= 2) {
					int hide = ((data[1] & XEMBED_MAPPED) == 0);
					if (hide) {
						// In theory we have to check the embedding with this and remove icons that refuse embedding.
						// In practice we have no idea when the other application processes the event and accepts the
						// embed
						// so we cannot check now without a race.
						// Race can be triggered with PyGtk(2) apps.
						// We could defer this for later (if we set PropertyChangeMask in XSelectInput we get notified)
						// but
						// for some reason it breaks transparency for Qt icons. So we don't.
						// fprintf(stderr, RED "tint2: window refused embedding\n" RESET);
						// remove_icon(traywin);
						// XFree(data);
						// return FALSE;
					}
				}
				XFree(data);
			}
		} else {
			fprintf(stderr, RED "tint2 : xembed error\n" RESET);
			remove_icon(traywin);
			return FALSE;
		}
	}

	// Redirect rendering when using compositing
	if (systray_composited) {
		if (systray_profile)
			fprintf(stderr, "XDamageCreate(server.display, traywin->parent, XDamageReportRawRectangles)\n");
		traywin->damage = XDamageCreate(server.display, traywin->parent, XDamageReportRawRectangles);
		if (systray_profile)
			fprintf(stderr, "XCompositeRedirectWindow(server.display, traywin->parent, CompositeRedirectManual)\n");
		XCompositeRedirectWindow(server.display, traywin->parent, CompositeRedirectManual);
	}

	XRaiseWindow(server.display, traywin->win);

	// Make the icon visible
	if (!traywin->hide) {
		if (systray_profile)
			fprintf(stderr, "XMapWindow(server.display, traywin->win)\n");
		XMapWindow(server.display, traywin->win);
	}
	if (!traywin->hide && !panel->is_hidden) {
		if (systray_profile)
			fprintf(stderr, "XMapRaised(server.display, traywin->parent)\n");
		XMapRaised(server.display, traywin->parent);
	}

	if (systray_profile)
		fprintf(stderr, "XSync(server.display, False)\n");
	XSync(server.display, False);
	XSetErrorHandler(old);
	if (error != FALSE) {
		fprintf(stderr,
		        RED "systray %d: cannot embed icon for window %lu (%s) parent %lu pid %d\n" RESET,
		        __LINE__,
		        traywin->win,
		        traywin->name,
		        traywin->parent,
		        traywin->pid);
		remove_icon(traywin);
		return FALSE;
	}

	traywin->embedded = TRUE;

	if (systray_profile)
		fprintf(stderr,
		        "[%f] %s:%d win = %lu (%s)\n",
		        profiling_get_time(),
		        __FUNCTION__,
		        __LINE__,
		        traywin->win,
		        traywin->name);

	return TRUE;
}

void remove_icon(TrayWindow *traywin)
{
	if (systray_profile)
		fprintf(stderr,
		        "[%f] %s:%d win = %lu (%s)\n",
		        profiling_get_time(),
		        __FUNCTION__,
		        __LINE__,
		        traywin->win,
		        traywin->name);
	Panel *panel = systray.area.panel;

	// remove from our list
	systray.list_icons = g_slist_remove(systray.list_icons, traywin);
	fprintf(stderr, YELLOW "remove_icon: %lu (%s)\n" RESET, traywin->win, traywin->name);

	XSelectInput(server.display, traywin->win, NoEventMask);
	if (traywin->damage)
		XDamageDestroy(server.display, traywin->damage);

	// reparent to root
	XSync(server.display, False);
	error = FALSE;
	XErrorHandler old = XSetErrorHandler(window_error_handler);
	if (!traywin->hide)
		XUnmapWindow(server.display, traywin->win);
	XReparentWindow(server.display, traywin->win, server.root_win, 0, 0);
	XDestroyWindow(server.display, traywin->parent);
	XSync(server.display, False);
	XSetErrorHandler(old);
	stop_timeout(traywin->render_timeout);
	stop_timeout(traywin->resize_timeout);
	free(traywin->name);
	if (traywin->image) {
		imlib_context_set_image(traywin->image);
		imlib_free_image_and_decache();
	}
	g_free(traywin);

	// check empty systray
	int count = 0;
	GSList *l;
	for (l = systray.list_icons; l; l = l->next) {
		if (((TrayWindow *)l->data)->hide)
			continue;
		count++;
	}
	if (count == 0)
		hide(&systray.area);

	// Resize and redraw the systray
	if (systray_profile)
		fprintf(stderr, BLUE "[%f] %s:%d trigger resize & redraw\n" RESET, profiling_get_time(), __FUNCTION__, __LINE__);
	systray.area.resize_needed = TRUE;
	schedule_redraw(&systray.area);
	panel->area.resize_needed = TRUE;
	panel_refresh = TRUE;
	refresh_systray = TRUE;
}

void systray_resize_icon(void *t)
{
	// we end up in this function only in real transparency mode or if systray_task_asb != 100 0 0
	// we made also sure, that we always have a 32 bit visual, i.e. we can safely create 32 bit pixmaps here
	TrayWindow *traywin = t;

	unsigned int border_width;
	int xpos, ypos;
	unsigned int width, height, depth;
	Window root;
	if (!XGetGeometry(server.display, traywin->win, &root, &xpos, &ypos, &width, &height, &border_width, &depth)) {
		return;
	} else {
		if (1 || xpos != 0 || ypos != 0 || width != traywin->width || height != traywin->height) {
			if (systray_profile)
				fprintf(stderr,
						"XMoveResizeWindow(server.display, traywin->win = %ld, 0, 0, traywin->width = %d, traywin->height "
				        "= %d)\n",
				        traywin->win,
				        traywin->width,
				        traywin->height);
			if (0) {
				XMoveResizeWindow(server.display, traywin->win, 0, 0, traywin->width, traywin->height);
			}
			if (0) {
				XWindowChanges changes;
				changes.x = changes.y = 0;
				changes.width = traywin->width;
				changes.height = traywin->height;
				XConfigureWindow(server.display, traywin->win, CWX | CWY | CWWidth | CWHeight, &changes);
			}
			if (1) {
				XConfigureEvent ev;
				ev.type = ConfigureNotify;
				ev.serial = 0;
				ev.send_event = True;
				ev.event = traywin->win;
				ev.window = traywin->win;
				ev.x = 0;
				ev.y = 0;
				ev.width = traywin->width;
				ev.height = traywin->height;
				ev.border_width = 0;
				ev.above = None;
				ev.override_redirect = False;
				XSendEvent(server.display, traywin->win, False, StructureNotifyMask, (XEvent *)&ev);
			}
			XSync(server.display, False);
		}
	}
}

void systray_reconfigure_event(TrayWindow *traywin, XEvent *e)
{
	if (systray_profile)
		fprintf(stderr,
		        "XConfigure event: win = %lu (%s), x = %d, y = %d, w = %d, h = %d\n",
		        traywin->win,
		        traywin->name,
		        e->xconfigure.x,
		        e->xconfigure.y,
		        e->xconfigure.width,
		        e->xconfigure.height);

	if (!traywin->reparented)
		return;

	if (e->xconfigure.width != traywin->width || e->xconfigure.height != traywin->height || e->xconfigure.x != 0 ||
	    e->xconfigure.y != 0) {
		if (traywin->bad_size_counter < max_bad_resize_events) {
			struct timespec now;
			clock_gettime(CLOCK_MONOTONIC, &now);
			struct timespec earliest_resize = add_msec_to_timespec(traywin->time_last_resize, resize_period_threshold);
			if (compare_timespecs(&earliest_resize, &now) > 0) {
				// Fast resize, but below the threshold
				traywin->bad_size_counter++;
			} else {
				// Slow resize, reset counter
				traywin->time_last_resize.tv_sec = now.tv_sec;
				traywin->time_last_resize.tv_nsec = now.tv_nsec;
				traywin->bad_size_counter = 0;
			}
			if (traywin->bad_size_counter < min_bad_resize_events) {
				systray_resize_icon(traywin);
			} else {
				if (!traywin->resize_timeout)
					traywin->resize_timeout =
					    add_timeout(fast_resize_period, 0, systray_resize_icon, traywin, &traywin->resize_timeout);
			}
		} else {
			if (traywin->bad_size_counter == max_bad_resize_events) {
				traywin->bad_size_counter++;
				fprintf(stderr,
				        RED "Detected resize loop for tray icon %lu (%s), throttling resize events\n" RESET,
				        traywin->win,
				        traywin->name);
			}
			// Delayed resize
			// FIXME Normally we should force the icon to resize fill_color to the size we resized it to when we
			// embedded it.
			// However this triggers a resize loop in new versions of GTK, which we must avoid.
			if (!traywin->resize_timeout)
				traywin->resize_timeout =
				    add_timeout(slow_resize_period, 0, systray_resize_icon, traywin, &traywin->resize_timeout);
			return;
		}
	} else {
		// Correct size
		stop_timeout(traywin->resize_timeout);
	}

	// Resize and redraw the systray
	if (systray_profile)
		fprintf(stderr, BLUE "[%f] %s:%d trigger resize & redraw\n" RESET, profiling_get_time(), __FUNCTION__, __LINE__);
	panel_refresh = TRUE;
	refresh_systray = 1;
}

void systray_resize_request_event(TrayWindow *traywin, XEvent *e)
{
	if (systray_profile)
		fprintf(stderr,
		        "XResizeRequest event: win = %lu (%s), w = %d, h = %d\n",
		        traywin->win,
		        traywin->name,
		        e->xresizerequest.width,
		        e->xresizerequest.height);

	if (!traywin->reparented)
		return;

	if (e->xresizerequest.width != traywin->width || e->xresizerequest.height != traywin->height) {
		if (traywin->bad_size_counter < max_bad_resize_events) {
			struct timespec now;
			clock_gettime(CLOCK_MONOTONIC, &now);
			struct timespec earliest_resize = add_msec_to_timespec(traywin->time_last_resize, resize_period_threshold);
			if (compare_timespecs(&earliest_resize, &now) > 0) {
				// Fast resize, but below the threshold
				traywin->bad_size_counter++;
			} else {
				// Slow resize, reset counter
				traywin->time_last_resize.tv_sec = now.tv_sec;
				traywin->time_last_resize.tv_nsec = now.tv_nsec;
				traywin->bad_size_counter = 0;
			}
			if (traywin->bad_size_counter < min_bad_resize_events) {
				systray_resize_icon(traywin);
			} else {
				if (!traywin->resize_timeout)
					traywin->resize_timeout =
					    add_timeout(fast_resize_period, 0, systray_resize_icon, traywin, &traywin->resize_timeout);
			}
		} else {
			if (traywin->bad_size_counter == max_bad_resize_events) {
				traywin->bad_size_counter++;
				fprintf(stderr,
				        RED "Detected resize loop for tray icon %lu (%s), throttling resize events\n" RESET,
				        traywin->win,
				        traywin->name);
			}
			// Delayed resize
			// FIXME Normally we should force the icon to resize fill_color to the size we resized it to when we
			// embedded it.
			// However this triggers a resize loop in new versions of GTK, which we must avoid.
			if (!traywin->resize_timeout)
				traywin->resize_timeout =
				    add_timeout(slow_resize_period, 0, systray_resize_icon, traywin, &traywin->resize_timeout);
			return;
		}
	} else {
		// Correct size
		stop_timeout(traywin->resize_timeout);
	}

	// Resize and redraw the systray
	if (systray_profile)
		fprintf(stderr, BLUE "[%f] %s:%d trigger resize & redraw\n" RESET, profiling_get_time(), __FUNCTION__, __LINE__);
	panel_refresh = TRUE;
	refresh_systray = 1;
}

void systray_destroy_event(TrayWindow *traywin)
{
	if (systray_profile)
		fprintf(stderr,
		        "[%f] %s:%d win = %lu (%s)\n",
		        profiling_get_time(),
		        __FUNCTION__,
		        __LINE__,
		        traywin->win,
		        traywin->name);
	remove_icon(traywin);
}

void systray_render_icon_from_image(TrayWindow *traywin)
{
	if (!traywin->image)
		return;
	imlib_context_set_image(traywin->image);
	XCopyArea(server.display,
			  render_background,
			  systray.area.pix,
			  server.gc,
			  traywin->x - systray.area.posx,
			  traywin->y - systray.area.posy,
			  traywin->width,
			  traywin->height,
			  traywin->x - systray.area.posx,
			  traywin->y - systray.area.posy);
	render_image(systray.area.pix, traywin->x - systray.area.posx, traywin->y - systray.area.posy);
}

void systray_render_icon_composited(void *t)
{
	// we end up in this function only in real transparency mode or if systray_task_asb != 100 0 0
	// we made also sure, that we always have a 32 bit visual, i.e. we can safely create 32 bit pixmaps here
	TrayWindow *traywin = t;

	if (systray_profile)
		fprintf(stderr,
		        "[%f] %s:%d win = %lu (%s)\n",
		        profiling_get_time(),
		        __FUNCTION__,
		        __LINE__,
		        traywin->win,
		        traywin->name);

	// wine tray icons update whenever mouse is over them, so we limit the updates to 50 ms
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	struct timespec earliest_render = add_msec_to_timespec(traywin->time_last_render, min_refresh_period);
	if (compare_timespecs(&earliest_render, &now) > 0) {
		traywin->num_fast_renders++;
		if (traywin->num_fast_renders > max_fast_refreshes) {
			traywin->render_timeout =
			    add_timeout(min_refresh_period, 0, systray_render_icon_composited, traywin, &traywin->render_timeout);
			if (systray_profile)
				fprintf(stderr,
				        YELLOW "[%f] %s:%d win = %lu (%s) delaying rendering\n" RESET,
				        profiling_get_time(),
				        __FUNCTION__,
				        __LINE__,
				        traywin->win,
				        traywin->name);
			return;
		}
	} else {
		traywin->time_last_render.tv_sec = now.tv_sec;
		traywin->time_last_render.tv_nsec = now.tv_nsec;
		traywin->num_fast_renders = 0;
	}

	if (traywin->width == 0 || traywin->height == 0) {
		// reschedule rendering since the geometry information has not yet been processed (can happen on slow cpu)
		traywin->render_timeout =
		    add_timeout(min_refresh_period, 0, systray_render_icon_composited, traywin, &traywin->render_timeout);
		if (systray_profile)
			fprintf(stderr,
			        YELLOW "[%f] %s:%d win = %lu (%s) delaying rendering\n" RESET,
			        profiling_get_time(),
			        __FUNCTION__,
			        __LINE__,
			        traywin->win,
			        traywin->name);
		return;
	}

	if (traywin->render_timeout) {
		stop_timeout(traywin->render_timeout);
		traywin->render_timeout = NULL;
	}

	// good systray icons support 32 bit depth, but some icons are still 24 bit.
	// We create a heuristic mask for these icons, i.e. we get the rgb value in the top left corner, and
	// mask out all pixel with the same rgb value
	Panel *panel = systray.area.panel;

	// Very ugly hack, but somehow imlib2 is not able to get the image from the traywindow itself,
	// so we first render the tray window onto a pixmap, and then we tell imlib2 to use this pixmap as
	// drawable. If someone knows why it does not work with the traywindow itself, please tell me ;)
	Pixmap tmp_pmap = XCreatePixmap(server.display, traywin->win, traywin->width, traywin->height, 32);
	if (!tmp_pmap) {
		goto on_systray_error;
	}
	XRenderPictFormat *f;
	if (traywin->depth == 24) {
		f = XRenderFindStandardFormat(server.display, PictStandardRGB24);
	} else if (traywin->depth == 32) {
		f = XRenderFindStandardFormat(server.display, PictStandardARGB32);
	} else {
		fprintf(stderr, RED "Strange tray icon found with depth: %d\n" RESET, traywin->depth);
		XFreePixmap(server.display, tmp_pmap);
		return;
	}
	XRenderPictFormat *f32 = XRenderFindVisualFormat(server.display, server.visual32);
	if (!f || !f32) {
		XFreePixmap(server.display, tmp_pmap);
		goto on_systray_error;
	}

	XSync(server.display, False);
	error = FALSE;
	XErrorHandler old = XSetErrorHandler(window_error_handler);

	// if (server.real_transparency)
	// Picture pict_image = XRenderCreatePicture(server.display, traywin->parent, f, 0, 0);
	// reverted Rev 407 because here it's breaking alls icon with systray + xcompmgr
	Picture pict_image = XRenderCreatePicture(server.display, traywin->win, f, 0, 0);
	if (!pict_image) {
		XFreePixmap(server.display, tmp_pmap);
		XSetErrorHandler(old);
		goto on_error;
	}
	Picture pict_drawable =
		XRenderCreatePicture(server.display, tmp_pmap, XRenderFindVisualFormat(server.display, server.visual32), 0, 0);
	if (!pict_drawable) {
		XRenderFreePicture(server.display, pict_image);
		XFreePixmap(server.display, tmp_pmap);
		XSetErrorHandler(old);
		goto on_error;
	}
	XRenderComposite(server.display,
	                 PictOpSrc,
	                 pict_image,
	                 None,
	                 pict_drawable,
	                 0,
	                 0,
	                 0,
	                 0,
	                 0,
	                 0,
	                 traywin->width,
	                 traywin->height);
	XRenderFreePicture(server.display, pict_image);
	XRenderFreePicture(server.display, pict_drawable);
	// end of the ugly hack and we can continue as before

	imlib_context_set_visual(server.visual32);
	imlib_context_set_colormap(server.colormap32);
	imlib_context_set_drawable(tmp_pmap);
	Imlib_Image image = imlib_create_image_from_drawable(0, 0, 0, traywin->width, traywin->height, 1);
	imlib_context_set_visual(server.visual);
	imlib_context_set_colormap(server.colormap);
	XFreePixmap(server.display, tmp_pmap);
	if (!image) {
		imlib_context_set_visual(server.visual);
		imlib_context_set_colormap(server.colormap);
		XSetErrorHandler(old);
		goto on_error;
	} else {
		if (traywin->image) {
			imlib_context_set_image(traywin->image);
			imlib_free_image_and_decache();
		}
		traywin->image = image;
	}

	imlib_context_set_image(traywin->image);
	// if (traywin->depth == 24)
	// imlib_save_image("/home/thil77/test.jpg");
	imlib_image_set_has_alpha(1);
	DATA32 *data = imlib_image_get_data();
	if (traywin->depth == 24) {
		create_heuristic_mask(data, traywin->width, traywin->height);
	}

	int empty = image_empty(data, traywin->width, traywin->height);
	if (systray.alpha != 100 || systray.brightness != 0 || systray.saturation != 0)
		adjust_asb(data,
		           traywin->width,
		           traywin->height,
		           systray.alpha,
		           (float)systray.saturation / 100,
		           (float)systray.brightness / 100);
	imlib_image_put_back_data(data);

	systray_render_icon_from_image(traywin);

	if (traywin->damage)
		XDamageSubtract(server.display, traywin->damage, None, None);
	XSync(server.display, False);
	XSetErrorHandler(old);

	if (error)
		goto on_error;

	if (traywin->empty != empty) {
		traywin->empty = empty;
		systray.list_icons = g_slist_sort(systray.list_icons, compare_traywindows);
		// Resize and redraw the systray
		if (systray_profile)
			fprintf(stderr,
			        BLUE "[%f] %s:%d trigger resize & redraw\n" RESET,
			        profiling_get_time(),
			        __FUNCTION__,
			        __LINE__);
		systray.area.resize_needed = 1;
		schedule_redraw(&systray.area);
		panel->area.resize_needed = 1;
		panel_refresh = TRUE;
		refresh_systray = 1;
	}
	panel_refresh = TRUE;

	if (systray_profile)
		fprintf(stderr,
		        "[%f] %s:%d win = %lu (%s)\n",
		        profiling_get_time(),
		        __FUNCTION__,
		        __LINE__,
		        traywin->win,
		        traywin->name);

	return;

on_error:
	fprintf(stderr,
	        RED "systray %d: rendering error for icon %lu (%s) pid %d\n" RESET,
	        __LINE__,
	        traywin->win,
	        traywin->name,
	        traywin->pid);
	return;

on_systray_error:
	fprintf(stderr,
	        RED "systray %d: rendering error for icon %lu (%s) pid %d. "
	            "Disabling compositing and restarting systray...\n" RESET,
	        __LINE__,
	        traywin->win,
	        traywin->name,
	        traywin->pid);
	systray_composited = 0;
	stop_net();
	start_net();
	return;
}

void systray_render_icon(void *t)
{
	TrayWindow *traywin = t;
	if (systray_profile)
		fprintf(stderr,
		        "[%f] %s:%d win = %lu (%s)\n",
		        profiling_get_time(),
		        __FUNCTION__,
		        __LINE__,
		        traywin->win,
		        traywin->name);
	if (!traywin->reparented || !traywin->embedded) {
		if (systray_profile)
			fprintf(stderr,
			        YELLOW "[%f] %s:%d win = %lu (%s) delaying rendering\n" RESET,
			        profiling_get_time(),
			        __FUNCTION__,
			        __LINE__,
			        traywin->win,
			        traywin->name);
		stop_timeout(traywin->render_timeout);
		traywin->render_timeout =
		    add_timeout(min_refresh_period, 0, systray_render_icon, traywin, &traywin->render_timeout);
		return;
	}

	if (systray_composited) {
		XSync(server.display, False);
		error = FALSE;
		XErrorHandler old = XSetErrorHandler(window_error_handler);

		unsigned int border_width;
		int xpos, ypos;
		unsigned int width, height, depth;
		Window root;
		if (!XGetGeometry(server.display, traywin->win, &root, &xpos, &ypos, &width, &height, &border_width, &depth)) {
			stop_timeout(traywin->render_timeout);
			if (!traywin->resize_timeout)
				traywin->render_timeout =
				    add_timeout(min_refresh_period, 0, systray_render_icon, traywin, &traywin->render_timeout);
			systray_render_icon_from_image(traywin);
			XSetErrorHandler(old);
			return;
		} else {
			if (xpos != 0 || ypos != 0 || width != traywin->width || height != traywin->height) {
				stop_timeout(traywin->render_timeout);
				if (!traywin->resize_timeout)
					traywin->render_timeout =
					    add_timeout(min_refresh_period, 0, systray_render_icon, traywin, &traywin->render_timeout);
				systray_render_icon_from_image(traywin);
				if (systray_profile)
					fprintf(stderr,
					        YELLOW "[%f] %s:%d win = %lu (%s) delaying rendering\n" RESET,
					        profiling_get_time(),
					        __FUNCTION__,
					        __LINE__,
					        traywin->win,
					        traywin->name);
				XSetErrorHandler(old);
				return;
			}
		}
		XSetErrorHandler(old);
	}

	if (systray_profile)
		fprintf(stderr, "rendering tray icon\n");

	if (systray_composited) {
		systray_render_icon_composited(traywin);
	} else {
		// Trigger window repaint
		if (systray_profile)
			fprintf(stderr,
					"XClearArea(server.display, traywin->parent = %ld, 0, 0, traywin->width, traywin->height, True)\n",
			        traywin->parent);
		XClearArea(server.display, traywin->parent, 0, 0, 0, 0, True);
		if (systray_profile)
			fprintf(stderr,
					"XClearArea(server.display, traywin->win = %ld, 0, 0, traywin->width, traywin->height, True)\n",
			        traywin->win);
		XClearArea(server.display, traywin->win, 0, 0, 0, 0, True);
	}
}

void refresh_systray_icons()
{
	if (systray_profile)
		fprintf(stderr, BLUE "[%f] %s:%d\n" RESET, profiling_get_time(), __FUNCTION__, __LINE__);
	TrayWindow *traywin;
	GSList *l;
	for (l = systray.list_icons; l; l = l->next) {
		traywin = (TrayWindow *)l->data;
		if (traywin->hide)
			continue;
		systray_render_icon(traywin);
	}
}

gboolean systray_on_monitor(int i_monitor, int num_panelss)
{
	return (i_monitor == systray_monitor) ||
	       (i_monitor == 0 && (systray_monitor >= num_panelss || systray_monitor < 0));
}
