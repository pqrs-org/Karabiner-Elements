/*
 *  Main for evloop command line tool.
 *
 *  Runs a given script from file or stdin inside an eventloop.  The
 *  script can then access setTimeout() etc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(NO_SIGNAL)
#include <signal.h>
#endif

#include "duktape.h"

extern void poll_register(duk_context *ctx);
extern void socket_register(duk_context *ctx);
extern void fileio_register(duk_context *ctx);
extern void fileio_push_file_buffer(duk_context *ctx, const char *filename);
extern void fileio_push_file_string(duk_context *ctx, const char *filename);
extern void eventloop_register(duk_context *ctx);
extern duk_ret_t eventloop_run(duk_context *ctx, void *udata);

static int c_evloop = 0;

#if !defined(NO_SIGNAL)
static void my_sighandler(int x) {
	fprintf(stderr, "Got signal %d\n", x);
	fflush(stderr);
}
static void set_sigint_handler(void) {
	(void) signal(SIGINT, my_sighandler);
}
#endif  /* NO_SIGNAL */

/* Print error to stderr and pop error. */
static void print_error(duk_context *ctx, FILE *f) {
	if (duk_is_object(ctx, -1) && duk_has_prop_string(ctx, -1, "stack")) {
		/* XXX: print error objects specially */
		/* XXX: pcall the string coercion */
		duk_get_prop_string(ctx, -1, "stack");
		if (duk_is_string(ctx, -1)) {
			fprintf(f, "%s\n", duk_get_string(ctx, -1));
			fflush(f);
			duk_pop_2(ctx);
			return;
		} else {
			duk_pop(ctx);
		}
	}
	duk_to_string(ctx, -1);
	fprintf(f, "%s\n", duk_get_string(ctx, -1));
	fflush(f);
	duk_pop(ctx);
}

static duk_ret_t native_print(duk_context *ctx) {
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, duk_get_top(ctx) - 1);
	printf("%s\n", duk_safe_to_string(ctx, -1));
	return 0;
}

static void print_register(duk_context *ctx) {
	duk_push_c_function(ctx, native_print, DUK_VARARGS);
	duk_put_global_string(ctx, "print");
}

static duk_ret_t wrapped_compile_execute(duk_context *ctx, void *udata) {
	int comp_flags = 0;
	int rc;

	(void) udata;

	/* Compile input and place it into global _USERCODE */
	duk_compile(ctx, comp_flags);
	duk_push_global_object(ctx);
	duk_insert(ctx, -2);  /* [ ... global func ] */
	duk_put_prop_string(ctx, -2, "_USERCODE");
	duk_pop(ctx);
#if 0
	printf("compiled usercode\n");
#endif

	/* Start a zero timer which will call _USERCODE from within
	 * the event loop.
	 */
	fprintf(stderr, "set _USERCODE timer\n");
	fflush(stderr);
	duk_eval_string(ctx, "setTimeout(function() { _USERCODE(); }, 0);");
	duk_pop(ctx);

	/* Finally, launch eventloop.  This call only returns after the
	 * eventloop terminates.
	 */
	if (c_evloop) {
		fprintf(stderr, "calling eventloop_run()\n");
		fflush(stderr);
		rc = duk_safe_call(ctx, eventloop_run, NULL, 0 /*nargs*/, 1 /*nrets*/);
		if (rc != 0) {
			fprintf(stderr, "eventloop_run() failed: %s\n", duk_to_string(ctx, -1));
			fflush(stderr);
		}
		duk_pop(ctx);
	} else {
		fprintf(stderr, "calling EventLoop.run()\n");
		fflush(stderr);
		duk_eval_string(ctx, "EventLoop.run();");
		duk_pop(ctx);
	}

	return 0;
}

static int handle_fh(duk_context *ctx, FILE *f, const char *filename) {
	char *buf = NULL;
	int len;
	int got;
	int rc;
	int retval = -1;

	if (fseek(f, 0, SEEK_END) < 0) {
		goto error;
	}
	len = (int) ftell(f);
	if (fseek(f, 0, SEEK_SET) < 0) {
		goto error;
	}
	buf = (char *) malloc(len);
	if (!buf) {
		goto error;
	}

	got = fread((void *) buf, (size_t) 1, (size_t) len, f);

	duk_push_lstring(ctx, buf, got);
	duk_push_string(ctx, filename);

	free(buf);
	buf = NULL;

	rc = duk_safe_call(ctx, wrapped_compile_execute, NULL, 2 /*nargs*/, 1 /*nret*/);
	if (rc != DUK_EXEC_SUCCESS) {
		print_error(ctx, stderr);
		goto error;
	} else {
		duk_pop(ctx);
		retval = 0;
	}
	/* fall thru */

 error:
	if (buf) {
		free(buf);
	}
	return retval;
}

static int handle_file(duk_context *ctx, const char *filename) {
	FILE *f = NULL;
	int retval;

	f = fopen(filename, "rb");
	if (!f) {
		fprintf(stderr, "failed to open source file: %s\n", filename);
		fflush(stderr);
		goto error;
	}

	retval = handle_fh(ctx, f, filename);

	fclose(f);
	return retval;

 error:
	return -1;
}

static int handle_stdin(duk_context *ctx) {
	int retval;

	retval = handle_fh(ctx, stdin, "stdin");

	return retval;
}

static duk_ret_t init_duktape_context(duk_context *ctx, void *udata) {
	(void) udata;

	print_register(ctx);
	poll_register(ctx);
	socket_register(ctx);
	fileio_register(ctx);

	if (c_evloop) {
		fprintf(stderr, "Using C based eventloop (omit -c to use ECMAScript based eventloop)\n");
		fflush(stderr);

		eventloop_register(ctx);
		fileio_push_file_string(ctx, "c_eventloop.js");
		duk_eval(ctx);
	} else {
		fprintf(stderr, "Using ECMAScript based eventloop (give -c to use C based eventloop)\n");
		fflush(stderr);

		fileio_push_file_string(ctx, "ecma_eventloop.js");
		duk_eval(ctx);
	}

	return 0;
}

int main(int argc, char *argv[]) {
	duk_context *ctx = NULL;
	int retval = 1;
	const char *filename = NULL;
	int i;

#if !defined(NO_SIGNAL)
	set_sigint_handler();

	/* This is useful at the global level; libraries should avoid SIGPIPE though */
	/*signal(SIGPIPE, SIG_IGN);*/
#endif

	for (i = 1; i < argc; i++) {
		char *arg = argv[i];
		if (!arg) {
			goto usage;
		}
		if (strcmp(arg, "-c") == 0) {
			c_evloop = 1;
		} else if (strlen(arg) > 1 && arg[0] == '-') {
			goto usage;
		} else {
			if (filename) {
				goto usage;
			}
			filename = arg;
		}
	}
	if (!filename) {
		goto usage;
	}

	ctx = duk_create_heap_default();

	if (duk_safe_call(ctx, init_duktape_context, NULL, 0 /*nargs*/, 1 /*nrets*/) != 0) {
		fprintf(stderr, "Failed to initialize: %s\n", duk_safe_to_string(ctx, -1));
		goto cleanup;
	}
	duk_pop(ctx);

	fprintf(stderr, "Executing code from: '%s'\n", filename);
	fflush(stderr);

	if (strcmp(filename, "-") == 0) {
		if (handle_stdin(ctx) != 0) {
			goto cleanup;
		}
	} else {
		if (handle_file(ctx, filename) != 0) {
			goto cleanup;
		}
	}

	retval = 0;
	/* fall through */
 cleanup:
	if (ctx) {
		duk_destroy_heap(ctx);
	}

	return retval;

 usage:
	fprintf(stderr, "Usage: evloop [-c] <filename>\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Uses an ECMAScript based eventloop (ecma_eventloop.js) by default.\n");
	fprintf(stderr, "If -c option given, uses a C based eventloop (c_eventloop.{c,js}).\n");
	fprintf(stderr, "If <filename> is '-', the entire STDIN executed.\n");
	fflush(stderr);
	exit(1);
}
