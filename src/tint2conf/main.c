/**************************************************************************
*
* Tint2conf
*
* Copyright (C) 2009 Thierry lorthiois (lorthiois@bbsoft.fr)
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>

#include "../version.h"
#include "common.h"
#include "theme_view.h"



// default config file and directory
char *g_path_config = NULL;
char *g_path_dir = NULL;
char *g_default_theme = NULL;
int g_width, g_height;

GtkWidget *g_window;
GtkWidget *g_theme_view;

static GtkUIManager *globalUIManager = NULL;

static void menuAddWidget (GtkUIManager *, GtkWidget *, GtkContainer *);

// action on menus
static void menuAdd();
static void menuSaveAs();
static void menuDelete();
static void menuProperties();
static void menuQuit();
static void menuRefresh();
static void menuRefreshAll();
static void menuPreferences();
static void menuApply();
static void menuAbout();

static gboolean view_onPopupMenu (GtkWidget *treeview, gpointer userdata);
static gboolean view_onButtonPressed (GtkWidget *treeview, GdkEventButton *event, gpointer userdata);
static void windowSizeAllocated();
static void viewRowActivated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);


// theme files
static void load_theme();
static void read_config();
static void write_config();
static void check_theme();


// define menubar, toolbar and popup
static const char *global_ui =
	"<ui>"
	"  <menubar name='MenuBar'>"
	"    <menu action='ThemeMenu'>"
	"      <menuitem action='ThemeAdd'/>"
	"      <menuitem action='ThemeSaveAs'/>"
	"      <separator/>"
	"      <menuitem action='ThemeDelete'/>"
	"      <separator/>"
//	"      <menuitem action='ThemeProperties'/>"
//	"      <separator/>"
	"      <menuitem action='ThemeQuit'/>"
	"    </menu>"
	"    <menu action='EditMenu'>"
	"      <menuitem action='EditRefresh'/>"
	"      <menuitem action='EditRefreshAll'/>"
//	"      <separator/>"
//	"      <menuitem action='EditPreferences'/>"
	"    </menu>"
	"    <menu action='HelpMenu'>"
	"      <menuitem action='HelpAbout'/>"
	"    </menu>"
	"  </menubar>"
	"  <toolbar  name='ToolBar'>"
	"    <toolitem action='ThemeProperties'/>"
	"    <toolitem action='ViewApply'/>"
	"  </toolbar>"
	"  <popup  name='ThemePopup'>"
	"    <menuitem action='EditRefresh'/>"
	"    <menuitem action='ThemeProperties'/>"
	"    <menuitem action='ViewApply'/>"
	"    <separator/>"
	"    <menuitem action='ThemeDelete'/>"
	"  </popup>"
	"</ui>";


// define menubar and toolbar action
static GtkActionEntry entries[] = {
	{"ThemeMenu", NULL, "Theme", NULL, NULL, NULL},
	{"ThemeAdd", GTK_STOCK_ADD, "_Add...", "<Control>N", "Add theme", G_CALLBACK (menuAdd)},
	{"ThemeSaveAs", GTK_STOCK_SAVE_AS, "_Save as...", NULL, "Save theme as", G_CALLBACK (menuSaveAs)},
	{"ThemeDelete", GTK_STOCK_DELETE, "_Delete", NULL, "Delete theme", G_CALLBACK (menuDelete)},
	{"ThemeProperties", GTK_STOCK_PROPERTIES, "_Properties...", NULL, "Show properties", G_CALLBACK (menuProperties)},
	{"ThemeQuit", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit", G_CALLBACK (menuQuit)},
	{"EditMenu", NULL, "Edit", NULL, NULL, NULL},
	{"EditRefresh", GTK_STOCK_REFRESH, "Refresh", NULL, "Refresh", G_CALLBACK (menuRefresh)},
	{"EditRefreshAll", GTK_STOCK_REFRESH, "Refresh all", NULL, "Refresh all", G_CALLBACK (menuRefreshAll)},
//	{"EditPreferences", GTK_STOCK_PREFERENCES, "Preferences", NULL, "Preferences", G_CALLBACK (menuPreferences)},
	{"ViewApply", GTK_STOCK_APPLY, "Apply", NULL, "Apply theme", G_CALLBACK (menuApply)},
	{"HelpMenu", NULL, "Help", NULL, NULL, NULL},
	{"HelpAbout", GTK_STOCK_ABOUT, "_About", "<Control>A", "About", G_CALLBACK (menuAbout)}
};


int main (int argc, char ** argv)
{
	GtkWidget *vBox = NULL, *scrollbar = NULL;
	GtkActionGroup *actionGroup;

	gtk_init (&argc, &argv);
	g_thread_init( NULL );
	read_config();
	check_theme();

	// define main layout : container, menubar, toolbar
	g_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(g_window), _("Panel theming"));
	gtk_window_resize(GTK_WINDOW(g_window), g_width, g_height);
	g_signal_connect(G_OBJECT(g_window), "destroy", G_CALLBACK (menuQuit), NULL);
	g_signal_connect(g_window, "size-allocate", G_CALLBACK(windowSizeAllocated), NULL);
	vBox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER(g_window), vBox);

	actionGroup = gtk_action_group_new ("menuActionGroup");
   gtk_action_group_add_actions (actionGroup, entries, G_N_ELEMENTS (entries), NULL);
	globalUIManager = gtk_ui_manager_new();
   gtk_ui_manager_insert_action_group (globalUIManager, actionGroup, 0);
	gtk_ui_manager_add_ui_from_string (globalUIManager, global_ui, -1, NULL );
	g_signal_connect(globalUIManager, "add_widget", G_CALLBACK (menuAddWidget), vBox);
	gtk_ui_manager_ensure_update(globalUIManager);
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

   // load themes
	load_theme(g_theme_view);

	// rig up idle/thread routines
	//Glib::Thread::create(sigc::mem_fun(window.view, &Thumbview::load_cache_images), true);
	//Glib::Thread::create(sigc::mem_fun(window.view, &Thumbview::create_cache_images), true);

	gtk_widget_show_all(g_window);
	gtk_main ();
	return 0;
}


static void menuAddWidget (GtkUIManager * p_uiManager, GtkWidget * p_widget, GtkContainer * p_box)
{
   gtk_box_pack_start(GTK_BOX(p_box), p_widget, FALSE, FALSE, 0);
   gtk_widget_show(p_widget);
}


static void menuAbout()
{
	const char *authors[] = { "Thierry Lorthiois <lorthiois@bbsoft.fr>", "Andreas Fink <andreas.fink85@googlemail.com>", "Christian Ruppert <Spooky85@gmail.com> (Build system)", "Euan Freeman <euan04@gmail.com> (tintwizard)\n  See http://code.google.com/p/tintwizard/", NULL };

	gtk_show_about_dialog(g_window, "name", g_get_application_name( ),
								"comments", _("Theming tool for tint2 panel"),
								"version", VERSION_STRING,
								"copyright", _("Copyright 2009 tint2 team\nTint2 License GNU GPL version 2\nTintwizard License GNU GPL version 3"),
								"logo-icon-name", NULL, "authors", authors,
								/* Translators: translate "translator-credits" as
									your name to have it appear in the credits in the "About"
									dialog */
								"translator-credits", _("translator-credits"),
								NULL);
}


static void menuAdd()
{
	GtkWidget *dialog;
	GtkFileChooser *chooser;
	GtkFileFilter *filter;

	dialog = gtk_file_chooser_dialog_new(_("Add a theme"), g_window, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_ADD, GTK_RESPONSE_ACCEPT, NULL);
	chooser = GTK_FILE_CHOOSER(dialog);

	gtk_file_chooser_set_current_folder(chooser, g_get_home_dir());
	gtk_file_chooser_set_select_multiple(chooser, TRUE);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Tint2 theme files"));
	gtk_file_filter_add_pattern(filter, "*.tint2rc");
	gtk_file_chooser_add_filter(chooser, filter);

	if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *l, *list = gtk_file_chooser_get_filenames(chooser);

		gchar *file, *pt1, *name, *path;
		for (l = list; l ; l = l->next) {
			file = (char *)l->data;
			pt1 = strrchr (file, '/');
			if (pt1) {
				pt1++;
				if (*pt1) {
					name = g_strdup(pt1);
					path = g_build_filename (g_get_user_config_dir(), "tint2", name, NULL);
					copy_file(file, path);
			      custom_list_append(path);
					g_free(path);
					g_free(name);
				}
			}
		}

		g_slist_foreach(list, (GFunc)g_free, NULL);
		g_slist_free(list);
	}
	gtk_widget_destroy (dialog);
}


static void menuSaveAs ()
{
	GtkWidget *dialog;
	GtkFileChooser *chooser;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *file, *pt1;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
	if (!gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
		GtkWidget *w = gtk_message_dialog_new(g_window, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Select the theme to be saved."), NULL);
		g_signal_connect_swapped(w, "response", G_CALLBACK(gtk_widget_destroy), w);
		gtk_widget_show(w);
		return;
	}

	gtk_tree_model_get(model, &iter, COL_THEME_FILE, &file, -1);
	pt1 = strrchr (file, '/');
	if (pt1) pt1++;

	dialog = gtk_file_chooser_dialog_new(_("Save theme as"), g_window, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	chooser = GTK_FILE_CHOOSER(dialog);

	gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
	gtk_file_chooser_set_current_folder(chooser, g_get_home_dir());
	gtk_file_chooser_set_current_name(chooser, pt1);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(chooser);
		copy_file(file, filename);
		g_free (filename);
	}
	g_free(file);
	gtk_widget_destroy (dialog);
}


static void menuDelete()
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *file;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
		gtk_tree_model_get(model, &iter, COL_THEME_FILE, &file, -1);
		gtk_tree_selection_unselect_all(sel);

		// remove (gui and disk)
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
		g_remove(file);
		g_free(file);
	}
}


static void menuProperties()
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	char *file, *cmd;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
		gtk_tree_model_get(model, &iter, COL_THEME_FILE, &file,  -1);

		cmd = g_strdup_printf("gedit \'%s\' &", file);
		system(cmd);

		g_free(cmd);
		g_free(file);
	}
}


static void menuQuit()
{
	write_config();

	if (g_path_config)
		g_free(g_path_config);
	if (g_path_dir)
		g_free(g_path_dir);
	if (g_default_theme)
		g_free(g_default_theme);

   gtk_main_quit ();
}


static void menuRefresh()
{
	printf("menuRefresh\n");
}


static void menuRefreshAll()
{
	printf("menuRefreshAll\n");
}


static void menuPreferences()
{
	printf("menuPreferences\n");
}


static void menuApply()
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (g_default_theme) {
		g_free(g_default_theme);
		g_default_theme = NULL;
	}
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
		gtk_tree_model_get(model, &iter, COL_THEME_FILE, &g_default_theme,  -1);
		// overwrite tint2rc
		copy_file(g_default_theme, g_path_config);

		// restart panel
		system("killall -SIGUSR1 tint2");
	}
}


static void view_popup_menu(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	GtkWidget *w = gtk_ui_manager_get_widget(globalUIManager, "/ThemePopup");

	gtk_menu_popup(GTK_MENU(w), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0, gdk_event_get_time((GdkEvent*)event));
}


static gboolean view_onButtonPressed (GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
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


static gboolean view_onPopupMenu (GtkWidget *treeview, gpointer userdata)
{
	view_popup_menu(treeview, NULL, userdata);
	return TRUE;
}


static void viewRowActivated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	menuApply();
}


static void windowSizeAllocated()
{
	const gboolean isMaximized = g_window->window && (gdk_window_get_state(g_window->window) & GDK_WINDOW_STATE_MAXIMIZED);

	if(!isMaximized)
		gtk_window_get_size(GTK_WINDOW(g_window), &g_width, &g_height);
}


static void load_theme(GtkWidget *list)
{
	GDir *dir;
	gchar *pt1, *name, *file;
	gboolean have_iter, found_theme = FALSE;
	GtkTreeIter iter;
	GtkTreeModel *model;

	dir = g_dir_open(g_path_dir, 0, NULL);
	if (dir == NULL) return;
	while ((file = g_dir_read_name(dir))) {
		pt1 = strstr(file, ".tint2rc");
		if (pt1) {
			found_theme = TRUE;
			name = g_build_filename (g_path_dir, file, NULL);
			custom_list_append(name);
			g_free(name);
		}
	}
	g_dir_close(dir);

	if (!found_theme) {
		// create default theme file
		name = g_build_filename (g_get_user_config_dir(), "tint2", "default.tint2rc", NULL);
		copy_file(g_path_config, name);
		custom_list_append(name);
		g_free(name);
	}

	// search default theme
	found_theme = FALSE;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
	have_iter = gtk_tree_model_get_iter_first(model, &iter);
	while (have_iter) {
		gtk_tree_model_get(model, &iter, COL_THEME_FILE, &name,  -1);
		found_theme = (strcmp(name, g_default_theme) == 0);
		g_free(name);
		if (found_theme)
			break;
		have_iter = gtk_tree_model_iter_next(model, &iter);
	}

	// select theme
	GtkTreePath *path = NULL;
	if (found_theme)
		path = gtk_tree_model_get_path(model, &iter);
	else {
		have_iter = gtk_tree_model_get_iter_first(model, &iter);
		if (have_iter)
			path = gtk_tree_model_get_path(model, &iter);
	}
	if (path) {
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view)), &iter);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(g_theme_view), path, NULL, FALSE, 0, 0);
		gtk_tree_path_free(path);
	}

}


// theme file management
void read_config()
{
	char *path;

	if (g_default_theme != NULL) {
		g_free(g_default_theme);
		g_default_theme = NULL;
	}

	g_width = 500;
	g_height = 350;
	path = g_build_filename (g_get_user_config_dir(), "tint2", "tint2confrc", NULL);
	if (g_file_test (path, G_FILE_TEST_EXISTS)) {
		FILE *fp;
		char line[80];
		char *key, *value;
		if ((fp = fopen(path, "r")) != NULL) {
			while (fgets(line, sizeof(line), fp) != NULL) {
				if (parse_line(line, &key, &value)) {
					if (strcmp (key, "default_theme") == 0)
						g_default_theme = strdup (value);
					else if (strcmp (key, "width") == 0)
						g_width = atoi(value);
					else if (strcmp (key, "height") == 0)
						g_height = atoi(value);
					free (key);
					free (value);
				}
			}
			fclose (fp);
		}
	}
	g_free(path);
}


void write_config()
{
	char *path;
	FILE *fp;

	path = g_build_filename (g_get_user_config_dir(), "tint2", "tint2confrc", NULL);
	fp = fopen(path, "w");
	if (fp != NULL) {
		fputs("#---------------------------------------------\n", fp);
		fputs("# TINT2CONF CONFIG FILE\n", fp);
		if (g_default_theme != NULL) {
			fprintf(fp, "default_theme = %s\n", g_default_theme);
			printf("default_theme %s\n", g_default_theme);
		}
		fprintf(fp, "width = %d\n", g_width);
		fprintf(fp, "height = %d\n", g_height);
		fputs("\n", fp);
		fclose (fp);
	}
	g_free(path);
}


void check_theme()
{
	g_path_dir = g_build_filename (g_get_user_config_dir(), "tint2", NULL);
	if (!g_file_test (g_path_dir, G_FILE_TEST_IS_DIR))
		g_mkdir(g_path_dir, 0777);

	g_path_config = g_build_filename (g_get_user_config_dir(), "tint2", "tint2rc", NULL);

}



