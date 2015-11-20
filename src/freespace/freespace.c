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
	FreeSpace *freespace = &panel->freespace;

	if (!freespace->area.bg)
		freespace->area.bg = &g_array_index(backgrounds, Background, 0);
	freespace->area.parent = p;
	freespace->area.panel = p;
	freespace->area.size_mode = LAYOUT_FIXED;
	freespace->area.resize_needed = 1;
	freespace->area.on_screen = TRUE;
	freespace->area._resize = resize_freespace;
}

int freespace_get_max_size(Panel *p)
{
	// Get space used by every element except the freespace
	GList *walk;
	int size = 0;
	for (walk = p->area.children; walk; walk = g_list_next(walk)) {
		Area *a = (Area *)walk->data;

		if (a->_resize == resize_freespace || !a->on_screen)
			continue;

		if (panel_horizontal)
			size += a->width + (a->bg->border.width * 2) + p->area.paddingx;
		else
			size += a->height + (a->bg->border.width * 2) + p->area.paddingy;
	}

	if (panel_horizontal)
		size = p->area.width - size - (p->area.bg->border.width * 2) - p->area.paddingxlr;
	else
		size = p->area.height - size - (p->area.bg->border.width * 2) - p->area.paddingxlr;

	return size;
}

gboolean resize_freespace(void *obj)
{
	FreeSpace *freespace = (FreeSpace *)obj;
	Panel *panel = (Panel *)freespace->area.panel;
	if (!freespace->area.on_screen)
		return 0;

	int old_size = panel_horizontal ? freespace->area.width : freespace->area.height;
	int size = freespace_get_max_size(panel);
	if (old_size == size)
		return 0;

	if (panel_horizontal) {
		freespace->area.width = size;
	} else {
		freespace->area.height = size;
	}

	freespace->area.redraw_needed = TRUE;
	panel_refresh = TRUE;
	return 1;
}
