// Tint2 : Separator plugin
// Author: Oskari Rauta

#include <string.h>
#include <stdio.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <pango/pangocairo.h>
#include <stdlib.h>
#include <math.h>

#include "window.h"
#include "server.h"
#include "panel.h"
#include "common.h"
#include "separator.h"

Separator *create_separator()
{
	Separator *separator = (Separator *)calloc(1, sizeof(Separator));
	separator->color.rgb[0] = 0.5;
	separator->color.rgb[1] = 0.5;
	separator->color.rgb[2] = 0.5;
	separator->color.alpha = 0.9;
	separator->style = SEPARATOR_DOTS;
	separator->thickness = 3;
	separator->area.paddingxlr = 1;
	return separator;
}

void destroy_separator(void *obj)
{
	Separator *separator = (Separator *)obj;
	remove_area(&separator->area);
	free_area(&separator->area);
	free_and_null(separator);
}

void init_separator()
{
	GList *to_remove = panel_config.separator_list;
	for (int k = 0; k < strlen(panel_items_order) && to_remove; k++) {
		if (panel_items_order[k] == ':') {
			to_remove = to_remove->next;
		}
	}

	if (to_remove) {
		if (to_remove == panel_config.separator_list) {
			g_list_free_full(to_remove, destroy_separator);
			panel_config.separator_list = NULL;
		} else {
			// Cut panel_config.separator_list
			if (to_remove->prev)
				to_remove->prev->next = NULL;
			to_remove->prev = NULL;
			// Remove all elements of to_remove and to_remove itself
			g_list_free_full(to_remove, destroy_separator);
		}
	}
}

void init_separator_panel(void *p)
{
	Panel *panel = (Panel *)p;

	// Make sure this is only done once if there are multiple items
	if (panel->separator_list)
		return;

	// panel->separator_list is now a copy of the pointer panel_config.separator_list
	// We make it a deep copy
	panel->separator_list = g_list_copy_deep(panel_config.separator_list, NULL, NULL);

	for (GList *l = panel->separator_list; l; l = l->next) {
		Separator *separator = (Separator *)l->data;
		if (!separator->area.bg)
			separator->area.bg = &g_array_index(backgrounds, Background, 0);
		separator->area.parent = p;
		separator->area.panel = p;
		snprintf(separator->area.name, sizeof(separator->area.name), "separator");
		separator->area.size_mode = LAYOUT_FIXED;
		separator->area.resize_needed = 1;
		separator->area.on_screen = TRUE;
		separator->area._resize = resize_separator;
		separator->area._draw_foreground = draw_separator;
	}
}

void cleanup_separator()
{
	// Cleanup frontends
	for (int i = 0; i < num_panels; i++) {
		g_list_free_full(panels[i].separator_list, destroy_separator);
		panels[i].separator_list = NULL;
	}

	// Cleanup backends
	g_list_free_full(panel_config.separator_list, destroy_separator);
	panel_config.separator_list = NULL;
}

gboolean resize_separator(void *obj)
{
	Separator *separator = (Separator *)obj;
	if (!separator->area.on_screen)
		return FALSE;

	if (panel_horizontal) {
		separator->area.width =
		    separator->thickness + 2 * separator->area.paddingxlr + left_right_border_width(&separator->area);
		separator->length =
		    separator->area.height - 2 * separator->area.paddingy - top_bottom_border_width(&separator->area);
	} else {
		separator->area.height =
		    separator->thickness + 2 * separator->area.paddingxlr + top_bottom_border_width(&separator->area);
		separator->length =
		    separator->area.width - 2 * separator->area.paddingy - left_right_border_width(&separator->area);
	}

	schedule_redraw(&separator->area);
	panel_refresh = TRUE;
	return TRUE;
}

void draw_separator_line(void *obj, cairo_t *c);
void draw_separator_dots(void *obj, cairo_t *c);

void draw_separator(void *obj, cairo_t *c)
{
	Separator *separator = (Separator *)obj;

	if (separator->style == SEPARATOR_EMPTY)
		return;
	else if (separator->style == SEPARATOR_LINE)
		draw_separator_line(separator, c);
	else if (separator->style == SEPARATOR_DOTS)
		draw_separator_dots(separator, c);
}

void draw_separator_line(void *obj, cairo_t *c)
{
	Separator *separator = (Separator *)obj;

	if (separator->thickness <= 0)
		return;

	cairo_set_source_rgba(c,
	                      separator->color.rgb[0],
	                      separator->color.rgb[1],
	                      separator->color.rgb[2],
	                      separator->color.alpha);
	cairo_set_line_width(c, separator->thickness);
	cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND);
	if (panel_horizontal) {
		cairo_move_to(c, separator->area.width / 2.0, separator->area.height / 2.0 - separator->length / 2.0);
		cairo_line_to(c, separator->area.width / 2.0, separator->area.height / 2.0 + separator->length / 2.0);
	} else {
		cairo_move_to(c, separator->area.width / 2.0 - separator->length / 2.0, separator->area.height / 2.0);
		cairo_line_to(c, separator->area.width / 2.0 - separator->length / 2.0, separator->area.height / 2.0);
	}
	cairo_stroke(c);
}

void draw_separator_dots(void *obj, cairo_t *c)
{
	Separator *separator = (Separator *)obj;
	if (separator->thickness <= 0)
		return;

	cairo_set_source_rgba(c,
	                      separator->color.rgb[0],
	                      separator->color.rgb[1],
	                      separator->color.rgb[2],
	                      separator->color.alpha);
	cairo_set_line_width(c, 0);

	int num_circles = separator->length / (1.618 * separator->thickness - 1);
	double spacing = (separator->length - num_circles * separator->thickness) / MAX(1.0, num_circles - 1.0);
	if (spacing > separator->thickness)
		num_circles++;
	spacing = (separator->length - num_circles * separator->thickness) / MAX(1.0, num_circles - 1.0);
	double offset = (panel_horizontal ? separator->area.height : separator->area.width) / 2.0 - separator->length / 2.0;
	if (num_circles == 1)
		offset += spacing / 2.0;
	for (int i = 0; i < num_circles; i++) {
		if (panel_horizontal) {
			cairo_arc(c,
					  separator->area.width / 2.0,
					  offset + separator->thickness / 2.0,
					  separator->thickness / 2.0,
					  0,
					  2 * M_PI);
		} else {
			cairo_arc(c,
					  offset + separator->thickness / 2.0,
					  separator->area.height / 2.0,
					  separator->thickness / 2.0,
					  0,
					  2 * M_PI);
		}
		cairo_stroke_preserve(c);
		cairo_fill(c);
		offset += separator->thickness + spacing;
	}
}
