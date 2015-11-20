/**************************************************************************
* window :
* -
*
* Check COPYING file for Copyright
*
**************************************************************************/

#ifndef WINDOW_H
#define WINDOW_H

#include <glib.h>
#include <pango/pangocairo.h>
#include <X11/Xlib.h>

GSList *get_desktop_names();
int get_current_desktop();
void change_desktop(int desktop);

Window get_active_window();

gboolean window_is_iconified(Window win);
gboolean window_is_urgent(Window win);
gboolean window_is_hidden(Window win);
gboolean window_is_active(Window win);
gboolean window_is_skip_taskbar(Window win);
int get_window_desktop(Window win);
int get_window_monitor(Window win);

void activate_window(Window win);
void close_window(Window win);
void get_window_coordinates(Window win, int *x, int *y, int *w, int *h);
void toggle_window_maximized(Window win);
void toggle_window_shade(Window win);
void change_window_desktop(Window win, int desktop);

int get_icon_count(gulong *data, int num);
gulong *get_best_icon(gulong *data, int icon_count, int num, int *iw, int *ih, int best_icon_size);

#endif
