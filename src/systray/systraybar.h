/**************************************************************************
* Copyright (C) 2009 thierry lorthiois (lorthiois@bbsoft.fr)
*
* systraybar
* based on 'docker-1.5' from Ben Jansens
*
**************************************************************************/

#ifndef SYSTRAYBAR_H
#define SYSTRAYBAR_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include "area.h"


typedef struct {
   // always start with area
   Area area;

	GSList *list_icons;
} Systraybar;


typedef struct
{
   Window id;
   int x, y;
   int width, height;

   long *icon_data;
} TrayWindow;


extern Window net_sel_win;
extern Systraybar systray;


void init_systray();
void cleanup_systray();
int net_init();
void net_message(XClientMessageEvent *e);
void icon_remove(TrayWindow *traywin);

void resize_systray (void *obj);


#endif

