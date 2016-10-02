#ifndef GRADIENT_H
#define GRADIENT_H

#include <glib.h>
#include <cairo.h>

#include "color.h"

//////////////////////////////////////////////////////////////////////
// Gradient types read from config options, not associated to any area

typedef enum GradientType {
	GRADIENT_VERTICAL = 0,
	GRADIENT_HORIZONTAL,
	GRADIENT_CENTERED,
	GRADIENT_LINEAR,
	GRADIENT_RADIAL
} GradientType;

typedef struct ColorStop {
	Color color;
	// offset in 0-1
	double offset;
} ColorStop;

typedef enum Origin {
	ORIGIN_ELEMENT = 0,
	ORIGIN_PARENT,
	ORIGIN_PANEL,
	ORIGIN_SCREEN,
	ORIGIN_DESKTOP
} Origin;

typedef enum SizeVariable {
	SIZE_WIDTH = 0,
	SIZE_HEIGHT,
	SIZE_LEFT,
	SIZE_RIGHT,
	SIZE_TOP,
	SIZE_BOTTOM,
	SIZE_CENTER,
	SIZE_RADIUS
} SizeVariable;

typedef struct Offset {
	gboolean constant;
	// if constant == true
	double constant_value;
	// else
	Origin variable_element;
	SizeVariable variable;
	double multiplier;
} Offset;

typedef struct ControlPoint {
	Origin origin;
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
Origin origin_from_string(const char *str);
Offset *offset_from_string(const char *str);
void init_gradient(GradientClass *g, GradientType type);
void cleanup_gradient(GradientClass *g);

/////////////////////////////////////////
// Gradient instances associated to Areas

struct Area;
typedef struct Area Area;

typedef struct OffsetInstance {
	gboolean constant;
	// if constant == true
	double constant_value;
	// else
	Area *variable_element;
	SizeVariable variable;
	double multiplier;
} OffsetInstance;

typedef struct ControlPointInstance {
	Area *origin;
	// Each element is an OffsetInstance
	GList *offsets_x;
	GList *offsets_y;
	GList *offsets_r;
} ControlPointInstance;

typedef struct GradientInstance {
	GradientClass *gradient_class;
	Area *area;
	ControlPointInstance from;
	ControlPointInstance to;
	cairo_pattern_t *pattern;
	// Each element is an Area whose geometry is used to compute this gradient
	// TODO why do we need it?
	GList *gradient_dependencies;
} GradientInstance;

#endif // GRADIENT_H
