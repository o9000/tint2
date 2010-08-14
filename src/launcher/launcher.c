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

#define ICON_FALLBACK "exec"

char *icon_path(Launcher *launcher, const char *icon_name, int size);
void launcher_load_themes(Launcher *launcher);
void free_desktop_entry(DesktopEntry *entry);
int launcher_read_desktop_file(const char *path, DesktopEntry *entry);
Imlib_Image scale_icon(Imlib_Image original, int icon_size);
void free_icon(Imlib_Image icon);
void free_icon_theme(IconTheme *theme);

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
	launcher->area.size_mode = SIZE_BY_CONTENT;
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

	fprintf(stderr, "Loading themes...\n");
	launcher_load_themes(launcher);
	fprintf(stderr, "Done\n");

	// Load apps (.desktop style launcher items)
	GSList* app = launcher->list_apps;
	while (app != NULL) {
		DesktopEntry entry;
		launcher_read_desktop_file(app->data, &entry);
		if (entry.exec) {
			LauncherIcon *launcherIcon = calloc(1, sizeof(LauncherIcon));
			launcherIcon->is_app_desktop = 1;
			launcherIcon->cmd = strdup(entry.exec);
			launcherIcon->icon_name = entry.icon ? strdup(entry.icon) : strdup(ICON_FALLBACK);
			launcherIcon->icon_size = 1;
			free_desktop_entry(&entry);
			launcher->list_icons = g_slist_append(launcher->list_icons, launcherIcon);
		}
		app = g_slist_next(app);
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
			if (launcherIcon) {
				free_icon(launcherIcon->icon_scaled);
				free_icon(launcherIcon->icon_original);
				free(launcherIcon->icon_name);
				free(launcherIcon->icon_path);
				free(launcherIcon->cmd);
			}
			free(launcherIcon);
		}
		g_slist_free(launcher->list_icons);

		for (l = launcher->list_apps; l ; l = l->next) {
			free(l->data);
		}
		g_slist_free(launcher->list_apps);

		for (l = launcher->icon_themes; l ; l = l->next) {
			IconTheme *theme = (IconTheme*) l->data;
			free_icon_theme(theme);
			free(theme);
		}
		g_slist_free(launcher->icon_themes);

		for (l = launcher->icon_theme_names; l ; l = l->next) {
			free(l->data);
		}
		g_slist_free(launcher->icon_theme_names);
		launcher->list_apps = launcher->list_icons = launcher->icon_theme_names = launcher->icon_themes = NULL;
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

	// Resize icons if necessary
	for (l = launcher->list_icons; l ; l = l->next) {
		LauncherIcon *launcherIcon = (LauncherIcon *)l->data;
		if (launcherIcon->icon_size != icon_size || !launcherIcon->icon_original) {
			launcherIcon->icon_size = icon_size;
			// Get the path for an icon file with the new size
			char *new_icon_path = icon_path(launcher, launcherIcon->icon_name, launcherIcon->icon_size);
			if (!new_icon_path) {
				// Draw a blank icon
				free_icon(launcherIcon->icon_original);
				launcherIcon->icon_original = NULL;
				free_icon(launcherIcon->icon_scaled);
				launcherIcon->icon_scaled = NULL;
				new_icon_path = icon_path(launcher, ICON_FALLBACK, launcherIcon->icon_size);
				if (new_icon_path) {
					launcherIcon->icon_original = imlib_load_image(new_icon_path);
					fprintf(stderr, "%s %d: Using icon %s\n", __FILE__, __LINE__, new_icon_path);
					free(new_icon_path);
				}
				launcherIcon->icon_scaled = scale_icon(launcherIcon->icon_original, icon_size);
				continue;
			}
			if (launcherIcon->icon_path && strcmp(new_icon_path, launcherIcon->icon_path) == 0) {
				// If it's the same file just rescale
				free_icon(launcherIcon->icon_scaled);
				launcherIcon->icon_scaled = scale_icon(launcherIcon->icon_original, icon_size);
				free(new_icon_path);
				fprintf(stderr, "%s %d: Using icon %s\n", __FILE__, __LINE__, launcherIcon->icon_path);
			} else {
				// Free the old files
				free_icon(launcherIcon->icon_original);
				free_icon(launcherIcon->icon_scaled);
				// Load the new file and scale
				launcherIcon->icon_original = imlib_load_image(new_icon_path);
				launcherIcon->icon_scaled = scale_icon(launcherIcon->icon_original, launcherIcon->icon_size);
				free(launcherIcon->icon_path);
				launcherIcon->icon_path = new_icon_path;
				fprintf(stderr, "%s %d: Using icon %s\n", __FILE__, __LINE__, launcherIcon->icon_path);
			}
		}
	}
	
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
		Imlib_Image icon_scaled = launcherIcon->icon_scaled;
		// Render
		imlib_context_set_image (icon_scaled);
		if (server.real_transparency) {
			render_image(launcher->area.pix, pos_x, pos_y, imlib_image_get_width(), imlib_image_get_height() );
		}
		else {
			imlib_context_set_drawable(launcher->area.pix);
			imlib_render_image_on_drawable (pos_x, pos_y);
		}
	}
}

Imlib_Image scale_icon(Imlib_Image original, int icon_size)
{
	Imlib_Image icon_scaled;
	if (original) {
		imlib_context_set_image (original);
		icon_scaled = imlib_create_cropped_scaled_image(0, 0, imlib_image_get_width(), imlib_image_get_height(), icon_size, icon_size);
	} else {
		icon_scaled = imlib_create_image(icon_size, icon_size);
		imlib_context_set_image (icon_scaled);
		imlib_context_set_color(255, 255, 255, 255);
		imlib_image_fill_rectangle(0, 0, icon_size, icon_size);
	}
	return icon_scaled;
}

void free_icon(Imlib_Image icon)
{
	if (icon) {
		imlib_context_set_image(icon);
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

// Splits line at first '=' and returns 1 if successful, and parts are not empty
// key and value point to the parts
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

int parse_theme_line(char *line, char **key, char **value)
{
	return parse_dektop_line(line, key, value);
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

//TODO Use UTF8 when parsing the file
int launcher_read_desktop_file(const char *path, DesktopEntry *entry)
{
	FILE *fp;
	char line[4096];
	char *key, *value;

	entry->name = entry->icon = entry->exec = NULL;

	if ((fp = fopen(path, "rt")) == NULL) {
		fprintf(stderr, "Could not open file %s\n", path);
		return 0;
	}

	while (fgets(line, sizeof(line), fp) != NULL) {
		int len = strlen(line);
		if (len == 0)
			continue;
		line[len - 1] = '\0';
		if (parse_dektop_line(line, &key, &value)) {
			if (strcmp(key, "Name") == 0) {
				entry->name = strdup(value);
			} else if (strcmp(key, "Exec") == 0) {
				entry->exec = strdup(value);
			} else if (strcmp(key, "Icon") == 0) {
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

void free_desktop_entry(DesktopEntry *entry)
{
	free(entry->name);
	free(entry->icon);
	free(entry->exec);
}

void test_launcher_read_desktop_file()
{
	fprintf(stdout, "\033[1;33m");
	DesktopEntry entry;
	launcher_read_desktop_file("/usr/share/applications/firefox.desktop", &entry);
	printf("Name:%s Icon:%s Exec:%s\n", entry.name, entry.icon, entry.exec);
	fprintf(stdout, "\033[0m");
}

//TODO Use UTF8 when parsing the file
IconTheme *load_theme(char *name)
{
	// Look for name/index.theme in $HOME/.icons, /usr/share/icons, /usr/share/pixmaps (stop at the first found)
	// Parse index.theme -> list of IconThemeDir with attributes
	// Return IconTheme*

	IconTheme *theme;
	char *file_name;
	FILE *f;
	char line[2048];

	if (name == NULL)
		return NULL;

	fprintf(stderr, "Loading icon theme %s\n", name);

	file_name = malloc(100 + strlen(name));
	sprintf(file_name, "~/.icons/%s/index.theme", name);
	if (!g_file_test(file_name, G_FILE_TEST_EXISTS)) {
		sprintf(file_name, "/usr/share/icons/%s/index.theme", name);
		if (!g_file_test(file_name, G_FILE_TEST_EXISTS)) {
			sprintf(file_name, "/usr/share/pixmaps/%s/index.theme", name);
			if (!g_file_test(file_name, G_FILE_TEST_EXISTS)) {
				free(file_name);
				file_name = NULL;
			}
		}
	}
	if (!file_name) {
		fprintf(stderr, "Could not load theme %s\n", name);
		return NULL;
	}

	if ((f = fopen(file_name, "rt")) == NULL)
		return NULL;

	free(file_name);

	theme = calloc(1, sizeof(IconTheme));
	theme->name = strdup(name);
	theme->list_inherits = NULL;
	theme->list_directories = NULL;

	IconThemeDir *current_dir = NULL;
	int inside_header = 1;
	while (fgets(line, sizeof(line), f) != NULL) {
		char *key, *value;
		
		int line_len = strlen(line);
		if (line_len >= 1) {
			if (line[line_len - 1] == '\n') {
				line[line_len - 1] = '\0';
				line_len--;
			}
		}

		if (line_len == 0)
			continue;

		if (inside_header) {
			if (parse_theme_line(line, &key, &value)) {
				if (strcmp(key, "Inherits") == 0) {
					// value is like oxygen,wood,default
					char *token;
					token = strtok(value, ",\n");
					while (token != NULL)
					{
						theme->list_inherits = g_slist_append(theme->list_inherits, strdup(token));
						token = strtok(NULL, ",\n");
					}
				} else if (strcmp(key, "Directories") == 0) {
					// value is like 48x48/apps,48x48/mimetypes,32x32/apps,scalable/apps,scalable/mimetypes
					char *token;
					token = strtok(value, ",\n");
					while (token != NULL)
					{
						IconThemeDir *dir = calloc(1, sizeof(IconThemeDir));
						dir->name = strdup(token);
						dir->max_size = dir->min_size = dir->size = -1;
						dir->type = ICON_DIR_TYPE_THRESHOLD;
						dir->threshold = 2;
						theme->list_directories = g_slist_append(theme->list_directories, dir);
						token = strtok(NULL, ",\n");
					}
				}
			}
		} else if (current_dir != NULL) {
			if (parse_theme_line(line, &key, &value)) {
				if (strcmp(key, "Size") == 0) {
					// value is like 24
					sscanf(value, "%d", &current_dir->size);
					if (current_dir->max_size == -1)
						current_dir->max_size = current_dir->size;
					if (current_dir->min_size == -1)
						current_dir->min_size = current_dir->size;
				} else if (strcmp(key, "MaxSize") == 0) {
					// value is like 24
					sscanf(value, "%d", &current_dir->max_size);
				} else if (strcmp(key, "MinSize") == 0) {
					// value is like 24
					sscanf(value, "%d", &current_dir->min_size);
				} else if (strcmp(key, "Threshold") == 0) {
					// value is like 2
					sscanf(value, "%d", &current_dir->threshold);
				} else if (strcmp(key, "Type") == 0) {
					// value is Fixed, Scalable or Threshold
					if (strcmp(value, "Fixed") == 0) {
						current_dir->type = ICON_DIR_TYPE_FIXED;
					} else if (strcmp(value, "Scalable") == 0) {
						current_dir->type = ICON_DIR_TYPE_SCALABLE;
					} else if (strcmp(value, "Threshold") == 0) {
						current_dir->type = ICON_DIR_TYPE_THRESHOLD;
					}
				} else if (strcmp(key, "Context") == 0) {
					// usual values: Actions, Applications, Devices, FileSystems, MimeTypes
					current_dir->context = strdup(value);
				}
			}
		}

		if (line[0] == '[' && line[line_len - 1] == ']' && strcmp(line, "[Icon Theme]") != 0) {
			inside_header = 0;
			current_dir = NULL;
			line[line_len - 1] = '\0';
			char *dir_name = line + 1;
			GSList* dir_item = theme->list_directories;
			while (dir_item != NULL)
			{
				IconThemeDir *dir = dir_item->data;
				if (strcmp(dir->name, dir_name) == 0) {
					current_dir = dir;
					break;
				}
				dir_item = g_slist_next(dir_item);
			}
		}
	}
	fclose(f);
	
	return theme;
}

void free_icon_theme(IconTheme *theme)
{
	free(theme->name);
	GSList *l_inherits;
	for (l_inherits = theme->list_inherits; l_inherits ; l_inherits = l_inherits->next) {
		free(l_inherits->data);
	}
	GSList *l_dir;
	for (l_dir = theme->list_directories; l_dir ; l_dir = l_dir->next) {
		IconThemeDir *dir = (IconThemeDir *)l_dir->data;
		free(dir->name);
		free(dir->context);
		free(l_dir->data);
	}
}

void test_launcher_read_theme_file()
{
	fprintf(stdout, "\033[1;33m");
	IconTheme *theme = load_theme("oxygen");
	if (!theme) {
		printf("Could not load theme\n");
		return;
	}
	printf("Loaded theme: %s\n", theme->name);
	GSList* item = theme->list_inherits;
	while (item != NULL)
	{
		printf("Inherits:%s\n", (char*)item->data);
		item = g_slist_next(item);
	}
	item = theme->list_directories;
	while (item != NULL)
	{
		IconThemeDir *dir = item->data;
		printf("Dir:%s Size=%d MinSize=%d MaxSize=%d Threshold=%d Type=%s Context=%s\n",
			   dir->name, dir->size, dir->min_size, dir->max_size, dir->threshold,
			   dir->type == ICON_DIR_TYPE_FIXED ? "Fixed" :
			   dir->type == ICON_DIR_TYPE_SCALABLE ? "Scalable" :
			   dir->type == ICON_DIR_TYPE_THRESHOLD ? "Threshold" : "?????",
			   dir->context);
		item = g_slist_next(item);
	}
	fprintf(stdout, "\033[0m");
}

// Populates the icon_themes list
void launcher_load_themes(Launcher *launcher)
{
	// load the user theme, all the inherited themes recursively (DFS), and the hicolor theme
	// avoid inheritance loops

	GSList *queue = NULL;
	GSList *queued = NULL;

	GSList* icon_theme_name_item;
	for (icon_theme_name_item = launcher->icon_theme_names; icon_theme_name_item; icon_theme_name_item = g_slist_next(icon_theme_name_item)) {
		int duplicate = 0;
		GSList* queued_item = queued;
		while (queued_item != NULL) {
			if (strcmp(queued_item->data, icon_theme_name_item->data) == 0) {
				duplicate = 1;
				break;
			}
			queued_item = g_slist_next(queued_item);
		}
		if (!duplicate) {
			queue = g_slist_append(queue, strdup(icon_theme_name_item->data));
			queued = g_slist_append(queued, strdup(icon_theme_name_item->data));
		}
	}

	int hicolor_loaded = 0;
	while (queue || !hicolor_loaded) {
		if (!queue) {
			GSList* queued_item = queued;
			while (queued_item != NULL) {
				if (strcmp(queued_item->data, "hicolor") == 0) {
					hicolor_loaded = 1;
					break;
				}
				queued_item = g_slist_next(queued_item);
			}
			if (hicolor_loaded)
				break;
			queue = g_slist_append(queue, strdup("hicolor"));
			queued = g_slist_append(queued, strdup("hicolor"));
		}
		
		char *name = queue->data;
		queue = g_slist_remove(queue, name);

		IconTheme *theme = load_theme(name);
		if (theme != NULL) {
			launcher->icon_themes = g_slist_append(launcher->icon_themes, theme);

			GSList* item = theme->list_inherits;
			int pos = 0;
			while (item != NULL)
			{
				char *parent = item->data;
				int duplicate = 0;
				GSList* queued_item = queued;
				while (queued_item != NULL) {
					if (strcmp(queued_item->data, parent) == 0) {
						duplicate = 1;
						break;
					}
					queued_item = g_slist_next(queued_item);
				}
				if (!duplicate) {
					queue = g_slist_insert(queue, strdup(parent), pos);
					pos++;
					queued = g_slist_append(queued, strdup(parent));
				}
				item = g_slist_next(item);
			}
		}
	}

	// Free the queue
	GSList *l;
	for (l = queue; l ; l = l->next)
		free(l->data);
	g_slist_free(queue);
	for (l = queued; l ; l = l->next)
		free(l->data);
	g_slist_free(queued);
}

int directory_matches_size(IconThemeDir *dir, int size)
{
	if (dir->type == ICON_DIR_TYPE_FIXED) {
		return dir->size == size;
	} else if (dir->type == ICON_DIR_TYPE_SCALABLE) {
		return dir->min_size <= size && size <= dir->max_size;
	} else /*if (dir->type == ICON_DIR_TYPE_THRESHOLD)*/ {
		return dir->size - dir->threshold <= size && size <= dir->size + dir->threshold;
	}
}

int directory_size_distance(IconThemeDir *dir, int size)
{
	if (dir->type == ICON_DIR_TYPE_FIXED) {
		return abs(dir->size - size);
	} else if (dir->type == ICON_DIR_TYPE_SCALABLE) {
		if (size < dir->min_size) {
			return dir->min_size - size;
		} else if (size > dir->max_size) {
			return size - dir->max_size;
		} else {
			return 0;
		}
	} else /*if (dir->type == ICON_DIR_TYPE_THRESHOLD)*/ {
		if (size < dir->size - dir->threshold) {
			return dir->min_size - size;
		} else if (size > dir->size + dir->threshold) {
			return size - dir->max_size;
		} else {
			return 0;
		}
	}
}

// Returns the full path to an icon file (or NULL) given the icon name
char *icon_path(Launcher *launcher, const char *icon_name, int size)
{
	if (icon_name == NULL)
		return NULL;
	
	// If the icon_name is already a path and the file exists, return it
	if (strstr(icon_name, "/") == icon_name) {
		if (g_file_test(icon_name, G_FILE_TEST_EXISTS))
			return strdup(icon_name);
		else
			return NULL;
	}

	GSList *basenames = NULL;
	basenames = g_slist_append(basenames, "~/.icons");
	basenames = g_slist_append(basenames, "/usr/share/icons");
	basenames = g_slist_append(basenames, "/usr/share/pixmaps");

	GSList *extensions = NULL;
	extensions = g_slist_append(extensions, "png");
	extensions = g_slist_append(extensions, "xpm");

	// Stage 1: exact size match
	GSList *theme;
	for (theme = launcher->icon_themes; theme; theme = g_slist_next(theme)) {
		GSList *dir;
		for (dir = ((IconTheme*)theme->data)->list_directories; dir; dir = g_slist_next(dir)) {
			if (directory_matches_size((IconThemeDir*)dir->data, size)) {
				GSList *base;
				for (base = basenames; base; base = g_slist_next(base)) {
					GSList *ext;
					for (ext = extensions; ext; ext = g_slist_next(ext)) {
						char *base_name = (char*) base->data;
						char *theme_name = ((IconTheme*)theme->data)->name;
						char *dir_name = ((IconThemeDir*)dir->data)->name;
						char *extension = (char*) ext->data;
						char *file_name = malloc(strlen(base_name) + strlen(theme_name) +
							strlen(dir_name) + strlen(icon_name) + strlen(extension) + 100);
						// filename = directory/$(themename)/subdirectory/iconname.extension
						sprintf(file_name, "%s/%s/%s/%s.%s", base_name, theme_name, dir_name, icon_name, extension);
						//printf("checking %s\n", file_name);
						if (g_file_test(file_name, G_FILE_TEST_EXISTS)) {
							g_slist_free(basenames);
							g_slist_free(extensions);
							return file_name;
						} else {
							free(file_name);
							file_name = NULL;
						}
					}
				}
			}
		}
	}

	// Stage 2: best size match
	// Contrary to the freedesktop spec, we are not choosing the closest icon in size, but the next larger icon
	// otherwise the quality is usually crap (for size 22, if you can choose 16 or 32, you're better with 32)
	// We do fallback to the closest size if we cannot find a larger or equal icon
	int minimal_size = INT_MAX;
	char *best_file_name = NULL;
	int next_larger_size = -1;
	char *next_larger = NULL;
	for (theme = launcher->icon_themes; theme; theme = g_slist_next(theme)) {
		GSList *dir;
		for (dir = ((IconTheme*)theme->data)->list_directories; dir; dir = g_slist_next(dir)) {
			GSList *base;
			for (base = basenames; base; base = g_slist_next(base)) {
				GSList *ext;
				for (ext = extensions; ext; ext = g_slist_next(ext)) {
					char *base_name = (char*) base->data;
					char *theme_name = ((IconTheme*)theme->data)->name;
					char *dir_name = ((IconThemeDir*)dir->data)->name;
					char *extension = (char*) ext->data;
					char *file_name = malloc(strlen(base_name) + strlen(theme_name) +
					strlen(dir_name) + strlen(icon_name) + strlen(extension) + 100);
					// filename = directory/$(themename)/subdirectory/iconname.extension
					sprintf(file_name, "%s/%s/%s/%s.%s", base_name, theme_name, dir_name, icon_name, extension);
					if (g_file_test(file_name, G_FILE_TEST_EXISTS)) {
						if (directory_size_distance((IconThemeDir*)dir->data, size) < minimal_size) {
							if (best_file_name) {
								free(best_file_name);
								best_file_name = NULL;
							}
							best_file_name = strdup(file_name);
							minimal_size = directory_size_distance((IconThemeDir*)dir->data, size);
						}
						if (((IconThemeDir*)dir->data)->size >= size && (next_larger_size == -1 || ((IconThemeDir*)dir->data)->size < next_larger_size)) {
							if (next_larger) {
								free(next_larger);
								next_larger = NULL;
							}
							next_larger = strdup(file_name);
							next_larger_size = ((IconThemeDir*)dir->data)->size;
						}
					}
					free(file_name);
				}
			}
		}
	}
	if (next_larger) {
		g_slist_free(basenames);
		g_slist_free(extensions);
		free(best_file_name);
		return next_larger;
	}
	if (best_file_name) {
		g_slist_free(basenames);
		g_slist_free(extensions);
		return best_file_name;
	}

	// Stage 3: look in unthemed icons
	{
		GSList *base;
		for (base = basenames; base; base = g_slist_next(base)) {
			GSList *ext;
			for (ext = extensions; ext; ext = g_slist_next(ext)) {
				char *base_name = (char*) base->data;
				char *extension = (char*) ext->data;
				char *file_name = malloc(strlen(base_name) + strlen(icon_name) +
					strlen(extension) + 100);
				// filename = directory/iconname.extension
				sprintf(file_name, "%s/%s.%s", base_name, icon_name, extension);
				//printf("checking %s\n", file_name);
				if (g_file_test(file_name, G_FILE_TEST_EXISTS)) {
					g_slist_free(basenames);
					g_slist_free(extensions);
					return file_name;
				} else {
					free(file_name);
					file_name = NULL;
				}
			}
		}
	}

	fprintf(stderr, "Could not find icon %s\n", icon_name);

	return NULL;
}
