#ifndef __icons_h
#define __icons_h

#include <glib.h>
#include <X11/Xlib.h>
#include "docker.h"

extern gboolean error;

gboolean icon_add(Window id, TrayWindowType type);
void icon_remove(GSList *node);

#endif /* __icons_h */
