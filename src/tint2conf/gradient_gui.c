#include "gradient_gui.h"

#include <math.h>

GtkListStore *gradient_ids, *gradient_stop_ids;
GList *gradients;
GtkWidget *current_gradient, *gradient_combo_type, *gradient_start_color, *gradient_end_color, *current_gradient_stop,
*gradient_stop_color, *gradient_stop_offset;

int gradient_index_safe(int index)
{
	if (index <= 0)
		index = 0;
	if (index >= get_model_length(GTK_TREE_MODEL(gradient_ids)))
		index = 0;
	return index;
}

GtkWidget *create_gradient_combo()
{
	GtkWidget *combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(gradient_ids));
	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "pixbuf", grColPixbuf, NULL);
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "wrap-mode", PANGO_WRAP_WORD, NULL);
	g_object_set(renderer, "wrap-width", 300, NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", grColText, NULL);
	return combo;
}

void create_gradient(GtkWidget *parent)
{
	gradient_ids = gtk_list_store_new(grNumCols, GDK_TYPE_PIXBUF, GTK_TYPE_INT, GTK_TYPE_STRING);
	gradients = NULL;

	gradient_stop_ids = gtk_list_store_new(grStopNumCols, GDK_TYPE_PIXBUF);

	GtkWidget *table, *label, *button;
	int row, col;
	// GtkTooltips *tooltips = gtk_tooltips_new();

	table = gtk_table_new(1, 4, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

	row = 0, col = 0;
	label = gtk_label_new(_("<b>Gradient</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	current_gradient = create_gradient_combo();
	gtk_widget_show(current_gradient);
	gtk_table_attach(GTK_TABLE(table), current_gradient, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	button = gtk_button_new_from_stock("gtk-add");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(gradient_duplicate), NULL);
	gtk_widget_show(button);
	gtk_table_attach(GTK_TABLE(table), button, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	button = gtk_button_new_from_stock("gtk-remove");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(gradient_delete), NULL);
	gtk_widget_show(button);
	gtk_table_attach(GTK_TABLE(table), button, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	table = gtk_table_new(3, 4, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

	row = 0, col = 2;
	label = gtk_label_new(_("Type"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	gradient_combo_type = gtk_combo_box_new_text();
	gtk_widget_show(gradient_combo_type);
	gtk_table_attach(GTK_TABLE(table), gradient_combo_type, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;
	gtk_combo_box_append_text(GTK_COMBO_BOX(gradient_combo_type), _("Vertical"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(gradient_combo_type), _("Horizontal"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(gradient_combo_type), _("Radial"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(gradient_combo_type), 0);

	row++, col = 2;
	label = gtk_label_new(_("Start color"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	gradient_start_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(gradient_start_color), TRUE);
	gtk_widget_show(gradient_start_color);
	gtk_table_attach(GTK_TABLE(table), gradient_start_color, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	row++, col = 2;
	label = gtk_label_new(_("End color"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	gradient_end_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(gradient_end_color), TRUE);
	gtk_widget_show(gradient_end_color);
	gtk_table_attach(GTK_TABLE(table), gradient_end_color, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	change_paragraph(parent);

	table = gtk_table_new(1, 4, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

	row = 0, col = 0;
	label = gtk_label_new(_("<b>Color stop</b>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	current_gradient_stop = create_gradient_stop_combo(NULL);
	gtk_widget_show(current_gradient_stop);
	gtk_table_attach(GTK_TABLE(table), current_gradient_stop, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	button = gtk_button_new_from_stock("gtk-add");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(gradient_stop_duplicate), NULL);
	gtk_widget_show(button);
	gtk_table_attach(GTK_TABLE(table), button, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	button = gtk_button_new_from_stock("gtk-remove");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(gradient_stop_delete), NULL);
	gtk_widget_show(button);
	gtk_table_attach(GTK_TABLE(table), button, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	table = gtk_table_new(3, 4, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
	gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

	row = 0, col = 2;
	label = gtk_label_new(_("Color"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	gradient_stop_color = gtk_color_button_new();
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(gradient_stop_color), TRUE);
	gtk_widget_show(gradient_stop_color);
	gtk_table_attach(GTK_TABLE(table), gradient_stop_color, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	row++, col = 2;
	label = gtk_label_new(_("Position"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	gradient_stop_offset = gtk_spin_button_new_with_range(0, 100, 1);
	gtk_widget_show(gradient_stop_offset);
	gtk_table_attach(GTK_TABLE(table), gradient_stop_offset, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	g_signal_connect(G_OBJECT(current_gradient), "changed", G_CALLBACK(current_gradient_changed), NULL);
	g_signal_connect(G_OBJECT(gradient_combo_type), "changed", G_CALLBACK(gradient_update), NULL);
	g_signal_connect(G_OBJECT(gradient_start_color), "color-set", G_CALLBACK(gradient_update), NULL);
	g_signal_connect(G_OBJECT(gradient_end_color), "color-set", G_CALLBACK(gradient_update), NULL);

	g_signal_connect(G_OBJECT(current_gradient_stop), "changed", G_CALLBACK(current_gradient_stop_changed), NULL);
	g_signal_connect(G_OBJECT(gradient_stop_color), "color-set", G_CALLBACK(gradient_stop_update), NULL);
	g_signal_connect(G_OBJECT(gradient_stop_offset), "value-changed", G_CALLBACK(gradient_stop_update), NULL);

	change_paragraph(parent);
}

void gradient_create_new(GradientConfigType t)
{
	int index = get_model_length(GTK_TREE_MODEL(gradient_ids));

	GradientConfig *g = (GradientConfig *)calloc(1, sizeof(GradientConfig));
	g->type = t;
	gradients = g_list_append(gradients, g);
	GtkTreeIter iter;
	gtk_list_store_append(gradient_ids, &iter);
	gtk_list_store_set(gradient_ids, &iter,
					   grColPixbuf, NULL,
					   grColId, &index,
					   grColText, index == 0 ? _("None") : "",
					   -1);

	gradient_update_image(index);
	gtk_combo_box_set_active(GTK_COMBO_BOX(current_gradient), index);
	current_gradient_changed(0, 0);
}

gpointer copy_GradientConfigColorStop(gconstpointer arg, gpointer data)
{
	GradientConfigColorStop *old = (GradientConfigColorStop *)arg;

	GradientConfigColorStop *copy = (GradientConfigColorStop *)calloc(1, sizeof(GradientConfigColorStop));
	memcpy(copy, old, sizeof(GradientConfigColorStop));
	return copy;
}

void gradient_duplicate(GtkWidget *widget, gpointer data)
{
	int old_index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_gradient));
	if (old_index < 0) {
		gradient_create_new(GRADIENT_CONFIG_VERTICAL);
		return;
	}

	GradientConfig *g_old = (GradientConfig*)g_list_nth(gradients, (guint)old_index)->data;

	int index = get_model_length(GTK_TREE_MODEL(gradient_ids));

	GradientConfig *g = (GradientConfig *)calloc(1, sizeof(GradientConfig));
	g->type = g_old->type;
	g->start_color = g_old->start_color;
	g->end_color = g_old->end_color;
	g->extra_color_stops = g_list_copy_deep(g->extra_color_stops, copy_GradientConfigColorStop, NULL);
	gradients = g_list_append(gradients, g);
	GtkTreeIter iter;
	gtk_list_store_append(gradient_ids, &iter);
	gtk_list_store_set(gradient_ids, &iter,
					   grColPixbuf, NULL,
					   grColId, &index,
					   grColText, "",
					   -1);

	gradient_update_image(index);
	gtk_combo_box_set_active(GTK_COMBO_BOX(current_gradient), index);
	current_gradient_changed(0, 0);
}

void gradient_delete(GtkWidget *widget, gpointer data)
{
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_gradient));
	if (index < 0)
		return;

	if (get_model_length(GTK_TREE_MODEL(gradient_ids)) <= 1)
		return;

	GtkTreePath *path;
	GtkTreeIter iter;

	path = gtk_tree_path_new_from_indices(index, -1);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(gradient_ids), &iter, path);
	gtk_tree_path_free(path);

	gradients = g_list_remove(gradients, g_list_nth(gradients, (guint)index)->data);
	gtk_list_store_remove(gradient_ids, &iter);

	background_update_for_gradient(index);

	if (index >= get_model_length(GTK_TREE_MODEL(gradient_ids)))
		index--;
	gtk_combo_box_set_active(GTK_COMBO_BOX(current_gradient), index);
}

void gradient_draw(cairo_t *c, GradientConfig *g, int w, int h, gboolean preserve)
{
	cairo_pattern_t *gpat;
	if (g->type == GRADIENT_CONFIG_VERTICAL)
		gpat = cairo_pattern_create_linear(0, 0, 0, h);
	else if (g->type == GRADIENT_CONFIG_HORIZONTAL)
		gpat = cairo_pattern_create_linear(0, 0, w, 0);
	else gpat = cairo_pattern_create_radial(w/2, h/2, 0, w/2, h/2, sqrt(w * w + h * h) / 2);
	cairo_pattern_add_color_stop_rgba(gpat, 0.0, g->start_color.color.rgb[0], g->start_color.color.rgb[1], g->start_color.color.rgb[2], g->start_color.color.alpha);
	for (GList *l = g->extra_color_stops; l; l = l->next) {
		GradientConfigColorStop *stop = (GradientConfigColorStop *)l->data;
		cairo_pattern_add_color_stop_rgba(gpat,
										  stop->offset,
										  stop->color.rgb[0],
										  stop->color.rgb[1],
										  stop->color.rgb[2],
										  stop->color.alpha);
	}
	cairo_pattern_add_color_stop_rgba(gpat, 1.0, g->end_color.color.rgb[0], g->end_color.color.rgb[1], g->end_color.color.rgb[2], g->end_color.color.alpha);
	cairo_set_source(c, gpat);
	cairo_rectangle(c, 0, 0, w, h);
	if (preserve)
		cairo_fill_preserve(c);
	else
		cairo_fill(c);
	cairo_pattern_destroy(gpat);
}

void gradient_update_image(int index)
{
	GradientConfig *g = (GradientConfig *)g_list_nth(gradients, (guint)index)->data;

	int w = 70;
	int h = 30;
	GdkPixmap *pixmap = gdk_pixmap_new(NULL, w, h, 24);

	cairo_t *cr = gdk_cairo_create(pixmap);
	cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
	cairo_rectangle(cr, 0, 0, w, h);
	cairo_fill(cr);

	gradient_draw(cr, g, w, h, FALSE);

	GdkPixbuf *pixbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, gdk_colormap_get_system(), 0, 0, 0, 0, w, h);
	if (pixmap)
		g_object_unref(pixmap);

	GtkTreePath *path;
	GtkTreeIter iter;

	path = gtk_tree_path_new_from_indices(index, -1);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(gradient_ids), &iter, path);
	gtk_tree_path_free(path);

	gtk_list_store_set(gradient_ids, &iter,
					   grColPixbuf, pixbuf,
					   -1);
	if (pixbuf)
		g_object_unref(pixbuf);
}

void gradient_force_update()
{
	gradient_update(NULL, NULL);
}

static gboolean gradient_updates_disabled = FALSE;
void gradient_update(GtkWidget *widget, gpointer data)
{
	if (gradient_updates_disabled)
		return;
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_gradient));
	if (index <= 0)
		return;

	GtkTreePath *path;
	GtkTreeIter iter;

	path = gtk_tree_path_new_from_indices(index, -1);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(gradient_ids), &iter, path);
	gtk_tree_path_free(path);

	GradientConfig *g = (GradientConfig *)g_list_nth(gradients, (guint)index)->data;

	g->type = (GradientConfigType)MAX(0, gtk_combo_box_get_active(GTK_COMBO_BOX(gradient_combo_type)));

	GdkColor color;
	int opacity;

	gtk_color_button_get_color(GTK_COLOR_BUTTON(gradient_start_color), &color);
	opacity = MIN(100, 0.5 + gtk_color_button_get_alpha(GTK_COLOR_BUTTON(gradient_start_color)) * 100.0 / 0xffff);
	gdkColor2CairoColor(color, &g->start_color.color.rgb[0], &g->start_color.color.rgb[1], &g->start_color.color.rgb[2]);
	g->start_color.color.alpha = opacity / 100.0;

	gtk_color_button_get_color(GTK_COLOR_BUTTON(gradient_end_color), &color);
	opacity = MIN(100, 0.5 + gtk_color_button_get_alpha(GTK_COLOR_BUTTON(gradient_end_color)) * 100.0 / 0xffff);
	gdkColor2CairoColor(color, &g->end_color.color.rgb[0], &g->end_color.color.rgb[1], &g->end_color.color.rgb[2]);
	g->end_color.color.alpha = opacity / 100.0;

	gtk_list_store_set(gradient_ids, &iter,
					   grColPixbuf, NULL,
					   grColId, &index,
					   grColText, "",
					   -1);
	gradient_update_image(index);
	background_update_for_gradient(index);
}

void current_gradient_changed(GtkWidget *widget, gpointer data)
{
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_gradient));
	if (index < 0)
		return;

	gtk_widget_set_sensitive(gradient_combo_type, index > 0);
	gtk_widget_set_sensitive(gradient_start_color, index > 0);
	gtk_widget_set_sensitive(gradient_end_color, index > 0);
	gtk_widget_set_sensitive(current_gradient_stop, index > 0);
	gtk_widget_set_sensitive(gradient_stop_color, index > 0);
	gtk_widget_set_sensitive(gradient_stop_offset, index > 0);

	gradient_updates_disabled = TRUE;

	GradientConfig *g = (GradientConfig *)g_list_nth(gradients, (guint)index)->data;

	gtk_combo_box_set_active(GTK_COMBO_BOX(gradient_combo_type), g->type);

	GdkColor color;
	int opacity;

	cairoColor2GdkColor(g->start_color.color.rgb[0], g->start_color.color.rgb[1], g->start_color.color.rgb[2], &color);
	opacity = g->start_color.color.alpha * 65535;
	gtk_color_button_set_color(GTK_COLOR_BUTTON(gradient_start_color), &color);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(gradient_start_color), opacity);

	cairoColor2GdkColor(g->end_color.color.rgb[0], g->end_color.color.rgb[1], g->end_color.color.rgb[2], &color);
	opacity = g->end_color.color.alpha * 65535;
	gtk_color_button_set_color(GTK_COLOR_BUTTON(gradient_end_color), &color);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(gradient_end_color), opacity);

	int old_stop_index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_gradient_stop));
	gtk_list_store_clear(gradient_stop_ids);
	int stop_index = 0;
	for (GList *l = g->extra_color_stops; l; l = l->next, stop_index++) {
		GtkTreeIter iter;
		gtk_list_store_append(gradient_stop_ids, &iter);
		gtk_list_store_set(gradient_stop_ids, &iter,
						   grStopColPixbuf, NULL,
						   -1);
		gradient_stop_update_image(stop_index);
	}
	if (old_stop_index >= 0 && old_stop_index < stop_index)
		gtk_combo_box_set_active(GTK_COMBO_BOX(current_gradient_stop), old_stop_index);
	else
		gtk_combo_box_set_active(GTK_COMBO_BOX(current_gradient_stop), stop_index - 1);

	gradient_updates_disabled = FALSE;
	gradient_update_image(index);
}

//////// Gradient stops

GtkWidget *create_gradient_stop_combo()
{
	GtkWidget *combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(gradient_stop_ids));
	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "pixbuf", grStopColPixbuf, NULL);
	return combo;
}

void gradient_stop_create_new()
{
	int g_index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_gradient));
	if (g_index < 0)
		return;

	GradientConfig *g = (GradientConfig *)g_list_nth(gradients, (guint)g_index)->data;
	GradientConfigColorStop *stop = (GradientConfigColorStop *)calloc(1, sizeof(GradientConfigColorStop));
	if (g->extra_color_stops)
		memcpy(stop, g_list_last(g->extra_color_stops)->data, sizeof(GradientConfigColorStop));
	else
		memcpy(stop, &g->start_color, sizeof(GradientConfigColorStop));
	g->extra_color_stops = g_list_append(g->extra_color_stops, stop);
	GtkTreeIter iter;
	gtk_list_store_append(gradient_stop_ids, &iter);
	gtk_list_store_set(gradient_stop_ids, &iter,
					   grStopColPixbuf, NULL,
					   -1);

	int stop_index = g_list_length(g->extra_color_stops) - 1;
	gradient_stop_update_image(stop_index);
	gtk_combo_box_set_active(GTK_COMBO_BOX(current_gradient_stop), stop_index);
	current_gradient_stop_changed(0, 0);
	gradient_update_image(g_index);
	background_update_for_gradient(g_index);
}

void gradient_stop_duplicate(GtkWidget *widget, gpointer data)
{
	gradient_stop_create_new();
}

void gradient_stop_delete(GtkWidget *widget, gpointer data)
{
	int g_index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_gradient));
	if (g_index < 0)
		return;

	GradientConfig *g = (GradientConfig *)g_list_nth(gradients, (guint)g_index)->data;
	if (!g->extra_color_stops)
		return;

	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_gradient_stop));
	if (index < 0)
		return;

	GtkTreePath *path;
	GtkTreeIter iter;

	path = gtk_tree_path_new_from_indices(index, -1);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(gradient_stop_ids), &iter, path);
	gtk_tree_path_free(path);

	g->extra_color_stops = g_list_remove(g->extra_color_stops, g_list_nth(g->extra_color_stops, (guint)index)->data);
	gtk_list_store_remove(gradient_stop_ids, &iter);

	if (index >= get_model_length(GTK_TREE_MODEL(gradient_stop_ids)))
		index--;
	gtk_combo_box_set_active(GTK_COMBO_BOX(current_gradient_stop), index);

	gradient_update_image(g_index);
	background_update_for_gradient(g_index);
}

void gradient_stop_update_image(int index)
{
	int g_index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_gradient));
	if (g_index < 0)
		return;
	GradientConfig *g = (GradientConfig *)g_list_nth(gradients, (guint)g_index)->data;
	GradientConfigColorStop *stop = (GradientConfigColorStop *)g_list_nth(g->extra_color_stops, (guint)index)->data;

	int w = 70;
	int h = 30;
	GdkPixmap *pixmap = gdk_pixmap_new(NULL, w, h, 24);

	cairo_t *cr = gdk_cairo_create(pixmap);
	cairo_set_source_rgba(cr, stop->color.rgb[0], stop->color.rgb[1], stop->color.rgb[2], stop->color.alpha);
	cairo_rectangle(cr, 0, 0, w, h);
	cairo_fill(cr);

	GdkPixbuf *pixbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, gdk_colormap_get_system(), 0, 0, 0, 0, w, h);
	if (pixmap)
		g_object_unref(pixmap);

	GtkTreePath *path;
	GtkTreeIter iter;

	path = gtk_tree_path_new_from_indices(index, -1);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(gradient_stop_ids), &iter, path);
	gtk_tree_path_free(path);

	gtk_list_store_set(gradient_stop_ids, &iter,
					   grStopColPixbuf, pixbuf,
					   -1);
	if (pixbuf)
		g_object_unref(pixbuf);
}

void current_gradient_stop_changed(GtkWidget *widget, gpointer data)
{
	int g_index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_gradient));
	if (g_index < 0)
		return;
	GradientConfig *g = (GradientConfig *)g_list_nth(gradients, (guint)g_index)->data;

	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_gradient_stop));
	if (index < 0)
		return;

	GradientConfigColorStop *stop = (GradientConfigColorStop *)g_list_nth(g->extra_color_stops, (guint)index)->data;

	gradient_updates_disabled = TRUE;

	GdkColor color;
	int opacity;

	cairoColor2GdkColor(stop->color.rgb[0], stop->color.rgb[1], stop->color.rgb[2], &color);
	opacity = stop->color.alpha * 65535;
	gtk_color_button_set_color(GTK_COLOR_BUTTON(gradient_stop_color), &color);
	gtk_color_button_set_alpha(GTK_COLOR_BUTTON(gradient_stop_color), opacity);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gradient_stop_offset), stop->offset * 100);

	gradient_updates_disabled = FALSE;
}

void gradient_stop_update(GtkWidget *widget, gpointer data)
{
	if (gradient_updates_disabled)
		return;

	int g_index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_gradient));
	if (g_index < 0)
		return;
	GradientConfig *g = (GradientConfig *)g_list_nth(gradients, (guint)g_index)->data;

	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_gradient_stop));
	if (index < 0)
		return;

	GradientConfigColorStop *stop = (GradientConfigColorStop *)g_list_nth(g->extra_color_stops, (guint)index)->data;

	GdkColor color;
	int opacity;

	gtk_color_button_get_color(GTK_COLOR_BUTTON(gradient_stop_color), &color);
	opacity = MIN(100, 0.5 + gtk_color_button_get_alpha(GTK_COLOR_BUTTON(gradient_stop_color)) * 100.0 / 0xffff);
	gdkColor2CairoColor(color, &stop->color.rgb[0], &stop->color.rgb[1], &stop->color.rgb[2]);
	stop->color.alpha = opacity / 100.0;

	stop->offset = gtk_spin_button_get_value(GTK_SPIN_BUTTON(gradient_stop_offset)) / 100.;

	gradient_stop_update_image(index);
	gradient_update_image(g_index);
	background_update_for_gradient(g_index);
}
