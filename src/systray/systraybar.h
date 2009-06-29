/**************************************************************************
* Copyright (C) 2009 thierry lorthiois (lorthiois@bbsoft.fr)
*
* systraybar
* systray implementation come from 'docker-1.5' by Ben Jansens,
* and from systray/xembed specification (freedesktop.org).
*
**************************************************************************/

#ifndef SYSTRAYBAR_H
#define SYSTRAYBAR_H

#include "common.h"
#include "area.h"

// XEMBED messages
#define XEMBED_EMBEDDED_NOTIFY		0
// Flags for _XEMBED_INFO
#define XEMBED_MAPPED		(1 << 0)


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
} TrayWindow;


extern Window net_sel_win;
extern Systraybar systray;
extern int refresh_systray;


void init_systray();
void cleanup_systray();
void draw_systray(void *obj, cairo_t *c, int active);
void resize_systray(void *obj);


// systray protocol
int init_net();
void cleanup_net();
void net_message(XClientMessageEvent *e);

gboolean add_icon(Window id);
void remove_icon(TrayWindow *traywin);

void refresh_systray_icon();

void kde_update_icons();

#endif

