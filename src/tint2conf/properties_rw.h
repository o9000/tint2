#ifndef PROPERTIES_RW
#define PROPERTIES_RW

#include <gtk/gtk.h>

char *get_current_theme_file_name();
gboolean config_is_manual(const char *path);
void config_read_file (const char *path);
void config_save_file(const char *path);

#endif
