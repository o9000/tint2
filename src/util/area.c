/**************************************************************************
*
* Tint2 : area
*
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "window.h"
#include "server.h"
#include "area.h"


void refresh (Area *a)
{
   if (a->redraw) {
      //printf("draw pix\n");
      draw(a, 0);
      if (a->use_active)
	      draw(a, 1);
	   a->redraw = 0;
      //printf("end draw pix\n");
	}

   Pixmap *pmap = (a->is_active == 0) ? (&a->pix.pmap) : (&a->pix_active.pmap);

	// draw current Area
   XCopyArea (server.dsp, *pmap, server.pmap, server.gc, 0, 0, a->width, a->height, a->posx, a->posy);

   // and then refresh child object
   GSList *l = a->list;
   for (; l ; l = l->next)
      refresh(l->data);
}


void set_redraw (Area *a)
{
   a->redraw = 1;

   GSList *l;
   for (l = a->list ; l ; l = l->next)
      set_redraw(l->data);
}


void draw (Area *a, int active)
{
   Pixmap *pmap = (active == 0) ? (&a->pix.pmap) : (&a->pix_active.pmap);

   //printf("begin draw area\n");
   if (*pmap) XFreePixmap (server.dsp, *pmap);
   *pmap = server_create_pixmap (a->width, a->height);

   // add layer of root pixmap
   XCopyArea (server.dsp, server.pmap, *pmap, server.gc, a->posx, a->posy, a->width, a->height, 0, 0);

   cairo_surface_t *cs;
   cairo_t *c;

   cs = cairo_xlib_surface_create (server.dsp, *pmap, server.visual, a->width, a->height);
   c = cairo_create (cs);

   draw_background (a, c, active);

   if (a->draw_foreground)
      a->draw_foreground(a, c, active);

   cairo_destroy (c);
   cairo_surface_destroy (cs);
}


void draw_background (Area *a, cairo_t *c, int active)
{
   Pmap *pix = (active == 0) ? (&a->pix) : (&a->pix_active);

   if (pix->back.alpha > 0.0) {
      //printf("    draw_background (%d %d) RGBA (%lf, %lf, %lf, %lf)\n", a->posx, a->posy, pix->back.color[0], pix->back.color[1], pix->back.color[2], pix->back.alpha);
      draw_rect(c, pix->border.width, pix->border.width, a->width-(2.0 * pix->border.width), a->height-(2.0*pix->border.width), pix->border.rounded - pix->border.width/1.571);
      cairo_set_source_rgba(c, pix->back.color[0], pix->back.color[1], pix->back.color[2], pix->back.alpha);

      cairo_fill(c);
   }

   if (pix->border.width > 0 && pix->border.alpha > 0.0) {
      cairo_set_line_width (c, pix->border.width);

      // draw border inside (x, y, width, height)
      draw_rect(c, pix->border.width/2.0, pix->border.width/2.0, a->width - pix->border.width, a->height - pix->border.width, pix->border.rounded);
      /*
      // convert : radian = degre * M_PI/180
      // définir le dégradé dans un carré de (0,0) (100,100)
      // ensuite ce dégradé est extrapolé selon le ratio width/height
      // dans repère (0, 0) (100, 100)
      double X0, Y0, X1, Y1, degre;
      // x = X * (a->width / 100), y = Y * (a->height / 100)
      double x0, y0, x1, y1;
      X0 = 0;
      Y0 = 100;
      X1 = 100;
      Y1 = 0;
      degre = 45;
      // et ensuite faire la changement d'unité du repère
      // car ce qui doit resté inchangée est les traits et pas la direction

      // il faut d'abord appliquer une rotation de 90° (et -180° si l'angle est supérieur à 180°)
      // ceci peut être appliqué une fois pour toute au départ
      // ensuite calculer l'angle dans le nouveau repère
      // puis faire une rotation de 90°
      x0 = X0 * ((double)a->width / 100);
      x1 = X1 * ((double)a->width / 100);
      y0 = Y0 * ((double)a->height / 100);
      y1 = Y1 * ((double)a->height / 100);

      x0 = X0 * ((double)a->height / 100);
      x1 = X1 * ((double)a->height / 100);
      y0 = Y0 * ((double)a->width / 100);
      y1 = Y1 * ((double)a->width / 100);
      printf("repère (%d, %d)  points (%lf, %lf) (%lf, %lf)\n", a->width, a->height, x0, y0, x1, y1);

      cairo_pattern_t *linpat;
      linpat = cairo_pattern_create_linear (x0, y0, x1, y1);
      cairo_pattern_add_color_stop_rgba (linpat, 0, a->border.color[0], a->border.color[1], a->border.color[2], a->border.alpha);
      cairo_pattern_add_color_stop_rgba (linpat, 1, a->border.color[0], a->border.color[1], a->border.color[2], 0);
      cairo_set_source (c, linpat);
      */
      cairo_set_source_rgba (c, pix->border.color[0], pix->border.color[1], pix->border.color[2], pix->border.alpha);

      cairo_stroke (c);
      //cairo_pattern_destroy (linpat);
   }
}


void remove_area (Area *a)
{
   Area *parent;

   parent = (Area*)a->parent;
   parent->list = g_slist_remove(parent->list, a);
   set_redraw (parent);

}


void add_area (Area *a)
{
   Area *parent;

   parent = (Area*)a->parent;
   parent->list = g_slist_remove(parent->list, a);
   set_redraw (parent);

}


void free_area (Area *a)
{
   GSList *l0;
   for (l0 = a->list; l0 ; l0 = l0->next)
      free_area (l0->data);

   if (a->list) {
      g_slist_free(a->list);
      a->list = 0;
   }
}

