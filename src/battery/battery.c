/**************************************************************************
*
* Tint2 : Generic battery
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

#include "window.h"
#include "server.h"
#include "panel.h"
#include "battery.h"
#include "timer.h"
#include "common.h"

gboolean bat1_has_font;
PangoFontDescription *bat1_font_desc;
gboolean bat2_has_font;
PangoFontDescription *bat2_font_desc;
char *bat1_format;
char *bat2_format;
struct BatteryState battery_state;
gboolean battery_enabled;
gboolean battery_tooltip_enabled;
int percentage_hide;
static Timer battery_timeout;

#define BATTERY_BUF_SIZE 256
static char buf_bat_line1[BATTERY_BUF_SIZE];
static char buf_bat_line2[BATTERY_BUF_SIZE];

int8_t battery_low_status;
gboolean battery_low_cmd_sent;
gboolean battery_full_cmd_sent;
char *ac_connected_cmd;
char *ac_disconnected_cmd;
char *battery_low_cmd;
char *battery_full_cmd;
char *battery_lclick_command;
char *battery_mclick_command;
char *battery_rclick_command;
char *battery_uwheel_command;
char *battery_dwheel_command;
gboolean battery_found;

char *battery_sys_prefix = (char *)"";

void battery_init_fonts();
char *battery_get_tooltip(void *obj);
int battery_compute_desired_size(void *obj);
void battery_dump_geometry(void *obj, int indent);

void default_battery()
{
    battery_enabled = FALSE;
    battery_tooltip_enabled = TRUE;
    battery_found = FALSE;
    percentage_hide = 101;
    battery_low_cmd_sent = FALSE;
    battery_full_cmd_sent = FALSE;
    INIT_TIMER(battery_timeout);
    bat1_has_font = FALSE;
    bat1_font_desc = NULL;
    bat1_format = NULL;
    bat2_has_font = FALSE;
    bat2_font_desc = NULL;
    bat2_format = NULL;
    ac_connected_cmd = NULL;
    ac_disconnected_cmd = NULL;
    battery_low_cmd = NULL;
    battery_full_cmd = NULL;
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
}

void cleanup_battery()
{
    pango_font_description_free(bat1_font_desc);
    bat1_font_desc = NULL;
    pango_font_description_free(bat2_font_desc);
    bat2_font_desc = NULL;
    free(battery_low_cmd);
    battery_low_cmd = NULL;
    free(battery_full_cmd);
    battery_full_cmd = NULL;
    free(bat1_format);
    bat1_format = NULL;
    free(bat2_format);
    bat2_format = NULL;
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
    free(ac_connected_cmd);
    ac_connected_cmd = NULL;
    free(ac_disconnected_cmd);
    ac_disconnected_cmd = NULL;
    stop_timer(&battery_timeout);
    battery_found = FALSE;

    battery_os_free();
}

// Appends addendum to dest, and does not allow dest to grow over limit (including NULL terminator).
char *strnappend(char *dest, const char *addendum, size_t limit)
{
    char *tmp = strdup(dest);

    // Actually concatenate them.
    snprintf(dest, limit, "%s%s", tmp, addendum);

    free(tmp);
    return dest;
}

void battery_update_text(char *dest, char *format)
{
    if (!battery_enabled || !dest || !format)
        return;
    // We want to loop over the format specifier, replacing any known symbols with our battery data.
    // First, erase anything already stored in the buffer.
    // This ensures the string will always be null-terminated.
    bzero(dest, BATTERY_BUF_SIZE);

    for (size_t o = 0; o < strlen(format); o++) {
        char buf[BATTERY_BUF_SIZE];
        bzero(buf, BATTERY_BUF_SIZE);
        char *c = &format[o];
        // Format specification:
        // %s :	State (charging, discharging, full, unknown)
        // %m :	Minutes left (estimated).
        // %h : Hours left (estimated).
        // %t :	Time left. This is equivalent to the old behaviour; i.e. "(plugged in)" or "hrs:mins" otherwise.
        // %p :	Percentage left. Includes the % sign.
        if (*c == '%') {
            c++;
            o++; // Skip the format control character.
            switch (*c) {
            case 's':
                // Append the appropriate status message to the string.
                strnappend(dest,
                        (battery_state.state == BATTERY_CHARGING)
                            ? "Charging"
                            : (battery_state.state == BATTERY_DISCHARGING)
                                  ? "Discharging"
                                  : (battery_state.state == BATTERY_FULL)
                                        ? "Full"
                                        : "Unknown",
                        BATTERY_BUF_SIZE);
                break;
            case 'm':
                snprintf(buf, sizeof(buf), "%02d", battery_state.time.minutes);
                strnappend(dest, buf, BATTERY_BUF_SIZE);
                break;
            case 'h':
                snprintf(buf, sizeof(buf), "%02d", battery_state.time.hours);
                strnappend(dest, buf, BATTERY_BUF_SIZE);
                break;
            case 'p':
                snprintf(buf, sizeof(buf), "%d%%", battery_state.percentage);
                strnappend(dest, buf, BATTERY_BUF_SIZE);
                break;
            case 't':
                if (battery_state.state == BATTERY_FULL) {
                    snprintf(buf, sizeof(buf), "Full");
                    strnappend(dest, buf, BATTERY_BUF_SIZE);
                } else if (battery_state.time.hours > 0 && battery_state.time.minutes > 0) {
                    snprintf(buf, sizeof(buf), "%02d:%02d", battery_state.time.hours, battery_state.time.minutes);
                    strnappend(dest, buf, BATTERY_BUF_SIZE);
                }
                break;

            case '%':
            case '\0':
                strnappend(dest, "%", BATTERY_BUF_SIZE);
                break;
            default:
                fprintf(stderr, "tint2: Battery: unrecognised format specifier '%%%c'.\n", *c);
                buf[0] = *c;
                strnappend(dest, buf, BATTERY_BUF_SIZE);
            }
        } else {
            buf[0] = *c;
            strnappend(dest, buf, BATTERY_BUF_SIZE);
        }
    }
}

void init_battery()
{
    if (!battery_enabled)
        return;

    battery_found = battery_os_init();

    change_timer(&battery_timeout, true, 10, 30000, update_battery_tick, 0);

    update_battery();
}

void reinit_battery()
{
    battery_os_free();
    battery_found = battery_os_init();
    update_battery();
}

void init_battery_panel(void *p)
{
    Panel *panel = (Panel *)p;
    Battery *battery = &panel->battery;

    if (!battery_enabled)
        return;

    battery_init_fonts();

    if (!battery->area.bg)
        battery->area.bg = &g_array_index(backgrounds, Background, 0);

    battery->area.parent = p;
    battery->area.panel = p;
    snprintf(battery->area.name, sizeof(battery->area.name), "Battery");
    battery->area._draw_foreground = draw_battery;
    battery->area.size_mode = LAYOUT_FIXED;
    battery->area._resize = resize_battery;
    battery->area._compute_desired_size = battery_compute_desired_size;
    battery->area._is_under_mouse = full_width_area_is_under_mouse;
    battery->area.on_screen = TRUE;
    battery->area.resize_needed = 1;
    battery->area.has_mouse_over_effect =
        panel_config.mouse_effects && (battery_lclick_command || battery_mclick_command || battery_rclick_command ||
                                       battery_uwheel_command || battery_dwheel_command);
    battery->area.has_mouse_press_effect = battery->area.has_mouse_over_effect;
    if (battery_tooltip_enabled)
        battery->area._get_tooltip_text = battery_get_tooltip;
    instantiate_area_gradients(&battery->area);

    if (!bat1_format && !bat2_format) {
        bat1_format = strdup("%p");
        bat2_format = strdup("%t");
    }
}

void battery_init_fonts()
{
    if (!bat1_font_desc) {
        bat1_font_desc = pango_font_description_from_string(get_default_font());
        pango_font_description_set_size(bat1_font_desc, pango_font_description_get_size(bat1_font_desc) - PANGO_SCALE);
    }
    if (!bat2_font_desc) {
        bat2_font_desc = pango_font_description_from_string(get_default_font());
        pango_font_description_set_size(bat2_font_desc, pango_font_description_get_size(bat2_font_desc) - PANGO_SCALE);
    }
}

void battery_default_font_changed()
{
    if (!battery_enabled)
        return;
    if (bat1_has_font && bat2_has_font)
        return;
    if (!bat1_has_font) {
        pango_font_description_free(bat1_font_desc);
        bat1_font_desc = NULL;
    }
    if (!bat2_has_font) {
        pango_font_description_free(bat2_font_desc);
        bat2_font_desc = NULL;
    }
    battery_init_fonts();
    for (int i = 0; i < num_panels; i++) {
        panels[i].battery.area.resize_needed = TRUE;
        schedule_redraw(&panels[i].battery.area);
    }
    schedule_panel_redraw();
}

void update_battery_tick(void *arg)
{
    if (!battery_enabled)
        return;

    gboolean old_found = battery_found;
    int old_percentage = battery_state.percentage;
    gboolean old_ac_connected = battery_state.ac_connected;
    int16_t old_hours = battery_state.time.hours;
    int8_t old_minutes = battery_state.time.minutes;

    if (!battery_found) {
        init_battery();
        old_ac_connected = battery_state.ac_connected;
    }
    if (update_battery() != 0) {
        // Try to reconfigure on failed update
        init_battery();
    }

    if (old_ac_connected != battery_state.ac_connected) {
        if (battery_state.ac_connected)
            tint_exec_no_sn(ac_connected_cmd);
        else
            tint_exec_no_sn(ac_disconnected_cmd);
    }

    if (battery_state.percentage < battery_low_status && battery_state.state == BATTERY_DISCHARGING &&
        !battery_low_cmd_sent) {
        tint_exec_no_sn(battery_low_cmd);
        battery_low_cmd_sent = TRUE;
    }
    if (battery_state.percentage > battery_low_status && battery_state.state == BATTERY_CHARGING &&
        battery_low_cmd_sent) {
        battery_low_cmd_sent = FALSE;
    }

    if ((battery_state.percentage >= 100 || battery_state.state == BATTERY_FULL) &&
        !battery_full_cmd_sent) {
        tint_exec_no_sn(battery_full_cmd);
        battery_full_cmd_sent = TRUE;
    }
    if (battery_state.percentage < 100 && battery_state.state != BATTERY_FULL &&
        battery_full_cmd_sent) {
        battery_full_cmd_sent = FALSE;
    }

    for (int i = 0; i < num_panels; i++) {
        // Show/hide if needed
        if (!battery_found) {
            hide(&panels[i].battery.area);
        } else {
            if (battery_state.percentage >= percentage_hide)
                hide(&panels[i].battery.area);
            else
                show(&panels[i].battery.area);
        }
        // Redraw if needed
        if (panels[i].battery.area.on_screen) {
            if (old_found != battery_found || old_percentage != battery_state.percentage ||
                old_hours != battery_state.time.hours || old_minutes != battery_state.time.minutes) {
                panels[i].battery.area.resize_needed = TRUE;
                schedule_panel_redraw();
            }
        }
    }
}

int update_battery()
{
    // Reset
    battery_state.state = BATTERY_UNKNOWN;
    battery_state.percentage = 0;
    battery_state.ac_connected = FALSE;
    battery_state_set_time(&battery_state, 0);

    int err = battery_os_update(&battery_state);

    // Clamp percentage to 100 in case battery is misreporting that its current charge is more than its max
    if (battery_state.percentage > 100) {
        battery_state.percentage = 100;
    }

    battery_update_text(buf_bat_line1, bat1_format);
    if (bat2_format != 0) {
        battery_update_text(buf_bat_line2, bat2_format);
    }

    return err;
}

int battery_compute_desired_size(void *obj)
{
    Battery *battery = (Battery *)obj;
    return text_area_compute_desired_size(&battery->area, buf_bat_line1, buf_bat_line2, bat1_font_desc, bat2_font_desc);
}

gboolean resize_battery(void *obj)
{
    Battery *battery = (Battery *)obj;
    return resize_text_area(&battery->area,
                            buf_bat_line1,
                            buf_bat_line2,
                            bat1_font_desc,
                            bat2_font_desc,
                            &battery->bat1_posy,
                            &battery->bat2_posy);
}

void draw_battery(void *obj, cairo_t *c)
{
    Battery *battery = (Battery *)obj;
    draw_text_area(&battery->area,
                   c,
                   buf_bat_line1,
                   buf_bat_line2,
                   bat1_font_desc,
                   bat2_font_desc,
                   battery->bat1_posy,
                   battery->bat2_posy,
                   &battery->font_color);
}

void battery_dump_geometry(void *obj, int indent)
{
    Battery *battery = (Battery *)obj;
    fprintf(stderr, "tint2: %*sText 1: y = %d, text = %s\n", indent, "", battery->bat1_posy, buf_bat_line1);
    fprintf(stderr, "tint2: %*sText 2: y = %d, text = %s\n", indent, "", battery->bat2_posy, buf_bat_line2);
}

char *battery_get_tooltip(void *obj)
{
    return battery_os_tooltip();
}

void battery_action(void *obj, int button, int x, int y, Time time)
{
    char *command = NULL;
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
    tint_exec(command, NULL, NULL, time, obj, x, y, FALSE, TRUE);
}
