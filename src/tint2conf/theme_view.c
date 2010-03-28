
#include "theme_view.h"


enum { PROP_PERCENTAGE = 1, PROP_THEME, PROP_SNAPSHOT, };

static gpointer parent_class = NULL;

static void custom_list_init(CustomList *cellprogress);
static void custom_list_class_init(CustomListClass *klass);
static void custom_list_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec);
static void custom_list_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec);
static void custom_list_finalize(GObject *gobject);


// These functions are the heart of our custom cell renderer
static void custom_list_get_size(GtkCellRenderer *cell, GtkWidget *widget, GdkRectangle *cell_area, gint *x_offset, gint *y_offset, gint *width, gint *height);
static void custom_list_render(GtkCellRenderer *cell, GdkWindow *window, GtkWidget *widget, GdkRectangle *background_area, GdkRectangle *cell_area, GdkRectangle *expose_area, guint flags);



// register our type with the GObject type system if we haven't done so yet.
// Everything else is done in the callbacks.
GType custom_list_get_type()
{
	static GType type = 0;

	if (type)
		return type;

	if (1) {
		static const GTypeInfo cell_info =
		{
			sizeof (CustomListClass),
			NULL,
			NULL,
			(GClassInitFunc)custom_list_class_init,
			NULL,
			NULL,
			sizeof (CustomList),
			0,
			(GInstanceInitFunc)custom_list_init,
		};

		// Derive from GtkCellRenderer
		type = g_type_register_static(GTK_TYPE_CELL_RENDERER, "CustomList", &cell_info, 0);
	}

	return type;
}


// klass initialisation
// - override the parent's functions that we need to implement.
// - declare our property
// - If you want cells that are editable, you need to override 'activate' and 'start_editing'.
static void custom_list_class_init(CustomListClass *klass)
{
	GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS(klass);
	GObjectClass         *object_class = G_OBJECT_CLASS(klass);

printf("custom_list_class_init : deb\n");
	parent_class           = g_type_class_peek_parent (klass);

	cell_class->render   = custom_list_render;
	cell_class->get_size = custom_list_get_size;
	object_class->get_property = custom_list_get_property;
	object_class->set_property = custom_list_set_property;
	object_class->finalize = custom_list_finalize;

	// Install our very own properties
	g_object_class_install_property(object_class, PROP_PERCENTAGE, g_param_spec_double("percentage", "Percentage", "The fractional progress to display", 0.0, 1.0, 0.0, G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_THEME, g_param_spec_string("theme", "Theme", "Theme file name", NULL, G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_SNAPSHOT, g_param_spec_string("snapshot", "Snapshot", "Snapshot file name", NULL, G_PARAM_READWRITE));
}


// CustomList renderer initialisation
static void custom_list_init(CustomList *custom_list)
{
	printf("custom_list_init : deb\n");
	// set some default properties of the parent (GtkCellRenderer).
	GTK_CELL_RENDERER(custom_list)->mode = GTK_CELL_RENDERER_MODE_INERT;
	GTK_CELL_RENDERER(custom_list)->xpad = 2;
	GTK_CELL_RENDERER(custom_list)->ypad = 2;
}


static void custom_list_finalize(GObject *object)
{
/*
  CustomList *cellrendererprogress = CUSTOM_LIST(object);
*/

printf("custom_list_finalize\n");
	// Free any dynamically allocated resources here

	(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}


static void custom_list_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
{
	CustomList *custom_list = CUSTOM_LIST(object);

	switch (param_id) {
		case PROP_PERCENTAGE:
			printf("custom_list_set_property : PROP_PERCENTAGE\n");
			custom_list->progress = g_value_get_double(value);
			break;

		case PROP_THEME:
			printf("custom_list_set_property : PROP_THEME\n");
			//if (custom_list->nameTheme) g_free(custom_list->nameTheme);
			custom_list->nameTheme = g_strdup(g_value_get_string(value));
			break;

		case PROP_SNAPSHOT:
			printf("custom_list_set_property : PROP_SNAPSHOT\n");
			custom_list->nameSnapshot = g_strdup(g_value_get_string(value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
			break;
	}
}


static void custom_list_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *psec)
{
	CustomList *custom_list = CUSTOM_LIST(object);

	switch (param_id) {
		case PROP_PERCENTAGE:
			printf("custom_list_get_property : PROP_PERCENTAGE\n");
			g_value_set_double(value, custom_list->progress);
			break;

		case PROP_THEME:
			printf("custom_list_get_property : PROP_THEME\n");
			g_value_set_string(value, g_strdup(custom_list->nameTheme));
			break;

		case PROP_SNAPSHOT:
			printf("custom_list_get_property : PROP_SNAPSHOT\n");
			g_value_set_string(value, g_strdup(custom_list->nameSnapshot));
			break;
			//g_value_set_object (value, (GObject *)priv->image);

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, psec);
			break;
	}
}


GtkCellRenderer *custom_list_new()
{
	return g_object_new(CUSTOM_LIST_TYPE, NULL);
}


/***************************************************************************
 *
 *  custom_cell_renderer_progress_get_size: crucial - calculate the size
 *                                          of our cell, taking into account
 *                                          padding and alignment properties
 *                                          of parent.
 *
 ***************************************************************************/

#define FIXED_WIDTH   100
#define FIXED_HEIGHT  20

static void custom_list_get_size(GtkCellRenderer *cell, GtkWidget *widget, GdkRectangle *cell_area, gint *x_offset, gint *y_offset, gint *width, gint *height)
{
	gint calc_width;
	gint calc_height;

	calc_width  = (gint) cell->xpad * 2 + FIXED_WIDTH;
	calc_height = (gint) cell->ypad * 2 + FIXED_HEIGHT;

	if (width)
		*width = calc_width;

	if (height)
		*height = calc_height;

	if (cell_area) {
		if (x_offset) {
			*x_offset = cell->xalign * (cell_area->width - calc_width);
			*x_offset = MAX (*x_offset, 0);
		}

		if (y_offset) {
			*y_offset = cell->yalign * (cell_area->height - calc_height);
			*y_offset = MAX (*y_offset, 0);
		}
	}
}


/***************************************************************************
 *
 *  custom_cell_renderer_progress_render: crucial - do the rendering.
 *
 ***************************************************************************/

static void custom_list_render(GtkCellRenderer *cell, GdkWindow *window, GtkWidget *widget, GdkRectangle *background_area, GdkRectangle *cell_area, GdkRectangle *expose_area, guint flags)
{
	CustomList *custom_list = CUSTOM_LIST(cell);
	GtkStateType state;
	gint width, height;
	gint x_offset, y_offset;

	//printf("custom_list_render\n");
	custom_list_get_size (cell, widget, cell_area, &x_offset, &y_offset, &width, &height);

	if (GTK_WIDGET_HAS_FOCUS (widget))
		state = GTK_STATE_ACTIVE;
	else
		state = GTK_STATE_NORMAL;

	width  -= cell->xpad*2;
	height -= cell->ypad*2;

	gtk_paint_box (widget->style, window, GTK_STATE_NORMAL, GTK_SHADOW_IN, NULL, widget, "trough", cell_area->x + x_offset + cell->xpad, cell_area->y + y_offset + cell->ypad, width - 1, height - 1);

	gtk_paint_box (widget->style, window, state, GTK_SHADOW_OUT, NULL, widget, "bar", cell_area->x + x_offset + cell->xpad, cell_area->y + y_offset + cell->ypad, width * custom_list->progress, height - 1);
}



