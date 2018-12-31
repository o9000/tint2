#include "button.h"

#include <string.h>
#include <stdio.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <math.h>
#include <pango/pangocairo.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

#include "window.h"
#include "server.h"
#include "panel.h"
#include "timer.h"
#include "common.h"

char *button_get_tooltip(void *obj);
void button_init_fonts();
int button_compute_desired_size(void *obj);
void button_dump_geometry(void *obj, int indent);

void default_button()
{
}

Button *create_button()
{
    Button *button = calloc(1, sizeof(Button));
    button->backend = calloc(1, sizeof(ButtonBackend));
    button->backend->centered = TRUE;
    button->backend->font_color.alpha = 0.5;
    return button;
}

gpointer create_button_frontend(gconstpointer arg, gpointer data)
{
    Button *button_backend = (Button *)arg;

    Button *button_frontend = calloc(1, sizeof(Button));
    button_frontend->backend = button_backend->backend;
    button_backend->backend->instances = g_list_append(button_backend->backend->instances, button_frontend);
    button_frontend->frontend = calloc(1, sizeof(ButtonFrontend));
    return button_frontend;
}

void destroy_button(void *obj)
{
    Button *button = (Button *)obj;
    if (button->frontend) {
        // This is a frontend element
        free_icon(button->frontend->icon);
        free_icon(button->frontend->icon_hover);
        free_icon(button->frontend->icon_pressed);
        button->backend->instances = g_list_remove_all(button->backend->instances, button);
        free_and_null(button->frontend);
        remove_area(&button->area);
        free_area(&button->area);
        free_and_null(button);
    } else {
        // This is a backend element
        free_and_null(button->backend->text);
        free_and_null(button->backend->icon_name);
        free_and_null(button->backend->tooltip);

        button->backend->bg = NULL;
        pango_font_description_free(button->backend->font_desc);
        button->backend->font_desc = NULL;
        free_and_null(button->backend->lclick_command);
        free_and_null(button->backend->mclick_command);
        free_and_null(button->backend->rclick_command);
        free_and_null(button->backend->dwheel_command);
        free_and_null(button->backend->uwheel_command);

        if (button->backend->instances) {
            fprintf(stderr, "tint2: Error: Attempt to destroy backend while there are still frontend instances!\n");
            exit(EXIT_FAILURE);
        }
        free(button->backend);
        free(button);
    }
}

void init_button()
{
    GList *to_remove = panel_config.button_list;
    for (int k = 0; k < strlen(panel_items_order) && to_remove; k++) {
        if (panel_items_order[k] == 'P') {
            to_remove = to_remove->next;
        }
    }

    if (to_remove) {
        if (to_remove == panel_config.button_list) {
            g_list_free_full(to_remove, destroy_button);
            panel_config.button_list = NULL;
        } else {
            // Cut panel_config.button_list
            if (to_remove->prev)
                to_remove->prev->next = NULL;
            to_remove->prev = NULL;
            // Remove all elements of to_remove and to_remove itself
            g_list_free_full(to_remove, destroy_button);
        }
    }

    button_init_fonts();
    for (GList *l = panel_config.button_list; l; l = l->next) {
        Button *button = l->data;

        // Set missing config options
        if (!button->backend->bg)
            button->backend->bg = &g_array_index(backgrounds, Background, 0);
    }
}

void init_button_panel(void *p)
{
    Panel *panel = (Panel *)p;

    // Make sure this is only done once if there are multiple items
    if (panel->button_list && ((Button *)panel->button_list->data)->frontend)
        return;

    // panel->button_list is now a copy of the pointer panel_config.button_list
    // We make it a deep copy
    panel->button_list = g_list_copy_deep(panel_config.button_list, create_button_frontend, NULL);

    load_icon_themes();

    for (GList *l = panel->button_list; l; l = l->next) {
        Button *button = l->data;
        button->area.bg = button->backend->bg;
        button->area.paddingx = button->backend->paddingx;
        button->area.paddingy = button->backend->paddingy;
        button->area.paddingxlr = button->backend->paddingxlr;
        button->area.parent = panel;
        button->area.panel = panel;
        button->area._dump_geometry = button_dump_geometry;
        button->area._compute_desired_size = button_compute_desired_size;
        snprintf(button->area.name, sizeof(button->area.name), "Button");
        button->area._draw_foreground = draw_button;
        button->area.size_mode = LAYOUT_FIXED;
        button->area._resize = resize_button;
        button->area._get_tooltip_text = button_get_tooltip;
        button->area._is_under_mouse = full_width_area_is_under_mouse;
        button->area.has_mouse_press_effect =
            panel_config.mouse_effects &&
            (button->area.has_mouse_over_effect = button->backend->lclick_command || button->backend->mclick_command ||
                                                  button->backend->rclick_command || button->backend->uwheel_command ||
                                                  button->backend->dwheel_command);

        button->area.resize_needed = TRUE;
        button->area.on_screen = TRUE;
        instantiate_area_gradients(&button->area);

        button_reload_icon(button);
    }
}

void button_init_fonts()
{
    for (GList *l = panel_config.button_list; l; l = l->next) {
        Button *button = l->data;
        if (!button->backend->font_desc)
            button->backend->font_desc = pango_font_description_from_string(get_default_font());
    }
}

void button_default_font_changed()
{
    gboolean needs_update = FALSE;
    for (GList *l = panel_config.button_list; l; l = l->next) {
        Button *button = l->data;

        if (!button->backend->has_font) {
            pango_font_description_free(button->backend->font_desc);
            button->backend->font_desc = NULL;
            needs_update = TRUE;
        }
    }
    if (!needs_update)
        return;

    button_init_fonts();
    for (int i = 0; i < num_panels; i++) {
        for (GList *l = panels[i].button_list; l; l = l->next) {
            Button *button = l->data;

            if (!button->backend->has_font) {
                button->area.resize_needed = TRUE;
                schedule_redraw(&button->area);
            }
        }
    }
    schedule_panel_redraw();
}

void button_reload_icon(Button *button)
{
    free_icon(button->frontend->icon);
    free_icon(button->frontend->icon_hover);
    free_icon(button->frontend->icon_pressed);
    button->frontend->icon = NULL;
    button->frontend->icon_hover = NULL;
    button->frontend->icon_pressed = NULL;

    button->frontend->icon_load_size = button->frontend->iconw;

    if (!button->backend->icon_name)
        return;

    char *new_icon_path = get_icon_path(icon_theme_wrapper, button->backend->icon_name, button->frontend->iconw, TRUE);
    if (new_icon_path)
        button->frontend->icon = load_image(new_icon_path, TRUE);
    free(new_icon_path);
    // On loading error, fallback to default
    if (!button->frontend->icon) {
        new_icon_path = get_icon_path(icon_theme_wrapper, DEFAULT_ICON, button->frontend->iconw, TRUE);
        if (new_icon_path)
            button->frontend->icon = load_image(new_icon_path, TRUE);
        free(new_icon_path);
    }
    Imlib_Image original = button->frontend->icon;
    button->frontend->icon = scale_icon(button->frontend->icon, button->frontend->iconw);
    free_icon(original);

    if (panel_config.mouse_effects) {
        button->frontend->icon_hover = adjust_icon(button->frontend->icon,
                                                   panel_config.mouse_over_alpha,
                                                   panel_config.mouse_over_saturation,
                                                   panel_config.mouse_over_brightness);
        button->frontend->icon_pressed = adjust_icon(button->frontend->icon,
                                                     panel_config.mouse_pressed_alpha,
                                                     panel_config.mouse_pressed_saturation,
                                                     panel_config.mouse_pressed_brightness);
    }
    schedule_redraw(&button->area);
}

void button_default_icon_theme_changed()
{
    for (int i = 0; i < num_panels; i++) {
        for (GList *l = panels[i].button_list; l; l = l->next) {
            Button *button = l->data;
            button_reload_icon(button);
        }
    }
    schedule_panel_redraw();
}

void cleanup_button()
{
    // Cleanup frontends
    for (int i = 0; i < num_panels; i++) {
        g_list_free_full(panels[i].button_list, destroy_button);
        panels[i].button_list = NULL;
    }

    // Cleanup backends
    g_list_free_full(panel_config.button_list, destroy_button);
    panel_config.button_list = NULL;
}

int button_compute_desired_size(void *obj)
{
    Button *button = (Button *)obj;
    Panel *panel = (Panel *)button->area.panel;
    int horiz_padding = (panel_horizontal ? button->area.paddingxlr : button->area.paddingy) * panel->scale;
    int vert_padding = (panel_horizontal ? button->area.paddingy : button->area.paddingxlr) * panel->scale;
    int interior_padding = button->area.paddingx * panel->scale;

    int icon_w, icon_h;
    if (button->backend->icon_name) {
        if (panel_horizontal)
            icon_h = icon_w = button->area.height - top_bottom_border_width(&button->area) - 2 * vert_padding;
        else
            icon_h = icon_w = button->area.width - left_right_border_width(&button->area) - 2 * horiz_padding;
        if (button->backend->max_icon_size) {
            icon_w = MIN(icon_w, button->backend->max_icon_size * panel->scale);
            icon_h = MIN(icon_h, button->backend->max_icon_size * panel->scale);
        }
    } else {
        icon_h = icon_w = 0;
    }

    int txt_height, txt_width;
    if (button->backend->text) {
        if (panel_horizontal) {
            get_text_size2(button->backend->font_desc,
                           &txt_height,
                           &txt_width,
                           panel->area.height,
                           panel->area.width,
                           button->backend->text,
                           strlen(button->backend->text),
                           PANGO_WRAP_WORD_CHAR,
                           PANGO_ELLIPSIZE_NONE,
                           button->backend->centered ? PANGO_ALIGN_CENTER : PANGO_ALIGN_LEFT,
                           FALSE,
                           panel->scale);
        } else {
            get_text_size2(button->backend->font_desc,
                           &txt_height,
                           &txt_width,
                           panel->area.height,
                           button->area.width - icon_w - (icon_w ? interior_padding : 0) - 2 * horiz_padding -
                               left_right_border_width(&button->area),
                           button->backend->text,
                           strlen(button->backend->text),
                           PANGO_WRAP_WORD_CHAR,
                           PANGO_ELLIPSIZE_NONE,
                           button->backend->centered ? PANGO_ALIGN_CENTER : PANGO_ALIGN_LEFT,
                           FALSE,
                           panel->scale);
        }
    } else {
        txt_height = txt_width = 0;
    }

    if (panel_horizontal) {
        int new_size = txt_width + icon_w + (txt_width && icon_w ? interior_padding : 0);
        new_size += 2 * horiz_padding + left_right_border_width(&button->area);
        return new_size;
    } else {
        int new_size;
        new_size = txt_height + 2 * vert_padding + top_bottom_border_width(&button->area);
        new_size = MAX(new_size, icon_h + 2 * vert_padding + top_bottom_border_width(&button->area));
        return new_size;
    }
}

gboolean resize_button(void *obj)
{
    Button *button = (Button *)obj;
    Panel *panel = (Panel *)button->area.panel;
    Area *area = &button->area;
    int horiz_padding = (panel_horizontal ? button->area.paddingxlr : button->area.paddingy) * panel->scale;
    int vert_padding = (panel_horizontal ? button->area.paddingy : button->area.paddingxlr) * panel->scale;
    int interior_padding = button->area.paddingx * panel->scale;

    int icon_w, icon_h;
    if (button->backend->icon_name) {
        if (panel_horizontal)
            icon_h = icon_w = button->area.height - top_bottom_border_width(&button->area) - 2 * vert_padding;
        else
            icon_h = icon_w = button->area.width - left_right_border_width(&button->area) - 2 * horiz_padding;
        if (button->backend->max_icon_size) {
            icon_w = MIN(icon_w, button->backend->max_icon_size * panel->scale);
            icon_h = MIN(icon_h, button->backend->max_icon_size * panel->scale);
        }
    } else {
        icon_h = icon_w = 0;
    }

    button->frontend->iconw = icon_w;
    button->frontend->iconh = icon_h;
    if (button->frontend->icon_load_size != button->frontend->iconw)
        button_reload_icon(button);

    int available_w, available_h;
    if (panel_horizontal) {
        available_w = panel->area.width;
        available_h = area->height - 2 * area->paddingy - left_right_border_width(area);
    } else {
        available_w =
            area->width - icon_w - (icon_w ? interior_padding : 0) - 2 * horiz_padding - left_right_border_width(area);
        available_h = panel->area.height;
    }

    int txt_height, txt_width;
    if (button->backend->text) {
        get_text_size2(button->backend->font_desc,
                       &txt_height,
                       &txt_width,
                       available_h,
                       available_w,
                       button->backend->text,
                       strlen(button->backend->text),
                       PANGO_WRAP_WORD_CHAR,
                       PANGO_ELLIPSIZE_NONE,
                       button->backend->centered ? PANGO_ALIGN_CENTER : PANGO_ALIGN_LEFT,
                       FALSE,
                       panel->scale);
    } else {
        txt_height = txt_width = 0;
    }

    gboolean result = FALSE;
    if (panel_horizontal) {
        int new_size = txt_width + icon_w + (txt_width && icon_w ? interior_padding : 0);
        new_size += 2 * horiz_padding + left_right_border_width(&button->area);
        if (new_size != button->area.width) {
            button->area.width = new_size;
            result = TRUE;
        }
    } else {
        int new_size;
        new_size = txt_height + 2 * vert_padding + top_bottom_border_width(&button->area);
        new_size = MAX(new_size, icon_h + 2 * vert_padding + top_bottom_border_width(&button->area));
        if (new_size != button->area.height) {
            button->area.height = new_size;
            result = TRUE;
        }
    }
    button->frontend->textw = txt_width;
    button->frontend->texth = txt_height;
    if (button->backend->centered) {
        if (icon_w) {
            button->frontend->icony = (button->area.height - icon_h) / 2;
            button->frontend->iconx =
                (button->area.width - txt_width - (txt_width ? interior_padding : 0) - icon_w) / 2;
            button->frontend->texty = (button->area.height - txt_height) / 2;
            button->frontend->textx = button->frontend->iconx + icon_w + interior_padding;
        } else {
            button->frontend->texty = (button->area.height - txt_height) / 2;
            button->frontend->textx = (button->area.width - txt_width) / 2;
        }
    } else {
        if (icon_w) {
            button->frontend->icony = (button->area.height - icon_h) / 2;
            button->frontend->iconx = left_border_width(&button->area) + horiz_padding;
            button->frontend->texty = (button->area.height - txt_height) / 2;
            button->frontend->textx = button->frontend->iconx + icon_w + interior_padding;
        } else {
            button->frontend->texty = (button->area.height - txt_height) / 2;
            button->frontend->textx = left_border_width(&button->area) + horiz_padding;
        }
    }

    schedule_redraw(&button->area);

    return result;
}

void draw_button(void *obj, cairo_t *c)
{
    Button *button = obj;
    Panel *panel = (Panel *)button->area.panel;

    if (button->frontend->icon) {
        // Render icon
        Imlib_Image image;
        if (panel_config.mouse_effects) {
            if (button->area.mouse_state == MOUSE_OVER)
                image = button->frontend->icon_hover ? button->frontend->icon_hover : button->frontend->icon;
            else if (button->area.mouse_state == MOUSE_DOWN)
                image = button->frontend->icon_pressed ? button->frontend->icon_pressed : button->frontend->icon;
            else
                image = button->frontend->icon;
        } else {
            image = button->frontend->icon;
        }

        imlib_context_set_image(image);
        render_image(button->area.pix, button->frontend->iconx, button->frontend->icony);
    }

    // Render text
    if (button->backend->text) {
        PangoContext *context = pango_cairo_create_context(c);
        pango_cairo_context_set_resolution(context, 96 * panel->scale);
        PangoLayout *layout = pango_layout_new(context);

        pango_layout_set_font_description(layout, button->backend->font_desc);
        pango_layout_set_width(layout, (button->frontend->textw + TINT2_PANGO_SLACK) * PANGO_SCALE);
        pango_layout_set_alignment(layout, button->backend->centered ? PANGO_ALIGN_CENTER : PANGO_ALIGN_LEFT);
        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
        pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
        pango_layout_set_text(layout, button->backend->text, strlen(button->backend->text));

        pango_cairo_update_layout(c, layout);
        draw_text(layout,
                  c,
                  button->frontend->textx,
                  button->frontend->texty,
                  &button->backend->font_color,
                  panel_config.font_shadow ? layout : NULL);

        g_object_unref(layout);
        g_object_unref(context);
    }
}

void button_dump_geometry(void *obj, int indent)
{
    Button *button = obj;

    if (button->frontend->icon) {
        Imlib_Image tmp = imlib_context_get_image();
        imlib_context_set_image(button->frontend->icon);
        fprintf(stderr,
                "tint2: %*sIcon: x = %d, y = %d, w = %d, h = %d\n",
                indent,
                "",
                button->frontend->iconx,
                button->frontend->icony,
                imlib_image_get_width(),
                imlib_image_get_height());
        if (tmp)
            imlib_context_set_image(tmp);
    }
    fprintf(stderr,
            "tint2: %*sText: x = %d, y = %d, w = %d, align = %s, text = %s\n",
            indent,
            "",
            button->frontend->textx,
            button->frontend->texty,
            button->frontend->textw,
            button->backend->centered ? "center" : "left",
            button->backend->text);
}

void button_action(void *obj, int mouse_button, int x, int y, Time time)
{
    Button *button = (Button *)obj;
    char *command = NULL;
    switch (mouse_button) {
    case 1:
        command = button->backend->lclick_command;
        break;
    case 2:
        command = button->backend->mclick_command;
        break;
    case 3:
        command = button->backend->rclick_command;
        break;
    case 4:
        command = button->backend->uwheel_command;
        break;
    case 5:
        command = button->backend->dwheel_command;
        break;
    }
    tint_exec(command, NULL, NULL, time, obj, x, y, FALSE, TRUE);
}

char *button_get_tooltip(void *obj)
{
    Button *button = obj;

    if (button->backend->tooltip && strlen(button->backend->tooltip) > 0)
        return strdup(button->backend->tooltip);
    return NULL;
}
