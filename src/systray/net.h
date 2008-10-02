#ifndef __net_h
#define __net_h

#include <glib.h>
#include <X11/Xlib.h>
#include "docker.h"

extern Window net_sel_win;
extern Atom net_opcode_atom;

void net_init();
void net_message(XClientMessageEvent *e);
void net_icon_remove(TrayWindow *traywin);

#endif /* __net_h */
