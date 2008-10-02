/**************************************************************************
* base class for all objects (panel, taskbar, task, systray, clock, ...).
* each object 'inherit' Area and implement draw_foreground if needed.
* 
* Area is at the begining of each object so &object == &area.
* 
* une zone comprend :
* - fond : couleur / opacité
* - contenu
* - largeur / hauteur
* - paddingx / paddingy
* - pixmap mémorisant l'affichage (évite de redessiner l'objet à chaque rafraichissement)
* - une liste de sous objets
*
* un objet comprend les actions:
* 1) redraw(obj)
*    force l'indicateur 'redraw' sur l'objet
*    parcoure la liste des sous objets => redraw(obj)
* 2) draw(obj)
*    dessine le background, dessine le contenu dans pmap
*    parcoure la liste des sous objets => draw(obj)
*    le pmap de l'objet se base sur le pmap de l'objet parent (cumul des couches)
* 3) draw_background(obj)
*    dessine le fond dans pmap
* 4) draw_foreground(obj) = 0 : fonction virtuelle à redéfinir
*    dessine le contenu dans pmap
*    si l'objet n'a pas de contenu, la fonction est nulle
* 5) resize_width(obj, width) = 0 : fonction virtuelle à redéfinir
*    recalcule la largeur de l'objet (car la hauteur est fixe)
*    - taille systray calculée à partir de la liste des icones
*    - taille clock calculée à partir de l'heure
*    - taille d'une tache calculée à partir de la taskbar (ajout, suppression, taille)
*    - taille d'une taskbar calculée à partir de la taille du panel et des autres objets
* 6) voir refresh(obj)
* 
* Implémentation :
* - tous les éléments du panel possèdent 1 objet en début de structure
*   panel, taskbar, systray, task, ...
* - l'objet est en fait une zone (area). 
*   l'imbrication des sous objet doit permettre de gérer le layout.
* - on a une relation 1<->1 entre un objet et une zone graphique
*   les taskbar affichent toutes les taches.
*   donc on utilise la liste des objets pour gérer la liste des taches.
* - les taches ont 2 objets : l'un pour la tache inactive et l'autre pour la tache active
*   draw(obj) est appellé sur le premier objet automatiquement 
*   et draw_foreground(obj) lance l'affichage du 2 ieme objet
*   ainsi la taskbar gère bien une liste d'objets mais draw(obj) dessine les 2 objets
* - les fonctions de refresh et de draw sont totalement dissociées
* 
* ----------------------------------------------------
* A évaluer :
* 1. voir comment définir et gérer le panel_layout avec les objets
*    => peut on s'affranchir des données spécifiques à chaque objet ?
*    => comment gérer l'affichage du layout ?
*    => comment configurer le layout ?
*    => voir le cumul des couches et l'imbrication entre objet et parent ?
* 2. voir la fonction de refresh des objets ??
*    surtout le refresh des taches qui est différent pour la tache active
* 
* 3. tester l'implémentation et évaluer les autres abstractions possibles ?
* 
* 4. comment gérer le groupage des taches
* 5. la clock est le contenu du panel. mais elle ne tiens pas compte du padding vertical ?
*    c'est ok pour la clock. voir l'impact sur paddingx ?
* 
* voir resize_taskbar(), resize_clock() et resize_tasks()
* voir les taches actives et inactives ?? une seule tache est active !
* variable widthChanged ou bien emission d'un signal ???
*
* 6) config(obj) configure un objet (définie les positions verticales)
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
   // need redraw Pixmap
   int redraw;
   
   int paddingx, paddingy;   
   int width, height;
   Pixmap pmap;
   
   Color back;
   Border border;
   
   // absolute coordinate in panel
   int posx, posy;
   // parent Area
   void *parent;

   // pointer to function
   // draw_foreground : return 1 if width changed, return O otherwise
   int (*draw_foreground)(void *obj, cairo_t *c);
   void (*add_child)(void *obj);
   int (*remove_child)(void *obj);
   
   // list of child
   GSList *list;
} Area;


// redraw an area and childs
void redraw (Area *a);

// draw background and foreground
// return 1 if width changed, return O otherwise
int  draw (Area *a);
void draw_background (Area *a, cairo_t *c);

void refresh (Area *a);
void remove_area (Area *a);
void add_area (Area *a);

#endif

