/**************************************************************************
* Tint2 : systraybar
*
* Copyright (C) 2009 thierry lorthiois (lorthiois@bbsoft.fr)
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

#include "systraybar.h"
#include "server.h"
#include "window.h"
#include "panel.h"



int resize_systray (Systraybar *sysbar)
{
   return 0;
}

/*
// initialise taskbar posx and width
void resize_taskbar()
{
   int taskbar_width, modulo_width, taskbar_on_screen;

   if (panel.mode == MULTI_DESKTOP) taskbar_on_screen = panel.nb_desktop;
   else taskbar_on_screen = panel.nb_monitor;

   taskbar_width = panel.area.width - (2 * panel.area.paddingx) - (2 * panel.area.pix.border.width);
   if (panel.clock.time1_format)
      taskbar_width -= (panel.clock.area.width + panel.area.paddingx);
   taskbar_width = (taskbar_width - ((taskbar_on_screen-1) * panel.area.paddingx)) / taskbar_on_screen;

   if (taskbar_on_screen > 1)
      modulo_width = (taskbar_width - ((taskbar_on_screen-1) * panel.area.paddingx)) % taskbar_on_screen;
   else
      modulo_width = 0;

   int i, nb, modulo=0, posx=0;
   nb = panel.nb_desktop * panel.nb_monitor;
   for (i=0 ; i < nb ; i++) {
      if ((i % taskbar_on_screen) == 0) {
         posx = panel.area.pix.border.width + panel.area.paddingx;
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
*/


