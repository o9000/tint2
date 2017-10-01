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
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <Imlib2.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <unistd.h>

#include "systraybar.h"
#include "server.h"
#include "panel.h"
#include "window.h"

GSList *icons;

/* defined in the systray spec */
#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

// selection window
Window net_sel_win = None;

// freedesktop specification doesn't allow multi systray
Systray systray;
gboolean refresh_systray;
gboolean systray_enabled;
int systray_max_icon_size;
int systray_monitor;
int chrono;
int systray_composited;
int systray_profile;
char *systray_hide_name_filter;
regex_t *systray_hide_name_regex;
// background pixmap if we render ourselves the icons
static Pixmap render_background;

const int min_refresh_period = 50;
const int max_fast_refreshes = 5;
const int resize_period_threshold = 1000;
const int fast_resize_period = 50;
const int slow_resize_period = 5000;
const int min_bad_resize_events = 3;
const int max_bad_resize_events = 10;

int systray_compute_desired_size(void *obj);
void systray_dump_geometry(void *obj, int indent);

void default_systray()
{
    systray_enabled = 0;
    memset(&systray, 0, sizeof(systray));
    render_background = 0;
    chrono = 0;
    systray.alpha = 100;
    systray.sort = SYSTRAY_SORT_LEFT2RIGHT;
    systray.area._draw_foreground = draw_systray;
    systray.area._on_change_layout = on_change_systray;
    systray.area.size_mode = LAYOUT_FIXED;
    systray.area._resize = resize_systray;
    systray_profile = getenv("SYSTRAY_PROFILING") != NULL;
    systray_hide_name_filter = NULL;
    systray_hide_name_regex = NULL;
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
    if (systray_hide_name_regex) {
        regfree(systray_hide_name_regex);
        free_and_null(systray_hide_name_regex);
    }
    if (systray_hide_name_filter)
        free_and_null(systray_hide_name_filter);
}

void init_systray()
{
    if (!systray_enabled)
        return;

    systray_composited = !server.disable_transparency && server.visual32 && server.colormap32;
    fprintf(stderr, "tint2: Systray composited rendering %s\n", systray_composited ? "on" : "off");

    if (!systray_composited) {
        fprintf(stderr, "tint2: systray_asb forced to 100 0 0\n");
        systray.alpha = 100;
        systray.brightness = systray.saturation = 0;
    }
}

void init_systray_panel(void *p)
{
    Panel *panel = (Panel *)p;
    systray.area.parent = panel;
    systray.area.panel = panel;
    systray.area._dump_geometry = systray_dump_geometry;
    systray.area._compute_desired_size = systray_compute_desired_size;
    snprintf(systray.area.name, sizeof(systray.area.name), "Systray");
    if (!systray.area.bg)
        systray.area.bg = &g_array_index(backgrounds, Background, 0);
    show(&systray.area);
    schedule_redraw(&systray.area);
    refresh_systray = TRUE;
    instantiate_area_gradients(&systray.area);
}

void systray_compute_geometry(int *size)
{
    systray.icon_size = panel_horizontal ? systray.area.height : systray.area.width;
    systray.icon_size -=
        MAX(left_right_border_width(&systray.area), top_bottom_border_width(&systray.area)) + 2 * systray.area.paddingy;
    if (systray_max_icon_size > 0)
        systray.icon_size = MIN(systray.icon_size, systray_max_icon_size);

    int count = 0;
    for (GSList *l = systray.list_icons; l; l = l->next) {
        count++;
    }

    if (panel_horizontal) {
        int height = systray.area.height - top_bottom_border_width(&systray.area) - 2 * systray.area.paddingy;
        // here icons_per_column always higher than 0
        systray.icons_per_column = (height + systray.area.paddingx) / (systray.icon_size + systray.area.paddingx);
        systray.margin =
            height - (systray.icons_per_column - 1) * (systray.icon_size + systray.area.paddingx) - systray.icon_size;
        systray.icons_per_row = count / systray.icons_per_column + (count % systray.icons_per_column != 0);
        *size = left_right_border_width(&systray.area) + 2 * systray.area.paddingxlr +
                (systray.icon_size * systray.icons_per_row) + ((systray.icons_per_row - 1) * systray.area.paddingx);
    } else {
        int width = systray.area.width - left_right_border_width(&systray.area) - 2 * systray.area.paddingy;
        // here icons_per_row always higher than 0
        systray.icons_per_row = (width + systray.area.paddingx) / (systray.icon_size + systray.area.paddingx);
        systray.margin =
            width - (systray.icons_per_row - 1) * (systray.icon_size + systray.area.paddingx) - systray.icon_size;
        systray.icons_per_column = count / systray.icons_per_row + (count % systray.icons_per_row != 0);
        *size = top_bottom_border_width(&systray.area) + (2 * systray.area.paddingxlr) +
                (systray.icon_size * systray.icons_per_column) +
                ((systray.icons_per_column - 1) * systray.area.paddingx);
    }
}

int systray_compute_desired_size(void *obj)
{
    int size;
    systray_compute_geometry(&size);
    return size;
}

gboolean resize_systray(void *obj)
{
    if (systray_profile)
        fprintf(stderr, "tint2: [%f] %s:%d\n", profiling_get_time(), __func__, __LINE__);

    int size;
    systray_compute_geometry(&size);

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

    gboolean result = refresh_systray;
    if (net_sel_win == None) {
        start_net();
        result = TRUE;
    }

    if (panel_horizontal) {
        if (systray.area.width != size) {
            systray.area.width = size;
            result = TRUE;
        }
    } else {
        if (systray.area.height != size) {
            systray.area.height = size;
            result = TRUE;
        }
    }

    on_change_systray(&systray.area);

    return result;
}

void draw_systray(void *obj, cairo_t *c)
{
    if (systray_profile)
        fprintf(stderr, BLUE "tint2: [%f] %s:%d" RESET "\n", profiling_get_time(), __func__, __LINE__);
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

void systray_dump_geometry(void *obj, int indent)
{
    Systray *tray = (Systray *)obj;

    fprintf(stderr, "tint2: %*sIcons:\n", indent, "");
    indent += 2;
    for (GSList *l = tray->list_icons; l; l = l->next) {
        TrayWindow *traywin = (TrayWindow *)l->data;
        fprintf(stderr,
                "%*sIcon: x = %d, y = %d, w = %d, h = %d, name = %s\n",
                indent,
                "",
                traywin->x,
                traywin->y,
                traywin->width,
                traywin->height,
                traywin->name);
    }
}

void on_change_systray(void *obj)
{
    if (systray_profile)
        fprintf(stderr, "tint2: [%f] %s:%d\n", profiling_get_time(), __func__, __LINE__);
    if (systray.icons_per_column == 0 || systray.icons_per_row == 0)
        return;

    // systray.area.posx/posy are computed by rendering engine.
    // Based on this we calculate the positions of the tray icons.
    Panel *panel = systray.area.panel;
    int posx, posy;
    int start;
    if (panel_horizontal) {
        posy = start = top_border_width(&panel->area) + panel->area.paddingy + top_border_width(&systray.area) +
                       systray.area.paddingy + systray.margin / 2;
        posx = systray.area.posx + left_border_width(&systray.area) + systray.area.paddingxlr;
    } else {
        posx = start = left_border_width(&panel->area) + panel->area.paddingy + left_border_width(&systray.area) +
                       systray.area.paddingy + systray.margin / 2;
        posy = systray.area.posy + top_border_width(&systray.area) + systray.area.paddingxlr;
    }

    TrayWindow *traywin;
    GSList *l;
    int i;
    for (i = 1, l = systray.list_icons; l; i++, l = l->next) {
        traywin = (TrayWindow *)l->data;

        traywin->y = posy;
        traywin->x = posx;
        if (systray_profile)
            fprintf(stderr,
                    "%s:%d win = %lu (%s), parent = %lu, x = %d, y = %d\n",
                    __func__,
                    __LINE__,
                    traywin->win,
                    traywin->name,
                    traywin->parent,
                    posx,
                    posy);
        traywin->width = systray.icon_size;
        traywin->height = systray.icon_size;
        if (panel_horizontal) {
            if (i % systray.icons_per_column) {
                posy += systray.icon_size + systray.area.paddingx;
            } else {
                posy = start;
                posx += (systray.icon_size + systray.area.paddingx);
            }
        } else {
            if (i % systray.icons_per_row) {
                posx += systray.icon_size + systray.area.paddingx;
            } else {
                posx = start;
                posy += (systray.icon_size + systray.area.paddingx);
            }
        }

        // position and size the icon window
        unsigned int border_width;
        int xpos, ypos;
        unsigned int width, height, depth;
        Window root;
        if (!XGetGeometry(server.display, traywin->parent, &root, &xpos, &ypos, &width, &height, &border_width, &depth)) {
            fprintf(stderr, RED "tint2: Couldn't get geometry of window!" RESET "\n");
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
        fprintf(stderr, "tint2: [%f] %s:%d\n", profiling_get_time(), __func__, __LINE__);
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

        fprintf(stderr, RED "tint2: another systray is running, cannot use systray" RESET);
        if (ret == Success && prop) {
            pid = prop[1] * 256;
            pid += prop[0];
            fprintf(stderr, "tint2:  pid=%d", pid);
        }
        fprintf(stderr, RESET "\n");
        return;
    }

    // init systray protocol
    net_sel_win = XCreateSimpleWindow(server.display, server.root_win, -1, -1, 1, 1, 0, 0, 0);
    fprintf(stderr, "tint2: systray window %ld\n", net_sel_win);

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
        fprintf(stderr, RED "tint2: cannot find systray manager" RESET "\n");
        return;
    }

    fprintf(stderr, GREEN "tint2: systray started" RESET "\n");
    if (systray_profile)
        fprintf(stderr, "tint2: [%f] %s:%d\n", profiling_get_time(), __func__, __LINE__);
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

void handle_systray_event(XClientMessageEvent *e)
{
    if (systray_profile)
        fprintf(stderr, "tint2: [%f] %s:%d\n", profiling_get_time(), __func__, __LINE__);

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
            fprintf(stderr, "tint2: message from dockapp: %s\n", e->data.b);
        else
            fprintf(stderr, RED "tint2: SYSTEM_TRAY : unknown message type" RESET "\n");
        break;
    }
}

void stop_net()
{
    if (systray_profile)
        fprintf(stderr, "tint2: [%f] %s:%d\n", profiling_get_time(), __func__, __LINE__);
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
        fprintf(stderr, RED "tint2: [%f] %s:%d" RESET "\n", profiling_get_time(), __func__, __LINE__);
    error = TRUE;
    if (e->error_code != BadWindow) {
        fprintf(stderr, RED "tint2: systray: error code %d" RESET "\n", e->error_code);
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

void print_icons()
{
    fprintf(stderr, "tint2: systray.list_icons: \n");
    for (GSList *l = systray.list_icons; l; l = l->next) {
        TrayWindow *t = l->data;
        fprintf(stderr, "tint2: %s\n", t->name);
    }
    fprintf(stderr, "tint2: systray.list_icons order: \n");
    for (GSList *l = systray.list_icons; l; l = l->next) {
        if (l->next) {
            TrayWindow *t = l->data;
            TrayWindow *u = l->next->data;
            int cmp = compare_traywindows(t, u);
            fprintf(stderr, "tint2: %s %s %s\n", t->name, cmp < 0 ? "<" : cmp == 0 ? "=" : ">", u->name);
        }
    }
}

gboolean reject_icon(Window win)
{
    if (systray_hide_name_filter && strlen(systray_hide_name_filter)) {
        if (!systray_hide_name_regex) {
            systray_hide_name_regex = (regex_t *)calloc(1, sizeof(*systray_hide_name_regex));
            if (regcomp(systray_hide_name_regex, systray_hide_name_filter, 0) != 0) {
                fprintf(stderr, RED "tint2: Could not compile regex %s" RESET "\n", systray_hide_name_filter);
                free_and_null(systray_hide_name_regex);
                return FALSE;
            }
        }
        char *name = get_window_name(win);
        if (regexec(systray_hide_name_regex, name, 0, NULL, 0) == 0) {
            fprintf(stderr, GREEN "tint2: Filtering out systray icon '%s'" RESET "\n", name);
            return TRUE;
        }
    }
    return FALSE;
}

gboolean add_icon(Window win)
{
    // Avoid duplicates
    for (GSList *l = systray.list_icons; l; l = l->next) {
        TrayWindow *other = (TrayWindow *)l->data;
        if (other->win == win) {
            return FALSE;
        }
    }

    // Filter out systray_hide_by_icon_name
    if (reject_icon(win))
        return FALSE;

    // Dangerous actions begin
    XSync(server.display, False);
    error = FALSE;
    XErrorHandler old = XSetErrorHandler(window_error_handler);

    XSelectInput(server.display, win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);

    char *name = get_window_name(win);
    if (systray_profile)
        fprintf(stderr, "tint2: [%f] %s:%d win = %lu (%s)\n", profiling_get_time(), __func__, __LINE__, win, name);
    Panel *panel = systray.area.panel;

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

    // Create the parent window that will embed the icon
    XWindowAttributes attr;
    if (systray_profile)
        fprintf(stderr, "tint2: XGetWindowAttributes(server.display, win = %ld, &attr)\n", win);
    if (XGetWindowAttributes(server.display, win, &attr) == False) {
        free(name);
        XSelectInput(server.display, win, NoEventMask);

        // Dangerous actions end
        XSync(server.display, False);
        XSetErrorHandler(old);

        return FALSE;
    }

    // Dangerous actions end
    XSync(server.display, False);
    XSetErrorHandler(old);

    unsigned long mask = 0;
    XSetWindowAttributes set_attr;
    Visual *visual = server.visual;
    fprintf(stderr,
            GREEN "add_icon: %lu (%s), pid %d, visual %p, colormap %lu, depth %d, width %d, height %d" RESET "\n",
            win,
            name,
            pid,
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
        fprintf(stderr, "tint2: XCreateWindow(...)\n");
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
    traywin->depth = attr.depth;
    // Reparenting is done at the first paint event when the window is positioned correctly over its empty background,
    // to prevent graphical corruptions in icons with fake transparency
    traywin->pid = pid;
    traywin->name = name;
    traywin->chrono = chrono;
    chrono++;

    show(&systray.area);

    if (systray.sort == SYSTRAY_SORT_RIGHT2LEFT)
        systray.list_icons = g_slist_prepend(systray.list_icons, traywin);
    else
        systray.list_icons = g_slist_append(systray.list_icons, traywin);
    systray.list_icons = g_slist_sort(systray.list_icons, compare_traywindows);
    // print_icons();

    if (!panel->is_hidden) {
        if (systray_profile)
            fprintf(stderr, "tint2: XMapRaised(server.display, traywin->parent)\n");
        XMapRaised(server.display, traywin->parent);
    }

    if (systray_profile)
        fprintf(stderr, "tint2: [%f] %s:%d\n", profiling_get_time(), __func__, __LINE__);

    // Resize and redraw the systray
    if (systray_profile)
        fprintf(stderr,
                BLUE "[%f] %s:%d trigger resize & redraw" RESET "\n",
                profiling_get_time(),
                __func__,
                __LINE__);
    systray.area.resize_needed = TRUE;
    panel->area.resize_needed = TRUE;
    schedule_redraw(&systray.area);
    refresh_systray = TRUE;
    return TRUE;
}

gboolean reparent_icon(TrayWindow *traywin)
{
    if (systray_profile)
        fprintf(stderr,
                "[%f] %s:%d win = %lu (%s)\n",
                profiling_get_time(),
                __func__,
                __LINE__,
                traywin->win,
                traywin->name);
    if (traywin->reparented)
        return TRUE;

    // Watch for the icon trying to resize itself / closing again
    XSync(server.display, False);
    error = FALSE;
    XErrorHandler old = XSetErrorHandler(window_error_handler);
    XWithdrawWindow(server.display, traywin->win, server.screen);
    XReparentWindow(server.display, traywin->win, traywin->parent, 0, 0);

    if (systray_profile)
        fprintf(stderr,
                "XMoveResizeWindow(server.display, traywin->win = %ld, 0, 0, traywin->width = %d, traywin->height = "
                "%d)\n",
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
            fprintf(stderr, "tint2: XSendEvent(server.display, traywin->win, False, NoEventMask, &e)\n");
        XSendEvent(server.display, traywin->win, False, NoEventMask, &e);
    }

    XSync(server.display, False);
    XSetErrorHandler(old);
    if (error != FALSE) {
        fprintf(stderr,
                RED "systray %d: cannot embed icon for window %lu (%s) parent %lu pid %d" RESET "\n",
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
                __func__,
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
                __func__,
                __LINE__,
                traywin->win,
                traywin->name);
    if (traywin->embedded)
        return TRUE;

    Panel *panel = systray.area.panel;

    XSync(server.display, False);
    error = FALSE;
    XErrorHandler old = XSetErrorHandler(window_error_handler);

    // Redirect rendering when using compositing
    if (systray_composited) {
        if (systray_profile)
            fprintf(stderr, "tint2: XDamageCreate(server.display, traywin->parent, XDamageReportRawRectangles)\n");
        traywin->damage = XDamageCreate(server.display, traywin->parent, XDamageReportRawRectangles);
        if (systray_profile)
            fprintf(stderr, "tint2: XCompositeRedirectWindow(server.display, traywin->parent, CompositeRedirectManual)\n");
        XCompositeRedirectWindow(server.display, traywin->parent, CompositeRedirectManual);
    }

    XRaiseWindow(server.display, traywin->win);

    // Make the icon visible
    if (systray_profile)
        fprintf(stderr, "tint2: XMapWindow(server.display, traywin->win)\n");
    XMapWindow(server.display, traywin->win);
    if (!panel->is_hidden) {
        if (systray_profile)
            fprintf(stderr, "tint2: XMapRaised(server.display, traywin->parent)\n");
        XMapRaised(server.display, traywin->parent);
    }

    if (systray_profile)
        fprintf(stderr, "tint2: XSync(server.display, False)\n");
    XSync(server.display, False);
    XSetErrorHandler(old);
    if (error != FALSE) {
        fprintf(stderr,
                RED "systray %d: cannot embed icon for window %lu (%s) parent %lu pid %d" RESET "\n",
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
                __func__,
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
                __func__,
                __LINE__,
                traywin->win,
                traywin->name);
    Panel *panel = systray.area.panel;

    // remove from our list
    systray.list_icons = g_slist_remove(systray.list_icons, traywin);
    fprintf(stderr, YELLOW "tint2: remove_icon: %lu (%s)" RESET "\n", traywin->win, traywin->name);

    XSelectInput(server.display, traywin->win, NoEventMask);
    if (traywin->damage)
        XDamageDestroy(server.display, traywin->damage);

    // reparent to root
    XSync(server.display, False);
    error = FALSE;
    XErrorHandler old = XSetErrorHandler(window_error_handler);
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
        count++;
    }
    if (count == 0)
        hide(&systray.area);

    // Resize and redraw the systray
    if (systray_profile)
        fprintf(stderr,
                BLUE "[%f] %s:%d trigger resize & redraw" RESET "\n",
                profiling_get_time(),
                __func__,
                __LINE__);
    systray.area.resize_needed = TRUE;
    panel->area.resize_needed = TRUE;
    schedule_redraw(&systray.area);
    refresh_systray = TRUE;
}

void systray_resize_icon(void *t)
{
    TrayWindow *traywin = t;

    unsigned int border_width;
    int xpos, ypos;
    unsigned int width, height, depth;
    Window root;
    if (!XGetGeometry(server.display, traywin->win, &root, &xpos, &ypos, &width, &height, &border_width, &depth)) {
        return;
    } else {
        if (systray_profile)
            fprintf(stderr,
                    "systray_resize_icon win = %ld, w = %d, h = %d\n",
                    traywin->win,
                    traywin->width,
                    traywin->height);
        // This is the obvious thing to do but GTK tray icons do not respect the new size
        if (0) {
            XMoveResizeWindow(server.display, traywin->win, 0, 0, traywin->width, traywin->height);
        }
        // This is similar but GTK tray icons still do not respect the new size
        if (0) {
            XWindowChanges changes;
            changes.x = changes.y = 0;
            changes.width = traywin->width;
            changes.height = traywin->height;
            XConfigureWindow(server.display, traywin->win, CWX | CWY | CWWidth | CWHeight, &changes);
        }
        // This is what WMs send to windows to resize them, the new size must not be ignored.
        // A bit brutal but works with GTK and everything else.
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
                        RED "Detected resize loop for tray icon %lu (%s), throttling resize events" RESET "\n",
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
        fprintf(stderr,
                BLUE "[%f] %s:%d trigger resize & redraw" RESET "\n",
                profiling_get_time(),
                __func__,
                __LINE__);
    schedule_panel_redraw();
    refresh_systray = TRUE;
}

void systray_property_notify(TrayWindow *traywin, XEvent *e)
{
    Atom at = e->xproperty.atom;
    if (at == server.atom.WM_NAME) {
        free(traywin->name);
        traywin->name = get_window_name(traywin->win);
        if (systray.sort == SYSTRAY_SORT_ASCENDING || systray.sort == SYSTRAY_SORT_DESCENDING) {
            systray.list_icons = g_slist_sort(systray.list_icons, compare_traywindows);
            // print_icons();
        }
    }
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
                        RED "Detected resize loop for tray icon %lu (%s), throttling resize events" RESET "\n",
                        traywin->win,
                        traywin->name);
            }
            // Delayed resize
            // FIXME Normally we should force the icon to resize to the size we resized it to when we embedded it.
            // However this triggers a resize loop in some versions of GTK, which we must avoid.
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
        fprintf(stderr,
                BLUE "[%f] %s:%d trigger resize & redraw" RESET "\n",
                profiling_get_time(),
                __func__,
                __LINE__);
    schedule_panel_redraw();
    refresh_systray = TRUE;
}

void systray_destroy_event(TrayWindow *traywin)
{
    if (systray_profile)
        fprintf(stderr,
                "[%f] %s:%d win = %lu (%s)\n",
                profiling_get_time(),
                __func__,
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
                __func__,
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
                        YELLOW "[%f] %s:%d win = %lu (%s) delaying rendering" RESET "\n",
                        profiling_get_time(),
                        __func__,
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
                    YELLOW "[%f] %s:%d win = %lu (%s) delaying rendering" RESET "\n",
                    profiling_get_time(),
                    __func__,
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
        fprintf(stderr, RED "tint2: Strange tray icon found with depth: %d" RESET "\n", traywin->depth);
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

    if (systray.alpha != 100 || systray.brightness != 0 || systray.saturation != 0)
        adjust_asb(data,
                   traywin->width,
                   traywin->height,
                   systray.alpha / 100.0,
                   systray.saturation / 100.0,
                   systray.brightness / 100.0);
    imlib_image_put_back_data(data);

    systray_render_icon_from_image(traywin);

    if (traywin->damage)
        XDamageSubtract(server.display, traywin->damage, None, None);
    XSync(server.display, False);
    XSetErrorHandler(old);

    if (error)
        goto on_error;

    schedule_panel_redraw();

    if (systray_profile)
        fprintf(stderr,
                "[%f] %s:%d win = %lu (%s)\n",
                profiling_get_time(),
                __func__,
                __LINE__,
                traywin->win,
                traywin->name);

    return;

on_error:
    fprintf(stderr,
            RED "systray %d: rendering error for icon %lu (%s) pid %d" RESET "\n",
            __LINE__,
            traywin->win,
            traywin->name,
            traywin->pid);
    return;

on_systray_error:
    fprintf(stderr,
            RED "systray %d: rendering error for icon %lu (%s) pid %d. "
                "Disabling compositing and restarting systray..." RESET "\n",
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
    if (!traywin->reparented || !traywin->embedded) {
        //		if (systray_profile)
        //			fprintf(stderr,
        //			        YELLOW "[%f] %s:%d win = %lu (%s) delaying rendering" RESET "\n",
        //			        profiling_get_time(),
        //			        __func__,
        //			        __LINE__,
        //			        traywin->win,
        //			        traywin->name);
        stop_timeout(traywin->render_timeout);
        traywin->render_timeout =
            add_timeout(min_refresh_period, 0, systray_render_icon, traywin, &traywin->render_timeout);
        return;
    }

    if (systray_profile)
        fprintf(stderr,
                "[%f] %s:%d win = %lu (%s)\n",
                profiling_get_time(),
                __func__,
                __LINE__,
                traywin->win,
                traywin->name);

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
            if (!traywin->render_timeout)
                traywin->render_timeout =
                    add_timeout(min_refresh_period, 0, systray_render_icon, traywin, &traywin->render_timeout);
            systray_render_icon_from_image(traywin);
            XSetErrorHandler(old);
            return;
        } else {
            if (xpos != 0 || ypos != 0 || width != traywin->width || height != traywin->height) {
                stop_timeout(traywin->render_timeout);
                if (!traywin->render_timeout)
                    traywin->render_timeout =
                        add_timeout(min_refresh_period, 0, systray_render_icon, traywin, &traywin->render_timeout);
                systray_render_icon_from_image(traywin);
                if (systray_profile)
                    fprintf(stderr,
                            YELLOW "[%f] %s:%d win = %lu (%s) delaying rendering" RESET "\n",
                            profiling_get_time(),
                            __func__,
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
        fprintf(stderr, "tint2: rendering tray icon\n");

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
        fprintf(stderr, BLUE "tint2: [%f] %s:%d" RESET "\n", profiling_get_time(), __func__, __LINE__);
    TrayWindow *traywin;
    GSList *l;
    for (l = systray.list_icons; l; l = l->next) {
        traywin = (TrayWindow *)l->data;
        systray_render_icon(traywin);
    }
}

gboolean systray_on_monitor(int i_monitor, int n_panels)
{
    return (i_monitor == systray_monitor) || (i_monitor == 0 && (systray_monitor >= n_panels || systray_monitor < 0));
}

TrayWindow *systray_find_icon(Window win)
{
    for (GSList *l = systray.list_icons; l; l = l->next) {
        TrayWindow *traywin = (TrayWindow *)l->data;
        if (traywin->win == win || traywin->parent == win)
            return traywin;
    }
    return NULL;
}
