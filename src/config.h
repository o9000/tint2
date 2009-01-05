/**************************************************************************
* config : 
* - parse config file in Panel struct.
*
* Check COPYING file for Copyright
*
**************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

// list of background
GSList *list_back;


int  config_read_file (const char *path);
int  config_read ();
void config_taskbar();
void config_finish ();
void cleanup_taskbar();
void cleanup ();
void save_config ();

#endif
