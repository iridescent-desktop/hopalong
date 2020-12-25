/*
 * Hopalong - a friendly Wayland compositor
 * Copyright (c) 2020 Ariadne Conill <ariadne@dereferenced.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include "hopalong-server.h"
#include "hopalong-view.h"
#include "hopalong-output.h"
#include "hopalong-pango-util.h"

static struct wlr_texture *
generate_minimize_texture(struct hopalong_output *output, float color[4])
{
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 32, 32);
	return_val_if_fail(surface != NULL, false);

	cairo_t *cr = cairo_create(surface);
	return_val_if_fail(cr != NULL, false);

	cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

	cairo_set_source_rgba(cr, color[0], color[1], color[2], color[3]);
	cairo_set_line_width(cr, 3.0);

	cairo_move_to(cr, 8, 16);
	cairo_line_to(cr, 16, 24);
	cairo_line_to(cr, 24, 16);

	cairo_stroke(cr);

	cairo_surface_flush(surface);

	unsigned char *data = cairo_image_surface_get_data(surface);
	struct wlr_renderer *renderer = wlr_backend_get_renderer(output->wlr_output->backend);
	struct wlr_texture *texture = wlr_texture_from_pixels(renderer,
		WL_SHM_FORMAT_ARGB8888,
		cairo_image_surface_get_stride(surface),
		32, 32, data);

	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	return texture;
}

static struct wlr_texture *
generate_maximize_texture(struct hopalong_output *output, float color[4])
{
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 32, 32);
	return_val_if_fail(surface != NULL, false);

	cairo_t *cr = cairo_create(surface);
	return_val_if_fail(cr != NULL, false);

	cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

	cairo_set_source_rgba(cr, color[0], color[1], color[2], color[3]);
	cairo_set_line_width(cr, 3.0);

	cairo_move_to(cr, 8, 16);
	cairo_line_to(cr, 16, 8);
	cairo_line_to(cr, 24, 16);
	cairo_line_to(cr, 16, 24);
	cairo_line_to(cr, 8, 16);

	cairo_stroke(cr);

	cairo_surface_flush(surface);

	unsigned char *data = cairo_image_surface_get_data(surface);
	struct wlr_renderer *renderer = wlr_backend_get_renderer(output->wlr_output->backend);
	struct wlr_texture *texture = wlr_texture_from_pixels(renderer,
		WL_SHM_FORMAT_ARGB8888,
		cairo_image_surface_get_stride(surface),
		32, 32, data);

	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	return texture;
}

static struct wlr_texture *
generate_close_texture(struct hopalong_output *output, float color[4])
{
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 32, 32);
	return_val_if_fail(surface != NULL, false);

	cairo_t *cr = cairo_create(surface);
	return_val_if_fail(cr != NULL, false);

	cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

	cairo_set_source_rgba(cr, color[0], color[1], color[2], color[3]);
	cairo_set_line_width(cr, 3.0);

	cairo_move_to(cr, 8, 24);
	cairo_line_to(cr, 24, 8);

	cairo_move_to(cr, 24, 24);
	cairo_line_to(cr, 8, 8);

	cairo_stroke(cr);

	cairo_surface_flush(surface);

	unsigned char *data = cairo_image_surface_get_data(surface);
	struct wlr_renderer *renderer = wlr_backend_get_renderer(output->wlr_output->backend);
	struct wlr_texture *texture = wlr_texture_from_pixels(renderer,
		WL_SHM_FORMAT_ARGB8888,
		cairo_image_surface_get_stride(surface),
		32, 32, data);

	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	return texture;
}

struct hopalong_generated_textures *
hopalong_generate_builtin_textures_for_output(struct hopalong_output *output)
{
	float active[4] = {1.0, 1.0, 1.0, 1.0};
	float inactive[4] = {0.2, 0.2, 0.2, 1.0};

	struct hopalong_generated_textures *gentex = calloc(1, sizeof(*gentex));

	gentex->minimize = generate_minimize_texture(output, active);
	gentex->minimize_inactive = generate_minimize_texture(output, inactive);

	gentex->maximize = generate_maximize_texture(output, active);
	gentex->maximize_inactive = generate_maximize_texture(output, inactive);

	gentex->close = generate_close_texture(output, active);
	gentex->close_inactive = generate_close_texture(output, inactive);

	return gentex;
}

static bool
hopalong_view_generate_title_texture(struct hopalong_output *output, struct hopalong_view *view)
{
	struct wlr_renderer *renderer = wlr_backend_get_renderer(output->wlr_output->backend);

	if (view->title)
	{
		wlr_texture_destroy(view->title);
		wlr_texture_destroy(view->title_inactive);
	}

	const char *font = "Sans 10";

	char title[4096] = {};

	const char *title_data = hopalong_view_getprop(view, HOPALONG_VIEW_TITLE);
	if (title_data == NULL)
		title_data = hopalong_view_getprop(view, HOPALONG_VIEW_APP_ID);
	if (title_data != NULL)
		strlcpy(title, title_data, sizeof title);

	float scale = output->wlr_output->scale;
	int w = 400;
	int h = 32;

	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
	return_val_if_fail(surface != NULL, false);

	cairo_t *cr = cairo_create(surface);
	return_val_if_fail(cr != NULL, false);

	cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
	cairo_font_options_t *fo = cairo_font_options_create();
	return_val_if_fail(fo != NULL, false);

	cairo_font_options_set_hint_style(fo, CAIRO_HINT_STYLE_FULL);
	cairo_font_options_set_antialias(fo, CAIRO_ANTIALIAS_GRAY);

	cairo_set_font_options(cr, fo);
	hopalong_pango_util_get_text_size(cr, font, &w, NULL, NULL, scale, true, "%s", title);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
	return_val_if_fail(surface != NULL, false);

	cr = cairo_create(surface);
	return_val_if_fail(cr != NULL, false);

	PangoContext *pango = pango_cairo_create_context(cr);
	cairo_move_to(cr, 0, 0);

	/* active color first */
	float color[4] = {1.0, 1.0, 1.0, 1.0};
	cairo_set_source_rgba(cr, color[0], color[1], color[2], color[3]);
	hopalong_pango_util_printf(cr, font, scale, true, "%s", title);
	cairo_surface_flush(surface);

	unsigned char *data = cairo_image_surface_get_data(surface);

	view->title = wlr_texture_from_pixels(renderer,
		WL_SHM_FORMAT_ARGB8888,
		cairo_image_surface_get_stride(surface),
		w, h, data);

	/* inactive color */
	float inactive_color[4] = {0.2, 0.2, 0.2, 1.0};
	cairo_set_source_rgba(cr, inactive_color[0], inactive_color[1], inactive_color[2], inactive_color[3]);
	hopalong_pango_util_printf(cr, font, scale, true, "%s", title);
	cairo_surface_flush(surface);

	data = cairo_image_surface_get_data(surface);

	view->title_inactive = wlr_texture_from_pixels(renderer,
		WL_SHM_FORMAT_ARGB8888,
		cairo_image_surface_get_stride(surface),
		w, h, data);

	view->title_box.width = w;
	view->title_box.height = h;
	view->title_dirty = false;

	g_object_unref(pango);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	return true;
}

bool
hopalong_view_generate_textures(struct hopalong_output *output, struct hopalong_view *view)
{
	return_val_if_fail(output != NULL, false);
	return_val_if_fail(view != NULL, false);

	if (view->title_dirty && !hopalong_view_generate_title_texture(output, view))
		return false;

	return true;
}

void
hopalong_view_minimize(struct hopalong_view *view)
{
	return_if_fail(view != NULL);
	return_if_fail(view->ops != NULL);

	view->ops->minimize(view);
}

void
hopalong_view_maximize(struct hopalong_view *view)
{
	return_if_fail(view != NULL);
	return_if_fail(view->ops != NULL);

	view->ops->maximize(view);
}

void
hopalong_view_close(struct hopalong_view *view)
{
	return_if_fail(view != NULL);
	return_if_fail(view->ops != NULL);

	view->ops->close(view);
}

const char *
hopalong_view_getprop(struct hopalong_view *view, enum hopalong_view_prop prop)
{
	return_val_if_fail(view != NULL, NULL);
	return_val_if_fail(view->ops != NULL, NULL);

	return view->ops->getprop(view, prop);
}