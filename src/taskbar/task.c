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
   Task *new_tsk;
   int desktop, monitor;

   if (!win) return;
   if (window_is_hidden (win) || win == window.main_win) return;

   new_tsk = malloc(sizeof(Task));
   new_tsk->win = win;
   new_tsk->title = 0;
   new_tsk->icon_data = 0;

   get_icon(new_tsk);
   get_title(new_tsk);
   memcpy(&new_tsk->area, &g_task.area, sizeof(Area));
   desktop = window_get_desktop (new_tsk->win);
   monitor = window_get_monitor (new_tsk->win);

   //if (panel.mode == MULTI_MONITOR) monitor = window_get_monitor (new_tsk->win);
   //else monitor = 0;
   //printf("task %s : desktop %d, monitor %d\n", new_tsk->title, desktop, monitor);
   XSelectInput (server.dsp, new_tsk->win, PropertyChangeMask|StructureNotifyMask);

   Taskbar *tskbar;
   if (desktop == 0xFFFFFFFF) {
      tskbar = &panel.taskbar[index(0, monitor)];
      new_tsk->all_desktop = 1;
   }
   else {
      tskbar = &panel.taskbar[index(desktop, monitor)];
      new_tsk->all_desktop = 0;
   }

   //printf("add_task %d  %s\n", index(desktop, monitor), new_tsk->title);
   new_tsk->area.parent = tskbar;
   tskbar->area.list = g_slist_append(tskbar->area.list, new_tsk);

   if (resize_tasks (tskbar))
      set_redraw (&tskbar->area);

   if (desktop == 0xFFFFFFFF) {
      // task on all desktop
      int i;
      Task *new_tsk2;
      for (i = 1 ; i < server.nb_desktop ; i++) {
         new_tsk2 = malloc(sizeof(Task));
         memcpy(new_tsk2, new_tsk, sizeof(Task));
         
         new_tsk2->title = 0;
         new_tsk2->icon_data = 0;
         get_icon(new_tsk2);
         get_title(new_tsk2);

         tskbar = &panel.taskbar[index(i, monitor)];
         new_tsk2->area.parent = tskbar;
         tskbar->area.list = g_slist_append(tskbar->area.list, new_tsk2);

         if (resize_tasks (tskbar))
            set_redraw (&tskbar->area);
      }
   }
}


void remove_task (Task *tsk)
{
   if (!tsk) return;

   Taskbar *tskbar;
   tskbar = (Taskbar*)tsk->area.parent;
   tskbar->area.list = g_slist_remove(tskbar->area.list, tsk);
   resize_tasks (tskbar);
   set_redraw (&tskbar->area);
   //printf("remove_task %d  %s\n", index(tskbar->desktop, tskbar->monitor), tsk->title);

   if (tsk == panel.task_active) 
     	panel.task_active = 0;
   if (tsk == panel.task_drag) 
     	panel.task_drag = 0;

   if (tsk->title) {
      free (tsk->title);
      tsk->title = 0;
   }
   if (tsk->icon_data) {
      free (tsk->icon_data);
      tsk->icon_data = 0;
   }

   XFreePixmap (server.dsp, tsk->area.pix.pmap);
   XFreePixmap (server.dsp, tsk->area.pix_active.pmap);
   free(tsk);
}


void get_title(Task *tsk)
{
   if (!g_task.text) return;

   char *title, *name;

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
   if (g_task.icon) strcpy(title, " ");
   else title[0] = 0;
   strcat(title, name);
   if (name) XFree (name);

   if (tsk->title)
      free(tsk->title);
   tsk->title = title;
}


void get_icon (Task *tsk)
{
   if (!g_task.icon) return;

   long *data;
   int num;

   data = server_get_property (tsk->win, server.atom._NET_WM_ICON, XA_CARDINAL, &num);
   if (data) {
      printf("get_icon plein\n");
      // ARGB
      int w, h;
      long *tmp_data;
      tmp_data = get_best_icon (data, get_icon_count (data, num), num, &w, &h, g_task.icon_size1);

      tsk->icon_width = w;
      tsk->icon_height = h;
      tsk->icon_data = malloc (w * h * sizeof (long));
      memcpy (tsk->icon_data, tmp_data, w * h * sizeof (long));

      XFree (data);
   }
   else {
      printf("get_icon vide\n");
      //XWMHints *hints;
      //hints = XGetWMHints(server.dsp, tkwin);
      //if (hints != NULL) {
      //   XFree(hints);
      //}
      // XChangeProperty (display, windowH, XInternAtom (display, "_NET_WM_ICON", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char*) data, dataSize);
      return;
   }
}


void draw_task_icon (Task *tsk, int text_width, int active)
{
   if (tsk->icon_data == 0) get_icon (tsk);
   if (tsk->icon_data == 0) return;

   Pixmap *pmap = (active == 0) ? (&tsk->area.pix.pmap) : (&tsk->area.pix_active.pmap);

   /* Find pos */
   int pos_x;
   if (g_task.centered) {
      if (g_task.text)
         pos_x = (tsk->area.width - text_width - g_task.icon_size1) / 2;
      else
         pos_x = (tsk->area.width - g_task.icon_size1) / 2;
   }
   else pos_x = g_task.area.paddingx + g_task.area.pix.border.width;

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
   opacity = (active == 0) ? (255*g_task.font.alpha) : (255*g_task.font_active.alpha);
   for (i = 127; i < 256; i++) alpha[i] = opacity;

   imlib_set_color_modifier_tables (red, green, blue, alpha);

   //imlib_render_image_on_drawable (pos_x, pos_y);
   imlib_render_image_on_drawable_at_size (pos_x, g_task.icon_posy, g_task.icon_size1, g_task.icon_size1);

   imlib_free_color_modifier ();
   imlib_free_image ();
   if (sizeof(long) != 4) free(data);
}


void draw_foreground_task (void *obj, cairo_t *c, int active)
{
   Task *tsk = obj;
   PangoLayout *layout;
   config_color *config_text;
   int width, height;

   if (g_task.text) {
		/* Layout */
		layout = pango_cairo_create_layout (c);
		pango_layout_set_font_description (layout, g_task.font_desc);
		pango_layout_set_text (layout, tsk->title, -1);

		/* Drawing width and Cut text */
		pango_layout_set_width (layout, ((Taskbar*)tsk->area.parent)->text_width * PANGO_SCALE);
		pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

		/* Center text */
		if (g_task.centered) pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
		else pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);

		pango_layout_get_pixel_size (layout, &width, &height);

		if (active) config_text = &g_task.font_active;
		else config_text = &g_task.font;

		cairo_set_source_rgba (c, config_text->color[0], config_text->color[1], config_text->color[2], config_text->alpha);

		pango_cairo_update_layout (c, layout);
		cairo_move_to (c, g_task.text_posx, g_task.text_posy);
		pango_cairo_show_layout (c, layout);

		if (g_task.font_shadow) {
			cairo_set_source_rgba (c, 0.0, 0.0, 0.0, 0.5);
			pango_cairo_update_layout (c, layout);
			cairo_move_to (c, g_task.text_posx + 1, g_task.text_posy + 1);
			pango_cairo_show_layout (c, layout);
		}
		g_object_unref (layout);
   }

   if (g_task.icon) {
      // icon use same opacity as text
      draw_task_icon (tsk, width, active);
   }
}

