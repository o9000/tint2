#ifndef __kde_h
#define __kde_h

#include <glib.h>
#include <X11/Xlib.h>

extern Atom kde_systray_prop;

void kde_update_icons();
void kde_init();

#endif /* __kde_h */
