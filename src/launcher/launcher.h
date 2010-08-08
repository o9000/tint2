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

typedef struct DesktopEntry {
	char *name;
	char *exec;
	char *icon;
} DesktopEntry;

#define ICON_DIR_TYPE_SCALABLE 0
#define ICON_DIR_TYPE_FIXED 1
#define ICON_DIR_TYPE_THRESHOLD 2
typedef struct IconThemeDir {
	char *name;
	int size;
	int type;
	int max_size;
	int min_size;
	int threshold;
} IconThemeDir;

typedef struct IconTheme {
	char *name;
	GSList *list_inherits; // each item is a char* (theme name)
	GSList *list_directories; // each item is an IconThemeDir*
} IconTheme;

extern int launcher_enabled;
extern int launcher_max_icon_size;

extern GSList *icon_themes; // each item is an IconTheme*

// default global data
void default_launcher();

// initialize launcher : y position, precision, ...
void init_launcher();
void init_launcher_panel(void *panel);
void cleanup_launcher();

void resize_launcher(void *obj);
void draw_launcher (void *obj, cairo_t *c);

void launcher_action(LauncherIcon *icon);

void test_launcher_read_desktop_file();

#endif
