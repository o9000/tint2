/**************************************************************************
*
* Tint2 panel
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

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>
#include <X11/extensions/Xdamage.h>
#include <Imlib2.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>

#ifdef HAVE_SN
#include <libsn/sn.h>
#endif

#include <version.h>
#include "server.h"
#include "window.h"
#include "config.h"
#include "drag_and_drop.h"
#include "task.h"
#include "taskbar.h"
#include "systraybar.h"
#include "launcher.h"
#include "mouse_actions.h"
#include "panel.h"
#include "tooltip.h"
#include "timer.h"
#include "xsettings-client.h"
#include "uevent.h"

#ifdef ENABLE_LIBUNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#else
#ifdef ENABLE_EXECINFO
#include <execinfo.h>
#endif
#endif

// Global process state variables

static gboolean hidden_dnd;

XSettingsClient *xsettings_client = NULL;

timeout *detect_compositor_timer = NULL;
int detect_compositor_timer_counter = 0;

gboolean debug_fps = FALSE;
gboolean debug_frames = FALSE;
float *fps_distribution = NULL;
int frame = 0;

void create_fps_distribution()
{
    // measure FPS with resolution:
    // 0-59: 1		   (60 samples)
    // 60-199: 10      (14)
    // 200-1,999: 25   (72)
    // 1k-19,999: 1000 (19)
    // 20x+: inf       (1)
    // => 166 samples
    if (fps_distribution)
        return;
    fps_distribution = calloc(170, sizeof(float));
}

void cleanup_fps_distribution()
{
    free(fps_distribution);
    fps_distribution = NULL;
}

void sample_fps(double fps)
{
    int fps_rounded = (int)(fps + 0.5);
    int i = 1;
    if (fps_rounded < 60) {
        i += fps_rounded;
    } else {
        i += 60;
        if (fps_rounded < 200) {
            i += (fps_rounded - 60) / 10;
        } else {
            i += 14;
            if (fps_rounded < 2000) {
                i += (fps_rounded - 200) / 25;
            } else {
                i += 72;
                if (fps_rounded < 20000) {
                    i += (fps_rounded - 2000) / 1000;
                } else {
                    i += 20;
                }
            }
        }
    }
    // fprintf(stderr, "fps = %.0f => i = %d\n", fps, i);
    fps_distribution[i] += 1.;
    fps_distribution[0] += 1.;
}

void fps_compute_stats(double *low, double *median, double *high, double *samples)
{
    *median = *low = *high = *samples = -1;
    if (!fps_distribution || fps_distribution[0] < 1)
        return;
    float total = fps_distribution[0];
    *samples = (double)fps_distribution[0];
    float cum_low = 0.05f * total;
    float cum_median = 0.5f * total;
    float cum_high = 0.95f * total;
    float cum = 0;
    for (int i = 1; i <= 166; i++) {
        double value =
            (i < 60) ? i : (i < 74) ? (60 + (i - 60) * 10) : (i < 146) ? (200 + (i - 74) * 25)
                                                                       : (i < 165) ? (2000 + (i - 146) * 1000) : 20000;
        // fprintf(stderr, "%6.0f (i = %3d) : %.0f | ", value, i, (double)fps_distribution[i]);
        cum += fps_distribution[i];
        if (*low < 0 && cum >= cum_low)
            *low = value;
        if (*median < 0 && cum >= cum_median)
            *median = value;
        if (*high < 0 && cum >= cum_high)
            *high = value;
    }
}

void detect_compositor(void *arg)
{
    if (server.composite_manager) {
        stop_timeout(detect_compositor_timer);
        return;
    }

    detect_compositor_timer_counter--;
    if (detect_compositor_timer_counter < 0) {
        stop_timeout(detect_compositor_timer);
        return;
    }

    // No compositor, check for one
    if (XGetSelectionOwner(server.display, server.atom._NET_WM_CM_S0) != None) {
        stop_timeout(detect_compositor_timer);
        // Restart tint2
        fprintf(stderr, "Detected compositor, restarting tint2...\n");
        kill(getpid(), SIGUSR1);
    }
}

void start_detect_compositor()
{
    // Already have a compositor, nothing to do
    if (server.composite_manager)
        return;

    stop_timeout(detect_compositor_timer);
    // Check every 0.5 seconds for up to 30 seconds
    detect_compositor_timer_counter = 60;
    detect_compositor_timer = add_timeout(500, 500, detect_compositor, 0, &detect_compositor_timer);
}

void signal_handler(int sig)
{
    // signal handler is light as it should be
    signal_pending = sig;
}

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

void log_string(int fd, const char *s)
{
    write_string(2, s);
    write_string(fd, s);
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

void handle_crash(const char *reason)
{
#ifndef DISABLE_BACKTRACE
    char path[4096];
    sprintf(path, "%s/.tint2-crash.log", get_home_dir());
    int log_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    log_string(log_fd, RED "tint2 crashed, reason: ");
    log_string(log_fd, reason);
    log_string(log_fd, RESET "\n");
    dump_backtrace(log_fd);
    log_string(log_fd, RED "Please create a bug report with this log output." RESET "\n");
    close(log_fd);
#endif
}

#ifdef BACKTRACE_ON_SIGNAL
void crash_handler(int sig)
{
    handle_crash(signal_name(sig));
    struct sigaction sa = {.sa_handler = SIG_DFL};
    sigaction(sig, &sa, 0);
    raise(sig);
}
#endif

void x11_io_error(Display *display)
{
    handle_crash("X11 I/O error");
}

void print_usage()
{
    printf("Usage: tint2 [OPTION...]\n"
           "\n"
           "Options:\n"
           "  -c path_to_config_file   Loads the configuration file from a\n"
           "                           custom location.\n"
           "  -v, --version            Prints version information and exits.\n"
           "  -h, --help               Display this help and exits.\n"
           "\n"
           "For more information, run `man tint2` or visit the project page\n"
           "<https://gitlab.com/o9000/tint2>.\n");
}

void init(int argc, char *argv[])
{
    // Make stdout/stderr flush after a newline (for some reason they don't even if tint2 is started from a terminal)
    setlinebuf(stdout);
    setlinebuf(stderr);

    // set global data
    default_config();
    default_timeout();
    default_systray();
    memset(&server, 0, sizeof(server));
#ifdef ENABLE_BATTERY
    default_battery();
#endif
    default_clock();
    default_launcher();
    default_taskbar();
    default_tooltip();
    default_execp();
    default_button();
    default_panel();

    // Read command line arguments
    for (int i = 1; i < argc; ++i) {
        int error = 0;
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            exit(0);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("tint2 version %s\n", VERSION_STRING);
            exit(0);
        } else if (strcmp(argv[i], "-c") == 0) {
            if (i + 1 < argc) {
                i++;
                config_path = strdup(argv[i]);
            } else {
                error = 1;
            }
        } else if (strcmp(argv[i], "-s") == 0) {
            if (i + 1 < argc) {
                i++;
                snapshot_path = strdup(argv[i]);
            } else {
                error = 1;
            }
        } else if (i + 1 == argc) {
            config_path = strdup(argv[i]);
        }
#ifdef ENABLE_BATTERY
        else if (strcmp(argv[i], "--battery-sys-prefix") == 0) {
            if (i + 1 < argc) {
                i++;
                battery_sys_prefix = strdup(argv[i]);
            } else {
                error = 1;
            }
        }
#endif

        else {
            error = 1;
        }
        if (error) {
            print_usage();
            exit(1);
        }
    }
    // Set signal handlers
    signal_pending = 0;

    struct sigaction sa_chld = {.sa_handler = SIG_DFL, .sa_flags = SA_NOCLDWAIT | SA_RESTART};
    sigaction(SIGCHLD, &sa_chld, 0);

    struct sigaction sa = {.sa_handler = signal_handler, .sa_flags = SA_RESTART};
    sigaction(SIGUSR1, &sa, 0);
    sigaction(SIGUSR2, &sa, 0);
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
    sigaction(SIGHUP, &sa, 0);

#ifdef BACKTRACE_ON_SIGNAL
    struct sigaction sa_crash = {.sa_handler = crash_handler};
    sigaction(SIGSEGV, &sa_crash, 0);
    sigaction(SIGFPE, &sa_crash, 0);
    sigaction(SIGPIPE, &sa_crash, 0);
    sigaction(SIGBUS, &sa_crash, 0);
    sigaction(SIGABRT, &sa_crash, 0);
    sigaction(SIGSYS, &sa_crash, 0);
#endif

    debug_geometry = getenv("DEBUG_GEOMETRY") != NULL;
    debug_gradients = getenv("DEBUG_GRADIENTS") != NULL;
    debug_fps = getenv("DEBUG_FPS") != NULL;
    debug_frames = getenv("DEBUG_FRAMES") != NULL;
    if (debug_fps)
        create_fps_distribution();
}

static int sigchild_pipe_valid = FALSE;
static int sigchild_pipe[2];

#ifdef HAVE_SN
static int error_trap_depth = 0;

static void error_trap_push(SnDisplay *display, Display *xdisplay)
{
    ++error_trap_depth;
}

static void error_trap_pop(SnDisplay *display, Display *xdisplay)
{
    if (error_trap_depth == 0) {
        fprintf(stderr, "Error trap underflow!\n");
        return;
    }

    XSync(xdisplay, False); /* get all errors out of the queue */
    --error_trap_depth;
}
#endif // HAVE_SN

static void sigchld_handler(int sig)
{
    if (!sigchild_pipe_valid)
        return;
    int savedErrno = errno;
    ssize_t unused = write(sigchild_pipe[1], "x", 1);
    (void)unused;
    fsync(sigchild_pipe[1]);
    errno = savedErrno;
}

static void sigchld_handler_async()
{
    // Wait for all dead processes
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) != -1 && pid != 0) {
#ifdef HAVE_SN
        if (startup_notifications) {
            SnLauncherContext *ctx = (SnLauncherContext *)g_tree_lookup(server.pids, GINT_TO_POINTER(pid));
            if (ctx) {
                g_tree_remove(server.pids, GINT_TO_POINTER(pid));
                sn_launcher_context_complete(ctx);
                sn_launcher_context_unref(ctx);
            }
        }
#endif
        for (GList *l = panel_config.execp_list; l; l = l->next) {
            Execp *execp = (Execp *)l->data;
            if (g_tree_lookup(execp->backend->cmd_pids, GINT_TO_POINTER(pid)))
                execp_cmd_completed(execp, pid);
        }
    }
}

void init_X11_pre_config()
{
    server.display = XOpenDisplay(NULL);
    if (!server.display) {
        fprintf(stderr, "tint2: could not open display.\n");
        exit(1);
    }
    XSetErrorHandler((XErrorHandler)server_catch_error);
    XSetIOErrorHandler((XIOErrorHandler)x11_io_error);
    server_init_atoms();
    server.screen = DefaultScreen(server.display);
    server.root_win = RootWindow(server.display, server.screen);
    server.desktop = get_current_desktop();

    setlocale(LC_ALL, "");
    // config file use '.' as decimal separator
    setlocale(LC_NUMERIC, "POSIX");

    /* Catch events */
    XSelectInput(server.display, server.root_win, PropertyChangeMask | StructureNotifyMask);

    // get monitor and desktop config
    get_monitors();
    get_desktops();

    server.disable_transparency = 0;

    xsettings_client = xsettings_client_new(server.display, server.screen, xsettings_notify_cb, NULL, NULL);
}

void init_X11_post_config()
{
    server_init_visual();
    server_init_xdamage();

    gboolean need_sigchld = FALSE;
#ifdef HAVE_SN
    // Initialize startup-notification
    if (startup_notifications) {
        server.sn_display = sn_display_new(server.display, error_trap_push, error_trap_pop);
        server.pids = g_tree_new(cmp_ptr);
        need_sigchld = TRUE;
    }
#endif // HAVE_SN
    if (panel_config.execp_list)
        need_sigchld = TRUE;

    if (need_sigchld) {
        // Setup a handler for child termination
        if (pipe(sigchild_pipe) != 0) {
            fprintf(stderr, "Creating pipe failed.\n");
        } else {
            fcntl(sigchild_pipe[0], F_SETFL, O_NONBLOCK | fcntl(sigchild_pipe[0], F_GETFL));
            fcntl(sigchild_pipe[1], F_SETFL, O_NONBLOCK | fcntl(sigchild_pipe[1], F_GETFL));
            sigchild_pipe_valid = 1;
            struct sigaction act = {.sa_handler = sigchld_handler, .sa_flags = SA_RESTART};
            if (sigaction(SIGCHLD, &act, 0)) {
                perror("sigaction");
            }
        }
    }

    imlib_context_set_display(server.display);
    imlib_context_set_visual(server.visual);
    imlib_context_set_colormap(server.colormap);

    // load default icon
    const gchar *const *data_dirs = g_get_system_data_dirs();
    for (int i = 0; data_dirs[i] != NULL; i++) {
        gchar *path = g_build_filename(data_dirs[i], "tint2", "default_icon.png", NULL);
        if (g_file_test(path, G_FILE_TEST_EXISTS))
            default_icon = load_image(path, TRUE);
        g_free(path);
    }
    if (!default_icon) {
        fprintf(stderr,
                RED "Could not load default_icon.png. Please check that tint2 has been installed correctly!" RESET
                    "\n");
    }
}

void cleanup()
{
    cleanup_button();
    cleanup_execp();
    cleanup_systray();
    cleanup_tooltip();
    cleanup_clock();
    cleanup_launcher();
#ifdef ENABLE_BATTERY
    cleanup_battery();
#endif
    cleanup_panel();
    cleanup_config();

    if (default_icon) {
        imlib_context_set_image(default_icon);
        imlib_free_image();
        default_icon = NULL;
    }
    imlib_context_disconnect_display();

    xsettings_client_destroy(xsettings_client);
    xsettings_client = NULL;

    cleanup_server();
    cleanup_timeout();

    if (server.display)
        XCloseDisplay(server.display);
    server.display = NULL;

    if (sigchild_pipe_valid) {
        sigchild_pipe_valid = FALSE;
        close(sigchild_pipe[1]);
        close(sigchild_pipe[0]);
    }

    uevent_cleanup();
    cleanup_fps_distribution();
}

void dump_panel_to_file(const Panel *panel, const char *path)
{
    imlib_context_set_drawable(panel->temp_pmap);
    Imlib_Image img = imlib_create_image_from_drawable(0, 0, 0, panel->area.width, panel->area.height, 1);

    if (!img) {
        XImage *ximg =
            XGetImage(server.display, panel->temp_pmap, 0, 0, panel->area.width, panel->area.height, AllPlanes, ZPixmap);

        if (ximg) {
            DATA32 *pixels = (DATA32 *)calloc(panel->area.width * panel->area.height, sizeof(DATA32));
            for (int x = 0; x < panel->area.width; x++) {
                for (int y = 0; y < panel->area.height; y++) {
                    DATA32 xpixel = XGetPixel(ximg, x, y);

                    DATA32 r = (xpixel >> 16) & 0xff;
                    DATA32 g = (xpixel >> 8) & 0xff;
                    DATA32 b = (xpixel >> 0) & 0xff;
                    DATA32 a = 0x0;

                    DATA32 argb = (a << 24) | (r << 16) | (g << 8) | b;
                    pixels[y * panel->area.width + x] = argb;
                }
            }
            XDestroyImage(ximg);
            img = imlib_create_image_using_data(panel->area.width, panel->area.height, pixels);
        }
    }

    if (img) {
        imlib_context_set_image(img);
        if (!panel_horizontal) {
            // rotate 90° vertical panel
            imlib_image_flip_horizontal();
            imlib_image_flip_diagonal();
        }
        imlib_save_image(path);
        imlib_free_image();
    }
}

void get_snapshot(const char *path)
{
    Panel *panel = &panels[0];

    if (panel->area.width > server.monitors[0].width)
        panel->area.width = server.monitors[0].width;

    panel->temp_pmap =
        XCreatePixmap(server.display, server.root_win, panel->area.width, panel->area.height, server.depth);
    render_panel(panel);

    XSync(server.display, False);

    dump_panel_to_file(panel, path);
}



void update_desktop_names()
{
    if (!taskbarname_enabled)
        return;
    GSList *list = get_desktop_names();
    for (int i = 0; i < num_panels; i++) {
        int j;
        GSList *l;
        for (j = 0, l = list; j < panels[i].num_desktops; j++) {
            gchar *name;
            if (l) {
                name = g_strdup(l->data);
                l = l->next;
            } else {
                name = g_strdup_printf("%d", j + 1);
            }
            Taskbar *taskbar = &panels[i].taskbar[j];
            if (strcmp(name, taskbar->bar_name.name) != 0) {
                g_free(taskbar->bar_name.name);
                taskbar->bar_name.name = name;
                taskbar->bar_name.area.resize_needed = 1;
            } else {
                g_free(name);
            }
        }
    }
    for (GSList *l = list; l; l = l->next)
        g_free(l->data);
    g_slist_free(list);
    schedule_panel_redraw();
}

void update_task_desktop(Task *task)
{
    // fprintf(stderr, "%s %d:\n", __FUNCTION__, __LINE__);
    Window win = task->win;
    remove_task(task);
    task = add_task(win);
    reset_active_task();
    schedule_panel_redraw();
}

void event_property_notify(XEvent *e)
{
    gboolean debug = FALSE;

    Window win = e->xproperty.window;
    Atom at = e->xproperty.atom;

    if (xsettings_client)
        xsettings_client_process_event(xsettings_client, e);
    for (int i = 0; i < num_panels; i++) {
        Panel *p = &panels[i];
        if (win == p->main_win) {
            if (at == server.atom._NET_WM_DESKTOP && get_window_desktop(p->main_win) != ALL_DESKTOPS)
                replace_panel_all_desktops(p);
            return;
        }
    }
    if (win == server.root_win) {
        if (!server.got_root_win) {
            XSelectInput(server.display, server.root_win, PropertyChangeMask | StructureNotifyMask);
            server.got_root_win = TRUE;
        }

        // Change name of desktops
        else if (at == server.atom._NET_DESKTOP_NAMES) {
            if (debug)
                fprintf(stderr, "%s %d: win = root, atom = _NET_DESKTOP_NAMES\n", __FUNCTION__, __LINE__);
            update_desktop_names();
        }
        // Change desktops
        else if (at == server.atom._NET_NUMBER_OF_DESKTOPS || at == server.atom._NET_DESKTOP_GEOMETRY ||
                 at == server.atom._NET_DESKTOP_VIEWPORT || at == server.atom._NET_WORKAREA ||
                 at == server.atom._NET_CURRENT_DESKTOP) {
            if (debug)
                fprintf(stderr, "%s %d: win = root, atom = ?? desktops changed\n", __FUNCTION__, __LINE__);
            if (!taskbar_enabled)
                return;
            int old_num_desktops = server.num_desktops;
            int old_desktop = server.desktop;
            server_get_number_of_desktops();
            server.desktop = get_current_desktop();
            if (old_num_desktops != server.num_desktops) { // If number of desktop changed
                if (server.num_desktops <= server.desktop) {
                    server.desktop = server.num_desktops - 1;
                }
                cleanup_taskbar();
                init_taskbar();
                for (int i = 0; i < num_panels; i++) {
                    init_taskbar_panel(&panels[i]);
                    set_panel_items_order(&panels[i]);
                    panels[i].area.resize_needed = 1;
                }
                taskbar_refresh_tasklist();
                reset_active_task();
                update_all_taskbars_visibility();
                if (old_desktop != server.desktop)
                    tooltip_trigger_hide();
                schedule_panel_redraw();
            } else if (old_desktop != server.desktop) {
                tooltip_trigger_hide();
                for (int i = 0; i < num_panels; i++) {
                    Panel *panel = &panels[i];
                    set_taskbar_state(&panel->taskbar[old_desktop], TASKBAR_NORMAL);
                    set_taskbar_state(&panel->taskbar[server.desktop], TASKBAR_ACTIVE);
                    // check ALL_DESKTOPS task => resize taskbar
                    Taskbar *taskbar;
                    if (server.num_desktops > old_desktop) {
                        taskbar = &panel->taskbar[old_desktop];
                        GList *l = taskbar->area.children;
                        if (taskbarname_enabled)
                            l = l->next;
                        for (; l; l = l->next) {
                            Task *task = l->data;
                            if (task->desktop == ALL_DESKTOPS) {
                                task->area.on_screen = always_show_all_desktop_tasks;
                                taskbar->area.resize_needed = 1;
                                schedule_panel_redraw();
                                if (taskbar_mode == MULTI_DESKTOP)
                                    panel->area.resize_needed = 1;
                            }
                        }
                    }
                    taskbar = &panel->taskbar[server.desktop];
                    GList *l = taskbar->area.children;
                    if (taskbarname_enabled)
                        l = l->next;
                    for (; l; l = l->next) {
                        Task *task = l->data;
                        if (task->desktop == ALL_DESKTOPS) {
                            task->area.on_screen = TRUE;
                            taskbar->area.resize_needed = 1;
                            if (taskbar_mode == MULTI_DESKTOP)
                                panel->area.resize_needed = 1;
                        }
                    }

                    if (server.viewports) {
                        GList *need_update = NULL;

                        GHashTableIter iter;
                        gpointer key, value;

                        g_hash_table_iter_init(&iter, win_to_task);
                        while (g_hash_table_iter_next(&iter, &key, &value)) {
                            Window task_win = *(Window *)key;
                            Task *task = get_task(task_win);
                            if (task) {
                                int desktop = get_window_desktop(task_win);
                                if (desktop != task->desktop) {
                                    need_update = g_list_append(need_update, task);
                                }
                            }
                        }
                        for (l = need_update; l; l = l->next) {
                            Task *task = l->data;
                            update_task_desktop(task);
                        }
                    }
                }
            }
        }
        // Window list
        else if (at == server.atom._NET_CLIENT_LIST) {
            if (debug)
                fprintf(stderr, "%s %d: win = root, atom = _NET_CLIENT_LIST\n", __FUNCTION__, __LINE__);
            taskbar_refresh_tasklist();
            update_all_taskbars_visibility();
            schedule_panel_redraw();
        }
        // Change active
        else if (at == server.atom._NET_ACTIVE_WINDOW) {
            if (debug)
                fprintf(stderr, "%s %d: win = root, atom = _NET_ACTIVE_WINDOW\n", __FUNCTION__, __LINE__);
            reset_active_task();
            schedule_panel_redraw();
        } else if (at == server.atom._XROOTPMAP_ID || at == server.atom._XROOTMAP_ID) {
            if (debug)
                fprintf(stderr, "%s %d: win = root, atom = _XROOTPMAP_ID\n", __FUNCTION__, __LINE__);
            // change Wallpaper
            for (int i = 0; i < num_panels; i++) {
                set_panel_background(&panels[i]);
            }
            schedule_panel_redraw();
        }
    } else {
        TrayWindow *traywin = systray_find_icon(win);
        if (traywin) {
            systray_property_notify(traywin, e);
            return;
        }

        Task *task = get_task(win);
        if (debug) {
            char *atom_name = XGetAtomName(server.display, at);
            fprintf(stderr,
                    "%s %d: win = %ld, task = %s, atom = %s\n",
                    __FUNCTION__,
                    __LINE__,
                    win,
                    task ? (task->title ? task->title : "??") : "null",
                    atom_name);
            XFree(atom_name);
        }
        if (!task) {
            if (debug)
                fprintf(stderr, "%s %d\n", __FUNCTION__, __LINE__);
            if (at == server.atom._NET_WM_STATE) {
                // xfce4 sends _NET_WM_STATE after minimized to tray, so we need to check if window is mapped
                // if it is mapped and not set as skip_taskbar, we must add it to our task list
                XWindowAttributes wa;
                XGetWindowAttributes(server.display, win, &wa);
                if (wa.map_state == IsViewable && !window_is_skip_taskbar(win)) {
                    if ((task = add_task(win)))
                        schedule_panel_redraw();
                }
            }
            return;
        }
        // printf("atom root_win = %s, %s\n", XGetAtomName(server.display, at), task->title);

        // Window title changed
        if (at == server.atom._NET_WM_VISIBLE_NAME || at == server.atom._NET_WM_NAME || at == server.atom.WM_NAME) {
            if (task_update_title(task)) {
                if (g_tooltip.mapped && (g_tooltip.area == (Area *)task)) {
                    tooltip_copy_text((Area *)task);
                    tooltip_update();
                }
                if (taskbar_sort_method == TASKBAR_SORT_TITLE)
                    sort_taskbar_for_win(win);
                schedule_panel_redraw();
            }
        }
        // Demand attention
        else if (at == server.atom._NET_WM_STATE) {
            if (debug) {
                int count;
                Atom *atom_state = server_get_property(win, server.atom._NET_WM_STATE, XA_ATOM, &count);
                for (int j = 0; j < count; j++) {
                    char *atom_state_name = XGetAtomName(server.display, atom_state[j]);
                    fprintf(stderr, "%s %d: _NET_WM_STATE = %s\n", __FUNCTION__, __LINE__, atom_state_name);
                    XFree(atom_state_name);
                }
                XFree(atom_state);
            }
            if (window_is_urgent(win)) {
                add_urgent(task);
            }
            if (window_is_skip_taskbar(win)) {
                remove_task(task);
                schedule_panel_redraw();
            }
        } else if (at == server.atom.WM_STATE) {
            // Iconic state
            TaskState state = (active_task && task->win == active_task->win ? TASK_ACTIVE : TASK_NORMAL);
            if (window_is_iconified(win))
                state = TASK_ICONIFIED;
            set_task_state(task, state);
            schedule_panel_redraw();
        }
        // Window icon changed
        else if (at == server.atom._NET_WM_ICON) {
            task_update_icon(task);
            schedule_panel_redraw();
        }
        // Window desktop changed
        else if (at == server.atom._NET_WM_DESKTOP) {
            int desktop = get_window_desktop(win);
            // printf("  Window desktop changed %d, %d\n", task->desktop, desktop);
            // bug in windowmaker : send unecessary 'desktop changed' when focus changed
            if (desktop != task->desktop) {
                update_task_desktop(task);
            }
        } else if (at == server.atom.WM_HINTS) {
            XWMHints *wmhints = XGetWMHints(server.display, win);
            if (wmhints && wmhints->flags & XUrgencyHint) {
                add_urgent(task);
            }
            XFree(wmhints);
            task_update_icon(task);
            schedule_panel_redraw();
        }

        if (!server.got_root_win)
            server.root_win = RootWindow(server.display, server.screen);
    }
}

void event_expose(XEvent *e)
{
    Panel *panel;
    panel = get_panel(e->xany.window);
    if (!panel)
        return;
    // TODO : one panel_refresh per panel ?
    schedule_panel_redraw();
}

void event_configure_notify(XEvent *e)
{
    Window win = e->xconfigure.window;

    if (0) {
        Task *task = get_task(win);
        fprintf(stderr,
                "%s %d: win = %ld, task = %s\n",
                __FUNCTION__,
                __LINE__,
                win,
                task ? (task->title ? task->title : "??") : "null");
    }

    // change in root window (xrandr)
    if (win == server.root_win) {
        fprintf(stderr,
                YELLOW "%s %d: triggering tint2 restart due to configuration change in the root window" RESET "\n",
                __FILE__,
                __LINE__);
        signal_pending = SIGUSR1;
        return;
    }

    TrayWindow *traywin = systray_find_icon(win);
    if (traywin) {
        systray_reconfigure_event(traywin, e);
        return;
    }

    // 'win' move in another monitor
    if (num_panels > 1 || hide_task_diff_monitor) {
        Task *task = get_task(win);
        if (task) {
            Panel *p = task->area.panel;
            int monitor = get_window_monitor(win);
            if ((hide_task_diff_monitor && p->monitor != monitor && task->area.on_screen) ||
                (hide_task_diff_monitor && p->monitor == monitor && !task->area.on_screen) ||
                (p->monitor != monitor && num_panels > 1)) {
                remove_task(task);
                task = add_task(win);
                if (win == get_active_window()) {
                    set_task_state(task, TASK_ACTIVE);
                    active_task = task;
                }
                schedule_panel_redraw();
            }
        }
    }

    if (server.viewports) {
        Task *task = get_task(win);
        if (task) {
            int desktop = get_window_desktop(win);
            if (task->desktop != desktop) {
                update_task_desktop(task);
            }
        }
    }

    sort_taskbar_for_win(win);
}

gboolean handle_x_event_autohide(XEvent *e)
{
    Panel *panel = get_panel(e->xany.window);
    if (panel && panel_autohide) {
        if (e->type == EnterNotify)
            autohide_trigger_show(panel);
        else if (e->type == LeaveNotify)
            autohide_trigger_hide(panel);
        if (panel->is_hidden) {
            if (e->type == ClientMessage && e->xclient.message_type == server.atom.XdndPosition) {
                hidden_dnd = TRUE;
                autohide_show(panel);
            } else {
                // discard further processing of this event because the panel is not visible yet
                return TRUE;
            }
        } else if (hidden_dnd && e->type == ClientMessage &&
                   e->xclient.message_type == server.atom.XdndLeave) {
            hidden_dnd = FALSE;
            autohide_hide(panel);
        }
    }
    return FALSE;
}

void handle_x_event(XEvent *e)
{
#if HAVE_SN
    if (startup_notifications)
        sn_display_process_event(server.sn_display, e);
#endif // HAVE_SN

    if (handle_x_event_autohide(e))
        return;

    Panel *panel = get_panel(e->xany.window);
    switch (e->type) {
    case ButtonPress: {
        tooltip_hide(0);
        handle_mouse_press_event(e);
        Area *area = find_area_under_mouse(panel, e->xbutton.x, e->xbutton.y);
        if (panel_config.mouse_effects)
            mouse_over(area, TRUE);
        break;
    }

    case ButtonRelease: {
        handle_mouse_release_event(e);
        Area *area = find_area_under_mouse(panel, e->xbutton.x, e->xbutton.y);
        if (panel_config.mouse_effects)
            mouse_over(area, FALSE);
        break;
    }

    case MotionNotify: {
        unsigned int button_mask = Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask;
        if (e->xmotion.state & button_mask)
            handle_mouse_move_event(e);

        Area *area = find_area_under_mouse(panel, e->xmotion.x, e->xmotion.y);
        if (area->_get_tooltip_text)
            tooltip_trigger_show(area, panel, e);
        else
            tooltip_trigger_hide();
        if (panel_config.mouse_effects)
            mouse_over(area, e->xmotion.state & button_mask);
        break;
    }

    case LeaveNotify: {
        tooltip_trigger_hide();
        if (panel_config.mouse_effects)
            mouse_out();
        break;
    }

    case Expose:
        event_expose(e);
        break;

    case PropertyNotify:
        event_property_notify(e);
        break;

    case ConfigureNotify:
        event_configure_notify(e);
        break;

    case ConfigureRequest: {
        TrayWindow *traywin = systray_find_icon(e->xany.window);
        if (traywin)
            systray_reconfigure_event(traywin, e);
        break;
    }

    case ResizeRequest: {
        TrayWindow *traywin = systray_find_icon(e->xany.window);
        if (traywin)
            systray_resize_request_event(traywin, e);
        break;
    }

    case ReparentNotify: {
        if (!systray_enabled)
            break;
        Panel *systray_panel = (Panel *)systray.area.panel;
        if (e->xany.window == systray_panel->main_win) // don't care
            break;
        TrayWindow *traywin = systray_find_icon(e->xreparent.window);
        if (traywin) {
            if (traywin->win == e->xreparent.window) {
                if (traywin->parent == e->xreparent.parent) {
                    embed_icon(traywin);
                } else {
                    remove_icon(traywin);
                }
                break;
            }
        }
        break;
    }

    case UnmapNotify:
        break;

    case DestroyNotify:
        if (e->xany.window == server.composite_manager) {
            // Stop real_transparency
            fprintf(stderr,
                    YELLOW "%s %d: triggering tint2 restart due to compositor shutdown" RESET "\n",
                    __FILE__,
                    __LINE__);
            signal_pending = SIGUSR1;
            break;
        }
        if (e->xany.window == g_tooltip.window || !systray_enabled)
            break;
        for (GSList *it = systray.list_icons; it; it = g_slist_next(it)) {
            if (((TrayWindow *)it->data)->win == e->xany.window) {
                systray_destroy_event((TrayWindow *)it->data);
                break;
            }
        }
        break;

    case ClientMessage: {
        XClientMessageEvent *ev = &e->xclient;
        if (ev->data.l[1] == server.atom._NET_WM_CM_S0) {
            if (ev->data.l[2] == None) {
                // Stop real_transparency
                fprintf(stderr,
                        YELLOW "%s %d: triggering tint2 restart due to change in transparency" RESET "\n",
                        __FILE__,
                        __LINE__);
                signal_pending = SIGUSR1;
            } else {
                // Start real_transparency
                fprintf(stderr,
                        YELLOW "%s %d: triggering tint2 restart due to change in transparency" RESET "\n",
                        __FILE__,
                        __LINE__);
                signal_pending = SIGUSR1;
            }
        }
        if (systray_enabled && e->xclient.message_type == server.atom._NET_SYSTEM_TRAY_OPCODE &&
            e->xclient.format == 32 && e->xclient.window == net_sel_win) {
            handle_systray_event(&e->xclient);
        } else if (e->xclient.message_type == server.atom.XdndEnter) {
            handle_dnd_enter(&e->xclient);
        } else if (e->xclient.message_type == server.atom.XdndPosition) {
            handle_dnd_position(&e->xclient);
        } else if (e->xclient.message_type == server.atom.XdndDrop) {
            handle_dnd_drop(&e->xclient);
        }
        break;
    }

    case SelectionNotify: {
        handle_dnd_selection_notify(&e->xselection);
        break;
    }

    default:
        if (e->type == server.xdamage_event_type) {
            XDamageNotifyEvent *de = (XDamageNotifyEvent *)e;
            TrayWindow *traywin = systray_find_icon(de->drawable);
            if (traywin)
                systray_render_icon(traywin);
        }
    }
}

int main(int argc, char *argv[])
{
start:
    init(argc, argv);

    init_X11_pre_config();

    if (!config_read()) {
        fprintf(stderr, "Could not read config file.\n");
        print_usage();
        cleanup();
        exit(1);
    }

    init_X11_post_config();
    start_detect_compositor();

    init_panel();

    if (snapshot_path) {
        get_snapshot(snapshot_path);
        cleanup();
        exit(0);
    }


    int x11_fd = ConnectionNumber(server.display);
    XSync(server.display, False);

    dnd_init();
    int ufd = uevent_init();
    hidden_dnd = FALSE;

    double ts_event_read = 0;
    double ts_event_processed = 0;
    double ts_render_finished = 0;
    double ts_flush_finished = 0;
    gboolean first_render = TRUE;
    while (1) {
        if (panel_refresh) {
            if (debug_fps)
                ts_event_processed = get_time();
            if (systray_profile)
                fprintf(stderr,
                        BLUE "[%f] %s:%d redrawing panel" RESET "\n",
                        profiling_get_time(),
                        __FUNCTION__,
                        __LINE__);
            panel_refresh = FALSE;

            for (int i = 0; i < num_panels; i++) {
                Panel *panel = &panels[i];
                if (!first_render)
                    shrink_panel(panel);

                if (!panel->is_hidden || panel->area.resize_needed) {
                    if (panel->temp_pmap)
                        XFreePixmap(server.display, panel->temp_pmap);
                    panel->temp_pmap = XCreatePixmap(server.display,
                                                     server.root_win,
                                                     panel->area.width,
                                                     panel->area.height,
                                                     server.depth);
                    render_panel(panel);
                }

                if (panel->is_hidden) {
                    if (!panel->hidden_pixmap) {
                        panel->hidden_pixmap = XCreatePixmap(server.display,
                                                             server.root_win,
                                                             panel->hidden_width,
                                                             panel->hidden_height,
                                                             server.depth);
                        int xoff = 0, yoff = 0;
                        if (panel_horizontal && panel_position & BOTTOM)
                            yoff = panel->area.height - panel->hidden_height;
                        else if (!panel_horizontal && panel_position & RIGHT)
                            xoff = panel->area.width - panel->hidden_width;
                        XCopyArea(server.display,
                                  panel->area.pix,
                                  panel->hidden_pixmap,
                                  server.gc,
                                  xoff,
                                  yoff,
                                  panel->hidden_width,
                                  panel->hidden_height,
                                  0,
                                  0);
                    }
                    XCopyArea(server.display,
                              panel->hidden_pixmap,
                              panel->main_win,
                              server.gc,
                              0,
                              0,
                              panel->hidden_width,
                              panel->hidden_height,
                              0,
                              0);
                    XSetWindowBackgroundPixmap(server.display, panel->main_win, panel->hidden_pixmap);
                } else {
                    XCopyArea(server.display,
                              panel->temp_pmap,
                              panel->main_win,
                              server.gc,
                              0,
                              0,
                              panel->area.width,
                              panel->area.height,
                              0,
                              0);
                    if (panel == (Panel *)systray.area.panel) {
                        if (refresh_systray && panel && !panel->is_hidden) {
                            refresh_systray = FALSE;
                            XSetWindowBackgroundPixmap(server.display, panel->main_win, panel->temp_pmap);
                            refresh_systray_icons();
                        }
                    }
                }
            }
            if (first_render) {
                first_render = FALSE;
                if (panel_shrink)
                    schedule_panel_redraw();
            }
            if (debug_fps)
                ts_render_finished = get_time();
            XFlush(server.display);
            if (debug_fps && ts_event_read > 0) {
                ts_flush_finished = get_time();
                double period = ts_flush_finished - ts_event_read;
                double fps = 1.0 / period;
                sample_fps(fps);
                double proc_ratio = (ts_event_processed - ts_event_read) / period;
                double render_ratio = (ts_render_finished - ts_event_processed) / period;
                double flush_ratio = (ts_flush_finished - ts_render_finished) / period;
                double fps_low, fps_median, fps_high, fps_samples;
                fps_compute_stats(&fps_low, &fps_median, &fps_high, &fps_samples);
                fprintf(stderr,
                        BLUE "frame %d: fps = %.0f (low %.0f, med %.0f, high %.0f, samples %.0f) : processing %.0f%%, "
                             "rendering %.0f%%, "
                             "flushing %.0f%%" RESET "\n",
                        frame,
                        fps,
                        fps_low,
                        fps_median,
                        fps_high,
                        fps_samples,
                        proc_ratio * 100,
                        render_ratio * 100,
                        flush_ratio * 100);
            }
            if (debug_frames) {
                for (int i = 0; i < num_panels; i++) {
                    char path[256];
                    sprintf(path, "tint2-%d-panel-%d-frame-%d.png", getpid(), i, frame);
                    dump_panel_to_file(&panels[i], path);
                }
            }
            frame++;
        }

        // Create a File Description Set containing x11_fd
        fd_set fdset;
        FD_ZERO(&fdset);
        FD_SET(x11_fd, &fdset);
        int maxfd = x11_fd;
        if (sigchild_pipe_valid) {
            FD_SET(sigchild_pipe[0], &fdset);
            maxfd = maxfd < sigchild_pipe[0] ? sigchild_pipe[0] : maxfd;
        }
        for (GList *l = panel_config.execp_list; l; l = l->next) {
            Execp *execp = (Execp *)l->data;
            int fd = execp->backend->child_pipe_stdout;
            if (fd > 0) {
                FD_SET(fd, &fdset);
                maxfd = maxfd < fd ? fd : maxfd;
            }
            fd = execp->backend->child_pipe_stderr;
            if (fd > 0) {
                FD_SET(fd, &fdset);
                maxfd = maxfd < fd ? fd : maxfd;
            }
        }
        if (ufd > 0) {
            FD_SET(ufd, &fdset);
            maxfd = maxfd < ufd ? ufd : maxfd;
        }
        update_next_timeout();
        struct timeval *select_timeout = (next_timeout.tv_sec >= 0 && next_timeout.tv_usec >= 0) ? &next_timeout : NULL;

        // Wait for X Event or a Timer
        ts_event_read = 0;
        if (XPending(server.display) > 0 || select(maxfd + 1, &fdset, 0, 0, select_timeout) >= 0) {
            uevent_handler();

            if (sigchild_pipe_valid) {
                char buffer[1];
                while (read(sigchild_pipe[0], buffer, sizeof(buffer)) > 0) {
                    sigchld_handler_async();
                }
            }
            for (GList *l = panel_config.execp_list; l; l = l->next) {
                Execp *execp = (Execp *)l->data;
                if (read_execp(execp)) {
                    GList *l_instance;
                    for (l_instance = execp->backend->instances; l_instance; l_instance = l_instance->next) {
                        Execp *instance = (Execp *)l_instance->data;
                        execp_update_post_read(instance);
                    }
                }
            }
            if (XPending(server.display) > 0) {
                XEvent e;
                XNextEvent(server.display, &e);
                if (debug_fps)
                    ts_event_read = get_time();

                handle_x_event(&e);
            }
        }

        callback_timeout_expired();

        if (signal_pending) {
            cleanup();
            if (signal_pending == SIGUSR1) {
                fprintf(stderr, YELLOW "%s %d: restarting tint2..." RESET "\n", __FILE__, __LINE__);
                // restart tint2
                // SIGUSR1 used when : user's signal, composite manager stop/start or xrandr
                goto start;
            } else if (signal_pending == SIGUSR2) {
                fprintf(stderr, YELLOW "%s %d: reexecuting tint2..." RESET "\n", __FILE__, __LINE__);
                if (execvp(argv[0], argv) == -1) {
                    fprintf(stderr, RED "%s %d: failed!" RESET "\n", __FILE__, __LINE__);
                    return 1;
                }
            } else {
                // SIGINT, SIGTERM, SIGHUP
                exit(0);
            }
        }
    }
}
