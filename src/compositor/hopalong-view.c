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

	if (view->xdg_surface->toplevel->title)
		strlcpy(title, view->xdg_surface->toplevel->title, sizeof title);
	else if (view->xdg_surface->toplevel->app_id)
		strlcpy(title, view->xdg_surface->toplevel->app_id, sizeof title);

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
