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
#include "strnatcmp.h"
#include "theme_view.h"

// The data columns that we export via the tree model interface
GtkWidget *g_theme_view;
GtkListStore *theme_list_store;
int g_width_list, g_height_list;
GtkCellRenderer *g_renderer;

gint theme_name_compare(GtkTreeModel *model,
						GtkTreeIter *a,
						GtkTreeIter *b,
						gpointer user_data);

GtkWidget *create_view()
{
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget  *view;

	theme_list_store = gtk_list_store_new(NB_COL, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF);

	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(theme_list_store));
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(view), TRUE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

	g_object_unref(theme_list_store); // destroy store automatically with view

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", COL_THEME_FILE);
	gtk_tree_view_column_set_visible(col, FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", COL_THEME_NAME);
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
	sortable = GTK_TREE_SORTABLE(theme_list_store);
	gtk_tree_sortable_set_sort_column_id(sortable, COL_THEME_FILE, GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_func(sortable, COL_THEME_FILE, theme_name_compare, NULL, NULL);
	return view;
}

gint theme_name_compare(GtkTreeModel *model,
						GtkTreeIter *a,
						GtkTreeIter *b,
						gpointer user_data)
{
	gchar *path_a, *path_b;
	gtk_tree_model_get(model, a, COL_THEME_FILE, &path_a, -1);
	gtk_tree_model_get(model, b, COL_THEME_FILE, &path_b, -1);

	gboolean home_a = strstr(path_a, g_get_user_config_dir()) == path_a;
	gboolean home_b = strstr(path_b, g_get_user_config_dir()) == path_b;

	if (home_a && !home_b)
		return -1;
	if (!home_a && home_b)
		return 1;

	gchar *name_a = path_a;
	gchar *p;
	for (p = name_a; *p; p++) {
		if (*p == '/')
			name_a = p + 1;
	}
	gchar *name_b = path_b;
	for (p = name_b; *p; p++) {
		if (*p == '/')
			name_b = p + 1;
	}
	if (g_str_equal(name_a, name_b))
		return 0;
	if (g_str_equal(name_a, "tint2rc"))
		return -1;
	if (g_str_equal(name_b, "tint2rc"))
		return 1;
	gint result = strnatcasecmp(name_a, name_b);
	g_free(path_a);
	g_free(path_b);
	return result;
}

void theme_list_append(const gchar *path, const gchar *suffix)
{
	GtkTreeIter iter;

	gtk_list_store_append(theme_list_store, &iter);
	gtk_list_store_set(theme_list_store, &iter, COL_THEME_FILE, path, -1);

	gchar *name = strrchr(path, '/') + 1;

	gchar *full_name;
	if (suffix) {
		full_name = g_strdup_printf("%s\n(%s)", name, suffix);
	} else {
		full_name = g_strdup(name);
	}
	gtk_list_store_set(theme_list_store, &iter, COL_THEME_NAME, full_name, -1);
	g_free(full_name);
}


gboolean update_snapshot()
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean have_iter;

	gint pixWidth = 200, pixHeight = 30;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
	have_iter = gtk_tree_model_get_iter_first(model, &iter);
	while (have_iter) {
		GdkPixbuf *pixbuf;
		gtk_tree_model_get(model, &iter, COL_SNAPSHOT, &pixbuf, -1);
		if (pixbuf) {
			pixWidth = MAX(pixWidth, gdk_pixbuf_get_width(pixbuf));
			pixHeight = MAX(pixHeight, gdk_pixbuf_get_height(pixbuf));
			g_object_unref(pixbuf);
			have_iter = gtk_tree_model_iter_next(model, &iter);
			continue;
		}

		// build panel's snapshot
		gchar *path;
		gtk_tree_model_get(model, &iter,
						   COL_THEME_FILE, &path,
						   -1);


		gchar *snap = g_build_filename(g_get_user_config_dir(), "tint2", "snap.jpg", NULL);
		g_remove(snap);

		gchar *cmd = g_strdup_printf("tint2 -c \'%s\' -s \'%s\' 1>/dev/null 2>/dev/null", path, snap);
		if (system(cmd) == 0) {
			// load
			pixbuf = gdk_pixbuf_new_from_file(snap, NULL);
			if (pixbuf == NULL) {
				printf("snapshot NULL : %s\n", cmd);
			}
		}
		g_free(cmd);
		g_free(snap);
		g_free(path);

		pixWidth = MAX(pixWidth, gdk_pixbuf_get_width(pixbuf));
		pixHeight = MAX(pixHeight, gdk_pixbuf_get_height(pixbuf));

		gtk_list_store_set(theme_list_store, &iter, COL_SNAPSHOT, pixbuf, -1);
		if (pixbuf)
			g_object_unref(pixbuf);
		have_iter = gtk_tree_model_iter_next(model, &iter);
	}

	gtk_cell_renderer_set_fixed_size(g_renderer, pixWidth + 30, pixHeight + 30);

	return FALSE;
}
