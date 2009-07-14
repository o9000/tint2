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

#include <string>
#include <stdio.h>
#include <gtkmm.h>
#include <glib.h>

#include "mainwin.h"


// g++ `pkg-config --libs --cflags gtkmm-2.4 --libs gthread-2.0` -o tint2conf main.cpp mainwin.cpp thumbview.cpp

int main (int argc, char ** argv)
{
	// set up i18n
	//bindtextdomain(PACKAGE, LOCALEDIR);
	//bind_textdomain_codeset(PACKAGE, "UTF-8");
	//textdomain(PACKAGE);

	Gtk::Main kit(argc, argv);
	Glib::thread_init();

	MainWin window;

	window.view.set_dir("");
	window.view.load_dir();

	// rig up idle/thread routines
	Glib::Thread::create(sigc::mem_fun(window.view, &Thumbview::load_cache_images), true);
	Glib::Thread::create(sigc::mem_fun(window.view, &Thumbview::create_cache_images), true);

	//Shows the window and returns when it is closed.
	Gtk::Main::run(window);

	return 0;
}

