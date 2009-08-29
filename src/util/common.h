/**************************************************************************
* Common declarations
*
**************************************************************************/

#ifndef COMMON_H
#define COMMON_H


#define WM_CLASS_TINT   "panel"

#include "area.h"

/*
void fxfree(void** ptr){
  if(*ptr){
    free(*ptr);
    *ptr=NULL;
    }
  }
FXint fxmalloc(void** ptr,unsigned long size){
  *ptr=NULL;
  if(size!=0){
    if((*ptr=malloc(size))==NULL) return FALSE;
    }
  return TRUE;
  }
*/

// mouse actions
enum { NONE=0, CLOSE, TOGGLE, ICONIFY, SHADE, TOGGLE_ICONIFY, MAXIMIZE_RESTORE, MAXIMIZE, RESTORE, DESKTOP_LEFT, DESKTOP_RIGHT };

#define ALLDESKTOP  0xFFFFFFFF


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


// adjust HSB on an ARGB icon
void adjust_hsb(unsigned int *data, int w, int h, float hue, float satur, float bright);



#endif

