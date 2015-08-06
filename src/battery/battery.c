/**************************************************************************
*
* Tint2 : battery
*
* Copyright (C) 2009-2015 Sebastian Reichel <sre@ring0.de>
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
int battery_tooltip_enabled;
int percentage_hide;
static timeout* battery_timeout;

static char buf_bat_percentage[10];
static char buf_bat_time[20];

int8_t battery_low_status;
unsigned char battery_low_cmd_sent;
char *battery_low_cmd;
char *battery_lclick_command;
char *battery_mclick_command;
char *battery_rclick_command;
char *battery_uwheel_command;
char *battery_dwheel_command;
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
	
	if (!battery_found) {
		init_battery();
	}
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
		if (!battery_found) {
			if (panel1[i].battery.area.on_screen == 1) {
				hide(&panel1[i].battery.area);
				panel_refresh = 1;
			}
		} else {
			if (battery_state.percentage >= percentage_hide) {
				if (panel1[i].battery.area.on_screen == 1) {
					hide(&panel1[i].battery.area);
					panel_refresh = 1;
				}
			} else {
				if (panel1[i].battery.area.on_screen == 0) {
					show(&panel1[i].battery.area);
					panel_refresh = 1;
				}
			}
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
	battery_tooltip_enabled = 1;
	battery_found = 0;
	percentage_hide = 101;
	battery_low_cmd_sent = 0;
	battery_timeout = NULL;
	bat1_font_desc = NULL;
	bat2_font_desc = NULL;
	battery_low_cmd = NULL;
	battery_lclick_command = NULL;
	battery_mclick_command = NULL;
	battery_rclick_command = NULL;
	battery_uwheel_command = NULL;
	battery_dwheel_command = NULL;
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
	free(battery_low_cmd);
	battery_low_cmd = NULL;
	free(battery_lclick_command);
	battery_lclick_command = NULL;
	free(battery_mclick_command);
	battery_mclick_command = NULL;
	free(battery_rclick_command);
	battery_rclick_command = NULL;
	free(battery_uwheel_command);
	battery_uwheel_command = NULL;
	free(battery_dwheel_command);
	battery_dwheel_command = NULL;
	stop_timeout(battery_timeout);
	battery_timeout = NULL;
	battery_found = 0;

#if defined(__OpenBSD__) || defined(__NetBSD__)
	if ((apm_fd != -1) && (close(apm_fd) == -1))
		warn("cannot close /dev/apm");
	apm_fd = -1;
#elif defined(__linux)
	free_linux_batteries();
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
	int sysctl_out = 0;
	size_t len = sizeof(sysctl_out);
	battery_found = (sysctlbyname("hw.acpi.battery.state", &sysctl_out, &len, NULL, 0) == 0) ||
					(sysctlbyname("hw.acpi.battery.time", &sysctl_out, &len, NULL, 0) == 0) ||
					(sysctlbyname("hw.acpi.battery.life", &sysctl_out, &len, NULL, 0) == 0);
#elif defined(__linux)
	battery_found = init_linux_batteries();
#endif

	if (!battery_timeout)
		battery_timeout = add_timeout(10, 30000, update_battery_tick, 0, &battery_timeout);
}

const char* battery_get_tooltip(void* obj) {
#if defined(__linux)
	return linux_batteries_get_tooltip();
#else
	return g_strdup("No tooltip support for this OS!");
#endif
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

	if (battery_tooltip_enabled)
		battery->area._get_tooltip_text = battery_get_tooltip;
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
	update_linux_batteries(&battery_state.state, &energy_now, &energy_full, &seconds);
#endif

	battery_state.time.hours = seconds / 3600;
	seconds -= 3600 * battery_state.time.hours;
	battery_state.time.minutes = seconds / 60;
	seconds -= 60 * battery_state.time.minutes;
	battery_state.time.seconds = seconds;

	if (energy_full > 0)
		new_percentage = 0.5 + ((energy_now <= energy_full ? energy_now : energy_full) * 100.0) / energy_full;

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
	draw_text(layout, c, 0, battery->bat1_posy, &battery->font, ((Panel*)battery->area.panel)->font_shadow);

	pango_layout_set_font_description(layout, bat2_font_desc);
	pango_layout_set_indent(layout, 0);
	pango_layout_set_text(layout, buf_bat_time, strlen(buf_bat_time));
	pango_layout_set_width(layout, battery->area.width * PANGO_SCALE);

	pango_cairo_update_layout(c, layout);
	draw_text(layout, c, 0, battery->bat2_posy, &battery->font, ((Panel*)battery->area.panel)->font_shadow);
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

void battery_action(int button)
{
	char *command = 0;
	switch (button) {
		case 1:
		command = battery_lclick_command;
		break;
		case 2:
		command = battery_mclick_command;
		break;
		case 3:
		command = battery_rclick_command;
		break;
		case 4:
		command = battery_uwheel_command;
		break;
		case 5:
		command = battery_dwheel_command;
		break;
	}
	tint_exec(command);
}
