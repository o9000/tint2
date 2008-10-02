#include "icons.h"
#include "net.h"
#include <assert.h>
#include <stdlib.h>

gboolean error;
int window_error_handler(Display *d, XErrorEvent *e)
{
  d=d;e=e;
  if (e->error_code == BadWindow) {
    error = TRUE;
  } else {
    g_printerr("X ERROR NOT BAD WINDOW!\n");
    abort();
  }
  return 0;
}


gboolean icon_swallow(TrayWindow *traywin)
{
  XErrorHandler old;

  error = FALSE;
  old = XSetErrorHandler(window_error_handler);
  XReparentWindow(display, traywin->id, win, 0, 0);
  XSync(display, False);
  XSetErrorHandler(old);

  return !error;
}


/*
  The traywin must have its id and type set.
*/
gboolean icon_add(Window id, TrayWindowType type)
{
  TrayWindow *traywin;

  assert(id);
  assert(type);

  if (wmaker) {
    /* do we have room in our window for another icon? */
    unsigned int max = (width / icon_size) * (height / icon_size);
    if (g_slist_length(icons) >= max)
      return FALSE; /* no room, sorry! REJECTED! */
  }

  traywin = g_new0(TrayWindow, 1);
  traywin->type = type;
  traywin->id = id;

  if (!icon_swallow(traywin)) {
    g_free(traywin);
    return FALSE;
  }

  /* find the positon for the systray app window */
  if (!wmaker) {
    traywin->x = border + (horizontal ? width : 0);
    traywin->y = border + (horizontal ? 0 : height);
  } else {
    int count = g_slist_length(icons);
    traywin->x = border + ((width % icon_size) / 2) +
      (count % (width / icon_size)) * icon_size;
    traywin->y = border + ((height % icon_size) / 2) +
      (count / (height / icon_size)) * icon_size;
  }

  /* add the new icon to the list */
  icons = g_slist_append(icons, traywin);

  /* watch for the icon trying to resize itself! BAD ICON! BAD! */
  XSelectInput(display, traywin->id, StructureNotifyMask);

  /* position and size the icon window */
  XMoveResizeWindow(display, traywin->id,
                    traywin->x, traywin->y, icon_size, icon_size);
  
  /* resize our window so that the new window can fit in it */
  fix_geometry();

  /* flush before clearing, otherwise the clear isn't effective. */
  XFlush(display);
  /* make sure the new child will get the right stuff in its background
     for ParentRelative. */
  XClearWindow(display, win);
  
  /* show the window */
  XMapRaised(display, traywin->id);

  return TRUE;
}


void icon_remove(GSList *node)
{
  XErrorHandler old;
  TrayWindow *traywin = node->data;
  Window traywin_id = traywin->id;

  if (traywin->type == NET)
    net_icon_remove(traywin);

  XSelectInput(display, traywin->id, NoEventMask);
  
  /* remove it from our list */
  g_free(node->data);
  icons = g_slist_remove_link(icons, node);

  /* reparent it to root */
  error = FALSE;
  old = XSetErrorHandler(window_error_handler);
  XReparentWindow(display, traywin_id, root, 0, 0);
  XSync(display, False);
  XSetErrorHandler(old);

  reposition_icons();
  fix_geometry();
}
