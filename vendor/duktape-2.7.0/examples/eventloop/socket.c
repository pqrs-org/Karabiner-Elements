/*
 *  TCP sockets binding example.
 */

#define _GNU_SOURCE
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#include "duktape.h"

#define  ERROR_FROM_ERRNO(ctx)  do { \
		(void) duk_error(ctx, DUK_ERR_ERROR, "%s (errno=%d)", strerror(errno), errno); \
	} while (0)

static void set_nonblocking(duk_context *ctx, int fd) {
	int rc;
	int flags;

	rc = fcntl(fd, F_GETFL);
	if (rc < 0) {
		ERROR_FROM_ERRNO(ctx);
	}
	flags = rc;

	flags |= O_NONBLOCK;

	rc = fcntl(fd, F_SETFL, flags);
	if (rc < 0) {
		ERROR_FROM_ERRNO(ctx);
	}
}

static void set_reuseaddr(duk_context *ctx, int fd) {
	int val;
	int rc;

	val = 1;
	rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *) &val, sizeof(val));
	if (rc != 0) {
		ERROR_FROM_ERRNO(ctx);
	}
}

#if defined(__APPLE__)
static void set_nosigpipe(duk_context *ctx, int fd) {
	int val;
	int rc;

	val = 1;
	rc = setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (const void *) &val, sizeof(val));
	if (rc != 0) {
		ERROR_FROM_ERRNO(ctx);
	}
}
#endif

static int socket_create_server_socket(duk_context *ctx) {
	const char *addr = duk_to_string(ctx, 0);
	int port = duk_to_int(ctx, 1);
	int sock;
	struct sockaddr_in sockaddr;
	struct hostent *ent;
	struct in_addr **addr_list;
	struct in_addr *addr_inet;
	int i;
	int rc;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		ERROR_FROM_ERRNO(ctx);
	}

	set_nonblocking(ctx, sock);
	set_reuseaddr(ctx, sock);
#if defined(__APPLE__)
	set_nosigpipe(ctx, sock);
#endif

	ent = gethostbyname(addr);
	if (!ent) {
		ERROR_FROM_ERRNO(ctx);
	}

	addr_list = (struct in_addr **) ent->h_addr_list;
	addr_inet = NULL;
	for (i = 0; addr_list[i]; i++) {
		addr_inet = addr_list[i];
		break;
	}
	if (!addr_inet) {
		(void) duk_error(ctx, DUK_ERR_ERROR, "cannot resolve %s", addr);
	}

	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr = *addr_inet;

	rc = bind(sock, (const struct sockaddr *) &sockaddr, sizeof(sockaddr));
	if (rc < 0) {
		ERROR_FROM_ERRNO(ctx);
	}

	rc = listen(sock, 10 /*backlog*/);
	if (rc < 0) {
		(void) close(sock);
		ERROR_FROM_ERRNO(ctx);
	}

	duk_push_int(ctx, sock);
	return 1;
}

static int socket_close(duk_context *ctx) {
	int sock = duk_to_int(ctx, 0);
	int rc;

	rc = close(sock);
	if (rc < 0) {
		ERROR_FROM_ERRNO(ctx);
	}
	return 0;
}

static int socket_accept(duk_context *ctx) {
	int sock = duk_to_int(ctx, 0);
	int rc;
	struct sockaddr_in addr;
	socklen_t addrlen;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addrlen = sizeof(addr);

	rc = accept(sock, (struct sockaddr *) &addr, &addrlen);
	if (rc < 0) {
		ERROR_FROM_ERRNO(ctx);
	}

	set_nonblocking(ctx, sock);
#if defined(__APPLE__)
	set_nosigpipe(ctx, sock);
#endif

	if (addrlen == sizeof(addr)) {
		uint32_t tmp = ntohl(addr.sin_addr.s_addr);

		duk_push_object(ctx);

		duk_push_string(ctx, "fd");
		duk_push_int(ctx, rc);
		duk_put_prop(ctx, -3);
		duk_push_string(ctx, "addr");
		duk_push_sprintf(ctx, "%d.%d.%d.%d", ((tmp >> 24) & 0xff), ((tmp >> 16) & 0xff), ((tmp >> 8) & 0xff), (tmp & 0xff));
		duk_put_prop(ctx, -3);
		duk_push_string(ctx, "port");
		duk_push_int(ctx, ntohs(addr.sin_port));
		duk_put_prop(ctx, -3);

		return 1;
	}

	return 0;
}

static int socket_connect(duk_context *ctx) {
	const char *addr = duk_to_string(ctx, 0);
	int port = duk_to_int(ctx, 1);
	int sock;
	struct sockaddr_in sockaddr;
	struct hostent *ent;
	struct in_addr **addr_list;
	struct in_addr *addr_inet;
	int i;
	int rc;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		ERROR_FROM_ERRNO(ctx);
	}

	set_nonblocking(ctx, sock);
#if defined(__APPLE__)
	set_nosigpipe(ctx, sock);
#endif

	ent = gethostbyname(addr);
	if (!ent) {
		ERROR_FROM_ERRNO(ctx);
	}

	addr_list = (struct in_addr **) ent->h_addr_list;
	addr_inet = NULL;
	for (i = 0; addr_list[i]; i++) {
		addr_inet = addr_list[i];
		break;
	}
	if (!addr_inet) {
		(void) duk_error(ctx, DUK_ERR_ERROR, "cannot resolve %s", addr);
	}

	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr = *addr_inet;

	rc = connect(sock, (const struct sockaddr *) &sockaddr, (socklen_t) sizeof(sockaddr));
	if (rc < 0) {
		if (errno == EINPROGRESS) {
#if 0
			fprintf(stderr, "connect() returned EINPROGRESS as expected, need to poll writability\n");
			fflush(stderr);
#endif
		} else {
			ERROR_FROM_ERRNO(ctx);
		}
	}

	duk_push_int(ctx, sock);
	return 1;
}

static int socket_read(duk_context *ctx) {
	int sock = duk_to_int(ctx, 0);
	char readbuf[1024];
	int rc;
	void *data;

	rc = recvfrom(sock, (void *) readbuf, sizeof(readbuf), 0, NULL, NULL);
	if (rc < 0) {
		ERROR_FROM_ERRNO(ctx);
	}

	data = duk_push_fixed_buffer(ctx, rc);
	memcpy(data, readbuf, rc);
	return 1;
}

static int socket_write(duk_context *ctx) {
	int sock = duk_to_int(ctx, 0);
	const char *data;
	size_t len;
	ssize_t rc;

	data = duk_require_buffer_data(ctx, 1, &len);

	/* MSG_NOSIGNAL: avoid SIGPIPE */
#if defined(__APPLE__)
	rc = sendto(sock, (void *) data, len, 0, NULL, 0);
#else
	rc = sendto(sock, (void *) data, len, MSG_NOSIGNAL, NULL, 0);
#endif
	if (rc < 0) {
		ERROR_FROM_ERRNO(ctx);
	}

	duk_push_int(ctx, rc);
	return 1;
}

static duk_function_list_entry socket_funcs[] = {
	{ "createServerSocket", socket_create_server_socket, 2 },
	{ "close", socket_close, 1 },
	{ "accept", socket_accept, 1 },
	{ "connect", socket_connect, 2 },
	{ "read", socket_read, 1 },
	{ "write", socket_write, 2 },
	{ NULL, NULL, 0 }
};

void socket_register(duk_context *ctx) {
	/* Set global 'Socket'. */
	duk_push_global_object(ctx);
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, socket_funcs);
	duk_put_prop_string(ctx, -2, "Socket");
	duk_pop(ctx);
}
