#include "execplugin.h"

#include <string.h>
#include <stdio.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <math.h>
#include <pango/pangocairo.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

#include "window.h"
#include "server.h"
#include "panel.h"
#include "timer.h"
#include "common.h"

void execp_timer_callback(void *arg);
char *execp_get_tooltip(void *obj);
void execp_init_fonts();
int execp_compute_desired_size(void *obj);
void execp_dump_geometry(void *obj, int indent);

void default_execp()
{
}

Execp *create_execp()
{
	Execp *execp = calloc(1, sizeof(Execp));
	execp->backend = calloc(1, sizeof(ExecpBackend));
	execp->backend->child_pipe = -1;
	execp->backend->cmd_pids = g_tree_new(cmp_ptr);
	execp->backend->interval = 30;
	execp->backend->cache_icon = TRUE;
	execp->backend->centered = TRUE;
	execp->backend->font_color.alpha = 0.5;
	return execp;
}

gpointer create_execp_frontend(gconstpointer arg, gpointer data)
{
	Execp *execp_backend = (Execp *)arg;

	Execp *execp_frontend = calloc(1, sizeof(Execp));
	execp_frontend->backend = execp_backend->backend;
	execp_backend->backend->instances = g_list_append(execp_backend->backend->instances, execp_frontend);
	execp_frontend->frontend = calloc(1, sizeof(ExecpFrontend));
	return execp_frontend;
}

void destroy_execp(void *obj)
{
	Execp *execp = (Execp *)obj;
	if (execp->frontend) {
		// This is a frontend element
		execp->backend->instances = g_list_remove_all(execp->backend->instances, execp);
		free_and_null(execp->frontend);
		remove_area(&execp->area);
		free_area(&execp->area);
		free_and_null(execp);
	} else {
		// This is a backend element
		stop_timeout(execp->backend->timer);
		execp->backend->timer = NULL;

		if (execp->backend->icon) {
			imlib_context_set_image(execp->backend->icon);
			imlib_free_image();
			execp->backend->icon = NULL;
		}
		free_and_null(execp->backend->buf_output);
		free_and_null(execp->backend->text);
		free_and_null(execp->backend->icon_path);
		if (execp->backend->child) {
			kill(-execp->backend->child, SIGHUP);
			execp->backend->child = 0;
		}
		if (execp->backend->child_pipe >= 0) {
			close(execp->backend->child_pipe);
			execp->backend->child_pipe = -1;
		}
		if (execp->backend->cmd_pids) {
			g_tree_destroy(execp->backend->cmd_pids);
			execp->backend->cmd_pids = NULL;
		}

		execp->backend->bg = NULL;
		pango_font_description_free(execp->backend->font_desc);
		execp->backend->font_desc = NULL;
		free_and_null(execp->backend->command);
		free_and_null(execp->backend->tooltip);
		free_and_null(execp->backend->lclick_command);
		free_and_null(execp->backend->mclick_command);
		free_and_null(execp->backend->rclick_command);
		free_and_null(execp->backend->dwheel_command);
		free_and_null(execp->backend->uwheel_command);

		if (execp->backend->instances) {
			fprintf(stderr, "Error: Attempt to destroy backend while there are still frontend instances!\n");
			exit(-1);
		}
		free(execp->backend);
		free(execp);
	}
}

void init_execp()
{
	GList *to_remove = panel_config.execp_list;
	for (int k = 0; k < strlen(panel_items_order) && to_remove; k++) {
		if (panel_items_order[k] == 'E') {
			to_remove = to_remove->next;
		}
	}

	if (to_remove) {
		if (to_remove == panel_config.execp_list) {
			g_list_free_full(to_remove, destroy_execp);
			panel_config.execp_list = NULL;
		} else {
			// Cut panel_config.execp_list
			if (to_remove->prev)
				to_remove->prev->next = NULL;
			to_remove->prev = NULL;
			// Remove all elements of to_remove and to_remove itself
			g_list_free_full(to_remove, destroy_execp);
		}
	}

	execp_init_fonts();
	for (GList *l = panel_config.execp_list; l; l = l->next) {
		Execp *execp = l->data;

		// Set missing config options
		if (!execp->backend->bg)
			execp->backend->bg = &g_array_index(backgrounds, Background, 0);
		execp->backend->buf_capacity = 1024;
		execp->backend->buf_output = calloc(execp->backend->buf_capacity, 1);
		execp->backend->text = strdup(" ");
		execp->backend->icon_path = NULL;
	}
}

void init_execp_panel(void *p)
{
	Panel *panel = (Panel *)p;

	// Make sure this is only done once if there are multiple items
	if (panel->execp_list && ((Execp *)panel->execp_list->data)->frontend)
		return;

	// panel->execp_list is now a copy of the pointer panel_config.execp_list
	// We make it a deep copy
	panel->execp_list = g_list_copy_deep(panel_config.execp_list, create_execp_frontend, NULL);

	for (GList *l = panel->execp_list; l; l = l->next) {
		Execp *execp = l->data;
		execp->area.bg = execp->backend->bg;
		execp->area.paddingx = execp->backend->paddingx;
		execp->area.paddingy = execp->backend->paddingy;
		execp->area.paddingxlr = execp->backend->paddingxlr;
		execp->area.parent = panel;
		execp->area.panel = panel;
		execp->area._dump_geometry = execp_dump_geometry;
		execp->area._compute_desired_size = execp_compute_desired_size;
		snprintf(execp->area.name,
		         sizeof(execp->area.name),
		         "Execp %s",
		         execp->backend->command ? execp->backend->command : "null");
		execp->area._draw_foreground = draw_execp;
		execp->area.size_mode = LAYOUT_FIXED;
		execp->area._resize = resize_execp;
		execp->area._get_tooltip_text = execp_get_tooltip;
		execp->area._is_under_mouse = full_width_area_is_under_mouse;
		execp->area.has_mouse_press_effect =
		    panel_config.mouse_effects &&
		    (execp->area.has_mouse_over_effect = execp->backend->lclick_command || execp->backend->mclick_command ||
		                                         execp->backend->rclick_command || execp->backend->uwheel_command ||
		                                         execp->backend->dwheel_command);

		execp->area.resize_needed = TRUE;
		execp->area.on_screen = TRUE;
		instantiate_area_gradients(&execp->area);

		if (!execp->backend->timer)
			execp->backend->timer = add_timeout(10, 0, execp_timer_callback, execp, &execp->backend->timer);
	}
}

void execp_init_fonts()
{
	for (GList *l = panel_config.execp_list; l; l = l->next) {
		Execp *execp = l->data;
		if (!execp->backend->font_desc)
			execp->backend->font_desc = pango_font_description_from_string(get_default_font());
	}
}

void execp_default_font_changed()
{
	gboolean needs_update = FALSE;
	for (GList *l = panel_config.execp_list; l; l = l->next) {
		Execp *execp = l->data;

		if (!execp->backend->has_font) {
			pango_font_description_free(execp->backend->font_desc);
			execp->backend->font_desc = NULL;
			needs_update = TRUE;
		}
	}
	if (!needs_update)
		return;

	execp_init_fonts();
	for (int i = 0; i < num_panels; i++) {
		for (GList *l = panels[i].execp_list; l; l = l->next) {
			Execp *execp = l->data;

			if (!execp->backend->has_font) {
				execp->area.resize_needed = TRUE;
				schedule_redraw(&execp->area);
			}
		}
	}
	panel_refresh = TRUE;
}

void cleanup_execp()
{
	// Cleanup frontends
	for (int i = 0; i < num_panels; i++) {
		g_list_free_full(panels[i].execp_list, destroy_execp);
		panels[i].execp_list = NULL;
	}

	// Cleanup backends
	g_list_free_full(panel_config.execp_list, destroy_execp);
	panel_config.execp_list = NULL;
}

// Called from backend functions.
gboolean reload_icon(Execp *execp)
{
	char *icon_path = execp->backend->icon_path;

	if (execp->backend->has_icon && icon_path) {
		if (execp->backend->icon) {
			imlib_context_set_image(execp->backend->icon);
			imlib_free_image();
		}
		execp->backend->icon = load_image(icon_path, execp->backend->cache_icon);
		if (execp->backend->icon) {
			imlib_context_set_image(execp->backend->icon);
			int w = imlib_image_get_width();
			int h = imlib_image_get_height();
			if (w && h) {
				if (execp->backend->icon_w) {
					if (!execp->backend->icon_h) {
						h = (int)(0.5 + h * execp->backend->icon_w / (float)(w));
						w = execp->backend->icon_w;
					} else {
						w = execp->backend->icon_w;
						h = execp->backend->icon_h;
					}
				} else {
					if (execp->backend->icon_h) {
						w = (int)(0.5 + w * execp->backend->icon_h / (float)(h));
						h = execp->backend->icon_h;
					}
				}
				if (w < 1)
					w = 1;
				if (h < 1)
					h = 1;
			}
			if (w != imlib_image_get_width() || h != imlib_image_get_height()) {
				Imlib_Image icon_scaled =
				    imlib_create_cropped_scaled_image(0, 0, imlib_image_get_width(), imlib_image_get_height(), w, h);
				imlib_context_set_image(execp->backend->icon);
				imlib_free_image();
				execp->backend->icon = icon_scaled;
			}
			return TRUE;
		}
	}
	return FALSE;
}

int execp_compute_desired_size(void *obj)
{
	Execp *execp = (Execp *)obj;
	Panel *panel = (Panel *)execp->area.panel;
	int horiz_padding = (panel_horizontal ? execp->area.paddingxlr : execp->area.paddingy);
	int vert_padding = (panel_horizontal ? execp->area.paddingy : execp->area.paddingxlr);
	int interior_padding = execp->area.paddingx;

	int icon_w, icon_h;
	if (reload_icon(execp)) {
		if (execp->backend->icon) {
			imlib_context_set_image(execp->backend->icon);
			icon_w = imlib_image_get_width();
			icon_h = imlib_image_get_height();
		} else {
			icon_w = icon_h = 0;
		}
	} else {
		icon_w = icon_h = 0;
	}

	int text_next_line = !panel_horizontal && icon_w > execp->area.width / 2;

	int txt_height_ink, txt_height, txt_width;
	if (panel_horizontal) {
		get_text_size2(execp->backend->font_desc,
					   &txt_height_ink,
					   &txt_height,
					   &txt_width,
					   panel->area.height,
					   panel->area.width,
					   execp->backend->text,
					   strlen(execp->backend->text),
					   PANGO_WRAP_WORD_CHAR,
					   PANGO_ELLIPSIZE_NONE,
					   execp->backend->has_markup);
	} else {
		get_text_size2(execp->backend->font_desc,
					   &txt_height_ink,
					   &txt_height,
					   &txt_width,
					   panel->area.height,
					   !text_next_line
						   ? execp->area.width - icon_w - (icon_w ? interior_padding : 0) - 2 * horiz_padding -
								 left_right_border_width(&execp->area)
						   : execp->area.width - 2 * horiz_padding - left_right_border_width(&execp->area),
					   execp->backend->text,
					   strlen(execp->backend->text),
					   PANGO_WRAP_WORD_CHAR,
					   PANGO_ELLIPSIZE_NONE,
					   execp->backend->has_markup);
	}

	if (panel_horizontal) {
		int new_size = txt_width;
		if (icon_w)
			new_size += interior_padding + icon_w;
		new_size += 2 * horiz_padding + left_right_border_width(&execp->area);
		return new_size;
	} else {
		int new_size;
		if (!text_next_line) {
			new_size = txt_height + 2 * vert_padding + top_bottom_border_width(&execp->area);
			new_size = MAX(new_size, icon_h + 2 * vert_padding + top_bottom_border_width(&execp->area));
		} else {
			new_size =
				icon_h + interior_padding + txt_height + 2 * vert_padding + top_bottom_border_width(&execp->area);
		}
		return new_size;
	}
}

gboolean resize_execp(void *obj)
{
	Execp *execp = (Execp *)obj;
	Panel *panel = (Panel *)execp->area.panel;
	int horiz_padding = (panel_horizontal ? execp->area.paddingxlr : execp->area.paddingy);
	int vert_padding = (panel_horizontal ? execp->area.paddingy : execp->area.paddingxlr);
	int interior_padding = execp->area.paddingx;

	int icon_w, icon_h;
	if (reload_icon(execp)) {
		if (execp->backend->icon) {
			imlib_context_set_image(execp->backend->icon);
			icon_w = imlib_image_get_width();
			icon_h = imlib_image_get_height();
		} else {
			icon_w = icon_h = 0;
		}
	} else {
		icon_w = icon_h = 0;
	}

	int text_next_line = !panel_horizontal && icon_w > execp->area.width / 2;

	int txt_height_ink, txt_height, txt_width;
	if (panel_horizontal) {
		get_text_size2(execp->backend->font_desc,
		               &txt_height_ink,
		               &txt_height,
		               &txt_width,
		               panel->area.height,
		               panel->area.width,
		               execp->backend->text,
		               strlen(execp->backend->text),
		               PANGO_WRAP_WORD_CHAR,
		               PANGO_ELLIPSIZE_NONE,
		               execp->backend->has_markup);
	} else {
		get_text_size2(execp->backend->font_desc,
		               &txt_height_ink,
		               &txt_height,
		               &txt_width,
		               panel->area.height,
		               !text_next_line
		                   ? execp->area.width - icon_w - (icon_w ? interior_padding : 0) - 2 * horiz_padding -
		                         left_right_border_width(&execp->area)
		                   : execp->area.width - 2 * horiz_padding - left_right_border_width(&execp->area),
		               execp->backend->text,
		               strlen(execp->backend->text),
		               PANGO_WRAP_WORD_CHAR,
		               PANGO_ELLIPSIZE_NONE,
		               execp->backend->has_markup);
	}

	gboolean result = FALSE;
	if (panel_horizontal) {
		int new_size = txt_width;
		if (icon_w)
			new_size += interior_padding + icon_w;
		new_size += 2 * horiz_padding + left_right_border_width(&execp->area);
		if (new_size > execp->area.width || new_size < (execp->area.width - 6)) {
			// we try to limit the number of resize
			execp->area.width = new_size + 1;
			result = TRUE;
		}
	} else {
		int new_size;
		if (!text_next_line) {
			new_size = txt_height + 2 * vert_padding + top_bottom_border_width(&execp->area);
			new_size = MAX(new_size, icon_h + 2 * vert_padding + top_bottom_border_width(&execp->area));
		} else {
			new_size =
			    icon_h + interior_padding + txt_height + 2 * vert_padding + top_bottom_border_width(&execp->area);
		}
		if (new_size != execp->area.height) {
			execp->area.height = new_size;
			result = TRUE;
		}
	}
	execp->frontend->textw = txt_width;
	execp->frontend->texth = txt_height;
	if (execp->backend->centered) {
		if (icon_w) {
			if (!text_next_line) {
				execp->frontend->icony = (execp->area.height - icon_h) / 2;
				execp->frontend->iconx = (execp->area.width - txt_width - interior_padding - icon_w) / 2;
				execp->frontend->texty = (execp->area.height - txt_height) / 2;
				execp->frontend->textx = execp->frontend->iconx + icon_w + interior_padding;
			} else {
				execp->frontend->icony = (execp->area.height - icon_h - interior_padding - txt_height) / 2;
				execp->frontend->iconx = (execp->area.width - icon_w) / 2;
				execp->frontend->texty = execp->frontend->icony + icon_h + interior_padding;
				execp->frontend->textx = (execp->area.width - txt_width) / 2;
			}
		} else {
			execp->frontend->texty = (execp->area.height - txt_height) / 2;
			execp->frontend->textx = (execp->area.width - txt_width) / 2;
		}
	} else {
		if (icon_w) {
			if (!text_next_line) {
				execp->frontend->icony = (execp->area.height - icon_h) / 2;
				execp->frontend->iconx = left_border_width(&execp->area) + horiz_padding;
				execp->frontend->texty = (execp->area.height - txt_height) / 2;
				execp->frontend->textx = execp->frontend->iconx + icon_w + interior_padding;
			} else {
				execp->frontend->icony = (execp->area.height - icon_h - interior_padding - txt_height) / 2;
				execp->frontend->iconx = left_border_width(&execp->area) + horiz_padding;
				execp->frontend->texty = execp->frontend->icony + icon_h + interior_padding;
				execp->frontend->textx = execp->frontend->iconx;
			}
		} else {
			execp->frontend->texty = (execp->area.height - txt_height) / 2;
			execp->frontend->textx = left_border_width(&execp->area) + horiz_padding;
		}
	}

	schedule_redraw(&execp->area);

	return result;
}

void draw_execp(void *obj, cairo_t *c)
{
	Execp *execp = obj;
	PangoLayout *layout = pango_cairo_create_layout(c);

	if (execp->backend->has_icon && execp->backend->icon) {
		imlib_context_set_image(execp->backend->icon);
		// Render icon
		render_image(execp->area.pix, execp->frontend->iconx, execp->frontend->icony);
	}

	// draw layout
	pango_layout_set_font_description(layout, execp->backend->font_desc);
	pango_layout_set_width(layout, execp->frontend->textw * PANGO_SCALE);
	pango_layout_set_alignment(layout, execp->backend->centered ? PANGO_ALIGN_CENTER : PANGO_ALIGN_LEFT);
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
	if (!execp->backend->has_markup)
		pango_layout_set_text(layout, execp->backend->text, strlen(execp->backend->text));
	else
		pango_layout_set_markup(layout, execp->backend->text, strlen(execp->backend->text));

	pango_cairo_update_layout(c, layout);
	draw_text(layout,
	          c,
	          execp->frontend->textx,
	          execp->frontend->texty,
	          &execp->backend->font_color,
	          panel_config.font_shadow);

	g_object_unref(layout);
}

void execp_dump_geometry(void *obj, int indent)
{
	Execp *execp = obj;

	if (execp->backend->has_icon && execp->backend->icon) {
		Imlib_Image tmp = imlib_context_get_image();
		imlib_context_set_image(execp->backend->icon);
		fprintf(stderr,
		        "%*sIcon: x = %d, y = %d, w = %d, h = %d\n",
		        indent,
		        "",
		        execp->frontend->iconx,
		        execp->frontend->icony,
		        imlib_image_get_width(),
		        imlib_image_get_height());
		if (tmp)
			imlib_context_set_image(tmp);
	}
	fprintf(stderr,
	        "%*sText: x = %d, y = %d, w = %d, align = %s, text = %s\n",
	        indent,
	        "",
	        execp->frontend->textx,
	        execp->frontend->texty,
	        execp->frontend->textw,
	        execp->backend->centered ? "center" : "left",
	        execp->backend->text);
}

void execp_force_update(Execp *execp)
{
	if (execp->backend->child_pipe > 0) {
		// Command currently running, nothing to do
	} else {
		if (execp->backend->timer)
			stop_timeout(execp->backend->timer);
		// Run command right away
		execp->backend->timer = add_timeout(10, 0, execp_timer_callback, execp, &execp->backend->timer);
	}
}

void execp_action(void *obj, int button, int x, int y)
{
	Execp *execp = obj;
	char *command = NULL;
	switch (button) {
	case 1:
		command = execp->backend->lclick_command;
		break;
	case 2:
		command = execp->backend->mclick_command;
		break;
	case 3:
		command = execp->backend->rclick_command;
		break;
	case 4:
		command = execp->backend->uwheel_command;
		break;
	case 5:
		command = execp->backend->dwheel_command;
		break;
	}
	if (command) {
		char *full_cmd = g_strdup_printf("export EXECP_X=%d;"
		                                 "export EXECP_Y=%d;"
		                                 "export EXECP_W=%d;"
		                                 "export EXECP_H=%d; %s",
		                                 x,
		                                 y,
		                                 execp->area.width,
		                                 execp->area.height,
		                                 command);
		pid_t pid = fork();
		if (pid < 0) {
			fprintf(stderr, "Could not fork\n");
		} else if (pid == 0) {
			// Child process
			// Allow children to exist after parent destruction
			setsid();
			// Run the command
			execl("/bin/sh", "/bin/sh", "-c", full_cmd, NULL);
			fprintf(stderr, "Failed to execlp %s\n", full_cmd);
			exit(1);
		}
		// Parent process
		g_tree_insert(execp->backend->cmd_pids, GINT_TO_POINTER(pid), GINT_TO_POINTER(1));
		g_free(full_cmd);
	} else {
		execp_force_update(execp);
	}
}

void execp_cmd_completed(Execp *execp, pid_t pid)
{
	g_tree_remove(execp->backend->cmd_pids, GINT_TO_POINTER(pid));
	execp_force_update(execp);
}

void execp_timer_callback(void *arg)
{
	Execp *execp = arg;

	if (!execp->backend->command)
		return;

	// Still running!
	if (execp->backend->child_pipe > 0)
		return;

	int pipe_fd[2];
	if (pipe(pipe_fd)) {
		// TODO maybe write this in tooltip, but if this happens we're screwed anyways
		fprintf(stderr, "Execp: Creating pipe failed!\n");
		return;
	}

	fcntl(pipe_fd[0], F_SETFL, O_NONBLOCK | fcntl(pipe_fd[0], F_GETFL));

	// Fork and run command, capturing stdout in pipe
	pid_t child = fork();
	if (child == -1) {
		// TODO maybe write this in tooltip, but if this happens we're screwed anyways
		fprintf(stderr, "Fork failed.\n");
		close(pipe_fd[1]);
		close(pipe_fd[0]);
		return;
	} else if (child == 0) {
		// We are in the child
		close(pipe_fd[0]);
		dup2(pipe_fd[1], 1); // 1 is stdout
		close(pipe_fd[1]);
		setpgid(0, 0);
		execl("/bin/sh", "/bin/sh", "-c", execp->backend->command, NULL);
		// This should never happen!
		fprintf(stdout, "execl() failed\nexecl() failed\n");
		fflush(stdout);
		exit(0);
	}
	close(pipe_fd[1]);
	execp->backend->child = child;
	execp->backend->child_pipe = pipe_fd[0];
	execp->backend->buf_length = 0;
	execp->backend->buf_output[execp->backend->buf_length] = '\0';
	execp->backend->last_update_start_time = time(NULL);
}

gboolean read_execp(void *obj)
{
	Execp *execp = (Execp *)obj;

	if (execp->backend->child_pipe < 0)
		return FALSE;

	gboolean command_finished = FALSE;
	while (1) {
		// Make sure there is free space in the buffer
		if (execp->backend->buf_capacity - execp->backend->buf_length < 1024) {
			execp->backend->buf_capacity *= 2;
			execp->backend->buf_output = realloc(execp->backend->buf_output, execp->backend->buf_capacity);
		}
		ssize_t count = read(execp->backend->child_pipe,
		                     execp->backend->buf_output + execp->backend->buf_length,
		                     execp->backend->buf_capacity - execp->backend->buf_length - 1);
		if (count > 0) {
			// Successful read
			execp->backend->buf_length += count;
			execp->backend->buf_output[execp->backend->buf_length] = '\0';
			continue;
		} else if (count == 0) {
			// End of file
			command_finished = TRUE;
			break;
		} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
			// No more data available at the moment
			break;
		} else if (errno == EINTR) {
			// Harmless interruption by signal
			continue;
		} else {
			// Error
			command_finished = TRUE;
			break;
		}
		break;
	}

	if (command_finished) {
		execp->backend->child = 0;
		close(execp->backend->child_pipe);
		execp->backend->child_pipe = -1;
		if (execp->backend->interval)
			execp->backend->timer =
			    add_timeout(execp->backend->interval * 1000, 0, execp_timer_callback, execp, &execp->backend->timer);
	}

	if (!execp->backend->continuous && command_finished) {
		free_and_null(execp->backend->text);
		free_and_null(execp->backend->icon_path);
		if (!execp->backend->has_icon) {
			execp->backend->text = strdup(execp->backend->buf_output);
		} else {
			char *text = strchr(execp->backend->buf_output, '\n');
			if (text) {
				*text = '\0';
				text++;
				execp->backend->text = strdup(text);
			} else {
				execp->backend->text = strdup("");
			}
			execp->backend->icon_path = strdup(execp->backend->buf_output);
		}
		int len = strlen(execp->backend->text);
		if (len > 0 && execp->backend->text[len - 1] == '\n')
			execp->backend->text[len - 1] = '\0';
		execp->backend->buf_length = 0;
		execp->backend->buf_output[execp->backend->buf_length] = '\0';
		execp->backend->last_update_finish_time = time(NULL);
		execp->backend->last_update_duration =
		    execp->backend->last_update_finish_time - execp->backend->last_update_start_time;
		return TRUE;
	} else if (execp->backend->continuous > 0) {
		// Count lines in buffer
		int num_lines = 0;
		char *last = execp->backend->buf_output;
		char *end = NULL;
		for (char *c = execp->backend->buf_output; *c; c++) {
			if (*c == '\n') {
				num_lines++;
				if (num_lines == execp->backend->continuous)
					end = c;
			}
			last = c;
		}
		if (*last && *last != '\n')
			num_lines++;
		if (num_lines >= execp->backend->continuous) {
			if (end)
				*end = '\0';
			free_and_null(execp->backend->text);
			free_and_null(execp->backend->icon_path);
			if (!execp->backend->has_icon) {
				execp->backend->text = strdup(execp->backend->buf_output);
			} else {
				char *text = strchr(execp->backend->buf_output, '\n');
				if (text) {
					*text = '\0';
					text++;
					execp->backend->text = strdup(text);
				} else {
					execp->backend->text = strdup("");
				}
				execp->backend->icon_path = strdup(execp->backend->buf_output);
			}
			int len = strlen(execp->backend->text);
			if (len > 0 && execp->backend->text[len - 1] == '\n')
				execp->backend->text[len - 1] = '\0';

			if (end) {
				char *next = end + 1;
				int copied = next - execp->backend->buf_output;
				int remaining = execp->backend->buf_length - copied;
				if (remaining > 0) {
					memmove(execp->backend->buf_output, next, remaining);
					execp->backend->buf_length = remaining;
					execp->backend->buf_output[execp->backend->buf_length] = '\0';
				} else {
					execp->backend->buf_length = 0;
					execp->backend->buf_output[execp->backend->buf_length] = '\0';
				}
			}

			execp->backend->last_update_finish_time = time(NULL);
			execp->backend->last_update_duration =
			    execp->backend->last_update_finish_time - execp->backend->last_update_start_time;
			return TRUE;
		}
	}
	return FALSE;
}

const char *time_to_string(int seconds, char *buffer)
{
	if (seconds < 60) {
		sprintf(buffer, "%ds", seconds);
	} else if (seconds < 60 * 60) {
		int m = seconds / 60;
		seconds = seconds % 60;
		int s = seconds;
		sprintf(buffer, "%d:%ds", m, s);
	} else {
		int h = seconds / (60 * 60);
		seconds = seconds % (60 * 60);
		int m = seconds / 60;
		seconds = seconds % 60;
		int s = seconds;
		sprintf(buffer, "%d:%d:%ds", h, m, s);
	}
	return buffer;
}

char *execp_get_tooltip(void *obj)
{
	Execp *execp = obj;

	if (execp->backend->tooltip) {
		if (strlen(execp->backend->tooltip) > 0)
			return strdup(execp->backend->tooltip);
		else
			return NULL;
	}

	time_t now = time(NULL);

	char tmp_buf1[256];
	char tmp_buf2[256];
	char tmp_buf3[256];
	if (execp->backend->child_pipe < 0) {
		// Not executing command
		if (execp->backend->last_update_finish_time) {
			// We updated at least once
			if (execp->backend->interval > 0) {
				sprintf(execp->backend->tooltip_text,
				        "Last update finished %s ago (took %s). Next update starting in %s.",
				        time_to_string((int)(now - execp->backend->last_update_finish_time), tmp_buf1),
				        time_to_string((int)execp->backend->last_update_duration, tmp_buf2),
				        time_to_string((int)(execp->backend->interval - (now - execp->backend->last_update_finish_time)),
				                       tmp_buf3));
			} else {
				sprintf(execp->backend->tooltip_text,
				        "Last update finished %s ago (took %s).",
				        time_to_string((int)(now - execp->backend->last_update_finish_time), tmp_buf1),
				        time_to_string((int)execp->backend->last_update_duration, tmp_buf2));
			}
		} else {
			// we never requested an update
			sprintf(execp->backend->tooltip_text, "Never updated. No update scheduled.");
		}
	} else {
		// Currently executing command
		if (execp->backend->last_update_finish_time) {
			// we finished updating at least once
			sprintf(execp->backend->tooltip_text,
			        "Last update finished %s ago. Update in progress (started %s ago).",
			        time_to_string((int)(now - execp->backend->last_update_finish_time), tmp_buf1),
			        time_to_string((int)(now - execp->backend->last_update_start_time), tmp_buf3));
		} else {
			// we never finished an update
			sprintf(execp->backend->tooltip_text,
			        "First update in progress (started %s seconds ago).",
			        time_to_string((int)(now - execp->backend->last_update_start_time), tmp_buf1));
		}
	}
	return strdup(execp->backend->tooltip_text);
}
