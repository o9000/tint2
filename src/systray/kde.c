#include "kde.h"
#include "icons.h"
#include "docker.h"
#include "xproperty.h"
#include <assert.h>
#include <X11/Xatom.h>

Atom kde_systray_prop = None;

void kde_init()
{
  kde_systray_prop = XInternAtom(display,
                                 "_KDE_NET_SYSTEM_TRAY_WINDOWS", False);
  assert(kde_systray_prop);

  XSelectInput(display, root, PropertyChangeMask);
  kde_update_icons();
}

void kde_update_icons()
{
  gulong count = (unsigned) -1; /* grab as many as possible */
  Window *ids;
  unsigned int i;
  GSList *it, *next;
  gboolean removed = FALSE; /* were any removed? */

  if (! xprop_get32(root, kde_systray_prop, XA_WINDOW, sizeof(Window)*8,
                    &count, &ids))
    return;

  /* add new windows to our list */
  for (i = 0; i < count; ++i) {
    for (it = icons; it != NULL; it = g_slist_next(it)) {
      TrayWindow *traywin = it->data;
      if (traywin->id == ids[i])
        break;
    }
    if (!it)
      icon_add(ids[i], KDE);
  }

  /* remove windows from our list that no longer exist in the property */
  for (it = icons; it != NULL;) {
    TrayWindow *traywin = it->data;
    gboolean exists;

    if (traywin->type != KDE) {
      /* don't go removing non-kde windows */
      exists = TRUE;
    } else {
      exists = FALSE;
      for (i = 0; i < count; ++i) {
        if (traywin->id == ids[i]) {
          exists = TRUE;
          break;
        }
      }
    }
    
    next = g_slist_next(it);
    if (!exists) {
      icon_remove(it);
      removed =TRUE;
    }
    it = next;
  }

  if (removed) {
    /* at least one tray app was removed, so reorganize 'em all and resize*/
    reposition_icons();
    fix_geometry();
  }

  XFree(ids);
}
