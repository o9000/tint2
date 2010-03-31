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
#include "timer.h"

#ifdef ENABLE_BATTERY
#include "battery.h"
#endif

// global path
char *config_path = 0;
char *snapshot_path = 0;

// --------------------------------------------------
// backward compatibility
static int old_task_icon_size;
// detect if it's an old config file
// ==1
static int old_config_file;


void init_config()
{
	if (backgrounds)
		g_array_free(backgrounds, 1);
	backgrounds = g_array_new(0, 0, sizeof(Background));

	// append full transparency background
	Background transparent_bg;
	memset(&transparent_bg, 0, sizeof(Background));
	g_array_append_val(backgrounds, transparent_bg);

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
	systray.alpha = 100;
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
	if (strcmp(monitor, "all") == 0)
		return -1;
	else {
		char* endptr;
		int ret_int = strtol(monitor, &endptr, 10);
		if (*endptr == 0)
			return ret_int-1;
		else {
			// monitor specified by name, not by index
			int i, j;
			for (i=0; i<server.nb_monitor; ++i) {
				j = 0;
				while (server.monitor[i].names[j] != 0) {
					if (strcmp(monitor, server.monitor[i].names[j++]) == 0)
						return i;
				}
			}
		}
	}
	return -1;
}

void add_entry (char *key, char *value)
{
	char *value1=0, *value2=0, *value3=0;

	/* Background and border */
	if (strcmp (key, "rounded") == 0) {
		// 'rounded' is the first parameter => alloc a new background
		Background bg;
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
		max_tick_urgent = (atoi (value) * 2) + 1;
	else if (strcmp (key, "panel_layer") == 0) {
		if (strcmp(value, "bottom") == 0)
			panel_layer = BOTTOM_LAYER;
		else if (strcmp(value, "normal") == 0)
			panel_layer = NORMAL_LAYER;
		else if (strcmp(value, "top") == 0)
			panel_layer = TOP_LAYER;
	}

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
	else if (strcmp (key, "taskbar_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.g_taskbar.area.paddingxlr = panel_config.g_taskbar.area.paddingx = atoi (value1);
		if (value2) panel_config.g_taskbar.area.paddingy = atoi (value2);
		if (value3) panel_config.g_taskbar.area.paddingx = atoi (value3);
	}
	else if (strcmp (key, "taskbar_background_id") == 0) {
		int id = atoi (value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.g_taskbar.bg = &g_array_index(backgrounds, Background, id);
		panel_config.g_taskbar.area.bg = panel_config.g_taskbar.bg;
	}
	else if (strcmp (key, "taskbar_active_background_id") == 0) {
		int id = atoi (value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.g_taskbar.bg_active = &g_array_index(backgrounds, Background, id);
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

	/* Systray */
	// systray disabled in snapshot mode
	else if (strcmp (key, "systray") == 0 && snapshot_path == 0) {
		systray_enabled = atoi(value);
		// systray is latest option added. files without 'systray' are old.
		old_config_file = 0;
	}
	else if (strcmp (key, "systray_padding") == 0 && snapshot_path == 0) {
		if (old_config_file)
			systray_enabled = 1;
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

	/* Tooltip */
	else if (strcmp (key, "tooltip") == 0)
		g_tooltip.enabled = atoi(value);
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
	else if (strcmp(key, "autohide_height") == 0)
		panel_autohide_height = atoi(value);

	else
		fprintf(stderr, "tint2 : invalid option \"%s\",\n  upgrade tint2 or correct your config file\n", key);

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

	if (old_task_icon_size) {
		panel_config.g_task.area.paddingy = ((int)panel_config.area.height - (2 * panel_config.area.paddingy) - old_task_icon_size) / 2;
	}
	return 1;
}



