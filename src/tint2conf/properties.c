/**************************************************************************
*
* Tint2conf
*
* Copyright (C) 2009 Thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************/

#include "main.h"
#include "properties.h"
#include "properties_rw.h"
#include "../launcher/apps-common.h"
#include "../launcher/icon-theme-common.h"
#include "../util/common.h"

#define ROW_SPACING 10
#define COL_SPACING 8
#define DEFAULT_HOR_SPACING 5


GtkWidget *panel_width, *panel_height, *panel_margin_x, *panel_margin_y, *panel_padding_x, *panel_padding_y, *panel_spacing;
GtkWidget *panel_wm_menu, *panel_dock, *panel_autohide, *panel_autohide_show_time, *panel_autohide_hide_time, *panel_autohide_size;
GtkWidget *panel_combo_strut_policy, *panel_combo_layer, *panel_combo_width_type, *panel_combo_height_type, *panel_combo_monitor;
GtkWidget *panel_window_name, *disable_transparency;

GtkListStore *panel_items, *all_items;
GtkWidget *panel_items_view, *all_items_view;

GtkWidget *screen_position[12];
GSList *screen_position_group;
GtkWidget *panel_background;

// taskbar
GtkWidget *taskbar_show_desktop, *taskbar_show_name, *taskbar_padding_x, *taskbar_padding_y, *taskbar_spacing;
GtkWidget *taskbar_hide_inactive_tasks, *taskbar_hide_diff_monitor;
GtkWidget *taskbar_name_padding_x, *taskbar_name_padding_y, *taskbar_name_inactive_color, *taskbar_name_active_color, *taskbar_name_font;
GtkWidget *taskbar_active_background, *taskbar_inactive_background;
GtkWidget *taskbar_name_active_background, *taskbar_name_inactive_background;
GtkWidget *taskbar_distribute_size, *taskbar_sort_order;

// task
GtkWidget *task_mouse_left, *task_mouse_middle, *task_mouse_right, *task_mouse_scroll_up, *task_mouse_scroll_down;
GtkWidget *task_show_icon, *task_show_text, *task_align_center, *font_shadow;
GtkWidget *task_maximum_width, *task_maximum_height, *task_padding_x, *task_padding_y, *task_spacing, *task_font;
GtkWidget *task_default_color, *task_default_color_set,
		  *task_default_icon_opacity, *task_default_icon_osb_set,
		  *task_default_icon_saturation,
		  *task_default_icon_brightness,
		  *task_default_background, *task_default_background_set;
GtkWidget *task_normal_color, *task_normal_color_set,
		  *task_normal_icon_opacity, *task_normal_icon_osb_set,
		  *task_normal_icon_saturation,
		  *task_normal_icon_brightness,
		  *task_normal_background, *task_normal_background_set;
GtkWidget *task_active_color, *task_active_color_set,
		  *task_active_icon_opacity, *task_active_icon_osb_set,
		  *task_active_icon_saturation,
		  *task_active_icon_brightness,
		  *task_active_background, *task_active_background_set;
GtkWidget *task_urgent_color, *task_urgent_color_set,
		  *task_urgent_icon_opacity, *task_urgent_icon_osb_set,
		  *task_urgent_icon_saturation,
		  *task_urgent_icon_brightness,
		  *task_urgent_background, *task_urgent_background_set;
GtkWidget *task_urgent_blinks;
GtkWidget *task_iconified_color, *task_iconified_color_set,
		  *task_iconified_icon_opacity, *task_iconified_icon_osb_set,
		  *task_iconified_icon_saturation,
		  *task_iconified_icon_brightness,
		  *task_iconified_background, *task_iconified_background_set;

// clock
GtkWidget *clock_format_line1, *clock_format_line2, *clock_tmz_line1, *clock_tmz_line2;
GtkWidget *clock_left_command, *clock_right_command;
GtkWidget *clock_padding_x, *clock_padding_y, *clock_font_line1, *clock_font_line2, *clock_font_color;
GtkWidget *clock_background;

// battery
GtkWidget *battery_hide_if_higher, *battery_alert_if_lower, *battery_alert_cmd;
GtkWidget *battery_padding_x, *battery_padding_y, *battery_font_line1, *battery_font_line2, *battery_font_color;
GtkWidget *battery_background;

// systray
GtkWidget *systray_icon_order, *systray_padding_x, *systray_padding_y, *systray_spacing;
GtkWidget *systray_icon_size, *systray_icon_opacity, *systray_icon_saturation, *systray_icon_brightness;
GtkWidget *systray_background, *systray_monitor;

// tooltip
GtkWidget *tooltip_padding_x, *tooltip_padding_y, *tooltip_font, *tooltip_font_color;
GtkWidget *tooltip_task_show, *tooltip_show_after, *tooltip_hide_after;
GtkWidget *clock_format_tooltip, *clock_tmz_tooltip;
GtkWidget *tooltip_background;

// launcher

GtkListStore *launcher_apps, *all_apps;
GtkWidget *launcher_apps_view, *all_apps_view;
GtkWidget *launcher_apps_dirs;

GtkWidget *launcher_icon_size, *launcher_icon_theme, *launcher_padding_x, *launcher_padding_y, *launcher_spacing;
GtkWidget *launcher_icon_opacity, *launcher_icon_saturation, *launcher_icon_brightness;
GtkWidget *margin_x, *margin_y;
GtkWidget *launcher_background;
GtkWidget *startup_notifications;
IconThemeWrapper *icon_theme;
GtkWidget *launcher_tooltip;

GtkListStore *backgrounds;
GtkWidget *current_background,
		  *background_fill_color,
		  *background_border_color,
		  *background_border_width,
		  *background_corner_radius;


GtkWidget *addScrollBarToWidget(GtkWidget *widget);
gboolean gtk_tree_model_iter_prev_tint2(GtkTreeModel *model, GtkTreeIter *iter);

void change_paragraph(GtkWidget *widget);
void create_general(GtkWidget *parent);
void create_background(GtkWidget *parent);
void background_duplicate(GtkWidget *widget, gpointer data);
void background_delete(GtkWidget *widget, gpointer data);
void background_update_image(int index);
void background_update(GtkWidget *widget, gpointer data);
void current_background_changed(GtkWidget *widget, gpointer data);
void create_panel(GtkWidget *parent);
void create_panel_items(GtkWidget *parent);
void create_launcher(GtkWidget *parent);
gchar *get_default_theme_name();
void icon_theme_changed();
void load_icons(GtkListStore *apps);
void create_taskbar(GtkWidget *parent);
void create_task(GtkWidget *parent);
void create_task_status(GtkWidget *notebook,
						char *name,
						GtkWidget **task_status_color,
						GtkWidget **task_status_color_set,
						GtkWidget **task_status_icon_opacity,
						GtkWidget **task_status_icon_osb_set,
						GtkWidget **task_status_icon_saturation,
						GtkWidget **task_status_icon_brightness,
						GtkWidget **task_status_background,
						GtkWidget **task_status_background_set);
void create_clock(GtkWidget *parent);
void create_systemtray(GtkWidget *parent);
void create_battery(GtkWidget *parent);
void create_tooltip(GtkWidget *parent);
void panel_add_item(GtkWidget *widget, gpointer data);
void panel_remove_item(GtkWidget *widget, gpointer data);
void panel_move_item_down(GtkWidget *widget, gpointer data);
void panel_move_item_up(GtkWidget *widget, gpointer data);

void applyClicked(GtkWidget *widget, gpointer data)
{
	gchar *file = get_current_theme_file_name();
	if (file) {
		if (config_is_manual(file)) {
			gchar *backup_path = g_strdup_printf("%s.backup.%ld", file, time(NULL));
			copy_file(file, backup_path);
			g_free(backup_path);
		}

		config_save_file(file);
	}
	int unused = system("killall -SIGUSR1 tint2");
	(void)unused;
	g_free(file);
	g_timeout_add(SNAPSHOT_TICK, (GSourceFunc)update_snapshot, NULL);
}

void cancelClicked(GtkWidget *widget, gpointer data)
{
	GtkWidget *view = (GtkWidget *)data;
	gtk_widget_destroy(view);
}

void okClicked(GtkWidget *widget, gpointer data)
{
	applyClicked(widget, data);
	cancelClicked(widget, data);
}

GtkWidget *create_properties()
{
	GtkWidget *view, *dialog_vbox3, *button, *notebook;
	GtkTooltips *tooltips;
	GtkWidget *page_panel, *page_panel_items, *page_launcher, *page_taskbar, *page_battery, *page_clock,
			  *page_tooltip, *page_systemtray, *page_task, *page_background;
	GtkWidget *label;

	tooltips = gtk_tooltips_new();
	(void) tooltips;

	// global layer
	view = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(view), _("Properties"));
	gtk_window_set_modal(GTK_WINDOW(view), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(view), 800, 600);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(view), TRUE);
	gtk_window_set_type_hint(GTK_WINDOW(view), GDK_WINDOW_TYPE_HINT_DIALOG);

	dialog_vbox3 = GTK_DIALOG(view)->vbox;
	gtk_widget_show(dialog_vbox3);

	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);
	gtk_container_set_border_width(GTK_CONTAINER(notebook), 5);
	gtk_box_pack_start(GTK_BOX(dialog_vbox3), notebook, TRUE, TRUE, 6);
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_LEFT);

	button = gtk_button_new_from_stock("gtk-apply");
	gtk_widget_show(button);
	gtk_dialog_add_action_widget(GTK_DIALOG(view), button, GTK_RESPONSE_APPLY);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(applyClicked), NULL);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);

	button = gtk_button_new_from_stock("gtk-cancel");
	gtk_widget_show(button);
	gtk_dialog_add_action_widget(GTK_DIALOG(view), button, GTK_RESPONSE_CANCEL);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(cancelClicked), view);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);

	button = gtk_button_new_from_stock("gtk-ok");
	gtk_widget_show(button);
	gtk_dialog_add_action_widget(GTK_DIALOG(view), button, GTK_RESPONSE_OK);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(okClicked), view);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);

	// notebook
	label = gtk_label_new(_("Backgrounds"));
	gtk_widget_show(label);
	page_background = gtk_vbox_new(FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_background), 10);
	gtk_widget_show(page_background);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), addScrollBarToWidget(page_background), label);
	create_background(page_background);

	label = gtk_label_new(_("Panel"));
	gtk_widget_show(label);
	page_panel = gtk_vbox_new(FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_panel), 10);
	gtk_widget_show(page_panel);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), addScrollBarToWidget(page_panel), label);
	create_panel(page_panel);

	label = gtk_label_new(_("Panel items"));
	gtk_widget_show(label);
	page_panel_items = gtk_vbox_new(FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_panel_items), 10);
	gtk_widget_show(page_panel_items);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), addScrollBarToWidget(page_panel_items), label);
	create_panel_items(page_panel_items);

	label = gtk_label_new(_("Launcher"));
	gtk_widget_show(label);
	page_launcher = gtk_vbox_new(FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_launcher), 10);
	gtk_widget_show(page_launcher);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), addScrollBarToWidget(page_launcher), label);
	create_launcher(page_launcher);

	label = gtk_label_new(_("Taskbar"));
	gtk_widget_show(label);
	page_taskbar = gtk_vbox_new(FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_taskbar), 10);
	gtk_widget_show(page_taskbar);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), addScrollBarToWidget(page_taskbar), label);
	create_taskbar(page_taskbar);

	label = gtk_label_new(_("Task buttons"));
	gtk_widget_show(label);
	page_task = gtk_vbox_new(FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_task), 10);
	gtk_widget_show(page_task);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), addScrollBarToWidget(page_task), label);
	create_task(page_task);

	label = gtk_label_new(_("Clock"));
	gtk_widget_show(label);
	page_clock = gtk_vbox_new(FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_clock), 10);
	gtk_widget_show(page_clock);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), addScrollBarToWidget(page_clock), label);
	create_clock(page_clock);

	label = gtk_label_new(_("System tray"));
	gtk_widget_show(label);
	page_systemtray = gtk_vbox_new(FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_systemtray), 10);
	gtk_widget_show(page_systemtray);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), addScrollBarToWidget(page_systemtray), label);
	create_systemtray(page_systemtray);

	label = gtk_label_new(_("Battery"));
	gtk_widget_show(label);
	page_battery = gtk_vbox_new(FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_battery), 10);
	gtk_widget_show(page_battery);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), addScrollBarToWidget(page_battery), label);
	create_battery(page_battery);

	label = gtk_label_new(_("Tooltip"));
	gtk_widget_show(label);
	page_tooltip = gtk_vbox_new(FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_tooltip), 10);
	gtk_widget_show(page_tooltip);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), addScrollBarToWidget(page_tooltip), label);
	create_tooltip(page_tooltip);

	return view;
}

void change_paragraph(GtkWidget *widget)
{
	GtkWidget *hbox;
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(widget), hbox, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
}

GtkWidget *create_background_combo()
{
	GtkWidget *combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(backgrounds));
	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "pixbuf", 0, NULL);
	return combo;
}

void gdkColor2CairoColor(GdkColor color, double *red, double *green, double *blue)
{
	*red = color.red / (double)0xffff;
	*green = color.green / (double)0xffff;
	*blue = color.blue / (double)0xffff;
}

void cairoColor2GdkColor(double red, double green, double blue, GdkColor *color)
{
	color->pixel = 0;
	color->red = red * 0xffff;
	color->green = green * 0xffff;
	color->blue = blue * 0xffff;
}

void create_background(GtkWidget *parent)
{
	backgrounds = gtk_list_store_new(bgNumCols,
									 GDK_TYPE_PIXBUF,
									 GDK_TYPE_COLOR,
									 GTK_TYPE_INT,
									 GDK_TYPE_COLOR,
									 GTK_TYPE_INT,
									 GTK_TYPE_INT,
									 GTK_TYPE_INT);

	GtkWidget *table, *label, *button;
	int row, col;
	GtkTooltips *tooltips = gtk_tooltips_new();

	table = gtk_table_new(1, 4, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

	row = 0, col = 0;
	label = gtk_label_new(_("<b>Background</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	current_background = create_background_combo();
	gtk_widget_show(current_background);
	gtk_table_attach(GTK_TABLE(table), current_background, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, current_background, "Selects the background you would like to modify", NULL);

	button = gtk_button_new_from_stock("gtk-add");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(background_duplicate), NULL);
	gtk_widget_show(button);
	gtk_table_attach(GTK_TABLE(table), button, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, button, "Creates a copy of the current background", NULL);

	button = gtk_button_new_from_stock("gtk-remove");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(background_delete), NULL);
	gtk_widget_show(button);
	gtk_table_attach(GTK_TABLE(table), button, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, button, "Deletes the current background", NULL);

	table = gtk_table_new(4, 4, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

	row++, col = 2;
	label = gtk_label_new(_("Fill color"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	background_fill_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(background_fill_color), TRUE);
	gtk_widget_show(background_fill_color);
	gtk_table_attach(GTK_TABLE(table), background_fill_color, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, background_fill_color, "The fill color of the current background", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Border color"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	background_border_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(background_border_color), TRUE);
	gtk_widget_show(background_border_color);
	gtk_table_attach(GTK_TABLE(table), background_border_color, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, background_border_color, "The border color of the current background", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Border width"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	background_border_width = gtk_spin_button_new_with_range(0, 100, 1);
	gtk_widget_show(background_border_width);
	gtk_table_attach(GTK_TABLE(table), background_border_width, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, background_border_width, "The width of the border of the current background, in pixels", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Corner radius"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	background_corner_radius = gtk_spin_button_new_with_range(0, 100, 1);
	gtk_widget_show(background_corner_radius);
	gtk_table_attach(GTK_TABLE(table), background_corner_radius, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, background_corner_radius, "The corner radius of the current background", NULL);

	g_signal_connect(G_OBJECT(current_background), "changed", G_CALLBACK(current_background_changed), NULL);
	g_signal_connect(G_OBJECT(background_fill_color), "color-set", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_border_color), "color-set", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_border_width), "value-changed", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_corner_radius), "value-changed", G_CALLBACK(background_update), NULL);

	change_paragraph(parent);
}

int get_model_length(GtkTreeModel *model)
{
	int i;
	for (i = 0; ; i++) {
		GtkTreePath *path;
		GtkTreeIter iter;

		path = gtk_tree_path_new_from_indices(i, -1);
		gboolean end = gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_path_free(path);

		if (!end) {
			return i;
		}
	}
}

int background_index_safe(int index)
{
	if (index <= 0)
		index = 0;
	if (index >= get_model_length(GTK_TREE_MODEL(backgrounds)))
		index = 0;
	return index;
}

void background_create_new()
{
	int r = 0;
	int b = 0;
	GdkColor fillColor;
	cairoColor2GdkColor(0, 0, 0, &fillColor);
	int fillOpacity = 0;
	GdkColor borderColor;
	cairoColor2GdkColor(0, 0, 0, &borderColor);
	int borderOpacity = 0;

	int index = 0;
	GtkTreeIter iter;

	gtk_list_store_append(backgrounds, &iter);
	gtk_list_store_set(backgrounds, &iter,
					   bgColPixbuf, NULL,
					   bgColFillColor, &fillColor,
					   bgColFillOpacity, fillOpacity,
					   bgColBorderColor, &borderColor,
					   bgColBorderOpacity, borderOpacity,
					   bgColBorderWidth, b,
					   bgColCornerRadius, r,
					   -1);

	background_update_image(index);
	gtk_combo_box_set_active(GTK_COMBO_BOX(current_background), get_model_length(GTK_TREE_MODEL(backgrounds)) - 1);
}

void background_duplicate(GtkWidget *widget, gpointer data)
{
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_background));
	if (index < 0) {
		background_create_new();
		return;
	}

	GtkTreePath *path;
	GtkTreeIter iter;

	path = gtk_tree_path_new_from_indices(index, -1);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
	gtk_tree_path_free(path);

	int r;
	int b;
	GdkColor *fillColor;
	int fillOpacity;
	GdkColor *borderColor;
	int borderOpacity;

	gtk_tree_model_get(GTK_TREE_MODEL(backgrounds), &iter,
					   bgColFillColor, &fillColor,
					   bgColFillOpacity, &fillOpacity,
					   bgColBorderColor, &borderColor,
					   bgColBorderOpacity, &borderOpacity,
					   bgColBorderWidth, &b,
					   bgColCornerRadius, &r,
					   -1);

	gtk_list_store_append(backgrounds, &iter);
	gtk_list_store_set(backgrounds, &iter,
					   bgColPixbuf, NULL,
					   bgColFillColor, fillColor,
					   bgColFillOpacity, fillOpacity,
					   bgColBorderColor, borderColor,
					   bgColBorderOpacity, borderOpacity,
					   bgColBorderWidth, b,
					   bgColCornerRadius, r,
					   -1);
	g_boxed_free(GDK_TYPE_COLOR, fillColor);
	g_boxed_free(GDK_TYPE_COLOR, borderColor);
	background_update_image(get_model_length(GTK_TREE_MODEL(backgrounds)) - 1);
	gtk_combo_box_set_active(GTK_COMBO_BOX(current_background), get_model_length(GTK_TREE_MODEL(backgrounds)) - 1);
}

void background_delete(GtkWidget *widget, gpointer data)
{
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_background));
	if (index < 0)
		return;

	if (get_model_length(GTK_TREE_MODEL(backgrounds)) <= 1)
		return;

	GtkTreePath *path;
	GtkTreeIter iter;

	path = gtk_tree_path_new_from_indices(index, -1);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
	gtk_tree_path_free(path);

	gtk_list_store_remove(backgrounds, &iter);

	if (index == get_model_length(GTK_TREE_MODEL(backgrounds)))
		index--;
	gtk_combo_box_set_active(GTK_COMBO_BOX(current_background), index);
}

void background_update_image(int index)
{
	GtkTreePath *path;
	GtkTreeIter iter;

	path = gtk_tree_path_new_from_indices(index, -1);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
	gtk_tree_path_free(path);

	int w = 70;
	int h = 30;
	int r;
	int b;
	GdkPixbuf *pixbuf;
	GdkColor *fillColor;
	int fillOpacity = 50;
	GdkColor *borderColor;
	int borderOpacity = 100;

	gtk_tree_model_get(GTK_TREE_MODEL(backgrounds), &iter,
					   bgColFillColor, &fillColor,
					   bgColFillOpacity, &fillOpacity,
					   bgColBorderColor, &borderColor,
					   bgColBorderOpacity, &borderOpacity,
					   bgColBorderWidth, &b,
					   bgColCornerRadius, &r,
					   -1);

	double bg_r, bg_g, bg_b, bg_a;
	gdkColor2CairoColor(*fillColor, &bg_r, &bg_g, &bg_b);
	bg_a = fillOpacity / 100.0;
	double b_r, b_g, b_b, b_a;
	gdkColor2CairoColor(*borderColor, &b_r, &b_g, &b_b);
	b_a = borderOpacity / 100.0;

	g_boxed_free(GDK_TYPE_COLOR, fillColor);
	g_boxed_free(GDK_TYPE_COLOR, borderColor);

	GdkPixmap *pixmap = gdk_pixmap_new(NULL, w, h, 24);

	cairo_t *cr = gdk_cairo_create(pixmap);
	cairo_set_line_width(cr, b);

	cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
	cairo_rectangle(cr, 0, 0, w, h);
	cairo_fill(cr);

	double degrees = 3.1415926 / 180.0;

	cairo_new_sub_path(cr);
	cairo_arc(cr, w - r - b, r + b, r, -90 * degrees, 0 * degrees);
	cairo_arc(cr, w - r - b, h - r - b, r, 0 * degrees, 90 * degrees);
	cairo_arc(cr, r + b, h - r - b, r, 90 * degrees, 180 * degrees);
	cairo_arc(cr, r + b, r + b, r, 180 * degrees, 270 * degrees);
	cairo_close_path(cr);

	cairo_set_source_rgba(cr, bg_r, bg_g, bg_b, bg_a);
	cairo_fill_preserve(cr);
	cairo_set_source_rgba(cr, b_r, b_g, b_b, b_a);
	cairo_set_line_width(cr, b);
	cairo_stroke(cr);
	cairo_destroy(cr);
	cr = NULL;

	pixbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, gdk_colormap_get_system(), 0, 0, 0, 0, w, h);
	if (pixmap)
		g_object_unref(pixmap);

	gtk_list_store_set(backgrounds, &iter,
					   bgColPixbuf, pixbuf,
					   -1);
	if (pixbuf)
		g_object_unref(pixbuf);
}

void background_force_update()
{
	background_update(NULL, NULL);
}

void background_update(GtkWidget *widget, gpointer data)
{
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_background));
	if (index < 0)
		return;

	GtkTreePath *path;
	GtkTreeIter iter;

	path = gtk_tree_path_new_from_indices(index, -1);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
	gtk_tree_path_free(path);

	int r;
	int b;
	GdkColor fillColor;
	int fillOpacity;
	GdkColor borderColor;
	int borderOpacity;

	r = gtk_spin_button_get_value(GTK_SPIN_BUTTON(background_corner_radius));
	b = gtk_spin_button_get_value(GTK_SPIN_BUTTON(background_border_width));
	gtk_color_button_get_color(GTK_COLOR_BUTTON(background_fill_color), &fillColor);
	fillOpacity = MIN(100, 0.5 + gtk_color_button_get_alpha(GTK_COLOR_BUTTON(background_fill_color)) * 100.0 / 0xffff);
	gtk_color_button_get_color(GTK_COLOR_BUTTON(background_border_color), &borderColor);
	borderOpacity = MIN(100, 0.5 + gtk_color_button_get_alpha(GTK_COLOR_BUTTON(background_border_color)) * 100.0 / 0xffff);

	gtk_list_store_set(backgrounds, &iter,
					   bgColPixbuf, NULL,
					   bgColFillColor, &fillColor,
					   bgColFillOpacity, fillOpacity,
					   bgColBorderColor, &borderColor,
					   bgColBorderOpacity, borderOpacity,
					   bgColBorderWidth, b,
					   bgColCornerRadius, r,
					   -1);
	background_update_image(index);
}

void current_background_changed(GtkWidget *widget, gpointer data)
{
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_background));
	if (index < 0)
		return;

	GtkTreePath *path;
	GtkTreeIter iter;

	path = gtk_tree_path_new_from_indices(index, -1);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
	gtk_tree_path_free(path);

	int r;
	int b;
	GdkColor *fillColor;
	int fillOpacity;
	GdkColor *borderColor;
	int borderOpacity;

	gtk_tree_model_get(GTK_TREE_MODEL(backgrounds), &iter,
					   bgColFillColor, &fillColor,
					   bgColFillOpacity, &fillOpacity,
					   bgColBorderColor, &borderColor,
					   bgColBorderOpacity, &borderOpacity,
					   bgColBorderWidth, &b,
					   bgColCornerRadius, &r,
					   -1);

	gtk_color_button_set_color(GTK_COLOR_BUTTON(background_fill_color), fillColor);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_fill_color), (fillOpacity*0xffff)/100);
	gtk_color_button_set_color(GTK_COLOR_BUTTON(background_border_color), borderColor);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_border_color), (borderOpacity*0xffff)/100);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(background_border_width), b);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(background_corner_radius), r);

	g_boxed_free(GDK_TYPE_COLOR, fillColor);
	g_boxed_free(GDK_TYPE_COLOR, borderColor);
}

void create_panel(GtkWidget *parent)
{
	int i;
	GtkWidget *table, *hbox, *position;
	GtkWidget *label;
	int row, col;
	GtkTooltips *tooltips = gtk_tooltips_new();

	label = gtk_label_new(_("<b>Geometry</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);
	hbox = gtk_hbox_new(FALSE, 20);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

	table = gtk_table_new(2, 10, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

	row = 0;
	col = 2;
	label = gtk_label_new(_("Position"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	position = gtk_table_new(5, 5, FALSE);
	gtk_widget_show(position);
	for (i = 0; i < 12; ++i) {
		GSList *group = i == 0 ? NULL : gtk_radio_button_get_group(GTK_RADIO_BUTTON(screen_position[0]));
		screen_position[i] = gtk_radio_button_new(group);
		g_object_set(screen_position[i], "draw-indicator", FALSE, NULL);
		gtk_widget_show(screen_position[i]);

		if (i <= 2 || i >= 9) {
			gtk_widget_set_size_request(screen_position[i], 30, 15);
		} else {
			gtk_widget_set_size_request(screen_position[i], 15, 25);
		}
	}
	gtk_table_attach_defaults(GTK_TABLE(position), screen_position[0], 1, 2, 0, 1);
	gtk_tooltips_set_tip(tooltips, screen_position[0], "Position on screen: top-left, horizontal panel", NULL);
	gtk_table_attach_defaults(GTK_TABLE(position), screen_position[1], 2, 3, 0, 1);
	gtk_tooltips_set_tip(tooltips, screen_position[1], "Position on screen: top-center, horizontal panel", NULL);
	gtk_table_attach_defaults(GTK_TABLE(position), screen_position[2], 3, 4, 0, 1);
	gtk_tooltips_set_tip(tooltips, screen_position[2], "Position on screen: top-right, horizontal panel", NULL);
	gtk_table_attach_defaults(GTK_TABLE(position), screen_position[3], 0, 1, 1, 2);
	gtk_tooltips_set_tip(tooltips, screen_position[3], "Position on screen: top-left, vertical panel", NULL);
	gtk_table_attach_defaults(GTK_TABLE(position), screen_position[4], 0, 1, 2, 3);
	gtk_tooltips_set_tip(tooltips, screen_position[4], "Position on screen: center-left, vertical panel", NULL);
	gtk_table_attach_defaults(GTK_TABLE(position), screen_position[5], 0, 1, 3, 4);
	gtk_tooltips_set_tip(tooltips, screen_position[5], "Position on screen: bottom-left, vertical panel", NULL);
	gtk_table_attach_defaults(GTK_TABLE(position), screen_position[6], 4, 5, 1, 2);
	gtk_tooltips_set_tip(tooltips, screen_position[6], "Position on screen: top-right, vertical panel", NULL);
	gtk_table_attach_defaults(GTK_TABLE(position), screen_position[7], 4, 5, 2, 3);
	gtk_tooltips_set_tip(tooltips, screen_position[7], "Position on screen: center-right, vertical panel", NULL);
	gtk_table_attach_defaults(GTK_TABLE(position), screen_position[8], 4, 5, 3, 4);
	gtk_tooltips_set_tip(tooltips, screen_position[8], "Position on screen: bottom-right, vertical panel", NULL);
	gtk_table_attach_defaults(GTK_TABLE(position), screen_position[9], 1, 2, 4, 5);
	gtk_tooltips_set_tip(tooltips, screen_position[9], "Position on screen: bottom-left, horizontal panel", NULL);
	gtk_table_attach_defaults(GTK_TABLE(position), screen_position[10], 2, 3, 4, 5);
	gtk_tooltips_set_tip(tooltips, screen_position[10], "Position on screen: bottom-center, horizontal panel", NULL);
	gtk_table_attach_defaults(GTK_TABLE(position), screen_position[11], 3, 4, 4, 5);
	gtk_tooltips_set_tip(tooltips, screen_position[11], "Position on screen: bottom-right, horizontal panel", NULL);
	gtk_table_attach(GTK_TABLE(table), position, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);

	row++;
	col = 2;
	label = gtk_label_new(_("Monitor"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_combo_monitor = gtk_combo_box_new_text();
	gtk_widget_show(panel_combo_monitor);
	gtk_table_attach(GTK_TABLE(table), panel_combo_monitor, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_monitor), _("All"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_monitor), _("1"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_monitor), _("2"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_monitor), _("3"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_monitor), _("4"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_monitor), _("5"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_monitor), _("6"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 0);
	gtk_tooltips_set_tip(tooltips, panel_combo_monitor, "The monitor on which the panel is placed", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Width"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_width = gtk_spin_button_new_with_range(0, 9000, 1);
	gtk_widget_show(panel_width);
	gtk_table_attach(GTK_TABLE(table), panel_width, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, panel_width, "The width of the panel", NULL);

	panel_combo_width_type = gtk_combo_box_new_text();
	gtk_widget_show(panel_combo_width_type);
	gtk_table_attach(GTK_TABLE(table), panel_combo_width_type, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_width_type), _("Percent"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_width_type), _("Pixels"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_width_type), 0);
	gtk_tooltips_set_tip(tooltips, panel_combo_width_type, "The units used to specify the width of the panel: pixels or percentage of the monitor size", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Height"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_height = gtk_spin_button_new_with_range(0, 9000, 1);
	gtk_widget_show(panel_height);
	gtk_table_attach(GTK_TABLE(table), panel_height, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, panel_height, "The height of the panel", NULL);

	panel_combo_height_type = gtk_combo_box_new_text();
	gtk_widget_show(panel_combo_height_type);
	gtk_table_attach(GTK_TABLE(table), panel_combo_height_type, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_height_type), _("Percent"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_height_type), _("Pixels"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_height_type), 0);
	gtk_tooltips_set_tip(tooltips, panel_combo_height_type, "The units used to specify the height of the panel: pixels or percentage of the monitor size", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Horizontal margin"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_margin_x = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(panel_margin_x);
	gtk_table_attach(GTK_TABLE(table), panel_margin_x, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, panel_margin_x, "Creates a space between the panel and the edge of the monitor. "
						 "For left-aligned panels, the space is created on the right of the panel; "
						 "for right-aligned panels, it is created on the left; "
						 "for centered panels, it is evenly distributed on both sides of the panel.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Vertical margin"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_margin_y = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(panel_margin_y);
	gtk_table_attach(GTK_TABLE(table), panel_margin_y, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, panel_margin_y, "Creates a space between the panel and the edge of the monitor. "
						 "For top-aligned panels, the space is created on the bottom of the panel; "
						 "for bottom-aligned panels, it is created on the top; "
						 "for centered panels, it is evenly distributed on both sides of the panel.", NULL);

	change_paragraph(parent);

	label = gtk_label_new(_("<b>Appearance</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(2, 10, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

	row = 0;
	col = 2;
	label = gtk_label_new(_("Background"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_background = create_background_combo();
	gtk_widget_show(panel_background);
	gtk_table_attach(GTK_TABLE(table), panel_background, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, panel_background, "Selects the background used to display the panel. "
						 "Backgrounds can be edited in the Backgrounds tab.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Horizontal padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(panel_padding_x);
	gtk_table_attach(GTK_TABLE(table), panel_padding_x, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, panel_padding_x, "Specifies the horizontal padding of the panel. "
						 "This is the space between the border of the panel and the elements inside.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Vertical padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(panel_padding_y);
	gtk_table_attach(GTK_TABLE(table), panel_padding_y, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, panel_padding_y, "Specifies the vertical padding of the panel. "
						 "This is the space between the border of the panel and the elements inside.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Spacing"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_spacing = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(panel_spacing);
	gtk_table_attach(GTK_TABLE(table), panel_spacing, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, panel_spacing, "Specifies the spacing between elements inside the panel.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Ignore compositor"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	disable_transparency = gtk_check_button_new();
	gtk_widget_show(disable_transparency);
	gtk_table_attach(GTK_TABLE(table), disable_transparency, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, disable_transparency, "If enabled, the compositor will not be used to draw a transparent panel. "
						 "May fix display corruption problems on broken graphics stacks.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Font shadows"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	font_shadow = gtk_check_button_new();
	gtk_widget_show(font_shadow);
	gtk_table_attach(GTK_TABLE(table), font_shadow, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, font_shadow, "If enabled, a shadow will be drawn behind text. "
						 "This may improve legibility on transparent panels.", NULL);

	change_paragraph(parent);

	label = gtk_label_new(_("<b>Autohide</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(2, 10, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

	row = 0;
	col = 2;
	label = gtk_label_new(_("Autohide"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_autohide = gtk_check_button_new();
	gtk_widget_show(panel_autohide);
	gtk_table_attach(GTK_TABLE(table), panel_autohide, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, panel_autohide, "If enabled, the panel is hidden when the mouse cursor leaves the panel.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Show panel after"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_autohide_show_time = gtk_spin_button_new_with_range(0, 10000, 0.1);
	gtk_widget_show(panel_autohide_show_time);
	gtk_table_attach(GTK_TABLE(table), panel_autohide_show_time, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, panel_autohide_show_time, "Specifies a delay after which the panel is shown when the mouse cursor enters the panel.", NULL);

	label = gtk_label_new(_("seconds"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	row++;
	col = 2;
	label = gtk_label_new(_("Hidden size"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_autohide_size = gtk_spin_button_new_with_range(1, 500, 1);
	gtk_widget_show(panel_autohide_size);
	gtk_table_attach(GTK_TABLE(table), panel_autohide_size, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, panel_autohide_size, "Specifies the size of the panel when hidden, in pixels.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Hide panel after"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_autohide_hide_time = gtk_spin_button_new_with_range(0, 10000, 0.1);
	gtk_widget_show(panel_autohide_hide_time);
	gtk_table_attach(GTK_TABLE(table), panel_autohide_hide_time, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, panel_autohide_hide_time, "Specifies a delay after which the panel is hidden when the mouse cursor leaves the panel.", NULL);

	label = gtk_label_new(_("seconds"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	change_paragraph(parent);

	label = gtk_label_new(_("<b>Window manager interaction</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(2, 12, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

	row = 0;
	col = 2;
	label = gtk_label_new(_("Forward mouse events"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_wm_menu = gtk_check_button_new();
	gtk_widget_show(panel_wm_menu);
	gtk_table_attach(GTK_TABLE(table), panel_wm_menu, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, panel_wm_menu, "If enabled, mouse events not handled by panel elements are forwarded to the desktop. "
						 "Useful on desktop environments that show a start menu when right clicking the desktop, "
						 "or switch the desktop when rotating the mouse wheel over the desktop.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Place panel in dock"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_dock = gtk_check_button_new();
	gtk_widget_show(panel_dock);
	gtk_table_attach(GTK_TABLE(table), panel_dock, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, panel_dock, "If enabled, places the panel in the dock area of the window manager. "
						 "Windows placed in the dock are usually treated differently than normal windows. "
						 "The exact behavior depends on the window manager and its configuration.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Panel layer"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_combo_layer = gtk_combo_box_new_text();
	gtk_widget_show(panel_combo_layer);
	gtk_table_attach(GTK_TABLE(table), panel_combo_layer, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_layer), _("Top"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_layer), _("Normal"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_layer), _("Bottom"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_layer), 1);
	gtk_tooltips_set_tip(tooltips, panel_combo_layer, "Specifies the layer on which the panel window should be placed. \n"
						 "Top means the panel should always cover other windows. \n"
						 "Bottom means other windows should always cover the panel. \n"
						 "Normal means that other windows may or may not cover the panel, depending on which has focus. \n"
						 "Note that some window managers prevent this option from working correctly if the panel is placed in the dock.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Maximized windows"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_combo_strut_policy = gtk_combo_box_new_text();
	gtk_widget_show(panel_combo_strut_policy);
	gtk_table_attach(GTK_TABLE(table), panel_combo_strut_policy, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_strut_policy), _("Match the panel size"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_strut_policy), _("Match the hidden panel size"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(panel_combo_strut_policy), _("Fill the screen"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_strut_policy), 0);
	gtk_tooltips_set_tip(tooltips, panel_combo_strut_policy, "Specifies the size of maximized windows. \n"
						 "Match the panel size means that maximized windows should extend to the edge of the panel. \n"
						 "Match the hidden panel size means that maximized windows should extend to the edge of the panel when hidden; "
						 "when visible, the panel and the windows will overlap. \n"
						 "Fill the screen means that maximized windows will always have the same size as the screen.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Window name"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	panel_window_name = gtk_entry_new();
	gtk_widget_show(panel_window_name);
	gtk_entry_set_width_chars(GTK_ENTRY(panel_window_name), 28);
	gtk_entry_set_text(GTK_ENTRY(panel_window_name), "tint2");
	gtk_table_attach(GTK_TABLE(table), panel_window_name, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, panel_window_name, "Specifies the name of the panel window. "
						 "This is useful if you want to configure special treatment of tint2 windows in your "
						 "window manager or compositor.", NULL);

	change_paragraph(parent);
}

void create_panel_items(GtkWidget *parent)
{
	GtkWidget *table, *label, *button, *image;
	GtkTooltips *tooltips = gtk_tooltips_new();

	panel_items = gtk_list_store_new(itemsNumCols, G_TYPE_STRING, G_TYPE_STRING);
	all_items = gtk_list_store_new(itemsNumCols, G_TYPE_STRING, G_TYPE_STRING);

	GtkTreeIter iter;
	gtk_list_store_append(all_items, &iter);
	gtk_list_store_set(all_items, &iter,
					   itemsColName, "Battery",
					   itemsColValue, "B",
					   -1);
	gtk_list_store_append(all_items, &iter);
	gtk_list_store_set(all_items, &iter,
					   itemsColName, "Clock",
					   itemsColValue, "C",
					   -1);
	gtk_list_store_append(all_items, &iter);
	gtk_list_store_set(all_items, &iter,
					   itemsColName, "Notification area (system tray)",
					   itemsColValue, "S",
					   -1);
	gtk_list_store_append(all_items, &iter);
	gtk_list_store_set(all_items, &iter,
					   itemsColName, "Taskbar",
					   itemsColValue, "T",
					   -1);
	gtk_list_store_append(all_items, &iter);
	gtk_list_store_set(all_items, &iter,
					   itemsColName, "Launcher",
					   itemsColValue, "L",
					   -1);

	panel_items_view = gtk_tree_view_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(panel_items_view),
												-1,
												"",
												gtk_cell_renderer_text_new(),
												"text", itemsColName,
												NULL);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(panel_items_view), FALSE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(panel_items_view)), GTK_SELECTION_SINGLE);
	gtk_tree_view_set_model(GTK_TREE_VIEW(panel_items_view), GTK_TREE_MODEL(panel_items));
	g_object_unref(panel_items);
	gtk_tooltips_set_tip(tooltips, panel_items_view, "Specifies the elements that will appear in the panel and their order. "
													 "Elements can be added by selecting them in the list of available elements, then clicking on "
													 "the add left button.", NULL);


	all_items_view = gtk_tree_view_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(all_items_view),
												-1,
												"",
												gtk_cell_renderer_text_new(),
												"text", itemsColName,
												NULL);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(all_items_view), FALSE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(all_items_view)), GTK_SELECTION_SINGLE);
	gtk_tree_view_set_model(GTK_TREE_VIEW(all_items_view), GTK_TREE_MODEL(all_items));
	g_object_unref(all_items);
	gtk_tooltips_set_tip(tooltips, all_items_view, "Lists all the possible elements that can appear in the panel. "
												   "Elements can be added to the panel by selecting them, then clicking on "
												   "the add left button.", NULL);

	table = gtk_table_new(2, 3, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

	label = gtk_label_new(_("<b>Elements selected</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new(_("<b>Elements available</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	GtkWidget *vbox;
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 0);

	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(panel_move_item_up), NULL);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	gtk_tooltips_set_tip(tooltips, button, "Moves up the current element in the list of selected elements.", NULL);

	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(panel_move_item_down), NULL);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	gtk_tooltips_set_tip(tooltips, button, "Moves down the current element in the list of selected elements.", NULL);

	label = gtk_label_new(_(" "));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(panel_add_item), NULL);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	gtk_tooltips_set_tip(tooltips, button, "Copies the current element in the list of available elements to the list of selected elements.", NULL);

	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(panel_remove_item), NULL);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	gtk_tooltips_set_tip(tooltips, button, "Removes the current element from the list of selected elements.", NULL);

	gtk_table_attach(GTK_TABLE(table), vbox, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

	gtk_widget_show(panel_items_view);
	gtk_table_attach(GTK_TABLE(table), addScrollBarToWidget(panel_items_view), 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

	gtk_widget_show(all_items_view);
	gtk_table_attach(GTK_TABLE(table), addScrollBarToWidget(all_items_view), 2, 3, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

	change_paragraph(parent);
}

gboolean panel_contains(const char *value)
{
	GtkTreeModel *model = GTK_TREE_MODEL(panel_items);

	GtkTreeIter i;
	if (!gtk_tree_model_get_iter_first(model, &i)) {
		return FALSE;
	}

	while (1) {
		gchar *v;
		gtk_tree_model_get(model, &i,
						   itemsColValue, &v,
						   -1);
		if (g_str_equal(value, v)) {
			return TRUE;
		}

		if (!gtk_tree_model_iter_next(model, &i)) {
			break;
		}
	}

	return FALSE;
}

char *get_panel_items()
{
	char *result = calloc(1, 256 * sizeof(char));
	GtkTreeModel *model = GTK_TREE_MODEL(panel_items);

	GtkTreeIter i;
	if (!gtk_tree_model_get_iter_first(model, &i)) {
		return FALSE;
	}

	while (1) {
		gchar *v;
		gtk_tree_model_get(model, &i,
						   itemsColValue, &v,
						   -1);
		strcat(result, v);

		if (!gtk_tree_model_iter_next(model, &i)) {
			break;
		}
	}

	return result;
}

void set_panel_items(const char *items)
{
	gtk_list_store_clear(panel_items);

	for (; items && *items; items++) {
		const char *value = NULL;
		const char *name = NULL;

		char v = *items;
		if (v == 'B') {
			value = "B";
			name = "Battery";
		} else if (v == 'C') {
			value = "C";
			name = "Clock";
		} else if (v == 'S') {
			value = "S";
			name = "Notification area (system tray)";
		} else if (v == 'T') {
			value = "T";
			name = "Taskbar";
		} else if (v == 'L') {
			value = "L";
			name = "Launcher";
		} else {
			continue;
		}

		GtkTreeIter iter;
		gtk_list_store_append(panel_items, &iter);
		gtk_list_store_set(panel_items, &iter,
						   itemsColName, name,
						   itemsColValue, value,
						   -1);
	}
}

void panel_add_item(GtkWidget *widget, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(all_items_view)), &model, &iter)) {
		gchar *name;
		gchar *value;

		gtk_tree_model_get(model, &iter,
						   itemsColName, &name,
						   itemsColValue, &value,
						   -1);

		if (!panel_contains(value)) {
			GtkTreeIter iter;
			gtk_list_store_append(panel_items, &iter);
			gtk_list_store_set(panel_items, &iter,
							   itemsColName, g_strdup(name),
							   itemsColValue, g_strdup(value),
							   -1);
		}
	}
}

void panel_remove_item(GtkWidget *widget, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(panel_items_view)), &model, &iter)) {
		gtk_list_store_remove(panel_items, &iter);
	}
}

void panel_move_item_down(GtkWidget *widget, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(panel_items_view)), &model, &iter)) {
		GtkTreeIter next = iter;
		if (gtk_tree_model_iter_next(model, &next)) {
			gtk_list_store_swap(panel_items, &iter, &next);
		}
	}
}

void panel_move_item_up(GtkWidget *widget, gpointer data)
{
	{
		GtkTreeIter iter;
		GtkTreeModel *model;

		if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(panel_items_view)), &model, &iter)) {
			GtkTreeIter prev = iter;
			if (gtk_tree_model_iter_prev_tint2(model, &prev)) {
				gtk_list_store_swap(panel_items, &iter, &prev);
			}
		}
	}
}

enum {
	iconsColName = 0,
	iconsColDescr,
	iconsNumCols
};
GtkListStore *icon_themes;

void launcher_add_app(GtkWidget *widget, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(all_apps_view)), &model, &iter)) {
		GdkPixbuf *pixbuf;
		gchar *name;
		gchar *path;
		gchar *iconName;

		gtk_tree_model_get(model, &iter,
						   appsColIcon, &pixbuf,
						   appsColText, &name,
						   appsColPath, &path,
						   appsColIconName, &iconName,
						   -1);

		GtkTreeIter iter;
		gtk_list_store_append(launcher_apps, &iter);
		gtk_list_store_set(launcher_apps, &iter,
						   appsColIcon, pixbuf,
						   appsColText, g_strdup(name),
						   appsColPath, g_strdup(path),
						   appsColIconName, g_strdup(iconName),
						   -1);
		if (pixbuf)
			g_object_unref(pixbuf);
	}
}

void launcher_remove_app(GtkWidget *widget, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(launcher_apps_view)), &model, &iter)) {
		gtk_list_store_remove(launcher_apps, &iter);
	}
}

void launcher_move_app_down(GtkWidget *widget, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(launcher_apps_view)), &model, &iter)) {
		GtkTreeIter next = iter;
		if (gtk_tree_model_iter_next(model, &next)) {
			gtk_list_store_swap(launcher_apps, &iter, &next);
		}
	}
}

void launcher_move_app_up(GtkWidget *widget, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(launcher_apps_view)), &model, &iter)) {
		GtkTreeIter prev = iter;
		if (gtk_tree_model_iter_prev_tint2(model, &prev)) {
			gtk_list_store_swap(launcher_apps, &iter, &prev);
		}
	}
}

gboolean gtk_tree_model_iter_prev_tint2(GtkTreeModel *model, GtkTreeIter *iter)
{
	GtkTreeIter i;
	if (!gtk_tree_model_get_iter_first(model, &i)) {
		return FALSE;
	}
	GtkTreePath *piter = gtk_tree_model_get_path(model, iter);
	if (!piter)
		return FALSE;

	while (1) {
		GtkTreeIter next = i;
		if (gtk_tree_model_iter_next(model, &next)) {
			GtkTreePath *pn = gtk_tree_model_get_path(model, &next);
			if (!pn)
				continue;
			if (gtk_tree_path_compare(piter, pn) == 0) {
				gtk_tree_path_free(piter);
				gtk_tree_path_free(pn);
				*iter = i;
				return TRUE;
			}
			gtk_tree_path_free(pn);
			i = next;
		} else {
			break;
		}
	}
	gtk_tree_path_free(piter);
	return FALSE;
}

// Note: the returned pointer must be released with g_free!
gchar *get_current_icon_theme()
{
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(launcher_icon_theme));
	if (index <= 0) {
		return NULL;
	}

	GtkTreePath *path;
	GtkTreeIter iter;

	path = gtk_tree_path_new_from_indices(index, -1);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(icon_themes), &iter, path);
	gtk_tree_path_free(path);

	gchar *name;
	gtk_tree_model_get(GTK_TREE_MODEL(icon_themes), &iter,
					   iconsColName, &name,
					   -1);
	return name;
}

void set_current_icon_theme(const char *theme)
{
	int i;
	for (i = 0; ; i++) {
		GtkTreePath *path;
		GtkTreeIter iter;

		path = gtk_tree_path_new_from_indices(i, -1);
		gboolean end = gtk_tree_model_get_iter(GTK_TREE_MODEL(icon_themes), &iter, path);
		gtk_tree_path_free(path);

		if (!end) {
			break;
		}

		gchar *name;
		gtk_tree_model_get(GTK_TREE_MODEL(icon_themes), &iter,
						   iconsColName, &name,
						   -1);
		if (g_str_equal(name, theme)) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(launcher_icon_theme), i);
			g_free(name);
			break;
		}
		g_free(name);
	}
}

void icon_theme_changed()
{
	if (icon_theme)
		free_themes(icon_theme);

	gchar *icon_theme_name = get_current_icon_theme();
	if (!icon_theme_name || g_str_equal(icon_theme_name, "")) {
		g_free(icon_theme_name);
		icon_theme_name = get_default_theme_name();
	}
	icon_theme = load_themes(icon_theme_name);
	g_free(icon_theme_name);

	load_icons(launcher_apps);
	load_icons(all_apps);
}

void launcher_icon_theme_changed(GtkWidget *widget, gpointer data)
{
	icon_theme_changed();
}

GdkPixbuf *load_icon(const gchar *name)
{
	int size = 22;
	char *path = get_icon_path(icon_theme, name, size);
	GdkPixbuf *pixbuf = path ? gdk_pixbuf_new_from_file_at_size(path, size, size, NULL) : NULL;
	free(path);
	return pixbuf;
}

void load_icons(GtkListStore *apps)
{
	int i;
	for (i = 0; ; i++) {
		GtkTreePath *path;
		GtkTreeIter iter;

		path = gtk_tree_path_new_from_indices(i, -1);
		gboolean found = gtk_tree_model_get_iter(GTK_TREE_MODEL(apps), &iter, path);
		gtk_tree_path_free(path);

		if (!found)
			break;

		gchar *iconName;
		gtk_tree_model_get(GTK_TREE_MODEL(apps), &iter,
						   appsColIconName, &iconName,
						   -1);
		GdkPixbuf *pixbuf = load_icon(iconName);
		gtk_list_store_set(apps, &iter,
						   appsColIcon, pixbuf,
						   -1);
		if (pixbuf)
			g_object_unref(pixbuf);
		g_free(iconName);
	}
}

void load_desktop_file(const char *file, gboolean selected)
{
	DesktopEntry entry;
	if (read_desktop_file(file, &entry)) {
		GdkPixbuf *pixbuf = load_icon(entry.icon);
		GtkTreeIter iter;
		gtk_list_store_append(selected ? launcher_apps : all_apps, &iter);
		gtk_list_store_set(selected ? launcher_apps :all_apps, &iter,
						   appsColIcon, pixbuf,
						   appsColText, g_strdup(entry.name),
						   appsColPath, g_strdup(file),
						   appsColIconName, g_strdup(entry.icon),
						   -1);
		if (pixbuf)
			g_object_unref(pixbuf);
	} else {
		printf("Could not load %s\n", file);
		GtkTreeIter iter;
		gtk_list_store_append(selected ? launcher_apps : all_apps, &iter);
		gtk_list_store_set(selected ? launcher_apps :all_apps, &iter,
						   appsColIcon, NULL,
						   appsColText, g_strdup(file),
						   appsColPath, g_strdup(file),
						   appsColIconName, g_strdup(""),
						   -1);
	}
	free_desktop_entry(&entry);
}

void load_desktop_files(const gchar *path)
{
	GDir *d = g_dir_open(path, 0, NULL);
	if (!d)
		return;
	const gchar *name;
	while ((name = g_dir_read_name(d))) {
		gchar *file = g_build_filename(path, name, NULL);
		if (!g_file_test(file, G_FILE_TEST_IS_DIR) &&
			g_str_has_suffix(file, ".desktop")) {
			load_desktop_file(file, FALSE);
		} else if (g_file_test(file, G_FILE_TEST_IS_DIR)) {
			load_desktop_files(file);
		}
		g_free(file);
	}
	g_dir_close(d);
}

void load_theme_file(const char *file_name, const char *theme)
{
	if (!file_name || !theme) {
		return;
	}

	FILE *f;
	if ((f = fopen(file_name, "rt")) == NULL) {
		return;
	}

	char *line = NULL;
	size_t line_size;

	while (getline(&line, &line_size, f) >= 0) {
		char *key, *value;

		int line_len = strlen(line);
		if (line_len >= 1) {
			if (line[line_len - 1] == '\n') {
				line[line_len - 1] = '\0';
				line_len--;
			}
		}

		if (line_len == 0)
			continue;

		if (parse_theme_line(line, &key, &value)) {
			if (strcmp(key, "Name") == 0) {
				// value is like Tango
				GtkTreeIter iter;
				gtk_list_store_append(icon_themes, &iter);
				gtk_list_store_set(icon_themes, &iter,
								   iconsColName, g_strdup(theme),
								   iconsColDescr, g_strdup(value),
								   -1);
			}
		}

		if (line[0] == '[' && line[line_len - 1] == ']' && strcmp(line, "[Icon Theme]") != 0) {
			break;
		}
	}
	fclose(f);
	free(line);
}

void load_icon_themes(const gchar *path, const gchar *parent)
{
	GDir *d = g_dir_open(path, 0, NULL);
	if (!d)
		return;
	const gchar *name;
	while ((name = g_dir_read_name(d))) {
		gchar *file = g_build_filename(path, name, NULL);
		if (parent &&
			g_file_test(file, G_FILE_TEST_IS_REGULAR) &&
			g_str_equal(name, "index.theme")) {
			load_theme_file(file, parent);
		} else if (g_file_test(file, G_FILE_TEST_IS_DIR)) {
			load_icon_themes(file, name);
		}
		g_free(file);
	}
	g_dir_close(d);
}

gchar *get_default_theme_name()
{
	gchar *name = NULL;
	g_object_get(gtk_settings_get_default(),
				 "gtk-icon-theme-name",
				 &name,
				 NULL);
	if (!name) {
		name = g_strdup("hicolor");
	}
	return name;
}

GtkWidget *addScrollBarToWidget(GtkWidget *widget)
{
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled_window),
								   GTK_POLICY_NEVER,
								   GTK_POLICY_AUTOMATIC);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), widget);

	gtk_widget_set_size_request(scrolled_window, 100, 300);

	gtk_widget_show(widget);
	gtk_widget_show(scrolled_window);

	return scrolled_window;
}

void create_launcher(GtkWidget *parent)
{
	GtkWidget *image;
	GtkTooltips *tooltips = gtk_tooltips_new();

	icon_theme = NULL;

	launcher_apps = gtk_list_store_new(appsNumCols, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	all_apps = gtk_list_store_new(appsNumCols, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	icon_themes = gtk_list_store_new(iconsNumCols, G_TYPE_STRING, G_TYPE_STRING);

	launcher_apps_view = gtk_tree_view_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(launcher_apps_view),
												-1,
												"",
												gtk_cell_renderer_pixbuf_new(),
												"pixbuf", appsColIcon,
												NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(launcher_apps_view),
												-1,
												"",
												gtk_cell_renderer_text_new(),
												"text", appsColText,
												NULL);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(launcher_apps_view), FALSE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(launcher_apps_view)), GTK_SELECTION_SINGLE);
	gtk_tree_view_set_model(GTK_TREE_VIEW(launcher_apps_view), GTK_TREE_MODEL(launcher_apps));
	g_object_unref(launcher_apps);
	gtk_tooltips_set_tip(tooltips, launcher_apps_view, "Specifies the application launchers that will appear in the launcher and their order. "
												  "Launchers can be added by selecting an item in the list of available applications, then clicking on "
												  "the add left button.", NULL);

	all_apps_view = gtk_tree_view_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(all_apps_view),
												-1,
												"",
												gtk_cell_renderer_pixbuf_new(),
												"pixbuf", appsColIcon,
												NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(all_apps_view),
												-1,
												"",
												gtk_cell_renderer_text_new(),
												"text", appsColText,
												NULL);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(all_apps_view), FALSE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(all_apps_view)), GTK_SELECTION_SINGLE);
	gtk_tree_view_set_model(GTK_TREE_VIEW(all_apps_view), GTK_TREE_MODEL(all_apps));
	g_object_unref(all_apps);
	gtk_tooltips_set_tip(tooltips, all_apps_view, "Lists all the applications detected on the system. "
											 "Launchers can be added to the launcher by selecting an application, then clicking on "
											 "the add left button.", NULL);

	GtkWidget *table, *label, *button;
	int row, col;

	table = gtk_table_new(2, 3, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

	label = gtk_label_new(_("<b>Applications selected</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new(_("<b>Applications available</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	GtkWidget *vbox;
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 0);

	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(launcher_move_app_up), NULL);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	gtk_tooltips_set_tip(tooltips, button, "Moves up the current launcher in the list of selected applications.", NULL);

	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(launcher_move_app_down), NULL);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	gtk_tooltips_set_tip(tooltips, button, "Moves down the current launcher in the list of selected applications.", NULL);

	label = gtk_label_new(_(" "));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(launcher_add_app), NULL);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	gtk_tooltips_set_tip(tooltips, button, "Copies the current application in the list of available applications to the list of selected applications.", NULL);

	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(launcher_remove_app), NULL);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	gtk_tooltips_set_tip(tooltips, button, "Removes the current application from the list of selected application.", NULL);

	gtk_table_attach(GTK_TABLE(table), vbox, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

	gtk_widget_show(launcher_apps_view);
	gtk_table_attach(GTK_TABLE(table), addScrollBarToWidget(launcher_apps_view), 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

	gtk_widget_show(all_apps_view);
	gtk_table_attach(GTK_TABLE(table), addScrollBarToWidget(all_apps_view), 2, 3, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

	change_paragraph(parent);

	label = gtk_label_new(_("<b>Additional application directories</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	launcher_apps_dirs = gtk_entry_new();
	gtk_widget_show(launcher_apps_dirs);
	gtk_box_pack_start(GTK_BOX(parent), launcher_apps_dirs, FALSE, FALSE, 0);
	gtk_tooltips_set_tip(tooltips, launcher_apps_dirs, "Specifies a path to a directory from which the launcher is loading all .desktop files (all subdirectories are explored recursively). "
						 "Can be used multiple times, in which case the paths must be separated by commas. Leading ~ is expaned to the path of the user's home directory.", NULL);

	label = gtk_label_new(_("<b>Appearance</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(7, 10, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

	row = 0, col = 2;
	label = gtk_label_new(_("Background"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	launcher_background = create_background_combo();
	gtk_widget_show(launcher_background);
	gtk_table_attach(GTK_TABLE(table), launcher_background, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, launcher_background, "Selects the background used to display the launcher. "
														"Backgrounds can be edited in the Backgrounds tab.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Horizontal padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	launcher_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(launcher_padding_x);
	gtk_table_attach(GTK_TABLE(table), launcher_padding_x, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, launcher_padding_x, "Specifies the horizontal padding of the launcher. "
						 "This is the space between the border and the elements inside.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Vertical padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	launcher_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(launcher_padding_y);
	gtk_table_attach(GTK_TABLE(table), launcher_padding_y, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, launcher_padding_y, "Specifies the vertical padding of the launcher. "
						 "This is the space between the border and the elements inside.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Spacing"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	launcher_spacing = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(launcher_spacing);
	gtk_table_attach(GTK_TABLE(table), launcher_spacing, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, launcher_spacing, "Specifies the spacing between the elements inside the launcher.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Icon size"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	launcher_icon_size = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(launcher_icon_size);
	gtk_table_attach(GTK_TABLE(table), launcher_icon_size, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, launcher_icon_size, "Specifies the size of the launcher icons, in pixels.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Icon opacity"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	launcher_icon_opacity = gtk_spin_button_new_with_range(0, 100, 1);
	gtk_widget_show(launcher_icon_opacity);
	gtk_table_attach(GTK_TABLE(table), launcher_icon_opacity, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, launcher_icon_opacity, "Specifies the opacity of the launcher icons, in percent.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Icon saturation"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	launcher_icon_saturation = gtk_spin_button_new_with_range(-100, 100, 1);
	gtk_widget_show(launcher_icon_saturation);
	gtk_table_attach(GTK_TABLE(table), launcher_icon_saturation, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, launcher_icon_saturation, "Specifies the saturation adjustment of the launcher icons, in percent.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Icon brightness"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	launcher_icon_brightness = gtk_spin_button_new_with_range(-100, 100, 1);
	gtk_widget_show(launcher_icon_brightness);
	gtk_table_attach(GTK_TABLE(table), launcher_icon_brightness, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, launcher_icon_brightness, "Specifies the brightness adjustment of the launcher icons, in percent.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Icon theme"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+3, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	launcher_icon_theme = gtk_combo_box_new_with_model(GTK_TREE_MODEL(icon_themes));
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(launcher_icon_theme), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(launcher_icon_theme), renderer, "text", iconsColDescr, NULL);
	g_signal_connect(G_OBJECT(launcher_icon_theme), "changed", G_CALLBACK(launcher_icon_theme_changed), NULL);
	gtk_widget_show(launcher_icon_theme);
	gtk_table_attach(GTK_TABLE(table), launcher_icon_theme, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, launcher_icon_theme, "The icon theme used to display launcher icons. If left blank, "
						 "tint2 will detect and use the icon theme of your desktop as long as you have "
						 "an XSETTINGS manager running (most desktop environments do).", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Startup notifications"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	startup_notifications = gtk_check_button_new();
	gtk_widget_show(startup_notifications);
	gtk_table_attach(GTK_TABLE(table), startup_notifications, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, startup_notifications, "If enabled, startup notifications are shown when starting applications from the launcher. "
						 "The appearance may vary depending on your desktop environment configuration; normally, a busy mouse cursor is displayed until the application starts.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Tooltips"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	launcher_tooltip = gtk_check_button_new();
	gtk_widget_show(launcher_tooltip);
	gtk_table_attach(GTK_TABLE(table), launcher_tooltip, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, launcher_tooltip, "If enabled, shows a tooltip with the application name when the mouse is moved over an application launcher.", NULL);

	change_paragraph(parent);

	GtkTreeIter iter;
	gtk_list_store_append(icon_themes, &iter);
	gtk_list_store_set(icon_themes, &iter,
					   0, "",
					   -1);

	fprintf(stderr, "Loading icon themes\n");
	const GSList *location;
	for (location = get_icon_locations(); location; location = g_slist_next(location)) {
		const gchar *path = (gchar*) location->data;
		load_icon_themes(path, NULL);
	}
	fprintf(stderr, "Icon themes loaded\n");

	fprintf(stderr, "Loading .desktop files\n");
	load_desktop_files("/usr/share/applications");
	gchar *path = g_build_filename(g_get_home_dir(), ".local/share/applications", NULL);
	load_desktop_files(path);
	g_free(path);

	icon_theme_changed();
	load_icons(launcher_apps);
	load_icons(all_apps);
	fprintf(stderr, "Desktop files loaded\n");
}

void create_taskbar(GtkWidget *parent)
{
	GtkWidget *table, *label;
	int row, col;
	GtkTooltips *tooltips = gtk_tooltips_new();

	label = gtk_label_new(_("<b>Options</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(4, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = col = 0;

	col = 2;
	row++;
	label = gtk_label_new(_("Show a taskbar for each desktop"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_show_desktop = gtk_check_button_new();
	gtk_widget_show(taskbar_show_desktop);
	gtk_table_attach(GTK_TABLE(table), taskbar_show_desktop, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_show_desktop, "If enabled, the taskbar is split into multiple smaller taskbars, one for each virtual desktop.", NULL);

	col = 2;
	row++;
	label = gtk_label_new(_("Distribute size between taskbars"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_distribute_size = gtk_check_button_new();
	gtk_widget_show(taskbar_distribute_size);
	gtk_table_attach(GTK_TABLE(table), taskbar_distribute_size, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_distribute_size, "If enabled and 'Show a taskbar for each desktop' is also enabled, "
						 "the available size is distributed between taskbars proportionally to the number of tasks.", NULL);

	col = 2;
	row++;
	label = gtk_label_new(_("Hide inactive tasks"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_hide_inactive_tasks = gtk_check_button_new();
	gtk_widget_show(taskbar_hide_inactive_tasks);
	gtk_table_attach(GTK_TABLE(table), taskbar_hide_inactive_tasks, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_hide_inactive_tasks, "If enabled, only the active task will be shown in the taskbar.", NULL);

	col = 2;
	row++;
	label = gtk_label_new(_("Hide tasks from different monitors"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_hide_diff_monitor = gtk_check_button_new();
	gtk_widget_show(taskbar_hide_diff_monitor);
	gtk_table_attach(GTK_TABLE(table), taskbar_hide_diff_monitor, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_hide_diff_monitor, "If enabled, tasks that are not on the same monitor as the panel will not be displayed. "
						 "This behavior is enabled automatically if the panel monitor is set to 'All'.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Task sorting"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_sort_order = gtk_combo_box_new_text();
	gtk_widget_show(taskbar_sort_order);
	gtk_table_attach(GTK_TABLE(table), taskbar_sort_order, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_combo_box_append_text(GTK_COMBO_BOX(taskbar_sort_order), _("None"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(taskbar_sort_order), _("By title"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(taskbar_sort_order), _("By center"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_sort_order), 0);
	gtk_tooltips_set_tip(tooltips, taskbar_sort_order, "Specifies how tasks should be sorted on the taskbar. \n"
						 "'None' means that new tasks are added to the end, and the user can also reorder task buttons by mouse dragging. \n"
						 "'By title' means that tasks are sorted by their window titles. \n"
						 "'By center' means that tasks are sorted geometrically by their window centers.", NULL);

	change_paragraph(parent);

	label = gtk_label_new(_("<b>Appearance</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(3, 12, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = col = 0;

	col = 2;
	label = gtk_label_new(_("Horizontal padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(taskbar_padding_x);
	gtk_table_attach(GTK_TABLE(table), taskbar_padding_x, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_padding_x, "Specifies the horizontal padding of the taskbar. "
						 "This is the space between the border and the elements inside.", NULL);

	col = 2;
	row++;
	label = gtk_label_new(_("Vertical padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(taskbar_padding_y);
	gtk_table_attach(GTK_TABLE(table), taskbar_padding_y, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_padding_y, "Specifies the vertical padding of the taskbar. "
						 "This is the space between the border and the elements inside.", NULL);

	col = 2;
	row++;
	label = gtk_label_new(_("Spacing"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_spacing = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(taskbar_spacing);
	gtk_table_attach(GTK_TABLE(table), taskbar_spacing, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_spacing, "Specifies the spacing between the elements inside the taskbar.", NULL);

	col = 2;
	row++;
	label = gtk_label_new(_("Active background"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_active_background = create_background_combo();
	gtk_widget_show(taskbar_active_background);
	gtk_table_attach(GTK_TABLE(table), taskbar_active_background, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_active_background, "Selects the background used to display the taskbar of the current desktop. "
																"Backgrounds can be edited in the Backgrounds tab.", NULL);
	col = 2;
	row++;
	label = gtk_label_new(_("Inactive background"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_inactive_background = create_background_combo();
	gtk_widget_show(taskbar_inactive_background);
	gtk_table_attach(GTK_TABLE(table), taskbar_inactive_background, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_inactive_background, "Selects the background used to display taskbars of inactive desktops. "
																"Backgrounds can be edited in the Backgrounds tab.", NULL);

	change_paragraph(parent);

	label = gtk_label_new(_("<b>Desktop name</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(6, 22, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = col = 0;

	col = 2;
	row++;
	label = gtk_label_new(_("Show desktop name"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_show_name = gtk_check_button_new();
	gtk_widget_show(taskbar_show_name);
	gtk_table_attach(GTK_TABLE(table), taskbar_show_name, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_show_name, "If enabled, displays the name of the desktop at the top/left of the taskbar. "
						 "The name is set by your window manager; you might be able to configure it there.", NULL);

	col = 2;
	row++;
	label = gtk_label_new(_("Horizontal padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_name_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(taskbar_name_padding_x);
	gtk_table_attach(GTK_TABLE(table), taskbar_name_padding_x, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_name_padding_x, "Specifies the horizontal padding of the desktop name. "
						 "This is the space between the border and the text inside.", NULL);

	col = 2;
	row++;
	label = gtk_label_new(_("Vertical padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_name_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(taskbar_name_padding_y);
	gtk_table_attach(GTK_TABLE(table), taskbar_name_padding_y, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_name_padding_y, "Specifies the vertical padding of the desktop name. "
						 "This is the space between the border and the text inside.", NULL);

	col = 2;
	row++;
	label = gtk_label_new(_("Active font color"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_name_active_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(taskbar_name_active_color), TRUE);
	gtk_widget_show(taskbar_name_active_color);
	gtk_table_attach(GTK_TABLE(table), taskbar_name_active_color, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_name_active_color, "Specifies the font color used to display the name of the current desktop.", NULL);

	col = 2;
	row++;
	label = gtk_label_new(_("Inactive font color"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_name_inactive_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(taskbar_name_inactive_color), TRUE);
	gtk_widget_show(taskbar_name_inactive_color);
	gtk_table_attach(GTK_TABLE(table), taskbar_name_inactive_color, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_name_inactive_color, "Specifies the font color used to display the name of inactive desktops.", NULL);

	col = 2;
	row++;
	label = gtk_label_new(_("Font"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_name_font = gtk_font_button_new();
	gtk_widget_show(taskbar_name_font);
	gtk_table_attach(GTK_TABLE(table), taskbar_name_font, col, col+3, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(taskbar_name_font), TRUE);
	gtk_tooltips_set_tip(tooltips, taskbar_name_font, "Specifies the font used to display the desktop name.", NULL);

	col = 2;
	row++;
	label = gtk_label_new(_("Active background"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_name_active_background = create_background_combo();
	gtk_widget_show(taskbar_name_active_background);
	gtk_table_attach(GTK_TABLE(table), taskbar_name_active_background, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_name_active_background, "Selects the background used to display the name of the current desktop. "
																"Backgrounds can be edited in the Backgrounds tab.", NULL);

	col = 2;
	row++;
	label = gtk_label_new(_("Inactive background"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	taskbar_name_inactive_background = create_background_combo();
	gtk_widget_show(taskbar_name_inactive_background);
	gtk_table_attach(GTK_TABLE(table), taskbar_name_inactive_background, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, taskbar_name_inactive_background, "Selects the background used to display the name of inactive desktops. "
																"Backgrounds can be edited in the Backgrounds tab.", NULL);

	change_paragraph(parent);
}

void create_task(GtkWidget *parent)
{
	GtkWidget *table, *label, *notebook;
	int row, col;
	GtkTooltips *tooltips = gtk_tooltips_new();

	label = gtk_label_new(_("<b>Mouse events</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(3, 10, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = 0, col = 2;

	label = gtk_label_new(_("Left click"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	task_mouse_left = gtk_combo_box_new_text();
	gtk_widget_show(task_mouse_left);
	gtk_table_attach(GTK_TABLE(table), task_mouse_left, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_left), _("None"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_left), _("Close"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_left), _("Toggle"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_left), _("Iconify"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_left), _("Shade"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_left), _("Toggle or iconify"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_left), _("Maximize or restore"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_left), _("Desktop left"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_left), _("Desktop right"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_left), _("Next task"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_left), _("Previous task"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(task_mouse_left), 5);
	gtk_tooltips_set_tip(tooltips, task_mouse_left, "Specifies the action performed when task buttons receive a left click event: \n"
						 "'None' means that no action is taken. \n"
						 "'Close' closes the task. \n"
						 "'Toggle' toggles the task. \n"
						 "'Iconify' iconifies (minimizes) the task. \n"
						 "'Shade' shades (collapses) the task. \n"
						 "'Toggle or iconify' toggles or iconifies the task. \n"
						 "'Maximize or restore' maximizes or minimizes the task. \n"
						 "'Desktop left' sends the task to the previous desktop. \n"
						 "'Desktop right' sends the task to the next desktop. \n"
						 "'Next task' sends the focus to the next task. \n"
						 "'Previous task' sends the focus to the previous task.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Wheel scroll up"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	task_mouse_scroll_up = gtk_combo_box_new_text();
	gtk_widget_show(task_mouse_scroll_up);
	gtk_table_attach(GTK_TABLE(table), task_mouse_scroll_up, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_up), _("None"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_up), _("Close"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_up), _("Toggle"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_up), _("Iconify"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_up), _("Shade"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_up), _("Toggle or iconify"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_up), _("Maximize or restore"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_up), _("Desktop left"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_up), _("Desktop right"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_up), _("Next task"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_up), _("Previous task"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(task_mouse_scroll_up), 0);
	gtk_tooltips_set_tip(tooltips, task_mouse_scroll_up, "Specifies the action performed when task buttons receive a scroll up event: \n"
						 "'None' means that no action is taken. \n"
						 "'Close' closes the task. \n"
						 "'Toggle' toggles the task. \n"
						 "'Iconify' iconifies (minimizes) the task. \n"
						 "'Shade' shades (collapses) the task. \n"
						 "'Toggle or iconify' toggles or iconifies the task. \n"
						 "'Maximize or restore' maximizes or minimizes the task. \n"
						 "'Desktop left' sends the task to the previous desktop. \n"
						 "'Desktop right' sends the task to the next desktop. \n"
						 "'Next task' sends the focus to the next task. \n"
						 "'Previous task' sends the focus to the previous task.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Middle click"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	task_mouse_middle = gtk_combo_box_new_text();
	gtk_widget_show(task_mouse_middle);
	gtk_table_attach(GTK_TABLE(table), task_mouse_middle, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_middle), _("None"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_middle), _("Close"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_middle), _("Toggle"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_middle), _("Iconify"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_middle), _("sShade"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_middle), _("Toggle or iconify"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_middle), _("Maximize or restore"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_middle), _("Desktop left"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_middle), _("Desktop right"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_middle), _("Next task"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_middle), _("Previous task"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(task_mouse_middle), 0);
	gtk_tooltips_set_tip(tooltips, task_mouse_middle, "Specifies the action performed when task buttons receive a middle click event: \n"
						 "'None' means that no action is taken. \n"
						 "'Close' closes the task. \n"
						 "'Toggle' toggles the task. \n"
						 "'Iconify' iconifies (minimizes) the task. \n"
						 "'Shade' shades (collapses) the task. \n"
						 "'Toggle or iconify' toggles or iconifies the task. \n"
						 "'Maximize or restore' maximizes or minimizes the task. \n"
						 "'Desktop left' sends the task to the previous desktop. \n"
						 "'Desktop right' sends the task to the next desktop. \n"
						 "'Next task' sends the focus to the next task. \n"
						 "'Previous task' sends the focus to the previous task.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Wheel scroll down"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	task_mouse_scroll_down = gtk_combo_box_new_text();
	gtk_widget_show(task_mouse_scroll_down);
	gtk_table_attach(GTK_TABLE(table), task_mouse_scroll_down, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_down), _("None"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_down), _("Close"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_down), _("Toggle"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_down), _("Iconify"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_down), _("Shade"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_down), _("Toggle or iconify"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_down), _("Maximize or restore"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_down), _("Desktop left"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_down), _("Desktop right"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_down), _("Next task"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_scroll_down), _("Previous task"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(task_mouse_scroll_down), 0);
	gtk_tooltips_set_tip(tooltips, task_mouse_scroll_down, "Specifies the action performed when task buttons receive a scroll down event: \n"
						 "'None' means that no action is taken. \n"
						 "'Close' closes the task. \n"
						 "'Toggle' toggles the task. \n"
						 "'Iconify' iconifies (minimizes) the task. \n"
						 "'Shade' shades (collapses) the task. \n"
						 "'Toggle or iconify' toggles or iconifies the task. \n"
						 "'Maximize or restore' maximizes or minimizes the task. \n"
						 "'Desktop left' sends the task to the previous desktop. \n"
						 "'Desktop right' sends the task to the next desktop. \n"
						 "'Next task' sends the focus to the next task. \n"
						 "'Previous task' sends the focus to the previous task.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Right click"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	task_mouse_right = gtk_combo_box_new_text();
	gtk_widget_show(task_mouse_right);
	gtk_table_attach(GTK_TABLE(table), task_mouse_right, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_right), _("None"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_right), _("Close"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_right), _("Toggle"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_right), _("Iconify"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_right), _("Shade"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_right), _("Toggle or iconify"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_right), _("Maximize or restore"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_right), _("Desktop left"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_right), _("Desktop right"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_right), _("Next task"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(task_mouse_right), _("Previous task"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(task_mouse_right), 1);
	gtk_tooltips_set_tip(tooltips, task_mouse_right, "Specifies the action performed when task buttons receive a right click event: \n"
						 "'None' means that no action is taken. \n"
						 "'Close' closes the task. \n"
						 "'Toggle' toggles the task. \n"
						 "'Iconify' iconifies (minimizes) the task. \n"
						 "'Shade' shades (collapses) the task. \n"
						 "'Toggle or iconify' toggles or iconifies the task. \n"
						 "'Maximize or restore' maximizes or minimizes the task. \n"
						 "'Desktop left' sends the task to the previous desktop. \n"
						 "'Desktop right' sends the task to the next desktop. \n"
						 "'Next task' sends the focus to the next task. \n"
						 "'Previous task' sends the focus to the previous task.", NULL);

	change_paragraph(parent);

	label = gtk_label_new(_("<b>Appearance</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(4, 13, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = 0, col = 2;

	label = gtk_label_new(_("Show icon"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	task_show_icon = gtk_check_button_new();
	gtk_widget_show(task_show_icon);
	gtk_table_attach(GTK_TABLE(table), task_show_icon, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, task_show_icon, "If enabled, the window icon is shown on task buttons.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Show text"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	task_show_text = gtk_check_button_new();
	gtk_widget_show(task_show_text);
	gtk_table_attach(GTK_TABLE(table), task_show_text, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, task_show_text, "If enabled, the window title is shown on task buttons.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Center text"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	task_align_center = gtk_check_button_new();
	gtk_widget_show(task_align_center);
	gtk_table_attach(GTK_TABLE(table), task_align_center, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, task_align_center, "If enabled, the text is centered on task buttons. Otherwise, it is left-aligned.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Show tooltips"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	tooltip_task_show = gtk_check_button_new();
	gtk_widget_show(tooltip_task_show);
	gtk_table_attach(GTK_TABLE(table), tooltip_task_show, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, tooltip_task_show, "If enabled, a tooltip showing the window title is displayed when the mouse cursor moves over task buttons.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Maximum width"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	task_maximum_width = gtk_spin_button_new_with_range(0, 9000, 1);
	gtk_widget_show(task_maximum_width);
	gtk_table_attach(GTK_TABLE(table), task_maximum_width, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, task_maximum_width, "Specifies the maximum width of the task buttons.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Maximum height"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	task_maximum_height = gtk_spin_button_new_with_range(0, 9000, 1);
	gtk_widget_show(task_maximum_height);
	gtk_table_attach(GTK_TABLE(table), task_maximum_height, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, task_maximum_height, "Specifies the maximum height of the task buttons.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Horizontal padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	task_padding_x = gtk_spin_button_new_with_range(0, 9000, 1);
	gtk_widget_show(task_padding_x);
	gtk_table_attach(GTK_TABLE(table), task_padding_x, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, task_padding_x, "Specifies the horizontal padding of the task buttons. "
						 "This is the space between the border and the content inside.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Vertical padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	task_padding_y = gtk_spin_button_new_with_range(0, 9000, 1);
	gtk_widget_show(task_padding_y);
	gtk_table_attach(GTK_TABLE(table), task_padding_y, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, task_padding_y, "Specifies the vertical padding of the task buttons. "
						 "This is the space between the border and the content inside.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Spacing"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	task_spacing = gtk_spin_button_new_with_range(0, 9000, 1);
	gtk_widget_show(task_spacing);
	gtk_table_attach(GTK_TABLE(table), task_spacing, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, task_spacing, "Specifies the spacing between the icon and the text.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Font"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	task_font = gtk_font_button_new();
	gtk_widget_show(task_font);
	gtk_table_attach(GTK_TABLE(table), task_font, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(task_font), TRUE);
	gtk_tooltips_set_tip(tooltips, task_font, "Specifies the font used to display the task button text.", NULL);

	change_paragraph(parent);

	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);
	gtk_container_set_border_width(GTK_CONTAINER(notebook), 0);
	gtk_box_pack_start(GTK_BOX(parent), notebook, TRUE, TRUE, 0);

	create_task_status(notebook,
					   "Default style",
					   &task_default_color,
					   &task_default_color_set,
					   &task_default_icon_opacity,
					   &task_default_icon_osb_set,
					   &task_default_icon_saturation,
					   &task_default_icon_brightness,
					   &task_default_background,
					   &task_default_background_set);
	create_task_status(notebook,
					   "Normal task",
					   &task_normal_color,
					   &task_normal_color_set,
					   &task_normal_icon_opacity,
					   &task_normal_icon_osb_set,
					   &task_normal_icon_saturation,
					   &task_normal_icon_brightness,
					   &task_normal_background,
					   &task_normal_background_set);
	create_task_status(notebook,
					   "Active task",
					   &task_active_color,
					   &task_active_color_set,
					   &task_active_icon_opacity,
					   &task_active_icon_osb_set,
					   &task_active_icon_saturation,
					   &task_active_icon_brightness,
					   &task_active_background,
					   &task_active_background_set);
	create_task_status(notebook,
					   "Urgent task",
					   &task_urgent_color,
					   &task_urgent_color_set,
					   &task_urgent_icon_opacity,
					   &task_urgent_icon_osb_set,
					   &task_urgent_icon_saturation,
					   &task_urgent_icon_brightness,
					   &task_urgent_background,
					   &task_urgent_background_set);
	create_task_status(notebook,
					   "Iconified task",
					   &task_iconified_color,
					   &task_iconified_color_set,
					   &task_iconified_icon_opacity,
					   &task_iconified_icon_osb_set,
					   &task_iconified_icon_saturation,
					   &task_iconified_icon_brightness,
					   &task_iconified_background,
					   &task_iconified_background_set);
}

void task_status_toggle_button_callback(GtkWidget *widget, gpointer data)
{
	GtkWidget *child1, *child2, *child3;
	child1 = child2 = child3 = NULL;

	if (widget == task_default_color_set) {
		child1 = task_default_color;
	} else if (widget == task_default_icon_osb_set) {
		child1 = task_default_icon_opacity;
		child2 = task_default_icon_saturation;
		child3 = task_default_icon_brightness;
	} else if (widget == task_default_background_set) {
		child1 = task_default_background;
	}
	else
	if (widget == task_normal_color_set) {
		child1 = task_normal_color;
	} else if (widget == task_normal_icon_osb_set) {
		child1 = task_normal_icon_opacity;
		child2 = task_normal_icon_saturation;
		child3 = task_normal_icon_brightness;
	} else if (widget == task_normal_background_set) {
		child1 = task_normal_background;
	}
	else
	if (widget == task_active_color_set) {
		child1 = task_active_color;
	} else if (widget == task_active_icon_osb_set) {
		child1 = task_active_icon_opacity;
		child2 = task_active_icon_saturation;
		child3 = task_active_icon_brightness;
	} else if (widget == task_active_background_set) {
		child1 = task_active_background;
	}
	else
	if (widget == task_urgent_color_set) {
		child1 = task_urgent_color;
	} else if (widget == task_urgent_icon_osb_set) {
		child1 = task_urgent_icon_opacity;
		child2 = task_urgent_icon_saturation;
		child3 = task_urgent_icon_brightness;
	} else if (widget == task_urgent_background_set) {
		child1 = task_urgent_background;
	}
	else
	if (widget == task_iconified_color_set) {
		child1 = task_iconified_color;
	} else if (widget == task_iconified_icon_osb_set) {
		child1 = task_iconified_icon_opacity;
		child2 = task_iconified_icon_saturation;
		child3 = task_iconified_icon_brightness;
	} else if (widget == task_iconified_background_set) {
		child1 = task_iconified_background;
	}

	if (child1)
		gtk_widget_set_sensitive(child1, GTK_TOGGLE_BUTTON(widget)->active);
	if (child2)
		gtk_widget_set_sensitive(child2, GTK_TOGGLE_BUTTON(widget)->active);
	if (child3)
		gtk_widget_set_sensitive(child3, GTK_TOGGLE_BUTTON(widget)->active);
}

void create_task_status(GtkWidget *notebook,
						char *name,
						GtkWidget **task_status_color,
						GtkWidget **task_status_color_set,
						GtkWidget **task_status_icon_opacity,
						GtkWidget **task_status_icon_osb_set,
						GtkWidget **task_status_icon_saturation,
						GtkWidget **task_status_icon_brightness,
						GtkWidget **task_status_background,
						GtkWidget **task_status_background_set)
{
	GtkTooltips *tooltips = gtk_tooltips_new();
	GtkWidget *label = gtk_label_new(_(name));
	gtk_widget_show(label);
	GtkWidget *page_task = gtk_vbox_new(FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_task), 10);
	gtk_widget_show(page_task);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_task, label);

	GtkWidget *table = gtk_table_new(6, 3, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(page_task), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

	*task_status_color_set = gtk_check_button_new();
	gtk_widget_show(*task_status_color_set);
	gtk_table_attach(GTK_TABLE(table), *task_status_color_set, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
	gtk_signal_connect(GTK_OBJECT(*task_status_color_set), "toggled", GTK_SIGNAL_FUNC(task_status_toggle_button_callback), NULL);
	gtk_tooltips_set_tip(tooltips, *task_status_color_set, "If enabled, a custom font color is used to display the task text.", NULL);

	label = gtk_label_new(_("Font color"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);

	*task_status_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(*task_status_color), TRUE);
	gtk_widget_show(*task_status_color);
	gtk_table_attach(GTK_TABLE(table), *task_status_color, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);
	gtk_tooltips_set_tip(tooltips, *task_status_color, "Specifies the font color used to display the task text.", NULL);

	*task_status_icon_osb_set = gtk_check_button_new();
	gtk_widget_show(*task_status_icon_osb_set);
	gtk_table_attach(GTK_TABLE(table), *task_status_icon_osb_set, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
	gtk_signal_connect(GTK_OBJECT(*task_status_icon_osb_set), "toggled", GTK_SIGNAL_FUNC(task_status_toggle_button_callback), NULL);
	gtk_tooltips_set_tip(tooltips, *task_status_icon_osb_set, "If enabled, a custom opacity/saturation/brightness is used to display the task icon.", NULL);

	label = gtk_label_new(_("Icon opacity"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);

	*task_status_icon_opacity = gtk_spin_button_new_with_range(0, 100, 1);
	gtk_widget_show(*task_status_icon_opacity);
	gtk_table_attach(GTK_TABLE(table), *task_status_icon_opacity, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);
	gtk_tooltips_set_tip(tooltips, *task_status_icon_opacity, "Specifies the opacity (in %) used to display the task icon.", NULL);

	label = gtk_label_new(_("Icon saturation"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);

	*task_status_icon_saturation = gtk_spin_button_new_with_range(-100, 100, 1);
	gtk_widget_show(*task_status_icon_saturation);
	gtk_table_attach(GTK_TABLE(table), *task_status_icon_saturation, 2, 3, 2, 3, GTK_FILL, 0, 0, 0);
	gtk_tooltips_set_tip(tooltips, *task_status_icon_saturation, "Specifies the saturation adjustment (in %) used to display the task icon.", NULL);

	label = gtk_label_new(_("Icon brightness"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 1, 2, 3, 4, GTK_FILL, 0, 0, 0);

	*task_status_icon_brightness = gtk_spin_button_new_with_range(-100, 100, 1);
	gtk_widget_show(*task_status_icon_brightness);
	gtk_table_attach(GTK_TABLE(table), *task_status_icon_brightness, 2, 3, 3, 4, GTK_FILL, 0, 0, 0);
	gtk_tooltips_set_tip(tooltips, *task_status_icon_brightness, "Specifies the brightness adjustment (in %) used to display the task icon.", NULL);

	*task_status_background_set = gtk_check_button_new();
	gtk_widget_show(*task_status_background_set);
	gtk_table_attach(GTK_TABLE(table), *task_status_background_set, 0, 1, 4, 5, GTK_FILL, 0, 0, 0);
	gtk_signal_connect(GTK_OBJECT(*task_status_background_set), "toggled", GTK_SIGNAL_FUNC(task_status_toggle_button_callback), NULL);
	gtk_tooltips_set_tip(tooltips, *task_status_background_set, "If enabled, a custom background is used to display the task.", NULL);

	label = gtk_label_new(_("Background"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 1, 2, 4, 5, GTK_FILL, 0, 0, 0);

	*task_status_background = create_background_combo();
	gtk_widget_show(*task_status_background);
	gtk_table_attach(GTK_TABLE(table), *task_status_background, 2, 3, 4, 5, GTK_FILL, 0, 0, 0);
	gtk_tooltips_set_tip(tooltips, *task_status_background, "Selects the background used to display the task. "
														   "Backgrounds can be edited in the Backgrounds tab.", NULL);

	if (*task_status_color == task_urgent_color) {
		label = gtk_label_new(_("Blinks"));
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
		gtk_widget_show(label);
		gtk_table_attach(GTK_TABLE(table), label, 1, 2, 5, 6, GTK_FILL, 0, 0, 0);

		task_urgent_blinks = gtk_spin_button_new_with_range(0, 1000000, 1);
		gtk_widget_show(task_urgent_blinks);
		gtk_table_attach(GTK_TABLE(table), task_urgent_blinks, 2, 3, 5, 6, GTK_FILL, 0, 0, 0);
		gtk_tooltips_set_tip(tooltips, task_urgent_blinks, "Specifies how many times urgent tasks blink.", NULL);
	}

	task_status_toggle_button_callback(*task_status_color_set, NULL);
	task_status_toggle_button_callback(*task_status_icon_osb_set, NULL);
	task_status_toggle_button_callback(*task_status_background_set, NULL);
}

void create_clock(GtkWidget *parent)
{
	GtkWidget *table;
	GtkWidget *label;
	int row, col;
	GtkTooltips *tooltips = gtk_tooltips_new();

	table = gtk_table_new(1, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = 0, col = 2;

	label = gtk_label_new(_("<b>Format</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(3, 10, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = 0, col = 2;

	label = gtk_label_new(_("First line format"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	clock_format_line1 = gtk_entry_new();
	gtk_widget_show(clock_format_line1);
	gtk_entry_set_width_chars(GTK_ENTRY(clock_format_line1), 16);
	gtk_table_attach(GTK_TABLE(table), clock_format_line1, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, clock_format_line1,
						 "Specifies the format used to display the first line of the clock text. "
						 "See 'man strftime' for all the available options.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Second line format"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	clock_format_line2 = gtk_entry_new();
	gtk_widget_show(clock_format_line2);
	gtk_entry_set_width_chars(GTK_ENTRY(clock_format_line2), 16);
	gtk_table_attach(GTK_TABLE(table), clock_format_line2, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, clock_format_line2,
						 "Specifies the format used to display the second line of the clock text. "
						 "See 'man strftime' for all the available options.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("First line timezone"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	clock_tmz_line1 = gtk_entry_new();
	gtk_widget_show(clock_tmz_line1);
	gtk_entry_set_width_chars(GTK_ENTRY(clock_tmz_line1), 16);
	gtk_table_attach(GTK_TABLE(table), clock_tmz_line1, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, clock_tmz_line1,
						 "Specifies the timezone used to display the first line of the clock text. If empty, the current timezone is used. "
						 "Otherwise, it must be set to a valid value of the TZ environment variable.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Second line timezone"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	clock_tmz_line2 = gtk_entry_new();
	gtk_widget_show(clock_tmz_line2);
	gtk_entry_set_width_chars(GTK_ENTRY(clock_tmz_line2), 16);
	gtk_table_attach(GTK_TABLE(table), clock_tmz_line2, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, clock_tmz_line2,
						 "Specifies the timezone used to display the second line of the clock text. If empty, the current timezone is used. "
						 "Otherwise, it must be set to a valid value of the TZ environment variable.", NULL);

	change_paragraph(parent);

	label = gtk_label_new(_("<b>Mouse events</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(2, 10, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = 0, col = 2;

	label = gtk_label_new(_("Left click command"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	clock_left_command = gtk_entry_new();
	gtk_widget_show(clock_left_command);
	gtk_entry_set_width_chars(GTK_ENTRY(clock_left_command), 50);
	gtk_table_attach(GTK_TABLE(table), clock_left_command, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, clock_left_command,
						 "Specifies a command that will be executed when the clock receives a left click.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Right click command"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	clock_right_command = gtk_entry_new();
	gtk_widget_show(clock_right_command);
	gtk_entry_set_width_chars(GTK_ENTRY(clock_right_command), 50);
	gtk_table_attach(GTK_TABLE(table), clock_right_command, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, clock_right_command,
						 "Specifies a command that will be executed when the clock receives a right click.", NULL);

	change_paragraph(parent);

	label = gtk_label_new(_("<b>Appearance</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(3, 22, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = 0, col = 2;

	label = gtk_label_new(_("Background"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	clock_background = create_background_combo();
	gtk_widget_show(clock_background);
	gtk_table_attach(GTK_TABLE(table), clock_background, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, clock_background, "Selects the background used to display the clock. "
													 "Backgrounds can be edited in the Backgrounds tab.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Horizontal padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	clock_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(clock_padding_x);
	gtk_table_attach(GTK_TABLE(table), clock_padding_x, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, clock_padding_x, "Specifies the horizontal padding of the clock. "
						 "This is the space between the border and the content inside.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Vertical padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	clock_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(clock_padding_y);
	gtk_table_attach(GTK_TABLE(table), clock_padding_y, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, clock_padding_y, "Specifies the vertical padding of the clock. "
						 "This is the space between the border and the content inside.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Font first line"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	clock_font_line1 = gtk_font_button_new();
	gtk_widget_show(clock_font_line1);
	gtk_table_attach(GTK_TABLE(table), clock_font_line1, col, col+3, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(clock_font_line1), TRUE);
	gtk_tooltips_set_tip(tooltips, clock_font_line1, "Specifies the font used to display the first line of the clock.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Font second line"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	clock_font_line2 = gtk_font_button_new();
	gtk_widget_show(clock_font_line2);
	gtk_table_attach(GTK_TABLE(table), clock_font_line2, col, col+3, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(clock_font_line2), TRUE);
	gtk_tooltips_set_tip(tooltips, clock_font_line2, "Specifies the font used to display the second line of the clock.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Font color"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	clock_font_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(clock_font_color), TRUE);
	gtk_widget_show(clock_font_color);
	gtk_table_attach(GTK_TABLE(table), clock_font_color, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, clock_font_color, "Specifies the font color used to display the clock.", NULL);

	change_paragraph(parent);

	label = gtk_label_new(_("<b>Tooltip</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(3, 10, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = 0, col = 2;

	label = gtk_label_new(_("Format"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	clock_format_tooltip = gtk_entry_new();
	gtk_widget_show(clock_format_tooltip);
	gtk_entry_set_width_chars(GTK_ENTRY(clock_format_tooltip), 30);
	gtk_table_attach(GTK_TABLE(table), clock_format_tooltip, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, clock_format_tooltip, "Specifies the format used to display the clock tooltip. "
						 "See 'man strftime' for the available options.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Timezone"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	clock_tmz_tooltip = gtk_entry_new();
	gtk_widget_show(clock_tmz_tooltip);
	gtk_entry_set_width_chars(GTK_ENTRY(clock_tmz_tooltip), 16);
	gtk_table_attach(GTK_TABLE(table), clock_tmz_tooltip, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, clock_tmz_tooltip, "Specifies the timezone used to display the clock tooltip. If empty, the current timezone is used. "
													  "Otherwise, it must be set to a valid value of the TZ environment variable.", NULL);

	change_paragraph(parent);
}

void create_systemtray(GtkWidget *parent)
{
	GtkWidget *table;
	GtkWidget *label;
	int row, col;
	GtkTooltips *tooltips = gtk_tooltips_new();

	label = gtk_label_new(_("<b>Options</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(2, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = 0;
	col = 2;

	label = gtk_label_new(_("Icon ordering"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	systray_icon_order = gtk_combo_box_new_text();
	gtk_widget_show(systray_icon_order);
	gtk_table_attach(GTK_TABLE(table), systray_icon_order, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_combo_box_append_text(GTK_COMBO_BOX(systray_icon_order), _("Ascending"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(systray_icon_order), _("Descending"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(systray_icon_order), _("Left to right"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(systray_icon_order), _("Right to left"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(systray_icon_order), 0);
	gtk_tooltips_set_tip(tooltips, systray_icon_order, "Specifies the order used to arrange the system tray icons. \n"
						 "'Ascending' means that icons are sorted in ascending order of their window names. \n"
						 "'Descending' means that icons are sorted in descending order of their window names. \n"
						 "'Left to right' means that icons are always added to the left. \n"
						 "'Right to left' means that icons are always added to the right.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Monitor"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	systray_monitor = gtk_combo_box_new_text();
	gtk_widget_show(systray_monitor);
	gtk_table_attach(GTK_TABLE(table), systray_monitor, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_combo_box_append_text(GTK_COMBO_BOX(systray_monitor), _("1"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(systray_monitor), _("2"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(systray_monitor), _("3"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(systray_monitor), _("4"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(systray_monitor), _("5"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(systray_monitor), _("6"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(systray_monitor), 0);
	gtk_tooltips_set_tip(tooltips, systray_monitor, "Specifies the monitor on which to place the system tray. "
						 "Due to technical limitations, the system tray cannot be displayed on multiple monitors.", NULL);

	change_paragraph(parent);

	label = gtk_label_new(_("<b>Appearance</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(6, 10, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = 0, col = 2;

	label = gtk_label_new(_("Background"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	systray_background = create_background_combo();
	gtk_widget_show(systray_background);
	gtk_table_attach(GTK_TABLE(table), systray_background, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, systray_background, "Selects the background used to display the system tray. "
													   "Backgrounds can be edited in the Backgrounds tab.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Horizontal padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	systray_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(systray_padding_x);
	gtk_table_attach(GTK_TABLE(table), systray_padding_x, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, systray_padding_x, "Specifies the horizontal padding of the system tray. "
						 "This is the space between the border and the content inside.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Vertical padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	systray_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(systray_padding_y);
	gtk_table_attach(GTK_TABLE(table), systray_padding_y, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, systray_padding_y, "Specifies the vertical padding of the system tray. "
						 "This is the space between the border and the content inside.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Spacing"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	systray_spacing = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(systray_spacing);
	gtk_table_attach(GTK_TABLE(table), systray_spacing, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, systray_spacing, "Specifies the spacing between system tray icons.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Icon size"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	systray_icon_size = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(systray_icon_size);
	gtk_table_attach(GTK_TABLE(table), systray_icon_size, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, systray_icon_size, "Specifies the size of the system tray icons, in pixels.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Icon opacity"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	systray_icon_opacity = gtk_spin_button_new_with_range(0, 100, 1);
	gtk_widget_show(systray_icon_opacity);
	gtk_table_attach(GTK_TABLE(table), systray_icon_opacity, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, systray_icon_opacity, "Specifies the opacity of the system tray icons, in percent.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Icon saturation"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	systray_icon_saturation = gtk_spin_button_new_with_range(-100, 100, 1);
	gtk_widget_show(systray_icon_saturation);
	gtk_table_attach(GTK_TABLE(table), systray_icon_saturation, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, systray_icon_saturation, "Specifies the saturation adjustment of the system tray icons, in percent.", NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Icon brightness"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	systray_icon_brightness = gtk_spin_button_new_with_range(-100, 100, 1);
	gtk_widget_show(systray_icon_brightness);
	gtk_table_attach(GTK_TABLE(table), systray_icon_brightness, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, systray_icon_brightness, "Specifies the brightness adjustment of the system tray icons, in percent.", NULL);
}

void create_battery(GtkWidget *parent)
{
	GtkWidget *table, *label;
	int row, col;
	GtkTooltips *tooltips = gtk_tooltips_new();

	table = gtk_table_new(1, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = 0, col = 2;

	label = gtk_label_new(_("<b>Thresholds</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(2, 10, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = 0, col = 2;

	label = gtk_label_new(_("Hide if charge higher than"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	battery_hide_if_higher = gtk_spin_button_new_with_range(0, 101, 1);
	gtk_widget_show(battery_hide_if_higher);
	gtk_table_attach(GTK_TABLE(table), battery_hide_if_higher, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, battery_hide_if_higher, "Minimum battery level for which to hide the batter applet. Use 101 to always show the batter applet.", NULL);

	label = gtk_label_new(_("%"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	row++, col = 2;
	label = gtk_label_new(_("Alert if charge lower than"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	battery_alert_if_lower = gtk_spin_button_new_with_range(0, 100, 1);
	gtk_widget_show(battery_alert_if_lower);
	gtk_table_attach(GTK_TABLE(table), battery_alert_if_lower, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, battery_alert_if_lower, "Battery level for which to display an alert.", NULL);

	label = gtk_label_new(_("%"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	row++, col = 2;
	label = gtk_label_new(_("Alert command"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	battery_alert_cmd = gtk_entry_new();
	gtk_widget_show(battery_alert_cmd);
	gtk_entry_set_width_chars(GTK_ENTRY(battery_alert_cmd), 50);
	gtk_table_attach(GTK_TABLE(table), battery_alert_cmd, col, col+3, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, battery_alert_cmd, "Command to be executed when the alert threshold is reached.", NULL);

	change_paragraph(parent);

	label = gtk_label_new(_("<b>Appearance</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(4, 22, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = 0, col = 2;

	label = gtk_label_new(_("Background"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	battery_background = create_background_combo();
	gtk_widget_show(battery_background);
	gtk_table_attach(GTK_TABLE(table), battery_background, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, battery_background, "Selects the background used to display the battery. "
													   "Backgrounds can be edited in the Backgrounds tab.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Horizontal padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	battery_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(battery_padding_x);
	gtk_table_attach(GTK_TABLE(table), battery_padding_x, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, battery_padding_x, "Specifies the horizontal padding of the battery. "
						 "This is the space between the border and the content inside.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Vertical padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	battery_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(battery_padding_y);
	gtk_table_attach(GTK_TABLE(table), battery_padding_y, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, battery_padding_y, "Specifies the vertical padding of the battery. "
						 "This is the space between the border and the content inside.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Font first line"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	battery_font_line1 = gtk_font_button_new();
	gtk_widget_show(battery_font_line1);
	gtk_table_attach(GTK_TABLE(table), battery_font_line1, col, col+3, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(battery_font_line1), TRUE);
	gtk_tooltips_set_tip(tooltips, battery_font_line1, "Specifies the font used to display the first line of the battery text.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Font second line"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	battery_font_line2 = gtk_font_button_new();
	gtk_widget_show(battery_font_line2);
	gtk_table_attach(GTK_TABLE(table), battery_font_line2, col, col+3, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(battery_font_line2), TRUE);
	gtk_tooltips_set_tip(tooltips, battery_font_line2, "Specifies the font used to display the second line of the battery text.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Font color"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	battery_font_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(battery_font_color), TRUE);
	gtk_widget_show(battery_font_color);
	gtk_table_attach(GTK_TABLE(table), battery_font_color, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, battery_font_color, "Specifies the font clor used to display the battery text.", NULL);

	change_paragraph(parent);
}

void create_tooltip(GtkWidget *parent)
{
	GtkWidget *table;
	GtkWidget *label;
	int row, col;
	GtkTooltips *tooltips = gtk_tooltips_new();

	label = gtk_label_new(_("<b>Timing</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(2, 22, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = 0, col = 2;

	label = gtk_label_new(_("Show delay"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	tooltip_show_after = gtk_spin_button_new_with_range(0, 10000, 0.1);
	gtk_widget_show(tooltip_show_after);
	gtk_table_attach(GTK_TABLE(table), tooltip_show_after, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, tooltip_show_after, "Specifies a delay after which to show the tooltip when moving the mouse over an element.", NULL);

	label = gtk_label_new(_("seconds"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	row++, col = 2;
	label = gtk_label_new(_("Hide delay"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	tooltip_hide_after = gtk_spin_button_new_with_range(0, 10000, 0.1);
	gtk_widget_show(tooltip_hide_after);
	gtk_table_attach(GTK_TABLE(table), tooltip_hide_after, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	gtk_tooltips_set_tip(tooltips, tooltip_hide_after, "Specifies a delay after which to hide the tooltip when moving the mouse outside an element.", NULL);
	col++;

	label = gtk_label_new(_("seconds"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	change_paragraph(parent);
	
	label = gtk_label_new(_("<b>Appearance</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

	table = gtk_table_new(3, 10, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
	row = 0, col = 2;

	label = gtk_label_new(_("Background"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	tooltip_background = create_background_combo();
	gtk_widget_show(tooltip_background);
	gtk_table_attach(GTK_TABLE(table), tooltip_background, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, tooltip_background, "Selects the background used to display the tooltip. "
													   "Backgrounds can be edited in the Backgrounds tab.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Horizontal padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	tooltip_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(tooltip_padding_x);
	gtk_table_attach(GTK_TABLE(table), tooltip_padding_x, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, tooltip_padding_x, "Specifies the horizontal padding of the tooltip. "
						 "This is the space between the border and the content inside.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Vertical padding"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	tooltip_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
	gtk_widget_show(tooltip_padding_y);
	gtk_table_attach(GTK_TABLE(table), tooltip_padding_y, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, tooltip_padding_y, "Specifies the vertical padding of the tooltip. "
						 "This is the space between the border and the content inside.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Font"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	tooltip_font = gtk_font_button_new();
	gtk_widget_show(tooltip_font);
	gtk_table_attach(GTK_TABLE(table), tooltip_font, col, col+3, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(tooltip_font), TRUE);
	gtk_tooltips_set_tip(tooltips, tooltip_font, "Specifies the font used to display the text of the tooltip.", NULL);

	row++, col = 2;
	label = gtk_label_new(_("Font color"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	tooltip_font_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(tooltip_font_color), TRUE);
	gtk_widget_show(tooltip_font_color);
	gtk_table_attach(GTK_TABLE(table), tooltip_font_color, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, tooltip_font_color, "Specifies the font color used to display the text of the tooltip.", NULL);

	change_paragraph(parent);
}
