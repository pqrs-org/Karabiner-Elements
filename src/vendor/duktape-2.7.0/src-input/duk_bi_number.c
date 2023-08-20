/*
 *  Number built-ins
 */

#include "duk_internal.h"

#if defined(DUK_USE_NUMBER_BUILTIN)

DUK_LOCAL duk_double_t duk__push_this_number_plain(duk_hthread *thr) {
	duk_hobject *h;

	/* Number built-in accepts a plain number or a Number object (whose
	 * internal value is operated on).  Other types cause TypeError.
	 */

	duk_push_this(thr);
	if (duk_is_number(thr, -1)) {
		DUK_DDD(DUK_DDDPRINT("plain number value: %!T", (duk_tval *) duk_get_tval(thr, -1)));
		goto done;
	}
	h = duk_get_hobject(thr, -1);
	if (!h || (DUK_HOBJECT_GET_CLASS_NUMBER(h) != DUK_HOBJECT_CLASS_NUMBER)) {
		DUK_DDD(DUK_DDDPRINT("unacceptable this value: %!T", (duk_tval *) duk_get_tval(thr, -1)));
		DUK_ERROR_TYPE(thr, "number expected");
		DUK_WO_NORETURN(return 0.0;);
	}
	duk_xget_owndataprop_stridx_short(thr, -1, DUK_STRIDX_INT_VALUE);
	DUK_ASSERT(duk_is_number(thr, -1));
	DUK_DDD(DUK_DDDPRINT("number object: %!T, internal value: %!T",
	                     (duk_tval *) duk_get_tval(thr, -2),
	                     (duk_tval *) duk_get_tval(thr, -1)));
	duk_remove_m2(thr);

done:
	return duk_get_number(thr, -1);
}

DUK_INTERNAL duk_ret_t duk_bi_number_constructor(duk_hthread *thr) {
	duk_idx_t nargs;
	duk_hobject *h_this;

	/*
	 *  The Number constructor uses ToNumber(arg) for number coercion
	 *  (coercing an undefined argument to NaN).  However, if the
	 *  argument is not given at all, +0 must be used instead.  To do
	 *  this, a vararg function is used.
	 */

	nargs = duk_get_top(thr);
	if (nargs == 0) {
		duk_push_int(thr, 0);
	}
	duk_to_number(thr, 0);
	duk_set_top(thr, 1);
	DUK_ASSERT_TOP(thr, 1);

	if (!duk_is_constructor_call(thr)) {
		return 1;
	}

	/*
	 *  E5 Section 15.7.2.1 requires that the constructed object
	 *  must have the original Number.prototype as its internal
	 *  prototype.  However, since Number.prototype is non-writable
	 *  and non-configurable, this doesn't have to be enforced here:
	 *  The default object (bound to 'this') is OK, though we have
	 *  to change its class.
	 *
	 *  Internal value set to ToNumber(arg) or +0; if no arg given,
	 *  ToNumber(undefined) = NaN, so special treatment is needed
	 *  (above).  String internal value is immutable.
	 */

	/* XXX: helper */
	duk_push_this(thr);
	h_this = duk_known_hobject(thr, -1);
	DUK_HOBJECT_SET_CLASS_NUMBER(h_this, DUK_HOBJECT_CLASS_NUMBER);

	DUK_ASSERT(DUK_HOBJECT_GET_PROTOTYPE(thr->heap, h_this) == thr->builtins[DUK_BIDX_NUMBER_PROTOTYPE]);
	DUK_ASSERT(DUK_HOBJECT_GET_CLASS_NUMBER(h_this) == DUK_HOBJECT_CLASS_NUMBER);
	DUK_ASSERT(DUK_HOBJECT_HAS_EXTENSIBLE(h_this));

	duk_dup_0(thr); /* -> [ val obj val ] */
	duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_INT_VALUE, DUK_PROPDESC_FLAGS_NONE);
	return 0; /* no return value -> don't replace created value */
}

DUK_INTERNAL duk_ret_t duk_bi_number_prototype_value_of(duk_hthread *thr) {
	(void) duk__push_this_number_plain(thr);
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_number_prototype_to_string(duk_hthread *thr) {
	duk_small_int_t radix;
	duk_small_uint_t n2s_flags;

	(void) duk__push_this_number_plain(thr);
	if (duk_is_undefined(thr, 0)) {
		radix = 10;
	} else {
		radix = (duk_small_int_t) duk_to_int_check_range(thr, 0, 2, 36);
	}
	DUK_DDD(DUK_DDDPRINT("radix=%ld", (long) radix));

	n2s_flags = 0;

	duk_numconv_stringify(thr, radix /*radix*/, 0 /*digits*/, n2s_flags /*flags*/);
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_number_prototype_to_locale_string(duk_hthread *thr) {
	/* XXX: just use toString() for now; permitted although not recommended.
	 * nargs==1, so radix is passed to toString().
	 */
	return duk_bi_number_prototype_to_string(thr);
}

/*
 *  toFixed(), toExponential(), toPrecision()
 */

/* XXX: shared helper for toFixed(), toExponential(), toPrecision()? */

DUK_INTERNAL duk_ret_t duk_bi_number_prototype_to_fixed(duk_hthread *thr) {
	duk_small_int_t frac_digits;
	duk_double_t d;
	duk_small_int_t c;
	duk_small_uint_t n2s_flags;

	/* In ES5.1 frac_digits is coerced first; in ES2015 the 'this number
	 * value' check is done first.
	 */
	d = duk__push_this_number_plain(thr);
	frac_digits = (duk_small_int_t) duk_to_int_check_range(thr, 0, 0, 20);

	c = (duk_small_int_t) DUK_FPCLASSIFY(d);
	if (c == DUK_FP_NAN || c == DUK_FP_INFINITE) {
		goto use_to_string;
	}

	if (d >= 1.0e21 || d <= -1.0e21) {
		goto use_to_string;
	}

	n2s_flags = DUK_N2S_FLAG_FIXED_FORMAT | DUK_N2S_FLAG_FRACTION_DIGITS;

	duk_numconv_stringify(thr, 10 /*radix*/, frac_digits /*digits*/, n2s_flags /*flags*/);
	return 1;

use_to_string:
	DUK_ASSERT_TOP(thr, 2);
	duk_to_string(thr, -1);
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_number_prototype_to_exponential(duk_hthread *thr) {
	duk_bool_t frac_undefined;
	duk_small_int_t frac_digits;
	duk_double_t d;
	duk_small_int_t c;
	duk_small_uint_t n2s_flags;

	d = duk__push_this_number_plain(thr);

	frac_undefined = duk_is_undefined(thr, 0);
	duk_to_int(thr, 0); /* for side effects */

	c = (duk_small_int_t) DUK_FPCLASSIFY(d);
	if (c == DUK_FP_NAN || c == DUK_FP_INFINITE) {
		goto use_to_string;
	}

	frac_digits = (duk_small_int_t) duk_to_int_check_range(thr, 0, 0, 20);

	n2s_flags = DUK_N2S_FLAG_FORCE_EXP | (frac_undefined ? 0 : DUK_N2S_FLAG_FIXED_FORMAT);

	duk_numconv_stringify(thr, 10 /*radix*/, frac_digits + 1 /*leading digit + fractions*/, n2s_flags /*flags*/);
	return 1;

use_to_string:
	DUK_ASSERT_TOP(thr, 2);
	duk_to_string(thr, -1);
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_number_prototype_to_precision(duk_hthread *thr) {
	/* The specification has quite awkward order of coercion and
	 * checks for toPrecision().  The operations below are a bit
	 * reordered, within constraints of observable side effects.
	 */

	duk_double_t d;
	duk_small_int_t prec;
	duk_small_int_t c;
	duk_small_uint_t n2s_flags;

	DUK_ASSERT_TOP(thr, 1);

	d = duk__push_this_number_plain(thr);
	if (duk_is_undefined(thr, 0)) {
		goto use_to_string;
	}
	DUK_ASSERT_TOP(thr, 2);

	duk_to_int(thr, 0); /* for side effects */

	c = (duk_small_int_t) DUK_FPCLASSIFY(d);
	if (c == DUK_FP_NAN || c == DUK_FP_INFINITE) {
		goto use_to_string;
	}

	prec = (duk_small_int_t) duk_to_int_check_range(thr, 0, 1, 21);

	n2s_flags = DUK_N2S_FLAG_FIXED_FORMAT | DUK_N2S_FLAG_NO_ZERO_PAD;

	duk_numconv_stringify(thr, 10 /*radix*/, prec /*digits*/, n2s_flags /*flags*/);
	return 1;

use_to_string:
	/* Used when precision is undefined; also used for NaN (-> "NaN"),
	 * and +/- infinity (-> "Infinity", "-Infinity").
	 */

	DUK_ASSERT_TOP(thr, 2);
	duk_to_string(thr, -1);
	return 1;
}

/*
 *  ES2015 isFinite() etc
 */

#if defined(DUK_USE_ES6)
DUK_INTERNAL duk_ret_t duk_bi_number_check_shared(duk_hthread *thr) {
	duk_int_t magic;
	duk_bool_t ret = 0;

	if (duk_is_number(thr, 0)) {
		duk_double_t d;

		magic = duk_get_current_magic(thr);
		d = duk_get_number(thr, 0);

		switch (magic) {
		case 0: /* isFinite() */
			ret = duk_double_is_finite(d);
			break;
		case 1: /* isInteger() */
			ret = duk_double_is_integer(d);
			break;
		case 2: /* isNaN() */
			ret = duk_double_is_nan(d);
			break;
		default: /* isSafeInteger() */
			DUK_ASSERT(magic == 3);
			ret = duk_double_is_safe_integer(d);
		}
	}

	duk_push_boolean(thr, ret);
	return 1;
}
#endif /* DUK_USE_ES6 */

#endif /* DUK_USE_NUMBER_BUILTIN */
