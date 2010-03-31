
#ifndef THEME_VIEW
#define THEME_VIEW

#include <gtk/gtk.h>

enum { COL_THEME_FILE = 0, COL_SNAPSHOT, NB_COL, };

GtkWidget *create_view();

void custom_list_append(const gchar *name);


#endif



