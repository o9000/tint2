/**************************************************************************
*
* Tint2 : clock
*
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
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

#include <string.h>
#include <stdio.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <pango/pangocairo.h>

#include "window.h"
#include "server.h"
#include "taskbar.h"
#include "panel.h"
#include "area.h"
#include "clock.h"


char *time1_format = 0;
char *time2_format = 0;
struct timeval time_clock;
int  time_precision;
PangoFontDescription *time1_font_desc;
PangoFontDescription *time2_font_desc;
static char buf_time[40];
static char buf_date[40];


void init_clock(Clock *clock, Area *parent)
{
   Panel *panel = (Panel *)parent;
   int time_height, time_height_ink, date_height, date_height_ink;

	clock->area.parent = parent;
	clock->area.panel = panel;
   if (!time1_format) return;

   clock->area._draw_foreground = draw_foreground_clock;
   clock->area._resize = resize_clock;

   if (strchr(time1_format, 'S') == NULL) time_precision = 60;
   else time_precision = 1;

   // update clock to force update (-time_precision)
   struct timeval stv;
   gettimeofday(&stv, 0);
   time_clock.tv_sec = stv.tv_sec - time_precision;
   time_clock.tv_sec -= time_clock.tv_sec % time_precision;

   clock->area.posy = parent->pix.border.width + parent->paddingy;
   clock->area.height = parent->height - (2 * clock->area.posy);
   clock->area.resize = 1;
   clock->area.redraw = 1;

   strftime(buf_time, sizeof(buf_time), time1_format, localtime(&time_clock.tv_sec));
   if (time2_format)
      strftime(buf_date, sizeof(buf_date), time2_format, localtime(&time_clock.tv_sec));

   get_text_size(time1_font_desc, &time_height_ink, &time_height, parent->height, buf_time, strlen(buf_time));
   clock->time1_posy = (clock->area.height - time_height) / 2;

   if (time2_format) {
      get_text_size(time2_font_desc, &date_height_ink, &date_height, parent->height, buf_date, strlen(buf_date));

      clock->time1_posy -= ((date_height_ink + 2) / 2);
      clock->time2_posy = clock->time1_posy + time_height + 2 - (time_height - time_height_ink)/2 - (date_height - date_height_ink)/2;
   }
}


void draw_foreground_clock (void *obj, cairo_t *c, int active)
{
   Clock *clock = obj;
   PangoLayout *layout;

   //printf("  draw_foreground_clock : %s en (%d, %d)\n", buf_time, clock->area.posx, clock->area.width);
   layout = pango_cairo_create_layout (c);

   // draw layout
   pango_layout_set_font_description (layout, time1_font_desc);
   pango_layout_set_width (layout, clock->area.width * PANGO_SCALE);
   pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
   pango_layout_set_text (layout, buf_time, strlen(buf_time));

   cairo_set_source_rgba (c, clock->font.color[0], clock->font.color[1], clock->font.color[2], clock->font.alpha);

   pango_cairo_update_layout (c, layout);
   cairo_move_to (c, 0, clock->time1_posy);
   pango_cairo_show_layout (c, layout);

   if (time2_format) {
      pango_layout_set_font_description (layout, time2_font_desc);
      pango_layout_set_indent(layout, 0);
      pango_layout_set_text (layout, buf_date, strlen(buf_date));
      pango_layout_set_width (layout, clock->area.width * PANGO_SCALE);

      pango_cairo_update_layout (c, layout);
      cairo_move_to (c, 0, clock->time2_posy);
      pango_cairo_show_layout (c, layout);
   }

   g_object_unref (layout);
}


void resize_clock (void *obj)
{
   Clock *clock = obj;
   PangoLayout *layout;
   int time_width, date_width, new_width;

   time_width = date_width = 0;
   clock->area.redraw = 1;

   strftime(buf_time, sizeof(buf_time), time1_format, localtime(&time_clock.tv_sec));
   if (time2_format)
      strftime(buf_date, sizeof(buf_date), time2_format, localtime(&time_clock.tv_sec));

   //printf("  resize_clock\n");
   cairo_surface_t *cs;
   cairo_t *c;
	Pixmap pmap;
	pmap = XCreatePixmap (server.dsp, server.root_win, clock->area.width, clock->area.height, server.depth);

   cs = cairo_xlib_surface_create (server.dsp, pmap, server.visual, clock->area.width, clock->area.height);
   c = cairo_create (cs);
   layout = pango_cairo_create_layout (c);

   // check width
   pango_layout_set_font_description (layout, time1_font_desc);
   pango_layout_set_indent(layout, 0);
   pango_layout_set_text (layout, buf_time, strlen(buf_time));
   pango_layout_get_pixel_size (layout, &time_width, NULL);
   if (time2_format) {
      pango_layout_set_font_description (layout, time2_font_desc);
      pango_layout_set_indent(layout, 0);
      pango_layout_set_text (layout, buf_date, strlen(buf_date));
      pango_layout_get_pixel_size (layout, &date_width, NULL);
   }

   if (time_width > date_width) new_width = time_width;
   else new_width = date_width;
   new_width += (2*clock->area.paddingxlr) + (2*clock->area.pix.border.width);

   if (new_width > clock->area.width || new_width < (clock->area.width-3)) {
      int i;
      Panel *panel = ((Area*)obj)->panel;

      //printf("clock_width %d, new_width %d\n", clock->area.width, new_width);
      // resize clock
      clock->area.width = new_width;
      clock->area.posx = panel->area.width - clock->area.width - panel->area.paddingxlr - panel->area.pix.border.width;

      // resize other objects on panel
		for (i=0 ; i < nb_panel ; i++) {
			panel1[i].area.resize = 1;
		}
		panel_refresh = 1;
   }

   g_object_unref (layout);
   cairo_destroy (c);
   cairo_surface_destroy (cs);
   XFreePixmap (server.dsp, pmap);
}

