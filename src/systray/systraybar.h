/**************************************************************************
* Copyright (C) 2009 thierry lorthiois (lorthiois@bbsoft.fr)
*
* systraybar
*
**************************************************************************/

#ifndef SYSTRAYBAR_H
#define SYSTRAYBAR_H

#include "area.h"


typedef struct {
   // always start with area
   Area area;

} Systraybar;


typedef struct
{
   Window id;
   int x, y;

   Window win;
   long *icon_data;
   int icon_width;
   int icon_height;
} TrayWindow;


void init_systray();
void cleanup_systray();
int  net_init();

// return 1 if task_width changed
int resize_systray (Systraybar *sysbar);



#endif

