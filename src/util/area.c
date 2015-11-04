/**************************************************************************
*
* Tint2 : area
*
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
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
 * DATA ORGANISATION :
 * Areas in tint2 are similar to widgets in a GUI. 
 * All graphical objects (panel, taskbar, task, systray, clock, ...) 'inherit' an abstract class 'Area'.
 * This class 'Area' manage the background, border, size, position and padding.
 * Area is at the begining of each object (&object == &area).
 * 
 * tint2 define one panel per monitor. And each panel have a tree of Area.
 * The root of the tree is Panel.Area. And task, clock, systray, taskbar,... are nodes.
 * 
 * The tree give the localisation of each object :
 * - tree's root is in the background while tree's leafe are foreground objects
 * - position of a node/Area depend on the layout : parent's position (posx, posy), size of previous brothers and parent's padding
 * - size of a node/Area depend on the content (SIZE_BY_CONTENT objects) or on the layout (SIZE_BY_LAYOUT objects) 
 * 
 * DRAWING AND LAYERING ENGINE :
 * Redrawing an object (like the clock) could come from an 'external event' (date change) 
 * or from a 'layering event' (position change).
 * The following 'drawing engine' take care of :
 * - posx/posy of all Area
 * - 'layering event' propagation between object
 * 1) browse tree SIZE_BY_CONTENT
 *  - resize SIZE_BY_CONTENT node : children are resized before parent
 * 	- if 'size' changed then 'resize = 1' on the parent
 * 2) browse tree SIZE_BY_LAYOUT and POSITION
 *  - resize SIZE_BY_LAYOUT node : parent is resized before children
 *  - calculate position (posx,posy) : parent is calculated before children
 * 	- if 'position' changed then 'redraw = 1'
 * 3) browse tree REDRAW
 *  - redraw needed objects : parent is drawn before children
 *
 * CONFIGURE PANEL'S LAYOUT :
 * 'panel_items' parameter (in config) define the list and the order of nodes in tree's panel.
 * 'panel_items = SC' define a panel with just Systray and Clock.
 * So the tree 'Panel.Area' will have 2 childs (Systray and Clock).
 *
 ************************************************************/

Area *mouse_over_area = NULL;

void init_rendering(void *obj, int pos)
{
	Area *a = (Area*)obj;
	
	// initialize fixed position/size
	GList *l;
	for (l = a->children; l ; l = l->next) {
		Area *child = ((Area*)l->data);
		if (panel_horizontal) {
			child->posy = pos + a->bg->border.width + a->paddingy;
			child->height = a->height - (2 * (a->bg->border.width + a->paddingy));
			if (child->_on_change_layout)
				child->_on_change_layout(child);
			init_rendering(child, child->posy);
		}
		else {
			child->posx = pos + a->bg->border.width + a->paddingy;
			child->width = a->width - (2 * (a->bg->border.width + a->paddingy));
			if (child->_on_change_layout)
				child->_on_change_layout(child);
			init_rendering(child, child->posx);
		}
	}
}


void rendering(void *obj)
{
	Panel *panel = (Panel*)obj;

	size_by_content(&panel->area);
	size_by_layout(&panel->area, 1);
	
	refresh(&panel->area);
}


void size_by_content (Area *a)
{
	// don't resize hiden objects
	if (!a->on_screen)
		return;

	// children node are resized before its parent
	GList *l;
	for (l = a->children; l ; l = l->next)
		size_by_content(l->data);
	
	// calculate area's size
	a->on_changed = 0;
	if (a->resize && a->size_mode == SIZE_BY_CONTENT) {
		a->resize = 0;

		if (a->_resize) {
			if (a->_resize(a)) {
				// 'size' changed => 'resize = 1' on the parent
				((Area*)a->parent)->resize = 1;
				a->on_changed = 1;
			}
		}
	}
}


void size_by_layout (Area *a, int level)
{
	// don't resize hiden objects
	if (!a->on_screen)
		return;

	// parent node is resized before its children
	// calculate area's size
	GList *l;
	if (a->resize && a->size_mode == SIZE_BY_LAYOUT) {
		a->resize = 0;

		if (a->_resize) {
			a->_resize(a);
			// resize childs with SIZE_BY_LAYOUT
			for (l = a->children; l ; l = l->next) {
				Area *child = ((Area*)l->data);
				if (child->size_mode == SIZE_BY_LAYOUT && child->children)
					child->resize = 1;
			}
		}
	}

	// update position of children
	if (a->children) {
		if (a->alignment == ALIGN_LEFT) {
			int pos = (panel_horizontal ? a->posx : a->posy) + a->bg->border.width + a->paddingxlr;

			for (l = a->children; l ; l = l->next) {
				Area *child = ((Area*)l->data);
				if (!child->on_screen)
					continue;

				if (panel_horizontal) {
					if (pos != child->posx) {
						// pos changed => redraw
						child->posx = pos;
						child->on_changed = 1;
					}
				} else {
					if (pos != child->posy) {
						// pos changed => redraw
						child->posy = pos;
						child->on_changed = 1;
					}
				}

				size_by_layout(child, level+1);

				pos += panel_horizontal ? child->width + a->paddingx : child->height + a->paddingx;
			}
		} else if (a->alignment == ALIGN_RIGHT) {
			int pos = (panel_horizontal ? a->posx + a->width : a->posy + a->height) - a->bg->border.width - a->paddingxlr;

			for (l = g_list_last(a->children); l ; l = l->prev) {
				Area *child = ((Area*)l->data);
				if (!child->on_screen)
					continue;

				pos -= panel_horizontal ? child->width : child->height;

				if (panel_horizontal) {
					if (pos != child->posx) {
						// pos changed => redraw
						child->posx = pos;
						child->on_changed = 1;
					}
				} else {
					if (pos != child->posy) {
						// pos changed => redraw
						child->posy = pos;
						child->on_changed = 1;
					}
				}

				size_by_layout(child, level+1);

				pos -= a->paddingx;
			}
		} else if (a->alignment == ALIGN_CENTER) {

			int children_size = 0;

			for (l = a->children; l ; l = l->next) {
				Area *child = ((Area*)l->data);
				if (!child->on_screen)
					continue;

				children_size += panel_horizontal ? child->width : child->height;
				children_size += (l == a->children) ? 0 : a->paddingx;
			}

			int pos = (panel_horizontal ? a->posx : a->posy) + a->bg->border.width + a->paddingxlr;
			pos += ((panel_horizontal ? a->width : a->height) - children_size) / 2;

			for (l = a->children; l ; l = l->next) {
				Area *child = ((Area*)l->data);
				if (!child->on_screen)
					continue;

				if (panel_horizontal) {
					if (pos != child->posx) {
						// pos changed => redraw
						child->posx = pos;
						child->on_changed = 1;
					}
				} else {
					if (pos != child->posy) {
						// pos changed => redraw
						child->posy = pos;
						child->on_changed = 1;
					}
				}

				size_by_layout(child, level+1);

				pos += panel_horizontal ? child->width + a->paddingx : child->height + a->paddingx;
			}
		}
	}

	if (a->on_changed) {
		// pos/size changed
		a->redraw = 1;
		if (a->_on_change_layout)
			a->_on_change_layout (a);
	}
}


void refresh (Area *a)
{
	// don't draw and resize hide objects
	if (!a->on_screen) return;

	// don't draw transparent objects (without foreground and without background)
	if (a->redraw) {
		a->redraw = 0;
		// force redraw of child
		//GList *l;
		//for (l = a->children ; l ; l = l->next)
			//((Area*)l->data)->redraw = 1;

		//printf("draw area posx %d, width %d\n", a->posx, a->width);
		draw(a);
	}

	// draw current Area
	if (a->pix == 0) printf("empty area posx %d, width %d\n", a->posx, a->width);
	XCopyArea (server.dsp, a->pix, ((Panel *)a->panel)->temp_pmap, server.gc, 0, 0, a->width, a->height, a->posx, a->posy);

	// and then refresh child object
	GList *l;
	for (l = a->children; l ; l = l->next)
		refresh((Area*)l->data);
}


int resize_by_layout(void *obj, int maximum_size)
{
	Area *child, *a = (Area*)obj;
	int size, nb_by_content=0, nb_by_layout=0;

	if (panel_horizontal) {		
		// detect free size for SIZE_BY_LAYOUT's Area
		size = a->width - (2 * (a->paddingxlr + a->bg->border.width));
		GList *l;
		for (l = a->children ; l ; l = l->next) {
			child = (Area*)l->data;
			if (child->on_screen && child->size_mode == SIZE_BY_CONTENT) {
				size -= child->width;
				nb_by_content++;
			}
			if (child->on_screen && child->size_mode == SIZE_BY_LAYOUT)
				nb_by_layout++;
		}
		//printf("  resize_by_layout Deb %d, %d\n", nb_by_content, nb_by_layout);
		if (nb_by_content+nb_by_layout)
			size -= ((nb_by_content+nb_by_layout-1) * a->paddingx);

		int width=0, modulo=0, old_width;
		if (nb_by_layout) {
			width = size / nb_by_layout;
			modulo = size % nb_by_layout;
			if (width > maximum_size && maximum_size != 0) {
				width = maximum_size;
				modulo = 0;
			}
		}

		// resize SIZE_BY_LAYOUT objects
		for (l = a->children ; l ; l = l->next) {
			child = (Area*)l->data;
			if (child->on_screen && child->size_mode == SIZE_BY_LAYOUT) {
				old_width = child->width;
				child->width = width;
				if (modulo) {
					child->width++;
					modulo--;
				}
				if (child->width != old_width)
					child->on_changed = 1;
			}
		}
	}
	else {
		// detect free size for SIZE_BY_LAYOUT's Area
		size = a->height - (2 * (a->paddingxlr + a->bg->border.width));
		GList *l;
		for (l = a->children ; l ; l = l->next) {
			child = (Area*)l->data;
			if (child->on_screen && child->size_mode == SIZE_BY_CONTENT) {
				size -= child->height;
				nb_by_content++;
			}
			if (child->on_screen && child->size_mode == SIZE_BY_LAYOUT)
				nb_by_layout++;
		}
		if (nb_by_content+nb_by_layout)
			size -= ((nb_by_content+nb_by_layout-1) * a->paddingx);

		int height=0, modulo=0;
		if (nb_by_layout) {
			height = size / nb_by_layout;
			modulo = size % nb_by_layout;
			if (height > maximum_size && maximum_size != 0) {
				height = maximum_size;
				modulo = 0;
			}
		}

		// resize SIZE_BY_LAYOUT objects
		for (l = a->children ; l ; l = l->next) {
			child = (Area*)l->data;
			if (child->on_screen && child->size_mode == SIZE_BY_LAYOUT) {
				int old_height = child->height;
				child->height = height;
				if (modulo) {
					child->height++;
					modulo--;
				}
				if (child->height != old_height)
					child->on_changed = 1;
			}
		}
	}
	return 0;
}


void set_redraw (Area *a)
{
	a->redraw = 1;

	GList *l;
	for (l = a->children ; l ; l = l->next)
		set_redraw((Area*)l->data);
}

void hide(Area *a)
{
	Area *parent = (Area*)a->parent;

	a->on_screen = 0;
	parent->resize = 1;
	if (panel_horizontal)
		a->width = 0;
	else
		a->height = 0;
}

void show(Area *a)
{
	Area *parent = (Area*)a->parent;

	a->on_screen = 1;
	parent->resize = 1;
	a->resize = 1;
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
	if (a->bg->back.alpha > 0.0 ||
		(panel_config.mouse_effects && (a->mouse_over_effect || a->mouse_press_effect))) {
		//printf("    draw_background (%d %d) RGBA (%lf, %lf, %lf, %lf)\n", a->posx, a->posy, pix->back.color[0], pix->back.color[1], pix->back.color[2], pix->back.alpha);
		if (a->mouse_state == MOUSE_OVER)
			cairo_set_source_rgba(c, a->bg->back_hover.color[0], a->bg->back_hover.color[1], a->bg->back_hover.color[2], a->bg->back_hover.alpha);
		else if (a->mouse_state == MOUSE_DOWN)
			cairo_set_source_rgba(c, a->bg->back_pressed.color[0], a->bg->back_pressed.color[1], a->bg->back_pressed.color[2], a->bg->back_pressed.alpha);
		else
			cairo_set_source_rgba(c, a->bg->back.color[0], a->bg->back.color[1], a->bg->back.color[2], a->bg->back.alpha);
		draw_rect(c, a->bg->border.width, a->bg->border.width, a->width-(2.0 * a->bg->border.width), a->height-(2.0*a->bg->border.width), a->bg->border.rounded - a->bg->border.width/1.571);
		cairo_fill(c);
	}

	if (a->bg->border.width > 0) {
		cairo_set_line_width (c, a->bg->border.width);

		// draw border inside (x, y, width, height)
		if (a->mouse_state == MOUSE_OVER)
			cairo_set_source_rgba(c, a->bg->border_hover.color[0], a->bg->border_hover.color[1], a->bg->border_hover.color[2], a->bg->border_hover.alpha);
		else if (a->mouse_state == MOUSE_DOWN)
			cairo_set_source_rgba(c, a->bg->border_pressed.color[0], a->bg->border_pressed.color[1], a->bg->border_pressed.color[2], a->bg->border_pressed.alpha);
		else
			cairo_set_source_rgba(c, a->bg->border.color[0], a->bg->border.color[1], a->bg->border.color[2], a->bg->border.alpha);
		draw_rect(c, a->bg->border.width/2.0, a->bg->border.width/2.0, a->width - a->bg->border.width, a->height - a->bg->border.width, a->bg->border.rounded);

		cairo_stroke(c);
	}
}


void remove_area (void *a)
{
	Area *area = (Area*)a;
	Area *parent = (Area*)area->parent;

	if (parent) {
		parent->children = g_list_remove(parent->children, area);
		parent->resize = 1;
		set_redraw(parent);
	}

	if (mouse_over_area == a) {
		mouse_out();
	}
}


void add_area (Area *a)
{
	Area *parent = (Area*)a->parent;

	parent->children = g_list_append(parent->children, a);
	set_redraw (parent);

}


void free_area (Area *a)
{
	if (!a)
		return;

	GList *l0;
	for (l0 = a->children; l0 ; l0 = l0->next)
		free_area (l0->data);

	if (a->children) {
		g_list_free(a->children);
		a->children = 0;
	}
	if (a->pix) {
		XFreePixmap (server.dsp, a->pix);
		a->pix = 0;
	}
	if (mouse_over_area == a) {
		mouse_over_area = NULL;
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

void mouse_over(Area *area, int pressed)
{
	if (mouse_over_area == area && !area)
		return;

	MouseState new_state = MOUSE_NORMAL;
	if (area) {
		if (!pressed) {
			new_state = area->mouse_over_effect ? MOUSE_OVER : MOUSE_NORMAL;
		} else {
			new_state = area->mouse_press_effect
						? MOUSE_DOWN
						: area->mouse_over_effect
						  ? MOUSE_OVER
						  : MOUSE_NORMAL;
		}
	}

	if (mouse_over_area == area && mouse_over_area->mouse_state == new_state)
		return;
	mouse_out();
	if (new_state == MOUSE_NORMAL)
		return;
	mouse_over_area = area;

	mouse_over_area->mouse_state = new_state;
	set_redraw(mouse_over_area);
	panel_refresh = 1;
}

void mouse_out()
{
	if (!mouse_over_area)
		return;
	mouse_over_area->mouse_state = MOUSE_NORMAL;
	set_redraw(mouse_over_area);
	panel_refresh = 1;
	mouse_over_area = NULL;
}

void init_background(Background *bg)
{
	memset(bg, 0, sizeof(Background));
}
