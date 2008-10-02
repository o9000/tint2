/**************************************************************************
*
* Copyright (C) 2008 Pål Staurland (staura@gmail.com)
* Modified (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
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
#include <cairo.h>
#include <cairo-xlib.h>
#include <pango/pangocairo.h>

#include "server.h"
#include "window.h"
#include "task.h"
#include "panel.h"


void visual_refresh ()
{
   server_refresh_root_pixmap ();
   
   draw (&panel.area);
   refresh (&panel.area);
   
/*
pour version 0.7
gestion du systray
  positionnement et taille fixe du systray (objet systray)
  détection des notifications (détection des icones, ajout a la liste)
  ajouter la transparence des icones
  gérer le redimentionnement des éléments
    => voir si lon peut faire abstraction sur le positionnement des objets ?
       sachant que certains objets (task, taskbar) on une taille définit par l'extérieur
       et d'autres objets (clock, systray) on une taille définit par l'intérieur

gestion du layout 
  voir le positionnement des taskbar, task et systray
  définir panel_layout dans la configuration
  comment gérer le multi panel avec des layouts différents

vérifier le niveau d'abstraction du code
  utiliser la fonction draw(obj) récurrente sur Taskbar, Task, Systray, Clock
  est ce compatible avec l'affichage de la tache active et les changement de taille -> redessine le panel

correction de bugs : 
  memory, segfault
  background
  remettre en place single_desktop avec nouveau layout
  remettre en place multi_monitor avec nouveau layout
  vérifier le changement de configuration

pour version 0.8
gestion du thème
  voir la gestion du dégradé sur le bord et le fond (inkscape)
  faut-il trois coordonnées de padding x, y, x inter-objects

gestion du zoom  
  définir le zoom du panel

*/

   if (panel.clock.time1_format) {
      if (panel.clock.area.redraw)
         panel.refresh = 1;
      if (draw (&panel.clock.area)) {
         panel.clock.area.redraw = 1;
         draw (&panel.clock.area);
         resize_clock();
         resize_taskbar();
         redraw(&panel.area);
      }
      refresh (&panel.clock.area);
   }

   // TODO: ne pas afficher les taskbar invisibles
   //if (panel.mode != MULTI_DESKTOP && desktop != server.desktop) continue;
   Task *tsk;
   Taskbar *tskbar;
   GSList *l0;
   for (l0 = panel.area.list; l0 ; l0 = l0->next) {
      tskbar = l0->data;
      draw (&tskbar->area);
      refresh (&tskbar->area);
      
      GSList *l1;
      for (l1 = tskbar->area.list; l1 ; l1 = l1->next) {
         tsk = l1->data;
         draw(&tsk->area);
         
         if (tsk == panel.task_active) refresh (&tsk->area_active);
         else refresh (&tsk->area);
      }
   }

   XCopyArea (server.dsp, server.pmap, window.main_win, server.gc, 0, 0, panel.area.width, panel.area.height, 0, 0);
   XFlush(server.dsp);
   panel.refresh = 0;
}


void set_panel_properties (Window win)
{
   XStoreName (server.dsp, win, "tint2");

   // TODO: check if the name is really needed for a panel/taskbar ?
   gsize len;
   gchar *name = g_locale_to_utf8("tint2", -1, NULL, &len, NULL);
   if (name != NULL) {
      XChangeProperty(server.dsp, win, server.atom._NET_WM_NAME, server.atom.UTF8_STRING, 8, PropModeReplace, (unsigned char *) name, (int) len);
      g_free(name);
   }
  
   // Dock
   long val = server.atom._NET_WM_WINDOW_TYPE_DOCK;
   XChangeProperty (server.dsp, win, server.atom._NET_WM_WINDOW_TYPE, XA_ATOM, 32, PropModeReplace, (unsigned char *) &val, 1);

   // Reserved space
   long   struts [12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
   if (panel.position & TOP) {
      struts[2] = panel.area.height + panel.marginy;
      struts[8] = server.posx;
      struts[9] = server.posx + panel.area.width;
   }
   else {
      struts[3] = panel.area.height + panel.marginy;
      struts[10] = server.posx;
      struts[11] = server.posx + panel.area.width;
   }
   XChangeProperty (server.dsp, win, server.atom._NET_WM_STRUT_PARTIAL, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &struts, 12);
   // Old specification
   XChangeProperty (server.dsp, win, server.atom._NET_WM_STRUT, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &struts, 4);
   
   // Sticky and below other window
   val = 0xFFFFFFFF;
   XChangeProperty (server.dsp, win, server.atom._NET_WM_DESKTOP, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &val, 1);
   Atom state[4];
   state[0] = server.atom._NET_WM_STATE_SKIP_PAGER;
   state[1] = server.atom._NET_WM_STATE_SKIP_TASKBAR;
   state[2] = server.atom._NET_WM_STATE_STICKY;
   state[3] = server.atom._NET_WM_STATE_BELOW;
   XChangeProperty (server.dsp, win, server.atom._NET_WM_STATE, XA_ATOM, 32, PropModeReplace, (unsigned char *) state, 4);   
   
   // Fixed position
   XSizeHints size_hints;
   size_hints.flags = PPosition;
   XChangeProperty (server.dsp, win, XA_WM_NORMAL_HINTS, XA_WM_SIZE_HINTS, 32, PropModeReplace, (unsigned char *) &size_hints, sizeof (XSizeHints) / 4);
   
   // Unfocusable
   XWMHints wmhints;
   wmhints.flags = InputHint;
   wmhints.input = False;
   XChangeProperty (server.dsp, win, XA_WM_HINTS, XA_WM_HINTS, 32, PropModeReplace, (unsigned char *) &wmhints, sizeof (XWMHints) / 4);
}


void window_draw_panel ()
{
   Window win;

   /* panel position determined here */
   if (panel.position & LEFT) server.posx = server.monitor[panel.monitor].x + panel.marginx;
   else {
      if (panel.position & RIGHT) server.posx = server.monitor[panel.monitor].x + server.monitor[panel.monitor].width - panel.area.width - panel.marginx;
      else server.posx = server.monitor[panel.monitor].x + ((server.monitor[panel.monitor].width - panel.area.width) / 2);
   }
   if (panel.position & TOP) server.posy = server.monitor[panel.monitor].y + panel.marginy;
   else server.posy = server.monitor[panel.monitor].y + server.monitor[panel.monitor].height - panel.area.height - panel.marginy;

   /* Catch some events */
   XSetWindowAttributes att = { ParentRelative, 0L, 0, 0L, 0, 0, Always, 0L, 0L, False, ExposureMask|ButtonPressMask|ButtonReleaseMask, NoEventMask, False, 0, 0 };
               
   /* XCreateWindow(display, parent, x, y, w, h, border, depth, class, visual, mask, attrib) */
   if (window.main_win) XDestroyWindow(server.dsp, window.main_win);
   win = XCreateWindow (server.dsp, server.root_win, server.posx, server.posy, panel.area.width, panel.area.height, 0, server.depth, InputOutput, CopyFromParent, CWEventMask, &att);

   set_panel_properties (win);
   window.main_win = win;

   // replaced : server.gc = DefaultGC (server.dsp, 0);
   if (server.gc) XFree(server.gc);
   XGCValues gcValues;
   server.gc = XCreateGC(server.dsp, win, (unsigned long) 0, &gcValues);
   
   XMapWindow (server.dsp, win);
   XFlush (server.dsp);
}


void resize_clock()
{
   panel.clock.area.posx = panel.area.width - panel.clock.area.width - panel.area.paddingx - panel.area.border.width;
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
   
   int posx, modulo, i;
   Taskbar *tskbar;
   GSList *l0;
   for (i = 0, l0 = panel.area.list; l0 ; i++, l0 = l0->next) {
      if ((i % taskbar_on_screen) == 0) {
         posx = panel.area.border.width + panel.area.paddingx;
         modulo = modulo_width;
      }
      else posx += taskbar_width + panel.area.paddingx;

      tskbar = l0->data;      
      tskbar->area.posx = posx;
      tskbar->area.width = taskbar_width;
      if (modulo) {
         tskbar->area.width++;
         modulo--;
      }

      resize_tasks(tskbar);
   }
}


