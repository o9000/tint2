/**************************************************************************
*
* Tint2 : read/write config file
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

#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glib/gstdio.h>
#include <pango/pangocairo.h>
#include <Imlib2.h>

#include "common.h"
#include "server.h"
#include "task.h"
#include "taskbar.h"
#include "clock.h"
#include "panel.h"
#include "config.h"
#include "window.h"


void cleanup_taskbar()
{
   GSList *l0;
   Task *tsk;

   int i, nb;
   nb = panel.nb_desktop * panel.nb_monitor;
   for (i=0 ; i < nb ; i++) {
      l0 = panel.taskbar[i].area.list;
      while (l0) {
         tsk = l0->data;
         l0 = l0->next;
         // careful : remove_task change l0->next
         remove_task (tsk);
      }

      free_area (&panel.taskbar[i].area);
   }

   free(panel.taskbar);
   panel.taskbar = 0;

   free_area(&panel.area);
}


void cleanup ()
{
   if (panel.old_task_font) free(panel.old_task_font);
   if (g_task.font_desc) pango_font_description_free(g_task.font_desc);
   if (panel.clock.time1_font_desc) pango_font_description_free(panel.clock.time1_font_desc);
   if (panel.clock.time2_font_desc) pango_font_description_free(panel.clock.time2_font_desc);
   if (panel.taskbar) cleanup_taskbar();
   if (panel.clock.time1_format) g_free(panel.clock.time1_format);
   if (panel.clock.time2_format) g_free(panel.clock.time2_format);
   if (server.monitor) free(server.monitor);
   XCloseDisplay(server.dsp);
}


void copy_file(const char *pathSrc, const char *pathDest)
{
   FILE *fileSrc, *fileDest;
   char line[100];
   int  nb;

   fileSrc = fopen(pathSrc, "rb");
   if (fileSrc == NULL) return;

   fileDest = fopen(pathDest, "wb");
   if (fileDest == NULL) return;

   while ((nb = fread(line, 1, 100, fileSrc)) > 0) fwrite(line, 1, nb, fileDest);

   fclose (fileDest);
   fclose (fileSrc);
}


void extract_values (const char *value, char **value1, char **value2)
{
   char *b;

   if (*value1) free (*value1);
   if (*value2) free (*value2);

   if ((b = strchr (value, ' '))) {
      b[0] = '\0';
      b++;
      *value2 = strdup (b);
      g_strstrip(*value2);
   }
   else *value2 = 0;

   *value1 = strdup (value);
   g_strstrip(*value1);
}


int hex_char_to_int (char c)
{
   int r;

   if (c >= '0' && c <= '9')  r = c - '0';
   else if (c >= 'a' && c <= 'f')  r = c - 'a' + 10;
   else if (c >= 'A' && c <= 'F')  r = c - 'A' + 10;
   else  r = 0;

   return r;
}


int hex_to_rgb (char *hex, int *r, int *g, int *b)
{
   int len;

   if (hex == NULL || hex[0] != '#') return (0);

   len = strlen (hex);
   if (len == 3 + 1) {
      *r = hex_char_to_int (hex[1]);
      *g = hex_char_to_int (hex[2]);
      *b = hex_char_to_int (hex[3]);
   }
   else if (len == 6 + 1) {
      *r = hex_char_to_int (hex[1]) * 16 + hex_char_to_int (hex[2]);
      *g = hex_char_to_int (hex[3]) * 16 + hex_char_to_int (hex[4]);
      *b = hex_char_to_int (hex[5]) * 16 + hex_char_to_int (hex[6]);
   }
   else if (len == 12 + 1) {
      *r = hex_char_to_int (hex[1]) * 16 + hex_char_to_int (hex[2]);
      *g = hex_char_to_int (hex[5]) * 16 + hex_char_to_int (hex[6]);
      *b = hex_char_to_int (hex[9]) * 16 + hex_char_to_int (hex[10]);
   }
   else return 0;

   return 1;
}


void get_color (char *hex, double *rgb)
{
   int r, g, b;
   hex_to_rgb (hex, &r, &g, &b);

   rgb[0] = (r / 255.0);
   rgb[1] = (g / 255.0);
   rgb[2] = (b / 255.0);
}


void get_action (char *event, int *action)
{
   if (strcmp (event, "none") == 0)
      *action = NONE;
   else if (strcmp (event, "close") == 0)
      *action = CLOSE;
   else if (strcmp (event, "toggle") == 0)
      *action = TOGGLE;
   else if (strcmp (event, "iconify") == 0)
      *action = ICONIFY;
   else if (strcmp (event, "shade") == 0)
      *action = SHADE;
   else if (strcmp (event, "toggle_iconify") == 0)
      *action = TOGGLE_ICONIFY;
}


void add_entry (char *key, char *value)
{
   char *value1=0, *value2=0;

   /* Background and border */
   if (strcmp (key, "rounded") == 0) {
      // 'rounded' is the first parameter => alloc a new background
      Area *a = calloc(1, sizeof(Area));
      a->pix.border.rounded = atoi (value);
      list_back = g_slist_append(list_back, a);
   }
   else if (strcmp (key, "border_width") == 0) {
      Area *a = g_slist_last(list_back)->data;
      a->pix.border.width = atoi (value);
   }
   else if (strcmp (key, "background_color") == 0) {
      Area *a = g_slist_last(list_back)->data;
      extract_values(value, &value1, &value2);
      get_color (value1, a->pix.back.color);
      if (value2) a->pix.back.alpha = (atoi (value2) / 100.0);
      else a->pix.back.alpha = 0.5;
   }
   else if (strcmp (key, "border_color") == 0) {
      Area *a = g_slist_last(list_back)->data;
      extract_values(value, &value1, &value2);
      get_color (value1, a->pix.border.color);
      if (value2) a->pix.border.alpha = (atoi (value2) / 100.0);
      else a->pix.border.alpha = 0.5;
   }

   /* Panel */
   else if (strcmp (key, "panel_monitor") == 0) {
      panel.monitor = atoi (value);
      if (panel.monitor > 0) panel.monitor -= 1;
   }
   else if (strcmp (key, "panel_size") == 0) {
      extract_values(value, &value1, &value2);
      panel.area.width = atoi (value1);
      if (value2) panel.area.height = atoi (value2);
   }
   else if (strcmp (key, "panel_margin") == 0) {
      extract_values(value, &value1, &value2);
      panel.marginx = atoi (value1);
      if (value2) panel.marginy = atoi (value2);
   }
   else if (strcmp (key, "panel_padding") == 0) {
      extract_values(value, &value1, &value2);
      panel.area.paddingx = atoi (value1);
      if (value2) panel.area.paddingy = atoi (value2);
   }
   else if (strcmp (key, "panel_position") == 0) {
      extract_values(value, &value1, &value2);
      if (strcmp (value1, "top") == 0) panel.position = TOP;
      else panel.position = BOTTOM;

      if (!value2) panel.position = CENTER;
      else {
         if (strcmp (value2, "left") == 0) panel.position |= LEFT;
         else {
            if (strcmp (value2, "right") == 0) panel.position |= RIGHT;
            else panel.position |= CENTER;
         }
      }
   }
   else if (strcmp (key, "font_shadow") == 0)
      g_task.font_shadow = atoi (value);
   else if (strcmp (key, "panel_background_id") == 0) {
      int id = atoi (value);
      Area *a = g_slist_nth_data(list_back, id);
      memcpy(&panel.area.pix.back, &a->pix.back, sizeof(Color));
      memcpy(&panel.area.pix.border, &a->pix.border, sizeof(Border));
   }

   /* Clock */
   else if (strcmp (key, "time1_format") == 0) {
      if (panel.clock.time1_format) g_free(panel.clock.time1_format);
      if (strlen(value) > 0) panel.clock.time1_format = strdup (value);
      else panel.clock.time1_format = 0;
   }
   else if (strcmp (key, "time2_format") == 0) {
      if (panel.clock.time2_format) g_free(panel.clock.time2_format);
      if (strlen(value) > 0) panel.clock.time2_format = strdup (value);
      else panel.clock.time2_format = 0;
   }
   else if (strcmp (key, "time1_font") == 0) {
      if (panel.clock.time1_font_desc) pango_font_description_free(panel.clock.time1_font_desc);
      panel.clock.time1_font_desc = pango_font_description_from_string (value);
   }
   else if (strcmp (key, "time2_font") == 0) {
      if (panel.clock.time2_font_desc) pango_font_description_free(panel.clock.time2_font_desc);
      panel.clock.time2_font_desc = pango_font_description_from_string (value);
   }
   else if (strcmp (key, "clock_font_color") == 0) {
      extract_values(value, &value1, &value2);
      get_color (value1, panel.clock.font.color);
      if (value2) panel.clock.font.alpha = (atoi (value2) / 100.0);
      else panel.clock.font.alpha = 0.1;
   }
   else if (strcmp (key, "clock_padding") == 0) {
      extract_values(value, &value1, &value2);
      panel.clock.area.paddingx = atoi (value1);
      if (value2) panel.clock.area.paddingy = atoi (value2);
   }
   else if (strcmp (key, "clock_background_id") == 0) {
      int id = atoi (value);
      Area *a = g_slist_nth_data(list_back, id);
      memcpy(&panel.clock.area.pix.back, &a->pix.back, sizeof(Color));
      memcpy(&panel.clock.area.pix.border, &a->pix.border, sizeof(Border));
   }

   /* Taskbar */
   else if (strcmp (key, "taskbar_mode") == 0) {
      if (strcmp (value, "multi_desktop") == 0) panel.mode = MULTI_DESKTOP;
      else if (strcmp (value, "multi_monitor") == 0) panel.mode = MULTI_MONITOR;
      else panel.mode = SINGLE_DESKTOP;
   }
   else if (strcmp (key, "taskbar_padding") == 0) {
      extract_values(value, &value1, &value2);
      g_taskbar.paddingx = atoi (value1);
      if (value2) g_taskbar.paddingy = atoi (value2);
   }
   else if (strcmp (key, "taskbar_background_id") == 0) {
      int id = atoi (value);
      Area *a = g_slist_nth_data(list_back, id);
      memcpy(&g_taskbar.pix.back, &a->pix.back, sizeof(Color));
      memcpy(&g_taskbar.pix.border, &a->pix.border, sizeof(Border));
   }

   /* Task */
   else if (strcmp (key, "task_text") == 0)
      g_task.text = atoi (value);
   else if (strcmp (key, "task_icon") == 0)
      g_task.icon = atoi (value);
   else if (strcmp (key, "task_centered") == 0)
      g_task.centered = atoi (value);
   else if (strcmp (key, "task_width") == 0)
      g_task.maximum_width = atoi (value);
   else if (strcmp (key, "task_padding") == 0) {
      extract_values(value, &value1, &value2);
      g_task.area.paddingx = atoi (value1);
      if (value2) {
         g_task.area.paddingy = atoi (value2);
      }
   }
   else if (strcmp (key, "task_font") == 0) {
      if (g_task.font_desc) pango_font_description_free(g_task.font_desc);
      g_task.font_desc = pango_font_description_from_string (value);
   }
   else if (strcmp (key, "task_font_color") == 0) {
      extract_values(value, &value1, &value2);
      get_color (value1, g_task.font.color);
      if (value2) g_task.font.alpha = (atoi (value2) / 100.0);
      else g_task.font.alpha = 0.1;
   }
   else if (strcmp (key, "task_active_font_color") == 0) {
      extract_values(value, &value1, &value2);
      get_color (value1, g_task.font_active.color);
      if (value2) g_task.font_active.alpha = (atoi (value2) / 100.0);
      else g_task.font_active.alpha = 0.1;
   }
   else if (strcmp (key, "task_background_id") == 0) {
      int id = atoi (value);
      Area *a = g_slist_nth_data(list_back, id);
      memcpy(&g_task.area.pix.back, &a->pix.back, sizeof(Color));
      memcpy(&g_task.area.pix.border, &a->pix.border, sizeof(Border));
   }
   else if (strcmp (key, "task_active_background_id") == 0) {
      int id = atoi (value);
      Area *a = g_slist_nth_data(list_back, id);
      memcpy(&g_task.area.pix_active.back, &a->pix.back, sizeof(Color));
      memcpy(&g_task.area.pix_active.border, &a->pix.border, sizeof(Border));
   }

   /* Mouse actions */
   else if (strcmp (key, "mouse_middle") == 0)
      get_action (value, &panel.mouse_middle);
   else if (strcmp (key, "mouse_right") == 0)
      get_action (value, &panel.mouse_right);
   else if (strcmp (key, "mouse_scroll_up") == 0)
      get_action (value, &panel.mouse_scroll_up);
   else if (strcmp (key, "mouse_scroll_down") == 0)
      get_action (value, &panel.mouse_scroll_down);


   /* Read old config for backward compatibility */
   else if (strcmp (key, "font") == 0) {
      panel.old_config_file = 1;
      if (g_task.font_desc) pango_font_description_free(g_task.font_desc);
      g_task.font_desc = pango_font_description_from_string (value);
      if (panel.old_task_font) free(panel.old_task_font);
      panel.old_task_font = strdup (value);
   }
   else if (strcmp (key, "font_color") == 0)
      get_color (value, g_task.font.color);
   else if (strcmp (key, "font_alpha") == 0)
      g_task.font.alpha = (atoi (value) / 100.0);
   else if (strcmp (key, "font_active_color") == 0)
      get_color (value, g_task.font_active.color);
   else if (strcmp (key, "font_active_alpha") == 0)
      g_task.font_active.alpha = (atoi (value) / 100.0);
   else if (strcmp (key, "panel_show_all_desktop") == 0) {
      if (atoi (value) == 0) panel.mode = SINGLE_DESKTOP;
      else panel.mode = MULTI_DESKTOP;
   }
   else if (strcmp (key, "panel_width") == 0)
      panel.area.width = atoi (value);
   else if (strcmp (key, "panel_height") == 0)
      panel.area.height = atoi (value);
   else if (strcmp (key, "panel_background") == 0)
      panel.old_panel_background = atoi (value);
   else if (strcmp (key, "panel_background_alpha") == 0)
      panel.area.pix.back.alpha = (atoi (value) / 100.0);
   else if (strcmp (key, "panel_border_alpha") == 0)
      panel.area.pix.border.alpha = (atoi (value) / 100.0);
   else if (strcmp (key, "task_icon") == 0)
      panel.old_task_icon = atoi (value);
   else if (strcmp (key, "task_background") == 0)
      panel.old_task_background = atoi (value);
   else if (strcmp (key, "task_background_alpha") == 0)
      g_task.area.pix.back.alpha = (atoi (value) / 100.0);
   else if (strcmp (key, "task_active_background_alpha") == 0)
      g_task.area.pix_active.back.alpha = (atoi (value) / 100.0);
   else if (strcmp (key, "task_border_alpha") == 0)
      g_task.area.pix.border.alpha = (atoi (value) / 100.0);
   else if (strcmp (key, "task_active_border_alpha") == 0)
      g_task.area.pix_active.border.alpha = (atoi (value) / 100.0);
   // disabled parameters
   else if (strcmp (key, "task_active_border_width") == 0) ;
   else if (strcmp (key, "task_active_rounded") == 0) ;

   else
      fprintf(stderr, "Invalid option: \"%s\", correct your config file\n", key);

   if (value1) free (value1);
   if (value2) free (value2);
}


int parse_line (const char *line)
{
   char *a, *b, *key, *value;

   /* Skip useless lines */
   if ((line[0] == '#') || (line[0] == '\n')) return 0;
   if (!(a = strchr (line, '='))) return 0;

   /* overwrite '=' with '\0' */
   a[0] = '\0';
   key = strdup (line);
   a++;

   /* overwrite '\n' with '\0' if '\n' present */
   if ((b = strchr (a, '\n'))) b[0] = '\0';

   value = strdup (a);

   g_strstrip(key);
   g_strstrip(value);

   add_entry (key, value);

   free (key);
   free (value);
   return 1;
}


void config_taskbar()
{
   int i, j;

   if (g_task.area.pix.border.rounded > g_task.area.height/2) {
      g_task.area.pix.border.rounded = g_task.area.height/2;
      g_task.area.pix_active.border.rounded = g_task.area.pix.border.rounded;
   }

   for (i=0 ; i < 15 ; i++) {
      server.nb_desktop = server_get_number_of_desktop ();
      if (server.nb_desktop > 0) break;
      sleep(1);
   }
   if (server.nb_desktop == 0) {
      server.nb_desktop = 1;
      fprintf(stderr, "tint2 warning : cannot found number of desktop.\n");
   }

   if (panel.taskbar) cleanup_taskbar();

   panel.nb_desktop = server.nb_desktop;
   if (panel.mode == MULTI_MONITOR) panel.nb_monitor = server.nb_monitor;
   else panel.nb_monitor = 1;
   panel.taskbar = calloc(panel.nb_desktop * panel.nb_monitor, sizeof(Taskbar));
   g_slist_free(panel.area.list);
   panel.area.list = 0;

   Taskbar *tskbar;
   for (i=0 ; i < panel.nb_desktop ; i++) {
      for (j=0 ; j < panel.nb_monitor ; j++) {
         tskbar = &panel.taskbar[index(i,j)];
         memcpy(&tskbar->area, &g_taskbar, sizeof(Area));
         tskbar->desktop = i;
         tskbar->monitor = j;

         // TODO: redefinir panel.area.list en fonction des objets visibles
         panel.area.list = g_slist_append(panel.area.list, tskbar);
      }
   }
   if (panel.clock.time1_format)
      panel.area.list = g_slist_append(panel.area.list, &panel.clock);

   //printf("taskbar (desktop x monitor) : (%d x %d)\n", panel.nb_desktop, panel.nb_monitor);
   resize_taskbar();
   task_refresh_tasklist ();
   panel.refresh = 1;
}


void config_finish ()
{
   int height_ink, height;

   if (panel.old_config_file) save_config();

   // get monitor's configuration
   get_monitors();

   if (panel.monitor > (server.nb_monitor-1)) {
      panel.sleep_mode = 1;
      printf("tint2 sleep and wait monitor %d.\n", panel.monitor+1);
   }
   else {
      panel.sleep_mode = 0;
      //printf("tint2 wake up on monitor %d\n", panel.monitor+1);
      if (!server.monitor[panel.monitor].width || !server.monitor[panel.monitor].height)
         fprintf(stderr, "tint2 error : invalid monitor size.\n");
   }

   if (!panel.area.width) panel.area.width = server.monitor[panel.monitor].width;

   // taskbar
   g_taskbar.posy = panel.area.pix.border.width + panel.area.paddingy;
   g_taskbar.height = panel.area.height - (2 * g_taskbar.posy);
   g_taskbar.redraw = 1;

   // task
   g_task.area.posy = g_taskbar.posy + g_taskbar.pix.border.width + g_taskbar.paddingy;
   g_task.area.height = panel.area.height - (2 * g_task.area.posy);
   g_task.area.use_active = 1;
   g_task.area.redraw = 1;

   if (!g_task.maximum_width)
      g_task.maximum_width = server.monitor[panel.monitor].width;

   if (panel.area.pix.border.rounded > panel.area.height/2)
      panel.area.pix.border.rounded = panel.area.height/2;

   // clock
   init_clock(&panel.clock, panel.area.height);

   // compute vertical position : text and icon
   get_text_size(g_task.font_desc, &height_ink, &height, panel.area.height, "TAjpg", 5);
   g_task.text_posy = (g_task.area.height - height) / 2.0;

   // add task_icon_size
   g_task.text_posx = g_task.area.paddingx + g_task.area.pix.border.width;
   if (g_task.icon) {
      g_task.icon_size1 = g_task.area.height - (2 * g_task.area.paddingy);
      g_task.text_posx += g_task.icon_size1;
      g_task.icon_posy = (g_task.area.height - g_task.icon_size1) / 2;
   }

   config_taskbar();
   visible_object();

   // cleanup background list
   GSList *l0;
   for (l0 = list_back; l0 ; l0 = l0->next) {
      free(l0->data);
   }
   g_slist_free(list_back);
   list_back = NULL;
}


int config_read ()
{
   const gchar * const * system_dirs;
   char *path1, *path2, *dir;
   gint i;

   // check tint2rc file according to XDG specification
   path1 = g_build_filename (g_get_user_config_dir(), "tint2", "tint2rc", NULL);
   if (!g_file_test (path1, G_FILE_TEST_EXISTS)) {

      path2 = 0;
      system_dirs = g_get_system_config_dirs();
      for (i = 0; system_dirs[i]; i++) {
         path2 = g_build_filename(system_dirs[i], "tint2", "tint2rc", NULL);

         if (g_file_test(path2, G_FILE_TEST_EXISTS)) break;
         g_free (path2);
         path2 = 0;
      }

      if (path2) {
         // copy file in user directory (path1)
         dir = g_build_filename (g_get_user_config_dir(), "tint2", NULL);
         if (!g_file_test (dir, G_FILE_TEST_IS_DIR)) g_mkdir(dir, 0777);
         g_free(dir);

         copy_file(path2, path1);
         g_free(path2);
      }
   }

   i = config_read_file (path1);
   g_free(path1);
   return i;
}


int config_read_file (const char *path)
{
   FILE *fp;
   char line[80];

   if ((fp = fopen(path, "r")) == NULL) return 0;

   while (fgets(line, sizeof(line), fp) != NULL)
      parse_line (line);

   fclose (fp);
   return 1;
}


void save_config ()
{
   fprintf(stderr, "tint2 warning : convert user's config file\n");
   panel.area.paddingx = panel.area.paddingy = panel.marginx;
   panel.marginx = panel.marginy = 0;

   if (panel.old_task_icon == 0) g_task.icon_size1 = 0;
   if (panel.old_panel_background == 0) panel.area.pix.back.alpha = 0;
   if (panel.old_task_background == 0) {
      g_task.area.pix.back.alpha = 0;
      g_task.area.pix_active.back.alpha = 0;
   }
   g_task.area.pix.border.rounded = g_task.area.pix.border.rounded / 2;
   g_task.area.pix_active.border.rounded = g_task.area.pix.border.rounded;
   panel.area.pix.border.rounded = panel.area.pix.border.rounded / 2;

   char *path;
   FILE *fp;

   path = g_build_filename (g_get_user_config_dir(), "tint2", "tint2rc", NULL);
   fp = fopen(path, "w");
   g_free(path);
   if (fp == NULL) return;

   fputs("#---------------------------------------------\n", fp);
   fputs("# TINT CONFIG FILE\n", fp);
   fputs("#---------------------------------------------\n\n", fp);
   fputs("#---------------------------------------------\n", fp);
   fputs("# PANEL\n", fp);
   fputs("#---------------------------------------------\n", fp);
   if (panel.mode == SINGLE_DESKTOP) fputs("panel_mode = single_desktop\n", fp);
   else fputs("panel_mode = multi_desktop\n", fp);
   fputs("panel_monitor = 1\n", fp);
   if (panel.position & BOTTOM) fputs("panel_position = bottom", fp);
   else fputs("panel_position = top", fp);
   if (panel.position & LEFT) fputs(" left\n", fp);
   else if (panel.position & RIGHT) fputs(" right\n", fp);
   else fputs(" center\n", fp);
   fprintf(fp, "panel_size = %d %d\n", panel.area.width, panel.area.height);
   fprintf(fp, "panel_margin = %d %d\n", panel.marginx, panel.marginy);
   fprintf(fp, "panel_padding = %d %d\n", panel.area.paddingx, panel.area.paddingy);
   fprintf(fp, "font_shadow = %d\n", g_task.font_shadow);

   fputs("\n#---------------------------------------------\n", fp);
   fputs("# PANEL BACKGROUND AND BORDER\n", fp);
   fputs("#---------------------------------------------\n", fp);
   fprintf(fp, "panel_rounded = %d\n", panel.area.pix.border.rounded);
   fprintf(fp, "panel_border_width = %d\n", panel.area.pix.border.width);
   fprintf(fp, "panel_background_color = #%02x%02x%02x %d\n", (int)(panel.area.pix.back.color[0]*255), (int)(panel.area.pix.back.color[1]*255), (int)(panel.area.pix.back.color[2]*255), (int)(panel.area.pix.back.alpha*100));
   fprintf(fp, "panel_border_color = #%02x%02x%02x %d\n", (int)(panel.area.pix.border.color[0]*255), (int)(panel.area.pix.border.color[1]*255), (int)(panel.area.pix.border.color[2]*255), (int)(panel.area.pix.border.alpha*100));

   fputs("\n#---------------------------------------------\n", fp);
   fputs("# TASKS\n", fp);
   fputs("#---------------------------------------------\n", fp);
   fprintf(fp, "task_centered = %d\n", g_task.centered);
   fprintf(fp, "task_width = %d\n", g_task.maximum_width);
   fprintf(fp, "task_padding = %d\n", g_task.area.paddingx);
   fprintf(fp, "task_icon = %d\n", g_task.icon);
   fprintf(fp, "task_font = %s\n", panel.old_task_font);
   fprintf(fp, "task_font_color = #%02x%02x%02x %d\n", (int)(g_task.font.color[0]*255), (int)(g_task.font.color[1]*255), (int)(g_task.font.color[2]*255), (int)(g_task.font.alpha*100));
   fprintf(fp, "task_active_font_color = #%02x%02x%02x %d\n", (int)(g_task.font_active.color[0]*255), (int)(g_task.font_active.color[1]*255), (int)(g_task.font_active.color[2]*255), (int)(g_task.font_active.alpha*100));

   fputs("\n#---------------------------------------------\n", fp);
   fputs("# TASK BACKGROUND AND BORDER\n", fp);
   fputs("#---------------------------------------------\n", fp);
   fprintf(fp, "task_rounded = %d\n", g_task.area.pix.border.rounded);
   fprintf(fp, "task_background_color = #%02x%02x%02x %d\n", (int)(g_task.area.pix.back.color[0]*255), (int)(g_task.area.pix.back.color[1]*255), (int)(g_task.area.pix.back.color[2]*255), (int)(g_task.area.pix.back.alpha*100));
   fprintf(fp, "task_active_background_color = #%02x%02x%02x %d\n", (int)(g_task.area.pix_active.back.color[0]*255), (int)(g_task.area.pix_active.back.color[1]*255), (int)(g_task.area.pix_active.back.color[2]*255), (int)(g_task.area.pix_active.back.alpha*100));
   fprintf(fp, "task_border_width = %d\n", g_task.area.pix.border.width);
   fprintf(fp, "task_border_color = #%02x%02x%02x %d\n", (int)(g_task.area.pix.border.color[0]*255), (int)(g_task.area.pix.border.color[1]*255), (int)(g_task.area.pix.border.color[2]*255), (int)(g_task.area.pix.border.alpha*100));
   fprintf(fp, "task_active_border_color = #%02x%02x%02x %d\n", (int)(g_task.area.pix_active.border.color[0]*255), (int)(g_task.area.pix_active.border.color[1]*255), (int)(g_task.area.pix_active.border.color[2]*255), (int)(g_task.area.pix_active.border.alpha*100));

   fputs("\n#---------------------------------------------\n", fp);
   fputs("# CLOCK\n", fp);
   fputs("#---------------------------------------------\n", fp);
   fputs("#time1_format = %H:%M\n", fp);
   fputs("time1_font = sans bold 8\n", fp);
   fputs("#time2_format = %A %d %B\n", fp);
   fputs("time2_font = sans 6\n", fp);
   fputs("clock_font_color = #ffffff 75\n", fp);

   fputs("\n#---------------------------------------------\n", fp);
   fputs("# MOUSE ACTION ON TASK\n", fp);
   fputs("#---------------------------------------------\n", fp);
   if (panel.mouse_middle == NONE) fputs("mouse_middle = none\n", fp);
   else if (panel.mouse_middle == CLOSE) fputs("mouse_middle = close\n", fp);
   else if (panel.mouse_middle == TOGGLE) fputs("mouse_middle = toggle\n", fp);
   else if (panel.mouse_middle == ICONIFY) fputs("mouse_middle = iconify\n", fp);
   else if (panel.mouse_middle == SHADE) fputs("mouse_middle = shade\n", fp);
   else fputs("mouse_middle = toggle_iconify\n", fp);

   if (panel.mouse_right == NONE) fputs("mouse_right = none\n", fp);
   else if (panel.mouse_right == CLOSE) fputs("mouse_right = close\n", fp);
   else if (panel.mouse_right == TOGGLE) fputs("mouse_right = toggle\n", fp);
   else if (panel.mouse_right == ICONIFY) fputs("mouse_right = iconify\n", fp);
   else if (panel.mouse_right == SHADE) fputs("mouse_right = shade\n", fp);
   else fputs("mouse_right = toggle_iconify\n", fp);

   if (panel.mouse_scroll_up == NONE) fputs("mouse_scroll_up = none\n", fp);
   else if (panel.mouse_scroll_up == CLOSE) fputs("mouse_scroll_up = close\n", fp);
   else if (panel.mouse_scroll_up == TOGGLE) fputs("mouse_scroll_up = toggle\n", fp);
   else if (panel.mouse_scroll_up == ICONIFY) fputs("mouse_scroll_up = iconify\n", fp);
   else if (panel.mouse_scroll_up == SHADE) fputs("mouse_scroll_up = shade\n", fp);
   else fputs("mouse_scroll_up = toggle_iconify\n", fp);

   if (panel.mouse_scroll_down == NONE) fputs("mouse_scroll_down = none\n", fp);
   else if (panel.mouse_scroll_down == CLOSE) fputs("mouse_scroll_down = close\n", fp);
   else if (panel.mouse_scroll_down == TOGGLE) fputs("mouse_scroll_down = toggle\n", fp);
   else if (panel.mouse_scroll_down == ICONIFY) fputs("mouse_scroll_down = iconify\n", fp);
   else if (panel.mouse_scroll_down == SHADE) fputs("mouse_scroll_down = shade\n", fp);
   else fputs("mouse_scroll_down = toggle_iconify\n", fp);

   fputs("\n\n", fp);
   fclose (fp);

   panel.old_config_file = 0;
}

