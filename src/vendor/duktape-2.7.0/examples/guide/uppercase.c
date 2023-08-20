/* uppercase.c */
#include <stdio.h>
#include <stdlib.h>
#include "duktape.h"

static int dummy_upper_case(duk_context *ctx) {
    size_t sz;
    const char *val = duk_require_lstring(ctx, 0, &sz);
    size_t i;

    /* We're going to need 'sz' additional entries on the stack. */
    duk_require_stack(ctx, sz);

    for (i = 0; i < sz; i++) {
        char ch = val[i];
        if (ch >= 'a' && ch <= 'z') {
            ch = ch - 'a' + 'A';
        }
        duk_push_lstring(ctx, (const char *) &ch, 1);
    }

    duk_concat(ctx, sz);
    return 1;
}

int main(int argc, char *argv[]) {
    duk_context *ctx;

    if (argc < 2) { exit(1); }

    ctx = duk_create_heap_default();
    if (!ctx) { exit(1); }

    duk_push_c_function(ctx, dummy_upper_case, 1);
    duk_push_string(ctx, argv[1]);
    duk_call(ctx, 1);
    printf("%s -> %s\n", argv[1], duk_to_string(ctx, -1));
    duk_pop(ctx);

    duk_destroy_heap(ctx);
    return 0;
}
