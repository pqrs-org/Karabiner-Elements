/*
 *  Sandboxing example
 *
 *  Uses custom memory allocation functions which keep track of total amount
 *  of memory allocated, imposing a maximum total allocation size.
 */

#include <stdio.h>
#include <stdlib.h>
#include "duktape.h"

/*
 *  Memory allocator which backs to standard library memory functions but
 *  keeps a small header to track current allocation size.
 *
 *  Many other sandbox allocation models are useful, e.g. preallocated pools.
 */

typedef struct {
	/* The double value in the union is there to ensure alignment is
	 * good for IEEE doubles too.  In many 32-bit environments 4 bytes
	 * would be sufficiently aligned and the double value is unnecessary.
	 */
	union {
		size_t sz;
		double d;
	} u;
} alloc_hdr;

static size_t total_allocated = 0;
static size_t max_allocated = 256 * 1024;  /* 256kB sandbox */

static void sandbox_dump_memstate(void) {
#if 0
	fprintf(stderr, "Total allocated: %ld\n", (long) total_allocated);
	fflush(stderr);
#endif
}

static void *sandbox_alloc(void *udata, duk_size_t size) {
	alloc_hdr *hdr;

	(void) udata;  /* Suppress warning. */

	if (size == 0) {
		return NULL;
	}

	if (total_allocated + size > max_allocated) {
		fprintf(stderr, "Sandbox maximum allocation size reached, %ld requested in sandbox_alloc\n",
		        (long) size);
		fflush(stderr);
		return NULL;
	}

	hdr = (alloc_hdr *) malloc(size + sizeof(alloc_hdr));
	if (!hdr) {
		return NULL;
	}
	hdr->u.sz = size;
	total_allocated += size;
	sandbox_dump_memstate();
	return (void *) (hdr + 1);
}

static void *sandbox_realloc(void *udata, void *ptr, duk_size_t size) {
	alloc_hdr *hdr;
	size_t old_size;
	void *t;

	(void) udata;  /* Suppress warning. */

	/* Handle the ptr-NULL vs. size-zero cases explicitly to minimize
	 * platform assumptions.  You can get away with much less in specific
	 * well-behaving environments.
	 */

	if (ptr) {
		hdr = (alloc_hdr *) (((char *) ptr) - sizeof(alloc_hdr));
		old_size = hdr->u.sz;

		if (size == 0) {
			total_allocated -= old_size;
			free((void *) hdr);
			sandbox_dump_memstate();
			return NULL;
		} else {
			if (total_allocated - old_size + size > max_allocated) {
				fprintf(stderr, "Sandbox maximum allocation size reached, %ld requested in sandbox_realloc\n",
				        (long) size);
				fflush(stderr);
				return NULL;
			}

			t = realloc((void *) hdr, size + sizeof(alloc_hdr));
			if (!t) {
				return NULL;
			}
			hdr = (alloc_hdr *) t;
			total_allocated -= old_size;
			total_allocated += size;
			hdr->u.sz = size;
			sandbox_dump_memstate();
			return (void *) (hdr + 1);
		}
	} else {
		if (size == 0) {
			return NULL;
		} else {
			if (total_allocated + size > max_allocated) {
				fprintf(stderr, "Sandbox maximum allocation size reached, %ld requested in sandbox_realloc\n",
				        (long) size);
				fflush(stderr);
				return NULL;
			}

			hdr = (alloc_hdr *) malloc(size + sizeof(alloc_hdr));
			if (!hdr) {
				return NULL;
			}
			hdr->u.sz = size;
			total_allocated += size;
			sandbox_dump_memstate();
			return (void *) (hdr + 1);
		}
	}
}

static void sandbox_free(void *udata, void *ptr) {
	alloc_hdr *hdr;

	(void) udata;  /* Suppress warning. */

	if (!ptr) {
		return;
	}
	hdr = (alloc_hdr *) (((char *) ptr) - sizeof(alloc_hdr));
	total_allocated -= hdr->u.sz;
	free((void *) hdr);
	sandbox_dump_memstate();
}

/*
 *  Sandbox setup and test
 */

static duk_ret_t duk__print(duk_context *ctx) {
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, duk_get_top(ctx) - 1);
	printf("%s\n", duk_safe_to_string(ctx, -1));
	return 0;
}

static duk_ret_t do_sandbox_test(duk_context *ctx, void *udata) {
	FILE *f;
	char buf[4096];
	size_t ret;
	const char *globobj;

	(void) udata;

	/*
	 *  Setup sandbox
	 */

	/* Minimal print() provider. */
	duk_push_c_function(ctx, duk__print, DUK_VARARGS);
	duk_put_global_string(ctx, "print");

	globobj =
		"({\n"
		"    print: print,\n"
		"    Math: {\n"
		"        max: Math.max\n"
		"    }\n"
		"})\n";
#if 1
	fprintf(stderr, "Sandbox global object:\n----------------\n%s----------------\n", globobj);
	fflush(stderr);
#endif
	duk_eval_string(ctx, globobj);
	duk_set_global_object(ctx);

	/*
	 *  Execute code from specified file
	 */

	f = fopen(duk_require_string(ctx, -1), "rb");
	if (!f) {
		duk_error(ctx, DUK_ERR_ERROR, "failed to open file");
	}

	for (;;) {
		if (ferror(f)) {
			fclose(f);
			duk_error(ctx, DUK_ERR_ERROR, "ferror when reading file");
		}
		if (feof(f)) {
			break;
		}

		ret = fread(buf, 1, sizeof(buf), f);
		if (ret == 0) {
			break;
		}

		duk_push_lstring(ctx, (const char *) buf, ret);
	}

	duk_concat(ctx, duk_get_top(ctx) - 1);  /* -1 for filename */

	/* -> [ ... filename source ] */

	duk_insert(ctx, -2);

	/* -> [ ... source filename ] */

	duk_compile(ctx, 0 /*flags*/);  /* Compile as program */
	duk_call(ctx, 0 /*nargs*/);

	return 0;
}

/*
 *  Main
 */

static void sandbox_fatal(void *udata, const char *msg) {
	(void) udata;  /* Suppress warning. */
	fprintf(stderr, "FATAL: %s\n", (msg ? msg : "no message"));
	fflush(stderr);
	exit(1);  /* must not return */
}

int main(int argc, char *argv[]) {
	duk_context *ctx;
	duk_int_t rc;

	if (argc < 2) {
		fprintf(stderr, "Usage: sandbox <test.js>\n");
		fflush(stderr);
		exit(1);
	}

	ctx = duk_create_heap(sandbox_alloc,
	                      sandbox_realloc,
	                      sandbox_free,
	                      NULL,
	                      sandbox_fatal);

	duk_push_string(ctx, argv[1]);
	rc = duk_safe_call(ctx, do_sandbox_test, NULL, 1 /*nargs*/, 1 /*nrets*/);
	if (rc) {
		fprintf(stderr, "ERROR: %s\n", duk_safe_to_string(ctx, -1));
		fflush(stderr);
	}

	duk_destroy_heap(ctx);

	/* Should be zero. */
	fprintf(stderr, "Final allocation: %ld\n", (long) total_allocated);
	fflush(stderr);

	return 1;
}
