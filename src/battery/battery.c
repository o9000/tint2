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
struct BatteryState battery_state;
gboolean battery_enabled;
gboolean battery_tooltip_enabled;
int percentage_hide;
static timeout *battery_timeout;

static char buf_bat_percentage[10];
static char buf_bat_time[20];

int8_t battery_low_status;
gboolean battery_low_cmd_sent;
char *ac_connected_cmd;
char *ac_disconnected_cmd;
char *battery_low_cmd;
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
    battery_timeout = NULL;
    bat1_has_font = FALSE;
    bat1_font_desc = NULL;
    bat2_has_font = FALSE;
    bat2_font_desc = NULL;
    ac_connected_cmd = NULL;
    ac_disconnected_cmd = NULL;
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
    free(ac_connected_cmd);
    ac_connected_cmd = NULL;
    free(ac_disconnected_cmd);
    ac_disconnected_cmd = NULL;
    stop_timeout(battery_timeout);
    battery_timeout = NULL;
    battery_found = FALSE;

    battery_os_free();
}

void init_battery()
{
    if (!battery_enabled)
        return;

    battery_found = battery_os_init();

    if (!battery_timeout)
        battery_timeout = add_timeout(10, 30000, update_battery_tick, 0, &battery_timeout);

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
            tint_exec(ac_connected_cmd);
        else
            tint_exec(ac_disconnected_cmd);
    }

    if (battery_state.percentage < battery_low_status && battery_state.state == BATTERY_DISCHARGING &&
        !battery_low_cmd_sent) {
        tint_exec(battery_low_cmd);
        battery_low_cmd_sent = TRUE;
    }
    if (battery_state.percentage > battery_low_status && battery_state.state == BATTERY_CHARGING &&
        battery_low_cmd_sent) {
        battery_low_cmd_sent = FALSE;
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

    return err;
}

int battery_compute_desired_size(void *obj)
{
    Battery *battery = (Battery *)obj;
    Panel *panel = (Panel *)battery->area.panel;
    int bat_percentage_height, bat_percentage_width, bat_percentage_height_ink;
    int bat_time_height, bat_time_width, bat_time_height_ink;

    snprintf(buf_bat_percentage, sizeof(buf_bat_percentage), "%d%%", battery_state.percentage);
    if (battery_state.state == BATTERY_FULL) {
        strcpy(buf_bat_time, "Full");
    } else {
        snprintf(buf_bat_time, sizeof(buf_bat_time), "%02d:%02d", battery_state.time.hours, battery_state.time.minutes);
    }
    get_text_size2(bat1_font_desc,
                   &bat_percentage_height_ink,
                   &bat_percentage_height,
                   &bat_percentage_width,
                   panel->area.height,
                   panel->area.width,
                   buf_bat_percentage,
                   strlen(buf_bat_percentage),
                   PANGO_WRAP_WORD_CHAR,
                   PANGO_ELLIPSIZE_NONE,
                   FALSE);
    get_text_size2(bat2_font_desc,
                   &bat_time_height_ink,
                   &bat_time_height,
                   &bat_time_width,
                   panel->area.height,
                   panel->area.width,
                   buf_bat_time,
                   strlen(buf_bat_time),
                   PANGO_WRAP_WORD_CHAR,
                   PANGO_ELLIPSIZE_NONE,
                   FALSE);

    if (panel_horizontal) {
        int new_size = (bat_percentage_width > bat_time_width) ? bat_percentage_width : bat_time_width;
        new_size += 2 * battery->area.paddingxlr + left_right_border_width(&battery->area);
        return new_size;
    } else {
        int new_size = bat_percentage_height + bat_time_height + 2 * battery->area.paddingxlr +
                       top_bottom_border_width(&battery->area);
        return new_size;
    }
}

gboolean resize_battery(void *obj)
{
    Battery *battery = (Battery *)obj;
    Panel *panel = (Panel *)battery->area.panel;
    int bat_percentage_height, bat_percentage_width, bat_percentage_height_ink;
    int bat_time_height, bat_time_width, bat_time_height_ink;
    int ret = 0;

    snprintf(buf_bat_percentage, sizeof(buf_bat_percentage), "%d%%", battery_state.percentage);
    if (battery_state.state == BATTERY_FULL) {
        strcpy(buf_bat_time, "Full");
    } else {
        snprintf(buf_bat_time, sizeof(buf_bat_time), "%02d:%02d", battery_state.time.hours, battery_state.time.minutes);
    }
    get_text_size2(bat1_font_desc,
                   &bat_percentage_height_ink,
                   &bat_percentage_height,
                   &bat_percentage_width,
                   panel->area.height,
                   panel->area.width,
                   buf_bat_percentage,
                   strlen(buf_bat_percentage),
                   PANGO_WRAP_WORD_CHAR,
                   PANGO_ELLIPSIZE_NONE,
                   FALSE);
    get_text_size2(bat2_font_desc,
                   &bat_time_height_ink,
                   &bat_time_height,
                   &bat_time_width,
                   panel->area.height,
                   panel->area.width,
                   buf_bat_time,
                   strlen(buf_bat_time),
                   PANGO_WRAP_WORD_CHAR,
                   PANGO_ELLIPSIZE_NONE,
                   FALSE);

    if (panel_horizontal) {
        int new_size = (bat_percentage_width > bat_time_width) ? bat_percentage_width : bat_time_width;
        new_size += 2 * battery->area.paddingxlr + left_right_border_width(&battery->area);
        if (new_size > battery->area.width || new_size < battery->area.width - 2) {
            // we try to limit the number of resize
            battery->area.width = new_size;
            battery->bat1_posy = (battery->area.height - bat_percentage_height - bat_time_height) / 2;
            battery->bat2_posy = battery->bat1_posy + bat_percentage_height;
            ret = 1;
        }
    } else {
        int new_size = bat_percentage_height + bat_time_height + 2 * battery->area.paddingxlr +
                       top_bottom_border_width(&battery->area);
        if (new_size > battery->area.height || new_size < battery->area.height - 2) {
            battery->area.height = new_size;
            battery->bat1_posy = (battery->area.height - bat_percentage_height - bat_time_height - 2) / 2;
            battery->bat2_posy = battery->bat1_posy + bat_percentage_height + 2;
            ret = 1;
        }
    }

    schedule_redraw(&battery->area);
    return ret;
}

void draw_battery(void *obj, cairo_t *c)
{
    Battery *battery = obj;

    PangoLayout *layout = pango_cairo_create_layout(c);
    pango_layout_set_font_description(layout, bat1_font_desc);
    pango_layout_set_width(layout, battery->area.width * PANGO_SCALE);
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
    pango_layout_set_text(layout, buf_bat_percentage, strlen(buf_bat_percentage));

    cairo_set_source_rgba(c,
                          battery->font_color.rgb[0],
                          battery->font_color.rgb[1],
                          battery->font_color.rgb[2],
                          battery->font_color.alpha);

    pango_cairo_update_layout(c, layout);
    draw_text(layout, c, 0, battery->bat1_posy, &battery->font_color, ((Panel *)battery->area.panel)->font_shadow);

    pango_layout_set_font_description(layout, bat2_font_desc);
    pango_layout_set_indent(layout, 0);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
    pango_layout_set_text(layout, buf_bat_time, strlen(buf_bat_time));
    pango_layout_set_width(layout, battery->area.width * PANGO_SCALE);

    pango_cairo_update_layout(c, layout);
    draw_text(layout, c, 0, battery->bat2_posy, &battery->font_color, ((Panel *)battery->area.panel)->font_shadow);
    pango_cairo_show_layout(c, layout);

    g_object_unref(layout);
}

void battery_dump_geometry(void *obj, int indent)
{
    Battery *battery = obj;
    fprintf(stderr, "%*sText 1: y = %d, text = %s\n", indent, "", battery->bat1_posy, buf_bat_percentage);
    fprintf(stderr, "%*sText 2: y = %d, text = %s\n", indent, "", battery->bat2_posy, buf_bat_time);
}

char *battery_get_tooltip(void *obj)
{
    return battery_os_tooltip();
}

void battery_action(int button)
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
    tint_exec(command);
}
