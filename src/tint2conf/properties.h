
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
extern GtkWidget *panel_window_name, *disable_transparency;
extern GtkWidget *panel_mouse_effects;
extern GtkWidget *mouse_hover_icon_opacity, *mouse_hover_icon_saturation, *mouse_hover_icon_brightness;
extern GtkWidget *mouse_pressed_icon_opacity, *mouse_pressed_icon_saturation, *mouse_pressed_icon_brightness;
extern GtkWidget *panel_primary_monitor_first;

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
extern GtkWidget *taskbar_name_padding_x, *taskbar_name_padding_y, *taskbar_name_inactive_color, *taskbar_name_active_color;
extern GtkWidget *taskbar_name_font, *taskbar_name_font_set;
extern GtkWidget *taskbar_active_background, *taskbar_inactive_background;
extern GtkWidget *taskbar_name_active_background, *taskbar_name_inactive_background;
extern GtkWidget *taskbar_distribute_size, *taskbar_sort_order, *taskbar_alignment, *taskbar_always_show_all_desktop_tasks;

// task
extern GtkWidget *task_mouse_left, *task_mouse_middle, *task_mouse_right, *task_mouse_scroll_up, *task_mouse_scroll_down;
extern GtkWidget *task_show_icon, *task_show_text, *task_align_center, *font_shadow;
extern GtkWidget *task_maximum_width, *task_maximum_height, *task_padding_x, *task_padding_y, *task_spacing;
extern GtkWidget *task_font, *task_font_set;
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
extern GtkWidget *clock_mclick_command, *clock_rclick_command, *clock_uwheel_command, *clock_dwheel_command;
extern GtkWidget *clock_padding_x, *clock_padding_y;
extern GtkWidget *clock_font_line1, *clock_font_line1_set, *clock_font_line2, *clock_font_line2_set, *clock_font_color;
extern GtkWidget *clock_background;

// battery
extern GtkWidget *battery_hide_if_higher, *battery_alert_if_lower, *battery_alert_cmd;
extern GtkWidget *battery_padding_x, *battery_padding_y;
extern GtkWidget *battery_font_line1, *battery_font_line1_set, *battery_font_line2, *battery_font_line2_set, *battery_font_color;
extern GtkWidget *battery_background;
extern GtkWidget *battery_tooltip;
extern GtkWidget *battery_left_command, *battery_mclick_command, *battery_right_command, *battery_uwheel_command, *battery_dwheel_command;
extern GtkWidget *ac_connected_cmd, *ac_disconnected_cmd;

// systray
extern GtkWidget *systray_icon_order, *systray_padding_x, *systray_padding_y, *systray_spacing;
extern GtkWidget *systray_icon_size, *systray_icon_opacity, *systray_icon_saturation, *systray_icon_brightness;
extern GtkWidget *systray_background, *systray_monitor;

// tooltip
extern GtkWidget *tooltip_padding_x, *tooltip_padding_y, *tooltip_font, *tooltip_font_set, *tooltip_font_color;
extern GtkWidget *tooltip_task_show, *tooltip_show_after, *tooltip_hide_after;
extern GtkWidget *clock_format_tooltip, *clock_tmz_tooltip;
extern GtkWidget *tooltip_background;

// Executor
typedef struct Executor {
	char name[256];
	GtkWidget *container;
	GtkWidget *page_execp;
	GtkWidget *page_label;
	GtkWidget *execp_command, *execp_interval, *execp_has_icon, *execp_cache_icon, *execp_show_tooltip;
	GtkWidget *execp_continuous, *execp_markup, *execp_tooltip;
	GtkWidget *execp_left_command, *execp_right_command;
	GtkWidget *execp_mclick_command, *execp_rclick_command, *execp_uwheel_command, *execp_dwheel_command;
	GtkWidget *execp_font, *execp_font_set, *execp_font_color, *execp_padding_x, *execp_padding_y, *execp_centered;
	GtkWidget *execp_background, *execp_icon_w, *execp_icon_h;
} Executor;

extern GArray *executors;

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
extern GtkWidget *launcher_icon_opacity, *launcher_icon_saturation, *launcher_icon_brightness;
extern GtkWidget *margin_x, *margin_y;
extern GtkWidget *launcher_background, *launcher_icon_background;
extern GtkWidget *startup_notifications;
extern IconThemeWrapper *icon_theme;
extern GtkWidget *launcher_tooltip;
extern GtkWidget *launcher_icon_theme_override;

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
	bgColText,
	bgColFillColorOver,
	bgColFillOpacityOver,
	bgColBorderColorOver,
	bgColBorderOpacityOver,
	bgColFillColorPress,
	bgColFillOpacityPress,
	bgColBorderColorPress,
	bgColBorderOpacityPress,
	bgNumCols
};

extern GtkListStore *backgrounds;
extern GtkWidget *current_background,
		  *background_fill_color,
		  *background_border_color,
		  *background_fill_color_over,
		  *background_border_color_over,
		  *background_fill_color_press,
		  *background_border_color_press,
		  *background_border_width,
		  *background_corner_radius;

void background_create_new();
void background_force_update();
int background_index_safe(int index);

GtkWidget *create_properties();

void execp_create_new();
Executor *execp_get_last();
void execp_remove(int i);
void execp_update_indices();

#endif
