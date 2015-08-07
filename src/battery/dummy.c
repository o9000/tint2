/**************************************************************************
*
* Tint2 : Dummy battery (non-functional)
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

#include <string.h>
#include "common.h"
#include "battery.h"

#warning tint2 has no battery support for this operating system!

gboolean battery_os_init() {
	return FALSE;
}

void battery_os_free() {
	return;
}

int battery_os_update(struct batstate *state) {
	return -1;
}

char* battery_os_tooltip() {
	return strdup("Operating System not supported");
}
