#include <stdio.h>
#include <stdlib.h>
#include "duktape.h"
#include "duk_alloc_pool.h"

void my_fatal(const char *msg) {
	fprintf(stderr, "*** FATAL: %s\n", msg ? msg : "no message");
	fflush(stderr);
	abort();
}

static duk_ret_t my_print(duk_context *ctx) {
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, duk_get_top(ctx) - 1);
	printf("%s\n", duk_safe_to_string(ctx, -1));
	return 1;
}

static void dump_pool_state(duk_pool_global *g) {
	int i;
	long total_size = 0;
	long total_used = 0;

	for (i = 0; i < g->num_pools; i++) {
		duk_pool_state *st = g->states + i;
		int free, used;
		duk_pool_free *f;

		for (free = 0, f = st->first; f; f = f->next) {
			free++;
		}
		used = st->count - free;
		printf("Pool %2d: block size %5d, count %4d/%4d, bytes %6d/%6d\n",
		       i, (int) st->size, used, (int) st->count,
		       (int) st->size * used, (int) st->size * (int) st->count);

		total_size += (long) st->size * (long) st->count;
		total_used += (long) st->size * (long) used;
	}
	printf("=== Total: %ld/%ld, free %ld\n",
	       (long) total_used, (long) total_size, (long) (total_size - total_used));
}

int main(int argc, char *argv[]) {
	duk_context *ctx;
	int i;
	int exitcode = 0;

	/* NOTE! This pool configuration is NOT a good pool configuration
	 * for practical use (and is not intended to be one).  A production
	 * pool configuration should be created using measurements.
	 */
	const duk_pool_config pool_configs[15] = {
		{ 16, 20, 200 },
		{ 20, 40, 100 },
		{ 24, 40, 100 },
		{ 32, 60, 50 },
		{ 40, 60, 50 },
		{ 48, 60, 50 },
		{ 56, 60, 50 },
		{ 64, 60, 50 },
		{ 80, 60, 50 },
		{ 256, 100, 10 },
		{ 1024, 20, 2 },
		{ 2048, 20, 2 },
		{ 4096, 100, 2 },
		{ 6144, 60, 2 },
		{ 8192, 100, 2 },
	};
	duk_pool_state pool_states[15];  /* Count must match pool_configs[]. */
	duk_pool_global pool_global;

	char buffer[200000];
	void *pool_udata;

	pool_udata = duk_alloc_pool_init(buffer, sizeof(buffer), pool_configs, pool_states, sizeof(pool_configs) / sizeof(duk_pool_config), &pool_global);
	if (!pool_udata) {
		return 1;
	}

	printf("Pool after pool init:\n");
	dump_pool_state(&pool_global);

	ctx = duk_create_heap(duk_alloc_pool, duk_realloc_pool, duk_free_pool, pool_udata, NULL);
	if (!ctx) {
		return 1;
	}

	printf("Pool after Duktape heap creation:\n");
	dump_pool_state(&pool_global);

	duk_push_c_function(ctx, my_print, DUK_VARARGS);
	duk_put_global_string(ctx, "print");
	duk_push_c_function(ctx, my_print, DUK_VARARGS);
	duk_put_global_string(ctx, "alert");
	printf("top after init: %ld\n", (long) duk_get_top(ctx));

	for (i = 1; i < argc; i++) {
		printf("Evaling: %s\n", argv[i]);
		if (duk_peval_string(ctx, argv[i]) != 0) {
			exitcode = 1;
		}
		printf("--> %s\n", duk_safe_to_string(ctx, -1));
		duk_pop(ctx);
	}

	printf("Pool after evaling code:\n");
	dump_pool_state(&pool_global);

	printf("Done\n");
	duk_destroy_heap(ctx);
	return exitcode;
}
