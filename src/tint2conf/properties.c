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

#define ROW_SPACING  10
#define COL_SPACING  8
#define DEFAULT_HOR_SPACING  5

void change_paragraph(GtkWidget  *widget);
void create_general(GtkWidget  *parent);
void create_panel(GtkWidget  *parent);
void create_panel_items(GtkWidget  *parent);
void create_launcher(GtkWidget  *parent);
void create_taskbar(GtkWidget  *parent);
void create_task(GtkWidget  *parent);
void create_clock(GtkWidget  *parent);
void create_systemtray(GtkWidget  *parent);
void create_battery(GtkWidget  *parent);
void create_tooltip(GtkWidget  *parent);
void create_background(GtkWidget  *parent);


GtkWidget *create_properties()
{
	GtkWidget  *view, *dialog_vbox3, *button, *notebook;
	GtkTooltips *tooltips;
	GtkWidget *page_panel, *page_panel_items, *page_launcher,  *page_taskbar,  *page_battery, *page_clock, *page_tooltip, *page_systemtray, *page_task, *page_background;
	GtkWidget *label;

	tooltips = gtk_tooltips_new ();

	// global layer
	view = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (view), _("Properties"));
	gtk_window_set_modal (GTK_WINDOW (view), TRUE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (view), TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (view), GDK_WINDOW_TYPE_HINT_DIALOG);

	dialog_vbox3 = GTK_DIALOG (view)->vbox;
	gtk_widget_show (dialog_vbox3);

	notebook = gtk_notebook_new ();
	gtk_widget_show (notebook);
	gtk_container_set_border_width(GTK_CONTAINER(notebook), 5);
	gtk_box_pack_start (GTK_BOX (dialog_vbox3), notebook, TRUE, TRUE, 6);
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_LEFT);

	button = gtk_button_new_from_stock ("gtk-apply");
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (view), button, GTK_RESPONSE_APPLY);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

	button = gtk_button_new_from_stock ("gtk-cancel");
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (view), button, GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

	button = gtk_button_new_from_stock ("gtk-ok");
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (view), button, GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

	// notebook
	label = gtk_label_new (_("Panel"));
	gtk_widget_show (label);
	page_panel = gtk_vbox_new (FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_panel), 10);
	gtk_widget_show (page_panel);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_panel, label);
	create_panel(page_panel);

	label = gtk_label_new (_("Panel items"));
	gtk_widget_show (label);
	page_panel_items = gtk_vbox_new (FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_panel_items), 10);
	gtk_widget_show (page_panel_items);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_panel_items, label);
	create_panel_items(page_panel_items);

	label = gtk_label_new (_("Launcher"));
	gtk_widget_show (label);
	page_launcher = gtk_vbox_new (FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_launcher), 10);
	gtk_widget_show (page_launcher);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_launcher, label);
	create_launcher(page_launcher);

	label = gtk_label_new (_("Taskbar"));
	gtk_widget_show (label);
	page_taskbar = gtk_vbox_new (FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_taskbar), 10);
	gtk_widget_show (page_taskbar);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_taskbar, label);
	create_taskbar(page_taskbar);

	label = gtk_label_new (_("Task"));
	gtk_widget_show (label);
	page_task = gtk_vbox_new (FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_task), 10);
	gtk_widget_show (page_task);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_task, label);
	create_task(page_task);

	label = gtk_label_new (_("Clock"));
	gtk_widget_show (label);
	page_clock = gtk_vbox_new (FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_clock), 10);
	gtk_widget_show (page_clock);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_clock, label);
	create_clock(page_clock);

	label = gtk_label_new (_("Notification"));
	gtk_widget_show (label);
	page_systemtray = gtk_vbox_new (FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_systemtray), 10);
	gtk_widget_show (page_systemtray);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_systemtray, label);
	create_systemtray(page_systemtray);

	label = gtk_label_new (_("Battery"));
	gtk_widget_show (label);
	page_battery = gtk_vbox_new (FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_battery), 10);
	gtk_widget_show (page_battery);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_battery, label);
	create_battery(page_battery);

	label = gtk_label_new (_("Tooltip"));
	gtk_widget_show (label);
	page_tooltip = gtk_vbox_new (FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_tooltip), 10);
	gtk_widget_show (page_tooltip);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_tooltip, label);
	create_tooltip(page_tooltip);

	label = gtk_label_new (_("Background"));
	gtk_widget_show (label);
	page_background = gtk_vbox_new (FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_background), 10);
	gtk_widget_show (page_background);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_background, label);
	create_background(page_background);
	
	return view;
}


void change_paragraph(GtkWidget  *widget)
{
	GtkWidget  *hbox;
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start(GTK_BOX (widget), hbox, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
}


void create_panel(GtkWidget  *parent)
{
	int i;
	GtkWidget  *screen_position[12];
	GtkWidget  *table, *hbox, *frame;
	GtkWidget  *label;

	label = gtk_label_new (_("<b>Position and size</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 20);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (parent), hbox, FALSE, FALSE, 0);

	// left marging
	table = gtk_table_new (2, 1, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	table = gtk_table_new (3, 8, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Width"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);

	panel_width = gtk_spin_button_new_with_range (0, 9000, 1);
	gtk_widget_show (panel_width);
	gtk_table_attach (GTK_TABLE (table), panel_width, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	panel_combo_width_type = gtk_combo_box_new_text ();
	gtk_widget_show (panel_combo_width_type);
	gtk_table_attach (GTK_TABLE (table), panel_combo_width_type, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_width_type), _("Percent"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_width_type), _("Pixels"));

	label = gtk_label_new (_("Marging x"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 0, 1, GTK_FILL, 0, 0, 0);

	panel_margin_x = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (panel_margin_x);
	gtk_entry_set_max_length (GTK_ENTRY (panel_margin_x), 3);
	gtk_table_attach (GTK_TABLE (table), panel_margin_x, 7, 8, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Height"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);

	panel_height = gtk_spin_button_new_with_range (0, 9000, 1);
	gtk_widget_show (panel_height);
	gtk_table_attach (GTK_TABLE (table), panel_height, 2, 3, 1, 2,  GTK_FILL, 0, 0, 0);

	change_paragraph(parent);

	panel_combo_height_type = gtk_combo_box_new_text ();
	gtk_widget_show (panel_combo_height_type);
	gtk_table_attach (GTK_TABLE (table), panel_combo_height_type, 3, 4, 1, 2, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_height_type), _("Percent"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_height_type), _("Pixels"));

	label = gtk_label_new (_("Marging y"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 1, 2, GTK_FILL, 0, 0, 0);

	panel_margin_y = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (panel_margin_y);
	gtk_entry_set_max_length (GTK_ENTRY (panel_margin_y), 3);
	gtk_table_attach (GTK_TABLE (table), panel_margin_y, 7, 8, 1, 2,  GTK_FILL, 0, 0, 0);

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

	table = gtk_table_new (5, 5, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
	for (i = 0; i < 12; ++i) {
		screen_position[i] = gtk_toggle_button_new ();
		gtk_widget_show (screen_position[i]);

		if (i <= 2 || i >= 9)
			gtk_widget_set_size_request (screen_position[i], 30, 15);
		else
			gtk_widget_set_size_request (screen_position[i], 15, 25);

//		g_signal_connect (G_OBJECT (screen_position[i]), "button-press-event", G_CALLBACK (screen_position_pressed));
//		g_signal_connect (G_OBJECT (screen_position[i]), "key-press-event", G_CALLBACK (screen_position_pressed));
	}
	gtk_table_attach_defaults (GTK_TABLE (table), screen_position[0], 1, 2, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), screen_position[1], 2, 3, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), screen_position[2], 3, 4, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), screen_position[3], 0, 1, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE (table), screen_position[4], 0, 1, 2, 3);
	gtk_table_attach_defaults (GTK_TABLE (table), screen_position[5], 0, 1, 3, 4);
	gtk_table_attach_defaults (GTK_TABLE (table), screen_position[6], 4, 5, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE (table), screen_position[7], 4, 5, 2, 3);
	gtk_table_attach_defaults (GTK_TABLE (table), screen_position[8], 4, 5, 3, 4);
	gtk_table_attach_defaults (GTK_TABLE (table), screen_position[9], 1, 2, 4, 5);
	gtk_table_attach_defaults (GTK_TABLE (table), screen_position[10], 2, 3, 4, 5);
	gtk_table_attach_defaults (GTK_TABLE (table), screen_position[11], 3, 4, 4, 5);

	label = gtk_label_new (_("<b>Autohide</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (2, 10, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Autohide"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	panel_autohide = gtk_check_button_new ();
	gtk_widget_show (panel_autohide);
	gtk_table_attach (GTK_TABLE (table), panel_autohide, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Show panel after"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 0, 1, GTK_FILL, 0, 0, 0);

	panel_autohide_show_time = gtk_spin_button_new_with_range (0, 10000, 0.1);
	gtk_widget_show (panel_autohide_show_time);
	gtk_table_attach (GTK_TABLE (table), panel_autohide_show_time, 7, 8, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("seconds"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 8, 9, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Hidden size"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	panel_autohide_size = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (panel_autohide_size);
	gtk_entry_set_max_length (GTK_ENTRY (panel_autohide_size), 3);
	gtk_table_attach (GTK_TABLE (table), panel_autohide_size, 3, 4, 1, 2, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Hide panel after"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 1, 2, GTK_FILL, 0, 0, 0);

	panel_autohide_hide_time = gtk_spin_button_new_with_range (0, 10000, 0.1);
	gtk_widget_show (panel_autohide_hide_time);
	gtk_table_attach (GTK_TABLE (table), panel_autohide_hide_time, 7, 8, 1, 2,  GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("seconds"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 8, 9, 1, 2, GTK_FILL, 0, 0, 0);

	change_paragraph(parent);
	
	label = gtk_label_new (_("<b>Appearance</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (2, 10, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Horizontal padding"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	panel_padding_x = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (panel_padding_x);
	gtk_entry_set_max_length (GTK_ENTRY (panel_padding_x), 3);
	gtk_table_attach (GTK_TABLE (table), panel_padding_x, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Background"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 0, 1, GTK_FILL, 0, 0, 0);

	combo_background = gtk_combo_box_new_text ();
	gtk_widget_show (combo_background);
	gtk_table_attach (GTK_TABLE (table), combo_background, 7, 8, 0, 1, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 1"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 2"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 3"));

	label = gtk_label_new (_("Vertical padding"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	panel_padding_y = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (panel_padding_y);
	gtk_entry_set_max_length (GTK_ENTRY (panel_padding_y), 3);
	gtk_table_attach (GTK_TABLE (table), panel_padding_y, 3, 4, 1, 2,  GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Spacing"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 1, 2, GTK_FILL, 0, 0, 0);

	panel_spacing = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (panel_spacing);
	gtk_entry_set_max_length (GTK_ENTRY (panel_spacing), 3);
	gtk_table_attach (GTK_TABLE (table), panel_spacing, 7, 8, 1, 2, GTK_FILL, 0, 0, 0);

	change_paragraph(parent);

	label = gtk_label_new (_("<b>Window manager</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (2, 12, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Show WM menu"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	panel_wm_menu = gtk_check_button_new ();
	gtk_widget_show (panel_wm_menu);
	gtk_entry_set_max_length (GTK_ENTRY (panel_wm_menu), 3);
	gtk_table_attach (GTK_TABLE (table), panel_wm_menu, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Place in dock"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	panel_dock = gtk_check_button_new ();
	gtk_widget_show (panel_dock);
	gtk_entry_set_max_length (GTK_ENTRY (panel_dock), 3);
	gtk_table_attach (GTK_TABLE (table), panel_dock, 3, 4, 1, 2, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Panel's layer"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 0, 1, GTK_FILL, 0, 0, 0);

	panel_combo_layer = gtk_combo_box_new_text ();
	gtk_widget_show (panel_combo_layer);
	gtk_entry_set_max_length (GTK_ENTRY (panel_combo_layer), 3);
	gtk_table_attach (GTK_TABLE (table), panel_combo_layer, 7, 8, 0, 1, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_layer), _("Top"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_layer), _("Normal"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_layer), _("Bottom"));

	label = gtk_label_new (_("Maximized window"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 1, 2, GTK_FILL, 0, 0, 0);

	panel_combo_strut_policy = gtk_combo_box_new_text ();
	gtk_widget_show (panel_combo_strut_policy);
	gtk_entry_set_max_length (GTK_ENTRY (panel_combo_strut_policy), 3);
	gtk_table_attach (GTK_TABLE (table), panel_combo_strut_policy, 7, 8, 1, 2, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_strut_policy), _("Follow panel size"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_strut_policy), _("Follow hidden size"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_strut_policy), _("Full screen"));

	label = gtk_label_new (_("Monitor"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 10, 11, 0, 1, GTK_FILL, 0, 0, 0);

	panel_combo_monitor = gtk_combo_box_new_text ();
	gtk_widget_show (panel_combo_monitor);
	gtk_table_attach (GTK_TABLE (table), panel_combo_monitor, 11, 12, 0, 1, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_monitor), _("All"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_monitor), _("1"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_monitor), _("2"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_monitor), _("3"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_monitor), _("4"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_monitor), _("5"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (panel_combo_monitor), _("6"));

	change_paragraph(parent);
}


void create_panel_items(GtkWidget  *parent)
{
	GtkWidget  *table, *label;

	table = gtk_table_new (2, 2, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Items order"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

	items_order = gtk_entry_new ();
	gtk_widget_show (items_order);
	gtk_entry_set_width_chars (GTK_ENTRY (items_order), 15);
	gtk_table_attach (GTK_TABLE (table), items_order, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);

	change_paragraph(parent);
}


void create_launcher(GtkWidget  *parent)
{
	GtkWidget  *table, *label;
	GtkWidget  *margin_x, *margin_y, *combo_background;

	label = gtk_label_new (_("<b>Applications</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	change_paragraph(parent);

	label = gtk_label_new (_("<b>Appearance</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (6, 10, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);
	
	label = gtk_label_new (_("Horizontal padding"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	margin_x = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (margin_x);
	gtk_entry_set_max_length (GTK_ENTRY (margin_x), 3);
	gtk_table_attach (GTK_TABLE (table), margin_x, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Vertical padding"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	margin_y = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (margin_y);
	gtk_entry_set_max_length (GTK_ENTRY (margin_y), 3);
	gtk_table_attach (GTK_TABLE (table), margin_y, 3, 4, 1, 2,  GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Spacing"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 2, 3, GTK_FILL, 0, 0, 0);

	margin_x = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (margin_x);
	gtk_entry_set_max_length (GTK_ENTRY (margin_x), 3);
	gtk_table_attach (GTK_TABLE (table), margin_x, 3, 4, 2, 3, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Background"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 0, 1, GTK_FILL, 0, 0, 0);

	combo_background = gtk_combo_box_new_text ();
	gtk_widget_show (combo_background);
	gtk_table_attach (GTK_TABLE (table), combo_background, 7, 8, 0, 1, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 1"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 2"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 3"));

	label = gtk_label_new (_("Icon size"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 1, 2, GTK_FILL, 0, 0, 0);

	margin_x = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (margin_x);
	gtk_entry_set_max_length (GTK_ENTRY (margin_x), 3);
	gtk_table_attach (GTK_TABLE (table), margin_x, 7, 8, 1, 2, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Icon theme"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 2, 3, GTK_FILL, 0, 0, 0);

	combo_background = gtk_combo_box_new_text ();
	gtk_widget_show (combo_background);
	gtk_table_attach (GTK_TABLE (table), combo_background, 7, 8, 2, 3, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Theme 1"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Theme 2"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Theme 3"));

	change_paragraph(parent);
}


void create_taskbar(GtkWidget  *parent)
{
	GtkWidget  *table, *label;

	table = gtk_table_new (2, 2, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Show all desktop"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

	taskbar_show_desktop = gtk_check_button_new ();
	gtk_widget_show (taskbar_show_desktop);
	gtk_entry_set_max_length (GTK_ENTRY (taskbar_show_desktop), 3);
	gtk_table_attach (GTK_TABLE (table), taskbar_show_desktop, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Show taskbar name"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);

	taskbar_show_name = gtk_check_button_new ();
	gtk_widget_show (taskbar_show_name);
	gtk_entry_set_max_length (GTK_ENTRY (taskbar_show_name), 3);
	gtk_table_attach (GTK_TABLE (table), taskbar_show_name, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);

	change_paragraph(parent);

	label = gtk_label_new (_("<b>Taskbar Appearance</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (3, 12, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Horizontal padding"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	taskbar_padding_x = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (taskbar_padding_x);
	gtk_entry_set_max_length (GTK_ENTRY (taskbar_padding_x), 3);
	gtk_table_attach (GTK_TABLE (table), taskbar_padding_x, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Vertical padding"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	taskbar_padding_y = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (taskbar_padding_y);
	gtk_entry_set_max_length (GTK_ENTRY (taskbar_padding_y), 3);
	gtk_table_attach (GTK_TABLE (table), taskbar_padding_y, 3, 4, 1, 2,  GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Spacing"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 2, 3, GTK_FILL, 0, 0, 0);

	taskbar_spacing = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (taskbar_spacing);
	gtk_entry_set_max_length (GTK_ENTRY (taskbar_spacing), 3);
	gtk_table_attach (GTK_TABLE (table), taskbar_spacing, 3, 4, 2, 3, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Inactive background"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 0, 1, GTK_FILL, 0, 0, 0);

	combo_background = gtk_combo_box_new_text ();
	gtk_widget_show (combo_background);
	gtk_table_attach (GTK_TABLE (table), combo_background, 7, 8, 0, 1, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 1"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 2"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 3"));

	label = gtk_label_new (_("Active background"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 1, 2, GTK_FILL, 0, 0, 0);

	combo_background = gtk_combo_box_new_text ();
	gtk_widget_show (combo_background);
	gtk_table_attach (GTK_TABLE (table), combo_background, 7, 8, 1, 2, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 1"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 2"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 3"));

	change_paragraph(parent);

	label = gtk_label_new (_("<b>Name appearance</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (6, 22, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Horizontal padding"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	taskbar_name_padding_x = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (taskbar_name_padding_x);
	gtk_entry_set_max_length (GTK_ENTRY (taskbar_name_padding_x), 3);
	gtk_table_attach (GTK_TABLE (table), taskbar_name_padding_x, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Inactive font color"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	taskbar_name_inactive_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(taskbar_name_inactive_color), TRUE);
	gtk_widget_show (taskbar_name_inactive_color);
	gtk_table_attach (GTK_TABLE (table), taskbar_name_inactive_color, 3, 4, 1, 2, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Active font color"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 2, 3, GTK_FILL, 0, 0, 0);

	taskbar_name_active_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(taskbar_name_active_color), TRUE);
	gtk_widget_show (taskbar_name_active_color);
	gtk_table_attach (GTK_TABLE (table), taskbar_name_active_color, 3, 4, 2, 3, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Font"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 0, 1, GTK_FILL, 0, 0, 0);

	taskbar_name_font = gtk_font_button_new ();
	gtk_widget_show (taskbar_name_font);
	gtk_table_attach (GTK_TABLE (table), taskbar_name_font, 7, 22, 0, 1, GTK_FILL, 0, 0, 0);
	//gtk_font_button_set_show_style (GTK_FONT_BUTTON (taskbar_name_font), FALSE);

	label = gtk_label_new (_("Inactive background"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 1, 2, GTK_FILL, 0, 0, 0);

	combo_background = gtk_combo_box_new_text ();
	gtk_widget_show (combo_background);
	gtk_table_attach (GTK_TABLE (table), combo_background, 7, 8, 1, 2, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 1"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 2"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 3"));

	label = gtk_label_new (_("Active background"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 2, 3, GTK_FILL, 0, 0, 0);

	combo_background = gtk_combo_box_new_text ();
	gtk_widget_show (combo_background);
	gtk_table_attach (GTK_TABLE (table), combo_background, 7, 8, 2, 3, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 1"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 2"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 3"));

	change_paragraph(parent);
}


void create_task(GtkWidget  *parent)
{
	GtkWidget  *table, *label, *notebook, *page_task;
	GtkWidget  *margin_x, *combo_background;

	label = gtk_label_new (_("<b>Mouse action</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (2, 10, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Middle click"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	combo_background = gtk_combo_box_new_text ();
	gtk_widget_show (combo_background);
	gtk_table_attach (GTK_TABLE (table), combo_background, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("none"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("close"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("toggle"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("iconify"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("shade"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("toggle_iconify"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("maximize_restore"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("desktop_left"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("desktop_right"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("next_task"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("previous_task"));

	label = gtk_label_new (_("Wheel scroll up"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 0, 1, GTK_FILL, 0, 0, 0);

	combo_background = gtk_combo_box_new_text ();
	gtk_widget_show (combo_background);
	gtk_table_attach (GTK_TABLE (table), combo_background, 7, 8, 0, 1, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("none"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("close"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("toggle"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("iconify"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("shade"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("toggle_iconify"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("maximize_restore"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("desktop_left"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("desktop_right"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("next_task"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("previous_task"));

	label = gtk_label_new (_("Right click"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	combo_background = gtk_combo_box_new_text ();
	gtk_widget_show (combo_background);
	gtk_table_attach (GTK_TABLE (table), combo_background, 3, 4, 1, 2, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("none"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("close"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("toggle"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("iconify"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("shade"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("toggle_iconify"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("maximize_restore"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("desktop_left"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("desktop_right"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("next_task"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("previous_task"));

	label = gtk_label_new (_("Wheel scroll down"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 1, 2, GTK_FILL, 0, 0, 0);

	combo_background = gtk_combo_box_new_text ();
	gtk_widget_show (combo_background);
	gtk_table_attach (GTK_TABLE (table), combo_background, 7, 8, 1, 2, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("none"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("close"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("toggle"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("iconify"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("shade"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("toggle_iconify"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("maximize_restore"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("desktop_left"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("desktop_right"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("next_task"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("previous_task"));

	change_paragraph(parent);

	label = gtk_label_new (_("<b>Appearance</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (4, 10, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Show icon"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	margin_x = gtk_check_button_new ();
	gtk_widget_show (margin_x);
	gtk_entry_set_max_length (GTK_ENTRY (margin_x), 3);
	gtk_table_attach (GTK_TABLE (table), margin_x, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Show text"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	margin_x = gtk_check_button_new ();
	gtk_widget_show (margin_x);
	gtk_entry_set_max_length (GTK_ENTRY (margin_x), 3);
	gtk_table_attach (GTK_TABLE (table), margin_x, 3, 4, 1, 2, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Align center"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 2, 3, GTK_FILL, 0, 0, 0);

	margin_x = gtk_check_button_new ();
	gtk_widget_show (margin_x);
	gtk_entry_set_max_length (GTK_ENTRY (margin_x), 3);
	gtk_table_attach (GTK_TABLE (table), margin_x, 3, 4, 2, 3, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Font shadow"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 3, 4, GTK_FILL, 0, 0, 0);

	margin_x = gtk_check_button_new ();
	gtk_widget_show (margin_x);
	gtk_entry_set_max_length (GTK_ENTRY (margin_x), 3);
	gtk_table_attach (GTK_TABLE (table), margin_x, 3, 4, 3, 4, GTK_FILL, 0, 0, 0);

	// tasks
	notebook = gtk_notebook_new ();
	gtk_widget_show (notebook);
	gtk_container_set_border_width(GTK_CONTAINER(notebook), 0);
	gtk_box_pack_start (GTK_BOX (parent), notebook, TRUE, TRUE, 0);

	// notebook
	label = gtk_label_new (_("Normal task"));
	gtk_widget_show (label);
	page_task = gtk_vbox_new (FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_task), 10);
	gtk_widget_show (page_task);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_task, label);

	label = gtk_label_new (_("Active task"));
	gtk_widget_show (label);
	page_task = gtk_vbox_new (FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_task), 10);
	gtk_widget_show (page_task);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_task, label);

	label = gtk_label_new (_("Urgent task"));
	gtk_widget_show (label);
	page_task = gtk_vbox_new (FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_task), 10);
	gtk_widget_show (page_task);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_task, label);

	label = gtk_label_new (_("Iconified task"));
	gtk_widget_show (label);
	page_task = gtk_vbox_new (FALSE, DEFAULT_HOR_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(page_task), 10);
	gtk_widget_show (page_task);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_task, label);
}


void create_clock(GtkWidget  *parent)
{
	GtkWidget  *table;
	GtkWidget  *label;

	table = gtk_table_new (1, 2, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

//	change_paragraph(parent);

	label = gtk_label_new (_("<b>Format and timezone</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (3, 10, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("First line format"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	clock_format_line1 = gtk_entry_new ();
	gtk_widget_show (clock_format_line1);
	gtk_entry_set_width_chars (GTK_ENTRY (clock_format_line1), 16);
	gtk_table_attach (GTK_TABLE (table), clock_format_line1, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Second line format"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	clock_format_line2 = gtk_entry_new ();
	gtk_widget_show (clock_format_line2);
	gtk_entry_set_width_chars (GTK_ENTRY (clock_format_line2), 16);
	gtk_table_attach (GTK_TABLE (table), clock_format_line2, 3, 4, 1, 2, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Tooltip format"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 2, 3, GTK_FILL, 0, 0, 0);

	clock_format_tooltip = gtk_entry_new ();
	gtk_widget_show (clock_format_tooltip);
	gtk_entry_set_width_chars (GTK_ENTRY (clock_format_tooltip), 16);
	gtk_table_attach (GTK_TABLE (table), clock_format_tooltip, 3, 4, 2, 3, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("First line timezone"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 0, 1, GTK_FILL, 0, 0, 0);

	clock_tmz_line1 = gtk_entry_new ();
	gtk_widget_show (clock_tmz_line1);
	gtk_entry_set_width_chars (GTK_ENTRY (clock_tmz_line1), 16);
	gtk_table_attach (GTK_TABLE (table), clock_tmz_line1, 7, 8, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Second line timezone"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 1, 2, GTK_FILL, 0, 0, 0);

	clock_tmz_line2 = gtk_entry_new ();
	gtk_widget_show (clock_tmz_line2);
	gtk_entry_set_width_chars (GTK_ENTRY (clock_tmz_line2), 16);
	gtk_table_attach (GTK_TABLE (table), clock_tmz_line2, 7, 8, 1, 2, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Tooltip timezone"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 2, 3, GTK_FILL, 0, 0, 0);

	clock_tmz_tooltip = gtk_entry_new ();
	gtk_widget_show (clock_tmz_tooltip);
	gtk_entry_set_width_chars (GTK_ENTRY (clock_tmz_tooltip), 16);
	gtk_table_attach (GTK_TABLE (table), clock_tmz_tooltip, 7, 8, 2, 3, GTK_FILL, 0, 0, 0);

	change_paragraph(parent);

	label = gtk_label_new (_("<b>Mouse action</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (2, 10, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Left click command"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	clock_left_command = gtk_entry_new ();
	gtk_widget_show (clock_left_command);
	gtk_entry_set_width_chars (GTK_ENTRY (clock_left_command), 16);
	gtk_table_attach (GTK_TABLE (table), clock_left_command, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Right click command"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 0, 1, GTK_FILL, 0, 0, 0);

	clock_right_command = gtk_entry_new ();
	gtk_widget_show (clock_right_command);
	gtk_entry_set_width_chars (GTK_ENTRY (clock_right_command), 16);
	gtk_table_attach (GTK_TABLE (table), clock_right_command, 7, 8, 0, 1, GTK_FILL, 0, 0, 0);

	change_paragraph(parent);

	label = gtk_label_new (_("<b>Appearance</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (3, 22, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Horizontal padding"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	clock_padding_x = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (clock_padding_x);
	gtk_entry_set_max_length (GTK_ENTRY (clock_padding_x), 3);
	gtk_table_attach (GTK_TABLE (table), clock_padding_x, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Vertical padding"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	clock_padding_y = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (clock_padding_y);
	gtk_entry_set_max_length (GTK_ENTRY (clock_padding_y), 3);
	gtk_table_attach (GTK_TABLE (table), clock_padding_y, 3, 4, 1, 2,  GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Background"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 2, 3, GTK_FILL, 0, 0, 0);

	combo_background = gtk_combo_box_new_text ();
	gtk_widget_show (combo_background);
	gtk_table_attach (GTK_TABLE (table), combo_background, 3, 4, 2, 3, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 1"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 2"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 3"));

	label = gtk_label_new (_("Font first line"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 0, 1, GTK_FILL, 0, 0, 0);

	clock_font_line1 = gtk_font_button_new ();
	gtk_widget_show (clock_font_line1);
	gtk_table_attach (GTK_TABLE (table), clock_font_line1, 7, 22, 0, 1, GTK_FILL, 0, 0, 0);
//	gtk_font_button_set_show_style (GTK_FONT_BUTTON (clock_font_line1), FALSE);

	label = gtk_label_new (_("Font second line"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 1, 2, GTK_FILL, 0, 0, 0);

	clock_font_line2 = gtk_font_button_new ();
	gtk_widget_show (clock_font_line2);
	gtk_table_attach (GTK_TABLE (table), clock_font_line2, 7, 22, 1, 2, GTK_FILL, 0, 0, 0);
//	gtk_font_button_set_show_style (GTK_FONT_BUTTON (clock_font_line2), FALSE);

	label = gtk_label_new (_("Font color"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 2, 3, GTK_FILL, 0, 0, 0);

	clock_font_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(clock_font_color), TRUE);
	gtk_widget_show (clock_font_color);
	gtk_table_attach (GTK_TABLE (table), clock_font_color, 7, 8, 2, 3, GTK_FILL, 0, 0, 0);

	change_paragraph(parent);
}


void create_systemtray(GtkWidget  *parent)
{
	GtkWidget  *table;
	GtkWidget  *label;

	table = gtk_table_new (2, 2, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Icon ordering"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);

	systray_icon_order = gtk_combo_box_new_text ();
	gtk_widget_show (systray_icon_order);
	gtk_table_attach (GTK_TABLE (table), systray_icon_order, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (systray_icon_order), _("Ascending"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (systray_icon_order), _("Descending"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (systray_icon_order), _("Left to right"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (systray_icon_order), _("Right to left"));

	change_paragraph(parent);

	label = gtk_label_new (_("<b>Appearance</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (6, 10, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);
	
	label = gtk_label_new (_("Horizontal padding"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	systray_padding_x = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (systray_padding_x);
	gtk_entry_set_max_length (GTK_ENTRY (systray_padding_x), 3);
	gtk_table_attach (GTK_TABLE (table), systray_padding_x, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Vertical padding"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	systray_padding_y = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (systray_padding_y);
	gtk_entry_set_max_length (GTK_ENTRY (systray_padding_y), 3);
	gtk_table_attach (GTK_TABLE (table), systray_padding_y, 3, 4, 1, 2,  GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Spacing"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 2, 3, GTK_FILL, 0, 0, 0);

	systray_spacing = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (systray_spacing);
	gtk_entry_set_max_length (GTK_ENTRY (systray_spacing), 3);
	gtk_table_attach (GTK_TABLE (table), systray_spacing, 3, 4, 2, 3, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Background"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 3, 4, GTK_FILL, 0, 0, 0);

	combo_background = gtk_combo_box_new_text ();
	gtk_widget_show (combo_background);
	gtk_table_attach (GTK_TABLE (table), combo_background, 3, 4, 3, 4, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 1"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 2"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 3"));

	label = gtk_label_new (_("Icon size"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 0, 1, GTK_FILL, 0, 0, 0);

	systray_icon_size = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (systray_icon_size);
	gtk_entry_set_max_length (GTK_ENTRY (systray_icon_size), 3);
	gtk_table_attach (GTK_TABLE (table), systray_icon_size, 7, 8, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Icon opacity"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 1, 2, GTK_FILL, 0, 0, 0);

	systray_icon_opacity = gtk_spin_button_new_with_range (0, 100, 1);
	gtk_widget_show (systray_icon_opacity);
	gtk_entry_set_max_length (GTK_ENTRY (systray_icon_opacity), 3);
	gtk_table_attach (GTK_TABLE (table), systray_icon_opacity, 7, 8, 1, 2, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Icon saturation"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 2, 3, GTK_FILL, 0, 0, 0);

	systray_icon_saturation = gtk_spin_button_new_with_range (-100, 100, 1);
	gtk_widget_show (systray_icon_saturation);
	gtk_entry_set_max_length (GTK_ENTRY (systray_icon_saturation), 4);
	gtk_table_attach (GTK_TABLE (table), systray_icon_saturation, 7, 8, 2, 3, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Icon brightness"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 3, 4, GTK_FILL, 0, 0, 0);

	systray_icon_brightness = gtk_spin_button_new_with_range (-100, 100, 1);
	gtk_widget_show (systray_icon_brightness);
	gtk_entry_set_max_length (GTK_ENTRY (systray_icon_brightness), 4);
	gtk_table_attach (GTK_TABLE (table), systray_icon_brightness, 7, 8, 3, 4, GTK_FILL, 0, 0, 0);

}


void create_battery(GtkWidget  *parent)
{
	GtkWidget  *table, *label;

	table = gtk_table_new (1, 2, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("<b>Event</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (2, 10, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Hide if charge higher than"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	battery_hide_if_higher = gtk_spin_button_new_with_range (0, 101, 1);
	gtk_widget_show (battery_hide_if_higher);
	gtk_entry_set_max_length (GTK_ENTRY (battery_hide_if_higher), 3);
	gtk_table_attach (GTK_TABLE (table), battery_hide_if_higher, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("%"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 4, 5, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Alert if charge lower than"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	battery_alert_if_lower = gtk_spin_button_new_with_range (0, 100, 1);
	gtk_widget_show (battery_alert_if_lower);
	gtk_entry_set_max_length (GTK_ENTRY (battery_alert_if_lower), 3);
	gtk_table_attach (GTK_TABLE (table), battery_alert_if_lower, 3, 4, 1, 2, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("%"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 4, 5, 1, 2, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Alert command"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 7, 8, 1, 2, GTK_FILL, 0, 0, 0);

	battery_alert_cmd = gtk_entry_new ();
	gtk_widget_show (battery_alert_cmd);
	gtk_entry_set_width_chars (GTK_ENTRY (battery_alert_cmd), 28);
	gtk_table_attach (GTK_TABLE (table), battery_alert_cmd, 8, 9, 1, 2, GTK_FILL, 0, 0, 0);


	change_paragraph(parent);

	label = gtk_label_new (_("<b>Appearance</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (4, 22, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Horizontal padding"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	battery_padding_x = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (battery_padding_x);
	gtk_entry_set_max_length (GTK_ENTRY (battery_padding_x), 3);
	gtk_table_attach (GTK_TABLE (table), battery_padding_x, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Vertical padding"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	battery_padding_y = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (battery_padding_y);
	gtk_entry_set_max_length (GTK_ENTRY (battery_padding_y), 3);
	gtk_table_attach (GTK_TABLE (table), battery_padding_y, 3, 4, 1, 2,  GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Background"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 2, 3, GTK_FILL, 0, 0, 0);

	combo_background = gtk_combo_box_new_text ();
	gtk_widget_show (combo_background);
	gtk_table_attach (GTK_TABLE (table), combo_background, 3, 4, 2, 3, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 1"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 2"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 3"));

	label = gtk_label_new (_("Font first line"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 0, 1, GTK_FILL, 0, 0, 0);

	battery_font_line1 = gtk_font_button_new ();
	gtk_widget_show (battery_font_line1);
	gtk_table_attach (GTK_TABLE (table), battery_font_line1, 7, 22, 0, 1, GTK_FILL, 0, 0, 0);
//	gtk_font_button_set_show_style (GTK_FONT_BUTTON (battery_font_line1), FALSE);

	label = gtk_label_new (_("Font second line"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 1, 2, GTK_FILL, 0, 0, 0);

	battery_font_line2 = gtk_font_button_new ();
	gtk_widget_show (battery_font_line2);
	gtk_table_attach (GTK_TABLE (table), battery_font_line2, 7, 22, 1, 2, GTK_FILL, 0, 0, 0);
//	gtk_font_button_set_show_style (GTK_FONT_BUTTON (battery_font_line2), FALSE);

	label = gtk_label_new (_("Font color"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 2, 3, GTK_FILL, 0, 0, 0);

	battery_font_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(battery_font_color), TRUE);
	gtk_widget_show (battery_font_color);
	gtk_table_attach (GTK_TABLE (table), battery_font_color, 7, 8, 2, 3, GTK_FILL, 0, 0, 0);

	change_paragraph(parent);
}


void create_tooltip(GtkWidget  *parent)
{
	GtkWidget  *table;
	GtkWidget  *margin_x, *margin_y, *combo_background;
	GtkWidget  *label;

	table = gtk_table_new (1, 2, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Show tooltip"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

	margin_x = gtk_check_button_new ();
	gtk_widget_show (margin_x);
	gtk_table_attach (GTK_TABLE (table), margin_x, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);

	change_paragraph(parent);
	
	label = gtk_label_new (_("<b>Timing</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (2, 22, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Show tooltip after"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	margin_x = gtk_spin_button_new_with_range (0, 10000, 0.1);
	gtk_widget_show (margin_x);
	gtk_table_attach (GTK_TABLE (table), margin_x, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("seconds"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 4, 5, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Hide tooltip after"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	margin_y = gtk_spin_button_new_with_range (0, 10000, 0.1);
	gtk_widget_show (margin_y);
	gtk_table_attach (GTK_TABLE (table), margin_y, 3, 4, 1, 2,  GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("seconds"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 4, 5, 1, 2, GTK_FILL, 0, 0, 0);

	change_paragraph(parent);
	
	label = gtk_label_new (_("<b>Appearance</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_widget_show (label);
	gtk_box_pack_start(GTK_BOX (parent), label, FALSE, FALSE, 0);

	table = gtk_table_new (3, 10, FALSE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table), ROW_SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table), COL_SPACING);

	label = gtk_label_new (_("Horizontal padding"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	margin_x = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (margin_x);
	gtk_entry_set_max_length (GTK_ENTRY (margin_x), 3);
	gtk_table_attach (GTK_TABLE (table), margin_x, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Vertical padding"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

	margin_y = gtk_spin_button_new_with_range (0, 500, 1);
	gtk_widget_show (margin_y);
	gtk_entry_set_max_length (GTK_ENTRY (margin_y), 3);
	gtk_table_attach (GTK_TABLE (table), margin_y, 3, 4, 1, 2,  GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Background"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 2, 3, GTK_FILL, 0, 0, 0);

	combo_background = gtk_combo_box_new_text ();
	gtk_widget_show (combo_background);
	gtk_table_attach (GTK_TABLE (table), combo_background, 3, 4, 2, 3, GTK_FILL, 0, 0, 0);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 1"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 2"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_background), _("Id 3"));

	label = gtk_label_new (_("Font"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 0, 1, GTK_FILL, 0, 0, 0);

	margin_x = gtk_font_button_new ();
	gtk_widget_show (margin_x);
	gtk_table_attach (GTK_TABLE (table), margin_x, 7, 22, 0, 1, GTK_FILL, 0, 0, 0);
//	gtk_font_button_set_show_style (GTK_FONT_BUTTON (margin_x), FALSE);

	label = gtk_label_new (_("Font color"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 6, 7, 1, 2, GTK_FILL, 0, 0, 0);

	margin_x = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(margin_x), TRUE);
	gtk_widget_show (margin_x);
	gtk_table_attach (GTK_TABLE (table), margin_x, 7, 8, 1, 2, GTK_FILL, 0, 0, 0);

	change_paragraph(parent);
}


void create_background(GtkWidget  *parent)
{

	change_paragraph(parent);
}

