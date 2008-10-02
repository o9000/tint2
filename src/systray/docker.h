#ifndef __docker_h
#define __docker_h

#include <glib.h>
#include <X11/Xlib.h>

extern Display *display;
extern Window root, win;
extern GSList *icons;
extern int width, height;
extern int border;
extern gboolean horizontal;
extern int icon_size;
extern gboolean wmaker;

typedef enum {
  KDE = 1, /* kde specific */
  NET      /* follows the standard (freedesktop.org) */
} TrayWindowType;

typedef struct
{
  TrayWindowType type;
  Window id;
  int x, y;
} TrayWindow;

void reposition_icons();
void fix_geometry();

#endif /* __docker_h */
