/**************************************************************************
* Common declarations
* 
**************************************************************************/

#ifndef COMMON_H
#define COMMON_H


#define WM_CLASS_TINT   "panel"

#include "area.h"

// taskbar table : convert 2 dimension in 1 dimension
#define index(i, j) ((i * panel.nb_monitor) + j)

// mouse actions
enum { NONE=0, CLOSE, TOGGLE, ICONIFY, SHADE, TOGGLE_ICONIFY };



typedef struct config_border
{
   double color[3];
   double alpha;
   int width;
   int rounded;
} config_border;


typedef struct config_color
{
   double color[3];
   double alpha;
} config_color;




#endif

