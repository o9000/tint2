/**************************************************************************
* Tint2 : launcher
*
* Copyright (C) 2010       (mrovi@interfete-web-club.com)
*
* SVG support: https://github.com/ixxra/tint2-svg
* Copyright (C) 2010       Rene Garcia (garciamx@gmail.com)
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
#include <glib.h>
#include <glib/gstdio.h>
#include <sys/types.h>

#include "window.h"
#include "server.h"
#include "area.h"
#include "panel.h"
#include "taskbar.h"
#include "launcher.h"
#include "apps-common.h"
#include "icon-theme-common.h"

int launcher_enabled;
int launcher_max_icon_size;
int launcher_tooltip_enabled;
int launcher_alpha;
int launcher_saturation;
int launcher_brightness;
char *icon_theme_name_config;
char *icon_theme_name_xsettings;
int launcher_icon_theme_override;
int startup_notifications;
Background *launcher_icon_bg;
GList *launcher_icon_gradients;

IconThemeWrapper *icon_theme_wrapper;

Imlib_Image scale_icon(Imlib_Image original, int icon_size);
void free_icon(Imlib_Image icon);
void launcher_icon_dump_geometry(void *obj, int indent);
void launcher_reload_icon(Launcher *launcher, LauncherIcon *launcherIcon);
void launcher_reload_icon_image(Launcher *launcher, LauncherIcon *launcherIcon);
void launcher_reload_hidden_icons(Launcher *launcher);
void launcher_icon_on_change_layout(void *obj);
int launcher_compute_desired_size(void *obj);

void default_launcher()
{
	launcher_enabled = 0;
	launcher_max_icon_size = 0;
	launcher_tooltip_enabled = 0;
	launcher_alpha = 100;
	launcher_saturation = 0;
	launcher_brightness = 0;
	icon_theme_name_config = NULL;
	icon_theme_name_xsettings = NULL;
	launcher_icon_theme_override = 0;
	startup_notifications = 0;
	launcher_icon_bg = NULL;
	launcher_icon_gradients = NULL;
}

void init_launcher()
{
}

void init_launcher_panel(void *p)
{
	Panel *panel = (Panel *)p;
	Launcher *launcher = &panel->launcher;

	launcher->area.parent = p;
	launcher->area.panel = p;
	snprintf(launcher->area.name, sizeof(launcher->area.name), "Launcher");
	launcher->area._draw_foreground = NULL;
	launcher->area.size_mode = LAYOUT_FIXED;
	launcher->area._resize = resize_launcher;
	launcher->area._compute_desired_size = launcher_compute_desired_size;
	launcher->area.resize_needed = 1;
	schedule_redraw(&launcher->area);
	if (!launcher->area.bg)
		launcher->area.bg = &g_array_index(backgrounds, Background, 0);

	if (!launcher_icon_bg)
		launcher_icon_bg = &g_array_index(backgrounds, Background, 0);

	// check consistency
	if (launcher->list_apps == NULL)
		return;

	// This will be recomputed on resize, we just initialize to a non-zero value
	launcher->icon_size = launcher_max_icon_size > 0 ? launcher_max_icon_size : 24;

	launcher->area.on_screen = TRUE;
	schedule_panel_redraw();
	instantiate_area_gradients(&launcher->area);

	load_icon_themes();
	launcher_load_icons(launcher);
}

void free_icon_themes()
{
	free_themes(icon_theme_wrapper);
	icon_theme_wrapper = NULL;
}

void cleanup_launcher()
{
	for (int i = 0; i < num_panels; i++) {
		Panel *panel = &panels[i];
		Launcher *launcher = &panel->launcher;
		cleanup_launcher_theme(launcher);
	}

	for (GSList *l = panel_config.launcher.list_apps; l; l = l->next) {
		free(l->data);
	}
	g_slist_free(panel_config.launcher.list_apps);
	panel_config.launcher.list_apps = NULL;

	free(icon_theme_name_config);
	icon_theme_name_config = NULL;

	free(icon_theme_name_xsettings);
	icon_theme_name_xsettings = NULL;

	launcher_enabled = FALSE;
}

void cleanup_launcher_theme(Launcher *launcher)
{
	free_area(&launcher->area);
	for (GSList *l = launcher->list_icons; l; l = l->next) {
		LauncherIcon *launcherIcon = (LauncherIcon *)l->data;
		if (launcherIcon) {
			free_icon(launcherIcon->image);
			free_icon(launcherIcon->image_hover);
			free_icon(launcherIcon->image_pressed);
			free(launcherIcon->icon_name);
			free(launcherIcon->icon_path);
			free(launcherIcon->cmd);
			g_free(launcherIcon->icon_tooltip);
			free(launcherIcon->config_path);
		}
		free(launcherIcon);
	}
	g_slist_free(launcher->list_icons);
	launcher->list_icons = NULL;
}

int launcher_compute_icon_size(Launcher *launcher)
{
	int icon_size = panel_horizontal ? launcher->area.height : launcher->area.width;
	icon_size = icon_size - MAX(left_right_border_width(&launcher->area), top_bottom_border_width(&launcher->area)) -
	            (2 * launcher->area.paddingy);
	if (launcher_max_icon_size > 0 && icon_size > launcher_max_icon_size)
		icon_size = launcher_max_icon_size;
	return icon_size;
}

void launcher_compute_geometry(Launcher *launcher,
                               int *size,
                               int *icon_size,
                               int *icons_per_column,
                               int *icons_per_row,
                               int *margin)
{
	int count = 0;
	for (GSList *l = launcher->list_icons; l; l = l->next) {
		LauncherIcon *launcherIcon = (LauncherIcon *)l->data;
		if (launcherIcon->area.on_screen)
			count++;
	}

	*icon_size = launcher_compute_icon_size(launcher);
	*icons_per_column = 1;
	*icons_per_row = 1;
	*margin = 0;
	if (panel_horizontal) {
		if (!count) {
			*size = 0;
		} else {
			int height = launcher->area.height - top_bottom_border_width(&launcher->area) - 2 * launcher->area.paddingy;
			// here icons_per_column always higher than 0
			*icons_per_column = (height + launcher->area.paddingx) / (*icon_size + launcher->area.paddingx);
			*margin = height - (*icons_per_column - 1) * (*icon_size + launcher->area.paddingx) - *icon_size;
			*icons_per_row = count / *icons_per_column + (count % *icons_per_column != 0);
			*size = left_right_border_width(&launcher->area) + 2 * launcher->area.paddingxlr +
			        (*icon_size * *icons_per_row) + ((*icons_per_row - 1) * launcher->area.paddingx);
		}
	} else {
		if (!count) {
			*size = 0;
		} else {
			int width = launcher->area.width - top_bottom_border_width(&launcher->area) - 2 * launcher->area.paddingy;
			// here icons_per_row always higher than 0
			*icons_per_row = (width + launcher->area.paddingx) / (*icon_size + launcher->area.paddingx);
			*margin = width - (*icons_per_row - 1) * (*icon_size + launcher->area.paddingx) - *icon_size;
			*icons_per_column = count / *icons_per_row + (count % *icons_per_row != 0);
			*size = top_bottom_border_width(&launcher->area) + 2 * launcher->area.paddingxlr +
			        (*icon_size * *icons_per_column) + ((*icons_per_column - 1) * launcher->area.paddingx);
		}
	}
}

int launcher_compute_desired_size(void *obj)
{
	Launcher *launcher = (Launcher *)obj;

	int size, icon_size, icons_per_column, icons_per_row, margin;
	launcher_compute_geometry(launcher, &size, &icon_size, &icons_per_column, &icons_per_row, &margin);
	return size;
}

gboolean resize_launcher(void *obj)
{
	Launcher *launcher = (Launcher *)obj;

	int size, icons_per_column, icons_per_row, margin;
	launcher_compute_geometry(launcher, &size, &launcher->icon_size, &icons_per_column, &icons_per_row, &margin);

	// Resize icons if necessary
	for (GSList *l = launcher->list_icons; l; l = l->next) {
		LauncherIcon *launcherIcon = (LauncherIcon *)l->data;
		if (launcherIcon->icon_size != launcher->icon_size || !launcherIcon->image) {
			launcherIcon->icon_size = launcher->icon_size;
			launcherIcon->area.width = launcherIcon->icon_size;
			launcherIcon->area.height = launcherIcon->icon_size;
			launcher_reload_icon_image(launcher, launcherIcon);
		}
	}
	save_icon_cache(icon_theme_wrapper);

	int count = 0;
	gboolean needs_repositioning = FALSE;
	for (GSList *l = launcher->list_icons; l; l = l->next) {
		LauncherIcon *launcherIcon = (LauncherIcon *)l->data;
		if (launcherIcon->area.on_screen) {
			count++;
			if (launcherIcon->area.posx < 0 || launcherIcon->area.posy < 0)
				needs_repositioning = TRUE;
		}
	}

	if (!needs_repositioning) {
		if (panel_horizontal) {
			if (launcher->area.width == size)
				return FALSE;
			launcher->area.width = size;
		} else {
			if (launcher->area.height == size)
				return FALSE;
			launcher->area.height = size;
		}
	}

	int posx, posy;
	int start;
	if (panel_horizontal) {
		posy = start = top_border_width(&launcher->area) + launcher->area.paddingy + margin / 2;
		posx = left_border_width(&launcher->area) + launcher->area.paddingxlr;
	} else {
		posx = start = left_border_width(&launcher->area) + launcher->area.paddingy + margin / 2;
		posy = top_border_width(&launcher->area) + launcher->area.paddingxlr;
	}

	int i = 0;
	for (GSList *l = launcher->list_icons; l; l = l->next) {
		LauncherIcon *launcherIcon = (LauncherIcon *)l->data;
		if (!launcherIcon->area.on_screen)
			continue;
		i++;
		launcherIcon->y = posy;
		launcherIcon->x = posx;
		launcher_icon_on_change_layout(launcherIcon);
		// printf("launcher %d : %d,%d\n", i, posx, posy);
		if (panel_horizontal) {
			if (i % icons_per_column) {
				posy += launcher->icon_size + launcher->area.paddingx;
			} else {
				posy = start;
				posx += (launcher->icon_size + launcher->area.paddingx);
			}
		} else {
			if (i % icons_per_row) {
				posx += launcher->icon_size + launcher->area.paddingx;
			} else {
				posx = start;
				posy += (launcher->icon_size + launcher->area.paddingx);
			}
		}
	}

	if ((panel_horizontal && icons_per_column == 1) || (!panel_horizontal && icons_per_row == 1)) {
		launcher->area._is_under_mouse = full_width_area_is_under_mouse;
		for (GSList *l = launcher->list_icons; l; l = l->next)
			((LauncherIcon *)l->data)->area._is_under_mouse = full_width_area_is_under_mouse;
	} else {
		launcher->area._is_under_mouse = NULL;
		for (GSList *l = launcher->list_icons; l; l = l->next)
			((LauncherIcon *)l->data)->area._is_under_mouse = NULL;
	}

	return TRUE;
}

// Here we override the default layout of the icons; normally Area layouts its children
// in a stack; we need to layout them in a kind of table
void launcher_icon_on_change_layout(void *obj)
{
	LauncherIcon *launcherIcon = (LauncherIcon *)obj;
	launcherIcon->area.posy = ((Area *)launcherIcon->area.parent)->posy + launcherIcon->y;
	launcherIcon->area.posx = ((Area *)launcherIcon->area.parent)->posx + launcherIcon->x;
	launcherIcon->area.width = launcherIcon->icon_size;
	launcherIcon->area.height = launcherIcon->icon_size;
}

int launcher_icon_compute_desired_size(void *obj)
{
	LauncherIcon *icon = (LauncherIcon *)obj;
	return icon->icon_size;
}

char *launcher_icon_get_tooltip_text(void *obj)
{
	LauncherIcon *launcherIcon = (LauncherIcon *)obj;
	return strdup(launcherIcon->icon_tooltip);
}

void draw_launcher_icon(void *obj, cairo_t *c)
{
	LauncherIcon *launcherIcon = (LauncherIcon *)obj;

	Imlib_Image image;
	// Render
	if (panel_config.mouse_effects) {
		if (launcherIcon->area.mouse_state == MOUSE_OVER)
			image = launcherIcon->image_hover ? launcherIcon->image_hover : launcherIcon->image;
		else if (launcherIcon->area.mouse_state == MOUSE_DOWN)
			image = launcherIcon->image_pressed ? launcherIcon->image_pressed : launcherIcon->image;
		else
			image = launcherIcon->image;
	} else {
		image = launcherIcon->image;
	}
	imlib_context_set_image(image);
	render_image(launcherIcon->area.pix, 0, 0);
}

void launcher_icon_dump_geometry(void *obj, int indent)
{
	LauncherIcon *launcherIcon = (LauncherIcon *)obj;
	fprintf(stderr, "%*sIcon: w = h = %d, name = %s\n", indent, "", launcherIcon->icon_size, launcherIcon->icon_name);
}

Imlib_Image scale_icon(Imlib_Image original, int icon_size)
{
	Imlib_Image icon_scaled;
	if (original) {
		imlib_context_set_image(original);
		icon_scaled = imlib_create_cropped_scaled_image(0,
		                                                0,
		                                                imlib_image_get_width(),
		                                                imlib_image_get_height(),
		                                                icon_size,
		                                                icon_size);

		imlib_context_set_image(icon_scaled);
		imlib_image_set_has_alpha(1);
		DATA32 *data = imlib_image_get_data();
		adjust_asb(data,
		           icon_size,
		           icon_size,
		           launcher_alpha / 100.0,
		           launcher_saturation / 100.0,
		           launcher_brightness / 100.0);
		imlib_image_put_back_data(data);

		imlib_context_set_image(icon_scaled);
	} else {
		icon_scaled = imlib_create_image(icon_size, icon_size);
		imlib_context_set_image(icon_scaled);
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

void launcher_action(LauncherIcon *icon, XEvent *evt)
{
	launcher_reload_icon((Launcher *)icon->area.parent, icon);
	launcher_reload_hidden_icons((Launcher *)icon->area.parent);
	char *cmd = calloc(strlen(icon->cmd) + 10, 1);
	sprintf(cmd, "(%s&)", icon->cmd);
#if HAVE_SN
	SnLauncherContext *ctx = 0;
	Time time;
	if (startup_notifications) {
		ctx = sn_launcher_context_new(server.sn_display, server.screen);
		sn_launcher_context_set_name(ctx, icon->icon_tooltip);
		sn_launcher_context_set_description(ctx, "Application launched from tint2");
		sn_launcher_context_set_binary_name(ctx, icon->cmd);
		// Get a timestamp from the X event
		if (evt->type == ButtonPress || evt->type == ButtonRelease) {
			time = evt->xbutton.time;
		} else {
			fprintf(stderr, "Unknown X event: %d\n", evt->type);
			free(cmd);
			return;
		}
		sn_launcher_context_initiate(ctx, "tint2", icon->cmd, time);
	}
#endif /* HAVE_SN */
	pid_t pid;
	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Could not fork\n");
	} else if (pid == 0) {
// Child process
#if HAVE_SN
		if (startup_notifications) {
			sn_launcher_context_setup_child_process(ctx);
		}
#endif // HAVE_SN
		// Allow children to exist after parent destruction
		setsid();
		// Run the command
		if (icon->cwd)
			chdir(icon->cwd);
		execl("/bin/sh", "/bin/sh", "-c", icon->cmd, NULL);
		fprintf(stderr, "Failed to execlp %s\n", icon->cmd);
#if HAVE_SN
		if (startup_notifications) {
			sn_launcher_context_unref(ctx);
		}
#endif // HAVE_SN
		exit(1);
	} else {
// Parent process
#if HAVE_SN
		if (startup_notifications) {
			g_tree_insert(server.pids, GINT_TO_POINTER(pid), ctx);
		}
#endif // HAVE_SN
	}
	free(cmd);
}



// Populates the list_icons list from the list_apps list
void launcher_load_icons(Launcher *launcher)
{
	// Load apps (.desktop style launcher items)
	GSList *app = launcher->list_apps;
	int index = 0;
	while (app != NULL) {
		index++;
		LauncherIcon *launcherIcon = (LauncherIcon *)calloc(1, sizeof(LauncherIcon));
		launcherIcon->area.panel = launcher->area.panel;
		launcherIcon->area._draw_foreground = draw_launcher_icon;
		launcherIcon->area.size_mode = LAYOUT_FIXED;
		launcherIcon->area._resize = NULL;
		launcherIcon->area._compute_desired_size = launcher_icon_compute_desired_size;
		sprintf(launcherIcon->area.name, "LauncherIcon %d", index);
		launcherIcon->area.resize_needed = 0;
		launcherIcon->area.has_mouse_over_effect = panel_config.mouse_effects;
		launcherIcon->area.has_mouse_press_effect = launcherIcon->area.has_mouse_over_effect;
		launcherIcon->area.bg = launcher_icon_bg;
		launcherIcon->area.on_screen = TRUE;
		launcherIcon->area.posx = -1;
		launcherIcon->area._on_change_layout = launcher_icon_on_change_layout;
		launcherIcon->area._dump_geometry = launcher_icon_dump_geometry;
		if (launcher_tooltip_enabled) {
			launcherIcon->area._get_tooltip_text = launcher_icon_get_tooltip_text;
		} else {
			launcherIcon->area._get_tooltip_text = NULL;
		}
		launcherIcon->config_path = strdup(app->data);
		add_area(&launcherIcon->area, (Area *)launcher);
		launcher->list_icons = g_slist_append(launcher->list_icons, launcherIcon);
		launcherIcon->icon_size = launcher->icon_size;
		launcher_reload_icon(launcher, launcherIcon);
		instantiate_area_gradients(&launcherIcon->area);
		app = g_slist_next(app);
	}
}

void launcher_reload_icon(Launcher *launcher, LauncherIcon *launcherIcon)
{
	DesktopEntry entry;
	if (read_desktop_file(launcherIcon->config_path, &entry) && entry.exec) {
		schedule_redraw(&launcherIcon->area);
		if (launcherIcon->cmd)
			free(launcherIcon->cmd);
		launcherIcon->cmd = strdup(entry.exec);
		if (launcherIcon->cwd) 
			free(launcherIcon->cwd);
		if (entry.cwd)
			launcherIcon->cwd = strdup(entry.cwd);
		else
			launcherIcon->cwd = NULL;
		if (launcherIcon->icon_name)
			free(launcherIcon->icon_name);
		launcherIcon->icon_name = entry.icon ? strdup(entry.icon) : strdup(DEFAULT_ICON);
		if (entry.name) {
			if (entry.generic_name) {
				launcherIcon->icon_tooltip = g_strdup_printf("%s (%s)", entry.name, entry.generic_name);
			} else {
				launcherIcon->icon_tooltip = g_strdup_printf("%s", entry.name);
			}
		} else {
			if (entry.generic_name) {
				launcherIcon->icon_tooltip = g_strdup_printf("%s", entry.generic_name);
			} else if (entry.exec) {
				launcherIcon->icon_tooltip = g_strdup_printf("%s", entry.exec);
			}
		}
		launcher_reload_icon_image(launcher, launcherIcon);
		show(&launcherIcon->area);
	} else {
		hide(&launcherIcon->area);
	}
	free_desktop_entry(&entry);
}

void launcher_reload_hidden_icons(Launcher *launcher)
{
	for (GSList *l = launcher->list_icons; l; l = l->next) {
		LauncherIcon *launcherIcon = (LauncherIcon *)l->data;
		if (!launcherIcon->area.on_screen)
			launcher_reload_icon(launcher, launcherIcon);
	}
}

void launcher_reload_icon_image(Launcher *launcher, LauncherIcon *launcherIcon)
{
	free_icon(launcherIcon->image);
	free_icon(launcherIcon->image_hover);
	free_icon(launcherIcon->image_pressed);
	launcherIcon->image = NULL;

	char *new_icon_path = get_icon_path(icon_theme_wrapper, launcherIcon->icon_name, launcherIcon->icon_size, TRUE);
	if (new_icon_path)
		launcherIcon->image = load_image(new_icon_path, TRUE);
	// On loading error, fallback to default
	if (!launcherIcon->image) {
		free(new_icon_path);
		new_icon_path = get_icon_path(icon_theme_wrapper, DEFAULT_ICON, launcherIcon->icon_size, TRUE);
		if (new_icon_path)
			launcherIcon->image = load_image(new_icon_path, TRUE);
	}
	Imlib_Image original = launcherIcon->image;
	launcherIcon->image = scale_icon(launcherIcon->image, launcherIcon->icon_size);
	free_icon(original);
	free(launcherIcon->icon_path);
	launcherIcon->icon_path = new_icon_path;
	// fprintf(stderr, "launcher.c %d: Using icon %s\n", __LINE__, launcherIcon->icon_path);

	if (panel_config.mouse_effects) {
		launcherIcon->image_hover = adjust_icon(launcherIcon->image,
		                                        panel_config.mouse_over_alpha,
		                                        panel_config.mouse_over_saturation,
		                                        panel_config.mouse_over_brightness);
		launcherIcon->image_pressed = adjust_icon(launcherIcon->image,
		                                          panel_config.mouse_pressed_alpha,
		                                          panel_config.mouse_pressed_saturation,
		                                          panel_config.mouse_pressed_brightness);
	}
	schedule_redraw(&launcherIcon->area);
}

void load_icon_themes()
{
	if (icon_theme_wrapper)
		return;
	icon_theme_wrapper =
	    load_themes(launcher_icon_theme_override
	                    ? (icon_theme_name_config ? icon_theme_name_config
	                                              : icon_theme_name_xsettings ? icon_theme_name_xsettings : "hicolor")
	                    : (icon_theme_name_xsettings ? icon_theme_name_xsettings
	                                                 : icon_theme_name_config ? icon_theme_name_config : "hicolor"));
}

void launcher_default_icon_theme_changed()
{
	for (int i = 0; i < num_panels; i++) {
		Launcher *launcher = &panels[i].launcher;
		cleanup_launcher_theme(launcher);
		launcher_load_icons(launcher);
		launcher->area.resize_needed = 1;
	}
	schedule_panel_redraw();
}
