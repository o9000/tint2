/**************************************************************************
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
*
* taskbar
*
**************************************************************************/

#ifndef TASKBAR_H
#define TASKBAR_H

#include "task.h"


typedef struct {
   // always start with area
   Area area;

   int desktop;

   // task parameters
   int task_width;
   int task_modulo;
   int text_width;
} Taskbar;



void init_taskbar();
void cleanup_taskbar();

Task *task_get_task (Window win);
void task_refresh_tasklist ();

// return 1 if task_width changed
int resize_tasks (Taskbar *tskbar);

void resize_taskbar(void *panel);


#endif

