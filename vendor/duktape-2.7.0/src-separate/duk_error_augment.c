/*
 *  Augmenting errors at their creation site and their throw site.
 *
 *  When errors are created, traceback data is added by built-in code
 *  and a user error handler (if defined) can process or replace the
 *  error.  Similarly, when errors are thrown, a user error handler
 *  (if defined) can process or replace the error.
 *
 *  Augmentation and other processing at error creation time is nice
 *  because an error is only created once, but it may be thrown and
 *  rethrown multiple times.  User error handler registered for processing
 *  an error at its throw site must be careful to handle rethrowing in
 *  a useful manner.
 *
 *  Error augmentation may throw an internal error (e.g. alloc error).
 *
 *  ECMAScript allows throwing any values, so all values cannot be
 *  augmented.  Currently, the built-in augmentation at error creation
 *  only augments error values which are Error instances (= have the
 *  built-in Error.prototype in their prototype chain) and are also
 *  extensible.  User error handlers have no limitations in this respect.
 */

#include "duk_internal.h"

/*
 *  Helper for calling a user error handler.
 *
 *  'thr' must be the currently active thread; the error handler is called
 *  in its context.  The valstack of 'thr' must have the error value on
 *  top, and will be replaced by another error value based on the return
 *  value of the error handler.
 *
 *  The helper calls duk_handle_call() recursively in protected mode.
 *  Before that call happens, no longjmps should happen; as a consequence,
 *  we must assume that the valstack contains enough temporary space for
 *  arguments and such.
 *
 *  While the error handler runs, any errors thrown will not trigger a
 *  recursive error handler call (this is implemented using a heap level
 *  flag which will "follow" through any coroutines resumed inside the
 *  error handler).  If the error handler is not callable or throws an
 *  error, the resulting error replaces the original error (for Duktape
 *  internal errors, duk_error_throw.c further substitutes this error with
 *  a DoubleError which is not ideal).  This would be easy to change and
 *  even signal to the caller.
 *
 *  The user error handler is stored in 'Duktape.errCreate' or
 *  'Duktape.errThrow' depending on whether we're augmenting the error at
 *  creation or throw time.  There are several alternatives to this approach,
 *  see doc/error-objects.rst for discussion.
 *
 *  Note: since further longjmp()s may occur while calling the error handler
 *  (for many reasons, e.g. a labeled 'break' inside the handler), the
 *  caller can make no assumptions on the thr->heap->lj state after the
 *  call (this affects especially duk_error_throw.c).  This is not an issue
 *  as long as the caller writes to the lj state only after the error handler
 *  finishes.
 */

#if defined(DUK_USE_ERRTHROW) || defined(DUK_USE_ERRCREATE)
DUK_LOCAL void duk__err_augment_user(duk_hthread *thr, duk_small_uint_t stridx_cb) {
	duk_tval *tv_hnd;
	duk_int_t rc;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT_STRIDX_VALID(stridx_cb);

	if (thr->heap->augmenting_error) {
		DUK_D(DUK_DPRINT("recursive call to error augmentation, ignore"));
		return;
	}

	/*
	 *  Check whether or not we have an error handler.
	 *
	 *  We must be careful of not triggering an error when looking up the
	 *  property.  For instance, if the property is a getter, we don't want
	 *  to call it, only plain values are allowed.  The value, if it exists,
	 *  is not checked.  If the value is not a function, a TypeError happens
	 *  when it is called and that error replaces the original one.
	 */

	DUK_ASSERT_VALSTACK_SPACE(thr, 4); /* 3 entries actually needed below */

	/* [ ... errval ] */

	if (thr->builtins[DUK_BIDX_DUKTAPE] == NULL) {
		/* When creating built-ins, some of the built-ins may not be set
		 * and we want to tolerate that when throwing errors.
		 */
		DUK_DD(DUK_DDPRINT("error occurred when DUK_BIDX_DUKTAPE is NULL, ignoring"));
		return;
	}
	tv_hnd = duk_hobject_find_entry_tval_ptr_stridx(thr->heap, thr->builtins[DUK_BIDX_DUKTAPE], stridx_cb);
	if (tv_hnd == NULL) {
		DUK_DD(DUK_DDPRINT("error handler does not exist or is not a plain value: %!T", (duk_tval *) tv_hnd));
		return;
	}
	DUK_DDD(DUK_DDDPRINT("error handler dump (callability not checked): %!T", (duk_tval *) tv_hnd));
	duk_push_tval(thr, tv_hnd);

	/* [ ... errval errhandler ] */

	duk_insert(thr, -2); /* -> [ ... errhandler errval ] */
	duk_push_undefined(thr);
	duk_insert(thr, -2); /* -> [ ... errhandler undefined(= this) errval ] */

	/* [ ... errhandler undefined errval ] */

	/*
	 *  heap->augmenting_error prevents recursive re-entry and also causes
	 *  call handling to use a larger (but not unbounded) call stack limit
	 *  for the duration of error augmentation.
	 *
	 *  We ignore errors now: a success return and an error value both
	 *  replace the original error value.  (This would be easy to change.)
	 */

	DUK_ASSERT(thr->heap->augmenting_error == 0);
	thr->heap->augmenting_error = 1;

	rc = duk_pcall_method(thr, 1);
	DUK_UNREF(rc); /* no need to check now: both success and error are OK */

	DUK_ASSERT(thr->heap->augmenting_error == 1);
	thr->heap->augmenting_error = 0;

	/* [ ... errval ] */
}
#endif /* DUK_USE_ERRTHROW || DUK_USE_ERRCREATE */

/*
 *  Add ._Tracedata to an error on the stack top.
 */

#if defined(DUK_USE_TRACEBACKS)
DUK_LOCAL void duk__add_traceback(duk_hthread *thr,
                                  duk_hthread *thr_callstack,
                                  const char *c_filename,
                                  duk_int_t c_line,
                                  duk_small_uint_t flags) {
	duk_activation *act;
	duk_int_t depth;
	duk_int_t arr_size;
	duk_tval *tv;
	duk_hstring *s;
	duk_uint32_t u32;
	duk_double_t d;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr_callstack != NULL);

	/* [ ... error ] */

	/*
	 *  The traceback format is pretty arcane in an attempt to keep it compact
	 *  and cheap to create.  It may change arbitrarily from version to version.
	 *  It should be decoded/accessed through version specific accessors only.
	 *
	 *  See doc/error-objects.rst.
	 */

	DUK_DDD(DUK_DDDPRINT("adding traceback to object: %!T", (duk_tval *) duk_get_tval(thr, -1)));

	/* Preallocate array to correct size, so that we can just write out
	 * the _Tracedata values into the array part.
	 */
	act = thr->callstack_curr;
	depth = DUK_USE_TRACEBACK_DEPTH;
	DUK_ASSERT(thr_callstack->callstack_top <= DUK_INT_MAX); /* callstack limits */
	if (depth > (duk_int_t) thr_callstack->callstack_top) {
		depth = (duk_int_t) thr_callstack->callstack_top;
	}
	if (depth > 0) {
		if (flags & DUK_AUGMENT_FLAG_SKIP_ONE) {
			DUK_ASSERT(act != NULL);
			act = act->parent;
			depth--;
		}
	}
	arr_size = depth * 2;
	if (thr->compile_ctx != NULL && thr->compile_ctx->h_filename != NULL) {
		arr_size += 2;
	}
	if (c_filename) {
		/* We need the C filename to be interned before getting the
		 * array part pointer to avoid any GC interference while the
		 * array part is populated.
		 */
		duk_push_string(thr, c_filename);
		arr_size += 2;
	}

	/* XXX: Uninitialized would be OK.  Maybe add internal primitive to
	 * push bare duk_harray with size?
	 */
	DUK_D(DUK_DPRINT("preallocated _Tracedata to %ld items", (long) arr_size));
	tv = duk_push_harray_with_size_outptr(thr, (duk_uint32_t) arr_size);
	duk_clear_prototype(thr, -1);
	DUK_ASSERT(duk_is_bare_object(thr, -1));
	DUK_ASSERT(arr_size == 0 || tv != NULL);

	/* Compiler SyntaxErrors (and other errors) come first, and are
	 * blamed by default (not flagged "noblame").
	 */
	if (thr->compile_ctx != NULL && thr->compile_ctx->h_filename != NULL) {
		s = thr->compile_ctx->h_filename;
		DUK_TVAL_SET_STRING(tv, s);
		DUK_HSTRING_INCREF(thr, s);
		tv++;

		u32 = (duk_uint32_t) thr->compile_ctx->curr_token.start_line; /* (flags<<32) + (line), flags = 0 */
		DUK_TVAL_SET_U32(tv, u32);
		tv++;
	}

	/* Filename/line from C macros (__FILE__, __LINE__) are added as an
	 * entry with a special format: (string, number).  The number contains
	 * the line and flags.
	 */

	/* [ ... error c_filename? arr ] */

	if (c_filename) {
		DUK_ASSERT(DUK_TVAL_IS_STRING(thr->valstack_top - 2));
		s = DUK_TVAL_GET_STRING(thr->valstack_top - 2); /* interned c_filename */
		DUK_ASSERT(s != NULL);
		DUK_TVAL_SET_STRING(tv, s);
		DUK_HSTRING_INCREF(thr, s);
		tv++;

		d = ((flags & DUK_AUGMENT_FLAG_NOBLAME_FILELINE) ?
                         ((duk_double_t) DUK_TB_FLAG_NOBLAME_FILELINE) * DUK_DOUBLE_2TO32 :
                         0.0) +
		    (duk_double_t) c_line;
		DUK_TVAL_SET_DOUBLE(tv, d);
		tv++;
	}

	/* Traceback depth doesn't take into account the filename/line
	 * special handling above (intentional).
	 */
	for (; depth-- > 0; act = act->parent) {
		duk_uint32_t pc;
		duk_tval *tv_src;

		/* [... arr] */

		DUK_ASSERT(act != NULL); /* depth check above, assumes book-keeping is correct */
		DUK_ASSERT_DISABLE(act->pc >= 0); /* unsigned */

		/* Add function object. */
		tv_src = &act->tv_func; /* object (function) or lightfunc */
		DUK_ASSERT(DUK_TVAL_IS_OBJECT(tv_src) || DUK_TVAL_IS_LIGHTFUNC(tv_src));
		DUK_TVAL_SET_TVAL(tv, tv_src);
		DUK_TVAL_INCREF(thr, tv);
		tv++;

		/* Add a number containing: pc, activation flags.
		 *
		 * PC points to next instruction, find offending PC.  Note that
		 * PC == 0 for native code.
		 */
		pc = (duk_uint32_t) duk_hthread_get_act_prev_pc(thr_callstack, act);
		DUK_ASSERT_DISABLE(pc >= 0); /* unsigned */
		DUK_ASSERT((duk_double_t) pc < DUK_DOUBLE_2TO32); /* assume PC is at most 32 bits and non-negative */
		d = ((duk_double_t) act->flags) * DUK_DOUBLE_2TO32 + (duk_double_t) pc;
		DUK_TVAL_SET_DOUBLE(tv, d);
		tv++;
	}

#if defined(DUK_USE_ASSERTIONS)
	{
		duk_harray *a;
		a = (duk_harray *) duk_known_hobject(thr, -1);
		DUK_ASSERT(a != NULL);
		DUK_ASSERT((duk_uint32_t) (tv - DUK_HOBJECT_A_GET_BASE(thr->heap, (duk_hobject *) a)) == a->length);
		DUK_ASSERT(a->length == (duk_uint32_t) arr_size);
		DUK_ASSERT(duk_is_bare_object(thr, -1));
	}
#endif

	/* [ ... error c_filename? arr ] */

	if (c_filename) {
		duk_remove_m2(thr);
	}

	/* [ ... error arr ] */

	duk_xdef_prop_stridx_short_wec(thr, -2, DUK_STRIDX_INT_TRACEDATA); /* -> [ ... error ] */
}
#endif /* DUK_USE_TRACEBACKS */

/*
 *  Add .fileName and .lineNumber to an error on the stack top.
 */

#if defined(DUK_USE_AUGMENT_ERROR_CREATE) && !defined(DUK_USE_TRACEBACKS)
DUK_LOCAL void duk__add_fileline(duk_hthread *thr,
                                 duk_hthread *thr_callstack,
                                 const char *c_filename,
                                 duk_int_t c_line,
                                 duk_small_uint_t flags) {
#if defined(DUK_USE_ASSERTIONS)
	duk_int_t entry_top;
#endif

#if defined(DUK_USE_ASSERTIONS)
	entry_top = duk_get_top(thr);
#endif

	/*
	 *  If tracebacks are disabled, 'fileName' and 'lineNumber' are added
	 *  as plain own properties.  Since Error.prototype has accessors of
	 *  the same name, we need to define own properties directly (cannot
	 *  just use e.g. duk_put_prop_stridx).  Existing properties are not
	 *  overwritten in case they already exist.
	 */

	if (thr->compile_ctx != NULL && thr->compile_ctx->h_filename != NULL) {
		/* Compiler SyntaxError (or other error) gets the primary blame.
		 * Currently no flag to prevent blaming.
		 */
		duk_push_uint(thr, (duk_uint_t) thr->compile_ctx->curr_token.start_line);
		duk_push_hstring(thr, thr->compile_ctx->h_filename);
	} else if (c_filename && (flags & DUK_AUGMENT_FLAG_NOBLAME_FILELINE) == 0) {
		/* C call site gets blamed next, unless flagged not to do so.
		 * XXX: file/line is disabled in minimal builds, so disable this
		 * too when appropriate.
		 */
		duk_push_int(thr, c_line);
		duk_push_string(thr, c_filename);
	} else {
		/* Finally, blame the innermost callstack entry which has a
		 * .fileName property.
		 */
		duk_small_uint_t depth;
		duk_uint32_t ecma_line;
		duk_activation *act;

		DUK_ASSERT(thr_callstack->callstack_top <= DUK_INT_MAX); /* callstack limits */
		depth = DUK_USE_TRACEBACK_DEPTH;
		if (depth > thr_callstack->callstack_top) {
			depth = thr_callstack->callstack_top;
		}
		for (act = thr_callstack->callstack_curr; depth-- > 0; act = act->parent) {
			duk_hobject *func;
			duk_uint32_t pc;

			DUK_ASSERT(act != NULL);
			func = DUK_ACT_GET_FUNC(act);
			if (func == NULL) {
				/* Lightfunc, not blamed now. */
				continue;
			}

			/* PC points to next instruction, find offending PC,
			 * PC == 0 for native code.
			 */
			pc = duk_hthread_get_act_prev_pc(
			    thr,
			    act); /* thr argument only used for thr->heap, so specific thread doesn't matter */
			DUK_UNREF(pc);
			DUK_ASSERT_DISABLE(pc >= 0); /* unsigned */
			DUK_ASSERT((duk_double_t) pc < DUK_DOUBLE_2TO32); /* assume PC is at most 32 bits and non-negative */

			duk_push_hobject(thr, func);

			/* [ ... error func ] */

			duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_FILE_NAME);
			if (!duk_is_string_notsymbol(thr, -1)) {
				duk_pop_2(thr);
				continue;
			}

			/* [ ... error func fileName ] */

			ecma_line = 0;
#if defined(DUK_USE_PC2LINE)
			if (DUK_HOBJECT_IS_COMPFUNC(func)) {
				ecma_line = duk_hobject_pc2line_query(thr, -2, (duk_uint_fast32_t) pc);
			} else {
				/* Native function, no relevant lineNumber. */
			}
#endif /* DUK_USE_PC2LINE */
			duk_push_u32(thr, ecma_line);

			/* [ ... error func fileName lineNumber ] */

			duk_replace(thr, -3);

			/* [ ... error lineNumber fileName ] */
			goto define_props;
		}

		/* No activation matches, use undefined for both .fileName and
		 * .lineNumber (matches what we do with a _Tracedata based
		 * no-match lookup.
		 */
		duk_push_undefined(thr);
		duk_push_undefined(thr);
	}

define_props:
	/* [ ... error lineNumber fileName ] */
#if defined(DUK_USE_ASSERTIONS)
	DUK_ASSERT(duk_get_top(thr) == entry_top + 2);
#endif
	duk_xdef_prop_stridx_short(thr, -3, DUK_STRIDX_FILE_NAME, DUK_PROPDESC_FLAGS_C | DUK_PROPDESC_FLAG_NO_OVERWRITE);
	duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_LINE_NUMBER, DUK_PROPDESC_FLAGS_C | DUK_PROPDESC_FLAG_NO_OVERWRITE);
}
#endif /* DUK_USE_AUGMENT_ERROR_CREATE && !DUK_USE_TRACEBACKS */

/*
 *  Add line number to a compiler error.
 */

#if defined(DUK_USE_AUGMENT_ERROR_CREATE)
DUK_LOCAL void duk__add_compiler_error_line(duk_hthread *thr) {
	/* Append a "(line NNN)" to the "message" property of any error
	 * thrown during compilation.  Usually compilation errors are
	 * SyntaxErrors but they can also be out-of-memory errors and
	 * the like.
	 */

	/* [ ... error ] */

	DUK_ASSERT(duk_is_object(thr, -1));

	if (!(thr->compile_ctx != NULL && thr->compile_ctx->h_filename != NULL)) {
		return;
	}

	DUK_DDD(DUK_DDDPRINT("compile error, before adding line info: %!T", (duk_tval *) duk_get_tval(thr, -1)));

	if (duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_MESSAGE)) {
		duk_bool_t at_end;

		/* Best guesstimate that error occurred at end of input, token
		 * truncated by end of input, etc.
		 */
#if 0
		at_end = (thr->compile_ctx->curr_token.start_offset + 1 >= thr->compile_ctx->lex.input_length);
		at_end = (thr->compile_ctx->lex.window[0].codepoint < 0 || thr->compile_ctx->lex.window[1].codepoint < 0);
#endif
		at_end = (thr->compile_ctx->lex.window[0].codepoint < 0);

		DUK_D(DUK_DPRINT("syntax error, determined at_end=%ld; curr_token.start_offset=%ld, "
		                 "lex.input_length=%ld, window[0].codepoint=%ld, window[1].codepoint=%ld",
		                 (long) at_end,
		                 (long) thr->compile_ctx->curr_token.start_offset,
		                 (long) thr->compile_ctx->lex.input_length,
		                 (long) thr->compile_ctx->lex.window[0].codepoint,
		                 (long) thr->compile_ctx->lex.window[1].codepoint));

		duk_push_sprintf(thr,
		                 " (line %ld%s)",
		                 (long) thr->compile_ctx->curr_token.start_line,
		                 at_end ? ", end of input" : "");
		duk_concat(thr, 2);
		duk_put_prop_stridx_short(thr, -2, DUK_STRIDX_MESSAGE);
	} else {
		duk_pop(thr);
	}

	DUK_DDD(DUK_DDDPRINT("compile error, after adding line info: %!T", (duk_tval *) duk_get_tval(thr, -1)));
}
#endif /* DUK_USE_AUGMENT_ERROR_CREATE */

/*
 *  Augment an error being created using Duktape specific properties
 *  like _Tracedata or .fileName/.lineNumber.
 */

#if defined(DUK_USE_AUGMENT_ERROR_CREATE)
DUK_LOCAL void duk__err_augment_builtin_create(duk_hthread *thr,
                                               duk_hthread *thr_callstack,
                                               const char *c_filename,
                                               duk_int_t c_line,
                                               duk_hobject *obj,
                                               duk_small_uint_t flags) {
#if defined(DUK_USE_ASSERTIONS)
	duk_int_t entry_top;
#endif

#if defined(DUK_USE_ASSERTIONS)
	entry_top = duk_get_top(thr);
#endif
	DUK_ASSERT(obj != NULL);

	DUK_UNREF(obj); /* unreferenced w/o tracebacks */

	duk__add_compiler_error_line(thr);

#if defined(DUK_USE_TRACEBACKS)
	/* If tracebacks are enabled, the '_Tracedata' property is the only
	 * thing we need: 'fileName' and 'lineNumber' are virtual properties
	 * which use '_Tracedata'.  (Check _Tracedata only as own property.)
	 */
	if (duk_hobject_find_entry_tval_ptr_stridx(thr->heap, obj, DUK_STRIDX_INT_TRACEDATA) != NULL) {
		DUK_DDD(DUK_DDDPRINT("error value already has a '_Tracedata' property, not modifying it"));
	} else {
		duk__add_traceback(thr, thr_callstack, c_filename, c_line, flags);
	}
#else
	/* Without tracebacks the concrete .fileName and .lineNumber need
	 * to be added directly.
	 */
	duk__add_fileline(thr, thr_callstack, c_filename, c_line, flags);
#endif

#if defined(DUK_USE_ASSERTIONS)
	DUK_ASSERT(duk_get_top(thr) == entry_top);
#endif
}
#endif /* DUK_USE_AUGMENT_ERROR_CREATE */

/*
 *  Augment an error at creation time with _Tracedata/fileName/lineNumber
 *  and allow a user error handler (if defined) to process/replace the error.
 *  The error to be augmented is at the stack top.
 *
 *  thr: thread containing the error value
 *  thr_callstack: thread which should be used for generating callstack etc.
 *  c_filename: C __FILE__ related to the error
 *  c_line: C __LINE__ related to the error
 *  flags & DUK_AUGMENT_FLAG_NOBLAME_FILELINE:
 *      if true, don't fileName/line as error source, otherwise use traceback
 *      (needed because user code filename/line are reported but internal ones
 *      are not)
 */

#if defined(DUK_USE_AUGMENT_ERROR_CREATE)
DUK_INTERNAL void duk_err_augment_error_create(duk_hthread *thr,
                                               duk_hthread *thr_callstack,
                                               const char *c_filename,
                                               duk_int_t c_line,
                                               duk_small_uint_t flags) {
	duk_hobject *obj;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr_callstack != NULL);

	/* [ ... error ] */

	/*
	 *  Criteria for augmenting:
	 *
	 *   - augmentation enabled in build (naturally)
	 *   - error value internal prototype chain contains the built-in
	 *     Error prototype object (i.e. 'val instanceof Error')
	 *
	 *  Additional criteria for built-in augmenting:
	 *
	 *   - error value is an extensible object
	 */

	obj = duk_get_hobject(thr, -1);
	if (!obj) {
		DUK_DDD(DUK_DDDPRINT("value is not an object, skip both built-in and user augment"));
		return;
	}
	if (!duk_hobject_prototype_chain_contains(thr, obj, thr->builtins[DUK_BIDX_ERROR_PROTOTYPE], 1 /*ignore_loop*/)) {
		/* If the value has a prototype loop, it's critical not to
		 * throw here.  Instead, assume the value is not to be
		 * augmented.
		 */
		DUK_DDD(DUK_DDDPRINT("value is not an error instance, skip both built-in and user augment"));
		return;
	}
	if (DUK_HOBJECT_HAS_EXTENSIBLE(obj)) {
		DUK_DDD(DUK_DDDPRINT("error meets criteria, built-in augment"));
		duk__err_augment_builtin_create(thr, thr_callstack, c_filename, c_line, obj, flags);
	} else {
		DUK_DDD(DUK_DDDPRINT("error does not meet criteria, no built-in augment"));
	}

	/* [ ... error ] */

#if defined(DUK_USE_ERRCREATE)
	duk__err_augment_user(thr, DUK_STRIDX_ERR_CREATE);
#endif
}
#endif /* DUK_USE_AUGMENT_ERROR_CREATE */

/*
 *  Augment an error at throw time; allow a user error handler (if defined)
 *  to process/replace the error.  The error to be augmented is at the
 *  stack top.
 */

#if defined(DUK_USE_AUGMENT_ERROR_THROW)
DUK_INTERNAL void duk_err_augment_error_throw(duk_hthread *thr) {
#if defined(DUK_USE_ERRTHROW)
	duk__err_augment_user(thr, DUK_STRIDX_ERR_THROW);
#endif /* DUK_USE_ERRTHROW */
}
#endif /* DUK_USE_AUGMENT_ERROR_THROW */
