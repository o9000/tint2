#ifndef EXECPLUGIN_H
#define EXECPLUGIN_H

#include <sys/time.h>
#include <pango/pangocairo.h>

#include "area.h"
#include "common.h"
#include "timer.h"

// Architecture:
// Panel panel_config contains an array of Execp, each storing all config options and all the state variables.
// Only these run commands.
//
// Tint2 maintains an array of Panels, one for each monitor. Each stores an array of Execp which was initially copied
// from panel_config. Each works as a frontend to the corresponding Execp in panel_config as backend, using the
// backend's config and state variables.

typedef struct ExecpBackend {
	// Config:
	// Command to execute at a specified interval
	char *command;
	// Interval in seconds
	int interval;
	// 1 if first line of output is an icon path
	gboolean has_icon;
	gboolean cache_icon;
	int icon_w;
	int icon_h;
	char *tooltip;
	gboolean centered;
	gboolean has_font;
	PangoFontDescription *font_desc;
	Color font_color;
	int continuous;
	gboolean has_markup;
	char *lclick_command;
	char *mclick_command;
	char *rclick_command;
	char *uwheel_command;
	char *dwheel_command;
	// paddingxlr = horizontal padding left/right
	// paddingx = horizontal padding between childs
	int paddingxlr, paddingx, paddingy;
	Background *bg;

	// Backend state:
	timeout *timer;
	int child_pipe;
	pid_t child;

	// Command output buffer
	char *buf_output;
	int buf_length;
	int buf_capacity;

	// Text extracted from the output buffer
	char *text;
	// Icon path extracted from the output buffer
	char *icon_path;
	Imlib_Image icon;
	char tooltip_text[512];

	// The time the last command was started
	time_t last_update_start_time;
	// The time the last output was obtained
	time_t last_update_finish_time;
	// The time it took to execute last command
	time_t last_update_duration;

	// List of Execp which are frontends for this backend, one for each panel
	GList *instances;
} ExecpBackend;

typedef struct ExecpFrontend {
	// Frontend state:
	int iconx;
	int icony;
	int textx;
	int texty;
	int textw;
	int texth;
} ExecpFrontend;

typedef struct Execp {
	Area area;
	// All elements have the backend pointer set. However only backend elements have ownership.
	ExecpBackend *backend;
	// Set only for frontend Execp items.
	ExecpFrontend *frontend;
} Execp;

// Called before the config is read and panel_config/panels are created.
// Afterwards, the config parsing code creates the array of Execp in panel_config and populates the configuration fields
// in the backend.
// Probably does nothing.
void default_execp();

// Creates a new Execp item with only the backend field set. The state is NOT initialized. The config is initialized to
// the default values.
// This will be used by the config code to populate its backedn config fields.
Execp *create_execp();

void destroy_execp(void *obj);

// Called after the config is read and panel_config is populated, but before panels are created.
// Initializes the state of the backend items.
// panel_config.panel_items is used to determine which backend items are enabled. The others should be destroyed and
// removed from panel_config.execp_list.
void init_execp();

// Called after each on-screen panel is created, with a pointer to the panel.
// Initializes the state of the frontend items. Also adds a pointer to it in backend->instances.
// At this point the Area has not been added yet to the GUI tree, but it will be added right away.
void init_execp_panel(void *panel);

// Called just before the panels are destroyed. Afterwards, tint2 exits or restarts and reads the config again.
// Releases all frontends and then all the backends.
// The frontend items are not freed by this function, only their members. The items are Areas which are freed in the
// GUI element tree cleanup function (remove_area).
void cleanup_execp();

// Called on draw, obj = pointer to the front-end Execp item.
void draw_execp(void *obj, cairo_t *c);

// Called on resize, obj = pointer to the front-end Execp item.
// Returns 1 if the new size is different than the previous size.
gboolean resize_execp(void *obj);

// Called on mouse click event.
void execp_action(void *obj, int button, int x, int y);

// Called to check if new output from the command can be read.
// No command might be running.
// Returns 1 if the output has been updated and a redraw is needed.
gboolean read_execp(void *obj);

void execp_default_font_changed();

#endif // EXECPLUGIN_H
