/**************************************************************************
 * Copyright (C) 2010       (mrovi@interfete-web-club.com)
 *
 *
 **************************************************************************/

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "common.h"
#include "area.h"

typedef struct Launcher {
	// always start with area
	Area area;
	GSList *list_icon_paths;
	GSList *list_cmds;
	GSList *list_icons;
} Launcher;

typedef struct LauncherIcon {
	Imlib_Image icon;
	char *cmd;
	int x, y;
	int width, height;
} LauncherIcon;

extern int launcher_enabled;
extern int launcher_max_icon_size;

// default global data
void default_launcher();

// initialize launcher : y position, precision, ...
void init_launcher();
void init_launcher_panel(void *panel);
void cleanup_launcher();

void resize_launcher(void *obj);
void draw_launcher (void *obj, cairo_t *c);

void launcher_action(LauncherIcon *icon);

#endif
