/**************************************************************************
* Common declarations
*
**************************************************************************/

#ifndef COMMON_H
#define COMMON_H

#define WM_CLASS_TINT "panel"

#include <glib.h>
#include <Imlib2.h>
#include <pango/pangocairo.h>
#include "area.h"

#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RED "\033[1;31m"
#define BLUE "\033[1;34m"
#define RESET "\033[0m"

#define MAX3(a, b, c) MAX(MAX(a, b), c)
#define MIN3(a, b, c) MIN(MIN(a, b), c)

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

#define ALL_DESKTOPS 0xFFFFFFFF

// add c to s
char *append(char *s, char c);

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
char *expand_tilde(const char *s);

// The opposite of expand_tilde: replaces the path to the user's home directory with "~".
// The caller takes ownership of the string.
char *contract_tilde(const char *s);

// Color
int hex_char_to_int(char c);
int hex_to_rgb(char *hex, int *r, int *g, int *b);
void get_color(char *hex, double *rgb);

Imlib_Image load_image(const char *path, int cached);

// Adjusts the alpha/saturation/brightness on an ARGB image.
// Parameters:
// * alpha_adjust: multiplicative:
//    * 0 = full transparency
//    * 1 = no adjustment
//    * 2 = twice the current opacity
// * satur_adjust: additive:
//   * -1 = full grayscale
//   *  0 = no adjustment
//   *  1 = full color
// * bright_adjust: additive:
//   * -1 = black
//   *  0 = no adjustment
//   *  1 = white
void adjust_asb(DATA32 *data, int w, int h, float alpha_adjust, float satur_adjust, float bright_adjust);
Imlib_Image adjust_icon(Imlib_Image original, int alpha, int saturation, int brightness);

void create_heuristic_mask(DATA32 *data, int w, int h);

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
void draw_rect_on_sides(cairo_t *c, double x, double y, double w, double h, double r, int border_mask);

// Clears the pixmap (with transparent color)
void clear_pixmap(Pixmap p, int x, int y, int w, int h);

// Appends to the list locations all the directories contained in the environment variable var (split by ":").
// Optional suffixes are added to each directory. The suffix arguments MUST end with NULL.
// Returns the new value of the list.
GSList *load_locations_from_env(GSList *locations, const char *var, ...);

GSList *slist_remove_duplicates(GSList *list, GCompareFunc eq, GDestroyNotify fr);

#define free_and_null(p) \
	{                    \
		free(p);         \
		p = NULL;        \
	}

#if !GLIB_CHECK_VERSION(2, 33, 4)
GList *g_list_copy_deep(GList *list, GCopyFunc func, gpointer user_data);
#endif

#if !GLIB_CHECK_VERSION(2, 38, 0)
#define g_assert_null(expr) g_assert((expr) == NULL)
#endif

#endif
