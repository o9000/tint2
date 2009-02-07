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

		// taskbar
		panel->g_taskbar.posy = panel->area.pix.border.width + panel->area.paddingy;
		panel->g_taskbar.height = panel->area.height - (2 * panel->g_taskbar.posy);
		panel->g_taskbar.redraw = 1;

		// task
		panel->g_task.area.draw_foreground = draw_foreground_task;
		panel->g_task.area.posy = panel->g_taskbar.posy + panel->g_taskbar.pix.border.width + panel->g_taskbar.paddingy;
		panel->g_task.area.height = panel->area.height - (2 * panel->g_task.area.posy);
		panel->g_task.area.use_active = 1;
		panel->g_task.area.redraw = 1;

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
		}

	   resize_taskbar(panel);
	}

}


void cleanup_taskbar()
{
   Panel *panel;
   int i, j;
	GSList *l0;
	Task *tsk;

	for (i=0 ; i < nb_panel ; i++) {
		panel = &panel1[i];
		if (!panel->taskbar) continue;

		for (j=0 ; j < panel->nb_desktop ; j++) {
			l0 = panel->taskbar[j].area.list;
			while (l0) {
				tsk = l0->data;
				l0 = l0->next;
				// careful : remove_task change l0->next
				remove_task (tsk);
			}

			free_area (&panel->taskbar[j].area);
		}

		free(panel->taskbar);
		panel->taskbar = 0;
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
				if (tsk->win != win[k]) remove_task (tsk);
			}
		}
   }

   // Add any new
   for (i = 0; i < num_results; i++)
      if (!task_get_task (win[i]))
         add_task (win[i]);

   XFree (win);
}


int resize_tasks (Taskbar *taskbar)
{
   int ret, task_count, pixel_width, modulo_width=0;
   int x, taskbar_width;
   Task *tsk;
   Panel *panel = (Panel*)taskbar->area.panel;
   GSList *l;

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
	//printf("monitor %d, resize_tasks %d %d\n", panel->monitor, task_count, pixel_width);

   if ((taskbar->task_width == pixel_width) && (taskbar->task_modulo == modulo_width)) {
      ret = 0;
   }
   else {
      ret = 1;
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
      if (modulo_width) {
         tsk->area.width++;
         modulo_width--;
      }

      x += tsk->area.width + panel->g_taskbar.paddingx;
   }
   return ret;
}


// initialise taskbar posx and width
void resize_taskbar(void *p)
{
   Panel *panel = p;
   int taskbar_width, modulo_width, taskbar_on_screen;

   if (panel_mode == MULTI_DESKTOP) taskbar_on_screen = panel->nb_desktop;
   else taskbar_on_screen = 1;

   taskbar_width = panel->area.width - (2 * panel->area.paddingxlr) - (2 * panel->area.pix.border.width);
   if (time1_format)
      taskbar_width -= (panel->clock.area.width + panel->area.paddingx);
   taskbar_width = (taskbar_width - ((taskbar_on_screen-1) * panel->area.paddingx)) / taskbar_on_screen;

   if (taskbar_on_screen > 1)
      modulo_width = (taskbar_width - ((taskbar_on_screen-1) * panel->area.paddingx)) % taskbar_on_screen;
   else
      modulo_width = 0;

   int i, modulo=0, posx=0;
   for (i=0 ; i < panel->nb_desktop ; i++) {
      if ((i % taskbar_on_screen) == 0) {
         posx = panel->area.pix.border.width + panel->area.paddingxlr;
         modulo = modulo_width;
      }
      else posx += taskbar_width + panel->area.paddingx;

      panel->taskbar[i].area.posx = posx;
      panel->taskbar[i].area.width = taskbar_width;
      if (modulo) {
         panel->taskbar[i].area.width++;
         modulo--;
      }

      resize_tasks(&panel->taskbar[i]);
   }
}


