/*
 *  Thread builtins
 */

#include "duk_internal.h"

/*
 *  Constructor
 */

#if defined(DUK_USE_COROUTINE_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_thread_constructor(duk_hthread *thr) {
	duk_hthread *new_thr;
	duk_hobject *func;

	/* Check that the argument is callable; this is not 100% because we
	 * don't allow native functions to be a thread's initial function.
	 * Resume will reject such functions in any case.
	 */
	/* XXX: need a duk_require_func_promote_lfunc() */
	func = duk_require_hobject_promote_lfunc(thr, 0);
	DUK_ASSERT(func != NULL);
	duk_require_callable(thr, 0);

	duk_push_thread(thr);
	new_thr = (duk_hthread *) duk_known_hobject(thr, -1);
	new_thr->state = DUK_HTHREAD_STATE_INACTIVE;

	/* push initial function call to new thread stack; this is
	 * picked up by resume().
	 */
	duk_push_hobject(new_thr, func);

	return 1; /* return thread */
}
#endif

/*
 *  Resume a thread.
 *
 *  The thread must be in resumable state, either (a) new thread which hasn't
 *  yet started, or (b) a thread which has previously yielded.  This method
 *  must be called from an ECMAScript function.
 *
 *  Args:
 *    - thread
 *    - value
 *    - isError (defaults to false)
 *
 *  Note: yield and resume handling is currently asymmetric.
 */

#if defined(DUK_USE_COROUTINE_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_thread_resume(duk_hthread *ctx) {
	duk_hthread *thr = (duk_hthread *) ctx;
	duk_hthread *thr_resume;
	duk_hobject *caller_func;
	duk_small_uint_t is_error;

	DUK_DDD(DUK_DDDPRINT("Duktape.Thread.resume(): thread=%!T, value=%!T, is_error=%!T",
	                     (duk_tval *) duk_get_tval(thr, 0),
	                     (duk_tval *) duk_get_tval(thr, 1),
	                     (duk_tval *) duk_get_tval(thr, 2)));

	DUK_ASSERT(thr->state == DUK_HTHREAD_STATE_RUNNING);
	DUK_ASSERT(thr->heap->curr_thread == thr);

	thr_resume = duk_require_hthread(thr, 0);
	DUK_ASSERT(duk_get_top(thr) == 3);
	is_error = (duk_small_uint_t) duk_to_boolean_top_pop(thr);
	DUK_ASSERT(duk_get_top(thr) == 2);

	/* [ thread value ] */

	/*
	 *  Thread state and calling context checks
	 */

	if (thr->callstack_top < 2) {
		DUK_DD(DUK_DDPRINT(
		    "resume state invalid: callstack should contain at least 2 entries (caller and Duktape.Thread.resume)"));
		goto state_error;
	}
	DUK_ASSERT(thr->callstack_curr != NULL);
	DUK_ASSERT(thr->callstack_curr->parent != NULL);
	DUK_ASSERT(DUK_ACT_GET_FUNC(thr->callstack_curr) != NULL); /* us */
	DUK_ASSERT(DUK_HOBJECT_IS_NATFUNC(DUK_ACT_GET_FUNC(thr->callstack_curr)));
	DUK_ASSERT(DUK_ACT_GET_FUNC(thr->callstack_curr->parent) != NULL); /* caller */

	caller_func = DUK_ACT_GET_FUNC(thr->callstack_curr->parent);
	if (!DUK_HOBJECT_IS_COMPFUNC(caller_func)) {
		DUK_DD(DUK_DDPRINT("resume state invalid: caller must be ECMAScript code"));
		goto state_error;
	}

	/* Note: there is no requirement that: 'thr->callstack_preventcount == 1'
	 * like for yield.
	 */

	if (thr_resume->state != DUK_HTHREAD_STATE_INACTIVE && thr_resume->state != DUK_HTHREAD_STATE_YIELDED) {
		DUK_DD(DUK_DDPRINT("resume state invalid: target thread must be INACTIVE or YIELDED"));
		goto state_error;
	}

	DUK_ASSERT(thr_resume->state == DUK_HTHREAD_STATE_INACTIVE || thr_resume->state == DUK_HTHREAD_STATE_YIELDED);

	/* Further state-dependent pre-checks */

	if (thr_resume->state == DUK_HTHREAD_STATE_YIELDED) {
		/* no pre-checks now, assume a previous yield() has left things in
		 * tip-top shape (longjmp handler will assert for these).
		 */
	} else {
		duk_hobject *h_fun;

		DUK_ASSERT(thr_resume->state == DUK_HTHREAD_STATE_INACTIVE);

		/* The initial function must be an ECMAScript function (but
		 * can be bound).  We must make sure of that before we longjmp
		 * because an error in the RESUME handler call processing will
		 * not be handled very cleanly.
		 */
		if ((thr_resume->callstack_top != 0) || (thr_resume->valstack_top - thr_resume->valstack != 1)) {
			goto state_error;
		}

		duk_push_tval(thr, DUK_GET_TVAL_NEGIDX(thr_resume, -1));
		duk_resolve_nonbound_function(thr);
		h_fun = duk_require_hobject(thr, -1); /* reject lightfuncs on purpose */
		if (!DUK_HOBJECT_IS_CALLABLE(h_fun) || !DUK_HOBJECT_IS_COMPFUNC(h_fun)) {
			goto state_error;
		}
		duk_pop(thr);
	}

#if 0
	/* This check would prevent a heap destruction time finalizer from
	 * launching a coroutine, which would ensure that during finalization
	 * 'thr' would always equal heap_thread.  Normal runtime finalizers
	 * run with ms_running == 0, i.e. outside mark-and-sweep.  See GH-2030.
	 */
	if (thr->heap->ms_running) {
		DUK_D(DUK_DPRINT("refuse Duktape.Thread.resume() when ms_running != 0"));
		goto state_error;
	}
#endif

	/*
	 *  The error object has been augmented with a traceback and other
	 *  info from its creation point -- usually another thread.  The
	 *  error handler is called here right before throwing, but it also
	 *  runs in the resumer's thread.  It might be nice to get a traceback
	 *  from the resumee but this is not the case now.
	 */

#if defined(DUK_USE_AUGMENT_ERROR_THROW)
	if (is_error) {
		DUK_ASSERT_TOP(thr, 2); /* value (error) is at stack top */
		duk_err_augment_error_throw(thr); /* in resumer's context */
	}
#endif

#if defined(DUK_USE_DEBUG)
	if (is_error) {
		DUK_DDD(DUK_DDDPRINT("RESUME ERROR: thread=%!T, value=%!T",
		                     (duk_tval *) duk_get_tval(thr, 0),
		                     (duk_tval *) duk_get_tval(thr, 1)));
	} else if (thr_resume->state == DUK_HTHREAD_STATE_YIELDED) {
		DUK_DDD(DUK_DDDPRINT("RESUME NORMAL: thread=%!T, value=%!T",
		                     (duk_tval *) duk_get_tval(thr, 0),
		                     (duk_tval *) duk_get_tval(thr, 1)));
	} else {
		DUK_DDD(DUK_DDDPRINT("RESUME INITIAL: thread=%!T, value=%!T",
		                     (duk_tval *) duk_get_tval(thr, 0),
		                     (duk_tval *) duk_get_tval(thr, 1)));
	}
#endif

	thr->heap->lj.type = DUK_LJ_TYPE_RESUME;

	/* lj value2: thread */
	DUK_ASSERT(thr->valstack_bottom < thr->valstack_top);
	DUK_TVAL_SET_TVAL_UPDREF(thr, &thr->heap->lj.value2, &thr->valstack_bottom[0]); /* side effects */

	/* lj value1: value */
	DUK_ASSERT(thr->valstack_bottom + 1 < thr->valstack_top);
	DUK_TVAL_SET_TVAL_UPDREF(thr, &thr->heap->lj.value1, &thr->valstack_bottom[1]); /* side effects */
	DUK_TVAL_CHKFAST_INPLACE_SLOW(&thr->heap->lj.value1);

	thr->heap->lj.iserror = is_error;

	DUK_ASSERT(thr->heap->lj.jmpbuf_ptr != NULL); /* call is from executor, so we know we have a jmpbuf */
	duk_err_longjmp(thr); /* execution resumes in bytecode executor */
	DUK_UNREACHABLE();
	/* Never here, fall through to error (from compiler point of view). */

state_error:
	DUK_DCERROR_TYPE_INVALID_STATE(thr);
}
#endif

/*
 *  Yield the current thread.
 *
 *  The thread must be in yieldable state: it must have a resumer, and there
 *  must not be any yield-preventing calls (native calls and constructor calls,
 *  currently) in the thread's call stack (otherwise a resume would not be
 *  possible later).  This method must be called from an ECMAScript function.
 *
 *  Args:
 *    - value
 *    - isError (defaults to false)
 *
 *  Note: yield and resume handling is currently asymmetric.
 */

#if defined(DUK_USE_COROUTINE_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_thread_yield(duk_hthread *thr) {
	duk_hobject *caller_func;
	duk_small_uint_t is_error;

	DUK_DDD(DUK_DDDPRINT("Duktape.Thread.yield(): value=%!T, is_error=%!T",
	                     (duk_tval *) duk_get_tval(thr, 0),
	                     (duk_tval *) duk_get_tval(thr, 1)));

	DUK_ASSERT(thr->state == DUK_HTHREAD_STATE_RUNNING);
	DUK_ASSERT(thr->heap->curr_thread == thr);

	DUK_ASSERT(duk_get_top(thr) == 2);
	is_error = (duk_small_uint_t) duk_to_boolean_top_pop(thr);
	DUK_ASSERT(duk_get_top(thr) == 1);

	/* [ value ] */

	/*
	 *  Thread state and calling context checks
	 */

	if (!thr->resumer) {
		DUK_DD(DUK_DDPRINT("yield state invalid: current thread must have a resumer"));
		goto state_error;
	}
	DUK_ASSERT(thr->resumer->state == DUK_HTHREAD_STATE_RESUMED);

	if (thr->callstack_top < 2) {
		DUK_DD(DUK_DDPRINT(
		    "yield state invalid: callstack should contain at least 2 entries (caller and Duktape.Thread.yield)"));
		goto state_error;
	}
	DUK_ASSERT(thr->callstack_curr != NULL);
	DUK_ASSERT(thr->callstack_curr->parent != NULL);
	DUK_ASSERT(DUK_ACT_GET_FUNC(thr->callstack_curr) != NULL); /* us */
	DUK_ASSERT(DUK_HOBJECT_IS_NATFUNC(DUK_ACT_GET_FUNC(thr->callstack_curr)));
	DUK_ASSERT(DUK_ACT_GET_FUNC(thr->callstack_curr->parent) != NULL); /* caller */

	caller_func = DUK_ACT_GET_FUNC(thr->callstack_curr->parent);
	if (!DUK_HOBJECT_IS_COMPFUNC(caller_func)) {
		DUK_DD(DUK_DDPRINT("yield state invalid: caller must be ECMAScript code"));
		goto state_error;
	}

	DUK_ASSERT(thr->callstack_preventcount >= 1); /* should never be zero, because we (Duktape.Thread.yield) are on the stack */
	if (thr->callstack_preventcount != 1) {
		/* Note: the only yield-preventing call is Duktape.Thread.yield(), hence check for 1, not 0 */
		DUK_DD(DUK_DDPRINT("yield state invalid: there must be no yield-preventing calls in current thread callstack "
		                   "(preventcount is %ld)",
		                   (long) thr->callstack_preventcount));
		goto state_error;
	}

	/*
	 *  The error object has been augmented with a traceback and other
	 *  info from its creation point -- usually the current thread.
	 *  The error handler, however, is called right before throwing
	 *  and runs in the yielder's thread.
	 */

#if defined(DUK_USE_AUGMENT_ERROR_THROW)
	if (is_error) {
		DUK_ASSERT_TOP(thr, 1); /* value (error) is at stack top */
		duk_err_augment_error_throw(thr); /* in yielder's context */
	}
#endif

#if defined(DUK_USE_DEBUG)
	if (is_error) {
		DUK_DDD(DUK_DDDPRINT("YIELD ERROR: value=%!T", (duk_tval *) duk_get_tval(thr, 0)));
	} else {
		DUK_DDD(DUK_DDDPRINT("YIELD NORMAL: value=%!T", (duk_tval *) duk_get_tval(thr, 0)));
	}
#endif

	/*
	 *  Process yield
	 *
	 *  After longjmp(), processing continues in bytecode executor longjmp
	 *  handler, which will e.g. update thr->resumer to NULL.
	 */

	thr->heap->lj.type = DUK_LJ_TYPE_YIELD;

	/* lj value1: value */
	DUK_ASSERT(thr->valstack_bottom < thr->valstack_top);
	DUK_TVAL_SET_TVAL_UPDREF(thr, &thr->heap->lj.value1, &thr->valstack_bottom[0]); /* side effects */
	DUK_TVAL_CHKFAST_INPLACE_SLOW(&thr->heap->lj.value1);

	thr->heap->lj.iserror = is_error;

	DUK_ASSERT(thr->heap->lj.jmpbuf_ptr != NULL); /* call is from executor, so we know we have a jmpbuf */
	duk_err_longjmp(thr); /* execution resumes in bytecode executor */
	DUK_UNREACHABLE();
	/* Never here, fall through to error (from compiler point of view). */

state_error:
	DUK_DCERROR_TYPE_INVALID_STATE(thr);
}
#endif

#if defined(DUK_USE_COROUTINE_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_thread_current(duk_hthread *thr) {
	duk_push_current_thread(thr);
	return 1;
}
#endif
