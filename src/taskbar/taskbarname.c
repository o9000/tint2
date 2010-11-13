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


void default_taskbarname()
{
	taskbarname_enabled = 0;
	taskbarname_font_desc = 0;
}


void init_taskbarname_panel(void *p)
{
	Panel *panel =(Panel*)p;
	int j;
	
	if (!taskbarname_enabled || !taskbar_enabled) return;
	
	for (j=0 ; j < panel->nb_desktop ; j++) {
		panel->taskbar[j].bar_name.name = g_strdup_printf("%d", j+1);
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
			if (taskbarname_font_desc)	pango_font_description_free(taskbarname_font_desc);
			if (tskbar->bar_name.name)	g_free(tskbar->bar_name.name);
			free_area (&tskbar->bar_name.area);
			for (k=0; k<TASKBAR_STATE_COUNT; ++k) {
				if (tskbar->bar_name.state_pix[k]) XFreePixmap(server.dsp, tskbar->bar_name.state_pix[k]);
			}
		}
	}
}


void draw_taskbarname (void *obj, cairo_t *c)
{
	Taskbarname *taskbar_name = obj;
	PangoLayout *layout;

	layout = pango_cairo_create_layout (c);

	// draw layout
	pango_layout_set_font_description (layout, taskbarname_font_desc);
	pango_layout_set_width (layout, taskbar_name->area.width * PANGO_SCALE);
	pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
	pango_layout_set_text (layout, taskbar_name->name, strlen(taskbar_name->name));

	cairo_set_source_rgba (c, taskbarname_font.color[0], taskbarname_font.color[1], taskbarname_font.color[2], taskbarname_font.alpha);

	pango_cairo_update_layout (c, layout);
	cairo_move_to (c, 0, 2);
	pango_cairo_show_layout (c, layout);

	g_object_unref (layout);
	printf("draw_taskbarname %s ******************************\n", taskbar_name->name);
}


int resize_taskbarname(void *obj)
{
	Taskbarname *taskbar_name = (Taskbar*)obj;
	Panel *panel = taskbar_name->area.panel;
	int time_height, time_width, ret = 0;

	taskbar_name->area.redraw = 1;
	/*
	strftime(buf_time, sizeof(buf_time), time1_format, clock_gettime_for_tz(time1_timezone));
	get_text_size2(time1_font_desc, &time_height_ink, &time_height, &time_width, panel->area.height, panel->area.width, buf_time, strlen(buf_time));
	if (time2_format) {
		strftime(buf_date, sizeof(buf_date), time2_format, clock_gettime_for_tz(time2_timezone));
		get_text_size2(time2_font_desc, &date_height_ink, &date_height, &date_width, panel->area.height, panel->area.width, buf_date, strlen(buf_date));
	}

	if (panel_horizontal) {
		int new_size = (time_width > date_width) ? time_width : date_width;
		new_size += (2*clock->area.paddingxlr) + (2*clock->area.bg->border.width);
		if (new_size > clock->area.width || new_size < (clock->area.width-6)) {
			// we try to limit the number of resize
			clock->area.width = new_size + 1;
			clock->time1_posy = (clock->area.height - time_height) / 2;
			if (time2_format) {
				clock->time1_posy -= ((date_height_ink + 2) / 2);
				clock->time2_posy = clock->time1_posy + time_height + 2 - (time_height - time_height_ink)/2 - (date_height - date_height_ink)/2;
			}
			ret = 1;
		}
	}
	else {
		int new_size = time_height + date_height + (2 * (clock->area.paddingxlr + clock->area.bg->border.width));
		if (new_size != clock->area.height) {
			// we try to limit the number of resize
			clock->area.height =  new_size;
			clock->time1_posy = (clock->area.height - time_height) / 2;
			if (time2_format) {
				clock->time1_posy -= ((date_height_ink + 2) / 2);
				clock->time2_posy = clock->time1_posy + time_height + 2 - (time_height - time_height_ink)/2 - (date_height - date_height_ink)/2;
			}
			ret = 1;
		}
	}
	return ret;
*/

	taskbar_name->area.width = 10;
	return 1;
}




