/**************************************************************************
*
* Copyright (C) 2009 Andreas.Fink (Andreas.Fink85@gmail.com)
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

#include <stdio.h>
#include <unistd.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include "server.h"
#include "tooltip.h"
#include "panel.h"

// TODO: Use timer_create instead of setitimer, because SIGALRM is not the right signal for this...
// Reason: If we want to implement autohide we have to use another signal...

static int x, y, width, height;

// give the tooltip some reasonable default values
Tooltip g_tooltip = {
	.task = 0,
	.window = 0,
	.show_timeout = { .it_interval={0, 0}, .it_value={0, 0} },
	.hide_timeout = { .it_interval={0, 0}, .it_value={0, 0} },
	.enabled = False,
	.current_state = TOOLTIP_ABOUT_TO_HIDE,
	.mapped = False,
	.paddingx = 0,
	.paddingy = 0,
	.font_color = { .color={1, 1, 1}, .alpha=1 },
	.background_color = { .color={0.5, 0.4, 0.5}, .alpha=1 },
	.border = { .color={0, 0, 0}, .alpha=1,  .width=1, .rounded=0 },
	.font_desc = 0
};

void init_tooltip()
{
	if (!g_tooltip.font_desc)
		g_tooltip.font_desc = pango_font_description_from_string("sans 10");

	XSetWindowAttributes attr;
	attr.override_redirect = True;
	attr.event_mask = ExposureMask;
	if (g_tooltip.window) XDestroyWindow(server.dsp, g_tooltip.window);
	g_tooltip.window = XCreateWindow(server.dsp, server.root_win, 0, 0, 100, 20, 0, server.depth, InputOutput, CopyFromParent, CWOverrideRedirect|CWEventMask, &attr);
}


void cleanup_tooltip()
{
	alarm(0);
	tooltip_hide();
	g_tooltip.enabled = False;
	g_tooltip.current_state = TOOLTIP_ABOUT_TO_HIDE;
	if (g_tooltip.task) {
		g_tooltip.task = 0;
	}
	if (g_tooltip.window) {
		XDestroyWindow(server.dsp, g_tooltip.window);
		g_tooltip.window = 0;
	}
	if (g_tooltip.font_desc) {
		pango_font_description_free(g_tooltip.font_desc);
		g_tooltip.font_desc = 0;
	}
}


void tooltip_sighandler(int sig)
{
	if (g_tooltip.current_state == TOOLTIP_ABOUT_TO_SHOW)
		tooltip_show();
	else if (g_tooltip.current_state == TOOLTIP_ABOUT_TO_HIDE)
		tooltip_hide();
}


void tooltip_trigger_show(Task* task, int x_root, int y_root)
{
	x = x_root;
	y = y_root;

	if (g_tooltip.mapped && g_tooltip.task != task) {
		g_tooltip.task = task;
		tooltip_update();
		alarm(0);
	}
	else if (!g_tooltip.mapped) {
		g_tooltip.current_state = TOOLTIP_ABOUT_TO_SHOW;
		g_tooltip.task = task;
		struct timeval t = g_tooltip.show_timeout.it_value;
		if (t.tv_sec == 0 && t.tv_usec == 0) {
			alarm(0);
			tooltip_show();
		}
		else
			setitimer(ITIMER_REAL, &g_tooltip.show_timeout, 0);
	}
}


void tooltip_show()
{
	if (!g_tooltip.mapped) {
		g_tooltip.mapped = True;
		XMapWindow(server.dsp, g_tooltip.window);
		tooltip_update();
		alarm(0);
	}
}


void tooltip_update_geometry()
{
	cairo_surface_t *cs;
	cairo_t *c;
	PangoLayout* layout;
	cs = cairo_xlib_surface_create(server.dsp, g_tooltip.window, server.visual, width, height);
	c = cairo_create(cs);
	layout = pango_cairo_create_layout(c);
	pango_layout_set_font_description(layout, g_tooltip.font_desc);
	pango_layout_set_text(layout, g_tooltip.task->title, -1);
	PangoRectangle r1, r2;
	pango_layout_get_pixel_extents(layout, &r1, &r2);
	width = 2*g_tooltip.border.width + 2*g_tooltip.paddingx + r2.width;
	height = 2*g_tooltip.border.width + 2*g_tooltip.paddingy + r2.height;

	Panel* panel = g_tooltip.task->area.panel;
	if (panel_horizontal && panel_position & BOTTOM)
		y = panel->posy-height;
	else if (panel_horizontal && panel_position & TOP)
		y = panel->posy + panel->area.height;
	else if (panel_position & LEFT)
		x = panel->posx + panel->area.width;
	else
			x = panel->posx - width;
	g_object_unref(layout);
	cairo_destroy(c);
	cairo_surface_destroy(cs);
}


void tooltip_adjust_geometry()
{
	// adjust coordinates and size to not go offscreen
	// it seems quite impossible that the height needs to be adjusted, but we do it anyway.

	int min_x, min_y, max_width, max_height;
	Panel* panel = g_tooltip.task->area.panel;
	int screen_width = server.monitor[panel->monitor].width;
	int screen_height = server.monitor[panel->monitor].height;
	if ( x+width <= screen_width && y+height <= screen_height && x>=0 && y>=0)
		return;    // no adjustment needed

	if (panel_horizontal) {
		min_x=0;
		max_width=screen_width;
		max_height=screen_height-panel->area.height;
		if (panel_position & BOTTOM)
			min_y=0;
		else
			min_y=panel->area.height;
	}
	else {
		max_width=screen_width-panel->area.width;
		min_y=0;
		max_height=screen_height;
		if (panel_position & LEFT)
			min_x=panel->area.width;
		else
			min_x=0;
	}

	if (x+width > server.monitor[panel->monitor].width)
		x = server.monitor[panel->monitor].width-width;
	if ( y+height>server.monitor[panel->monitor].height)
		y = server.monitor[panel->monitor].height-height;

	if (x<min_x)
		x=min_x;
	if (width>max_width)
		width = max_width;
	if (y<min_y)
		y=min_y;
	if (height>max_height)
		height=max_height;
}

void tooltip_update()
{
	if (!g_tooltip.task) {
		tooltip_hide();
		return;
	}

	tooltip_update_geometry();
	tooltip_adjust_geometry();
	XMoveResizeWindow(server.dsp, g_tooltip.window, x, y, width, height);

	// Stuff for drawing the tooltip
	cairo_surface_t *cs;
	cairo_t *c;
	PangoLayout* layout;
	cs = cairo_xlib_surface_create(server.dsp, g_tooltip.window, server.visual, width, height);
	c = cairo_create(cs);
	Color bc = g_tooltip.background_color;
	cairo_rectangle(c, 0, 0, width, height);
	cairo_set_source_rgb(c, bc.color[0], bc.color[1], bc.color[2]);
	cairo_fill(c);
	Border b = g_tooltip.border;
	cairo_set_source_rgba(c, b.color[0], b.color[1], b.color[2], b.alpha);
	cairo_set_line_width(c, b.width);
	cairo_rectangle(c, b.width/2.0, b.width/2.0, width-b.width, height-b.width);
	cairo_stroke(c);

	config_color fc = g_tooltip.font_color;
	cairo_set_source_rgba(c, fc.color[0], fc.color[1], fc.color[2], fc.alpha);
	layout = pango_cairo_create_layout(c);
	pango_layout_set_font_description(layout, g_tooltip.font_desc);
	pango_layout_set_text(layout, g_tooltip.task->title, -1);
	PangoRectangle r1, r2;
	pango_layout_get_pixel_extents(layout, &r1, &r2);
	pango_layout_set_width(layout, width*PANGO_SCALE);
	pango_layout_set_height(layout, height*PANGO_SCALE);
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
	// I do not know why this is the right way, but with the below cairo_move_to it seems to be centered (horiz. and vert.)
	cairo_move_to(c, -r1.x/2+g_tooltip.border.width+g_tooltip.paddingx, -r1.y/2+g_tooltip.border.width+g_tooltip.paddingy);
	pango_cairo_show_layout (c, layout);

	g_object_unref (layout);
	cairo_destroy (c);
	cairo_surface_destroy (cs);
}


void tooltip_trigger_hide(Tooltip* tooltip)
{
	if (g_tooltip.mapped) {
		g_tooltip.current_state = TOOLTIP_ABOUT_TO_HIDE;
		struct timeval t = g_tooltip.hide_timeout.it_value;
		g_tooltip.task = 0;
		if (t.tv_sec == 0 && t.tv_usec == 0) {
			tooltip_hide();
			alarm(0);
		}
		else
			setitimer(ITIMER_REAL, &g_tooltip.hide_timeout, 0);
	}
	else {
		// tooltip not visible yet, but maybe an alarm is still pending
		alarm(0);
	}
}


void tooltip_hide()
{
	if (g_tooltip.mapped) {
		g_tooltip.mapped = False;
		XUnmapWindow(server.dsp, g_tooltip.window);
	}
}
