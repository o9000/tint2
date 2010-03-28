
#ifndef THEME_VIEW
#define THEME_VIEW

#include <gtk/gtk.h>


#define CUSTOM_LIST_TYPE  (custom_list_get_type())
#define CUSTOM_LIST(obj)  (G_TYPE_CHECK_INSTANCE_CAST((obj), CUSTOM_LIST_TYPE, CustomList))
#define CUSTOM_LIST_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), CUSTOM_LIST_TYPE, CustomListClass))
#define CUSTOM_IS_LIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUSTOM_LIST_TYPE))
#define CUSTOM_IS_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CUSTOM_LIST_TYPE))
#define CUSTOM_LIST_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  CUSTOM_LIST_TYPE, CustomListClass))

typedef struct _CustomList CustomList;
typedef struct _CustomListClass CustomListClass;


struct _CustomList
{
	GtkCellRenderer parent;

	gdouble  progress;
	gchar  *nameTheme;
	gchar  *nameSnapshot;
	GdkPixbuf  *pixbuf;
};

struct _CustomListClass
{
  GtkCellRendererClass  parent_class;
};


// return the type CustomList
GType custom_list_get_type();

// return a new cell renderer instance
GtkCellRenderer *custom_list_new();


#endif



