/*
 *  Cast helpers.
 *
 *  C99+ coercion is challenging portability-wise because out-of-range casts
 *  may invoke implementation defined or even undefined behavior.  See e.g.
 *  http://blog.frama-c.com/index.php?post/2013/10/09/Overflow-float-integer.
 *
 *  Provide explicit cast helpers which try to avoid implementation defined
 *  or undefined behavior.  These helpers can then be simplified in the vast
 *  majority of cases where the implementation defined or undefined behavior
 *  is not problematic.
 */

#include "duk_internal.h"

/* Portable double-to-integer cast which avoids undefined behavior and avoids
 * relying on fmin(), fmax(), or other intrinsics.  Out-of-range results are
 * not assumed by caller, but here value is clamped, NaN converts to minval.
 */
#define DUK__DOUBLE_INT_CAST1(tname, minval, maxval) \
	do { \
		if (DUK_LIKELY(x >= (duk_double_t) (minval))) { \
			DUK_ASSERT(!DUK_ISNAN(x)); \
			if (DUK_LIKELY(x <= (duk_double_t) (maxval))) { \
				return (tname) x; \
			} else { \
				return (tname) (maxval); \
			} \
		} else { \
			/* NaN or below minval.  Since we don't care about the result \
			 * for out-of-range values, just return the minimum value for \
			 * both. \
			 */ \
			return (tname) (minval); \
		} \
	} while (0)

/* Rely on specific NaN behavior for duk_double_{fmin,fmax}(): if either
 * argument is a NaN, return the second argument.  This avoids a
 * NaN-to-integer cast which is undefined behavior.
 */
#define DUK__DOUBLE_INT_CAST2(tname, minval, maxval) \
	do { \
		return (tname) duk_double_fmin(duk_double_fmax(x, (duk_double_t) (minval)), (duk_double_t) (maxval)); \
	} while (0)

/* Another solution which doesn't need C99+ behavior for fmin() and fmax(). */
#define DUK__DOUBLE_INT_CAST3(tname, minval, maxval) \
	do { \
		if (DUK_ISNAN(x)) { \
			/* 0 or any other value is fine. */ \
			return (tname) 0; \
		} else \
			return (tname) DUK_FMIN(DUK_FMAX(x, (duk_double_t) (minval)), (duk_double_t) (maxval)); \
	} \
	} \
	while (0)

/* C99+ solution: relies on specific fmin() and fmax() behavior in C99: if
 * one argument is NaN but the other isn't, the non-NaN argument is returned.
 * Because the limits are non-NaN values, explicit NaN check is not needed.
 * This may not work on all legacy platforms, and also doesn't seem to inline
 * the fmin() and fmax() calls (unless one uses -ffast-math which we don't
 * support).
 */
#define DUK__DOUBLE_INT_CAST4(tname, minval, maxval) \
	do { \
		return (tname) DUK_FMIN(DUK_FMAX(x, (duk_double_t) (minval)), (duk_double_t) (maxval)); \
	} while (0)

DUK_INTERNAL duk_int_t duk_double_to_int_t(duk_double_t x) {
#if defined(DUK_USE_ALLOW_UNDEFINED_BEHAVIOR)
	/* Real world solution: almost any practical platform will provide
	 * an integer value without any guarantees what it is (which is fine).
	 */
	return (duk_int_t) x;
#else
	DUK__DOUBLE_INT_CAST1(duk_int_t, DUK_INT_MIN, DUK_INT_MAX);
#endif
}

DUK_INTERNAL duk_uint_t duk_double_to_uint_t(duk_double_t x) {
#if defined(DUK_USE_ALLOW_UNDEFINED_BEHAVIOR)
	return (duk_uint_t) x;
#else
	DUK__DOUBLE_INT_CAST1(duk_uint_t, DUK_UINT_MIN, DUK_UINT_MAX);
#endif
}

DUK_INTERNAL duk_int32_t duk_double_to_int32_t(duk_double_t x) {
#if defined(DUK_USE_ALLOW_UNDEFINED_BEHAVIOR)
	return (duk_int32_t) x;
#else
	DUK__DOUBLE_INT_CAST1(duk_int32_t, DUK_INT32_MIN, DUK_INT32_MAX);
#endif
}

DUK_INTERNAL duk_uint32_t duk_double_to_uint32_t(duk_double_t x) {
#if defined(DUK_USE_ALLOW_UNDEFINED_BEHAVIOR)
	return (duk_uint32_t) x;
#else
	DUK__DOUBLE_INT_CAST1(duk_uint32_t, DUK_UINT32_MIN, DUK_UINT32_MAX);
#endif
}

/* Largest IEEE double that doesn't round to infinity in the default rounding
 * mode.  The exact midpoint between (1 - 2^(-24)) * 2^128 and 2^128 rounds to
 * infinity, at least on x64.  This number is one double unit below that
 * midpoint.  See misc/float_cast.c.
 */
#define DUK__FLOAT_ROUND_LIMIT 340282356779733623858607532500980858880.0

/* Maximum IEEE float.  Double-to-float conversion above this would be out of
 * range and thus technically undefined behavior.
 */
#define DUK__FLOAT_MAX 340282346638528859811704183484516925440.0

DUK_INTERNAL duk_float_t duk_double_to_float_t(duk_double_t x) {
	/* Even a double-to-float cast is technically undefined behavior if
	 * the double is out-of-range.  C99 Section 6.3.1.5:
	 *
	 *   If the value being converted is in the range of values that can
	 *   be represented but cannot be represented exactly, the result is
	 *   either the nearest higher or nearest lower representable value,
	 *   chosen in an implementation-defined manner.  If the value being
	 *   converted is outside the range of values that can be represented,
	 *   the behavior is undefined.
	 */
#if defined(DUK_USE_ALLOW_UNDEFINED_BEHAVIOR)
	return (duk_float_t) x;
#else
	duk_double_t t;

	t = DUK_FABS(x);
	DUK_ASSERT((DUK_ISNAN(x) && DUK_ISNAN(t)) || (!DUK_ISNAN(x) && !DUK_ISNAN(t)));

	if (DUK_LIKELY(t <= DUK__FLOAT_MAX)) {
		/* Standard in-range case, try to get here with a minimum
		 * number of checks and branches.
		 */
		DUK_ASSERT(!DUK_ISNAN(x));
		return (duk_float_t) x;
	} else if (t <= DUK__FLOAT_ROUND_LIMIT) {
		/* Out-of-range, but rounds to min/max float. */
		DUK_ASSERT(!DUK_ISNAN(x));
		if (x < 0.0) {
			return (duk_float_t) -DUK__FLOAT_MAX;
		} else {
			return (duk_float_t) DUK__FLOAT_MAX;
		}
	} else if (DUK_ISNAN(x)) {
		/* Assumes double NaN -> float NaN considered "in range". */
		DUK_ASSERT(DUK_ISNAN(x));
		return (duk_float_t) x;
	} else {
		/* Out-of-range, rounds to +/- Infinity. */
		if (x < 0.0) {
			return (duk_float_t) -DUK_DOUBLE_INFINITY;
		} else {
			return (duk_float_t) DUK_DOUBLE_INFINITY;
		}
	}
#endif
}
