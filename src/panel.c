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

#include <stdio.h>
#include <stdlib.h>
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


int signal_pending;
// --------------------------------------------------
// mouse events
int mouse_middle;
int mouse_right;
int mouse_scroll_up;
int mouse_scroll_down;

int panel_mode;
int panel_position;
int panel_refresh;

Task *task_active = 0;
Task *task_drag = 0;

Panel *panel1 = 0;
int  nb_panel;



void init_panel()
{
	int i;
	Panel *p;
	for (i=0 ; i < nb_panel ; i++) {
		p = &panel1[i];

		p->area.parent = p;
		p->area.panel = p;
		p->area.visible = 1;
		p->area._resize = resize_panel;
		p->g_taskbar.parent = p;
		p->g_taskbar.panel = p;
		p->g_task.area.panel = p;

		// add childs
	   if (time1_format)
			p->area.list = g_slist_append(p->area.list, &p->clock);
		//panel->area.list = g_slist_append(panel->area.list, &panel->trayer);

		// detect panel size
		if (p->pourcentx)
			p->area.width = (float)server.monitor[p->monitor].width * p->initial_width / 100;
		else
			p->area.width = p->initial_width;
		if (p->pourcenty)
			p->area.height = (float)server.monitor[p->monitor].height * p->initial_height / 100;
		else
			p->area.height = p->initial_height;

		// full width mode
		if (!p->area.width)
			p->area.width = server.monitor[p->monitor].width;

		if (p->area.pix.border.rounded > p->area.height/2)
			p->area.pix.border.rounded = p->area.height/2;

		/* panel position determined here */
		if (panel_position & LEFT) {
			p->posx = server.monitor[p->monitor].x + p->marginx;
		}
		else {
			if (panel_position & RIGHT) {
				p->posx = server.monitor[p->monitor].x + server.monitor[p->monitor].width - p->area.width - p->marginx;
			}
			else {
				p->posx = server.monitor[p->monitor].x + ((server.monitor[p->monitor].width - p->area.width) / 2);
			}
		}
		if (panel_position & TOP) {
			p->posy = server.monitor[p->monitor].y + p->marginy;
		}
		else {
			p->posy = server.monitor[p->monitor].y + server.monitor[p->monitor].height - p->area.height - p->marginy;
		}

		// Catch some events
		XSetWindowAttributes att = { ParentRelative, 0L, 0, 0L, 0, 0, Always, 0L, 0L, False, ExposureMask|ButtonPressMask|ButtonReleaseMask, NoEventMask, False, 0, 0 };

		// XCreateWindow(display, parent, x, y, w, h, border, depth, class, visual, mask, attrib)
		// main_win doesn't include panel.area.paddingx, so we have WM capabilities on left and right.
		if (p->main_win) XDestroyWindow(server.dsp, p->main_win);
		//win = XCreateWindow (server.dsp, server.root_win, p->posx+p->area.paddingxlr, p->posy, p->area.width-(2*p->area.paddingxlr), p->area.height, 0, server.depth, InputOutput, CopyFromParent, CWEventMask, &att);
		p->main_win = XCreateWindow(server.dsp, server.root_win, p->posx, p->posy, p->area.width, p->area.height, 0, server.depth, InputOutput, CopyFromParent, CWEventMask, &att);

		set_panel_properties(p);
		set_panel_background(p);

		XMapWindow (server.dsp, p->main_win);

		init_clock(&p->clock, &p->area);
	}
	panel_refresh = 1;
}


void cleanup_panel()
{
	if (!panel1) return;

   cleanup_taskbar();

	// font allocated once
   if (panel1[0].g_task.font_desc) {
   	pango_font_description_free(panel1[0].g_task.font_desc);
   	panel1[0].g_task.font_desc = 0;
	}

	int i;
	Panel *p;
	for (i=0 ; i < nb_panel ; i++) {
		p = &panel1[i];

		free_area(&p->area);
	   free_area(&p->g_task.area);
	   free_area(&p->g_taskbar);

		if (p->temp_pmap) {
			XFreePixmap(server.dsp, p->temp_pmap);
			p->temp_pmap = 0;
		}
		if (p->main_win) {
			XDestroyWindow(server.dsp, p->main_win);
			p->main_win = 0;
		}
	}

   if (panel1) free(panel1);
   panel1 = 0;
}


void resize_panel(void *obj)
{
   Panel *panel = (Panel*)obj;
   int taskbar_width, modulo_width, taskbar_on_screen;

//printf("resize_panel :  : posx et width des barres de taches\n");

   if (panel_mode == MULTI_DESKTOP) taskbar_on_screen = panel->nb_desktop;
   else taskbar_on_screen = 1;

   taskbar_width = panel->area.width - (2 * panel->area.paddingxlr) - (2 * panel->area.pix.border.width);
   if (time1_format)
      taskbar_width -= (panel->clock.area.width + panel->area.paddingx);
   //taskbar_width -= (panel->trayer.area.width + panel->area.paddingx);

   taskbar_width = (taskbar_width - ((taskbar_on_screen-1) * panel->area.paddingx)) / taskbar_on_screen;

   if (taskbar_on_screen > 1)
      modulo_width = (taskbar_width - ((taskbar_on_screen-1) * panel->area.paddingx)) % taskbar_on_screen;
   else
      modulo_width = 0;

	// change posx and width for all taskbar
   int i, modulo=0, posx=0;
   for (i=0 ; i < panel->nb_desktop ; i++) {
      if ((i % taskbar_on_screen) == 0) {
         posx = panel->area.pix.border.width + panel->area.paddingxlr;
         modulo = modulo_width;
      }
      else posx += taskbar_width + panel->area.paddingx;

      panel->taskbar[i].area.posx = posx;
      panel->taskbar[i].area.width = taskbar_width;
      panel->taskbar[i].area.resize = 1;
      if (modulo) {
         panel->taskbar[i].area.width++;
         modulo--;
      }
   }
}


void visible_object()
{
   Panel *panel;
   int i, j;

	for (i=0 ; i < nb_panel ; i++) {
		panel = &panel1[i];

		// clock before taskbar because resize(clock) can resize others object
		if (time1_format)
			panel->clock.area.visible = 1;
		else
			panel->clock.area.visible = 0;

		Taskbar *taskbar;
		for (j=0 ; j < panel->nb_desktop ; j++) {
			taskbar = &panel->taskbar[j];
			if (panel_mode != MULTI_DESKTOP && taskbar->desktop != server.desktop) {
				// (SINGLE_DESKTOP or SINGLE_MONITOR) and not current desktop
				taskbar->area.visible = 0;
			}
			else {
				taskbar->area.visible = 1;
			}
		}
	}
	panel_refresh = 1;
}


void set_panel_properties(Panel *p)
{
   XStoreName (server.dsp, p->main_win, "tint2");

   // TODO: check if the name is really needed for a panel/taskbar ?
   gsize len;
   gchar *name = g_locale_to_utf8("tint2", -1, NULL, &len, NULL);
   if (name != NULL) {
      XChangeProperty(server.dsp, p->main_win, server.atom._NET_WM_NAME, server.atom.UTF8_STRING, 8, PropModeReplace, (unsigned char *) name, (int) len);
      g_free(name);
   }

   // Dock
   long val = server.atom._NET_WM_WINDOW_TYPE_DOCK;
   XChangeProperty (server.dsp, p->main_win, server.atom._NET_WM_WINDOW_TYPE, XA_ATOM, 32, PropModeReplace, (unsigned char *) &val, 1);

   // Reserved space
   long   struts [12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
   if (panel_position & TOP) {
      struts[2] = p->area.height + p->marginy;
      struts[8] = p->posx;
      // p->area.width - 1 allowed full screen on monitor 2
      struts[9] = p->posx + p->area.width - 1;
   }
   else {
      struts[3] = p->area.height + p->marginy;
      struts[10] = p->posx;
      // p->area.width - 1 allowed full screen on monitor 2
      struts[11] = p->posx + p->area.width - 1;
   }
   // Old specification : fluxbox need _NET_WM_STRUT.
   XChangeProperty (server.dsp, p->main_win, server.atom._NET_WM_STRUT, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &struts, 4);
   XChangeProperty (server.dsp, p->main_win, server.atom._NET_WM_STRUT_PARTIAL, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &struts, 12);

   // Sticky and below other window
   val = 0xFFFFFFFF;
   XChangeProperty (server.dsp, p->main_win, server.atom._NET_WM_DESKTOP, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &val, 1);
   Atom state[4];
   state[0] = server.atom._NET_WM_STATE_SKIP_PAGER;
   state[1] = server.atom._NET_WM_STATE_SKIP_TASKBAR;
   state[2] = server.atom._NET_WM_STATE_STICKY;
   state[3] = server.atom._NET_WM_STATE_BELOW;
   XChangeProperty (server.dsp, p->main_win, server.atom._NET_WM_STATE, XA_ATOM, 32, PropModeReplace, (unsigned char *) state, 4);

   // Fixed position
   XSizeHints size_hints;
   size_hints.flags = PPosition;
   XChangeProperty (server.dsp, p->main_win, XA_WM_NORMAL_HINTS, XA_WM_SIZE_HINTS, 32, PropModeReplace, (unsigned char *) &size_hints, sizeof (XSizeHints) / 4);

   // Unfocusable
   XWMHints wmhints;
   wmhints.flags = InputHint;
   wmhints.input = False;
   XChangeProperty (server.dsp, p->main_win, XA_WM_HINTS, XA_WM_HINTS, 32, PropModeReplace, (unsigned char *) &wmhints, sizeof (XWMHints) / 4);

   // Undecorated
   long prop[5] = { 2, 0, 0, 0, 0 };
   XChangeProperty(server.dsp, p->main_win, server.atom._MOTIF_WM_HINTS, server.atom._MOTIF_WM_HINTS, 32, PropModeReplace, (unsigned char *) prop, 5);
}


void set_panel_background(Panel *p)
{
   get_root_pixmap();

   if (p->area.pix.pmap) XFreePixmap (server.dsp, p->area.pix.pmap);
   p->area.pix.pmap = XCreatePixmap (server.dsp, server.root_win, p->area.width, p->area.height, server.depth);

   // copy background (server.root_pmap) in panel.area.pix.pmap
   Window dummy;
   int  x, y;
   XTranslateCoordinates(server.dsp, p->main_win, server.root_win, 0, 0, &x, &y, &dummy);
   XSetTSOrigin(server.dsp, server.gc, -x, -y) ;
   XFillRectangle(server.dsp, p->area.pix.pmap, server.gc, 0, 0, p->area.width, p->area.height);

   // draw background panel
   cairo_surface_t *cs;
   cairo_t *c;
   cs = cairo_xlib_surface_create (server.dsp, p->area.pix.pmap, server.visual, p->area.width, p->area.height);
   c = cairo_create (cs);

   draw_background(&p->area, c, 0);

   cairo_destroy (c);
   cairo_surface_destroy (cs);

	// redraw panel's object
   GSList *l0;
   Area *a;
	for (l0 = p->area.list; l0 ; l0 = l0->next) {
      a = l0->data;
      set_redraw(a);
   }
}


Panel *get_panel(Window win)
{
	int i;
	for (i=0 ; i < nb_panel ; i++) {
		if (panel1[i].main_win == win) {
			return &panel1[i];
		}
	}
	return 0;
}

