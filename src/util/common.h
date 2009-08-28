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
enum { NONE=0, CLOSE, TOGGLE, ICONIFY, SHADE, TOGGLE_ICONIFY, MAXIMIZE_RESTORE, MAXIMIZE, RESTORE };

#define ALLDESKTOP  0xFFFFFFFF

#define MIN3(x,y,z)  ((y) <= (z) ? ((x) <= (y) ? (x) : (y)) : ((x) <= (z) ? (x) : (z)))
#define MAX3(x,y,z)  ((y) >= (z) ? ((x) >= (y) ? (x) : (y)) : ((x) >= (z) ? (x) : (z)))


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

