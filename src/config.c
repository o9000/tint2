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
#include "systraybar.h"
#include "clock.h"
#include "panel.h"
#include "config.h"
#include "window.h"

#ifdef ENABLE_BATTERY
#include "battery.h"
#endif

// global path
char *config_path = 0;
char *thumbnail_path = 0;

// --------------------------------------------------
// backward compatibility
static int save_file_config;
static int old_task_icon_size;
static char *old_task_font;
static char *old_time1_font;
static char *old_time2_font;
static Area *area_task, *area_task_active;

#ifdef ENABLE_BATTERY
static char *old_bat1_font;
static char *old_bat2_font;
#endif

// temporary panel
static Panel *panel_config = 0;

// temporary list of background
static GSList *list_back;


void init_config()
{
	cleanup_panel();

   // get monitor and desktop config
   get_monitors_and_desktops();

   // append full transparency background
   list_back = g_slist_append(0, calloc(1, sizeof(Area)));

	panel_config = calloc(1, sizeof(Panel));
	systray.sort = 1;
	// window manager's menu default value == false
	wm_menu = 0;
	max_tick_urgent = 7;
}


void cleanup_config()
{
	free(panel_config);
	panel_config = 0;

   // cleanup background list
   GSList *l0;
   for (l0 = list_back; l0 ; l0 = l0->next) {
      free(l0->data);
   }
   g_slist_free(list_back);
   list_back = NULL;
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


void extract_values (const char *value, char **value1, char **value2, char **value3)
{
   char *b=0, *c=0;

   if (*value1) free (*value1);
   if (*value2) free (*value2);
   if (*value3) free (*value3);

   if ((b = strchr (value, ' '))) {
      b[0] = '\0';
      b++;
	}
	else {
   	*value2 = 0;
   	*value3 = 0;
	}
	*value1 = strdup (value);
	g_strstrip(*value1);

	if (b) {
		if ((c = strchr (b, ' '))) {
			c[0] = '\0';
			c++;
		}
		else {
			c = 0;
			*value3 = 0;
		}
		*value2 = strdup (b);
		g_strstrip(*value2);
	}

	if (c) {
		*value3 = strdup (c);
		g_strstrip(*value3);
	}
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
   else if (strcmp (event, "maximize_restore") == 0)
      *action = MAXIMIZE_RESTORE;
   else if (strcmp (event, "desktop_left") == 0)
      *action = DESKTOP_LEFT;
   else if (strcmp (event, "desktop_right") == 0)
      *action = DESKTOP_RIGHT;
}


void add_entry (char *key, char *value)
{
   char *value1=0, *value2=0, *value3=0;

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
      extract_values(value, &value1, &value2, &value3);
      get_color (value1, a->pix.back.color);
      if (value2) a->pix.back.alpha = (atoi (value2) / 100.0);
      else a->pix.back.alpha = 0.5;
   }
   else if (strcmp (key, "border_color") == 0) {
      Area *a = g_slist_last(list_back)->data;
      extract_values(value, &value1, &value2, &value3);
      get_color (value1, a->pix.border.color);
      if (value2) a->pix.border.alpha = (atoi (value2) / 100.0);
      else a->pix.border.alpha = 0.5;
   }

   /* Panel */
   else if (strcmp (key, "panel_monitor") == 0) {
   	if (strcmp (value, "all") == 0) panel_config->monitor = -1;
   	else {
      	panel_config->monitor = atoi (value);
      	if (panel_config->monitor > 0) panel_config->monitor -= 1;
		}
   }
   else if (strcmp (key, "panel_size") == 0) {
      extract_values(value, &value1, &value2, &value3);

		char *b;
		if ((b = strchr (value1, '%'))) {
			b[0] = '\0';
	      panel_config->pourcentx = 1;
		}
      panel_config->initial_width = atof(value1);
      if (value2) {
			if ((b = strchr (value2, '%'))) {
				b[0] = '\0';
				panel_config->pourcenty = 1;
			}
      	panel_config->initial_height = atof(value2);
		}
   }
   else if (strcmp (key, "panel_margin") == 0) {
      extract_values(value, &value1, &value2, &value3);
      panel_config->marginx = atoi (value1);
      if (value2) panel_config->marginy = atoi (value2);
   }
   else if (strcmp (key, "panel_padding") == 0) {
      extract_values(value, &value1, &value2, &value3);
      panel_config->area.paddingxlr = panel_config->area.paddingx = atoi (value1);
      if (value2) panel_config->area.paddingy = atoi (value2);
      if (value3) panel_config->area.paddingx = atoi (value3);
   }
   else if (strcmp (key, "panel_position") == 0) {
      extract_values(value, &value1, &value2, &value3);
      if (strcmp (value1, "top") == 0) panel_position = TOP;
      else {
			if (strcmp (value1, "bottom") == 0) panel_position = BOTTOM;
			else panel_position = CENTER;
		}

      if (!value2) panel_position |= CENTER;
      else {
         if (strcmp (value2, "left") == 0) panel_position |= LEFT;
         else {
            if (strcmp (value2, "right") == 0) panel_position |= RIGHT;
            else panel_position |= CENTER;
         }
      }
      if (!value3) panel_horizontal = 1;
      else {
         if (strcmp (value3, "vertical") == 0) panel_horizontal = 0;
         else panel_horizontal = 1;
		}
   }
   else if (strcmp (key, "font_shadow") == 0)
      panel_config->g_task.font_shadow = atoi (value);
   else if (strcmp (key, "panel_background_id") == 0) {
      int id = atoi (value);
      Area *a = g_slist_nth_data(list_back, id);
      memcpy(&panel_config->area.pix.back, &a->pix.back, sizeof(Color));
      memcpy(&panel_config->area.pix.border, &a->pix.border, sizeof(Border));
   }
   else if (strcmp (key, "wm_menu") == 0)
      wm_menu = atoi (value);
   else if (strcmp (key, "urgent_nb_of_blink") == 0)
      max_tick_urgent = (atoi (value) * 2) + 1;

   /* Battery */
   else if (strcmp (key, "battery") == 0) {
#ifdef ENABLE_BATTERY
		if(atoi(value) == 1)
			panel_config->battery.area.on_screen = 1;
#else
		if(atoi(value) == 1)
			printf("tint2 is build without battery support\n");
#endif
   }
   else if (strcmp (key, "battery_low_status") == 0) {
#ifdef ENABLE_BATTERY
		battery_low_status = atoi(value);
		if(battery_low_status < 0 || battery_low_status > 100)
			battery_low_status = 0;
#endif
   }
   else if (strcmp (key, "battery_low_cmd") == 0) {
#ifdef ENABLE_BATTERY
      if (battery_low_cmd) g_free(battery_low_cmd);
      if (strlen(value) > 0) battery_low_cmd = strdup (value);
      else battery_low_cmd = 0;
#endif
   }
   else if (strcmp (key, "bat1_font") == 0) {
#ifdef ENABLE_BATTERY
   	if (save_file_config) old_bat1_font = strdup (value);
      if (bat1_font_desc) pango_font_description_free(bat1_font_desc);
      bat1_font_desc = pango_font_description_from_string (value);
#endif
   }
   else if (strcmp (key, "bat2_font") == 0) {
#ifdef ENABLE_BATTERY
   	if (save_file_config) old_bat2_font = strdup (value);
      if (bat2_font_desc) pango_font_description_free(bat2_font_desc);
      bat2_font_desc = pango_font_description_from_string (value);
#endif
   }
   else if (strcmp (key, "battery_font_color") == 0) {
#ifdef ENABLE_BATTERY
      extract_values(value, &value1, &value2, &value3);
      get_color (value1, panel_config->battery.font.color);
      if (value2) panel_config->battery.font.alpha = (atoi (value2) / 100.0);
      else panel_config->battery.font.alpha = 0.5;
#endif
   }
   else if (strcmp (key, "battery_padding") == 0) {
#ifdef ENABLE_BATTERY
      extract_values(value, &value1, &value2, &value3);
      panel_config->battery.area.paddingxlr = panel_config->battery.area.paddingx = atoi (value1);
      if (value2) panel_config->battery.area.paddingy = atoi (value2);
      if (value3) panel_config->battery.area.paddingx = atoi (value3);
#endif
   }
   else if (strcmp (key, "battery_background_id") == 0) {
#ifdef ENABLE_BATTERY
      int id = atoi (value);
      Area *a = g_slist_nth_data(list_back, id);
      memcpy(&panel_config->battery.area.pix.back, &a->pix.back, sizeof(Color));
      memcpy(&panel_config->battery.area.pix.border, &a->pix.border, sizeof(Border));
#endif
   }

   /* Clock */
   else if (strcmp (key, "time1_format") == 0) {
      if (time1_format) g_free(time1_format);
      if (strlen(value) > 0) {
      	time1_format = strdup (value);
	      panel_config->clock.area.on_screen = 1;
		}
      else {
      	time1_format = 0;
	      panel_config->clock.area.on_screen = 0;
		}
   }
   else if (strcmp (key, "time2_format") == 0) {
      if (time2_format) g_free(time2_format);
      if (strlen(value) > 0) time2_format = strdup (value);
      else time2_format = 0;
   }
   else if (strcmp (key, "time1_font") == 0) {
   	if (save_file_config) old_time1_font = strdup (value);
      if (time1_font_desc) pango_font_description_free(time1_font_desc);
      time1_font_desc = pango_font_description_from_string (value);
   }
   else if (strcmp (key, "time2_font") == 0) {
   	if (save_file_config) old_time2_font = strdup (value);
      if (time2_font_desc) pango_font_description_free(time2_font_desc);
      time2_font_desc = pango_font_description_from_string (value);
   }
   else if (strcmp (key, "clock_font_color") == 0) {
      extract_values(value, &value1, &value2, &value3);
      get_color (value1, panel_config->clock.font.color);
      if (value2) panel_config->clock.font.alpha = (atoi (value2) / 100.0);
      else panel_config->clock.font.alpha = 0.5;
   }
   else if (strcmp (key, "clock_padding") == 0) {
      extract_values(value, &value1, &value2, &value3);
      panel_config->clock.area.paddingxlr = panel_config->clock.area.paddingx = atoi (value1);
      if (value2) panel_config->clock.area.paddingy = atoi (value2);
      if (value3) panel_config->clock.area.paddingx = atoi (value3);
   }
   else if (strcmp (key, "clock_background_id") == 0) {
      int id = atoi (value);
      Area *a = g_slist_nth_data(list_back, id);
      memcpy(&panel_config->clock.area.pix.back, &a->pix.back, sizeof(Color));
      memcpy(&panel_config->clock.area.pix.border, &a->pix.border, sizeof(Border));
   }
	else if (strcmp(key, "clock_lclick_command") == 0) {
		if (clock_lclick_command) g_free(clock_lclick_command);
		if (strlen(value) > 0) clock_lclick_command = strdup(value);
		else clock_lclick_command = 0;
	}
	else if (strcmp(key, "clock_rclick_command") == 0) {
		if (clock_rclick_command) g_free(clock_rclick_command);
		if (strlen(value) > 0) clock_rclick_command = strdup(value);
		else clock_rclick_command = 0;
	}

   /* Taskbar */
   else if (strcmp (key, "taskbar_mode") == 0) {
      if (strcmp (value, "multi_desktop") == 0) panel_mode = MULTI_DESKTOP;
      else panel_mode = SINGLE_DESKTOP;
   }
   else if (strcmp (key, "taskbar_padding") == 0) {
      extract_values(value, &value1, &value2, &value3);
      panel_config->g_taskbar.paddingxlr = panel_config->g_taskbar.paddingx = atoi (value1);
      if (value2) panel_config->g_taskbar.paddingy = atoi (value2);
      if (value3) panel_config->g_taskbar.paddingx = atoi (value3);
   }
   else if (strcmp (key, "taskbar_background_id") == 0) {
      int id = atoi (value);
      Area *a = g_slist_nth_data(list_back, id);
      memcpy(&panel_config->g_taskbar.pix.back, &a->pix.back, sizeof(Color));
      memcpy(&panel_config->g_taskbar.pix.border, &a->pix.border, sizeof(Border));
   }

   /* Task */
   else if (strcmp (key, "task_text") == 0)
      panel_config->g_task.text = atoi (value);
   else if (strcmp (key, "task_icon") == 0)
      panel_config->g_task.icon = atoi (value);
   else if (strcmp (key, "task_centered") == 0)
      panel_config->g_task.centered = atoi (value);
   else if (strcmp (key, "task_width") == 0) {
		// old parameter : just for backward compatibility
      panel_config->g_task.maximum_width = atoi (value);
      panel_config->g_task.maximum_height = 30;
	}
   else if (strcmp (key, "task_maximum_size") == 0) {
      extract_values(value, &value1, &value2, &value3);
      panel_config->g_task.maximum_width = atoi (value1);
		panel_config->g_task.maximum_height = 30;
		if (value2)
      	panel_config->g_task.maximum_height = atoi (value2);
	}
   else if (strcmp (key, "task_padding") == 0) {
      extract_values(value, &value1, &value2, &value3);
      panel_config->g_task.area.paddingxlr = panel_config->g_task.area.paddingx = atoi (value1);
      if (value2) panel_config->g_task.area.paddingy = atoi (value2);
      if (value3) panel_config->g_task.area.paddingx = atoi (value3);
   }
   else if (strcmp (key, "task_font") == 0) {
   	if (save_file_config) old_task_font = strdup (value);
      if (panel_config->g_task.font_desc) pango_font_description_free(panel_config->g_task.font_desc);
      panel_config->g_task.font_desc = pango_font_description_from_string (value);
   }
   else if (strcmp (key, "task_font_color") == 0) {
      extract_values(value, &value1, &value2, &value3);
      get_color (value1, panel_config->g_task.font.color);
      if (value2) panel_config->g_task.font.alpha = (atoi (value2) / 100.0);
      else panel_config->g_task.font.alpha = 0.1;
   }
   else if (strcmp (key, "task_active_font_color") == 0) {
      extract_values(value, &value1, &value2, &value3);
      get_color (value1, panel_config->g_task.font_active.color);
      if (value2) panel_config->g_task.font_active.alpha = (atoi (value2) / 100.0);
      else panel_config->g_task.font_active.alpha = 0.1;
   }
   else if (strcmp (key, "task_icon_hsb") == 0) {
      extract_values(value, &value1, &value2, &value3);
		panel_config->g_task.hue = atoi(value1);
		panel_config->g_task.saturation = atoi(value2);
		panel_config->g_task.brightness = atoi(value3);
   }
   else if (strcmp (key, "task_active_icon_hsb") == 0) {
      extract_values(value, &value1, &value2, &value3);
		panel_config->g_task.hue_active = atoi(value1);
		panel_config->g_task.saturation_active = atoi(value2);
		panel_config->g_task.brightness_active = atoi(value3);
   }
   else if (strcmp (key, "task_background_id") == 0) {
      int id = atoi (value);
      Area *a = g_slist_nth_data(list_back, id);
      memcpy(&panel_config->g_task.area.pix.back, &a->pix.back, sizeof(Color));
      memcpy(&panel_config->g_task.area.pix.border, &a->pix.border, sizeof(Border));
   }
   else if (strcmp (key, "task_active_background_id") == 0) {
      int id = atoi (value);
      Area *a = g_slist_nth_data(list_back, id);
      memcpy(&panel_config->g_task.area.pix_active.back, &a->pix.back, sizeof(Color));
      memcpy(&panel_config->g_task.area.pix_active.border, &a->pix.border, sizeof(Border));
   }

   /* Systray */
   else if (strcmp (key, "systray_padding") == 0) {
      extract_values(value, &value1, &value2, &value3);
      systray.area.paddingxlr = systray.area.paddingx = atoi (value1);
      if (value2) systray.area.paddingy = atoi (value2);
      if (value3) systray.area.paddingx = atoi (value3);
      systray.area.on_screen = 1;
   }
   else if (strcmp (key, "systray_background_id") == 0) {
      int id = atoi (value);
      Area *a = g_slist_nth_data(list_back, id);
      memcpy(&systray.area.pix.back, &a->pix.back, sizeof(Color));
      memcpy(&systray.area.pix.border, &a->pix.border, sizeof(Border));
   }
	else if (strcmp(key, "systray_sort") == 0) {
		if (strcmp(value, "desc") == 0)
			systray.sort = -1;
		else
			systray.sort = 1;
	}

   /* Mouse actions */
   else if (strcmp (key, "mouse_middle") == 0)
      get_action (value, &mouse_middle);
   else if (strcmp (key, "mouse_right") == 0)
      get_action (value, &mouse_right);
   else if (strcmp (key, "mouse_scroll_up") == 0)
      get_action (value, &mouse_scroll_up);
   else if (strcmp (key, "mouse_scroll_down") == 0)
      get_action (value, &mouse_scroll_down);


   /* Read tint-0.6 config for backward compatibility */
   else if (strcmp (key, "panel_mode") == 0) {
		save_file_config = 1;
      if (strcmp (value, "single_desktop") == 0) panel_mode = SINGLE_DESKTOP;
      else panel_mode = MULTI_DESKTOP;
   }
   else if (strcmp (key, "panel_rounded") == 0) {
      Area *a = calloc(1, sizeof(Area));
      a->pix.border.rounded = atoi (value);
      list_back = g_slist_append(list_back, a);
   }
   else if (strcmp (key, "panel_border_width") == 0) {
      Area *a = g_slist_last(list_back)->data;
      a->pix.border.width = atoi (value);
   }
   else if (strcmp (key, "panel_background_color") == 0) {
      Area *a = g_slist_last(list_back)->data;
      extract_values(value, &value1, &value2, &value3);
      get_color (value1, a->pix.back.color);
      if (value2) a->pix.back.alpha = (atoi (value2) / 100.0);
      else a->pix.back.alpha = 0.5;
   }
   else if (strcmp (key, "panel_border_color") == 0) {
      Area *a = g_slist_last(list_back)->data;
      extract_values(value, &value1, &value2, &value3);
      get_color (value1, a->pix.border.color);
      if (value2) a->pix.border.alpha = (atoi (value2) / 100.0);
      else a->pix.border.alpha = 0.5;
   }
   else if (strcmp (key, "task_text_centered") == 0)
      panel_config->g_task.centered = atoi (value);
   else if (strcmp (key, "task_margin") == 0) {
      panel_config->g_taskbar.paddingxlr = 0;
      panel_config->g_taskbar.paddingx = atoi (value);
   }
   else if (strcmp (key, "task_icon_size") == 0)
      old_task_icon_size = atoi (value);
   else if (strcmp (key, "task_rounded") == 0) {
      area_task = calloc(1, sizeof(Area));
      area_task->pix.border.rounded = atoi (value);
      list_back = g_slist_append(list_back, area_task);

      area_task_active = calloc(1, sizeof(Area));
      area_task_active->pix.border.rounded = atoi (value);
      list_back = g_slist_append(list_back, area_task_active);
   }
   else if (strcmp (key, "task_background_color") == 0) {
      extract_values(value, &value1, &value2, &value3);
      get_color (value1, area_task->pix.back.color);
      if (value2) area_task->pix.back.alpha = (atoi (value2) / 100.0);
      else area_task->pix.back.alpha = 0.5;
   }
   else if (strcmp (key, "task_active_background_color") == 0) {
      extract_values(value, &value1, &value2, &value3);
      get_color (value1, area_task_active->pix.back.color);
      if (value2) area_task_active->pix.back.alpha = (atoi (value2) / 100.0);
      else area_task_active->pix.back.alpha = 0.5;
   }
   else if (strcmp (key, "task_border_width") == 0) {
      area_task->pix.border.width = atoi (value);
      area_task_active->pix.border.width = atoi (value);
   }
   else if (strcmp (key, "task_border_color") == 0) {
      extract_values(value, &value1, &value2, &value3);
      get_color (value1, area_task->pix.border.color);
      if (value2) area_task->pix.border.alpha = (atoi (value2) / 100.0);
      else area_task->pix.border.alpha = 0.5;
   }
   else if (strcmp (key, "task_active_border_color") == 0) {
      extract_values(value, &value1, &value2, &value3);
      get_color (value1, area_task_active->pix.border.color);
      if (value2) area_task_active->pix.border.alpha = (atoi (value2) / 100.0);
      else area_task_active->pix.border.alpha = 0.5;
   }

   else
      fprintf(stderr, "tint2 : invalid option \"%s\", correct your config file\n", key);

   if (value1) free (value1);
   if (value2) free (value2);
   if (value3) free (value3);
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


void config_finish ()
{
   if (panel_config->monitor > (server.nb_monitor-1)) {
		// server.nb_monitor minimum value is 1 (see get_monitors_and_desktops())
		// and panel_config->monitor is higher
		fprintf(stderr, "warning : monitor not found. tint2 default to monitor 1.\n");
		panel_config->monitor = 0;
   }

	// alloc panels
   int i;
   if (panel_config->monitor >= 0) {
   	// one monitor
	   nb_panel = 1;
	   panel1 = calloc(nb_panel, sizeof(Panel));
	   memcpy(panel1, panel_config, sizeof(Panel));
	   panel1->monitor = panel_config->monitor;
	}
	else {
   	// all monitors
	   nb_panel = server.nb_monitor;
	   panel1 = calloc(nb_panel, sizeof(Panel));

	   for (i=0 ; i < nb_panel ; i++) {
		   memcpy(&panel1[i], panel_config, sizeof(Panel));
		   panel1[i].monitor = i;
		}
	}

	// TODO: user can configure layout => ordered objects in panel.area.list
	// clock and systray before taskbar because resize(clock) can resize others object ??
   init_panel();
	init_clock();
#ifdef ENABLE_BATTERY
	init_battery();
#endif
	init_systray();
	init_taskbar();
	visible_object();

	cleanup_config();

	task_refresh_tasklist();
}


int config_read ()
{
   const gchar * const * system_dirs;
   char *path1;
   gint i;

	save_file_config = 0;

   // follow XDG specification
deb:
   // check tint2rc in user directory
   path1 = g_build_filename (g_get_user_config_dir(), "tint2", "tint2rc", NULL);
   if (g_file_test (path1, G_FILE_TEST_EXISTS)) {
		i = config_read_file (path1);
		config_path = strdup(path1);
		g_free(path1);
	   return i;
	}

	g_free(path1);
	if (save_file_config) {
		fprintf(stderr, "tint2 exit : enable to write $HOME/.config/tint2/tint2rc\n");
		exit(0);
	}

	// check old tintrc config file
	path1 = g_build_filename (g_get_user_config_dir(), "tint", "tintrc", NULL);
	if (g_file_test (path1, G_FILE_TEST_EXISTS)) {
		save_file_config = 1;
		config_read_file (path1);
		g_free(path1);
		goto deb;
	}

	// copy tint2rc from system directory to user directory
	g_free(path1);
	char *path2 = 0;
	system_dirs = g_get_system_config_dirs();
	for (i = 0; system_dirs[i]; i++) {
		path2 = g_build_filename(system_dirs[i], "tint2", "tint2rc", NULL);

		if (g_file_test(path2, G_FILE_TEST_EXISTS)) break;
		g_free (path2);
		path2 = 0;
	}

	if (path2) {
		// copy file in user directory (path1)
		char *dir = g_build_filename (g_get_user_config_dir(), "tint2", NULL);
		if (!g_file_test (dir, G_FILE_TEST_IS_DIR)) g_mkdir(dir, 0777);
		g_free(dir);

		path1 = g_build_filename (g_get_user_config_dir(), "tint2", "tint2rc", NULL);
		copy_file(path2, path1);
		g_free(path2);

		i = config_read_file (path1);
		config_path = strdup(path1);
		g_free(path1);
		return i;
	}
	return 0;
}


int config_read_file (const char *path)
{
   FILE *fp;
   char line[80];

   if ((fp = fopen(path, "r")) == NULL) return 0;
	old_task_font = 0;
	old_time1_font = 0;
	old_time2_font = 0;

   while (fgets(line, sizeof(line), fp) != NULL)
      parse_line (line);

   fclose (fp);

	if (save_file_config)
		save_config();

	if (old_task_font) {
		g_free(old_task_font);
		old_task_font = 0;
	}
	if (old_time1_font) {
		g_free(old_time1_font);
		old_time1_font = 0;
	}
	if (old_time2_font) {
		g_free(old_time2_font);
		old_time2_font = 0;
	}
   return 1;
}


void save_config ()
{
   fprintf(stderr, "tint2 : convert user's config file\n");

   char *path, *dir;
   FILE *fp;

	if (old_task_icon_size) {
		panel_config->g_task.area.paddingy = ((int)panel_config->initial_height - (2 * panel_config->area.paddingy) - old_task_icon_size) / 2;
	}

   dir = g_build_filename (g_get_user_config_dir(), "tint2", NULL);
	if (!g_file_test (dir, G_FILE_TEST_IS_DIR)) g_mkdir(dir, 0777);
	g_free(dir);

   path = g_build_filename (g_get_user_config_dir(), "tint2", "tint2rc", NULL);
   fp = fopen(path, "w");
   g_free(path);
   if (fp == NULL) return;

   fputs("#---------------------------------------------\n", fp);
   fputs("# TINT2 CONFIG FILE\n", fp);
   fputs("#---------------------------------------------\n\n", fp);
   fputs("#---------------------------------------------\n", fp);
   fputs("# BACKGROUND AND BORDER\n", fp);
   fputs("#---------------------------------------------\n", fp);
   GSList *l0;
   Area *a;
   l0 = list_back->next;
   while (l0) {
      a = l0->data;
		fprintf(fp, "rounded = %d\n", a->pix.border.rounded);
		fprintf(fp, "border_width = %d\n", a->pix.border.width);
		fprintf(fp, "background_color = #%02x%02x%02x %d\n", (int)(a->pix.back.color[0]*255), (int)(a->pix.back.color[1]*255), (int)(a->pix.back.color[2]*255), (int)(a->pix.back.alpha*100));
		fprintf(fp, "border_color = #%02x%02x%02x %d\n\n", (int)(a->pix.border.color[0]*255), (int)(a->pix.border.color[1]*255), (int)(a->pix.border.color[2]*255), (int)(a->pix.border.alpha*100));

   	l0 = l0->next;
   }

   fputs("#---------------------------------------------\n", fp);
   fputs("# PANEL\n", fp);
   fputs("#---------------------------------------------\n", fp);
   fputs("panel_monitor = all\n", fp);
   if (panel_position & BOTTOM) fputs("panel_position = bottom", fp);
   else fputs("panel_position = top", fp);
   if (panel_position & LEFT) fputs(" left horizontal\n", fp);
   else if (panel_position & RIGHT) fputs(" right horizontal\n", fp);
   else fputs(" center horizontal\n", fp);
	fprintf(fp, "panel_size = %d %d\n", (int)panel_config->initial_width, (int)panel_config->initial_height);
	fprintf(fp, "panel_margin = %d %d\n", panel_config->marginx, panel_config->marginy);
   fprintf(fp, "panel_padding = %d %d %d\n", panel_config->area.paddingxlr, panel_config->area.paddingy, panel_config->area.paddingx);
   fprintf(fp, "font_shadow = %d\n", panel_config->g_task.font_shadow);
   fputs("panel_background_id = 1\n", fp);
   fputs("wm_menu = 0\n", fp);

   fputs("\n#---------------------------------------------\n", fp);
   fputs("# TASKBAR\n", fp);
   fputs("#---------------------------------------------\n", fp);
   if (panel_mode == MULTI_DESKTOP) fputs("taskbar_mode = multi_desktop\n", fp);
   else fputs("taskbar_mode = single_desktop\n", fp);
   fprintf(fp, "taskbar_padding = 0 0 %d\n", panel_config->g_taskbar.paddingx);
   fputs("taskbar_background_id = 0\n", fp);

   fputs("\n#---------------------------------------------\n", fp);
   fputs("# TASK\n", fp);
   fputs("#---------------------------------------------\n", fp);
	if (old_task_icon_size) fputs("task_icon = 1\n", fp);
	else fputs("task_icon = 0\n", fp);
	fputs("task_text = 1\n", fp);
   fprintf(fp, "task_maximum_size = %d %d\n", panel_config->g_task.maximum_width, panel_config->g_task.maximum_height);
   fprintf(fp, "task_centered = %d\n", panel_config->g_task.centered);
   fprintf(fp, "task_padding = %d %d\n", panel_config->g_task.area.paddingx, panel_config->g_task.area.paddingy);
   fprintf(fp, "task_font = %s\n", old_task_font);
	fprintf(fp, "task_font_color = #%02x%02x%02x %d\n", (int)(panel_config->g_task.font.color[0]*255), (int)(panel_config->g_task.font.color[1]*255), (int)(panel_config->g_task.font.color[2]*255), (int)(panel_config->g_task.font.alpha*100));
	fprintf(fp, "task_active_font_color = #%02x%02x%02x %d\n", (int)(panel_config->g_task.font_active.color[0]*255), (int)(panel_config->g_task.font_active.color[1]*255), (int)(panel_config->g_task.font_active.color[2]*255), (int)(panel_config->g_task.font_active.alpha*100));
   fputs("task_background_id = 2\n", fp);
   fputs("task_active_background_id = 3\n", fp);

   fputs("\n#---------------------------------------------\n", fp);
   fputs("# SYSTRAYBAR\n", fp);
   fputs("#---------------------------------------------\n", fp);
   fputs("systray_padding = 4 3 4\n", fp);
   fputs("systray_background_id = 0\n", fp);

   fputs("\n#---------------------------------------------\n", fp);
   fputs("# CLOCK\n", fp);
   fputs("#---------------------------------------------\n", fp);
	if (time1_format) fprintf(fp, "time1_format = %s\n", time1_format);
	else fputs("#time1_format = %H:%M\n", fp);
   fprintf(fp, "time1_font = %s\n", old_time1_font);
	if (time2_format) fprintf(fp, "time2_format = %s\n", time2_format);
	else fputs("#time2_format = %A %d %B\n", fp);
   fprintf(fp, "time2_font = %s\n", old_time2_font);
	fprintf(fp, "clock_font_color = #%02x%02x%02x %d\n", (int)(panel_config->clock.font.color[0]*255), (int)(panel_config->clock.font.color[1]*255), (int)(panel_config->clock.font.color[2]*255), (int)(panel_config->clock.font.alpha*100));
   fputs("clock_padding = 2 2\n", fp);
   fputs("clock_background_id = 0\n", fp);
   fputs("#clock_lclick_command = xclock\n", fp);
   fputs("clock_rclick_command = orage\n", fp);

#ifdef ENABLE_BATTERY
	fputs("\n#---------------------------------------------\n", fp);
	fputs("# BATTERY\n", fp);
	fputs("#---------------------------------------------\n", fp);
	fprintf(fp, "battery = %d\n", panel_config->battery.area.on_screen);
	fprintf(fp, "battery_low_status = %d\n", battery_low_status);
	fprintf(fp, "battery_low_cmd = %s\n", battery_low_cmd);
	fprintf(fp, "bat1_font = %s\n", old_bat1_font);
	fprintf(fp, "bat2_font = %s\n", old_bat2_font);
	fprintf(fp, "battery_font_color = #%02x%02x%02x %d\n", (int)(panel_config->battery.font.color[0]*255), (int)(panel_config->battery.font.color[1]*255), (int)(panel_config->battery.font.color[2]*255), (int)(panel_config->battery.font.alpha*100));
	fputs("battery_padding = 2 2\n", fp);
	fputs("battery_background_id = 0\n", fp);
#endif

   fputs("\n#---------------------------------------------\n", fp);
   fputs("# MOUSE ACTION ON TASK\n", fp);
   fputs("#---------------------------------------------\n", fp);
   if (mouse_middle == NONE) fputs("mouse_middle = none\n", fp);
   else if (mouse_middle == CLOSE) fputs("mouse_middle = close\n", fp);
   else if (mouse_middle == TOGGLE) fputs("mouse_middle = toggle\n", fp);
   else if (mouse_middle == ICONIFY) fputs("mouse_middle = iconify\n", fp);
   else if (mouse_middle == SHADE) fputs("mouse_middle = shade\n", fp);
   else fputs("mouse_middle = toggle_iconify\n", fp);

   if (mouse_right == NONE) fputs("mouse_right = none\n", fp);
   else if (mouse_right == CLOSE) fputs("mouse_right = close\n", fp);
   else if (mouse_right == TOGGLE) fputs("mouse_right = toggle\n", fp);
   else if (mouse_right == ICONIFY) fputs("mouse_right = iconify\n", fp);
   else if (mouse_right == SHADE) fputs("mouse_right = shade\n", fp);
   else fputs("mouse_right = toggle_iconify\n", fp);

   if (mouse_scroll_up == NONE) fputs("mouse_scroll_up = none\n", fp);
   else if (mouse_scroll_up == CLOSE) fputs("mouse_scroll_up = close\n", fp);
   else if (mouse_scroll_up == TOGGLE) fputs("mouse_scroll_up = toggle\n", fp);
   else if (mouse_scroll_up == ICONIFY) fputs("mouse_scroll_up = iconify\n", fp);
   else if (mouse_scroll_up == SHADE) fputs("mouse_scroll_up = shade\n", fp);
   else fputs("mouse_scroll_up = toggle_iconify\n", fp);

   if (mouse_scroll_down == NONE) fputs("mouse_scroll_down = none\n", fp);
   else if (mouse_scroll_down == CLOSE) fputs("mouse_scroll_down = close\n", fp);
   else if (mouse_scroll_down == TOGGLE) fputs("mouse_scroll_down = toggle\n", fp);
   else if (mouse_scroll_down == ICONIFY) fputs("mouse_scroll_down = iconify\n", fp);
   else if (mouse_scroll_down == SHADE) fputs("mouse_scroll_down = shade\n", fp);
   else fputs("mouse_scroll_down = toggle_iconify\n", fp);

   fputs("\n\n", fp);
   fclose (fp);
}

