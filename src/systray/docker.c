#include "version.h"
#include "kde.h"
#include "icons.h"
#include "docker.h"
#include "net.h"

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xutil.h>

int argc;
char **argv;

Window win = None, hint_win = None, root = None;
gboolean wmaker = FALSE; /* WindowMakerMode!!! wheeee */
Display *display = NULL;
GSList *icons = NULL;
int width = 0, height = 0;
int border = 1; /* blank area around icons. must be > 0 */
gboolean horizontal = TRUE; /* layout direction */
int icon_size = 24; /* width and height of systray icons */

//static char *display_string = NULL;
/* excluding the border. sum of all child apps */
static gboolean exit_app = FALSE;

/*
void create_hint_win()
{
  XWMHints hints;
  XClassHint classhints;

  hint_win = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, 0, 0);
  assert(hint_win);

  hints.flags = StateHint | WindowGroupHint | IconWindowHint;
  hints.initial_state = WithdrawnState;
  hints.window_group = hint_win;
  hints.icon_window = win;

  classhints.res_name = "docker";
  classhints.res_class = "Docker";

  XSetWMProperties(display, hint_win, NULL, NULL, argv, argc,
                   NULL, &hints, &classhints);

  XMapWindow(display, hint_win);
}


void create_main_window()
{
  XWMHints hints;
  XTextProperty text;
  char *name = "Docker";

  // the border must be > 0 if not in wmaker mode
  assert(wmaker || border > 0);

  if (!wmaker)
    win = XCreateSimpleWindow(display, root, 0, 0,
                              border * 2, border * 2, 0, 0, 0);
  else
    win = XCreateSimpleWindow(display, root, 0, 0,
                              64, 64, 0, 0, 0);

  assert(win);

  XStringListToTextProperty(&name, 1, &text);
  XSetWMName(display, win, &text);

  hints.flags = StateHint;
  hints.initial_state = WithdrawnState;
  XSetWMHints(display, win, &hints);

  create_hint_win();

  XSync(display, False);
  XSetWindowBackgroundPixmap(display, win, ParentRelative);
  XClearWindow(display, win);
}
*/

void reposition_icons()
{
  int x = border + ((width % icon_size) / 2),
      y = border + ((height % icon_size) / 2);
  GSList *it;

  for (it = icons; it != NULL; it = g_slist_next(it)) {
    TrayWindow *traywin = it->data;
    traywin->x = x;
    traywin->y = y;
    XMoveWindow(display, traywin->id, x, y);
    XSync(display, False);
    if (wmaker) {
      x += icon_size;
      if (x + icon_size > width) {
        x = border;
        y += icon_size;
      }
    } else if (horizontal)
      x += icon_size;
    else
      y += icon_size;
  }
}


void fix_geometry()
{
  GSList *it;

  // in wmaker mode we're a fixed size
  if (wmaker) return;

  //* find the proper width and height
  width = horizontal ? 0 : icon_size;
  height = horizontal ? icon_size : 0;
  for (it = icons; it != NULL; it = g_slist_next(it)) {
    if (horizontal)
      width += icon_size;
    else
      height += icon_size;
  }

  XResizeWindow(display, win, width + border * 2, height + border * 2);
}

/*
int main(int c, char **v)
{
  struct sigaction act;

  argc = c; argv = v;

  act.sa_handler = signal_handler;
  act.sa_flags = 0;
  sigaction(SIGSEGV, &act, NULL);
  sigaction(SIGPIPE, &act, NULL);
  sigaction(SIGFPE, &act, NULL);
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGHUP, &act, NULL);

  parse_cmd_line(argc, argv);

  display = XOpenDisplay(display_string);
  if (!display) {
    g_printerr("Unable to open Display %s. Exiting.\n",
               DisplayString(display_string));
  }

  root = RootWindow(display, DefaultScreen(display));
  assert(root);

  if (wmaker)
    width = height = 64 - border * 2;

  create_main_window();

  // set up to find KDE systray icons, and get any that already exist
  kde_init();

  net_init();

  // we want to get ConfigureNotify events, and assume our parent's background
  // has changed when we do, so we need to refresh ourself to match
  XSelectInput(display, win, StructureNotifyMask);

  event_loop();

  XCloseDisplay(display);

  return 0;
}
*/
