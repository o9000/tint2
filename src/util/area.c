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
#include "common.h"

Area *mouse_over_area = NULL;

void init_background(Background *bg)
{
	memset(bg, 0, sizeof(Background));
}

void initialize_positions(void *obj, int offset)
{
	Area *a = (Area *)obj;
	for (GList *l = a->children; l; l = l->next) {
		Area *child = ((Area *)l->data);
		if (panel_horizontal) {
			child->posy = offset + a->bg->border.width + a->paddingy;
			child->height = a->height - (2 * (a->bg->border.width + a->paddingy));
			if (child->_on_change_layout)
				child->_on_change_layout(child);
			initialize_positions(child, child->posy);
		} else {
			child->posx = offset + a->bg->border.width + a->paddingy;
			child->width = a->width - (2 * (a->bg->border.width + a->paddingy));
			if (child->_on_change_layout)
				child->_on_change_layout(child);
			initialize_positions(child, child->posx);
		}
	}
}

void relayout_fixed(Area *a)
{
	if (!a->on_screen)
		return;

	// Children are resized before the parent
	GList *l;
	for (l = a->children; l; l = l->next)
		relayout_fixed(l->data);

	// Recalculate size
	a->_changed = 0;
	if (a->resize_needed && a->size_mode == LAYOUT_FIXED) {
		a->resize_needed = 0;

		if (a->_resize) {
			if (a->_resize(a)) {
				// The size hash changed => resize needed for the parent
				if (a->parent)
					((Area *)a->parent)->resize_needed = 1;
				a->_changed = 1;
			}
		}
	}
}

void relayout_dynamic(Area *a, int level)
{
	if (!a->on_screen)
		return;

	// Area is resized before its children
	if (a->resize_needed && a->size_mode == LAYOUT_DYNAMIC) {
		a->resize_needed = 0;

		if (a->_resize) {
			a->_resize(a);
			// resize children with LAYOUT_DYNAMIC
			for (GList *l = a->children; l; l = l->next) {
				Area *child = ((Area *)l->data);
				if (child->size_mode == LAYOUT_DYNAMIC && child->children)
					child->resize_needed = 1;
			}
		}
	}

	// Layout children
	if (a->children) {
		if (a->alignment == ALIGN_LEFT) {
			int pos = (panel_horizontal ? a->posx : a->posy) + a->bg->border.width + a->paddingxlr;

			for (GList *l = a->children; l; l = l->next) {
				Area *child = ((Area *)l->data);
				if (!child->on_screen)
					continue;

				if (panel_horizontal) {
					if (pos != child->posx) {
						// pos changed => redraw
						child->posx = pos;
						child->_changed = 1;
					}
				} else {
					if (pos != child->posy) {
						// pos changed => redraw
						child->posy = pos;
						child->_changed = 1;
					}
				}

				relayout_dynamic(child, level + 1);

				pos += panel_horizontal ? child->width + a->paddingx : child->height + a->paddingx;
			}
		} else if (a->alignment == ALIGN_RIGHT) {
			int pos = (panel_horizontal ? a->posx + a->width : a->posy + a->height) - a->bg->border.width - a->paddingxlr;

			for (GList *l = g_list_last(a->children); l; l = l->prev) {
				Area *child = ((Area *)l->data);
				if (!child->on_screen)
					continue;

				pos -= panel_horizontal ? child->width : child->height;

				if (panel_horizontal) {
					if (pos != child->posx) {
						// pos changed => redraw
						child->posx = pos;
						child->_changed = 1;
					}
				} else {
					if (pos != child->posy) {
						// pos changed => redraw
						child->posy = pos;
						child->_changed = 1;
					}
				}

				relayout_dynamic(child, level + 1);

				pos -= a->paddingx;
			}
		} else if (a->alignment == ALIGN_CENTER) {

			int children_size = 0;

			for (GList *l = a->children; l; l = l->next) {
				Area *child = ((Area *)l->data);
				if (!child->on_screen)
					continue;

				children_size += panel_horizontal ? child->width : child->height;
				children_size += (l == a->children) ? 0 : a->paddingx;
			}

			int pos = (panel_horizontal ? a->posx : a->posy) + a->bg->border.width + a->paddingxlr;
			pos += ((panel_horizontal ? a->width : a->height) - children_size) / 2;

			for (GList *l = a->children; l; l = l->next) {
				Area *child = ((Area *)l->data);
				if (!child->on_screen)
					continue;

				if (panel_horizontal) {
					if (pos != child->posx) {
						// pos changed => redraw
						child->posx = pos;
						child->_changed = 1;
					}
				} else {
					if (pos != child->posy) {
						// pos changed => redraw
						child->posy = pos;
						child->_changed = 1;
					}
				}

				relayout_dynamic(child, level + 1);

				pos += panel_horizontal ? child->width + a->paddingx : child->height + a->paddingx;
			}
		}
	}

	if (a->_changed) {
		// pos/size changed
		a->_redraw_needed = TRUE;
		if (a->_on_change_layout)
			a->_on_change_layout(a);
	}
}

void relayout(Area *a)
{
	relayout_fixed(a);
	relayout_dynamic(a, 1);
}

void draw_tree(Area *a)
{
	if (!a->on_screen)
		return;

	if (a->_redraw_needed) {
		a->_redraw_needed = 0;
		draw(a);
	}

	if (a->pix)
		XCopyArea(server.display, a->pix, ((Panel *)a->panel)->temp_pmap, server.gc, 0, 0, a->width, a->height, a->posx, a->posy);

	for (GList *l = a->children; l; l = l->next)
		draw_tree((Area *)l->data);
}

int relayout_with_constraint(Area *a, int maximum_size)
{
	int fixed_children_count = 0;
	int dynamic_children_count = 0;

	if (panel_horizontal) {
		// detect free size for LAYOUT_DYNAMIC Areas
		int size = a->width - (2 * (a->paddingxlr + a->bg->border.width));
		for (GList *l = a->children; l; l = l->next) {
			Area *child = (Area *)l->data;
			if (child->on_screen && child->size_mode == LAYOUT_FIXED) {
				size -= child->width;
				fixed_children_count++;
			}
			if (child->on_screen && child->size_mode == LAYOUT_DYNAMIC)
				dynamic_children_count++;
		}
		if (fixed_children_count + dynamic_children_count > 0)
			size -= (fixed_children_count + dynamic_children_count - 1) * a->paddingx;

		int width = 0;
		int modulo = 0;
		if (dynamic_children_count > 0) {
			width = size / dynamic_children_count;
			modulo = size % dynamic_children_count;
			if (width > maximum_size && maximum_size > 0) {
				width = maximum_size;
				modulo = 0;
			}
		}

		// Resize LAYOUT_DYNAMIC objects
		for (GList *l = a->children; l; l = l->next) {
			Area *child = (Area *)l->data;
			if (child->on_screen && child->size_mode == LAYOUT_DYNAMIC) {
				int old_width = child->width;
				child->width = width;
				if (modulo) {
					child->width++;
					modulo--;
				}
				if (child->width != old_width)
					child->_changed = 1;
			}
		}
	} else {
		// detect free size for LAYOUT_DYNAMIC's Area
		int size = a->height - (2 * (a->paddingxlr + a->bg->border.width));
		for (GList *l = a->children; l; l = l->next) {
			Area *child = (Area *)l->data;
			if (child->on_screen && child->size_mode == LAYOUT_FIXED) {
				size -= child->height;
				fixed_children_count++;
			}
			if (child->on_screen && child->size_mode == LAYOUT_DYNAMIC)
				dynamic_children_count++;
		}
		if (fixed_children_count + dynamic_children_count > 0)
			size -= (fixed_children_count + dynamic_children_count - 1) * a->paddingx;

		int height = 0;
		int modulo = 0;
		if (dynamic_children_count) {
			height = size / dynamic_children_count;
			modulo = size % dynamic_children_count;
			if (height > maximum_size && maximum_size != 0) {
				height = maximum_size;
				modulo = 0;
			}
		}

		// Resize LAYOUT_DYNAMIC objects
		for (GList *l = a->children; l; l = l->next) {
			Area *child = (Area *)l->data;
			if (child->on_screen && child->size_mode == LAYOUT_DYNAMIC) {
				int old_height = child->height;
				child->height = height;
				if (modulo) {
					child->height++;
					modulo--;
				}
				if (child->height != old_height)
					child->_changed = 1;
			}
		}
	}
	return 0;
}

void schedule_redraw(Area *a)
{
	a->_redraw_needed = TRUE;

	if (a->has_mouse_over_effect) {
		for (int i = 0; i < MOUSE_STATE_COUNT; i++) {
			XFreePixmap(server.display, a->pix_by_state[i]);
			if (a->pix == a->pix_by_state[i])
				a->pix = None;
			a->pix_by_state[i] = None;
		}
		if (a->pix) {
			XFreePixmap(server.display, a->pix);
			a->pix = None;
		}
	}

	for (GList *l = a->children; l; l = l->next)
		schedule_redraw((Area *)l->data);
	panel_refresh = TRUE;
}

void hide(Area *a)
{
	Area *parent = (Area *)a->parent;

	a->on_screen = FALSE;
	if (parent)
		parent->resize_needed = 1;
	if (panel_horizontal)
		a->width = 0;
	else
		a->height = 0;
}

void show(Area *a)
{
	Area *parent = (Area *)a->parent;

	a->on_screen = TRUE;
	if (parent)
		parent->resize_needed = 1;
	a->resize_needed = 1;
}

void draw(Area *a)
{
	if (a->pix) {
		XFreePixmap(server.display, a->pix);
		if (a->pix_by_state[a->has_mouse_over_effect ? a->mouse_state : 0] != a->pix)
			XFreePixmap(server.display, a->pix_by_state[a->has_mouse_over_effect ? a->mouse_state : 0]);
	}
	a->pix = XCreatePixmap(server.display, server.root_win, a->width, a->height, server.depth);
	a->pix_by_state[a->has_mouse_over_effect ? a->mouse_state : 0] = a->pix;

	if (!a->_clear) {
		// Add layer of root pixmap (or clear pixmap if real_transparency==true)
		if (server.real_transparency)
			clear_pixmap(a->pix, 0, 0, a->width, a->height);
		XCopyArea(server.display, ((Panel *)a->panel)->temp_pmap, a->pix, server.gc, a->posx, a->posy, a->width, a->height, 0, 0);
	} else {
		a->_clear(a);
	}

	cairo_surface_t *cs = cairo_xlib_surface_create(server.display, a->pix, server.visual, a->width, a->height);
	cairo_t *c = cairo_create(cs);

	draw_background(a, c);

	if (a->_draw_foreground)
		a->_draw_foreground(a, c);

	cairo_destroy(c);
	cairo_surface_destroy(cs);
}

void draw_background(Area *a, cairo_t *c)
{
	if (a->bg->fill_color.alpha > 0.0 ||
		(panel_config.mouse_effects && (a->has_mouse_over_effect || a->has_mouse_press_effect))) {
		if (a->mouse_state == MOUSE_OVER)
			cairo_set_source_rgba(c,
								  a->bg->fill_color_hover.rgb[0],
								  a->bg->fill_color_hover.rgb[1],
								  a->bg->fill_color_hover.rgb[2],
								  a->bg->fill_color_hover.alpha);
		else if (a->mouse_state == MOUSE_DOWN)
			cairo_set_source_rgba(c,
								  a->bg->fill_color_pressed.rgb[0],
								  a->bg->fill_color_pressed.rgb[1],
								  a->bg->fill_color_pressed.rgb[2],
								  a->bg->fill_color_pressed.alpha);
		else
			cairo_set_source_rgba(c,
								  a->bg->fill_color.rgb[0],
								  a->bg->fill_color.rgb[1],
								  a->bg->fill_color.rgb[2],
								  a->bg->fill_color.alpha);
		draw_rect(c,
				  a->bg->border.width,
				  a->bg->border.width,
				  a->width - (2.0 * a->bg->border.width),
				  a->height - (2.0 * a->bg->border.width),
				  a->bg->border.radius - a->bg->border.width / 1.571);
		cairo_fill(c);
	}

	if (a->bg->border.width > 0) {
		cairo_set_line_width(c, a->bg->border.width);

		// draw border inside (x, y, width, height)
		if (a->mouse_state == MOUSE_OVER)
			cairo_set_source_rgba(c,
								  a->bg->border_color_hover.rgb[0],
								  a->bg->border_color_hover.rgb[1],
								  a->bg->border_color_hover.rgb[2],
								  a->bg->border_color_hover.alpha);
		else if (a->mouse_state == MOUSE_DOWN)
			cairo_set_source_rgba(c,
								  a->bg->border_color_pressed.rgb[0],
								  a->bg->border_color_pressed.rgb[1],
								  a->bg->border_color_pressed.rgb[2],
								  a->bg->border_color_pressed.alpha);
		else
			cairo_set_source_rgba(c,
								  a->bg->border.color.rgb[0],
								  a->bg->border.color.rgb[1],
								  a->bg->border.color.rgb[2],
								  a->bg->border.color.alpha);
		draw_rect(c,
				  a->bg->border.width / 2.0,
				  a->bg->border.width / 2.0,
				  a->width - a->bg->border.width,
				  a->height - a->bg->border.width,
				  a->bg->border.radius);

		cairo_stroke(c);
	}
}

void remove_area(Area *a)
{
	Area *area = (Area *)a;
	Area *parent = (Area *)area->parent;

	if (parent) {
		parent->children = g_list_remove(parent->children, area);
		parent->resize_needed = TRUE;
		panel_refresh = TRUE;
		schedule_redraw(parent);
	}

	if (mouse_over_area == a) {
		mouse_out();
	}
}

void add_area(Area *a, Area *parent)
{
	g_assert_null(a->parent);

	a->parent = parent;
	if (parent) {
		parent->children = g_list_append(parent->children, a);
		parent->resize_needed = TRUE;
		schedule_redraw(parent);
		panel_refresh = TRUE;
	}
}

void free_area(Area *a)
{
	if (!a)
		return;

	for (GList *l = a->children; l; l = l->next)
		free_area(l->data);

	if (a->children) {
		g_list_free(a->children);
		a->children = NULL;
	}
	for (int i = 0; i < MOUSE_STATE_COUNT; i++) {
		XFreePixmap(server.display, a->pix_by_state[i]);
		if (a->pix == a->pix_by_state[i]) {
			a->pix = None;
		}
		a->pix_by_state[i] = None;
	}
	if (a->pix) {
		XFreePixmap(server.display, a->pix);
		a->pix = None;
	}
	if (mouse_over_area == a) {
		mouse_over_area = NULL;
	}
}

void mouse_over(Area *area, int pressed)
{
	if (mouse_over_area == area && !area)
		return;

	MouseState new_state = MOUSE_NORMAL;
	if (area) {
		if (!pressed) {
			new_state = area->has_mouse_over_effect ? MOUSE_OVER : MOUSE_NORMAL;
		} else {
			new_state = area->has_mouse_press_effect
						? MOUSE_DOWN
						: area->has_mouse_over_effect
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
	mouse_over_area->pix = mouse_over_area->pix_by_state[mouse_over_area->has_mouse_over_effect ? mouse_over_area->mouse_state : 0];
	if (!mouse_over_area->pix)
		mouse_over_area->_redraw_needed = TRUE;
	panel_refresh = TRUE;
}

void mouse_out()
{
	if (!mouse_over_area)
		return;
	mouse_over_area->mouse_state = MOUSE_NORMAL;
	mouse_over_area->pix = mouse_over_area->pix_by_state[mouse_over_area->has_mouse_over_effect ? mouse_over_area->mouse_state : 0];
	if (!mouse_over_area->pix)
		mouse_over_area->_redraw_needed = TRUE;
	panel_refresh = TRUE;
	mouse_over_area = NULL;
}

gboolean area_is_under_mouse(void *obj, int x, int y)
{
	Area *a = obj;
	if (!a->on_screen)
		return FALSE;

	if (a->_is_under_mouse)
		return a->_is_under_mouse(a, x, y);

	return x >= a->posx && x <= (a->posx + a->width) && y >= a->posy && y <= (a->posy + a->height);
}

gboolean full_width_area_is_under_mouse(void *obj, int x, int y)
{
	Area *a = obj;
	if (!a->on_screen)
		return FALSE;

	if (a->_is_under_mouse && a->_is_under_mouse != full_width_area_is_under_mouse)
		return a->_is_under_mouse(a, x, y);

	if (panel_horizontal)
		return x >= a->posx && x <= a->posx + a->width;
	else
		return y >= a->posy && y <= a->posy + a->height;
}

Area *find_area_under_mouse(void *root, int x, int y)
{
	Area *result = root;
	Area *new_result = result;
	do {
		result = new_result;
		GList *it = result->children;
		while (it) {
			Area *a = (Area *)it->data;
			if (area_is_under_mouse(a, x, y)) {
				new_result = a;
				break;
			}
			it = it->next;
		}
	} while (new_result != result);
	return result;
}
