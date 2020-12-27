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

#include <stdlib.h>
#include "hopalong-server.h"
#include "hopalong-layer-shell.h"

static void
hopalong_layer_shell_surface_map(struct wl_listener *listener, void *data)
{
	struct hopalong_view *view = wl_container_of(listener, view, map);
	wlr_surface_send_enter(view->layer_surface->surface, view->layer_surface->output);
}

static void
hopalong_layer_shell_surface_unmap(struct wl_listener *listener, void *data)
{
	struct hopalong_view *view = wl_container_of(listener, view, unmap);
	hopalong_view_unmap(view);
}

static void
hopalong_layer_shell_destroy(struct wl_listener *listener, void *data)
{
	struct hopalong_view *view = wl_container_of(listener, view, destroy);
	hopalong_view_destroy(view);
}

static enum hopalong_layer layer_mapping[HOPALONG_LAYER_COUNT] = {
	[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND]	= HOPALONG_LAYER_BACKGROUND,
	[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM]	= HOPALONG_LAYER_BOTTOM,
	[ZWLR_LAYER_SHELL_V1_LAYER_TOP]		= HOPALONG_LAYER_TOP,
	[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY]	= HOPALONG_LAYER_OVERLAY,
};

static void
arrange_layer(struct hopalong_view *view)
{
	struct wlr_layer_surface_v1 *layer = view->layer_surface;
	struct wlr_layer_surface_v1_state *state = &layer->current;

	struct wlr_output *output = layer->output;
	struct wlr_box usable_area = {};

	wlr_output_effective_resolution(output, &usable_area.width, &usable_area.height);

	struct wlr_box bounds = {
		.width = state->desired_width,
		.height = state->desired_height
	};

	switch (view->layer)
	{
	case HOPALONG_LAYER_BACKGROUND:
		bounds = usable_area;
		break;
	default:
		break;
	}

	wlr_layer_surface_v1_configure(layer, bounds.width, bounds.height);
	hopalong_view_map(view);
}

static void
hopalong_layer_shell_surface_commit(struct wl_listener *listener, void *data)
{
	struct hopalong_view *view = wl_container_of(listener, view, surface_commit);
	bool mapped = view->mapped;

	/* if the layer moved, prepare to put it on the right list. */
	if (mapped)
		hopalong_view_unmap(view);

	view->layer = layer_mapping[view->layer_surface->current.layer];
	arrange_layer(view);
}

static void
hopalong_layer_shell_nop(struct hopalong_view *view)
{
}

static const char *
hopalong_layer_shell_getprop(struct hopalong_view *view, enum hopalong_view_prop prop)
{
	return NULL;
}

static struct wlr_surface *
hopalong_layer_shell_get_surface(struct hopalong_view *view)
{
	return view->layer_surface->surface;
}

static bool
hopalong_layer_shell_get_geometry(struct hopalong_view *view, struct wlr_box *box)
{
	box->x = view->x;
	box->y = view->y;
	box->width = view->layer_surface->current.actual_width;
	box->height = view->layer_surface->current.actual_height;

	return true;
}

static struct wlr_surface *
hopalong_layer_shell_surface_at(struct hopalong_view *view, double x, double y, double *sx, double *sy)
{
	double view_sx = x - view->x;
	double view_sy = y - view->y;

	double _sx = 0.0, _sy = 0.0;
	struct wlr_surface *surface = wlr_layer_surface_v1_surface_at(view->layer_surface, view_sx, view_sy, &_sx, &_sy);

	if (surface != NULL)
	{
		*sx = _sx;
		*sy = _sy;
	}

	return surface;
}

static const struct hopalong_view_ops hopalong_layer_shell_view_ops = {
	.minimize = hopalong_layer_shell_nop,
	.maximize = hopalong_layer_shell_nop,
	.close = hopalong_layer_shell_nop,
	.getprop = hopalong_layer_shell_getprop,
	.get_surface = hopalong_layer_shell_get_surface,
	.set_activated = (void *) hopalong_layer_shell_nop,
	.get_geometry = hopalong_layer_shell_get_geometry,
	.set_size = (void *) hopalong_layer_shell_nop,
	.surface_at = hopalong_layer_shell_surface_at,
};

static void
hopalong_layer_shell_new_surface(struct wl_listener *listener, void *data)
{
	struct hopalong_server *server = wl_container_of(listener, server, new_layer_surface);
	struct wlr_layer_surface_v1 *layer_surface = data;

	struct hopalong_view *view = calloc(1, sizeof(*view));

	view->server = server;
	view->ops = &hopalong_layer_shell_view_ops;

	view->layer_surface = layer_surface;
	layer_surface->data = view;

	if (layer_surface->output == NULL)
	{
		struct wlr_output *output = wlr_output_layout_output_at(
			server->output_layout, server->cursor->x,
			server->cursor->y);

		layer_surface->output = output;
	}

	view->layer = layer_mapping[layer_surface->client_pending.layer];

	view->map.notify = hopalong_layer_shell_surface_map;
	wl_signal_add(&layer_surface->events.map, &view->map);

	view->unmap.notify = hopalong_layer_shell_surface_unmap;
	wl_signal_add(&layer_surface->events.unmap, &view->unmap);

	view->destroy.notify = hopalong_layer_shell_destroy;
	wl_signal_add(&layer_surface->events.destroy, &view->destroy);

	view->surface_commit.notify = hopalong_layer_shell_surface_commit;
	wl_signal_add(&layer_surface->surface->events.commit, &view->surface_commit);

	wl_list_insert(&server->views, &view->link);
}

void
hopalong_layer_shell_setup(struct hopalong_server *server)
{
	server->wlr_layer_shell = wlr_layer_shell_v1_create(server->display);

	server->new_layer_surface.notify = hopalong_layer_shell_new_surface;
	wl_signal_add(&server->wlr_layer_shell->events.new_surface, &server->new_layer_surface);
}

void
hopalong_layer_shell_teardown(struct hopalong_server *server)
{
}