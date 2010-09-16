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
	GSList *list_apps;			// List of char*, each is a path to a app.desktop file
	GSList *list_icons; 		// List of LauncherIcon*
	GSList *icon_theme_names; 	// List of char*, each is a theme name (oxygen, Tango...)
	GSList *icon_themes;		// List of IconTheme*
} Launcher;

typedef struct LauncherIcon {
	Imlib_Image icon_scaled;
	Imlib_Image icon_original;
	char *cmd;
	char *icon_name;
	char *icon_path;
	int icon_size;
	int is_app_desktop;
	int x, y;
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
	char *context;
} IconThemeDir;

typedef struct IconTheme {
	char *name;
	GSList *list_inherits; // each item is a char* (theme name)
	GSList *list_directories; // each item is an IconThemeDir*
} IconTheme;

extern int launcher_enabled;
extern int launcher_max_icon_size;

// default global data
void default_launcher();

// initialize launcher : y position, precision, ...
void init_launcher();
void init_launcher_panel(void *panel);
void cleanup_launcher();

int  resize_launcher(void *obj);
void draw_launcher (void *obj, cairo_t *c);

void launcher_action(LauncherIcon *icon);

void test_launcher_read_desktop_file();
void test_launcher_read_theme_file();

#endif
