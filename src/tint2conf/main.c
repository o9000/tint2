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

#include <stdio.h>
#include <locale.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>


// gcc -Wall -g main1.c -o tint2conf `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0 --libs gthread-2.0`
// TODO
// ** add, saveas
// - liste de fichiers tint2rc*
// - menu contextuel dans liste
// - double clic dans liste
// - données globales
// - delete
// - rename
// - apply
// - sauvegarde et lecture taille de fenetre
// - activation des menus sur sélection dans la liste
// - dialogue propriétés ...
// tint2 -d directory, plutot que -c config ??
// tint2 preview, comment gérer les barres verticales...


#define LONG_VERSION_STRING "0.7"

static GtkUIManager *myUIManager = NULL;

static void menuAddWidget (GtkUIManager *, GtkWidget *, GtkContainer *);
static void viewPopup(GtkWidget *wid,GdkEventButton *event,GtkWidget *menu);

// action on menus
static void menuAdd (GtkWindow * parent);
static void menuSaveAs (GtkWindow *parent);
static void menuProperties (void);
static void menuRename (void);
static void menuDelete (void);
static void menuQuit (void);
static void menuRefresh (void);
static void menuRefreshAll (void);
static void menuApply (void);
static void menuAbout(GtkWindow * parent);

static void onPopupMenu(GtkWidget *self, GdkEventButton *event);
static void viewRowActivated( GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);

static void loadDir();


// define menubar and toolbar
static const char *fallback_ui_file =
	"<ui>"
	"  <menubar name='MenuBar'>"
	"    <menu action='ThemeMenu'>"
	"      <menuitem action='ThemeAdd'/>"
	"      <menuitem action='ThemeSaveAs'/>"
	"      <separator/>"
	"      <menuitem action='ThemeProperties'/>"
	"      <menuitem action='ThemeRename'/>"
	"      <separator/>"
	"      <menuitem action='ThemeDelete'/>"
	"      <separator/>"
	"      <menuitem action='ThemeQuit'/>"
	"    </menu>"
	"    <menu action='ViewMenu'>"
	"      <menuitem action='ViewRefresh'/>"
	"      <menuitem action='ViewRefreshAll'/>"
	"    </menu>"
	"    <menu action='HelpMenu'>"
	"      <menuitem action='HelpAbout'/>"
	"    </menu>"
	"  </menubar>"
	"  <toolbar  name='ToolBar'>"
	"    <toolitem action='ViewRefreshAll'/>"
	"    <separator/>"
	"    <toolitem action='ThemeProperties'/>"
	"    <toolitem action='ViewApply'/>"
	"  </toolbar>"
	"</ui>";

// define menubar and toolbar action
static GtkActionEntry entries[] = {
	{"ThemeMenu", NULL, "Theme", NULL, NULL, NULL},
	{"ThemeAdd", GTK_STOCK_ADD, "_Add...", "<Control>N", "Add theme", G_CALLBACK (menuAdd)},
	{"ThemeSaveAs", GTK_STOCK_SAVE_AS, "_Save as...", NULL, "Save theme as", G_CALLBACK (menuSaveAs)},
	{"ThemeProperties", GTK_STOCK_PROPERTIES, "_Properties...", NULL, "Show properties", G_CALLBACK (menuProperties)},
	{"ThemeRename", NULL, "_Rename...", NULL, "Rename theme", G_CALLBACK (menuRename)},
	{"ThemeDelete", GTK_STOCK_DELETE, "_Delete", NULL, "Delete theme", G_CALLBACK (menuDelete)},
	{"ThemeQuit", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit", G_CALLBACK (menuQuit)},
	{"ViewMenu", NULL, "View", NULL, NULL, NULL},
	{"ViewRefresh", GTK_STOCK_REFRESH, "Refresh", NULL, "Refresh", G_CALLBACK (menuRefresh)},
	{"ViewRefreshAll", GTK_STOCK_REFRESH, "Refresh all", NULL, "Refresh all", G_CALLBACK (menuRefreshAll)},
	{"ViewApply", GTK_STOCK_APPLY, "Apply", NULL, "Apply theme", G_CALLBACK (menuApply)},
	{"HelpMenu", NULL, "Help", NULL, NULL, NULL},
	{"HelpAbout", GTK_STOCK_ABOUT, "_About", "<Control>A", "About", G_CALLBACK (menuAbout)}
};


int main (int argc, char ** argv)
{
	GtkWidget *window, *themeView, *popup, *item;
	GtkWidget *vBox = NULL;
	GtkActionGroup *actionGroup;
	GtkTreeViewColumn *col;
	GtkTreeSelection *sel;

	gtk_init (&argc, &argv);
	g_thread_init( NULL );

	// define main layout : container, menubar, toolbar, themeView
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), _("Tint2 theme"));
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 350);
	g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (menuQuit), NULL);
	vBox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER(window), vBox);

	actionGroup = gtk_action_group_new ("menuActionGroup");
   gtk_action_group_add_actions (actionGroup, entries, G_N_ELEMENTS (entries), NULL);
	myUIManager = gtk_ui_manager_new();
   gtk_ui_manager_insert_action_group (myUIManager, actionGroup, 0);
	gtk_ui_manager_add_ui_from_string ( myUIManager, fallback_ui_file, -1, NULL );
	g_signal_connect(myUIManager, "add_widget", G_CALLBACK (menuAddWidget), vBox);
	gtk_ui_manager_ensure_update(myUIManager);

	themeView = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(themeView), FALSE);
	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(themeView), TRUE);
	col = GTK_TREE_VIEW_COLUMN (g_object_new (GTK_TYPE_TREE_VIEW_COLUMN, "title", _("Theme"), "resizable", TRUE, "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL));
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(themeView));
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(sel), GTK_SELECTION_SINGLE);
   gtk_box_pack_start(GTK_BOX(vBox), themeView, TRUE, TRUE, 0);
   gtk_widget_show(themeView);
	g_signal_connect(themeView, "popup-menu", G_CALLBACK(onPopupMenu), NULL);
	g_signal_connect(themeView, "row-activated", G_CALLBACK(viewRowActivated), NULL);
	//g_signal_connect(themeView, "button-press-event", G_CALLBACK(onViewButtonPressed), (void *)onViewButtonPressed);
	//g_signal_connect(themeView, "button-release-event", G_CALLBACK(onViewButtonReleased), NULL);

	// popup menu
	// all you need to do is add the GDK_BUTTON_PRESS_MASK to the window's events
	gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK);
	popup = gtk_menu_new();
	item = gtk_menu_item_new_with_label("victory");
	gtk_menu_shell_append(GTK_MENU_SHELL(popup), item);
	gtk_menu_attach_to_widget(GTK_MENU(popup), window, NULL);
	gtk_widget_show_all(popup);
	g_signal_connect(G_OBJECT(window),"button-press-event", G_CALLBACK(viewPopup), (gpointer)popup);

   // load themes
	loadDir();

	// rig up idle/thread routines
	//Glib::Thread::create(sigc::mem_fun(window.view, &Thumbview::load_cache_images), true);
	//Glib::Thread::create(sigc::mem_fun(window.view, &Thumbview::create_cache_images), true);

	gtk_widget_show_all(window);
	gtk_main ();
	return 0;
}


static void menuAddWidget (GtkUIManager * p_uiManager, GtkWidget * p_widget, GtkContainer * p_box)
{
   gtk_box_pack_start(GTK_BOX(p_box), p_widget, FALSE, FALSE, 0);
   gtk_widget_show(p_widget);
}


static void viewPopup(GtkWidget *wid, GdkEventButton *event, GtkWidget *menu)
{
    if((event->button == 3) && (event->type == GDK_BUTTON_PRESS)) {
        gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,event->button,event->time);
    }
}


static void menuAbout(GtkWindow * parent)
{
	const char *authors[] = { "Thierry Lorthiois", "Christian Ruppert (Build system)", NULL };

	gtk_show_about_dialog( parent, "name", g_get_application_name( ),
								"comments", _("Theming tool for tint2 panel"),
								"version", LONG_VERSION_STRING,
								"copyright", _("Copyright 2009 tint2 team\nLicense GNU GPL version 2"),
								"logo-icon-name", NULL,
								"authors", authors,
								/* Translators: translate "translator-credits" as
									your name to have it appear in the credits in the "About"
									dialog */
								"translator-credits", _("translator-credits"),
								NULL );
}


static void menuAdd (GtkWindow *parent)
{
	GtkWidget *dialog;
	GtkFileChooser *chooser;
	GtkFileFilter *filter;

	dialog = gtk_file_chooser_dialog_new(_("Add a theme"), parent, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_ADD, GTK_RESPONSE_ACCEPT, NULL);
	chooser = GTK_FILE_CHOOSER(dialog);

	//gtk_dialog_set_alternative_button_order(GTK_DIALOG(w), GTK_RESPONSE_ACCEPT, GTK_RESPONSE_CANCEL, -1);
	gtk_file_chooser_set_current_folder(chooser, g_get_home_dir());
	gtk_file_chooser_set_select_multiple(chooser, TRUE);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Tint2 theme files"));
	gtk_file_filter_add_pattern(filter, "*.tint2rc");
	gtk_file_chooser_add_filter(chooser, filter);

	if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *l, *list = gtk_file_chooser_get_filenames(chooser);

		// TODO: remember this folder the next time we use this dialog
		//char *folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
		//pref_string_set( PREF_KEY_OPEN_DIALOG_FOLDER, folder );
		//g_free( folder );

		for (l = list; l ; l = l->next) {
			printf("fichier %s\n", (char *)l->data);
			//add_filename(core, l->data, FALSE);
			//tr_core_torrents_added( core );
		}

		g_slist_foreach(list, (GFunc)g_free, NULL);
		g_slist_free(list);
	}
	gtk_widget_destroy (dialog);
}


static void menuSaveAs (GtkWindow *parent)
{
	GtkWidget *dialog;
	GtkFileChooser *chooser;

	dialog = gtk_file_chooser_dialog_new (_("Save theme as"), parent, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	chooser = GTK_FILE_CHOOSER(dialog);

	gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);
	gtk_file_chooser_set_current_folder (chooser, g_get_home_dir());
	gtk_file_chooser_set_current_name (chooser, _("Untitled document"));

	if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(chooser);
		printf("fichier %s\n", filename);
		//save_to_file (filename);
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
}


static void menuProperties (void)
{
	printf("menuProperties\n");
}


static void menuRename (void)
{
	printf("menuRename\n");
}


static void menuDelete (void)
{
	printf("menuDelete\n");
}


static void menuQuit (void)
{
   gtk_main_quit ();
}


static void menuRefresh (void)
{
	printf("menuRefresh\n");
}


static void menuRefreshAll (void)
{
	printf("menuRefreshAll\n");
}


static void menuApply (void)
{
	printf("menuApply\n");
}


static void onPopupMenu(GtkWidget *self, GdkEventButton *event)
{
	//GtkWidget *menu = action_get_widget("/main-window-popup");

	//gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, (event ? event->button : 0), (event ? event->time : 0));
}


static void viewRowActivated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
    //action_activate("show-torrent-properties");
}


static void loadDir()
{
	char *path = g_build_filename (g_get_user_config_dir(), "tint2", NULL);
	if (!g_file_test (path, G_FILE_TEST_IS_DIR)) {
		g_mkdir(path, 0777);
	}
	g_free(path);

	GDir *dir;
	dir = g_dir_open(path, 0, NULL);

	g_dir_close(dir);
}

