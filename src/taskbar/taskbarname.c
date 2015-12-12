/**************************************************************************
*
* Tint2 : taskbarname
*
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <Imlib2.h>

#include "window.h"
#include "panel.h"
#include "taskbar.h"
#include "server.h"
#include "taskbarname.h"

int taskbarname_enabled;
Color taskbarname_font;
Color taskbarname_active_font;

void taskbarname_init_fonts();

void default_taskbarname()
{
	taskbarname_enabled = 0;
}

void init_taskbarname_panel(void *p)
{
	Panel *panel = (Panel *)p;
	Taskbar *taskbar;
	int j;

	if (!taskbarname_enabled)
		return;

	taskbarname_init_fonts();

	GSList *l, *list = get_desktop_names();
	for (j = 0, l = list; j < panel->num_desktops; j++) {
		taskbar = &panel->taskbar[j];
		memcpy(&taskbar->bar_name.area, &panel->g_taskbar.area_name, sizeof(Area));
		taskbar->bar_name.area.parent = taskbar;
		taskbar->bar_name.area.has_mouse_over_effect = 1;
		taskbar->bar_name.area.has_mouse_press_effect = 1;
		if (j == server.desktop)
			taskbar->bar_name.area.bg = panel->g_taskbar.background_name[TASKBAR_ACTIVE];
		else
			taskbar->bar_name.area.bg = panel->g_taskbar.background_name[TASKBAR_NORMAL];

		// use desktop number if name is missing
		if (l) {
			taskbar->bar_name.name = g_strdup(l->data);
			l = l->next;
		} else
			taskbar->bar_name.name = g_strdup_printf("%d", j + 1);

		// append the name at the beginning of taskbar
		taskbar->area.children = g_list_append(taskbar->area.children, &taskbar->bar_name);
	}

	for (l = list; l; l = l->next)
		g_free(l->data);
	g_slist_free(list);
}

void taskbarname_init_fonts()
{
	if (!panel_config.taskbarname_font_desc) {
		panel_config.taskbarname_font_desc = pango_font_description_from_string(get_default_font());
		pango_font_description_set_weight(panel_config.taskbarname_font_desc, PANGO_WEIGHT_BOLD);
	}
}

void taskbarname_default_font_changed()
{
	if (!taskbar_enabled)
		return;
	if (!taskbarname_enabled)
		return;
	if (panel_config.taskbarname_has_font)
		return;

	pango_font_description_free(panel_config.taskbarname_font_desc);
	panel_config.taskbarname_font_desc = NULL;
	taskbarname_init_fonts();
	for (int i = 0; i < num_panels; i++) {
		for (int j = 0; j < panels[i].num_desktops; j++) {
			Taskbar *taskbar = &panels[i].taskbar[j];
			taskbar->bar_name.area.resize_needed = TRUE;
			taskbar->bar_name.area.redraw_needed = TRUE;
		}
	}
	panel_refresh = TRUE;
}

void cleanup_taskbarname()
{
	int i, j, k;
	Panel *panel;
	Taskbar *taskbar;

	for (i = 0; i < num_panels; i++) {
		panel = &panels[i];
		for (j = 0; j < panel->num_desktops; j++) {
			taskbar = &panel->taskbar[j];
			g_free(taskbar->bar_name.name);
			taskbar->bar_name.name = NULL;
			free_area(&taskbar->bar_name.area);
			for (k = 0; k < TASKBAR_STATE_COUNT; ++k) {
				if (taskbar->bar_name.state_pix[k])
					XFreePixmap(server.dsp, taskbar->bar_name.state_pix[k]);
				taskbar->bar_name.state_pix[k] = 0;
			}
			remove_area((Area *)&taskbar->bar_name);
		}
	}
}

void draw_taskbarname(void *obj, cairo_t *c)
{
	Taskbarname *taskbar_name = obj;
	Taskbar *taskbar = taskbar_name->area.parent;
	PangoLayout *layout;
	Color *config_text = (taskbar->desktop == server.desktop) ? &taskbarname_active_font : &taskbarname_font;

	int state = (taskbar->desktop == server.desktop) ? TASKBAR_ACTIVE : TASKBAR_NORMAL;
	if (!panel_config.mouse_effects)
		taskbar_name->state_pix[state] = taskbar_name->area.pix;

	// draw content
	layout = pango_cairo_create_layout(c);
	pango_layout_set_font_description(layout, panel_config.taskbarname_font_desc);
	pango_layout_set_width(layout, taskbar_name->area.width * PANGO_SCALE);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
	pango_layout_set_text(layout, taskbar_name->name, strlen(taskbar_name->name));

	cairo_set_source_rgba(c, config_text->rgb[0], config_text->rgb[1], config_text->rgb[2], config_text->alpha);

	pango_cairo_update_layout(c, layout);
	draw_text(layout, c, 0, taskbar_name->posy, config_text, ((Panel *)taskbar_name->area.panel)->font_shadow);

	g_object_unref(layout);
	// printf("draw_taskbarname %s ******************************\n", taskbar_name->name);
}

gboolean resize_taskbarname(void *obj)
{
	Taskbarname *taskbar_name = obj;
	Panel *panel = taskbar_name->area.panel;
	int name_height, name_width, name_height_ink;
	int ret = 0;

	taskbar_name->area.redraw_needed = TRUE;
	get_text_size2(panel_config.taskbarname_font_desc,
				   &name_height_ink,
				   &name_height,
				   &name_width,
				   panel->area.height,
				   panel->area.width,
				   taskbar_name->name,
				   strlen(taskbar_name->name),
				   PANGO_WRAP_WORD_CHAR,
				   PANGO_ELLIPSIZE_NONE,
				   FALSE);

	if (panel_horizontal) {
		int new_size = name_width + (2 * (taskbar_name->area.paddingxlr + taskbar_name->area.bg->border.width));
		if (new_size != taskbar_name->area.width) {
			taskbar_name->area.width = new_size;
			taskbar_name->posy = (taskbar_name->area.height - name_height) / 2;
			ret = 1;
		}
	} else {
		int new_size = name_height + (2 * (taskbar_name->area.paddingxlr + taskbar_name->area.bg->border.width));
		if (new_size != taskbar_name->area.height) {
			taskbar_name->area.height = new_size;
			taskbar_name->posy = (taskbar_name->area.height - name_height) / 2;
			ret = 1;
		}
	}
	return ret;
}
