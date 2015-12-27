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

#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#endif

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

int parse_line(const char *line, char **key, char **value)
{
	char *a, *b;

	/* Skip useless lines */
	if ((line[0] == '#') || (line[0] == '\n'))
		return 0;
	if (!(a = strchr(line, '=')))
		return 0;

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
	return 1;
}

void tint_exec(const char *command)
{
	if (command) {
		if (fork() == 0) {
			// change for the fork the signal mask
			//			sigset_t sigset;
			//			sigprocmask(SIG_SETMASK, &sigset, 0);
			//			sigprocmask(SIG_UNBLOCK, &sigset, 0);
			execl("/bin/sh", "/bin/sh", "-c", command, NULL);
			_exit(0);
		}
	}
}

char *expand_tilde(char *s)
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

char *contract_tilde(char *s)
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

void extract_values(const char *value, char **value1, char **value2, char **value3)
{
	char *b = 0, *c = 0;

	if (*value1)
		free(*value1);
	if (*value2)
		free(*value2);
	if (*value3)
		free(*value3);

	if ((b = strchr(value, ' '))) {
		b[0] = '\0';
		b++;
	} else {
		*value2 = 0;
		*value3 = 0;
	}
	*value1 = strdup(value);
	g_strstrip(*value1);

	if (b) {
		if ((c = strchr(b, ' '))) {
			c[0] = '\0';
			c++;
		} else {
			c = 0;
			*value3 = 0;
		}
		*value2 = strdup(b);
		g_strstrip(*value2);
	}

	if (c) {
		*value3 = strdup(c);
		g_strstrip(*value3);
	}
}

void adjust_asb(DATA32 *data, int w, int h, int alpha, float satur, float bright)
{
	unsigned int x, y;
	unsigned int a, r, g, b, argb;
	unsigned long id;
	int cmax, cmin;
	float h2, f, p, q, t;
	float hue, saturation, brightness;
	float redc, greenc, bluec;

	for (y = 0; y < h; y++) {
		for (id = y * w, x = 0; x < w; x++, id++) {
			argb = data[id];
			a = (argb >> 24) & 0xff;
			// transparent => nothing to do.
			if (a == 0)
				continue;
			r = (argb >> 16) & 0xff;
			g = (argb >> 8) & 0xff;
			b = (argb)&0xff;

			// convert RGB to HSB
			cmax = (r > g) ? r : g;
			if (b > cmax)
				cmax = b;
			cmin = (r < g) ? r : g;
			if (b < cmin)
				cmin = b;
			brightness = ((float)cmax) / 255.0f;
			if (cmax != 0)
				saturation = ((float)(cmax - cmin)) / ((float)cmax);
			else
				saturation = 0;
			if (saturation == 0)
				hue = 0;
			else {
				redc = ((float)(cmax - r)) / ((float)(cmax - cmin));
				greenc = ((float)(cmax - g)) / ((float)(cmax - cmin));
				bluec = ((float)(cmax - b)) / ((float)(cmax - cmin));
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

			// adjust
			saturation += satur;
			if (saturation < 0.0)
				saturation = 0.0;
			if (saturation > 1.0)
				saturation = 1.0;
			brightness += bright;
			if (brightness < 0.0)
				brightness = 0.0;
			if (brightness > 1.0)
				brightness = 1.0;
			if (alpha != 100)
				a = (a * alpha) / 100;

			// convert HSB to RGB
			if (saturation == 0) {
				r = g = b = (int)(brightness * 255.0f + 0.5f);
			} else {
				h2 = (hue - (int)hue) * 6.0f;
				f = h2 - (int)(h2);
				p = brightness * (1.0f - saturation);
				q = brightness * (1.0f - saturation * f);
				t = brightness * (1.0f - (saturation * (1.0f - f)));
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

			argb = a;
			argb = (argb << 8) + r;
			argb = (argb << 8) + g;
			argb = (argb << 8) + b;
			data[id] = argb;
		}
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

int pixel_empty(DATA32 argb)
{

	DATA32 a = (argb >> 24) & 0xff;
	if (a == 0)
		return 1;

	DATA32 rgb = argb & 0xffFFff;
	return rgb == 0;
}

int image_empty(DATA32 *data, int w, int h)
{
	if (w > 0 && h > 0) {
		int x = w / 2;
		int y = h / 2;
		if (!pixel_empty(data[y * w + x])) {
			// fprintf(stderr, "Non-empty pixel: [%u, %u] = %x\n", x, y, data[y * w + x]);
			return 0;
		}
	}

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			if (!pixel_empty(data[y * w + x])) {
				// fprintf(stderr, "Non-empty pixel: [%u, %u] = %x\n", x, y, data[y * w + x]);
				return 0;
			}
		}
	}

	// fprintf(stderr, "All pixels are empty\n");
	return 1;
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

	Pixmap pixmap = XCreatePixmap(server.dsp, server.root_win, w, h, 32);
	imlib_context_set_drawable(pixmap);
	imlib_context_set_blend(0);
	imlib_render_image_on_drawable(0, 0);

	Pixmap mask = XCreatePixmap(server.dsp, server.root_win, w, h, 32);
	imlib_context_set_drawable(mask);
	imlib_context_set_blend(0);
	imlib_render_image_on_drawable(0, 0);

	Picture pict = XRenderCreatePicture(server.dsp, pixmap, XRenderFindStandardFormat(server.dsp, PictStandardARGB32), 0, 0);
	Picture pict_drawable = XRenderCreatePicture(server.dsp, d, XRenderFindVisualFormat(server.dsp, server.visual), 0, 0);
	Picture pict_mask =	XRenderCreatePicture(server.dsp, mask, XRenderFindStandardFormat(server.dsp, PictStandardARGB32), 0, 0);
	XRenderComposite(server.dsp, PictOpOver, pict, pict_mask, pict_drawable, 0, 0, 0, 0, x, y, w, h);

	XRenderFreePicture(server.dsp, pict_mask);
	XRenderFreePicture(server.dsp, pict_drawable);
	XRenderFreePicture(server.dsp, pict);
	XFreePixmap(server.dsp, mask);
	XFreePixmap(server.dsp, pixmap);
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
		char suffix[128];
		sprintf(suffix, "tmpimage-%d.png", getpid());
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
				gchar *name = g_build_filename(g_get_user_config_dir(), "tint2", suffix, NULL);
				GdkPixbuf *pixbuf = rsvg_handle_get_pixbuf(svg);
				gdk_pixbuf_save(pixbuf, name, "png", NULL, NULL);
			}
			exit(0);
		} else {
			// Parent
			waitpid(pid, 0, 0);
			gchar *name = g_build_filename(g_get_user_config_dir(), "tint2", suffix, NULL);
			image = imlib_load_image_immediately_without_cache(name);
			g_remove(name);
			g_free(name);
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
			   alpha,
			   (float)saturation / 100,
			   (float)brightness / 100);
	imlib_image_put_back_data(data);
	return copy;
}

void draw_rect(cairo_t *c, double x, double y, double w, double h, double r)
{
	if (r > 0.0) {
		double c1 = 0.55228475 * r;

		cairo_move_to(c, x + r, y);
		cairo_rel_line_to(c, w - 2 * r, 0);
		cairo_rel_curve_to(c, c1, 0.0, r, c1, r, r);
		cairo_rel_line_to(c, 0, h - 2 * r);
		cairo_rel_curve_to(c, 0.0, c1, c1 - r, r, -r, r);
		cairo_rel_line_to(c, -w + 2 * r, 0);
		cairo_rel_curve_to(c, -c1, 0, -r, -c1, -r, -r);
		cairo_rel_line_to(c, 0, -h + 2 * r);
		cairo_rel_curve_to(c, 0, -c1, r - c1, -r, r, -r);
	} else
		cairo_rectangle(c, x, y, w, h);
}

void clear_pixmap(Pixmap p, int x, int y, int w, int h)
{
	Picture pict = XRenderCreatePicture(server.dsp, p, XRenderFindVisualFormat(server.dsp, server.visual), 0, 0);
	XRenderColor col;
	col.red = col.green = col.blue = col.alpha = 0;
	XRenderFillRectangle(server.dsp, PictOpSrc, pict, &col, x, y, w, h);
	XRenderFreePicture(server.dsp, pict);
}

void get_text_size2(PangoFontDescription *font,
					int *height_ink,
					int *height,
					int *width,
					int panel_height,
					int panel_width,
					char *text,
					int len,
					PangoWrapMode wrap,
					PangoEllipsizeMode ellipsis,
					gboolean markup)
{
	PangoRectangle rect_ink, rect;

	Pixmap pmap = XCreatePixmap(server.dsp, server.root_win, panel_height, panel_width, server.depth);

	cairo_surface_t *cs = cairo_xlib_surface_create(server.dsp, pmap, server.visual, panel_height, panel_width);
	cairo_t *c = cairo_create(cs);

	PangoLayout *layout = pango_cairo_create_layout(c);
	pango_layout_set_width(layout, panel_width * PANGO_SCALE);
	pango_layout_set_height(layout, panel_height * PANGO_SCALE);
	pango_layout_set_wrap(layout, wrap);
	pango_layout_set_ellipsize(layout, ellipsis);
	pango_layout_set_font_description(layout, font);
	if (!markup)
		pango_layout_set_text(layout, text, len);
	else
		pango_layout_set_markup(layout, text, len);

	pango_layout_get_pixel_extents(layout, &rect_ink, &rect);
	*height_ink = rect_ink.height;
	*height = rect.height;
	*width = rect.width;
	// printf("dimension : %d - %d\n", rect_ink.height, rect.height);

	g_object_unref(layout);
	cairo_destroy(c);
	cairo_surface_destroy(cs);
	XFreePixmap(server.dsp, pmap);
}

#if !GLIB_CHECK_VERSION (2, 33, 4)
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
