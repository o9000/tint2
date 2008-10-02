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
void parse_cmd_line()
{
  int i;
  gboolean help = FALSE;
  
  for (i = 1; i < argc; i++) {
    if (0 == strcasecmp(argv[i], "-display")) {
      ++i;
      if (i < argc) {
        display_string = argv[i];
      } else { 
        g_printerr("-display requires a parameter\n");
        help = TRUE;
      }
    } else if (0 == strcasecmp(argv[i], "-wmaker")) {
      wmaker = TRUE;
    } else if (0 == strcasecmp(argv[i], "-vertical")) {
      horizontal = FALSE;
    } else if (0 == strcasecmp(argv[i], "-border")) {
      ++i;

      if (i < argc) {
        int b = atoi(argv[i]);
        if (b > 0) {
          border = b;
        } else {
          g_printerr("-border must be a value greater than 0\n");
          help = TRUE;
        }
      } else { 
        g_printerr("-border requires a parameter\n");
        help = TRUE;
      }
      } else if (0 == strcasecmp(argv[i], "-iconsize")) {
      ++i;
      if (i < argc) {
        int s = atoi(argv[i]);
        if (s > 0) {
          icon_size = s;
        } else {
          g_printerr("-iconsize must be a value greater than 0\n");
          help = TRUE;
        }
      } else { 
        g_printerr("-iconsize requires a parameter\n");
        help = TRUE;
      }
    } else {
      if (argv[i][0] == '-')
        help = TRUE;
    }


    if (help) {
      
      g_print("%s - version %s\n", argv[0], VERSION);
      g_print("Copyright 2003, Ben Jansens <ben@orodu.net>\n\n");
      g_print("Usage: %s [OPTIONS]\n\n", argv[0]);
      g_print("Options:\n");
      g_print("  -help             Show this help.\n");
      g_print("  -display DISLPAY  The X display to connect to.\n");
      g_print("  -border           The width of the border to put around the\n"
              "                    system tray icons. Defaults to 1.\n");
      g_print("  -vertical         Line up the icons vertically. Defaults to\n"
              "                    horizontally.\n");
      g_print("  -wmaker           WindowMaker mode. This makes docker a\n"
              "                    fixed size (64x64) to appear nicely in\n"
              "                    in WindowMaker.\n"
              "                    Note: In this mode, you have a fixed\n"
              "                    number of icons that docker can hold.\n");
      g_print("  -iconsize SIZE    The size (width and height) to display\n"
              "                    icons as in the system tray. Defaults to\n"
              "                    24.\n");
      exit(1);
    }
  }
}
*/

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

  /* the border must be > 0 if not in wmaker mode */
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

  /* in wmaker mode we're a fixed size */
  if (wmaker) return;
  
  /* find the proper width and height */
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


void event_loop()
{
  XEvent e;
  Window cover;
  GSList *it;
  
  while (!exit_app) {
    while (XPending(display)) {
      XNextEvent(display, &e);

      switch (e.type)
      {
      case PropertyNotify:
        /* systray window list has changed? */
        if (e.xproperty.atom == kde_systray_prop) {
          XSelectInput(display, win, NoEventMask);
          kde_update_icons();
          XSelectInput(display, win, StructureNotifyMask);

          while (XCheckTypedEvent(display, PropertyNotify, &e));
        }

        break;

      case ConfigureNotify:
        if (e.xany.window != win) {
          /* find the icon it pertains to and beat it into submission */
          GSList *it;
  
          for (it = icons; it != NULL; it = g_slist_next(it)) {
            TrayWindow *traywin = it->data;
            if (traywin->id == e.xany.window) {
              XMoveResizeWindow(display, traywin->id, traywin->x, traywin->y,
                                icon_size, icon_size);
              break;
            }
          }
          break;
        }
        
        /* briefly cover the entire containing window, which causes it and
           all of the icons to refresh their windows. finally, they update
           themselves when the background of the main window's parent changes.
        */
        cover = XCreateSimpleWindow(display, win, 0, 0,
                                    border * 2 + width, border * 2 + height,
                                    0, 0, 0);
        XMapWindow(display, cover);
        XDestroyWindow(display, cover);
        
        break;

      case ReparentNotify:
        if (e.xany.window == win) /* reparented to us */
          break;
      case UnmapNotify:
      case DestroyNotify:
        for (it = icons; it; it = g_slist_next(it)) {
          if (((TrayWindow*)it->data)->id == e.xany.window) {
            icon_remove(it);
            break;
          }
        }
        break;

      case ClientMessage:
        if (e.xclient.message_type == net_opcode_atom &&
            e.xclient.format == 32 &&
            e.xclient.window == net_sel_win)
          net_message(&e.xclient);
        
      default:
        break;
      }
    }
    usleep(500000);
  }

  /* remove/unparent all the icons */
  while (icons) {
    /* do the remove here explicitly, cuz the event handler isn't going to
       happen anymore. */
    icon_remove(icons);
  }
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
