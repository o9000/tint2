/**************************************************************************
* Tint2 : Icon theme handling
*
* Copyright (C) 2015       (mrovi9000@gmail.com)
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

/* http://standards.freedesktop.org/icon-theme-spec/ */

#include "icon-theme-common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apps-common.h"

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


int parse_theme_line(char *line, char **key, char **value)
{
	return parse_dektop_line(line, key, value);
}

GSList *icon_locations = NULL;
// Do not free the result.
const GSList *get_icon_locations()
{
	if (icon_locations)
		return icon_locations;

	gchar *path;
	path = g_build_filename(g_get_home_dir(), ".icons", NULL);
	icon_locations = g_slist_append(icon_locations, g_strdup(path));
	g_free(path);
	path = g_build_filename(g_get_home_dir(), ".local/share/icons", NULL);
	icon_locations = g_slist_append(icon_locations, g_strdup(path));
	g_free(path);
	icon_locations = g_slist_append(icon_locations, g_strdup("/usr/local/share/icons"));
	icon_locations = g_slist_append(icon_locations, g_strdup("/usr/local/share/pixmaps"));
	icon_locations = g_slist_append(icon_locations, g_strdup("/usr/share/icons"));
	icon_locations = g_slist_append(icon_locations, g_strdup("/usr/share/pixmaps"));
	icon_locations = g_slist_append(icon_locations, g_strdup("/opt/share/icons"));
	icon_locations = g_slist_append(icon_locations, g_strdup("/opt/share/pixmaps"));
	return icon_locations;
}

IconTheme *make_theme(char *name)
{
	IconTheme *theme = calloc(1, sizeof(IconTheme));
	theme->name = strdup(name);
	theme->list_inherits = NULL;
	theme->list_directories = NULL;
	return theme;
}

//TODO Use UTF8 when parsing the file
IconTheme *load_theme_from_index(char *file_name, char *name)
{
	IconTheme *theme;
	FILE *f;
	char *line = NULL;
	size_t line_size;

	if ((f = fopen(file_name, "rt")) == NULL) {
		fprintf(stderr, "Could not open theme '%s'\n", file_name);
		return NULL;
	}

	theme = make_theme(name);

	IconThemeDir *current_dir = NULL;
	int inside_header = 1;
	while (getline(&line, &line_size, f) >= 0) {
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
					while (token != NULL) {
						theme->list_inherits = g_slist_append(theme->list_inherits, strdup(token));
						token = strtok(NULL, ",\n");
					}
				} else if (strcmp(key, "Directories") == 0) {
					// value is like 48x48/apps,48x48/mimetypes,32x32/apps,scalable/apps,scalable/mimetypes
					char *token;
					token = strtok(value, ",\n");
					while (token != NULL) {
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
					// value is Fixed, Scalable or Threshold : default to scalable for unknown Type.
					if (strcmp(value, "Fixed") == 0) {
						current_dir->type = ICON_DIR_TYPE_FIXED;
					} else if (strcmp(value, "Threshold") == 0) {
						current_dir->type = ICON_DIR_TYPE_THRESHOLD;
					} else {
						current_dir->type = ICON_DIR_TYPE_SCALABLE;
					}
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

	free(line);
	return theme;
}

void load_theme_from_fs_dir(IconTheme *theme, char *dir_name)
{
	gchar *file_name = g_build_filename(dir_name, "index.theme", NULL);
	if (g_file_test(file_name, G_FILE_TEST_EXISTS)) {
		g_free(file_name);
		return;
	}

	GDir *d = g_dir_open(dir_name, 0, NULL);
	if (d) {
		const gchar *size_name;
		while ((size_name = g_dir_read_name(d))) {
			gchar *full_size_name = g_build_filename(dir_name, size_name, NULL);
			if (g_file_test(file_name, G_FILE_TEST_IS_DIR)) {
				int size, size2;
				if ((sscanf(size_name, "%dx%d", &size, &size2) == 2 && size == size2) ||
					(sscanf(size_name, "%d", &size) == 1)) {
					GDir *dSize = g_dir_open(full_size_name, 0, NULL);
					if (dSize) {
						const gchar *subdir_name;
						while ((subdir_name = g_dir_read_name(dSize))) {
							IconThemeDir *dir = calloc(1, sizeof(IconThemeDir));
							// value is like 48x48/apps
							gchar *value = g_build_filename(size_name, subdir_name, NULL);
							dir->name = strdup(value);
							g_free(value);
							dir->max_size = dir->min_size = dir->size = size;
							dir->type = ICON_DIR_TYPE_FIXED;
							theme->list_directories = g_slist_append(theme->list_directories, dir);
						}
						g_dir_close(dSize);
					}
				}
			}
			g_free(full_size_name);
		}
		g_dir_close(d);
	}
}

IconTheme *load_theme_from_fs(char *name, IconTheme *theme)
{
	char *dir_name = NULL;
	const GSList *location;
	for (location = get_icon_locations(); location; location = g_slist_next(location)) {
		gchar *path = (gchar*) location->data;
		dir_name = g_build_filename(path, name, NULL);
		if (g_file_test(dir_name, G_FILE_TEST_IS_DIR)) {
			if (!theme) {
				theme = make_theme(name);
			}
			load_theme_from_fs_dir(theme, dir_name);
		}
		g_free(dir_name);
		dir_name = NULL;
	}

	return theme;
}

IconTheme *load_theme(char *name)
{
	// Look for name/index.theme in $HOME/.icons, /usr/share/icons, /usr/share/pixmaps (stop at the first found)
	// Parse index.theme -> list of IconThemeDir with attributes
	// Return IconTheme*

	if (name == NULL)
		return NULL;

	char *file_name = NULL;
	const GSList *location;
	for (location = get_icon_locations(); location; location = g_slist_next(location)) {
		gchar *path = (gchar*) location->data;
		file_name = g_build_filename(path, name, "index.theme", NULL);
		if (!g_file_test(file_name, G_FILE_TEST_EXISTS)) {
			g_free(file_name);
			file_name = NULL;
		}
		if (file_name)
			break;
	}

	IconTheme *theme = NULL;
	if (file_name) {
		theme = load_theme_from_index(file_name, name);
		g_free(file_name);
	}

	return load_theme_from_fs(name, theme);
}

void free_icon_theme(IconTheme *theme)
{
	if (!theme)
		return;
	free(theme->name);
	theme->name = NULL;
	GSList *l_inherits;
	for (l_inherits = theme->list_inherits; l_inherits ; l_inherits = l_inherits->next) {
		free(l_inherits->data);
	}
	g_slist_free(theme->list_inherits);
	theme->list_inherits = NULL;
	GSList *l_dir;
	for (l_dir = theme->list_directories; l_dir ; l_dir = l_dir->next) {
		IconThemeDir *dir = (IconThemeDir *)l_dir->data;
		free(dir->name);
		free(l_dir->data);
	}
	g_slist_free(theme->list_directories);
	theme->list_directories = NULL;
}

void free_themes(IconThemeWrapper *themes)
{
	if (!themes)
		return;
	GSList *l;
	for (l = themes->themes; l ; l = l->next) {
		IconTheme *theme = (IconTheme*) l->data;
		free_icon_theme(theme);
		free(theme);
	}
	g_slist_free(themes->themes);
	for (l = themes->themes_fallback; l ; l = l->next) {
		IconTheme *theme = (IconTheme*) l->data;
		free_icon_theme(theme);
		free(theme);
	}
	g_slist_free(themes->themes_fallback);
	free(themes);
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
		printf("Dir:%s Size=%d MinSize=%d MaxSize=%d Threshold=%d Type=%s\n",
			   dir->name, dir->size, dir->min_size, dir->max_size, dir->threshold,
			   dir->type == ICON_DIR_TYPE_FIXED ? "Fixed" :
			   dir->type == ICON_DIR_TYPE_SCALABLE ? "Scalable" :
			   dir->type == ICON_DIR_TYPE_THRESHOLD ? "Threshold" : "?????");
		item = g_slist_next(item);
	}
	fprintf(stdout, "\033[0m");
}

gboolean str_list_contains(const GSList *list, const char *value)
{
	const GSList* item = list;
	while (item != NULL) {
		if (g_str_equal(item->data, value)) {
			return TRUE;
		}
		item = g_slist_next(item);
	}
	return FALSE;
}

void load_themes_helper(const char *name, GSList **themes, GSList **queued)
{
	if (str_list_contains(*queued, name))
		return;
	GSList *queue = g_slist_append(NULL, strdup(name));
	*queued = g_slist_append(*queued, strdup(name));

	// Load wrapper->themes
	while (queue) {
		char *name = queue->data;
		queue = g_slist_remove(queue, name);

		fprintf(stderr, " '%s',", name);
		IconTheme *theme = load_theme(name);
		if (theme != NULL) {
			*themes = g_slist_append(*themes, theme);

			GSList* item = theme->list_inherits;
			int pos = 0;
			while (item != NULL)
			{
				char *parent = item->data;
				if (!str_list_contains(*queued, parent)) {
					queue = g_slist_insert(queue, strdup(parent), pos);
					pos++;
					*queued = g_slist_append(*queued, strdup(parent));
				}
				item = g_slist_next(item);
			}
		}

		free(name);
	}
	fprintf(stderr, "\n");
	fflush(stderr);

	// Free the queue
	GSList *l;
	for (l = queue; l ; l = l->next)
		free(l->data);
	g_slist_free(queue);
}

IconThemeWrapper *load_themes(const char *icon_theme_name)
{
	IconThemeWrapper *wrapper = calloc(1, sizeof(IconThemeWrapper));

	if (!icon_theme_name) {
		fprintf(stderr, "Missing icon_theme_name theme, default to 'hicolor'.\n");
		icon_theme_name = "hicolor";
	} else {
		fprintf(stderr, "Loading %s. Icon theme :", icon_theme_name);
	}

	GSList *queued = NULL;
	load_themes_helper(icon_theme_name, &wrapper->themes, &queued);
	load_themes_helper("hicolor", &wrapper->themes, &queued);

	// Load wrapper->themes_fallback
	const GSList *location;
	for (location = get_icon_locations(); location; location = g_slist_next(location)) {
		gchar *path = (gchar*) location->data;
		GDir *d = g_dir_open(path, 0, NULL);
		if (d) {
			const gchar *name;
			while ((name = g_dir_read_name(d))) {
				gchar *file_name = g_build_filename(path, name, "index.theme", NULL);
				if (g_file_test(file_name, G_FILE_TEST_EXISTS) &&
					g_file_test(file_name, G_FILE_TEST_IS_REGULAR)) {
					load_themes_helper(name, &wrapper->themes_fallback, &queued);
				}
				g_free(file_name);
			}
			g_dir_close(d);
		}
	}

	// Free the queued list
	GSList *l;
	for (l = queued; l ; l = l->next)
		free(l->data);
	g_slist_free(queued);

	return wrapper;
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

gint compare_theme_directories(gconstpointer a, gconstpointer b, gpointer size_query)
{
	int size = GPOINTER_TO_INT(size_query);
	const IconThemeDir *da = (const IconThemeDir*)a;
	const IconThemeDir *db = (const IconThemeDir*)b;
	return abs(da->size - size) - abs(db->size - size);
}

#define DEBUG_ICON_SEARCH 0
char *get_icon_path_helper(GSList *themes, const char *icon_name, int size)
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

	const GSList *basenames = get_icon_locations();
	GSList *extensions = NULL;
	extensions = g_slist_append(extensions, ".png");
	extensions = g_slist_append(extensions, ".xpm");
#ifdef HAVE_RSVG
	extensions = g_slist_append(extensions, ".svg");
#endif
	// if the icon name already contains one of the extensions (e.g. vlc.png instead of vlc) add a special entry
	GSList *ext;
	for (ext = extensions; ext; ext = g_slist_next(ext)) {
		char *extension = (char*) ext->data;
		if (strlen(icon_name) > strlen(extension) &&
			strcmp(extension, icon_name + strlen(icon_name) - strlen(extension)) == 0) {
			extensions = g_slist_append(extensions, "");
			break;
		}
	}

	GSList *theme;

	// Best size match
	// Contrary to the freedesktop spec, we are not choosing the closest icon in size, but the next larger icon
	// otherwise the quality is usually crap (for size 22, if you can choose 16 or 32, you're better with 32)
	// We do fallback to the closest size if we cannot find a larger or equal icon

	// These 3 variables are used for keeping the closest size match
	int minimal_size = INT_MAX;
	char *best_file_name = NULL;
	GSList *best_file_theme = NULL;

	// These 3 variables are used for keeping the next larger match
	int next_larger_size = -1;
	char *next_larger = NULL;
	GSList *next_larger_theme = NULL;

	for (theme = themes; theme; theme = g_slist_next(theme)) {
		((IconTheme*)theme->data)->list_directories = g_slist_sort_with_data(((IconTheme*)theme->data)->list_directories,
																			 compare_theme_directories,
																			 GINT_TO_POINTER(size));
		GSList *dir;
		for (dir = ((IconTheme*)theme->data)->list_directories; dir; dir = g_slist_next(dir)) {
			// Closest match
			gboolean possible = directory_size_distance((IconThemeDir*)dir->data, size) < minimal_size &&
								(!best_file_theme ? TRUE : theme == best_file_theme);
			// Next larger match
			possible = possible ||
					   (((IconThemeDir*)dir->data)->size >= size &&
						(next_larger_size == -1 || ((IconThemeDir*)dir->data)->size < next_larger_size) &&
						(!next_larger_theme ? 1 : theme == next_larger_theme));
			if (!possible)
				continue;
			const GSList *base;
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
					sprintf(file_name, "%s/%s/%s/%s%s", base_name, theme_name, dir_name, icon_name, extension);
					if (DEBUG_ICON_SEARCH)
						printf("checking %s\n", file_name);
					if (g_file_test(file_name, G_FILE_TEST_EXISTS)) {
						if (DEBUG_ICON_SEARCH)
							printf("found: %s\n", file_name);
						// Closest match
						if (directory_size_distance((IconThemeDir*)dir->data, size) < minimal_size && (!best_file_theme ? 1 : theme == best_file_theme)) {
							if (best_file_name) {
								free(best_file_name);
								best_file_name = NULL;
							}
							best_file_name = strdup(file_name);
							minimal_size = directory_size_distance((IconThemeDir*)dir->data, size);
							best_file_theme = theme;
							if (DEBUG_ICON_SEARCH)
								printf("best_file_name = %s; minimal_size = %d\n", best_file_name, minimal_size);
						}
						// Next larger match
						if (((IconThemeDir*)dir->data)->size >= size &&
							(next_larger_size == -1 || ((IconThemeDir*)dir->data)->size < next_larger_size) &&
							(!next_larger_theme ? 1 : theme == next_larger_theme)) {
							if (next_larger) {
								free(next_larger);
								next_larger = NULL;
							}
							next_larger = strdup(file_name);
							next_larger_size = ((IconThemeDir*)dir->data)->size;
							next_larger_theme = theme;
							if (DEBUG_ICON_SEARCH)
								printf("next_larger = %s; next_larger_size = %d\n", next_larger, next_larger_size);
						}
					}
					free(file_name);
				}
			}
		}
	}
	if (next_larger) {
		g_slist_free(extensions);
		free(best_file_name);
		return next_larger;
	}
	if (best_file_name) {
		g_slist_free(extensions);
		return best_file_name;
	}

	// Look in unthemed icons
	{
		const GSList *base;
		for (base = basenames; base; base = g_slist_next(base)) {
			GSList *ext;
			for (ext = extensions; ext; ext = g_slist_next(ext)) {
				char *base_name = (char*) base->data;
				char *extension = (char*) ext->data;
				char *file_name = malloc(strlen(base_name) + strlen(icon_name) +
					strlen(extension) + 100);
				// filename = directory/iconname.extension
				sprintf(file_name, "%s/%s%s", base_name, icon_name, extension);
				if (DEBUG_ICON_SEARCH)
					printf("checking %s\n", file_name);
				if (g_file_test(file_name, G_FILE_TEST_EXISTS)) {
					g_slist_free(extensions);
					return file_name;
				} else {
					free(file_name);
					file_name = NULL;
				}
			}
		}
	}

	g_slist_free(extensions);
	return NULL;
}

char *get_icon_path(IconThemeWrapper *theme, const char *icon_name, int size)
{
	if (!theme)
		return NULL;
	icon_name = icon_name ? icon_name : DEFAULT_ICON;
	char *path = get_icon_path_helper(theme->themes, icon_name, size);
	if (!path) {
		path = get_icon_path_helper(theme->themes_fallback, icon_name, size);
	}
	if (!path) {
		fprintf(stderr, "Could not find icon %s\n", icon_name);
		path = get_icon_path_helper(theme->themes, DEFAULT_ICON, size);
	}
	if (!path) {
		path = get_icon_path_helper(theme->themes_fallback, DEFAULT_ICON, size);
	}
	return path;
}
