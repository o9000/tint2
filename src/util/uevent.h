/**************************************************************************
*
* Linux Kernel uevent handler
*
* Copyright (C) 2015 Sebastian Reichel <sre@ring0.de>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* or any later version as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************/

#ifndef UEVENT_H
#define UEVENT_H

#include <glib.h>

enum uevent_action {
    UEVENT_UNKNOWN = 0x01,
    UEVENT_ADD = 0x02,
    UEVENT_REMOVE = 0x04,
    UEVENT_CHANGE = 0x08,
};

struct uevent_parameter {
    char *key;
    char *val;
};

struct uevent {
    char *path;
    enum uevent_action action;
    int sequence;
    char *subsystem;
    GList *params;
};

struct uevent_notify {
    int action;      /* bitfield */
    char *subsystem; /* NULL => any */
    void *userdata;

    void (*cb)(struct uevent *e, void *userdata);
};

extern int uevent_fd;

#if ENABLE_UEVENT
int uevent_init();
void uevent_cleanup();
void uevent_handler();

void uevent_register_notifier(struct uevent_notify *nb);
void uevent_unregister_notifier(struct uevent_notify *nb);
#else
static inline int uevent_init()
{
    return -1;
}

static inline void uevent_cleanup()
{
}

static inline void uevent_handler()
{
}

static inline void uevent_register_notifier(struct uevent_notify *nb)
{
}

static inline void uevent_unregister_notifier(struct uevent_notify *nb)
{
}
#endif

#endif
