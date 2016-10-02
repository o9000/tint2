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
	// Panel *panel = separator->area.panel;
	if (!separator->area.on_screen)
		return FALSE;

	double d_height = panel_horizontal ? separator->area.height : separator->area.width;
	double d_thickness = round(d_height / 16) >= 1.0 ? round(d_height / 16) : 1.0;

	if (separator->style == 3)
		d_thickness = round(d_height / 8) >= 1.0 ? round(d_height / 8) : 1.0;

	double d_len = round(d_height / 16) >= 1.0 ? round(d_height / 16) : 1.0;
	double d_width = d_thickness * 5;

	if (separator->style == 4) {
		d_width = d_thickness * 7;
		d_thickness = d_thickness * 3;
	}

	if (separator->style == 2) {
		d_width = d_height;
		d_thickness = round(d_height / 8) >= 1.0 ? round(d_height / 8) : 1.0;
		d_len = d_thickness;
	}

	double d_empty_thickness = d_thickness;

	if (separator->style == 5 || separator->style == 6) {
		d_width = (d_thickness * 4) + 2.0;
		d_thickness = 1.0;
	}

	if (panel_horizontal) {
		separator->area.width = d_width;
		separator->area.height = d_height;
	} else {
		separator->area.width = d_height;
		separator->area.height = d_width;
	}

	separator->empty_thickness = d_empty_thickness;
	separator->thickness = d_thickness;
	separator->len = d_len;

	schedule_redraw(&separator->area);
	panel_refresh = TRUE;
	return TRUE;
}

void draw_separator(void *obj, cairo_t *c)
{
	Separator *separator = (Separator *)obj;

	if (separator->style == 0)
		return;

	double start_point = 0 + (separator->thickness * 2);
	double end_point = separator->area.height - (separator->thickness * 2);
	if (!panel_horizontal)
		end_point = separator->area.width - (separator->thickness * 2);
	double count = end_point - start_point;
	double thickness = separator->thickness;
	double len = separator->len;
	int alt = 0;
	double x_fix = 0;

	if (separator->style == 2) {
		if (!panel_horizontal)
			start_point = start_point + 2;
		cairo_set_source_rgba(c,
		                      separator->color.rgb[0],
		                      separator->color.rgb[1],
		                      separator->color.rgb[2],
		                      separator->color.alpha);
		cairo_set_line_width(c, 1);
		cairo_rectangle(c,
		                start_point - 2,
		                start_point - (panel_horizontal ? 0 : 4),
		                end_point - thickness - 3,
		                end_point - thickness - (panel_horizontal ? 3 : 3));
		cairo_stroke_preserve(c);
		cairo_fill(c);
		return;
	}

	if (count < thickness)
		return;

	while (((int)count) % 2) {
		if (alt) {
			start_point++;
			alt = 0;
		} else {
			end_point--;
			alt = 1;
		}
		count = end_point - start_point;
		if (count < thickness)
			return;
	}

	if (separator->style == 3 || separator->style == 4)
		x_fix = round(thickness / 2) + (separator->style == 4 ? 1.0 : 0.0);

	if (separator->style == 5 || separator->style == 6) {
		x_fix = -1.0;
		start_point = start_point + 2;
		end_point--;
	}

	double separator_pattern[] = {len, len};
	double separator_style6_pattern[] = {1.0};
	cairo_set_source_rgba(c,
	                      separator->color.rgb[0],
	                      separator->color.rgb[1],
	                      separator->color.rgb[2],
	                      separator->color.alpha);
	cairo_set_line_width(c, thickness);
	if (separator->style == 6)
		cairo_set_dash(c, separator_style6_pattern, 1, 0);
	else
		cairo_set_dash(c, separator_pattern, sizeof(separator_pattern) / sizeof(separator_pattern[0]), 0);
	if (panel_horizontal) {
		cairo_move_to(c, (separator->area.width / 2) - thickness + x_fix, start_point);
		cairo_line_to(c, (separator->area.width / 2) - thickness + x_fix, end_point);
	} else {
		cairo_move_to(c, start_point, (separator->area.height / 2) - thickness + x_fix);
		cairo_line_to(c, end_point, (separator->area.height / 2) - thickness + x_fix);
	}
	cairo_stroke(c);
}
