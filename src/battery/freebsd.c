/**************************************************************************
*
* Tint2 : FreeBSD battery
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

#if defined(__FreeBSD__)

#include <sys/types.h>
#include <sys/sysctl.h>
#include <string.h>

#include "common.h"
#include "battery.h"

gboolean battery_os_init() {
	int sysctl_out = 0;
	size_t len = sizeof(sysctl_out);

	return (sysctlbyname("hw.acpi.battery.state", &sysctl_out, &len, NULL, 0) == 0) ||
			(sysctlbyname("hw.acpi.battery.time", &sysctl_out, &len, NULL, 0) == 0) ||
			(sysctlbyname("hw.acpi.battery.life", &sysctl_out, &len, NULL, 0) == 0);
}

void battery_os_free() {
	return;
}

int battery_os_update(struct batstate *state) {
	int sysctl_out = 0;
	size_t len = sizeof(sysctl_out);
	gboolean err = 0;

	if (sysctlbyname("hw.acpi.battery.state", &sysctl_out, &len, NULL, 0) == 0) {
		switch(sysctl_out) {
		case 1:
			state->state = BATTERY_DISCHARGING;
			break;
		case 2:
			state->state = BATTERY_CHARGING;
			break;
		default:
			state->state = BATTERY_FULL;
			break;
		}
	} else {
		fprintf(stderr, "power update: no such sysctl");
		err = -1;
	}

	if (sysctlbyname("hw.acpi.battery.time", &sysctl_out, &len, NULL, 0) == 0)
		batstate_set_time(state, sysctl_out * 60);
	else
		err = -1;

	if (sysctlbyname("hw.acpi.battery.life", &sysctl_out, &len, NULL, 0) == 0)
		state->percentage = sysctl_out;
	else
		err = -1;
	
	return err;
}

char* battery_os_tooltip() {
	return strdup("Operating System not supported");
}

#endif
