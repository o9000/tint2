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
#include <glib/gi18n.h>

#ifdef HAVE_SN
#include <libsn/sn.h>
#endif

#include "window.h"
#include "server.h"
#include "area.h"
#include "panel.h"
#include "taskbar.h"
#include "launcher.h"

int launcher_enabled;
int launcher_max_icon_size;
int launcher_tooltip_enabled;
int launcher_alpha;
int launcher_saturation;
int launcher_brightness;
char *icon_theme_name; 
XSettingsClient *xsettings_client;

#define ICON_FALLBACK "application-x-executable"

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
	launcher_tooltip_enabled = 0;
	launcher_alpha = 100;
	launcher_saturation = 0;
	launcher_brightness = 0;
	icon_theme_name = 0;
	xsettings_client = NULL;
}


void init_launcher()
{
	if (launcher_enabled) {
		// if XSETTINGS manager running, tint2 read the icon_theme_name.
		xsettings_client = xsettings_client_new(server.dsp, server.screen, xsettings_notify_cb, NULL, NULL);
	}
}


void init_launcher_panel(void *p)
{
	Panel *panel =(Panel*)p;
	Launcher *launcher = &panel->launcher;

	launcher->area.parent = p;
	launcher->area.panel = p;
	launcher->area._draw_foreground = NULL;
	launcher->area.size_mode = SIZE_BY_CONTENT;
	launcher->area._resize = resize_launcher;
	launcher->area.resize = 1;
	launcher->area.redraw = 1;
	if (launcher->area.bg == 0)
		launcher->area.bg = &g_array_index(backgrounds, Background, 0);

	// check consistency
	if (launcher->list_apps == NULL)
		return;

	launcher->area.on_screen = 1;
	panel_refresh = 1;

	launcher_load_themes(launcher);
	launcher_load_icons(launcher);
}


void cleanup_launcher()
{
	int i;
	GSList *l;

	if (xsettings_client)
		xsettings_client_destroy(xsettings_client);
	for (i = 0 ; i < nb_panel ; i++) {
		Panel *panel = &panel1[i];
		Launcher *launcher = &panel->launcher;		
		cleanup_launcher_theme(launcher);
	}
	for (l = panel_config.launcher.list_apps; l ; l = l->next) {
		free(l->data);
	}
	g_slist_free(panel_config.launcher.list_apps);
	panel_config.launcher.list_apps = NULL;
	free(icon_theme_name);
	icon_theme_name = 0;
	launcher_enabled = 0;
}


void cleanup_launcher_theme(Launcher *launcher)
{
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
			free(launcherIcon->icon_tooltip);
		}
		free(launcherIcon);
	}
	g_slist_free(launcher->list_icons);

	for (l = launcher->list_themes; l ; l = l->next) {
		IconTheme *theme = (IconTheme*) l->data;
		free_icon_theme(theme);
		free(theme);
	}
	g_slist_free(launcher->list_themes);
	launcher->list_icons = launcher->list_themes = NULL;
}


int resize_launcher(void *obj)
{
	Launcher *launcher = obj;
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
			launcherIcon->area.width = launcherIcon->icon_size;
			launcherIcon->area.height = launcherIcon->icon_size;

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
					fprintf(stderr, "launcher.c %d: Using icon %s\n", __LINE__, new_icon_path);
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
				fprintf(stderr, "launcher.c %d: Using icon %s\n", __LINE__, launcherIcon->icon_path);
			} else {
				// Free the old files
				free_icon(launcherIcon->icon_original);
				free_icon(launcherIcon->icon_scaled);
				// Load the new file and scale
				launcherIcon->icon_original = imlib_load_image(new_icon_path);
				launcherIcon->icon_scaled = scale_icon(launcherIcon->icon_original, launcherIcon->icon_size);
				free(launcherIcon->icon_path);
				launcherIcon->icon_path = new_icon_path;
				fprintf(stderr, "launcher.c %d: Using icon %s\n", __LINE__, launcherIcon->icon_path);
			}
		}
	}
	
	count = g_slist_length(launcher->list_icons);

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
	}

	int i, posx, posy;
	int start = launcher->area.bg->border.width + launcher->area.paddingy + marging/2;
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
		//printf("launcher %d : %d,%d\n", i, posx, posy);
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
	return 1;
}

// Here we override the default layout of the icons; normally Area layouts its children
// in a stack; we need to layout them in a kind of table
void launcher_icon_on_change_layout(void *obj)
{
	LauncherIcon *launcherIcon = (LauncherIcon*)obj;
	launcherIcon->area.posy = ((Area*)launcherIcon->area.parent)->posy + launcherIcon->y;
	launcherIcon->area.posx = ((Area*)launcherIcon->area.parent)->posx + launcherIcon->x;
}

const char* launcher_icon_get_tooltip_text(void *obj)
{
	LauncherIcon *launcherIcon = (LauncherIcon*)obj;
	return launcherIcon->icon_tooltip;
}

void draw_launcher_icon(void *obj, cairo_t *c)
{
	LauncherIcon *launcherIcon = (LauncherIcon*)obj;
	Imlib_Image icon_scaled = launcherIcon->icon_scaled;
	// Render
	imlib_context_set_image (icon_scaled);
	if (server.real_transparency) {
		render_image(launcherIcon->area.pix, 0, 0, imlib_image_get_width(), imlib_image_get_height() );
	} else {
		imlib_context_set_drawable(launcherIcon->area.pix);
		imlib_render_image_on_drawable (0, 0);
	}
}

Imlib_Image scale_icon(Imlib_Image original, int icon_size)
{
	Imlib_Image icon_scaled;
	if (original) {
		imlib_context_set_image (original);
		icon_scaled = imlib_create_cropped_scaled_image(0, 0, imlib_image_get_width(), imlib_image_get_height(), icon_size, icon_size);
		imlib_context_set_image (icon_scaled);
		imlib_image_set_has_alpha(1);
		DATA32* data = imlib_image_get_data();
		adjust_asb(data, icon_size, icon_size, launcher_alpha, (float)launcher_saturation/100, (float)launcher_brightness/100);
		imlib_image_put_back_data(data);
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

void launcher_action(LauncherIcon *icon, XEvent* evt)
{
	char *cmd = malloc(strlen(icon->cmd) + 10);
	sprintf(cmd, "(%s&)", icon->cmd);
#if HAVE_SN
		SnLauncherContext* ctx;
		Time time;

		ctx = sn_launcher_context_new(server.sn_dsp, server.screen);
		sn_launcher_context_set_name(ctx, icon->icon_tooltip);
		sn_launcher_context_set_description(ctx, "Application launched from tint2");
		sn_launcher_context_set_binary_name (ctx, icon->cmd);
		// Get a timestamp from the X event
		if (evt->type == ButtonPress || evt->type == ButtonRelease) {
		    time = evt->xbutton.time;
		}
		else {
		    	fprintf(stderr, "Unknown X event: %d\n", evt->type);
			free(cmd);
			return;
		}
		sn_launcher_context_initiate(ctx, "tint2", icon->cmd, time);
#endif /* HAVE_SN */
	pid_t pid;
	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Could not fork\n");
	}
	else if (pid == 0) {
#if HAVE_SN
	        sn_launcher_context_setup_child_process (ctx);
#endif // HAVE_SN
		// Allow children to exist after parent destruction
		setsid ();
		// Run the command
		execl("/bin/sh", "/bin/sh", "-c", icon->cmd, NULL);

		fprintf(stderr, "Failed to execlp %s\n", icon->cmd);
#if HAVE_SN
		sn_launcher_context_unref (ctx);
#endif // HAVE_SN
		_exit(1);
	}
#if HAVE_SN
	else {
	        g_tree_insert (server.pids, GINT_TO_POINTER (pid), ctx);
	}
#endif // HAVE_SN
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
		char *exec2 = malloc(strlen(entry->exec) + (entry->name ? strlen(entry->name) : 1) + (entry->icon ? strlen(entry->icon) : 1) + 100);
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
	char *line = NULL;
	size_t line_size;
	char *key, *value;
	int i;

	entry->name = entry->icon = entry->exec = NULL;

	if ((fp = fopen(path, "rt")) == NULL) {
		fprintf(stderr, "Could not open file %s\n", path);
		return 0;
	}

	gchar **languages = (gchar **)g_get_language_names();
	// lang_index is the index of the language for the best Name key in the language vector
	// lang_index_default is a constant that encodes the Name key without a language
	int lang_index, lang_index_default;
#define LANG_DBG 0
	if (LANG_DBG) printf("Languages:");
	for (i = 0; languages[i]; i++) {
		if (LANG_DBG) printf(" %s", languages[i]);
	}
	if (LANG_DBG) printf("\n");
	lang_index_default = i;
	// we currently do not know about any Name key at all, so use an invalid index
	lang_index = lang_index_default + 1;

	int inside_desktop_entry = 0;
	while (getline(&line, &line_size, fp) >= 0) {
		int len = strlen(line);
		if (len == 0)
			continue;
		line[len - 1] = '\0';
		if (line[0] == '[') {
			inside_desktop_entry = (strcmp(line, "[Desktop Entry]") == 0);
		}
		if (inside_desktop_entry && parse_dektop_line(line, &key, &value)) {
			if (strstr(key, "Name") == key) {
				if (strcmp(key, "Name") == 0 && lang_index > lang_index_default) {
					entry->name = strdup(value);
					lang_index = lang_index_default;
				} else {
					for (i = 0; languages[i] && i < lang_index; i++) {
						gchar *localized_key = g_strdup_printf("Name[%s]", languages[i]);
						if (strcmp(key, localized_key) == 0) {
							if (entry->name)
								free(entry->name);
							entry->name = strdup(value);
							lang_index = i;
						}
						g_free(localized_key);
					}
				}
			} else if (!entry->exec && strcmp(key, "Exec") == 0) {
				entry->exec = strdup(value);
			} else if (!entry->icon && strcmp(key, "Icon") == 0) {
				entry->icon = strdup(value);
			}
		}
	}
	fclose (fp);
	// From this point:
	// entry->name, entry->icon, entry->exec will never be empty strings (can be NULL though)

	expand_exec(entry, path);
	
	free(line);
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
	char *line = NULL;
	size_t line_size;

	if (name == NULL)
		return NULL;

	file_name = g_build_filename(g_get_home_dir(), ".icons", name, "index.theme", NULL);
	if (!g_file_test(file_name, G_FILE_TEST_EXISTS)) {
		g_free (file_name);
		file_name = g_build_filename("/usr/share/icons", name, "index.theme", NULL);
		if (!g_file_test(file_name, G_FILE_TEST_EXISTS)) {
			g_free (file_name);
			file_name = g_build_filename("/usr/share/pixmaps", name, "index.theme", NULL);
			if (!g_file_test(file_name, G_FILE_TEST_EXISTS)) {
				g_free (file_name);
				file_name = NULL;
			}
		}
	}

	if (!file_name) {
		return NULL;
	}

	if ((f = fopen(file_name, "rt")) == NULL) {
		fprintf(stderr, "Could not open theme '%s'\n", file_name);
		return NULL;
	}

	g_free (file_name);

	theme = calloc(1, sizeof(IconTheme));
	theme->name = strdup(name);
	theme->list_inherits = NULL;
	theme->list_directories = NULL;

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
					// value is Fixed, Scalable or Threshold : default to scalable for unknown Type.
					if (strcmp(value, "Fixed") == 0) {
						current_dir->type = ICON_DIR_TYPE_FIXED;
					} else if (strcmp(value, "Threshold") == 0) {
						current_dir->type = ICON_DIR_TYPE_THRESHOLD;
					} else {
						current_dir->type = ICON_DIR_TYPE_SCALABLE;
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
	
	free(line);
	
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


// Populates the list_icons list
void launcher_load_icons(Launcher *launcher)
{
	// Load apps (.desktop style launcher items)
	GSList* app = launcher->list_apps;
	while (app != NULL) {
		DesktopEntry entry;
		launcher_read_desktop_file(app->data, &entry);
		if (entry.exec) {
			LauncherIcon *launcherIcon = calloc(1, sizeof(LauncherIcon));
			launcherIcon->area.parent = launcher;
			launcherIcon->area.panel = launcher->area.panel;
			launcherIcon->area._draw_foreground = draw_launcher_icon;
			launcherIcon->area.size_mode = SIZE_BY_CONTENT;
			launcherIcon->area._resize = NULL;
			launcherIcon->area.resize = 0;
			launcherIcon->area.redraw = 1;
			launcherIcon->area.bg = &g_array_index(backgrounds, Background, 0);
			launcherIcon->area.on_screen = 1;
			launcherIcon->area._on_change_layout = launcher_icon_on_change_layout;
			if (launcher_tooltip_enabled) 
				launcherIcon->area._get_tooltip_text = launcher_icon_get_tooltip_text;
			else
				launcherIcon->area._get_tooltip_text = NULL;
			launcherIcon->is_app_desktop = 1;
			launcherIcon->cmd = strdup(entry.exec);
			launcherIcon->icon_name = entry.icon ? strdup(entry.icon) : strdup(ICON_FALLBACK);
			launcherIcon->icon_size = 1;
			launcherIcon->icon_tooltip = entry.name ? strdup(entry.name) : strdup(entry.exec);
			free_desktop_entry(&entry);
			launcher->list_icons = g_slist_append(launcher->list_icons, launcherIcon);
			add_area(&launcherIcon->area);
		}
		app = g_slist_next(app);
	}
}


// Populates the list_themes list
void launcher_load_themes(Launcher *launcher)
{
	// load the user theme, all the inherited themes recursively (DFS), and the hicolor theme
	// avoid inheritance loops
	if (!icon_theme_name) {
		fprintf(stderr, "Missing launcher theme, default to 'hicolor'.\n");
		icon_theme_name = strdup("hicolor");
	} else {
		fprintf(stderr, "Loading %s. Icon theme :", icon_theme_name);
	}

	GSList *queue = g_slist_append(NULL, strdup(icon_theme_name));
	GSList *queued = g_slist_append(NULL, strdup(icon_theme_name));

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

		fprintf(stderr, " '%s',", name);
		IconTheme *theme = load_theme(name);
		if (theme != NULL) {
			launcher->list_themes = g_slist_append(launcher->list_themes, theme);

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
	fprintf(stderr, "\n");

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

#define DEBUG_ICON_SEARCH 0
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
	char *home_icons = g_build_filename(g_get_home_dir(), ".icons", NULL);
	basenames = g_slist_append(basenames, home_icons);
	char *home_local_icons = g_build_filename(g_get_home_dir(), ".local/share/icons", NULL);
	basenames = g_slist_append(basenames, home_local_icons);
	basenames = g_slist_append(basenames, "/usr/local/share/icons");
	basenames = g_slist_append(basenames, "/usr/local/share/pixmaps");
	basenames = g_slist_append(basenames, "/usr/share/icons");
	basenames = g_slist_append(basenames, "/usr/share/pixmaps");

	GSList *extensions = NULL;
	extensions = g_slist_append(extensions, ".png");
	extensions = g_slist_append(extensions, ".xpm");
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
	// Stage 1: exact size match
	// the theme must have a higher priority than having an exact size match, so we will just use
	// the code that searches for the best size match (it will find the exact size match if one exists)
	/*
	for (theme = launcher->list_themes; theme; theme = g_slist_next(theme)) {
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
						sprintf(file_name, "%s/%s/%s/%s%s", base_name, theme_name, dir_name, icon_name, extension);
						//printf("found exact: %s\n", file_name);
						//printf("checking %s\n", file_name);
						if (g_file_test(file_name, G_FILE_TEST_EXISTS)) {
							g_slist_free(basenames);
							g_slist_free(extensions);
							g_free(home_icons);
							g_free(home_local_icons);
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
	g_free (file_name);
	*/

	// Stage 2: best size match
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
	
	for (theme = launcher->list_themes; theme; theme = g_slist_next(theme)) {
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
		g_slist_free(basenames);
		g_slist_free(extensions);
		free(best_file_name);
		g_free(home_icons);
		g_free(home_local_icons);
		return next_larger;
	}
	if (best_file_name) {
		g_slist_free(basenames);
		g_slist_free(extensions);
		g_free(home_icons);
		g_free(home_local_icons);
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
				sprintf(file_name, "%s/%s%s", base_name, icon_name, extension);
				if (DEBUG_ICON_SEARCH)
					printf("checking %s\n", file_name);
				if (g_file_test(file_name, G_FILE_TEST_EXISTS)) {
					g_slist_free(basenames);
					g_slist_free(extensions);
					g_free(home_icons);
					g_free(home_local_icons);
					return file_name;
				} else {
					free(file_name);
					file_name = NULL;
				}
			}
		}
	}

	fprintf(stderr, "Could not find icon %s\n", icon_name);

	g_slist_free(basenames);
	g_slist_free(extensions);
	g_free(home_icons);
	g_free(home_local_icons);
	return NULL;
}

