
#include "theme_view.h"


GtkListStore *g_store;

// Some boring function declarations: GObject type system stuff
static void custom_cell_renderer_theme_init(CustomCellRendererTheme *cellprogress);
static void custom_cell_renderer_theme_class_init(CustomCellRendererThemeClass *klass);
static void custom_cell_renderer_theme_finalize(GObject *gobject);
static void custom_cell_renderer_theme_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec);
static void custom_cell_renderer_theme_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec);


// These functions are the heart of our custom cell renderer:
static void custom_cell_renderer_theme_get_size(GtkCellRenderer *cell, GtkWidget *widget, GdkRectangle *cell_area, gint *x_offset, gint *y_offset, gint *width, gint *height);
static void custom_cell_renderer_theme_render(GtkCellRenderer *cell, GdkWindow *window, GtkWidget *widget, GdkRectangle *background_area, GdkRectangle *cell_area, GdkRectangle *expose_area, guint flags);


static gpointer parent_class;

enum { PROP_TITLE = 1, };




GtkWidget *create_view_and_model(void)
{
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget  *view;
	GtkTreeSelection *sel;
	GtkTreeIter iter;

	g_store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING);
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(g_store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

	g_object_unref(g_store); // destroy store automatically with view
/*
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Deb", renderer, "title", COL_TEXT, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
*/
	renderer = g_object_new(CUSTOM_TYPE_CELL_RENDERER_THEME, NULL);
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "title", COL_TEXT);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(sel), GTK_SELECTION_SINGLE);
	g_signal_connect(sel, "changed",	G_CALLBACK(on_changed), NULL);

	return view;
}


void on_changed(GtkWidget *widget, gpointer label)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	char *value;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
		gtk_tree_model_get(model, &iter, COL_TEXT, &value,  -1);
		//gtk_label_set_text(GTK_LABEL(label), value);
		g_free(value);
	}

}


void add_to_list(GtkWidget *list, const gchar *str)
{
	GtkTreeIter iter;

	gtk_list_store_append(g_store, &iter);
	gtk_list_store_set(g_store, &iter, COL_TEXT, str, -1);
	//gtk_list_store_set (g_store, &iter, COL_TEXT, buf, -1);
}


/***************************************************************************
*  custom_cell_renderer_theme_get_type: here we register our type with
*                                          the GObject type system if we
*                                          haven't done so yet. Everything
*                                          else is done in the callbacks.
***************************************************************************/

GType custom_cell_renderer_theme_get_type (void)
{
	static GType cell_type = 0;

	if (cell_type == 0) {
		static const GTypeInfo cell_info =
		{
		sizeof (CustomCellRendererThemeClass),
		NULL,                                                     // base_init
		NULL,                                                     // base_finalize
		(GClassInitFunc) custom_cell_renderer_theme_class_init,
		NULL,                                                     // class_finalize
		NULL,                                                     // class_data
		sizeof (CustomCellRendererTheme),
		0,                                                        // n_preallocs
		(GInstanceInitFunc) custom_cell_renderer_theme_init,
		};

		// Derive from GtkCellRenderer
		cell_type = g_type_register_static(GTK_TYPE_CELL_RENDERER, "CustomCellRendererTheme", &cell_info, 0);
	}

	return cell_type;
}


static void custom_cell_renderer_theme_init(CustomCellRendererTheme *celltheme)
{
	// set some default properties
	GTK_CELL_RENDERER(celltheme)->mode = GTK_CELL_RENDERER_MODE_INERT;
	GTK_CELL_RENDERER(celltheme)->xpad = 2;
	GTK_CELL_RENDERER(celltheme)->ypad = 2;
	celltheme->title = 0;
	printf("custom_cell_renderer_theme_init\n\n");

}


/***************************************************************************
*  custom_cell_renderer_theme_class_init:
*
*  set up our own get_property and set_property functions, and
*  override the parent's functions that we need to implement.
*  And make our new "percentage" property known to the type system.
*  If you want cells that can be activated on their own (ie. not
*  just the whole row selected) or cells that are editable, you
*  will need to override 'activate' and 'start_editing' as well.
***************************************************************************/

static void custom_cell_renderer_theme_class_init(CustomCellRendererThemeClass *klass)
{
	GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS(klass);
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = custom_cell_renderer_theme_finalize;

	object_class->get_property = custom_cell_renderer_theme_get_property;
	object_class->set_property = custom_cell_renderer_theme_set_property;

	// Override the two crucial functions that are the heart of a cell renderer in the parent class
	cell_class->get_size = custom_cell_renderer_theme_get_size;
	cell_class->render = custom_cell_renderer_theme_render;

printf("custom_class_init\n\n");

	// Install our very own properties
	g_object_class_install_property (object_class, PROP_TITLE, g_param_spec_string("title", "Title", "Theme's title", 0, G_PARAM_READWRITE));
//	g_object_class_install_property (object_class, PROP_PERCENTAGE, g_param_spec_double ("percentage", "Percentage", "The fractional progress to display", 0, 1, 0, G_PARAM_READWRITE));
}


static void custom_cell_renderer_theme_finalize(GObject *object)
{
	CustomCellRendererTheme *celltheme = CUSTOM_CELL_RENDERER_THEME(object);

	// free any resources here
	//if (celltheme->title)
	//g_free(celltheme->title);

	// Free any dynamically allocated resources here
	(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}


static void custom_cell_renderer_theme_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *psec)
{
	CustomCellRendererTheme  *celltheme = CUSTOM_CELL_RENDERER_THEME(object);

	switch (param_id) {
		case PROP_TITLE:
		g_value_set_string(value, celltheme->title);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, psec);
		break;
	}
}


static void custom_cell_renderer_theme_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
{
	CustomCellRendererTheme *cellprogress = CUSTOM_CELL_RENDERER_THEME(object);

	switch (param_id) {
		case PROP_TITLE:
//printf("set_property**************************************\n");
		cellprogress->title = g_value_get_string(value);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
		break;
	}
}


#define FIXED_WIDTH   100
#define FIXED_HEIGHT  20

static void custom_cell_renderer_theme_get_size(GtkCellRenderer *cell, GtkWidget *widget, GdkRectangle *cell_area, gint *x_offset, gint *y_offset, gint *width, gint *height)
{
	gint calc_width;
	gint calc_height;

	// calculate the size of our cell, taking into account
	// padding and alignment properties of parent.
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


static void custom_cell_renderer_theme_render(GtkCellRenderer *cell, GdkWindow *window, GtkWidget *widget, GdkRectangle *background_area, GdkRectangle *cell_area, GdkRectangle *expose_area, guint flags)
{
	CustomCellRendererTheme *celltheme = CUSTOM_CELL_RENDERER_THEME(cell);
	GtkStateType  state;
	gint width, height, x_offset, y_offset;
	PangoLayout *layout;

	// do the rendering.
	custom_cell_renderer_theme_get_size(cell, widget, cell_area, &x_offset, &y_offset,	&width, &height);

	if (GTK_WIDGET_HAS_FOCUS (widget))
		state = GTK_STATE_ACTIVE;
	else
		state = GTK_STATE_NORMAL;

	width  -= cell->xpad*2;
	height -= cell->ypad*2;

	layout = gtk_widget_create_pango_layout(widget, "");
	pango_layout_set_text(layout, celltheme->title, strlen(celltheme->title));
	gtk_paint_layout (widget->style, widget->window, state, FALSE, NULL, widget, NULL, x_offset, y_offset, layout);
	//pango_layout_get_size (layout, &width, &height);
	g_object_unref (layout);

	//gtk_paint_layout
	//gtk_paint_box (widget->style, window, GTK_STATE_NORMAL, GTK_SHADOW_IN, NULL, widget, "title", cell_area->x + x_offset + cell->xpad, cell_area->y + y_offset + cell->ypad, width - 1, height - 1);

printf("custom_cell_renderer_theme_render\n");
	//gtk_paint_box (widget->style, window, state, GTK_SHADOW_OUT, NULL, widget, "bar", cell_area->x + x_offset + cell->xpad, cell_area->y + y_offset + cell->ypad, width * cellprogress->progress, height - 1);
}

