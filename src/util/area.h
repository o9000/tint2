/**************************************************************************
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
* 
* base class for all graphical objects (panel, taskbar, task, systray, clock, ...).
* Area is at the begining of each graphical object so &object == &area.
* 
* Area manage the background and border drawing, size and padding.
* Area also manage the tree of visible objects
*   panel -> taskbars -> tasks
*         -> systray -> icons
*         -> clock
* 
* draw_foreground(obj) and draw(obj) are virtual function.
* 
* resize_width(obj, width) = 0 : fonction virtuelle à redéfinir
*    recalcule la largeur de l'objet (car la hauteur est fixe)
*    - taille systray calculée à partir de la liste des icones
*    - taille clock calculée à partir de l'heure
*    - taille d'une tache calculée à partir de la taskbar (ajout, suppression, taille)
*    - taille d'une taskbar calculée à partir de la taille du panel et des autres objets
* 
* voir resize_taskbar(), resize_clock() et resize_tasks()
* variable widthChanged ou bien emission d'un signal ???
* voir config(obj) configure un objet (définie les positions verticales)
*
**************************************************************************/

#ifndef AREA_H
#define AREA_H

#include <X11/Xlib.h>
#include <pango/pangocairo.h>

#include "common.h"



typedef struct
{
   double color[3];
   double alpha;
   int width;
   int rounded;
} Border;


typedef struct
{
   double color[3];
   double alpha;
} Color;


typedef struct {
   // TODO: isoler 'draw' de 'refresh'
   // TODO: isoler les données locales des données communes aux freres
   // absolute coordinate in panel
   int posx, posy;
   int width, height;
   Pixmap pmap;

   // list of child : Area object
   GSList *list;
   
   // need redraw Pixmap
   int redraw;   
   int paddingx, paddingy;   
   // parent Area
   void *parent;
   
   Color back;
   Border border;
   
   // each object can overwrite following function
   void (*draw)(void *obj);
   void (*draw_foreground)(void *obj, cairo_t *c);
   void (*add_child)(void *obj);
   int (*remove_child)(void *obj);   
} Area;



// draw background and foreground
void refresh (Area *a);

// set 'redraw' on an area and childs
void set_redraw (Area *a);
void draw (Area *a);
void draw_background (Area *a, cairo_t *c);

void remove_area (Area *a);
void add_area (Area *a);
void free_area (Area *a);

#endif

