/**************************************************************************
* Common declarations
*
**************************************************************************/

#ifndef COMMON_H
#define COMMON_H

#define WM_CLASS_TINT "panel"

#include <Imlib2.h>
#include <pango/pangocairo.h>
#include "area.h"

#define GREEN  "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RED    "\033[31m"
#define BLUE   "\033[1;34m"
#define RESET  "\033[0m"

// mouse actions
typedef enum MouseAction {
	NONE = 0,
	CLOSE,
	TOGGLE,
	ICONIFY,
	SHADE,
	TOGGLE_ICONIFY,
	MAXIMIZE_RESTORE,
	MAXIMIZE,
	RESTORE,
	DESKTOP_LEFT,
	DESKTOP_RIGHT,
	NEXT_TASK,
	PREV_TASK
} MouseAction;

#define ALL_DESKTOPS  0xFFFFFFFF

// Copies a file to another path
void copy_file(const char *path_src, const char *path_dest);

// Parses lines with the format 'key = value' into key and value.
// Strips key and value.
// Values may contain spaces and the equal sign.
// Returns 1 if both key and value could be read, zero otherwise.
int parse_line(const char *line, char **key, char **value);

void extract_values(const char *value, char **value1, char **value2, char **value3);

// Executes a command in a shell.
void tint_exec(const char *command);

// Returns a copy of s in which "~" is expanded to the path to the user's home directory.
// The caller takes ownership of the string.
char *expand_tilde(char *s);

// The opposite of expand_tilde: replaces the path to the user's home directory with "~".
// The caller takes ownership of the string.
char *contract_tilde(char *s);

// Color
int hex_char_to_int(char c);
int hex_to_rgb(char *hex, int *r, int *g, int *b);
void get_color(char *hex, double *rgb);

Imlib_Image load_image(const char *path, int cached);

// Adjusts the alpha/saturation/brightness on an ARGB image.
// Parameters: alpha from 0 to 100, satur from 0 to 1, bright from 0 to 1.
void adjust_asb(DATA32 *data, int w, int h, int alpha, float satur, float bright);
Imlib_Image adjust_icon(Imlib_Image original, int alpha, int saturation, int brightness);

void create_heuristic_mask(DATA32 *data, int w, int h);

int image_empty(DATA32 *data, int w, int h);

// Renders the current Imlib image to a drawable. Wrapper around imlib_render_image_on_drawable.
void render_image(Drawable d, int x, int y);

void get_text_size2(PangoFontDescription *font,
					int *height_ink,
					int *height,
					int *width,
					int panel_height,
					int panel_with,
					char *text,
					int len,
					PangoWrapMode wrap,
					PangoEllipsizeMode ellipsis,
					gboolean markup);

void draw_text(PangoLayout *layout, cairo_t *c, int posx, int posy, Color *color, int font_shadow);

// Draws a rounded rectangle
void draw_rect(cairo_t *c, double x, double y, double w, double h, double r);

// Clears the pixmap (with transparent color)
void clear_pixmap(Pixmap p, int x, int y, int w, int h);

#define free_and_null(p) { free(p); p = NULL; }

#endif
