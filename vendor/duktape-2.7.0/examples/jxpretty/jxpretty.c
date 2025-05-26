/*
 *  Pretty print JSON from stdin into indented JX.
 */

#include <stdio.h>
#include <stdlib.h>
#include "duktape.h"

static duk_ret_t duk__print(duk_context *ctx) {
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, duk_get_top(ctx) - 1);
	printf("%s\n", duk_safe_to_string(ctx, -1));
	return 0;
}

static duk_ret_t do_jxpretty(duk_context *ctx, void *udata) {
	FILE *f = stdin;
	char buf[4096];
	size_t ret;

	(void) udata;

	for (;;) {
		if (ferror(f)) {
			(void) duk_error(ctx, DUK_ERR_ERROR, "ferror() on stdin");
		}
		if (feof(f)) {
			break;
		}

		ret = fread(buf, 1, sizeof(buf), f);
#if 0
		fprintf(stderr, "Read: %ld\n", (long) ret);
		fflush(stderr);
#endif
		if (ret == 0) {
			break;
		}

		duk_require_stack(ctx, 1);
		duk_push_lstring(ctx, (const char *) buf, ret);
	}

	duk_concat(ctx, duk_get_top(ctx));

	duk_eval_string(ctx, "(function (v) { print(Duktape.enc('jx', JSON.parse(v), null, 4)); })");
	duk_insert(ctx, -2);
	duk_call(ctx, 1);

	return 0;
}

int main(int argc, char *argv[]) {
	duk_context *ctx;
	duk_int_t rc;

	/* suppress warnings */
	(void) argc;
	(void) argv;

	ctx = duk_create_heap_default();

	duk_push_c_function(ctx, duk__print, DUK_VARARGS);
	duk_put_global_string(ctx, "print");

	rc = duk_safe_call(ctx, do_jxpretty, NULL, 0 /*nargs*/, 1 /*nrets*/);
	if (rc) {
		fprintf(stderr, "ERROR: %s\n", duk_safe_to_string(ctx, -1));
		fflush(stderr);
	}

	duk_destroy_heap(ctx);

	return 0;
}
