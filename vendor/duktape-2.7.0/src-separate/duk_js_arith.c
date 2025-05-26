/*
 *  Shared helpers for arithmetic operations
 */

#include "duk_internal.h"

/* ECMAScript modulus ('%') does not match IEEE 754 "remainder" operation
 * (implemented by remainder() in C99) but does seem to match ANSI C fmod().
 * Compare E5 Section 11.5.3 and "man fmod".
 */
DUK_INTERNAL double duk_js_arith_mod(double d1, double d2) {
#if defined(DUK_USE_POW_WORKAROUNDS)
	/* Specific fixes to common fmod() implementation issues:
	 * - test-bug-mingw-math-issues.js
	 */
	if (DUK_ISINF(d2)) {
		if (DUK_ISINF(d1)) {
			return DUK_DOUBLE_NAN;
		} else {
			return d1;
		}
	} else if (duk_double_equals(d1, 0.0)) {
		/* d1 +/-0 is returned as is (preserving sign) except when
		 * d2 is zero or NaN.
		 */
		if (duk_double_equals(d2, 0.0) || DUK_ISNAN(d2)) {
			return DUK_DOUBLE_NAN;
		} else {
			return d1;
		}
	}
#else
	/* Some ISO C assumptions. */
	DUK_ASSERT(duk_double_equals(DUK_FMOD(1.0, DUK_DOUBLE_INFINITY), 1.0));
	DUK_ASSERT(duk_double_equals(DUK_FMOD(-1.0, DUK_DOUBLE_INFINITY), -1.0));
	DUK_ASSERT(duk_double_equals(DUK_FMOD(1.0, -DUK_DOUBLE_INFINITY), 1.0));
	DUK_ASSERT(duk_double_equals(DUK_FMOD(-1.0, -DUK_DOUBLE_INFINITY), -1.0));
	DUK_ASSERT(DUK_ISNAN(DUK_FMOD(DUK_DOUBLE_INFINITY, DUK_DOUBLE_INFINITY)));
	DUK_ASSERT(DUK_ISNAN(DUK_FMOD(DUK_DOUBLE_INFINITY, -DUK_DOUBLE_INFINITY)));
	DUK_ASSERT(DUK_ISNAN(DUK_FMOD(-DUK_DOUBLE_INFINITY, DUK_DOUBLE_INFINITY)));
	DUK_ASSERT(DUK_ISNAN(DUK_FMOD(-DUK_DOUBLE_INFINITY, -DUK_DOUBLE_INFINITY)));
	DUK_ASSERT(duk_double_equals(DUK_FMOD(0.0, 1.0), 0.0) && DUK_SIGNBIT(DUK_FMOD(0.0, 1.0)) == 0);
	DUK_ASSERT(duk_double_equals(DUK_FMOD(-0.0, 1.0), 0.0) && DUK_SIGNBIT(DUK_FMOD(-0.0, 1.0)) != 0);
	DUK_ASSERT(duk_double_equals(DUK_FMOD(0.0, DUK_DOUBLE_INFINITY), 0.0) &&
	           DUK_SIGNBIT(DUK_FMOD(0.0, DUK_DOUBLE_INFINITY)) == 0);
	DUK_ASSERT(duk_double_equals(DUK_FMOD(-0.0, DUK_DOUBLE_INFINITY), 0.0) &&
	           DUK_SIGNBIT(DUK_FMOD(-0.0, DUK_DOUBLE_INFINITY)) != 0);
	DUK_ASSERT(duk_double_equals(DUK_FMOD(0.0, -DUK_DOUBLE_INFINITY), 0.0) &&
	           DUK_SIGNBIT(DUK_FMOD(0.0, DUK_DOUBLE_INFINITY)) == 0);
	DUK_ASSERT(duk_double_equals(DUK_FMOD(-0.0, -DUK_DOUBLE_INFINITY), 0.0) &&
	           DUK_SIGNBIT(DUK_FMOD(-0.0, -DUK_DOUBLE_INFINITY)) != 0);
	DUK_ASSERT(DUK_ISNAN(DUK_FMOD(0.0, 0.0)));
	DUK_ASSERT(DUK_ISNAN(DUK_FMOD(-0.0, 0.0)));
	DUK_ASSERT(DUK_ISNAN(DUK_FMOD(0.0, -0.0)));
	DUK_ASSERT(DUK_ISNAN(DUK_FMOD(-0.0, -0.0)));
	DUK_ASSERT(DUK_ISNAN(DUK_FMOD(0.0, DUK_DOUBLE_NAN)));
	DUK_ASSERT(DUK_ISNAN(DUK_FMOD(-0.0, DUK_DOUBLE_NAN)));
#endif

	return (duk_double_t) DUK_FMOD((double) d1, (double) d2);
}

/* Shared helper for Math.pow() and exponentiation operator. */
DUK_INTERNAL double duk_js_arith_pow(double x, double y) {
	/* The ANSI C pow() semantics differ from ECMAScript.
	 *
	 * E.g. when x==1 and y is +/- infinite, the ECMAScript required
	 * result is NaN, while at least Linux pow() returns 1.
	 */

	duk_small_int_t cx, cy, sx;

	DUK_UNREF(cx);
	DUK_UNREF(sx);
	cy = (duk_small_int_t) DUK_FPCLASSIFY(y);

	if (cy == DUK_FP_NAN) {
		goto ret_nan;
	}
	if (duk_double_equals(DUK_FABS(x), 1.0) && cy == DUK_FP_INFINITE) {
		goto ret_nan;
	}

#if defined(DUK_USE_POW_WORKAROUNDS)
	/* Specific fixes to common pow() implementation issues:
	 *   - test-bug-netbsd-math-pow.js: NetBSD 6.0 on x86 (at least)
	 *   - test-bug-mingw-math-issues.js
	 */
	cx = (duk_small_int_t) DUK_FPCLASSIFY(x);
	if (cx == DUK_FP_ZERO && y < 0.0) {
		sx = (duk_small_int_t) DUK_SIGNBIT(x);
		if (sx == 0) {
			/* Math.pow(+0,y) should be Infinity when y<0.  NetBSD pow()
			 * returns -Infinity instead when y is <0 and finite.  The
			 * if-clause also catches y == -Infinity (which works even
			 * without the fix).
			 */
			return DUK_DOUBLE_INFINITY;
		} else {
			/* Math.pow(-0,y) where y<0 should be:
			 *   - -Infinity if y<0 and an odd integer
			 *   - Infinity if y<0 but not an odd integer
			 * NetBSD pow() returns -Infinity for all finite y<0.  The
			 * if-clause also catches y == -Infinity (which works even
			 * without the fix).
			 */

			/* fmod() return value has same sign as input (negative) so
			 * the result here will be in the range ]-2,0], -1 indicates
			 * odd.  If x is -Infinity, NaN is returned and the odd check
			 * always concludes "not odd" which results in desired outcome.
			 */
			double tmp = DUK_FMOD(y, 2);
			if (tmp == -1.0) {
				return -DUK_DOUBLE_INFINITY;
			} else {
				/* Not odd, or y == -Infinity */
				return DUK_DOUBLE_INFINITY;
			}
		}
	} else if (cx == DUK_FP_NAN) {
		if (duk_double_equals(y, 0.0)) {
			/* NaN ** +/- 0 should always be 1, but is NaN on
			 * at least some Cygwin/MinGW versions.
			 */
			return 1.0;
		}
	}
#else
	/* Some ISO C assumptions. */
	DUK_ASSERT(duk_double_equals(DUK_POW(DUK_DOUBLE_NAN, 0.0), 1.0));
	DUK_ASSERT(DUK_ISINF(DUK_POW(0.0, -1.0)) && DUK_SIGNBIT(DUK_POW(0.0, -1.0)) == 0);
	DUK_ASSERT(DUK_ISINF(DUK_POW(-0.0, -2.0)) && DUK_SIGNBIT(DUK_POW(-0.0, -2.0)) == 0);
	DUK_ASSERT(DUK_ISINF(DUK_POW(-0.0, -3.0)) && DUK_SIGNBIT(DUK_POW(-0.0, -3.0)) != 0);
#endif

	return DUK_POW(x, y);

ret_nan:
	return DUK_DOUBLE_NAN;
}
