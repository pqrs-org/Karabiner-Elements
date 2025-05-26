/*
 *  ECMAScript specification algorithm and conversion helpers.
 *
 *  These helpers encapsulate the primitive ECMAScript operation semantics,
 *  and are used by the bytecode executor and the API (among other places).
 *  Some primitives are only implemented as part of the API and have no
 *  "internal" helper.  This is the case when an internal helper would not
 *  really be useful; e.g. the operation is rare, uses value stack heavily,
 *  etc.
 *
 *  The operation arguments depend on what is required to implement
 *  the operation:
 *
 *    - If an operation is simple and stateless, and has no side
 *      effects, it won't take an duk_hthread argument and its
 *      arguments may be duk_tval pointers (which are safe as long
 *      as no side effects take place).
 *
 *    - If complex coercions are required (e.g. a "ToNumber" coercion)
 *      or errors may be thrown, the operation takes an duk_hthread
 *      argument.  This also implies that the operation may have
 *      arbitrary side effects, invalidating any duk_tval pointers.
 *
 *    - For operations with potential side effects, arguments can be
 *      taken in several ways:
 *
 *      a) as duk_tval pointers, which makes sense if the "common case"
 *         can be resolved without side effects (e.g. coercion); the
 *         arguments are pushed to the valstack for coercion if
 *         necessary
 *
 *      b) as duk_tval values
 *
 *      c) implicitly on value stack top
 *
 *      d) as indices to the value stack
 *
 *  Future work:
 *
 *     - Argument styles may not be the most sensible in every case now.
 *
 *     - In-place coercions might be useful for several operations, if
 *       in-place coercion is OK for the bytecode executor and the API.
 */

#include "duk_internal.h"

/*
 *  ToPrimitive()  (E5 Section 9.1)
 *
 *  ==> implemented in the API.
 */

/*
 *  ToBoolean()  (E5 Section 9.2)
 */

DUK_INTERNAL duk_bool_t duk_js_toboolean(duk_tval *tv) {
	switch (DUK_TVAL_GET_TAG(tv)) {
	case DUK_TAG_UNDEFINED:
	case DUK_TAG_NULL:
		return 0;
	case DUK_TAG_BOOLEAN:
		DUK_ASSERT(DUK_TVAL_GET_BOOLEAN(tv) == 0 || DUK_TVAL_GET_BOOLEAN(tv) == 1);
		return DUK_TVAL_GET_BOOLEAN(tv);
	case DUK_TAG_STRING: {
		/* Symbols ToBoolean() coerce to true, regardless of their
		 * description.  This happens with no explicit check because
		 * of the symbol representation byte prefix.
		 */
		duk_hstring *h = DUK_TVAL_GET_STRING(tv);
		DUK_ASSERT(h != NULL);
		return (DUK_HSTRING_GET_BYTELEN(h) > 0 ? 1 : 0);
	}
	case DUK_TAG_OBJECT: {
		return 1;
	}
	case DUK_TAG_BUFFER: {
		/* Mimic Uint8Array semantics: objects coerce true, regardless
		 * of buffer length (zero or not) or context.
		 */
		return 1;
	}
	case DUK_TAG_POINTER: {
		void *p = DUK_TVAL_GET_POINTER(tv);
		return (p != NULL ? 1 : 0);
	}
	case DUK_TAG_LIGHTFUNC: {
		return 1;
	}
#if defined(DUK_USE_FASTINT)
	case DUK_TAG_FASTINT:
		if (DUK_TVAL_GET_FASTINT(tv) != 0) {
			return 1;
		} else {
			return 0;
		}
#endif
	default: {
		/* number */
		duk_double_t d;
#if defined(DUK_USE_PREFER_SIZE)
		int c;
#endif
		DUK_ASSERT(!DUK_TVAL_IS_UNUSED(tv));
		DUK_ASSERT(DUK_TVAL_IS_DOUBLE(tv));
		d = DUK_TVAL_GET_DOUBLE(tv);
#if defined(DUK_USE_PREFER_SIZE)
		c = DUK_FPCLASSIFY((double) d);
		if (c == DUK_FP_ZERO || c == DUK_FP_NAN) {
			return 0;
		} else {
			return 1;
		}
#else
		DUK_ASSERT(duk_double_is_nan_or_zero(d) == 0 || duk_double_is_nan_or_zero(d) == 1);
		return duk_double_is_nan_or_zero(d) ^ 1;
#endif
	}
	}
	DUK_UNREACHABLE();
	DUK_WO_UNREACHABLE(return 0;);
}

/*
 *  ToNumber()  (E5 Section 9.3)
 *
 *  Value to convert must be on stack top, and is popped before exit.
 *
 *  See: http://www.cs.indiana.edu/~burger/FP-Printing-PLDI96.pdf
 *       http://www.cs.indiana.edu/~burger/fp/index.html
 *
 *  Notes on the conversion:
 *
 *    - There are specific requirements on the accuracy of the conversion
 *      through a "Mathematical Value" (MV), so this conversion is not
 *      trivial.
 *
 *    - Quick rejects (e.g. based on first char) are difficult because
 *      the grammar allows leading and trailing white space.
 *
 *    - Quick reject based on string length is difficult even after
 *      accounting for white space; there may be arbitrarily many
 *      decimal digits.
 *
 *    - Standard grammar allows decimal values ("123"), hex values
 *      ("0x123") and infinities
 *
 *    - Unlike source code literals, ToNumber() coerces empty strings
 *      and strings with only whitespace to zero (not NaN).  However,
 *      while '' coerces to 0, '+' and '-' coerce to NaN.
 */

/* E5 Section 9.3.1 */
DUK_LOCAL duk_double_t duk__tonumber_string_raw(duk_hthread *thr) {
	duk_small_uint_t s2n_flags;
	duk_double_t d;

	DUK_ASSERT(duk_is_string(thr, -1));

	/* Quite lenient, e.g. allow empty as zero, but don't allow trailing
	 * garbage.
	 */
	s2n_flags = DUK_S2N_FLAG_TRIM_WHITE | DUK_S2N_FLAG_ALLOW_EXP | DUK_S2N_FLAG_ALLOW_PLUS | DUK_S2N_FLAG_ALLOW_MINUS |
	            DUK_S2N_FLAG_ALLOW_INF | DUK_S2N_FLAG_ALLOW_FRAC | DUK_S2N_FLAG_ALLOW_NAKED_FRAC |
	            DUK_S2N_FLAG_ALLOW_EMPTY_FRAC | DUK_S2N_FLAG_ALLOW_EMPTY_AS_ZERO | DUK_S2N_FLAG_ALLOW_LEADING_ZERO |
	            DUK_S2N_FLAG_ALLOW_AUTO_HEX_INT | DUK_S2N_FLAG_ALLOW_AUTO_OCT_INT | DUK_S2N_FLAG_ALLOW_AUTO_BIN_INT;

	duk_numconv_parse(thr, 10 /*radix*/, s2n_flags);

#if defined(DUK_USE_PREFER_SIZE)
	d = duk_get_number(thr, -1);
	duk_pop_unsafe(thr);
#else
	thr->valstack_top--;
	DUK_ASSERT(DUK_TVAL_IS_NUMBER(thr->valstack_top));
	DUK_ASSERT(DUK_TVAL_IS_DOUBLE(thr->valstack_top)); /* no fastint conversion in numconv now */
	DUK_ASSERT(!DUK_TVAL_NEEDS_REFCOUNT_UPDATE(thr->valstack_top));
	d = DUK_TVAL_GET_DOUBLE(thr->valstack_top); /* assumes not a fastint */
	DUK_TVAL_SET_UNDEFINED(thr->valstack_top);
#endif

	return d;
}

DUK_INTERNAL duk_double_t duk_js_tonumber(duk_hthread *thr, duk_tval *tv) {
	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(tv != NULL);

	switch (DUK_TVAL_GET_TAG(tv)) {
	case DUK_TAG_UNDEFINED: {
		/* return a specific NaN (although not strictly necessary) */
		duk_double_union du;
		DUK_DBLUNION_SET_NAN(&du);
		DUK_ASSERT(DUK_DBLUNION_IS_NORMALIZED(&du));
		return du.d;
	}
	case DUK_TAG_NULL: {
		/* +0.0 */
		return 0.0;
	}
	case DUK_TAG_BOOLEAN: {
		if (DUK_TVAL_IS_BOOLEAN_TRUE(tv)) {
			return 1.0;
		}
		return 0.0;
	}
	case DUK_TAG_STRING: {
		/* For Symbols ToNumber() is always a TypeError. */
		duk_hstring *h = DUK_TVAL_GET_STRING(tv);
		if (DUK_UNLIKELY(DUK_HSTRING_HAS_SYMBOL(h))) {
			DUK_ERROR_TYPE(thr, DUK_STR_CANNOT_NUMBER_COERCE_SYMBOL);
			DUK_WO_NORETURN(return 0.0;);
		}
		duk_push_hstring(thr, h);
		return duk__tonumber_string_raw(thr);
	}
	case DUK_TAG_BUFFER: /* plain buffer treated like object */
	case DUK_TAG_OBJECT: {
		duk_double_t d;
		duk_push_tval(thr, tv);
		duk_to_primitive(thr, -1, DUK_HINT_NUMBER); /* 'tv' becomes invalid */

		/* recursive call for a primitive value (guaranteed not to cause second
		 * recursion).
		 */
		DUK_ASSERT(duk_get_tval(thr, -1) != NULL);
		d = duk_js_tonumber(thr, duk_get_tval(thr, -1));

		duk_pop_unsafe(thr);
		return d;
	}
	case DUK_TAG_POINTER: {
		/* Coerce like boolean */
		void *p = DUK_TVAL_GET_POINTER(tv);
		return (p != NULL ? 1.0 : 0.0);
	}
	case DUK_TAG_LIGHTFUNC: {
		/* +(function(){}) -> NaN */
		return DUK_DOUBLE_NAN;
	}
#if defined(DUK_USE_FASTINT)
	case DUK_TAG_FASTINT:
		return (duk_double_t) DUK_TVAL_GET_FASTINT(tv);
#endif
	default: {
		/* number */
		DUK_ASSERT(!DUK_TVAL_IS_UNUSED(tv));
		DUK_ASSERT(DUK_TVAL_IS_DOUBLE(tv));
		return DUK_TVAL_GET_DOUBLE(tv);
	}
	}

	DUK_UNREACHABLE();
	DUK_WO_UNREACHABLE(return 0.0;);
}

/*
 *  ToInteger()  (E5 Section 9.4)
 */

/* exposed, used by e.g. duk_bi_date.c */
DUK_INTERNAL duk_double_t duk_js_tointeger_number(duk_double_t x) {
#if defined(DUK_USE_PREFER_SIZE)
	duk_small_int_t c = (duk_small_int_t) DUK_FPCLASSIFY(x);

	if (DUK_UNLIKELY(c == DUK_FP_NAN)) {
		return 0.0;
	} else if (DUK_UNLIKELY(c == DUK_FP_INFINITE)) {
		return x;
	} else {
		/* Finite, including neg/pos zero.  Neg zero sign must be
		 * preserved.
		 */
		return duk_double_trunc_towards_zero(x);
	}
#else /* DUK_USE_PREFER_SIZE */
	/* NaN and Infinity have the same exponent so it's a cheap
	 * initial check for the rare path.
	 */
	if (DUK_UNLIKELY(duk_double_is_nan_or_inf(x) != 0U)) {
		if (duk_double_is_nan(x)) {
			return 0.0;
		} else {
			return x;
		}
	} else {
		return duk_double_trunc_towards_zero(x);
	}
#endif /* DUK_USE_PREFER_SIZE */
}

DUK_INTERNAL duk_double_t duk_js_tointeger(duk_hthread *thr, duk_tval *tv) {
	/* XXX: fastint */
	duk_double_t d = duk_js_tonumber(thr, tv); /* invalidates tv */
	return duk_js_tointeger_number(d);
}

/*
 *  ToInt32(), ToUint32(), ToUint16()  (E5 Sections 9.5, 9.6, 9.7)
 */

/* combined algorithm matching E5 Sections 9.5 and 9.6 */
DUK_LOCAL duk_double_t duk__toint32_touint32_helper(duk_double_t x, duk_bool_t is_toint32) {
#if defined(DUK_USE_PREFER_SIZE)
	duk_small_int_t c;
#endif

#if defined(DUK_USE_PREFER_SIZE)
	c = (duk_small_int_t) DUK_FPCLASSIFY(x);
	if (c == DUK_FP_NAN || c == DUK_FP_ZERO || c == DUK_FP_INFINITE) {
		return 0.0;
	}
#else
	if (duk_double_is_nan_zero_inf(x)) {
		return 0.0;
	}
#endif

	/* x = sign(x) * floor(abs(x)), i.e. truncate towards zero, keep sign */
	x = duk_double_trunc_towards_zero(x);

	/* NOTE: fmod(x) result sign is same as sign of x, which
	 * differs from what Javascript wants (see Section 9.6).
	 */

	x = DUK_FMOD(x, DUK_DOUBLE_2TO32); /* -> x in ]-2**32, 2**32[ */

	if (x < 0.0) {
		x += DUK_DOUBLE_2TO32;
	}
	DUK_ASSERT(x >= 0 && x < DUK_DOUBLE_2TO32); /* -> x in [0, 2**32[ */

	if (is_toint32) {
		if (x >= DUK_DOUBLE_2TO31) {
			/* x in [2**31, 2**32[ */

			x -= DUK_DOUBLE_2TO32; /* -> x in [-2**31,2**31[ */
		}
	}

	return x;
}

DUK_INTERNAL duk_int32_t duk_js_toint32(duk_hthread *thr, duk_tval *tv) {
	duk_double_t d;

#if defined(DUK_USE_FASTINT)
	if (DUK_TVAL_IS_FASTINT(tv)) {
		return DUK_TVAL_GET_FASTINT_I32(tv);
	}
#endif

	d = duk_js_tonumber(thr, tv); /* invalidates tv */
	d = duk__toint32_touint32_helper(d, 1);
	DUK_ASSERT(DUK_FPCLASSIFY(d) == DUK_FP_ZERO || DUK_FPCLASSIFY(d) == DUK_FP_NORMAL);
	DUK_ASSERT(d >= -2147483648.0 && d <= 2147483647.0); /* [-0x80000000,0x7fffffff] */
	DUK_ASSERT(duk_double_equals(d, (duk_double_t) ((duk_int32_t) d))); /* whole, won't clip */
	return (duk_int32_t) d;
}

DUK_INTERNAL duk_uint32_t duk_js_touint32(duk_hthread *thr, duk_tval *tv) {
	duk_double_t d;

#if defined(DUK_USE_FASTINT)
	if (DUK_TVAL_IS_FASTINT(tv)) {
		return DUK_TVAL_GET_FASTINT_U32(tv);
	}
#endif

	d = duk_js_tonumber(thr, tv); /* invalidates tv */
	d = duk__toint32_touint32_helper(d, 0);
	DUK_ASSERT(DUK_FPCLASSIFY(d) == DUK_FP_ZERO || DUK_FPCLASSIFY(d) == DUK_FP_NORMAL);
	DUK_ASSERT(d >= 0.0 && d <= 4294967295.0); /* [0x00000000, 0xffffffff] */
	DUK_ASSERT(duk_double_equals(d, (duk_double_t) ((duk_uint32_t) d))); /* whole, won't clip */
	return (duk_uint32_t) d;
}

DUK_INTERNAL duk_uint16_t duk_js_touint16(duk_hthread *thr, duk_tval *tv) {
	/* should be a safe way to compute this */
	return (duk_uint16_t) (duk_js_touint32(thr, tv) & 0x0000ffffU);
}

/*
 *  ToString()  (E5 Section 9.8)
 *  ToObject()  (E5 Section 9.9)
 *  CheckObjectCoercible()  (E5 Section 9.10)
 *  IsCallable()  (E5 Section 9.11)
 *
 *  ==> implemented in the API.
 */

/*
 *  Loose equality, strict equality, and SameValue (E5 Sections 11.9.1, 11.9.4,
 *  9.12).  These have much in common so they can share some helpers.
 *
 *  Future work notes:
 *
 *    - Current implementation (and spec definition) has recursion; this should
 *      be fixed if possible.
 *
 *    - String-to-number coercion should be possible without going through the
 *      value stack (and be more compact) if a shared helper is invoked.
 */

/* Note that this is the same operation for strict and loose equality:
 *  - E5 Section 11.9.3, step 1.c (loose)
 *  - E5 Section 11.9.6, step 4 (strict)
 */

DUK_LOCAL duk_bool_t duk__js_equals_number(duk_double_t x, duk_double_t y) {
#if defined(DUK_USE_PARANOID_MATH)
	/* Straightforward algorithm, makes fewer compiler assumptions. */
	duk_small_int_t cx = (duk_small_int_t) DUK_FPCLASSIFY(x);
	duk_small_int_t cy = (duk_small_int_t) DUK_FPCLASSIFY(y);
	if (cx == DUK_FP_NAN || cy == DUK_FP_NAN) {
		return 0;
	}
	if (cx == DUK_FP_ZERO && cy == DUK_FP_ZERO) {
		return 1;
	}
	if (x == y) {
		return 1;
	}
	return 0;
#else /* DUK_USE_PARANOID_MATH */
	/* Better equivalent algorithm.  If the compiler is compliant, C and
	 * ECMAScript semantics are identical for this particular comparison.
	 * In particular, NaNs must never compare equal and zeroes must compare
	 * equal regardless of sign.  Could also use a macro, but this inlines
	 * already nicely (no difference on gcc, for instance).
	 */
	if (duk_double_equals(x, y)) {
		/* IEEE requires that NaNs compare false */
		DUK_ASSERT(DUK_FPCLASSIFY(x) != DUK_FP_NAN);
		DUK_ASSERT(DUK_FPCLASSIFY(y) != DUK_FP_NAN);
		return 1;
	} else {
		/* IEEE requires that zeros compare the same regardless
		 * of their signed, so if both x and y are zeroes, they
		 * are caught above.
		 */
		DUK_ASSERT(!(DUK_FPCLASSIFY(x) == DUK_FP_ZERO && DUK_FPCLASSIFY(y) == DUK_FP_ZERO));
		return 0;
	}
#endif /* DUK_USE_PARANOID_MATH */
}

DUK_LOCAL duk_bool_t duk__js_samevalue_number(duk_double_t x, duk_double_t y) {
#if defined(DUK_USE_PARANOID_MATH)
	duk_small_int_t cx = (duk_small_int_t) DUK_FPCLASSIFY(x);
	duk_small_int_t cy = (duk_small_int_t) DUK_FPCLASSIFY(y);

	if (cx == DUK_FP_NAN && cy == DUK_FP_NAN) {
		/* SameValue(NaN, NaN) = true, regardless of NaN sign or extra bits */
		return 1;
	}
	if (cx == DUK_FP_ZERO && cy == DUK_FP_ZERO) {
		/* Note: cannot assume that a non-zero return value of signbit() would
		 * always be the same -- hence cannot (portably) use something like:
		 *
		 *     signbit(x) == signbit(y)
		 */
		duk_small_int_t sx = DUK_SIGNBIT(x) ? 1 : 0;
		duk_small_int_t sy = DUK_SIGNBIT(y) ? 1 : 0;
		return (sx == sy);
	}

	/* normal comparison; known:
	 *   - both x and y are not NaNs (but one of them can be)
	 *   - both x and y are not zero (but one of them can be)
	 *   - x and y may be denormal or infinite
	 */

	return (x == y);
#else /* DUK_USE_PARANOID_MATH */
	duk_small_int_t cx = (duk_small_int_t) DUK_FPCLASSIFY(x);
	duk_small_int_t cy = (duk_small_int_t) DUK_FPCLASSIFY(y);

	if (duk_double_equals(x, y)) {
		/* IEEE requires that NaNs compare false */
		DUK_ASSERT(DUK_FPCLASSIFY(x) != DUK_FP_NAN);
		DUK_ASSERT(DUK_FPCLASSIFY(y) != DUK_FP_NAN);

		/* Using classification has smaller footprint than direct comparison. */
		if (DUK_UNLIKELY(cx == DUK_FP_ZERO && cy == DUK_FP_ZERO)) {
			/* Note: cannot assume that a non-zero return value of signbit() would
			 * always be the same -- hence cannot (portably) use something like:
			 *
			 *     signbit(x) == signbit(y)
			 */
			return duk_double_same_sign(x, y);
		}
		return 1;
	} else {
		/* IEEE requires that zeros compare the same regardless
		 * of their sign, so if both x and y are zeroes, they
		 * are caught above.
		 */
		DUK_ASSERT(!(DUK_FPCLASSIFY(x) == DUK_FP_ZERO && DUK_FPCLASSIFY(y) == DUK_FP_ZERO));

		/* Difference to non-strict/strict comparison is that NaNs compare
		 * equal and signed zero signs matter.
		 */
		if (DUK_UNLIKELY(cx == DUK_FP_NAN && cy == DUK_FP_NAN)) {
			/* SameValue(NaN, NaN) = true, regardless of NaN sign or extra bits */
			return 1;
		}
		return 0;
	}
#endif /* DUK_USE_PARANOID_MATH */
}

DUK_INTERNAL duk_bool_t duk_js_equals_helper(duk_hthread *thr, duk_tval *tv_x, duk_tval *tv_y, duk_small_uint_t flags) {
	duk_uint_t type_mask_x;
	duk_uint_t type_mask_y;

	/* If flags != 0 (strict or SameValue), thr can be NULL.  For loose
	 * equals comparison it must be != NULL.
	 */
	DUK_ASSERT(flags != 0 || thr != NULL);

	/*
	 *  Same type?
	 *
	 *  Note: since number values have no explicit tag in the 8-byte
	 *  representation, need the awkward if + switch.
	 */

#if defined(DUK_USE_FASTINT)
	if (DUK_TVAL_IS_FASTINT(tv_x) && DUK_TVAL_IS_FASTINT(tv_y)) {
		if (DUK_TVAL_GET_FASTINT(tv_x) == DUK_TVAL_GET_FASTINT(tv_y)) {
			return 1;
		} else {
			return 0;
		}
	} else
#endif
	    if (DUK_TVAL_IS_NUMBER(tv_x) && DUK_TVAL_IS_NUMBER(tv_y)) {
		duk_double_t d1, d2;

		/* Catches both doubles and cases where only one argument is
		 * a fastint so can't assume a double.
		 */
		d1 = DUK_TVAL_GET_NUMBER(tv_x);
		d2 = DUK_TVAL_GET_NUMBER(tv_y);
		if (DUK_UNLIKELY((flags & DUK_EQUALS_FLAG_SAMEVALUE) != 0)) {
			/* SameValue */
			return duk__js_samevalue_number(d1, d2);
		} else {
			/* equals and strict equals */
			return duk__js_equals_number(d1, d2);
		}
	} else if (DUK_TVAL_GET_TAG(tv_x) == DUK_TVAL_GET_TAG(tv_y)) {
		switch (DUK_TVAL_GET_TAG(tv_x)) {
		case DUK_TAG_UNDEFINED:
		case DUK_TAG_NULL: {
			return 1;
		}
		case DUK_TAG_BOOLEAN: {
			return DUK_TVAL_GET_BOOLEAN(tv_x) == DUK_TVAL_GET_BOOLEAN(tv_y);
		}
		case DUK_TAG_POINTER: {
			return DUK_TVAL_GET_POINTER(tv_x) == DUK_TVAL_GET_POINTER(tv_y);
		}
		case DUK_TAG_STRING:
		case DUK_TAG_OBJECT: {
			/* Heap pointer comparison suffices for strings and objects.
			 * Symbols compare equal if they have the same internal
			 * representation; again heap pointer comparison suffices.
			 */
			return DUK_TVAL_GET_HEAPHDR(tv_x) == DUK_TVAL_GET_HEAPHDR(tv_y);
		}
		case DUK_TAG_BUFFER: {
			/* In Duktape 2.x plain buffers mimic Uint8Array objects
			 * so always compare by heap pointer.  In Duktape 1.x
			 * strict comparison would compare heap pointers and
			 * non-strict would compare contents.
			 */
			return DUK_TVAL_GET_HEAPHDR(tv_x) == DUK_TVAL_GET_HEAPHDR(tv_y);
		}
		case DUK_TAG_LIGHTFUNC: {
			/* At least 'magic' has a significant impact on function
			 * identity.
			 */
			duk_small_uint_t lf_flags_x;
			duk_small_uint_t lf_flags_y;
			duk_c_function func_x;
			duk_c_function func_y;

			DUK_TVAL_GET_LIGHTFUNC(tv_x, func_x, lf_flags_x);
			DUK_TVAL_GET_LIGHTFUNC(tv_y, func_y, lf_flags_y);
			return ((func_x == func_y) && (lf_flags_x == lf_flags_y)) ? 1 : 0;
		}
#if defined(DUK_USE_FASTINT)
		case DUK_TAG_FASTINT:
#endif
		default: {
			DUK_ASSERT(!DUK_TVAL_IS_UNUSED(tv_x));
			DUK_ASSERT(!DUK_TVAL_IS_UNUSED(tv_y));
			DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv_x));
			DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv_y));
			DUK_UNREACHABLE();
			DUK_WO_UNREACHABLE(return 0;);
		}
		}
	}

	if ((flags & (DUK_EQUALS_FLAG_STRICT | DUK_EQUALS_FLAG_SAMEVALUE)) != 0) {
		return 0;
	}

	DUK_ASSERT(flags == 0); /* non-strict equality from here on */

	/*
	 *  Types are different; various cases for non-strict comparison
	 *
	 *  Since comparison is symmetric, we use a "swap trick" to reduce
	 *  code size.
	 */

	type_mask_x = duk_get_type_mask_tval(tv_x);
	type_mask_y = duk_get_type_mask_tval(tv_y);

	/* Undefined/null are considered equal (e.g. "null == undefined" -> true). */
	if ((type_mask_x & (DUK_TYPE_MASK_UNDEFINED | DUK_TYPE_MASK_NULL)) &&
	    (type_mask_y & (DUK_TYPE_MASK_NULL | DUK_TYPE_MASK_UNDEFINED))) {
		return 1;
	}

	/* Number/string -> coerce string to number (e.g. "'1.5' == 1.5" -> true). */
	if ((type_mask_x & DUK_TYPE_MASK_NUMBER) && (type_mask_y & DUK_TYPE_MASK_STRING)) {
		if (!DUK_TVAL_STRING_IS_SYMBOL(tv_y)) {
			duk_double_t d1, d2;
			d1 = DUK_TVAL_GET_NUMBER(tv_x);
			d2 = duk_to_number_tval(thr, tv_y);
			return duk__js_equals_number(d1, d2);
		}
	}
	if ((type_mask_x & DUK_TYPE_MASK_STRING) && (type_mask_y & DUK_TYPE_MASK_NUMBER)) {
		if (!DUK_TVAL_STRING_IS_SYMBOL(tv_x)) {
			duk_double_t d1, d2;
			d1 = DUK_TVAL_GET_NUMBER(tv_y);
			d2 = duk_to_number_tval(thr, tv_x);
			return duk__js_equals_number(d1, d2);
		}
	}

	/* Boolean/any -> coerce boolean to number and try again.  If boolean is
	 * compared to a pointer, the final comparison after coercion now always
	 * yields false (as pointer vs. number compares to false), but this is
	 * not special cased.
	 *
	 * ToNumber(bool) is +1.0 or 0.0.  Tagged boolean value is always 0 or 1.
	 */
	if (type_mask_x & DUK_TYPE_MASK_BOOLEAN) {
		DUK_ASSERT(DUK_TVAL_GET_BOOLEAN(tv_x) == 0 || DUK_TVAL_GET_BOOLEAN(tv_x) == 1);
		duk_push_uint(thr, DUK_TVAL_GET_BOOLEAN(tv_x));
		duk_push_tval(thr, tv_y);
		goto recursive_call;
	}
	if (type_mask_y & DUK_TYPE_MASK_BOOLEAN) {
		DUK_ASSERT(DUK_TVAL_GET_BOOLEAN(tv_y) == 0 || DUK_TVAL_GET_BOOLEAN(tv_y) == 1);
		duk_push_tval(thr, tv_x);
		duk_push_uint(thr, DUK_TVAL_GET_BOOLEAN(tv_y));
		goto recursive_call;
	}

	/* String-number-symbol/object -> coerce object to primitive (apparently without hint), then try again. */
	if ((type_mask_x & (DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_NUMBER)) && (type_mask_y & DUK_TYPE_MASK_OBJECT)) {
		/* No symbol check needed because symbols and strings are accepted. */
		duk_push_tval(thr, tv_x);
		duk_push_tval(thr, tv_y);
		duk_to_primitive(thr, -1, DUK_HINT_NONE); /* apparently no hint? */
		goto recursive_call;
	}
	if ((type_mask_x & DUK_TYPE_MASK_OBJECT) && (type_mask_y & (DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_NUMBER))) {
		/* No symbol check needed because symbols and strings are accepted. */
		duk_push_tval(thr, tv_x);
		duk_push_tval(thr, tv_y);
		duk_to_primitive(thr, -2, DUK_HINT_NONE); /* apparently no hint? */
		goto recursive_call;
	}

	/* Nothing worked -> not equal. */
	return 0;

recursive_call:
	/* Shared code path to call the helper again with arguments on stack top. */
	{
		duk_bool_t rc;
		rc = duk_js_equals_helper(thr, DUK_GET_TVAL_NEGIDX(thr, -2), DUK_GET_TVAL_NEGIDX(thr, -1), 0 /*flags:nonstrict*/);
		duk_pop_2_unsafe(thr);
		return rc;
	}
}

/*
 *  Comparisons (x >= y, x > y, x <= y, x < y)
 *
 *  E5 Section 11.8.5: implement 'x < y' and then use negate and eval_left_first
 *  flags to get the rest.
 */

/* XXX: this should probably just operate on the stack top, because it
 * needs to push stuff on the stack anyway...
 */

DUK_INTERNAL duk_small_int_t duk_js_data_compare(const duk_uint8_t *buf1,
                                                 const duk_uint8_t *buf2,
                                                 duk_size_t len1,
                                                 duk_size_t len2) {
	duk_size_t prefix_len;
	duk_small_int_t rc;

	prefix_len = (len1 <= len2 ? len1 : len2);

	/* duk_memcmp() is guaranteed to return zero (equal) for zero length
	 * inputs.
	 */
	rc = duk_memcmp_unsafe((const void *) buf1, (const void *) buf2, (size_t) prefix_len);

	if (rc < 0) {
		return -1;
	} else if (rc > 0) {
		return 1;
	}

	/* prefix matches, lengths matter now */
	if (len1 < len2) {
		/* e.g. "x" < "xx" */
		return -1;
	} else if (len1 > len2) {
		return 1;
	}

	return 0;
}

DUK_INTERNAL duk_small_int_t duk_js_string_compare(duk_hstring *h1, duk_hstring *h2) {
	/*
	 *  String comparison (E5 Section 11.8.5, step 4), which
	 *  needs to compare codepoint by codepoint.
	 *
	 *  However, UTF-8 allows us to use strcmp directly: the shared
	 *  prefix will be encoded identically (UTF-8 has unique encoding)
	 *  and the first differing character can be compared with a simple
	 *  unsigned byte comparison (which strcmp does).
	 *
	 *  This will not work properly for non-xutf-8 strings, but this
	 *  is not an issue for compliance.
	 */

	DUK_ASSERT(h1 != NULL);
	DUK_ASSERT(h2 != NULL);

	return duk_js_data_compare((const duk_uint8_t *) DUK_HSTRING_GET_DATA(h1),
	                           (const duk_uint8_t *) DUK_HSTRING_GET_DATA(h2),
	                           (duk_size_t) DUK_HSTRING_GET_BYTELEN(h1),
	                           (duk_size_t) DUK_HSTRING_GET_BYTELEN(h2));
}

#if 0 /* unused */
DUK_INTERNAL duk_small_int_t duk_js_buffer_compare(duk_heap *heap, duk_hbuffer *h1, duk_hbuffer *h2) {
	/* Similar to String comparison. */

	DUK_ASSERT(h1 != NULL);
	DUK_ASSERT(h2 != NULL);
	DUK_UNREF(heap);

	return duk_js_data_compare((const duk_uint8_t *) DUK_HBUFFER_GET_DATA_PTR(heap, h1),
	                           (const duk_uint8_t *) DUK_HBUFFER_GET_DATA_PTR(heap, h2),
	                           (duk_size_t) DUK_HBUFFER_GET_SIZE(h1),
	                           (duk_size_t) DUK_HBUFFER_GET_SIZE(h2));
}
#endif

#if defined(DUK_USE_FASTINT)
DUK_LOCAL duk_bool_t duk__compare_fastint(duk_bool_t retval, duk_int64_t v1, duk_int64_t v2) {
	DUK_ASSERT(retval == 0 || retval == 1);
	if (v1 < v2) {
		return retval ^ 1;
	} else {
		return retval;
	}
}
#endif

#if defined(DUK_USE_PARANOID_MATH)
DUK_LOCAL duk_bool_t duk__compare_number(duk_bool_t retval, duk_double_t d1, duk_double_t d2) {
	duk_small_int_t c1, s1, c2, s2;

	DUK_ASSERT(retval == 0 || retval == 1);
	c1 = (duk_small_int_t) DUK_FPCLASSIFY(d1);
	s1 = (duk_small_int_t) DUK_SIGNBIT(d1);
	c2 = (duk_small_int_t) DUK_FPCLASSIFY(d2);
	s2 = (duk_small_int_t) DUK_SIGNBIT(d2);

	if (c1 == DUK_FP_NAN || c2 == DUK_FP_NAN) {
		return 0; /* Always false, regardless of negation. */
	}

	if (c1 == DUK_FP_ZERO && c2 == DUK_FP_ZERO) {
		/* For all combinations: +0 < +0, +0 < -0, -0 < +0, -0 < -0,
		 * steps e, f, and g.
		 */
		return retval; /* false */
	}

	if (d1 == d2) {
		return retval; /* false */
	}

	if (c1 == DUK_FP_INFINITE && s1 == 0) {
		/* x == +Infinity */
		return retval; /* false */
	}

	if (c2 == DUK_FP_INFINITE && s2 == 0) {
		/* y == +Infinity */
		return retval ^ 1; /* true */
	}

	if (c2 == DUK_FP_INFINITE && s2 != 0) {
		/* y == -Infinity */
		return retval; /* false */
	}

	if (c1 == DUK_FP_INFINITE && s1 != 0) {
		/* x == -Infinity */
		return retval ^ 1; /* true */
	}

	if (d1 < d2) {
		return retval ^ 1; /* true */
	}

	return retval; /* false */
}
#else /* DUK_USE_PARANOID_MATH */
DUK_LOCAL duk_bool_t duk__compare_number(duk_bool_t retval, duk_double_t d1, duk_double_t d2) {
	/* This comparison tree relies doesn't match the exact steps in
	 * E5 Section 11.8.5 but should produce the same results.  The
	 * steps rely on exact IEEE semantics for NaNs, etc.
	 */

	DUK_ASSERT(retval == 0 || retval == 1);
	if (d1 < d2) {
		/* In no case should both (d1 < d2) and (d2 < d1) be true.
		 * It's possible that neither is true though, and that's
		 * handled below.
		 */
		DUK_ASSERT(!(d2 < d1));

		/* - d1 < d2, both d1/d2 are normals (not Infinity, not NaN)
		 * - d2 is +Infinity, d1 != +Infinity and NaN
		 * - d1 is -Infinity, d2 != -Infinity and NaN
		 */
		return retval ^ 1;
	} else {
		if (d2 < d1) {
			/* - !(d1 < d2), both d1/d2 are normals (not Infinity, not NaN)
			 * - d1 is +Infinity, d2 != +Infinity and NaN
			 * - d2 is -Infinity, d1 != -Infinity and NaN
			 */
			return retval;
		} else {
			/* - d1 and/or d2 is NaN
			 * - d1 and d2 are both +/- 0
			 * - d1 == d2 (including infinities)
			 */
			if (duk_double_is_nan(d1) || duk_double_is_nan(d2)) {
				/* Note: undefined from Section 11.8.5 always
				 * results in false return (see e.g. Section
				 * 11.8.3) - hence special treatment here.
				 */
				return 0; /* zero regardless of negation */
			} else {
				return retval;
			}
		}
	}
}
#endif /* DUK_USE_PARANOID_MATH */

DUK_INTERNAL duk_bool_t duk_js_compare_helper(duk_hthread *thr, duk_tval *tv_x, duk_tval *tv_y, duk_small_uint_t flags) {
	duk_double_t d1, d2;
	duk_small_int_t rc;
	duk_bool_t retval;

	DUK_ASSERT(DUK_COMPARE_FLAG_NEGATE == 1); /* Rely on this flag being lowest. */
	retval = flags & DUK_COMPARE_FLAG_NEGATE;
	DUK_ASSERT(retval == 0 || retval == 1);

	/* Fast path for fastints */
#if defined(DUK_USE_FASTINT)
	if (DUK_LIKELY(DUK_TVAL_IS_FASTINT(tv_x) && DUK_TVAL_IS_FASTINT(tv_y))) {
		return duk__compare_fastint(retval, DUK_TVAL_GET_FASTINT(tv_x), DUK_TVAL_GET_FASTINT(tv_y));
	}
#endif /* DUK_USE_FASTINT */

	/* Fast path for numbers (one of which may be a fastint) */
#if !defined(DUK_USE_PREFER_SIZE)
	if (DUK_LIKELY(DUK_TVAL_IS_NUMBER(tv_x) && DUK_TVAL_IS_NUMBER(tv_y))) {
		return duk__compare_number(retval, DUK_TVAL_GET_NUMBER(tv_x), DUK_TVAL_GET_NUMBER(tv_y));
	}
#endif

	/* Slow path */

	duk_push_tval(thr, tv_x);
	duk_push_tval(thr, tv_y);

	if (flags & DUK_COMPARE_FLAG_EVAL_LEFT_FIRST) {
		duk_to_primitive(thr, -2, DUK_HINT_NUMBER);
		duk_to_primitive(thr, -1, DUK_HINT_NUMBER);
	} else {
		duk_to_primitive(thr, -1, DUK_HINT_NUMBER);
		duk_to_primitive(thr, -2, DUK_HINT_NUMBER);
	}

	/* Note: reuse variables */
	tv_x = DUK_GET_TVAL_NEGIDX(thr, -2);
	tv_y = DUK_GET_TVAL_NEGIDX(thr, -1);

	if (DUK_TVAL_IS_STRING(tv_x) && DUK_TVAL_IS_STRING(tv_y)) {
		duk_hstring *h1 = DUK_TVAL_GET_STRING(tv_x);
		duk_hstring *h2 = DUK_TVAL_GET_STRING(tv_y);
		DUK_ASSERT(h1 != NULL);
		DUK_ASSERT(h2 != NULL);

		if (DUK_LIKELY(!DUK_HSTRING_HAS_SYMBOL(h1) && !DUK_HSTRING_HAS_SYMBOL(h2))) {
			rc = duk_js_string_compare(h1, h2);
			duk_pop_2_unsafe(thr);
			if (rc < 0) {
				return retval ^ 1;
			} else {
				return retval;
			}
		}

		/* One or both are Symbols: fall through to handle in the
		 * generic path.  Concretely, ToNumber() will fail.
		 */
	}

	/* Ordering should not matter (E5 Section 11.8.5, step 3.a). */
#if 0
	if (flags & DUK_COMPARE_FLAG_EVAL_LEFT_FIRST) {
		d1 = duk_to_number_m2(thr);
		d2 = duk_to_number_m1(thr);
	} else {
		d2 = duk_to_number_m1(thr);
		d1 = duk_to_number_m2(thr);
	}
#endif
	d1 = duk_to_number_m2(thr);
	d2 = duk_to_number_m1(thr);

	/* We want to duk_pop_2_unsafe(thr); because the values are numbers
	 * no decref check is needed.
	 */
#if defined(DUK_USE_PREFER_SIZE)
	duk_pop_2_nodecref_unsafe(thr);
#else
	DUK_ASSERT(!DUK_TVAL_NEEDS_REFCOUNT_UPDATE(duk_get_tval(thr, -2)));
	DUK_ASSERT(!DUK_TVAL_NEEDS_REFCOUNT_UPDATE(duk_get_tval(thr, -1)));
	DUK_ASSERT(duk_get_top(thr) >= 2);
	thr->valstack_top -= 2;
	tv_x = thr->valstack_top;
	tv_y = tv_x + 1;
	DUK_TVAL_SET_UNDEFINED(tv_x); /* Value stack policy */
	DUK_TVAL_SET_UNDEFINED(tv_y);
#endif

	return duk__compare_number(retval, d1, d2);
}

/*
 *  instanceof
 */

/*
 *  ES2015 Section 7.3.19 describes the OrdinaryHasInstance() algorithm
 *  which covers both bound and non-bound functions; in effect the algorithm
 *  includes E5 Sections 11.8.6, 15.3.5.3, and 15.3.4.5.3.
 *
 *  ES2015 Section 12.9.4 describes the instanceof operator which first
 *  checks @@hasInstance well-known symbol and falls back to
 *  OrdinaryHasInstance().
 *
 *  Limited Proxy support: don't support 'getPrototypeOf' trap but
 *  continue lookup in Proxy target if the value is a Proxy.
 */

DUK_LOCAL duk_bool_t duk__js_instanceof_helper(duk_hthread *thr, duk_tval *tv_x, duk_tval *tv_y, duk_bool_t skip_sym_check) {
	duk_hobject *func;
	duk_hobject *val;
	duk_hobject *proto;
	duk_tval *tv;
	duk_bool_t skip_first;
	duk_uint_t sanity;

	/*
	 *  Get the values onto the stack first.  It would be possible to cover
	 *  some normal cases without resorting to the value stack.
	 *
	 *  The right hand side could be a light function (as they generally
	 *  behave like objects).  Light functions never have a 'prototype'
	 *  property so E5.1 Section 15.3.5.3 step 3 always throws a TypeError.
	 *  Using duk_require_hobject() is thus correct (except for error msg).
	 */

	duk_push_tval(thr, tv_x);
	duk_push_tval(thr, tv_y);
	func = duk_require_hobject(thr, -1);
	DUK_ASSERT(func != NULL);

#if defined(DUK_USE_SYMBOL_BUILTIN)
	/*
	 *  @@hasInstance check, ES2015 Section 12.9.4, Steps 2-4.
	 */
	if (!skip_sym_check) {
		if (duk_get_method_stridx(thr, -1, DUK_STRIDX_WELLKNOWN_SYMBOL_HAS_INSTANCE)) {
			/* [ ... lhs rhs func ] */
			duk_insert(thr, -3); /* -> [ ... func lhs rhs ] */
			duk_swap_top(thr, -2); /* -> [ ... func rhs(this) lhs ] */
			duk_call_method(thr, 1);
			return duk_to_boolean_top_pop(thr);
		}
	}
#else
	DUK_UNREF(skip_sym_check);
#endif

	/*
	 *  For bound objects, [[HasInstance]] just calls the target function
	 *  [[HasInstance]].  If that is again a bound object, repeat until
	 *  we find a non-bound Function object.
	 *
	 *  The bound function chain is now "collapsed" so there can be only
	 *  one bound function in the chain.
	 */

	if (!DUK_HOBJECT_IS_CALLABLE(func)) {
		/*
		 *  Note: of native ECMAScript objects, only Function instances
		 *  have a [[HasInstance]] internal property.  Custom objects might
		 *  also have it, but not in current implementation.
		 *
		 *  XXX: add a separate flag, DUK_HOBJECT_FLAG_ALLOW_INSTANCEOF?
		 */
		goto error_invalid_rval;
	}

	if (DUK_HOBJECT_HAS_BOUNDFUNC(func)) {
		duk_push_tval(thr, &((duk_hboundfunc *) (void *) func)->target);
		duk_replace(thr, -2);
		func = duk_require_hobject(thr, -1); /* lightfunc throws */

		/* Rely on Function.prototype.bind() never creating bound
		 * functions whose target is not proper.
		 */
		DUK_ASSERT(func != NULL);
		DUK_ASSERT(DUK_HOBJECT_IS_CALLABLE(func));
	}

	/*
	 *  'func' is now a non-bound object which supports [[HasInstance]]
	 *  (which here just means DUK_HOBJECT_FLAG_CALLABLE).  Move on
	 *  to execute E5 Section 15.3.5.3.
	 */

	DUK_ASSERT(func != NULL);
	DUK_ASSERT(!DUK_HOBJECT_HAS_BOUNDFUNC(func));
	DUK_ASSERT(DUK_HOBJECT_IS_CALLABLE(func));

	/* [ ... lval rval(func) ] */

	/* For lightfuncs, buffers, and pointers start the comparison directly
	 * from the virtual prototype object.
	 */
	skip_first = 0;
	tv = DUK_GET_TVAL_NEGIDX(thr, -2);
	switch (DUK_TVAL_GET_TAG(tv)) {
	case DUK_TAG_LIGHTFUNC:
		val = thr->builtins[DUK_BIDX_FUNCTION_PROTOTYPE];
		DUK_ASSERT(val != NULL);
		break;
	case DUK_TAG_BUFFER:
		val = thr->builtins[DUK_BIDX_UINT8ARRAY_PROTOTYPE];
		DUK_ASSERT(val != NULL);
		break;
	case DUK_TAG_POINTER:
		val = thr->builtins[DUK_BIDX_POINTER_PROTOTYPE];
		DUK_ASSERT(val != NULL);
		break;
	case DUK_TAG_OBJECT:
		skip_first = 1; /* Ignore object itself on first round. */
		val = DUK_TVAL_GET_OBJECT(tv);
		DUK_ASSERT(val != NULL);
		break;
	default:
		goto pop2_and_false;
	}
	DUK_ASSERT(val != NULL); /* Loop doesn't actually rely on this. */

	/* Look up .prototype of rval.  Leave it on the value stack in case it
	 * has been virtualized (e.g. getter, Proxy trap).
	 */
	duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_PROTOTYPE); /* -> [ ... lval rval rval.prototype ] */
#if defined(DUK_USE_VERBOSE_ERRORS)
	proto = duk_get_hobject(thr, -1);
	if (proto == NULL) {
		goto error_invalid_rval_noproto;
	}
#else
	proto = duk_require_hobject(thr, -1);
#endif

	sanity = DUK_HOBJECT_PROTOTYPE_CHAIN_SANITY;
	do {
		/*
		 *  Note: prototype chain is followed BEFORE first comparison.  This
		 *  means that the instanceof lval is never itself compared to the
		 *  rval.prototype property.  This is apparently intentional, see E5
		 *  Section 15.3.5.3, step 4.a.
		 *
		 *  Also note:
		 *
		 *      js> (function() {}) instanceof Function
		 *      true
		 *      js> Function instanceof Function
		 *      true
		 *
		 *  For the latter, h_proto will be Function.prototype, which is the
		 *  built-in Function prototype.  Because Function.[[Prototype]] is
		 *  also the built-in Function prototype, the result is true.
		 */

		if (!val) {
			goto pop3_and_false;
		}

		DUK_ASSERT(val != NULL);
#if defined(DUK_USE_ES6_PROXY)
		val = duk_hobject_resolve_proxy_target(val);
#endif

		if (skip_first) {
			skip_first = 0;
		} else if (val == proto) {
			goto pop3_and_true;
		}

		DUK_ASSERT(val != NULL);
		val = DUK_HOBJECT_GET_PROTOTYPE(thr->heap, val);
	} while (--sanity > 0);

	DUK_ASSERT(sanity == 0);
	DUK_ERROR_RANGE(thr, DUK_STR_PROTOTYPE_CHAIN_LIMIT);
	DUK_WO_NORETURN(return 0;);

pop2_and_false:
	duk_pop_2_unsafe(thr);
	return 0;

pop3_and_false:
	duk_pop_3_unsafe(thr);
	return 0;

pop3_and_true:
	duk_pop_3_unsafe(thr);
	return 1;

error_invalid_rval:
	DUK_ERROR_TYPE(thr, DUK_STR_INVALID_INSTANCEOF_RVAL);
	DUK_WO_NORETURN(return 0;);

#if defined(DUK_USE_VERBOSE_ERRORS)
error_invalid_rval_noproto:
	DUK_ERROR_TYPE(thr, DUK_STR_INVALID_INSTANCEOF_RVAL_NOPROTO);
	DUK_WO_NORETURN(return 0;);
#endif
}

#if defined(DUK_USE_SYMBOL_BUILTIN)
DUK_INTERNAL duk_bool_t duk_js_instanceof_ordinary(duk_hthread *thr, duk_tval *tv_x, duk_tval *tv_y) {
	return duk__js_instanceof_helper(thr, tv_x, tv_y, 1 /*skip_sym_check*/);
}
#endif

DUK_INTERNAL duk_bool_t duk_js_instanceof(duk_hthread *thr, duk_tval *tv_x, duk_tval *tv_y) {
	return duk__js_instanceof_helper(thr, tv_x, tv_y, 0 /*skip_sym_check*/);
}

/*
 *  in
 */

/*
 *  E5 Sections 11.8.7, 8.12.6.
 *
 *  Basically just a property existence check using [[HasProperty]].
 */

DUK_INTERNAL duk_bool_t duk_js_in(duk_hthread *thr, duk_tval *tv_x, duk_tval *tv_y) {
	duk_bool_t retval;

	/*
	 *  Get the values onto the stack first.  It would be possible to cover
	 *  some normal cases without resorting to the value stack (e.g. if
	 *  lval is already a string).
	 */

	/* XXX: The ES5/5.1/6 specifications require that the key in 'key in obj'
	 * must be string coerced before the internal HasProperty() algorithm is
	 * invoked.  A fast path skipping coercion could be safely implemented for
	 * numbers (as number-to-string coercion has no side effects).  For ES2015
	 * proxy behavior, the trap 'key' argument must be in a string coerced
	 * form (which is a shame).
	 */

	/* TypeError if rval is not an object or object like (e.g. lightfunc
	 * or plain buffer).
	 */
	duk_push_tval(thr, tv_x);
	duk_push_tval(thr, tv_y);
	duk_require_type_mask(thr, -1, DUK_TYPE_MASK_OBJECT | DUK_TYPE_MASK_LIGHTFUNC | DUK_TYPE_MASK_BUFFER);

	(void) duk_to_property_key_hstring(thr, -2);

	retval = duk_hobject_hasprop(thr, DUK_GET_TVAL_NEGIDX(thr, -1), DUK_GET_TVAL_NEGIDX(thr, -2));

	duk_pop_2_unsafe(thr);
	return retval;
}

/*
 *  typeof
 *
 *  E5 Section 11.4.3.
 *
 *  Very straightforward.  The only question is what to return for our
 *  non-standard tag / object types.
 *
 *  There is an unfortunate string constant define naming problem with
 *  typeof return values for e.g. "Object" and "object"; careful with
 *  the built-in string defines.  The LC_XXX defines are used for the
 *  lowercase variants now.
 */

DUK_INTERNAL duk_small_uint_t duk_js_typeof_stridx(duk_tval *tv_x) {
	duk_small_uint_t stridx = 0;

	switch (DUK_TVAL_GET_TAG(tv_x)) {
	case DUK_TAG_UNDEFINED: {
		stridx = DUK_STRIDX_LC_UNDEFINED;
		break;
	}
	case DUK_TAG_NULL: {
		/* Note: not a typo, "object" is returned for a null value. */
		stridx = DUK_STRIDX_LC_OBJECT;
		break;
	}
	case DUK_TAG_BOOLEAN: {
		stridx = DUK_STRIDX_LC_BOOLEAN;
		break;
	}
	case DUK_TAG_POINTER: {
		/* Implementation specific. */
		stridx = DUK_STRIDX_LC_POINTER;
		break;
	}
	case DUK_TAG_STRING: {
		duk_hstring *str;

		/* All internal keys are identified as Symbols. */
		str = DUK_TVAL_GET_STRING(tv_x);
		DUK_ASSERT(str != NULL);
		if (DUK_UNLIKELY(DUK_HSTRING_HAS_SYMBOL(str))) {
			stridx = DUK_STRIDX_LC_SYMBOL;
		} else {
			stridx = DUK_STRIDX_LC_STRING;
		}
		break;
	}
	case DUK_TAG_OBJECT: {
		duk_hobject *obj = DUK_TVAL_GET_OBJECT(tv_x);
		DUK_ASSERT(obj != NULL);
		if (DUK_HOBJECT_IS_CALLABLE(obj)) {
			stridx = DUK_STRIDX_LC_FUNCTION;
		} else {
			stridx = DUK_STRIDX_LC_OBJECT;
		}
		break;
	}
	case DUK_TAG_BUFFER: {
		/* Implementation specific.  In Duktape 1.x this would be
		 * 'buffer', in Duktape 2.x changed to 'object' because plain
		 * buffers now mimic Uint8Array objects.
		 */
		stridx = DUK_STRIDX_LC_OBJECT;
		break;
	}
	case DUK_TAG_LIGHTFUNC: {
		stridx = DUK_STRIDX_LC_FUNCTION;
		break;
	}
#if defined(DUK_USE_FASTINT)
	case DUK_TAG_FASTINT:
#endif
	default: {
		/* number */
		DUK_ASSERT(!DUK_TVAL_IS_UNUSED(tv_x));
		DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv_x));
		stridx = DUK_STRIDX_LC_NUMBER;
		break;
	}
	}

	DUK_ASSERT_STRIDX_VALID(stridx);
	return stridx;
}

/*
 *  IsArray()
 */

DUK_INTERNAL duk_bool_t duk_js_isarray_hobject(duk_hobject *h) {
	DUK_ASSERT(h != NULL);
#if defined(DUK_USE_ES6_PROXY)
	h = duk_hobject_resolve_proxy_target(h);
#endif
	return (DUK_HOBJECT_GET_CLASS_NUMBER(h) == DUK_HOBJECT_CLASS_ARRAY ? 1 : 0);
}

DUK_INTERNAL duk_bool_t duk_js_isarray(duk_tval *tv) {
	DUK_ASSERT(tv != NULL);
	if (DUK_TVAL_IS_OBJECT(tv)) {
		return duk_js_isarray_hobject(DUK_TVAL_GET_OBJECT(tv));
	}
	return 0;
}

/*
 *  Array index and length
 *
 *  Array index: E5 Section 15.4
 *  Array length: E5 Section 15.4.5.1 steps 3.c - 3.d (array length write)
 */

/* Compure array index from string context, or return a "not array index"
 * indicator.
 */
DUK_INTERNAL duk_uarridx_t duk_js_to_arrayindex_string(const duk_uint8_t *str, duk_uint32_t blen) {
	duk_uarridx_t res;

	/* Only strings with byte length 1-10 can be 32-bit array indices.
	 * Leading zeroes (except '0' alone), plus/minus signs are not allowed.
	 * We could do a lot of prechecks here, but since most strings won't
	 * start with any digits, it's simpler to just parse the number and
	 * fail quickly.
	 */

	res = 0;
	if (blen == 0) {
		goto parse_fail;
	}
	do {
		duk_uarridx_t dig;
		dig = (duk_uarridx_t) (*str++) - DUK_ASC_0;

		if (dig <= 9U) {
			/* Careful overflow handling.  When multiplying by 10:
			 * - 0x19999998 x 10 = 0xfffffff0: no overflow, and adding
			 *   0...9 is safe.
			 * - 0x19999999 x 10 = 0xfffffffa: no overflow, adding
			 *   0...5 is safe, 6...9 overflows.
			 * - 0x1999999a x 10 = 0x100000004: always overflow.
			 */
			if (DUK_UNLIKELY(res >= 0x19999999UL)) {
				if (res >= 0x1999999aUL) {
					/* Always overflow. */
					goto parse_fail;
				}
				DUK_ASSERT(res == 0x19999999UL);
				if (dig >= 6U) {
					goto parse_fail;
				}
				res = 0xfffffffaUL + dig;
				DUK_ASSERT(res >= 0xfffffffaUL);
				DUK_ASSERT_DISABLE(res <= 0xffffffffUL); /* range */
			} else {
				res = res * 10U + dig;
				if (DUK_UNLIKELY(res == 0)) {
					/* If 'res' is 0, previous 'res' must
					 * have been 0 and we scanned in a zero.
					 * This is only allowed if blen == 1,
					 * i.e. the exact string '0'.
					 */
					if (blen == (duk_uint32_t) 1) {
						return 0;
					}
					goto parse_fail;
				}
			}
		} else {
			/* Because 'dig' is unsigned, catches both values
			 * above '9' and below '0'.
			 */
			goto parse_fail;
		}
	} while (--blen > 0);

	return res;

parse_fail:
	return DUK_HSTRING_NO_ARRAY_INDEX;
}

#if !defined(DUK_USE_HSTRING_ARRIDX)
/* Get array index for a string which is known to be an array index.  This helper
 * is needed when duk_hstring doesn't concretely store the array index, but strings
 * are flagged as array indices at intern time.
 */
DUK_INTERNAL duk_uarridx_t duk_js_to_arrayindex_hstring_fast_known(duk_hstring *h) {
	const duk_uint8_t *p;
	duk_uarridx_t res;
	duk_uint8_t t;

	DUK_ASSERT(h != NULL);
	DUK_ASSERT(DUK_HSTRING_HAS_ARRIDX(h));

	p = DUK_HSTRING_GET_DATA(h);
	res = 0;
	for (;;) {
		t = *p++;
		if (DUK_UNLIKELY(t == 0)) {
			/* Scanning to NUL is always safe for interned strings. */
			break;
		}
		DUK_ASSERT(t >= (duk_uint8_t) DUK_ASC_0 && t <= (duk_uint8_t) DUK_ASC_9);
		res = res * 10U + (duk_uarridx_t) t - (duk_uarridx_t) DUK_ASC_0;
	}
	return res;
}

DUK_INTERNAL duk_uarridx_t duk_js_to_arrayindex_hstring_fast(duk_hstring *h) {
	DUK_ASSERT(h != NULL);
	if (!DUK_HSTRING_HAS_ARRIDX(h)) {
		return DUK_HSTRING_NO_ARRAY_INDEX;
	}
	return duk_js_to_arrayindex_hstring_fast_known(h);
}
#endif /* DUK_USE_HSTRING_ARRIDX */
