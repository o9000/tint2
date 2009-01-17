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
#include "area.h"
#include "clock.h"
#include "panel.h"


void init_clock(Clock *clock, int panel_height)
{
   char buf_time[40];
   char buf_date[40];
   int time_height, time_height_ink, date_height, date_height_ink;

   if (!clock->time1_format) return;

   if (strchr(clock->time1_format, 'S') == NULL) clock->time_precision = 60;
   else clock->time_precision = 1;

   clock->area.posy = panel.area.pix.border.width + panel.area.paddingy;
   clock->area.height = panel.area.height - (2 * clock->area.posy);
   clock->area.redraw = 1;

   gettimeofday(&clock->clock, 0);
   clock->clock.tv_sec -= clock->clock.tv_sec % clock->time_precision;

   strftime(buf_time, sizeof(buf_time), clock->time1_format, localtime(&clock->clock.tv_sec));
   if (clock->time2_format)
      strftime(buf_date, sizeof(buf_date), clock->time2_format, localtime(&clock->clock.tv_sec));

   get_text_size(clock->time1_font_desc, &time_height_ink, &time_height, panel_height, buf_time, strlen(buf_time));
   clock->time1_posy = (clock->area.height - time_height) / 2;

   if (clock->time2_format) {
      get_text_size(clock->time2_font_desc, &date_height_ink, &date_height, panel_height, buf_date, strlen(buf_date));

      clock->time1_posy -= ((date_height_ink + 2) / 2);
      clock->time2_posy = clock->time1_posy + time_height + 2 - (time_height - time_height_ink)/2 - (date_height - date_height_ink)/2;
   }
}


void draw_foreground_clock (void *obj, cairo_t *c)
{
   Clock *clock = obj;
   PangoLayout *layout;
   char buf_time[40];
   char buf_date[40];
   int time_width, date_width, new_width;

   time_width = date_width = 0;
   strftime(buf_time, sizeof(buf_time), clock->time1_format, localtime(&clock->clock.tv_sec));
   if (clock->time2_format)
      strftime(buf_date, sizeof(buf_date), clock->time2_format, localtime(&clock->clock.tv_sec));

   //printf("  draw_foreground_clock : %s\n", buf_time);
redraw:
   layout = pango_cairo_create_layout (c);

   // check width
   pango_layout_set_font_description (layout, clock->time1_font_desc);
   pango_layout_set_indent(layout, 0);
   pango_layout_set_text (layout, buf_time, strlen(buf_time));
   pango_layout_get_pixel_size (layout, &time_width, NULL);
   if (clock->time2_format) {
      pango_layout_set_font_description (layout, clock->time2_font_desc);
      pango_layout_set_indent(layout, 0);
      pango_layout_set_text (layout, buf_date, strlen(buf_date));
      pango_layout_get_pixel_size (layout, &date_width, NULL);
   }
   if (time_width > date_width) new_width = time_width;
   else new_width = date_width;
   new_width += (2*clock->area.paddingx) + (2*clock->area.pix.border.width);

   if (new_width > clock->area.width || (new_width != clock->area.width && date_width > time_width)) {
      //printf("clock_width %d, new_width %d\n", clock->area.width, new_width);
      // resize clock
      clock->area.width = new_width;
      panel.clock.area.posx = panel.area.width - panel.clock.area.width - panel.area.paddingx - panel.area.pix.border.width;

      g_object_unref (layout);
      resize_taskbar();
      goto redraw;
   }

   // draw layout
   pango_layout_set_font_description (layout, clock->time1_font_desc);
   pango_layout_set_width (layout, clock->area.width * PANGO_SCALE);
   pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
   pango_layout_set_text (layout, buf_time, strlen(buf_time));

   cairo_set_source_rgba (c, clock->font.color[0], clock->font.color[1], clock->font.color[2], clock->font.alpha);

   pango_cairo_update_layout (c, layout);
   cairo_move_to (c, 0, clock->time1_posy);
   pango_cairo_show_layout (c, layout);

   if (clock->time2_format) {
      pango_layout_set_font_description (layout, clock->time2_font_desc);
      pango_layout_set_indent(layout, 0);
      pango_layout_set_text (layout, buf_date, strlen(buf_date));
      pango_layout_set_width (layout, clock->area.width * PANGO_SCALE);

      pango_cairo_update_layout (c, layout);
      cairo_move_to (c, 0, clock->time2_posy);
      pango_cairo_show_layout (c, layout);
   }

   g_object_unref (layout);
}

