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
#include <pango/pangoxft.h>
#include <Imlib2.h>

#include "common.h"
#include "server.h"
#include "panel.h"
#include "task.h"
#include "taskbar.h"
#include "systraybar.h"
#include "clock.h"
#include "config.h"
#include "window.h"
#include "tooltip.h"

#ifdef ENABLE_BATTERY
#include "battery.h"
#endif

// global path
char *config_path = 0;
char *snapshot_path = 0;

// --------------------------------------------------
// backward compatibility
static int old_task_icon_size;
static Area *area_task;
static Area *area_task_active;
// detect if it's an old config file
// ==1
static int old_config_file;

// temporary list of background
static GSList *list_back;



void init_config()
{
	// append full transparency background
	list_back = g_slist_append(0, calloc(1, sizeof(Area)));

	// tint2 could reload config, so we cleanup objects
	cleanup_systray();
#ifdef ENABLE_BATTERY
	cleanup_battery();
#endif
	cleanup_clock();
	cleanup_tooltip();

	// panel's default value
	if (panel_config.g_task.font_desc) {
		pango_font_description_free(panel_config.g_task.font_desc);
	}
	memset(&panel_config, 0, sizeof(Panel));
	panel_config.g_task.alpha = 100;
	panel_config.g_task.alpha_active = 100;
	systray.sort = 3;
	old_config_file = 1;

	// window manager's menu default value == false
	wm_menu = 0;
	max_tick_urgent = 7;

	// flush pango cache if possible
	//pango_xft_shutdown_display(server.dsp, server.screen);
	//PangoFontMap *font_map = pango_xft_get_font_map(server.dsp, server.screen);
	//pango_fc_font_map_shutdown(font_map);
}


void cleanup_config()
{
	// cleanup background list
	GSList *l0;
	for (l0 = list_back; l0 ; l0 = l0->next) {
		free(l0->data);
	}
	g_slist_free(list_back);
	list_back = NULL;
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
	else if (strcmp (event, "next_task") == 0)
		*action = NEXT_TASK;
	else if (strcmp (event, "prev_task") == 0)
		*action = PREV_TASK;
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
		if (strcmp (value, "all") == 0) panel_config.monitor = -1;
		else {
			panel_config.monitor = atoi (value);
			if (panel_config.monitor > 0) panel_config.monitor -= 1;
		}
		if (panel_config.monitor > (server.nb_monitor-1)) {
			// server.nb_monitor minimum value is 1 (see get_monitors())
			fprintf(stderr, "warning : monitor not found. tint2 default to all monitors.\n");
			panel_config.monitor = 0;
		}
	}
	else if (strcmp (key, "panel_size") == 0) {
		extract_values(value, &value1, &value2, &value3);

		char *b;
		if ((b = strchr (value1, '%'))) {
			b[0] = '\0';
			panel_config.pourcentx = 1;
		}
		panel_config.area.width = atoi(value1);
		if (panel_config.area.width == 0) {
			// full width mode
			panel_config.area.width = 100;
			panel_config.pourcentx = 1;
		}
		if (value2) {
			if ((b = strchr (value2, '%'))) {
				b[0] = '\0';
				panel_config.pourcenty = 1;
			}
			panel_config.area.height = atoi(value2);
		}
	}
	else if (strcmp (key, "panel_margin") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.marginx = atoi (value1);
		if (value2) panel_config.marginy = atoi (value2);
	}
	else if (strcmp (key, "panel_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.area.paddingxlr = panel_config.area.paddingx = atoi (value1);
		if (value2) panel_config.area.paddingy = atoi (value2);
		if (value3) panel_config.area.paddingx = atoi (value3);
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
		panel_config.g_task.font_shadow = atoi (value);
	else if (strcmp (key, "panel_background_id") == 0) {
		int id = atoi (value);
		Area *a = g_slist_nth_data(list_back, id);
		memcpy(&panel_config.area.pix.back, &a->pix.back, sizeof(Color));
		memcpy(&panel_config.area.pix.border, &a->pix.border, sizeof(Border));
	}
	else if (strcmp (key, "wm_menu") == 0)
		wm_menu = atoi (value);
	else if (strcmp (key, "panel_dock") == 0)
		panel_dock = atoi (value);
	else if (strcmp (key, "urgent_nb_of_blink") == 0)
		max_tick_urgent = (atoi (value) * 2) + 1;

	/* Battery */
	else if (strcmp (key, "battery") == 0) {
#ifdef ENABLE_BATTERY
		if(atoi(value) == 1)
			battery_enabled = 1;
#else
		if(atoi(value) == 1)
			fprintf(stderr, "tint2 is build without battery support\n");
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
		if (strlen(value) > 0)
			battery_low_cmd = strdup (value);
#endif
	}
	else if (strcmp (key, "bat1_font") == 0) {
#ifdef ENABLE_BATTERY
		bat1_font_desc = pango_font_description_from_string (value);
#endif
	}
	else if (strcmp (key, "bat2_font") == 0) {
#ifdef ENABLE_BATTERY
		bat2_font_desc = pango_font_description_from_string (value);
#endif
	}
	else if (strcmp (key, "battery_font_color") == 0) {
#ifdef ENABLE_BATTERY
		extract_values(value, &value1, &value2, &value3);
		get_color (value1, panel_config.battery.font.color);
		if (value2) panel_config.battery.font.alpha = (atoi (value2) / 100.0);
		else panel_config.battery.font.alpha = 0.5;
#endif
	}
	else if (strcmp (key, "battery_padding") == 0) {
#ifdef ENABLE_BATTERY
		extract_values(value, &value1, &value2, &value3);
		panel_config.battery.area.paddingxlr = panel_config.battery.area.paddingx = atoi (value1);
		if (value2) panel_config.battery.area.paddingy = atoi (value2);
		if (value3) panel_config.battery.area.paddingx = atoi (value3);
#endif
	}
	else if (strcmp (key, "battery_background_id") == 0) {
#ifdef ENABLE_BATTERY
		int id = atoi (value);
		Area *a = g_slist_nth_data(list_back, id);
		memcpy(&panel_config.battery.area.pix.back, &a->pix.back, sizeof(Color));
		memcpy(&panel_config.battery.area.pix.border, &a->pix.border, sizeof(Border));
#endif
	}

	/* Clock */
	else if (strcmp (key, "time1_format") == 0) {
		if (strlen(value) > 0) {
			time1_format = strdup (value);
			clock_enabled = 1;
		}
	}
	else if (strcmp (key, "time2_format") == 0) {
		if (strlen(value) > 0)
			time2_format = strdup (value);
	}
	else if (strcmp (key, "time1_font") == 0) {
		time1_font_desc = pango_font_description_from_string (value);
	}
	else if (strcmp (key, "time2_font") == 0) {
		time2_font_desc = pango_font_description_from_string (value);
	}
	else if (strcmp (key, "clock_font_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		get_color (value1, panel_config.clock.font.color);
		if (value2) panel_config.clock.font.alpha = (atoi (value2) / 100.0);
		else panel_config.clock.font.alpha = 0.5;
	}
	else if (strcmp (key, "clock_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.clock.area.paddingxlr = panel_config.clock.area.paddingx = atoi (value1);
		if (value2) panel_config.clock.area.paddingy = atoi (value2);
		if (value3) panel_config.clock.area.paddingx = atoi (value3);
	}
	else if (strcmp (key, "clock_background_id") == 0) {
		int id = atoi (value);
		Area *a = g_slist_nth_data(list_back, id);
		memcpy(&panel_config.clock.area.pix.back, &a->pix.back, sizeof(Color));
		memcpy(&panel_config.clock.area.pix.border, &a->pix.border, sizeof(Border));
	}
	else if (strcmp(key, "clock_tooltip") == 0) {
		if (strlen(value) > 0)
			time_tooltip_format = strdup (value);
	}
	else if (strcmp(key, "clock_lclick_command") == 0) {
		if (strlen(value) > 0)
			clock_lclick_command = strdup(value);
	}
	else if (strcmp(key, "clock_rclick_command") == 0) {
		if (strlen(value) > 0)
			clock_rclick_command = strdup(value);
	}

	/* Taskbar */
	else if (strcmp (key, "taskbar_mode") == 0) {
		if (strcmp (value, "multi_desktop") == 0) panel_mode = MULTI_DESKTOP;
		else panel_mode = SINGLE_DESKTOP;
	}
	else if (strcmp (key, "taskbar_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.g_taskbar.paddingxlr = panel_config.g_taskbar.paddingx = atoi (value1);
		if (value2) panel_config.g_taskbar.paddingy = atoi (value2);
		if (value3) panel_config.g_taskbar.paddingx = atoi (value3);
	}
	else if (strcmp (key, "taskbar_background_id") == 0) {
		int id = atoi (value);
		Area *a = g_slist_nth_data(list_back, id);
		memcpy(&panel_config.g_taskbar.pix.back, &a->pix.back, sizeof(Color));
		memcpy(&panel_config.g_taskbar.pix.border, &a->pix.border, sizeof(Border));
	}
	else if (strcmp (key, "taskbar_active_background_id") == 0) {
		int id = atoi (value);
		Area *a = g_slist_nth_data(list_back, id);
		memcpy(&panel_config.g_taskbar.pix_active.back, &a->pix.back, sizeof(Color));
		memcpy(&panel_config.g_taskbar.pix_active.border, &a->pix.border, sizeof(Border));
		panel_config.g_taskbar.use_active = 1;
	}

	/* Task */
	else if (strcmp (key, "task_text") == 0)
		panel_config.g_task.text = atoi (value);
	else if (strcmp (key, "task_icon") == 0)
		panel_config.g_task.icon = atoi (value);
	else if (strcmp (key, "task_centered") == 0)
		panel_config.g_task.centered = atoi (value);
	else if (strcmp (key, "task_width") == 0) {
		// old parameter : just for backward compatibility
		panel_config.g_task.maximum_width = atoi (value);
		panel_config.g_task.maximum_height = 30;
	}
	else if (strcmp (key, "task_maximum_size") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.g_task.maximum_width = atoi (value1);
		panel_config.g_task.maximum_height = 30;
		if (value2)
			panel_config.g_task.maximum_height = atoi (value2);
	}
	else if (strcmp (key, "task_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.g_task.area.paddingxlr = panel_config.g_task.area.paddingx = atoi (value1);
		if (value2) panel_config.g_task.area.paddingy = atoi (value2);
		if (value3) panel_config.g_task.area.paddingx = atoi (value3);
	}
	else if (strcmp (key, "task_font") == 0) {
		panel_config.g_task.font_desc = pango_font_description_from_string (value);
	}
	else if (strcmp (key, "task_font_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		get_color (value1, panel_config.g_task.font.color);
		if (value2) panel_config.g_task.font.alpha = (atoi (value2) / 100.0);
		else panel_config.g_task.font.alpha = 0.1;
	}
	else if (strcmp (key, "task_active_font_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		get_color (value1, panel_config.g_task.font_active.color);
		if (value2) panel_config.g_task.font_active.alpha = (atoi (value2) / 100.0);
		else panel_config.g_task.font_active.alpha = 0.1;
	}
	else if (strcmp (key, "task_icon_asb") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.g_task.alpha = atoi(value1);
		panel_config.g_task.saturation = atoi(value2);
		panel_config.g_task.brightness = atoi(value3);
	}
	else if (strcmp (key, "task_active_icon_asb") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.g_task.alpha_active = atoi(value1);
		panel_config.g_task.saturation_active = atoi(value2);
		panel_config.g_task.brightness_active = atoi(value3);
	}
	else if (strcmp (key, "task_background_id") == 0) {
		int id = atoi (value);
		Area *a = g_slist_nth_data(list_back, id);
		memcpy(&panel_config.g_task.area.pix.back, &a->pix.back, sizeof(Color));
		memcpy(&panel_config.g_task.area.pix.border, &a->pix.border, sizeof(Border));
	}
	else if (strcmp (key, "task_active_background_id") == 0) {
		int id = atoi (value);
		Area *a = g_slist_nth_data(list_back, id);
		memcpy(&panel_config.g_task.area.pix_active.back, &a->pix.back, sizeof(Color));
		memcpy(&panel_config.g_task.area.pix_active.border, &a->pix.border, sizeof(Border));
	}

	/* Systray */
	else if (strcmp (key, "systray") == 0) {
		systray_enabled = atoi(value);
		// systray is latest option added. files without 'systray' are old.
		old_config_file = 0;
	}
	else if (strcmp (key, "systray_padding") == 0) {
		if (old_config_file)
			systray_enabled = 1;
		extract_values(value, &value1, &value2, &value3);
		systray.area.paddingxlr = systray.area.paddingx = atoi (value1);
		if (value2) systray.area.paddingy = atoi (value2);
		if (value3) systray.area.paddingx = atoi (value3);
	}
	else if (strcmp (key, "systray_background_id") == 0) {
		int id = atoi (value);
		Area *a = g_slist_nth_data(list_back, id);
		memcpy(&systray.area.pix.back, &a->pix.back, sizeof(Color));
		memcpy(&systray.area.pix.border, &a->pix.border, sizeof(Border));
	}
	else if (strcmp(key, "systray_sort") == 0) {
		if (strcmp(value, "descending") == 0)
			systray.sort = -1;
		else if (strcmp(value, "ascending") == 0)
			systray.sort = 1;
		else if (strcmp(value, "left2right") == 0)
			systray.sort = 2;
		else  if (strcmp(value, "right2left") == 0)
			systray.sort = 3;
	}

	/* Tooltip */
	else if (strcmp (key, "tooltip") == 0)
		g_tooltip.enabled = atoi(value);
	else if (strcmp (key, "tooltip_show_timeout") == 0) {
		double timeout = atof(value);
		int sec = (int)timeout;
		int nsec = (timeout-sec)*1e9;
		if (nsec < 0)  // can happen because of double is not precise such that (sec > timeout)==TRUE
			nsec = 0;
		g_tooltip.show_timeout = (struct timespec){.tv_sec=sec, .tv_nsec=nsec};
	}
	else if (strcmp (key, "tooltip_hide_timeout") == 0) {
		double timeout = atof(value);
		int sec = (int)timeout;
		int nsec = (timeout-sec)*1e9;
		if (nsec < 0)  // can happen because of double is not precise such that (sec > timeout)==TRUE
			nsec = 0;
		g_tooltip.hide_timeout = (struct timespec){.tv_sec=sec, .tv_nsec=nsec};
	}
	else if (strcmp (key, "tooltip_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		if (value1) g_tooltip.paddingx = atoi(value1);
		if (value2) g_tooltip.paddingy = atoi(value2);
	}
	else if (strcmp (key, "tooltip_background_id") == 0) {
		int id = atoi (value);
		Area *a = g_slist_nth_data(list_back, id);
		memcpy(&g_tooltip.background_color, &a->pix.back, sizeof(Color));
		memcpy(&g_tooltip.border, &a->pix.border, sizeof(Border));
	}
	else if (strcmp (key, "tooltip_font_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, g_tooltip.font_color.color);
		if (value2) g_tooltip.font_color.alpha = (atoi (value2) / 100.0);
		else g_tooltip.font_color.alpha = 0.1;
	}
	else if (strcmp (key, "tooltip_font") == 0) {
		g_tooltip.font_desc = pango_font_description_from_string(value);
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
		panel_config.g_task.centered = atoi (value);
	else if (strcmp (key, "task_margin") == 0) {
		panel_config.g_taskbar.paddingxlr = 0;
		panel_config.g_taskbar.paddingx = atoi (value);
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


int config_read ()
{
	const gchar * const * system_dirs;
	char *path1;
	gint i;

	// follow XDG specification
	// check tint2rc in user directory
	path1 = g_build_filename (g_get_user_config_dir(), "tint2", "tint2rc", NULL);
	if (g_file_test (path1, G_FILE_TEST_EXISTS)) {
		i = config_read_file (path1);
		config_path = strdup(path1);
		g_free(path1);
		return i;
	}
	g_free(path1);

	// copy tint2rc from system directory to user directory
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
	char *key, *value;

	if ((fp = fopen(path, "r")) == NULL) return 0;

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (parse_line(line, &key, &value)) {
			add_entry (key, value);
			free (key);
			free (value);
		}
	}
	fclose (fp);

	if (old_task_icon_size) {
		panel_config.g_task.area.paddingy = ((int)panel_config.area.height - (2 * panel_config.area.paddingy) - old_task_icon_size) / 2;
	}
	return 1;
}



