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

#if defined(__FreeBSD__) || defined(__DragonFly__)

#include <sys/types.h>
#include <sys/sysctl.h>
#include <string.h>

#include "common.h"
#include "battery.h"

gboolean battery_os_init()
{
	int sysctl_out = 0;
	size_t len = sizeof(sysctl_out);

	return (sysctlbyname("hw.acpi.battery.state", &sysctl_out, &len, NULL, 0) == 0) ||
		   (sysctlbyname("hw.acpi.battery.time", &sysctl_out, &len, NULL, 0) == 0) ||
		   (sysctlbyname("hw.acpi.battery.life", &sysctl_out, &len, NULL, 0) == 0);
}

void battery_os_free()
{
	return;
}

int battery_os_update(BatteryState *state)
{
	int sysctl_out = 0;
	size_t len = sizeof(sysctl_out);
	gboolean err = 0;

	if (sysctlbyname("hw.acpi.battery.state", &sysctl_out, &len, NULL, 0) == 0) {
		switch (sysctl_out) {
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
		battery_state_set_time(state, sysctl_out * 60);
	else
		err = -1;

	if (sysctlbyname("hw.acpi.battery.life", &sysctl_out, &len, NULL, 0) == 0)
		state->percentage = sysctl_out;
	else
		err = -1;

	if (sysctlbyname("hw.acpi.acline", &sysctl_out, &len, NULL, 0) == 0)
		state->ac_connected = sysctl_out;

	return err;
}

char *battery_os_tooltip()
{
	GString *tooltip = g_string_new("");
	gchar *result;

	g_string_append_printf(tooltip, "Battery\n");

	gchar *state = (battery_state.state == BATTERY_UNKNOWN) ? "Level" : chargestate2str(battery_state.state);

	g_string_append_printf(tooltip, "\t%s: %d%%", state, battery_state.percentage);

	g_string_append_c(tooltip, '\n');
	g_string_append_printf(tooltip, "AC\n");
	g_string_append_printf(tooltip, battery_state.ac_connected ? "\tConnected" : "\tDisconnected");

	result = tooltip->str;
	g_string_free(tooltip, FALSE);

	return result;
}

#endif
