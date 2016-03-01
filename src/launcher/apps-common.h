/**************************************************************************
 * Copyright (C) 2015       (mrovi9000@gmail.com)
 *
 *
 **************************************************************************/

#ifndef APPS_COMMON_H
#define APPS_COMMON_H

#include <glib.h>

typedef struct DesktopEntry {
	char *name;
	char *exec;
	char *icon;
	char *path;
	gboolean hidden_from_menus;
} DesktopEntry;

// Parses a line of the form "key = value". Modifies the line.
// Returns 1 if successful, and parts are not empty.
// Key and value point to the parts.
int parse_dektop_line(char *line, char **key, char **value);

// Reads the .desktop file from the given path into the DesktopEntry entry.
// The DesktopEntry object must be initially empty.
// Returns 1 if successful.
gboolean read_desktop_file(const char *path, DesktopEntry *entry);

// Empties DesktopEntry: releases the memory of the *members* of entry.
void free_desktop_entry(DesktopEntry *entry);

// Returns a list of the directories used to store desktop files.
// Do not free the result, it is cached.
const GSList *get_apps_locations();

#endif
