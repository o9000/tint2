
#include "theme_view.h"

// The data columns that we export via the tree model interface
GtkListStore *g_store;
int g_width_list, g_height_list;
GtkCellRenderer *g_renderer;



GtkWidget *create_view()
{
	GtkTreeViewColumn   *col;
	GtkCellRenderer     *renderer;
	GtkWidget           *view;

	g_store = gtk_list_store_new(NB_COL, G_TYPE_STRING, GDK_TYPE_PIXBUF);

	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(g_store));
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(view), TRUE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

	g_object_unref(g_store); // destroy store automatically with view

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", COL_THEME_FILE);
	gtk_tree_view_column_set_visible(col, FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	g_width_list = 200;
	g_height_list = 30;
	g_renderer = gtk_cell_renderer_pixbuf_new();
	g_object_set(g_renderer, "xalign", 0.0, NULL);
	gtk_cell_renderer_set_fixed_size(g_renderer, g_width_list, g_height_list);
	// specific to gtk-2.18 or higher
	//gtk_cell_renderer_set_padding(g_renderer, 5, 5);
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(col, g_renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, g_renderer, "pixbuf", COL_SNAPSHOT);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	//g_timeout_add(50, (GSourceFunc) increase_timeout, NULL);

	return view;
}


void custom_list_append(const gchar *name)
{
	GtkTreeIter  iter;
	gchar *snap, *cmd;
	gint pixWidth, pixHeight;
	gboolean changeSize = FALSE;
	GdkPixbuf *icon;

	// build panel's snapshot
	snap = g_build_filename (g_get_user_config_dir(), "tint2", "snap.jpg", NULL);
	g_remove(snap);

	cmd = g_strdup_printf("tint2 -c %s -s %s", name, snap);
	system(cmd);
	g_free(cmd);

	// load
	icon = gdk_pixbuf_new_from_file(snap, NULL);
	g_free(snap);
	if (!icon)
		return;

	pixWidth = gdk_pixbuf_get_width(icon);
	pixHeight = gdk_pixbuf_get_height(icon);
	if (g_width_list < pixWidth) {
		g_width_list = pixWidth;
		changeSize = TRUE;
	}
	if (g_height_list < (pixHeight+6)) {
		g_height_list = pixHeight+6;
		changeSize = TRUE;
	}
	if (changeSize)
		gtk_cell_renderer_set_fixed_size(g_renderer, g_width_list, g_height_list);

	gtk_list_store_append(g_store, &iter);
	gtk_list_store_set(g_store, &iter, COL_THEME_FILE, name, COL_SNAPSHOT, icon, -1);
}

/*
gboolean increase_timeout (GtkCellRenderer *renderer)
{
  GtkTreeIter  iter;
  gfloat       perc = 0.0;
  //gchar        buf[20];

  gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_store), &iter);

  gtk_tree_model_get (GTK_TREE_MODEL(g_store), &iter, COL_SNAPSHOT, &perc, -1);

  if ( perc > (1.0-STEP)  ||  (perc < STEP && perc > 0.0) )
  {
    increasing = (!increasing);
  }

  if (increasing)
    perc = perc + STEP;
  else
    perc = perc - STEP;

  //g_snprintf(buf, sizeof(buf), "%u %%", (guint)(perc*100));

  gtk_list_store_set (g_store, &iter, COL_SNAPSHOT, perc, -1);

  return TRUE;
}
*/

