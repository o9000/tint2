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
#include "timer.h"

typedef enum TaskState {
	TASK_NORMAL = 0,
	TASK_ACTIVE,
	TASK_ICONIFIED,
	TASK_URGENT,
	TASK_STATE_COUNT,
} TaskState;

extern timeout *urgent_timeout;
extern GSList *urgent_list;

// --------------------------------------------------
// global task parameter
typedef struct GlobalTask {
	Area area;

	gboolean text;
	gboolean icon;
	gboolean centered;

	int icon_posy;
	int icon_size1;
	int maximum_width;
	int maximum_height;
	int alpha[TASK_STATE_COUNT];
	int saturation[TASK_STATE_COUNT];
	int brightness[TASK_STATE_COUNT];
	int config_asb_mask;
	Background *background[TASK_STATE_COUNT];
	int config_background_mask;
	// starting position for text ~ task_padding + task_border + icon_size
	double text_posx, text_height;

	PangoFontDescription *font_desc;
	Color font[TASK_STATE_COUNT];
	int config_font_mask;
	gboolean tooltip_enabled;
} GlobalTask;

typedef struct {
	// always start with area
	Area area;

	// TODO: group task with list of windows here
	Window win;
	int desktop;
	TaskState current_state;
	Imlib_Image icon[TASK_STATE_COUNT];
	Imlib_Image icon_hover[TASK_STATE_COUNT];
	Imlib_Image icon_press[TASK_STATE_COUNT];
	Pixmap state_pix[TASK_STATE_COUNT];
	unsigned int icon_width;
	unsigned int icon_height;
	char *title;
	int urgent_tick;
	// These may not be up-to-date
	int win_x;
	int win_y;
	int win_w;
	int win_h;
} Task;

Task *add_task(Window win);
void remove_task(Task *task);

void draw_task(void *obj, cairo_t *c);
void on_change_task(void *obj);

void get_icon(Task *task);
gboolean get_title(Task *task);
void active_task();
void set_task_state(Task *task, TaskState state);
void set_task_redraw(Task *task);

Task *find_active_task(Task *current_task, Task *active_task);
Task *next_task(Task *task);
Task *prev_task(Task *task);

void add_urgent(Task *task);
void del_urgent(Task *task);

#endif
