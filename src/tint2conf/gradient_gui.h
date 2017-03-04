#ifndef GRADIENT_GUI_H
#define GRADIENT_GUI_H

#include "gui.h"

int gradient_index_safe(int index);
void create_gradient(GtkWidget *parent);
GtkWidget *create_gradient_combo();
void gradient_duplicate(GtkWidget *widget, gpointer data);
void gradient_delete(GtkWidget *widget, gpointer data);
void gradient_update_image(int index);
void gradient_update(GtkWidget *widget, gpointer data);
void gradient_force_update();
void current_gradient_changed(GtkWidget *widget, gpointer data);
void background_update_for_gradient(int gradient_id);

typedef enum GradientConfigType {
	GRADIENT_CONFIG_VERTICAL = 0,
	GRADIENT_CONFIG_HORIZONTAL,
	GRADIENT_CONFIG_RADIAL
} GradientConfigType;

typedef struct GradientConfigColorStop {
	Color color;
	// offset in 0-1
	double offset;
} GradientConfigColorStop;

typedef struct GradientConfig {
	GradientConfigType type;
	GradientConfigColorStop start_color;
	GradientConfigColorStop end_color;
	// Each element is a GradientConfigColorStop
	GList *extra_color_stops;
} GradientConfig;

void gradient_create_new(GradientConfigType t);
void gradient_draw(cairo_t *c, GradientConfig *g, int w, int h, gboolean preserve);

#endif
