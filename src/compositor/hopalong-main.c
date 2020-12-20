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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <spawn.h>
#include "hopalong-server.h"

static void
launch_session_leader(const char *socket, char *program)
{
	char sockenv[4096];
	char xdgruntimedir[4096];
	char *sockenvp[] = { sockenv, xdgruntimedir, NULL };

	snprintf(sockenv, sizeof sockenv, "WAYLAND_DISPLAY=%s", socket);
	snprintf(xdgruntimedir, sizeof xdgruntimedir, "XDG_RUNTIME_DIR=%s", getenv("XDG_RUNTIME_DIR"));

	char *shellargs[] = { "/bin/sh", "-c", program, NULL };

	wlr_log(WLR_INFO, "Launching session leader: %s", program);

	pid_t child;
	if (posix_spawn(&child, "/bin/sh", NULL, NULL, shellargs, sockenvp) != 0)
		wlr_log(WLR_ERROR, "Failed to launch session leader (%s): %s", program, strerror(errno));

	wlr_log(WLR_INFO, "Session leader running as PID %u", child);
}

static void
version(void)
{
	printf("hopalong 0.1\n");
	exit(EXIT_SUCCESS);
}

static void
usage(int code)
{
	printf("usage: hopalong [options] [program]\n"
	       "\n"
	       "If [program] is specified, it will be launched to start a session.\n");
	exit(code);
}

int
main(int argc, char *argv[])
{
	wlr_log_init(WLR_INFO, NULL);

	static struct option long_options[] = {
		{"version",	no_argument, 0, 'V'},
		{"help",	no_argument, 0, 'h'},
		{NULL,		0,	     0, 0 },
	};

	for (;;)
	{
		int c = getopt_long(argc, argv, "Vh", long_options, NULL);

		if (c == -1)
			break;

		switch (c)
		{
		case 'V':
			version();
			break;

		case 'h':
			usage(EXIT_SUCCESS);
			break;

		default:
			usage(EXIT_FAILURE);
			break;
		}
	}

	struct hopalong_server *server = hopalong_server_new();

	if (server == NULL)
	{
		fprintf(stderr, "Failed to initialize Hopalong.\n");
		return EXIT_FAILURE;
	}

	const char *socket = hopalong_server_add_socket(server);
	if (socket == NULL)
	{
		fprintf(stderr, "Failed to open socket for Wayland clients.\n");
		return EXIT_FAILURE;	
	}

	wlr_log(WLR_INFO, "Listening for Wayland clients at: %s", socket);

	if (optind < argc)
		launch_session_leader(socket, argv[optind]);

	if (!hopalong_server_run(server))
	{
		hopalong_server_destroy(server);

		fprintf(stderr, "Hopalong server terminating uncleanly.\n");
		return EXIT_FAILURE;
	}

	hopalong_server_destroy(server);
	return EXIT_SUCCESS;
}
