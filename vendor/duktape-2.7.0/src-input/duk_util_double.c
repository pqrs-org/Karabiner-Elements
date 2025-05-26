/*
 *  IEEE double helpers.
 */

#include "duk_internal.h"

DUK_INTERNAL duk_bool_t duk_double_is_anyinf(duk_double_t x) {
	duk_double_union du;
	du.d = x;
	return DUK_DBLUNION_IS_ANYINF(&du);
}

DUK_INTERNAL duk_bool_t duk_double_is_posinf(duk_double_t x) {
	duk_double_union du;
	du.d = x;
	return DUK_DBLUNION_IS_POSINF(&du);
}

DUK_INTERNAL duk_bool_t duk_double_is_neginf(duk_double_t x) {
	duk_double_union du;
	du.d = x;
	return DUK_DBLUNION_IS_NEGINF(&du);
}

DUK_INTERNAL duk_bool_t duk_double_is_nan(duk_double_t x) {
	duk_double_union du;
	du.d = x;
	/* Assumes we're dealing with a Duktape internal NaN which is
	 * NaN normalized if duk_tval requires it.
	 */
	DUK_ASSERT(DUK_DBLUNION_IS_NORMALIZED(&du));
	return DUK_DBLUNION_IS_NAN(&du);
}

DUK_INTERNAL duk_bool_t duk_double_is_nan_or_zero(duk_double_t x) {
	duk_double_union du;
	du.d = x;
	/* Assumes we're dealing with a Duktape internal NaN which is
	 * NaN normalized if duk_tval requires it.
	 */
	DUK_ASSERT(DUK_DBLUNION_IS_NORMALIZED(&du));
	return DUK_DBLUNION_IS_NAN(&du) || DUK_DBLUNION_IS_ANYZERO(&du);
}

DUK_INTERNAL duk_bool_t duk_double_is_nan_or_inf(duk_double_t x) {
	duk_double_union du;
	du.d = x;
	/* If exponent is 0x7FF the argument is either a NaN or an
	 * infinity.  We don't need to check any other fields.
	 */
#if defined(DUK_USE_64BIT_OPS)
#if defined(DUK_USE_DOUBLE_ME)
	return (du.ull[DUK_DBL_IDX_ULL0] & DUK_U64_CONSTANT(0x000000007ff00000)) == DUK_U64_CONSTANT(0x000000007ff00000);
#else
	return (du.ull[DUK_DBL_IDX_ULL0] & DUK_U64_CONSTANT(0x7ff0000000000000)) == DUK_U64_CONSTANT(0x7ff0000000000000);
#endif
#else
	return (du.ui[DUK_DBL_IDX_UI0] & 0x7ff00000UL) == 0x7ff00000UL;
#endif
}

DUK_INTERNAL duk_bool_t duk_double_is_nan_zero_inf(duk_double_t x) {
	duk_double_union du;
#if defined(DUK_USE_64BIT_OPS)
	duk_uint64_t t;
#else
	duk_uint32_t t;
#endif
	du.d = x;
#if defined(DUK_USE_64BIT_OPS)
#if defined(DUK_USE_DOUBLE_ME)
	t = du.ull[DUK_DBL_IDX_ULL0] & DUK_U64_CONSTANT(0x000000007ff00000);
	if (t == DUK_U64_CONSTANT(0x0000000000000000)) {
		t = du.ull[DUK_DBL_IDX_ULL0] & DUK_U64_CONSTANT(0x0000000080000000);
		return t == 0;
	}
	if (t == DUK_U64_CONSTANT(0x000000007ff00000)) {
		return 1;
	}
#else
	t = du.ull[DUK_DBL_IDX_ULL0] & DUK_U64_CONSTANT(0x7ff0000000000000);
	if (t == DUK_U64_CONSTANT(0x0000000000000000)) {
		t = du.ull[DUK_DBL_IDX_ULL0] & DUK_U64_CONSTANT(0x8000000000000000);
		return t == 0;
	}
	if (t == DUK_U64_CONSTANT(0x7ff0000000000000)) {
		return 1;
	}
#endif
#else
	t = du.ui[DUK_DBL_IDX_UI0] & 0x7ff00000UL;
	if (t == 0x00000000UL) {
		return DUK_DBLUNION_IS_ANYZERO(&du);
	}
	if (t == 0x7ff00000UL) {
		return 1;
	}
#endif
	return 0;
}

DUK_INTERNAL duk_small_uint_t duk_double_signbit(duk_double_t x) {
	duk_double_union du;
	du.d = x;
	return (duk_small_uint_t) DUK_DBLUNION_GET_SIGNBIT(&du);
}

DUK_INTERNAL duk_double_t duk_double_trunc_towards_zero(duk_double_t x) {
	/* XXX: optimize */
	duk_small_uint_t s = duk_double_signbit(x);
	x = DUK_FLOOR(DUK_FABS(x)); /* truncate towards zero */
	if (s) {
		x = -x;
	}
	return x;
}

DUK_INTERNAL duk_bool_t duk_double_same_sign(duk_double_t x, duk_double_t y) {
	duk_double_union du1;
	duk_double_union du2;
	du1.d = x;
	du2.d = y;

	return (((du1.ui[DUK_DBL_IDX_UI0] ^ du2.ui[DUK_DBL_IDX_UI0]) & 0x80000000UL) == 0);
}

DUK_INTERNAL duk_double_t duk_double_fmin(duk_double_t x, duk_double_t y) {
	/* Doesn't replicate fmin() behavior exactly: for fmin() if one
	 * argument is a NaN, the other argument should be returned.
	 * Duktape doesn't rely on this behavior so the replacement can
	 * be simplified.
	 */
	return (x < y ? x : y);
}

DUK_INTERNAL duk_double_t duk_double_fmax(duk_double_t x, duk_double_t y) {
	/* Doesn't replicate fmax() behavior exactly: for fmax() if one
	 * argument is a NaN, the other argument should be returned.
	 * Duktape doesn't rely on this behavior so the replacement can
	 * be simplified.
	 */
	return (x > y ? x : y);
}

DUK_INTERNAL duk_bool_t duk_double_is_finite(duk_double_t x) {
	return !duk_double_is_nan_or_inf(x);
}

DUK_INTERNAL duk_bool_t duk_double_is_integer(duk_double_t x) {
	if (duk_double_is_nan_or_inf(x)) {
		return 0;
	} else {
		return duk_double_equals(duk_js_tointeger_number(x), x);
	}
}

DUK_INTERNAL duk_bool_t duk_double_is_safe_integer(duk_double_t x) {
	/* >>> 2**53-1
	 * 9007199254740991
	 */
	return duk_double_is_integer(x) && DUK_FABS(x) <= 9007199254740991.0;
}

/* Check whether a duk_double_t is a whole number in the 32-bit range (reject
 * negative zero), and if so, return a duk_int32_t.
 * For compiler use: don't allow negative zero as it will cause trouble with
 * LDINT+LDINTX, positive zero is OK.
 */
DUK_INTERNAL duk_bool_t duk_is_whole_get_int32_nonegzero(duk_double_t x, duk_int32_t *ival) {
	duk_int32_t t;

	t = duk_double_to_int32_t(x);
	if (!duk_double_equals((duk_double_t) t, x)) {
		return 0;
	}
	if (t == 0) {
		duk_double_union du;
		du.d = x;
		if (DUK_DBLUNION_HAS_SIGNBIT(&du)) {
			return 0;
		}
	}
	*ival = t;
	return 1;
}

/* Check whether a duk_double_t is a whole number in the 32-bit range, and if
 * so, return a duk_int32_t.
 */
DUK_INTERNAL duk_bool_t duk_is_whole_get_int32(duk_double_t x, duk_int32_t *ival) {
	duk_int32_t t;

	t = duk_double_to_int32_t(x);
	if (!duk_double_equals((duk_double_t) t, x)) {
		return 0;
	}
	*ival = t;
	return 1;
}

/* Division: division by zero is undefined behavior (and may in fact trap)
 * so it needs special handling for portability.
 */

DUK_INTERNAL DUK_INLINE duk_double_t duk_double_div(duk_double_t x, duk_double_t y) {
#if !defined(DUK_USE_ALLOW_UNDEFINED_BEHAVIOR)
	if (DUK_UNLIKELY(duk_double_equals(y, 0.0) != 0)) {
		/* In C99+ division by zero is undefined behavior so
		 * avoid it entirely.  Hopefully the compiler is
		 * smart enough to avoid emitting any actual code
		 * because almost all practical platforms behave as
		 * expected.
		 */
		if (x > 0.0) {
			if (DUK_SIGNBIT(y)) {
				return -DUK_DOUBLE_INFINITY;
			} else {
				return DUK_DOUBLE_INFINITY;
			}
		} else if (x < 0.0) {
			if (DUK_SIGNBIT(y)) {
				return DUK_DOUBLE_INFINITY;
			} else {
				return -DUK_DOUBLE_INFINITY;
			}
		} else {
			/* +/- 0, NaN */
			return DUK_DOUBLE_NAN;
		}
	}
#endif

	return x / y;
}

/* Double and float byteorder changes. */

DUK_INTERNAL DUK_INLINE void duk_dblunion_host_to_little(duk_double_union *u) {
#if defined(DUK_USE_DOUBLE_LE)
	/* HGFEDCBA -> HGFEDCBA */
	DUK_UNREF(u);
#elif defined(DUK_USE_DOUBLE_ME)
	duk_uint32_t a, b;

	/* DCBAHGFE -> HGFEDCBA */
	a = u->ui[0];
	b = u->ui[1];
	u->ui[0] = b;
	u->ui[1] = a;
#elif defined(DUK_USE_DOUBLE_BE)
	/* ABCDEFGH -> HGFEDCBA */
#if defined(DUK_USE_64BIT_OPS)
	u->ull[0] = DUK_BSWAP64(u->ull[0]);
#else
	duk_uint32_t a, b;

	a = u->ui[0];
	b = u->ui[1];
	u->ui[0] = DUK_BSWAP32(b);
	u->ui[1] = DUK_BSWAP32(a);
#endif
#else
#error internal error
#endif
}

DUK_INTERNAL DUK_INLINE void duk_dblunion_little_to_host(duk_double_union *u) {
	duk_dblunion_host_to_little(u);
}

DUK_INTERNAL DUK_INLINE void duk_dblunion_host_to_big(duk_double_union *u) {
#if defined(DUK_USE_DOUBLE_LE)
	/* HGFEDCBA -> ABCDEFGH */
#if defined(DUK_USE_64BIT_OPS)
	u->ull[0] = DUK_BSWAP64(u->ull[0]);
#else
	duk_uint32_t a, b;

	a = u->ui[0];
	b = u->ui[1];
	u->ui[0] = DUK_BSWAP32(b);
	u->ui[1] = DUK_BSWAP32(a);
#endif
#elif defined(DUK_USE_DOUBLE_ME)
	duk_uint32_t a, b;

	/* DCBAHGFE -> ABCDEFGH */
	a = u->ui[0];
	b = u->ui[1];
	u->ui[0] = DUK_BSWAP32(a);
	u->ui[1] = DUK_BSWAP32(b);
#elif defined(DUK_USE_DOUBLE_BE)
	/* ABCDEFGH -> ABCDEFGH */
	DUK_UNREF(u);
#else
#error internal error
#endif
}

DUK_INTERNAL DUK_INLINE void duk_dblunion_big_to_host(duk_double_union *u) {
	duk_dblunion_host_to_big(u);
}

DUK_INTERNAL DUK_INLINE void duk_fltunion_host_to_big(duk_float_union *u) {
#if defined(DUK_USE_DOUBLE_LE) || defined(DUK_USE_DOUBLE_ME)
	/* DCBA -> ABCD */
	u->ui[0] = DUK_BSWAP32(u->ui[0]);
#elif defined(DUK_USE_DOUBLE_BE)
	/* ABCD -> ABCD */
	DUK_UNREF(u);
#else
#error internal error
#endif
}

DUK_INTERNAL DUK_INLINE void duk_fltunion_big_to_host(duk_float_union *u) {
	duk_fltunion_host_to_big(u);
}

/* Comparison: ensures comparison operates on exactly correct types, avoiding
 * some floating point comparison pitfalls (e.g. atan2() assertions failed on
 * -m32 with direct comparison, even with explicit casts).
 */
#if defined(DUK_USE_GCC_PRAGMAS)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#elif defined(DUK_USE_CLANG_PRAGMAS)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#endif

DUK_INTERNAL DUK_ALWAYS_INLINE duk_bool_t duk_double_equals(duk_double_t x, duk_double_t y) {
	return x == y;
}

DUK_INTERNAL DUK_ALWAYS_INLINE duk_bool_t duk_float_equals(duk_float_t x, duk_float_t y) {
	return x == y;
}
#if defined(DUK_USE_GCC_PRAGMAS)
#pragma GCC diagnostic pop
#elif defined(DUK_USE_CLANG_PRAGMAS)
#pragma clang diagnostic pop
#endif
