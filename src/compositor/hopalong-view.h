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

#ifndef HOPALONG_COMPOSITOR_VIEW_H
#define HOPALONG_COMPOSITOR_VIEW_H

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

struct hopalong_output;
struct hopalong_server;

enum hopalong_view_frame_area {
	HOPALONG_VIEW_FRAME_AREA_TOP,
	HOPALONG_VIEW_FRAME_AREA_BOTTOM,
	HOPALONG_VIEW_FRAME_AREA_LEFT,
	HOPALONG_VIEW_FRAME_AREA_RIGHT,
	HOPALONG_VIEW_FRAME_AREA_TITLEBAR,
	HOPALONG_VIEW_FRAME_AREA_MINIMIZE,
	HOPALONG_VIEW_FRAME_AREA_MAXIMIZE,
	HOPALONG_VIEW_FRAME_AREA_CLOSE,
	HOPALONG_VIEW_FRAME_AREA_COUNT,
};

struct hopalong_view {
	struct wl_list link;
	struct hopalong_server *server;
	struct wlr_xdg_surface *xdg_surface;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener set_title;
	bool mapped;
	int x, y;

	struct wlr_box frame_areas[HOPALONG_VIEW_FRAME_AREA_COUNT];

	/* the area of the frame the pointer is hovering over if any */
	int frame_area;
	int frame_area_edges;

	/* textures owned by this view */
	struct wlr_texture *title;
	struct wlr_texture *title_inactive;
	struct wlr_box title_box;
	bool title_dirty;

	/* client-side decorations */
	bool using_csd;
};

extern bool hopalong_view_generate_textures(struct hopalong_output *output, struct hopalong_view *view);

#endif
