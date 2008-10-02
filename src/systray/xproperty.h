#ifndef __xproperty_h
#define __xproperty_h

#include <glib.h>
#include <X11/Xlib.h>

/* if the func returns TRUE, the returned value must be XFree()'d */
gboolean xprop_get8(Window window, Atom atom, Atom type, int size,
                    gulong *count, guchar **value);
gboolean xprop_get32(Window window, Atom atom, Atom type, int size,
                     gulong *count, gulong **value);

#endif /* __xproperty_h */
