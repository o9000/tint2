/**************************************************************************
* task :
* -
*
**************************************************************************/

#ifndef TASK_H
#define TASK_H

#include <X11/Xlib.h>
#include <pango/pangocairo.h>
#include "common.h"


// --------------------------------------------------
// global task parameter
typedef struct {
   Area area;

   int text;
   int icon;
   int icon_size1;
   int centered;
   int maximum_width;
   int font_shadow;
   // icon position
   int icon_posy;
   // starting position for text ~ task_padding + task_border + icon_size
   double text_posx, text_posy;
   PangoFontDescription *font_desc;
   config_color font;
   config_color font_active;
} Global_task;



// --------------------------------------------------
// task parameter
typedef struct {
   // always start with area
   Area area;

   // TODO: group task with list of windows here
   Window win;
   long *icon_data;
   int icon_width;
   int icon_height;
   char *title;
} Task;


Global_task g_task;


void add_task (Window win);
void remove_task (Task *tsk);

void draw_foreground_task (void *obj, cairo_t *c, int active);

void get_icon (Task *tsk);
void get_title(Task *tsk);

#endif

