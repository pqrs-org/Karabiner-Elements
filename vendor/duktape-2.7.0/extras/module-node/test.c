#include <stdio.h>
#include <string.h>
#include "duktape.h"
#include "duk_module_node.h"

static duk_ret_t cb_resolve_module(duk_context *ctx) {
	const char *module_id;
	const char *parent_id;

	module_id = duk_require_string(ctx, 0);
	parent_id = duk_require_string(ctx, 1);

	duk_push_sprintf(ctx, "%s.js", module_id);
	printf("resolve_cb: id:'%s', parent-id:'%s', resolve-to:'%s'\n",
		module_id, parent_id, duk_get_string(ctx, -1));

	return 1;
}

static duk_ret_t cb_load_module(duk_context *ctx) {
	const char *filename;
	const char *module_id;

	module_id = duk_require_string(ctx, 0);
	duk_get_prop_string(ctx, 2, "filename");
	filename = duk_require_string(ctx, -1);

	printf("load_cb: id:'%s', filename:'%s'\n", module_id, filename);

	if (strcmp(module_id, "pig.js") == 0) {
		duk_push_sprintf(ctx, "module.exports = 'you\\'re about to get eaten by %s';",
			module_id);
	} else if (strcmp(module_id, "cow.js") == 0) {
		duk_push_string(ctx, "module.exports = require('pig');");
	} else if (strcmp(module_id, "ape.js") == 0) {
		duk_push_string(ctx, "module.exports = { module: module, __filename: __filename, wasLoaded: module.loaded };");
	} else if (strcmp(module_id, "badger.js") == 0) {
		duk_push_string(ctx, "exports.foo = 123; exports.bar = 234;");
	} else if (strcmp(module_id, "comment.js") == 0) {
		duk_push_string(ctx, "exports.foo = 123; exports.bar = 234; // comment");
	} else if (strcmp(module_id, "shebang.js") == 0) {
		duk_push_string(ctx, "#!ignored\nexports.foo = 123; exports.bar = 234;");
	} else {
		(void) duk_type_error(ctx, "cannot find module: %s", module_id);
	}

	return 1;
}

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

	duk_push_object(ctx);
	duk_push_c_function(ctx, cb_resolve_module, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "resolve");
	duk_push_c_function(ctx, cb_load_module, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "load");
	duk_module_node_init(ctx);
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
