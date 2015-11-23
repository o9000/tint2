/**************************************************************************
* Copyright (C) 2009-2015 Sebastian Reichel <sre@ring0.de>
*
* Battery with functional data (percentage, time to life) and drawing data
* (area, font, ...). Each panel use his own drawing data.
*
**************************************************************************/

#ifndef BATTERY_H
#define BATTERY_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"
#include "area.h"

// battery drawing parameter (per panel)
typedef struct Battery {
	// always start with area
	Area area;

	Color font;
	int bat1_posy;
	int bat2_posy;
} Battery;

typedef enum ChargeState {
	BATTERY_UNKNOWN = 0,
	BATTERY_CHARGING,
	BATTERY_DISCHARGING,
	BATTERY_FULL,
} ChargeState;

typedef struct BatteryTime {
	int16_t hours;
	int8_t minutes;
	int8_t seconds;
} BatteryTime;

typedef struct BatteryState {
	int percentage;
	BatteryTime time;
	ChargeState state;
	gboolean ac_connected;
} BatteryState;

extern struct BatteryState battery_state;
extern PangoFontDescription *bat1_font_desc;
extern PangoFontDescription *bat2_font_desc;
extern gboolean battery_enabled;
extern gboolean battery_tooltip_enabled;
extern int percentage_hide;

extern int8_t battery_low_status;
extern char *battery_low_cmd;

extern char *ac_connected_cmd;
extern char *ac_disconnected_cmd;

extern char *battery_lclick_command;
extern char *battery_mclick_command;
extern char *battery_rclick_command;
extern char *battery_uwheel_command;
extern char *battery_dwheel_command;

static inline gchar *chargestate2str(ChargeState state)
{
	switch (state) {
	case BATTERY_CHARGING:
		return "Charging";
	case BATTERY_DISCHARGING:
		return "Discharging";
	case BATTERY_FULL:
		return "Full";
	case BATTERY_UNKNOWN:
	default:
		return "Unknown";
	};
}

static inline void battery_state_set_time(BatteryState *state, int seconds)
{
	state->time.hours = seconds / 3600;
	seconds -= 3600 * state->time.hours;
	state->time.minutes = seconds / 60;
	seconds -= 60 * state->time.minutes;
	state->time.seconds = seconds;
}

// default global data
void default_battery();

// freed memory
void cleanup_battery();

void update_battery_tick(void *arg);
int update_battery();

void init_battery();
void init_battery_panel(void *panel);

void reinit_battery();
void draw_battery(void *obj, cairo_t *c);

gboolean resize_battery(void *obj);

void battery_action(int button);

/* operating system specific functions */
gboolean battery_os_init();
void battery_os_free();
int battery_os_update(BatteryState *state);
char *battery_os_tooltip();

#endif
