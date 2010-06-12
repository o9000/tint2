
#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "properties.h"


GtkWidget *create_properties()
{
	GtkWidget  *view;
	GtkTooltips *tooltips;
	GtkWidget *dialog_vbox3;
	GtkWidget *notebook2;
	GtkWidget *notebook5;

	tooltips = gtk_tooltips_new ();

	view = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (view), "Preferences");
	gtk_window_set_modal (GTK_WINDOW (view), TRUE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (view), TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (view), GDK_WINDOW_TYPE_HINT_DIALOG);

	dialog_vbox3 = GTK_DIALOG (view)->vbox;
	gtk_widget_show (dialog_vbox3);

	notebook2 = gtk_notebook_new ();
	gtk_widget_show (notebook2);
	gtk_box_pack_start (GTK_BOX (dialog_vbox3), notebook2, TRUE, TRUE, 6);
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook2), GTK_POS_LEFT);

	notebook5 = gtk_notebook_new ();
	gtk_widget_show (notebook5);
	gtk_container_add (GTK_CONTAINER (notebook2), notebook5);

printf("create_properties : fin\n");
	return view;
}
