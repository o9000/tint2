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

#include "taskbar.h"
#include "server.h"
#include "window.h"
#include "panel.h"



Task *task_get_task (Window win)
{
   Task *tsk;
   GSList *l0;
   int i, nb;
   
   nb = panel.nb_desktop * panel.nb_monitor;
   for (i=0 ; i < nb ; i++) {
      for (l0 = panel.taskbar[i].area.list; l0 ; l0 = l0->next) {
         tsk = l0->data;
         if (win == tsk->win) return tsk;
      }
   }
   return 0;
}


void task_refresh_tasklist ()
{
   Window *win, active_win;
   int num_results, i, j, nb;
   GSList *l0;
   Task *tsk;

   win = server_get_property (server.root_win, server.atom._NET_CLIENT_LIST, XA_WINDOW, &num_results);

   if (!win) return;
   
   // Remove any old and set active win
   active_win = window_get_active ();

   nb = panel.nb_desktop * panel.nb_monitor;
   for (i=0 ; i < nb ; i++) {
      for (l0 = panel.taskbar[i].area.list; l0 ; l0 = l0->next) {
         tsk = l0->data;
         
         if (tsk->win == active_win) panel.task_active = tsk;
         
         for (j = 0; j < num_results; j++) {
            if (tsk->win == win[j]) break;
         }
         if (tsk->win != win[j]) remove_task (tsk);
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
   GSList *l;

   // new task width for 'desktop'
   task_count = g_slist_length(taskbar->area.list);
   if (!task_count) pixel_width = g_task.maximum_width;
   else {
      taskbar_width = taskbar->area.width - (2 * g_taskbar.border.width) - ((task_count+1) * g_taskbar.paddingx);

      pixel_width = taskbar_width / task_count;
      if (pixel_width > g_task.maximum_width) pixel_width = g_task.maximum_width;
      else modulo_width = taskbar_width % task_count;
   }

   if ((taskbar->task_width == pixel_width) && (taskbar->task_modulo == modulo_width)) {
      ret = 0;
   }
   else {
      ret = 1;
      taskbar->task_width = pixel_width;
      taskbar->task_modulo = modulo_width;
      taskbar->text_width = pixel_width - g_task.text_posx - g_task.area.border.width - g_task.area.paddingx;
   }
      
   // change pos_x and width for all tasks
   x = taskbar->area.posx + taskbar->area.border.width + taskbar->area.paddingx;
   for (l = taskbar->area.list; l ; l = l->next) {
      tsk = l->data;
      tsk->area.posx = x;
      tsk->area_active.posx = x;
      tsk->area.width = pixel_width;
      tsk->area_active.width = pixel_width;
      if (modulo_width) {
         tsk->area.width++;
         tsk->area_active.width++;
         modulo_width--;
      }

      x += tsk->area.width + g_taskbar.paddingx;
   }
   return ret;
}


// initialise taskbar posx and width
void resize_taskbar()
{
   int taskbar_width, modulo_width, taskbar_on_screen;

   if (panel.mode == MULTI_DESKTOP) taskbar_on_screen = panel.nb_desktop;
   else taskbar_on_screen = panel.nb_monitor;
   
   taskbar_width = panel.area.width - (2 * panel.area.paddingx) - (2 * panel.area.border.width);
   if (panel.clock.time1_format) 
      taskbar_width -= (panel.clock.area.width + panel.area.paddingx);
   taskbar_width = (taskbar_width - ((taskbar_on_screen-1) * panel.area.paddingx)) / taskbar_on_screen;

   if (taskbar_on_screen > 1)
      modulo_width = (taskbar_width - ((taskbar_on_screen-1) * panel.area.paddingx)) % taskbar_on_screen;
   else 
      modulo_width = 0;
   
   int posx, modulo, i, nb;
   nb = panel.nb_desktop * panel.nb_monitor;
   for (i=0 ; i < nb ; i++) {
      if ((i % taskbar_on_screen) == 0) {
         posx = panel.area.border.width + panel.area.paddingx;
         modulo = modulo_width;
      }
      else posx += taskbar_width + panel.area.paddingx;

      panel.taskbar[i].area.posx = posx;
      panel.taskbar[i].area.width = taskbar_width;
      if (modulo) {
         panel.taskbar[i].area.width++;
         modulo--;
      }
      
      resize_tasks(&panel.taskbar[i]);
   }
}


