/**************************************************************************
* Tint2 : systraybar
*
* Copyright (C) 2009 thierry lorthiois (lorthiois@bbsoft.fr)
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
Window net_sel_win = None;

// freedesktop specification doesn't allow multi systray
Systraybar systray;


void init_systray()
{
	cleanup_systray();

	Panel *panel = &panel1[0];
	systray.area.parent = panel;
	systray.area.panel = panel;
	systray.area._draw_foreground = draw_systray;
	systray.area._resize = resize_systray;

	if (systray.area.on_screen) {
		if (XGetSelectionOwner(server.dsp, server.atom._NET_SYSTEM_TRAY_SCREEN) != None) {
			fprintf(stderr, "tint2 : another systray is running\n");
			systray.area.on_screen = 0;
		}
	}

	if (systray.area.on_screen)
		systray.area.on_screen = net_init();

	if (!systray.area.on_screen)
		return;

	// configure systray
	// draw only one systray (even with multi panel)
	systray.area.posy = panel->area.pix.border.width + panel->area.paddingy;
	systray.area.height = panel->area.height - (2 * systray.area.posy);
	systray.area.width = 0;

	systray.area.posx = panel->area.width - panel->area.paddingxlr - panel->area.pix.border.width - systray.area.width;
	if (panel->clock.area.on_screen)
		systray.area.posx -= (panel->clock.area.width + panel->area.paddingx);

	systray.area.redraw = 1;
}


void cleanup_systray()
{
	free_area(&systray.area);

	if (net_sel_win != None) {
  		XDestroyWindow(server.dsp, net_sel_win);
  		net_sel_win = None;
	}
   if (systray.list_icons) {
      g_slist_free(systray.list_icons);
      systray.list_icons = 0;
   }
}


void draw_systray(void *obj, cairo_t *c, int active)
{
	Systraybar *sysbar = obj;
	Panel *panel = sysbar->area.panel;
	TrayWindow *traywin;
	GSList *l;
	int icon_size;

	icon_size = sysbar->area.height - (2 * sysbar->area.pix.border.width) - (2 * sysbar->area.paddingy);
	for (l = systray.list_icons; l ; l = l->next) {
		traywin = (TrayWindow*)l->data;

		printf("draw_systray %d %d\n", systray.area.posx, systray.area.width);
		// watch for the icon trying to resize itself!
		XSelectInput(server.dsp, traywin->id, StructureNotifyMask);

		// position and size the icon window
		XMoveResizeWindow(server.dsp, traywin->id, traywin->x, traywin->y, icon_size, icon_size);

		// resize our window so that the new window can fit in it
		//fix_geometry();

		// flush before clearing, otherwise the clear isn't effective.
		XFlush(server.dsp);
		// make sure the new child will get the right stuff in its background
		// for ParentRelative.
		XClearWindow(server.dsp, panel->main_win);

		// show the window
		XMapRaised(server.dsp, traywin->id);
	}
}


void resize_systray(void *obj)
{
	Systraybar *sysbar = obj;
	Panel *panel = sysbar->area.panel;
	TrayWindow *traywin;
	GSList *l;
	int count, posx, posy;
	int icon_size;

	icon_size = sysbar->area.height - (2 * sysbar->area.pix.border.width) - (2 * sysbar->area.paddingy);
	count = g_slist_length(systray.list_icons);

	if (!count) systray.area.width = 0;
	else systray.area.width = (2 * systray.area.pix.border.width) + (2 * systray.area.paddingxlr) + (icon_size * count) + ((count-1) * systray.area.paddingx);

	systray.area.posx = panel->area.width - panel->area.pix.border.width - panel->area.paddingxlr - systray.area.width;
	if (panel->clock.area.on_screen)
		systray.area.posx -= (panel->clock.area.width + panel->area.paddingx);

	systray.area.redraw = 1;

	posy = panel->area.pix.border.width + panel->area.paddingy + systray.area.pix.border.width + systray.area.paddingy;
	posx = systray.area.posx + systray.area.pix.border.width + systray.area.paddingxlr;
	for (l = systray.list_icons; l ; l = l->next) {
		traywin = (TrayWindow*)l->data;

		traywin->y = posy;
		traywin->x = posx;
		posx += (icon_size + systray.area.paddingx);
	}

	// resize other objects on panel
	printf("resize_systray %d %d\n", systray.area.posx, systray.area.width);
}


int net_init()
{
	// init systray protocol
   net_sel_win = XCreateSimpleWindow(server.dsp, server.root_win, -1, -1, 1, 1, 0, 0, 0);

	// v0.2 trayer specification. tint2 always orizontal.
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


//Window win, root;
int width, height;
int border;
int icon_size;


void fix_geometry()
{
  GSList *it;
  Panel *panel = systray.area.panel;

  // find the proper width and height
  width = 0;
  height = icon_size;
  for (it = icons; it != NULL; it = g_slist_next(it)) {
    width += icon_size;
  }

  XResizeWindow(server.dsp, panel->main_win, width + border * 2, height + border * 2);
}


gboolean error;
int window_error_handler(Display *d, XErrorEvent *e)
{
  d=d;e=e;
  if (e->error_code == BadWindow) {
    error = TRUE;
  } else {
    //g_printerr("X ERROR NOT BAD WINDOW!\n");
    abort();
  }
  return 0;
}


gboolean icon_swallow(TrayWindow *traywin)
{
  XErrorHandler old;
  Panel *panel = systray.area.panel;

  error = FALSE;
  old = XSetErrorHandler(window_error_handler);
  XReparentWindow(server.dsp, traywin->id, panel->main_win, 0, 0);
  XSync(server.dsp, False);
  XSetErrorHandler(old);

  return !error;
}


// The traywin must have its id and type set.
gboolean icon_add(Window id)
{
	TrayWindow *traywin;

	traywin = g_new0(TrayWindow, 1);
	traywin->id = id;

	systray.list_icons = g_slist_prepend(systray.list_icons, traywin);
  	printf("ajout d'un icone %d (%lx)\n", g_slist_length(systray.list_icons), id);
  	systray.area.resize = 1;
  	systray.area.redraw = 1;

	// changed in systray force resize on panel
	Panel *panel = systray.area.panel;
	panel->area.resize = 1;
	panel_refresh = 1;

	if (!icon_swallow(traywin)) {
		printf("not icon_swallow\n");
		g_free(traywin);
		return FALSE;
	}
	else
		printf("icon_swallow\n");
	//return TRUE;

// => calcul x, y, width, height dans resize
/*
	// find the positon for the systray app window
	int count = g_slist_length(icons);
	traywin->x = border + ((width % icon_size) / 2) +
	(count % (width / icon_size)) * icon_size;
	traywin->y = border + ((height % icon_size) / 2) +
	(count / (height / icon_size)) * icon_size;

	// add the new icon to the list
	icons = g_slist_append(icons, traywin);
*/

	return TRUE;
}


void icon_remove(TrayWindow *traywin)
{
	XErrorHandler old;
	Window win_id = traywin->id;

	XSelectInput(server.dsp, traywin->id, NoEventMask);

	// remove it from our list
	systray.list_icons = g_slist_remove(systray.list_icons, traywin);
	g_free(traywin);
	printf("suppression d'un icone %d\n", g_slist_length(systray.list_icons));
  	systray.area.resize = 1;

	// changed in systray force resize on panel
	Panel *panel = systray.area.panel;
	panel->area.resize = 1;
	panel_refresh = 1;
	return;

/*
	// reparent it to root
	error = FALSE;
	old = XSetErrorHandler(window_error_handler);
	XReparentWindow(server.dsp, win_id, root, 0, 0);
	XSync(server.dsp, False);
	XSetErrorHandler(old);

	reposition_icons();
	fix_geometry();
	*/
}


void net_message(XClientMessageEvent *e)
{
	unsigned long opcode;
	Window id;

	opcode = e->data.l[1];

	switch (opcode) {
		case SYSTEM_TRAY_REQUEST_DOCK:
			id = e->data.l[2];
			if (id) icon_add(id);
			break;

		case SYSTEM_TRAY_BEGIN_MESSAGE:
			printf("message from dockapp\n");
			id = e->window;
			break;

		case SYSTEM_TRAY_CANCEL_MESSAGE:
			printf("message cancelled\n");
			id = e->window;
			break;

		default:
			if (opcode == server.atom._NET_SYSTEM_TRAY_MESSAGE_DATA) {
				printf("message from dockapp:\n  %s\n", e->data.b);
				id = e->window;
			}
			// unknown message type. not in the spec
			break;
	}
}


/*
void event_loop()
{
  XEvent e;
  Window cover;
  GSList *it;

  while (!exit_app) {
    while (XPending(server.dsp)) {
      XNextEvent(display, &e);

      switch (e.type)
      {
      case PropertyNotify:
        // systray window list has changed?
        if (e.xproperty.atom == kde_systray_prop) {
          XSelectInput(display, win, NoEventMask);
          kde_update_icons();
          XSelectInput(display, win, StructureNotifyMask);

          while (XCheckTypedEvent(display, PropertyNotify, &e));
        }

        break;

      case ConfigureNotify:
        if (e.xany.window != win) {
          // find the icon it pertains to and beat it into submission
          GSList *it;

          for (it = icons; it != NULL; it = g_slist_next(it)) {
            TrayWindow *traywin = it->data;
            if (traywin->id == e.xany.window) {
              XMoveResizeWindow(display, traywin->id, traywin->x, traywin->y,
                                icon_size, icon_size);
              break;
            }
          }
          break;
        }

        // briefly cover the entire containing window, which causes it and
        // all of the icons to refresh their windows. finally, they update
        // themselves when the background of the main window's parent changes.

        cover = XCreateSimpleWindow(display, win, 0, 0,
                                    border * 2 + width, border * 2 + height,
                                    0, 0, 0);
        XMapWindow(display, cover);
        XDestroyWindow(display, cover);

        break;

      case ReparentNotify:
        if (e.xany.window == win) // reparented to us
          break;
      case UnmapNotify:
      case DestroyNotify:
        for (it = icons; it; it = g_slist_next(it)) {
          if (((TrayWindow*)it->data)->id == e.xany.window) {
            icon_remove(it);
            break;
          }
        }
        break;

      case ClientMessage:
        if (e.xclient.message_type == net_opcode_atom &&
            e.xclient.format == 32 &&
            e.xclient.window == net_sel_win)
          net_message(&e.xclient);

      default:
        break;
      }
    }
    usleep(500000);
  }

  // remove/unparent all the icons
  while (icons) {
    // do the remove here explicitly, cuz the event handler isn't going to
    // happen anymore.
    icon_remove(icons);
  }
}
*/

