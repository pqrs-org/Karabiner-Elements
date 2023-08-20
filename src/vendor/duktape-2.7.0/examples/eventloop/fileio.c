/*
 *  File I/O binding example.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "duktape.h"

/* Push file as a buffer. */
void fileio_push_file_buffer(duk_context *ctx, const char *filename) {
	FILE *f = NULL;
	long len;
	void *buf;
	size_t got;

	if (!filename) {
		goto error;
	}

	f = fopen(filename, "rb");
	if (!f) {
		goto error;
	}

	if (fseek(f, 0, SEEK_END) != 0) {
		goto error;
	}

	len = ftell(f);

	if (fseek(f, 0, SEEK_SET) != 0) {
		goto error;
	}

	buf = duk_push_fixed_buffer(ctx, (size_t) len);

	got = fread(buf, 1, len, f);
	if (got != (size_t) len) {
		duk_pop(ctx);
		goto error;
	}

	fclose(f);
	return;

 error:
	if (f) {
		fclose(f);
	}
	duk_push_undefined(ctx);
}

/* Push file as a string. */
void fileio_push_file_string(duk_context *ctx, const char *filename) {
	fileio_push_file_buffer(ctx, filename);
	if (duk_is_buffer_data(ctx, -1)) {
		duk_buffer_to_string(ctx, -1);
	}
}

static int fileio_readfile(duk_context *ctx) {
	const char *filename = duk_to_string(ctx, 0);
	fileio_push_file_buffer(ctx, filename);
	if (!duk_is_buffer_data(ctx, -1)) {
		return DUK_RET_ERROR;
	}
	return 1;
}

static duk_function_list_entry fileio_funcs[] = {
	{ "readfile", fileio_readfile, 1 },
	{ NULL, NULL, 0 }
};

void fileio_register(duk_context *ctx) {
	/* Set global 'FileIo'. */
	duk_push_global_object(ctx);
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, fileio_funcs);
	duk_put_prop_string(ctx, -2, "FileIo");
	duk_pop(ctx);
}
