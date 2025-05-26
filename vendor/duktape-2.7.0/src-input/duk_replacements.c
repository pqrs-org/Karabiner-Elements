/*
 *  Replacements for missing platform functions.
 *
 *  Unlike the originals, fpclassify() and signbit() replacements don't
 *  work on any floating point types, only doubles.  The C typing here
 *  mimics the standard prototypes.
 */

#include "duk_internal.h"

#if defined(DUK_USE_COMPUTED_NAN)
DUK_INTERNAL double duk_computed_nan;
#endif

#if defined(DUK_USE_COMPUTED_INFINITY)
DUK_INTERNAL double duk_computed_infinity;
#endif

#if defined(DUK_USE_REPL_FPCLASSIFY)
DUK_INTERNAL int duk_repl_fpclassify(double x) {
	duk_double_union u;
	duk_uint_fast16_t expt;
	duk_small_int_t mzero;

	u.d = x;
	expt = (duk_uint_fast16_t) (u.us[DUK_DBL_IDX_US0] & 0x7ff0UL);
	if (expt > 0x0000UL && expt < 0x7ff0UL) {
		/* expt values [0x001,0x7fe] = normal */
		return DUK_FP_NORMAL;
	}

	mzero = (u.ui[DUK_DBL_IDX_UI1] == 0 && (u.ui[DUK_DBL_IDX_UI0] & 0x000fffffUL) == 0);
	if (expt == 0x0000UL) {
		/* expt 0x000 is zero/subnormal */
		if (mzero) {
			return DUK_FP_ZERO;
		} else {
			return DUK_FP_SUBNORMAL;
		}
	} else {
		/* expt 0xfff is infinite/nan */
		if (mzero) {
			return DUK_FP_INFINITE;
		} else {
			return DUK_FP_NAN;
		}
	}
}
#endif

#if defined(DUK_USE_REPL_SIGNBIT)
DUK_INTERNAL int duk_repl_signbit(double x) {
	duk_double_union u;
	u.d = x;
	return (int) (u.uc[DUK_DBL_IDX_UC0] & 0x80UL);
}
#endif

#if defined(DUK_USE_REPL_ISFINITE)
DUK_INTERNAL int duk_repl_isfinite(double x) {
	int c = DUK_FPCLASSIFY(x);
	if (c == DUK_FP_NAN || c == DUK_FP_INFINITE) {
		return 0;
	} else {
		return 1;
	}
}
#endif

#if defined(DUK_USE_REPL_ISNAN)
DUK_INTERNAL int duk_repl_isnan(double x) {
	int c = DUK_FPCLASSIFY(x);
	return (c == DUK_FP_NAN);
}
#endif

#if defined(DUK_USE_REPL_ISINF)
DUK_INTERNAL int duk_repl_isinf(double x) {
	int c = DUK_FPCLASSIFY(x);
	return (c == DUK_FP_INFINITE);
}
#endif
