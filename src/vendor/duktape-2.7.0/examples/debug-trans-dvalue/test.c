/*
 *  Example program using the dvalue debug transport.
 */

#include <stdio.h>
#include <stdlib.h>

#include "duktape.h"
#include "duk_trans_dvalue.h"

void my_cooperate(duk_trans_dvalue_ctx *ctx, int block) {
	static int first_blocked = 1;

	if (!block) {
		/* Duktape is not blocked; you can cooperate with e.g. a user
		 * interface here and send dvalues to Duktape, but don't block.
		 */
		return;
	}

	/* Duktape is blocked on a read and won't continue until debug
	 * command(s) are sent.
	 *
	 * Normally you'd enter your own event loop here, and process
	 * events until something needs to be sent to Duktape.  For
	 * example, the user might press a "Step over" button in the
	 * UI which would cause dvalues to be sent.  You can then
	 * return from this callback.
	 *
	 * The code below sends some example messages for testing the
	 * dvalue handling of the transport.
	 *
	 * If you create dvalues manually and send them using
	 * duk_trans_dvalue_send(), you must free the dvalues after
	 * the send call returns using duk_dvalue_free().
	 */

	if (first_blocked) {
		char *tmp;
		int i;

		/* First time Duktape becomes blocked, send DumpHeap which
		 * exercises a lot of parsing code.
		 *
		 * NOTE: Valgrind may complain about reading uninitialized
		 * bytes.  This is caused by the DumpHeap command writing out
		 * verbatim duk_tval values which are intentionally not
		 * always fully initialized for performance reasons.
		 */
		first_blocked = 0;

		fprintf(stderr, "Duktape is blocked, send DumpHeap\n");
		fflush(stderr);

		duk_trans_dvalue_send_req(ctx);
		duk_trans_dvalue_send_integer(ctx, 0x20);  /* DumpHeap */
		duk_trans_dvalue_send_eom(ctx);

		/* Also send a dummy TriggerStatus request with trailing dvalues
		 * ignored by Duktape; Duktape will parse the dvalues to be able to
		 * skip them, so that the dvalue encoding is exercised.
		 */

		tmp = malloc(100000);  /* long buffer, >= 65536 chars */
		for (i = 0; i < 100000; i++) {
			tmp[i] = (char) i;
		}
		duk_trans_dvalue_send_req(ctx);
		duk_trans_dvalue_send_integer(ctx, 0x11);  /* TriggerStatus */
		duk_trans_dvalue_send_string(ctx, "dummy");  /* short, <= 31 chars */
		duk_trans_dvalue_send_string(ctx, "123456789012345678901234567890foobar");  /* medium, >= 32 chars */
		duk_trans_dvalue_send_lstring(ctx, (const char *) tmp, 65535UL);
		duk_trans_dvalue_send_lstring(ctx, (const char *) tmp, 65536UL);
		duk_trans_dvalue_send_lstring(ctx, (const char *) tmp, 100000UL);
		duk_trans_dvalue_send_buffer(ctx, (const char *) tmp, 255U);
		duk_trans_dvalue_send_buffer(ctx, (const char *) tmp, 65535UL);
		duk_trans_dvalue_send_buffer(ctx, (const char *) tmp, 65536UL);
		duk_trans_dvalue_send_buffer(ctx, (const char *) tmp, 100000UL);
		duk_trans_dvalue_send_unused(ctx);
		duk_trans_dvalue_send_undefined(ctx);
		duk_trans_dvalue_send_null(ctx);
		duk_trans_dvalue_send_true(ctx);
		duk_trans_dvalue_send_false(ctx);
		duk_trans_dvalue_send_number(ctx, 123.456);
		duk_trans_dvalue_send_object(ctx, 12 /*classnum*/, (const char *) tmp, 8);  /* fake ptr len */
		duk_trans_dvalue_send_pointer(ctx, (const char *) tmp, 8);  /* fake ptr len */
		duk_trans_dvalue_send_lightfunc(ctx, 0xdabc /*lf_flags*/, (const char *) tmp, 8);  /* fake ptr len */
		duk_trans_dvalue_send_heapptr(ctx, (const char *) tmp, 8);  /* fake ptr len */

		duk_trans_dvalue_send_eom(ctx);
	}

	fprintf(stderr, "Duktape is blocked, send Eval and StepInto to resume execution\n");
	fflush(stderr);

	/* duk_trans_dvalue_send_req_cmd() sends a REQ dvalue followed by
	 * an integer dvalue (command) for convenience.
	 */

	duk_trans_dvalue_send_req_cmd(ctx, 0x1e);  /* 0x1e = Eval */
	duk_trans_dvalue_send_string(ctx, "evalMe");
	duk_trans_dvalue_send_eom(ctx);

	duk_trans_dvalue_send_req_cmd(ctx, 0x14);  /* 0x14 = StepOver */
	duk_trans_dvalue_send_eom(ctx);
}

void my_received(duk_trans_dvalue_ctx *ctx, duk_dvalue *dv) {
	char buf[DUK_DVALUE_TOSTRING_BUFLEN];
	(void) ctx;

	duk_dvalue_to_string(dv, buf);
	fprintf(stderr, "Received dvalue: %s\n", buf);
	fflush(stderr);

	/* Here a normal debug client would wait for dvalues until an EOM
	 * dvalue was received (which completes a debug message).  The
	 * debug message would then be handled, possibly causing UI changes
	 * and/or causing debug commands to be sent to Duktape.
	 *
	 * The callback is responsible for eventually freeing the dvalue.
	 * Here we free it immediately, but an actual client would probably
	 * gather dvalues into an array or linked list to handle when the
	 * debug message was complete.
	 */

	duk_dvalue_free(dv);
}

void my_handshake(duk_trans_dvalue_ctx *ctx, const char *line) {
	(void) ctx;

	/* The Duktape handshake line is given in 'line' (without LF).
	 * The 'line' argument can be accessed for the duration of the
	 * callback (read only).  Don't free 'line' here, the transport
	 * handles that.
	 */

	fprintf(stderr, "Received handshake line: '%s'\n", line);
	fflush(stderr);
}

void my_detached(duk_trans_dvalue_ctx *ctx) {
	(void) ctx;

	/* Detached call forwarded as is. */

	fprintf(stderr, "Debug transport detached\n");
	fflush(stderr);
}

static duk_ret_t native_print(duk_context *ctx) {
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, duk_get_top(ctx) - 1);
	printf("%s\n", duk_to_string(ctx, -1));
	return 0;
}

int main(int argc, char *argv[]) {
	duk_context *ctx;
	duk_trans_dvalue_ctx *trans_ctx;
	int exitval = 0;

	(void) argc; (void) argv;  /* suppress warning */

	ctx = duk_create_heap_default();
	if (!ctx) {
		fprintf(stderr, "Failed to create Duktape heap\n");
		fflush(stderr);
		exitval = 1;
		goto cleanup;
	}

	duk_push_c_function(ctx, native_print, DUK_VARARGS);
	duk_put_global_string(ctx, "print");

	trans_ctx = duk_trans_dvalue_init();
	if (!trans_ctx) {
		fprintf(stderr, "Failed to create debug transport context\n");
		fflush(stderr);
		exitval = 1;
		goto cleanup;
	}
	trans_ctx->cooperate = my_cooperate;
	trans_ctx->received = my_received;
	trans_ctx->handshake = my_handshake;
	trans_ctx->detached = my_detached;

	/* Attach debugger; this will fail with a fatal error here unless
	 * debugger support is compiled in.  To fail more gracefully, call
	 * this under a duk_safe_call() to catch the error.
	 */
	duk_debugger_attach(ctx,
	                    duk_trans_dvalue_read_cb,
	                    duk_trans_dvalue_write_cb,
	                    duk_trans_dvalue_peek_cb,
	                    duk_trans_dvalue_read_flush_cb,
	                    duk_trans_dvalue_write_flush_cb,
	                    NULL,  /* app request cb */
	                    duk_trans_dvalue_detached_cb,
	                    (void *) trans_ctx);

	fprintf(stderr, "Debugger attached, running eval\n");
	fflush(stderr);

	/* Evaluate simple test code, callbacks will "step over" until end.
	 *
	 * The test code here is just for exercising the debug transport.
	 * The 'evalMe' variable is evaluated (using debugger command Eval)
	 * before every step to force different dvalues to be carried over
	 * the transport.
	 */

	duk_eval_string(ctx,
		"var evalMe;\n"
		"\n"
		"print('Hello world!');\n"
		"[ undefined, null, true, false, 123, -123, 123.1, 0, -0, 1/0, 0/0, -1/0, \n"
		"  'foo', Uint8Array.allocPlain('bar'), Duktape.Pointer('dummy'), Math.cos, \n"
		"].forEach(function (val) {\n"
		"    print(val);\n"
		"    evalMe = val;\n"
		"});\n"
		"\n"
		"var str = 'xxx'\n"
		"for (i = 0; i < 10; i++) {\n"
		"    print(i, str);\n"
		"    evalMe = str;\n"
		"    evalMe = Uint8Array.allocPlain(str);\n"
		"    str = str + str;\n"
		"}\n"
	);
	duk_pop(ctx);

	duk_debugger_detach(ctx);

 cleanup:
	if (trans_ctx) {
		duk_trans_dvalue_free(trans_ctx);
		trans_ctx = NULL;
	}
	if (ctx) {
		duk_destroy_heap(ctx);
	}

	return exitval;
}
