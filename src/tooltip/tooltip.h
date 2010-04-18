/**************************************************************************
*
* Copyright (C) 2009 Andreas.Fink (Andreas.Fink85@gmail.com)
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

#ifndef TOOLTIP_H
#define TOOLTIP_H

#include "task.h"
#include "panel.h"
#include "timer.h"


typedef struct {
	Area* area;    // never ever use the area attribut if you are not 100% sure that this area was not freed
	char* tooltip_text;
	Panel* panel;
	Window window;
	int show_timeout_msec;
	int hide_timeout_msec;
	Bool enabled;
	Bool mapped;
	int paddingx;
	int paddingy;
	PangoFontDescription* font_desc;
	Color font_color;
	Background* bg;
	timeout* timeout;
} Tooltip;

extern Tooltip g_tooltip;


// default global data
void default_tooltip();

// freed memory
void cleanup_tooltip();

void init_tooltip();
void tooltip_trigger_show(Area* area, Panel* p, int x, int y);
void tooltip_show(void* /*arg*/);
void tooltip_update();
void tooltip_trigger_hide();
void tooltip_hide(void* /*arg*/);
void tooltip_copy_text(Area* area);

#endif // TOOLTIP_H
