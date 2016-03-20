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
#include <unistd.h>

#ifdef HAVE_VERSION_H
#include "version.h"
#endif

#include "main.h"
#include "common.h"
#include "theme_view.h"
#include "properties.h"
#include "properties_rw.h"

void refresh_theme(const char *given_path);
void remove_theme(const char *given_path);
static void theme_selection_changed(GtkWidget *treeview, gpointer userdata);
static gchar *find_theme_in_system_dirs(const gchar *given_name);

// ====== Utilities ======

// Returns ~/.config/tint2/tint2rc (or equivalent).
// Needs gfree.
gchar *get_home_config_path()
{
	return g_build_filename(g_get_user_config_dir(), "tint2", "tint2rc", NULL);
}

// Returns /etc/xdg/tint2/tint2rc (or equivalent).
// Needs gfree.
gchar *get_etc_config_path()
{
	gchar *path = NULL;
	const gchar *const *system_dirs = g_get_system_config_dirs();
	for (int i = 0; system_dirs[i]; i++) {
		path = g_build_filename(system_dirs[i], "tint2", "tint2rc", NULL);
		if (g_file_test(path, G_FILE_TEST_EXISTS))
			return path;
		g_free(path);
		path = NULL;
	}
	return g_strdup("/dev/null");
}

gboolean startswith(const char *str, const char *prefix)
{
	return strstr(str, prefix) == str;
}

gboolean endswith(const char *str, const char *suffix)
{
	return strlen(str) >= strlen(suffix) && g_str_equal(str + strlen(str) - strlen(suffix), suffix);
}

// Returns TRUE if the theme file is in ~/.config.
gboolean theme_is_editable(const char *filepath)
{
	return startswith(filepath, g_get_user_config_dir());
}

// Returns TRUE if the theme file is ~/.config/tint2/tint2rc.
gboolean theme_is_default(const char *filepath)
{
	gchar *default_path = get_home_config_path();
	gboolean result = g_str_equal(default_path, filepath);
	g_free(default_path);
	return result;
}

// Extracts the file name from the path. Do not free!
char *file_name_from_path(const char *filepath)
{
	char *name = strrchr(filepath, '/');
	if (!name)
		return NULL;
	name++;
	if (!*name)
		return NULL;
	return name;
}

void make_backup(const char *filepath)
{
	gchar *backup_path = g_strdup_printf("%s.backup.%ld", filepath, time(NULL));
	copy_file(filepath, backup_path);
	g_free(backup_path);
}

// Imports a file to ~/.config/tint2/.
// If a file with the same name exists, it does not overwrite it.
// Takes care of updating the theme list in the GUI.
// Returns the new location. Needs gfree.
gchar *import_no_overwrite(const char *filepath)
{
	gchar *filename = file_name_from_path(filepath);
	if (!filename)
		return NULL;

	gchar *newpath = g_build_filename(g_get_user_config_dir(), "tint2", filename, NULL);
	if (!g_file_test(newpath, G_FILE_TEST_EXISTS)) {
		copy_file(filepath, newpath);
		theme_list_append(newpath, NULL);
		g_timeout_add(SNAPSHOT_TICK, (GSourceFunc)update_snapshot, NULL);
	}

	return newpath;
}

// Copies a theme file from filepath to newpath.
// Takes care of updating the theme list in the GUI.
void import_with_overwrite(const char *filepath, const char *newpath)
{
	gboolean theme_existed = g_file_test(newpath, G_FILE_TEST_EXISTS);
	if (theme_existed)
		make_backup(newpath);

	copy_file(filepath, newpath);

	if (theme_is_editable(newpath)) {
		if (!theme_existed) {
			theme_list_append(newpath, NULL);
			g_timeout_add(SNAPSHOT_TICK, (GSourceFunc)update_snapshot, NULL);
		} else {
			int unused = system("killall -SIGUSR1 tint2 || pkill -SIGUSR1 -x tint2");
			(void)unused;
			refresh_theme(newpath);
		}
	}
}

static void menuAddWidget(GtkUIManager *ui_manager, GtkWidget *p_widget, GtkContainer *p_box)
{
	gtk_box_pack_start(GTK_BOX(p_box), p_widget, FALSE, FALSE, 0);
	gtk_widget_show(p_widget);
}

static void menuAddWidget(GtkUIManager *, GtkWidget *, GtkContainer *);
static void menuImportFile();
static void menuSaveAs();
static void menuDelete();
static void menuReset();
static void edit_theme();
static void make_selected_theme_default();
static void menuAbout();
static gboolean view_onPopupMenu(GtkWidget *treeview, gpointer userdata);
static gboolean view_onButtonPressed(GtkWidget *treeview, GdkEventButton *event, gpointer userdata);
static void viewRowActivated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);

static void select_first_theme();
static void load_all_themes();

// ====== Globals ======

GtkWidget *g_window;
static GtkUIManager *globalUIManager = NULL;
GtkWidget *tint_cmd;

GtkActionGroup *actionGroup = NULL;

static const char *global_ui = "<ui>"
							   "  <menubar name='MenuBar'>"
							   "    <menu action='ThemeMenu'>"
							   "      <menuitem action='ThemeImportFile'/>"
							   "      <separator/>"
							   "      <menuitem action='ThemeEdit'/>"
							   "      <menuitem action='ThemeSaveAs'/>"
							   "      <menuitem action='ThemeMakeDefault'/>"
							   "      <menuitem action='ThemeReset'/>"
							   "      <menuitem action='ThemeDelete'/>"
							   "      <separator/>"
							   "      <menuitem action='ThemeRefresh'/>"
							   "      <menuitem action='RefreshAll'/>"
							   "      <separator/>"
							   "      <menuitem action='Quit'/>"
							   "    </menu>"
							   "    <menu action='HelpMenu'>"
							   "      <menuitem action='HelpAbout'/>"
							   "    </menu>"
							   "  </menubar>"
							   "  <toolbar  name='ToolBar'>"
							   "    <toolitem action='ThemeEdit'/>"
							   "    <toolitem action='ThemeMakeDefault'/>"
							   "  </toolbar>"
							   "  <popup  name='ThemePopup'>"
							   "    <menuitem action='ThemeEdit'/>"
							   "    <menuitem action='ThemeRefresh'/>"
							   "    <separator/>"
							   "    <menuitem action='ThemeReset'/>"
							   "    <menuitem action='ThemeDelete'/>"
							   "  </popup>"
							   "</ui>";

int main(int argc, char **argv)
{
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	GtkWidget *vBox = NULL, *scrollbar = NULL;
	gtk_init(&argc, &argv);

#if !GLIB_CHECK_VERSION(2, 31, 0)
	g_thread_init(NULL);
#endif

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
	gtk_window_resize(GTK_WINDOW(g_window), 920, 600);
	g_signal_connect(G_OBJECT(g_window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	vBox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(g_window), vBox);

	actionGroup = gtk_action_group_new("menuActionGroup");

	// Menubar and toolbar entries
	GtkActionEntry entries[] =
		{{"ThemeMenu", NULL, _("Theme"), NULL, NULL, NULL},
		 {"ThemeImportFile", GTK_STOCK_ADD, _("_Import theme..."), "<Control>N", _("Import theme"), G_CALLBACK(menuImportFile)},
		 {"ThemeSaveAs", GTK_STOCK_SAVE_AS, _("_Save as..."), NULL, _("Save theme as"), G_CALLBACK(menuSaveAs)},
		 {"ThemeDelete", GTK_STOCK_DELETE, _("_Delete"), NULL, _("Delete theme"), G_CALLBACK(menuDelete)},
		 {"ThemeReset", GTK_STOCK_REVERT_TO_SAVED, _("_Reset"), NULL, _("Reset theme"), G_CALLBACK(menuReset)},
		 {"ThemeEdit", GTK_STOCK_PROPERTIES, _("_Edit theme..."), NULL, _("Edit selected theme"), G_CALLBACK(edit_theme)},
		 {"ThemeMakeDefault", GTK_STOCK_APPLY, _("_Make default"), NULL, _("Replace the default theme with the selected one"), G_CALLBACK(make_selected_theme_default)},
		 {"ThemeRefresh", GTK_STOCK_REFRESH, _("Refresh"), NULL, _("Refresh"), G_CALLBACK(refresh_current_theme)},
		 {"RefreshAll", GTK_STOCK_REFRESH, _("Refresh all"), NULL, _("Refresh all"), G_CALLBACK(load_all_themes)},
		 {"Quit", GTK_STOCK_QUIT, _("_Quit"), "<control>Q", _("Quit"), G_CALLBACK(gtk_main_quit)},
		 {"HelpMenu", NULL, _("Help"), NULL, NULL, NULL},
		 {"HelpAbout", GTK_STOCK_ABOUT, _("_About"), "<Control>A", _("About"), G_CALLBACK(menuAbout)}};

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
	gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
	col++;

	tint_cmd = gtk_entry_new();
	gtk_widget_show(tint_cmd);
	gtk_entry_set_text(GTK_ENTRY(tint_cmd), "");
	gtk_table_attach(GTK_TABLE(table), tint_cmd, col, col + 1, row, row + 1, GTK_FILL | GTK_EXPAND, 0, 0, 0);

	scrollbar = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vBox), scrollbar, TRUE, TRUE, 0);

	// define theme view
	g_theme_view = create_view();
	gtk_container_add(GTK_CONTAINER(scrollbar), g_theme_view);
	gtk_widget_show(g_theme_view);
	g_signal_connect(g_theme_view, "button-press-event", (GCallback)view_onButtonPressed, NULL);
	g_signal_connect(g_theme_view, "popup-menu", (GCallback)view_onPopupMenu, NULL);
	g_signal_connect(g_theme_view, "row-activated", G_CALLBACK(viewRowActivated), NULL);
	g_signal_connect(g_theme_view, "cursor-changed", G_CALLBACK(theme_selection_changed), NULL);

	// load themes
	load_all_themes();

	gtk_widget_show_all(g_window);
	gtk_main();
	return 0;
}

static void menuAbout()
{
	const char *authors[] = {"Thierry Lorthiois <lorthiois@bbsoft.fr>",
							 "Andreas Fink <andreas.fink85@googlemail.com>",
							 "Christian Ruppert <Spooky85@gmail.com> (Build system)",
							 "Euan Freeman <euan04@gmail.com> (tintwizard http://code.google.com/p/tintwizard)",
							 (NULL)};

	gtk_show_about_dialog(GTK_WINDOW(g_window),
						  "name",
						  g_get_application_name(),
						  "comments",
						  _("Theming tool for tint2 panel"),
						  "version",
						  VERSION_STRING,
						  "copyright",
						  _("Copyright 2009-2015 tint2 team\nTint2 License GNU GPL version 2\nTintwizard License GNU "
							"GPL version 3"),
						  "logo-icon-name",
						  "taskbar",
						  "authors",
						  authors,
						  /* Translators: translate "translator-credits" as your name to have it appear in the credits
							 in the "About" dialog */
						  "translator-credits",
						  _("translator-credits"),
						  NULL);
}

// ====== Theme import/copy/delete ======

// Shows open dialog and copies the selected files to ~ without overwrite.
static void menuImportFile()
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Import theme(s)"),
													GTK_WINDOW(g_window),
													GTK_FILE_CHOOSER_ACTION_OPEN,
													GTK_STOCK_CANCEL,
													GTK_RESPONSE_CANCEL,
													GTK_STOCK_ADD,
													GTK_RESPONSE_ACCEPT,
													NULL);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	gtk_file_chooser_set_select_multiple(chooser, TRUE);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
		gtk_widget_destroy(dialog);
		return;
	}

	GSList *list = gtk_file_chooser_get_filenames(chooser);
	for (GSList *l = list; l; l = l->next) {
		gchar *newpath = import_no_overwrite(l->data);
		g_free(newpath);
	}
	g_slist_foreach(list, (GFunc)g_free, NULL);
	g_slist_free(list);
	gtk_widget_destroy(dialog);
}

// Returns the path to the currently selected theme, or NULL if no theme is selected. Needs gfree.
gchar *get_current_theme_path()
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
	GtkTreeIter iter;
	GtkTreeModel *model;
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
		gchar *filepath;
		gtk_tree_model_get(model, &iter, COL_THEME_FILE, &filepath, -1);
		return filepath;
	}
	return NULL;
}

// Returns the path to the currently selected theme, or NULL if no theme is selected. Needs gfree.
// Shows an error box if not theme is selected.
gchar *get_selected_theme_or_warn()
{
	gchar *filepath = get_current_theme_path();
	if (!filepath) {
		GtkWidget *w = gtk_message_dialog_new(GTK_WINDOW(g_window),
											  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
											  GTK_MESSAGE_ERROR,
											  GTK_BUTTONS_CLOSE,
											  _("Please select a theme first."));
		g_signal_connect_swapped(w, "response", G_CALLBACK(gtk_widget_destroy), w);
		gtk_widget_show(w);
	}
	return filepath;
}

// For the selected theme, shows save dialog (with overwrite confirmation) and copies to the selected file with overwrite.
// Shows error box if no theme is selected.
static void menuSaveAs()
{
	gchar *filepath = get_selected_theme_or_warn();
	if (!filepath)
		return;

	gchar *filename = file_name_from_path(filepath);
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Save theme as"),
													GTK_WINDOW(g_window),
													GTK_FILE_CHOOSER_ACTION_SAVE,
													GTK_STOCK_CANCEL,
													GTK_RESPONSE_CANCEL,
													GTK_STOCK_SAVE,
													GTK_RESPONSE_ACCEPT,
													NULL);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
	gchar *config_dir = g_build_filename(g_get_home_dir(), ".config", "tint2", NULL);
	gtk_file_chooser_set_current_folder(chooser, config_dir);
	g_free(config_dir);
	gtk_file_chooser_set_current_name(chooser, filename);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *newpath = gtk_file_chooser_get_filename(chooser);
		import_with_overwrite(filepath, newpath);
		g_free(newpath);
	}
	gtk_widget_destroy(dialog);
	g_free(filepath);
}

// Deletes the selected theme with confirmation.
static void menuDelete()
{
	gchar *filepath = get_selected_theme_or_warn();
	if (!filepath)
		return;

	GtkWidget *w = gtk_message_dialog_new(GTK_WINDOW(g_window),
										  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_MESSAGE_QUESTION,
										  GTK_BUTTONS_YES_NO,
										  _("Delete the selected theme?"));
	gint response = gtk_dialog_run(GTK_DIALOG(w));
	gtk_widget_destroy(w);
	if (response != GTK_RESPONSE_YES) {
		g_free(filepath);
		return;
	}

	GFile *file = g_file_new_for_path(filepath);
	if (g_file_trash(file, NULL, NULL)) {
		remove_theme(filepath);
	}
	g_object_unref(G_OBJECT(file));
	g_free(filepath);
}

static void menuReset()
{
	gchar *filepath = get_selected_theme_or_warn();
	if (!filepath)
		return;
	gchar *filename = file_name_from_path(filepath);
	if (!filename) {
		g_free(filepath);
		return;
	}

	gchar *syspath = find_theme_in_system_dirs(filename);
	if (!syspath)
		syspath = find_theme_in_system_dirs("tint2rc");
	if (!syspath) {
		g_free(filepath);
		return;
	}

	GtkWidget *w = gtk_message_dialog_new(GTK_WINDOW(g_window),
										  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_MESSAGE_QUESTION,
										  GTK_BUTTONS_YES_NO,
										  _("Reset the selected theme?"));
	gint response = gtk_dialog_run(GTK_DIALOG(w));
	gtk_widget_destroy(w);
	if (response != GTK_RESPONSE_YES) {
		g_free(filepath);
		g_free(syspath);
		return;
	}

	import_with_overwrite(syspath, filepath);
	g_free(filepath);
	g_free(syspath);
}

// ====== Theme popup menu ======

static void show_popup_menu(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(globalUIManager, "/ThemePopup")),
				   NULL,
				   NULL,
				   NULL,
				   NULL,
				   (event != NULL) ? event->button : 0,
				   gdk_event_get_time((GdkEvent *)event));
}

static gboolean view_onButtonPressed(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	// single click with the right mouse button?
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		if (gtk_tree_selection_count_selected_rows(selection) <= 1) {
			// Get tree path for row that was clicked
			GtkTreePath *path;
			if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
											  (gint)event->x,
											  (gint)event->y,
											  &path,
											  NULL,
											  NULL,
											  NULL)) {
				gtk_tree_selection_unselect_all(selection);
				gtk_tree_selection_select_path(selection, path);
				gtk_tree_path_free(path);
			}
		}
		show_popup_menu(treeview, event, userdata);
		return TRUE;
	}
	return FALSE;
}

static gboolean view_onPopupMenu(GtkWidget *treeview, gpointer userdata)
{
	show_popup_menu(treeview, NULL, userdata);
	return TRUE;
}

static void theme_selection_changed(GtkWidget *treeview, gpointer userdata)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
	GtkTreeIter iter;
	GtkTreeModel *model;
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
		gchar *filepath;
		gtk_tree_model_get(model, &iter, COL_THEME_FILE, &filepath, -1);
		gboolean isdefault = theme_is_default(filepath);
		gchar *text = isdefault ? g_strdup_printf("tint2") : g_strdup_printf("tint2 -c %s", filepath);
		gtk_entry_set_text(GTK_ENTRY(tint_cmd), text);
		g_free(text);
		gboolean editable = theme_is_editable(filepath);
		g_free(filepath);
		gtk_action_set_sensitive(gtk_action_group_get_action(actionGroup, "ThemeSaveAs"), TRUE);
		gtk_action_set_sensitive(gtk_action_group_get_action(actionGroup, "ThemeDelete"), editable);
		gtk_action_set_sensitive(gtk_action_group_get_action(actionGroup, "ThemeReset"), editable);
		gtk_action_set_sensitive(gtk_action_group_get_action(actionGroup, "ThemeMakeDefault"), !isdefault);
		gtk_action_set_sensitive(gtk_action_group_get_action(actionGroup, "ThemeEdit"), TRUE);
		gtk_action_set_sensitive(gtk_action_group_get_action(actionGroup, "ThemeRefresh"), TRUE);
	} else {
		gtk_entry_set_text(GTK_ENTRY(tint_cmd), "");
		gtk_action_set_sensitive(gtk_action_group_get_action(actionGroup, "ThemeSaveAs"), FALSE);
		gtk_action_set_sensitive(gtk_action_group_get_action(actionGroup, "ThemeDelete"), FALSE);
		gtk_action_set_sensitive(gtk_action_group_get_action(actionGroup, "ThemeReset"), FALSE);
		gtk_action_set_sensitive(gtk_action_group_get_action(actionGroup, "ThemeMakeDefault"), FALSE);
		gtk_action_set_sensitive(gtk_action_group_get_action(actionGroup, "ThemeEdit"), FALSE);
		gtk_action_set_sensitive(gtk_action_group_get_action(actionGroup, "ThemeRefresh"), FALSE);
	}
}

void select_first_theme()
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_first(model, &iter)) {
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view)), &iter);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(g_theme_view), path, NULL, FALSE, 0, 0);
		gtk_tree_path_free(path);
	}
	theme_selection_changed(NULL, NULL);
}

void select_theme(const char *given_path)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
	GtkTreeIter iter;

	gboolean have_iter = gtk_tree_model_get_iter_first(model, &iter);
	while (have_iter) {
		gchar *filepath;
		gtk_tree_model_get(model, &iter, COL_THEME_FILE, &filepath, -1);
		if (g_str_equal(filepath, given_path)) {
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view)), &iter);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(g_theme_view), path, NULL, FALSE, 0, 0);
			gtk_tree_path_free(path);
			g_free(filepath);
			break;
		}
		g_free(filepath);
		have_iter = gtk_tree_model_iter_next(model, &iter);
	}
	theme_selection_changed(NULL, NULL);
}

// Edits the selected theme. If it is read-only, it copies first to ~.
static void edit_theme()
{
	gchar *filepath = get_selected_theme_or_warn();
	if (!filepath)
		return;

	gboolean editable = theme_is_editable(filepath);
	if (!editable) {
		gchar *newpath = import_no_overwrite(filepath);
		g_free(filepath);
		filepath = newpath;
		select_theme(filepath);
	}
	create_please_wait(GTK_WINDOW(g_window));
	process_events();
	GtkWidget *prop = create_properties();
	config_read_file(filepath);
	save_icon_cache(icon_theme);
	gtk_window_present(GTK_WINDOW(prop));
	g_free(filepath);

	destroy_please_wait();
}

static void make_selected_theme_default()
{
	gchar *filepath = get_selected_theme_or_warn();
	if (!filepath)
		return;

	gchar *default_path = get_home_config_path();
	if (g_str_equal(filepath, default_path)) {
		g_free(filepath);
		g_free(default_path);
		return;
	}

	GtkWidget *w = gtk_message_dialog_new(GTK_WINDOW(g_window),
										  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_MESSAGE_QUESTION,
										  GTK_BUTTONS_YES_NO,
										  _("Replace the default theme with the selected theme?"));
	gint response = gtk_dialog_run(GTK_DIALOG(w));
	gtk_widget_destroy(w);
	if (response != GTK_RESPONSE_YES) {
		g_free(filepath);
		g_free(default_path);
		return;
	}

	import_with_overwrite(filepath, default_path);
	select_first_theme();

	g_free(filepath);
	g_free(default_path);
}

static void viewRowActivated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	edit_theme();
}

// ====== Theme load/reload ======

static void ensure_default_theme_exists()
{
	// Without a user tint2rc file, copy the default
	gchar *path_home = g_build_filename(g_get_user_config_dir(), "tint2", "tint2rc", NULL);
	if (!g_file_test(path_home, G_FILE_TEST_EXISTS)) {
		const gchar *const *system_dirs = g_get_system_config_dirs();
		for (int i = 0; system_dirs[i]; i++) {
			gchar *path = g_build_filename(system_dirs[i], "tint2", "tint2rc", NULL);
			if (g_file_test(path, G_FILE_TEST_EXISTS)) {
				copy_file(path, path_home);
				break;
			}
			g_free(path);
		}
	}
	g_free(path_home);
}

static gboolean load_user_themes()
{
	// Load configs from home directory
	gchar *tint2_config_dir = g_build_filename(g_get_user_config_dir(), "tint2", NULL);
	GDir *dir = g_dir_open(tint2_config_dir, 0, NULL);
	if (dir == NULL) {
		g_free(tint2_config_dir);
		return FALSE;
	}
	gboolean found_theme = FALSE;

	const gchar *file_name;
	while ((file_name = g_dir_read_name(dir))) {
		if (!g_file_test(file_name, G_FILE_TEST_IS_DIR) && !strstr(file_name, "backup") && !strstr(file_name, "copy") &&
			!strstr(file_name, "~") && (endswith(file_name, "tint2rc") || endswith(file_name, ".conf"))) {
			found_theme = TRUE;
			gchar *path = g_build_filename(tint2_config_dir, file_name, NULL);
			theme_list_append(path, NULL);
			g_free(path);
		}
	}
	g_dir_close(dir);
	g_free(tint2_config_dir);

	return found_theme;
}

static gboolean load_themes_from_dirs(const gchar *const *dirs)
{
	gboolean found_theme = FALSE;
	for (int i = 0; dirs[i]; i++) {
		gchar *path_tint2 = g_build_filename(dirs[i], "tint2", NULL);
		GDir *dir = g_dir_open(path_tint2, 0, NULL);
		if (dir) {
			const gchar *file_name;
			while ((file_name = g_dir_read_name(dir))) {
				if (!g_file_test(file_name, G_FILE_TEST_IS_DIR) && !strstr(file_name, "backup") &&
					!strstr(file_name, "copy") && !strstr(file_name, "~") &&
					(endswith(file_name, "tint2rc") || endswith(file_name, ".conf"))) {
					found_theme = TRUE;
					gchar *path = g_build_filename(path_tint2, file_name, NULL);
					theme_list_append(path, dirs[i]);
					g_free(path);
				}
			}
			g_dir_close(dir);
		}
		g_free(path_tint2);
	}
	return found_theme;
}

static gboolean load_system_themes()
{
	gboolean found_theme = FALSE;
	if (load_themes_from_dirs(g_get_system_config_dirs()))
		found_theme = TRUE;
	if (load_themes_from_dirs(g_get_system_data_dirs()))
		found_theme = TRUE;
	return found_theme;
}

static gchar *find_theme_in_dirs(const gchar *const *dirs, const gchar *given_name)
{
	for (int i = 0; dirs[i]; i++) {
		gchar *filepath = g_build_filename(dirs[i], "tint2", given_name, NULL);
		if (g_file_test(filepath, G_FILE_TEST_EXISTS)) {
			return filepath;
		}
		g_free(filepath);
	}
	return NULL;
}

static gchar *find_theme_in_system_dirs(const gchar *given_name)
{
	gchar *result = find_theme_in_dirs(g_get_system_config_dirs(), given_name);
	if (result)
		return result;
	return find_theme_in_dirs(g_get_system_data_dirs(), given_name);
}

static void load_all_themes()
{
	ensure_default_theme_exists();

	gtk_list_store_clear(GTK_LIST_STORE(theme_list_store));
	theme_selection_changed(NULL, NULL);

	gboolean found_themes = FALSE;
	if (load_user_themes())
		found_themes = TRUE;
	if (load_system_themes())
		found_themes = TRUE;

	if (found_themes) {
		select_first_theme();

		GtkTreeIter iter;
		GtkTreeModel *model;
		gboolean have_iter;

		model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
		have_iter = gtk_tree_model_get_iter_first(model, &iter);
		while (have_iter) {
			gtk_list_store_set(theme_list_store, &iter, COL_SNAPSHOT, NULL, -1);
			have_iter = gtk_tree_model_iter_next(model, &iter);
		}

		g_timeout_add(SNAPSHOT_TICK, (GSourceFunc)update_snapshot, NULL);
	}
}

void refresh_current_theme()
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
	GtkTreeIter iter;
	GtkTreeModel *model;
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
		gtk_list_store_set(theme_list_store, &iter, COL_SNAPSHOT, NULL, -1);
		g_timeout_add(SNAPSHOT_TICK, (GSourceFunc)update_snapshot, NULL);
	}
}

void refresh_theme(const char *given_path)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
	GtkTreeIter iter;

	gboolean have_iter = gtk_tree_model_get_iter_first(model, &iter);
	while (have_iter) {
		gchar *filepath;
		gtk_tree_model_get(model, &iter, COL_THEME_FILE, &filepath, -1);
		if (g_str_equal(filepath, given_path)) {
			gtk_list_store_set(theme_list_store, &iter, COL_SNAPSHOT, NULL, -1);
			g_timeout_add(SNAPSHOT_TICK, (GSourceFunc)update_snapshot, NULL);
			g_free(filepath);
			break;
		}
		g_free(filepath);
		have_iter = gtk_tree_model_iter_next(model, &iter);
	}
}

void remove_theme(const char *given_path)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
	GtkTreeIter iter;

	gboolean have_iter = gtk_tree_model_get_iter_first(model, &iter);
	while (have_iter) {
		gchar *filepath;
		gtk_tree_model_get(model, &iter, COL_THEME_FILE, &filepath, -1);
		if (g_str_equal(filepath, given_path)) {
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
			theme_selection_changed(NULL, NULL);
			g_free(filepath);
			break;
		}
		g_free(filepath);
		have_iter = gtk_tree_model_iter_next(model, &iter);
	}
}
