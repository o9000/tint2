#include "gradient.h"

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

gboolean read_double(const char *str, double *value)
{
	if (!str[0])
		return FALSE;
	char *end;
	*value = strtod(str, &end);
	if (end[0])
		return FALSE;
	return TRUE;
}

gboolean read_double_with_percent(const char *str, double *value)
{
	if (!str[0])
		return FALSE;
	char *end;
	*value = strtod(str, &end);
	if (end[0] == '%' && !end[1]) {
		*value *= 0.01;
		return TRUE;
	}
	if (end[0])
		return FALSE;
	return TRUE;
}

GradientType gradient_type_from_string(const char *str)
{
	if (g_str_equal(str, "horizontal"))
		return GRADIENT_HORIZONTAL;
	if (g_str_equal(str, "vertical"))
		return GRADIENT_VERTICAL;
	if (g_str_equal(str, "centered"))
		return GRADIENT_CENTERED;
	if (g_str_equal(str, "linear"))
		return GRADIENT_LINEAR;
	if (g_str_equal(str, "radial"))
		return GRADIENT_RADIAL;
	fprintf(stderr, RED "Invalid gradient type: %s" RESET "\n", str);
	return GRADIENT_VERTICAL;
}

gboolean read_origin_from_string(const char *str, Origin *element)
{
	if (g_str_equal(str, "element")) {
		*element = ORIGIN_ELEMENT;
		return TRUE;
	}
	if (g_str_equal(str, "parent")) {
		*element = ORIGIN_PARENT;
		return TRUE;
	}
	if (g_str_equal(str, "panel")) {
		*element = ORIGIN_PANEL;
		return TRUE;
	}
	if (g_str_equal(str, "screen")) {
		*element = ORIGIN_SCREEN;
		return TRUE;
	}
	if (g_str_equal(str, "desktop")) {
		*element = ORIGIN_DESKTOP;
		return TRUE;
	}
	return FALSE;
}

Origin origin_from_string(const char *str)
{
	Origin result;
	if (read_origin_from_string(str, &result))
		return result;
	fprintf(stderr, RED "Invalid origin type: %s" RESET "\n", str);
	return ORIGIN_ELEMENT;
}

gboolean read_size_from_string(const char *str, SizeVariable *variable)
{
	if (g_str_equal(str, "width")) {
		*variable = SIZE_WIDTH;
		return TRUE;
	}
	if (g_str_equal(str, "height")) {
		*variable = SIZE_HEIGHT;
		return TRUE;
	}
	if (g_str_equal(str, "left")) {
		*variable = SIZE_LEFT;
		return TRUE;
	}
	if (g_str_equal(str, "right")) {
		*variable = SIZE_RIGHT;
		return TRUE;
	}
	if (g_str_equal(str, "top")) {
		*variable = SIZE_TOP;
		return TRUE;
	}
	if (g_str_equal(str, "bottom")) {
		*variable = SIZE_BOTTOM;
		return TRUE;
	}
	if (g_str_equal(str, "center")) {
		*variable = SIZE_CENTER;
		return TRUE;
	}
	if (g_str_equal(str, "radius")) {
		*variable = SIZE_RADIUS;
		return TRUE;
	}
	return FALSE;
}

gboolean read_size_variable_from_string(const char *str,
                                        Origin *variable_element,
                                        SizeVariable *variable,
                                        double *multiplier)
{
	if (read_size_from_string(str, variable)) {
		*variable_element = ORIGIN_ELEMENT;
		*multiplier = 1;
		return TRUE;
	}

	char *value1 = 0, *value2 = 0, *value3 = 0, *value4 = 0;
	extract_values_4(str, &value1, &value2, &value3, &value4);

	if (value1 && value2 && !value3) {
		if (read_origin_from_string(value1, variable_element) && read_size_from_string(value2, variable)) {
			*multiplier = 1;
			if (value1)
				free(value1);
			if (value2)
				free(value2);
			if (value3)
				free(value3);
			if (value4)
				free(value4);
			return TRUE;
		}
	}

	if (value1 && value2 && value3 && value4) {
		if (read_origin_from_string(value1, variable_element) && read_size_from_string(value2, variable) &&
		    g_str_equal(value3, "*") && read_double_with_percent(value4, multiplier)) {
			if (value1)
				free(value1);
			if (value2)
				free(value2);
			if (value3)
				free(value3);
			if (value4)
				free(value4);
			return TRUE;
		}
	}

	if (value1)
		free(value1);
	if (value2)
		free(value2);
	if (value3)
		free(value3);
	if (value4)
		free(value4);

	return FALSE;
}

Offset *offset_from_string(const char *str)
{
	Offset *offset = (Offset *)calloc(1, sizeof(Offset));
	// number ?
	if (read_double(str, &offset->constant_value)) {
		offset->constant = TRUE;
		return offset;
	}
	// SIZE ?
	offset->constant = FALSE;

	if (read_size_variable_from_string(str, &offset->variable_element, &offset->variable, &offset->multiplier)) {
		return offset;
	}

	free(offset);
	return NULL;
}

void init_gradient(GradientClass *g, GradientType type)
{
	memset(g, 0, sizeof(*g));
	g->type = type;
	if (g->type == GRADIENT_VERTICAL) {
		g->from.origin = ORIGIN_ELEMENT;
		Offset *offset_top = (Offset *)calloc(1, sizeof(Offset));
		offset_top->constant = TRUE;
		offset_top->constant_value = 0;
		g->from.offsets_y = g_list_append(g->from.offsets_y, offset_top);
		Offset *offset_bottom = (Offset *)calloc(1, sizeof(Offset));
		offset_bottom->constant = FALSE;
		offset_bottom->variable_element = ORIGIN_ELEMENT;
		offset_bottom->variable = SIZE_HEIGHT;
		offset_bottom->multiplier = 1.0;
		g->from.offsets_y = g_list_append(g->from.offsets_y, offset_bottom);
	} else if (g->type == GRADIENT_HORIZONTAL) {
		g->from.origin = ORIGIN_ELEMENT;
		Offset *offset_left = (Offset *)calloc(1, sizeof(Offset));
		offset_left->constant = TRUE;
		offset_left->constant_value = 0;
		g->from.offsets_x = g_list_append(g->from.offsets_x, offset_left);
		Offset *offset_right = (Offset *)calloc(1, sizeof(Offset));
		offset_right->constant = FALSE;
		offset_right->variable_element = ORIGIN_ELEMENT;
		offset_right->variable = SIZE_WIDTH;
		offset_right->multiplier = 1.0;
		g->from.offsets_x = g_list_append(g->from.offsets_x, offset_right);
	} else if (g->type == GRADIENT_CENTERED) {
		g->from.origin = ORIGIN_ELEMENT;
		Offset *offset_center_x = (Offset *)calloc(1, sizeof(Offset));
		offset_center_x->constant = FALSE;
		offset_center_x->variable_element = ORIGIN_ELEMENT;
		offset_center_x->variable = SIZE_CENTER;
		offset_center_x->multiplier = 1.0;
		g->from.offsets_x = g_list_append(g->from.offsets_x, offset_center_x);
		Offset *offset_center_y = (Offset *)calloc(1, sizeof(Offset));
		offset_center_y->constant = FALSE;
		offset_center_y->variable_element = ORIGIN_ELEMENT;
		offset_center_y->variable = SIZE_CENTER;
		offset_center_y->multiplier = 1.0;
		g->from.offsets_y = g_list_append(g->from.offsets_y, offset_center_y);
		Offset *offset_center_r = (Offset *)calloc(1, sizeof(Offset));
		offset_center_x->constant = TRUE;
		offset_center_x->constant_value = 0;
		g->from.offsets_r = g_list_append(g->from.offsets_r, offset_center_r);
		g->to.origin = ORIGIN_ELEMENT;
		offset_center_x = (Offset *)calloc(1, sizeof(Offset));
		offset_center_x->constant = FALSE;
		offset_center_x->variable_element = ORIGIN_ELEMENT;
		offset_center_x->variable = SIZE_CENTER;
		offset_center_x->multiplier = 1.0;
		g->to.offsets_x = g_list_append(g->to.offsets_x, offset_center_x);
		offset_center_y = (Offset *)calloc(1, sizeof(Offset));
		offset_center_y->constant = FALSE;
		offset_center_y->variable_element = ORIGIN_ELEMENT;
		offset_center_y->variable = SIZE_CENTER;
		offset_center_y->multiplier = 1.0;
		g->to.offsets_y = g_list_append(g->to.offsets_y, offset_center_y);
		offset_center_r = (Offset *)calloc(1, sizeof(Offset));
		offset_center_r->constant = FALSE;
		offset_center_r->variable_element = ORIGIN_ELEMENT;
		offset_center_r->variable = SIZE_RADIUS;
		offset_center_r->multiplier = 1.0;
		g->to.offsets_r = g_list_append(g->to.offsets_r, offset_center_r);
	} else if (g->type == GRADIENT_LINEAR) {
		// Nothing to do, the user has to add control points
	} else if (g->type == GRADIENT_RADIAL) {
		// Nothing to do, the user has to add control points
	}
}

void cleanup_gradient(GradientClass *g)
{
	g_list_free_full(g->extra_color_stops, free);
	g_list_free_full(g->from.offsets_x, free);
	g_list_free_full(g->from.offsets_y, free);
	g_list_free_full(g->from.offsets_r, free);
	g_list_free_full(g->to.offsets_x, free);
	g_list_free_full(g->to.offsets_y, free);
	g_list_free_full(g->to.offsets_r, free);
}
