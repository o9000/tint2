/**************************************************************************
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
*
* base class for all graphical objects (panel, taskbar, task, systray, clock, ...).
* Area is at the begining of each object (&object == &area).
*
* Area manage the background and border drawing, size and padding.
* Each Area has one Pixmap (pix).
*
* Area manage the tree of all objects. Parent object drawn before child object.
*   panel -> taskbars -> tasks
*         -> systray -> icons
*         -> clock
*
* draw_foreground(obj) and resize(obj) are virtual function.
*
**************************************************************************/

#ifndef AREA_H
#define AREA_H

#include <glib.h>
#include <X11/Xlib.h>
#include <cairo.h>
#include <cairo-xlib.h>


typedef struct
{
	double color[3];
	double alpha;
	int width;
	int rounded;
} Border;


typedef struct
{
	double color[3];
	double alpha;
} Color;

typedef struct
{
	Color back;
	Border border;
} Background;


// way to calculate the size
// SIZE_BY_LAYOUT objects : taskbar and task
// SIZE_BY_CONTENT objects : clock, battery, launcher, systray
enum { SIZE_BY_LAYOUT, SIZE_BY_CONTENT };

typedef struct {
	// coordinate relative to panel window
	int posx, posy;
	// width and height including border
	int width, height;
	Pixmap pix;
	Background *bg;

	// list of child : Area object
	GSList *list;

	// object visible on screen. 
	// An object (like systray) could be enabled but hidden (because no tray icon).
	int on_screen;
	// way to calculate the size (SIZE_BY_CONTENT or SIZE_BY_LAYOUT)
	int size_mode;
	// need to calculate position and width
	int resize;
	// need redraw Pixmap
	int redraw;
	// paddingxlr = horizontal padding left/right
	// paddingx = horizontal padding between childs
	int paddingxlr, paddingx, paddingy;
	// parent Area
	void *parent;
	// panel
	void *panel;

	// each object can overwrite following function
	void (*_draw_foreground)(void *obj, cairo_t *c);
	// update area's content and update size (width/heith). 
	// return '1' if size changed, '0' otherwise.
	int (*_resize)(void *obj);
	void (*_add_child)(void *obj);
	int (*_remove_child)(void *obj);
	const char* (*_get_tooltip_text)(void *obj);
} Area;

// on startup, initialize fixed pos/size
void init_rendering(void *obj, int pos);

void rendering(void *obj);
void size_by_content (Area *a);
void size_by_layout (Area *a, int pos, int level);
// draw background and foreground
void refresh (Area *a);
 
// generic resize for SIZE_BY_LAYOUT objects
int resize_by_layout(void *obj);

// set 'redraw' on an area and childs
void set_redraw (Area *a);

// hide/unhide area
void hide(Area *a);
void show(Area *a);

// draw pixmap
void draw (Area *a);
void draw_background (Area *a, cairo_t *c);

void remove_area (Area *a);
void add_area (Area *a);
void free_area (Area *a);

// draw rounded rectangle
void draw_rect(cairo_t *c, double x, double y, double w, double h, double r);

// clear pixmap with transparent color
void clear_pixmap(Pixmap p, int x, int y, int w, int h);
#endif

