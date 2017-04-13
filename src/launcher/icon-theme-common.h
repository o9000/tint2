/**************************************************************************
 * Copyright (C) 2015       (mrovi9000@gmail.com)
 *
 **************************************************************************/

#ifndef ICON_THEME_COMMON_H
#define ICON_THEME_COMMON_H

#include <glib.h>
#include "cache.h"

typedef struct IconThemeWrapper {
    // The icon theme name for which this wrapper was created
    char *icon_theme_name;
    // List of IconTheme*
    GSList *themes;
    // Themes are loaded lazily when needed.
    gboolean _themes_loaded;
    // List of IconTheme*
    GSList *themes_fallback;
    // Fallback themes are loaded lazily when needed.
    gboolean _fallback_loaded;
    Cache _cache;
    // List of icon theme names that have been queued for loading.
    // Used to avoid loading the same theme twice, and to avoid cycles.
    GSList *_queued;
} IconThemeWrapper;

typedef struct IconTheme {
    char *name;
    char *description;
    GSList *list_inherits;    // each item is a char* (theme name)
    GSList *list_directories; // each item is an IconThemeDir*
} IconTheme;

// Parses a line of the form "key = value". Modifies the line.
// Returns 1 if successful, and parts are not empty.
// Key and value point to the parts.
int parse_theme_line(char *line, char **key, char **value);

// Returns an IconThemeWrapper* containing the icon theme identified by the name icon_theme_name, all the
// inherited themes, the hicolor theme and possibly fallback themes.
IconThemeWrapper *load_themes(const char *icon_theme_name);

void save_icon_cache(IconThemeWrapper *wrapper);

void free_themes(IconThemeWrapper *wrapper);
void free_icon_theme(IconTheme *theme);

#define DEFAULT_ICON "application-x-executable"

// Returns the full path to an icon file (or NULL) given the list of icon themes to search and the icon name
// Note: needs to be released with free().
char *get_icon_path(IconThemeWrapper *wrapper, const char *icon_name, int size, gboolean use_fallbacks);

// Returns a list of the directories used to store icons.
// Do not free the result, it is cached.
const GSList *get_icon_locations();

#endif
