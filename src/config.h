/**************************************************************************
* config :
* - parse config file in Panel struct.
*
* Check COPYING file for Copyright
*
**************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

extern char *config_path;
extern char *snapshot_path;

// default global data
void default_config();

// freed memory
void cleanup_config();

int  config_read_file (const char *path);
int  config_read ();

#endif

