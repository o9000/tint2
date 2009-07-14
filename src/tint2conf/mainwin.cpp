
#include <gtkmm/stock.h>
#include <iostream>
#include "mainwin.h"


MainWin::MainWin()
{
	set_title("Tint2 config");
	set_default_size(600, 350);

	add(m_Box); // put a MenuBar at the top of the box and other stuff below it.

	//Create actions for menus and toolbars:
	m_refActionGroup = Gtk::ActionGroup::create();

	//File menu:
	m_refActionGroup->add(Gtk::Action::create("FileMenu", "File"));
	//Sub-menu.
	m_refActionGroup->add(Gtk::Action::create("FileOpen", Gtk::Stock::OPEN, "_Open", "Open config file"), sigc::mem_fun(*this, &MainWin::on_menu_file_new_generic));
	m_refActionGroup->add(Gtk::Action::create("FileSaveAs", Gtk::Stock::SAVE_AS, "_Save As", "Save config as"), sigc::mem_fun(*this, &MainWin::on_menu_file_new_generic));
	m_refActionGroup->add(Gtk::Action::create("FileRefreshAll", Gtk::Stock::REFRESH, "_Refresh all", "Refresh all config file"), sigc::mem_fun(*this, &MainWin::on_menu_file_new_generic));
	m_refActionGroup->add(Gtk::Action::create("FileQuit", Gtk::Stock::QUIT), sigc::mem_fun(*this, &MainWin::on_menu_file_quit));

	//Edit menu:
	m_refActionGroup->add(Gtk::Action::create("EditMenu", "Edit"));
	m_refActionGroup->add(Gtk::Action::create("EditProperties", Gtk::Stock::PROPERTIES, "_Properties...", "Show properties"), sigc::mem_fun(*this, &MainWin::on_menu_others));
	m_refActionGroup->add(Gtk::Action::create("EditRename", "_Rename...", "Rename current config"), sigc::mem_fun(*this, &MainWin::on_menu_others));
	m_refActionGroup->add(Gtk::Action::create("EditDelete", Gtk::Stock::DELETE), sigc::mem_fun(*this, &MainWin::on_menu_others));
	m_refActionGroup->add(Gtk::Action::create("EditApply", Gtk::Stock::APPLY, "_Apply", "Apply current config"), sigc::mem_fun(*this, &MainWin::on_menu_others));
	m_refActionGroup->add(Gtk::Action::create("EditRefresh", Gtk::Stock::REFRESH), sigc::mem_fun(*this, &MainWin::on_menu_others));

	//Help menu:
	m_refActionGroup->add( Gtk::Action::create("HelpMenu", "Help") );
	m_refActionGroup->add( Gtk::Action::create("About", Gtk::Stock::ABOUT), sigc::mem_fun(*this, &MainWin::on_menu_about) );

	m_refUIManager = Gtk::UIManager::create();
	m_refUIManager->insert_action_group(m_refActionGroup);

	add_accel_group(m_refUIManager->get_accel_group());

	//Layout the actions in a menubar and toolbar:
	Glib::ustring ui_info =
        "<ui>"
        "  <menubar name='MenuBar'>"
        "    <menu action='FileMenu'>"
        "      <menuitem action='FileOpen'/>"
        "      <menuitem action='FileSaveAs'/>"
        "      <separator/>"
        "      <menuitem action='FileRefreshAll'/>"
        "      <separator/>"
        "      <menuitem action='FileQuit'/>"
        "    </menu>"
        "    <menu action='EditMenu'>"
        "      <menuitem action='EditProperties'/>"
        "      <menuitem action='EditRename'/>"
        "      <separator/>"
        "      <menuitem action='EditDelete'/>"
        "      <separator/>"
        "      <menuitem action='EditRefresh'/>"
        "    </menu>"
        "    <menu action='HelpMenu'>"
        "      <menuitem action='About'/>"
        "    </menu>"
        "  </menubar>"
        "  <toolbar  name='ToolBar'>"
        "    <toolitem action='FileRefreshAll'/>"
        "    <separator/>"
        "    <toolitem action='EditProperties'/>"
        "    <toolitem action='EditApply'/>"
        "  </toolbar>"
        "</ui>";

	#ifdef GLIBMM_EXCEPTIONS_ENABLED
	try
	{
		m_refUIManager->add_ui_from_string(ui_info);
	}
	catch(const Glib::Error& ex)
	{
		std::cerr << "building menus failed: " <<  ex.what();
	}
	#else
	std::auto_ptr<Glib::Error> ex;
	m_refUIManager->add_ui_from_string(ui_info, ex);
	if(ex.get())
	{
		std::cerr << "building menus failed: " <<  ex->what();
	}
	#endif //GLIBMM_EXCEPTIONS_ENABLED

  //Get the menubar and toolbar widgets, and add them to a container widget:
  Gtk::Widget* pMenubar = m_refUIManager->get_widget("/MenuBar");
  if(pMenubar)
		m_Box.pack_start(*pMenubar, Gtk::PACK_SHRINK);

  Gtk::Widget* pToolbar = m_refUIManager->get_widget("/ToolBar") ;
  if(pToolbar)
		m_Box.pack_start(*pToolbar, Gtk::PACK_SHRINK);


	m_Box.add(view);

	show_all_children();
}

MainWin::~MainWin()
{
}


void MainWin::on_menu_file_quit()
{
	hide();
}


void MainWin::on_menu_file_new_generic()
{
	std::cout << "A File|New menu item was selected." << std::endl;
}


void MainWin::on_menu_others()
{
	std::cout << "A menu item was selected." << std::endl;
}


void MainWin::on_menu_about()
{
	Glib::ustring message = "tint2conf " + Glib::ustring(VERSION);
	Gtk::MessageDialog dialog(*this, message);

	dialog.set_title("About tint2conf");
	dialog.set_secondary_text("Config tool for tint2.\n\nCopyright (C) 2009 Thierry lorthiois.    \nRefer to source code from Nitrogen\nby Dave Foster & Javeed Shaikh.");

	dialog.run();
}

