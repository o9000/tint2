#ifndef BUTTON_H
#define BUTTON_H

#include <sys/time.h>
#include <pango/pangocairo.h>

#include "area.h"
#include "common.h"
#include "timer.h"

// Architecture:
// Panel panel_config contains an array of Button, each storing all config options and all the state variables.
// Only these run commands.
//
// Tint2 maintains an array of Panels, one for each monitor. Each stores an array of Button which was initially copied
// from panel_config. Each works as a frontend to the corresponding Button in panel_config as backend, using the
// backend's config and state variables.

typedef struct ButtonBackend {
    // Config:
    char *icon_name;
    char *text;
    char *tooltip;
    gboolean centered;
    int max_icon_size;
    gboolean has_font;
    PangoFontDescription *font_desc;
    Color font_color;
    char *lclick_command;
    char *mclick_command;
    char *rclick_command;
    char *uwheel_command;
    char *dwheel_command;
    // paddingxlr = horizontal padding left/right
    // paddingx = horizontal padding between childs
    int paddingxlr, paddingx, paddingy;
    Background *bg;

    // List of Button which are frontends for this backend, one for each panel
    GList *instances;
} ButtonBackend;

typedef struct ButtonFrontend {
    // Frontend state:
    Imlib_Image icon;
    Imlib_Image icon_hover;
    Imlib_Image icon_pressed;
    int icon_load_size;
    int iconx;
    int icony;
    int iconw;
    int iconh;
    int textx;
    int texty;
    int textw;
    int texth;
} ButtonFrontend;

typedef struct Button {
    Area area;
    // All elements have the backend pointer set. However only backend elements have ownership.
    ButtonBackend *backend;
    // Set only for frontend Button items.
    ButtonFrontend *frontend;
} Button;

// Called before the config is read and panel_config/panels are created.
// Afterwards, the config parsing code creates the array of Button in panel_config and populates the configuration
// fields
// in the backend.
// Probably does nothing.
void default_button();

// Creates a new Button item with only the backend field set. The state is NOT initialized. The config is initialized to
// the default values.
// This will be used by the config code to populate its backedn config fields.
Button *create_button();

void destroy_button(void *obj);

// Called after the config is read and panel_config is populated, but before panels are created.
// Initializes the state of the backend items.
// panel_config.panel_items is used to determine which backend items are enabled. The others should be destroyed and
// removed from panel_config.button_list.
void init_button();

// Called after each on-screen panel is created, with a pointer to the panel.
// Initializes the state of the frontend items. Also adds a pointer to it in backend->instances.
// At this point the Area has not been added yet to the GUI tree, but it will be added right away.
void init_button_panel(void *panel);

// Called just before the panels are destroyed. Afterwards, tint2 exits or restarts and reads the config again.
// Releases all frontends and then all the backends.
// The frontend items are not freed by this function, only their members. The items are Areas which are freed in the
// GUI element tree cleanup function (remove_area).
void cleanup_button();

// Called on draw, obj = pointer to the front-end Button item.
void draw_button(void *obj, cairo_t *c);

// Called on resize, obj = pointer to the front-end Button item.
// Returns 1 if the new size is different than the previous size.
gboolean resize_button(void *obj);

// Called on mouse click event.
void button_action(void *obj, int button, int x, int y);

void button_default_font_changed();
void button_default_icon_theme_changed();

void button_reload_icon(Button *button);

#endif // BUTTON_H
