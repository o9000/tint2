
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

// clock

// battery

// systray
GtkWidget  *systray_icon_order;

// tooltip

// launcher

// background
GtkWidget  *combo_background;
GtkWidget  *margin_x, *margin_y;

GtkWidget *create_properties();


#endif

