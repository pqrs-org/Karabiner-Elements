#include <stdio.h>
#include "duktape.h"
#include "duk_print_alert.h"

int main(int argc, char *argv[]) {
	duk_context *ctx;
	int i;
	int exitcode = 0;

	ctx = duk_create_heap_default();
	if (!ctx) {
		return 1;
	}

	duk_print_alert_init(ctx, 0 /*flags*/);
	printf("top after init: %ld\n", (long) duk_get_top(ctx));

	for (i = 1; i < argc; i++) {
		printf("Evaling: %s\n", argv[i]);
		if (duk_peval_string(ctx, argv[i]) != 0) {
			exitcode = 1;
		}
		printf("--> %s\n", duk_safe_to_string(ctx, -1));
		duk_pop(ctx);
	}

	printf("Done\n");
	duk_destroy_heap(ctx);
	return exitcode;
}
