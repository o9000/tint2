/**************************************************************************
* task :
* -
*
**************************************************************************/

#ifndef TASK_H
#define TASK_H

#include <X11/Xlib.h>
#include <pango/pangocairo.h>
#include <Imlib2.h>
#include "common.h"


// --------------------------------------------------
// global task parameter
typedef struct {
	Area area;

	int text;
	int icon;
	int centered;

	int icon_posy;
	int icon_size1;
	int maximum_width;
	int maximum_height;
   int hue, saturation, brightness;
   int hue_active, saturation_active, brightness_active;
	// starting position for text ~ task_padding + task_border + icon_size
	double text_posx, text_posy;

	int font_shadow;
	PangoFontDescription *font_desc;
	config_color font;
	config_color font_active;
} Global_task;



typedef struct {
	// always start with area
	Area area;

	// TODO: group task with list of windows here
	Window win;
	int  desktop;
	// ARGB icon
	unsigned int *icon_data;
	unsigned int *icon_data_active;
	unsigned int icon_width;
	unsigned int icon_height;
	char *title;
} Task;



Task *add_task (Window win);
void remove_task (Task *tsk);

void draw_task (void *obj, cairo_t *c, int active);

void get_icon (Task *tsk);
void get_title(Task *tsk);


#endif

