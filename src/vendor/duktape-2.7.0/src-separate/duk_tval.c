#include "duk_internal.h"

#if defined(DUK_USE_FASTINT)

/*
 *  Manually optimized double-to-fastint downgrade check.
 *
 *  This check has a large impact on performance, especially for fastint
 *  slow paths, so must be changed carefully.  The code should probably be
 *  optimized for the case where the result does not fit into a fastint,
 *  to minimize the penalty for "slow path code" dealing with fractions etc.
 *
 *  At least on one tested soft float ARM platform double-to-int64 coercion
 *  is very slow (and sometimes produces incorrect results, see self tests).
 *  This algorithm combines a fastint compatibility check and extracting the
 *  integer value from an IEEE double for setting the tagged fastint.  For
 *  other platforms a more naive approach might be better.
 *
 *  See doc/fastint.rst for details.
 */

DUK_INTERNAL DUK_ALWAYS_INLINE void duk_tval_set_number_chkfast_fast(duk_tval *tv, duk_double_t x) {
	duk_double_union du;
	duk_int64_t i;
	duk_small_int_t expt;
	duk_small_int_t shift;

	/* XXX: optimize for packed duk_tval directly? */

	du.d = x;
	i = (duk_int64_t) DUK_DBLUNION_GET_INT64(&du);
	expt = (duk_small_int_t) ((i >> 52) & 0x07ff);
	shift = expt - 1023;

	if (shift >= 0 && shift <= 46) { /* exponents 1023 to 1069 */
		duk_int64_t t;

		if (((DUK_I64_CONSTANT(0x000fffffffffffff) >> shift) & i) == 0) {
			t = i | DUK_I64_CONSTANT(0x0010000000000000); /* implicit leading one */
			t = t & DUK_I64_CONSTANT(0x001fffffffffffff);
			t = t >> (52 - shift);
			if (i < 0) {
				t = -t;
			}
			DUK_TVAL_SET_FASTINT(tv, t);
			return;
		}
	} else if (shift == -1023) { /* exponent 0 */
		if (i >= 0 && (i & DUK_I64_CONSTANT(0x000fffffffffffff)) == 0) {
			/* Note: reject negative zero. */
			DUK_TVAL_SET_FASTINT(tv, (duk_int64_t) 0);
			return;
		}
	} else if (shift == 47) { /* exponent 1070 */
		if (i < 0 && (i & DUK_I64_CONSTANT(0x000fffffffffffff)) == 0) {
			DUK_TVAL_SET_FASTINT(tv, (duk_int64_t) DUK_FASTINT_MIN);
			return;
		}
	}

	DUK_TVAL_SET_DOUBLE(tv, x);
	return;
}

DUK_INTERNAL DUK_NOINLINE void duk_tval_set_number_chkfast_slow(duk_tval *tv, duk_double_t x) {
	duk_tval_set_number_chkfast_fast(tv, x);
}

/*
 *  Manually optimized number-to-double conversion
 */

#if defined(DUK_USE_FASTINT) && defined(DUK_USE_PACKED_TVAL)
DUK_INTERNAL DUK_ALWAYS_INLINE duk_double_t duk_tval_get_number_packed(duk_tval *tv) {
	duk_double_union du;
	duk_uint64_t t;

	t = (duk_uint64_t) DUK_DBLUNION_GET_UINT64(tv);
	if ((t >> 48) != DUK_TAG_FASTINT) {
		return tv->d;
	} else if (t & DUK_U64_CONSTANT(0x0000800000000000)) {
		t = (duk_uint64_t) (-((duk_int64_t) t)); /* avoid unary minus on unsigned */
		t = t & DUK_U64_CONSTANT(0x0000ffffffffffff); /* negative */
		t |= DUK_U64_CONSTANT(0xc330000000000000);
		DUK_DBLUNION_SET_UINT64(&du, t);
		return du.d + 4503599627370496.0; /* 1 << 52 */
	} else if (t != 0) {
		t &= DUK_U64_CONSTANT(0x0000ffffffffffff); /* positive */
		t |= DUK_U64_CONSTANT(0x4330000000000000);
		DUK_DBLUNION_SET_UINT64(&du, t);
		return du.d - 4503599627370496.0; /* 1 << 52 */
	} else {
		return 0.0; /* zero */
	}
}
#endif /* DUK_USE_FASTINT && DUK_USE_PACKED_TVAL */

#if 0 /* unused */
#if defined(DUK_USE_FASTINT) && !defined(DUK_USE_PACKED_TVAL)
DUK_INTERNAL DUK_ALWAYS_INLINE duk_double_t duk_tval_get_number_unpacked(duk_tval *tv) {
	duk_double_union du;
	duk_uint64_t t;

	DUK_ASSERT(tv->t == DUK_TAG_NUMBER || tv->t == DUK_TAG_FASTINT);

	if (tv->t == DUK_TAG_FASTINT) {
		if (tv->v.fi >= 0) {
			t = DUK_U64_CONSTANT(0x4330000000000000) | (duk_uint64_t) tv->v.fi;
			DUK_DBLUNION_SET_UINT64(&du, t);
			return du.d - 4503599627370496.0;  /* 1 << 52 */
		} else {
			t = DUK_U64_CONSTANT(0xc330000000000000) | (duk_uint64_t) (-tv->v.fi);
			DUK_DBLUNION_SET_UINT64(&du, t);
			return du.d + 4503599627370496.0;  /* 1 << 52 */
		}
	} else {
		return tv->v.d;
	}
}
#endif /* DUK_USE_FASTINT && DUK_USE_PACKED_TVAL */
#endif /* 0 */

#if defined(DUK_USE_FASTINT) && !defined(DUK_USE_PACKED_TVAL)
DUK_INTERNAL DUK_ALWAYS_INLINE duk_double_t duk_tval_get_number_unpacked_fastint(duk_tval *tv) {
	duk_double_union du;
	duk_uint64_t t;

	DUK_ASSERT(tv->t == DUK_TAG_FASTINT);

	if (tv->v.fi >= 0) {
		t = DUK_U64_CONSTANT(0x4330000000000000) | (duk_uint64_t) tv->v.fi;
		DUK_DBLUNION_SET_UINT64(&du, t);
		return du.d - 4503599627370496.0; /* 1 << 52 */
	} else {
		t = DUK_U64_CONSTANT(0xc330000000000000) | (duk_uint64_t) (-tv->v.fi);
		DUK_DBLUNION_SET_UINT64(&du, t);
		return du.d + 4503599627370496.0; /* 1 << 52 */
	}
}
#endif /* DUK_USE_FASTINT && DUK_USE_PACKED_TVAL */

#endif /* DUK_USE_FASTINT */

/*
 *  Assertion helpers.
 */

#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL void duk_tval_assert_valid(duk_tval *tv) {
	DUK_ASSERT(tv != NULL);
}
#endif
