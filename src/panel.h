/**************************************************************************
* Copyright (C) 2008 Pål Staurland (staura@gmail.com)
* Modified (C) 2008/2009 thierry lorthiois (lorthiois@bbsoft.fr)
*
* panel :
* - draw panel and all objects according to panel_layout
*
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
#include "systraybar.h"
#include "launcher.h"
#include "freespace.h"
#include "execplugin.h"
#include "separator.h"
#include "button.h"

#ifdef ENABLE_BATTERY
#include "battery.h"
#endif

extern int signal_pending;
// --------------------------------------------------
// mouse events
extern MouseAction mouse_left;
extern MouseAction mouse_middle;
extern MouseAction mouse_right;
extern MouseAction mouse_scroll_up;
extern MouseAction mouse_scroll_down;
extern MouseAction mouse_tilt_left;
extern MouseAction mouse_tilt_right;

// panel mode
typedef enum TaskbarMode {
    SINGLE_DESKTOP = 0,
    MULTI_DESKTOP,
} TaskbarMode;

typedef enum Layer {
    BOTTOM_LAYER,
    NORMAL_LAYER,
    TOP_LAYER,
} Layer;

// panel position
typedef enum PanelPosition {
    LEFT = 0x01,
    RIGHT = 0x02,
    CENTER = 0X04,
    TOP = 0X08,
    BOTTOM = 0x10,
} PanelPosition;

typedef enum Strut {
    STRUT_MINIMUM,
    STRUT_FOLLOW_SIZE,
    STRUT_NONE,
} Strut;

extern TaskbarMode taskbar_mode;
extern gboolean wm_menu;
extern gboolean panel_dock;
extern Layer panel_layer;
extern char *panel_window_name;
extern PanelPosition panel_position;
extern gboolean panel_horizontal;
extern gboolean panel_refresh;
extern gboolean task_dragged;
extern gboolean panel_autohide;
extern int panel_autohide_show_timeout;
extern int panel_autohide_hide_timeout;
extern int panel_autohide_height; // for vertical panels this is of course the width
extern gboolean panel_shrink;
extern Strut panel_strut_policy;
extern char *panel_items_order;
extern int max_tick_urgent;
extern GArray *backgrounds;
extern GArray *gradients;
extern Imlib_Image default_icon;
#define DEFAULT_FONT "sans 10"
extern char *default_font;
extern XSettingsClient *xsettings_client;
extern gboolean startup_notifications;
extern gboolean debug_geometry;
extern gboolean debug_fps;
extern gboolean debug_frames;

typedef struct Panel {
    Area area;

    Window main_win;
    Pixmap temp_pmap;

    // position relative to root window
    int posx, posy;
    int marginx, marginy;
    gboolean fractional_width, fractional_height;
    int max_size;
    int monitor;
    int font_shadow;
    gboolean mouse_effects;
    // Mouse effects for icons
    int mouse_over_alpha;
    int mouse_over_saturation;
    int mouse_over_brightness;
    int mouse_pressed_alpha;
    int mouse_pressed_saturation;
    int mouse_pressed_brightness;

    // Per-panel parameters and states for Taskbar and Task
    GlobalTaskbar g_taskbar;
    GlobalTask g_task;

    // Array of Taskbar, with num_desktops items
    Taskbar *taskbar;
    int num_desktops;
    gboolean taskbarname_has_font;
    PangoFontDescription *taskbarname_font_desc;

    Clock clock;

#ifdef ENABLE_BATTERY
    Battery battery;
#endif

    Launcher launcher;
    GList *freespace_list;
    GList *separator_list;
    GList *execp_list;
    GList *button_list;

    // Autohide
    gboolean is_hidden;
    int hidden_width, hidden_height;
    Pixmap hidden_pixmap;
    timeout *autohide_timeout;
} Panel;

extern Panel panel_config;
extern Panel *panels;
extern int num_panels;

// default global data
void default_panel();

// freed memory
void cleanup_panel();

// realloc panels according to number of monitor
// use panel_config as default value
void init_panel();

void init_panel_size_and_position(Panel *panel);
gboolean resize_panel(void *obj);
void render_panel(Panel *panel);
void shrink_panel(Panel *panel);
void _schedule_panel_redraw(const char *file, const char *function, const int line);
#define schedule_panel_redraw() _schedule_panel_redraw(__FILE__, __FUNCTION__, __LINE__)

void set_panel_items_order(Panel *p);
void place_panel_all_desktops(Panel *p);
void replace_panel_all_desktops(Panel *p);
void set_panel_properties(Panel *p);
void set_panel_window_geometry(Panel *panel);
void set_panel_layer(Panel *p, Layer layer);

// draw background panel
void set_panel_background(Panel *p);

// detect witch panel
Panel *get_panel(Window win);

Taskbar *click_taskbar(Panel *panel, int x, int y);
Task *click_task(Panel *panel, int x, int y);
Launcher *click_launcher(Panel *panel, int x, int y);
LauncherIcon *click_launcher_icon(Panel *panel, int x, int y);
Clock *click_clock(Panel *panel, int x, int y);

#ifdef ENABLE_BATTERY
Battery *click_battery(Panel *panel, int x, int y);
#endif

Area *click_area(Panel *panel, int x, int y);
Execp *click_execp(Panel *panel, int x, int y);
Button *click_button(Panel *panel, int x, int y);

void autohide_show(void *p);
void autohide_hide(void *p);
void autohide_trigger_show(Panel *p);
void autohide_trigger_hide(Panel *p);

const char *get_default_font();

void default_icon_theme_changed();
void default_font_changed();

void free_icon(Imlib_Image icon);
Imlib_Image scale_icon(Imlib_Image original, int icon_size);

#endif
