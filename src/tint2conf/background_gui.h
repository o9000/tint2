#ifndef BACKGROUND_GUI_H
#define BACKGROUND_GUI_H

#include "gui.h"

void create_background(GtkWidget *parent);
void background_duplicate(GtkWidget *widget, gpointer data);
void background_delete(GtkWidget *widget, gpointer data);
void background_update_image(int index);
void background_update(GtkWidget *widget, gpointer data);
void current_background_changed(GtkWidget *widget, gpointer data);
void background_combo_changed(GtkWidget *widget, gpointer data);
GtkWidget *create_background_combo(const char *label);

#endif
