/**************************************************************************
*
* Tint2 : taskbar
*
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
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

#include "task.h"
#include "taskbar.h"
#include "server.h"
#include "window.h"
#include "panel.h"



void init_taskbar()
{
	Panel *panel;
	int i, j;

	for (i=0 ; i < nb_panel ; i++) {
		panel = &panel1[i];

		if (panel->taskbar) {
			free(panel->taskbar);
			panel->taskbar = 0;
		}

		// taskbar
		panel->g_taskbar.area._resize = resize_taskbar;
		panel->g_taskbar.area.redraw = 1;
		panel->g_taskbar.area.on_screen = 1;
		if (panel_horizontal) {
			panel->g_taskbar.area.posy = panel->area.bg->border.width + panel->area.paddingy;
			panel->g_taskbar.area.height = panel->area.height - (2 * panel->g_taskbar.area.posy);
		}
		else {
			panel->g_taskbar.area.posx = panel->area.bg->border.width + panel->area.paddingy;
			panel->g_taskbar.area.width = panel->area.width - (2 * panel->g_taskbar.area.posx);
		}

		// task
		panel->g_task.area._draw_foreground = draw_task;
		panel->g_task.area.redraw = 1;
		panel->g_task.area.on_screen = 1;
		if ((panel->g_task.config_asb_mask & (1<<TASK_ACTIVE)) == 0) {
			panel->g_task.alpha[TASK_ACTIVE] = panel->g_task.alpha[TASK_NORMAL];
			panel->g_task.saturation[TASK_ACTIVE] = panel->g_task.saturation[TASK_NORMAL];
			panel->g_task.brightness[TASK_ACTIVE] = panel->g_task.brightness[TASK_NORMAL];
		}
		if ((panel->g_task.config_asb_mask & (1<<TASK_ICONIFIED)) == 0) {
			panel->g_task.alpha[TASK_ICONIFIED] = panel->g_task.alpha[TASK_NORMAL];
			panel->g_task.saturation[TASK_ICONIFIED] = panel->g_task.saturation[TASK_NORMAL];
			panel->g_task.brightness[TASK_ICONIFIED] = panel->g_task.brightness[TASK_NORMAL];
		}
		if ((panel->g_task.config_asb_mask & (1<<TASK_URGENT)) == 0) {
			panel->g_task.alpha[TASK_URGENT] = panel->g_task.alpha[TASK_ACTIVE];
			panel->g_task.saturation[TASK_URGENT] = panel->g_task.saturation[TASK_ACTIVE];
			panel->g_task.brightness[TASK_URGENT] = panel->g_task.brightness[TASK_ACTIVE];
		}
		if ((panel->g_task.config_font_mask & (1<<TASK_ACTIVE)) == 0) panel->g_task.font[TASK_ACTIVE] = panel->g_task.font[TASK_NORMAL];
		if ((panel->g_task.config_font_mask & (1<<TASK_ICONIFIED)) == 0) panel->g_task.font[TASK_ICONIFIED] = panel->g_task.font[TASK_NORMAL];
		if ((panel->g_task.config_font_mask & (1<<TASK_URGENT)) == 0) panel->g_task.font[TASK_URGENT] = panel->g_task.font[TASK_ACTIVE];
		if ((panel->g_task.config_background_mask & (1<<TASK_ACTIVE)) == 0) panel->g_task.background[TASK_ACTIVE] = panel->g_task.background[TASK_NORMAL];
		if ((panel->g_task.config_background_mask & (1<<TASK_ICONIFIED)) == 0) panel->g_task.background[TASK_ICONIFIED] = panel->g_task.background[TASK_NORMAL];
		if ((panel->g_task.config_background_mask & (1<<TASK_URGENT)) == 0) panel->g_task.background[TASK_URGENT] = panel->g_task.background[TASK_ACTIVE];

		if (panel_horizontal) {
			panel->g_task.area.posy = panel->g_taskbar.area.posy + panel->g_taskbar.bg->border.width + panel->g_taskbar.area.paddingy;
			panel->g_task.area.height = panel->area.height - (2 * panel->g_task.area.posy);
		}
		else {
			panel->g_task.area.posx = panel->g_taskbar.area.posx + panel->g_taskbar.bg->border.width + panel->g_taskbar.area.paddingy;
			panel->g_task.area.width = panel->area.width - (2 * panel->g_task.area.posx);
			panel->g_task.area.height = panel->g_task.maximum_height;
		}

		int k;
		for (k=0; k<TASK_STATE_COUNT; ++k) {
			if (panel->g_task.background[k]->border.rounded > panel->g_task.area.height/2) {
				printf("task%sbackground_id is too big. Please fix your tint2rc\n", k==0 ? "_" : k==1 ? "_active_" : k==2 ? "_iconified_" : "_urgent_");
				g_array_append_val(backgrounds, *panel->g_task.background[k]);
				panel->g_task.background[k] = &g_array_index(backgrounds, Background, backgrounds->len-1);
				panel->g_task.background[k]->border.rounded = panel->g_task.area.height/2;
			}
		}

		// compute vertical position : text and icon
		int height_ink, height;
		get_text_size(panel->g_task.font_desc, &height_ink, &height, panel->area.height, "TAjpg", 5);

		if (!panel->g_task.maximum_width && panel_horizontal)
			panel->g_task.maximum_width = server.monitor[panel->monitor].width;

		panel->g_task.text_posx = panel->g_task.background[0]->border.width + panel->g_task.area.paddingxlr;
		panel->g_task.text_posy = (panel->g_task.area.height - height) / 2.0;
		if (panel->g_task.icon) {
			panel->g_task.icon_size1 = panel->g_task.area.height - (2 * panel->g_task.area.paddingy);
			panel->g_task.text_posx += panel->g_task.icon_size1;
			panel->g_task.icon_posy = (panel->g_task.area.height - panel->g_task.icon_size1) / 2;
		}
		//printf("monitor %d, task_maximum_width %d\n", panel->monitor, panel->g_task.maximum_width);

		Taskbar *tskbar;
		panel->nb_desktop = server.nb_desktop;
		panel->taskbar = calloc(panel->nb_desktop, sizeof(Taskbar));
		for (j=0 ; j < panel->nb_desktop ; j++) {
			tskbar = &panel->taskbar[j];
			memcpy(&tskbar->area, &panel->g_taskbar, sizeof(Area));
			tskbar->desktop = j;
			if (j == server.desktop && panel->g_taskbar.use_active)
				tskbar->area.bg = panel->g_taskbar.bg_active;

			// add taskbar to the panel
			panel->area.list = g_slist_append(panel->area.list, tskbar);
		}
	}
}


void cleanup_taskbar()
{
	Panel *panel;
	Taskbar *tskbar;
	int i, j;
	GSList *l0;
	Task *tsk;

	for (i=0 ; i < nb_panel ; i++) {
		panel = &panel1[i];

		for (j=0 ; j < panel->nb_desktop ; j++) {
			tskbar = &panel->taskbar[j];
			l0 = tskbar->area.list;
			while (l0) {
				tsk = l0->data;
				l0 = l0->next;
				// careful : remove_task change l0->next
				remove_task (tsk);
			}
			free_area (&tskbar->area);

			// remove taskbar from the panel
			panel->area.list = g_slist_remove(panel->area.list, tskbar);
		}
	}

	for (i=0 ; i < nb_panel ; i++) {
		panel = &panel1[i];
		if (panel->taskbar) {
			free(panel->taskbar);
			panel->taskbar = 0;
		}
	}
}


Task *task_get_task (Window win)
{
	Task *tsk;
	GSList *l0;
	int i, j;

	for (i=0 ; i < nb_panel ; i++) {
		for (j=0 ; j < panel1[i].nb_desktop ; j++) {
			for (l0 = panel1[i].taskbar[j].area.list; l0 ; l0 = l0->next) {
				tsk = l0->data;
				if (win == tsk->win)
					return tsk;
			}
		}
	}
	return 0;
}


GSList* task_get_tasks(Window win)
{
	GSList* l = 0;
	GSList* it;
	Task* tsk;
	int i, j;
	for (i=0; i<nb_panel; ++i) {
		for (j=0; j<panel1[i].nb_desktop; ++j) {
			for (it=panel1[i].taskbar[j].area.list; it; it=it->next) {
				tsk = it->data;
				if (win == tsk->win)
					l = g_slist_prepend(l, tsk);
			}
		}
	}
	return l;
}


void task_refresh_tasklist ()
{
	Window *win;
	int num_results, i, j, k;
	GSList *l0;
	Task *tsk;

	win = server_get_property (server.root_win, server.atom._NET_CLIENT_LIST, XA_WINDOW, &num_results);
	if (!win) return;

	// Remove any old and set active win
	active_task();

	for (i=0 ; i < nb_panel ; i++) {
		for (j=0 ; j < panel1[i].nb_desktop ; j++) {
			l0 = panel1[i].taskbar[j].area.list;
			while (l0) {
				tsk = l0->data;
				l0 = l0->next;

				for (k = 0; k < num_results; k++) {
					if (tsk->win == win[k]) break;
				}
				// careful : remove_task change l0->next
				if (k == num_results) remove_task (tsk);
			}
		}
	}

	// Add any new
	for (i = 0; i < num_results; i++)
		if (!task_get_task (win[i]))
			add_task (win[i]);

	XFree (win);
}


void resize_taskbar(void *obj)
{
	Taskbar *taskbar = (Taskbar*)obj;
	Panel *panel = (Panel*)taskbar->area.panel;
	Task *tsk;
	GSList *l;
	int  task_count, border_width;

	//printf("resize_taskbar : posx et width des taches\n");
	taskbar->area.redraw = 1;

	border_width = taskbar->area.bg->border.width;

	if (panel_horizontal) {
		int  pixel_width, modulo_width=0;
		int  x, taskbar_width;

		// new task width for 'desktop'
		task_count = g_slist_length(taskbar->area.list);
		if (!task_count) pixel_width = panel->g_task.maximum_width;
		else {
			taskbar_width = taskbar->area.width - (2 * border_width) - (2 * panel->g_taskbar.area.paddingxlr);
			if (task_count>1) taskbar_width -= ((task_count-1) * panel->g_taskbar.area.paddingx);

			pixel_width = taskbar_width / task_count;
			if (pixel_width > panel->g_task.maximum_width)
				pixel_width = panel->g_task.maximum_width;
			else
				modulo_width = taskbar_width % task_count;
		}

		taskbar->task_width = pixel_width;
		taskbar->task_modulo = modulo_width;
		taskbar->text_width = pixel_width - panel->g_task.text_posx - panel->g_task.area.bg->border.width - panel->g_task.area.paddingx;

		// change pos_x and width for all tasks
		x = taskbar->area.posx + border_width + taskbar->area.paddingxlr;
		for (l = taskbar->area.list; l ; l = l->next) {
			tsk = l->data;
			if (!tsk->area.on_screen) continue;
			tsk->area.posx = x;
			tsk->area.width = pixel_width;
			if (modulo_width) {
				tsk->area.width++;
				modulo_width--;
			}

			x += tsk->area.width + panel->g_taskbar.area.paddingx;
		}
	}
	else {
		int  pixel_height, modulo_height=0;
		int  y, taskbar_height;

		// new task width for 'desktop'
		task_count = g_slist_length(taskbar->area.list);
		if (!task_count) pixel_height = panel->g_task.maximum_height;
		else {
			taskbar_height = taskbar->area.height - (2 * border_width) - (2 * panel->g_taskbar.area.paddingxlr);
			if (task_count>1) taskbar_height -= ((task_count-1) * panel->g_taskbar.area.paddingx);

			pixel_height = taskbar_height / task_count;
			if (pixel_height > panel->g_task.maximum_height)
				pixel_height = panel->g_task.maximum_height;
			else
				modulo_height = taskbar_height % task_count;
		}

		taskbar->task_width = pixel_height;
		taskbar->task_modulo = modulo_height;
		taskbar->text_width = taskbar->area.width - (2 * panel->g_taskbar.area.paddingy) - panel->g_task.text_posx - panel->g_task.area.bg->border.width - panel->g_task.area.paddingx;

		// change pos_y and height for all tasks
		y = taskbar->area.posy + border_width + taskbar->area.paddingxlr;
		for (l = taskbar->area.list; l ; l = l->next) {
			tsk = l->data;
			if (!tsk->area.on_screen) continue;
			tsk->area.posy = y;
			tsk->area.height = pixel_height;
			if (modulo_height) {
				tsk->area.height++;
				modulo_height--;
			}

			y += tsk->area.height + panel->g_taskbar.area.paddingx;
		}
	}
}



