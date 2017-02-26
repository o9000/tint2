#include <limits.h>
#include <stdlib.h>

#include "main.h"
#include "properties.h"
#include "properties_rw.h"
#include "../launcher/apps-common.h"
#include "../launcher/icon-theme-common.h"
#include "../util/common.h"
#include "strnatcmp.h"

#define ROW_SPACING 10
#define COL_SPACING 8
#define DEFAULT_HOR_SPACING 5

gint compare_strings(gconstpointer a, gconstpointer b);
void change_paragraph(GtkWidget *widget);
int get_model_length(GtkTreeModel *model);
void gdkColor2CairoColor(GdkColor color, double *red, double *green, double *blue);
void cairoColor2GdkColor(double red, double green, double blue, GdkColor *color);
