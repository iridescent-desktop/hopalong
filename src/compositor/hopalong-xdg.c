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
#include "hopalong-xdg.h"
#include "hopalong-server.h"

bool
hopalong_xdg_view_at(struct hopalong_view *view,
	double lx, double ly, struct wlr_surface **surface, double *sx, double *sy)
{
	double view_sx = lx - view->x;
	double view_sy = ly - view->y;

	double _sx, _sy;
	struct wlr_surface *_surface = NULL;
	_surface = wlr_xdg_surface_surface_at(view->xdg_surface, view_sx, view_sy, &_sx, &_sy);

	if (_surface != NULL)
	{
		*sx = _sx;
		*sy = _sy;
		*surface = _surface;
		return true;
	}

	return false;
}

struct hopalong_view *
hopalong_xdg_desktop_view_at(struct hopalong_server *server, double lx, double ly,
	struct wlr_surface **surface, double *sx, double *sy)
{
	struct hopalong_view *view;

	wl_list_for_each(view, &server->views, link)
	{
		if (hopalong_xdg_view_at(view, lx, ly, surface, sx, sy))
			return view;
	}

	return NULL;
}

void
hopalong_xdg_focus_view(struct hopalong_view *view, struct wlr_surface *surface)
{
	return_if_fail(view != NULL);

	struct hopalong_server *server = view->server;
	return_if_fail(server != NULL);

	struct wlr_seat *seat = server->seat;
	return_if_fail(seat != NULL);

	struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
	if (prev_surface == surface)
		return;

	if (prev_surface != NULL)
	{
		struct wlr_xdg_surface *previous =
			wlr_xdg_surface_from_wlr_surface(seat->keyboard_state.focused_surface);

		wlr_xdg_toplevel_set_activated(previous, false);
	}

	struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);
	wlr_seat_keyboard_notify_enter(seat, view->xdg_surface->surface,
		keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);

	wl_list_remove(&view->link);
	wl_list_insert(&server->views, &view->link);
	wlr_xdg_toplevel_set_activated(view->xdg_surface, true);
}

static void
hopalong_xdg_surface_map(struct wl_listener *listener, void *data)
{
	return_if_fail(listener != NULL);

	struct hopalong_view *view = wl_container_of(listener, view, map);
	view->mapped = true;

	hopalong_xdg_focus_view(view, view->xdg_surface->surface);
}

static void
hopalong_xdg_surface_unmap(struct wl_listener *listener, void *data)
{
	return_if_fail(listener != NULL);

	struct hopalong_view *view = wl_container_of(listener, view, unmap);
	view->mapped = false;
}

static void
hopalong_xdg_surface_destroy(struct wl_listener *listener, void *data)
{
	return_if_fail(listener != NULL);

	struct hopalong_view *view = wl_container_of(listener, view, destroy);
	wl_list_remove(&view->link);
	free(view);
}

static void
hopalong_xdg_begin_drag(struct hopalong_view *view, enum hopalong_cursor_mode mode, uint32_t edges)
{
	struct hopalong_server *server = view->server;
	return_if_fail(server != NULL);

	struct wlr_surface *focused_surface = server->seat->pointer_state.focused_surface;
	return_if_fail(focused_surface != NULL);

	if (view->xdg_surface->surface != focused_surface)
		return;

	server->grabbed_view = view;
	server->cursor_mode = mode;

	if (mode == HOPALONG_CURSOR_MOVE)
	{
		server->grab_x = server->cursor->x - view->x;
		server->grab_y = server->cursor->y - view->y;
	}
	else if (mode == HOPALONG_CURSOR_RESIZE)
	{
		struct wlr_box geo_box;

		wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);

		double border_x = (view->x + geo_box.x) + ((edges & WLR_EDGE_RIGHT) ? geo_box.width : 0);
		double border_y = (view->y + geo_box.y) + ((edges & WLR_EDGE_BOTTOM) ? geo_box.height : 0);

		server->grab_x = server->cursor->x - border_x;
		server->grab_y = server->cursor->y - border_y;

		server->grab_geobox = geo_box;
		server->grab_geobox.x += view->x;
		server->grab_geobox.y += view->y;

		server->resize_edges = edges;
	}
}

static void
hopalong_xdg_toplevel_request_move(struct wl_listener *listener, void *data)
{
	return_if_fail(listener != NULL);

	struct hopalong_view *view = wl_container_of(listener, view, request_move);
	hopalong_xdg_begin_drag(view, HOPALONG_CURSOR_MOVE, 0);
}

static void
hopalong_xdg_toplevel_request_resize(struct wl_listener *listener, void *data)
{
	return_if_fail(listener != NULL);
	return_if_fail(data != NULL);

	struct wlr_xdg_toplevel_resize_event *event = data;
	struct hopalong_view *view = wl_container_of(listener, view, request_resize);
	hopalong_xdg_begin_drag(view, HOPALONG_CURSOR_RESIZE, event->edges);
}

struct hopalong_deco_state {
	struct hopalong_server *server;
	struct wlr_xdg_toplevel_decoration_v1 *wlr_deco;

	struct wl_listener request_mode;
	struct wl_listener destroy;
};

static void
hopalong_xdg_decoration_request_mode(struct wl_listener *listener, void *data)
{
	struct wlr_xdg_toplevel_decoration_v1 *wlr_deco = data;
	wlr_xdg_toplevel_decoration_v1_set_mode(wlr_deco,
			WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
}

static void
hopalong_xdg_decoration_destroy(struct wl_listener *listener, void *data)
{
	struct hopalong_deco_state *d = wl_container_of(listener, d, destroy);

	wl_list_remove(&d->destroy.link);
	wl_list_remove(&d->request_mode.link);
	free(d);
}

static void
hopalong_xdg_toplevel_new_decoration(struct wl_listener *listener, void *data)
{
	return_if_fail(listener != NULL);
	return_if_fail(data != NULL);

	struct hopalong_server *server = wl_container_of(listener, server, new_toplevel_decoration);
	return_if_fail(server != NULL);

	struct wlr_xdg_toplevel_decoration_v1 *wlr_deco = data;

	struct hopalong_deco_state *deco_state = calloc(1, sizeof(*deco_state));

	deco_state->server = server;
	deco_state->wlr_deco = wlr_deco;

	deco_state->request_mode.notify = hopalong_xdg_decoration_request_mode;
	wl_signal_add(&wlr_deco->events.request_mode, &deco_state->request_mode);

	deco_state->destroy.notify = hopalong_xdg_decoration_destroy;
	wl_signal_add(&wlr_deco->events.destroy, &deco_state->destroy);

	hopalong_xdg_decoration_request_mode(&deco_state->request_mode, wlr_deco);
}

static void
hopalong_xdg_new_surface(struct wl_listener *listener, void *data)
{
	struct hopalong_server *server = wl_container_of(listener, server, new_xdg_surface);
	return_if_fail(server != NULL);

	struct wlr_xdg_surface *xdg_surface = data;
	return_if_fail(xdg_surface != NULL);

	if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL)
		return;

	struct hopalong_view *view = calloc(1, sizeof(*view));
	view->server = server;
	view->xdg_surface = xdg_surface;

	/* hook up our xdg_surface events */
	view->map.notify = hopalong_xdg_surface_map;
	wl_signal_add(&xdg_surface->events.map, &view->map);

	view->unmap.notify = hopalong_xdg_surface_unmap;
	wl_signal_add(&xdg_surface->events.unmap, &view->unmap);

	view->destroy.notify = hopalong_xdg_surface_destroy;
	wl_signal_add(&xdg_surface->events.destroy, &view->destroy);

	/* hook up our xdg_toplevel events */
	struct wlr_xdg_toplevel *xdg_toplevel = xdg_surface->toplevel;
	return_if_fail(xdg_toplevel != NULL);

	view->request_move.notify = hopalong_xdg_toplevel_request_move;
	wl_signal_add(&xdg_toplevel->events.request_move, &view->request_move);

	view->request_resize.notify = hopalong_xdg_toplevel_request_resize;
	wl_signal_add(&xdg_toplevel->events.request_resize, &view->request_resize);

	/* add to the list of views */
	wl_list_insert(&server->views, &view->link);
}

/*
 * Sets up resources related to XDG shell support.
 */
void
hopalong_xdg_shell_setup(struct hopalong_server *server)
{
	return_if_fail(server != NULL);

	wl_list_init(&server->views);

	server->xdg_shell = wlr_xdg_shell_create(server->display);
	return_if_fail(server->xdg_shell);

	server->new_xdg_surface.notify = hopalong_xdg_new_surface;
	wl_signal_add(&server->xdg_shell->events.new_surface, &server->new_xdg_surface);

	server->xdg_deco_mgr = wlr_xdg_decoration_manager_v1_create(server->display);
	server->new_toplevel_decoration.notify = hopalong_xdg_toplevel_new_decoration;
	wl_signal_add(&server->xdg_deco_mgr->events.new_toplevel_decoration, &server->new_toplevel_decoration);
}

/*
 * Tears down resources related to XDG shell support.
 */
void
hopalong_xdg_shell_teardown(struct hopalong_server *server)
{
	return_if_fail(server != NULL);
}
