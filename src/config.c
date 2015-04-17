/**************************************************************************
*
* Tint2 : read/write config file
*
* Copyright (C) 2007 Pål Staurland (staura@gmail.com)
* Modified (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
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

#include "config.h"

#ifndef TINT2CONF

#include "common.h"
#include "server.h"
#include "panel.h"
#include "task.h"
#include "taskbar.h"
#include "taskbarname.h"
#include "systraybar.h"
#include "launcher.h"
#include "clock.h"
#include "window.h"
#include "tooltip.h"
#include "timer.h"

#ifdef ENABLE_BATTERY
#include "battery.h"
#endif

#endif

// global path
char *config_path;
char *snapshot_path;

#ifndef TINT2CONF

// --------------------------------------------------
// backward compatibility
// detect if it's an old config file (==1)
static int new_config_file;


void default_config()
{
	config_path = NULL;
	snapshot_path = NULL;
	new_config_file = 0;
}

void cleanup_config()
{
	free(config_path);
	config_path = NULL;
	free(snapshot_path);
	snapshot_path = NULL;
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
	else
		fprintf(stderr, "Error: unrecognized action '%s'. Please fix your config file.\n", event);
}


int get_task_status(char* status)
{
	if (strcmp(status, "active") == 0)
		return TASK_ACTIVE;
	if (strcmp(status, "iconified") == 0)
		return TASK_ICONIFIED;
	if (strcmp(status, "urgent") == 0)
		return TASK_URGENT;
	return TASK_NORMAL;
}


int config_get_monitor(char* monitor)
{
	if (strcmp(monitor, "all") != 0) {
		char* endptr;
		int ret_int = strtol(monitor, &endptr, 10);
		if (*endptr == 0)
			return ret_int-1;
		else {
			// monitor specified by name, not by index
			int i, j;
			for (i=0; i<server.nb_monitor; ++i) {
				if (server.monitor[i].names == 0) 
					// xrandr can't identify monitors
					continue;
				j = 0;
				while (server.monitor[i].names[j] != 0) {
					if (strcmp(monitor, server.monitor[i].names[j++]) == 0)
						return i;
				}
			}
		}
	}
	// monitor == "all" or monitor not found or xrandr can't identify monitors
	return -1;
}

void load_launcher_app_dir(const char *path)
{
	GDir *d = g_dir_open(path, 0, NULL);
	if (d) {
		const gchar *name;
		while ((name = g_dir_read_name(d))) {
			gchar *file = g_build_filename(path, name, NULL);
			if (!g_file_test(file, G_FILE_TEST_IS_DIR) &&
				g_str_has_suffix(file, ".desktop")) {
				panel_config.launcher.list_apps = g_slist_append(panel_config.launcher.list_apps, strdup(file));
			} else if (g_file_test(file, G_FILE_TEST_IS_DIR)) {
				load_launcher_app_dir(file);
			}
			g_free(file);
		}
		g_dir_close(d);
	}
}

void add_entry (char *key, char *value)
{
	char *value1=0, *value2=0, *value3=0;

	/* Background and border */
	if (strcmp (key, "rounded") == 0) {
		// 'rounded' is the first parameter => alloc a new background
		Background bg;
		memset(&bg, 0, sizeof(bg));
		bg.border.rounded = atoi(value);
		g_array_append_val(backgrounds, bg);
	}
	else if (strcmp (key, "border_width") == 0) {
		g_array_index(backgrounds, Background, backgrounds->len-1).border.width = atoi(value);
	}
	else if (strcmp (key, "background_color") == 0) {
		Background* bg = &g_array_index(backgrounds, Background, backgrounds->len-1);
		extract_values(value, &value1, &value2, &value3);
		get_color (value1, bg->back.color);
		if (value2) bg->back.alpha = (atoi (value2) / 100.0);
		else bg->back.alpha = 0.5;
	}
	else if (strcmp (key, "border_color") == 0) {
		Background* bg = &g_array_index(backgrounds, Background, backgrounds->len-1);
		extract_values(value, &value1, &value2, &value3);
		get_color (value1, bg->border.color);
		if (value2) bg->border.alpha = (atoi (value2) / 100.0);
		else bg->border.alpha = 0.5;
	}

	/* Panel */
	else if (strcmp (key, "panel_monitor") == 0) {
		panel_config.monitor = config_get_monitor(value);
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
	else if (strcmp (key, "panel_items") == 0) {
		new_config_file = 1;
		panel_items_order = strdup(value);
		int j;
		for (j=0 ; j < strlen(panel_items_order) ; j++) {
			if (panel_items_order[j] == 'L')
				launcher_enabled = 1;
			if (panel_items_order[j] == 'T')
				taskbar_enabled = 1;
			if (panel_items_order[j] == 'B') {
#ifdef ENABLE_BATTERY
				battery_enabled = 1;
#else
				fprintf(stderr, "tint2 is build without battery support\n");
#endif
			}
			if (panel_items_order[j] == 'S') {
				// systray disabled in snapshot mode
				if (snapshot_path == 0)
					systray_enabled = 1;
			}
			if (panel_items_order[j] == 'C')
				clock_enabled = 1;
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
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.area.bg = &g_array_index(backgrounds, Background, id);
	}
	else if (strcmp (key, "wm_menu") == 0)
		wm_menu = atoi (value);
	else if (strcmp (key, "panel_dock") == 0)
		panel_dock = atoi (value);
	else if (strcmp (key, "urgent_nb_of_blink") == 0)
		max_tick_urgent = atoi (value);
	else if (strcmp (key, "panel_layer") == 0) {
		if (strcmp(value, "bottom") == 0)
			panel_layer = BOTTOM_LAYER;
		else if (strcmp(value, "top") == 0)
			panel_layer = TOP_LAYER;
		else
			panel_layer = NORMAL_LAYER;
	}
	else if (strcmp (key, "disable_transparency") == 0) {
		server.disable_transparency = atoi (value);
	}
	else if (strcmp (key, "panel_window_name") == 0) {
		if (strlen(value) > 0) {
			free(panel_window_name);
			panel_window_name = strdup (value);
		}
	}

	/* Battery */
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
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.battery.area.bg = &g_array_index(backgrounds, Background, id);
#endif
	}
	else if (strcmp (key, "battery_hide") == 0) {
#ifdef ENABLE_BATTERY
		percentage_hide = atoi (value);
		if (percentage_hide == 0)
			percentage_hide = 101;
#endif
	}

	/* Clock */
	else if (strcmp (key, "time1_format") == 0) {
		if (new_config_file == 0) {
			clock_enabled = 1;
			if (panel_items_order) {
				gchar* tmp = g_strconcat(panel_items_order, "C", NULL);
				free(panel_items_order);
				panel_items_order = strdup(tmp);
				g_free(tmp);
			}
			else 
				panel_items_order = strdup("C");
		}
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
	else if (strcmp(key, "time1_timezone") == 0) {
		if (strlen(value) > 0)
			time1_timezone = strdup(value);
	}
	else if (strcmp(key, "time2_timezone") == 0) {
		if (strlen(value) > 0)
			time2_timezone = strdup(value);
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
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.clock.area.bg = &g_array_index(backgrounds, Background, id);
	}
	else if (strcmp(key, "clock_tooltip") == 0) {
		if (strlen(value) > 0)
			time_tooltip_format = strdup (value);
	}
	else if (strcmp(key, "clock_tooltip_timezone") == 0) {
		if (strlen(value) > 0)
			time_tooltip_timezone = strdup(value);
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
	else if (strcmp (key, "taskbar_distribute_size") == 0) {
		taskbar_distribute_size = atoi(value);
	}
	else if (strcmp (key, "taskbar_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.g_taskbar.area.paddingxlr = panel_config.g_taskbar.area.paddingx = atoi (value1);
		if (value2) panel_config.g_taskbar.area.paddingy = atoi (value2);
		if (value3) panel_config.g_taskbar.area.paddingx = atoi (value3);
	}
	else if (strcmp (key, "taskbar_background_id") == 0) {
		int id = atoi (value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.g_taskbar.background[TASKBAR_NORMAL] = &g_array_index(backgrounds, Background, id);
		if (panel_config.g_taskbar.background[TASKBAR_ACTIVE] == 0)
			panel_config.g_taskbar.background[TASKBAR_ACTIVE] = panel_config.g_taskbar.background[TASKBAR_NORMAL];
	}
	else if (strcmp (key, "taskbar_active_background_id") == 0) {
		int id = atoi (value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.g_taskbar.background[TASKBAR_ACTIVE] = &g_array_index(backgrounds, Background, id);
	}
	else if (strcmp (key, "taskbar_name") == 0) {
		taskbarname_enabled = atoi (value);
	}
	else if (strcmp (key, "taskbar_name_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.g_taskbar.area_name.paddingxlr = panel_config.g_taskbar.area_name.paddingx = atoi (value1);
	}
	else if (strcmp (key, "taskbar_name_background_id") == 0) {
		int id = atoi (value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.g_taskbar.background_name[TASKBAR_NORMAL] = &g_array_index(backgrounds, Background, id);
		if (panel_config.g_taskbar.background_name[TASKBAR_ACTIVE] == 0)
			panel_config.g_taskbar.background_name[TASKBAR_ACTIVE] = panel_config.g_taskbar.background_name[TASKBAR_NORMAL];
	}
	else if (strcmp (key, "taskbar_name_active_background_id") == 0) {
		int id = atoi (value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.g_taskbar.background_name[TASKBAR_ACTIVE] = &g_array_index(backgrounds, Background, id);
	}
	else if (strcmp (key, "taskbar_name_font") == 0) {
		taskbarname_font_desc = pango_font_description_from_string (value);
	}
	else if (strcmp (key, "taskbar_name_font_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		get_color (value1, taskbarname_font.color);
		if (value2) taskbarname_font.alpha = (atoi (value2) / 100.0);
		else taskbarname_font.alpha = 0.5;
	}
	else if (strcmp (key, "taskbar_name_active_font_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		get_color (value1, taskbarname_active_font.color);
		if (value2) taskbarname_active_font.alpha = (atoi (value2) / 100.0);
		else taskbarname_active_font.alpha = 0.5;
	}
	else if (strcmp (key, "taskbar_hide_inactive_tasks") == 0) {
		hide_inactive_tasks = atoi (value);
	}
	else if (strcmp (key, "taskbar_hide_different_monitor") == 0) {
		hide_task_diff_monitor = atoi (value);
	}
	else if (strcmp (key, "taskbar_sort_order") == 0) {
		if (strcmp(value, "center") == 0) {
			taskbar_sort_method = TASKBAR_SORT_CENTER;
		} else if (strcmp(value, "title") == 0) {
			taskbar_sort_method = TASKBAR_SORT_TITLE;
		} else {
			taskbar_sort_method = TASKBAR_NOSORT;
		}
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
	else if (g_regex_match_simple("task.*_font_color", key, 0, 0)) {
		gchar** split = g_regex_split_simple("_", key, 0, 0);
		int status = get_task_status(split[1]);
		g_strfreev(split);
		extract_values(value, &value1, &value2, &value3);
		float alpha = 1;
		if (value2) alpha = (atoi (value2) / 100.0);
		get_color (value1, panel_config.g_task.font[status].color);
		panel_config.g_task.font[status].alpha = alpha;
		panel_config.g_task.config_font_mask |= (1<<status);
	}
	else if (g_regex_match_simple("task.*_icon_asb", key, 0, 0)) {
		gchar** split = g_regex_split_simple("_", key, 0, 0);
		int status = get_task_status(split[1]);
		g_strfreev(split);
		extract_values(value, &value1, &value2, &value3);
		panel_config.g_task.alpha[status] = atoi(value1);
		panel_config.g_task.saturation[status] = atoi(value2);
		panel_config.g_task.brightness[status] = atoi(value3);
		panel_config.g_task.config_asb_mask |= (1<<status);
	}
	else if (g_regex_match_simple("task.*_background_id", key, 0, 0)) {
		gchar** split = g_regex_split_simple("_", key, 0, 0);
		int status = get_task_status(split[1]);
		g_strfreev(split);
		int id = atoi (value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.g_task.background[status] = &g_array_index(backgrounds, Background, id);
		panel_config.g_task.config_background_mask |= (1<<status);
		if (status == TASK_NORMAL) panel_config.g_task.area.bg = panel_config.g_task.background[TASK_NORMAL];
	}
	// "tooltip" is deprecated but here for backwards compatibility
	else if (strcmp (key, "task_tooltip") == 0 || strcmp(key, "tooltip") == 0)
		panel_config.g_task.tooltip_enabled = atoi(value);

	/* Systray */
	else if (strcmp (key, "systray_padding") == 0) {
		if (new_config_file == 0 && systray_enabled == 0) {
			systray_enabled = 1;
			if (panel_items_order) {
				gchar* tmp = g_strconcat(panel_items_order, "S", NULL);
				free(panel_items_order);
				panel_items_order = strdup(tmp);
				g_free(tmp);
			}
			else
				panel_items_order = strdup("S");
		}
		extract_values(value, &value1, &value2, &value3);
		systray.area.paddingxlr = systray.area.paddingx = atoi (value1);
		if (value2) systray.area.paddingy = atoi (value2);
		if (value3) systray.area.paddingx = atoi (value3);
	}
	else if (strcmp (key, "systray_background_id") == 0) {
		int id = atoi (value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		systray.area.bg = &g_array_index(backgrounds, Background, id);
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
	else if (strcmp(key, "systray_icon_size") == 0) {
		systray_max_icon_size = atoi(value);
	}
	else if (strcmp(key, "systray_icon_asb") == 0) {
		extract_values(value, &value1, &value2, &value3);
		systray.alpha = atoi(value1);
		systray.saturation = atoi(value2);
		systray.brightness = atoi(value3);
	}
	else if (strcmp(key, "systray_monitor") == 0) {
		systray_monitor = atoi(value) - 1;
	}

	/* Launcher */
	else if (strcmp (key, "launcher_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.launcher.area.paddingxlr = panel_config.launcher.area.paddingx = atoi (value1);
		if (value2) panel_config.launcher.area.paddingy = atoi (value2);
		if (value3) panel_config.launcher.area.paddingx = atoi (value3);
	}
	else if (strcmp (key, "launcher_background_id") == 0) {
		int id = atoi (value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.launcher.area.bg = &g_array_index(backgrounds, Background, id);
	}
	else if (strcmp(key, "launcher_icon_size") == 0) {
		launcher_max_icon_size = atoi(value);
	}
	else if (strcmp(key, "launcher_item_app") == 0) {
		char *app = expand_tilde(value);
		panel_config.launcher.list_apps = g_slist_append(panel_config.launcher.list_apps, app);
	}
	else if (strcmp(key, "launcher_apps_dir") == 0) {
		char *path = expand_tilde(value);
		load_launcher_app_dir(path);
		free(path);
	}
	else if (strcmp(key, "launcher_icon_theme") == 0) {
		// if XSETTINGS manager running, tint2 use it.
		if (icon_theme_name_config)
			free(icon_theme_name_config);
		icon_theme_name_config = strdup(value);
	}
	else if (strcmp(key, "launcher_icon_asb") == 0) {
		extract_values(value, &value1, &value2, &value3);
		launcher_alpha = atoi(value1);
		launcher_saturation = atoi(value2);
		launcher_brightness = atoi(value3);
	}
	else if (strcmp(key, "launcher_tooltip") == 0) {
		launcher_tooltip_enabled = atoi(value);
	}
	else if (strcmp(key, "startup_notifications") == 0) {
		startup_notifications = atoi(value);
	}

	/* Tooltip */
	else if (strcmp (key, "tooltip_show_timeout") == 0) {
		int timeout_msec = 1000*atof(value);
		g_tooltip.show_timeout_msec = timeout_msec;
	}
	else if (strcmp (key, "tooltip_hide_timeout") == 0) {
		int timeout_msec = 1000*atof(value);
		g_tooltip.hide_timeout_msec = timeout_msec;
	}
	else if (strcmp (key, "tooltip_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		if (value1) g_tooltip.paddingx = atoi(value1);
		if (value2) g_tooltip.paddingy = atoi(value2);
	}
	else if (strcmp (key, "tooltip_background_id") == 0) {
		int id = atoi (value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		g_tooltip.bg = &g_array_index(backgrounds, Background, id);
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
	else if (strcmp (key, "mouse_left") == 0)
		get_action (value, &mouse_left);
	else if (strcmp (key, "mouse_middle") == 0)
		get_action (value, &mouse_middle);
	else if (strcmp (key, "mouse_right") == 0)
		get_action (value, &mouse_right);
	else if (strcmp (key, "mouse_scroll_up") == 0)
		get_action (value, &mouse_scroll_up);
	else if (strcmp (key, "mouse_scroll_down") == 0)
		get_action (value, &mouse_scroll_down);

	/* autohide options */
	else if (strcmp(key, "autohide") == 0)
		panel_autohide = atoi(value);
	else if (strcmp(key, "autohide_show_timeout") == 0)
		panel_autohide_show_timeout = 1000*atof(value);
	else if (strcmp(key, "autohide_hide_timeout") == 0)
		panel_autohide_hide_timeout = 1000*atof(value);
	else if (strcmp(key, "strut_policy") == 0) {
		if (strcmp(value, "follow_size") == 0)
			panel_strut_policy = STRUT_FOLLOW_SIZE;
		else if (strcmp(value, "none") == 0)
			panel_strut_policy = STRUT_NONE;
		else
			panel_strut_policy = STRUT_MINIMUM;
	}
	else if (strcmp(key, "autohide_height") == 0) {
		panel_autohide_height = atoi(value);
		if (panel_autohide_height == 0) {
			// autohide need height > 0
			panel_autohide_height = 1;
		}
	}

	// old config option
	else if (strcmp(key, "systray") == 0) {
		if (new_config_file == 0) {
			systray_enabled = atoi(value);
			if (systray_enabled) {
				if (panel_items_order) {
					gchar* tmp = g_strconcat(panel_items_order, "S", NULL);
					free(panel_items_order);
					panel_items_order = strdup(tmp);
					g_free(tmp);
				}
				else
					panel_items_order = strdup("S");
			}
		}
	}
#ifdef ENABLE_BATTERY
	else if (strcmp(key, "battery") == 0) {
		if (new_config_file == 0) {
			battery_enabled = atoi(value);
			if (battery_enabled) {
				if (panel_items_order) {
					gchar* tmp = g_strconcat(panel_items_order, "B", NULL);
					free(panel_items_order);
					panel_items_order = strdup(tmp);
					g_free(tmp);
				}
				else
					panel_items_order = strdup("B");
			}
		}
	}
#endif
	else
		fprintf(stderr, "tint2 : invalid option \"%s\",\n  upgrade tint2 or correct your config file\n", key);

	if (value1) free (value1);
	if (value2) free (value2);
	if (value3) free (value3);
}


int config_read ()
{
	const gchar * const * system_dirs;
	gchar *path1;
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
	gchar *path2 = 0;
	system_dirs = g_get_system_config_dirs();
	for (i = 0; system_dirs[i]; i++) {
		path2 = g_build_filename(system_dirs[i], "tint2", "tint2rc", NULL);

		if (g_file_test(path2, G_FILE_TEST_EXISTS)) break;
		g_free(path2);
		path2 = 0;
	}

	if (path2) {
		// copy file in user directory (path1)
		gchar *dir = g_build_filename (g_get_user_config_dir(), "tint2", NULL);
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
	char line[512];
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
	
	// append Taskbar item
	if (new_config_file == 0) {
		taskbar_enabled = 1;
		if (panel_items_order) {
			gchar* tmp = g_strconcat("T", panel_items_order, NULL);
			free(panel_items_order);
			panel_items_order = strdup(tmp);
			g_free(tmp);
		}
		else 
			panel_items_order = strdup("T");
	}

	return 1;
}

#endif

