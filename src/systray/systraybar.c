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

Window net_sel_win = None;


void init_systray()
{
   Panel *panel;
   Systraybar *sysbar;
   int i, run_systray;

	cleanup_systray();

	run_systray = 0;
	for (i=0 ; i < nb_panel ; i++) {
	   if (panel1[i].systray.area.visible) {
	   	run_systray = 1;
	   	break;
		}
	}
	if (run_systray) {
		if (XGetSelectionOwner(server.dsp, server.atom._NET_SYSTEM_TRAY_SCREEN) != None) {
			fprintf(stderr, "tint2 warning : another systray is running\n");
			run_systray = 0;
		}
	}

	if (run_systray)
		run_systray = net_init();

	for (i=0 ; i < nb_panel ; i++) {
	   panel = &panel1[i];
	   sysbar = &panel->systray;

	   if (!run_systray) {
	   	sysbar->area.visible = 0;
	   	continue;
		}
	   if (!sysbar->area.visible)
	   	continue;

		sysbar->area.parent = panel;
		sysbar->area.panel = panel;

		sysbar->area.posy = panel->area.pix.border.width + panel->area.paddingy;
		sysbar->area.height = panel->area.height - (2 * sysbar->area.posy);
		sysbar->area.width = 100;

		sysbar->area.posx = panel->area.width - panel->area.paddingxlr - panel->area.pix.border.width - sysbar->area.width;
	   if (panel->clock.area.visible)
	   	sysbar->area.posx -= (panel->clock.area.width + panel->area.paddingx);

		sysbar->area.redraw = 1;
	}
}


void cleanup_systray()
{
   Panel *panel;
   int i;

	for (i=0 ; i < nb_panel ; i++) {
		panel = &panel1[i];
	   if (!panel->systray.area.visible) continue;

		free_area(&panel->systray.area);
	}

	if (net_sel_win != None) {
  		XDestroyWindow(server.dsp, net_sel_win);
  		net_sel_win = None;
	}
}


int resize_systray (Systraybar *sysbar)
{
   return 0;
}



Window win, root;
int width, height;
int border;
int icon_size;


void fix_geometry()
{
  GSList *it;

  //* find the proper width and height
  width = 0;
  height = icon_size;
  for (it = icons; it != NULL; it = g_slist_next(it)) {
    width += icon_size;
  }

  XResizeWindow(server.dsp, win, width + border * 2, height + border * 2);
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

  error = FALSE;
  old = XSetErrorHandler(window_error_handler);
  XReparentWindow(server.dsp, traywin->id, win, 0, 0);
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

  if (!icon_swallow(traywin)) {
    g_free(traywin);
    return FALSE;
  }

  // find the positon for the systray app window
	int count = g_slist_length(icons);
	traywin->x = border + ((width % icon_size) / 2) +
	(count % (width / icon_size)) * icon_size;
	traywin->y = border + ((height % icon_size) / 2) +
	(count / (height / icon_size)) * icon_size;

  // add the new icon to the list
  icons = g_slist_append(icons, traywin);

  // watch for the icon trying to resize itself!
  XSelectInput(server.dsp, traywin->id, StructureNotifyMask);

  // position and size the icon window
  XMoveResizeWindow(server.dsp, traywin->id, traywin->x, traywin->y, icon_size, icon_size);

  // resize our window so that the new window can fit in it
  fix_geometry();

  // flush before clearing, otherwise the clear isn't effective.
  XFlush(server.dsp);
  // make sure the new child will get the right stuff in its background
  // for ParentRelative.
  XClearWindow(server.dsp, win);

  // show the window
  XMapRaised(server.dsp, traywin->id);

  return TRUE;
}


int net_init()
{
	// init systray protocol
   net_sel_win = XCreateSimpleWindow(server.dsp, server.root_win, -1, -1, 1, 1, 0, 0, 0);

	XSetSelectionOwner(server.dsp, server.atom._NET_SYSTEM_TRAY_SCREEN, net_sel_win, CurrentTime);
	if (XGetSelectionOwner(server.dsp, server.atom._NET_SYSTEM_TRAY_SCREEN) != net_sel_win) {
		fprintf(stderr, "tint2 warning : can't get systray manager\n");
		return 0;
 	}

	XEvent m;
	m.type = ClientMessage;
	m.xclient.message_type = server.atom.MANAGER;
	m.xclient.format = 32;
	m.xclient.data.l[0] = CurrentTime;
	m.xclient.data.l[1] = server.atom._NET_SYSTEM_TRAY_SCREEN;
	m.xclient.data.l[2] = net_sel_win;
	m.xclient.data.l[3] = 0;
	m.xclient.data.l[4] = 0;
	XSendEvent(server.dsp, server.root_win, False, StructureNotifyMask, &m);
	return 1;
}


void net_message(XClientMessageEvent *e)
{
  unsigned long opcode;
  Window id;

  opcode = e->data.l[1];

  switch (opcode)
  {
  case SYSTEM_TRAY_REQUEST_DOCK: /* dock a new icon */
    id = e->data.l[2];
    if (id && icon_add(id))
      XSelectInput(server.dsp, id, StructureNotifyMask);
    break;

  case SYSTEM_TRAY_BEGIN_MESSAGE:
    //g_printerr("Message From Dockapp\n");
    id = e->window;
    break;

  case SYSTEM_TRAY_CANCEL_MESSAGE:
    //g_printerr("Message Cancelled\n");
    id = e->window;
    break;

  default:
    if (opcode == server.atom._NET_SYSTEM_TRAY_MESSAGE_DATA) {
      //g_printerr("Text For Message From Dockapp:\n%s\n", e->data.b);
      id = e->window;
      break;
    }

    /* unknown message type. not in the spec. */
    //g_printerr("Warning: Received unknown client message to System Tray selection window.\n");
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

