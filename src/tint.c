/**************************************************************************
*
* Tint2 panel
*
* Copyright (C) 2007 Pål Staurland (staura@gmail.com)
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

#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>
#include <Imlib2.h>
#include <signal.h>

#include "server.h"
#include "window.h"
#include "config.h"
#include "task.h"
#include "taskbar.h"
#include "panel.h"
#include "docker.h"
#include "net.h"
#include "kde.h"


void signal_handler(int sig)
{
	// signal handler is light as it should be
	panel.signal_pending = sig;
}


void init ()
{
	// Set signal handler
   signal(SIGUSR1, signal_handler);
   signal(SIGINT, signal_handler);
   signal(SIGTERM, signal_handler);

   // set global data
   memset(&panel, 0, sizeof(Panel));
   memset(&server, 0, sizeof(Server_global));
   memset(&g_task, 0, sizeof(Global_task));
   memset(&g_taskbar, 0, sizeof(Area));
   panel.clock.area.draw_foreground = draw_foreground_clock;
   g_task.area.draw_foreground = draw_foreground_task;
   window.main_win = 0;

   server.dsp = XOpenDisplay (NULL);
   if (!server.dsp) {
      fprintf(stderr, "Could not open display.\n");
      exit(0);
   }
   server_init_atoms ();
   server.screen = DefaultScreen (server.dsp);
	server.root_win = RootWindow(server.dsp, server.screen);
   server.depth = DefaultDepth (server.dsp, server.screen);
   server.visual = DefaultVisual (server.dsp, server.screen);
   server.desktop = server_get_current_desktop ();

   XSetErrorHandler ((XErrorHandler) server_catch_error);

   // init systray
   display = server.dsp;
   root = RootWindow(display, DefaultScreen(display));
   //create_main_window();
   //kde_init();
   //net_init();
   //printf("ici 4\n");

   imlib_context_set_display (server.dsp);
   imlib_context_set_visual (server.visual);
   imlib_context_set_colormap (DefaultColormap (server.dsp, server.screen));

   /* Catch events */
   XSelectInput (server.dsp, server.root_win, PropertyChangeMask|StructureNotifyMask);

   setlocale (LC_ALL, "");
}


void window_action (Task *tsk, int action)
{
   switch (action) {
      case CLOSE:
         set_close (tsk->win);
         break;
      case TOGGLE:
         set_active(tsk->win);
         break;
      case ICONIFY:
         XIconifyWindow (server.dsp, tsk->win, server.screen);
         break;
      case TOGGLE_ICONIFY:
         if (tsk == panel.task_active) XIconifyWindow (server.dsp, tsk->win, server.screen);
         else set_active (tsk->win);
         break;
      case SHADE:
         window_toggle_shade (tsk->win);
         break;
   }
}


void event_button_press (int x, int y)
{
   if (panel.mode == SINGLE_DESKTOP) {
      // drag and drop disabled
      XLowerWindow (server.dsp, window.main_win);
      return;
   }

   Taskbar *tskbar;
   GSList *l0;
   for (l0 = panel.area.list; l0 ; l0 = l0->next) {
      tskbar = l0->data;
      if (x >= tskbar->area.posx && x <= (tskbar->area.posx + tskbar->area.width))
         break;
   }

   if (l0) {
      Task *tsk;
      for (l0 = tskbar->area.list; l0 ; l0 = l0->next) {
         tsk = l0->data;
         if (x >= tsk->area.posx && x <= (tsk->area.posx + tsk->area.width)) {
            panel.task_drag = tsk;
            break;
         }
      }
   }

   XLowerWindow (server.dsp, window.main_win);
}


void event_button_release (int button, int x, int y)
{
   int action = TOGGLE_ICONIFY;
   // TODO: convert event_button_press(int x, int y) to area->event_button_press()
   // if systray is ok

   switch (button) {
      case 2:
         action = panel.mouse_middle;
         break;
      case 3:
         action = panel.mouse_right;
         break;
      case 4:
         action = panel.mouse_scroll_up;
         break;
      case 5:
         action = panel.mouse_scroll_down;
         break;
   }

   // search taskbar
   Taskbar *tskbar;
   GSList *l0;
   for (l0 = panel.area.list; l0 ; l0 = l0->next) {
      tskbar = l0->data;
      if (x >= tskbar->area.posx && x <= (tskbar->area.posx + tskbar->area.width))
         goto suite;
   }

   // TODO: check better solution to keep window below
   XLowerWindow (server.dsp, window.main_win);
   panel.task_drag = 0;
   return;

suite:
   // drag and drop task
   if (panel.task_drag) {
      if (tskbar != panel.task_drag->area.parent && action == TOGGLE_ICONIFY) {
         windows_set_desktop(panel.task_drag->win, tskbar->desktop);
         if (tskbar->desktop == server.desktop)
            set_active(panel.task_drag->win);
         panel.task_drag = 0;
         return;
      }
      else panel.task_drag = 0;
   }

   // switch desktop
   if (panel.mode == MULTI_DESKTOP)
      if (tskbar->desktop != server.desktop && action != CLOSE)
         set_desktop (tskbar->desktop);

   // action on task
   Task *tsk;
   GSList *l;
   for (l = tskbar->area.list ; l ; l = l->next) {
      tsk = l->data;
      if (x >= tsk->area.posx && x <= (tsk->area.posx + tsk->area.width)) {
         window_action (tsk, action);
         break;
      }
   }

   // to keep window below
   XLowerWindow (server.dsp, window.main_win);
}


void event_property_notify (Window win, Atom at)
{

   if (win == server.root_win) {
      if (!server.got_root_win) {
         XSelectInput (server.dsp, server.root_win, PropertyChangeMask|StructureNotifyMask);
         server.got_root_win = 1;
      }

      /* Change number of desktops */
      else if (at == server.atom._NET_NUMBER_OF_DESKTOPS) {
         config_taskbar();
         visible_object();
      }
      /* Change desktop */
      else if (at == server.atom._NET_CURRENT_DESKTOP) {
         server.desktop = server_get_current_desktop ();
         if (panel.mode != MULTI_DESKTOP) {
            visible_object();
         }
      }
      /* Window list */
      else if (at == server.atom._NET_CLIENT_LIST) {
         task_refresh_tasklist ();
         panel.refresh = 1;
      }
      /* Change active */
      else if (at == server.atom._NET_ACTIVE_WINDOW) {
      	if (panel.task_active) {
      		panel.task_active->area.is_active = 0;
         	panel.task_active = 0;
			}
         Window w1 = window_get_active ();
         Task *t = task_get_task(w1);
         if (!t) {
            Window w2;
            if (XGetTransientForHint(server.dsp, w1, &w2) != 0)
               if (w2) t = task_get_task(w2);
         }
         if (t) {
	      	t->area.is_active = 1;
         	panel.task_active = t;
			}
         panel.refresh = 1;
      }
      /* Wallpaper changed */
      else if (at == server.atom._XROOTPMAP_ID) {
         XFreePixmap (server.dsp, panel.area.pix.pmap);
         panel.area.pix.pmap = 0;
         panel.refresh = 1;
      }
   }
   else {
      Task *tsk;
      tsk = task_get_task (win);
      if (!tsk) return;
      //printf("atom root_win = %s, %s\n", XGetAtomName(server.dsp, at), tsk->title);

      /* Window title changed */
      if (at == server.atom._NET_WM_VISIBLE_NAME || at == server.atom._NET_WM_NAME || at == server.atom.WM_NAME) {
         get_title(tsk);
         tsk->area.redraw = 1;
         panel.refresh = 1;
      }
      /* Iconic state */
      else if (at == server.atom.WM_STATE) {
         if (window_is_iconified (win))
            if (panel.task_active == tsk) {
            	tsk->area.is_active = 0;
            	panel.task_active = 0;
				}
      }
      /* Window icon changed */
      else if (at == server.atom._NET_WM_ICON) {
         if (tsk->icon_data) {
            free (tsk->icon_data);
            tsk->icon_data = 0;
         }
         tsk->area.redraw = 1;
         panel.refresh = 1;
      }
      /* Window desktop changed */
      else if (at == server.atom._NET_WM_DESKTOP) {
         add_task (tsk->win);
         remove_task (tsk);
         panel.refresh = 1;
      }

      if (!server.got_root_win) server.root_win = RootWindow (server.dsp, server.screen);
   }
}


void event_configure_notify (Window win)
{
   Task *tsk;

   tsk = task_get_task (win);
   if (!tsk) return;

   Taskbar *tskbar = tsk->area.parent;
   if (tskbar->monitor != window_get_monitor (win)) {
      // task on another monitor
      add_task (tsk->win);
      remove_task (tsk);
      panel.refresh = 1;
   }
}


void event_timer()
{
   struct timeval stv;

   if (!panel.clock.time1_format) return;

   if (gettimeofday(&stv, 0)) return;

   if (abs(stv.tv_sec - panel.clock.clock.tv_sec) < panel.clock.time_precision) return;

   // update clock
   panel.clock.clock.tv_sec = stv.tv_sec;
   panel.clock.clock.tv_sec -= panel.clock.clock.tv_sec % panel.clock.time_precision;
   panel.clock.area.redraw = 1;
   panel.refresh = 1;
}


int main (int argc, char *argv[])
{
   XEvent e;
   fd_set fd;
   int x11_fd, i, c;
   struct timeval tv;

   c = getopt (argc, argv, "c:");
   init ();

load_config:
   if (panel.area.pix.pmap) XFreePixmap (server.dsp, panel.area.pix.pmap);
   panel.area.pix.pmap = 0;
   // append full transparency background
   list_back = g_slist_append(0, calloc(1, sizeof(Area)));

   // read tint2rc config
   i = 0;
   if (c != -1)
      i = config_read_file (optarg);
   if (!i)
      i = config_read ();
   if (!i) {
      fprintf(stderr, "usage: tint2 [-c] <config_file>\n");
      cleanup();
      exit(1);
   }
   config_finish ();

   window_draw_panel ();

   // BUG: refresh(clock) is needed here, but 'on the paper' it's not necessary.
   refresh(&panel.clock.area);

   x11_fd = ConnectionNumber (server.dsp);
   XSync (server.dsp, False);

   while (1) {
      // thanks to AngryLlama for the timer
      // Create a File Description Set containing x11_fd
      FD_ZERO (&fd);
      FD_SET (x11_fd, &fd);

      tv.tv_usec = 500000;
      tv.tv_sec = 0;

      // Wait for X Event or a Timer
      if (select(x11_fd+1, &fd, 0, 0, &tv)) {
         while (XPending (server.dsp)) {
            XNextEvent(server.dsp, &e);

            switch (e.type) {
               case ButtonPress:
                  if (e.xbutton.button == 1) event_button_press (e.xbutton.x, e.xbutton.y);
                  break;

               case ButtonRelease:
                  event_button_release (e.xbutton.button, e.xbutton.x, e.xbutton.y);
                  break;

               case Expose:
                  XCopyArea (server.dsp, panel.area.pix.pmap, server.root_win, server.gc_root, 0, 0, panel.area.width, panel.area.height, server.posx, server.posy);
                  XCopyArea (server.dsp, server.pmap, window.main_win, server.gc, panel.area.paddingx, 0, panel.area.width-(2*panel.area.paddingx), panel.area.height, 0, 0);
                  break;

               case PropertyNotify:
                  //printf("PropertyNotify\n");
                  event_property_notify (e.xproperty.window, e.xproperty.atom);
                  break;

               case ConfigureNotify:
                  if (e.xconfigure.window == server.root_win)
                     goto load_config;
                  else
                     if (panel.mode == MULTI_MONITOR)
                        event_configure_notify (e.xconfigure.window);
                  break;
            }
         }
      }
      else event_timer();

		switch (panel.signal_pending) {
			case SIGUSR1:
            goto load_config;
			case SIGINT:
			case SIGTERM:
			   cleanup ();
			   return 0;
      }

      if (panel.refresh && !panel.sleep_mode) {
         visual_refresh ();
         //printf("   *** visual_refresh\n");
      }
   }
}


