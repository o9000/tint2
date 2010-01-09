/**************************************************************************
*
* Tint2 : task
*
* Copyright (C) 2007 Pål Staurland (staura@gmail.com)
* Modified (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <unistd.h>

#include "window.h"
#include "task.h"
#include "taskbar.h"
#include "server.h"
#include "panel.h"
#include "tooltip.h"
#include "timer.h"

static timeout* urgent_timeout = 0;
static GSList* urgent_list = 0;

const char* task_get_tooltip(void* obj)
{
	Task* t = obj;
	return t->title;
}


Task *add_task (Window win)
{
	if (!win) return 0;
	if (window_is_hidden(win)) return 0;

	int monitor;

	Task new_tsk;
	new_tsk.win = win;
	new_tsk.desktop = window_get_desktop (win);
	if (nb_panel > 1) {
		monitor = window_get_monitor (win);
		if (monitor >= nb_panel) monitor = 0;
	}
	else monitor = 0;
	new_tsk.area.panel = &panel1[monitor];
	new_tsk.current_state = window_is_iconified(win) ? TASK_ICONIFIED : TASK_NORMAL;

	// allocate only one title and one icon
	// even with task_on_all_desktop and with task_on_all_panel
	new_tsk.title = 0;
	int k;
	for (k=0; k<TASK_STATE_COUNT; ++k)
		new_tsk.icon[k] = 0;
	get_title(&new_tsk);
	get_icon(&new_tsk);

	//printf("task %s : desktop %d, monitor %d\n", new_tsk->title, desktop, monitor);
	XSelectInput (server.dsp, new_tsk.win, PropertyChangeMask|StructureNotifyMask);

	Taskbar *tskbar;
	Task *new_tsk2=0;
	int i, j;
	// QUESTION: Do we need to iterate over nb_panel (we know the monitor, why not taking panel1[monitor]???
	for (i=0 ; i < nb_panel ; i++) {
		for (j=0 ; j < panel1[i].nb_desktop ; j++) {
			if (new_tsk.desktop != ALLDESKTOP && new_tsk.desktop != j) continue;
			if (nb_panel > 1 && panel1[i].monitor != monitor) continue;

			tskbar = &panel1[i].taskbar[j];
			new_tsk2 = malloc(sizeof(Task));
			memcpy(&new_tsk2->area, &panel1[i].g_task.area, sizeof(Area));
			new_tsk2->area.parent = tskbar;
			new_tsk2->win = new_tsk.win;
			new_tsk2->desktop = new_tsk.desktop;
			set_task_state(new_tsk2, new_tsk.current_state);
			if (new_tsk2->desktop == ALLDESKTOP && server.desktop != j) {
				// hide ALLDESKTOP task on non-current desktop
				new_tsk2->area.on_screen = 0;
			}
			new_tsk2->title = new_tsk.title;
			new_tsk2->area._get_tooltip_text = task_get_tooltip;
			for (k=0; k<TASK_STATE_COUNT; ++k)
				new_tsk2->icon[k] = new_tsk.icon[k];
			new_tsk2->icon_width = new_tsk.icon_width;
			new_tsk2->icon_height = new_tsk.icon_height;
			tskbar->area.list = g_slist_append(tskbar->area.list, new_tsk2);
			tskbar->area.resize = 1;
			//printf("add_task panel %d, desktop %d, task %s\n", i, j, new_tsk2->title);
		}
	}
	if (window_is_urgent(win))
		add_urgent(new_tsk2);
	return new_tsk2;
}


void remove_task (Task *tsk)
{
	if (!tsk) return;

	Window win = tsk->win;
	int desktop = tsk->desktop;

	// free title and icon just for the first task
	// even with task_on_all_desktop and with task_on_all_panel
	//printf("remove_task %s %d\n", tsk->title, tsk->desktop);
	if (tsk->title)
		free (tsk->title);
	int k;
	for (k=0; k<TASK_STATE_COUNT; ++k) {
		if (tsk->icon[k]) {
			imlib_context_set_image(tsk->icon[k]);
			imlib_free_image();
			tsk->icon[k] = 0;
		}
	}

	int i, j;
	Task *tsk2;
	Taskbar *tskbar;
	for (i=0 ; i < nb_panel ; i++) {
		for (j=0 ; j < panel1[i].nb_desktop ; j++) {
			if (desktop != ALLDESKTOP && desktop != j) continue;

			GSList *l0;
			tskbar = &panel1[i].taskbar[j];
			for (l0 = tskbar->area.list; l0 ; ) {
				tsk2 = l0->data;
				l0 = l0->next;
				if (win == tsk2->win) {
					tskbar->area.list = g_slist_remove(tskbar->area.list, tsk2);
					tskbar->area.resize = 1;

					if (tsk2 == task_active)
						task_active = 0;
					if (tsk2 == task_drag)
						task_drag = 0;
					if (g_slist_find(urgent_list, tsk2))
						del_urgent(tsk2);

					XFreePixmap (server.dsp, tsk2->area.pix);
					free(tsk2);
				}
			}
		}
	}
}


void get_title(Task *tsk)
{
	Panel *panel = tsk->area.panel;
	char *title, *name;

	if (!panel->g_task.text && !g_tooltip.enabled) return;

	name = server_get_property (tsk->win, server.atom._NET_WM_VISIBLE_NAME, server.atom.UTF8_STRING, 0);
	if (!name || !strlen(name)) {
		name = server_get_property (tsk->win, server.atom._NET_WM_NAME, server.atom.UTF8_STRING, 0);
		if (!name || !strlen(name)) {
			name = server_get_property (tsk->win, server.atom.WM_NAME, XA_STRING, 0);
			if (!name || !strlen(name)) {
				name = malloc(10);
				strcpy(name, "Untitled");
			}
		}
	}

	// add space before title
	title = malloc(strlen(name)+2);
	if (panel->g_task.icon) strcpy(title, " ");
	else title[0] = 0;
	strcat(title, name);
	if (name) XFree (name);

	tsk->area.redraw = 1;
	if (tsk->title)
		free(tsk->title);
	tsk->title = title;
}


void get_icon (Task *tsk)
{
	Panel *panel = tsk->area.panel;
	if (!panel->g_task.icon) return;
	int i;
	Imlib_Image img = NULL;
	XWMHints *hints = 0;
	long *data = 0;

	int k;
	for (k=0; k<TASK_STATE_COUNT; ++k) {
		if (tsk->icon[k]) {
			imlib_context_set_image(tsk->icon[k]);
			imlib_free_image();
			tsk->icon[k] = 0;
		}
	}
	tsk->area.redraw = 1;

	data = server_get_property (tsk->win, server.atom._NET_WM_ICON, XA_CARDINAL, &i);
	if (data) {
		// get ARGB icon
		int w, h;
		long *tmp_data;

		tmp_data = get_best_icon (data, get_icon_count (data, i), i, &w, &h, panel->g_task.icon_size1);

#ifdef __x86_64__
		DATA32 icon_data[w * h];
		int length = w * h;
		for (i = 0; i < length; ++i)
			icon_data[i] =  tmp_data[i];
		img = imlib_create_image_using_copied_data (w, h, icon_data);
#else
		img = imlib_create_image_using_data (w, h, (DATA32*)tmp_data);
#endif
	}
	else {
		// get Pixmap icon
		hints = XGetWMHints(server.dsp, tsk->win);
		if (hints) {
			if (hints->flags & IconPixmapHint && hints->icon_pixmap != 0) {
				// get width, height and depth for the pixmap
				Window root;
				int  icon_x, icon_y;
				uint border_width, bpp;
				uint w, h;

				//printf("  get pixmap\n");
				XGetGeometry(server.dsp, hints->icon_pixmap, &root, &icon_x, &icon_y, &w, &h, &border_width, &bpp);
				imlib_context_set_drawable(hints->icon_pixmap);
				img = imlib_create_image_from_drawable(hints->icon_mask, 0, 0, w, h, 0);
			}
		}
	}
	if (img == NULL) {
		imlib_context_set_image(default_icon);
		img = imlib_clone_image();
	}

	// transform icons
	imlib_context_set_image(img);
	imlib_image_set_has_alpha(1);
	int w, h;
	w = imlib_image_get_width();
	h = imlib_image_get_height();
	Imlib_Image orig_image = imlib_create_cropped_scaled_image(0, 0, w, h, panel->g_task.icon_size1, panel->g_task.icon_size1);
	imlib_free_image();

	imlib_context_set_image(orig_image);
	tsk->icon_width = imlib_image_get_width();
	tsk->icon_height = imlib_image_get_height();
	for (k=0; k<TASK_STATE_COUNT; ++k) {
		imlib_context_set_image(orig_image);
		tsk->icon[k] = imlib_clone_image();
		imlib_context_set_image(tsk->icon[k]);
		DATA32 *data32;
		if (panel->g_task.alpha[k] != 100 || panel->g_task.saturation[k] != 0 || panel->g_task.brightness[k] != 0) {
			data32 = imlib_image_get_data();
			adjust_asb(data32, tsk->icon_width, tsk->icon_height, panel->g_task.alpha[k], (float)panel->g_task.saturation[k]/100, (float)panel->g_task.brightness[k]/100);
			imlib_image_put_back_data(data32);
		}
	}
	imlib_context_set_image(orig_image);
	imlib_free_image();

	if (hints)
		XFree(hints);
	if (data)
		XFree (data);
}


void draw_task_icon (Task *tsk, int text_width)
{
	if (tsk->icon[tsk->current_state] == 0) return;

	// Find pos
	int pos_x;
	Panel *panel = (Panel*)tsk->area.panel;
	if (panel->g_task.centered) {
		if (panel->g_task.text)
			pos_x = (tsk->area.width - text_width - panel->g_task.icon_size1) / 2;
		else
			pos_x = (tsk->area.width - panel->g_task.icon_size1) / 2;
	}
	else pos_x = panel->g_task.area.paddingxlr + tsk->area.bg->border.width;

	// Render
	imlib_context_set_image (tsk->icon[tsk->current_state]);
	if (real_transparency) {
		render_image(tsk->area.pix, pos_x, panel->g_task.icon_posy, imlib_image_get_width(), imlib_image_get_height() );
	}
	else {
		imlib_context_set_drawable(tsk->area.pix);
		imlib_render_image_on_drawable (pos_x, panel->g_task.icon_posy);
	}
}


void draw_task (void *obj, cairo_t *c)
{
	Task *tsk = obj;
	PangoLayout *layout;
	Color *config_text;
	int width=0, height;
	Panel *panel = (Panel*)tsk->area.panel;

	if (panel->g_task.text) {
		/* Layout */
		layout = pango_cairo_create_layout (c);
		pango_layout_set_font_description (layout, panel->g_task.font_desc);
		pango_layout_set_text (layout, tsk->title, -1);

		/* Drawing width and Cut text */
		// pango use U+22EF or U+2026
		pango_layout_set_width (layout, ((Taskbar*)tsk->area.parent)->text_width * PANGO_SCALE);
		pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

		/* Center text */
		if (panel->g_task.centered) pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
		else pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);

		pango_layout_get_pixel_size (layout, &width, &height);

		config_text = &panel->g_task.font[tsk->current_state];

		cairo_set_source_rgba (c, config_text->color[0], config_text->color[1], config_text->color[2], config_text->alpha);

		pango_cairo_update_layout (c, layout);
		cairo_move_to (c, panel->g_task.text_posx, panel->g_task.text_posy);
		pango_cairo_show_layout (c, layout);

		if (panel->g_task.font_shadow) {
			cairo_set_source_rgba (c, 0.0, 0.0, 0.0, 0.5);
			pango_cairo_update_layout (c, layout);
			cairo_move_to (c, panel->g_task.text_posx + 1, panel->g_task.text_posy + 1);
			pango_cairo_show_layout (c, layout);
		}
		g_object_unref (layout);
	}

	if (panel->g_task.icon) {
		// icon use same opacity as text
		draw_task_icon (tsk, width);
	}
}


Task *next_task(Task *tsk)
{
	GSList *l0;
	int i, j;
	Task *tsk1;

	for (i=0 ; i < nb_panel ; i++) {
		for (j=0 ; j < panel1[i].nb_desktop ; j++) {
			for (l0 = panel1[i].taskbar[j].area.list; l0 ; l0 = l0->next) {
				tsk1 = l0->data;
				if (tsk1 == tsk) {
					if (l0->next == NULL) l0 = panel1[i].taskbar[j].area.list;
					else l0 = l0->next;
					return l0->data;
				}
			}
		}
	}

	return NULL;
}

Task *prev_task(Task *tsk)
{
	GSList *l0;
	int i, j;
	Task *tsk1, *tsk2;

	for (i=0 ; i < nb_panel ; i++) {
		for (j=0 ; j < panel1[i].nb_desktop ; j++) {
			tsk2 = NULL;
			for (l0 = panel1[i].taskbar[j].area.list; l0 ; l0 = l0->next) {
				tsk1 = l0->data;
				if (tsk1 == tsk) {
					if (l0 == panel1[i].taskbar[j].area.list) {
						l0 = g_slist_last ( l0 );
						tsk2 = l0->data;
					}
					return tsk2;
				}
				tsk2 = tsk1;
			}
		}
	}

	return NULL;
}


void active_task()
{
	GSList *l0;
	int i, j;
	Task *tsk1, *tsk2;

	if (task_active) {
		for (i=0 ; i < nb_panel ; i++) {
			for (j=0 ; j < panel1[i].nb_desktop ; j++) {
				for (l0 = panel1[i].taskbar[j].area.list; l0 ; l0 = l0->next) {
					tsk1 = l0->data;
					if (tsk1->win == task_active->win)
						set_task_state(tsk1, window_is_iconified(tsk1->win) ? TASK_ICONIFIED : TASK_NORMAL);
				}
			}
		}
		task_active = 0;
	}

	Window w1 = window_get_active ();
	//printf("Change active task %ld\n", w1);

	tsk2 = task_get_task(w1);
	if (!tsk2) {
		Window w2;
		if (XGetTransientForHint(server.dsp, w1, &w2) != 0)
			if (w2) tsk2 = task_get_task(w2);
	}

	if ( g_slist_find(urgent_list, tsk2) )
		del_urgent(tsk2);

	// put active state on all task (multi_desktop)
	if (tsk2) {
		for (i=0 ; i < nb_panel ; i++) {
			for (j=0 ; j < panel1[i].nb_desktop ; j++) {
				for (l0 = panel1[i].taskbar[j].area.list; l0 ; l0 = l0->next) {
					tsk1 = l0->data;
					if (tsk1->win == tsk2->win)
						set_task_state(tsk1, TASK_ACTIVE);
				}
			}
		}
		task_active = tsk2;
	}
}


void set_task_state(Task *tsk, int state)
{
	tsk->current_state = state;
	tsk->area.bg = panel1[0].g_task.background[state];
	tsk->area.redraw = 1;
}


void blink_urgent(void* arg)
{
	GSList* urgent_task = urgent_list;
	while (urgent_task) {
		Task* t = urgent_task->data;
		if ( t->urgent_tick < max_tick_urgent) {
			if (t->urgent_tick++ % 2)
				set_task_state(t, TASK_URGENT);
			else
				set_task_state(t, window_is_iconified(t->win) ? TASK_ICONIFIED : TASK_NORMAL);
		}
		urgent_task = urgent_task->next;
	}
	panel_refresh = 1;
}


void add_urgent(Task *tsk)
{
	if (!tsk)
		return;

	// some programs set urgency hint although they are active
	if ( task_active && task_active->win == tsk->win )
		return;

	// reset counter to 0, remove tasks which are already in the list
	GSList* urgent_add = task_get_tasks(tsk->win);
	GSList* it = urgent_add;
	Task* tsk2;
	while (it) {
		tsk2 = it->data;
		tsk2->urgent_tick = 0;
		it = it->next;
		if (g_slist_find(urgent_list, tsk2))
			urgent_add = g_slist_remove(urgent_add, tsk2);
	}

	// not yet in the list, so we have to add it
	urgent_list = g_slist_concat(urgent_add, urgent_list);

	if (urgent_timeout == 0)
		urgent_timeout = add_timeout(10, 1000, blink_urgent, 0);
}


void del_urgent(Task *tsk)
{
	if (!tsk)
		return;
	
	GSList* urgent_del = task_get_tasks(tsk->win);
	GSList* it = urgent_del;
	while(it) {
		urgent_list = g_slist_remove(urgent_list, it->data);
		it = it->next;
	}
	g_slist_free(urgent_del);
	if (!urgent_list) {
		stop_timeout(urgent_timeout);
		urgent_timeout = 0;
	}
}
