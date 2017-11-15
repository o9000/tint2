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

#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

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
            if (get_task_buttons(window)) {
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
    int desktop = get_property32(win, server.atom._NET_WM_DESKTOP, XA_CARDINAL);
    if (desktop == ALL_DESKTOPS)
        return desktop;
    if (!server.viewports)
        return CLAMP(desktop, 0, server.num_desktops - 1);

    int x, y, w, h;
    get_window_coordinates(win, &x, &y, &w, &h);

    desktop = get_current_desktop();
    // Window coordinates are relative to the current viewport, make them absolute
    x += server.viewports[desktop].x;
    y += server.viewports[desktop].y;

    if (x < 0 || y < 0) {
        int num_results;
        long *x_screen_size =
            server_get_property(server.root_win, server.atom._NET_DESKTOP_GEOMETRY, XA_CARDINAL, &num_results);
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
    // fprintf(stderr, "tint2: window %lx %s : viewport %d, (%d, %d)\n", win, get_task(win) ? get_task(win)->title :
    // "??",
    // best_match+1, x, y);
    return best_match;
}

int get_window_monitor(Window win)
{
    int x, y, w, h;
    get_window_coordinates(win, &x, &y, &w, &h);

    int best_match = -1;
    int match_right = 0;
    int match_bottom = 0;
    // There is an ambiguity when a window is right on the edge between screens.
    // In that case, prefer the monitor which is on the right and bottom of the window's top-left corner.
    for (int i = 0; i < server.num_monitors; i++) {
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
    // fprintf(stderr, "tint2: desktop %d, window %lx %s : monitor %d, (%d, %d)\n", 1 + get_current_desktop(), win,
    // get_task(win) ? get_task(win)->title : "??", best_match+1, x, y);
    return best_match;
}

gboolean get_window_coordinates(Window win, int *x, int *y, int *w, int *h)
{
    int dummy_int;
    unsigned ww, wh, bw, bh;
    Window src;
    if (!XTranslateCoordinates(server.display, win, server.root_win, 0, 0, x, y, &src))
        return FALSE;
    if (!XGetGeometry(server.display, win, &src, &dummy_int, &dummy_int, &ww, &wh, &bw, &bh))
        return FALSE;
    *w = ww + bw;
    *h = wh + bh;
    return TRUE;
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
    if (icon_count < 1 || num < 1)
        return NULL;

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

// Thanks zcodes!
char *get_window_name(Window win)
{
    XTextProperty text_property;
    Status status = XGetWMName(server.display, win, &text_property);
    if (!status || !text_property.value || !text_property.nitems) {
        return strdup("");
    }

    char **name_list;
    int count;
    status = Xutf8TextPropertyToTextList(server.display, &text_property, &name_list, &count);
    if (status < Success || !count) {
        XFree(text_property.value);
        return strdup("");
    }

    if (!name_list[0]) {
        XFreeStringList(name_list);
        XFree(text_property.value);
        return strdup("");
    }

    char *result = strdup(name_list[0]);
    XFreeStringList(name_list);
    XFree(text_property.value);
    return result;
}

void smooth_thumbnail(cairo_surface_t *image_surface)
{
    u_int32_t *data = (u_int32_t *)cairo_image_surface_get_data(image_surface);
    const size_t tw = cairo_image_surface_get_width(image_surface);
    const size_t th = cairo_image_surface_get_height(image_surface);
    const size_t rmask = 0xff0000;
    const size_t gmask = 0xff00;
    const size_t bmask = 0xff;
    for (size_t i = 0; i < tw * (th - 1) - 1; i++) {
        u_int32_t c1 = data[i];
        u_int32_t c2 = data[i+1];
        u_int32_t c3 = data[i+tw];
        u_int32_t b = (6 * (c1 & bmask) + (c2 & bmask) + (c3 & bmask)) / 8;
        u_int32_t g = (6 * (c1 & gmask) + (c2 & gmask) + (c3 & gmask)) / 8;
        u_int32_t r = (6 * (c1 & rmask) + (c2 & rmask) + (c3 & rmask)) / 8;
        data[i] = (r & rmask) | (g & gmask) | (b & bmask);
    }
}

cairo_surface_t *screenshot(Window win, size_t size)
{
    cairo_surface_t *result = NULL;
    XWindowAttributes wa;
    if (!XGetWindowAttributes(server.display, win, &wa) || wa.width <= 0 || wa.height <= 0)
        goto err0;

    size_t w, h;
    w = (size_t)wa.width;
    h = (size_t)wa.height;
    size_t tw, th, fw;
    size_t ox, oy;
    tw = size;
    th = h * tw / w;
    if (th > tw * 0.618) {
        th = (size_t)(tw * 0.618);
        fw = w * th / h;
        ox = (tw - fw) / 2;
        oy = 0;
    } else {
        fw = tw;
        ox = oy = 0;
    }

    XShmSegmentInfo shminfo;
    XImage *ximg = XShmCreateImage(server.display,
                                   wa.visual,
                                   (unsigned)wa.depth,
                                   ZPixmap,
                                   NULL,
                                   &shminfo,
                                   (unsigned)wa.width,
                                   (unsigned)wa.height);
    if (!ximg) {
        fprintf(stderr, RED "tint2: !ximg" RESET "\n");
        goto err0;
    }
    shminfo.shmid = shmget(IPC_PRIVATE, (size_t)(ximg->bytes_per_line * ximg->height), IPC_CREAT | 0777);
    if (shminfo.shmid < 0) {
        fprintf(stderr, RED "tint2: !shmget" RESET "\n");
        goto err1;
    }
    shminfo.shmaddr = ximg->data = (char *)shmat(shminfo.shmid, 0, 0);
    if (!shminfo.shmaddr) {
        fprintf(stderr, RED "tint2: !shmat" RESET "\n");
        goto err2;
    }
    shminfo.readOnly = False;
    if (!XShmAttach(server.display, &shminfo)) {
        fprintf(stderr, RED "tint2: !xshmattach" RESET "\n");
        goto err3;
    }
    if (!XShmGetImage(server.display, win, ximg, 0, 0, AllPlanes)) {
        fprintf(stderr, RED "tint2: !xshmgetimage" RESET "\n");
        goto err4;
    }

    result = cairo_image_surface_create(CAIRO_FORMAT_RGB24, (int)tw, (int)th);
    u_int32_t *data = (u_int32_t *)cairo_image_surface_get_data(result);
    memset(data, 0, tw * th);

    const size_t xstep = w / fw;
    const size_t ystep = h / th;
    const size_t offset_x1 = xstep / 4;
    const size_t offset_y1 = ystep / 4;
    const size_t offset_x2 = 3 * xstep / 4;
    const size_t offset_y2 = ystep / 4;
    const size_t offset_x3 = xstep / 4;
    const size_t offset_y3 = 3 * ystep / 4;
    const size_t offset_x4 = 3 * xstep / 4;
    const size_t offset_y4 = ystep / 4;
    const size_t offset_x5 = xstep / 2;
    const size_t offset_y5 = ystep / 2;
    const size_t offset_x6 = 5 * xstep / 8;
    const size_t offset_y6 = 3 * ystep / 16;
    const size_t offset_x7 = 3 * xstep / 16;
    const size_t offset_y7 = 5 * ystep / 8;
    const size_t rmask = 0xff0000;
    const size_t gmask = 0xff00;
    const size_t bmask = 0xff;
    for (size_t yt = 0, y = 0; yt < th; yt++, y += ystep) {
        for (size_t xt = 0, x = 0; xt < fw; xt++, x += xstep) {
            size_t j = yt * tw + ox + xt;
            u_int32_t c1 = (u_int32_t)XGetPixel(ximg, (int)(x + offset_x1), (int)(y + offset_y1));
            u_int32_t c2 = (u_int32_t)XGetPixel(ximg, (int)(x + offset_x2), (int)(y + offset_y2));
            u_int32_t c3 = (u_int32_t)XGetPixel(ximg, (int)(x + offset_x3), (int)(y + offset_y3));
            u_int32_t c4 = (u_int32_t)XGetPixel(ximg, (int)(x + offset_x4), (int)(y + offset_y4));
            u_int32_t c5 = (u_int32_t)XGetPixel(ximg, (int)(x + offset_x5), (int)(y + offset_y5));
            u_int32_t c6 = (u_int32_t)XGetPixel(ximg, (int)(x + offset_x6), (int)(y + offset_y6));
            u_int32_t c7 = (u_int32_t)XGetPixel(ximg, (int)(x + offset_x7), (int)(y + offset_y7));
            u_int32_t b = ((c1 & bmask) + (c2 & bmask) + (c3 & bmask) + (c4 & bmask) + (c5 & bmask) * 2 + (c6 & bmask) +
                           (c7 & bmask)) /
                          8;
            u_int32_t g = ((c1 & gmask) + (c2 & gmask) + (c3 & gmask) + (c4 & gmask) + (c5 & gmask) * 2 + (c6 & gmask) +
                           (c7 & gmask)) /
                          8;
            u_int32_t r = ((c1 & rmask) + (c2 & rmask) + (c3 & rmask) + (c4 & rmask) + (c5 & rmask) * 2 + (c6 & rmask) +
                           (c7 & rmask)) /
                          8;
            data[j] = (r & rmask) | (g & gmask) | (b & bmask);
        }
    }

    // 2nd pass
    smooth_thumbnail(result);

err4:
    XShmDetach(server.display, &shminfo);
err3:
    shmdt(shminfo.shmaddr);
err2:
    shmctl(shminfo.shmid, IPC_RMID, NULL);
err1:
    XDestroyImage(ximg);
err0:
    return result;
}

cairo_surface_t *get_window_thumbnail_cairo(Window win, int size)
{
    static cairo_filter_t filter = CAIRO_FILTER_BEST;
    XWindowAttributes wa;
    if (!XGetWindowAttributes(server.display, win, &wa))
        return NULL;
    int w, h;
    w = wa.width;
    h = wa.height;

    int tw, th;
    double sx, sy;
    double ox, oy;
    tw = size;
    th = h * tw / w;
    if (th > tw * 0.618) {
        th = (int)(tw * 0.618);
        sy = th / (double)h;
        double fw = w * th / h;
        sx = fw / w;
        ox = (tw - fw) / 2;
        oy = 0;
    } else {
        sx = tw / (double)w;
        sy = th / (double)h;
        ox = oy = 0;
    }

    cairo_surface_t *x11_surface = cairo_xlib_surface_create(server.display, win, wa.visual, w, h);

    cairo_surface_t *full_surface = cairo_surface_create_similar_image(x11_surface, CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr0 = cairo_create(full_surface);
    cairo_set_source_surface(cr0, x11_surface, 0, 0);
    cairo_paint(cr0);
    cairo_destroy(cr0);

    cairo_surface_t *image_surface = cairo_surface_create_similar_image(full_surface, CAIRO_FORMAT_ARGB32, tw, th);

    double start_time = get_time();
    cairo_t *cr = cairo_create(image_surface);
    cairo_translate(cr, ox, oy);
    cairo_scale(cr, sx, sy);
    cairo_set_source_surface(cr, full_surface, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(cr), filter);
    cairo_paint(cr);
    cairo_destroy(cr);
    cairo_surface_destroy(x11_surface);
    if (filter == CAIRO_FILTER_FAST)
        smooth_thumbnail(image_surface);
    double end_time = get_time();

    if (end_time - start_time > 0.020)
        filter = CAIRO_FILTER_FAST;
    else if (end_time - start_time < 0.010)
        filter = CAIRO_FILTER_BEST;

    return image_surface;
}

gboolean cairo_surface_is_blank(cairo_surface_t *image_surface)
{
    uint32_t *pixels = (uint32_t *)cairo_image_surface_get_data(image_surface);
    gboolean empty = TRUE;
    int size = cairo_image_surface_get_width(image_surface) * cairo_image_surface_get_height(image_surface);
    for (int i = 0; empty && i < size; i++) {
        if (pixels[i] & 0xffFFff)
            empty = FALSE;
    }
    return empty;
}

cairo_surface_t *get_window_thumbnail(Window win, int size)
{
    cairo_surface_t *image_surface = NULL;
    if (server.has_shm && server.composite_manager) {
        image_surface = screenshot(win, (size_t)size);
        if (image_surface && cairo_surface_is_blank(image_surface)) {
            cairo_surface_destroy(image_surface);
            image_surface = NULL;
        }
        if (!image_surface)
            fprintf(stderr, YELLOW "tint2: thumbnail fast path failed, trying slow path" RESET "\n");
    }

    if (!image_surface) {
        image_surface = get_window_thumbnail_cairo(win, size);
        if (image_surface && cairo_surface_is_blank(image_surface)) {
            cairo_surface_destroy(image_surface);
            image_surface = NULL;
        }
        if (!image_surface)
            fprintf(stderr, YELLOW "tint2: thumbnail slow path failed" RESET "\n");
    }


    if (!image_surface)
        return NULL;

    return image_surface;
}
