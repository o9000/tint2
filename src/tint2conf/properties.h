
#ifndef PROPERTIES
#define PROPERTIES

#include <gtk/gtk.h>

// panel
GtkWidget  *panel_width, *panel_height, *panel_margin_x, *panel_margin_y, *panel_padding_x, *panel_padding_y, *panel_spacing;
GtkWidget  *panel_wm_menu, *panel_dock, *panel_autohide, *panel_autohide_show_time, *panel_autohide_hide_time, *panel_autohide_size;
GtkWidget  *panel_combo_strut_policy, *panel_combo_layer, *panel_combo_width_type, *panel_combo_height_type, *panel_combo_monitor;
GtkWidget  *items_order;

// taskbar
GtkWidget  *taskbar_show_desktop, *taskbar_show_name, *taskbar_padding_x, *taskbar_padding_y, *taskbar_spacing;
GtkWidget  *taskbar_name_padding_x, *taskbar_name_inactive_color, *taskbar_name_active_color, *taskbar_name_font;

// task
GtkWidget  *task_mouse_middle, *task_mouse_right, *task_mouse_scroll_up,  *task_mouse_scroll_down;
GtkWidget  *task_show_icon, *task_show_text, *task_align_center,  *task_font_shadow;
GtkWidget  *task_maximum_width, *task_maximum_height, *task_padding_x, *task_padding_y, *task_font;

// clock
GtkWidget  *clock_format_line1, *clock_format_line2, *clock_tmz_line1, *clock_tmz_line2;
GtkWidget  *clock_left_command, *clock_right_command;
GtkWidget  *clock_padding_x, *clock_padding_y, *clock_font_line1, *clock_font_line2, *clock_font_color;

// battery
GtkWidget  *battery_hide_if_higher, *battery_alert_if_lower, *battery_alert_cmd;
GtkWidget  *battery_padding_x, *battery_padding_y, *battery_font_line1, *battery_font_line2, *battery_font_color;

// systray
GtkWidget  *systray_icon_order, *systray_padding_x, *systray_padding_y, *systray_spacing;
GtkWidget  *systray_icon_size, *systray_icon_opacity, *systray_icon_saturation, *systray_icon_brightness;

// tooltip
GtkWidget  *tooltip_padding_x, *tooltip_padding_y, *tooltip_font, *tooltip_font_color;
GtkWidget  *tooltip_task_show, *tooltip_show_after, *tooltip_hide_after;
GtkWidget  *clock_format_tooltip, *clock_tmz_tooltip;

// launcher
GtkWidget  *launcher_icon_size, *launcher_icon_theme, *launcher_padding_x, *launcher_padding_y, *launcher_spacing;

// background
GtkWidget  *combo_background;
GtkWidget  *margin_x, *margin_y;

GtkWidget *create_properties();


#endif

