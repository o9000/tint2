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
#include "theme_view.h"

// The data columns that we export via the tree model interface
GtkWidget *g_theme_view;
GtkListStore *g_store;
int g_width_list, g_height_list;
GtkCellRenderer *g_renderer;



GtkWidget *create_view()
{
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget  *view;

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

	GtkTreeSortable *sortable;
	sortable = GTK_TREE_SORTABLE(g_store);
	gtk_tree_sortable_set_sort_column_id(sortable, COL_THEME_FILE, GTK_SORT_ASCENDING);
	return view;
}


void custom_list_append(const gchar *name)
{
	GtkTreeIter  iter;

	gtk_list_store_append(g_store, &iter);
	gtk_list_store_set(g_store, &iter, COL_THEME_FILE, name, -1);
}


gboolean update_snapshot()
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GdkPixbuf *icon;
	gboolean have_iter, found = FALSE;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
	have_iter = gtk_tree_model_get_iter_first(model, &iter);
	while (have_iter) {
		gtk_tree_model_get(model, &iter, COL_SNAPSHOT, &icon, -1);
		if (icon != NULL) {
			g_object_unref(icon);
			have_iter = gtk_tree_model_iter_next(model, &iter);
		}
		else {
			found = TRUE;
			break;
		}
	}

	if (found) {
		// build panel's snapshot
		GdkPixbuf *pixbuf;
		gchar *name, *snap, *cmd;
		gint pixWidth, pixHeight;
		gboolean changeSize = FALSE;

		snap = g_build_filename (g_get_user_config_dir(), "tint2", "snap.jpg", NULL);
		g_remove(snap);

		gtk_tree_model_get(model, &iter, COL_THEME_FILE, &name, -1);
		cmd = g_strdup_printf("tint2 -c \'%s\' -s \'%s\'", name, snap);
		system(cmd);

		// load
		pixbuf = gdk_pixbuf_new_from_file(snap, NULL);
		if (pixbuf == NULL) {
			printf("snapshot NULL : %s\n", cmd);
			found = FALSE;
		}
		g_free(snap);
		g_free(cmd);
		g_free(name);

		pixWidth = gdk_pixbuf_get_width(pixbuf);
		pixHeight = gdk_pixbuf_get_height(pixbuf);
		if (g_width_list != pixWidth) {
			g_width_list = pixWidth;
			changeSize = TRUE;
		}
		if (g_height_list != (pixHeight+30)) {
			g_height_list = pixHeight+30;
			changeSize = TRUE;
		}
		if (changeSize)
			gtk_cell_renderer_set_fixed_size(g_renderer, g_width_list, g_height_list);

		gtk_list_store_set(g_store, &iter, COL_SNAPSHOT, pixbuf, -1);
	}
	return found;
}



