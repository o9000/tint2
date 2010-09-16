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
#include "area.h"
#include "panel.h"
#include "taskbar.h"
#include "battery.h"
#include "clock.h"
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
unsigned char battery_low_cmd_send;
char *battery_low_cmd;
char *path_energy_now;
char *path_energy_full;
char *path_current_now;
char *path_status;

#if defined(__OpenBSD__) || defined(__NetBSD__)
int apm_fd;
#endif

void update_batterys(void* arg)
{
	int i;
	update_battery();
	for (i=0 ; i < nb_panel ; i++) {
		if (battery_state.percentage >= percentage_hide) {
			if (panel1[i].battery.area.on_screen == 1) {
				panel1[i].battery.area.on_screen = 0;
				// force resize on panel
				panel1[i].area.resize = 1;
				panel_refresh = 1;
			}
			continue;
		}
		else {
			if (panel1[i].battery.area.on_screen == 0) {
				panel1[i].battery.area.on_screen = 1;
				// force resize on panel
				panel1[i].area.resize = 1;
				panel_refresh = 1;
			}
		}
		panel1[i].battery.area.resize = 1;
	}
}

void default_battery()
{
	battery_enabled = 0;
	percentage_hide = 101;
	battery_low_cmd_send = 0;
	battery_timeout = 0;
	bat1_font_desc = 0;
	bat2_font_desc = 0;
	battery_low_cmd = 0;
	path_energy_now = 0;
	path_energy_full = 0;
	path_current_now = 0;
	path_status = 0;
#if defined(__OpenBSD__) || defined(__NetBSD__)
	apm_fd = -1;
#endif
}

void cleanup_battery()
{
	if (bat1_font_desc) pango_font_description_free(bat1_font_desc);
	if (bat2_font_desc) pango_font_description_free(bat2_font_desc);
	if (path_energy_now) g_free(path_energy_now);
	if (path_energy_full) g_free(path_energy_full);
	if (path_current_now) g_free(path_current_now);
	if (path_status) g_free(path_status);
	if (battery_low_cmd) g_free(battery_low_cmd);

#if defined(__OpenBSD__) || defined(__NetBSD__)
	if ((apm_fd != -1) && (close(apm_fd) == -1))
		warn("cannot close /dev/apm");
#endif
}


void init_battery()
{
	if (!battery_enabled) return;

#if defined(__OpenBSD__) || defined(__NetBSD__)
	apm_fd = open("/dev/apm", O_RDONLY);
	if (apm_fd < 0) {
		warn("init_battery: failed to open /dev/apm.");
		battery_enabled = 0;
		return;
	}

#elif !defined(__FreeBSD__)
	// check battery
	GDir *directory = 0;
	GError *error = NULL;
	const char *entryname;
	char *battery_dir = 0;

	directory = g_dir_open("/sys/class/power_supply", 0, &error);
	if (error)
		g_error_free(error);
	else {
		while ((entryname=g_dir_read_name(directory))) {
			if (strncmp(entryname,"AC", 2) == 0) continue;

			char *path1 = g_build_filename("/sys/class/power_supply", entryname, "present", NULL);
			if (g_file_test (path1, G_FILE_TEST_EXISTS)) {
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
		fprintf(stderr, "ERROR: battery applet can't found power_supply\n");
		default_battery();
		return;
	}

	char *path1 = g_build_filename(battery_dir, "energy_now", NULL);
	if (g_file_test (path1, G_FILE_TEST_EXISTS)) {
		path_energy_now = g_build_filename(battery_dir, "energy_now", NULL);
		path_energy_full = g_build_filename(battery_dir, "energy_full", NULL);
	}
	else {
		char *path2 = g_build_filename(battery_dir, "charge_now", NULL);
		if (g_file_test (path2, G_FILE_TEST_EXISTS)) {
			path_energy_now = g_build_filename(battery_dir, "charge_now", NULL);
			path_energy_full = g_build_filename(battery_dir, "charge_full", NULL);
		}
		else {
			fprintf(stderr, "ERROR: can't found energy_* or charge_*\n");
		}
		g_free(path2);
	}
	if (path_energy_now && path_energy_full) {
		path_current_now = g_build_filename(battery_dir, "current_now", NULL);
		path_status = g_build_filename(battery_dir, "status", NULL);

		// check file
		FILE *fp1, *fp2, *fp3, *fp4;
		fp1 = fopen(path_energy_now, "r");
		fp2 = fopen(path_energy_full, "r");
		fp3 = fopen(path_current_now, "r");
		fp4 = fopen(path_status, "r");
		if (fp1 == NULL || fp2 == NULL || fp3 == NULL || fp4 == NULL) {
			cleanup_battery();
			default_battery();
			fprintf(stderr, "ERROR: battery applet can't open energy_now\n");
		}
		fclose(fp1);
		fclose(fp2);
		fclose(fp3);
		fclose(fp4);
	}

	g_free(path1);
	g_free(battery_dir);
#endif

	if (battery_enabled && battery_timeout==0)
		battery_timeout = add_timeout(10, 10000, update_batterys, 0);
}


void init_battery_panel(void *p)
{
	Panel *panel = (Panel*)p;
	Battery *battery = &panel->battery;
	int bat_percentage_height, bat_percentage_height_ink, bat_time_height, bat_time_height_ink;

	if (!battery_enabled)
		return;

	battery->area.parent = p;
	battery->area.panel = p;
	battery->area._draw_foreground = draw_battery;
	battery->area.size_mode = SIZE_BY_CONTENT;
	battery->area._resize = resize_battery;
	battery->area.resize = 1;
	battery->area.redraw = 1;
	battery->area.on_screen = 1;

	update_battery(&battery_state);
	snprintf(buf_bat_percentage, sizeof(buf_bat_percentage), "%d%%", battery_state.percentage);
	snprintf(buf_bat_time, sizeof(buf_bat_time), "%02d:%02d", battery_state.time.hours, battery_state.time.minutes);

	get_text_size(bat1_font_desc, &bat_percentage_height_ink, &bat_percentage_height, panel->area.height, buf_bat_percentage, strlen(buf_bat_percentage));
	get_text_size(bat2_font_desc, &bat_time_height_ink, &bat_time_height, panel->area.height, buf_bat_time, strlen(buf_bat_time));

	if (panel_horizontal) {
		// panel horizonal => fixed height and posy
		battery->area.posy = panel->area.bg->border.width + panel->area.paddingy;
		battery->area.height = panel->area.height - (2 * battery->area.posy);
	}
	else {
		// panel vertical => fixed width, height, posy and posx
		battery->area.posy = panel->clock.area.posy + panel->clock.area.height + panel->area.paddingx;
		battery->area.height = (2 * battery->area.paddingxlr) + (bat_time_height + bat_percentage_height);
		battery->area.posx = panel->area.bg->border.width + panel->area.paddingy;
		battery->area.width = panel->area.width - (2 * panel->area.bg->border.width) - (2 * panel->area.paddingy);
	}

	battery->bat1_posy = (battery->area.height - bat_percentage_height) / 2;
	battery->bat1_posy -= ((bat_time_height_ink + 2) / 2);
	battery->bat2_posy = battery->bat1_posy + bat_percentage_height + 2 - (bat_percentage_height - bat_percentage_height_ink)/2 - (bat_time_height - bat_time_height_ink)/2;
}


void update_battery() {
#if !defined(__OpenBSD__) && !defined(__NetBSD__) && !defined(__FreeBSD__)
	// unused on OpenBSD, silence compiler warnings
	FILE *fp;
	char tmp[25];
	int64_t current_now = 0;
#endif
#if defined(__FreeBSD__)
	int sysctl_out =  0;
	size_t len = 0;
#endif
	int64_t energy_now = 0, energy_full = 0;
	int seconds = 0;
	int8_t new_percentage = 0;

#if defined(__OpenBSD__) || defined(__NetBSD__)
	struct apm_power_info info;
	if (ioctl(apm_fd, APM_IOC_GETPOWER, &(info)) < 0)
		warn("power update: APM_IOC_GETPOWER");

	// best attempt at mapping to linux battery states
	battery_state.state = BATTERY_UNKNOWN;
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

#elif defined(__FreeBSD__)
	len = sizeof(sysctl_out);

	if (sysctlbyname("hw.acpi.battery.state", &sysctl_out, &len, NULL, 0) != 0)
		fprintf(stderr, "power update: no such sysctl");

	// attemp to map the battery state to linux
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
	fp = fopen(path_status, "r");
	if(fp != NULL) {
		if (fgets(tmp, sizeof tmp, fp)) {
			battery_state.state = BATTERY_UNKNOWN;
			if(strcasecmp(tmp, "Charging\n")==0) battery_state.state = BATTERY_CHARGING;
			if(strcasecmp(tmp, "Discharging\n")==0) battery_state.state = BATTERY_DISCHARGING;
			if(strcasecmp(tmp, "Full\n")==0) battery_state.state = BATTERY_FULL;
		}
		fclose(fp);
	}

	fp = fopen(path_energy_now, "r");
	if(fp != NULL) {
		if (fgets(tmp, sizeof tmp, fp)) energy_now = atoi(tmp);
		fclose(fp);
	}

	fp = fopen(path_energy_full, "r");
	if(fp != NULL) {
		if (fgets(tmp, sizeof tmp, fp)) energy_full = atoi(tmp);
		fclose(fp);
	}

	fp = fopen(path_current_now, "r");
	if(fp != NULL) {
		if (fgets(tmp, sizeof tmp, fp)) current_now = atoi(tmp);
		fclose(fp);
	}

	if(current_now > 0) {
		switch(battery_state.state) {
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
	} else seconds = 0;
#endif

	battery_state.time.hours = seconds / 3600;
	seconds -= 3600 * battery_state.time.hours;
	battery_state.time.minutes = seconds / 60;
	seconds -= 60 * battery_state.time.minutes;
	battery_state.time.seconds = seconds;

	if(energy_full > 0)
		new_percentage = (energy_now*100)/energy_full;

	if(battery_low_status > new_percentage && battery_state.state == BATTERY_DISCHARGING && !battery_low_cmd_send) {
		tint_exec(battery_low_cmd);
		battery_low_cmd_send = 1;
	}
	if(battery_low_status < new_percentage && battery_state.state == BATTERY_CHARGING && battery_low_cmd_send) {
		battery_low_cmd_send = 0;
	}

	battery_state.percentage = new_percentage;

	// clamp percentage to 100 in case battery is misreporting that its current charge is more than its max
	if(battery_state.percentage > 100) {
		battery_state.percentage = 100;
	}
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
	PangoLayout *layout;
	int percentage_width, time_width, new_width, ret = 0;

	percentage_width = time_width = 0;
	battery->area.redraw = 1;

	snprintf(buf_bat_percentage, sizeof(buf_bat_percentage), "%d%%", battery_state.percentage);
	if(battery_state.state == BATTERY_FULL) {
		strcpy(buf_bat_time, "Full");
	} else {
		snprintf(buf_bat_time, sizeof(buf_bat_time), "%02d:%02d", battery_state.time.hours, battery_state.time.minutes);
	}
	// vertical panel doen't adjust width
	if (!panel_horizontal) return ret;

	cairo_surface_t *cs;
	cairo_t *c;
	Pixmap pmap;
	pmap = XCreatePixmap(server.dsp, server.root_win, battery->area.width, battery->area.height, server.depth);

	cs = cairo_xlib_surface_create(server.dsp, pmap, server.visual, battery->area.width, battery->area.height);
	c = cairo_create(cs);
	layout = pango_cairo_create_layout(c);

	// check width
	pango_layout_set_font_description(layout, bat1_font_desc);
	pango_layout_set_indent(layout, 0);
	pango_layout_set_text(layout, buf_bat_percentage, strlen(buf_bat_percentage));
	pango_layout_get_pixel_size(layout, &percentage_width, NULL);

	pango_layout_set_font_description(layout, bat2_font_desc);
	pango_layout_set_indent(layout, 0);
	pango_layout_set_text(layout, buf_bat_time, strlen(buf_bat_time));
	pango_layout_get_pixel_size(layout, &time_width, NULL);

	if(percentage_width > time_width) new_width = percentage_width;
	else new_width = time_width;

	new_width += (2*battery->area.paddingxlr) + (2*battery->area.bg->border.width);

	int old_width = battery->area.width;

	Panel *panel = ((Area*)obj)->panel;
	battery->area.width = new_width + 1;
	battery->area.posx = panel->area.width - battery->area.width - panel->area.paddingxlr - panel->area.bg->border.width;
	if (panel->clock.area.on_screen)
		battery->area.posx -= (panel->clock.area.width + panel->area.paddingx);

	if (new_width > old_width || new_width < (old_width-6)) {
		// refresh and resize other objects on panel
		// we try to limit the number of refresh
		// printf("battery_width %d, new_width %d\n", battery->area.width, new_width);
		ret = 1;
	}

	g_object_unref (layout);
	cairo_destroy (c);
	cairo_surface_destroy (cs);
	XFreePixmap (server.dsp, pmap);
	return ret;
}

