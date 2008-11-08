/**************************************************************************
* panel : 
* - draw panel and all objects according to panel_layout
* 
* Check COPYING file for Copyright
*
**************************************************************************/

#ifndef PANEL_H
#define PANEL_H

#include <pango/pangocairo.h>
#include <sys/time.h>

#include "common.h"
#include "clock.h"
#include "task.h"
#include "taskbar.h"


//panel mode
enum { SINGLE_DESKTOP=0, MULTI_DESKTOP, MULTI_MONITOR };

//panel alignment
enum { LEFT=0x01, RIGHT=0x02, CENTER=0X04, TOP=0X08, BOTTOM=0x10 };


typedef struct {
   // always start with area
   Area area;

   // --------------------------------------------------
   // backward compatibility
   int old_config_file;
   int old_task_icon;
   int old_panel_background;
   int old_task_background;
   char *old_task_font;

   // --------------------------------------------------
   // panel
   int signal_pending;
   int sleep_mode;
   int refresh;
   int monitor;
   int position;
   int marginx, marginy;

   // --------------------------------------------------
   // taskbar point to the first taskbar in panel.area.list. number of tasbar == nb_desktop x nb_monitor.
   Taskbar *taskbar;
   int mode;
   int nb_desktop;
   int nb_monitor;
   Task *task_active;
   Task *task_drag;
   
   // --------------------------------------------------
   // clock
   Clock clock;

   // --------------------------------------------------
   // systray

   // --------------------------------------------------
   // mouse events
   int mouse_middle;
   int mouse_right;
   int mouse_scroll_up;
   int mouse_scroll_down;
} Panel;


Panel panel;


void visual_refresh ();
void set_panel_properties (Window win);
void window_draw_panel ();
void visible_object();


#endif
