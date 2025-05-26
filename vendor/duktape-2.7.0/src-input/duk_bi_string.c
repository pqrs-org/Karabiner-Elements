/*
 *  String built-ins
 *
 *  Most String built-ins must only accept strings (or String objects).
 *  Symbols, represented internally as strings, must be generally rejected.
 *  The duk_push_this_coercible_to_string() helper does this automatically.
 */

/* XXX: There are several limitations in the current implementation for
 * strings with >= 0x80000000UL characters.  In some cases one would need
 * to be able to represent the range [-0xffffffff,0xffffffff] and so on.
 * Generally character and byte length are assumed to fit into signed 32
 * bits (< 0x80000000UL).  Places with issues are not marked explicitly
 * below in all cases, look for signed type usage (duk_int_t etc) for
 * offsets/lengths.
 */

#include "duk_internal.h"

#if defined(DUK_USE_STRING_BUILTIN)

/*
 *  Helpers
 */

DUK_LOCAL duk_hstring *duk__str_tostring_notregexp(duk_hthread *thr, duk_idx_t idx) {
	duk_hstring *h;

	if (duk_get_class_number(thr, idx) == DUK_HOBJECT_CLASS_REGEXP) {
		DUK_ERROR_TYPE_INVALID_ARGS(thr);
		DUK_WO_NORETURN(return NULL;);
	}
	h = duk_to_hstring(thr, idx);
	DUK_ASSERT(h != NULL);

	return h;
}

DUK_LOCAL duk_int_t
duk__str_search_shared(duk_hthread *thr, duk_hstring *h_this, duk_hstring *h_search, duk_int_t start_cpos, duk_bool_t backwards) {
	duk_int_t cpos;
	duk_int_t bpos;
	const duk_uint8_t *p_start, *p_end, *p;
	const duk_uint8_t *q_start;
	duk_int_t q_blen;
	duk_uint8_t firstbyte;
	duk_uint8_t t;

	cpos = start_cpos;

	/* Empty searchstring always matches; cpos must be clamped here.
	 * (If q_blen were < 0 due to clamped coercion, it would also be
	 * caught here.)
	 */
	q_start = DUK_HSTRING_GET_DATA(h_search);
	q_blen = (duk_int_t) DUK_HSTRING_GET_BYTELEN(h_search);
	if (q_blen <= 0) {
		return cpos;
	}
	DUK_ASSERT(q_blen > 0);

	bpos = (duk_int_t) duk_heap_strcache_offset_char2byte(thr, h_this, (duk_uint32_t) cpos);

	p_start = DUK_HSTRING_GET_DATA(h_this);
	p_end = p_start + DUK_HSTRING_GET_BYTELEN(h_this);
	p = p_start + bpos;

	/* This loop is optimized for size.  For speed, there should be
	 * two separate loops, and we should ensure that memcmp() can be
	 * used without an extra "will searchstring fit" check.  Doing
	 * the preconditioning for 'p' and 'p_end' is easy but cpos
	 * must be updated if 'p' is wound back (backward scanning).
	 */

	firstbyte = q_start[0]; /* leading byte of match string */
	while (p <= p_end && p >= p_start) {
		t = *p;

		/* For ECMAScript strings, this check can only match for
		 * initial UTF-8 bytes (not continuation bytes).  For other
		 * strings all bets are off.
		 */

		if ((t == firstbyte) && ((duk_size_t) (p_end - p) >= (duk_size_t) q_blen)) {
			DUK_ASSERT(q_blen > 0);
			if (duk_memcmp((const void *) p, (const void *) q_start, (size_t) q_blen) == 0) {
				return cpos;
			}
		}

		/* track cpos while scanning */
		if (backwards) {
			/* when going backwards, we decrement cpos 'early';
			 * 'p' may point to a continuation byte of the char
			 * at offset 'cpos', but that's OK because we'll
			 * backtrack all the way to the initial byte.
			 */
			if ((t & 0xc0) != 0x80) {
				cpos--;
			}
			p--;
		} else {
			if ((t & 0xc0) != 0x80) {
				cpos++;
			}
			p++;
		}
	}

	/* Not found.  Empty string case is handled specially above. */
	return -1;
}

/*
 *  Constructor
 */

DUK_INTERNAL duk_ret_t duk_bi_string_constructor(duk_hthread *thr) {
	duk_hstring *h;
	duk_uint_t flags;

	/* String constructor needs to distinguish between an argument not given at all
	 * vs. given as 'undefined'.  We're a vararg function to handle this properly.
	 */

	/* XXX: copy current activation flags to thr, including current magic,
	 * is_constructor_call etc.  This takes a few bytes in duk_hthread but
	 * makes call sites smaller (there are >30 is_constructor_call and get
	 * current magic call sites.
	 */

	if (duk_get_top(thr) == 0) {
		duk_push_hstring_empty(thr);
	} else {
		h = duk_to_hstring_acceptsymbol(thr, 0);
		if (DUK_UNLIKELY(DUK_HSTRING_HAS_SYMBOL(h) && !duk_is_constructor_call(thr))) {
			duk_push_symbol_descriptive_string(thr, h);
			duk_replace(thr, 0);
		}
	}
	duk_to_string(thr, 0); /* catches symbol argument for constructor call */
	DUK_ASSERT(duk_is_string(thr, 0));
	duk_set_top(thr, 1); /* Top may be 1 or larger. */

	if (duk_is_constructor_call(thr)) {
		/* String object internal value is immutable */
		flags = DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_FASTREFS | DUK_HOBJECT_FLAG_EXOTIC_STRINGOBJ |
		        DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_STRING);
		duk_push_object_helper(thr, flags, DUK_BIDX_STRING_PROTOTYPE);
		duk_dup_0(thr);
		duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_INT_VALUE, DUK_PROPDESC_FLAGS_NONE);
	}
	/* Note: unbalanced stack on purpose */

	return 1;
}

DUK_LOCAL duk_ret_t duk__construct_from_codepoints(duk_hthread *thr, duk_bool_t nonbmp) {
	duk_bufwriter_ctx bw_alloc;
	duk_bufwriter_ctx *bw;
	duk_idx_t i, n;
	duk_ucodepoint_t cp;

	/* XXX: It would be nice to build the string directly but ToUint16()
	 * coercion is needed so a generic helper would not be very
	 * helpful (perhaps coerce the value stack first here and then
	 * build a string from a duk_tval number sequence in one go?).
	 */

	n = duk_get_top(thr);

	bw = &bw_alloc;
	DUK_BW_INIT_PUSHBUF(thr, bw, (duk_size_t) n); /* initial estimate for ASCII only codepoints */

	for (i = 0; i < n; i++) {
		/* XXX: could improve bufwriter handling to write multiple codepoints
		 * with one ensure call but the relative benefit would be quite small.
		 */

		if (nonbmp) {
			/* ES2015 requires that (1) SameValue(cp, ToInteger(cp)) and
			 * (2) cp >= 0 and cp <= 0x10ffff.  This check does not
			 * implement the steps exactly but the outcome should be
			 * the same.
			 */
			duk_int32_t i32 = 0;
			if (!duk_is_whole_get_int32(duk_to_number(thr, i), &i32) || i32 < 0 || i32 > 0x10ffffL) {
				DUK_DCERROR_RANGE_INVALID_ARGS(thr);
			}
			DUK_ASSERT(i32 >= 0 && i32 <= 0x10ffffL);
			cp = (duk_ucodepoint_t) i32;
			DUK_BW_WRITE_ENSURE_CESU8(thr, bw, cp);
		} else {
#if defined(DUK_USE_NONSTD_STRING_FROMCHARCODE_32BIT)
			/* ToUint16() coercion is mandatory in the E5.1 specification, but
			 * this non-compliant behavior makes more sense because we support
			 * non-BMP codepoints.  Don't use CESU-8 because that'd create
			 * surrogate pairs.
			 */
			cp = (duk_ucodepoint_t) duk_to_uint32(thr, i);
			DUK_BW_WRITE_ENSURE_XUTF8(thr, bw, cp);
#else
			cp = (duk_ucodepoint_t) duk_to_uint16(thr, i);
			DUK_ASSERT(cp >= 0 && cp <= 0x10ffffL);
			DUK_BW_WRITE_ENSURE_CESU8(thr, bw, cp);
#endif
		}
	}

	DUK_BW_COMPACT(thr, bw);
	(void) duk_buffer_to_string(thr, -1); /* Safe, extended UTF-8 or CESU-8 encoded. */
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_string_constructor_from_char_code(duk_hthread *thr) {
	return duk__construct_from_codepoints(thr, 0 /*nonbmp*/);
}

#if defined(DUK_USE_ES6)
DUK_INTERNAL duk_ret_t duk_bi_string_constructor_from_code_point(duk_hthread *thr) {
	return duk__construct_from_codepoints(thr, 1 /*nonbmp*/);
}
#endif

/*
 *  toString(), valueOf()
 */

DUK_INTERNAL duk_ret_t duk_bi_string_prototype_to_string(duk_hthread *thr) {
	duk_tval *tv;

	duk_push_this(thr);
	tv = duk_require_tval(thr, -1);
	DUK_ASSERT(tv != NULL);

	if (DUK_TVAL_IS_STRING(tv)) {
		/* return as is */
	} else if (DUK_TVAL_IS_OBJECT(tv)) {
		duk_hobject *h = DUK_TVAL_GET_OBJECT(tv);
		DUK_ASSERT(h != NULL);

		/* Must be a "string object", i.e. class "String" */
		if (DUK_HOBJECT_GET_CLASS_NUMBER(h) != DUK_HOBJECT_CLASS_STRING) {
			goto type_error;
		}

		duk_xget_owndataprop_stridx_short(thr, -1, DUK_STRIDX_INT_VALUE);
		DUK_ASSERT(duk_is_string(thr, -1));
	} else {
		goto type_error;
	}

	(void) duk_require_hstring_notsymbol(thr, -1); /* Reject symbols (and wrapped symbols). */
	return 1;

type_error:
	DUK_DCERROR_TYPE_INVALID_ARGS(thr);
}

/*
 *  Character and charcode access
 */

DUK_INTERNAL duk_ret_t duk_bi_string_prototype_char_at(duk_hthread *thr) {
	duk_hstring *h;
	duk_int_t pos;

	/* XXX: faster implementation */

	h = duk_push_this_coercible_to_string(thr);
	DUK_ASSERT(h != NULL);

	pos = duk_to_int(thr, 0);

	if (sizeof(duk_size_t) >= sizeof(duk_uint_t)) {
		/* Cast to duk_size_t works in this case:
		 * - If pos < 0, (duk_size_t) pos will always be
		 *   >= max_charlen, and result will be the empty string
		 *   (see duk_substring()).
		 * - If pos >= 0, pos + 1 cannot wrap.
		 */
		DUK_ASSERT((duk_size_t) DUK_INT_MIN >= DUK_HSTRING_MAX_BYTELEN);
		DUK_ASSERT((duk_size_t) DUK_INT_MAX + 1U > (duk_size_t) DUK_INT_MAX);
		duk_substring(thr, -1, (duk_size_t) pos, (duk_size_t) pos + 1U);
	} else {
		/* If size_t is smaller than int, explicit bounds checks
		 * are needed because an int may wrap multiple times.
		 */
		if (DUK_UNLIKELY(pos < 0 || (duk_uint_t) pos >= (duk_uint_t) DUK_HSTRING_GET_CHARLEN(h))) {
			duk_push_hstring_empty(thr);
		} else {
			duk_substring(thr, -1, (duk_size_t) pos, (duk_size_t) pos + 1U);
		}
	}

	return 1;
}

/* Magic: 0=charCodeAt, 1=codePointAt */
DUK_INTERNAL duk_ret_t duk_bi_string_prototype_char_code_at(duk_hthread *thr) {
	duk_int_t pos;
	duk_hstring *h;
	duk_bool_t clamped;
	duk_uint32_t cp;
	duk_int_t magic;

	/* XXX: faster implementation */

	DUK_DDD(DUK_DDDPRINT("arg=%!T", (duk_tval *) duk_get_tval(thr, 0)));

	h = duk_push_this_coercible_to_string(thr);
	DUK_ASSERT(h != NULL);

	pos = duk_to_int_clamped_raw(thr,
	                             0 /*index*/,
	                             0 /*min(incl)*/,
	                             (duk_int_t) DUK_HSTRING_GET_CHARLEN(h) - 1 /*max(incl)*/,
	                             &clamped /*out_clamped*/);
#if defined(DUK_USE_ES6)
	magic = duk_get_current_magic(thr);
#else
	DUK_ASSERT(duk_get_current_magic(thr) == 0);
	magic = 0;
#endif
	if (clamped) {
		/* For out-of-bounds indices .charCodeAt() returns NaN and
		 * .codePointAt() returns undefined.
		 */
		if (magic != 0) {
			return 0;
		}
		duk_push_nan(thr);
	} else {
		DUK_ASSERT(pos >= 0);
		cp = (duk_uint32_t) duk_hstring_char_code_at_raw(thr, h, (duk_uint_t) pos, (duk_bool_t) magic /*surrogate_aware*/);
		duk_push_u32(thr, cp);
	}
	return 1;
}

/*
 *  substring(), substr(), slice()
 */

/* XXX: any chance of merging these three similar but still slightly
 * different algorithms so that footprint would be reduced?
 */

DUK_INTERNAL duk_ret_t duk_bi_string_prototype_substring(duk_hthread *thr) {
	duk_hstring *h;
	duk_int_t start_pos, end_pos;
	duk_int_t len;

	h = duk_push_this_coercible_to_string(thr);
	DUK_ASSERT(h != NULL);
	len = (duk_int_t) DUK_HSTRING_GET_CHARLEN(h);

	/* [ start end str ] */

	start_pos = duk_to_int_clamped(thr, 0, 0, len);
	if (duk_is_undefined(thr, 1)) {
		end_pos = len;
	} else {
		end_pos = duk_to_int_clamped(thr, 1, 0, len);
	}
	DUK_ASSERT(start_pos >= 0 && start_pos <= len);
	DUK_ASSERT(end_pos >= 0 && end_pos <= len);

	if (start_pos > end_pos) {
		duk_int_t tmp = start_pos;
		start_pos = end_pos;
		end_pos = tmp;
	}

	DUK_ASSERT(end_pos >= start_pos);

	duk_substring(thr, -1, (duk_size_t) start_pos, (duk_size_t) end_pos);
	return 1;
}

#if defined(DUK_USE_SECTION_B)
DUK_INTERNAL duk_ret_t duk_bi_string_prototype_substr(duk_hthread *thr) {
	duk_hstring *h;
	duk_int_t start_pos, end_pos;
	duk_int_t len;

	/* Unlike non-obsolete String calls, substr() algorithm in E5.1
	 * specification will happily coerce undefined and null to strings
	 * ("undefined" and "null").
	 */
	duk_push_this(thr);
	h = duk_to_hstring_m1(thr); /* Reject Symbols. */
	DUK_ASSERT(h != NULL);
	len = (duk_int_t) DUK_HSTRING_GET_CHARLEN(h);

	/* [ start length str ] */

	/* The implementation for computing of start_pos and end_pos differs
	 * from the standard algorithm, but is intended to result in the exactly
	 * same behavior.  This is not always obvious.
	 */

	/* combines steps 2 and 5; -len ensures max() not needed for step 5 */
	start_pos = duk_to_int_clamped(thr, 0, -len, len);
	if (start_pos < 0) {
		start_pos = len + start_pos;
	}
	DUK_ASSERT(start_pos >= 0 && start_pos <= len);

	/* combines steps 3, 6; step 7 is not needed */
	if (duk_is_undefined(thr, 1)) {
		end_pos = len;
	} else {
		DUK_ASSERT(start_pos <= len);
		end_pos = start_pos + duk_to_int_clamped(thr, 1, 0, len - start_pos);
	}
	DUK_ASSERT(start_pos >= 0 && start_pos <= len);
	DUK_ASSERT(end_pos >= 0 && end_pos <= len);
	DUK_ASSERT(end_pos >= start_pos);

	duk_substring(thr, -1, (duk_size_t) start_pos, (duk_size_t) end_pos);
	return 1;
}
#endif /* DUK_USE_SECTION_B */

DUK_INTERNAL duk_ret_t duk_bi_string_prototype_slice(duk_hthread *thr) {
	duk_hstring *h;
	duk_int_t start_pos, end_pos;
	duk_int_t len;

	h = duk_push_this_coercible_to_string(thr);
	DUK_ASSERT(h != NULL);
	len = (duk_int_t) DUK_HSTRING_GET_CHARLEN(h);

	/* [ start end str ] */

	start_pos = duk_to_int_clamped(thr, 0, -len, len);
	if (start_pos < 0) {
		start_pos = len + start_pos;
	}
	if (duk_is_undefined(thr, 1)) {
		end_pos = len;
	} else {
		end_pos = duk_to_int_clamped(thr, 1, -len, len);
		if (end_pos < 0) {
			end_pos = len + end_pos;
		}
	}
	DUK_ASSERT(start_pos >= 0 && start_pos <= len);
	DUK_ASSERT(end_pos >= 0 && end_pos <= len);

	if (end_pos < start_pos) {
		end_pos = start_pos;
	}

	DUK_ASSERT(end_pos >= start_pos);

	duk_substring(thr, -1, (duk_size_t) start_pos, (duk_size_t) end_pos);
	return 1;
}

/*
 *  Case conversion
 */

DUK_INTERNAL duk_ret_t duk_bi_string_prototype_caseconv_shared(duk_hthread *thr) {
	duk_small_int_t uppercase = duk_get_current_magic(thr);

	(void) duk_push_this_coercible_to_string(thr);
	duk_unicode_case_convert_string(thr, (duk_bool_t) uppercase);
	return 1;
}

/*
 *  indexOf() and lastIndexOf()
 */

DUK_INTERNAL duk_ret_t duk_bi_string_prototype_indexof_shared(duk_hthread *thr) {
	duk_hstring *h_this;
	duk_hstring *h_search;
	duk_int_t clen_this;
	duk_int_t cpos;
	duk_small_uint_t is_lastindexof = (duk_small_uint_t) duk_get_current_magic(thr); /* 0=indexOf, 1=lastIndexOf */

	h_this = duk_push_this_coercible_to_string(thr);
	DUK_ASSERT(h_this != NULL);
	clen_this = (duk_int_t) DUK_HSTRING_GET_CHARLEN(h_this);

	h_search = duk_to_hstring(thr, 0);
	DUK_ASSERT(h_search != NULL);

	duk_to_number(thr, 1);
	if (duk_is_nan(thr, 1) && is_lastindexof) {
		/* indexOf: NaN should cause pos to be zero.
		 * lastIndexOf: NaN should cause pos to be +Infinity
		 * (and later be clamped to len).
		 */
		cpos = clen_this;
	} else {
		cpos = duk_to_int_clamped(thr, 1, 0, clen_this);
	}

	cpos = duk__str_search_shared(thr, h_this, h_search, cpos, is_lastindexof /*backwards*/);
	duk_push_int(thr, cpos);
	return 1;
}

/*
 *  replace()
 */

/* XXX: the current implementation works but is quite clunky; it compiles
 * to almost 1,4kB of x86 code so it needs to be simplified (better approach,
 * shared helpers, etc).  Some ideas for refactoring:
 *
 * - a primitive to convert a string into a regexp matcher (reduces matching
 *   code at the cost of making matching much slower)
 * - use replace() as a basic helper for match() and split(), which are both
 *   much simpler
 * - API call to get_prop and to_boolean
 */

DUK_INTERNAL duk_ret_t duk_bi_string_prototype_replace(duk_hthread *thr) {
	duk_hstring *h_input;
	duk_hstring *h_match;
	duk_hstring *h_search;
	duk_hobject *h_re;
	duk_bufwriter_ctx bw_alloc;
	duk_bufwriter_ctx *bw;
#if defined(DUK_USE_REGEXP_SUPPORT)
	duk_bool_t is_regexp;
	duk_bool_t is_global;
#endif
	duk_bool_t is_repl_func;
	duk_uint32_t match_start_coff, match_start_boff;
#if defined(DUK_USE_REGEXP_SUPPORT)
	duk_int_t match_caps;
#endif
	duk_uint32_t prev_match_end_boff;
	const duk_uint8_t *r_start, *r_end, *r; /* repl string scan */
	duk_size_t tmp_sz;

	DUK_ASSERT_TOP(thr, 2);
	h_input = duk_push_this_coercible_to_string(thr);
	DUK_ASSERT(h_input != NULL);

	bw = &bw_alloc;
	DUK_BW_INIT_PUSHBUF(thr, bw, DUK_HSTRING_GET_BYTELEN(h_input)); /* input size is good output starting point */

	DUK_ASSERT_TOP(thr, 4);

	/* stack[0] = search value
	 * stack[1] = replace value
	 * stack[2] = input string
	 * stack[3] = result buffer
	 */

	h_re = duk_get_hobject_with_class(thr, 0, DUK_HOBJECT_CLASS_REGEXP);
	if (h_re) {
#if defined(DUK_USE_REGEXP_SUPPORT)
		is_regexp = 1;
		is_global = duk_get_prop_stridx_boolean(thr, 0, DUK_STRIDX_GLOBAL, NULL);

		if (is_global) {
			/* start match from beginning */
			duk_push_int(thr, 0);
			duk_put_prop_stridx_short(thr, 0, DUK_STRIDX_LAST_INDEX);
		}
#else /* DUK_USE_REGEXP_SUPPORT */
		DUK_DCERROR_UNSUPPORTED(thr);
#endif /* DUK_USE_REGEXP_SUPPORT */
	} else {
		duk_to_string(thr, 0); /* rejects symbols */
#if defined(DUK_USE_REGEXP_SUPPORT)
		is_regexp = 0;
		is_global = 0;
#endif
	}

	if (duk_is_function(thr, 1)) {
		is_repl_func = 1;
		r_start = NULL;
		r_end = NULL;
	} else {
		duk_hstring *h_repl;

		is_repl_func = 0;
		h_repl = duk_to_hstring(thr, 1); /* reject symbols */
		DUK_ASSERT(h_repl != NULL);
		r_start = DUK_HSTRING_GET_DATA(h_repl);
		r_end = r_start + DUK_HSTRING_GET_BYTELEN(h_repl);
	}

	prev_match_end_boff = 0;

	for (;;) {
		/*
		 *  If matching with a regexp:
		 *    - non-global RegExp: lastIndex not touched on a match, zeroed
		 *      on a non-match
		 *    - global RegExp: on match, lastIndex will be updated by regexp
		 *      executor to point to next char after the matching part (so that
		 *      characters in the matching part are not matched again)
		 *
		 *  If matching with a string:
		 *    - always non-global match, find first occurrence
		 *
		 *  We need:
		 *    - The character offset of start-of-match for the replacer function
		 *    - The byte offsets for start-of-match and end-of-match to implement
		 *      the replacement values $&, $`, and $', and to copy non-matching
		 *      input string portions (including header and trailer) verbatim.
		 *
		 *  NOTE: the E5.1 specification is a bit vague how the RegExp should
		 *  behave in the replacement process; e.g. is matching done first for
		 *  all matches (in the global RegExp case) before any replacer calls
		 *  are made?  See: test-bi-string-proto-replace.js for discussion.
		 */

		DUK_ASSERT_TOP(thr, 4);

#if defined(DUK_USE_REGEXP_SUPPORT)
		if (is_regexp) {
			duk_dup_0(thr);
			duk_dup_2(thr);
			duk_regexp_match(thr); /* [ ... regexp input ] -> [ res_obj ] */
			if (!duk_is_object(thr, -1)) {
				duk_pop(thr);
				break;
			}

			duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_INDEX);
			DUK_ASSERT(duk_is_number(thr, -1));
			match_start_coff = duk_get_uint(thr, -1);
			duk_pop(thr);

			duk_get_prop_index(thr, -1, 0);
			DUK_ASSERT(duk_is_string(thr, -1));
			h_match = duk_known_hstring(thr, -1);
			duk_pop(thr); /* h_match is borrowed, remains reachable through match_obj */

			if (DUK_HSTRING_GET_BYTELEN(h_match) == 0) {
				/* This should be equivalent to match() algorithm step 8.f.iii.2:
				 * detect an empty match and allow it, but don't allow it twice.
				 */
				duk_uint32_t last_index;

				duk_get_prop_stridx_short(thr, 0, DUK_STRIDX_LAST_INDEX);
				last_index = (duk_uint32_t) duk_get_uint(thr, -1);
				DUK_DDD(DUK_DDDPRINT("empty match, bump lastIndex: %ld -> %ld",
				                     (long) last_index,
				                     (long) (last_index + 1)));
				duk_pop(thr);
				duk_push_uint(thr, (duk_uint_t) (last_index + 1));
				duk_put_prop_stridx_short(thr, 0, DUK_STRIDX_LAST_INDEX);
			}

			DUK_ASSERT(duk_get_length(thr, -1) <= DUK_INT_MAX); /* string limits */
			match_caps = (duk_int_t) duk_get_length(thr, -1);
		} else {
#else /* DUK_USE_REGEXP_SUPPORT */
		{ /* unconditionally */
#endif /* DUK_USE_REGEXP_SUPPORT */
			const duk_uint8_t *p_start, *p_end, *p; /* input string scan */
			const duk_uint8_t *q_start; /* match string */
			duk_size_t p_blen;
			duk_size_t q_blen;

#if defined(DUK_USE_REGEXP_SUPPORT)
			DUK_ASSERT(!is_global); /* single match always */
#endif

			p_start = DUK_HSTRING_GET_DATA(h_input);
			p_end = p_start + DUK_HSTRING_GET_BYTELEN(h_input);
			p_blen = (duk_size_t) DUK_HSTRING_GET_BYTELEN(h_input);
			p = p_start;

			h_search = duk_known_hstring(thr, 0);
			q_start = DUK_HSTRING_GET_DATA(h_search);
			q_blen = (duk_size_t) DUK_HSTRING_GET_BYTELEN(h_search);

			if (q_blen > p_blen) {
				break; /* no match */
			}

			p_end -= q_blen; /* ensure full memcmp() fits in while */
			DUK_ASSERT(p_end >= p);

			match_start_coff = 0;

			while (p <= p_end) {
				DUK_ASSERT(p + q_blen <= DUK_HSTRING_GET_DATA(h_input) + DUK_HSTRING_GET_BYTELEN(h_input));
				if (duk_memcmp((const void *) p, (const void *) q_start, (size_t) q_blen) == 0) {
					duk_dup_0(thr);
					h_match = duk_known_hstring(thr, -1);
#if defined(DUK_USE_REGEXP_SUPPORT)
					match_caps = 0;
#endif
					goto found;
				}

				/* track utf-8 non-continuation bytes */
				if ((p[0] & 0xc0) != 0x80) {
					match_start_coff++;
				}
				p++;
			}

			/* not found */
			break;
		}
	found:

		/* stack[0] = search value
		 * stack[1] = replace value
		 * stack[2] = input string
		 * stack[3] = result buffer
		 * stack[4] = regexp match OR match string
		 */

		match_start_boff = (duk_uint32_t) duk_heap_strcache_offset_char2byte(thr, h_input, match_start_coff);

		tmp_sz = (duk_size_t) (match_start_boff - prev_match_end_boff);
		DUK_BW_WRITE_ENSURE_BYTES(thr, bw, DUK_HSTRING_GET_DATA(h_input) + prev_match_end_boff, tmp_sz);

		prev_match_end_boff = match_start_boff + DUK_HSTRING_GET_BYTELEN(h_match);

		if (is_repl_func) {
			duk_idx_t idx_args;
			duk_hstring *h_repl;

			/* regexp res_obj is at index 4 */

			duk_dup_1(thr);
			idx_args = duk_get_top(thr);

#if defined(DUK_USE_REGEXP_SUPPORT)
			if (is_regexp) {
				duk_int_t idx;
				duk_require_stack(thr, match_caps + 2);
				for (idx = 0; idx < match_caps; idx++) {
					/* match followed by capture(s) */
					duk_get_prop_index(thr, 4, (duk_uarridx_t) idx);
				}
			} else {
#else /* DUK_USE_REGEXP_SUPPORT */
			{ /* unconditionally */
#endif /* DUK_USE_REGEXP_SUPPORT */
				/* match == search string, by definition */
				duk_dup_0(thr);
			}
			duk_push_uint(thr, (duk_uint_t) match_start_coff);
			duk_dup_2(thr);

			/* [ ... replacer match [captures] match_char_offset input ] */

			duk_call(thr, duk_get_top(thr) - idx_args);
			h_repl = duk_to_hstring_m1(thr); /* -> [ ... repl_value ] */
			DUK_ASSERT(h_repl != NULL);

			DUK_BW_WRITE_ENSURE_HSTRING(thr, bw, h_repl);

			duk_pop(thr); /* repl_value */
		} else {
			r = r_start;

			while (r < r_end) {
				duk_int_t ch1;
				duk_int_t ch2;
#if defined(DUK_USE_REGEXP_SUPPORT)
				duk_int_t ch3;
#endif
				duk_size_t left;

				ch1 = *r++;
				if (ch1 != DUK_ASC_DOLLAR) {
					goto repl_write;
				}
				DUK_ASSERT(r <= r_end);
				left = (duk_size_t) (r_end - r);

				if (left <= 0) {
					goto repl_write;
				}

				ch2 = r[0];
				switch (ch2) {
				case DUK_ASC_DOLLAR: {
					ch1 = (1 << 8) + DUK_ASC_DOLLAR;
					goto repl_write;
				}
				case DUK_ASC_AMP: {
					DUK_BW_WRITE_ENSURE_HSTRING(thr, bw, h_match);
					r++;
					continue;
				}
				case DUK_ASC_GRAVE: {
					tmp_sz = (duk_size_t) match_start_boff;
					DUK_BW_WRITE_ENSURE_BYTES(thr, bw, DUK_HSTRING_GET_DATA(h_input), tmp_sz);
					r++;
					continue;
				}
				case DUK_ASC_SINGLEQUOTE: {
					duk_uint32_t match_end_boff;

					/* Use match charlen instead of bytelen, just in case the input and
					 * match codepoint encodings would have different lengths.
					 */
					/* XXX: charlen computed here, and also in char2byte helper. */
					match_end_boff = (duk_uint32_t) duk_heap_strcache_offset_char2byte(
					    thr,
					    h_input,
					    match_start_coff + (duk_uint_fast32_t) DUK_HSTRING_GET_CHARLEN(h_match));

					tmp_sz = (duk_size_t) (DUK_HSTRING_GET_BYTELEN(h_input) - match_end_boff);
					DUK_BW_WRITE_ENSURE_BYTES(thr, bw, DUK_HSTRING_GET_DATA(h_input) + match_end_boff, tmp_sz);
					r++;
					continue;
				}
				default: {
#if defined(DUK_USE_REGEXP_SUPPORT)
					duk_int_t capnum, captmp, capadv;
					/* XXX: optional check, match_caps is zero if no regexp,
					 * so dollar will be interpreted literally anyway.
					 */

					if (!is_regexp) {
						goto repl_write;
					}

					if (!(ch2 >= DUK_ASC_0 && ch2 <= DUK_ASC_9)) {
						goto repl_write;
					}
					capnum = ch2 - DUK_ASC_0;
					capadv = 1;

					if (left >= 2) {
						ch3 = r[1];
						if (ch3 >= DUK_ASC_0 && ch3 <= DUK_ASC_9) {
							captmp = capnum * 10 + (ch3 - DUK_ASC_0);
							if (captmp < match_caps) {
								capnum = captmp;
								capadv = 2;
							}
						}
					}

					if (capnum > 0 && capnum < match_caps) {
						DUK_ASSERT(is_regexp != 0); /* match_caps == 0 without regexps */

						/* regexp res_obj is at offset 4 */
						duk_get_prop_index(thr, 4, (duk_uarridx_t) capnum);
						if (duk_is_string(thr, -1)) {
							duk_hstring *h_tmp_str;

							h_tmp_str = duk_known_hstring(thr, -1);

							DUK_BW_WRITE_ENSURE_HSTRING(thr, bw, h_tmp_str);
						} else {
							/* undefined -> skip (replaced with empty) */
						}
						duk_pop(thr);
						r += capadv;
						continue;
					} else {
						goto repl_write;
					}
#else /* DUK_USE_REGEXP_SUPPORT */
					goto repl_write; /* unconditionally */
#endif /* DUK_USE_REGEXP_SUPPORT */
				} /* default case */
				} /* switch (ch2) */

			repl_write:
				/* ch1 = (r_increment << 8) + byte */

				DUK_BW_WRITE_ENSURE_U8(thr, bw, (duk_uint8_t) (ch1 & 0xff));
				r += ch1 >> 8;
			} /* while repl */
		} /* if (is_repl_func) */

		duk_pop(thr); /* pop regexp res_obj or match string */

#if defined(DUK_USE_REGEXP_SUPPORT)
		if (!is_global) {
#else
		{ /* unconditionally; is_global==0 */
#endif
			break;
		}
	}

	/* trailer */
	tmp_sz = (duk_size_t) (DUK_HSTRING_GET_BYTELEN(h_input) - prev_match_end_boff);
	DUK_BW_WRITE_ENSURE_BYTES(thr, bw, DUK_HSTRING_GET_DATA(h_input) + prev_match_end_boff, tmp_sz);

	DUK_ASSERT_TOP(thr, 4);
	DUK_BW_COMPACT(thr, bw);
	(void) duk_buffer_to_string(thr, -1); /* Safe if inputs are safe. */
	return 1;
}

/*
 *  split()
 */

/* XXX: very messy now, but works; clean up, remove unused variables (nomimally
 * used so compiler doesn't complain).
 */

DUK_INTERNAL duk_ret_t duk_bi_string_prototype_split(duk_hthread *thr) {
	duk_hstring *h_input;
	duk_hstring *h_sep;
	duk_uint32_t limit;
	duk_uint32_t arr_idx;
#if defined(DUK_USE_REGEXP_SUPPORT)
	duk_bool_t is_regexp;
#endif
	duk_bool_t matched; /* set to 1 if any match exists (needed for empty input special case) */
	duk_uint32_t prev_match_end_coff, prev_match_end_boff;
	duk_uint32_t match_start_boff, match_start_coff;
	duk_uint32_t match_end_boff, match_end_coff;

	h_input = duk_push_this_coercible_to_string(thr);
	DUK_ASSERT(h_input != NULL);

	duk_push_array(thr);

	if (duk_is_undefined(thr, 1)) {
		limit = 0xffffffffUL;
	} else {
		limit = duk_to_uint32(thr, 1);
	}

	if (limit == 0) {
		return 1;
	}

	/* If the separator is a RegExp, make a "clone" of it.  The specification
	 * algorithm calls [[Match]] directly for specific indices; we emulate this
	 * by tweaking lastIndex and using a "force global" variant of duk_regexp_match()
	 * which will use global-style matching even when the RegExp itself is non-global.
	 */

	if (duk_is_undefined(thr, 0)) {
		/* The spec algorithm first does "R = ToString(separator)" before checking
		 * whether separator is undefined.  Since this is side effect free, we can
		 * skip the ToString() here.
		 */
		duk_dup_2(thr);
		duk_put_prop_index(thr, 3, 0);
		return 1;
	} else if (duk_get_hobject_with_class(thr, 0, DUK_HOBJECT_CLASS_REGEXP) != NULL) {
#if defined(DUK_USE_REGEXP_SUPPORT)
		duk_push_hobject_bidx(thr, DUK_BIDX_REGEXP_CONSTRUCTOR);
		duk_dup_0(thr);
		duk_new(thr, 1); /* [ ... RegExp val ] -> [ ... res ] */
		duk_replace(thr, 0);
		/* lastIndex is initialized to zero by new RegExp() */
		is_regexp = 1;
#else
		DUK_DCERROR_UNSUPPORTED(thr);
#endif
	} else {
		duk_to_string(thr, 0);
#if defined(DUK_USE_REGEXP_SUPPORT)
		is_regexp = 0;
#endif
	}

	/* stack[0] = separator (string or regexp)
	 * stack[1] = limit
	 * stack[2] = input string
	 * stack[3] = result array
	 */

	prev_match_end_boff = 0;
	prev_match_end_coff = 0;
	arr_idx = 0;
	matched = 0;

	for (;;) {
		/*
		 *  The specification uses RegExp [[Match]] to attempt match at specific
		 *  offsets.  We don't have such a primitive, so we use an actual RegExp
		 *  and tweak lastIndex.  Since the RegExp may be non-global, we use a
		 *  special variant which forces global-like behavior for matching.
		 */

		DUK_ASSERT_TOP(thr, 4);

#if defined(DUK_USE_REGEXP_SUPPORT)
		if (is_regexp) {
			duk_dup_0(thr);
			duk_dup_2(thr);
			duk_regexp_match_force_global(thr); /* [ ... regexp input ] -> [ res_obj ] */
			if (!duk_is_object(thr, -1)) {
				duk_pop(thr);
				break;
			}
			matched = 1;

			duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_INDEX);
			DUK_ASSERT(duk_is_number(thr, -1));
			match_start_coff = duk_get_uint(thr, -1);
			match_start_boff = (duk_uint32_t) duk_heap_strcache_offset_char2byte(thr, h_input, match_start_coff);
			duk_pop(thr);

			if (match_start_coff == DUK_HSTRING_GET_CHARLEN(h_input)) {
				/* don't allow an empty match at the end of the string */
				duk_pop(thr);
				break;
			}

			duk_get_prop_stridx_short(thr, 0, DUK_STRIDX_LAST_INDEX);
			DUK_ASSERT(duk_is_number(thr, -1));
			match_end_coff = duk_get_uint(thr, -1);
			match_end_boff = (duk_uint32_t) duk_heap_strcache_offset_char2byte(thr, h_input, match_end_coff);
			duk_pop(thr);

			/* empty match -> bump and continue */
			if (prev_match_end_boff == match_end_boff) {
				duk_push_uint(thr, (duk_uint_t) (match_end_coff + 1));
				duk_put_prop_stridx_short(thr, 0, DUK_STRIDX_LAST_INDEX);
				duk_pop(thr);
				continue;
			}
		} else {
#else /* DUK_USE_REGEXP_SUPPORT */
		{ /* unconditionally */
#endif /* DUK_USE_REGEXP_SUPPORT */
			const duk_uint8_t *p_start, *p_end, *p; /* input string scan */
			const duk_uint8_t *q_start; /* match string */
			duk_size_t q_blen, q_clen;

			p_start = DUK_HSTRING_GET_DATA(h_input);
			p_end = p_start + DUK_HSTRING_GET_BYTELEN(h_input);
			p = p_start + prev_match_end_boff;

			h_sep = duk_known_hstring(thr, 0); /* symbol already rejected above */
			q_start = DUK_HSTRING_GET_DATA(h_sep);
			q_blen = (duk_size_t) DUK_HSTRING_GET_BYTELEN(h_sep);
			q_clen = (duk_size_t) DUK_HSTRING_GET_CHARLEN(h_sep);

			p_end -= q_blen; /* ensure full memcmp() fits in while */

			match_start_coff = prev_match_end_coff;

			if (q_blen == 0) {
				/* Handle empty separator case: it will always match, and always
				 * triggers the check in step 13.c.iii initially.  Note that we
				 * must skip to either end of string or start of first codepoint,
				 * skipping over any continuation bytes!
				 *
				 * Don't allow an empty string to match at the end of the input.
				 */

				matched = 1; /* empty separator can always match */

				match_start_coff++;
				p++;
				while (p < p_end) {
					if ((p[0] & 0xc0) != 0x80) {
						goto found;
					}
					p++;
				}
				goto not_found;
			}

			DUK_ASSERT(q_blen > 0 && q_clen > 0);
			while (p <= p_end) {
				DUK_ASSERT(p + q_blen <= DUK_HSTRING_GET_DATA(h_input) + DUK_HSTRING_GET_BYTELEN(h_input));
				DUK_ASSERT(q_blen > 0); /* no issues with empty memcmp() */
				if (duk_memcmp((const void *) p, (const void *) q_start, (size_t) q_blen) == 0) {
					/* never an empty match, so step 13.c.iii can't be triggered */
					goto found;
				}

				/* track utf-8 non-continuation bytes */
				if ((p[0] & 0xc0) != 0x80) {
					match_start_coff++;
				}
				p++;
			}

		not_found:
			/* not found */
			break;

		found:
			matched = 1;
			match_start_boff = (duk_uint32_t) (p - p_start);
			match_end_coff = (duk_uint32_t) (match_start_coff + q_clen); /* constrained by string length */
			match_end_boff = (duk_uint32_t) (match_start_boff + q_blen); /* ditto */

			/* empty match (may happen with empty separator) -> bump and continue */
			if (prev_match_end_boff == match_end_boff) {
				prev_match_end_boff++;
				prev_match_end_coff++;
				continue;
			}
		} /* if (is_regexp) */

		/* stack[0] = separator (string or regexp)
		 * stack[1] = limit
		 * stack[2] = input string
		 * stack[3] = result array
		 * stack[4] = regexp res_obj (if is_regexp)
		 */

		DUK_DDD(DUK_DDDPRINT("split; match_start b=%ld,c=%ld, match_end b=%ld,c=%ld, prev_end b=%ld,c=%ld",
		                     (long) match_start_boff,
		                     (long) match_start_coff,
		                     (long) match_end_boff,
		                     (long) match_end_coff,
		                     (long) prev_match_end_boff,
		                     (long) prev_match_end_coff));

		duk_push_lstring(thr,
		                 (const char *) (DUK_HSTRING_GET_DATA(h_input) + prev_match_end_boff),
		                 (duk_size_t) (match_start_boff - prev_match_end_boff));
		duk_put_prop_index(thr, 3, arr_idx);
		arr_idx++;
		if (arr_idx >= limit) {
			goto hit_limit;
		}

#if defined(DUK_USE_REGEXP_SUPPORT)
		if (is_regexp) {
			duk_size_t i, len;

			len = duk_get_length(thr, 4);
			for (i = 1; i < len; i++) {
				DUK_ASSERT(i <= DUK_UARRIDX_MAX); /* cannot have >4G captures */
				duk_get_prop_index(thr, 4, (duk_uarridx_t) i);
				duk_put_prop_index(thr, 3, arr_idx);
				arr_idx++;
				if (arr_idx >= limit) {
					goto hit_limit;
				}
			}

			duk_pop(thr);
			/* lastIndex already set up for next match */
		} else {
#else /* DUK_USE_REGEXP_SUPPORT */
		{
		/* unconditionally */
#endif /* DUK_USE_REGEXP_SUPPORT */
			/* no action */
		}

		prev_match_end_boff = match_end_boff;
		prev_match_end_coff = match_end_coff;
		continue;
	} /* for */

	/* Combined step 11 (empty string special case) and 14-15. */

	DUK_DDD(DUK_DDDPRINT("split trailer; prev_end b=%ld,c=%ld", (long) prev_match_end_boff, (long) prev_match_end_coff));

	if (DUK_HSTRING_GET_BYTELEN(h_input) > 0 || !matched) {
		/* Add trailer if:
		 *   a) non-empty input
		 *   b) empty input and no (zero size) match found (step 11)
		 */

		duk_push_lstring(thr,
		                 (const char *) DUK_HSTRING_GET_DATA(h_input) + prev_match_end_boff,
		                 (duk_size_t) (DUK_HSTRING_GET_BYTELEN(h_input) - prev_match_end_boff));
		duk_put_prop_index(thr, 3, arr_idx);
		/* No arr_idx update or limit check */
	}

	return 1;

hit_limit:
#if defined(DUK_USE_REGEXP_SUPPORT)
	if (is_regexp) {
		duk_pop(thr);
	}
#endif

	return 1;
}

/*
 *  Various
 */

#if defined(DUK_USE_REGEXP_SUPPORT)
DUK_LOCAL void duk__to_regexp_helper(duk_hthread *thr, duk_idx_t idx, duk_bool_t force_new) {
	duk_hobject *h;

	/* Shared helper for match() steps 3-4, search() steps 3-4. */

	DUK_ASSERT(idx >= 0);

	if (force_new) {
		goto do_new;
	}

	h = duk_get_hobject_with_class(thr, idx, DUK_HOBJECT_CLASS_REGEXP);
	if (!h) {
		goto do_new;
	}
	return;

do_new:
	duk_push_hobject_bidx(thr, DUK_BIDX_REGEXP_CONSTRUCTOR);
	duk_dup(thr, idx);
	duk_new(thr, 1); /* [ ... RegExp val ] -> [ ... res ] */
	duk_replace(thr, idx);
}
#endif /* DUK_USE_REGEXP_SUPPORT */

#if defined(DUK_USE_REGEXP_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_string_prototype_search(duk_hthread *thr) {
	/* Easiest way to implement the search required by the specification
	 * is to do a RegExp test() with lastIndex forced to zero.  To avoid
	 * side effects on the argument, "clone" the RegExp if a RegExp was
	 * given as input.
	 *
	 * The global flag of the RegExp should be ignored; setting lastIndex
	 * to zero (which happens when "cloning" the RegExp) should have an
	 * equivalent effect.
	 */

	DUK_ASSERT_TOP(thr, 1);
	(void) duk_push_this_coercible_to_string(thr); /* at index 1 */
	duk__to_regexp_helper(thr, 0 /*index*/, 1 /*force_new*/);

	/* stack[0] = regexp
	 * stack[1] = string
	 */

	/* Avoid using RegExp.prototype methods, as they're writable and
	 * configurable and may have been changed.
	 */

	duk_dup_0(thr);
	duk_dup_1(thr); /* [ ... re_obj input ] */
	duk_regexp_match(thr); /* -> [ ... res_obj ] */

	if (!duk_is_object(thr, -1)) {
		duk_push_int(thr, -1);
		return 1;
	}

	duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_INDEX);
	DUK_ASSERT(duk_is_number(thr, -1));
	return 1;
}
#endif /* DUK_USE_REGEXP_SUPPORT */

#if defined(DUK_USE_REGEXP_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_string_prototype_match(duk_hthread *thr) {
	duk_bool_t global;
	duk_int_t prev_last_index;
	duk_int_t this_index;
	duk_int_t arr_idx;

	DUK_ASSERT_TOP(thr, 1);
	(void) duk_push_this_coercible_to_string(thr);
	duk__to_regexp_helper(thr, 0 /*index*/, 0 /*force_new*/);
	global = duk_get_prop_stridx_boolean(thr, 0, DUK_STRIDX_GLOBAL, NULL);
	DUK_ASSERT_TOP(thr, 2);

	/* stack[0] = regexp
	 * stack[1] = string
	 */

	if (!global) {
		duk_regexp_match(thr); /* -> [ res_obj ] */
		return 1; /* return 'res_obj' */
	}

	/* Global case is more complex. */

	/* [ regexp string ] */

	duk_push_int(thr, 0);
	duk_put_prop_stridx_short(thr, 0, DUK_STRIDX_LAST_INDEX);
	duk_push_array(thr);

	/* [ regexp string res_arr ] */

	prev_last_index = 0;
	arr_idx = 0;

	for (;;) {
		DUK_ASSERT_TOP(thr, 3);

		duk_dup_0(thr);
		duk_dup_1(thr);
		duk_regexp_match(thr); /* -> [ ... regexp string ] -> [ ... res_obj ] */

		if (!duk_is_object(thr, -1)) {
			duk_pop(thr);
			break;
		}

		duk_get_prop_stridx_short(thr, 0, DUK_STRIDX_LAST_INDEX);
		DUK_ASSERT(duk_is_number(thr, -1));
		this_index = duk_get_int(thr, -1);
		duk_pop(thr);

		if (this_index == prev_last_index) {
			this_index++;
			duk_push_int(thr, this_index);
			duk_put_prop_stridx_short(thr, 0, DUK_STRIDX_LAST_INDEX);
		}
		prev_last_index = this_index;

		duk_get_prop_index(thr, -1, 0); /* match string */
		duk_put_prop_index(thr, 2, (duk_uarridx_t) arr_idx);
		arr_idx++;
		duk_pop(thr); /* res_obj */
	}

	if (arr_idx == 0) {
		duk_push_null(thr);
	}

	return 1; /* return 'res_arr' or 'null' */
}
#endif /* DUK_USE_REGEXP_SUPPORT */

DUK_INTERNAL duk_ret_t duk_bi_string_prototype_concat(duk_hthread *thr) {
	/* duk_concat() coerces arguments with ToString() in correct order */
	(void) duk_push_this_coercible_to_string(thr);
	duk_insert(thr, 0); /* this is relatively expensive */
	duk_concat(thr, duk_get_top(thr));
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_string_prototype_trim(duk_hthread *thr) {
	DUK_ASSERT_TOP(thr, 0);
	(void) duk_push_this_coercible_to_string(thr);
	duk_trim(thr, 0);
	DUK_ASSERT_TOP(thr, 1);
	return 1;
}

#if defined(DUK_USE_ES6)
DUK_INTERNAL duk_ret_t duk_bi_string_prototype_repeat(duk_hthread *thr) {
	duk_hstring *h_input;
	duk_size_t input_blen;
	duk_size_t result_len;
	duk_int_t count_signed;
	duk_uint_t count;
	const duk_uint8_t *src;
	duk_uint8_t *buf;
	duk_uint8_t *p;
	duk_double_t d;
#if !defined(DUK_USE_PREFER_SIZE)
	duk_size_t copy_size;
	duk_uint8_t *p_end;
#endif

	DUK_ASSERT_TOP(thr, 1);
	h_input = duk_push_this_coercible_to_string(thr);
	DUK_ASSERT(h_input != NULL);
	input_blen = DUK_HSTRING_GET_BYTELEN(h_input);

	/* Count is ToNumber() coerced; +Infinity must be always rejected
	 * (even if input string is zero length), as well as negative values
	 * and -Infinity.  -Infinity doesn't require an explicit check
	 * because duk_get_int() clamps it to DUK_INT_MIN which gets rejected
	 * as a negative value (regardless of input string length).
	 */
	d = duk_to_number(thr, 0);
	if (duk_double_is_posinf(d)) {
		goto fail_range;
	}
	count_signed = duk_get_int(thr, 0);
	if (count_signed < 0) {
		goto fail_range;
	}
	count = (duk_uint_t) count_signed;

	/* Overflow check for result length. */
	result_len = count * input_blen;
	if (count != 0 && result_len / count != input_blen) {
		goto fail_range;
	}

	/* Temporary fixed buffer, later converted to string. */
	buf = (duk_uint8_t *) duk_push_fixed_buffer_nozero(thr, result_len);
	DUK_ASSERT(buf != NULL);
	src = (const duk_uint8_t *) DUK_HSTRING_GET_DATA(h_input);
	DUK_ASSERT(src != NULL);

#if defined(DUK_USE_PREFER_SIZE)
	p = buf;
	while (count-- > 0) {
		duk_memcpy((void *) p, (const void *) src, input_blen); /* copy size may be zero, but pointers are valid */
		p += input_blen;
	}
#else /* DUK_USE_PREFER_SIZE */
	/* Take advantage of already copied pieces to speed up the process
	 * especially for small repeated strings.
	 */
	p = buf;
	p_end = p + result_len;
	copy_size = input_blen;
	for (;;) {
		duk_size_t remain = (duk_size_t) (p_end - p);
		DUK_DDD(DUK_DDDPRINT("remain=%ld, copy_size=%ld, input_blen=%ld, result_len=%ld",
		                     (long) remain,
		                     (long) copy_size,
		                     (long) input_blen,
		                     (long) result_len));
		if (remain <= copy_size) {
			/* If result_len is zero, this case is taken and does
			 * a zero size copy (with valid pointers).
			 */
			duk_memcpy((void *) p, (const void *) src, remain);
			break;
		} else {
			duk_memcpy((void *) p, (const void *) src, copy_size);
			p += copy_size;
		}

		src = (const duk_uint8_t *) buf; /* Use buf as source for larger copies. */
		copy_size = (duk_size_t) (p - buf);
	}
#endif /* DUK_USE_PREFER_SIZE */

	/* XXX: It would be useful to be able to create a duk_hstring with
	 * a certain byte size whose data area wasn't initialized and which
	 * wasn't in the string table yet.  This would allow a string to be
	 * constructed directly without a buffer temporary and when it was
	 * finished, it could be injected into the string table.  Currently
	 * this isn't possible because duk_hstrings are only tracked by the
	 * intern table (they are not in heap_allocated).
	 */

	duk_buffer_to_string(thr, -1); /* Safe if input is safe. */
	return 1;

fail_range:
	DUK_DCERROR_RANGE_INVALID_ARGS(thr);
}
#endif /* DUK_USE_ES6 */

DUK_INTERNAL duk_ret_t duk_bi_string_prototype_locale_compare(duk_hthread *thr) {
	duk_hstring *h1;
	duk_hstring *h2;
	duk_size_t h1_len, h2_len, prefix_len;
	duk_small_int_t ret = 0;
	duk_small_int_t rc;

	/* The current implementation of localeCompare() is simply a codepoint
	 * by codepoint comparison, implemented with a simple string compare
	 * because UTF-8 should preserve codepoint ordering (assuming valid
	 * shortest UTF-8 encoding).
	 *
	 * The specification requires that the return value must be related
	 * to the sort order: e.g. negative means that 'this' comes before
	 * 'that' in sort order.  We assume an ascending sort order.
	 */

	/* XXX: could share code with duk_js_ops.c, duk_js_compare_helper */

	h1 = duk_push_this_coercible_to_string(thr);
	DUK_ASSERT(h1 != NULL);

	h2 = duk_to_hstring(thr, 0);
	DUK_ASSERT(h2 != NULL);

	h1_len = (duk_size_t) DUK_HSTRING_GET_BYTELEN(h1);
	h2_len = (duk_size_t) DUK_HSTRING_GET_BYTELEN(h2);
	prefix_len = (h1_len <= h2_len ? h1_len : h2_len);

	rc = (duk_small_int_t) duk_memcmp((const void *) DUK_HSTRING_GET_DATA(h1),
	                                  (const void *) DUK_HSTRING_GET_DATA(h2),
	                                  (size_t) prefix_len);

	if (rc < 0) {
		ret = -1;
		goto done;
	} else if (rc > 0) {
		ret = 1;
		goto done;
	}

	/* prefix matches, lengths matter now */
	if (h1_len > h2_len) {
		ret = 1;
		goto done;
	} else if (h1_len == h2_len) {
		DUK_ASSERT(ret == 0);
		goto done;
	}
	ret = -1;
	goto done;

done:
	duk_push_int(thr, (duk_int_t) ret);
	return 1;
}

#if defined(DUK_USE_ES6)
DUK_INTERNAL duk_ret_t duk_bi_string_prototype_startswith_endswith(duk_hthread *thr) {
	duk_int_t magic;
	duk_hstring *h_target;
	duk_size_t blen_target;
	duk_hstring *h_search;
	duk_size_t blen_search;
	duk_int_t off;
	duk_bool_t result = 0;
	duk_size_t blen_left;

	/* Because string byte lengths are in [0,DUK_INT_MAX] it's safe to
	 * subtract two string lengths without overflow.
	 */
	DUK_ASSERT(DUK_HSTRING_MAX_BYTELEN <= DUK_INT_MAX);

	h_target = duk_push_this_coercible_to_string(thr);
	DUK_ASSERT(h_target != NULL);

	h_search = duk__str_tostring_notregexp(thr, 0);
	DUK_ASSERT(h_search != NULL);

	magic = duk_get_current_magic(thr);

	/* Careful to avoid pointer overflows in the matching logic. */

	blen_target = DUK_HSTRING_GET_BYTELEN(h_target);
	blen_search = DUK_HSTRING_GET_BYTELEN(h_search);

#if 0
	/* If search string is longer than the target string, we can
	 * never match.  Could check explicitly, but should be handled
	 * correctly below.
	 */
	if (blen_search > blen_target) {
		goto finish;
	}
#endif

	off = 0;
	if (duk_is_undefined(thr, 1)) {
		if (magic) {
			off = (duk_int_t) blen_target - (duk_int_t) blen_search;
		} else {
			DUK_ASSERT(off == 0);
		}
	} else {
		duk_int_t len;
		duk_int_t pos;

		DUK_ASSERT(DUK_HSTRING_MAX_BYTELEN <= DUK_INT_MAX);
		len = (duk_int_t) DUK_HSTRING_GET_CHARLEN(h_target);
		pos = duk_to_int_clamped(thr, 1, 0, len);
		DUK_ASSERT(pos >= 0 && pos <= len);

		off = (duk_int_t) duk_heap_strcache_offset_char2byte(thr, h_target, (duk_uint_fast32_t) pos);
		if (magic) {
			off -= (duk_int_t) blen_search;
		}
	}
	if (off < 0 || off > (duk_int_t) blen_target) {
		goto finish;
	}

	/* The main comparison can be done using a memcmp() rather than
	 * doing codepoint comparisons: for CESU-8 strings there is a
	 * canonical representation for every codepoint.  But we do need
	 * to deal with the char/byte offset translation to find the
	 * comparison range.
	 */

	DUK_ASSERT(off >= 0);
	DUK_ASSERT((duk_size_t) off <= blen_target);
	blen_left = blen_target - (duk_size_t) off;
	if (blen_left >= blen_search) {
		const duk_uint8_t *p_cmp_start = (const duk_uint8_t *) DUK_HSTRING_GET_DATA(h_target) + off;
		const duk_uint8_t *p_search = (const duk_uint8_t *) DUK_HSTRING_GET_DATA(h_search);
		if (duk_memcmp_unsafe((const void *) p_cmp_start, (const void *) p_search, (size_t) blen_search) == 0) {
			result = 1;
		}
	}

finish:
	duk_push_boolean(thr, result);
	return 1;
}
#endif /* DUK_USE_ES6 */

#if defined(DUK_USE_ES6)
DUK_INTERNAL duk_ret_t duk_bi_string_prototype_includes(duk_hthread *thr) {
	duk_hstring *h;
	duk_hstring *h_search;
	duk_int_t len;
	duk_int_t pos;

	h = duk_push_this_coercible_to_string(thr);
	DUK_ASSERT(h != NULL);

	h_search = duk__str_tostring_notregexp(thr, 0);
	DUK_ASSERT(h_search != NULL);

	len = (duk_int_t) DUK_HSTRING_GET_CHARLEN(h);
	pos = duk_to_int_clamped(thr, 1, 0, len);
	DUK_ASSERT(pos >= 0 && pos <= len);

	pos = duk__str_search_shared(thr, h, h_search, pos, 0 /*backwards*/);
	duk_push_boolean(thr, pos >= 0);
	return 1;
}
#endif /* DUK_USE_ES6 */
#endif /* DUK_USE_STRING_BUILTIN */
