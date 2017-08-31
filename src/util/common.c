/**************************************************************************
*
* Tint2 : common windows function
*
* Copyright (C) 2007 Pål Staurland (staura@gmail.com)
* Modified (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "common.h"
#include "../server.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>
#include <dirent.h>

#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#endif

#ifdef ENABLE_LIBUNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#else
#ifdef ENABLE_EXECINFO
#include <execinfo.h>
#endif
#endif

#include "../panel.h"
#include "timer.h"

void write_string(int fd, const char *s)
{
    int len = strlen(s);
    while (len > 0) {
        int count = write(fd, s, len);
        if (count >= 0) {
            s += count;
            len -= count;
        } else {
            break;
        }
    }
}

void log_string(int fd, const char *s)
{
    write_string(2, s);
    write_string(fd, s);
}

void dump_backtrace(int log_fd)
{
#ifndef DISABLE_BACKTRACE
    log_string(log_fd, "\n" YELLOW "Backtrace:" RESET "\n");

#ifdef ENABLE_LIBUNWIND
    unw_cursor_t cursor;
    unw_context_t context;
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    while (unw_step(&cursor) > 0) {
        unw_word_t offset;
        char fname[128];
        fname[0] = '\0';
        (void)unw_get_proc_name(&cursor, fname, sizeof(fname), &offset);
        log_string(log_fd, fname);
        log_string(log_fd, "\n");
    }
#else
#ifdef ENABLE_EXECINFO
#define MAX_TRACE_SIZE 128
    void *array[MAX_TRACE_SIZE];
    size_t size = backtrace(array, MAX_TRACE_SIZE);
    char **strings = backtrace_symbols(array, size);

    for (size_t i = 0; i < size; i++) {
        log_string(log_fd, strings[i]);
        log_string(log_fd, "\n");
    }

    free(strings);
#endif // ENABLE_EXECINFO
#endif // ENABLE_LIBUNWIND
#endif // DISABLE_BACKTRACE
}

// sleep() returns early when signals arrive. This function does not.
void safe_sleep(int seconds)
{
    double t0 = get_time();
    while (1) {
        double t = get_time();
        if (t > t0 + seconds)
            return;
        sleep(1);
    }
}

const char *signal_name(int sig)
{
    switch (sig) {
    case SIGHUP:
        return "SIGHUP: Hangup (POSIX).";
    case SIGINT:
        return "SIGINT: Interrupt (ANSI).";
    case SIGQUIT:
        return "SIGQUIT: Quit (POSIX).";
    case SIGILL:
        return "SIGILL: Illegal instruction (ANSI).";
    case SIGTRAP:
        return "SIGTRAP: Trace trap (POSIX).";
    case SIGABRT:
        return "SIGABRT/SIGIOT: Abort (ANSI) / IOT trap (4.2 BSD).";
    case SIGBUS:
        return "SIGBUS: BUS error (4.2 BSD).";
    case SIGFPE:
        return "SIGFPE: Floating-point exception (ANSI).";
    case SIGKILL:
        return "SIGKILL: Kill, unblockable (POSIX).";
    case SIGUSR1:
        return "SIGUSR1: User-defined signal 1 (POSIX).";
    case SIGSEGV:
        return "SIGSEGV: Segmentation violation (ANSI).";
    case SIGUSR2:
        return "SIGUSR2: User-defined signal 2 (POSIX).";
    case SIGPIPE:
        return "SIGPIPE: Broken pipe (POSIX).";
    case SIGALRM:
        return "SIGALRM: Alarm clock (POSIX).";
    case SIGTERM:
        return "SIGTERM: Termination (ANSI).";
    // case SIGSTKFLT: return "SIGSTKFLT: Stack fault.";
    case SIGCHLD:
        return "SIGCHLD: Child status has changed (POSIX).";
    case SIGCONT:
        return "SIGCONT: Continue (POSIX).";
    case SIGSTOP:
        return "SIGSTOP: Stop, unblockable (POSIX).";
    case SIGTSTP:
        return "SIGTSTP: Keyboard stop (POSIX).";
    case SIGTTIN:
        return "SIGTTIN: Background read from tty (POSIX).";
    case SIGTTOU:
        return "SIGTTOU: Background write to tty (POSIX).";
    case SIGURG:
        return "SIGURG: Urgent condition on socket (4.2 BSD).";
    case SIGXCPU:
        return "SIGXCPU: CPU limit exceeded (4.2 BSD).";
    case SIGXFSZ:
        return "SIGXFSZ: File size limit exceeded (4.2 BSD).";
    case SIGVTALRM:
        return "SIGVTALRM: Virtual alarm clock (4.2 BSD).";
    case SIGPROF:
        return "SIGPROF: Profiling alarm clock (4.2 BSD).";
    // case SIGPWR: return "SIGPWR: Power failure restart (System V).";
    case SIGSYS:
        return "SIGSYS: Bad system call.";
    }
    static char s[64];
    sprintf(s, "SIG=%d: Unknown", sig);
    return s;
}

const char *get_home_dir()
{
    const char *s = getenv("HOME");
    if (s)
        return s;
    struct passwd *pw = getpwuid(getuid());
    if (!pw)
        return NULL;
    return pw->pw_dir;
}

void copy_file(const char *path_src, const char *path_dest)
{
    if (g_str_equal(path_src, path_dest))
        return;

    FILE *file_src, *file_dest;
    char buffer[4096];
    int nb;

    file_src = fopen(path_src, "rb");
    if (file_src == NULL)
        return;

    file_dest = fopen(path_dest, "wb");
    if (file_dest == NULL) {
        fclose(file_src);
        return;
    }

    while ((nb = fread(buffer, 1, sizeof(buffer), file_src)) > 0) {
        if (nb != fwrite(buffer, 1, nb, file_dest)) {
            printf("Error while copying file %s to %s\n", path_src, path_dest);
        }
    }

    fclose(file_dest);
    fclose(file_src);
}

gboolean parse_line(const char *line, char **key, char **value)
{
    char *a, *b;

    /* Skip useless lines */
    if ((line[0] == '#') || (line[0] == '\n'))
        return FALSE;
    if (!(a = strchr(line, '=')))
        return FALSE;

    /* overwrite '=' with '\0' */
    a[0] = '\0';
    *key = strdup(line);
    a++;

    /* overwrite '\n' with '\0' if '\n' present */
    if ((b = strchr(a, '\n')))
        b[0] = '\0';

    *value = strdup(a);

    g_strstrip(*key);
    g_strstrip(*value);
    return TRUE;
}

extern char *config_path;

int setenvd(const char *name, const int value)
{
    char buf[256];
    sprintf(buf, "%d", value);
    return setenv(name, buf, 1);
}

#ifndef TINT2CONF
pid_t tint_exec(const char *command, const char *dir, const char *tooltip, Time time, Area *area, int x, int y)
{
    if (!command || strlen(command) == 0)
        return -1;

    if (area) {
        Panel *panel = (Panel *)area->panel;

        int aligned_x, aligned_y, aligned_x1, aligned_y1, aligned_x2, aligned_y2;
        int panel_x1, panel_x2, panel_y1, panel_y2;
        if (panel_horizontal) {
            if (area_is_first(area))
                aligned_x1 = panel->posx;
            else
                aligned_x1 = panel->posx + area->posx;

            if (area_is_last(area))
                aligned_x2 = panel->posx + panel->area.width;
            else
                aligned_x2 = panel->posx + area->posx + area->width;

            if (area_is_first(area))
                aligned_x = aligned_x1;
            else if (area_is_last(area))
                aligned_x = aligned_x2;
            else
                aligned_x = aligned_x1;

            if (panel_position & BOTTOM)
                aligned_y = panel->posy;
            else
                aligned_y = panel->posy + panel->area.height;

            aligned_y1 = aligned_y2 = aligned_y;

            panel_x1 = panel->posx;
            panel_x2 = panel->posx + panel->area.width;
            panel_y1 = panel_y2 = aligned_y;
        } else {
            if (area_is_first(area))
                aligned_y1 = panel->posy;
            else
                aligned_y1 = panel->posy + area->posy;

            if (area_is_last(area))
                aligned_y2 = panel->posy + panel->area.height;
            else
                aligned_y2 = panel->posy + area->posy + area->height;

            if (area_is_first(area))
                aligned_y = aligned_y1;
            else if (area_is_last(area))
                aligned_y = aligned_y2;
            else
                aligned_y = aligned_y1;

            if (panel_position & RIGHT)
                aligned_x = panel->posx;
            else
                aligned_x = panel->posx + panel->area.width;

            aligned_x1 = aligned_x2 = aligned_x;

            panel_x1 = panel_x2 = aligned_x;
            panel_y1 = panel->posy;
            panel_y2 = panel->posy + panel->area.height;
        }

        setenv("TINT2_CONFIG", config_path, 1);
        setenvd("TINT2_BUTTON_X", x);
        setenvd("TINT2_BUTTON_Y", y);
        setenvd("TINT2_BUTTON_W", area->width);
        setenvd("TINT2_BUTTON_H", area->height);
        setenvd("TINT2_BUTTON_ALIGNED_X", aligned_x);
        setenvd("TINT2_BUTTON_ALIGNED_Y", aligned_y);
        setenvd("TINT2_BUTTON_ALIGNED_X1", aligned_x1);
        setenvd("TINT2_BUTTON_ALIGNED_Y1", aligned_y1);
        setenvd("TINT2_BUTTON_ALIGNED_X2", aligned_x2);
        setenvd("TINT2_BUTTON_ALIGNED_Y2", aligned_y2);
        setenvd("TINT2_BUTTON_PANEL_X1", panel_x1);
        setenvd("TINT2_BUTTON_PANEL_Y1", panel_y1);
        setenvd("TINT2_BUTTON_PANEL_X2", panel_x2);
        setenvd("TINT2_BUTTON_PANEL_Y2", panel_y2);
    } else {
        setenv("TINT2_CONFIG", config_path, 1);
    }

    if (!command)
        return -1;

    if (!tooltip)
        tooltip = command;

#if HAVE_SN
    SnLauncherContext *ctx = 0;
    if (startup_notifications && time) {
        ctx = sn_launcher_context_new(server.sn_display, server.screen);
        sn_launcher_context_set_name(ctx, tooltip);
        sn_launcher_context_set_description(ctx, "Application launched from tint2");
        sn_launcher_context_set_binary_name(ctx, command);
        sn_launcher_context_initiate(ctx, "tint2", command, time);
    }
#endif /* HAVE_SN */
    pid_t pid;
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Could not fork\n");
    } else if (pid == 0) {
// Child process
#if HAVE_SN
        if (startup_notifications && time) {
            sn_launcher_context_setup_child_process(ctx);
        }
#endif // HAVE_SN
        // Allow children to exist after parent destruction
        setsid();
        // Run the command
        if (dir)
            chdir(dir);
        close_all_fds();
        execl("/bin/sh", "/bin/sh", "-c", command, NULL);
        fprintf(stderr, "Failed to execlp %s\n", command);
#if HAVE_SN
        if (startup_notifications && time) {
            sn_launcher_context_unref(ctx);
        }
#endif // HAVE_SN
        _exit(1);
    } else {
// Parent process
#if HAVE_SN
        if (startup_notifications && time) {
            g_tree_insert(server.pids, GINT_TO_POINTER(pid), ctx);
        }
#endif // HAVE_SN
    }

    unsetenv("TINT2_CONFIG");
    unsetenv("TINT2_BUTTON_X");
    unsetenv("TINT2_BUTTON_Y");
    unsetenv("TINT2_BUTTON_W");
    unsetenv("TINT2_BUTTON_H");
    unsetenv("TINT2_BUTTON_ALIGNED_X");
    unsetenv("TINT2_BUTTON_ALIGNED_Y");
    unsetenv("TINT2_BUTTON_ALIGNED_X1");
    unsetenv("TINT2_BUTTON_ALIGNED_Y1");
    unsetenv("TINT2_BUTTON_ALIGNED_X2");
    unsetenv("TINT2_BUTTON_ALIGNED_Y2");
    unsetenv("TINT2_BUTTON_PANEL_X1");
    unsetenv("TINT2_BUTTON_PANEL_Y1");
    unsetenv("TINT2_BUTTON_PANEL_X2");
    unsetenv("TINT2_BUTTON_PANEL_Y2");

    return pid;
}

void tint_exec_no_sn(const char *command)
{
    tint_exec(command, NULL, NULL, 0, NULL, 0, 0);
}
#endif

char *expand_tilde(const char *s)
{
    const gchar *home = g_get_home_dir();
    if (home && (strcmp(s, "~") == 0 || strstr(s, "~/") == s)) {
        char *result = calloc(strlen(home) + strlen(s), 1);
        strcat(result, home);
        strcat(result, s + 1);
        return result;
    } else {
        return strdup(s);
    }
}

char *contract_tilde(const char *s)
{
    const gchar *home = g_get_home_dir();
    if (!home)
        return strdup(s);

    char *home_slash = calloc(strlen(home) + 2, 1);
    strcat(home_slash, home);
    strcat(home_slash, "/");

    if ((strcmp(s, home) == 0 || strstr(s, home_slash) == s)) {
        char *result = calloc(strlen(s) - strlen(home) + 2, 1);
        strcat(result, "~");
        strcat(result, s + strlen(home));
        free(home_slash);
        return result;
    } else {
        free(home_slash);
        return strdup(s);
    }
}

int hex_char_to_int(char c)
{
    int r;

    if (c >= '0' && c <= '9')
        r = c - '0';
    else if (c >= 'a' && c <= 'f')
        r = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
        r = c - 'A' + 10;
    else
        r = 0;

    return r;
}

int hex_to_rgb(char *hex, int *r, int *g, int *b)
{
    if (hex == NULL || hex[0] != '#')
        return (0);

    int len = strlen(hex);
    if (len == 3 + 1) {
        *r = hex_char_to_int(hex[1]);
        *g = hex_char_to_int(hex[2]);
        *b = hex_char_to_int(hex[3]);
    } else if (len == 6 + 1) {
        *r = hex_char_to_int(hex[1]) * 16 + hex_char_to_int(hex[2]);
        *g = hex_char_to_int(hex[3]) * 16 + hex_char_to_int(hex[4]);
        *b = hex_char_to_int(hex[5]) * 16 + hex_char_to_int(hex[6]);
    } else if (len == 12 + 1) {
        *r = hex_char_to_int(hex[1]) * 16 + hex_char_to_int(hex[2]);
        *g = hex_char_to_int(hex[5]) * 16 + hex_char_to_int(hex[6]);
        *b = hex_char_to_int(hex[9]) * 16 + hex_char_to_int(hex[10]);
    } else
        return 0;

    return 1;
}

void get_color(char *hex, double *rgb)
{
    int r, g, b;
    r = g = b = 0;
    hex_to_rgb(hex, &r, &g, &b);

    rgb[0] = (r / 255.0);
    rgb[1] = (g / 255.0);
    rgb[2] = (b / 255.0);
}

void extract_values(const char *str, char **value1, char **value2, char **value3)
{
    *value1 = NULL;
    *value2 = NULL;
    *value3 = NULL;
    char **tokens = g_strsplit(str, " ", 3);
    if (tokens[0]) {
        *value1 = strdup(tokens[0]);
        if (tokens[1]) {
            *value2 = strdup(tokens[1]);
            if (tokens[2]) {
                *value3 = strdup(tokens[2]);
            }
        }
    }
    g_strfreev(tokens);
}

void extract_values_4(const char *str, char **value1, char **value2, char **value3, char **value4)
{
    *value1 = NULL;
    *value2 = NULL;
    *value3 = NULL;
    *value4 = NULL;
    char **tokens = g_strsplit(str, " ", 4);
    if (tokens[0]) {
        *value1 = strdup(tokens[0]);
        if (tokens[1]) {
            *value2 = strdup(tokens[1]);
            if (tokens[2]) {
                *value3 = strdup(tokens[2]);
                if (tokens[3]) {
                    *value4 = strdup(tokens[3]);
                }
            }
        }
    }
    g_strfreev(tokens);
}

void adjust_asb(DATA32 *data, int w, int h, float alpha_adjust, float satur_adjust, float bright_adjust)
{
    for (int id = 0; id < w * h; id++) {
        unsigned int argb = data[id];
        int a = (argb >> 24) & 0xff;
        // transparent => nothing to do.
        if (a == 0)
            continue;
        int r = (argb >> 16) & 0xff;
        int g = (argb >> 8) & 0xff;
        int b = (argb)&0xff;

        // Convert RGB to HSV
        int cmax = MAX3(r, g, b);
        int cmin = MIN3(r, g, b);
        int delta = cmax - cmin;
        float brightness = cmax / 255.0f;
        float saturation;
        if (cmax != 0)
            saturation = delta / (float)cmax;
        else
            saturation = 0;
        float hue;
        if (saturation == 0) {
            hue = 0;
        } else {
            float redc = (cmax - r) / (float)delta;
            float greenc = (cmax - g) / (float)delta;
            float bluec = (cmax - b) / (float)delta;
            if (r == cmax)
                hue = bluec - greenc;
            else if (g == cmax)
                hue = 2.0f + redc - bluec;
            else
                hue = 4.0f + greenc - redc;
            hue = hue / 6.0f;
            if (hue < 0)
                hue = hue + 1.0f;
        }

        // Adjust H, S
        saturation += satur_adjust;
        saturation = CLAMP(saturation, 0.0, 1.0);

        a *= alpha_adjust;
        a = CLAMP(a, 0, 255);

        // Convert HSV to RGB
        if (saturation == 0) {
            r = g = b = (int)(brightness * 255.0f + 0.5f);
        } else {
            float h2 = (hue - (int)hue) * 6.0f;
            float f = h2 - (int)(h2);
            float p = brightness * (1.0f - saturation);
            float q = brightness * (1.0f - saturation * f);
            float t = brightness * (1.0f - (saturation * (1.0f - f)));

            switch ((int)h2) {
            case 0:
                r = (int)(brightness * 255.0f + 0.5f);
                g = (int)(t * 255.0f + 0.5f);
                b = (int)(p * 255.0f + 0.5f);
                break;
            case 1:
                r = (int)(q * 255.0f + 0.5f);
                g = (int)(brightness * 255.0f + 0.5f);
                b = (int)(p * 255.0f + 0.5f);
                break;
            case 2:
                r = (int)(p * 255.0f + 0.5f);
                g = (int)(brightness * 255.0f + 0.5f);
                b = (int)(t * 255.0f + 0.5f);
                break;
            case 3:
                r = (int)(p * 255.0f + 0.5f);
                g = (int)(q * 255.0f + 0.5f);
                b = (int)(brightness * 255.0f + 0.5f);
                break;
            case 4:
                r = (int)(t * 255.0f + 0.5f);
                g = (int)(p * 255.0f + 0.5f);
                b = (int)(brightness * 255.0f + 0.5f);
                break;
            case 5:
                r = (int)(brightness * 255.0f + 0.5f);
                g = (int)(p * 255.0f + 0.5f);
                b = (int)(q * 255.0f + 0.5f);
                break;
            }
        }

        r += bright_adjust * 255;
        g += bright_adjust * 255;
        b += bright_adjust * 255;

        r = CLAMP(r, 0, 255);
        g = CLAMP(g, 0, 255);
        b = CLAMP(b, 0, 255);

        argb = a;
        argb = (argb << 8) + r;
        argb = (argb << 8) + g;
        argb = (argb << 8) + b;
        data[id] = argb;
    }
}

void create_heuristic_mask(DATA32 *data, int w, int h)
{
    // first we need to find the mask color, therefore we check all 4 edge pixel and take the color which
    // appears most often (we only need to check three edges, the 4th is implicitly clear)
    unsigned int topLeft = data[0], topRight = data[w - 1], bottomLeft = data[w * h - w], bottomRight = data[w * h - 1];
    int max = (topLeft == topRight) + (topLeft == bottomLeft) + (topLeft == bottomRight);
    int maskPos = 0;
    if (max < (topRight == topLeft) + (topRight == bottomLeft) + (topRight == bottomRight)) {
        max = (topRight == topLeft) + (topRight == bottomLeft) + (topRight == bottomRight);
        maskPos = w - 1;
    }
    if (max < (bottomLeft == topRight) + (bottomLeft == topLeft) + (bottomLeft == bottomRight))
        maskPos = w * h - w;

    // now mask out every pixel which has the same color as the edge pixels
    unsigned char *udata = (unsigned char *)data;
    unsigned char b = udata[4 * maskPos];
    unsigned char g = udata[4 * maskPos + 1];
    unsigned char r = udata[4 * maskPos + 1];
    for (int i = 0; i < h * w; ++i) {
        if (b - udata[0] == 0 && g - udata[1] == 0 && r - udata[2] == 0)
            udata[3] = 0;
        udata += 4;
    }
}

void render_image(Drawable d, int x, int y)
{
    if (!server.real_transparency) {
        imlib_context_set_blend(1);
        imlib_context_set_drawable(d);
        imlib_render_image_on_drawable(x, y);
        return;
    }

    int w = imlib_image_get_width(), h = imlib_image_get_height();

    Pixmap pixmap = XCreatePixmap(server.display, server.root_win, w, h, 32);
    imlib_context_set_drawable(pixmap);
    imlib_context_set_blend(0);
    imlib_render_image_on_drawable(0, 0);

    Pixmap mask = XCreatePixmap(server.display, server.root_win, w, h, 32);
    imlib_context_set_drawable(mask);
    imlib_context_set_blend(0);
    imlib_render_image_on_drawable(0, 0);

    Picture pict = XRenderCreatePicture(server.display,
                                        pixmap,
                                        XRenderFindStandardFormat(server.display, PictStandardARGB32),
                                        0,
                                        0);
    Picture pict_drawable =
        XRenderCreatePicture(server.display, d, XRenderFindVisualFormat(server.display, server.visual), 0, 0);
    Picture pict_mask =
        XRenderCreatePicture(server.display, mask, XRenderFindStandardFormat(server.display, PictStandardARGB32), 0, 0);
    XRenderComposite(server.display, PictOpOver, pict, pict_mask, pict_drawable, 0, 0, 0, 0, x, y, w, h);

    XRenderFreePicture(server.display, pict_mask);
    XRenderFreePicture(server.display, pict_drawable);
    XRenderFreePicture(server.display, pict);
    XFreePixmap(server.display, mask);
    XFreePixmap(server.display, pixmap);
}

void draw_text(PangoLayout *layout, cairo_t *c, int posx, int posy, Color *color, int font_shadow)
{
    if (font_shadow) {
        const int shadow_size = 3;
        const double shadow_edge_alpha = 0.0;
        int i, j;
        for (i = -shadow_size; i <= shadow_size; i++) {
            for (j = -shadow_size; j <= shadow_size; j++) {
                cairo_set_source_rgba(c,
                                      0.0,
                                      0.0,
                                      0.0,
                                      1.0 -
                                          (1.0 - shadow_edge_alpha) *
                                              sqrt((i * i + j * j) / (double)(shadow_size * shadow_size)));
                pango_cairo_update_layout(c, layout);
                cairo_move_to(c, posx + i, posy + j);
                pango_cairo_show_layout(c, layout);
            }
        }
    }
    cairo_set_source_rgba(c, color->rgb[0], color->rgb[1], color->rgb[2], color->alpha);
    pango_cairo_update_layout(c, layout);
    cairo_move_to(c, posx, posy);
    pango_cairo_show_layout(c, layout);
}

Imlib_Image load_image(const char *path, int cached)
{
    Imlib_Image image;
#ifdef HAVE_RSVG
    if (cached) {
        image = imlib_load_image_immediately(path);
    } else {
        image = imlib_load_image_immediately_without_cache(path);
    }
    if (!image && g_str_has_suffix(path, ".svg")) {
        char tmp_filename[128];
        sprintf(tmp_filename, "/tmp/tint2-%d.png", (int)getpid());
        int fd = open(tmp_filename, O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            // We fork here because librsvg allocates memory like crazy
            pid_t pid = fork();
            if (pid == 0) {
                // Child
                GError *err = NULL;
                RsvgHandle *svg = rsvg_handle_new_from_file(path, &err);

                if (err != NULL) {
                    fprintf(stderr, "Could not load svg image!: %s", err->message);
                    g_error_free(err);
                } else {
                    GdkPixbuf *pixbuf = rsvg_handle_get_pixbuf(svg);
                    gdk_pixbuf_save(pixbuf, tmp_filename, "png", NULL, NULL);
                }
                exit(0);
            } else {
                // Parent
                close(fd);
                waitpid(pid, 0, 0);
                image = imlib_load_image_immediately_without_cache(tmp_filename);
                unlink(tmp_filename);
            }
        }
    } else
#endif
    {
        if (cached) {
            image = imlib_load_image_immediately(path);
        } else {
            image = imlib_load_image_immediately_without_cache(path);
        }
    }
    return image;
}

Imlib_Image adjust_icon(Imlib_Image original, int alpha, int saturation, int brightness)
{
    if (!original)
        return NULL;

    imlib_context_set_image(original);
    Imlib_Image copy = imlib_clone_image();

    imlib_context_set_image(copy);
    imlib_image_set_has_alpha(1);
    DATA32 *data = imlib_image_get_data();
    adjust_asb(data,
               imlib_image_get_width(),
               imlib_image_get_height(),
               alpha / 100.0,
               saturation / 100.0,
               brightness / 100.0);
    imlib_image_put_back_data(data);
    return copy;
}

void draw_rect(cairo_t *c, double x, double y, double w, double h, double r)
{
    draw_rect_on_sides(c, x, y, w, h, r, BORDER_ALL);
}

void draw_rect_on_sides(cairo_t *c, double x, double y, double w, double h, double r, int border_mask)
{
    double c1 = 0.55228475 * r;
    cairo_move_to(c, x + r, y);
    // Top line
    if (border_mask & BORDER_TOP)
        cairo_rel_line_to(c, w - 2 * r, 0);
    else
        cairo_rel_move_to(c, w - 2 * r, y);
    // Top right corner
    if (r > 0) {
        if ((border_mask & BORDER_TOP) && (border_mask & BORDER_RIGHT))
            cairo_rel_curve_to(c, c1, 0.0, r, c1, r, r);
        else
            cairo_rel_move_to(c, r, r);
    }
    // Right line
    if (border_mask & BORDER_RIGHT)
        cairo_rel_line_to(c, 0, h - 2 * r);
    else
        cairo_rel_move_to(c, 0, h - 2 * r);
    // Bottom right corner
    if (r > 0) {
        if ((border_mask & BORDER_RIGHT) && (border_mask & BORDER_BOTTOM))
            cairo_rel_curve_to(c, 0.0, c1, c1 - r, r, -r, r);
        else
            cairo_rel_move_to(c, -r, r);
    }
    // Bottom line
    if (border_mask & BORDER_BOTTOM)
        cairo_rel_line_to(c, -w + 2 * r, 0);
    else
        cairo_rel_move_to(c, -w + 2 * r, 0);
    // Bottom left corner
    if (r > 0) {
        if ((border_mask & BORDER_LEFT) && (border_mask & BORDER_BOTTOM))
            cairo_rel_curve_to(c, -c1, 0, -r, -c1, -r, -r);
        else
            cairo_rel_move_to(c, -r, -r);
    }
    // Left line
    if (border_mask & BORDER_LEFT)
        cairo_rel_line_to(c, 0, -h + 2 * r);
    else
        cairo_rel_move_to(c, 0, -h + 2 * r);
    // Top left corner
    if (r > 0) {
        if ((border_mask & BORDER_LEFT) && (border_mask & BORDER_TOP))
            cairo_rel_curve_to(c, 0, -c1, r - c1, -r, r, -r);
        else
            cairo_rel_move_to(c, r, -r);
    }
}

void clear_pixmap(Pixmap p, int x, int y, int w, int h)
{
    Picture pict =
        XRenderCreatePicture(server.display, p, XRenderFindVisualFormat(server.display, server.visual), 0, 0);
    XRenderColor col;
    col.red = col.green = col.blue = col.alpha = 0;
    XRenderFillRectangle(server.display, PictOpSrc, pict, &col, x, y, w, h);
    XRenderFreePicture(server.display, pict);
}

void get_text_size2(const PangoFontDescription *font,
                    int *height_ink,
                    int *height,
                    int *width,
                    int available_height,
                    int available_width,
                    const char *text,
                    int text_len,
                    PangoWrapMode wrap,
                    PangoEllipsizeMode ellipsis,
                    gboolean markup)
{
    PangoRectangle rect_ink, rect;

    available_width = MAX(0, available_width);
    available_height = MAX(0, available_height);
    Pixmap pmap = XCreatePixmap(server.display, server.root_win, available_height, available_width, server.depth);

    cairo_surface_t *cs =
        cairo_xlib_surface_create(server.display, pmap, server.visual, available_height, available_width);
    cairo_t *c = cairo_create(cs);

    PangoLayout *layout = pango_cairo_create_layout(c);
    pango_layout_set_width(layout, available_width * PANGO_SCALE);
    pango_layout_set_height(layout, available_height * PANGO_SCALE);
    pango_layout_set_wrap(layout, wrap);
    pango_layout_set_ellipsize(layout, ellipsis);
    pango_layout_set_font_description(layout, font);
    text_len = MAX(0, text_len);
    if (!markup)
        pango_layout_set_text(layout, text, text_len);
    else
        pango_layout_set_markup(layout, text, text_len);

    pango_layout_get_pixel_extents(layout, &rect_ink, &rect);
    *height_ink = rect_ink.height;
    *height = rect.height;
    *width = rect.width;
    // printf("dimension : %d - %d\n", rect_ink.height, rect.height);

    g_object_unref(layout);
    cairo_destroy(c);
    cairo_surface_destroy(cs);
    XFreePixmap(server.display, pmap);
}

#if !GLIB_CHECK_VERSION(2, 34, 0)
GList *g_list_copy_deep(GList *list, GCopyFunc func, gpointer user_data)
{
    list = g_list_copy(list);

    if (func) {
        for (GList *l = list; l; l = l->next) {
            l->data = func(l->data, user_data);
        }
    }

    return list;
}
#endif

GSList *load_locations_from_env(GSList *locations, const char *var, ...)
{
    char *value = getenv(var);
    if (value) {
        value = strdup(value);
        char *p = value;
        char *t;
        for (char *token = strtok_r(value, ":", &t); token; token = strtok_r(NULL, ":", &t)) {
            va_list ap;
            va_start(ap, var);
            for (const char *suffix = va_arg(ap, const char *); suffix; suffix = va_arg(ap, const char *)) {
                locations = g_slist_append(locations, g_build_filename(token, suffix, NULL));
            }
            va_end(ap);
        }
        free(p);
    }
    return locations;
}

GSList *slist_remove_duplicates(GSList *list, GCompareFunc eq, GDestroyNotify fr)
{
    GSList *new_list = NULL;

    for (GSList *l1 = list; l1; l1 = g_slist_next(l1)) {
        gboolean duplicate = FALSE;
        for (GSList *l2 = new_list; l2; l2 = g_slist_next(l2)) {
            if (eq(l1->data, l2->data)) {
                duplicate = TRUE;
                break;
            }
        }
        if (!duplicate) {
            new_list = g_slist_append(new_list, l1->data);
            l1->data = NULL;
        }
    }

    g_slist_free_full(list, fr);

    return new_list;
}

gint cmp_ptr(gconstpointer a, gconstpointer b)
{
    if (a < b)
        return -1;
    else if (a == b)
        return 0;
    else
        return 1;
}

void close_all_fds()
{
    long maxfd = sysconf(_SC_OPEN_MAX);
    for (int fd = 3; fd < maxfd; fd++) {
        close(fd);
    }
}
