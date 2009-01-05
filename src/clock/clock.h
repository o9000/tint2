/**************************************************************************
* clock : 
* - draw clock, adjust width
*
**************************************************************************/

#ifndef CLOCK_H
#define CLOCK_H

#include <sys/time.h>
#include "common.h"
#include "area.h"


typedef struct Clock {
   // always start with area
   Area area;

   config_color font;
   PangoFontDescription *time1_font_desc;
   PangoFontDescription *time2_font_desc;
   int time1_posy;
   int time2_posy;
   char *time1_format;
   char *time2_format;

   struct timeval clock;
   int  time_precision;
} Clock;


// initialize clock : y position, precision, ...
void init_clock(Clock *clock, int panel_height);

void draw_foreground_clock (void *obj, cairo_t *c);


#endif
