/**************************************************************************
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
*
**************************************************************************/

#ifndef AREA_H
#define AREA_H

#include <glib.h>
#include <X11/Xlib.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include "color.h"
#include "gradient.h"

// DATA ORGANISATION
//
// Areas in tint2 are similar to widgets in a GUI.
// All graphical objects (panel, taskbar, task, systray, clock, ...) inherit the abstract class Area.
// This class 'Area' stores data about the background, border, size, position, padding and the child areas.
// Inheritance is simulated by having an Area member as the first member of each object (thus &object == &area).
//
// tint2 uses multiple panels, one per monitor. Each panel has an area containing the children objects in a tree of
// areas. The level in the tree gives the z-order: child areas are always displayed on top of their parents.
//
//
// LAYOUT
//
// Sibling areas never overlap.
//
// The position of an Area (posx, posy) is relative to the window (i.e. absolute) and
// is computed based on a simple box model:
// * parent position + parent padding + sum of the sizes of the previous siblings and spacing
//
// The size of an Area is:
// * SIZE_BY_CONTENT objects:
//   * fixed and set by the Area
//   * childred are resized before the parent
//   * if a child size has changed then the parent is resized
// * SIZE_BY_LAYOUT objects:
//   * expandable and computed as the total size of the parent - padding -
//     the size of the fixed sized siblings - spacing and divided by the number of expandable siblings
//   * the parent is resized before the children
//
//
// RENDERING
//
// Redrawing an object (like the clock) could come from an 'external event' (date change)
// or from a 'layout event' (position change).
//
//
// WIDGET LIFECYCLE
//
// Each widget that occurs once per panel is defined as a struct (e.g. Clock) which is stored as a member of Panel.
// Widgets that can occur more than once should be stored as an array, still as a member of Panel.
//
// There is a special Panel instance called 'panel_config' which stores the config options and the state variables
// of the widgets (however some config options are stored as global variables by the widgets).
//
// Tint2 maintains an array of Panel instances, one for each monitor. These contain the actual Areas that are used to
// render the panels on screen, interact with user input etc.
// Each Panel is initialized as a raw copy (memcpy, see init_panel()) of panel_config.
//
// Normally, widgets should implement the following functions:
//
// * void default_widget();
//
//   Called before the config is read and panel_config/panels are created.
//   Afterwards, the config parsing code creates the widget/widget array in panel_config and
//   populates the configuration fields.
//   If the widget uses global variables to store config options or other state variables, they should be initialized
//   here (e.g. with zero, NULL etc).
//
// * void init_widget();
//
//   Called after the config is read and panel_config is populated, but before panels are created.
//   Initializes the state of the widget in panel_config.
//   If the widget uses global variables to store config options or other state variables which depend on the config
//   options but not on the panel instance, they should be initialized here.
//   panel_config.panel_items can be used to determine which backend items are enabled.
//
// * void init_widget_panel(void *panel);
//
//   Called after each on-screen panel is created, with a pointer to the panel.
//   Completes the initialization of the widget.
//   At this point the widget Area has not been added yet to the GUI tree, but it will be added right afterwards.
//
// * void cleanup_widget();
//
//   Called just before the panels are destroyed. Afterwards, tint2 exits or restarts and reads the config again.
//   Must releases all resources.
//   The widget itself should not be freed by this function, only its members or global variables that were set.
//   The widget is freed by the Area tree cleanup function (remove_area).
//
// * void draw_widget(void *obj, cairo_t *c);
//
//   Called on draw, obj = pointer to the widget instance from the panel that is redrawn.
//   The Area's _draw_foreground member must point to this function.
//
// * int resize_widget(void *obj);
//
//   Called on resize, obj = pointer to the front-end Execp item.
//   Returns 1 if the new size is different than the previous size.
//   The Area's _resize member must point to this function.
//
// * void widget_action(void *obj, int button);
//
//   Called on mouse click event.
//
// * void widget_on_change_layout(void *obj);
//
//   Implemented only to override the default layout algorithm for this widget.
//   For example, if this widget is a cell in a table, its position and size should be computed here.
//   The Area's _on_change_layout member must point to this function.
//
// * char* widget_get_tooltip_text(void *obj);
//
//   Returns a copy of the tooltip to be displayed for this widget.
//   The caller takes ownership of the pointer.
//   The Area's _get_tooltip_text member must point to this function.

typedef enum BorderMask {
	BORDER_TOP     = 1 << 0,
	BORDER_BOTTOM  = 1 << 1,
	BORDER_LEFT    = 1 << 2,
	BORDER_RIGHT   = 1 << 3
} BorderMask;

#define BORDER_ALL (BORDER_TOP | BORDER_BOTTOM | BORDER_LEFT | BORDER_RIGHT)

typedef struct Border {
	// It's essential that the first member is color
	Color color;
	// Width in pixels
	int width;
	// Corner radius
	int radius;
	// Mask: bitwise OR of BorderMask
	int mask;
} Border;

typedef enum MouseState { MOUSE_NORMAL = 0, MOUSE_OVER = 1, MOUSE_DOWN = 2, MOUSE_STATE_COUNT } MouseState;

typedef struct Background {
	// Normal state
	Color fill_color;
	Border border;
	// On mouse hover
	Color fill_color_hover;
	Color border_color_hover;
	// On mouse press
	Color fill_color_pressed;
	Color border_color_pressed;
	// Pointer to a GradientClass or NULL, no ownership
	GradientClass *gradients[MOUSE_STATE_COUNT];
} Background;

typedef enum Layout {
	LAYOUT_DYNAMIC,
	LAYOUT_FIXED,
} Layout;

typedef enum Alignment {
	ALIGN_LEFT = 0,
	ALIGN_CENTER = 1,
	ALIGN_RIGHT = 2,
} Alignment;

struct Panel;

typedef struct Area {
	// Position relative to the panel window
	int posx, posy;
	// Size, including borders
	int width, height;
	int old_width, old_height;
	Background *bg;
	// Each element is a GradientInstance attached to this Area (list can be empty)
	GList *gradient_instances_by_state[MOUSE_STATE_COUNT];
	// Each element is a GradientInstance that depends on this Area's geometry (position or size)
	GList *dependent_gradients;
	// List of children, each one a pointer to Area
	GList *children;
	// Pointer to the parent Area or NULL
	void *parent;
	// Pointer to the Panel that contains this Area
	void *panel;
	Layout size_mode;
	Alignment alignment;
	gboolean has_mouse_over_effect;
	gboolean has_mouse_press_effect;
	// TODO padding/spacing is a clusterfuck
	// paddingxlr = padding
	// paddingy = vertical padding, sometimes
	// paddingx = spacing
	int paddingxlr, paddingx, paddingy;
	MouseState mouse_state;
	// Set to non-zero if the Area is visible. An object may exist but stay hidden.
	gboolean on_screen;
	// Set to non-zero if the size of the Area has to be recalculated.
	gboolean resize_needed;
	// Set to non-zero if the Area has to be redrawn.
	// Do not set this directly; use schedule_redraw() instead.
	gboolean _redraw_needed;
	// Set to non-zero if the position/size has changed, thus _on_change_layout needs to be called
	gboolean _changed;
	// This is the pixmap on which the Area is rendered. Render to it directly if needed.
	Pixmap pix;
	Pixmap pix_by_state[MOUSE_STATE_COUNT];
	char name[32];

	// Callbacks

	// Called on draw before any drawing takes place, obj = pointer to the Area
	void (*_clear)(void *obj);

	// Called on draw, obj = pointer to the Area
	void (*_draw_foreground)(void *obj, cairo_t *c);

	// Called on resize, obj = pointer to the Area
	// Returns 1 if the new size is different than the previous size.
	gboolean (*_resize)(void *obj);

	// Called before resize, obj = pointer to the Area
	// Returns the desired size of the Area
	int (*_compute_desired_size)(void *obj);

	// Implemented only to override the default layout algorithm for this widget.
	// For example, if this widget is a cell in a table, its position and size should be computed here.
	void (*_on_change_layout)(void *obj);

	// Returns a copy of the tooltip to be displayed for this widget.
	// The caller takes ownership of the pointer.
	char *(*_get_tooltip_text)(void *obj);

	// Returns true if the Area handles a mouse event at the given x, y coordinates relative to the window.
	// Leave this to NULL to use a default implementation.
	gboolean (*_is_under_mouse)(void *obj, int x, int y);

	// Prints the geometry of the object on stderr, with left indentation of indent spaces.
	void (*_dump_geometry)(void *obj, int indent);
} Area;

// Initializes the Background member to default values.
void init_background(Background *bg);

// Layout

// Called on startup to initialize the positions of all Areas in the Area tree.
// Parameters:
// * obj: pointer to Area
// * offset: offset in pixels from left/top, relative to the window
void initialize_positions(void *obj, int offset);

// Relayouts the Area and its children. Normally called on the root of the tree (i.e. the Panel).
void relayout(Area *a);

// Distributes the Area's size to its children, repositioning them as needed.
// If maximum_size > 0, it is an upper limit for the child size.
int relayout_with_constraint(Area *a, int maximum_size);

int compute_desired_size(Area *a);
int container_compute_desired_size(Area *a);

int left_border_width(Area *a);
int right_border_width(Area *a);
int left_right_border_width(Area *a);
int top_border_width(Area *a);
int bottom_border_width(Area *a);
int top_bottom_border_width(Area *a);

int left_bg_border_width(Background *bg);
int right_bg_border_width(Background *bg);
int top_bg_border_width(Background *bg);
int bottom_bg_border_width(Background *bg);
int left_right_bg_border_width(Background *bg);
int top_bottom_bg_border_width(Background *bg);

// Rendering

// Sets the redraw_needed flag on the area and its descendants
void schedule_redraw(Area *a);

// Recreates the Area pixmap and draws the background and the foreground
void draw(Area *a);

// Draws the background of the Area
void draw_background(Area *a, cairo_t *c);

// Explores the entire Area subtree (only if the on_screen flag set)
// and draws the areas with the redraw_needed flag set
void draw_tree(Area *a);

// Clears the on_screen flag, sets the size to zero and triggers a parent resize
void hide(Area *a);

// Sets the on_screen flag and triggers a parent and area resize
void show(Area *a);

// Area tree

void add_area(Area *a, Area *parent);
void remove_area(Area *a);
void free_area(Area *a);

// Mouse events

// Returns the area under the mouse for the given x, y mouse coordinates relative to the window.
// If no area is found, returns the root.
Area *find_area_under_mouse(void *root, int x, int y);

// Returns true if the Area handles a mouse event at the given x, y coordinates relative to the window.
gboolean area_is_under_mouse(void *obj, int x, int y);

// Returns true if the Area handles a mouse event at the given x, y coordinates relative to the window.
// The Area will also handle clicks on the border of its ancestors, including the panel.
// Useful so that a click at the edge of the screen is still handled by task buttons etc., even if technically
// they are outside the drawing area of the button.
gboolean full_width_area_is_under_mouse(void *obj, int x, int y);

void instantiate_area_gradients(Area *area);
void free_area_gradient_instances(Area *area);

void area_dump_geometry(Area *area, int indent);

void mouse_over(Area *area, int pressed);
void mouse_out();

void update_gradient(GradientInstance *gi);
void update_dependent_gradients(Area *a);

#endif
