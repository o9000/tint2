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


// --------------------------------------------------
// global taskbar parameter
Area g_systraybar;


// return 1 if task_width changed
int resize_systray (Systraybar *sysbar);



#endif

