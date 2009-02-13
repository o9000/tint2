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
		panel->g_taskbar._resize = resize_taskbar;
		panel->g_taskbar.posy = panel->area.pix.border.width + panel->area.paddingy;
		panel->g_taskbar.height = panel->area.height - (2 * panel->g_taskbar.posy);
		panel->g_taskbar.redraw = 1;
		panel->g_taskbar.visible = 1;

		// task
		panel->g_task.area._draw_foreground = draw_foreground_task;
		panel->g_task.area.posy = panel->g_taskbar.posy + panel->g_taskbar.pix.border.width + panel->g_taskbar.paddingy;
		panel->g_task.area.height = panel->area.height - (2 * panel->g_task.area.posy);
		panel->g_task.area.use_active = 1;
		panel->g_task.area.redraw = 1;
		panel->g_task.area.visible = 1;

		if (panel->g_task.area.pix.border.rounded > panel->g_task.area.height/2) {
			panel->g_task.area.pix.border.rounded = panel->g_task.area.height/2;
			panel->g_task.area.pix_active.border.rounded = panel->g_task.area.pix.border.rounded;
		}

		// compute vertical position : text and icon
		int height_ink, height;
		get_text_size(panel->g_task.font_desc, &height_ink, &height, panel->area.height, "TAjpg", 5);

		if (!panel->g_task.maximum_width)
			panel->g_task.maximum_width = server.monitor[panel->monitor].width;

		// add task_icon_size
		panel->g_task.text_posx = panel->g_task.area.paddingxlr + panel->g_task.area.pix.border.width;
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


void task_refresh_tasklist ()
{
   Window *win, active_win;
   int num_results, i, j, k;
   GSList *l0;
   Task *tsk;

   win = server_get_property (server.root_win, server.atom._NET_CLIENT_LIST, XA_WINDOW, &num_results);

   if (!win) return;

   // Remove any old and set active win
   active_win = window_get_active ();
	if (task_active) {
		task_active->area.is_active = 0;
		task_active = 0;
	}

	for (i=0 ; i < nb_panel ; i++) {
		for (j=0 ; j < panel1[i].nb_desktop ; j++) {
			l0 = panel1[i].taskbar[j].area.list;
			while (l0) {
				tsk = l0->data;
				l0 = l0->next;

				if (tsk->win == active_win) {
					tsk->area.is_active = 1;
					task_active = tsk;
				}

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
   int  task_count, pixel_width, modulo_width=0;
   int  x, taskbar_width;
   Task *tsk;
   GSList *l;

//printf("resize_taskbar : posx et width des taches\n");

   taskbar->area.redraw = 1;

   // new task width for 'desktop'
   task_count = g_slist_length(taskbar->area.list);
   if (!task_count) pixel_width = panel->g_task.maximum_width;
   else {
      taskbar_width = taskbar->area.width - (2 * panel->g_taskbar.pix.border.width) - (2 * panel->g_taskbar.paddingxlr);
      if (task_count>1) taskbar_width -= ((task_count-1) * panel->g_taskbar.paddingx);

      pixel_width = taskbar_width / task_count;
      if (pixel_width > panel->g_task.maximum_width)
      	pixel_width = panel->g_task.maximum_width;
      else
      	modulo_width = taskbar_width % task_count;
   }

   if ((taskbar->task_width == pixel_width) && (taskbar->task_modulo == modulo_width)) {
   }
   else {
      taskbar->task_width = pixel_width;
      taskbar->task_modulo = modulo_width;
      taskbar->text_width = pixel_width - panel->g_task.text_posx - panel->g_task.area.pix.border.width - panel->g_task.area.paddingx;
   }

   // change pos_x and width for all tasks
   x = taskbar->area.posx + taskbar->area.pix.border.width + taskbar->area.paddingxlr;
   for (l = taskbar->area.list; l ; l = l->next) {
      tsk = l->data;
      tsk->area.posx = x;
      tsk->area.width = pixel_width;
      tsk->area.redraw = 1;
      if (modulo_width) {
         tsk->area.width++;
         modulo_width--;
      }

      x += tsk->area.width + panel->g_taskbar.paddingx;
   }
}



