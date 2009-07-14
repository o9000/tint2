#ifndef TINT2CONF_MAINWIN_H
#define TINT2CONF_MAINWIN_H

#include <gtkmm.h>
#include "thumbview.h"

#define VERSION "0.2"


class MainWin : public Gtk::Window
{
public:
  MainWin();
  virtual ~MainWin();

	Thumbview view;
protected:
  //Signal handlers:
  void on_menu_file_new_generic();
  void on_menu_file_quit();
  void on_menu_others();
  void on_menu_about();

  //Child widgets:
  Gtk::VBox m_Box;

  Glib::RefPtr<Gtk::UIManager> m_refUIManager;
  Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup;
  Glib::RefPtr<Gtk::RadioAction> m_refChoiceOne, m_refChoiceTwo;
};

#endif

