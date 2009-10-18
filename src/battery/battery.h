/**************************************************************************
* Copyright (C) 2009 Sebastian Reichel <elektranox@gmail.com>
*
* Battery with functional data (percentage, time to life) and drawing data
* (area, font, ...). Each panel use his own drawing data.
* Need kernel > 2.6.23.
*
**************************************************************************/

#ifndef BATTERY_H
#define BATTERY_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"
#include "area.h"


typedef struct Battery {
	// always start with area
	Area area;

	config_color font;
	int bat1_posy;
	int bat2_posy;
} Battery;

enum chargestate {
	BATTERY_UNKNOWN,
	BATTERY_CHARGING,
	BATTERY_DISCHARGING
};

typedef struct battime {
	int16_t hours;
	int8_t minutes;
	int8_t seconds;
} battime;

typedef struct batstate {
	int percentage;
	struct battime time;
	enum chargestate state;
} batstate;

extern struct batstate battery_state;
extern PangoFontDescription *bat1_font_desc;
extern PangoFontDescription *bat2_font_desc;

extern int8_t battery_low_status;
extern char *battery_low_cmd;
extern char *path_energy_now, *path_energy_full, *path_current_now, *path_status;


// initialize clock : y position, ...
void update_battery();

void init_battery();
void init_battery_panel(void *panel);

void draw_battery(void *obj, cairo_t *c, int active);

void resize_battery(void *obj);

#endif
