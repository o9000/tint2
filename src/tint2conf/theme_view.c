#include <string.h>
#include "theme_view.h"


GtkListStore *g_store;

enum { PROP_TITLE = 1, };



GtkWidget *create_view(void)
{
	GtkCellRenderer    *renderer;
	GtkTreeViewColumn  *col;
	GtkTreeSelection   *sel;
	GtkListStore       *liststore;
	GtkWidget          *view;

	g_store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, GDK_TYPE_PIXBUF);
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(g_store));
	//gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(view), TRUE);
	//gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(view), TRUE);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", COL_TEXT);
	gtk_tree_view_column_set_title(col, " items 1 ");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", COL_PIX);
	gtk_tree_view_column_set_title(col, " image ");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
	g_signal_connect(sel, "changed", G_CALLBACK(on_changed), g_store);

	return view;
}


void on_changed(GtkWidget *widget, gpointer label)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	char *value;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
		gtk_tree_model_get(model, &iter, COL_TEXT, &value,  -1);
		//gtk_label_set_text(GTK_LABEL(label), value);
		g_free(value);
	}
}


void add_to_list(GtkWidget *list, const gchar *str)
{
	GtkTreeIter iter;
	gchar *cmd, *name, *snapshot;
	GdkPixbuf  *icon;
	GError  *error = NULL;

//printf("  %s\n", str);
	snapshot = g_build_filename (g_get_user_config_dir(), "tint2", "snap.jpg", NULL);
	cmd = g_strdup_printf("tint2 -c \'%s\' -s \'%s\'", str, snapshot, NULL);
	system(cmd);

	icon = gdk_pixbuf_new_from_file(snapshot, &error);
	g_free(snapshot);
	if (error) {
		g_warning ("Could not load icon: %s\n", error->message);
		g_error_free(error);
		return;
	}

	gtk_list_store_append(g_store, &iter);
	gtk_list_store_set(g_store, &iter, COL_TEXT, str, COL_PIX, icon, -1);

}


