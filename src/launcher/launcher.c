/**************************************************************************
* Tint2 : launcher
*
* Copyright (C) 2010       (mrovi@interfete-web-club.com)
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* as published by the Free Software Foundation.
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
#include <cairo.h>
#include <cairo-xlib.h>
#include <pango/pangocairo.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#include "window.h"
#include "server.h"
#include "area.h"
#include "panel.h"
#include "taskbar.h"
#include "launcher.h"

int launcher_enabled;
int launcher_max_icon_size;
GSList *icon_themes = 0;

void default_launcher()
{
	launcher_enabled = 0;
	launcher_max_icon_size = 0;
}


void init_launcher()
{
}


void init_launcher_panel(void *p)
{
	Panel *panel =(Panel*)p;
	Launcher *launcher = &panel->launcher;

	launcher->area.parent = p;
	launcher->area.panel = p;
	launcher->area._draw_foreground = draw_launcher;
	launcher->area._resize = resize_launcher;
	launcher->area.resize = 1;
	launcher->area.redraw = 1;
	launcher->area.on_screen = 1;

	if (panel_horizontal) {
		// panel horizonal => fixed height and posy
		launcher->area.posy = panel->area.bg->border.width + panel->area.paddingy;
		launcher->area.height = panel->area.height - (2 * launcher->area.posy);
	}
	else {
		// panel vertical => fixed width, height, posy and posx
		launcher->area.posx = panel->area.bg->border.width + panel->area.paddingxlr;
		launcher->area.width = panel->area.width - (2 * panel->area.bg->border.width) - (2 * panel->area.paddingy);
	}

	// Load icons
	{
		GSList* path = launcher->list_icon_paths;
		GSList* cmd = launcher->list_cmds;
		while (path != NULL && cmd != NULL)
		{
			LauncherIcon *launcherIcon = malloc(sizeof(LauncherIcon));
			launcherIcon->icon = imlib_load_image(path->data);
			launcherIcon->cmd = strdup(cmd->data);
			launcher->list_icons = g_slist_append(launcher->list_icons, launcherIcon);
			path = g_slist_next(path);
			cmd = g_slist_next(cmd);
		}
	}
}


void cleanup_launcher()
{
	int i;

	for (i = 0 ; i < nb_panel ; i++) {
		Panel *panel = &panel1[i];
		Launcher *launcher = &panel->launcher;
		free_area(&launcher->area);
		GSList *l;
		for (l = launcher->list_icons; l ; l = l->next) {
			LauncherIcon *launcherIcon = (LauncherIcon*)l->data;
			if (launcherIcon && launcherIcon->icon) {
				imlib_context_set_image (launcherIcon->icon);
				imlib_free_image();
			}
			free(launcherIcon->cmd);
			free(launcherIcon);
		}
		g_slist_free(launcher->list_icons);
		for (l = launcher->list_icon_paths; l ; l = l->next) {
			free(l->data);
		}
		g_slist_free(launcher->list_icon_paths);
		for (l = launcher->list_cmds; l ; l = l->next) {
			free(l->data);
		}
		g_slist_free(launcher->list_cmds);
	}
	launcher_enabled = 0;
}

void resize_launcher(void *obj)
{
	Launcher *launcher = obj;
	Panel *panel = launcher->area.panel;
	GSList *l;
	int count, icon_size;
	int icons_per_column=1, icons_per_row=1, marging=0;

	if (panel_horizontal)
		icon_size = launcher->area.height;
	else
		icon_size = launcher->area.width;
	icon_size = icon_size - (2 * launcher->area.bg->border.width) - (2 * launcher->area.paddingy);
	if (launcher_max_icon_size > 0 && icon_size > launcher_max_icon_size)
		icon_size = launcher_max_icon_size;
	
	count = 0;
	for (l = launcher->list_icons; l ; l = l->next) {
		count++;
	}

	if (panel_horizontal) {
		if (!count) launcher->area.width = 0;
		else {
			int height = launcher->area.height - 2*launcher->area.bg->border.width - 2*launcher->area.paddingy;
			// here icons_per_column always higher than 0
			icons_per_column = (height+launcher->area.paddingx) / (icon_size+launcher->area.paddingx);
			marging = height - (icons_per_column-1)*(icon_size+launcher->area.paddingx) - icon_size;
			icons_per_row = count / icons_per_column + (count%icons_per_column != 0);
			launcher->area.width = (2 * launcher->area.bg->border.width) + (2 * launcher->area.paddingxlr) + (icon_size * icons_per_row) + ((icons_per_row-1) * launcher->area.paddingx);
		}

		launcher->area.posx = panel->area.bg->border.width + panel->area.paddingxlr;
		launcher->area.posy = panel->area.bg->border.width;
	}
	else {
		if (!count) launcher->area.height = 0;
		else {
			int width = launcher->area.width - 2*launcher->area.bg->border.width - 2*launcher->area.paddingy;
			// here icons_per_row always higher than 0
			icons_per_row = (width+launcher->area.paddingx) / (icon_size+launcher->area.paddingx);
			marging = width - (icons_per_row-1)*(icon_size+launcher->area.paddingx) - icon_size;
			icons_per_column = count / icons_per_row+ (count%icons_per_row != 0);
			launcher->area.height = (2 * launcher->area.bg->border.width) + (2 * launcher->area.paddingxlr) + (icon_size * icons_per_column) + ((icons_per_column-1) * launcher->area.paddingx);
		}

		launcher->area.posx = panel->area.bg->border.width;
		launcher->area.posy = panel->area.height - panel->area.bg->border.width - panel->area.paddingxlr - launcher->area.height;
	}

	int i, posx, posy;
	int start = launcher->area.bg->border.width + launcher->area.paddingy;// +marging/2;
	if (panel_horizontal) {
		posy = start;
		posx = launcher->area.bg->border.width + launcher->area.paddingxlr;
	}
	else {
		posx = start;
		posy = launcher->area.bg->border.width + launcher->area.paddingxlr;
	}

	for (i=1, l = launcher->list_icons; l ; i++, l = l->next) {
		LauncherIcon *launcherIcon = (LauncherIcon*)l->data;
		
		launcherIcon->y = posy;
		launcherIcon->x = posx;
		launcherIcon->width = icon_size;
		launcherIcon->height = icon_size;
		if (panel_horizontal) {
			if (i % icons_per_column)
				posy += icon_size + launcher->area.paddingx;
			else {
				posy = start;
				posx += (icon_size + launcher->area.paddingx);
			}
		}
		else {
			if (i % icons_per_row)
				posx += icon_size + launcher->area.paddingx;
			else {
				posx = start;
				posy += (icon_size + launcher->area.paddingx);
			}
		}
	}
	
	// resize force the redraw
	launcher->area.redraw = 1;
}


void draw_launcher(void *obj, cairo_t *c)
{
	Launcher *launcher = obj;
	GSList *l;
	if (launcher->list_icons == 0) return;

	for (l = launcher->list_icons; l ; l = l->next) {
		LauncherIcon *launcherIcon = (LauncherIcon*)l->data;
		int pos_x = launcherIcon->x;
		int pos_y = launcherIcon->y;
		int width = launcherIcon->width;
		int height = launcherIcon->height;
		Imlib_Image icon_scaled;
		if (launcherIcon->icon) {
			imlib_context_set_image (launcherIcon->icon);
			icon_scaled = imlib_create_cropped_scaled_image(0, 0, imlib_image_get_width(), imlib_image_get_height(), width, height);
		} else {
			icon_scaled = imlib_create_image(width, height);
			imlib_context_set_image (icon_scaled);
			imlib_context_set_color(255, 255, 255, 255);
			imlib_image_fill_rectangle(0, 0, width, height);
		}

		// Render
		imlib_context_set_image (icon_scaled);
		if (server.real_transparency) {
			render_image(launcher->area.pix, pos_x, pos_y, imlib_image_get_width(), imlib_image_get_height() );
		}
		else {
			imlib_context_set_drawable(launcher->area.pix);
			imlib_render_image_on_drawable (pos_x, pos_y);
		}
		imlib_free_image();
	}
}

void launcher_action(LauncherIcon *icon)
{
	char *cmd = malloc(strlen(icon->cmd) + 10);
	sprintf(cmd, "(%s&)", icon->cmd);
	tint_exec(cmd);
	free(cmd);
}

/***************** Freedesktop app.desktop and icon theme handling  *********************/
/* http://standards.freedesktop.org/desktop-entry-spec/ */
/* http://standards.freedesktop.org/icon-theme-spec/ */

int parse_dektop_line(char *line, char **key, char **value)
{
	char *p;
	int found = 0;
	*key = line;
	for (p = line; *p; p++) {
		if (*p == '=') {
			*value = p + 1;
			*p = 0;
			found = 1;
			break;
		}
	}
	if (!found)
		return 0;
	if (found && (strlen(*key) == 0 || strlen(*value) == 0))
		return 0;
	return 1;
}

void expand_exec(DesktopEntry *entry, const char *path)
{
	// Expand % in exec
	// %i -> --icon Icon
	// %c -> Name
	// %k -> path
	if (entry->exec) {
		char *exec2 = malloc(strlen(entry->exec) + strlen(entry->name) + strlen(entry->icon) + 100);
		char *p, *q;
		// p will never point to an escaped char
		for (p = entry->exec, q = exec2; *p; p++, q++) {
			*q = *p; // Copy
			if (*p == '\\') {
				p++, q++;
				// Copy the escaped char
				if (*p == '%') // For % we delete the backslash, i.e. write % over it
					q--;
				*q = *p;
				if (!*p) break;
				continue;
			}
			if (*p == '%') {
				p++;
				if (!*p) break;
				if (*p == 'i' && entry->icon != NULL) {
					sprintf(q, "--icon '%s'", entry->icon);
					q += strlen("--icon ''");
					q += strlen(entry->icon);
					q--; // To balance the q++ in the for
				} else if (*p == 'c' && entry->name != NULL) {
					sprintf(q, "'%s'", entry->name);
					q += strlen("''");
					q += strlen(entry->name);
					q--; // To balance the q++ in the for
				} else if (*p == 'c') {
					sprintf(q, "'%s'", path);
					q += strlen("''");
					q += strlen(path);
					q--; // To balance the q++ in the for
				} else {
					// We don't care about other expansions
					q--; // Delete the last % from q
				}
				continue;
			}
		}
		*q = '\0';
		free(entry->exec);
		entry->exec = exec2;
	}
}

int launcher_read_desktop_file(const char *path, DesktopEntry *entry)
{
	FILE *fp;
	char line[4096];
	char *buffer;
	char *key, *value;

	entry->name = entry->icon = entry->exec = NULL;

	if ((fp = fopen(path, "r")) == NULL) {
		fprintf(stderr, "Could not open file %s\n", path);
		return 0;
	}

	buffer = line;
	while (fgets(buffer, sizeof(line) - (buffer - line), fp) != NULL) {
		//TODO use UTF8 capable strlen
		int len = strlen(buffer);
		int total_len = strlen(line);
		if (!g_utf8_validate(buffer, total_len, NULL)) {
			// Not a real newline, read more
			buffer += len;
			if (sizeof(line) - (buffer - line) < 2) {
				fprintf(stderr, "%s %d: line too long (%s)\n", __FILE__, __LINE__, path);
				break;
			} else {
				continue;
			}
		}
		// We have a valid line
		buffer[len-1] = '\0';
		buffer = line;
		if (parse_dektop_line(line, &key, &value)) {
			if (strcmp(key, "Name") == 0) {
				//TODO use UTF-8 capable strdup
				entry->name = strdup(value);
			} else if (strcmp(key, "Exec") == 0) {
				entry->exec = strdup(value);
			} else if (strcmp(key, "Icon") == 0) {
				//TODO use UTF-8 capable strdup
				entry->icon = strdup(value);
			}
		}
	}
	fclose (fp);
	// From this point:
	// entry->name, entry->icon, entry->exec will never be empty strings (can be NULL though)

	expand_exec(entry, path);
	
	return 1;
}

void test_launcher_read_desktop_file()
{
	DesktopEntry entry;
	launcher_read_desktop_file("/usr/share/applications/firefox.desktop", &entry);
	printf("Name:%s Icon:%s Exec:%s\n", entry.name, entry.icon, entry.exec);
}

IconTheme *load_theme(char *name)
{
	//TODO
	// Look for name/index.theme in $HOME/.icons, /usr/share/icons, /usr/share/pixmaps (stop at the first found)
	// Parse index.theme -> list of IconThemeDir with attributes
	// Return IconTheme struct
	return NULL;
}

// Populates the icon_themes list
void launcher_load_themes()
{
	//TODO load the user theme, all the inherited themes recursively (DFS), and the hicolor theme
	// avoid inheritance loops
}

// Returns the full path to an icon file (or NULL) given the icon name
char *icon_path(const char *name)
{
	//TODO
	/*

// Stage 1: exact size match
LookupIcon (iconname, size):
for each subdir in $(theme subdir list) {  // <---- each subdir in each theme
   for each directory in $(basename list) {
   for extension in ("png", "svg", "xpm") {
   if DirectoryMatchesSize(subdir, size) {
   filename = directory/$(themename)/subdirectory/iconname.extension
   if exist filename
	   return filename
}
}
}
}

// Stage 2: best size match
minimal_size = MAXINT
for each subdir in $(theme subdir list) {  // <---- each subdir in each theme
   for each directory in $(basename list) {
   for extension in ("png", "svg", "xpm") {
   filename = directory/$(themename)/subdirectory/iconname.extension
   if exist filename and DirectorySizeDistance(subdir, size) < minimal_size
	   closest_filename = filename
	   minimal_size = DirectorySizeDistance(subdir, size)
}
}
}
if closest_filename set
	return closest_filename

// Stage 3: look in unthemed icons 
for each directory in $(basename list) { // <---- $HOME/.icons, /usr/share/icons, /usr/share/pixmaps
   for extension in ("png", "svg", "xpm") {
   if exists directory/iconname.extension
	   return directory/iconname.extension
	   }
	   }
	   
	   return failed icon lookup

// With the following helper functions:

DirectoryMatchesSize(subdir, iconsize):
read Type and size data from subdir
if Type is Fixed
return Size == iconsize
if Type is Scaled
return MinSize <= iconsize <= MaxSize
if Type is Threshold
return Size - Threshold <= iconsize <= Size + Threshold

DirectorySizeDistance(subdir, size):
read Type and size data from subdir
if Type is Fixed
return abs(Size - iconsize)
if Type is Scaled
if iconsize < MinSize
return MinSize - iconsize
if iconsize > MaxSize
return iconsize - MaxSize
return 0
if Type is Threshold
if iconsize < Size - Threshold
return MinSize - iconsize
if iconsize > Size + Threshold
return iconsize - MaxSize
return 0
	*/
}
