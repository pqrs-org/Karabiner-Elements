#include <stdio.h>
#include "duktape.h"
#include "duk_cbor.h"

#define FMT_CBOR 1
#define FMT_JSON 2
#define FMT_JX 3
#define FMT_JS 4

static int read_format = 0;
static int write_format = 0;
static int write_indent = 0;

static void push_stdin(duk_context *ctx, int to_string) {
	unsigned char *buf;
	size_t off;
	size_t len;
	size_t got;

	off = 0;
	len = 256;
	buf = (unsigned char *) duk_push_dynamic_buffer(ctx, len);

	for (;;) {
#if 0
		fprintf(stderr, "reading %ld of input\n", (long) (len - off));
#endif
		got = fread(buf + off, 1, len - off, stdin);
		if (got == 0U) {
			break;
		}
		off += got;
		if (len - off < 256U) {
			size_t newlen = len * 2U;
			buf = (unsigned char *) duk_resize_buffer(ctx, -1, newlen);
			len = newlen;
		}
	}
	buf = (unsigned char *) duk_resize_buffer(ctx, -1, off);

	if (to_string) {
		duk_push_lstring(ctx, (const char *) buf, off);
		duk_remove(ctx, -2);
	}
}

static void usage_and_exit(void) {
	fprintf(stderr, "Usage: jsoncbor -r cbor|json|jx|js -w cbor|json|jx [--indent N]\n"
	                "       jsoncbor -e               # shorthand for -r json -w cbor\n"
	                "       jsoncbor -d [--indent N]  # shorthand for -r cbor -w json [--indent N]\n"
	                "\n"
	                "       Input is read from stdin, output is written to stdout.\n"
	                "       'jx' is a Duktape custom JSON extension.\n"
	                "       'js' means evaluate input as an ECMAScript expression.\n");
	exit(1);
}

static duk_ret_t transcode_helper(duk_context *ctx, void *udata) {
	const unsigned char *buf;
	size_t len;
	duk_idx_t top;

	(void) udata;

	/* For Duktape->JSON conversion map all typed arrays into base-64.
	 * This is generally more useful than the default behavior.  However,
	 * the base-64 value doesn't have any kind of 'tag' to allow it to be
	 * parsed back into binary automatically.  Right now the CBOR parser
	 * creates plain fixed buffers from incoming binary strings.
	 */
	duk_eval_string_noresult(ctx,
		"(function () {\n"
		"    Object.getPrototypeOf(new Uint8Array(0)).toJSON = function () {\n"
		"        return Duktape.enc('base64', this);\n"
		"    };\n"
		"}())\n");

	top = duk_get_top(ctx);

	if (read_format == FMT_CBOR) {
		push_stdin(ctx, 0 /*to_string*/);
		duk_cbor_decode(ctx, -1, 0);
	} else if (read_format == FMT_JSON) {
		push_stdin(ctx, 1 /*to_string*/);
		duk_json_decode(ctx, -1);
	} else if (read_format == FMT_JX) {
		duk_eval_string(ctx, "(function(v){return Duktape.dec('jx',v)})");
		push_stdin(ctx, 1 /*to_string*/);
		duk_call(ctx, 1);
	} else if (read_format == FMT_JS) {
		push_stdin(ctx, 1 /*to_string*/);
		duk_eval(ctx);
	} else {
		(void) duk_type_error(ctx, "invalid read format");
	}

	if (duk_get_top(ctx) != top + 1) {
		fprintf(stderr, "top invalid after decoding: %d vs. %d\n", duk_get_top(ctx), top + 1);
		exit(1);
	}

	if (write_format == FMT_CBOR) {
		duk_cbor_encode(ctx, -1, 0);
	} else if (write_format == FMT_JSON) {
		duk_eval_string(ctx, "(function(v,i){return JSON.stringify(v,null,i)})");
		duk_insert(ctx, -2);
		duk_push_int(ctx, write_indent);
		duk_call(ctx, 2);
	} else if (write_format == FMT_JX) {
		duk_eval_string(ctx, "(function(v,i){return Duktape.enc('jx',v,null,i)})");
		duk_insert(ctx, -2);
		duk_push_int(ctx, write_indent);
		duk_call(ctx, 2);
	} else {
		(void) duk_type_error(ctx, "invalid write format");
	}

	if (duk_is_string(ctx, -1)) {
		buf = (const unsigned char *) duk_require_lstring(ctx, -1, &len);
		fwrite((const void *) buf, 1, len, stdout);
		fprintf(stdout, "\n");
	} else {
		buf = (const unsigned char *) duk_require_buffer_data(ctx, -1, &len);
		fwrite((const void *) buf, 1, len, stdout);
	}

	if (duk_get_top(ctx) != top + 1) {
		fprintf(stderr, "top invalid after encoding: %d vs. %d\n", duk_get_top(ctx), top + 1);
		exit(1);
	}

	return 0;
}

static int parse_format(const char *s) {
	if (strcmp(s, "cbor") == 0) {
		return FMT_CBOR;
	} else if (strcmp(s, "json") == 0) {
		return FMT_JSON;
	} else if (strcmp(s, "jx") == 0) {
		return FMT_JX;
	} else if (strcmp(s, "js") == 0) {
		return FMT_JS;
	} else {
		return 0;
	}
}

int main(int argc, char *argv[]) {
	duk_context *ctx;
	duk_int_t rc;
	int exitcode = 0;
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-e") == 0) {
			read_format = FMT_JSON;
			write_format = FMT_CBOR;
		} else if (strcmp(argv[i], "-d") == 0) {
			read_format = FMT_CBOR;
			write_format = FMT_JSON;
		} else if (strcmp(argv[i], "-r") == 0) {
			if (i + 1 >= argc) {
				goto usage;
			}
			read_format = parse_format(argv[i + 1]);
			i++;
		} else if (strcmp(argv[i], "-w") == 0) {
			if (i + 1 >= argc) {
				goto usage;
			}
			write_format = parse_format(argv[i + 1]);
			i++;
		} else if (strcmp(argv[i], "--indent") == 0) {
			if (i + 1 >= argc) {
				goto usage;
			}
			write_indent = atoi(argv[i + 1]);
			i++;
		} else {
			goto usage;
		}
	}

	if (read_format == 0 || write_format == 0) {
		goto usage;
	}

	ctx = duk_create_heap_default();
	if (!ctx) {
		return 1;
	}

	rc = duk_safe_call(ctx, transcode_helper, NULL, 0, 1);
	if (rc != 0) {
		fprintf(stderr, "%s\n", duk_safe_to_string(ctx, -1));
		exitcode = 1;
	}
	/* duk_pop(ctx): unnecessary */

	duk_destroy_heap(ctx);

	return exitcode;

 usage:
	usage_and_exit();
}
