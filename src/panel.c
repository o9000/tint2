/**************************************************************************
*
* Copyright (C) 2008 Pål Staurland (staura@gmail.com)
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <pango/pangocairo.h>

#include "server.h"
#include "window.h"
#include "task.h"
#include "panel.h"


void visual_refresh ()
{
   if (!panel.area.pmap)
      set_panel_background(); 
   
   if (server.pmap) XFreePixmap (server.dsp, server.pmap);
   server.pmap = server_create_pixmap (panel.area.width, panel.area.height);
   XCopyArea (server.dsp, panel.area.pmap, server.pmap, server.gc, 0, 0, panel.area.width, panel.area.height, 0, 0);

   // draw child object
   GSList *l = panel.area.list;
   for (; l ; l = l->next)
      refresh (l->data);

   // main_win doesn't include panel.area.paddingx, so we have WM capabilities on left and right.
   XCopyArea (server.dsp, server.pmap, window.main_win, server.gc, panel.area.paddingx, 0, panel.area.width-(2*panel.area.paddingx), panel.area.height, 0, 0);
   XFlush (server.dsp);
   panel.refresh = 0;
}


void set_panel_properties (Window win)
{
   XStoreName (server.dsp, win, "tint2");

   // TODO: check if the name is really needed for a panel/taskbar ?
   gsize len;
   gchar *name = g_locale_to_utf8("tint2", -1, NULL, &len, NULL);
   if (name != NULL) {
      XChangeProperty(server.dsp, win, server.atom._NET_WM_NAME, server.atom.UTF8_STRING, 8, PropModeReplace, (unsigned char *) name, (int) len);
      g_free(name);
   }
  
   // Dock
   long val = server.atom._NET_WM_WINDOW_TYPE_DOCK;
   XChangeProperty (server.dsp, win, server.atom._NET_WM_WINDOW_TYPE, XA_ATOM, 32, PropModeReplace, (unsigned char *) &val, 1);

   // Reserved space
   long   struts [12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
   if (panel.position & TOP) {
      struts[2] = panel.area.height + panel.marginy;
      struts[8] = server.posx;
      struts[9] = server.posx + panel.area.width;
   }
   else {
      struts[3] = panel.area.height + panel.marginy;
      struts[10] = server.posx;
      struts[11] = server.posx + panel.area.width;
   }
   // Old specification : fluxbox need _NET_WM_STRUT.
   XChangeProperty (server.dsp, win, server.atom._NET_WM_STRUT, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &struts, 4);
   XChangeProperty (server.dsp, win, server.atom._NET_WM_STRUT_PARTIAL, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &struts, 12);
   
   // Sticky and below other window
   val = 0xFFFFFFFF;
   XChangeProperty (server.dsp, win, server.atom._NET_WM_DESKTOP, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &val, 1);
   Atom state[4];
   state[0] = server.atom._NET_WM_STATE_SKIP_PAGER;
   state[1] = server.atom._NET_WM_STATE_SKIP_TASKBAR;
   state[2] = server.atom._NET_WM_STATE_STICKY;
   state[3] = server.atom._NET_WM_STATE_BELOW;
   XChangeProperty (server.dsp, win, server.atom._NET_WM_STATE, XA_ATOM, 32, PropModeReplace, (unsigned char *) state, 4);   
   
   // Fixed position
   XSizeHints size_hints;
   size_hints.flags = PPosition;
   XChangeProperty (server.dsp, win, XA_WM_NORMAL_HINTS, XA_WM_SIZE_HINTS, 32, PropModeReplace, (unsigned char *) &size_hints, sizeof (XSizeHints) / 4);
   
   // Unfocusable
   XWMHints wmhints;
   wmhints.flags = InputHint;
   wmhints.input = False;
   XChangeProperty (server.dsp, win, XA_WM_HINTS, XA_WM_HINTS, 32, PropModeReplace, (unsigned char *) &wmhints, sizeof (XWMHints) / 4);
   
   // Undecorated
   long prop[5] = { 2, 0, 0, 0, 0 };
   XChangeProperty(server.dsp, win, server.atom._MOTIF_WM_HINTS, server.atom._MOTIF_WM_HINTS, 32, PropModeReplace, (unsigned char *) prop, 5);
}


void window_draw_panel ()
{
   Window win;

   /* panel position determined here */
   if (panel.position & LEFT) server.posx = server.monitor[panel.monitor].x + panel.marginx;
   else {
      if (panel.position & RIGHT) server.posx = server.monitor[panel.monitor].x + server.monitor[panel.monitor].width - panel.area.width - panel.marginx;
      else server.posx = server.monitor[panel.monitor].x + ((server.monitor[panel.monitor].width - panel.area.width) / 2);
   }
   if (panel.position & TOP) server.posy = server.monitor[panel.monitor].y + panel.marginy;
   else server.posy = server.monitor[panel.monitor].y + server.monitor[panel.monitor].height - panel.area.height - panel.marginy;

   /* Catch some events */
   XSetWindowAttributes att = { ParentRelative, 0L, 0, 0L, 0, 0, Always, 0L, 0L, False, ExposureMask|ButtonPressMask|ButtonReleaseMask, NoEventMask, False, 0, 0 };
               
   // XCreateWindow(display, parent, x, y, w, h, border, depth, class, visual, mask, attrib)
   // main_win doesn't include panel.area.paddingx, so we have WM capabilities on left and right.
   if (window.main_win) XDestroyWindow(server.dsp, window.main_win);
   win = XCreateWindow (server.dsp, server.root_win, server.posx+panel.area.paddingx, server.posy, panel.area.width-(2*panel.area.paddingx), panel.area.height, 0, server.depth, InputOutput, CopyFromParent, CWEventMask, &att);

   set_panel_properties (win);
   window.main_win = win;

   // replaced : server.gc = DefaultGC (server.dsp, 0);
   if (server.gc) XFree(server.gc);
   XGCValues gcValues;
   server.gc = XCreateGC(server.dsp, win, (unsigned long) 0, &gcValues);
   if (server.gc_root) XFree(server.gc_root);
   server.gc_root = XCreateGC(server.dsp, server.root_win, (unsigned long) 0, &gcValues);

   XMapWindow (server.dsp, win);
   XFlush (server.dsp);
}


void visible_object()
{
   if (panel.area.list) {
      g_slist_free(panel.area.list);
      panel.area.list = 0;   
   }

   // list of visible objects
   // start with clock because draw(clock) can resize others object
   if (panel.clock.time1_format)
      panel.area.list = g_slist_append(panel.area.list, &panel.clock);

   int i, j;
   Taskbar *taskbar;
   for (i=0 ; i < panel.nb_desktop ; i++) {
      for (j=0 ; j < panel.nb_monitor ; j++) {
         taskbar = &panel.taskbar[index(i,j)];
         if (panel.mode != MULTI_DESKTOP && taskbar->desktop != server.desktop) continue;
         
         panel.area.list = g_slist_append(panel.area.list, taskbar);
      }
   }
   set_redraw(&panel.area);
   panel.refresh = 1;
}


void set_panel_background()
{
   Pixmap wall = get_root_pixmap();

   panel.area.pmap = server_create_pixmap (panel.area.width, panel.area.height);

   // add layer of root pixmap
   XCopyArea (server.dsp, wall, panel.area.pmap, server.gc, server.posx, server.posy, panel.area.width, panel.area.height, 0, 0);

   // draw background panel
   cairo_surface_t *cs;
   cairo_t *c;
   cs = cairo_xlib_surface_create (server.dsp, panel.area.pmap, server.visual, panel.area.width, panel.area.height);
   c = cairo_create (cs);

   draw_background (&panel.area, c);
   
   cairo_destroy (c);
   cairo_surface_destroy (cs);

   // copy background panel on desktop window
   XCopyArea (server.dsp, panel.area.pmap, server.root_win, server.gc_root, 0, 0, panel.area.width, panel.area.height, server.posx, server.posy);

   set_redraw (&panel.area);
}


