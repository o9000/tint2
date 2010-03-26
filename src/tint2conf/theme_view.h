#ifndef THEME_VIEW
#define THEME_VIEW

#include <gtk/gtk.h>


extern GtkListStore *g_store;

enum { COL_TEXT = 0, COL_PIX, N_COLUMNS };


GtkWidget *create_view(void);

void on_changed(GtkWidget *widget, gpointer label);

void add_to_list(GtkWidget *list, const gchar *str);

#endif

