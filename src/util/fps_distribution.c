/**************************************************************************
*
* Copyright (C) 2017 tint2 authors
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

#include <stdlib.h>

#include "fps_distribution.h"

static float *fps_distribution = NULL;

void init_fps_distribution()
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
    // fprintf(stderr, "tint2: fps = %.0f => i = %d\n", fps, i);
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
        // fprintf(stderr, "tint2: %6.0f (i = %3d) : %.0f | ", value, i, (double)fps_distribution[i]);
        cum += fps_distribution[i];
        if (*low < 0 && cum >= cum_low)
            *low = value;
        if (*median < 0 && cum >= cum_median)
            *median = value;
        if (*high < 0 && cum >= cum_high)
            *high = value;
    }
}
