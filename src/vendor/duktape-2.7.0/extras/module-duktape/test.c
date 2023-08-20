#include <stdio.h>
#include <string.h>
#include "duktape.h"
#include "duk_module_duktape.h"

static duk_ret_t handle_print(duk_context *ctx) {
	printf("%s\n", duk_safe_to_string(ctx, 0));
	return 0;
}

static duk_ret_t handle_assert(duk_context *ctx) {
	if (duk_to_boolean(ctx, 0)) {
		return 0;
	}
	(void) duk_generic_error(ctx, "assertion failed: %s", duk_safe_to_string(ctx, 1));
	return 0;
}


int main(int argc, char *argv[]) {
	duk_context *ctx;
	int i;
	int exitcode = 0;

	ctx = duk_create_heap_default();
	if (!ctx) {
		return 1;
	}

	duk_push_c_function(ctx, handle_print, 1);
	duk_put_global_string(ctx, "print");
	duk_push_c_function(ctx, handle_assert, 2);
	duk_put_global_string(ctx, "assert");

	duk_module_duktape_init(ctx);
	printf("top after init: %ld\n", (long) duk_get_top(ctx));

	for (i = 1; i < argc; i++) {
		printf("Evaling: %s\n", argv[i]);
		if (duk_peval_string(ctx, argv[i]) != 0) {
			if (duk_get_prop_string(ctx, -1, "stack")) {
				duk_replace(ctx, -2);
			} else {
				duk_pop(ctx);
			}
			exitcode = 1;
		}
		printf("--> %s\n", duk_safe_to_string(ctx, -1));
		duk_pop(ctx);
	}

	printf("Done\n");
	duk_destroy_heap(ctx);
	return exitcode;
}
