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


// need GTK+-2.4 ou plus

#define LONG_VERSION_STRING "0.2"

static GtkUIManager *myUIManager = NULL;

static const char *fallback_ui_file =
        "<ui>"
        "  <menubar name='MenuBar'>"
        "    <menu action='ThemeMenu'>"
        "    </menu>"
        "  </menubar>"
        "  <toolbar  name='ToolBar'>"
        "    <toolitem action='ViewApply'/>"
        "  </toolbar>"
        "</ui>";


static void about( GtkWindow * parent )
{
	const char *authors[] = { "Thierry Lorthiois", "Christian Ruppert (Build system)", NULL };

	const char *website_url = "http://code.google.com/p/tint2/";

	gtk_show_about_dialog( parent, "name", g_get_application_name( ),
								"comments", _( "Theming tool for tint2 panel" ),
								"version", LONG_VERSION_STRING,
								"website", website_url, "website-label", website_url,
								"copyright", _( "Copyright 2009 Thierry Lorthiois" ),
								"logo-icon-name", "",
#ifdef SHOW_LICENSE
								"license", LICENSE, "wrap-license", TRUE,
#endif
								"authors", authors,
								/* Translators: translate "translator-credits" as
									your name to have it appear in the credits in the "About"
									dialog */
								"translator-credits", _( "translator-credits" ),
								NULL );
}


static void destroy( GtkWidget *widget, gpointer   data )
{
    gtk_main_quit ();
}


int main (int argc, char ** argv)
{
/*
	bindtextdomain( domain, LOCALEDIR );
	bind_textdomain_codeset( domain, "UTF-8" );
	textdomain( domain );
*/

	setlocale( LC_ALL, "" );
	gtk_init (&argc, &argv);
	g_thread_init( NULL );

	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Tint2 theme");
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 350);

	// define GUI action
	myUIManager = gtk_ui_manager_new ( );
	GtkActionGroup *action_group;

	//actions_init ( myUIManager, cbdata );
	gtk_ui_manager_add_ui_from_string ( myUIManager, fallback_ui_file, -1, NULL );
	gtk_ui_manager_ensure_update ( myUIManager );

   // XDG specification
	char *dir = g_build_filename (g_get_user_config_dir(), "tint2", NULL);
	if (!g_file_test (dir, G_FILE_TEST_IS_DIR)) g_mkdir(dir, 0777);
	//window.view.set_dir(dir);
	//window.view.load_dir();
	g_free(dir);

	g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);
	// rig up idle/thread routines
	//Glib::Thread::create(sigc::mem_fun(window.view, &Thumbview::load_cache_images), true);
	//Glib::Thread::create(sigc::mem_fun(window.view, &Thumbview::create_cache_images), true);

	gtk_widget_show (window);
	gtk_main ();

	return 0;
}

