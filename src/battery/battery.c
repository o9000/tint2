/**************************************************************************
*
* Tint2 : battery
*
* Copyright (C) 2009 Sebastian Reichel <elektranox@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* or any later version as published by the Free Software Foundation.
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
#include <stdlib.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <pango/pangocairo.h>

#if defined(__OpenBSD__) || defined(__NetBSD__)
#include <machine/apmvar.h>
#include <err.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#if defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include "window.h"
#include "server.h"
#include "panel.h"
#include "battery.h"
#include "timer.h"
#include "common.h"

PangoFontDescription *bat1_font_desc;
PangoFontDescription *bat2_font_desc;
struct batstate battery_state;
int battery_enabled;
int percentage_hide;
static timeout* battery_timeout;

static char buf_bat_percentage[10];
static char buf_bat_time[20];

int8_t battery_low_status;
unsigned char battery_low_cmd_sent;
char *battery_low_cmd;
gchar *path_energy_now;
gchar *path_energy_full;
gchar *path_current_now;
gchar *path_status;
int battery_found;

#if defined(__OpenBSD__) || defined(__NetBSD__)
int apm_fd;
#endif

void update_battery_tick(void* arg)
{
	if (!battery_enabled)
		return;

	int old_found = battery_found;
	int old_percentage = battery_state.percentage;
	int16_t old_hours = battery_state.time.hours;
	int8_t old_minutes = battery_state.time.minutes;
	
	if (update_battery() != 0) {
		// Reconfigure
		init_battery();
		// Try again
		update_battery();
	}
	if (old_found == battery_found &&
		old_percentage == battery_state.percentage &&
		old_hours == battery_state.time.hours &&
		old_minutes == battery_state.time.minutes) {
		return;
	}

	if (battery_state.percentage < battery_low_status &&
		battery_state.state == BATTERY_DISCHARGING &&
		!battery_low_cmd_sent) {
		tint_exec(battery_low_cmd);
		battery_low_cmd_sent = 1;
	}
	if (battery_state.percentage > battery_low_status &&
		battery_state.state == BATTERY_CHARGING &&
		battery_low_cmd_sent) {
		battery_low_cmd_sent = 0;
	}

	int i;
	for (i = 0; i < nb_panel; i++) {
		if (!battery_found && panel1[i].battery.area.on_screen == 1) {
			hide(&panel1[i].battery.area);
			panel_refresh = 1;
		} else if (battery_state.percentage >= percentage_hide && panel1[i].battery.area.on_screen == 1) {
			hide(&panel1[i].battery.area);
			panel_refresh = 1;
		} else if (battery_state.percentage < percentage_hide && panel1[i].battery.area.on_screen == 0) {
			show(&panel1[i].battery.area);
			panel_refresh = 1;
		}
		if (panel1[i].battery.area.on_screen == 1) {
			panel1[i].battery.area.resize = 1;
			panel_refresh = 1;
		}
	}
}

void default_battery()
{
	battery_enabled = 0;
	battery_found = 0;
	percentage_hide = 101;
	battery_low_cmd_sent = 0;
	battery_timeout = NULL;
	bat1_font_desc = NULL;
	bat2_font_desc = NULL;
	battery_low_cmd = NULL;
	path_energy_now = NULL;
	path_energy_full = NULL;
	path_current_now = NULL;
	path_status = NULL;
	battery_state.percentage = 0;
	battery_state.time.hours = 0;
	battery_state.time.minutes = 0;
	battery_state.time.seconds = 0;
	battery_state.state = BATTERY_UNKNOWN;
#if defined(__OpenBSD__) || defined(__NetBSD__)
	apm_fd = -1;
#endif
}

void cleanup_battery()
{
	pango_font_description_free(bat1_font_desc);
	bat1_font_desc = NULL;
	pango_font_description_free(bat2_font_desc);
	bat2_font_desc = NULL;
	g_free(path_energy_now);
	path_energy_now = NULL;
	g_free(path_energy_full);
	path_energy_full = NULL;
	g_free(path_current_now);
	path_current_now = NULL;
	g_free(path_status);
	path_status = NULL;
	free(battery_low_cmd);
	battery_low_cmd = NULL;
	stop_timeout(battery_timeout);
	battery_timeout = NULL;
	battery_found = 0;

#if defined(__OpenBSD__) || defined(__NetBSD__)
	if ((apm_fd != -1) && (close(apm_fd) == -1))
		warn("cannot close /dev/apm");
	apm_fd = -1;
#endif
}

void init_battery()
{
	if (!battery_enabled)
		return;
	battery_found = 0;

#if defined(__OpenBSD__) || defined(__NetBSD__)
	if (apm_fd > 0)
		close(apm_fd);
	apm_fd = open("/dev/apm", O_RDONLY);
	if (apm_fd < 0) {
		warn("ERROR: battery applet cannot open /dev/apm.");
		battery_found = 0;
	} else {
		battery_found = 1;
	}
#elif defined(__FreeBSD__)
	// Nothing to do
#else // Linux
	GDir *directory = 0;
	GError *error = NULL;
	const char *entryname;
	gchar *battery_dir = 0;

	directory = g_dir_open("/sys/class/power_supply", 0, &error);
	if (error) {
		g_error_free(error);
	} else {
		while ((entryname = g_dir_read_name(directory))) {
			if (strncmp(entryname, "AC", 2) == 0)
				continue;

			gchar *path1 = g_build_filename("/sys/class/power_supply", entryname, "present", NULL);
			if (g_file_test(path1, G_FILE_TEST_EXISTS)) {
				g_free(path1);
				battery_dir = g_build_filename("/sys/class/power_supply", entryname, NULL);
				break;
			}
			g_free(path1);
		}
	}
	if (directory)
		g_dir_close(directory);
	if (!battery_dir) {
		fprintf(stderr, "ERROR: battery applet cannot find any battery\n");
		battery_found = 0;
	} else {
		battery_found = 1;

		g_free(path_energy_now);
		path_energy_now = g_build_filename(battery_dir, "energy_now", NULL);
		if (!g_file_test(path_energy_now, G_FILE_TEST_EXISTS)) {
			g_free(path_energy_now);
			path_energy_now = g_build_filename(battery_dir, "charge_now", NULL);
		}
		if (!g_file_test(path_energy_now, G_FILE_TEST_EXISTS)) {
			fprintf(stderr, "ERROR: battery applet cannot find energy_now nor charge_now\n");
			g_free(path_energy_now);
			path_energy_now = NULL;
		}

		g_free(path_energy_full);
		path_energy_full = g_build_filename(battery_dir, "energy_full", NULL);
		if (!g_file_test(path_energy_full, G_FILE_TEST_EXISTS)) {
			g_free(path_energy_full);
			path_energy_full = g_build_filename(battery_dir, "charge_full", NULL);
		}
		if (!g_file_test(path_energy_full, G_FILE_TEST_EXISTS)) {
			fprintf(stderr, "ERROR: battery applet cannot find energy_now nor charge_now\n");
			g_free(path_energy_full);
			path_energy_full = NULL;
		}

		g_free(path_current_now);
		path_current_now = g_build_filename(battery_dir, "power_now", NULL);
		if (!g_file_test(path_current_now, G_FILE_TEST_EXISTS)) {
			g_free(path_current_now);
			path_current_now = g_build_filename(battery_dir, "current_now", NULL);
		}
		if (!g_file_test(path_current_now, G_FILE_TEST_EXISTS)) {
			fprintf(stderr, "ERROR: battery applet cannot find power_now nor current_now\n");
			g_free(path_current_now);
			path_current_now = NULL;
		}

		g_free(path_status);
		path_status = g_build_filename(battery_dir, "status", NULL);
		if (!g_file_test(path_status, G_FILE_TEST_EXISTS)) {
			fprintf(stderr, "ERROR: battery applet cannot find battery status\n");
			g_free(path_status);
			path_status = NULL;
		}

		g_free(battery_dir);
		battery_dir = NULL;
	}

	if (!path_status) {
		battery_found = 0;
		fprintf(stderr, "ERROR: battery applet cannot find any batteries\n");
	}
#endif

	if (battery_timeout == 0) {
		battery_timeout = add_timeout(10, 10000, update_battery_tick, 0);
	}
}


void init_battery_panel(void *p)
{
	Panel *panel = (Panel*)p;
	Battery *battery = &panel->battery;

	if (!battery_enabled)
		return;

	if (!bat1_font_desc)
		bat1_font_desc = pango_font_description_from_string(DEFAULT_FONT);
	if (!bat2_font_desc)
		bat2_font_desc = pango_font_description_from_string(DEFAULT_FONT);

	if (battery->area.bg == 0)
		battery->area.bg = &g_array_index(backgrounds, Background, 0);

	battery->area.parent = p;
	battery->area.panel = p;
	battery->area._draw_foreground = draw_battery;
	battery->area.size_mode = SIZE_BY_CONTENT;
	battery->area._resize = resize_battery;
	battery->area.on_screen = 1;
	battery->area.resize = 1;
}


int update_battery() {
	int64_t energy_now = 0,
			energy_full = 0;
	int seconds = 0;
	int8_t new_percentage = 0;
	int errors = 0;

	battery_state.state = BATTERY_UNKNOWN;

#if defined(__OpenBSD__) || defined(__NetBSD__)
	struct apm_power_info info;
	if (apm_fd > 0 && ioctl(apm_fd, APM_IOC_GETPOWER, &(info)) == 0) {
		// best attempt at mapping to Linux battery states
		switch (info.battery_state) {
		case APM_BATT_CHARGING:
			battery_state.state = BATTERY_CHARGING;
			break;
		default:
			battery_state.state = BATTERY_DISCHARGING;
			break;
		}

		if (info.battery_life == 100)
			battery_state.state = BATTERY_FULL;

		// no mapping for openbsd really
		energy_full = 0;
		energy_now = 0;

		if (info.minutes_left != -1)
			seconds = info.minutes_left * 60;
		else
			seconds = -1;

		new_percentage = info.battery_life;
	} else {
		warn("power update: APM_IOC_GETPOWER");
		errors = 1;
	}
#elif defined(__FreeBSD__)
	int sysctl_out = 0;
	size_t len = sizeof(sysctl_out);

	if (sysctlbyname("hw.acpi.battery.state", &sysctl_out, &len, NULL, 0) == 0) {
		// attemp to map the battery state to Linux
		battery_state.state = BATTERY_UNKNOWN;
		switch(sysctl_out) {
		case 1:
			battery_state.state = BATTERY_DISCHARGING;
			break;
		case 2:
			battery_state.state = BATTERY_CHARGING;
			break;
		default:
			battery_state.state = BATTERY_FULL;
			break;
		}
	} else {
		fprintf(stderr, "power update: no such sysctl");
		errors = 1;
	}

	// no mapping for freebsd
	energy_full = 0;
	energy_now = 0;

	if (sysctlbyname("hw.acpi.battery.time", &sysctl_out, &len, NULL, 0) != 0)
		seconds = -1;
	else
		seconds = sysctl_out * 60;

	// charging or error
	if (seconds < 0)
		seconds = 0;

	if (sysctlbyname("hw.acpi.battery.life", &sysctl_out, &len, NULL, 0) != 0)
		new_percentage = -1;
	else
		new_percentage = sysctl_out;
#else
	FILE *fp = NULL;
	char tmp[25] = "";
	int64_t current_now = 0;
	if (path_status) {
		fp = fopen(path_status, "r");
		if (fp != NULL) {
			if (fgets(tmp, sizeof(tmp), fp)) {
				if (strcasecmp(tmp, "Charging\n") == 0)
					battery_state.state = BATTERY_CHARGING;
				if (strcasecmp(tmp, "Discharging\n") == 0)
					battery_state.state = BATTERY_DISCHARGING;
				if (strcasecmp(tmp, "Full\n") == 0)
					battery_state.state = BATTERY_FULL;
			}
			fclose(fp);
		} else {
			errors = 1;
		}
	} else {
		errors = 1;
	}

	if (path_energy_now) {
		fp = fopen(path_energy_now, "r");
		if (fp != NULL) {
			if (fgets(tmp, sizeof tmp, fp))
				energy_now = atoi(tmp);
			fclose(fp);
		} else {
			errors = 1;
		}
	} else {
		errors = 1;
	}

	if (path_energy_full) {
		fp = fopen(path_energy_full, "r");
		if (fp != NULL) {
			if (fgets(tmp, sizeof tmp, fp))
				energy_full = atoi(tmp);
			fclose(fp);
		} else {
			errors = 1;
		}
	} else {
		errors = 1;
	}

	if (path_current_now) {
		fp = fopen(path_current_now, "r");
		if (fp != NULL) {
			if (fgets(tmp, sizeof tmp, fp))
				current_now = atoi(tmp);
			fclose(fp);
		} else {
			errors = 1;
		}
	} else {
		errors = 1;
	}

	if (current_now > 0) {
		switch (battery_state.state) {
		case BATTERY_CHARGING:
			seconds = 3600 * (energy_full - energy_now) / current_now;
			break;
		case BATTERY_DISCHARGING:
			seconds = 3600 * energy_now / current_now;
			break;
		default:
			seconds = 0;
			break;
		}
	} else {
		seconds = 0;
	}
#endif

	battery_state.time.hours = seconds / 3600;
	seconds -= 3600 * battery_state.time.hours;
	battery_state.time.minutes = seconds / 60;
	seconds -= 60 * battery_state.time.minutes;
	battery_state.time.seconds = seconds;

	if (energy_full > 0)
		new_percentage = ((energy_now <= energy_full ? energy_now : energy_full) * 100) / energy_full;

	battery_state.percentage = new_percentage;

	// clamp percentage to 100 in case battery is misreporting that its current charge is more than its max
	if (battery_state.percentage > 100) {
		battery_state.percentage = 100;
	}

	return errors;
}


void draw_battery (void *obj, cairo_t *c)
{
	Battery *battery = obj;
	PangoLayout *layout;

	layout = pango_cairo_create_layout (c);

	// draw layout
	pango_layout_set_font_description(layout, bat1_font_desc);
	pango_layout_set_width(layout, battery->area.width * PANGO_SCALE);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
	pango_layout_set_text(layout, buf_bat_percentage, strlen(buf_bat_percentage));

	cairo_set_source_rgba(c, battery->font.color[0], battery->font.color[1], battery->font.color[2], battery->font.alpha);

	pango_cairo_update_layout(c, layout);
	cairo_move_to(c, 0, battery->bat1_posy);
	pango_cairo_show_layout(c, layout);

	pango_layout_set_font_description(layout, bat2_font_desc);
	pango_layout_set_indent(layout, 0);
	pango_layout_set_text(layout, buf_bat_time, strlen(buf_bat_time));
	pango_layout_set_width(layout, battery->area.width * PANGO_SCALE);

	pango_cairo_update_layout(c, layout);
	cairo_move_to(c, 0, battery->bat2_posy);
	pango_cairo_show_layout(c, layout);

	g_object_unref(layout);
}


int resize_battery(void *obj)
{
	Battery *battery = obj;
	Panel *panel = battery->area.panel;
	int bat_percentage_height, bat_percentage_width, bat_percentage_height_ink;
	int bat_time_height, bat_time_width, bat_time_height_ink;
	int ret = 0;

	battery->area.redraw = 1;
	
	snprintf(buf_bat_percentage, sizeof(buf_bat_percentage), "%d%%", battery_state.percentage);
	if (battery_state.state == BATTERY_FULL) {
		strcpy(buf_bat_time, "Full");
	} else {
		snprintf(buf_bat_time, sizeof(buf_bat_time), "%02d:%02d", battery_state.time.hours, battery_state.time.minutes);
	}
	get_text_size2(bat1_font_desc, &bat_percentage_height_ink, &bat_percentage_height, &bat_percentage_width,
				   panel->area.height, panel->area.width, buf_bat_percentage, strlen(buf_bat_percentage));
	get_text_size2(bat2_font_desc, &bat_time_height_ink, &bat_time_height, &bat_time_width,
				   panel->area.height, panel->area.width, buf_bat_time, strlen(buf_bat_time));

	if (panel_horizontal) {
		int new_size = (bat_percentage_width > bat_time_width) ? bat_percentage_width : bat_time_width;
		new_size += 2 * battery->area.paddingxlr + 2 * battery->area.bg->border.width;
		if (new_size > battery->area.width ||
			new_size < battery->area.width - 2) {
			// we try to limit the number of resize
			battery->area.width = new_size;
			battery->bat1_posy = (battery->area.height - bat_percentage_height - bat_time_height) / 2;
			battery->bat2_posy = battery->bat1_posy + bat_percentage_height;
			ret = 1;
		}
	} else {
		int new_size = bat_percentage_height + bat_time_height +
					   (2 * (battery->area.paddingxlr + battery->area.bg->border.width));
		if (new_size > battery->area.height ||
			new_size < battery->area.height - 2) {
			battery->area.height =  new_size;
			battery->bat1_posy = (battery->area.height - bat_percentage_height - bat_time_height - 2) / 2;
			battery->bat2_posy = battery->bat1_posy + bat_percentage_height + 2;
			ret = 1;
		}
	}
	return ret;
}
