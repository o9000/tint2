/**************************************************************************
*
* Tint2 : common windows function
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


#include "common.h"



void adjust_hsb(DATA32 *data, int w, int h, float hu, float satur, float bright)
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
			hue += hu;
			if (hue < 0.0) hue = 0.0;
			if (hue > 1.0) hue = 1.0;

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


