#ifndef THEME_VIEW
#define THEME_VIEW

#include <gtk/gtk.h>

// Some boilerplate GObject type check and type cast macros.
// 'klass' is used here instead of 'class', because 'class' is a c++ keyword

#define CUSTOM_TYPE_CELL_RENDERER_THEME             (custom_cell_renderer_theme_get_type())
#define CUSTOM_CELL_RENDERER_THEME(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),  CUSTOM_TYPE_CELL_RENDERER_THEME, CustomCellRendererTheme))
#define CUSTOM_CELL_RENDERER_THEME_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  CUSTOM_TYPE_CELL_RENDERER_THEME, CustomCellRendererThemeClass))
#define CUSTOM_IS_CELL_PROGRESS_THEME(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUSTOM_TYPE_CELL_RENDERER_THEME))
#define CUSTOM_IS_CELL_PROGRESS_THEME_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  CUSTOM_TYPE_CELL_RENDERER_THEME))
#define CUSTOM_CELL_RENDERER_THEME_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  CUSTOM_TYPE_CELL_RENDERER_THEME, CustomCellRendererThemeClass))

extern GtkListStore *g_store;

enum { COL_TEXT = 0, N_COLUMNS };

typedef struct _CustomCellRendererTheme CustomCellRendererTheme;
typedef struct _CustomCellRendererThemeClass CustomCellRendererThemeClass;


struct _CustomCellRendererTheme
{
	GtkCellRenderer parent;

	gchar	*title;
	gdouble progress;
};


struct _CustomCellRendererThemeClass
{
	GtkCellRendererClass  parent_class;
};


GtkWidget *create_view_and_model(void);

GType  custom_cell_renderer_theme_get_type(void);

void on_changed(GtkWidget *widget, gpointer label);

void add_to_list(GtkWidget *list, const gchar *str);

#endif

