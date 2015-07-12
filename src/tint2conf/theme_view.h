
#ifndef THEME_VIEW
#define THEME_VIEW

#include <gtk/gtk.h>

extern GtkWidget *g_theme_view;
extern GtkListStore *g_store;
enum { COL_THEME_FILE = 0, COL_THEME_NAME, COL_SNAPSHOT, NB_COL, };

GtkWidget *create_view();

void custom_list_append(const gchar *path);

#endif



