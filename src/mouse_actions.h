#ifndef MOUSE_ACTIONS_H
#define MOUSE_ACTIONS_H

#include "panel.h"

gboolean tint2_handles_click(Panel *panel, XButtonEvent *e);

void handle_mouse_press_event(XEvent *e);
void handle_mouse_move_event(XEvent *e);
void handle_mouse_release_event(XEvent *e);

#endif
