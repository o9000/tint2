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
PangoFontDescription *taskbarname_font_desc;
Color taskbarname_font;
Color taskbarname_active_font;


void default_taskbarname()
{
	taskbarname_enabled = 0;
	taskbarname_font_desc = 0;
}


void init_taskbarname_panel(void *p)
{
	Panel *panel =(Panel*)p;
	Taskbar *tskbar;
	int j;
	
	if (!taskbarname_enabled) return;
	
	for (j=0 ; j < panel->nb_desktop ; j++) {
		tskbar = &panel->taskbar[j];
		memcpy(&tskbar->bar_name.area, &panel->g_taskbar.area_name, sizeof(Area));
		tskbar->bar_name.area.parent = tskbar;
		if (j == server.desktop)
			tskbar->bar_name.area.bg = panel->g_taskbar.background_name[TASKBAR_ACTIVE];
		else
			tskbar->bar_name.area.bg = panel->g_taskbar.background_name[TASKBAR_NORMAL];
		tskbar->bar_name.name = g_strdup_printf("%d", j+1);
		
		// append the name at the beginning of taskbar
		tskbar->area.list = g_slist_append(tskbar->area.list, &tskbar->bar_name);
	}
}


void cleanup_taskbarname()
{
	int i, j, k;
	Panel *panel;
	Taskbar *tskbar;

	for (i=0 ; i < nb_panel ; i++) {
		panel = &panel1[i];
		for (j=0 ; j < panel->nb_desktop ; j++) {
			tskbar = &panel->taskbar[j];
			if (tskbar->bar_name.name)	g_free(tskbar->bar_name.name);
			free_area (&tskbar->bar_name.area);
			for (k=0; k<TASKBAR_STATE_COUNT; ++k) {
				if (tskbar->bar_name.state_pix[k]) XFreePixmap(server.dsp, tskbar->bar_name.state_pix[k]);
			}
			tskbar->area.list = g_slist_remove(tskbar->area.list, &tskbar->bar_name);
		}
	}
}


void draw_taskbarname (void *obj, cairo_t *c)
{
	Taskbarname *taskbar_name = obj;
	Taskbar *taskbar = taskbar_name->area.parent;
	PangoLayout *layout;
	Color *config_text = (taskbar->desktop == server.desktop) ? &taskbarname_active_font : &taskbarname_font;

	int state = (taskbar->desktop == server.desktop) ? TASKBAR_ACTIVE : TASKBAR_NORMAL;
	taskbar_name->state_pix[state] = taskbar_name->area.pix;
	
	// draw content
	layout = pango_cairo_create_layout (c);
	pango_layout_set_font_description (layout, taskbarname_font_desc);
	pango_layout_set_width (layout, taskbar_name->area.width * PANGO_SCALE);
	pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
	pango_layout_set_text (layout, taskbar_name->name, strlen(taskbar_name->name));

	cairo_set_source_rgba (c, config_text->color[0], config_text->color[1], config_text->color[2], config_text->alpha);

	pango_cairo_update_layout (c, layout);
	cairo_move_to (c, 0, taskbar_name->posy);
	pango_cairo_show_layout (c, layout);

	g_object_unref (layout);
	//printf("draw_taskbarname %s ******************************\n", taskbar_name->name);
}


int resize_taskbarname(void *obj)
{
	Taskbarname *taskbar_name = obj;
	Panel *panel = taskbar_name->area.panel;
	int name_height, name_width, name_height_ink;
	int ret = 0;

	taskbar_name->area.redraw = 1;
	get_text_size2(taskbarname_font_desc, &name_height_ink, &name_height, &name_width, panel->area.height, panel->area.width, taskbar_name->name, strlen(taskbar_name->name));

	if (panel_horizontal) {
		int new_size = name_width + (2* (taskbar_name->area.paddingxlr + taskbar_name->area.bg->border.width));
		if (new_size != taskbar_name->area.width) {
			taskbar_name->area.width = new_size;
			taskbar_name->posy = (taskbar_name->area.height - name_height) / 2;
			ret = 1;
		}
	}
	else {
		int new_size = name_height + (2 * (taskbar_name->area.paddingxlr + taskbar_name->area.bg->border.width));
		if (new_size != taskbar_name->area.height) {
			taskbar_name->area.height =  new_size;
			taskbar_name->posy = (taskbar_name->area.height - name_height) / 2;
			ret = 1;
		}
	}
	return ret;
}




