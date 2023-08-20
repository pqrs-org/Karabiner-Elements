/*
 *  Create and throw an ECMAScript error object based on a code and a message.
 *
 *  Used when we throw errors internally.  ECMAScript generated error objects
 *  are created by ECMAScript code, and the throwing is handled by the bytecode
 *  executor.
 */

#include "duk_internal.h"

/*
 *  Create and throw an error (originating from Duktape internally)
 *
 *  Push an error object on top of the stack, possibly throw augmenting
 *  the error, and finally longjmp.
 *
 *  If an error occurs while we're dealing with the current error, we might
 *  enter an infinite recursion loop.  This is prevented by detecting a
 *  "double fault" through the heap->creating_error flag; the recursion
 *  then stops at the second level.
 */

#if defined(DUK_USE_VERBOSE_ERRORS)
DUK_INTERNAL void duk_err_create_and_throw(duk_hthread *thr,
                                           duk_errcode_t code,
                                           const char *msg,
                                           const char *filename,
                                           duk_int_t line) {
#else
DUK_INTERNAL void duk_err_create_and_throw(duk_hthread *thr, duk_errcode_t code) {
#endif
#if defined(DUK_USE_VERBOSE_ERRORS)
	DUK_DD(DUK_DDPRINT("duk_err_create_and_throw(): code=%ld, msg=%s, filename=%s, line=%ld",
	                   (long) code,
	                   (const char *) msg,
	                   (const char *) filename,
	                   (long) line));
#else
	DUK_DD(DUK_DDPRINT("duk_err_create_and_throw(): code=%ld", (long) code));
#endif

	DUK_ASSERT(thr != NULL);

	/* Even though nested call is possible because we throw an error when
	 * trying to create an error, the potential errors must happen before
	 * the longjmp state is configured.
	 */
	DUK_ASSERT_LJSTATE_UNSET(thr->heap);

	/* Sync so that augmentation sees up-to-date activations, NULL
	 * thr->ptr_curr_pc so that it's not used if side effects occur
	 * in augmentation or longjmp handling.
	 */
	duk_hthread_sync_and_null_currpc(thr);

	/*
	 *  Create and push an error object onto the top of stack.
	 *  The error is potentially augmented before throwing.
	 *
	 *  If a "double error" occurs, use a fixed error instance
	 *  to avoid further trouble.
	 */

	if (thr->heap->creating_error) {
		duk_tval tv_val;
		duk_hobject *h_err;

		thr->heap->creating_error = 0;

		h_err = thr->builtins[DUK_BIDX_DOUBLE_ERROR];
		if (h_err != NULL) {
			DUK_D(DUK_DPRINT("double fault detected -> use built-in fixed 'double error' instance"));
			DUK_TVAL_SET_OBJECT(&tv_val, h_err);
		} else {
			DUK_D(DUK_DPRINT("double fault detected; there is no built-in fixed 'double error' instance "
			                 "-> use the error code as a number"));
			DUK_TVAL_SET_I32(&tv_val, (duk_int32_t) code);
		}

		duk_err_setup_ljstate1(thr, DUK_LJ_TYPE_THROW, &tv_val);

		/* No augmentation to avoid any allocations or side effects. */
	} else {
		/* Prevent infinite recursion.  Extra call stack and C
		 * recursion headroom (see GH-191) is added for augmentation.
		 * That is now signalled by heap->augmenting error and taken
		 * into account in call handling without an explicit limit bump.
		 */
		thr->heap->creating_error = 1;

		duk_require_stack(thr, 1);

		/* XXX: usually unnecessary '%s' formatting here, but cannot
		 * use 'msg' as a format string directly.
		 */
#if defined(DUK_USE_VERBOSE_ERRORS)
		duk_push_error_object_raw(thr, code | DUK_ERRCODE_FLAG_NOBLAME_FILELINE, filename, line, "%s", (const char *) msg);
#else
		duk_push_error_object_raw(thr, code | DUK_ERRCODE_FLAG_NOBLAME_FILELINE, NULL, 0, NULL);
#endif

		/* Note that an alloc error may happen during error augmentation.
		 * This may happen both when the original error is an alloc error
		 * and when it's something else.  Because any error in augmentation
		 * must be handled correctly anyway, there's no special check for
		 * avoiding it for alloc errors (this differs from Duktape 1.x).
		 */
#if defined(DUK_USE_AUGMENT_ERROR_THROW)
		DUK_DDD(DUK_DDDPRINT("THROW ERROR (INTERNAL): %!iT (before throw augment)", (duk_tval *) duk_get_tval(thr, -1)));
		duk_err_augment_error_throw(thr);
#endif

		duk_err_setup_ljstate1(thr, DUK_LJ_TYPE_THROW, DUK_GET_TVAL_NEGIDX(thr, -1));
		thr->heap->creating_error = 0;

		/* Error is now created and we assume no errors can occur any
		 * more.  Check for debugger Throw integration only when the
		 * error is complete.  If we enter debugger message loop,
		 * creating_error must be 0 so that errors can be thrown in
		 * the paused state, e.g. in Eval commands.
		 */
#if defined(DUK_USE_DEBUGGER_SUPPORT)
		duk_err_check_debugger_integration(thr);
#endif
	}

	/*
	 *  Finally, longjmp
	 */

	DUK_DDD(DUK_DDDPRINT("THROW ERROR (INTERNAL): %!iT, %!iT (after throw augment)",
	                     (duk_tval *) &thr->heap->lj.value1,
	                     (duk_tval *) &thr->heap->lj.value2));

	duk_err_longjmp(thr);
	DUK_UNREACHABLE();
}

/*
 *  Helper for C function call negative return values.
 */

DUK_INTERNAL void duk_error_throw_from_negative_rc(duk_hthread *thr, duk_ret_t rc) {
	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(rc < 0);

	/*
	 *  The __FILE__ and __LINE__ information is intentionally not used in the
	 *  creation of the error object, as it isn't useful in the tracedata.  The
	 *  tracedata still contains the function which returned the negative return
	 *  code, and having the file/line of this function isn't very useful.
	 *
	 *  The error messages for DUK_RET_xxx shorthand are intentionally very
	 *  minimal: they're only really useful for low memory targets.
	 */

	duk_error_raw(thr, -rc, NULL, 0, "error (rc %ld)", (long) rc);
	DUK_WO_NORETURN(return;);
}
