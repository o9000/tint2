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
#include "colors.h"

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

void write_string(int fd, const char *s);
void log_string(int fd, const char *s);

void dump_backtrace(int log_fd);

// sleep() returns early when signals arrive. This function does not.
void safe_sleep(int seconds);

const char *signal_name(int sig);

const char *get_home_dir();

// Copies a file to another path
void copy_file(const char *path_src, const char *path_dest);

// Parses lines with the format 'key = value' into key and value.
// Strips key and value.
// Values may contain spaces and the equal sign.
// Returns 1 if both key and value could be read, zero otherwise.
gboolean parse_line(const char *line, char **key, char **value);

void extract_values(const char *value, char **value1, char **value2, char **value3);
void extract_values_4(const char *value, char **value1, char **value2, char **value3, char **value4);

// Executes a command in a shell.
pid_t tint_exec(const char *command,
                const char *dir,
                const char *tooltip,
                Time time,
                Area *area,
                int x,
                int y,
                gboolean terminal,
                gboolean startup_notification);
void tint_exec_no_sn(const char *command);
int setenvd(const char *name, const int value);

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
void adjust_color(Color *color, int alpha, int saturation, int brightness);

void create_heuristic_mask(DATA32 *data, int w, int h);

// Renders the current Imlib image to a drawable. Wrapper around imlib_render_image_on_drawable.
void render_image(Drawable d, int x, int y);

void get_text_size2(const PangoFontDescription *font,
                    int *height_ink,
                    int *height,
                    int *width,
                    int available_height,
                    int available_with,
                    const char *text,
                    int text_len,
                    PangoWrapMode wrap,
                    PangoEllipsizeMode ellipsis,
                    gboolean markup);

void draw_text(PangoLayout *layout, cairo_t *c, int posx, int posy, Color *color, int font_shadow);

// Draws a rounded rectangle
void draw_rect(cairo_t *c, double x, double y, double w, double h, double r);
void draw_rect_on_sides(cairo_t *c, double x, double y, double w, double h, double r, int border_mask);

// Clears the pixmap (with transparent color)
void clear_pixmap(Pixmap p, int x, int y, int w, int h);

void close_all_fds();

// Appends to the list locations all the directories contained in the environment variable var (split by ":").
// Optional suffixes are added to each directory. The suffix arguments MUST end with NULL.
// Returns the new value of the list.
GSList *load_locations_from_env(GSList *locations, const char *var, ...);

GSList *slist_remove_duplicates(GSList *list, GCompareFunc eq, GDestroyNotify fr);

// A trivial pointer comparator.
gint cmp_ptr(gconstpointer a, gconstpointer b);

GString *tint2_g_string_replace(GString *s, const char *from, const char *to);

void get_image_mean_color(const Imlib_Image image, Color *mean_color);

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

#if !GLIB_CHECK_VERSION(2, 40, 0)
#define g_assert_nonnull(expr) g_assert((expr) != NULL)
#endif

#endif
