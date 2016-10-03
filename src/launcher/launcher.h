/**************************************************************************
 * Copyright (C) 2010       (mrovi@interfete-web-club.com)
 *
 *
 **************************************************************************/

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "common.h"
#include "area.h"
#include "xsettings-client.h"
#include "icon-theme-common.h"

typedef struct Launcher {
	// always start with area
	Area area;
	GSList *list_apps;  // List of char*, each is a path to a app.desktop file
	GSList *list_icons; // List of LauncherIcon*
	IconThemeWrapper *icon_theme_wrapper;
} Launcher;

typedef struct LauncherIcon {
	// always start with area
	Area area;
	char *config_path;
	Imlib_Image image;
	Imlib_Image image_hover;
	Imlib_Image image_pressed;
	char *cmd;
	char *icon_name;
	char *icon_path;
	char *icon_tooltip;
	int icon_size;
	int x, y;
} LauncherIcon;

extern gboolean launcher_enabled;
extern int launcher_max_icon_size;
extern int launcher_tooltip_enabled;
extern int launcher_alpha;
extern int launcher_saturation;
extern int launcher_brightness;
extern char *icon_theme_name_xsettings; // theme name
extern char *icon_theme_name_config;
extern int launcher_icon_theme_override;
extern int startup_notifications;
extern Background *launcher_icon_bg;
extern GList *launcher_icon_gradients;

// default global data
void default_launcher();

// initialize launcher : y position, precision, ...
void init_launcher();
void init_launcher_panel(void *panel);
void cleanup_launcher();
void cleanup_launcher_theme(Launcher *launcher);

gboolean resize_launcher(void *obj);
void draw_launcher(void *obj, cairo_t *c);
void launcher_default_icon_theme_changed();

// Populates the list_icons list
void launcher_load_icons(Launcher *launcher);
// Populates the list_themes list
void launcher_load_themes(Launcher *launcher);
void launcher_action(LauncherIcon *icon, XEvent *e);

void test_launcher_read_desktop_file();
void test_launcher_read_theme_file();

#endif
