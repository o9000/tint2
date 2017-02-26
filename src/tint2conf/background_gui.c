#include "background_gui.h"
#include "gradient_gui.h"

GtkListStore *backgrounds;
GtkWidget *current_background, *background_fill_color, *background_border_color, *background_gradient,
    *background_fill_color_over, *background_border_color_over, *background_gradient_over, *background_fill_color_press,
    *background_border_color_press, *background_gradient_press, *background_border_width, *background_corner_radius,
    *background_border_sides_top, *background_border_sides_bottom, *background_border_sides_left,
    *background_border_sides_right;

GtkWidget *create_background_combo(const char *label)
{
	GtkWidget *combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(backgrounds));
	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "pixbuf", bgColPixbuf, NULL);
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "wrap-mode", PANGO_WRAP_WORD, NULL);
	g_object_set(renderer, "wrap-width", 300, NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", bgColText, NULL);
	g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(background_combo_changed), (void *)label);
	return combo;
}

void background_combo_changed(GtkWidget *widget, gpointer data)
{
	gchar *combo_text = (gchar *)data;
	if (!combo_text || g_str_equal(combo_text, ""))
		return;
	int selected_index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

	int index;
	for (index = 0;; index++) {
		GtkTreePath *path;
		GtkTreeIter iter;

		path = gtk_tree_path_new_from_indices(index, -1);
		gboolean found = gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
		gtk_tree_path_free(path);

		if (!found) {
			break;
		}

		gchar *text;
		gtk_tree_model_get(GTK_TREE_MODEL(backgrounds), &iter, bgColText, &text, -1);
		gchar **parts = g_strsplit(text, ", ", -1);
		int ifound;
		for (ifound = 0; parts[ifound]; ifound++) {
			if (g_str_equal(parts[ifound], combo_text))
				break;
		}
		if (parts[ifound] && index != selected_index) {
			for (; parts[ifound + 1]; ifound++) {
				gchar *tmp = parts[ifound];
				parts[ifound] = parts[ifound + 1];
				parts[ifound + 1] = tmp;
			}
			g_free(parts[ifound]);
			parts[ifound] = NULL;
			text = g_strjoinv(", ", parts);
			g_strfreev(parts);
			gtk_list_store_set(backgrounds, &iter, bgColText, text, -1);
			g_free(text);
		} else if (!parts[ifound] && index == selected_index) {
			if (!ifound) {
				text = g_strdup(combo_text);
			} else {
				for (ifound = 0; parts[ifound]; ifound++) {
					if (compare_strings(combo_text, parts[ifound]) < 0)
						break;
				}
				if (parts[ifound]) {
					gchar *tmp = parts[ifound];
					parts[ifound] = g_strconcat(combo_text, ", ", tmp, NULL);
					g_free(tmp);
				} else {
					ifound--;
					gchar *tmp = parts[ifound];
					parts[ifound] = g_strconcat(tmp, ", ", combo_text, NULL);
					g_free(tmp);
				}
				text = g_strjoinv(", ", parts);
				g_strfreev(parts);
			}
			gtk_list_store_set(backgrounds, &iter, bgColText, text, -1);
			g_free(text);
		}
	}
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
	                                 GTK_TYPE_INT,
	                                 GTK_TYPE_INT,
	                                 GTK_TYPE_STRING,
	                                 GDK_TYPE_COLOR,
	                                 GTK_TYPE_INT,
	                                 GDK_TYPE_COLOR,
	                                 GTK_TYPE_INT,
									 GTK_TYPE_INT,
	                                 GDK_TYPE_COLOR,
	                                 GTK_TYPE_INT,
	                                 GDK_TYPE_COLOR,
	                                 GTK_TYPE_INT,
									 GTK_TYPE_INT,
	                                 GTK_TYPE_BOOL,
	                                 GTK_TYPE_BOOL,
	                                 GTK_TYPE_BOOL,
	                                 GTK_TYPE_BOOL);

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
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	current_background = create_background_combo(NULL);
	gtk_widget_show(current_background);
	gtk_table_attach(GTK_TABLE(table), current_background, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, current_background, _("Selects the background you would like to modify"), NULL);

	button = gtk_button_new_from_stock("gtk-add");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(background_duplicate), NULL);
	gtk_widget_show(button);
	gtk_table_attach(GTK_TABLE(table), button, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, button, _("Creates a copy of the current background"), NULL);

	button = gtk_button_new_from_stock("gtk-remove");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(background_delete), NULL);
	gtk_widget_show(button);
	gtk_table_attach(GTK_TABLE(table), button, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, button, _("Deletes the current background"), NULL);

	table = gtk_table_new(4, 4, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

	row++, col = 2;
	label = gtk_label_new(_("Fill color"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	background_fill_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(background_fill_color), TRUE);
	gtk_widget_show(background_fill_color);
	gtk_table_attach(GTK_TABLE(table), background_fill_color, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, background_fill_color, _("The fill color of the current background"), NULL);

	row++, col = 2;
	label = gtk_label_new(_("Border color"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	background_border_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(background_border_color), TRUE);
	gtk_widget_show(background_border_color);
	gtk_table_attach(GTK_TABLE(table), background_border_color, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, background_border_color, _("The border color of the current background"), NULL);

	row++, col = 2;
	label = gtk_label_new(_("Gradient"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	background_gradient = create_gradient_combo();
	gtk_widget_show(background_gradient);
	gtk_table_attach(GTK_TABLE(table), background_gradient, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	row++, col = 2;
	label = gtk_label_new(_("Fill color (mouse over)"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	background_fill_color_over = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(background_fill_color_over), TRUE);
	gtk_widget_show(background_fill_color_over);
	gtk_table_attach(GTK_TABLE(table), background_fill_color_over, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips,
	                     background_fill_color_over,
	                     _("The fill color of the current background on mouse over"),
	                     NULL);

	row++, col = 2;
	label = gtk_label_new(_("Border color (mouse over)"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	background_border_color_over = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(background_border_color_over), TRUE);
	gtk_widget_show(background_border_color_over);
	gtk_table_attach(GTK_TABLE(table), background_border_color_over, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips,
	                     background_border_color_over,
	                     _("The border color of the current background on mouse over"),
	                     NULL);

	row++, col = 2;
	label = gtk_label_new(_("Gradient (mouse over)"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	background_gradient_over = create_gradient_combo();
	gtk_widget_show(background_gradient_over);
	gtk_table_attach(GTK_TABLE(table), background_gradient_over, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	row++, col = 2;
	label = gtk_label_new(_("Fill color (pressed)"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	background_fill_color_press = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(background_fill_color_press), TRUE);
	gtk_widget_show(background_fill_color_press);
	gtk_table_attach(GTK_TABLE(table), background_fill_color_press, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips,
	                     background_fill_color_press,
	                     _("The fill color of the current background on mouse button press"),
	                     NULL);

	row++, col = 2;
	label = gtk_label_new(_("Border color (pressed)"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	background_border_color_press = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(background_border_color_press), TRUE);
	gtk_widget_show(background_border_color_press);
	gtk_table_attach(GTK_TABLE(table), background_border_color_press, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips,
	                     background_border_color_press,
	                     _("The border color of the current background on mouse button press"),
	                     NULL);

	row++, col = 2;
	label = gtk_label_new(_("Gradient (pressed)"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	background_gradient_press = create_gradient_combo();
	gtk_widget_show(background_gradient_press);
	gtk_table_attach(GTK_TABLE(table), background_gradient_press, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	row++, col = 2;
	label = gtk_label_new(_("Border width"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	background_border_width = gtk_spin_button_new_with_range(0, 100, 1);
	gtk_widget_show(background_border_width);
	gtk_table_attach(GTK_TABLE(table), background_border_width, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips,
	                     background_border_width,
	                     _("The width of the border of the current background, in pixels"),
	                     NULL);

	row++, col = 2;
	label = gtk_label_new(_("Corner radius"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	background_corner_radius = gtk_spin_button_new_with_range(0, 100, 1);
	gtk_widget_show(background_corner_radius);
	gtk_table_attach(GTK_TABLE(table), background_corner_radius, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_tooltips_set_tip(tooltips, background_corner_radius, _("The corner radius of the current background"), NULL);

	row++;
	col = 2;
	label = gtk_label_new(_("Border sides"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	background_border_sides_top = gtk_check_button_new_with_label(_("Top"));
	gtk_widget_show(background_border_sides_top);
	gtk_table_attach(GTK_TABLE(table), background_border_sides_top, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	background_border_sides_bottom = gtk_check_button_new_with_label(_("Bottom"));
	gtk_widget_show(background_border_sides_bottom);
	gtk_table_attach(GTK_TABLE(table), background_border_sides_bottom, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	background_border_sides_left = gtk_check_button_new_with_label(_("Left"));
	gtk_widget_show(background_border_sides_left);
	gtk_table_attach(GTK_TABLE(table), background_border_sides_left, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	background_border_sides_right = gtk_check_button_new_with_label(_("Right"));
	gtk_widget_show(background_border_sides_right);
	gtk_table_attach(GTK_TABLE(table), background_border_sides_right, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	g_signal_connect(G_OBJECT(current_background), "changed", G_CALLBACK(current_background_changed), NULL);
	g_signal_connect(G_OBJECT(background_fill_color), "color-set", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_border_color), "color-set", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_gradient), "changed", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_fill_color_over), "color-set", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_border_color_over), "color-set", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_gradient_over), "changed", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_fill_color_press), "color-set", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_border_color_press), "color-set", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_gradient_over), "changed", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_border_width), "value-changed", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_corner_radius), "value-changed", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_border_sides_top), "toggled", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_border_sides_bottom), "toggled", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_border_sides_left), "toggled", G_CALLBACK(background_update), NULL);
	g_signal_connect(G_OBJECT(background_border_sides_right), "toggled", G_CALLBACK(background_update), NULL);

	change_paragraph(parent);
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
	gboolean sideTop = TRUE;
	gboolean sideBottom = TRUE;
	gboolean sideLeft = TRUE;
	gboolean sideRight = TRUE;
	GdkColor fillColor;
	cairoColor2GdkColor(0, 0, 0, &fillColor);
	int fillOpacity = 0;
	GdkColor fillColor2;
	cairoColor2GdkColor(0, 0, 0, &fillColor2);
	GdkColor borderColor;
	cairoColor2GdkColor(0, 0, 0, &borderColor);
	int borderOpacity = 0;

	GdkColor fillColorOver;
	cairoColor2GdkColor(0, 0, 0, &fillColorOver);
	int fillOpacityOver = 0;
	GdkColor borderColorOver;
	cairoColor2GdkColor(0, 0, 0, &borderColorOver);
	int borderOpacityOver = 0;

	GdkColor fillColorPress;
	cairoColor2GdkColor(0, 0, 0, &fillColorPress);
	int fillOpacityPress = 0;
	GdkColor borderColorPress;
	cairoColor2GdkColor(0, 0, 0, &borderColorPress);
	int borderOpacityPress = 0;

	int index = 0;
	GtkTreeIter iter;

	gtk_list_store_append(backgrounds, &iter);
	gtk_list_store_set(backgrounds,
	                   &iter,
	                   bgColPixbuf,
	                   NULL,
	                   bgColFillColor,
	                   &fillColor,
	                   bgColFillOpacity,
	                   fillOpacity,
	                   bgColBorderColor,
	                   &borderColor,
	                   bgColBorderOpacity,
	                   borderOpacity,
					   bgColGradientId,
					   -1,
	                   bgColBorderWidth,
	                   b,
	                   bgColCornerRadius,
	                   r,
	                   bgColText,
	                   "",
	                   bgColFillColorOver,
	                   &fillColorOver,
	                   bgColFillOpacityOver,
	                   fillOpacityOver,
	                   bgColBorderColorOver,
	                   &borderColorOver,
	                   bgColBorderOpacityOver,
	                   borderOpacityOver,
					   bgColGradientIdOver,
					   -1,
	                   bgColFillColorPress,
	                   &fillColorPress,
	                   bgColFillOpacityPress,
	                   fillOpacityPress,
	                   bgColBorderColorPress,
	                   &borderColorPress,
	                   bgColBorderOpacityPress,
	                   borderOpacityPress,
					   bgColGradientIdPress,
					   -1,
	                   bgColBorderSidesTop,
	                   sideTop,
	                   bgColBorderSidesBottom,
	                   sideBottom,
	                   bgColBorderSidesLeft,
	                   sideLeft,
	                   bgColBorderSidesRight,
	                   sideRight,
	                   -1);

	background_update_image(index);
	gtk_combo_box_set_active(GTK_COMBO_BOX(current_background), get_model_length(GTK_TREE_MODEL(backgrounds)) - 1);
	current_background_changed(0, 0);
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
	gboolean sideTop;
	gboolean sideBottom;
	gboolean sideLeft;
	gboolean sideRight;
	GdkColor *fillColor;
	int fillOpacity;
	GdkColor *borderColor;
	int borderOpacity;
	GdkColor *fillColorOver;
	int fillOpacityOver;
	GdkColor *borderColorOver;
	int borderOpacityOver;
	GdkColor *fillColorPress;
	int fillOpacityPress;
	GdkColor *borderColorPress;
	int borderOpacityPress;
	int gradient_id, gradient_id_over, gradient_id_press;

	gtk_tree_model_get(GTK_TREE_MODEL(backgrounds),
	                   &iter,
	                   bgColFillColor,
	                   &fillColor,
	                   bgColFillOpacity,
	                   &fillOpacity,
	                   bgColBorderColor,
	                   &borderColor,
	                   bgColBorderOpacity,
	                   &borderOpacity,
	                   bgColFillColorOver,
	                   &fillColorOver,
	                   bgColFillOpacityOver,
	                   &fillOpacityOver,
	                   bgColBorderColorOver,
	                   &borderColorOver,
	                   bgColBorderOpacityOver,
	                   &borderOpacityOver,
	                   bgColFillColorPress,
	                   &fillColorPress,
	                   bgColFillOpacityPress,
	                   &fillOpacityPress,
	                   bgColBorderColorPress,
	                   &borderColorPress,
	                   bgColBorderOpacityPress,
	                   &borderOpacityPress,
	                   bgColBorderWidth,
	                   &b,
	                   bgColCornerRadius,
	                   &r,
	                   bgColBorderSidesTop,
	                   &sideTop,
	                   bgColBorderSidesBottom,
	                   &sideBottom,
	                   bgColBorderSidesLeft,
	                   &sideLeft,
	                   bgColBorderSidesRight,
	                   &sideRight,
					   bgColGradientId,
					   &gradient_id,
					   bgColGradientIdOver,
					   &gradient_id_over,
					   bgColGradientIdPress,
					   &gradient_id_press,
	                   -1);

	gtk_list_store_append(backgrounds, &iter);
	gtk_list_store_set(backgrounds,
	                   &iter,
	                   bgColPixbuf,
	                   NULL,
	                   bgColFillColor,
	                   fillColor,
	                   bgColFillOpacity,
	                   fillOpacity,
	                   bgColBorderColor,
	                   borderColor,
					   bgColGradientId,
					   gradient_id,
	                   bgColBorderOpacity,
	                   borderOpacity,
	                   bgColText,
	                   "",
	                   bgColFillColorOver,
	                   fillColorOver,
	                   bgColFillOpacityOver,
	                   fillOpacityOver,
	                   bgColBorderColorOver,
	                   borderColorOver,
	                   bgColBorderOpacityOver,
	                   borderOpacityOver,
					   bgColGradientIdOver,
					   gradient_id_over,
	                   bgColFillColorPress,
	                   fillColorPress,
	                   bgColFillOpacityPress,
	                   fillOpacityPress,
	                   bgColBorderColorPress,
	                   borderColorPress,
	                   bgColBorderOpacityPress,
	                   borderOpacityPress,
					   bgColGradientIdPress,
					   gradient_id_press,
	                   bgColBorderWidth,
	                   b,
	                   bgColCornerRadius,
	                   r,
	                   bgColBorderSidesTop,
	                   sideTop,
	                   bgColBorderSidesBottom,
	                   sideBottom,
	                   bgColBorderSidesLeft,
	                   sideLeft,
	                   bgColBorderSidesRight,
	                   sideRight,
	                   -1);
	g_boxed_free(GDK_TYPE_COLOR, fillColor);
	g_boxed_free(GDK_TYPE_COLOR, borderColor);
	g_boxed_free(GDK_TYPE_COLOR, fillColorOver);
	g_boxed_free(GDK_TYPE_COLOR, borderColorOver);
	g_boxed_free(GDK_TYPE_COLOR, fillColorPress);
	g_boxed_free(GDK_TYPE_COLOR, borderColorPress);
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

	gtk_tree_model_get(GTK_TREE_MODEL(backgrounds),
	                   &iter,
	                   bgColFillColor,
	                   &fillColor,
	                   bgColFillOpacity,
	                   &fillOpacity,
	                   bgColBorderColor,
	                   &borderColor,
	                   bgColBorderOpacity,
	                   &borderOpacity,
	                   bgColBorderWidth,
	                   &b,
	                   bgColCornerRadius,
	                   &r,
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

	int gradient_index = gtk_combo_box_get_active(GTK_COMBO_BOX(background_gradient));
	if (index >= 1 && gradient_index >= 1) {
		GradientConfig *g = (GradientConfig *)g_list_nth(gradients, (guint)gradient_index)->data;
		gradient_draw(cr, g, w, h, TRUE);
	}

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

	gtk_list_store_set(backgrounds, &iter, bgColPixbuf, pixbuf, -1);
	if (pixbuf)
		g_object_unref(pixbuf);
}

void background_force_update()
{
	background_update(NULL, NULL);
}

static gboolean background_updates_disabled = FALSE;
void background_update(GtkWidget *widget, gpointer data)
{
	if (background_updates_disabled)
		return;
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

	r = gtk_spin_button_get_value(GTK_SPIN_BUTTON(background_corner_radius));
	b = gtk_spin_button_get_value(GTK_SPIN_BUTTON(background_border_width));

	gboolean sideTop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(background_border_sides_top));
	gboolean sideBottom = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(background_border_sides_bottom));
	gboolean sideLeft = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(background_border_sides_left));
	gboolean sideRight = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(background_border_sides_right));

	GdkColor fillColor;
	int fillOpacity;
	GdkColor borderColor;
	int borderOpacity;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(background_fill_color), &fillColor);
	fillOpacity = MIN(100, 0.5 + gtk_color_button_get_alpha(GTK_COLOR_BUTTON(background_fill_color)) * 100.0 / 0xffff);
	gtk_color_button_get_color(GTK_COLOR_BUTTON(background_border_color), &borderColor);
	borderOpacity =
	    MIN(100, 0.5 + gtk_color_button_get_alpha(GTK_COLOR_BUTTON(background_border_color)) * 100.0 / 0xffff);
	int gradient_id = gtk_combo_box_get_active(GTK_COMBO_BOX(background_gradient));

	GdkColor fillColorOver;
	int fillOpacityOver;
	GdkColor borderColorOver;
	int borderOpacityOver;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(background_fill_color_over), &fillColorOver);
	fillOpacityOver =
	    MIN(100, 0.5 + gtk_color_button_get_alpha(GTK_COLOR_BUTTON(background_fill_color_over)) * 100.0 / 0xffff);
	gtk_color_button_get_color(GTK_COLOR_BUTTON(background_border_color_over), &borderColorOver);
	borderOpacityOver =
	    MIN(100, 0.5 + gtk_color_button_get_alpha(GTK_COLOR_BUTTON(background_border_color_over)) * 100.0 / 0xffff);
	int gradient_id_over = gtk_combo_box_get_active(GTK_COMBO_BOX(background_gradient_over));

	GdkColor fillColorPress;
	int fillOpacityPress;
	GdkColor borderColorPress;
	int borderOpacityPress;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(background_fill_color_press), &fillColorPress);
	fillOpacityPress =
	    MIN(100, 0.5 + gtk_color_button_get_alpha(GTK_COLOR_BUTTON(background_fill_color_press)) * 100.0 / 0xffff);
	gtk_color_button_get_color(GTK_COLOR_BUTTON(background_border_color_press), &borderColorPress);
	borderOpacityPress =
	    MIN(100, 0.5 + gtk_color_button_get_alpha(GTK_COLOR_BUTTON(background_border_color_press)) * 100.0 / 0xffff);
	int gradient_id_press = gtk_combo_box_get_active(GTK_COMBO_BOX(background_gradient_press));

	gtk_list_store_set(backgrounds,
	                   &iter,
	                   bgColPixbuf,
	                   NULL,
	                   bgColFillColor,
	                   &fillColor,
	                   bgColFillOpacity,
	                   fillOpacity,
	                   bgColBorderColor,
	                   &borderColor,
	                   bgColBorderOpacity,
	                   borderOpacity,
					   bgColGradientId,
					   gradient_id,
	                   bgColFillColorOver,
	                   &fillColorOver,
	                   bgColFillOpacityOver,
	                   fillOpacityOver,
	                   bgColBorderColorOver,
	                   &borderColorOver,
	                   bgColBorderOpacityOver,
	                   borderOpacityOver,
					   bgColGradientIdOver,
					   gradient_id_over,
	                   bgColFillColorPress,
	                   &fillColorPress,
	                   bgColFillOpacityPress,
	                   fillOpacityPress,
	                   bgColBorderColorPress,
	                   &borderColorPress,
	                   bgColBorderOpacityPress,
	                   borderOpacityPress,
					   bgColGradientIdPress,
					   gradient_id_press,
	                   bgColBorderWidth,
	                   b,
	                   bgColCornerRadius,
	                   r,
	                   bgColBorderSidesTop,
	                   sideTop,
	                   bgColBorderSidesBottom,
	                   sideBottom,
	                   bgColBorderSidesLeft,
	                   sideLeft,
	                   bgColBorderSidesRight,
	                   sideRight,
	                   -1);
	background_update_image(index);
}

void current_background_changed(GtkWidget *widget, gpointer data)
{
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_background));
	if (index < 0)
		return;

	background_updates_disabled = TRUE;

	GtkTreePath *path;
	GtkTreeIter iter;

	path = gtk_tree_path_new_from_indices(index, -1);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
	gtk_tree_path_free(path);

	int r;
	int b;

	gboolean sideTop;
	gboolean sideBottom;
	gboolean sideLeft;
	gboolean sideRight;

	GdkColor *fillColor;
	int fillOpacity;
	GdkColor *borderColor;
	int borderOpacity;
	GdkColor *fillColorOver;
	int fillOpacityOver;
	GdkColor *borderColorOver;
	int borderOpacityOver;
	GdkColor *fillColorPress;
	int fillOpacityPress;
	GdkColor *borderColorPress;
	int borderOpacityPress;
	int gradient_id, gradient_id_over, gradient_id_press;

	gtk_tree_model_get(GTK_TREE_MODEL(backgrounds),
	                   &iter,
	                   bgColFillColor,
	                   &fillColor,
	                   bgColFillOpacity,
	                   &fillOpacity,
	                   bgColBorderColor,
	                   &borderColor,
	                   bgColBorderOpacity,
	                   &borderOpacity,
					   bgColGradientId,
					   &gradient_id,
	                   bgColFillColorOver,
	                   &fillColorOver,
	                   bgColFillOpacityOver,
	                   &fillOpacityOver,
	                   bgColBorderColorOver,
	                   &borderColorOver,
	                   bgColBorderOpacityOver,
	                   &borderOpacityOver,
					   bgColGradientIdOver,
					   &gradient_id_over,
	                   bgColFillColorPress,
	                   &fillColorPress,
	                   bgColFillOpacityPress,
	                   &fillOpacityPress,
	                   bgColBorderColorPress,
	                   &borderColorPress,
	                   bgColBorderOpacityPress,
	                   &borderOpacityPress,
					   bgColGradientIdPress,
					   &gradient_id_press,
	                   bgColBorderWidth,
	                   &b,
	                   bgColCornerRadius,
	                   &r,
	                   bgColBorderSidesTop,
	                   &sideTop,
	                   bgColBorderSidesBottom,
	                   &sideBottom,
	                   bgColBorderSidesLeft,
	                   &sideLeft,
	                   bgColBorderSidesRight,
	                   &sideRight,
	                   -1);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_border_sides_top), sideTop);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_border_sides_bottom), sideBottom);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_border_sides_left), sideLeft);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_border_sides_right), sideRight);

	gtk_color_button_set_color(GTK_COLOR_BUTTON(background_fill_color), fillColor);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_fill_color), (fillOpacity * 0xffff) / 100);

	gtk_color_button_set_color(GTK_COLOR_BUTTON(background_border_color), borderColor);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_border_color), (borderOpacity * 0xffff) / 100);

	gtk_combo_box_set_active(GTK_COMBO_BOX(background_gradient), gradient_id);

	gtk_color_button_set_color(GTK_COLOR_BUTTON(background_fill_color_over), fillColorOver);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_fill_color_over), (fillOpacityOver * 0xffff) / 100);
	gtk_color_button_set_color(GTK_COLOR_BUTTON(background_border_color_over), borderColorOver);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_border_color_over), (borderOpacityOver * 0xffff) / 100);
	gtk_combo_box_set_active(GTK_COMBO_BOX(background_gradient_over), gradient_id_over);

	gtk_color_button_set_color(GTK_COLOR_BUTTON(background_fill_color_press), fillColorPress);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_fill_color_press), (fillOpacityPress * 0xffff) / 100);
	gtk_color_button_set_color(GTK_COLOR_BUTTON(background_border_color_press), borderColorPress);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_border_color_press), (borderOpacityPress * 0xffff) / 100);
	gtk_combo_box_set_active(GTK_COMBO_BOX(background_gradient_press), gradient_id_press);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(background_border_width), b);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(background_corner_radius), r);

	g_boxed_free(GDK_TYPE_COLOR, fillColor);
	g_boxed_free(GDK_TYPE_COLOR, borderColor);
	g_boxed_free(GDK_TYPE_COLOR, fillColorOver);
	g_boxed_free(GDK_TYPE_COLOR, borderColorOver);
	g_boxed_free(GDK_TYPE_COLOR, fillColorPress);
	g_boxed_free(GDK_TYPE_COLOR, borderColorPress);

	background_updates_disabled = FALSE;
	background_update_image(index);
}
