/*
 *  Inspection
 */

#include "duk_internal.h"

/* For footprint efficient multiple value setting: arrays are much better than
 * varargs, format string with parsing is often better than string pointer arrays.
 */
DUK_LOCAL void duk__inspect_multiple_uint(duk_hthread *thr, const char *fmt, duk_int_t *vals) {
	duk_int_t val;
	const char *p;
	const char *p_curr;
	duk_size_t len;

	for (p = fmt;;) {
		len = DUK_STRLEN(p);
		p_curr = p;
		p += len + 1;
		if (len == 0) {
			/* Double NUL (= empty key) terminates. */
			break;
		}
		val = *vals++;
		if (val >= 0) {
			/* Negative values are markers to skip key. */
			duk_push_string(thr, p_curr);
			duk_push_int(thr, val);
			duk_put_prop(thr, -3);
		}
	}
}

/* Raw helper to extract internal information / statistics about a value.
 * The return value is an object with properties that are version specific.
 * The properties must not expose anything that would lead to security
 * issues (e.g. exposing compiled function 'data' buffer might be an issue).
 * Currently only counts and sizes and such are given so there shouldn't
 * be security implications.
 */

#define DUK__IDX_TYPE    0
#define DUK__IDX_ITAG    1
#define DUK__IDX_REFC    2
#define DUK__IDX_HBYTES  3
#define DUK__IDX_CLASS   4
#define DUK__IDX_PBYTES  5
#define DUK__IDX_ESIZE   6
#define DUK__IDX_ENEXT   7
#define DUK__IDX_ASIZE   8
#define DUK__IDX_HSIZE   9
#define DUK__IDX_BCBYTES 10
#define DUK__IDX_DBYTES  11
#define DUK__IDX_TSTATE  12
#define DUK__IDX_VARIANT 13

DUK_EXTERNAL void duk_inspect_value(duk_hthread *thr, duk_idx_t idx) {
	duk_tval *tv;
	duk_heaphdr *h;
	/* The temporary values should be in an array rather than individual
	 * variables which (in practice) ensures that the compiler won't map
	 * them to registers and emit a lot of unnecessary shuffling code.
	 */
	duk_int_t vals[14];

	DUK_ASSERT_API_ENTRY(thr);

	/* Assume two's complement and set everything to -1. */
	duk_memset((void *) &vals, (int) 0xff, sizeof(vals));
	DUK_ASSERT(vals[DUK__IDX_TYPE] == -1); /* spot check one */

	tv = duk_get_tval_or_unused(thr, idx);
	h = (DUK_TVAL_IS_HEAP_ALLOCATED(tv) ? DUK_TVAL_GET_HEAPHDR(tv) : NULL);

	vals[DUK__IDX_TYPE] = duk_get_type_tval(tv);
	vals[DUK__IDX_ITAG] = (duk_int_t) DUK_TVAL_GET_TAG(tv);

	duk_push_bare_object(thr); /* Invalidates 'tv'. */
	tv = NULL;

	if (h == NULL) {
		goto finish;
	}
	duk_push_pointer(thr, (void *) h);
	duk_put_prop_literal(thr, -2, "hptr");

#if 0
	/* Covers a lot of information, e.g. buffer and string variants. */
	duk_push_uint(thr, (duk_uint_t) DUK_HEAPHDR_GET_FLAGS(h));
	duk_put_prop_literal(thr, -2, "hflags");
#endif

#if defined(DUK_USE_REFERENCE_COUNTING)
	vals[DUK__IDX_REFC] = (duk_int_t) DUK_HEAPHDR_GET_REFCOUNT(h);
#endif
	vals[DUK__IDX_VARIANT] = 0;

	/* Heaphdr size and additional allocation size, followed by
	 * type specific stuff (with varying value count).
	 */
	switch ((duk_small_int_t) DUK_HEAPHDR_GET_TYPE(h)) {
	case DUK_HTYPE_STRING: {
		duk_hstring *h_str = (duk_hstring *) h;
		vals[DUK__IDX_HBYTES] = (duk_int_t) (sizeof(duk_hstring) + DUK_HSTRING_GET_BYTELEN(h_str) + 1);
#if defined(DUK_USE_HSTRING_EXTDATA)
		if (DUK_HSTRING_HAS_EXTDATA(h_str)) {
			vals[DUK__IDX_VARIANT] = 1;
		}
#endif
		break;
	}
	case DUK_HTYPE_OBJECT: {
		duk_hobject *h_obj = (duk_hobject *) h;

		/* XXX: variants here are maybe pointless; class is enough? */
		if (DUK_HOBJECT_IS_ARRAY(h_obj)) {
			vals[DUK__IDX_HBYTES] = sizeof(duk_harray);
		} else if (DUK_HOBJECT_IS_COMPFUNC(h_obj)) {
			vals[DUK__IDX_HBYTES] = sizeof(duk_hcompfunc);
		} else if (DUK_HOBJECT_IS_NATFUNC(h_obj)) {
			vals[DUK__IDX_HBYTES] = sizeof(duk_hnatfunc);
		} else if (DUK_HOBJECT_IS_THREAD(h_obj)) {
			vals[DUK__IDX_HBYTES] = sizeof(duk_hthread);
			vals[DUK__IDX_TSTATE] = ((duk_hthread *) h_obj)->state;
#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
		} else if (DUK_HOBJECT_IS_BUFOBJ(h_obj)) {
			vals[DUK__IDX_HBYTES] = sizeof(duk_hbufobj);
			/* XXX: some size information */
#endif
		} else {
			vals[DUK__IDX_HBYTES] = (duk_small_uint_t) sizeof(duk_hobject);
		}

		vals[DUK__IDX_CLASS] = (duk_int_t) DUK_HOBJECT_GET_CLASS_NUMBER(h_obj);
		vals[DUK__IDX_PBYTES] = (duk_int_t) DUK_HOBJECT_P_ALLOC_SIZE(h_obj);
		vals[DUK__IDX_ESIZE] = (duk_int_t) DUK_HOBJECT_GET_ESIZE(h_obj);
		vals[DUK__IDX_ENEXT] = (duk_int_t) DUK_HOBJECT_GET_ENEXT(h_obj);
		vals[DUK__IDX_ASIZE] = (duk_int_t) DUK_HOBJECT_GET_ASIZE(h_obj);
		vals[DUK__IDX_HSIZE] = (duk_int_t) DUK_HOBJECT_GET_HSIZE(h_obj);

		/* Note: e_next indicates the number of gc-reachable entries
		 * in the entry part, and also indicates the index where the
		 * next new property would be inserted.  It does *not* indicate
		 * the number of non-NULL keys present in the object.  That
		 * value could be counted separately but requires a pass through
		 * the key list.
		 */

		if (DUK_HOBJECT_IS_COMPFUNC(h_obj)) {
			duk_hbuffer *h_data = (duk_hbuffer *) DUK_HCOMPFUNC_GET_DATA(thr->heap, (duk_hcompfunc *) h_obj);
			vals[DUK__IDX_BCBYTES] = (duk_int_t) (h_data ? DUK_HBUFFER_GET_SIZE(h_data) : 0);
		}
		break;
	}
	case DUK_HTYPE_BUFFER: {
		duk_hbuffer *h_buf = (duk_hbuffer *) h;

		if (DUK_HBUFFER_HAS_DYNAMIC(h_buf)) {
			if (DUK_HBUFFER_HAS_EXTERNAL(h_buf)) {
				vals[DUK__IDX_VARIANT] = 2; /* buffer variant 2: external */
				vals[DUK__IDX_HBYTES] = (duk_uint_t) (sizeof(duk_hbuffer_external));
			} else {
				/* When alloc_size == 0 the second allocation may not
				 * actually exist.
				 */
				vals[DUK__IDX_VARIANT] = 1; /* buffer variant 1: dynamic */
				vals[DUK__IDX_HBYTES] = (duk_uint_t) (sizeof(duk_hbuffer_dynamic));
			}
			vals[DUK__IDX_DBYTES] = (duk_int_t) (DUK_HBUFFER_GET_SIZE(h_buf));
		} else {
			DUK_ASSERT(vals[DUK__IDX_VARIANT] == 0); /* buffer variant 0: fixed */
			vals[DUK__IDX_HBYTES] = (duk_int_t) (sizeof(duk_hbuffer_fixed) + DUK_HBUFFER_GET_SIZE(h_buf));
		}
		break;
	}
	}

finish:
	duk__inspect_multiple_uint(thr,
	                           "type"
	                           "\x00"
	                           "itag"
	                           "\x00"
	                           "refc"
	                           "\x00"
	                           "hbytes"
	                           "\x00"
	                           "class"
	                           "\x00"
	                           "pbytes"
	                           "\x00"
	                           "esize"
	                           "\x00"
	                           "enext"
	                           "\x00"
	                           "asize"
	                           "\x00"
	                           "hsize"
	                           "\x00"
	                           "bcbytes"
	                           "\x00"
	                           "dbytes"
	                           "\x00"
	                           "tstate"
	                           "\x00"
	                           "variant"
	                           "\x00"
	                           "\x00",
	                           (duk_int_t *) &vals);
}

DUK_EXTERNAL void duk_inspect_callstack_entry(duk_hthread *thr, duk_int_t level) {
	duk_activation *act;
	duk_uint_fast32_t pc;
	duk_uint_fast32_t line;

	DUK_ASSERT_API_ENTRY(thr);

	/* -1   = top callstack entry
	 * -2   = caller of level -1
	 * etc
	 */
	act = duk_hthread_get_activation_for_level(thr, level);
	if (act == NULL) {
		duk_push_undefined(thr);
		return;
	}
	duk_push_bare_object(thr);

	/* Relevant PC is just before current one because PC is
	 * post-incremented.  This should match what error augment
	 * code does.
	 */
	pc = duk_hthread_get_act_prev_pc(thr, act);

	duk_push_tval(thr, &act->tv_func);

	duk_push_uint(thr, (duk_uint_t) pc);
	duk_put_prop_stridx_short(thr, -3, DUK_STRIDX_PC);

#if defined(DUK_USE_PC2LINE)
	line = duk_hobject_pc2line_query(thr, -1, pc);
#else
	line = 0;
#endif
	duk_push_uint(thr, (duk_uint_t) line);
	duk_put_prop_stridx_short(thr, -3, DUK_STRIDX_LINE_NUMBER);

	duk_put_prop_stridx_short(thr, -2, DUK_STRIDX_LC_FUNCTION);
	/* Providing access to e.g. act->lex_env would be dangerous: these
	 * internal structures must never be accessible to the application.
	 * Duktape relies on them having consistent data, and this consistency
	 * is only asserted for, not checked for.
	 */
}
