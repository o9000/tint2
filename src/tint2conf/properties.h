
#ifndef PROPERTIES
#define PROPERTIES

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "../launcher/icon-theme-common.h"


// panel
extern GtkWidget *panel_width, *panel_height, *panel_margin_x, *panel_margin_y, *panel_padding_x, *panel_padding_y, *panel_spacing;
extern GtkWidget *panel_wm_menu, *panel_dock, *panel_autohide, *panel_autohide_show_time, *panel_autohide_hide_time, *panel_autohide_size;
extern GtkWidget *panel_combo_strut_policy, *panel_combo_layer, *panel_combo_width_type, *panel_combo_height_type, *panel_combo_monitor;

enum {
	itemsColName = 0,
	itemsColValue,
	itemsNumCols
};
extern GtkListStore *panel_items, *all_items;
extern GtkWidget *panel_items_view, *all_items_view;
char *get_panel_items();
void set_panel_items(const char *items);

extern GtkWidget *screen_position[12];
extern GSList *screen_position_group;
extern GtkWidget *panel_background;

#define POS_TLH 0
#define POS_TCH 1
#define POS_TRH 2

#define POS_TLV 3
#define POS_CLV 4
#define POS_BLV 5

#define POS_TRV 6
#define POS_CRV 7
#define POS_BRV 8

#define POS_BLH 9
#define POS_BCH 10
#define POS_BRH 11

// taskbar
extern GtkWidget *taskbar_show_desktop, *taskbar_show_name, *taskbar_padding_x, *taskbar_padding_y, *taskbar_spacing;
extern GtkWidget *taskbar_hide_inactive_tasks, *taskbar_hide_diff_monitor;
extern GtkWidget *taskbar_name_padding_x, *taskbar_name_inactive_color, *taskbar_name_active_color, *taskbar_name_font;
extern GtkWidget *taskbar_active_background, *taskbar_inactive_background;
extern GtkWidget *taskbar_name_active_background, *taskbar_name_inactive_background;

// task
extern GtkWidget *task_mouse_left, *task_mouse_middle, *task_mouse_right, *task_mouse_scroll_up, *task_mouse_scroll_down;
extern GtkWidget *task_show_icon, *task_show_text, *task_align_center, *task_font_shadow;
extern GtkWidget *task_maximum_width, *task_maximum_height, *task_padding_x, *task_padding_y, *task_font;
extern GtkWidget *task_default_color, *task_default_color_set,
		  *task_default_icon_opacity, *task_default_icon_osb_set,
		  *task_default_icon_saturation,
		  *task_default_icon_brightness,
		  *task_default_background, *task_default_background_set;
extern GtkWidget *task_normal_color, *task_normal_color_set,
		  *task_normal_icon_opacity, *task_normal_icon_osb_set,
		  *task_normal_icon_saturation,
		  *task_normal_icon_brightness,
		  *task_normal_background, *task_normal_background_set;
extern GtkWidget *task_active_color, *task_active_color_set,
		  *task_active_icon_opacity, *task_active_icon_osb_set,
		  *task_active_icon_saturation,
		  *task_active_icon_brightness,
		  *task_active_background, *task_active_background_set;
extern GtkWidget *task_urgent_color, *task_urgent_color_set,
		  *task_urgent_icon_opacity, *task_urgent_icon_osb_set,
		  *task_urgent_icon_saturation,
		  *task_urgent_icon_brightness,
		  *task_urgent_background, *task_urgent_background_set;
extern GtkWidget *task_urgent_blinks;
extern GtkWidget *task_iconified_color, *task_iconified_color_set,
		  *task_iconified_icon_opacity, *task_iconified_icon_osb_set,
		  *task_iconified_icon_saturation,
		  *task_iconified_icon_brightness,
		  *task_iconified_background, *task_iconified_background_set;

// clock
extern GtkWidget *clock_format_line1, *clock_format_line2, *clock_tmz_line1, *clock_tmz_line2;
extern GtkWidget *clock_left_command, *clock_right_command;
extern GtkWidget *clock_padding_x, *clock_padding_y, *clock_font_line1, *clock_font_line2, *clock_font_color;
extern GtkWidget *clock_background;

// battery
extern GtkWidget *battery_hide_if_higher, *battery_alert_if_lower, *battery_alert_cmd;
extern GtkWidget *battery_padding_x, *battery_padding_y, *battery_font_line1, *battery_font_line2, *battery_font_color;
extern GtkWidget *battery_background;

// systray
extern GtkWidget *systray_icon_order, *systray_padding_x, *systray_padding_y, *systray_spacing;
extern GtkWidget *systray_icon_size, *systray_icon_opacity, *systray_icon_saturation, *systray_icon_brightness;
extern GtkWidget *systray_background;

// tooltip
extern GtkWidget *tooltip_padding_x, *tooltip_padding_y, *tooltip_font, *tooltip_font_color;
extern GtkWidget *tooltip_task_show, *tooltip_show_after, *tooltip_hide_after;
extern GtkWidget *clock_format_tooltip, *clock_tmz_tooltip;
extern GtkWidget *tooltip_background;

// launcher

enum {
	appsColIcon = 0,
	appsColIconName,
	appsColText,
	appsColPath,
	appsNumCols
};

extern GtkListStore *launcher_apps, *all_apps;
extern GtkWidget *launcher_apps_view, *all_apps_view;
extern GtkWidget *launcher_apps_dirs;

extern GtkWidget *launcher_icon_size, *launcher_icon_theme, *launcher_padding_x, *launcher_padding_y, *launcher_spacing;
extern GtkWidget *margin_x, *margin_y;
extern GtkWidget *launcher_background;

extern IconThemeWrapper *icon_theme;

void load_desktop_file(const char *file, gboolean selected);
void set_current_icon_theme(const char *theme);
gchar *get_current_icon_theme();

// background
enum {
	bgColPixbuf = 0,
	bgColFillColor,
	bgColFillOpacity,
	bgColBorderColor,
	bgColBorderOpacity,
	bgColBorderWidth,
	bgColCornerRadius,
	bgNumCols
};

extern GtkListStore *backgrounds;
extern GtkWidget *current_background,
		  *background_fill_color,
		  *background_border_color,
		  *background_border_width,
		  *background_corner_radius;

void background_create_new();
void background_force_update();
int background_index_safe(int index);

GtkWidget *create_properties();

#endif
