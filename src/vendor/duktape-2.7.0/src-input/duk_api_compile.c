/*
 *  Compilation and evaluation
 */

#include "duk_internal.h"

typedef struct duk__compile_raw_args duk__compile_raw_args;
struct duk__compile_raw_args {
	duk_size_t src_length; /* should be first on 64-bit platforms */
	const duk_uint8_t *src_buffer;
	duk_uint_t flags;
};

/* Eval is just a wrapper now. */
DUK_EXTERNAL duk_int_t duk_eval_raw(duk_hthread *thr, const char *src_buffer, duk_size_t src_length, duk_uint_t flags) {
	duk_int_t rc;

	DUK_ASSERT_API_ENTRY(thr);

	/* Note: strictness is *not* inherited from the current Duktape/C.
	 * This would be confusing because the current strictness state
	 * depends on whether we're running inside a Duktape/C activation
	 * (= strict mode) or outside of any activation (= non-strict mode).
	 * See tests/api/test-eval-strictness.c for more discussion.
	 */

	/* [ ... source? filename? ] (depends on flags) */

	rc = duk_compile_raw(thr,
	                     src_buffer,
	                     src_length,
	                     flags | DUK_COMPILE_EVAL); /* may be safe, or non-safe depending on flags */

	/* [ ... closure/error ] */

	if (rc != DUK_EXEC_SUCCESS) {
		rc = DUK_EXEC_ERROR;
		goto got_rc;
	}

	duk_push_global_object(thr); /* explicit 'this' binding, see GH-164 */

	if (flags & DUK_COMPILE_SAFE) {
		rc = duk_pcall_method(thr, 0);
	} else {
		duk_call_method(thr, 0);
		rc = DUK_EXEC_SUCCESS;
	}

	/* [ ... result/error ] */

got_rc:
	if (flags & DUK_COMPILE_NORESULT) {
		duk_pop(thr);
	}

	return rc;
}

/* Helper which can be called both directly and with duk_safe_call(). */
DUK_LOCAL duk_ret_t duk__do_compile(duk_hthread *thr, void *udata) {
	duk__compile_raw_args *comp_args;
	duk_uint_t flags;
	duk_hcompfunc *h_templ;

	DUK_CTX_ASSERT_VALID(thr);
	DUK_ASSERT(udata != NULL);

	/* Note: strictness is not inherited from the current Duktape/C
	 * context.  Otherwise it would not be possible to compile
	 * non-strict code inside a Duktape/C activation (which is
	 * always strict now).  See tests/api/test-eval-strictness.c
	 * for discussion.
	 */

	/* [ ... source? filename? ] (depends on flags) */

	comp_args = (duk__compile_raw_args *) udata;
	flags = comp_args->flags;

	if (flags & DUK_COMPILE_NOFILENAME) {
		/* Automatic filename: 'eval' or 'input'. */
		duk_push_hstring_stridx(thr, (flags & DUK_COMPILE_EVAL) ? DUK_STRIDX_EVAL : DUK_STRIDX_INPUT);
	}

	/* [ ... source? filename ] */

	if (!comp_args->src_buffer) {
		duk_hstring *h_sourcecode;

		h_sourcecode = duk_get_hstring(thr, -2);
		if ((flags & DUK_COMPILE_NOSOURCE) || /* args incorrect */
		    (h_sourcecode == NULL)) { /* e.g. duk_push_string_file_raw() pushed undefined */
			DUK_ERROR_TYPE(thr, DUK_STR_NO_SOURCECODE);
			DUK_WO_NORETURN(return 0;);
		}
		DUK_ASSERT(h_sourcecode != NULL);
		comp_args->src_buffer = (const duk_uint8_t *) DUK_HSTRING_GET_DATA(h_sourcecode);
		comp_args->src_length = (duk_size_t) DUK_HSTRING_GET_BYTELEN(h_sourcecode);
	}
	DUK_ASSERT(comp_args->src_buffer != NULL);

	if (flags & DUK_COMPILE_FUNCTION) {
		flags |= DUK_COMPILE_EVAL | DUK_COMPILE_FUNCEXPR;
	}

	/* [ ... source? filename ] */

	duk_js_compile(thr, comp_args->src_buffer, comp_args->src_length, flags);

	/* [ ... source? func_template ] */

	if (flags & DUK_COMPILE_NOSOURCE) {
		;
	} else {
		duk_remove_m2(thr);
	}

	/* [ ... func_template ] */

	h_templ = (duk_hcompfunc *) duk_known_hobject(thr, -1);
	duk_js_push_closure(thr,
	                    h_templ,
	                    thr->builtins[DUK_BIDX_GLOBAL_ENV],
	                    thr->builtins[DUK_BIDX_GLOBAL_ENV],
	                    1 /*add_auto_proto*/);
	duk_remove_m2(thr); /* -> [ ... closure ] */

	/* [ ... closure ] */

	return 1;
}

DUK_EXTERNAL duk_int_t duk_compile_raw(duk_hthread *thr, const char *src_buffer, duk_size_t src_length, duk_uint_t flags) {
	duk__compile_raw_args comp_args_alloc;
	duk__compile_raw_args *comp_args = &comp_args_alloc;

	DUK_ASSERT_API_ENTRY(thr);

	if ((flags & DUK_COMPILE_STRLEN) && (src_buffer != NULL)) {
		/* String length is computed here to avoid multiple evaluation
		 * of a macro argument in the calling side.
		 */
		src_length = DUK_STRLEN(src_buffer);
	}

	comp_args->src_buffer = (const duk_uint8_t *) src_buffer;
	comp_args->src_length = src_length;
	comp_args->flags = flags;

	/* [ ... source? filename? ] (depends on flags) */

	if (flags & DUK_COMPILE_SAFE) {
		duk_int_t rc;
		duk_int_t nargs;
		duk_int_t nrets = 1;

		/* Arguments can be: [ source? filename? &comp_args] so that
		 * nargs is 1 to 3.  Call site encodes the correct nargs count
		 * directly into flags.
		 */
		nargs = flags & 0x07;
		DUK_ASSERT(nargs == ((flags & DUK_COMPILE_NOSOURCE) ? 0 : 1) + ((flags & DUK_COMPILE_NOFILENAME) ? 0 : 1));
		rc = duk_safe_call(thr, duk__do_compile, (void *) comp_args, nargs, nrets);

		/* [ ... closure ] */
		return rc;
	}

	(void) duk__do_compile(thr, (void *) comp_args);

	/* [ ... closure ] */
	return DUK_EXEC_SUCCESS;
}
