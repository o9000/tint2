/**************************************************************************
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
*
* base class for all graphical objects (panel, taskbar, task, systray, clock, ...).
* Area is at the begining of each object (&object == &area).
*
* Area manage the background and border drawing, size and padding.
* Each Area have 2 Pixmap (pix and pix_active).
*
* Area also manage the tree of visible objects. Parent object drawn before child object.
*   panel -> taskbars -> tasks
*         -> systray -> icons
*         -> clock
*
* draw_foreground(obj) is virtual function.
*
* TODO :
* resize_width(obj, width) = 0 : fonction virtuelle à redéfinir
*    recalcule la largeur de l'objet (car la hauteur est fixe)
*    - taille systray calculée à partir de la liste des icones
*    - taille clock calculée à partir de l'heure
*    - taille d'une tache calculée à partir de la taskbar (ajout, suppression, taille)
*    - taille d'une taskbar calculée à partir de la taille du panel et des autres objets
*
* voir resize_taskbar(), resize_clock() et resize_tasks()
* voir config(obj) configure un objet (définie les positions verticales)
*
**************************************************************************/

#ifndef AREA_H
#define AREA_H

#include <glib.h>
#include <X11/Xlib.h>
#include <cairo.h>
#include <cairo-xlib.h>


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


typedef struct
{
   Pixmap pmap;
   Color back;
   Border border;
} Pmap;


// TODO: isoler 'draw' de 'refresh'
// TODO: isoler les données locales des données communes aux freres
typedef struct {
   // absolute coordinate in panel
   int posx, posy;
   int width, height;
   Pmap pix;
   Pmap pix_active;

   // list of child : Area object
   GSList *list;

   // need redraw Pixmap
   int redraw;
   int use_active, is_active;
   int paddingx, paddingy;
   // parent Area
   void *parent;

   // each object can overwrite following function
   void (*draw_foreground)(void *obj, cairo_t *c, int active);
   void (*add_child)(void *obj);
   int (*remove_child)(void *obj);
} Area;



// draw background and foreground
void refresh (Area *a);

// set 'redraw' on an area and childs
void set_redraw (Area *a);

// draw pixmap and pixmap_active
void draw (Area *a, int active);
void draw_background (Area *a, cairo_t *c, int active);

void remove_area (Area *a);
void add_area (Area *a);
void free_area (Area *a);

// draw rounded rectangle
void draw_rect(cairo_t *c, double x, double y, double w, double h, double r);

#endif

