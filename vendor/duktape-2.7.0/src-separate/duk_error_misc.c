/*
 *  Error helpers
 */

#include "duk_internal.h"

/*
 *  Helper to walk the thread chain and see if there is an active error
 *  catcher.  Protected calls or finally blocks aren't considered catching.
 */

#if defined(DUK_USE_DEBUGGER_SUPPORT)
DUK_LOCAL duk_bool_t duk__have_active_catcher(duk_hthread *thr) {
	/* As noted above, a protected API call won't be counted as a
	 * catcher.  This is usually convenient, e.g. in the case of a top-
	 * level duk_pcall(), but may not always be desirable.  Perhaps add
	 * an argument to treat them as catchers?
	 */

	duk_activation *act;
	duk_catcher *cat;

	DUK_ASSERT(thr != NULL);

	for (; thr != NULL; thr = thr->resumer) {
		for (act = thr->callstack_curr; act != NULL; act = act->parent) {
			for (cat = act->cat; cat != NULL; cat = cat->parent) {
				if (DUK_CAT_HAS_CATCH_ENABLED(cat)) {
					return 1; /* all we need to know */
				}
			}
		}
	}
	return 0;
}
#endif /* DUK_USE_DEBUGGER_SUPPORT */

/*
 *  Get prototype object for an integer error code.
 */

DUK_INTERNAL duk_hobject *duk_error_prototype_from_code(duk_hthread *thr, duk_errcode_t code) {
	switch (code) {
	case DUK_ERR_EVAL_ERROR:
		return thr->builtins[DUK_BIDX_EVAL_ERROR_PROTOTYPE];
	case DUK_ERR_RANGE_ERROR:
		return thr->builtins[DUK_BIDX_RANGE_ERROR_PROTOTYPE];
	case DUK_ERR_REFERENCE_ERROR:
		return thr->builtins[DUK_BIDX_REFERENCE_ERROR_PROTOTYPE];
	case DUK_ERR_SYNTAX_ERROR:
		return thr->builtins[DUK_BIDX_SYNTAX_ERROR_PROTOTYPE];
	case DUK_ERR_TYPE_ERROR:
		return thr->builtins[DUK_BIDX_TYPE_ERROR_PROTOTYPE];
	case DUK_ERR_URI_ERROR:
		return thr->builtins[DUK_BIDX_URI_ERROR_PROTOTYPE];
	case DUK_ERR_ERROR:
	default:
		return thr->builtins[DUK_BIDX_ERROR_PROTOTYPE];
	}
}

/*
 *  Helper for debugger throw notify and pause-on-uncaught integration.
 */

#if defined(DUK_USE_DEBUGGER_SUPPORT)
DUK_INTERNAL void duk_err_check_debugger_integration(duk_hthread *thr) {
	duk_bool_t uncaught;
	duk_tval *tv_obj;

	/* If something is thrown with the debugger attached and nobody will
	 * catch it, execution is paused before the longjmp, turning over
	 * control to the debug client.  This allows local state to be examined
	 * before the stack is unwound.  Errors are not intercepted when debug
	 * message loop is active (e.g. for Eval).
	 */

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);

	/* XXX: Allow customizing the pause and notify behavior at runtime
	 * using debugger runtime flags.  For now the behavior is fixed using
	 * config options.
	 */

	if (!duk_debug_is_attached(thr->heap) || thr->heap->dbg_processing || thr->heap->lj.type != DUK_LJ_TYPE_THROW ||
	    thr->heap->creating_error) {
		DUK_D(DUK_DPRINT("skip debugger error integration; not attached, debugger processing, not THROW, or error thrown "
		                 "while creating error"));
		return;
	}

	/* Don't intercept a DoubleError, we may have caused the initial double
	 * fault and attempting to intercept it will cause us to be called
	 * recursively and exhaust the C stack.  (This should no longer happen
	 * for the initial throw because DoubleError path doesn't do a debugger
	 * integration check, but it might happen for rethrows.)
	 */
	tv_obj = &thr->heap->lj.value1;
	if (DUK_TVAL_IS_OBJECT(tv_obj) && DUK_TVAL_GET_OBJECT(tv_obj) == thr->builtins[DUK_BIDX_DOUBLE_ERROR]) {
		DUK_D(DUK_DPRINT("built-in DoubleError instance (re)thrown, not intercepting"));
		return;
	}

	uncaught = !duk__have_active_catcher(thr);

	/* Debugger code expects the value at stack top.  This also serves
	 * as a backup: we need to store/restore the longjmp state because
	 * when the debugger is paused Eval commands may be executed and
	 * they can arbitrarily clobber the longjmp state.
	 */
	duk_push_tval(thr, tv_obj);

	/* Store and reset longjmp state. */
	DUK_ASSERT_LJSTATE_SET(thr->heap);
	DUK_TVAL_DECREF_NORZ(thr, tv_obj);
	DUK_ASSERT(DUK_TVAL_IS_UNDEFINED(&thr->heap->lj.value2)); /* Always for THROW type. */
	DUK_TVAL_SET_UNDEFINED(tv_obj);
	thr->heap->lj.type = DUK_LJ_TYPE_UNKNOWN;
	DUK_ASSERT_LJSTATE_UNSET(thr->heap);

#if defined(DUK_USE_DEBUGGER_THROW_NOTIFY)
	/* Report it to the debug client */
	DUK_D(DUK_DPRINT("throw with debugger attached, report to client"));
	duk_debug_send_throw(thr, uncaught);
#endif

	if (uncaught) {
		if (thr->heap->dbg_pause_flags & DUK_PAUSE_FLAG_UNCAUGHT_ERROR) {
			DUK_D(DUK_DPRINT("PAUSE TRIGGERED by uncaught error"));
			duk_debug_halt_execution(thr, 1 /*use_prev_pc*/);
		}
	} else {
		if (thr->heap->dbg_pause_flags & DUK_PAUSE_FLAG_CAUGHT_ERROR) {
			DUK_D(DUK_DPRINT("PAUSE TRIGGERED by caught error"));
			duk_debug_halt_execution(thr, 1 /*use_prev_pc*/);
		}
	}

	/* Restore longjmp state. */
	DUK_ASSERT_LJSTATE_UNSET(thr->heap);
	thr->heap->lj.type = DUK_LJ_TYPE_THROW;
	tv_obj = DUK_GET_TVAL_NEGIDX(thr, -1);
	DUK_ASSERT(DUK_TVAL_IS_UNDEFINED(&thr->heap->lj.value1));
	DUK_ASSERT(DUK_TVAL_IS_UNDEFINED(&thr->heap->lj.value2));
	DUK_TVAL_SET_TVAL(&thr->heap->lj.value1, tv_obj);
	DUK_TVAL_INCREF(thr, tv_obj);
	DUK_ASSERT_LJSTATE_SET(thr->heap);

	duk_pop(thr);
}
#endif /* DUK_USE_DEBUGGER_SUPPORT */

/*
 *  Helpers for setting up heap longjmp state.
 */

DUK_INTERNAL void duk_err_setup_ljstate1(duk_hthread *thr, duk_small_uint_t lj_type, duk_tval *tv_val) {
	duk_heap *heap;

	DUK_ASSERT(thr != NULL);
	heap = thr->heap;
	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(tv_val != NULL);

	DUK_ASSERT_LJSTATE_UNSET(heap);

	heap->lj.type = lj_type;
	DUK_TVAL_SET_TVAL(&heap->lj.value1, tv_val);
	DUK_TVAL_INCREF(thr, tv_val);

	DUK_ASSERT_LJSTATE_SET(heap);
}
