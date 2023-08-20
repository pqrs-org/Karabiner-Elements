#include <stdio.h>
#include "duktape.h"
#include "duk_v1_compat.h"

static duk_ret_t my_print(duk_context *ctx) {
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, duk_get_top(ctx) - 1);
	printf("%s\n", duk_to_string(ctx, -1));
	fflush(stdout);
	return 0;
}

int main(int argc, char *argv[]) {
	duk_context *ctx;
	int i;
	duk_int_t rc;

	ctx = duk_create_heap_default();
	if (!ctx) {
		return 1;
	}

	/* Minimal print() provider. */
	duk_push_c_function(ctx, my_print, DUK_VARARGS);
	duk_put_global_string(ctx, "print");

	printf("top after init: %ld\n", (long) duk_get_top(ctx));

	for (i = 1; i < argc; i++) {
		printf("Evaling: %s\n", argv[i]);
		(void) duk_peval_string(ctx, argv[i]);
		printf("--> %s\n", duk_safe_to_string(ctx, -1));
		duk_pop(ctx);
	}

	/* Test duk_dump_context_{stdout,stderr}() */

	duk_push_string(ctx, "dump to stdout");
	duk_dump_context_stdout(ctx);
	duk_pop(ctx);

	duk_push_string(ctx, "dump to stderr");
	duk_dump_context_stderr(ctx);
	duk_pop(ctx);

	/* Test duk_eval_file() and related. */

	printf("top before duk_eval_file(): %ld\n", (long) duk_get_top(ctx));
	duk_eval_file(ctx, "test_eval1.js");
	printf("top after duk_eval_file(): %ld\n", (long) duk_get_top(ctx));
	printf(" --> %s\n", duk_safe_to_string(ctx, -1));
	duk_pop(ctx);

	printf("top before duk_eval_file_noresult(): %ld\n", (long) duk_get_top(ctx));
	duk_eval_file_noresult(ctx, "test_eval1.js");
	printf("top after duk_eval_file_noresult(): %ld\n", (long) duk_get_top(ctx));

	printf("top before duk_peval_file(): %ld\n", (long) duk_get_top(ctx));
	rc = duk_peval_file(ctx, "test_eval1.js");
	printf("top after duk_peval_file(): %ld\n", (long) duk_get_top(ctx));
	printf(" --> %ld, %s\n", (long) rc, duk_safe_to_string(ctx, -1));
	duk_pop(ctx);

	printf("top before duk_peval_file(): %ld\n", (long) duk_get_top(ctx));
	rc = duk_peval_file(ctx, "test_eval2.js");
	printf("top after duk_peval_file(): %ld\n", (long) duk_get_top(ctx));
	printf(" --> %ld, %s\n", (long) rc, duk_safe_to_string(ctx, -1));
	duk_pop(ctx);

	printf("top before duk_peval_file_noresult(): %ld\n", (long) duk_get_top(ctx));
	rc = duk_peval_file_noresult(ctx, "test_eval1.js");
	printf("top after duk_peval_file_noresult(): %ld\n", (long) duk_get_top(ctx));
	printf(" --> %ld\n", (long) rc);

	printf("top before duk_peval_file_noresult(): %ld\n", (long) duk_get_top(ctx));
	rc = duk_peval_file_noresult(ctx, "test_eval2.js");
	printf("top after duk_peval_file_noresult(): %ld\n", (long) duk_get_top(ctx));
	printf(" --> %ld\n", (long) rc);

	/* Test duk_compile_file() and related. */

	printf("top before duk_compile_file(): %ld\n", (long) duk_get_top(ctx));
	duk_compile_file(ctx, 0, "test_compile1.js");
	printf("top after duk_compile_file(): %ld\n", (long) duk_get_top(ctx));
	duk_call(ctx, 0);
	duk_pop(ctx);

	printf("top before duk_pcompile_file(): %ld\n", (long) duk_get_top(ctx));
	rc = duk_pcompile_file(ctx, 0, "test_compile1.js");
	printf("top after duk_pcompile_file(): %ld\n", (long) duk_get_top(ctx));
	printf(" --> %ld: %s\n", (long) rc, duk_safe_to_string(ctx, -1));
	duk_pop(ctx);

	printf("top before duk_pcompile_file(): %ld\n", (long) duk_get_top(ctx));
	rc = duk_pcompile_file(ctx, 0, "test_compile2.js");
	printf("top after duk_pcompile_file(): %ld\n", (long) duk_get_top(ctx));
	printf(" --> %ld: %s\n", (long) rc, duk_safe_to_string(ctx, -1));
	duk_pop(ctx);

	printf("Done\n");
	duk_destroy_heap(ctx);
	return 0;
}
