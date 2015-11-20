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
#include "timer.h"
#include <X11/extensions/Xdamage.h>

// XEMBED messages
#define XEMBED_EMBEDDED_NOTIFY 0
// Flags for _XEMBED_INFO
#define XEMBED_MAPPED (1 << 0)

typedef enum SystraySortMethod {
	SYSTRAY_SORT_ASCENDING = 0,
	SYSTRAY_SORT_DESCENDING,
	SYSTRAY_SORT_LEFT2RIGHT,
	SYSTRAY_SORT_RIGHT2LEFT,
} SystraySortMethod;

typedef struct {
	// always start with area
	Area area;

	GSList *list_icons;
	SystraySortMethod sort;
	int alpha, saturation, brightness;
	int icon_size, icons_per_column, icons_per_row, margin;
} Systraybar;

typedef struct {
	Window parent;
	Window win;
	int x, y;
	int width, height;
	// TODO: manage icon's show/hide
	gboolean hide;
	int depth;
	Damage damage;
	timeout *render_timeout;
	gboolean empty;
	int pid;
	int chrono;
	struct timespec time_last_render;
	int num_fast_renders;
	gboolean reparented;
	gboolean embedded;
	int bad_size_counter;
	timeout *resize_timeout;
	struct timespec time_last_resize;
	char *name;
	Imlib_Image image;
} TrayWindow;

// net_sel_win != None when protocol started
extern Window net_sel_win;
extern Systraybar systray;
extern gboolean refresh_systray;
extern gboolean systray_enabled;
extern int systray_max_icon_size;
extern int systray_monitor;
extern gboolean systray_profile;

// default global data
void default_systray();

// freed memory
void cleanup_systray();

// initialize protocol and panel position
void init_systray();
void init_systray_panel(void *p);

void draw_systray(void *obj, cairo_t *c);
gboolean resize_systray(void *obj);
void on_change_systray(void *obj);
gboolean systray_on_monitor(int i_monitor, int num_panelss);

// systray protocol
// many tray icon doesn't manage stop/restart of the systray manager
void start_net();
void stop_net();
void net_message(XClientMessageEvent *e);

gboolean add_icon(Window id);
gboolean reparent_icon(TrayWindow *traywin);
gboolean embed_icon(TrayWindow *traywin);
void remove_icon(TrayWindow *traywin);

void refresh_systray_icons();
void systray_render_icon(void *t);
gboolean request_embed_icon(TrayWindow *traywin);
void systray_resize_request_event(TrayWindow *traywin, XEvent *e);
gboolean request_embed_icon(TrayWindow *traywin);
void systray_reconfigure_event(TrayWindow *traywin, XEvent *e);
void systray_destroy_event(TrayWindow *traywin);
void kde_update_icons();

#endif
