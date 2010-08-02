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

