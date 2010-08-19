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
#include <X11/extensions/Xrender.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pango/pangocairo.h>

#include "area.h"
#include "server.h"
#include "panel.h"


/************************************************************
 * !!! This design is experimental and not yet fully implemented !!!!!!!!!!!!!
 * 
 * AREA :
 * Areas in tint2 are similar to widgets in a GUI. 
 * Graphical objects (panel, taskbar, task, systray, clock, ...) in tint2 'inherit' an Area class.
 * Area is an abstract class of objects. It's at the begining of each object (&object == &area).
 * Area manage the background and border drawing, size and padding.
 * 
 * DATA ORGANISATION :
 * tint2 define one panel per monitor. And each panel have a tree of Area (nodes).
 * The root of the tree is Panel.Area. And task, clock, systray, taskbar,... are nodes.
 * 
 * 'panel_items' parameter (in config) define the list and the order of nodes in tree's panel.
 * 'panel_items = SC' define a panel with just Systray and Clock.
 * So the root Panel.Area will have 2 childs (Systray and Clock).
 *
 * The tree allow to browse panel's objects from background to foreground and from left to right.
 * The position of each node/Area depend on parent's position and brothers on the left.
 * 
 * DRAWING EVENT :
 * In the end, redrawing an object (like the clock) could come from an external event (date change) 
 * or from a layering event (size or position change).
 * 
 * DRAWING LOOP :
 * 1) browse tree and resize SIZE_BY_CONTENT node
 * 	- children node are resized before its parent
 * 	- if 'size' changed then 'redraw = 1' and 'resize = 1' on the parent
 * 2) browse tree and resize SIZE_BY_LAYOUT node
 * 	- parent node is resized before its children
 * 	- if 'size' changed then 'redraw = 1' and 'resize = 1' on childs with SIZE_BY_LAYOUT
 * 3) calculate posx of objects
 * 	- parent's position is calculated before children's position
 * 	- if 'position' changed then 'redraw = 1'
 * 4) redraw needed objects
 * 	- parent node is drawn before its children
 * 
 * perhaps 2) and 3) can be merged...
 * répartition entre niveau global et niveau local ??
 *   size_by_content peut-il modifier redraw=1 en cas de changement ? ou est ce géré par chaque composant ?
 *   size_by_layout peut-il modifier redraw ?
 *   
 ************************************************************/


void refresh (Area *a)
{
	// don't draw and resize hide objects
	if (!a->on_screen) return;

	// don't draw transparent objects (without foreground and without background)
	if (a->redraw) {
		a->redraw = 0;
		// force redraw of child
		GSList *l;
		for (l = a->list ; l ; l = l->next)
			set_redraw(l->data);

		//printf("draw area posx %d, width %d\n", a->posx, a->width);
		draw(a);
	}

	// draw current Area
	if (a->pix == 0) printf("empty area posx %d, width %d\n", a->posx, a->width);
	XCopyArea (server.dsp, a->pix, ((Panel *)a->panel)->temp_pmap, server.gc, 0, 0, a->width, a->height, a->posx, a->posy);

	// and then refresh child object
	GSList *l;
	for (l = a->list; l ; l = l->next)
		refresh(l->data);
}


void size_by_content (Area *a)
{
	// don't draw and resize hide objects
	if (!a->on_screen) return;

	// children node are resized before its parent
	GSList *l;
	for (l = a->list; l ; l = l->next)
		size_by_content(l->data);
	
	// calculate current area's size
	if (a->resize && a->size_mode == SIZE_BY_CONTENT) {
		a->resize = 0;

		// if 'size' changed then 'resize = 1' on the parent
		if (a->_resize) {
			a->_resize(a);
			a->redraw = 1;
			((Area*)a->parent)->resize = 1;
		}
	}
}


void size_by_layout (Area *a)
{
	// don't draw and resize hide objects
	if (!a->on_screen) return;

	// parent node is resized before its children
	// calculate current area's size
	GSList *l;
	if (a->resize && a->size_mode == SIZE_BY_LAYOUT) {
		a->resize = 0;

		// if 'size' changed then 'resize = 1' on childs with SIZE_BY_LAYOUT
		if (a->_resize) {
			a->_resize(a);
			for (l = a->list; l ; l = l->next) {
				if (((Area*)l->data)->size_mode == SIZE_BY_LAYOUT)
					((Area*)l->data)->resize = 1;
			}
		}
	}

	for (l = a->list; l ; l = l->next)
		size_by_layout(l->data);	
}


void set_redraw (Area *a)
{
	a->redraw = 1;

	GSList *l;
	for (l = a->list ; l ; l = l->next)
		set_redraw(l->data);
}


void draw (Area *a)
{
	if (a->pix) XFreePixmap (server.dsp, a->pix);
	a->pix = XCreatePixmap (server.dsp, server.root_win, a->width, a->height, server.depth);

	// add layer of root pixmap (or clear pixmap if real_transparency==true)
	if (server.real_transparency)
		clear_pixmap(a->pix, 0 ,0, a->width, a->height);
	XCopyArea (server.dsp, ((Panel *)a->panel)->temp_pmap, a->pix, server.gc, a->posx, a->posy, a->width, a->height, 0, 0);

	cairo_surface_t *cs;
	cairo_t *c;

	cs = cairo_xlib_surface_create (server.dsp, a->pix, server.visual, a->width, a->height);
	c = cairo_create (cs);

	draw_background (a, c);

	if (a->_draw_foreground)
		a->_draw_foreground(a, c);

	cairo_destroy (c);
	cairo_surface_destroy (cs);
}


void draw_background (Area *a, cairo_t *c)
{
	if (a->bg->back.alpha > 0.0) {
		//printf("    draw_background (%d %d) RGBA (%lf, %lf, %lf, %lf)\n", a->posx, a->posy, pix->back.color[0], pix->back.color[1], pix->back.color[2], pix->back.alpha);
		draw_rect(c, a->bg->border.width, a->bg->border.width, a->width-(2.0 * a->bg->border.width), a->height-(2.0*a->bg->border.width), a->bg->border.rounded - a->bg->border.width/1.571);
		cairo_set_source_rgba(c, a->bg->back.color[0], a->bg->back.color[1], a->bg->back.color[2], a->bg->back.alpha);
		cairo_fill(c);
	}

	if (a->bg->border.width > 0 && a->bg->border.alpha > 0.0) {
		cairo_set_line_width (c, a->bg->border.width);

		// draw border inside (x, y, width, height)
		draw_rect(c, a->bg->border.width/2.0, a->bg->border.width/2.0, a->width - a->bg->border.width, a->height - a->bg->border.width, a->bg->border.rounded);
		/*
		// convert : radian = degre * M_PI/180
		// definir le degrade dans un carre de (0,0) (100,100)
		// ensuite ce degrade est extrapoler selon le ratio width/height
		// dans repere (0, 0) (100, 100)
		double X0, Y0, X1, Y1, degre;
		// x = X * (a->width / 100), y = Y * (a->height / 100)
		double x0, y0, x1, y1;
		X0 = 0;
		Y0 = 100;
		X1 = 100;
		Y1 = 0;
		degre = 45;
		// et ensuite faire la changement d'unite du repere
		// car ce qui doit reste inchangee est les traits et pas la direction

		// il faut d'abord appliquer une rotation de 90 (et -180 si l'angle est superieur a  180)
		// ceci peut etre applique une fois pour toute au depart
		// ensuite calculer l'angle dans le nouveau repare
		// puis faire une rotation de 90
		x0 = X0 * ((double)a->width / 100);
		x1 = X1 * ((double)a->width / 100);
		y0 = Y0 * ((double)a->height / 100);
		y1 = Y1 * ((double)a->height / 100);

		x0 = X0 * ((double)a->height / 100);
		x1 = X1 * ((double)a->height / 100);
		y0 = Y0 * ((double)a->width / 100);
		y1 = Y1 * ((double)a->width / 100);

		cairo_pattern_t *linpat;
		linpat = cairo_pattern_create_linear (x0, y0, x1, y1);
		cairo_pattern_add_color_stop_rgba (linpat, 0, a->border.color[0], a->border.color[1], a->border.color[2], a->border.alpha);
		cairo_pattern_add_color_stop_rgba (linpat, 1, a->border.color[0], a->border.color[1], a->border.color[2], 0);
		cairo_set_source (c, linpat);
		*/
		cairo_set_source_rgba (c, a->bg->border.color[0], a->bg->border.color[1], a->bg->border.color[2], a->bg->border.alpha);

		cairo_stroke (c);
		//cairo_pattern_destroy (linpat);
	}
}


void remove_area (Area *a)
{
	Area *parent = (Area*)a->parent;

	parent->list = g_slist_remove(parent->list, a);
	set_redraw (parent);

}


void add_area (Area *a)
{
	Area *parent = (Area*)a->parent;

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
	if (a->pix) {
		XFreePixmap (server.dsp, a->pix);
		a->pix = 0;
	}
}


void draw_rect(cairo_t *c, double x, double y, double w, double h, double r)
{
	if (r > 0.0) {
		double c1 = 0.55228475 * r;

		cairo_move_to(c, x+r, y);
		cairo_rel_line_to(c, w-2*r, 0);
		cairo_rel_curve_to(c, c1, 0.0, r, c1, r, r);
		cairo_rel_line_to(c, 0, h-2*r);
		cairo_rel_curve_to(c, 0.0, c1, c1-r, r, -r, r);
		cairo_rel_line_to (c, -w +2*r, 0);
		cairo_rel_curve_to (c, -c1, 0, -r, -c1, -r, -r);
		cairo_rel_line_to (c, 0, -h + 2 * r);
		cairo_rel_curve_to (c, 0, -c1, r - c1, -r, r, -r);
	}
	else
		cairo_rectangle(c, x, y, w, h);
}


void clear_pixmap(Pixmap p, int x, int y, int w, int h)
{
	Picture pict = XRenderCreatePicture(server.dsp, p, XRenderFindVisualFormat(server.dsp, server.visual), 0, 0);
	XRenderColor col = { .red=0, .green=0, .blue=0, .alpha=0 };
	XRenderFillRectangle(server.dsp, PictOpSrc, pict, &col, x, y, w, h);
	XRenderFreePicture(server.dsp, pict);
}
