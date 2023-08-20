/*
 *  Error built-ins
 */

#include "duk_internal.h"

DUK_INTERNAL duk_ret_t duk_bi_error_constructor_shared(duk_hthread *thr) {
	/* Behavior for constructor and non-constructor call is
	 * the same except for augmenting the created error.  When
	 * called as a constructor, the caller (duk_new()) will handle
	 * augmentation; when called as normal function, we need to do
	 * it here.
	 */

	duk_small_int_t bidx_prototype = duk_get_current_magic(thr);

	/* same for both error and each subclass like TypeError */
	duk_uint_t flags_and_class =
	    DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_FASTREFS | DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_ERROR);

	(void) duk_push_object_helper(thr, flags_and_class, bidx_prototype);

	/* If message is undefined, the own property 'message' is not set at
	 * all to save property space.  An empty message is inherited anyway.
	 */
	if (!duk_is_undefined(thr, 0)) {
		duk_to_string(thr, 0);
		duk_dup_0(thr); /* [ message error message ] */
		duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_MESSAGE, DUK_PROPDESC_FLAGS_WC);
	}

	/* Augment the error if called as a normal function.  __FILE__ and __LINE__
	 * are not desirable in this case.
	 */

#if defined(DUK_USE_AUGMENT_ERROR_CREATE)
	if (!duk_is_constructor_call(thr)) {
		duk_err_augment_error_create(thr, thr, NULL, 0, DUK_AUGMENT_FLAG_NOBLAME_FILELINE);
	}
#endif

	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_error_prototype_to_string(duk_hthread *thr) {
	/* XXX: optimize with more direct internal access */

	duk_push_this(thr);
	(void) duk_require_hobject_promote_mask(thr, -1, DUK_TYPE_MASK_LIGHTFUNC | DUK_TYPE_MASK_BUFFER);

	/* [ ... this ] */

	duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_NAME);
	if (duk_is_undefined(thr, -1)) {
		duk_pop(thr);
		duk_push_literal(thr, "Error");
	} else {
		duk_to_string(thr, -1);
	}

	/* [ ... this name ] */

	/* XXX: Are steps 6 and 7 in E5 Section 15.11.4.4 duplicated by
	 * accident or are they actually needed?  The first ToString()
	 * could conceivably return 'undefined'.
	 */
	duk_get_prop_stridx_short(thr, -2, DUK_STRIDX_MESSAGE);
	if (duk_is_undefined(thr, -1)) {
		duk_pop(thr);
		duk_push_hstring_empty(thr);
	} else {
		duk_to_string(thr, -1);
	}

	/* [ ... this name message ] */

	if (duk_get_length(thr, -2) == 0) {
		/* name is empty -> return message */
		return 1;
	}
	if (duk_get_length(thr, -1) == 0) {
		/* message is empty -> return name */
		duk_pop(thr);
		return 1;
	}
	duk_push_literal(thr, ": ");
	duk_insert(thr, -2); /* ... name ': ' message */
	duk_concat(thr, 3);

	return 1;
}

#if defined(DUK_USE_TRACEBACKS)

/*
 *  Traceback handling
 *
 *  The unified helper decodes the traceback and produces various requested
 *  outputs.  It should be optimized for size, and may leave garbage on stack,
 *  only the topmost return value matters.  For instance, traceback separator
 *  and decoded strings are pushed even when looking for filename only.
 *
 *  NOTE: although _Tracedata is an internal property, user code can currently
 *  write to the array (or replace it with something other than an array).
 *  The code below must tolerate arbitrary _Tracedata.  It can throw errors
 *  etc, but cannot cause a segfault or memory unsafe behavior.
 */

/* constants arbitrary, chosen for small loads */
#define DUK__OUTPUT_TYPE_TRACEBACK  (-1)
#define DUK__OUTPUT_TYPE_FILENAME   0
#define DUK__OUTPUT_TYPE_LINENUMBER 1

DUK_LOCAL duk_ret_t duk__error_getter_helper(duk_hthread *thr, duk_small_int_t output_type) {
	duk_idx_t idx_td;
	duk_small_int_t i; /* traceback depth fits into 16 bits */
	duk_small_int_t t; /* stack type fits into 16 bits */
	duk_small_int_t count_func = 0; /* traceback depth ensures fits into 16 bits */
	const char *str_tailcall = " tailcall";
	const char *str_strict = " strict";
	const char *str_construct = " construct";
	const char *str_prevyield = " preventsyield";
	const char *str_directeval = " directeval";
	const char *str_empty = "";

	DUK_ASSERT_TOP(thr, 0); /* fixed arg count */

	duk_push_this(thr);
	duk_xget_owndataprop_stridx_short(thr, -1, DUK_STRIDX_INT_TRACEDATA);
	idx_td = duk_get_top_index(thr);

	duk_push_hstring_stridx(thr, DUK_STRIDX_NEWLINE_4SPACE);
	duk_push_this(thr);

	/* [ ... this tracedata sep this ] */

	/* XXX: skip null filename? */

	if (duk_check_type(thr, idx_td, DUK_TYPE_OBJECT)) {
		/* Current tracedata contains 2 entries per callstack entry. */
		for (i = 0;; i += 2) {
			duk_int_t pc;
			duk_uint_t line;
			duk_uint_t flags;
			duk_double_t d;
			const char *funcname;
			const char *filename;
			duk_hobject *h_func;
			duk_hstring *h_name;

			duk_require_stack(thr, 5);
			duk_get_prop_index(thr, idx_td, (duk_uarridx_t) i);
			duk_get_prop_index(thr, idx_td, (duk_uarridx_t) (i + 1));
			d = duk_to_number_m1(thr);
			pc = duk_double_to_int_t(DUK_FMOD(d, DUK_DOUBLE_2TO32));
			flags = duk_double_to_uint_t(DUK_FLOOR(d / DUK_DOUBLE_2TO32));
			t = (duk_small_int_t) duk_get_type(thr, -2);

			if (t == DUK_TYPE_OBJECT || t == DUK_TYPE_LIGHTFUNC) {
				/*
				 *  ECMAScript/native function call or lightfunc call
				 */

				count_func++;

				/* [ ... v1(func) v2(pc+flags) ] */

				/* These may be systematically omitted by Duktape
				 * with certain config options, but allow user to
				 * set them on a case-by-case basis.
				 */
				duk_get_prop_stridx_short(thr, -2, DUK_STRIDX_NAME);
				duk_get_prop_stridx_short(thr, -3, DUK_STRIDX_FILE_NAME);

#if defined(DUK_USE_PC2LINE)
				line = (duk_uint_t) duk_hobject_pc2line_query(thr, -4, (duk_uint_fast32_t) pc);
#else
				line = 0;
#endif

				/* [ ... v1 v2 name filename ] */

				/* When looking for .fileName/.lineNumber, blame first
				 * function which has a .fileName.
				 */
				if (duk_is_string_notsymbol(thr, -1)) {
					if (output_type == DUK__OUTPUT_TYPE_FILENAME) {
						return 1;
					} else if (output_type == DUK__OUTPUT_TYPE_LINENUMBER) {
						duk_push_uint(thr, line);
						return 1;
					}
				}

				/* XXX: Change 'anon' handling here too, to use empty string for anonymous functions? */
				/* XXX: Could be improved by coercing to a readable duk_tval (especially string escaping) */
				h_name = duk_get_hstring_notsymbol(thr, -2); /* may be NULL */
				funcname = (h_name == NULL || h_name == DUK_HTHREAD_STRING_EMPTY_STRING(thr)) ?
                                               "[anon]" :
                                               (const char *) DUK_HSTRING_GET_DATA(h_name);
				filename = duk_get_string_notsymbol(thr, -1);
				filename = filename ? filename : "";
				DUK_ASSERT(funcname != NULL);
				DUK_ASSERT(filename != NULL);

				h_func = duk_get_hobject(thr, -4); /* NULL for lightfunc */

				if (h_func == NULL) {
					duk_push_sprintf(
					    thr,
					    "at %s light%s%s%s%s%s",
					    (const char *) funcname,
					    (const char *) ((flags & DUK_ACT_FLAG_STRICT) ? str_strict : str_empty),
					    (const char *) ((flags & DUK_ACT_FLAG_TAILCALLED) ? str_tailcall : str_empty),
					    (const char *) ((flags & DUK_ACT_FLAG_CONSTRUCT) ? str_construct : str_empty),
					    (const char *) ((flags & DUK_ACT_FLAG_DIRECT_EVAL) ? str_directeval : str_empty),
					    (const char *) ((flags & DUK_ACT_FLAG_PREVENT_YIELD) ? str_prevyield : str_empty));
				} else if (DUK_HOBJECT_HAS_NATFUNC(h_func)) {
					duk_push_sprintf(
					    thr,
					    "at %s (%s) native%s%s%s%s%s",
					    (const char *) funcname,
					    (const char *) filename,
					    (const char *) ((flags & DUK_ACT_FLAG_STRICT) ? str_strict : str_empty),
					    (const char *) ((flags & DUK_ACT_FLAG_TAILCALLED) ? str_tailcall : str_empty),
					    (const char *) ((flags & DUK_ACT_FLAG_CONSTRUCT) ? str_construct : str_empty),
					    (const char *) ((flags & DUK_ACT_FLAG_DIRECT_EVAL) ? str_directeval : str_empty),
					    (const char *) ((flags & DUK_ACT_FLAG_PREVENT_YIELD) ? str_prevyield : str_empty));
				} else {
					duk_push_sprintf(
					    thr,
					    "at %s (%s:%lu)%s%s%s%s%s",
					    (const char *) funcname,
					    (const char *) filename,
					    (unsigned long) line,
					    (const char *) ((flags & DUK_ACT_FLAG_STRICT) ? str_strict : str_empty),
					    (const char *) ((flags & DUK_ACT_FLAG_TAILCALLED) ? str_tailcall : str_empty),
					    (const char *) ((flags & DUK_ACT_FLAG_CONSTRUCT) ? str_construct : str_empty),
					    (const char *) ((flags & DUK_ACT_FLAG_DIRECT_EVAL) ? str_directeval : str_empty),
					    (const char *) ((flags & DUK_ACT_FLAG_PREVENT_YIELD) ? str_prevyield : str_empty));
				}
				duk_replace(thr, -5); /* [ ... v1 v2 name filename str ] -> [ ... str v2 name filename ] */
				duk_pop_3(thr); /* -> [ ... str ] */
			} else if (t == DUK_TYPE_STRING) {
				const char *str_file;

				/*
				 *  __FILE__ / __LINE__ entry, here 'pc' is line number directly.
				 *  Sometimes __FILE__ / __LINE__ is reported as the source for
				 *  the error (fileName, lineNumber), sometimes not.
				 */

				/* [ ... v1(filename) v2(line+flags) ] */

				/* When looking for .fileName/.lineNumber, blame compilation
				 * or C call site unless flagged not to do so.
				 */
				if (!(flags & DUK_TB_FLAG_NOBLAME_FILELINE)) {
					if (output_type == DUK__OUTPUT_TYPE_FILENAME) {
						duk_pop(thr);
						return 1;
					} else if (output_type == DUK__OUTPUT_TYPE_LINENUMBER) {
						duk_push_int(thr, pc);
						return 1;
					}
				}

				/* Tracedata is trusted but avoid any risk of using a NULL
				 * for %s format because it has undefined behavior.  Symbols
				 * don't need to be explicitly rejected as they pose no memory
				 * safety issues.
				 */
				str_file = (const char *) duk_get_string(thr, -2);
				duk_push_sprintf(thr,
				                 "at [anon] (%s:%ld) internal",
				                 (const char *) (str_file ? str_file : "null"),
				                 (long) pc);
				duk_replace(thr, -3); /* [ ... v1 v2 str ] -> [ ... str v2 ] */
				duk_pop(thr); /* -> [ ... str ] */
			} else {
				/* unknown, ignore */
				duk_pop_2(thr);
				break;
			}
		}

		if (count_func >= DUK_USE_TRACEBACK_DEPTH) {
			/* Possibly truncated; there is no explicit truncation
			 * marker so this is the best we can do.
			 */

			duk_push_hstring_stridx(thr, DUK_STRIDX_BRACKETED_ELLIPSIS);
		}
	}

	/* [ ... this tracedata sep this str1 ... strN ] */

	if (output_type != DUK__OUTPUT_TYPE_TRACEBACK) {
		return 0;
	} else {
		/* The 'this' after 'sep' will get ToString() coerced by
		 * duk_join() automatically.  We don't want to do that
		 * coercion when providing .fileName or .lineNumber (GH-254).
		 */
		duk_join(thr, duk_get_top(thr) - (idx_td + 2) /*count, not including sep*/);
		return 1;
	}
}

/* XXX: Output type could be encoded into native function 'magic' value to
 * save space.  For setters the stridx could be encoded into 'magic'.
 */

DUK_INTERNAL duk_ret_t duk_bi_error_prototype_stack_getter(duk_hthread *thr) {
	return duk__error_getter_helper(thr, DUK__OUTPUT_TYPE_TRACEBACK);
}

DUK_INTERNAL duk_ret_t duk_bi_error_prototype_filename_getter(duk_hthread *thr) {
	return duk__error_getter_helper(thr, DUK__OUTPUT_TYPE_FILENAME);
}

DUK_INTERNAL duk_ret_t duk_bi_error_prototype_linenumber_getter(duk_hthread *thr) {
	return duk__error_getter_helper(thr, DUK__OUTPUT_TYPE_LINENUMBER);
}

#else /* DUK_USE_TRACEBACKS */

/*
 *  Traceback handling when tracebacks disabled.
 *
 *  The fileName / lineNumber stubs are now necessary because built-in
 *  data will include the accessor properties in Error.prototype.  If those
 *  are removed for builds without tracebacks, these can also be removed.
 *  'stack' should still be present and produce a ToString() equivalent:
 *  this is useful for user code which prints a stacktrace and expects to
 *  see something useful.  A normal stacktrace also begins with a ToString()
 *  of the error so this makes sense.
 */

DUK_INTERNAL duk_ret_t duk_bi_error_prototype_stack_getter(duk_hthread *thr) {
	/* XXX: remove this native function and map 'stack' accessor
	 * to the toString() implementation directly.
	 */
	return duk_bi_error_prototype_to_string(thr);
}

DUK_INTERNAL duk_ret_t duk_bi_error_prototype_filename_getter(duk_hthread *thr) {
	DUK_UNREF(thr);
	return 0;
}

DUK_INTERNAL duk_ret_t duk_bi_error_prototype_linenumber_getter(duk_hthread *thr) {
	DUK_UNREF(thr);
	return 0;
}

#endif /* DUK_USE_TRACEBACKS */

DUK_LOCAL duk_ret_t duk__error_setter_helper(duk_hthread *thr, duk_small_uint_t stridx_key) {
	/* Attempt to write 'stack', 'fileName', 'lineNumber' works as if
	 * user code called Object.defineProperty() to create an overriding
	 * own property.  This allows user code to overwrite .fileName etc
	 * intuitively as e.g. "err.fileName = 'dummy'" as one might expect.
	 * See https://github.com/svaarala/duktape/issues/387.
	 */

	DUK_ASSERT_TOP(thr, 1); /* fixed arg count: value */

	duk_push_this(thr);
	duk_push_hstring_stridx(thr, stridx_key);
	duk_dup_0(thr);

	/* [ ... obj key value ] */

	DUK_DD(DUK_DDPRINT("error setter: %!T %!T %!T", duk_get_tval(thr, -3), duk_get_tval(thr, -2), duk_get_tval(thr, -1)));

	duk_def_prop(thr,
	             -3,
	             DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_HAVE_WRITABLE | DUK_DEFPROP_WRITABLE |
	                 DUK_DEFPROP_HAVE_ENUMERABLE | /*not enumerable*/
	                 DUK_DEFPROP_HAVE_CONFIGURABLE | DUK_DEFPROP_CONFIGURABLE);
	return 0;
}

DUK_INTERNAL duk_ret_t duk_bi_error_prototype_stack_setter(duk_hthread *thr) {
	return duk__error_setter_helper(thr, DUK_STRIDX_STACK);
}

DUK_INTERNAL duk_ret_t duk_bi_error_prototype_filename_setter(duk_hthread *thr) {
	return duk__error_setter_helper(thr, DUK_STRIDX_FILE_NAME);
}

DUK_INTERNAL duk_ret_t duk_bi_error_prototype_linenumber_setter(duk_hthread *thr) {
	return duk__error_setter_helper(thr, DUK_STRIDX_LINE_NUMBER);
}
