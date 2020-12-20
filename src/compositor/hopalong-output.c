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
#include "hopalong-output.h"

struct render_data {
	struct wlr_output *output;
	struct hopalong_view *view;
	struct wlr_renderer *renderer;
	struct timespec *when;
};

static void
render_surface(struct wlr_surface *surface, int sx, int sy, void *data)
{
	return_if_fail(surface != NULL);

	struct render_data *rdata = data;
	return_if_fail(rdata != NULL);

	struct hopalong_view *view = rdata->view;
	return_if_fail(view != NULL);

	struct wlr_output *output = rdata->output;
	return_if_fail(output != NULL);

	/* get a GPU texture */
	struct wlr_texture *texture = wlr_surface_get_texture(surface);
	return_if_fail(texture != NULL);

	/* translate to output-local coordinates */
	double ox = 0, oy = 0;
	wlr_output_layout_output_coords(view->server->output_layout, output, &ox, &oy);
	ox += view->x + sx;
	oy += view->y + sy;

	/* set up our box */
	struct wlr_box box = {
		.x = ox * output->scale,
		.y = oy * output->scale,
		.width = surface->current.width * output->scale,
		.height = surface->current.height * output->scale
	};

	/* convert box to matrix */
	float matrix[9];
	enum wl_output_transform transform = wlr_output_transform_invert(surface->current.transform);
	wlr_matrix_project_box(matrix, &box, transform, 0, output->transform_matrix);

	/* render the matrix + texture to the screen */
	wlr_render_texture_with_matrix(rdata->renderer, texture, matrix, 1);

	/* tell the client rendering is done */
	wlr_surface_send_frame_done(surface, rdata->when);
}

static void
hopalong_output_frame_notify(struct wl_listener *listener, void *data)
{
	struct hopalong_output *output = wl_container_of(listener, output, frame);
	return_if_fail(output != NULL);

	struct wlr_renderer *renderer = output->server->renderer;
	return_if_fail(renderer != NULL);

	/* get our render TS */
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	if (!wlr_output_attach_render(output->wlr_output, NULL))
		return;

	/* determine our viewport resolution. */
	int width, height;
	wlr_output_effective_resolution(output->wlr_output, &width, &height);

	/* start rendering */
	wlr_renderer_begin(renderer, width, height);

	/* clear to something slightly off-gray in order to show the renderer is alive */
	float color[4] = {0.3, 0.3, 0.5, 1.0};
	wlr_renderer_clear(renderer, color);

	/* render the views */
	struct hopalong_view *view;
	wl_list_for_each_reverse(view, &output->server->views, link)
	{
		if (!view->mapped)
			continue;

		struct render_data rdata = {
			.output = output->wlr_output,
			.view = view,
			.renderer = renderer,
			.when = &now,
		};

		wlr_xdg_surface_for_each_surface(view->xdg_surface, render_surface, &rdata);
	}

	/* renderer our cursor if we need to */
	wlr_output_render_software_cursors(output->wlr_output, NULL);

	/* finish rendering */
	wlr_renderer_end(renderer);
	wlr_output_commit(output->wlr_output);
}

/*
 * Creates a new Hopalong output from a wlroots output.
 */
struct hopalong_output *
hopalong_output_new_from_wlr_output(struct hopalong_server *server, struct wlr_output *wlr_output)
{
	return_val_if_fail(server != NULL, NULL);
	return_val_if_fail(wlr_output != NULL, NULL);

	struct hopalong_output *output = calloc(1, sizeof(*output));
	return_val_if_fail(output != NULL, NULL);

	output->wlr_output = wlr_output;
	output->server = server;

	output->frame.notify = hopalong_output_frame_notify;
	wl_signal_add(&wlr_output->events.frame, &output->frame);

	wl_list_insert(&server->outputs, &output->link);

	return output;
}

/*
 * Destroys a Hopalong output object.
 */
void
hopalong_output_destroy(struct hopalong_output *output)
{
	return_if_fail(output != NULL);

	/* XXX: should we destroy the underlying wlr_output? */

	wl_list_remove(&output->link);
	free(output);	
}
