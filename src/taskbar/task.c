/**************************************************************************
*
* Tint2 : task
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <Imlib2.h>

#include "window.h"
#include "task.h"
#include "server.h"
#include "panel.h"



void add_task (Window win)
{
   if (!win) return;
   if (window_is_hidden(win)) return;

   int monitor;

	Task new_tsk;
   new_tsk.win = win;
   new_tsk.area.panel = &panel1[0];
   new_tsk.desktop = window_get_desktop (win);
//   if (panel_mode == SINGLE_MONITOR) monitor = window_get_monitor (win);
   if (nb_panel > 1) monitor = window_get_monitor (win);
   else monitor = 0;

	// allocate only one title and one icon
	// even with task_on_all_desktop and with task_on_all_panel
   new_tsk.title = 0;
   new_tsk.icon_data = 0;
   get_title(&new_tsk);
   get_icon(&new_tsk);

   //printf("task %s : desktop %d, monitor %d\n", new_tsk->title, desktop, monitor);
   XSelectInput (server.dsp, new_tsk.win, PropertyChangeMask|StructureNotifyMask);

	Taskbar *tskbar;
   Task *new_tsk2;
	int i, j;
	for (i=0 ; i < nb_panel ; i++) {
		for (j=0 ; j < panel1[i].nb_desktop ; j++) {
			if (new_tsk.desktop != ALLDESKTOP && new_tsk.desktop != j) continue;
			if (nb_panel > 1 && panel1[i].monitor != monitor) continue;

	   	tskbar = &panel1[i].taskbar[j];
		   new_tsk2 = malloc(sizeof(Task));
		   memcpy(&new_tsk2->area, &panel1[i].g_task.area, sizeof(Area));
		   new_tsk2->area.parent = tskbar;
			new_tsk2->win = new_tsk.win;
			new_tsk2->desktop = new_tsk.desktop;
			new_tsk2->title = new_tsk.title;
			new_tsk2->icon_data = new_tsk.icon_data;
			new_tsk2->icon_width = new_tsk.icon_width;
			new_tsk2->icon_height = new_tsk.icon_height;
   		tskbar->area.list = g_slist_append(tskbar->area.list, new_tsk2);
   		tskbar->area.resize = 1;
			//printf("add_task panel %d, desktop %d, task %s\n", i, j, new_tsk2->title);
		}
	}
}


void remove_task (Task *tsk)
{
   if (!tsk) return;

   Window win = tsk->win;
   int desktop = tsk->desktop;

	// free title and icon just for the first task
	// even with task_on_all_desktop and with task_on_all_panel
	//printf("remove_task %s %d\n", tsk->title, tsk->desktop);
	if (tsk->title)
		free (tsk->title);
	if (tsk->icon_data)
		free (tsk->icon_data);

	int i, j;
   Task *tsk2;
   Taskbar *tskbar;
	for (i=0 ; i < nb_panel ; i++) {
		for (j=0 ; j < panel1[i].nb_desktop ; j++) {
			if (desktop != ALLDESKTOP && desktop != j) continue;

			GSList *l0;
			tskbar = &panel1[i].taskbar[j];
			for (l0 = tskbar->area.list; l0 ; ) {
				tsk2 = l0->data;
				l0 = l0->next;
				if (win == tsk2->win) {
					tskbar->area.list = g_slist_remove(tskbar->area.list, tsk2);
         		tskbar->area.resize = 1;

					if (tsk2 == task_active)
						task_active = 0;
					if (tsk2 == task_drag)
						task_drag = 0;

					XFreePixmap (server.dsp, tsk2->area.pix.pmap);
					XFreePixmap (server.dsp, tsk2->area.pix_active.pmap);
					free(tsk2);
				}
			}
		}
	}

}


void get_title(Task *tsk)
{
   Panel *panel = tsk->area.panel;
   char *title, *name;

   if (!panel->g_task.text) return;

   name = server_get_property (tsk->win, server.atom._NET_WM_VISIBLE_NAME, server.atom.UTF8_STRING, 0);
   if (!name || !strlen(name)) {
      name = server_get_property (tsk->win, server.atom._NET_WM_NAME, server.atom.UTF8_STRING, 0);
      if (!name || !strlen(name)) {
         name = server_get_property (tsk->win, server.atom.WM_NAME, XA_STRING, 0);
         if (!name || !strlen(name)) {
            name = malloc(10);
            strcpy(name, "Untitled");
         }
      }
   }

   // add space before title
   title = malloc(strlen(name)+2);
   if (panel->g_task.icon) strcpy(title, " ");
   else title[0] = 0;
   strcat(title, name);
   if (name) XFree (name);

	tsk->area.redraw = 1;
   if (tsk->title)
      free(tsk->title);
   tsk->title = title;
}


void get_icon (Task *tsk)
{
   Panel *panel = tsk->area.panel;
   if (!panel->g_task.icon) return;

	if (tsk->icon_data) {
		free (tsk->icon_data);
		tsk->icon_data = 0;
	}
   tsk->area.redraw = 1;

   long *data;
   int num;
   data = server_get_property (tsk->win, server.atom._NET_WM_ICON, XA_CARDINAL, &num);
   if (data) {
      // get ARGB icon
      int w, h;
      long *tmp_data;
      tmp_data = get_best_icon (data, get_icon_count (data, num), num, &w, &h, panel->g_task.icon_size1);

      tsk->icon_width = w;
      tsk->icon_height = h;
      tsk->icon_data = malloc (w * h * sizeof (long));
      memcpy (tsk->icon_data, tmp_data, w * h * sizeof (long));

      XFree (data);
   }
   else {
      // get Pixmap icon
      XWMHints *hints = XGetWMHints(server.dsp, tsk->win);
      if (hints) {
         if (hints->flags & IconPixmapHint && hints->icon_pixmap != 0) {
         	// get width, height and depth for the pixmap
				Window root;
				int  icon_x, icon_y;
				uint border_width, bpp;
				uint icon_width, icon_height;

				XGetGeometry(server.dsp, hints->icon_pixmap, &root, &icon_x, &icon_y, &icon_width, &icon_height, &border_width, &bpp);

		      //printf("  get_pixmap\n");
		      Imlib_Image  img;
				imlib_context_set_drawable(hints->icon_pixmap);
				img = imlib_create_image_from_drawable(hints->icon_mask, 0, 0, icon_width, icon_height, 0);
				imlib_context_set_image(img);
				unsigned int *data = imlib_image_get_data();
				if (!data) {
					return;
				}
				tsk->icon_width = imlib_image_get_width();
				tsk->icon_height = imlib_image_get_height();
				tsk->icon_data = malloc (tsk->icon_width * tsk->icon_height * sizeof (long));
				memcpy (tsk->icon_data, data, tsk->icon_width * tsk->icon_height * sizeof (long));
				imlib_free_image();
			}
         XFree(hints);
		}
   }
}


void draw_task_icon (Task *tsk, int text_width, int active)
{
   if (tsk->icon_data == 0) return;

   Pixmap *pmap = (active == 0) ? (&tsk->area.pix.pmap) : (&tsk->area.pix_active.pmap);

   /* Find pos */
   int pos_x;
   Panel *panel = (Panel*)tsk->area.panel;
   if (panel->g_task.centered) {
      if (panel->g_task.text)
         pos_x = (tsk->area.width - text_width - panel->g_task.icon_size1) / 2;
      else
         pos_x = (tsk->area.width - panel->g_task.icon_size1) / 2;
   }
   else pos_x = panel->g_task.area.paddingxlr + panel->g_task.area.pix.border.width;

   /* Render */
   Imlib_Image icon;
   Imlib_Color_Modifier cmod;
   DATA8 red[256], green[256], blue[256], alpha[256];

   // TODO: cpu improvement : compute only when icon changed
   DATA32 *data;
   /* do we have 64bit? => long = 8bit */
   if (sizeof(long) != 4) {
      int length = tsk->icon_width * tsk->icon_height;
      data = malloc(sizeof(DATA32) * length);
      int i;
      for (i = 0; i < length; ++i)
         data[i] = tsk->icon_data[i];
   }
   else data = (DATA32 *) tsk->icon_data;

   icon = imlib_create_image_using_data (tsk->icon_width, tsk->icon_height, data);
   imlib_context_set_image (icon);
   imlib_context_set_drawable (*pmap);

   cmod = imlib_create_color_modifier ();
   imlib_context_set_color_modifier (cmod);
   imlib_image_set_has_alpha (1);
   imlib_get_color_modifier_tables (red, green, blue, alpha);

   int i, opacity;
   opacity = (active == 0) ? (255*panel->g_task.font.alpha) : (255*panel->g_task.font_active.alpha);
   for (i = 127; i < 256; i++) alpha[i] = opacity;

   imlib_set_color_modifier_tables (red, green, blue, alpha);

   //imlib_render_image_on_drawable (pos_x, pos_y);
   imlib_render_image_on_drawable_at_size (pos_x, panel->g_task.icon_posy, panel->g_task.icon_size1, panel->g_task.icon_size1);

   imlib_free_color_modifier ();
   imlib_free_image ();
   if (sizeof(long) != 4) free(data);
}


void draw_task (void *obj, cairo_t *c, int active)
{
   Task *tsk = obj;
   PangoLayout *layout;
   config_color *config_text;
   int width=0, height;
   Panel *panel = (Panel*)tsk->area.panel;

   if (panel->g_task.text) {
		/* Layout */
		layout = pango_cairo_create_layout (c);
		pango_layout_set_font_description (layout, panel->g_task.font_desc);
		pango_layout_set_text (layout, tsk->title, -1);

		/* Drawing width and Cut text */
		// pango use U+22EF or U+2026
		pango_layout_set_width (layout, ((Taskbar*)tsk->area.parent)->text_width * PANGO_SCALE);
		pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

		/* Center text */
		if (panel->g_task.centered) pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
		else pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);

		pango_layout_get_pixel_size (layout, &width, &height);

		if (active) config_text = &panel->g_task.font_active;
		else config_text = &panel->g_task.font;

		cairo_set_source_rgba (c, config_text->color[0], config_text->color[1], config_text->color[2], config_text->alpha);

		pango_cairo_update_layout (c, layout);
		cairo_move_to (c, panel->g_task.text_posx, panel->g_task.text_posy);
		pango_cairo_show_layout (c, layout);

		if (panel->g_task.font_shadow) {
			cairo_set_source_rgba (c, 0.0, 0.0, 0.0, 0.5);
			pango_cairo_update_layout (c, layout);
			cairo_move_to (c, panel->g_task.text_posx + 1, panel->g_task.text_posy + 1);
			pango_cairo_show_layout (c, layout);
		}
		g_object_unref (layout);
   }

   if (panel->g_task.icon) {
      // icon use same opacity as text
      draw_task_icon (tsk, width, active);
   }
}

