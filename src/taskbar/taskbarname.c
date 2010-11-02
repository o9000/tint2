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

#include "taskbarname.h"
#include "window.h"
#include "panel.h"


int taskbarname_enabled;


void default_taskbarname()
{
	taskbarname_enabled = 0;
}

void cleanup_taskbarname()
{
	Panel *panel;
	Taskbar *tskbar;
	int i, j;

	for (i=0 ; i < nb_panel ; i++) {
		panel = &panel1[i];
	}
}


void init_taskbarname()
{
}


void init_taskbarname_panel(void *p)
{
	Panel *panel =(Panel*)p;
	
	if (!taskbarname_enabled) return;

}


void draw_taskbarname (void *obj, cairo_t *c)
{

}


int resize_taskbarname(void *obj)
{
	Taskbar *taskbar = (Taskbar*)obj;
	Panel *panel = (Panel*)taskbar->area.panel;

	return 0;
}




