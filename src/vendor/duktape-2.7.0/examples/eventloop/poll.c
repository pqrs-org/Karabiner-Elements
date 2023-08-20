/*
 *  C wrapper for poll().
 */

#define _GNU_SOURCE
#include <errno.h>
#include <string.h>
#include <stdio.h>

#if defined(_WIN32)
#include <WinSock2.h>
#define poll WSAPoll
#pragma comment(lib, "ws2_32")
#else
#include <poll.h>
#endif

#include <time.h>

#include "duktape.h"

static int poll_poll(duk_context *ctx) {
	int timeout = duk_to_int(ctx, 1);
	int i, n, nchanged;
	int fd, rc;
	struct pollfd fds[20];
	struct timespec ts;

	memset(fds, 0, sizeof(fds));

	n = 0;
	duk_enum(ctx, 0, 0 /*enum_flags*/);
	while (duk_next(ctx, -1, 0)) {
		if ((size_t) n >= sizeof(fds) / sizeof(struct pollfd)) {
			return -1;
		}

		/* [... enum key] */
		duk_dup_top(ctx);  /* -> [... enum key key] */
		duk_get_prop(ctx, 0);  /* -> [... enum key val] */
		fd = duk_to_int(ctx, -2);

		duk_push_string(ctx, "events");
		duk_get_prop(ctx, -2);  /* -> [... enum key val events] */

		fds[n].fd = fd;
		fds[n].events = duk_to_int(ctx, -1);
		fds[n].revents = 0;

		duk_pop_n(ctx, 3);  /* -> [... enum] */

		n++;
	}
	/* leave enum on stack */

	memset(&ts, 0, sizeof(ts));
	ts.tv_nsec = (timeout % 1000) * 1000000;
	ts.tv_sec = timeout / 1000;

	/*rc = ppoll(fds, n, &ts, NULL);*/
	rc = poll(fds, n, timeout);
	if (rc < 0) {
		(void) duk_error(ctx, DUK_ERR_ERROR, "%s (errno=%d)", strerror(errno), errno);
	}

	duk_push_array(ctx);
	nchanged = 0;
	for (i = 0; i < n; i++) {
		/* update revents */

		if (fds[i].revents) {
			duk_push_int(ctx, fds[i].fd);  /* -> [... retarr fd] */
			duk_put_prop_index(ctx, -2, nchanged);
			nchanged++;
		}

		duk_push_int(ctx, fds[i].fd);  /* -> [... retarr key] */
		duk_get_prop(ctx, 0);  /* -> [... retarr val] */
		duk_push_string(ctx, "revents");
		duk_push_int(ctx, fds[i].revents);  /* -> [... retarr val "revents" fds[i].revents] */
		duk_put_prop(ctx, -3);  /* -> [... retarr val] */
		duk_pop(ctx);
	}

	/* [retarr] */

	return 1;
}

static duk_function_list_entry poll_funcs[] = {
	{ "poll", poll_poll, 2 },
	{ NULL, NULL, 0 }
};

static duk_number_list_entry poll_consts[] = {
	{ "POLLIN", (double) POLLIN },
	{ "POLLPRI", (double) POLLPRI },
	{ "POLLOUT", (double) POLLOUT },
#if 0
	/* Linux 2.6.17 and upwards, requires _GNU_SOURCE etc, not added
	 * now because we don't use it.
	 */
	{ "POLLRDHUP", (double) POLLRDHUP },
#endif
	{ "POLLERR", (double) POLLERR },
	{ "POLLHUP", (double) POLLHUP },
	{ "POLLNVAL", (double) POLLNVAL },
	{ NULL, 0.0 }
};

void poll_register(duk_context *ctx) {
	/* Set global 'Poll' with functions and constants. */
	duk_push_global_object(ctx);
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, poll_funcs);
	duk_put_number_list(ctx, -1, poll_consts);
	duk_put_prop_string(ctx, -2, "Poll");
	duk_pop(ctx);
}
