/*
 *  Example debug transport using a Windows TCP socket
 *
 *  Provides a TCP server socket which a debug client can connect to.
 *  After that data is just passed through.
 *
 *  https://msdn.microsoft.com/en-us/library/windows/desktop/ms737593(v=vs.85).aspx
 *
 *  Compiling 'duk' with debugger support using MSVC (Visual Studio):
 *
 *    > python2 tools\configure.py \
 *          --output-directory prep
 *          -DDUK_USE_DEBUGGER_SUPPORT -DDUK_USE_INTERRUPT_COUNTER
 *    > cl /W3 /O2 /Feduk.exe \
 *          /DDUK_CMDLINE_DEBUGGER_SUPPORT
 *          /Iexamples\debug-trans-socket /Iprep
 *          examples\cmdline\duk_cmdline.c
 *          examples\debug-trans-socket\duk_trans_socket_windows.c
 *          prep\duktape.c
 *
 *  With MinGW:
 *
 *    $ python2 tools\configure.py \
 *          --output-directory prep
 *          -DDUK_USE_DEBUGGER_SUPPORT -DDUK_USE_INTERRUPT_COUNTER
 *    $ gcc -oduk.exe -Wall -O2 \
 *          -DDUK_CMDLINE_DEBUGGER_SUPPORT \
 *          -Iexamples/debug-trans-socket -Iprep \
 *          examples/cmdline/duk_cmdline.c \
 *          examples/debug-trans-socket/duk_trans_socket_windows.c \
 *          prep/duktape.c -lm -lws2_32
 */

#undef UNICODE
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif

/* MinGW workaround for missing getaddrinfo() etc:
 * http://programmingrants.blogspot.fi/2009/09/tips-on-undefined-reference-to.html
 */
#if defined(__MINGW32__) || defined(__MINGW64__)
#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0501
#endif
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include "duktape.h"
#include "duk_trans_socket.h"

#if defined(_MSC_VER)
#pragma comment (lib, "Ws2_32.lib")
#endif

#if !defined(DUK_DEBUG_PORT)
#define DUK_DEBUG_PORT 9091
#endif
#if !defined(DUK_DEBUG_ADDRESS)
#define DUK_DEBUG_ADDRESS "0.0.0.0"
#endif
#define DUK__STRINGIFY_HELPER(x) #x
#define DUK__STRINGIFY(x) DUK__STRINGIFY_HELPER(x)

#if 0
#define DEBUG_PRINTS
#endif

static SOCKET server_sock = INVALID_SOCKET;
static SOCKET client_sock = INVALID_SOCKET;
static int wsa_inited = 0;

/*
 *  Transport init and finish
 */

void duk_trans_socket_init(void) {
	WSADATA wsa_data;
	struct addrinfo hints;
	struct addrinfo *result = NULL;
	int rc;

	memset((void *) &wsa_data, 0, sizeof(wsa_data));
	memset((void *) &hints, 0, sizeof(hints));

	rc = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (rc != 0) {
		fprintf(stderr, "%s: WSAStartup() failed: %d\n", __FILE__, rc);
		fflush(stderr);
		goto fail;
	}
	wsa_inited = 1;

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	rc = getaddrinfo(DUK_DEBUG_ADDRESS, DUK__STRINGIFY(DUK_DEBUG_PORT), &hints, &result);
	if (rc != 0) {
		fprintf(stderr, "%s: getaddrinfo() failed: %d\n", __FILE__, rc);
		fflush(stderr);
		goto fail;
	}

	server_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (server_sock == INVALID_SOCKET) {
		fprintf(stderr, "%s: socket() failed with error: %ld\n",
		        __FILE__, (long) WSAGetLastError());
		fflush(stderr);
		goto fail;
	}

	rc = bind(server_sock, result->ai_addr, (int) result->ai_addrlen);
	if (rc == SOCKET_ERROR) {
		fprintf(stderr, "%s: bind() failed with error: %ld\n",
		        __FILE__, (long) WSAGetLastError());
		fflush(stderr);
		goto fail;
	}

	rc = listen(server_sock, SOMAXCONN);
	if (rc == SOCKET_ERROR) {
		fprintf(stderr, "%s: listen() failed with error: %ld\n",
		        __FILE__, (long) WSAGetLastError());
		fflush(stderr);
		goto fail;
	}

	if (result != NULL) {
		freeaddrinfo(result);
		result = NULL;
	}
	return;

 fail:
	if (result != NULL) {
		freeaddrinfo(result);
		result = NULL;
	}
	if (server_sock != INVALID_SOCKET) {
		(void) closesocket(server_sock);
		server_sock = INVALID_SOCKET;
	}
	if (wsa_inited) {
		WSACleanup();
		wsa_inited = 0;
	}
}

void duk_trans_socket_finish(void) {
	if (client_sock != INVALID_SOCKET) {
		(void) closesocket(client_sock);
		client_sock = INVALID_SOCKET;
	}
	if (server_sock != INVALID_SOCKET) {
		(void) closesocket(server_sock);
		server_sock = INVALID_SOCKET;
	}
	if (wsa_inited) {
		WSACleanup();
		wsa_inited = 0;
	}
}

void duk_trans_socket_waitconn(void) {
	if (server_sock == INVALID_SOCKET) {
		fprintf(stderr, "%s: no server socket, skip waiting for connection\n",
		        __FILE__);
		fflush(stderr);
		return;
	}
	if (client_sock != INVALID_SOCKET) {
		(void) closesocket(client_sock);
		client_sock = INVALID_SOCKET;
	}

	fprintf(stderr, "Waiting for debug connection on port %d\n", (int) DUK_DEBUG_PORT);
	fflush(stderr);

	client_sock = accept(server_sock, NULL, NULL);
	if (client_sock == INVALID_SOCKET) {
		fprintf(stderr, "%s: accept() failed with error %ld, skip waiting for connection\n",
		        __FILE__, (long) WSAGetLastError());
		fflush(stderr);
		goto fail;
	}

	fprintf(stderr, "Debug connection established\n");
	fflush(stderr);

	/* XXX: For now, close the listen socket because we won't accept new
	 * connections anyway.  A better implementation would allow multiple
	 * debug attaches.
	 */

	if (server_sock != INVALID_SOCKET) {
		(void) closesocket(server_sock);
		server_sock = INVALID_SOCKET;
	}
	return;

 fail:
	if (client_sock != INVALID_SOCKET) {
		(void) closesocket(client_sock);
		client_sock = INVALID_SOCKET;
	}
}

/*
 *  Duktape callbacks
 */

/* Duktape debug transport callback: (possibly partial) read. */
duk_size_t duk_trans_socket_read_cb(void *udata, char *buffer, duk_size_t length) {
	int ret;

	(void) udata;  /* not needed by the example */

#if defined(DEBUG_PRINTS)
	fprintf(stderr, "%s: udata=%p, buffer=%p, length=%ld\n",
	        __FUNCTION__, (void *) udata, (void *) buffer, (long) length);
	fflush(stderr);
#endif

	if (client_sock == INVALID_SOCKET) {
		return 0;
	}

	if (length == 0) {
		/* This shouldn't happen. */
		fprintf(stderr, "%s: read request length == 0, closing connection\n",
		        __FILE__);
		fflush(stderr);
		goto fail;
	}

	if (buffer == NULL) {
		/* This shouldn't happen. */
		fprintf(stderr, "%s: read request buffer == NULL, closing connection\n",
		        __FILE__);
		fflush(stderr);
		goto fail;
	}

	/* In a production quality implementation there would be a sanity
	 * timeout here to recover from "black hole" disconnects.
	 */

	ret = recv(client_sock, buffer, (int) length, 0);
	if (ret < 0) {
		fprintf(stderr, "%s: debug read failed, error %d, closing connection\n",
		        __FILE__, ret);
		fflush(stderr);
		goto fail;
	} else if (ret == 0) {
		fprintf(stderr, "%s: debug read failed, ret == 0 (EOF), closing connection\n",
		        __FILE__);
		fflush(stderr);
		goto fail;
	} else if (ret > (int) length) {
		fprintf(stderr, "%s: debug read failed, ret too large (%ld > %ld), closing connection\n",
		        __FILE__, (long) ret, (long) length);
		fflush(stderr);
		goto fail;
	}

	return (duk_size_t) ret;

 fail:
	if (client_sock != INVALID_SOCKET) {
		(void) closesocket(client_sock);
		client_sock = INVALID_SOCKET;
	}
	return 0;
}

/* Duktape debug transport callback: (possibly partial) write. */
duk_size_t duk_trans_socket_write_cb(void *udata, const char *buffer, duk_size_t length) {
	int ret;

	(void) udata;  /* not needed by the example */

#if defined(DEBUG_PRINTS)
	fprintf(stderr, "%s: udata=%p, buffer=%p, length=%ld\n",
	        __FUNCTION__, (void *) udata, (const void *) buffer, (long) length);
	fflush(stderr);
#endif

	if (client_sock == INVALID_SOCKET) {
		return 0;
	}

	if (length == 0) {
		/* This shouldn't happen. */
		fprintf(stderr, "%s: write request length == 0, closing connection\n",
		        __FILE__);
		fflush(stderr);
		goto fail;
	}

	if (buffer == NULL) {
		/* This shouldn't happen. */
		fprintf(stderr, "%s: write request buffer == NULL, closing connection\n",
		        __FILE__);
		fflush(stderr);
		goto fail;
	}

	/* In a production quality implementation there would be a sanity
	 * timeout here to recover from "black hole" disconnects.
	 */

	ret = send(client_sock, buffer, (int) length, 0);
	if (ret <= 0 || ret > (int) length) {
		fprintf(stderr, "%s: debug write failed, ret %d, closing connection\n",
		        __FILE__, ret);
		fflush(stderr);
		goto fail;
	}

	return (duk_size_t) ret;

 fail:
	if (client_sock != INVALID_SOCKET) {
		(void) closesocket(INVALID_SOCKET);
		client_sock = INVALID_SOCKET;
	}
	return 0;
}

duk_size_t duk_trans_socket_peek_cb(void *udata) {
	u_long avail;
	int rc;

	(void) udata;  /* not needed by the example */

#if defined(DEBUG_PRINTS)
	fprintf(stderr, "%s: udata=%p\n", __FUNCTION__, (void *) udata);
	fflush(stderr);
#endif

	if (client_sock == INVALID_SOCKET) {
		return 0;
	}

	avail = 0;
	rc = ioctlsocket(client_sock, FIONREAD, &avail);
	if (rc != 0) {
		fprintf(stderr, "%s: ioctlsocket() returned %d, closing connection\n",
		        __FILE__, rc);
		fflush(stderr);
		goto fail;  /* also returns 0, which is correct */
	} else {
		if (avail == 0) {
			return 0;  /* nothing to read */
		} else {
			return 1;  /* something to read */
		}
	}
	/* never here */

 fail:
	if (client_sock != INVALID_SOCKET) {
		(void) closesocket(client_sock);
		client_sock = INVALID_SOCKET;
	}
	return 0;
}

void duk_trans_socket_read_flush_cb(void *udata) {
	(void) udata;  /* not needed by the example */

#if defined(DEBUG_PRINTS)
	fprintf(stderr, "%s: udata=%p\n", __FUNCTION__, (void *) udata);
	fflush(stderr);
#endif

	/* Read flush: Duktape may not be making any more read calls at this
	 * time.  If the transport maintains a receive window, it can use a
	 * read flush as a signal to update the window status to the remote
	 * peer.  A read flush is guaranteed to occur before Duktape stops
	 * reading for a while; it may occur in other situations as well so
	 * it's not a 100% reliable indication.
	 */

	/* This TCP transport requires no read flush handling so ignore.
	 * You can also pass a NULL to duk_debugger_attach() and not
	 * implement this callback at all.
	 */
}

void duk_trans_socket_write_flush_cb(void *udata) {
	(void) udata;  /* not needed by the example */

#if defined(DEBUG_PRINTS)
	fprintf(stderr, "%s: udata=%p\n", __FUNCTION__, (void *) udata);
	fflush(stderr);
#endif

	/* Write flush.  If the transport combines multiple writes
	 * before actually sending, a write flush is an indication
	 * to write out any pending bytes: Duktape may not be doing
	 * any more writes on this occasion.
	 */

	/* This TCP transport requires no write flush handling so ignore.
	 * You can also pass a NULL to duk_debugger_attach() and not
	 * implement this callback at all.
	 */
	return;
}

#undef DUK__STRINGIFY_HELPER
#undef DUK__STRINGIFY
