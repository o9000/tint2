/**************************************************************************
*
* Tint2 : freespace
*
* Copyright (C) 2011 Mishael A Sibiryakov (death@junki.org)
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
#include <stdlib.h>

#include "window.h"
#include "server.h"
#include "panel.h"
#include "freespace.h"
#include "common.h"

void init_freespace_panel(void *p)
{
	Panel *panel = (Panel *)p;

	// Make sure this is only done once if there are multiple items
	if (panel->freespace_list)
		return;

	for (size_t k = 0; k < strlen(panel_items_order); k++) {
		if (panel_items_order[k] == 'F') {
			FreeSpace *freespace = (FreeSpace *) calloc(1, sizeof(FreeSpace));
			panel->freespace_list = g_list_append(panel->freespace_list, freespace);
			if (!freespace->area.bg)
				freespace->area.bg = &g_array_index(backgrounds, Background, 0);
			freespace->area.parent = p;
			freespace->area.panel = p;
			snprintf(freespace->area.name, sizeof(freespace->area.name), "Freespace");
			freespace->area.size_mode = LAYOUT_FIXED;
			freespace->area.resize_needed = 1;
			freespace->area.on_screen = TRUE;
			freespace->area._resize = resize_freespace;
		}
	}
}

void cleanup_freespace(Panel *panel)
{
	if (panel->freespace_list)
		g_list_free_full(panel->freespace_list, free);
	panel->freespace_list = NULL;
}

int freespace_get_max_size(Panel *p)
{
	// Get space used by every element except the freespace
	int size = 0;
	int spacers = 0;
	for (GList *walk = p->area.children; walk; walk = g_list_next(walk)) {
		Area *a = (Area *)walk->data;

		if (!a->on_screen)
			continue;
		if (a->_resize == resize_freespace) {
			spacers++;
			continue;
		}

		if (panel_horizontal)
			size += a->width + p->area.paddingx;
		else
			size += a->height + p->area.paddingy;
	}

	if (panel_horizontal)
		size = p->area.width - size - left_right_border_width(&p->area) - p->area.paddingxlr;
	else
		size = p->area.height - size - top_bottom_border_width(&p->area) - p->area.paddingxlr;

	return size / spacers;
}

gboolean resize_freespace(void *obj)
{
	FreeSpace *freespace = (FreeSpace *)obj;
	Panel *panel = (Panel *)freespace->area.panel;
	if (!freespace->area.on_screen)
		return FALSE;

	int old_size = panel_horizontal ? freespace->area.width : freespace->area.height;
	int size = freespace_get_max_size(panel);
	if (old_size == size)
		return FALSE;

	if (panel_horizontal) {
		freespace->area.width = size;
	} else {
		freespace->area.height = size;
	}

	schedule_redraw(&freespace->area);
	panel_refresh = TRUE;
	return TRUE;
}
