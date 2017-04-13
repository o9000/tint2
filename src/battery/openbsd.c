/**************************************************************************
*
* Tint2 : OpenBSD & NetBSD battery
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

#if defined(__OpenBSD__) || defined(__NetBSD__)

#include <stdint.h>
#include <err.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <machine/apmvar.h>

#include "common.h"
#include "battery.h"

int apm_fd = -1;

gboolean battery_os_init()
{
    if (apm_fd > 0)
        close(apm_fd);

    apm_fd = open("/dev/apm", O_RDONLY);

    if (apm_fd < 0) {
        warn("ERROR: battery applet cannot open /dev/apm.");
        return FALSE;
    } else {
        return TRUE;
    }
}

void battery_os_free()
{
    if ((apm_fd != -1) && (close(apm_fd) == -1))
        warn("cannot close /dev/apm");
    apm_fd = -1;
}

int battery_os_update(BatteryState *state)
{
    struct apm_power_info info;

    if (apm_fd > 0 && ioctl(apm_fd, APM_IOC_GETPOWER, &(info)) == 0) {
        // best attempt at mapping to Linux battery states
        switch (info.battery_state) {
        case APM_BATT_CHARGING:
            state->state = BATTERY_CHARGING;
            break;
        default:
            state->state = BATTERY_DISCHARGING;
            break;
        }

        if (info.battery_life > 100)
            info.battery_life = 100;
        if (info.battery_life == 100)
            state->state = BATTERY_FULL;

        state->percentage = info.battery_life;
        if (info.minutes_left != -1)
            battery_state_set_time(state, info.minutes_left * 60);

        state->ac_connected = info.ac_state == APM_AC_ON;
    } else {
        warn("power update: APM_IOC_GETPOWER");
        return -1;
    }

    return 0;
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
