/**************************************************************************
* config :
* - parse config file in Panel struct.
*
* Check COPYING file for Copyright
*
**************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include <glib.h>

extern char *config_path;
extern char *snapshot_path;

// default global data
void default_config();

// freed memory
void cleanup_config();

gboolean config_read();

#endif
