#ifndef GRADIENT_H
#define GRADIENT_H

#include <glib.h>
#include <cairo.h>

#include "color.h"

//////////////////////////////////////////////////////////////////////
// Gradient types read from config options, not associated to any area

typedef enum GradientType { GRADIENT_VERTICAL = 0, GRADIENT_HORIZONTAL, GRADIENT_CENTERED } GradientType;

typedef struct ColorStop {
    Color color;
    // offset in 0-1
    double offset;
} ColorStop;

typedef enum Element { ELEMENT_SELF = 0, ELEMENT_PARENT, ELEMENT_PANEL } Element;

typedef enum SizeVariable {
    SIZE_WIDTH = 0,
    SIZE_HEIGHT,
    SIZE_RADIUS,
    SIZE_LEFT,
    SIZE_RIGHT,
    SIZE_TOP,
    SIZE_BOTTOM,
    SIZE_CENTERX,
    SIZE_CENTERY
} SizeVariable;

typedef struct Offset {
    gboolean constant;
    // if constant == true
    double constant_value;
    // else
    Element element;
    SizeVariable variable;
    double multiplier;
} Offset;

typedef struct ControlPoint {
    // Each element is an Offset
    GList *offsets_x;
    GList *offsets_y;
    // Defined only for radial gradients
    GList *offsets_r;
} ControlPoint;

typedef struct GradientClass {
    GradientType type;
    Color start_color;
    Color end_color;
    // Each element is a ColorStop
    GList *extra_color_stops;
    ControlPoint from;
    ControlPoint to;
} GradientClass;

GradientType gradient_type_from_string(const char *str);
void init_gradient(GradientClass *g, GradientType type);
void cleanup_gradient(GradientClass *g);

/////////////////////////////////////////
// Gradient instances associated to Areas

struct Area;
typedef struct Area Area;

typedef struct GradientInstance {
    GradientClass *gradient_class;
    Area *area;
    cairo_pattern_t *pattern;
} GradientInstance;

extern gboolean debug_gradients;

#endif // GRADIENT_H
