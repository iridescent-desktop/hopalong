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

#include "hopalong-style.h"

static const struct hopalong_style default_style = {
	.base_bg = {0.3, 0.3, 0.5, 1.0},

	.title_bar_bg = {0.1, 0.1, 0.9, 1.0},
	.title_bar_bg_inactive = {0.9, 0.9, 0.9, 1.0},

	.title_bar_fg = {1.0, 1.0, 1.0, 1.0},
	.title_bar_fg_inactive = {0.2, 0.2, 0.2, 1.0},

	.minimize_btn_fg = {1.0, 1.0, 1.0, 1.0},
	.minimize_btn_fg_inactive = {0.2, 0.2, 0.2, 0.2},

	.maximize_btn_fg = {1.0, 1.0, 1.0, 1.0},
	.maximize_btn_fg_inactive = {0.2, 0.2, 0.2, 0.2},

	.close_btn_fg = {1.0, 1.0, 1.0, 1.0},
	.close_btn_fg_inactive = {0.2, 0.2, 0.2, 0.2},

	.border = {0.5, 0.5, 0.5, 1.0},
	.border_inactive = {0.5, 0.5, 0.5, 1.0},

	.border_thickness = 1,
	.title_bar_height = 32,
	.title_bar_padding = 8,

	.title_bar_font = "Sans 10",
};

// static struct hopalong_style custom_style = {};

const struct hopalong_style *
hopalong_style_get_default(void)
{
	return &default_style;
}