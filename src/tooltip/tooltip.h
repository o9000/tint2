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

#include <sys/time.h>

#include "task.h"

enum tooltip_state {
	TOOLTIP_ABOUT_TO_SHOW,
	TOOLTIP_ABOUT_TO_HIDE,
};

typedef struct {
	Task* task;
	Window window;
	struct itimerval show_timeout;
	struct itimerval hide_timeout;
	Bool enabled;
	enum tooltip_state current_state;
	Bool mapped;
	int paddingx;
	int paddingy;
	PangoFontDescription* font_desc;
	config_color font_color;
	Color background_color;
	Border border;
} Tooltip;

extern Tooltip g_tooltip;


void init_tooltip();
void tooltip_sighandler(int sig);
void tooltip_trigger_show(Task* task, int x, int y);
void tooltip_show();
void tooltip_update();
void tooltip_trigger_hide();
void tooltip_hide();

#endif // TOOLTIP_H
