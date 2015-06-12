/**************************************************************************
* Copyright (C) 2011 Mishael A Sibiryakov (death@junki.org)
**************************************************************************/

#ifndef FREESPACE_H
#define FREESPACE_H

#include "common.h"
#include "area.h"

typedef struct FreeSpace {
	Area area;
} FreeSpace;

void cleanup_freespace();
void init_freespace_panel(void *panel);

int resize_freespace(void *obj);

#endif
