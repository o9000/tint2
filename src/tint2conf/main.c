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

#include <time.h>

#ifdef HAVE_VERSION_H
#include "version.h"
#endif

#include "main.h"
#include "common.h"
#include "theme_view.h"
#include "properties.h"
#include "properties_rw.h"


// ====== Utilities ======

gchar *get_default_config_path()
{
	gchar *path = NULL;
	const gchar * const * system_dirs = g_get_system_config_dirs();
	int i;
	for (i = 0; system_dirs[i]; i++) {
		path = g_build_filename(system_dirs[i], "tint2", "tint2rc", NULL);
		if (g_file_test(path, G_FILE_TEST_EXISTS))
			return path;
		g_free(path);
		path = NULL;
	}
	return g_strdup("/dev/null");
}

int endswith(const char *str, const char *suffix)
{
	return strlen(str) >= strlen(suffix) &&
			strcmp(str + strlen(str) - strlen(suffix), suffix) == 0;
}

static void menuAddWidget(GtkUIManager *ui_manager, GtkWidget *p_widget, GtkContainer *p_box)
{
	gtk_box_pack_start(GTK_BOX(p_box), p_widget, FALSE, FALSE, 0);
	gtk_widget_show(p_widget);
}

static void menuAddWidget(GtkUIManager *, GtkWidget *, GtkContainer *);
static void menuImport();
static void menuImportDefault();
static void menuSaveAs();
static void menuDelete();
static void edit_current_theme();
static void refresh_current_theme();
static void menuAbout();
static gboolean view_onPopupMenu(GtkWidget *treeview, gpointer userdata);
static gboolean view_onButtonPressed(GtkWidget *treeview, GdkEventButton *event, gpointer userdata);
static gboolean theme_selected(GtkTreeSelection *selection,
							   GtkTreeModel *model,
							   GtkTreePath *path,
							   gboolean path_currently_selected,
							   gpointer userdata);

static void select_first_theme();
static void load_all_themes();


// ====== Globals ======

GtkWidget *g_window;
static GtkUIManager *globalUIManager = NULL;
GtkWidget *tint_cmd;

static const char *global_ui =
	"<ui>"
	"  <menubar name='MenuBar'>"
	"    <menu action='ThemeMenu'>"
	"      <menuitem action='ThemeAdd'/>"
	"      <menuitem action='ThemeDefault'/>"
	"      <separator/>"
	"      <menuitem action='ThemeProperties'/>"
	"      <menuitem action='ThemeSaveAs'/>"
	"      <menuitem action='ThemeDelete'/>"
	"      <separator/>"
	"      <menuitem action='ThemeQuit'/>"
	"    </menu>"
	"    <menu action='EditMenu'>"
	"      <menuitem action='EditRefresh'/>"
	"      <menuitem action='EditRefreshAll'/>"
	"      <separator/>"
	"    </menu>"
	"    <menu action='HelpMenu'>"
	"      <menuitem action='HelpAbout'/>"
	"    </menu>"
	"  </menubar>"
	"  <toolbar  name='ToolBar'>"
	"    <toolitem action='ThemeProperties'/>"
	"  </toolbar>"
	"  <popup  name='ThemePopup'>"
	"    <menuitem action='ThemeProperties'/>"
	"    <menuitem action='EditRefresh'/>"
	"    <separator/>"
	"    <menuitem action='ThemeDelete'/>"
	"  </popup>"
	"</ui>";


// define menubar and toolbar action
static GtkActionEntry entries[] = {
	{"ThemeMenu", NULL, _("Theme"), NULL, NULL, NULL},
	{"ThemeAdd", GTK_STOCK_ADD, _("_Import theme..."), "<Control>N", _("Import theme"), G_CALLBACK(menuImport)},
	{"ThemeDefault", GTK_STOCK_NEW, _("_Import default theme..."), NULL, _("Import default theme"), G_CALLBACK(menuImportDefault)},
	{"ThemeSaveAs", GTK_STOCK_SAVE_AS, _("_Save as..."), NULL, _("Save theme as"), G_CALLBACK(menuSaveAs)},
	{"ThemeDelete", GTK_STOCK_DELETE, _("_Delete"), NULL, _("Delete theme"), G_CALLBACK(menuDelete)},
	{"ThemeProperties", GTK_STOCK_PROPERTIES, _("_Edit theme..."), NULL, _("Edit selected theme"), G_CALLBACK(edit_current_theme)},
	{"ThemeQuit", GTK_STOCK_QUIT, _("_Quit"), "<control>Q", _("Quit"), G_CALLBACK(gtk_main_quit)},
	{"EditMenu", NULL, "Edit", NULL, NULL, NULL},
	{"EditRefresh", GTK_STOCK_REFRESH, _("Refresh"), NULL, _("Refresh"), G_CALLBACK(refresh_current_theme)},
	{"EditRefreshAll", GTK_STOCK_REFRESH, _("Refresh all"), NULL, _("Refresh all"), G_CALLBACK(load_all_themes)},
	{"HelpMenu", NULL, _("Help"), NULL, NULL, NULL},
	{"HelpAbout", GTK_STOCK_ABOUT, _("_About"), "<Control>A", _("About"), G_CALLBACK(menuAbout)}
};


int main(int argc, char **argv)
{
	GtkWidget *vBox = NULL, *scrollbar = NULL;
	GtkActionGroup *actionGroup;

	gtk_init(&argc, &argv);
	g_thread_init((NULL));

	{
		gchar *tint2_config_dir = g_build_filename(g_get_user_config_dir(), "tint2", NULL);
		if (!g_file_test(tint2_config_dir, G_FILE_TEST_IS_DIR))
			g_mkdir(tint2_config_dir, 0777);
		g_free(tint2_config_dir);
	}

	g_set_application_name(_("tint2conf"));
	gtk_window_set_default_icon_name("taskbar");
	
	// config file uses '.' as decimal separator
	setlocale(LC_NUMERIC, "POSIX");

	// define main layout : container, menubar, toolbar
	g_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(g_window), _("Panel theming"));
	gtk_window_resize(GTK_WINDOW(g_window), 800, 600);
	g_signal_connect(G_OBJECT(g_window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	vBox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(g_window), vBox);

	actionGroup = gtk_action_group_new("menuActionGroup");
	gtk_action_group_add_actions(actionGroup, entries, G_N_ELEMENTS(entries), NULL);
	globalUIManager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(globalUIManager, actionGroup, 0);
	gtk_ui_manager_add_ui_from_string(globalUIManager, global_ui, -1, (NULL));
	g_signal_connect(globalUIManager, "add_widget", G_CALLBACK(menuAddWidget), vBox);
	gtk_ui_manager_ensure_update(globalUIManager);

	GtkWidget *table, *label;
	int row, col;

	row = col = 0;
	table = gtk_table_new(1, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(vBox), table, FALSE, TRUE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), 8);
	gtk_table_set_col_spacings(GTK_TABLE(table), 8);

	label = gtk_label_new(_("Command to run tint2: "));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
	col++;

	tint_cmd = gtk_entry_new();
	gtk_widget_show(tint_cmd);
	gtk_entry_set_text(GTK_ENTRY(tint_cmd), "");
	gtk_table_attach(GTK_TABLE(table), tint_cmd, col, col+1, row, row+1, GTK_FILL | GTK_EXPAND, 0, 0, 0);

	scrollbar = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vBox), scrollbar, TRUE, TRUE, 0);

	// define theme view
	g_theme_view = create_view();
	gtk_container_add(GTK_CONTAINER(scrollbar), g_theme_view);
	gtk_widget_show(g_theme_view);
	g_signal_connect(g_theme_view, "button-press-event", (GCallback)view_onButtonPressed, NULL);
	g_signal_connect(g_theme_view, "popup-menu", (GCallback)view_onPopupMenu, NULL);
	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view)), theme_selected, NULL, NULL);

	// load themes
	load_all_themes();

	gtk_widget_show_all(g_window);
	gtk_main();
	return 0;
}

static void menuAbout()
{
	const char *authors[] = {
		"Thierry Lorthiois <lorthiois@bbsoft.fr>",
		"Andreas Fink <andreas.fink85@googlemail.com>",
		"Christian Ruppert <Spooky85@gmail.com> (Build system)",
		"Euan Freeman <euan04@gmail.com> (tintwizard http://code.google.com/p/tintwizard)",
		(NULL)
	};

	gtk_show_about_dialog(GTK_WINDOW(g_window),
						  "name", g_get_application_name( ),
						  "comments", _("Theming tool for tint2 panel"),
						  "version", VERSION_STRING,
						  "copyright", _("Copyright 2009-2015 tint2 team\nTint2 License GNU GPL version 2\nTintwizard License GNU GPL version 3"),
						  "logo-icon-name", "taskbar", "authors", authors,
						  /* Translators: translate "translator-credits" as
														  your name to have it appear in the credits in the "About"
														  dialog */
						  "translator-credits", _("translator-credits"),
						  NULL);
}


// ====== Theme import/copy/delete ======

static void menuImport()
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Import theme(s)"), GTK_WINDOW(g_window), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_ADD, GTK_RESPONSE_ACCEPT, NULL);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
		gtk_widget_destroy(dialog);
		return;
	}

	GSList *list = gtk_file_chooser_get_filenames(chooser);
	GSList *l;
	for (l = list; l ; l = l->next) {
		gchar *file = (char *)l->data;
		gchar *pt1 = strrchr(file, '/');
		if (!pt1)
			continue;
		pt1++;
		if (!*pt1)
			continue;

		gchar *name = pt1;
		gchar *path = g_build_filename(g_get_user_config_dir(), "tint2", name, NULL);
		if (g_file_test(path, G_FILE_TEST_EXISTS))
			continue;
		copy_file(file, path);
		g_free(path);
	}
	g_slist_foreach(list, (GFunc)g_free, NULL);
	g_slist_free(list);
	gtk_widget_destroy(dialog);
	load_all_themes();
}

static void menuImportDefault()
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Save default theme as"), GTK_WINDOW(g_window), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);

	gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
	gchar *config_dir;
	config_dir = g_build_filename(g_get_home_dir(), ".config", "tint2", NULL);
	gtk_file_chooser_set_current_folder(chooser, config_dir);
	g_free(config_dir);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *save_name = gtk_file_chooser_get_filename(chooser);
		gchar *path_default = get_default_config_path();
		copy_file(path_default, save_name);
		g_free(path_default);
		g_free(save_name);
	}
	gtk_widget_destroy(dialog);

	load_all_themes();
}

static void menuSaveAs()
{
	GtkWidget *dialog;
	GtkFileChooser *chooser;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *file, *pt1;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
	if (!gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
		GtkWidget *w = gtk_message_dialog_new(GTK_WINDOW(g_window), 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Select the theme to be saved."));
		g_signal_connect_swapped(w, "response", G_CALLBACK(gtk_widget_destroy), w);
		gtk_widget_show(w);
		return;
	}

	gtk_tree_model_get(model, &iter, COL_THEME_FILE, &file, -1);
	pt1 = strrchr(file, '/');
	if (pt1) pt1++;

	dialog = gtk_file_chooser_dialog_new(_("Save theme as"), GTK_WINDOW(g_window), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	chooser = GTK_FILE_CHOOSER(dialog);

	gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
	gchar *config_dir;
	config_dir = g_build_filename(g_get_home_dir(), ".config", "tint2", NULL);
	gtk_file_chooser_set_current_folder(chooser, config_dir);
	g_free(config_dir);
	gtk_file_chooser_set_current_name(chooser, pt1);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(chooser);
		copy_file(file, filename);
		g_free(filename);
	}
	g_free(file);
	gtk_widget_destroy(dialog);

	load_all_themes();
}

static void menuDelete()
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *filename;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
		gtk_tree_model_get(model, &iter, COL_THEME_FILE, &filename, -1);
		gtk_tree_selection_unselect_all(sel);

		// remove (gui and file)
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
		GFile *file = g_file_new_for_path(filename);
		g_file_trash(file, NULL, NULL);
		g_object_unref(G_OBJECT(file));
		g_free(filename);
	}
}


// ====== Theme popup menu ======

static void view_popup_menu(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	GtkWidget *w = gtk_ui_manager_get_widget(globalUIManager, "/ThemePopup");

	gtk_menu_popup(GTK_MENU(w), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0, gdk_event_get_time((GdkEvent*)event));
}

static gboolean view_onButtonPressed(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	// single click with the right mouse button?
	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3) {
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

		if (gtk_tree_selection_count_selected_rows(selection)  <= 1) {
			GtkTreePath *path;

			// Get tree path for row that was clicked
			if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview), (gint) event->x, (gint) event->y, &path, NULL, NULL, NULL)) {
				gtk_tree_selection_unselect_all(selection);
				gtk_tree_selection_select_path(selection, path);
				gtk_tree_path_free(path);
			}
		}

		view_popup_menu(treeview, event, userdata);
		return TRUE;
	}
	return FALSE;
}

static gboolean view_onPopupMenu(GtkWidget *treeview, gpointer userdata)
{
	view_popup_menu(treeview, NULL, userdata);
	return TRUE;
}


// ====== Theme selection ======

gboolean theme_selected(GtkTreeSelection *selection,
							   GtkTreeModel *model,
							   GtkTreePath *path,
							   gboolean path_currently_selected,
							   gpointer userdata)
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter(model, &iter, path)) {
		gchar *current_theme = NULL;
		gtk_tree_model_get(model, &iter, COL_THEME_FILE, &current_theme,  -1);
		if (!path_currently_selected) {
			gchar *text = g_strdup_printf("tint2 -c %s", current_theme);
			gtk_entry_set_text(GTK_ENTRY(tint_cmd), text);
			g_free(text);
		} else {
			gtk_entry_set_text(GTK_ENTRY(tint_cmd), "");
		}
		g_free(current_theme);
	}
	return TRUE;
}

void select_first_theme()
{
	gboolean have_iter;
	GtkTreeIter iter;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
	have_iter = gtk_tree_model_get_iter_first(model, &iter);
	if (have_iter) {
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view)), &iter);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(g_theme_view), path, NULL, FALSE, 0, 0);
		gtk_tree_path_free(path);
	}
	return;
}

char *get_current_theme_file_name()
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	char *file;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
		gtk_tree_model_get(model, &iter, COL_THEME_FILE, &file,  -1);
		return file;
	}
	return NULL;
}

static void edit_current_theme()
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	char *file;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
		gtk_tree_model_get(model, &iter, COL_THEME_FILE, &file,  -1);
		GtkWidget *prop;
		prop = create_properties();
		config_read_file(file);
		gtk_window_present(GTK_WINDOW(prop));
		g_free(file);
	}
}

// ====== Theme load/reload ======

static void load_all_themes()
{
	gtk_list_store_clear(GTK_LIST_STORE(g_store));

	gchar *tint2_config_dir = g_build_filename(g_get_user_config_dir(), "tint2", NULL);
	GDir *dir = g_dir_open(tint2_config_dir, 0, NULL);
	if (dir == NULL) {
		g_free(tint2_config_dir);
		return;
	}
	gboolean found_theme = FALSE;
	const gchar *file_name;
	while ((file_name = g_dir_read_name(dir))) {
		if (!g_file_test(file_name, G_FILE_TEST_IS_DIR) &&
			!strstr(file_name, "backup") &&
			!strstr(file_name, "copy") &&
			!strstr(file_name, "~") &&
			(endswith(file_name, "tint2rc") ||
			 endswith(file_name, ".conf"))) {
			found_theme = TRUE;
			gchar *name = g_build_filename(tint2_config_dir, file_name, NULL);
			custom_list_append(name);
			g_free(name);
		}
	}

	if (!found_theme) {
		gchar *path_tint2rc = g_build_filename (g_get_user_config_dir(), "tint2", "tint2rc", NULL);
		copy_file(get_default_config_path(), path_tint2rc);
		g_free(path_tint2rc);
		load_all_themes();
	} else {
		select_first_theme();

		GtkTreeIter iter;
		GtkTreeModel *model;
		gboolean have_iter;

		model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
		have_iter = gtk_tree_model_get_iter_first(model, &iter);
		while (have_iter) {
			gtk_list_store_set(g_store, &iter, COL_SNAPSHOT, NULL, -1);
			have_iter = gtk_tree_model_iter_next(model, &iter);
		}

		g_timeout_add(SNAPSHOT_TICK, (GSourceFunc)update_snapshot, NULL);
	}

	g_dir_close(dir);
	g_free(tint2_config_dir);
}

static void refresh_current_theme()
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
		gtk_list_store_set(g_store, &iter, COL_SNAPSHOT, NULL, -1);
	}

	g_timeout_add(SNAPSHOT_TICK, (GSourceFunc)update_snapshot, NULL);
}
