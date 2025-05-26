#include <stdio.h>
#include "duktape.h"
#include "duk_logging.h"

static duk_ret_t init_logging(duk_context *ctx, void *udata) {
	(void) udata;
	duk_logging_init(ctx, 0 /*flags*/);
	printf("top after init: %ld\n", (long) duk_get_top(ctx));

	/* C API test */
	duk_eval_string_noresult(ctx, "Duktape.Logger.clog.l = 0;");
	duk_log(ctx, DUK_LOG_TRACE, "c logger test: %d", 123);
	duk_log(ctx, DUK_LOG_DEBUG, "c logger test: %d", 123);
	duk_log(ctx, DUK_LOG_INFO, "c logger test: %d", 123);
	duk_log(ctx, DUK_LOG_WARN, "c logger test: %d", 123);
	duk_log(ctx, DUK_LOG_ERROR, "c logger test: %d", 123);
	duk_log(ctx, DUK_LOG_FATAL, "c logger test: %d", 123);
	duk_log(ctx, -1, "negative level: %s %d 0x%08lx", "arg string", -123, 0xdeadbeefUL);
	duk_log(ctx, 6, "level too large: %s %d 0x%08lx", "arg string", 123, 0x1234abcdUL);

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

	(void) duk_safe_call(ctx, init_logging, NULL, 0, 1);
	printf("logging init: %s\n", duk_safe_to_string(ctx, -1));
	duk_pop(ctx);

	for (i = 1; i < argc; i++) {
		printf("Evaling: %s\n", argv[i]);
		duk_push_string(ctx, argv[i]);
		duk_push_string(ctx, "evalCodeFileName");  /* for automatic logger name testing */
		if (duk_pcompile(ctx, DUK_COMPILE_EVAL) != 0) {
			exitcode = 1;
		} else {
			if (duk_pcall(ctx, 0) != 0) {
				exitcode = 1;
			}
		}
		printf("--> %s\n", duk_safe_to_string(ctx, -1));
		duk_pop(ctx);
	}

	printf("Done\n");
	duk_destroy_heap(ctx);
	return exitcode;
}
