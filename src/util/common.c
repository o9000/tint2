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
#include <unistd.h>

#include "common.h"
#include "../server.h"



void copy_file(const char *pathSrc, const char *pathDest)
{
	FILE *fileSrc, *fileDest;
	char buffer[100];
	int  nb;

	fileSrc = fopen(pathSrc, "rb");
	if (fileSrc == NULL) return;

	fileDest = fopen(pathDest, "wb");
	if (fileDest == NULL) return;

	while ((nb = fread(buffer, 1, sizeof(buffer), fileSrc)) > 0) {
		if ( nb != fwrite(buffer, 1, nb, fileDest)) {
			printf("Error while copying file %s to %s\n", pathSrc, pathDest);
		}
	}

	fclose (fileDest);
	fclose (fileSrc);
}


int parse_line (const char *line, char **key, char **value)
{
	char *a, *b;

	/* Skip useless lines */
	if ((line[0] == '#') || (line[0] == '\n')) return 0;
	if (!(a = strchr (line, '='))) return 0;

	/* overwrite '=' with '\0' */
	a[0] = '\0';
	*key = strdup (line);
	a++;

	/* overwrite '\n' with '\0' if '\n' present */
	if ((b = strchr (a, '\n'))) b[0] = '\0';

	*value = strdup (a);

	g_strstrip(*key);
	g_strstrip(*value);
	return 1;
}


void tint_exec(const char *command)
{
	if (command) {
		pid_t pid;
		pid = fork();
		if (pid == 0) {
			// change for the fork the signal mask
//			sigset_t sigset;
//			sigprocmask(SIG_SETMASK, &sigset, 0);
//			sigprocmask(SIG_UNBLOCK, &sigset, 0);
			execl("/bin/sh", "/bin/sh", "-c", command, NULL);
			_exit(0);
		}
	}
}


int hex_char_to_int (char c)
{
	int r;

	if (c >= '0' && c <= '9')  r = c - '0';
	else if (c >= 'a' && c <= 'f')  r = c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')  r = c - 'A' + 10;
	else  r = 0;

	return r;
}


int hex_to_rgb (char *hex, int *r, int *g, int *b)
{
	int len;

	if (hex == NULL || hex[0] != '#') return (0);

	len = strlen (hex);
	if (len == 3 + 1) {
		*r = hex_char_to_int (hex[1]);
		*g = hex_char_to_int (hex[2]);
		*b = hex_char_to_int (hex[3]);
	}
	else if (len == 6 + 1) {
		*r = hex_char_to_int (hex[1]) * 16 + hex_char_to_int (hex[2]);
		*g = hex_char_to_int (hex[3]) * 16 + hex_char_to_int (hex[4]);
		*b = hex_char_to_int (hex[5]) * 16 + hex_char_to_int (hex[6]);
	}
	else if (len == 12 + 1) {
		*r = hex_char_to_int (hex[1]) * 16 + hex_char_to_int (hex[2]);
		*g = hex_char_to_int (hex[5]) * 16 + hex_char_to_int (hex[6]);
		*b = hex_char_to_int (hex[9]) * 16 + hex_char_to_int (hex[10]);
	}
	else return 0;

	return 1;
}


void get_color (char *hex, double *rgb)
{
	int r, g, b;
	hex_to_rgb (hex, &r, &g, &b);

	rgb[0] = (r / 255.0);
	rgb[1] = (g / 255.0);
	rgb[2] = (b / 255.0);
}


void extract_values (const char *value, char **value1, char **value2, char **value3)
{
	char *b=0, *c=0;

	if (*value1) free (*value1);
	if (*value2) free (*value2);
	if (*value3) free (*value3);

	if ((b = strchr (value, ' '))) {
		b[0] = '\0';
		b++;
	}
	else {
		*value2 = 0;
		*value3 = 0;
	}
	*value1 = strdup (value);
	g_strstrip(*value1);

	if (b) {
		if ((c = strchr (b, ' '))) {
			c[0] = '\0';
			c++;
		}
		else {
			c = 0;
			*value3 = 0;
		}
		*value2 = strdup (b);
		g_strstrip(*value2);
	}

	if (c) {
		*value3 = strdup (c);
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

	for(y = 0; y < h; y++) {
		for(id = y * w, x = 0; x < w; x++, id++) {
			argb = data[id];
			a = (argb >> 24) & 0xff;
			// transparent => nothing to do.
			if (a == 0) continue;
			r = (argb >> 16) & 0xff;
			g = (argb >> 8) & 0xff;
			b = (argb) & 0xff;

			// convert RGB to HSB
			cmax = (r > g) ? r : g;
			if (b > cmax) cmax = b;
			cmin = (r < g) ? r : g;
			if (b < cmin) cmin = b;
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
			if (saturation < 0.0) saturation = 0.0;
			if (saturation > 1.0) saturation = 1.0;
			brightness += bright;
			if (brightness < 0.0) brightness = 0.0;
			if (brightness > 1.0) brightness = 1.0;
			if (alpha != 100)
				a = (a * alpha)/100;

			// convert HSB to RGB
			if (saturation == 0) {
				r = g = b = (int)(brightness * 255.0f + 0.5f);
			} else {
				h2 = (hue - (int)hue) * 6.0f;
				f = h2 - (int)(h2);
				p = brightness * (1.0f - saturation);
				q = brightness * (1.0f - saturation * f);
				t = brightness * (1.0f - (saturation * (1.0f - f)));
				switch ((int) h2) {
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


void createHeuristicMask(DATA32* data, int w, int h)
{
	// first we need to find the mask color, therefore we check all 4 edge pixel and take the color which
	// appears most often (we only need to check three edges, the 4th is implicitly clear)
	unsigned int topLeft = data[0], topRight = data[w-1], bottomLeft = data[w*h-w], bottomRight = data[w*h-1];
	int max = (topLeft == topRight) + (topLeft == bottomLeft) + (topLeft == bottomRight);
	int maskPos = 0;
	if ( max < (topRight == topLeft) + (topRight == bottomLeft) + (topRight == bottomRight) ) {
		max = (topRight == topLeft) + (topRight == bottomLeft) + (topRight == bottomRight);
		maskPos = w-1;
	}
	if ( max < (bottomLeft == topRight) + (bottomLeft == topLeft) + (bottomLeft == bottomRight) )
		maskPos = w*h-w;

	// now mask out every pixel which has the same color as the edge pixels
	unsigned char* udata = (unsigned char*)data;
	unsigned char b = udata[4*maskPos];
	unsigned char g = udata[4*maskPos+1];
	unsigned char r = udata[4*maskPos+1];
	int i;
	for (i=0; i<h*w; ++i) {
		if ( b-udata[0] == 0 && g-udata[1] == 0 && r-udata[2] == 0 )
			udata[3] = 0;
		udata += 4;
	}
}


void render_image(Drawable d, int x, int y, int w, int h)
{
	// in real_transparency mode imlib_render_image_on_drawable does not the right thing, because
	// the operation is IMLIB_OP_COPY, but we would need IMLIB_OP_OVER (which does not exist)
	// Therefore we have to do it with the XRender extension (i.e. copy what imlib is doing internally)
	// But first we need to render the image onto itself with PictOpIn to adjust the colors to the alpha channel
	Pixmap pmap_tmp = XCreatePixmap(server.dsp, server.root_win, w, h, 32);
	imlib_context_set_drawable(pmap_tmp);
	imlib_context_set_blend(0);
	imlib_render_image_on_drawable(0, 0);
	Picture pict_image = XRenderCreatePicture(server.dsp, pmap_tmp, XRenderFindStandardFormat(server.dsp, PictStandardARGB32), 0, 0);
	Picture pict_drawable = XRenderCreatePicture(server.dsp, d, XRenderFindVisualFormat(server.dsp, server.visual), 0, 0);
	XRenderComposite(server.dsp, PictOpIn, pict_image, None, pict_image, 0, 0, 0, 0, 0, 0, w, h);
	XRenderComposite(server.dsp, PictOpOver, pict_image, None, pict_drawable, 0, 0, 0, 0, x, y, w, h);
	imlib_context_set_blend(1);
	XFreePixmap(server.dsp, pmap_tmp);
	XRenderFreePicture(server.dsp, pict_image);
	XRenderFreePicture(server.dsp, pict_drawable);
}
