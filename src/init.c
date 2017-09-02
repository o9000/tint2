#include "init.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "drag_and_drop.h"
#include "fps_distribution.h"
#include "panel.h"
#include "server.h"
#include "signals.h"
#include "tooltip.h"
#include "tracing.h"
#include "uevent.h"
#include "version.h"

void print_usage()
{
    fprintf(stdout,
            "Usage: tint2 [OPTION...]\n"
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

void handle_cli_arguments(int argc, char **argv)
{
    // Read command line arguments
    for (int i = 1; i < argc; ++i) {
        gboolean error = FALSE;
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            exit(0);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            fprintf(stdout, "tint2 version %s\n", VERSION_STRING);
            exit(0);
        } else if (strcmp(argv[i], "-c") == 0) {
            if (i + 1 < argc) {
                i++;
                config_path = strdup(argv[i]);
            } else {
                error = TRUE;
            }
        } else if (strcmp(argv[i], "-s") == 0) {
            if (i + 1 < argc) {
                i++;
                snapshot_path = strdup(argv[i]);
            } else {
                error = TRUE;
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
                error = TRUE;
            }
        }
#endif
        else {
            error = TRUE;
        }
        if (error) {
            print_usage();
            exit(1);
        }
    }
}

void handle_env_vars()
{
    debug_geometry = getenv("DEBUG_GEOMETRY") != NULL;
    debug_gradients = getenv("DEBUG_GRADIENTS") != NULL;
    debug_icons = getenv("DEBUG_ICONS") != NULL;
    debug_fps = getenv("DEBUG_FPS") != NULL;
    debug_frames = getenv("DEBUG_FRAMES") != NULL;
    debug_dnd = getenv("DEBUG_DND") != NULL;
    if (debug_fps) {
        init_fps_distribution();
        char *s = getenv("TRACING_FPS_THRESHOLD");
        if (!s || sscanf(s, "%lf", &tracing_fps_threshold) != 1) {
            tracing_fps_threshold = 60;
        }
    }
}

static timeout *detect_compositor_timer = NULL;
static int detect_compositor_timer_counter = 0;

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
        fprintf(stderr, "tint2: Detected compositor, restarting tint2...\n");
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

void create_default_elements()
{
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
}

void init_post_config()
{
    server_init_visual();
    server_init_xdamage();

    imlib_context_set_display(server.display);
    imlib_context_set_visual(server.visual);
    imlib_context_set_colormap(server.colormap);

    init_signals_postconfig();

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

    XSync(server.display, False);
}

void init_X11_pre_config()
{
    server.display = XOpenDisplay(NULL);
    if (!server.display) {
        fprintf(stderr, "tint2: could not open display!\n");
        exit(1);
    }
    server.x11_fd = ConnectionNumber(server.display);
    XSetErrorHandler((XErrorHandler)server_catch_error);
    XSetIOErrorHandler((XIOErrorHandler)x11_io_error);
    server_init_atoms();
    server.screen = DefaultScreen(server.display);
    server.root_win = RootWindow(server.display, server.screen);
    server.desktop = get_current_desktop();

    // Needed since the config file uses '.' as decimal separator
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "POSIX");

    /* Catch events */
    XSelectInput(server.display, server.root_win, PropertyChangeMask | StructureNotifyMask);

    // get monitor and desktop config
    get_monitors();
    get_desktops();

    server.disable_transparency = 0;

    xsettings_client = xsettings_client_new(server.display, server.screen, xsettings_notify_cb, NULL, NULL);
}

void init(int argc, char **argv)
{
    setlinebuf(stdout);
    setlinebuf(stderr);
    default_config();
    handle_cli_arguments(argc, argv);
    create_default_elements();
    init_signals();
    handle_env_vars();

    init_X11_pre_config();
    if (!config_read()) {
        fprintf(stderr, "tint2: Could not read config file.\n");
        print_usage();
        cleanup();
        return;
    }

    init_post_config();
    start_detect_compositor();
    init_panel();
}

void cleanup()
{
#ifdef HAVE_SN
    if (startup_notifications) {
        sn_display_unref(server.sn_display);
        server.sn_display = NULL;
    }
#endif // HAVE_SN

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

#ifdef HAVE_TRACING
    cleanup_tracing();
#endif
}
