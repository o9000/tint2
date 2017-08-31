#ifndef FPS_DISTRIBUTION_H
#define FPS_DISTRIBUTION_H

void init_fps_distribution();
void cleanup_fps_distribution();
void sample_fps(double fps);
void fps_compute_stats(double *low, double *median, double *high, double *samples);

#endif
