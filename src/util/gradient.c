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
    if (g_str_equal(str, "radial"))
        return GRADIENT_CENTERED;
    fprintf(stderr, RED "tint2: Invalid gradient type: %s" RESET "\n", str);
    return GRADIENT_VERTICAL;
}

void init_gradient(GradientClass *g, GradientType type)
{
    memset(g, 0, sizeof(*g));
    g->type = type;
    if (g->type == GRADIENT_VERTICAL) {
        Offset *offset_top = (Offset *)calloc(1, sizeof(Offset));
        offset_top->constant = TRUE;
        offset_top->constant_value = 0;
        g->from.offsets_y = g_list_append(g->from.offsets_y, offset_top);
        Offset *offset_bottom = (Offset *)calloc(1, sizeof(Offset));
        offset_bottom->constant = FALSE;
        offset_bottom->element = ELEMENT_SELF;
        offset_bottom->variable = SIZE_HEIGHT;
        offset_bottom->multiplier = 1.0;
        g->to.offsets_y = g_list_append(g->to.offsets_y, offset_bottom);
    } else if (g->type == GRADIENT_HORIZONTAL) {
        Offset *offset_left = (Offset *)calloc(1, sizeof(Offset));
        offset_left->constant = TRUE;
        offset_left->constant_value = 0;
        g->from.offsets_x = g_list_append(g->from.offsets_x, offset_left);
        Offset *offset_right = (Offset *)calloc(1, sizeof(Offset));
        offset_right->constant = FALSE;
        offset_right->element = ELEMENT_SELF;
        offset_right->variable = SIZE_WIDTH;
        offset_right->multiplier = 1.0;
        g->to.offsets_x = g_list_append(g->to.offsets_x, offset_right);
    } else if (g->type == GRADIENT_CENTERED) {
        // from
        Offset *offset_center_x = (Offset *)calloc(1, sizeof(Offset));
        offset_center_x->constant = FALSE;
        offset_center_x->element = ELEMENT_SELF;
        offset_center_x->variable = SIZE_CENTERX;
        offset_center_x->multiplier = 1.0;
        g->from.offsets_x = g_list_append(g->from.offsets_x, offset_center_x);
        Offset *offset_center_y = (Offset *)calloc(1, sizeof(Offset));
        offset_center_y->constant = FALSE;
        offset_center_y->element = ELEMENT_SELF;
        offset_center_y->variable = SIZE_CENTERY;
        offset_center_y->multiplier = 1.0;
        g->from.offsets_y = g_list_append(g->from.offsets_y, offset_center_y);
        Offset *offset_center_r = (Offset *)calloc(1, sizeof(Offset));
        offset_center_r->constant = TRUE;
        offset_center_r->constant_value = 0;
        g->from.offsets_r = g_list_append(g->from.offsets_r, offset_center_r);
        // to
        offset_center_x = (Offset *)calloc(1, sizeof(Offset));
        offset_center_x->constant = FALSE;
        offset_center_x->element = ELEMENT_SELF;
        offset_center_x->variable = SIZE_CENTERX;
        offset_center_x->multiplier = 1.0;
        g->to.offsets_x = g_list_append(g->to.offsets_x, offset_center_x);
        offset_center_y = (Offset *)calloc(1, sizeof(Offset));
        offset_center_y->constant = FALSE;
        offset_center_y->element = ELEMENT_SELF;
        offset_center_y->variable = SIZE_CENTERY;
        offset_center_y->multiplier = 1.0;
        g->to.offsets_y = g_list_append(g->to.offsets_y, offset_center_y);
        offset_center_r = (Offset *)calloc(1, sizeof(Offset));
        offset_center_r->constant = FALSE;
        offset_center_r->element = ELEMENT_SELF;
        offset_center_r->variable = SIZE_RADIUS;
        offset_center_r->multiplier = 1.0;
        g->to.offsets_r = g_list_append(g->to.offsets_r, offset_center_r);
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
    bzero(g, sizeof(*g));
}
