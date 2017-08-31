/**************************************************************************
 * Copyright (C) 2017 tint2 authors
 *
 **************************************************************************/

#ifndef DRAG_AND_DROP_H
#define DRAG_AND_DROP_H

#include <X11/Xlib.h>
#include <glib.h>

extern gboolean hidden_panel_shown_for_dnd;

void dnd_init();

void handle_dnd_enter(XClientMessageEvent *e);
void handle_dnd_position(XClientMessageEvent *e);
void handle_dnd_drop(XClientMessageEvent *e);
void handle_dnd_selection_notify(XSelectionEvent *e);

#endif
