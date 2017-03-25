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
#include <pango/pango-font.h>
#include <Imlib2.h>

#include "config.h"

#ifndef TINT2CONF

#include "tint2rc.h"
#include "common.h"
#include "server.h"
#include "strnatcmp.h"
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
#include "separator.h"
#include "execplugin.h"

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
static gboolean new_config_file;

static gboolean read_bg_color_hover;
static gboolean read_border_color_hover;
static gboolean read_bg_color_press;
static gboolean read_border_color_press;
static gboolean read_panel_position;

void default_config()
{
	config_path = NULL;
	snapshot_path = NULL;
	new_config_file = FALSE;
	read_panel_position = FALSE;
}

void cleanup_config()
{
	free(config_path);
	config_path = NULL;
	free(snapshot_path);
	snapshot_path = NULL;
}

void get_action(char *event, MouseAction *action)
{
	if (strcmp(event, "none") == 0)
		*action = NONE;
	else if (strcmp(event, "close") == 0)
		*action = CLOSE;
	else if (strcmp(event, "toggle") == 0)
		*action = TOGGLE;
	else if (strcmp(event, "iconify") == 0)
		*action = ICONIFY;
	else if (strcmp(event, "shade") == 0)
		*action = SHADE;
	else if (strcmp(event, "toggle_iconify") == 0)
		*action = TOGGLE_ICONIFY;
	else if (strcmp(event, "maximize_restore") == 0)
		*action = MAXIMIZE_RESTORE;
	else if (strcmp(event, "desktop_left") == 0)
		*action = DESKTOP_LEFT;
	else if (strcmp(event, "desktop_right") == 0)
		*action = DESKTOP_RIGHT;
	else if (strcmp(event, "next_task") == 0)
		*action = NEXT_TASK;
	else if (strcmp(event, "prev_task") == 0)
		*action = PREV_TASK;
	else
		fprintf(stderr, "Error: unrecognized action '%s'. Please fix your config file.\n", event);
}

int get_task_status(char *status)
{
	if (strcmp(status, "active") == 0)
		return TASK_ACTIVE;
	if (strcmp(status, "iconified") == 0)
		return TASK_ICONIFIED;
	if (strcmp(status, "urgent") == 0)
		return TASK_URGENT;
	return -1;
}

int config_get_monitor(char *monitor)
{
	if (strcmp(monitor, "all") != 0) {
		char *endptr;
		int ret_int = strtol(monitor, &endptr, 10);
		if (*endptr == 0)
			return ret_int - 1;
		else {
			// monitor specified by name, not by index
			int i, j;
			for (i = 0; i < server.num_monitors; ++i) {
				if (server.monitors[i].names == 0)
					// xrandr can't identify monitors
					continue;
				j = 0;
				while (server.monitors[i].names[j] != 0) {
					if (strcmp(monitor, server.monitors[i].names[j++]) == 0)
						return i;
				}
			}
		}
	}
	// monitor == "all" or monitor not found or xrandr can't identify monitors
	return -1;
}

static gint compare_strings(gconstpointer a, gconstpointer b)
{
	return strnatcasecmp((const char *)a, (const char *)b);
}

void load_launcher_app_dir(const char *path)
{
	GList *subdirs = NULL;
	GList *files = NULL;

	GDir *d = g_dir_open(path, 0, NULL);
	if (d) {
		const gchar *name;
		while ((name = g_dir_read_name(d))) {
			gchar *file = g_build_filename(path, name, NULL);
			if (!g_file_test(file, G_FILE_TEST_IS_DIR) && g_str_has_suffix(file, ".desktop")) {
				files = g_list_append(files, file);
			} else if (g_file_test(file, G_FILE_TEST_IS_DIR)) {
				subdirs = g_list_append(subdirs, file);
			} else {
				g_free(file);
			}
		}
		g_dir_close(d);
	}

	subdirs = g_list_sort(subdirs, compare_strings);
	GList *l;
	for (l = subdirs; l; l = g_list_next(l)) {
		gchar *dir = (gchar *)l->data;
		load_launcher_app_dir(dir);
		g_free(dir);
	}
	g_list_free(subdirs);

	files = g_list_sort(files, compare_strings);
	for (l = files; l; l = g_list_next(l)) {
		gchar *file = (gchar *)l->data;
		panel_config.launcher.list_apps = g_slist_append(panel_config.launcher.list_apps, strdup(file));
		g_free(file);
	}
	g_list_free(files);
}

Separator *get_or_create_last_separator()
{
	if (!panel_config.separator_list) {
		fprintf(stderr, "Warning: separator items should shart with 'separator = new'\n");
		panel_config.separator_list = g_list_append(panel_config.separator_list, create_separator());
	}
	return (Separator *)g_list_last(panel_config.separator_list)->data;
}

Execp *get_or_create_last_execp()
{
	if (!panel_config.execp_list) {
		fprintf(stderr, "Warning: execp items should start with 'execp = new'\n");
		panel_config.execp_list = g_list_append(panel_config.execp_list, create_execp());
	}
	return (Execp *)g_list_last(panel_config.execp_list)->data;
}

Button *get_or_create_last_button()
{
	if (!panel_config.button_list) {
		fprintf(stderr, "Warning: button items should start with 'button = new'\n");
		panel_config.button_list = g_list_append(panel_config.button_list, create_button());
	}
	return (Button *)g_list_last(panel_config.button_list)->data;
}

void add_entry(char *key, char *value)
{
	char *value1 = 0, *value2 = 0, *value3 = 0;

	/* Background and border */
	if (strcmp(key, "rounded") == 0) {
		// 'rounded' is the first parameter => alloc a new background
		if (backgrounds->len > 0) {
			Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
			if (!read_bg_color_hover)
				memcpy(&bg->fill_color_hover, &bg->fill_color, sizeof(Color));
			if (!read_border_color_hover)
				memcpy(&bg->border_color_hover, &bg->border, sizeof(Color));
			if (!read_bg_color_press)
				memcpy(&bg->fill_color_pressed, &bg->fill_color_hover, sizeof(Color));
			if (!read_border_color_press)
				memcpy(&bg->border_color_pressed, &bg->border_color_hover, sizeof(Color));
		}
		Background bg;
		init_background(&bg);
		bg.border.radius = atoi(value);
		g_array_append_val(backgrounds, bg);
		read_bg_color_hover = FALSE;
		read_border_color_hover = FALSE;
		read_bg_color_press = FALSE;
		read_border_color_press = FALSE;
	} else if (strcmp(key, "border_width") == 0) {
		g_array_index(backgrounds, Background, backgrounds->len - 1).border.width = atoi(value);
	} else if (strcmp(key, "border_sides") == 0) {
		Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
		bg->border.mask = 0;
		if (strchr(value, 'l') || strchr(value, 'L'))
			bg->border.mask |= BORDER_LEFT;
		if (strchr(value, 'r') || strchr(value, 'R'))
			bg->border.mask |= BORDER_RIGHT;
		if (strchr(value, 't') || strchr(value, 'T'))
			bg->border.mask |= BORDER_TOP;
		if (strchr(value, 'b') || strchr(value, 'B'))
			bg->border.mask |= BORDER_BOTTOM;
		if (!bg->border.mask)
			bg->border.width = 0;
	} else if (strcmp(key, "background_color") == 0) {
		Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, bg->fill_color.rgb);
		if (value2)
			bg->fill_color.alpha = (atoi(value2) / 100.0);
		else
			bg->fill_color.alpha = 0.5;
	} else if (strcmp(key, "border_color") == 0) {
		Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, bg->border.color.rgb);
		if (value2)
			bg->border.color.alpha = (atoi(value2) / 100.0);
		else
			bg->border.color.alpha = 0.5;
	} else if (strcmp(key, "background_color_hover") == 0) {
		Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, bg->fill_color_hover.rgb);
		if (value2)
			bg->fill_color_hover.alpha = (atoi(value2) / 100.0);
		else
			bg->fill_color_hover.alpha = 0.5;
		read_bg_color_hover = 1;
	} else if (strcmp(key, "border_color_hover") == 0) {
		Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, bg->border_color_hover.rgb);
		if (value2)
			bg->border_color_hover.alpha = (atoi(value2) / 100.0);
		else
			bg->border_color_hover.alpha = 0.5;
		read_border_color_hover = 1;
	} else if (strcmp(key, "background_color_pressed") == 0) {
		Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, bg->fill_color_pressed.rgb);
		if (value2)
			bg->fill_color_pressed.alpha = (atoi(value2) / 100.0);
		else
			bg->fill_color_pressed.alpha = 0.5;
		read_bg_color_press = 1;
	} else if (strcmp(key, "border_color_pressed") == 0) {
		Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, bg->border_color_pressed.rgb);
		if (value2)
			bg->border_color_pressed.alpha = (atoi(value2) / 100.0);
		else
			bg->border_color_pressed.alpha = 0.5;
		read_border_color_press = 1;
	} else if (strcmp(key, "gradient_id") == 0) {
		Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
		int id = atoi(value);
		id = (id < gradients->len && id >= 0) ? id : -1;
		if (id >= 0)
			bg->gradients[MOUSE_NORMAL] = &g_array_index(gradients, GradientClass, id);
	} else if (strcmp(key, "gradient_id_hover") == 0 || strcmp(key, "hover_gradient_id") == 0) {
		Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
		int id = atoi(value);
		id = (id < gradients->len && id >= 0) ? id : -1;
		if (id >= 0)
			bg->gradients[MOUSE_OVER] = &g_array_index(gradients, GradientClass, id);
	} else if (strcmp(key, "gradient_id_pressed") == 0 || strcmp(key, "pressed_gradient_id") == 0) {
		Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
		int id = atoi(value);
		id = (id < gradients->len && id >= 0) ? id : -1;
		if (id >= 0)
			bg->gradients[MOUSE_DOWN] = &g_array_index(gradients, GradientClass, id);
	}

	/* Gradients */
	else if (strcmp(key, "gradient") == 0) {
		// Create a new gradient
		GradientClass g;
		init_gradient(&g, gradient_type_from_string(value));
		g_array_append_val(gradients, g);
	} else if (strcmp(key, "start_color") == 0) {
		GradientClass *g = &g_array_index(gradients, GradientClass, gradients->len - 1);
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, g->start_color.rgb);
		if (value2)
			g->start_color.alpha = (atoi(value2) / 100.0);
		else
			g->start_color.alpha = 0.5;
	} else if (strcmp(key, "end_color") == 0) {
		GradientClass *g = &g_array_index(gradients, GradientClass, gradients->len - 1);
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, g->end_color.rgb);
		if (value2)
			g->end_color.alpha = (atoi(value2) / 100.0);
		else
			g->end_color.alpha = 0.5;
	} else if (strcmp(key, "color_stop") == 0) {
		GradientClass *g = &g_array_index(gradients, GradientClass, gradients->len - 1);
		extract_values(value, &value1, &value2, &value3);
		ColorStop *color_stop = (ColorStop *) calloc(1, sizeof(ColorStop));
		color_stop->offset = atof(value1) / 100.0;
		get_color(value2, color_stop->color.rgb);
		if (value3)
			color_stop->color.alpha = (atoi(value3) / 100.0);
		else
			color_stop->color.alpha = 0.5;
		g->extra_color_stops = g_list_append(g->extra_color_stops, color_stop);
	}

	/* Panel */
	else if (strcmp(key, "panel_monitor") == 0) {
		panel_config.monitor = config_get_monitor(value);
	} else if (strcmp(key, "primary_monitor_first") == 0) {
		primary_monitor_first = atoi(value);
	} else if (strcmp(key, "panel_shrink") == 0) {
		panel_shrink = atoi(value);
	} else if (strcmp(key, "panel_size") == 0) {
		extract_values(value, &value1, &value2, &value3);

		char *b;
		if ((b = strchr(value1, '%'))) {
			b[0] = '\0';
			panel_config.fractional_width = TRUE;
		}
		panel_config.area.width = atoi(value1);
		if (panel_config.area.width == 0) {
			// full width mode
			panel_config.area.width = 100;
			panel_config.fractional_width = TRUE;
		}
		if (value2) {
			if ((b = strchr(value2, '%'))) {
				b[0] = '\0';
				panel_config.fractional_height = 1;
			}
			panel_config.area.height = atoi(value2);
		}
	} else if (strcmp(key, "panel_items") == 0) {
		new_config_file = TRUE;
		panel_items_order = strdup(value);
		systray_enabled = 0;
		launcher_enabled = 0;
#ifdef ENABLE_BATTERY
		battery_enabled = 0;
#endif
		clock_enabled = 0;
		taskbar_enabled = 0;
		for (int j = 0; j < strlen(panel_items_order); j++) {
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
	} else if (strcmp(key, "panel_margin") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.marginx = atoi(value1);
		if (value2)
			panel_config.marginy = atoi(value2);
	} else if (strcmp(key, "panel_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.area.paddingxlr = panel_config.area.paddingx = atoi(value1);
		if (value2)
			panel_config.area.paddingy = atoi(value2);
		if (value3)
			panel_config.area.paddingx = atoi(value3);
	} else if (strcmp(key, "panel_position") == 0) {
		read_panel_position = TRUE;
		extract_values(value, &value1, &value2, &value3);
		if (strcmp(value1, "top") == 0)
			panel_position = TOP;
		else {
			if (strcmp(value1, "bottom") == 0)
				panel_position = BOTTOM;
			else
				panel_position = CENTER;
		}

		if (!value2)
			panel_position |= CENTER;
		else {
			if (strcmp(value2, "left") == 0)
				panel_position |= LEFT;
			else {
				if (strcmp(value2, "right") == 0)
					panel_position |= RIGHT;
				else
					panel_position |= CENTER;
			}
		}
		if (!value3)
			panel_horizontal = 1;
		else {
			if (strcmp(value3, "vertical") == 0)
				panel_horizontal = 0;
			else
				panel_horizontal = 1;
		}
	} else if (strcmp(key, "font_shadow") == 0)
		panel_config.font_shadow = atoi(value);
	else if (strcmp(key, "panel_background_id") == 0) {
		int id = atoi(value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.area.bg = &g_array_index(backgrounds, Background, id);
	} else if (strcmp(key, "wm_menu") == 0)
		wm_menu = atoi(value);
	else if (strcmp(key, "panel_dock") == 0)
		panel_dock = atoi(value);
	else if (strcmp(key, "urgent_nb_of_blink") == 0)
		max_tick_urgent = atoi(value);
	else if (strcmp(key, "panel_layer") == 0) {
		if (strcmp(value, "bottom") == 0)
			panel_layer = BOTTOM_LAYER;
		else if (strcmp(value, "top") == 0)
			panel_layer = TOP_LAYER;
		else
			panel_layer = NORMAL_LAYER;
	} else if (strcmp(key, "disable_transparency") == 0) {
		server.disable_transparency = atoi(value);
	} else if (strcmp(key, "panel_window_name") == 0) {
		if (strlen(value) > 0) {
			free(panel_window_name);
			panel_window_name = strdup(value);
		}
	}

	/* Battery */
	else if (strcmp(key, "battery_low_status") == 0) {
#ifdef ENABLE_BATTERY
		battery_low_status = atoi(value);
		if (battery_low_status < 0 || battery_low_status > 100)
			battery_low_status = 0;
#endif
	} else if (strcmp(key, "battery_lclick_command") == 0) {
#ifdef ENABLE_BATTERY
		if (strlen(value) > 0)
			battery_lclick_command = strdup(value);
#endif
	} else if (strcmp(key, "battery_mclick_command") == 0) {
#ifdef ENABLE_BATTERY
		if (strlen(value) > 0)
			battery_mclick_command = strdup(value);
#endif
	} else if (strcmp(key, "battery_rclick_command") == 0) {
#ifdef ENABLE_BATTERY
		if (strlen(value) > 0)
			battery_rclick_command = strdup(value);
#endif
	} else if (strcmp(key, "battery_uwheel_command") == 0) {
#ifdef ENABLE_BATTERY
		if (strlen(value) > 0)
			battery_uwheel_command = strdup(value);
#endif
	} else if (strcmp(key, "battery_dwheel_command") == 0) {
#ifdef ENABLE_BATTERY
		if (strlen(value) > 0)
			battery_dwheel_command = strdup(value);
#endif
	} else if (strcmp(key, "battery_low_cmd") == 0) {
#ifdef ENABLE_BATTERY
		if (strlen(value) > 0)
			battery_low_cmd = strdup(value);
#endif
	} else if (strcmp(key, "ac_connected_cmd") == 0) {
#ifdef ENABLE_BATTERY
		if (strlen(value) > 0)
			ac_connected_cmd = strdup(value);
#endif
	} else if (strcmp(key, "ac_disconnected_cmd") == 0) {
#ifdef ENABLE_BATTERY
		if (strlen(value) > 0)
			ac_disconnected_cmd = strdup(value);
#endif
	} else if (strcmp(key, "bat1_font") == 0) {
#ifdef ENABLE_BATTERY
		bat1_font_desc = pango_font_description_from_string(value);
		bat1_has_font = TRUE;
#endif
	} else if (strcmp(key, "bat2_font") == 0) {
#ifdef ENABLE_BATTERY
		bat2_font_desc = pango_font_description_from_string(value);
		bat2_has_font = TRUE;
#endif
	} else if (strcmp(key, "battery_font_color") == 0) {
#ifdef ENABLE_BATTERY
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, panel_config.battery.font_color.rgb);
		if (value2)
			panel_config.battery.font_color.alpha = (atoi(value2) / 100.0);
		else
			panel_config.battery.font_color.alpha = 0.5;
#endif
	} else if (strcmp(key, "battery_padding") == 0) {
#ifdef ENABLE_BATTERY
		extract_values(value, &value1, &value2, &value3);
		panel_config.battery.area.paddingxlr = panel_config.battery.area.paddingx = atoi(value1);
		if (value2)
			panel_config.battery.area.paddingy = atoi(value2);
		if (value3)
			panel_config.battery.area.paddingx = atoi(value3);
#endif
	} else if (strcmp(key, "battery_background_id") == 0) {
#ifdef ENABLE_BATTERY
		int id = atoi(value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.battery.area.bg = &g_array_index(backgrounds, Background, id);
#endif
	} else if (strcmp(key, "battery_hide") == 0) {
#ifdef ENABLE_BATTERY
		percentage_hide = atoi(value);
		if (percentage_hide == 0)
			percentage_hide = 101;
#endif
	} else if (strcmp(key, "battery_tooltip") == 0) {
#ifdef ENABLE_BATTERY
		battery_tooltip_enabled = atoi(value);
#endif
	}

	/* Separator */
	else if (strcmp(key, "separator") == 0) {
		panel_config.separator_list = g_list_append(panel_config.separator_list, create_separator());
	} else if (strcmp(key, "separator_background_id") == 0) {
		Separator *separator = get_or_create_last_separator();
		int id = atoi(value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		separator->area.bg = &g_array_index(backgrounds, Background, id);
	} else if (strcmp(key, "separator_color") == 0) {
		Separator *separator = get_or_create_last_separator();
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, separator->color.rgb);
		if (value2)
			separator->color.alpha = (atoi(value2) / 100.0);
		else
			separator->color.alpha = 0.5;
	} else if (strcmp(key, "separator_style") == 0) {
		Separator *separator = get_or_create_last_separator();
		if (g_str_equal(value, "empty"))
			separator->style = SEPARATOR_EMPTY;
		else if (g_str_equal(value, "line"))
			separator->style = SEPARATOR_LINE;
		else if (g_str_equal(value, "dots"))
			separator->style = SEPARATOR_DOTS;
		else
			fprintf(stderr, RED "Invalid separator_style value: %s" RESET "\n", value);
	} else if (strcmp(key, "separator_size") == 0) {
		Separator *separator = get_or_create_last_separator();
		separator->thickness = atoi(value);
	} else if (strcmp(key, "separator_padding") == 0) {
		Separator *separator = get_or_create_last_separator();
		extract_values(value, &value1, &value2, &value3);
		separator->area.paddingxlr = separator->area.paddingx = atoi(value1);
		if (value2)
			separator->area.paddingy = atoi(value2);
		if (value3)
			separator->area.paddingx = atoi(value3);
	}

	/* Execp */
	else if (strcmp(key, "execp") == 0) {
		panel_config.execp_list = g_list_append(panel_config.execp_list, create_execp());
	} else if (strcmp(key, "execp_command") == 0) {
		Execp *execp = get_or_create_last_execp();
		free_and_null(execp->backend->command);
		if (strlen(value) > 0)
			execp->backend->command = strdup(value);
	} else if (strcmp(key, "execp_interval") == 0) {
		Execp *execp = get_or_create_last_execp();
		execp->backend->interval = 0;
		int v = atoi(value);
		if (v < 1) {
			fprintf(stderr, "execp_interval must be an integer >= 1\n");
		} else {
			execp->backend->interval = v;
		}
	} else if (strcmp(key, "execp_has_icon") == 0) {
		Execp *execp = get_or_create_last_execp();
		execp->backend->has_icon = atoi(value);
	} else if (strcmp(key, "execp_continuous") == 0) {
		Execp *execp = get_or_create_last_execp();
		execp->backend->continuous = atoi(value);
	} else if (strcmp(key, "execp_markup") == 0) {
		Execp *execp = get_or_create_last_execp();
		execp->backend->has_markup = atoi(value);
	} else if (strcmp(key, "execp_cache_icon") == 0) {
		Execp *execp = get_or_create_last_execp();
		execp->backend->cache_icon = atoi(value);
	} else if (strcmp(key, "execp_tooltip") == 0) {
		Execp *execp = get_or_create_last_execp();
		free_and_null(execp->backend->tooltip);
		execp->backend->tooltip = strdup(value);
	} else if (strcmp(key, "execp_font") == 0) {
		Execp *execp = get_or_create_last_execp();
		pango_font_description_free(execp->backend->font_desc);
		execp->backend->font_desc = pango_font_description_from_string(value);
		execp->backend->has_font = TRUE;
	} else if (strcmp(key, "execp_font_color") == 0) {
		Execp *execp = get_or_create_last_execp();
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, execp->backend->font_color.rgb);
		if (value2)
			execp->backend->font_color.alpha = atoi(value2) / 100.0;
		else
			execp->backend->font_color.alpha = 0.5;
	} else if (strcmp(key, "execp_padding") == 0) {
		Execp *execp = get_or_create_last_execp();
		extract_values(value, &value1, &value2, &value3);
		execp->backend->paddingxlr = execp->backend->paddingx = atoi(value1);
		if (value2)
			execp->backend->paddingy = atoi(value2);
		else
			execp->backend->paddingy = 0;
		if (value3)
			execp->backend->paddingx = atoi(value3);
	} else if (strcmp(key, "execp_background_id") == 0) {
		Execp *execp = get_or_create_last_execp();
		int id = atoi(value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		execp->backend->bg = &g_array_index(backgrounds, Background, id);
	} else if (strcmp(key, "execp_centered") == 0) {
		Execp *execp = get_or_create_last_execp();
		execp->backend->centered = atoi(value);
	} else if (strcmp(key, "execp_icon_w") == 0) {
		Execp *execp = get_or_create_last_execp();
		int v = atoi(value);
		if (v < 0) {
			fprintf(stderr, "execp_icon_w must be an integer >= 0\n");
		} else {
			execp->backend->icon_w = v;
		}
	} else if (strcmp(key, "execp_icon_h") == 0) {
		Execp *execp = get_or_create_last_execp();
		int v = atoi(value);
		if (v < 0) {
			fprintf(stderr, "execp_icon_h must be an integer >= 0\n");
		} else {
			execp->backend->icon_h = v;
		}
	} else if (strcmp(key, "execp_lclick_command") == 0) {
		Execp *execp = get_or_create_last_execp();
		free_and_null(execp->backend->lclick_command);
		if (strlen(value) > 0)
			execp->backend->lclick_command = strdup(value);
	} else if (strcmp(key, "execp_mclick_command") == 0) {
		Execp *execp = get_or_create_last_execp();
		free_and_null(execp->backend->mclick_command);
		if (strlen(value) > 0)
			execp->backend->mclick_command = strdup(value);
	} else if (strcmp(key, "execp_rclick_command") == 0) {
		Execp *execp = get_or_create_last_execp();
		free_and_null(execp->backend->rclick_command);
		if (strlen(value) > 0)
			execp->backend->rclick_command = strdup(value);
	} else if (strcmp(key, "execp_uwheel_command") == 0) {
		Execp *execp = get_or_create_last_execp();
		free_and_null(execp->backend->uwheel_command);
		if (strlen(value) > 0)
			execp->backend->uwheel_command = strdup(value);
	} else if (strcmp(key, "execp_dwheel_command") == 0) {
		Execp *execp = get_or_create_last_execp();
		free_and_null(execp->backend->dwheel_command);
		if (strlen(value) > 0)
			execp->backend->dwheel_command = strdup(value);
	}

	/* Button */
	else if (strcmp(key, "button") == 0) {
		panel_config.button_list = g_list_append(panel_config.button_list, create_button());
	} else if (strcmp(key, "button_icon") == 0) {
		Button *button = get_or_create_last_button();
		button->backend->icon_name = strdup(value);
	} else if (strcmp(key, "button_text") == 0) {
		Button *button = get_or_create_last_button();
		free_and_null(button->backend->text);
		button->backend->text = strdup(value);
	} else if (strcmp(key, "button_tooltip") == 0) {
		Button *button = get_or_create_last_button();
		free_and_null(button->backend->tooltip);
		button->backend->tooltip = strdup(value);
	} else if (strcmp(key, "button_font") == 0) {
		Button *button = get_or_create_last_button();
		pango_font_description_free(button->backend->font_desc);
		button->backend->font_desc = pango_font_description_from_string(value);
		button->backend->has_font = TRUE;
	} else if (strcmp(key, "button_font_color") == 0) {
		Button *button = get_or_create_last_button();
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, button->backend->font_color.rgb);
		if (value2)
			button->backend->font_color.alpha = atoi(value2) / 100.0;
		else
			button->backend->font_color.alpha = 0.5;
	} else if (strcmp(key, "button_padding") == 0) {
		Button *button = get_or_create_last_button();
		extract_values(value, &value1, &value2, &value3);
		button->backend->paddingxlr = button->backend->paddingx = atoi(value1);
		if (value2)
			button->backend->paddingy = atoi(value2);
		else
			button->backend->paddingy = 0;
		if (value3)
			button->backend->paddingx = atoi(value3);
	} else if (strcmp(key, "button_max_icon_size") == 0) {
		Button *button = get_or_create_last_button();
		extract_values(value, &value1, &value2, &value3);
		button->backend->max_icon_size = MAX(0, atoi(value));
	} else if (strcmp(key, "button_background_id") == 0) {
		Button *button = get_or_create_last_button();
		int id = atoi(value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		button->backend->bg = &g_array_index(backgrounds, Background, id);
	} else if (strcmp(key, "button_centered") == 0) {
		Button *button = get_or_create_last_button();
		button->backend->centered = atoi(value);
	} else if (strcmp(key, "button_lclick_command") == 0) {
		Button *button = get_or_create_last_button();
		free_and_null(button->backend->lclick_command);
		if (strlen(value) > 0)
			button->backend->lclick_command = strdup(value);
	} else if (strcmp(key, "button_mclick_command") == 0) {
		Button *button = get_or_create_last_button();
		free_and_null(button->backend->mclick_command);
		if (strlen(value) > 0)
			button->backend->mclick_command = strdup(value);
	} else if (strcmp(key, "button_rclick_command") == 0) {
		Button *button = get_or_create_last_button();
		free_and_null(button->backend->rclick_command);
		if (strlen(value) > 0)
			button->backend->rclick_command = strdup(value);
	} else if (strcmp(key, "button_uwheel_command") == 0) {
		Button *button = get_or_create_last_button();
		free_and_null(button->backend->uwheel_command);
		if (strlen(value) > 0)
			button->backend->uwheel_command = strdup(value);
	} else if (strcmp(key, "button_dwheel_command") == 0) {
		Button *button = get_or_create_last_button();
		free_and_null(button->backend->dwheel_command);
		if (strlen(value) > 0)
			button->backend->dwheel_command = strdup(value);
	}

	/* Clock */
	else if (strcmp(key, "time1_format") == 0) {
		if (!new_config_file) {
			clock_enabled = TRUE;
			if (panel_items_order) {
				gchar *tmp = g_strconcat(panel_items_order, "C", NULL);
				free(panel_items_order);
				panel_items_order = strdup(tmp);
				g_free(tmp);
			} else {
				panel_items_order = strdup("C");
			}
		}
		if (strlen(value) > 0) {
			time1_format = strdup(value);
			clock_enabled = TRUE;
		}
	} else if (strcmp(key, "time2_format") == 0) {
		if (strlen(value) > 0)
			time2_format = strdup(value);
	} else if (strcmp(key, "time1_font") == 0) {
		time1_font_desc = pango_font_description_from_string(value);
		time1_has_font = TRUE;
	} else if (strcmp(key, "time1_timezone") == 0) {
		if (strlen(value) > 0)
			time1_timezone = strdup(value);
	} else if (strcmp(key, "time2_timezone") == 0) {
		if (strlen(value) > 0)
			time2_timezone = strdup(value);
	} else if (strcmp(key, "time2_font") == 0) {
		time2_font_desc = pango_font_description_from_string(value);
		time2_has_font = TRUE;
	} else if (strcmp(key, "clock_font_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, panel_config.clock.font.rgb);
		if (value2)
			panel_config.clock.font.alpha = (atoi(value2) / 100.0);
		else
			panel_config.clock.font.alpha = 0.5;
	} else if (strcmp(key, "clock_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.clock.area.paddingxlr = panel_config.clock.area.paddingx = atoi(value1);
		if (value2)
			panel_config.clock.area.paddingy = atoi(value2);
		if (value3)
			panel_config.clock.area.paddingx = atoi(value3);
	} else if (strcmp(key, "clock_background_id") == 0) {
		int id = atoi(value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.clock.area.bg = &g_array_index(backgrounds, Background, id);
	} else if (strcmp(key, "clock_tooltip") == 0) {
		if (strlen(value) > 0)
			time_tooltip_format = strdup(value);
	} else if (strcmp(key, "clock_tooltip_timezone") == 0) {
		if (strlen(value) > 0)
			time_tooltip_timezone = strdup(value);
	} else if (strcmp(key, "clock_lclick_command") == 0) {
		if (strlen(value) > 0)
			clock_lclick_command = strdup(value);
	} else if (strcmp(key, "clock_mclick_command") == 0) {
		if (strlen(value) > 0)
			clock_mclick_command = strdup(value);
	} else if (strcmp(key, "clock_rclick_command") == 0) {
		if (strlen(value) > 0)
			clock_rclick_command = strdup(value);
	} else if (strcmp(key, "clock_uwheel_command") == 0) {
		if (strlen(value) > 0)
			clock_uwheel_command = strdup(value);
	} else if (strcmp(key, "clock_dwheel_command") == 0) {
		if (strlen(value) > 0)
			clock_dwheel_command = strdup(value);
	}

	/* Taskbar */
	else if (strcmp(key, "taskbar_mode") == 0) {
		if (strcmp(value, "multi_desktop") == 0)
			taskbar_mode = MULTI_DESKTOP;
		else
			taskbar_mode = SINGLE_DESKTOP;
	} else if (strcmp(key, "taskbar_distribute_size") == 0) {
		taskbar_distribute_size = atoi(value);
	} else if (strcmp(key, "taskbar_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.g_taskbar.area.paddingxlr = panel_config.g_taskbar.area.paddingx = atoi(value1);
		if (value2)
			panel_config.g_taskbar.area.paddingy = atoi(value2);
		if (value3)
			panel_config.g_taskbar.area.paddingx = atoi(value3);
	} else if (strcmp(key, "taskbar_background_id") == 0) {
		int id = atoi(value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.g_taskbar.background[TASKBAR_NORMAL] = &g_array_index(backgrounds, Background, id);
		if (panel_config.g_taskbar.background[TASKBAR_ACTIVE] == 0)
			panel_config.g_taskbar.background[TASKBAR_ACTIVE] = panel_config.g_taskbar.background[TASKBAR_NORMAL];
	} else if (strcmp(key, "taskbar_active_background_id") == 0) {
		int id = atoi(value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.g_taskbar.background[TASKBAR_ACTIVE] = &g_array_index(backgrounds, Background, id);
	} else if (strcmp(key, "taskbar_name") == 0) {
		taskbarname_enabled = atoi(value);
	} else if (strcmp(key, "taskbar_name_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.g_taskbar.area_name.paddingxlr = panel_config.g_taskbar.area_name.paddingx = atoi(value1);
		if (value2)
			panel_config.g_taskbar.area_name.paddingy = atoi(value2);
	} else if (strcmp(key, "taskbar_name_background_id") == 0) {
		int id = atoi(value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.g_taskbar.background_name[TASKBAR_NORMAL] = &g_array_index(backgrounds, Background, id);
		if (panel_config.g_taskbar.background_name[TASKBAR_ACTIVE] == 0)
			panel_config.g_taskbar.background_name[TASKBAR_ACTIVE] =
				panel_config.g_taskbar.background_name[TASKBAR_NORMAL];
	} else if (strcmp(key, "taskbar_name_active_background_id") == 0) {
		int id = atoi(value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.g_taskbar.background_name[TASKBAR_ACTIVE] = &g_array_index(backgrounds, Background, id);
	} else if (strcmp(key, "taskbar_name_font") == 0) {
		panel_config.taskbarname_font_desc = pango_font_description_from_string(value);
		panel_config.taskbarname_has_font = TRUE;
	} else if (strcmp(key, "taskbar_name_font_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, taskbarname_font.rgb);
		if (value2)
			taskbarname_font.alpha = (atoi(value2) / 100.0);
		else
			taskbarname_font.alpha = 0.5;
	} else if (strcmp(key, "taskbar_name_active_font_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, taskbarname_active_font.rgb);
		if (value2)
			taskbarname_active_font.alpha = (atoi(value2) / 100.0);
		else
			taskbarname_active_font.alpha = 0.5;
	} else if (strcmp(key, "taskbar_hide_inactive_tasks") == 0) {
		hide_inactive_tasks = atoi(value);
	} else if (strcmp(key, "taskbar_hide_different_monitor") == 0) {
		hide_task_diff_monitor = atoi(value);
	} else if (strcmp(key, "taskbar_hide_if_empty") == 0) {
		hide_taskbar_if_empty = atoi(value);
	} else if (strcmp(key, "taskbar_always_show_all_desktop_tasks") == 0) {
		always_show_all_desktop_tasks = atoi(value);
	} else if (strcmp(key, "taskbar_sort_order") == 0) {
		if (strcmp(value, "center") == 0) {
			taskbar_sort_method = TASKBAR_SORT_CENTER;
		} else if (strcmp(value, "title") == 0) {
			taskbar_sort_method = TASKBAR_SORT_TITLE;
		} else if (strcmp(value, "lru") == 0) {
			taskbar_sort_method = TASKBAR_SORT_LRU;
		} else if (strcmp(value, "mru") == 0) {
			taskbar_sort_method = TASKBAR_SORT_MRU;
		} else {
			taskbar_sort_method = TASKBAR_NOSORT;
		}
	} else if (strcmp(key, "task_align") == 0) {
		if (strcmp(value, "center") == 0) {
			taskbar_alignment = ALIGN_CENTER;
		} else if (strcmp(value, "right") == 0) {
			taskbar_alignment = ALIGN_RIGHT;
		} else {
			taskbar_alignment = ALIGN_LEFT;
		}
	}

	/* Task */
	else if (strcmp(key, "task_text") == 0)
		panel_config.g_task.has_text = atoi(value);
	else if (strcmp(key, "task_icon") == 0)
		panel_config.g_task.has_icon = atoi(value);
	else if (strcmp(key, "task_centered") == 0)
		panel_config.g_task.centered = atoi(value);
	else if (strcmp(key, "task_width") == 0) {
		// old parameter : just for backward compatibility
		panel_config.g_task.maximum_width = atoi(value);
		panel_config.g_task.maximum_height = 30;
	} else if (strcmp(key, "task_maximum_size") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.g_task.maximum_width = atoi(value1);
		if (value2)
			panel_config.g_task.maximum_height = atoi(value2);
		else
			panel_config.g_task.maximum_height = panel_config.g_task.maximum_width;
	} else if (strcmp(key, "task_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.g_task.area.paddingxlr = panel_config.g_task.area.paddingx = atoi(value1);
		if (value2)
			panel_config.g_task.area.paddingy = atoi(value2);
		if (value3)
			panel_config.g_task.area.paddingx = atoi(value3);
	} else if (strcmp(key, "task_font") == 0) {
		panel_config.g_task.font_desc = pango_font_description_from_string(value);
		panel_config.g_task.has_font = TRUE;
	} else if (g_regex_match_simple("task.*_font_color", key, 0, 0)) {
		gchar **split = g_regex_split_simple("_", key, 0, 0);
		int status = g_strv_length(split) == 3 ? TASK_NORMAL : get_task_status(split[1]);
		g_strfreev(split);
		if (status >= 0) {
			extract_values(value, &value1, &value2, &value3);
			float alpha = 1;
			if (value2)
				alpha = (atoi(value2) / 100.0);
			get_color(value1, panel_config.g_task.font[status].rgb);
			panel_config.g_task.font[status].alpha = alpha;
			panel_config.g_task.config_font_mask |= (1 << status);
		}
	} else if (g_regex_match_simple("task.*_icon_asb", key, 0, 0)) {
		gchar **split = g_regex_split_simple("_", key, 0, 0);
		int status = g_strv_length(split) == 3 ? TASK_NORMAL : get_task_status(split[1]);
		g_strfreev(split);
		if (status >= 0) {
			extract_values(value, &value1, &value2, &value3);
			panel_config.g_task.alpha[status] = atoi(value1);
			panel_config.g_task.saturation[status] = atoi(value2);
			panel_config.g_task.brightness[status] = atoi(value3);
			panel_config.g_task.config_asb_mask |= (1 << status);
		}
	} else if (g_regex_match_simple("task.*_background_id", key, 0, 0)) {
		gchar **split = g_regex_split_simple("_", key, 0, 0);
		int status = g_strv_length(split) == 3 ? TASK_NORMAL : get_task_status(split[1]);
		g_strfreev(split);
		if (status >= 0) {
			int id = atoi(value);
			id = (id < backgrounds->len && id >= 0) ? id : 0;
			panel_config.g_task.background[status] = &g_array_index(backgrounds, Background, id);
			panel_config.g_task.config_background_mask |= (1 << status);
			if (status == TASK_NORMAL)
				panel_config.g_task.area.bg = panel_config.g_task.background[TASK_NORMAL];
		}
	}
	// "tooltip" is deprecated but here for backwards compatibility
	else if (strcmp(key, "task_tooltip") == 0 || strcmp(key, "tooltip") == 0)
		panel_config.g_task.tooltip_enabled = atoi(value);

	/* Systray */
	else if (strcmp(key, "systray_padding") == 0) {
		if (!new_config_file && systray_enabled == 0) {
			systray_enabled = TRUE;
			if (panel_items_order) {
				gchar *tmp = g_strconcat(panel_items_order, "S", NULL);
				free(panel_items_order);
				panel_items_order = strdup(tmp);
				g_free(tmp);
			} else
				panel_items_order = strdup("S");
		}
		extract_values(value, &value1, &value2, &value3);
		systray.area.paddingxlr = systray.area.paddingx = atoi(value1);
		if (value2)
			systray.area.paddingy = atoi(value2);
		if (value3)
			systray.area.paddingx = atoi(value3);
	} else if (strcmp(key, "systray_background_id") == 0) {
		int id = atoi(value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		systray.area.bg = &g_array_index(backgrounds, Background, id);
	} else if (strcmp(key, "systray_sort") == 0) {
		if (strcmp(value, "descending") == 0)
			systray.sort = SYSTRAY_SORT_DESCENDING;
		else if (strcmp(value, "ascending") == 0)
			systray.sort = SYSTRAY_SORT_ASCENDING;
		else if (strcmp(value, "left2right") == 0)
			systray.sort = SYSTRAY_SORT_LEFT2RIGHT;
		else if (strcmp(value, "right2left") == 0)
			systray.sort = SYSTRAY_SORT_RIGHT2LEFT;
	} else if (strcmp(key, "systray_icon_size") == 0) {
		systray_max_icon_size = atoi(value);
	} else if (strcmp(key, "systray_icon_asb") == 0) {
		extract_values(value, &value1, &value2, &value3);
		systray.alpha = atoi(value1);
		systray.saturation = atoi(value2);
		systray.brightness = atoi(value3);
	} else if (strcmp(key, "systray_monitor") == 0) {
		systray_monitor = atoi(value) - 1;
	} else if (strcmp(key, "systray_name_filter") == 0) {
		if (systray_hide_name_filter)
			free(systray_hide_name_filter);
		systray_hide_name_filter = strdup(value);
	}

	/* Launcher */
	else if (strcmp(key, "launcher_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.launcher.area.paddingxlr = panel_config.launcher.area.paddingx = atoi(value1);
		if (value2)
			panel_config.launcher.area.paddingy = atoi(value2);
		if (value3)
			panel_config.launcher.area.paddingx = atoi(value3);
	} else if (strcmp(key, "launcher_background_id") == 0) {
		int id = atoi(value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		panel_config.launcher.area.bg = &g_array_index(backgrounds, Background, id);
	} else if (strcmp(key, "launcher_icon_background_id") == 0) {
		int id = atoi(value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		launcher_icon_bg = &g_array_index(backgrounds, Background, id);
	} else if (strcmp(key, "launcher_icon_size") == 0) {
		launcher_max_icon_size = atoi(value);
	} else if (strcmp(key, "launcher_item_app") == 0) {
		char *app = expand_tilde(value);
		panel_config.launcher.list_apps = g_slist_append(panel_config.launcher.list_apps, app);
	} else if (strcmp(key, "launcher_apps_dir") == 0) {
		char *path = expand_tilde(value);
		load_launcher_app_dir(path);
		free(path);
	} else if (strcmp(key, "launcher_icon_theme") == 0) {
		// if XSETTINGS manager running, tint2 use it.
		if (icon_theme_name_config)
			free(icon_theme_name_config);
		icon_theme_name_config = strdup(value);
	} else if (strcmp(key, "launcher_icon_theme_override") == 0) {
		launcher_icon_theme_override = atoi(value);
	} else if (strcmp(key, "launcher_icon_asb") == 0) {
		extract_values(value, &value1, &value2, &value3);
		launcher_alpha = atoi(value1);
		launcher_saturation = atoi(value2);
		launcher_brightness = atoi(value3);
	} else if (strcmp(key, "launcher_tooltip") == 0) {
		launcher_tooltip_enabled = atoi(value);
	} else if (strcmp(key, "startup_notifications") == 0) {
		startup_notifications = atoi(value);
	}

	/* Tooltip */
	else if (strcmp(key, "tooltip_show_timeout") == 0) {
		int timeout_msec = 1000 * atof(value);
		g_tooltip.show_timeout_msec = timeout_msec;
	} else if (strcmp(key, "tooltip_hide_timeout") == 0) {
		int timeout_msec = 1000 * atof(value);
		g_tooltip.hide_timeout_msec = timeout_msec;
	} else if (strcmp(key, "tooltip_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		if (value1)
			g_tooltip.paddingx = atoi(value1);
		if (value2)
			g_tooltip.paddingy = atoi(value2);
	} else if (strcmp(key, "tooltip_background_id") == 0) {
		int id = atoi(value);
		id = (id < backgrounds->len && id >= 0) ? id : 0;
		g_tooltip.bg = &g_array_index(backgrounds, Background, id);
	} else if (strcmp(key, "tooltip_font_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		get_color(value1, g_tooltip.font_color.rgb);
		if (value2)
			g_tooltip.font_color.alpha = (atoi(value2) / 100.0);
		else
			g_tooltip.font_color.alpha = 0.1;
	} else if (strcmp(key, "tooltip_font") == 0) {
		g_tooltip.font_desc = pango_font_description_from_string(value);
	}

	/* Mouse actions */
	else if (strcmp(key, "mouse_left") == 0)
		get_action(value, &mouse_left);
	else if (strcmp(key, "mouse_middle") == 0)
		get_action(value, &mouse_middle);
	else if (strcmp(key, "mouse_right") == 0)
		get_action(value, &mouse_right);
	else if (strcmp(key, "mouse_scroll_up") == 0)
		get_action(value, &mouse_scroll_up);
	else if (strcmp(key, "mouse_scroll_down") == 0)
		get_action(value, &mouse_scroll_down);
	else if (strcmp(key, "mouse_effects") == 0)
		panel_config.mouse_effects = atoi(value);
	else if (strcmp(key, "mouse_hover_icon_asb") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.mouse_over_alpha = atoi(value1);
		panel_config.mouse_over_saturation = atoi(value2);
		panel_config.mouse_over_brightness = atoi(value3);
	} else if (strcmp(key, "mouse_pressed_icon_asb") == 0) {
		extract_values(value, &value1, &value2, &value3);
		panel_config.mouse_pressed_alpha = atoi(value1);
		panel_config.mouse_pressed_saturation = atoi(value2);
		panel_config.mouse_pressed_brightness = atoi(value3);
	}

	/* autohide options */
	else if (strcmp(key, "autohide") == 0)
		panel_autohide = atoi(value);
	else if (strcmp(key, "autohide_show_timeout") == 0)
		panel_autohide_show_timeout = 1000 * atof(value);
	else if (strcmp(key, "autohide_hide_timeout") == 0)
		panel_autohide_hide_timeout = 1000 * atof(value);
	else if (strcmp(key, "strut_policy") == 0) {
		if (strcmp(value, "follow_size") == 0)
			panel_strut_policy = STRUT_FOLLOW_SIZE;
		else if (strcmp(value, "none") == 0)
			panel_strut_policy = STRUT_NONE;
		else
			panel_strut_policy = STRUT_MINIMUM;
	} else if (strcmp(key, "autohide_height") == 0) {
		panel_autohide_height = atoi(value);
		if (panel_autohide_height == 0) {
			// autohide need height > 0
			panel_autohide_height = 1;
		}
	}

	// old config option
	else if (strcmp(key, "systray") == 0) {
		if (!new_config_file) {
			systray_enabled = atoi(value);
			if (systray_enabled) {
				if (panel_items_order) {
					gchar *tmp = g_strconcat(panel_items_order, "S", NULL);
					free(panel_items_order);
					panel_items_order = strdup(tmp);
					g_free(tmp);
				} else
					panel_items_order = strdup("S");
			}
		}
	}
#ifdef ENABLE_BATTERY
	else if (strcmp(key, "battery") == 0) {
		if (!new_config_file) {
			battery_enabled = atoi(value);
			if (battery_enabled) {
				if (panel_items_order) {
					gchar *tmp = g_strconcat(panel_items_order, "B", NULL);
					free(panel_items_order);
					panel_items_order = strdup(tmp);
					g_free(tmp);
				} else
					panel_items_order = strdup("B");
			}
		}
	}
#endif
	else
		fprintf(stderr, "tint2 : invalid option \"%s\",\n  upgrade tint2 or correct your config file\n", key);

	if (value1)
		free(value1);
	if (value2)
		free(value2);
	if (value3)
		free(value3);
}

gboolean config_read_file(const char *path)
{
	FILE *fp = fopen(path, "r");
	if (!fp)
		return FALSE;

	char* line = NULL;
	size_t line_size = 0;
	while (getline(&line, &line_size, fp) >= 0) {
		char *key, *value;
		if (parse_line(line, &key, &value)) {
			add_entry(key, value);
			free(key);
			free(value);
		}
	}
	free(line);
	fclose(fp);

	if (!read_panel_position) {
		panel_horizontal = TRUE;
		panel_position = BOTTOM;
	}

	// append Taskbar item
	if (!new_config_file) {
		taskbar_enabled = TRUE;
		if (panel_items_order) {
			gchar *tmp = g_strconcat("T", panel_items_order, NULL);
			free(panel_items_order);
			panel_items_order = strdup(tmp);
			g_free(tmp);
		} else {
			panel_items_order = strdup("T");
		}
	}

	if (backgrounds->len > 0) {
		Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
		if (!read_bg_color_hover)
			memcpy(&bg->fill_color_hover, &bg->fill_color, sizeof(Color));
		if (!read_border_color_hover)
			memcpy(&bg->border_color_hover, &bg->border, sizeof(Color));
		if (!read_bg_color_press)
			memcpy(&bg->fill_color_pressed, &bg->fill_color_hover, sizeof(Color));
		if (!read_border_color_press)
			memcpy(&bg->border_color_pressed, &bg->border_color_hover, sizeof(Color));
	}

	return TRUE;
}

gboolean config_read_default_path()
{
	const gchar *const *system_dirs;
	gchar *path1;

	// follow XDG specification
	// check tint2rc in user directory
	path1 = g_build_filename(g_get_user_config_dir(), "tint2", "tint2rc", NULL);
	if (g_file_test(path1, G_FILE_TEST_EXISTS)) {
		gboolean result = config_read_file(path1);
		config_path = strdup(path1);
		g_free(path1);
		return result;
	}
	g_free(path1);

	// copy tint2rc from system directory to user directory

	fprintf(stderr, "tint2 warning: could not find a config file! Creating a default one.\n");
	// According to the XDG Base Directory Specification (https://specifications.freedesktop.org/basedir-spec/basedir-spec-0.6.html)
	// if the user's config directory does not exist, we should create it with permissions set to 0700.
	if (!g_file_test(g_get_user_config_dir(), G_FILE_TEST_IS_DIR))
		g_mkdir(g_get_user_config_dir(), 0700);

	gchar *path2 = 0;
	system_dirs = g_get_system_config_dirs();
	for (int i = 0; system_dirs[i]; i++) {
		path2 = g_build_filename(system_dirs[i], "tint2", "tint2rc", NULL);

		if (g_file_test(path2, G_FILE_TEST_EXISTS))
			break;
		g_free(path2);
		path2 = 0;
	}

	if (path2) {
		// copy file in user directory (path1)
		gchar *dir = g_build_filename(g_get_user_config_dir(), "tint2", NULL);
		if (!g_file_test(dir, G_FILE_TEST_IS_DIR))
			g_mkdir(dir, 0700);
		g_free(dir);

		path1 = g_build_filename(g_get_user_config_dir(), "tint2", "tint2rc", NULL);
		copy_file(path2, path1);
		g_free(path2);

		gboolean result = config_read_file(path1);
		config_path = strdup(path1);
		g_free(path1);
		return result;
	}

	// generate config file
	gchar *dir = g_build_filename(g_get_user_config_dir(), "tint2", NULL);
	if (!g_file_test(dir, G_FILE_TEST_IS_DIR))
		g_mkdir(dir, 0700);
	g_free(dir);

	path1 = g_build_filename(g_get_user_config_dir(), "tint2", "tint2rc", NULL);
	FILE *f = fopen(path1, "w");
	if (f) {
		fwrite(themes_tint2rc, 1, themes_tint2rc_len, f);
		fclose(f);
	}

	gboolean result = config_read_file(path1);
	config_path = strdup(path1);
	g_free(path1);
	return result;
}

gboolean config_read()
{
	if (config_path)
		return config_read_file(config_path);
	return config_read_default_path();
}

#endif
