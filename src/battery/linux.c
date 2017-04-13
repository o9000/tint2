/**************************************************************************
*
* Tint2 : Linux battery
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

#ifdef __linux__

#include <stdlib.h>

#include "common.h"
#include "battery.h"
#include "uevent.h"

enum psy_type {
    PSY_UNKNOWN,
    PSY_BATTERY,
    PSY_MAINS,
};

struct psy_battery {
    /* generic properties */
    gchar *name;
    /* monotonic time, in microseconds */
    gint64 timestamp;
    /* sysfs files */
    gchar *path_present;
    gchar *path_level_now;
    gchar *path_level_full;
    gchar *path_rate_now;
    gchar *path_status;
    /* values */
    gboolean present;
    gint level_now;
    gint level_full;
    gint rate_now;
    gchar unit;
    ChargeState status;
};

struct psy_mains {
    /* generic properties */
    gchar *name;
    /* sysfs files */
    gchar *path_online;
    /* values */
    gboolean online;
};

static void uevent_battery_update()
{
    update_battery_tick(NULL);
}
static struct uevent_notify psy_change = {UEVENT_CHANGE, "power_supply", NULL, uevent_battery_update};

static void uevent_battery_plug()
{
    printf("reinitialize batteries after HW change\n");
    reinit_battery();
}
static struct uevent_notify psy_plug = {UEVENT_ADD | UEVENT_REMOVE, "power_supply", NULL, uevent_battery_plug};

#define RETURN_ON_ERROR(err)                                                 \
    if (err) {                                                               \
        g_error_free(err);                                                   \
        fprintf(stderr, RED "%s:%d: errror" RESET "\n", __FILE__, __LINE__); \
        return FALSE;                                                        \
    }

static GList *batteries = NULL;
static GList *mains = NULL;

static guint8 level_to_percent(gint level_now, gint level_full)
{
    return 0.5 + ((level_now <= level_full ? level_now : level_full) * 100.0) / level_full;
}

static enum psy_type power_supply_get_type(const gchar *entryname)
{
    gchar *path_type = g_build_filename(battery_sys_prefix, "/sys/class/power_supply", entryname, "type", NULL);
    GError *error = NULL;
    gchar *type;
    gsize typelen;

    g_file_get_contents(path_type, &type, &typelen, &error);
    g_free(path_type);
    if (error) {
        fprintf(stderr, RED "%s:%d: read failed" RESET "\n", __FILE__, __LINE__);
        g_error_free(error);
        return PSY_UNKNOWN;
    }

    if (!g_strcmp0(type, "Battery\n")) {
        g_free(type);
        return PSY_BATTERY;
    }

    if (!g_strcmp0(type, "Mains\n")) {
        g_free(type);
        return PSY_MAINS;
    }

    g_free(type);

    return PSY_UNKNOWN;
}

static gboolean init_linux_battery(struct psy_battery *bat)
{
    const gchar *entryname = bat->name;

    bat->path_present = g_build_filename(battery_sys_prefix, "/sys/class/power_supply", entryname, "present", NULL);
    if (!g_file_test(bat->path_present, G_FILE_TEST_EXISTS)) {
        fprintf(stderr, RED "%s:%d: read failed" RESET "\n", __FILE__, __LINE__);
        goto err0;
    }

    bat->path_level_now =
        g_build_filename(battery_sys_prefix, "/sys/class/power_supply", entryname, "energy_now", NULL);
    bat->path_level_full =
        g_build_filename(battery_sys_prefix, "/sys/class/power_supply", entryname, "energy_full", NULL);
    bat->path_rate_now = g_build_filename(battery_sys_prefix, "/sys/class/power_supply", entryname, "power_now", NULL);
    bat->unit = 'W';

    if (!g_file_test(bat->path_level_now, G_FILE_TEST_EXISTS) ||
        !g_file_test(bat->path_level_full, G_FILE_TEST_EXISTS) ||
        !g_file_test(bat->path_rate_now, G_FILE_TEST_EXISTS)) {
        g_free(bat->path_level_now);
        g_free(bat->path_level_full);
        g_free(bat->path_rate_now);
        bat->path_level_now =
            g_build_filename(battery_sys_prefix, "/sys/class/power_supply", entryname, "charge_now", NULL);
        bat->path_level_full =
            g_build_filename(battery_sys_prefix, "/sys/class/power_supply", entryname, "charge_full", NULL);
        bat->path_rate_now =
            g_build_filename(battery_sys_prefix, "/sys/class/power_supply", entryname, "current_now", NULL);
        bat->unit = 'A';
    }
    if (!g_file_test(bat->path_level_now, G_FILE_TEST_EXISTS) ||
        !g_file_test(bat->path_level_full, G_FILE_TEST_EXISTS) ||
        !g_file_test(bat->path_rate_now, G_FILE_TEST_EXISTS)) {
        fprintf(stderr, RED "%s:%d: read failed" RESET "\n", __FILE__, __LINE__);
        goto err1;
    }

    bat->path_status = g_build_filename(battery_sys_prefix, "/sys/class/power_supply", entryname, "status", NULL);
    if (!g_file_test(bat->path_status, G_FILE_TEST_EXISTS)) {
        fprintf(stderr, RED "%s:%d: read failed" RESET "\n", __FILE__, __LINE__);
        goto err2;
    }

    return TRUE;

err2:
    g_free(bat->path_status);
err1:
    g_free(bat->path_level_now);
    g_free(bat->path_level_full);
    g_free(bat->path_rate_now);
err0:
    g_free(bat->path_present);

    return FALSE;
}

static gboolean init_linux_mains(struct psy_mains *ac)
{
    const gchar *entryname = ac->name;

    ac->path_online = g_build_filename(battery_sys_prefix, "/sys/class/power_supply", entryname, "online", NULL);
    if (!g_file_test(ac->path_online, G_FILE_TEST_EXISTS)) {
        fprintf(stderr, RED "%s:%d: read failed" RESET "\n", __FILE__, __LINE__);
        g_free(ac->path_online);
        return FALSE;
    }

    return TRUE;
}

static void psy_battery_free(gpointer data)
{
    struct psy_battery *bat = data;
    g_free(bat->name);
    g_free(bat->path_status);
    g_free(bat->path_rate_now);
    g_free(bat->path_level_full);
    g_free(bat->path_level_now);
    g_free(bat->path_present);
    g_free(bat);
}

static void psy_mains_free(gpointer data)
{
    struct psy_mains *ac = data;
    g_free(ac->name);
    g_free(ac->path_online);
    g_free(ac);
}

void battery_os_free()
{
    uevent_unregister_notifier(&psy_change);
    uevent_unregister_notifier(&psy_plug);

    g_list_free_full(batteries, psy_battery_free);
    batteries = NULL;
    g_list_free_full(mains, psy_mains_free);
    mains = NULL;
}

static void add_battery(const char *entryname)
{
    struct psy_battery *bat = g_malloc0(sizeof(*bat));
    bat->name = g_strdup(entryname);

    if (init_linux_battery(bat)) {
        batteries = g_list_append(batteries, bat);
        fprintf(stdout, GREEN "Found battery \"%s\"" RESET "\n", bat->name);
    } else {
        g_free(bat);
        fprintf(stderr, RED "Failed to initialize battery \"%s\"" RESET "\n", entryname);
    }
}

static void add_mains(const char *entryname)
{
    struct psy_mains *ac = g_malloc0(sizeof(*ac));
    ac->name = g_strdup(entryname);

    if (init_linux_mains(ac)) {
        mains = g_list_append(mains, ac);
        fprintf(stdout, GREEN "Found mains \"%s\"" RESET "\n", ac->name);
    } else {
        g_free(ac);
        fprintf(stderr, RED "Failed to initialize mains \"%s\"" RESET "\n", entryname);
    }
}

gboolean battery_os_init()
{
    GDir *directory = 0;
    GError *error = NULL;
    const char *entryname;

    battery_os_free();

    gchar *dir_path = g_build_filename(battery_sys_prefix, "/sys/class/power_supply", NULL);
    directory = g_dir_open(dir_path, 0, &error);
    g_free(dir_path);
    RETURN_ON_ERROR(error);

    while ((entryname = g_dir_read_name(directory))) {
        fprintf(stderr, GREEN "Found power device %s" RESET "\n", entryname);
        enum psy_type type = power_supply_get_type(entryname);

        switch (type) {
        case PSY_BATTERY:
            add_battery(entryname);
            break;
        case PSY_MAINS:
            add_mains(entryname);
            break;
        default:
            break;
        }
    }

    g_dir_close(directory);

    uevent_register_notifier(&psy_change);
    uevent_register_notifier(&psy_plug);

    return batteries != NULL;
}

static gint estimate_rate_usage(struct psy_battery *bat, gint old_level_now, gint64 old_timestamp)
{
    gint64 diff_level = ABS(bat->level_now - old_level_now);
    gint64 diff_time = bat->timestamp - old_timestamp;

    /* µW = (µWh * 3600) / (µs / 1000000) */
    gint rate = diff_level * 3600 * 1000000 / MAX(1, diff_time);

    return rate;
}

static gboolean update_linux_battery(struct psy_battery *bat)
{
    GError *error = NULL;
    gchar *data;
    gsize datalen;

    gint64 old_timestamp = bat->timestamp;
    int old_level_now = bat->level_now;
    gint old_rate_now = bat->rate_now;

    /* reset values */
    bat->present = 0;
    bat->status = BATTERY_UNKNOWN;
    bat->level_now = 0;
    bat->level_full = 0;
    bat->rate_now = 0;
    bat->timestamp = g_get_monotonic_time();

    /* present */
    g_file_get_contents(bat->path_present, &data, &datalen, &error);
    RETURN_ON_ERROR(error);
    bat->present = (atoi(data) == 1);
    g_free(data);

    /* we are done, if battery is not present */
    if (!bat->present)
        return TRUE;

    /* status */
    bat->status = BATTERY_UNKNOWN;
    g_file_get_contents(bat->path_status, &data, &datalen, &error);
    RETURN_ON_ERROR(error);
    if (!g_strcmp0(data, "Charging\n")) {
        bat->status = BATTERY_CHARGING;
    } else if (!g_strcmp0(data, "Discharging\n")) {
        bat->status = BATTERY_DISCHARGING;
    } else if (!g_strcmp0(data, "Full\n")) {
        bat->status = BATTERY_FULL;
    }
    g_free(data);

    /* level now */
    g_file_get_contents(bat->path_level_now, &data, &datalen, &error);
    RETURN_ON_ERROR(error);
    bat->level_now = atoi(data);
    g_free(data);

    /* level full */
    g_file_get_contents(bat->path_level_full, &data, &datalen, &error);
    RETURN_ON_ERROR(error);
    bat->level_full = atoi(data);
    g_free(data);

    /* rate now */
    g_file_get_contents(bat->path_rate_now, &data, &datalen, &error);
    if (g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NODEV)) {
        /* some hardware does not support reading current rate consumption */
        g_error_free(error);
        bat->rate_now = estimate_rate_usage(bat, old_level_now, old_timestamp);
        if (bat->rate_now == 0 && bat->status != BATTERY_FULL) {
            /* If the hardware updates the level slower than our sampling period,
             * we need to sample more rarely */
            bat->rate_now = old_rate_now;
            bat->timestamp = old_timestamp;
        }
    } else if (error) {
        g_error_free(error);
        return FALSE;
    } else {
        bat->rate_now = atoi(data);
        g_free(data);
    }

    return TRUE;
}

static gboolean update_linux_mains(struct psy_mains *ac)
{
    GError *error = NULL;
    gchar *data;
    gsize datalen;
    ac->online = FALSE;

    /* online */
    g_file_get_contents(ac->path_online, &data, &datalen, &error);
    RETURN_ON_ERROR(error);
    ac->online = (atoi(data) == 1);
    g_free(data);

    return TRUE;
}

int battery_os_update(BatteryState *state)
{
    GList *l;

    gint64 total_level_now = 0;
    gint64 total_level_full = 0;
    gint64 total_rate_now = 0;
    gint seconds = 0;

    gboolean charging = FALSE;
    gboolean discharging = FALSE;
    gboolean full = FALSE;
    gboolean ac_connected = FALSE;

    for (l = batteries; l != NULL; l = l->next) {
        struct psy_battery *bat = l->data;
        update_linux_battery(bat);

        total_level_now += bat->level_now;
        total_level_full += bat->level_full;
        total_rate_now += bat->rate_now;

        charging |= (bat->status == BATTERY_CHARGING);
        discharging |= (bat->status == BATTERY_DISCHARGING);
        full |= (bat->status == BATTERY_FULL);
    }

    for (l = mains; l != NULL; l = l->next) {
        struct psy_mains *ac = l->data;
        update_linux_mains(ac);
        ac_connected |= (ac->online);
    }

    /* build global state */
    if (charging && !discharging)
        state->state = BATTERY_CHARGING;
    else if (!charging && discharging)
        state->state = BATTERY_DISCHARGING;
    else if (!charging && !discharging && full)
        state->state = BATTERY_FULL;

    /* calculate seconds */
    if (total_rate_now > 0) {
        if (state->state == BATTERY_CHARGING)
            seconds = 3600 * (total_level_full - total_level_now) / total_rate_now;
        else if (state->state == BATTERY_DISCHARGING)
            seconds = 3600 * total_level_now / total_rate_now;
        seconds = MAX(0, seconds);
    }
    battery_state_set_time(state, seconds);

    /* calculate percentage */
    state->percentage = level_to_percent(total_level_now, total_level_full);

    /* AC state */
    state->ac_connected = ac_connected;

    return 0;
}

static gchar *level_human_readable(struct psy_battery *bat)
{
    gint now = bat->level_now;
    gint full = bat->level_full;

    if (full >= 1000000) {
        return g_strdup_printf("%d.%d / %d.%d %ch",
                               now / 1000000,
                               (now % 1000000) / 100000,
                               full / 1000000,
                               (full % 1000000) / 100000,
                               bat->unit);
    } else if (full >= 1000) {
        return g_strdup_printf("%d.%d / %d.%d m%ch",
                               now / 1000,
                               (now % 1000) / 100,
                               full / 1000,
                               (full % 1000) / 100,
                               bat->unit);
    } else {
        return g_strdup_printf("%d / %d µ%ch", now, full, bat->unit);
    }
}

static gchar *rate_human_readable(struct psy_battery *bat)
{
    gint rate = bat->rate_now;
    gchar unit = bat->unit;

    if (rate >= 1000000) {
        return g_strdup_printf("%d.%d %c", rate / 1000000, (rate % 1000000) / 100000, unit);
    } else if (rate >= 1000) {
        return g_strdup_printf("%d.%d m%c", rate / 1000, (rate % 1000) / 100, unit);
    } else if (rate > 0) {
        return g_strdup_printf("%d µ%c", rate, unit);
    } else {
        return g_strdup_printf("0 %c", unit);
    }
}

char *battery_os_tooltip()
{
    GList *l;
    GString *tooltip = g_string_new("");
    gchar *result;

    for (l = batteries; l != NULL; l = l->next) {
        struct psy_battery *bat = l->data;

        if (tooltip->len)
            g_string_append_c(tooltip, '\n');

        g_string_append_printf(tooltip, "%s\n", bat->name);

        if (!bat->present) {
            g_string_append_printf(tooltip, "\tnot connected");
            continue;
        }

        gchar *rate = rate_human_readable(bat);
        gchar *level = level_human_readable(bat);
        gchar *state = (bat->status == BATTERY_UNKNOWN) ? "energy" : chargestate2str(bat->status);

        guint8 percentage = level_to_percent(bat->level_now, bat->level_full);

        g_string_append_printf(tooltip, "\t%s: %s (%u %%)\n\trate: %s", state, level, percentage, rate);

        g_free(rate);
        g_free(level);
    }

    for (l = mains; l != NULL; l = l->next) {
        struct psy_mains *ac = l->data;

        if (tooltip->len)
            g_string_append_c(tooltip, '\n');

        g_string_append_printf(tooltip, "%s\n", ac->name);
        g_string_append_printf(tooltip, ac->online ? "\tConnected" : "\tDisconnected");
    }

    result = tooltip->str;
    g_string_free(tooltip, FALSE);

    return result;
}

#endif
