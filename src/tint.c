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

#ifdef HAVE_SN
#include <libsn/sn.h>
#include <sys/wait.h>
#endif

#include <version.h>
#include "server.h"
#include "window.h"
#include "config.h"
#include "task.h"
#include "taskbar.h"
#include "systraybar.h"
#include "launcher.h"
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

// Drag and Drop state variables
Window dnd_source_window;
Window dnd_target_window;
int dnd_version;
Atom dnd_selection;
Atom dnd_atom;
int dnd_sent_request;
char *dnd_launcher_exec;
XSettingsClient *xsettings_client = NULL;

timeout *detect_compositor_timer = NULL;
int detect_compositor_timer_counter = 0;

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
	case SIGWINCH:
		return "SIGWINCH: Window size change (4.3 BSD, Sun).";
	case SIGIO:
		return "SIGIO: Pollable event occurred (System V) / I/O now possible (4.2 BSD).";
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
	default_panel();

	// Read command line arguments
	for (int i = 1; i < argc; ++i) {
		int error = 0;
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			printf("Usage: tint2 [[-c] <config_file>]\n");
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
		} else {
			error = 1;
		}
		if (error) {
			printf("Usage: tint2 [[-c] <config_file>]\n");
			exit(1);
		}
	}
	// Set signal handlers
	signal_pending = 0;

	struct sigaction sa_chld = {.sa_handler = SIG_DFL, .sa_flags = SA_NOCLDWAIT | SA_RESTART};
	sigaction(SIGCHLD, &sa_chld, 0);

	struct sigaction sa = {.sa_handler = signal_handler, .sa_flags = SA_RESTART};
	sigaction(SIGUSR1, &sa, 0);
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
}

static int sn_pipe_valid = 0;
static int sn_pipe[2];

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

static void sigchld_handler(int sig)
{
	if (!startup_notifications)
		return;
	if (!sn_pipe_valid)
		return;
	ssize_t wur = write(sn_pipe[1], "x", 1);
	(void)wur;
	fsync(sn_pipe[1]);
}

static void sigchld_handler_async()
{
	if (!startup_notifications)
		return;
	// Wait for all dead processes
	pid_t pid;
	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
		SnLauncherContext *ctx;
		ctx = (SnLauncherContext *)g_tree_lookup(server.pids, GINT_TO_POINTER(pid));
		if (ctx) {
			g_tree_remove(server.pids, GINT_TO_POINTER(pid));
			sn_launcher_context_complete(ctx);
			sn_launcher_context_unref(ctx);
		}
	}
}

static gint cmp_ptr(gconstpointer a, gconstpointer b)
{
	if (a < b)
		return -1;
	else if (a == b)
		return 0;
	else
		return 1;
}
#else
static void sigchld_handler_async()
{
}
#endif // HAVE_SN

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

#ifdef HAVE_SN
	// Initialize startup-notification
	if (startup_notifications) {
		server.sn_display = sn_display_new(server.display, error_trap_push, error_trap_pop);
		server.pids = g_tree_new(cmp_ptr);
		// Setup a handler for child termination
		if (pipe(sn_pipe) != 0) {
			fprintf(stderr, "Creating pipe failed.\n");
		} else {
			fcntl(sn_pipe[0], F_SETFL, O_NONBLOCK | fcntl(sn_pipe[0], F_GETFL));
			fcntl(sn_pipe[1], F_SETFL, O_NONBLOCK | fcntl(sn_pipe[1], F_GETFL));
			sn_pipe_valid = 1;
			struct sigaction act = {.sa_handler = sigchld_handler, .sa_flags = SA_NOCLDWAIT | SA_RESTART};
			if (sigaction(SIGCHLD, &act, 0)) {
				perror("sigaction");
			}
		}
	}
#endif // HAVE_SN

	imlib_context_set_display(server.display);
	imlib_context_set_visual(server.visual);
	imlib_context_set_colormap(server.colormap);

	// load default icon
	const gchar *const *data_dirs = g_get_system_data_dirs();
	for (int i = 0; data_dirs[i] != NULL; i++) {
		gchar *path = g_build_filename(data_dirs[i], "tint2", "default_icon.png", NULL);
		if (g_file_test(path, G_FILE_TEST_EXISTS))
			default_icon = imlib_load_image(path);
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

#ifdef HAVE_SN
	if (startup_notifications) {
		if (sn_pipe_valid) {
			sn_pipe_valid = 0;
			close(sn_pipe[1]);
			close(sn_pipe[0]);
		}
	}
#endif

	uevent_cleanup();
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

	DATA32 *pixels = calloc(panel->area.width * panel->area.height, sizeof(DATA32));
	XImage *ximg =
		XGetImage(server.display, panel->temp_pmap, 0, 0, panel->area.width, panel->area.height, AllPlanes, ZPixmap);

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

	Imlib_Image img = imlib_create_image_using_data(panel->area.width, panel->area.height, pixels);

	// imlib_context_set_drawable(panel->temp_pmap);
	// Imlib_Image img = imlib_create_image_from_drawable(0, 0, 0, panel->area.width, panel->area.height, 1);

	imlib_context_set_image(img);
	if (!panel_horizontal) {
		// rotate 90° vertical panel
		imlib_image_flip_horizontal();
		imlib_image_flip_diagonal();
	}
	imlib_save_image(path);
	imlib_free_image();
}

void window_action(Task *task, MouseAction action)
{
	if (!task)
		return;
	switch (action) {
	case NONE:
		break;
	case CLOSE:
		close_window(task->win);
		break;
	case TOGGLE:
		activate_window(task->win);
		break;
	case ICONIFY:
		XIconifyWindow(server.display, task->win, server.screen);
		break;
	case TOGGLE_ICONIFY:
		if (active_task && task->win == active_task->win)
			XIconifyWindow(server.display, task->win, server.screen);
		else
			activate_window(task->win);
		break;
	case SHADE:
		toggle_window_shade(task->win);
		break;
	case MAXIMIZE_RESTORE:
		toggle_window_maximized(task->win);
		break;
	case MAXIMIZE:
		toggle_window_maximized(task->win);
		break;
	case RESTORE:
		toggle_window_maximized(task->win);
		break;
	case DESKTOP_LEFT: {
		if (task->desktop == 0)
			break;
		int desktop = task->desktop - 1;
		change_window_desktop(task->win, desktop);
		if (desktop == server.desktop)
			activate_window(task->win);
		break;
	}
	case DESKTOP_RIGHT: {
		if (task->desktop == server.num_desktops)
			break;
		int desktop = task->desktop + 1;
		change_window_desktop(task->win, desktop);
		if (desktop == server.desktop)
			activate_window(task->win);
		break;
	}
	case NEXT_TASK: {
		Task *task1 = next_task(find_active_task(task));
		activate_window(task1->win);
	} break;
	case PREV_TASK: {
		Task *task1 = prev_task(find_active_task(task));
		activate_window(task1->win);
	}
	}
}

int tint2_handles_click(Panel *panel, XButtonEvent *e)
{
	Task *task = click_task(panel, e->x, e->y);
	if (task) {
		if ((e->button == 1 && mouse_left != 0) || (e->button == 2 && mouse_middle != 0) ||
			(e->button == 3 && mouse_right != 0) || (e->button == 4 && mouse_scroll_up != 0) ||
			(e->button == 5 && mouse_scroll_down != 0)) {
			return 1;
		} else
			return 0;
	}
	LauncherIcon *icon = click_launcher_icon(panel, e->x, e->y);
	if (icon) {
		if (e->button == 1) {
			return 1;
		} else {
			return 0;
		}
	}
	// no launcher/task clicked --> check if taskbar clicked
	Taskbar *taskbar = click_taskbar(panel, e->x, e->y);
	if (taskbar && e->button == 1 && taskbar_mode == MULTI_DESKTOP)
		return 1;
	if (click_clock(panel, e->x, e->y)) {
		if ((e->button == 1 && clock_lclick_command) || (e->button == 2 && clock_mclick_command) ||
			(e->button == 3 && clock_rclick_command) || (e->button == 4 && clock_uwheel_command) ||
			(e->button == 5 && clock_dwheel_command))
			return 1;
		else
			return 0;
	}
#ifdef ENABLE_BATTERY
	if (click_battery(panel, e->x, e->y)) {
		if ((e->button == 1 && battery_lclick_command) || (e->button == 2 && battery_mclick_command) ||
			(e->button == 3 && battery_rclick_command) || (e->button == 4 && battery_uwheel_command) ||
			(e->button == 5 && battery_dwheel_command))
			return 1;
		else
			return 0;
	}
#endif
	if (click_execp(panel, e->x, e->y))
		return 1;
	return 0;
}

void forward_click(XEvent *e)
{
	// forward the click to the desktop window (thanks conky)
	XUngrabPointer(server.display, e->xbutton.time);
	e->xbutton.window = server.root_win;
	// icewm doesn't open under the mouse.
	// and xfce doesn't open at all.
	e->xbutton.x = e->xbutton.x_root;
	e->xbutton.y = e->xbutton.y_root;
	// printf("**** %d, %d\n", e->xbutton.x, e->xbutton.y);
	// XSetInputFocus(server.display, e->xbutton.window, RevertToParent, e->xbutton.time);
	XSendEvent(server.display, e->xbutton.window, False, ButtonPressMask, e);
}

void event_button_press(XEvent *e)
{
	Panel *panel = get_panel(e->xany.window);
	if (!panel)
		return;

	if (wm_menu && !tint2_handles_click(panel, &e->xbutton)) {
		forward_click(e);
		return;
	}
	task_drag = click_task(panel, e->xbutton.x, e->xbutton.y);

	if (panel_layer == BOTTOM_LAYER)
		XLowerWindow(server.display, panel->main_win);
}

void event_button_motion_notify(XEvent *e)
{
	Panel *panel = get_panel(e->xany.window);
	if (!panel || !task_drag)
		return;

	// Find the taskbar on the event's location
	Taskbar *event_taskbar = click_taskbar(panel, e->xbutton.x, e->xbutton.y);
	if (event_taskbar == NULL)
		return;

	// Find the task on the event's location
	Task *event_task = click_task(panel, e->xbutton.x, e->xbutton.y);

	// If the event takes place on the same taskbar as the task being dragged
	if (&event_taskbar->area == task_drag->area.parent) {
		if (taskbar_sort_method != TASKBAR_NOSORT) {
			sort_tasks(event_taskbar);
		} else {
			// Swap the task_drag with the task on the event's location (if they differ)
			if (event_task && event_task != task_drag) {
				GList *drag_iter = g_list_find(event_taskbar->area.children, task_drag);
				GList *task_iter = g_list_find(event_taskbar->area.children, event_task);
				if (drag_iter && task_iter) {
					gpointer temp = task_iter->data;
					task_iter->data = drag_iter->data;
					drag_iter->data = temp;
					event_taskbar->area.resize_needed = 1;
					panel_refresh = TRUE;
					task_dragged = 1;
				}
			}
		}
	} else { // The event is on another taskbar than the task being dragged
		if (task_drag->desktop == ALL_DESKTOPS || taskbar_mode != MULTI_DESKTOP)
			return;

		Taskbar *drag_taskbar = (Taskbar *)task_drag->area.parent;
		remove_area((Area *)task_drag);

		if (event_taskbar->area.posx > drag_taskbar->area.posx || event_taskbar->area.posy > drag_taskbar->area.posy) {
			int i = (taskbarname_enabled) ? 1 : 0;
			event_taskbar->area.children = g_list_insert(event_taskbar->area.children, task_drag, i);
		} else
			event_taskbar->area.children = g_list_append(event_taskbar->area.children, task_drag);

		// Move task to other desktop (but avoid the 'Window desktop changed' code in 'event_property_notify')
		task_drag->area.parent = &event_taskbar->area;
		task_drag->desktop = event_taskbar->desktop;

		change_window_desktop(task_drag->win, event_taskbar->desktop);

		if (taskbar_sort_method != TASKBAR_NOSORT) {
			sort_tasks(event_taskbar);
		}

		event_taskbar->area.resize_needed = 1;
		drag_taskbar->area.resize_needed = 1;
		task_dragged = 1;
		panel_refresh = TRUE;
		panel->area.resize_needed = 1;
	}
}

void event_button_release(XEvent *e)
{
	Panel *panel = get_panel(e->xany.window);
	if (!panel)
		return;

	if (wm_menu && !tint2_handles_click(panel, &e->xbutton)) {
		forward_click(e);
		if (panel_layer == BOTTOM_LAYER)
			XLowerWindow(server.display, panel->main_win);
		task_drag = 0;
		return;
	}

	int action = TOGGLE_ICONIFY;
	switch (e->xbutton.button) {
	case 1:
		action = mouse_left;
		break;
	case 2:
		action = mouse_middle;
		break;
	case 3:
		action = mouse_right;
		break;
	case 4:
		action = mouse_scroll_up;
		break;
	case 5:
		action = mouse_scroll_down;
		break;
	case 6:
		action = mouse_tilt_left;
		break;
	case 7:
		action = mouse_tilt_right;
		break;
	}

	if (click_clock(panel, e->xbutton.x, e->xbutton.y)) {
		clock_action(e->xbutton.button);
		if (panel_layer == BOTTOM_LAYER)
			XLowerWindow(server.display, panel->main_win);
		task_drag = 0;
		return;
	}

#ifdef ENABLE_BATTERY
	if (click_battery(panel, e->xbutton.x, e->xbutton.y)) {
		battery_action(e->xbutton.button);
		if (panel_layer == BOTTOM_LAYER)
			XLowerWindow(server.display, panel->main_win);
		task_drag = 0;
		return;
	}
#endif

	Execp *execp = click_execp(panel, e->xbutton.x, e->xbutton.y);
	if (execp) {
		execp_action(execp, e->xbutton.button, e->xbutton.x - execp->area.posx, e->xbutton.y - execp->area.posy);
		if (panel_layer == BOTTOM_LAYER)
			XLowerWindow(server.display, panel->main_win);
		task_drag = 0;
		return;
	}

	if (e->xbutton.button == 1 && click_launcher(panel, e->xbutton.x, e->xbutton.y)) {
		LauncherIcon *icon = click_launcher_icon(panel, e->xbutton.x, e->xbutton.y);
		if (icon) {
			launcher_action(icon, e);
		}
		task_drag = 0;
		return;
	}

	Taskbar *taskbar;
	if (!(taskbar = click_taskbar(panel, e->xbutton.x, e->xbutton.y))) {
		// TODO: check better solution to keep window below
		if (panel_layer == BOTTOM_LAYER)
			XLowerWindow(server.display, panel->main_win);
		task_drag = 0;
		return;
	}

	// drag and drop task
	if (task_dragged) {
		task_drag = 0;
		task_dragged = 0;
		return;
	}

	// switch desktop
	if (taskbar_mode == MULTI_DESKTOP) {
		gboolean diff_desktop = FALSE;
		if (taskbar->desktop != server.desktop && action != CLOSE && action != DESKTOP_LEFT &&
			action != DESKTOP_RIGHT) {
			diff_desktop = TRUE;
			change_desktop(taskbar->desktop);
		}
		Task *task = click_task(panel, e->xbutton.x, e->xbutton.y);
		if (task) {
			if (diff_desktop) {
				if (action == TOGGLE_ICONIFY) {
					if (!window_is_active(task->win))
						activate_window(task->win);
				} else {
					window_action(task, action);
				}
			} else {
				window_action(task, action);
			}
		}
	} else {
		window_action(click_task(panel, e->xbutton.x, e->xbutton.y), action);
	}

	// to keep window below
	if (panel_layer == BOTTOM_LAYER)
		XLowerWindow(server.display, panel->main_win);
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
	panel_refresh = TRUE;
}

void update_task_desktop(Task *task)
{
	// fprintf(stderr, "%s %d:\n", __FUNCTION__, __LINE__);
	Window win = task->win;
	remove_task(task);
	task = add_task(win);
	reset_active_task();
	panel_refresh = TRUE;
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
				place_panel_all_desktops(p);
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
			if (old_num_desktops != server.num_desktops) {
				if (server.num_desktops <= server.desktop) {
					server.desktop = server.num_desktops - 1;
				}
				cleanup_taskbar();
				init_taskbar();
				for (int i = 0; i < num_panels; i++) {
					init_taskbar_panel(&panels[i]);
					set_panel_items_order(&panels[i]);
					update_taskbar_visibility(&panels[i]);
					panels[i].area.resize_needed = 1;
				}
				taskbar_refresh_tasklist();
				reset_active_task();
				panel_refresh = TRUE;
			} else if (old_desktop != server.desktop) {
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
								panel_refresh = TRUE;
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
			panel_refresh = TRUE;
		}
		// Change active
		else if (at == server.atom._NET_ACTIVE_WINDOW) {
			if (debug)
				fprintf(stderr, "%s %d: win = root, atom = _NET_ACTIVE_WINDOW\n", __FUNCTION__, __LINE__);
			reset_active_task();
			panel_refresh = TRUE;
		} else if (at == server.atom._XROOTPMAP_ID || at == server.atom._XROOTMAP_ID) {
			if (debug)
				fprintf(stderr, "%s %d: win = root, atom = _XROOTPMAP_ID\n", __FUNCTION__, __LINE__);
			// change Wallpaper
			for (int i = 0; i < num_panels; i++) {
				set_panel_background(&panels[i]);
			}
			panel_refresh = TRUE;
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
						panel_refresh = TRUE;
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
				panel_refresh = TRUE;
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
				panel_refresh = TRUE;
			}
		} else if (at == server.atom.WM_STATE) {
			// Iconic state
			TaskState state = (active_task && task->win == active_task->win ? TASK_ACTIVE : TASK_NORMAL);
			if (window_is_iconified(win))
				state = TASK_ICONIFIED;
			set_task_state(task, state);
			panel_refresh = TRUE;
		}
		// Window icon changed
		else if (at == server.atom._NET_WM_ICON) {
			task_update_icon(task);
			panel_refresh = TRUE;
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
	panel_refresh = TRUE;
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
				panel_refresh = TRUE;
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

const char *GetAtomName(Display *disp, Atom a)
{
	if (a == None)
		return "None";
	else
		return XGetAtomName(disp, a);
}

typedef struct Property {
	unsigned char *data;
	int format, nitems;
	Atom type;
} Property;

// This fetches all the data from a property
struct Property read_property(Display *disp, Window w, Atom property)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long bytes_after;
	unsigned char *ret = 0;

	int read_bytes = 1024;

	// Keep trying to read the property until there are no
	// bytes unread.
	do {
		if (ret != 0)
			XFree(ret);
		XGetWindowProperty(disp,
						   w,
						   property,
						   0,
						   read_bytes,
						   False,
						   AnyPropertyType,
						   &actual_type,
						   &actual_format,
						   &nitems,
						   &bytes_after,
						   &ret);
		read_bytes *= 2;
	} while (bytes_after != 0);

	fprintf(stderr, "DnD %s:%d: Property:\n", __FILE__, __LINE__);
	fprintf(stderr, "DnD %s:%d: Actual type: %s\n", __FILE__, __LINE__, GetAtomName(disp, actual_type));
	fprintf(stderr, "DnD %s:%d: Actual format: %d\n", __FILE__, __LINE__, actual_format);
	fprintf(stderr, "DnD %s:%d: Number of items: %lu\n", __FILE__, __LINE__, nitems);

	Property p;
	p.data = ret;
	p.format = actual_format;
	p.nitems = nitems;
	p.type = actual_type;

	return p;
}

// This function takes a list of targets which can be converted to (atom_list, nitems)
// and a list of acceptable targets with prioritees (datatypes). It returns the highest
// entry in datatypes which is also in atom_list: ie it finds the best match.
Atom pick_target_from_list(Display *disp, Atom *atom_list, int nitems)
{
	Atom to_be_requested = None;
	int i;
	for (i = 0; i < nitems; i++) {
		const char *atom_name = GetAtomName(disp, atom_list[i]);
		fprintf(stderr, "DnD %s:%d: Type %d = %s\n", __FILE__, __LINE__, i, atom_name);

		// See if this data type is allowed and of higher priority (closer to zero)
		// than the present one.
		if (strcmp(atom_name, "STRING") == 0) {
			to_be_requested = atom_list[i];
		}
	}

	return to_be_requested;
}

// Finds the best target given up to three atoms provided (any can be None).
// Useful for part of the Xdnd protocol.
Atom pick_target_from_atoms(Display *disp, Atom t1, Atom t2, Atom t3)
{
	Atom atoms[3];
	int n = 0;

	if (t1 != None)
		atoms[n++] = t1;

	if (t2 != None)
		atoms[n++] = t2;

	if (t3 != None)
		atoms[n++] = t3;

	return pick_target_from_list(disp, atoms, n);
}

// Finds the best target given a local copy of a property.
Atom pick_target_from_targets(Display *disp, Property p)
{
	// The list of targets is a list of atoms, so it should have type XA_ATOM
	// but it may have the type TARGETS instead.

	if ((p.type != XA_ATOM && p.type != server.atom.TARGETS) || p.format != 32) {
		// This would be really broken. Targets have to be an atom list
		// and applications should support this. Nevertheless, some
		// seem broken (MATLAB 7, for instance), so ask for STRING
		// next instead as the lowest common denominator
		return XA_STRING;
	} else {
		Atom *atom_list = (Atom *)p.data;

		return pick_target_from_list(disp, atom_list, p.nitems);
	}
}

void dnd_enter(XClientMessageEvent *e)
{
	dnd_atom = None;
	int more_than_3 = e->data.l[1] & 1;
	dnd_source_window = e->data.l[0];
	dnd_version = (e->data.l[1] >> 24);

	fprintf(stderr, "DnD %s:%d: DnDEnter\n", __FILE__, __LINE__);
	fprintf(stderr, "DnD %s:%d: DnDEnter. Supports > 3 types = %s\n", __FILE__, __LINE__, more_than_3 ? "yes" : "no");
	fprintf(stderr, "DnD %s:%d: Protocol version = %d\n", __FILE__, __LINE__, dnd_version);
	fprintf(stderr, "DnD %s:%d: Type 1 = %s\n", __FILE__, __LINE__, GetAtomName(server.display, e->data.l[2]));
	fprintf(stderr, "DnD %s:%d: Type 2 = %s\n", __FILE__, __LINE__, GetAtomName(server.display, e->data.l[3]));
	fprintf(stderr, "DnD %s:%d: Type 3 = %s\n", __FILE__, __LINE__, GetAtomName(server.display, e->data.l[4]));

	// Query which conversions are available and pick the best

	if (more_than_3) {
		// Fetch the list of possible conversions
		// Notice the similarity to TARGETS with paste.
		Property p = read_property(server.display, dnd_source_window, server.atom.XdndTypeList);
		dnd_atom = pick_target_from_targets(server.display, p);
		XFree(p.data);
	} else {
		// Use the available list
		dnd_atom = pick_target_from_atoms(server.display, e->data.l[2], e->data.l[3], e->data.l[4]);
	}

	fprintf(stderr, "DnD %s:%d: Requested type = %s\n", __FILE__, __LINE__, GetAtomName(server.display, dnd_atom));
}

void dnd_position(XClientMessageEvent *e)
{
	dnd_target_window = e->window;
	int accept = 0;
	Panel *panel = get_panel(e->window);
	int x, y, mapX, mapY;
	Window child;
	x = (e->data.l[2] >> 16) & 0xFFFF;
	y = e->data.l[2] & 0xFFFF;
	XTranslateCoordinates(server.display, server.root_win, e->window, x, y, &mapX, &mapY, &child);
	Task *task = click_task(panel, mapX, mapY);
	if (task) {
		if (task->desktop != server.desktop)
			change_desktop(task->desktop);
		window_action(task, TOGGLE);
	} else {
		LauncherIcon *icon = click_launcher_icon(panel, mapX, mapY);
		if (icon) {
			accept = 1;
			dnd_launcher_exec = icon->cmd;
		} else {
			dnd_launcher_exec = 0;
		}
	}

	// send XdndStatus event to get more XdndPosition events
	XClientMessageEvent se;
	se.type = ClientMessage;
	se.window = e->data.l[0];
	se.message_type = server.atom.XdndStatus;
	se.format = 32;
	se.data.l[0] = e->window;      // XID of the target window
	se.data.l[1] = accept ? 1 : 0; // bit 0: accept drop    bit 1: send XdndPosition events if inside rectangle
	se.data.l[2] = 0;              // Rectangle x,y for which no more XdndPosition events
	se.data.l[3] = (1 << 16) | 1;  // Rectangle w,h for which no more XdndPosition events
	if (accept) {
		se.data.l[4] = dnd_version >= 2 ? e->data.l[4] : server.atom.XdndActionCopy;
	} else {
		se.data.l[4] = None; // None = drop will not be accepted
	}

	XSendEvent(server.display, e->data.l[0], False, NoEventMask, (XEvent *)&se);
}

void dnd_drop(XClientMessageEvent *e)
{
	if (dnd_target_window && dnd_launcher_exec) {
		if (dnd_version >= 1) {
			XConvertSelection(server.display,
							  server.atom.XdndSelection,
							  XA_STRING,
							  dnd_selection,
							  dnd_target_window,
							  e->data.l[2]);
		} else {
			XConvertSelection(server.display,
							  server.atom.XdndSelection,
							  XA_STRING,
							  dnd_selection,
							  dnd_target_window,
							  CurrentTime);
		}
	} else {
		// The source is sending anyway, despite instructions to the contrary.
		// So reply that we're not interested.
		XClientMessageEvent m;
		memset(&m, 0, sizeof(m));
		m.type = ClientMessage;
		m.display = e->display;
		m.window = e->data.l[0];
		m.message_type = server.atom.XdndFinished;
		m.format = 32;
		m.data.l[0] = dnd_target_window;
		m.data.l[1] = 0;
		m.data.l[2] = None; // Failed.
		XSendEvent(server.display, e->data.l[0], False, NoEventMask, (XEvent *)&m);
	}
}

int main(int argc, char *argv[])
{
start:
	init(argc, argv);

	init_X11_pre_config();

	if (!config_read()) {
		fprintf(stderr,
				"Could not read config file.\n"
				"Usage: tint2 [[-c] <config_file>]\n");
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

	int damage_event, damage_error;
	XDamageQueryExtension(server.display, &damage_event, &damage_error);
	int x11_fd = ConnectionNumber(server.display);
	XSync(server.display, False);

	// XDND initialization
	dnd_source_window = 0;
	dnd_target_window = 0;
	dnd_version = 0;
	dnd_selection = XInternAtom(server.display, "PRIMARY", 0);
	dnd_atom = None;
	dnd_sent_request = 0;
	dnd_launcher_exec = 0;

	int ufd = uevent_init();
	int hidden_dnd = 0;

	while (1) {
		if (panel_refresh) {
			if (systray_profile)
				fprintf(stderr,
						BLUE "[%f] %s:%d redrawing panel" RESET "\n",
						profiling_get_time(),
						__FUNCTION__,
						__LINE__);
			panel_refresh = FALSE;

			for (int i = 0; i < num_panels; i++) {
				Panel *panel = &panels[i];

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
					if (panel->temp_pmap)
						XFreePixmap(server.display, panel->temp_pmap);
					panel->temp_pmap = XCreatePixmap(server.display,
													 server.root_win,
													 panel->area.width,
													 panel->area.height,
													 server.depth);
					render_panel(panel);
					if (panel == (Panel *)systray.area.panel) {
						if (refresh_systray && panel && !panel->is_hidden) {
							refresh_systray = FALSE;
							XSetWindowBackgroundPixmap(server.display, panel->main_win, panel->temp_pmap);
							refresh_systray_icons();
						}
					}
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
				}
			}
			XFlush(server.display);
		}

		// Create a File Description Set containing x11_fd
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(x11_fd, &fdset);
		int maxfd = x11_fd;
		if (sn_pipe_valid) {
			FD_SET(sn_pipe[0], &fdset);
			maxfd = maxfd < sn_pipe[0] ? sn_pipe[0] : maxfd;
		}
		for (GList *l = panel_config.execp_list; l; l = l->next) {
			Execp *execp = (Execp *)l->data;
			int fd = execp->backend->child_pipe;
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
		if (XPending(server.display) > 0 || select(maxfd + 1, &fdset, 0, 0, select_timeout) >= 0) {
			uevent_handler();

			if (sn_pipe_valid) {
				char buffer[1];
				while (read(sn_pipe[0], buffer, sizeof(buffer)) > 0) {
					sigchld_handler_async();
				}
			}
			for (GList *l = panel_config.execp_list; l; l = l->next) {
				Execp *execp = (Execp *)l->data;
				if (read_execp(execp)) {
					GList *l_instance;
					for (l_instance = execp->backend->instances; l_instance; l_instance = l_instance->next) {
						Execp *instance = l_instance->data;
						instance->area.resize_needed = TRUE;
						panel_refresh = TRUE;
					}
				}
			}
			if (XPending(server.display) > 0) {
				XEvent e;
				XNextEvent(server.display, &e);
#if HAVE_SN
				if (startup_notifications)
					sn_display_process_event(server.sn_display, &e);
#endif // HAVE_SN

				Panel *panel = get_panel(e.xany.window);
				if (panel && panel_autohide) {
					if (e.type == EnterNotify)
						autohide_trigger_show(panel);
					else if (e.type == LeaveNotify)
						autohide_trigger_hide(panel);
					if (panel->is_hidden) {
						if (e.type == ClientMessage && e.xclient.message_type == server.atom.XdndPosition) {
							hidden_dnd = 1;
							autohide_show(panel);
						} else
							continue; // discard further processing of this event because the panel is not visible yet
					} else if (hidden_dnd && e.type == ClientMessage &&
							   e.xclient.message_type == server.atom.XdndLeave) {
						hidden_dnd = 0;
						autohide_hide(panel);
					}
				}

				switch (e.type) {
				case ButtonPress: {
					tooltip_hide(0);
					event_button_press(&e);
					Area *area = find_area_under_mouse(panel, e.xbutton.x, e.xbutton.y);
					if (panel_config.mouse_effects)
						mouse_over(area, 1);
					break;
				}

				case ButtonRelease: {
					event_button_release(&e);
					Area *area = find_area_under_mouse(panel, e.xbutton.x, e.xbutton.y);
					if (panel_config.mouse_effects)
						mouse_over(area, 0);
					break;
				}

				case MotionNotify: {
					unsigned int button_mask = Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask;
					if (e.xmotion.state & button_mask)
						event_button_motion_notify(&e);

					Area *area = find_area_under_mouse(panel, e.xmotion.x, e.xmotion.y);
					if (area->_get_tooltip_text)
						tooltip_trigger_show(area, panel, &e);
					else
						tooltip_trigger_hide();
					if (panel_config.mouse_effects)
						mouse_over(area, e.xmotion.state & button_mask);
					break;
				}

				case LeaveNotify:
					tooltip_trigger_hide();
					if (panel_config.mouse_effects)
						mouse_out();
					break;

				case Expose:
					event_expose(&e);
					break;

				case PropertyNotify:
					event_property_notify(&e);
					break;

				case ConfigureNotify:
					event_configure_notify(&e);
					break;

				case ConfigureRequest: {
					TrayWindow *traywin = systray_find_icon(e.xany.window);
					if (traywin)
						systray_reconfigure_event(traywin, &e);
					break;
				}

				case ResizeRequest: {
					TrayWindow *traywin = systray_find_icon(e.xany.window);
					if (traywin)
						systray_resize_request_event(traywin, &e);
					break;
				}

				case ReparentNotify: {
					if (!systray_enabled)
						break;
					Panel *systray_panel = (Panel *)systray.area.panel;
					if (e.xany.window == systray_panel->main_win) // don't care
						break;
					TrayWindow *traywin = systray_find_icon(e.xreparent.window);
					if (traywin) {
						if (traywin->win == e.xreparent.window) {
							if (traywin->parent == e.xreparent.parent) {
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
					if (e.xany.window == server.composite_manager) {
						// Stop real_transparency
						fprintf(stderr,
								YELLOW "%s %d: triggering tint2 restart due to compositor shutdown" RESET "\n",
								__FILE__,
								__LINE__);
						signal_pending = SIGUSR1;
						break;
					}
					if (e.xany.window == g_tooltip.window || !systray_enabled)
						break;
					for (GSList *it = systray.list_icons; it; it = g_slist_next(it)) {
						if (((TrayWindow *)it->data)->win == e.xany.window) {
							systray_destroy_event((TrayWindow *)it->data);
							break;
						}
					}
					break;

				case ClientMessage: {
					XClientMessageEvent *ev = &e.xclient;
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
					if (systray_enabled && e.xclient.message_type == server.atom._NET_SYSTEM_TRAY_OPCODE &&
						e.xclient.format == 32 && e.xclient.window == net_sel_win) {
						net_message(&e.xclient);
					} else if (e.xclient.message_type == server.atom.XdndEnter) {
						dnd_enter(&e.xclient);
					} else if (e.xclient.message_type == server.atom.XdndPosition) {
						dnd_position(&e.xclient);
					} else if (e.xclient.message_type == server.atom.XdndDrop) {
						dnd_drop(&e.xclient);
					}
					break;
				}

				case SelectionNotify: {
					Atom target = e.xselection.target;

					fprintf(stderr, "DnD %s:%d: A selection notify has arrived!\n", __FILE__, __LINE__);
					fprintf(stderr, "DnD %s:%d: Requestor = %lu\n", __FILE__, __LINE__, e.xselectionrequest.requestor);
					fprintf(stderr,
							"DnD %s:%d: Selection atom = %s\n",
							__FILE__,
							__LINE__,
							GetAtomName(server.display, e.xselection.selection));
					fprintf(stderr,
							"DnD %s:%d: Target atom    = %s\n",
							__FILE__,
							__LINE__,
							GetAtomName(server.display, target));
					fprintf(stderr,
							"DnD %s:%d: Property atom  = %s\n",
							__FILE__,
							__LINE__,
							GetAtomName(server.display, e.xselection.property));

					if (e.xselection.property != None && dnd_launcher_exec) {
						Property prop = read_property(server.display, dnd_target_window, dnd_selection);

						// If we're being given a list of targets (possible conversions)
						if (target == server.atom.TARGETS && !dnd_sent_request) {
							dnd_sent_request = 1;
							dnd_atom = pick_target_from_targets(server.display, prop);

							if (dnd_atom == None) {
								fprintf(stderr, "No matching datatypes.\n");
							} else {
								// Request the data type we are able to select
								fprintf(stderr, "Now requsting type %s", GetAtomName(server.display, dnd_atom));
								XConvertSelection(server.display,
												  dnd_selection,
												  dnd_atom,
												  dnd_selection,
												  dnd_target_window,
												  CurrentTime);
							}
						} else if (target == dnd_atom) {
							// Dump the binary data
							fprintf(stderr, "DnD %s:%d: Data begins:\n", __FILE__, __LINE__);
							fprintf(stderr, "--------\n");
							for (int i = 0; i < prop.nitems * prop.format / 8; i++)
								fprintf(stderr, "%c", ((char *)prop.data)[i]);
							fprintf(stderr, "--------\n");

							int cmd_length = 0;
							cmd_length += 1;                             // (
							cmd_length += strlen(dnd_launcher_exec) + 1; // exec + space
							cmd_length += 1;                             // open double quotes
							for (int i = 0; i < prop.nitems * prop.format / 8; i++) {
								char c = ((char *)prop.data)[i];
								if (c == '\n') {
									if (i < prop.nitems * prop.format / 8 - 1) {
										cmd_length += 3; // close double quotes, space, open double quotes
									}
								} else if (c == '\r') {
									// Nothing to do
								} else {
									cmd_length += 1; // 1 character
									if (c == '`' || c == '$' || c == '\\') {
										cmd_length += 1; // escape with one backslash
									}
								}
							}
							cmd_length += 1; // close double quotes
							cmd_length += 2; // &)
							cmd_length += 1; // terminator

							char *cmd = calloc(cmd_length, 1);
							cmd[0] = '\0';
							strcat(cmd, "(");
							strcat(cmd, dnd_launcher_exec);
							strcat(cmd, " \"");
							for (int i = 0; i < prop.nitems * prop.format / 8; i++) {
								char c = ((char *)prop.data)[i];
								if (c == '\n') {
									if (i < prop.nitems * prop.format / 8 - 1) {
										strcat(cmd, "\" \"");
									}
								} else if (c == '\r') {
									// Nothing to do
								} else {
									if (c == '`' || c == '$' || c == '\\') {
										strcat(cmd, "\\");
									}
									char sc[2];
									sc[0] = c;
									sc[1] = '\0';
									strcat(cmd, sc);
								}
							}
							strcat(cmd, "\"");
							strcat(cmd, "&)");
							fprintf(stderr, "DnD %s:%d: Running command: %s\n", __FILE__, __LINE__, cmd);
							tint_exec(cmd);
							free(cmd);

							// Reply OK.
							XClientMessageEvent m;
							memset(&m, 0, sizeof(m));
							m.type = ClientMessage;
							m.display = server.display;
							m.window = dnd_source_window;
							m.message_type = server.atom.XdndFinished;
							m.format = 32;
							m.data.l[0] = dnd_target_window;
							m.data.l[1] = 1;
							m.data.l[2] = server.atom.XdndActionCopy; // We only ever copy.
							XSendEvent(server.display, dnd_source_window, False, NoEventMask, (XEvent *)&m);
							XSync(server.display, False);
						}

						XFree(prop.data);
					}
					break;
				}

				default:
					if (e.type == XDamageNotify + damage_event) {
						XDamageNotifyEvent *de = (XDamageNotifyEvent *)&e;
						TrayWindow *traywin = systray_find_icon(de->drawable);
						if (traywin)
							systray_render_icon(traywin);
					}
				}
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
			} else {
				// SIGINT, SIGTERM, SIGHUP
				exit(0);
			}
		}
	}
}
