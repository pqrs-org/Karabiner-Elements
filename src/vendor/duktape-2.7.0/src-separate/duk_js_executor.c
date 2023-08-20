/*
 *  ECMAScript bytecode executor.
 */

#include "duk_internal.h"

/*
 *  Local declarations.
 */

DUK_LOCAL_DECL void duk__js_execute_bytecode_inner(duk_hthread *entry_thread, duk_activation *entry_act);

/*
 *  Misc helpers.
 */

/* Replace value stack top to value at 'tv_ptr'.  Optimize for
 * performance by only applying the net refcount change.
 */
#define DUK__REPLACE_TO_TVPTR(thr, tv_ptr) \
	do { \
		duk_hthread *duk__thr; \
		duk_tval *duk__tvsrc; \
		duk_tval *duk__tvdst; \
		duk_tval duk__tvtmp; \
		duk__thr = (thr); \
		duk__tvsrc = DUK_GET_TVAL_NEGIDX(duk__thr, -1); \
		duk__tvdst = (tv_ptr); \
		DUK_TVAL_SET_TVAL(&duk__tvtmp, duk__tvdst); \
		DUK_TVAL_SET_TVAL(duk__tvdst, duk__tvsrc); \
		DUK_TVAL_SET_UNDEFINED(duk__tvsrc); /* value stack init policy */ \
		duk__thr->valstack_top = duk__tvsrc; \
		DUK_TVAL_DECREF(duk__thr, &duk__tvtmp); \
	} while (0)

/* XXX: candidate of being an internal shared API call */
#if 0 /* unused */
DUK_LOCAL void duk__push_tvals_incref_only(duk_hthread *thr, duk_tval *tv_src, duk_small_uint_fast_t count) {
	duk_tval *tv_dst;
	duk_size_t copy_size;
	duk_size_t i;

	tv_dst = thr->valstack_top;
	copy_size = sizeof(duk_tval) * count;
	duk_memcpy((void *) tv_dst, (const void *) tv_src, copy_size);
	for (i = 0; i < count; i++) {
		DUK_TVAL_INCREF(thr, tv_dst);
		tv_dst++;
	}
	thr->valstack_top = tv_dst;
}
#endif

/*
 *  Arithmetic, binary, and logical helpers.
 *
 *  Note: there is no opcode for logical AND or logical OR; this is on
 *  purpose, because the evalution order semantics for them make such
 *  opcodes pretty pointless: short circuiting means they are most
 *  comfortably implemented as jumps.  However, a logical NOT opcode
 *  is useful.
 *
 *  Note: careful with duk_tval pointers here: they are potentially
 *  invalidated by any DECREF and almost any API call.  It's still
 *  preferable to work without making a copy but that's not always
 *  possible.
 */

DUK_LOCAL DUK_EXEC_ALWAYS_INLINE_PERF duk_double_t duk__compute_mod(duk_double_t d1, duk_double_t d2) {
	return (duk_double_t) duk_js_arith_mod((double) d1, (double) d2);
}

#if defined(DUK_USE_ES7_EXP_OPERATOR)
DUK_LOCAL DUK_EXEC_ALWAYS_INLINE_PERF duk_double_t duk__compute_exp(duk_double_t d1, duk_double_t d2) {
	return (duk_double_t) duk_js_arith_pow((double) d1, (double) d2);
}
#endif

DUK_LOCAL DUK_EXEC_ALWAYS_INLINE_PERF void duk__vm_arith_add(duk_hthread *thr,
                                                             duk_tval *tv_x,
                                                             duk_tval *tv_y,
                                                             duk_small_uint_fast_t idx_z) {
	/*
	 *  Addition operator is different from other arithmetic
	 *  operations in that it also provides string concatenation.
	 *  Hence it is implemented separately.
	 *
	 *  There is a fast path for number addition.  Other cases go
	 *  through potentially multiple coercions as described in the
	 *  E5 specification.  It may be possible to reduce the number
	 *  of coercions, but this must be done carefully to preserve
	 *  the exact semantics.
	 *
	 *  E5 Section 11.6.1.
	 *
	 *  Custom types also have special behavior implemented here.
	 */

	duk_double_union du;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(tv_x != NULL); /* may be reg or const */
	DUK_ASSERT(tv_y != NULL); /* may be reg or const */
	DUK_ASSERT_DISABLE(idx_z >= 0); /* unsigned */
	DUK_ASSERT((duk_uint_t) idx_z < (duk_uint_t) duk_get_top(thr));

	/*
	 *  Fast paths
	 */

#if defined(DUK_USE_FASTINT)
	if (DUK_TVAL_IS_FASTINT(tv_x) && DUK_TVAL_IS_FASTINT(tv_y)) {
		duk_int64_t v1, v2, v3;
		duk_int32_t v3_hi;
		duk_tval *tv_z;

		/* Input values are signed 48-bit so we can detect overflow
		 * reliably from high bits or just a comparison.
		 */

		v1 = DUK_TVAL_GET_FASTINT(tv_x);
		v2 = DUK_TVAL_GET_FASTINT(tv_y);
		v3 = v1 + v2;
		v3_hi = (duk_int32_t) (v3 >> 32);
		if (DUK_LIKELY(v3_hi >= DUK_I64_CONSTANT(-0x8000) && v3_hi <= DUK_I64_CONSTANT(0x7fff))) {
			tv_z = thr->valstack_bottom + idx_z;
			DUK_TVAL_SET_FASTINT_UPDREF(thr, tv_z, v3); /* side effects */
			return;
		} else {
			/* overflow, fall through */
			;
		}
	}
#endif /* DUK_USE_FASTINT */

	if (DUK_TVAL_IS_NUMBER(tv_x) && DUK_TVAL_IS_NUMBER(tv_y)) {
#if !defined(DUK_USE_EXEC_PREFER_SIZE)
		duk_tval *tv_z;
#endif

		du.d = DUK_TVAL_GET_NUMBER(tv_x) + DUK_TVAL_GET_NUMBER(tv_y);
#if defined(DUK_USE_EXEC_PREFER_SIZE)
		duk_push_number(thr, du.d); /* will NaN normalize result */
		duk_replace(thr, (duk_idx_t) idx_z);
#else /* DUK_USE_EXEC_PREFER_SIZE */
		DUK_DBLUNION_NORMALIZE_NAN_CHECK(&du);
		DUK_ASSERT(DUK_DBLUNION_IS_NORMALIZED(&du));
		tv_z = thr->valstack_bottom + idx_z;
		DUK_TVAL_SET_NUMBER_UPDREF(thr, tv_z, du.d); /* side effects */
#endif /* DUK_USE_EXEC_PREFER_SIZE */
		return;
	}

	/*
	 *  Slow path: potentially requires function calls for coercion
	 */

	duk_push_tval(thr, tv_x);
	duk_push_tval(thr, tv_y);
	duk_to_primitive(thr, -2, DUK_HINT_NONE); /* side effects -> don't use tv_x, tv_y after */
	duk_to_primitive(thr, -1, DUK_HINT_NONE);

	/* Since Duktape 2.x plain buffers are treated like ArrayBuffer. */
	if (duk_is_string(thr, -2) || duk_is_string(thr, -1)) {
		/* Symbols shouldn't technically be handled here, but should
		 * go into the default ToNumber() coercion path instead and
		 * fail there with a TypeError.  However, there's a ToString()
		 * in duk_concat_2() which also fails with TypeError so no
		 * explicit check is needed.
		 */
		duk_concat_2(thr); /* [... s1 s2] -> [... s1+s2] */
	} else {
		duk_double_t d1, d2;

		d1 = duk_to_number_m2(thr);
		d2 = duk_to_number_m1(thr);
		DUK_ASSERT(duk_is_number(thr, -2));
		DUK_ASSERT(duk_is_number(thr, -1));
		DUK_ASSERT_DOUBLE_IS_NORMALIZED(d1);
		DUK_ASSERT_DOUBLE_IS_NORMALIZED(d2);

		du.d = d1 + d2;
		duk_pop_2_unsafe(thr);
		duk_push_number(thr, du.d); /* will NaN normalize result */
	}
	duk_replace(thr, (duk_idx_t) idx_z); /* side effects */
}

DUK_LOCAL DUK_EXEC_ALWAYS_INLINE_PERF void duk__vm_arith_binary_op(duk_hthread *thr,
                                                                   duk_tval *tv_x,
                                                                   duk_tval *tv_y,
                                                                   duk_uint_fast_t idx_z,
                                                                   duk_small_uint_fast_t opcode) {
	/*
	 *  Arithmetic operations other than '+' have number-only semantics
	 *  and are implemented here.  The separate switch-case here means a
	 *  "double dispatch" of the arithmetic opcode, but saves code space.
	 *
	 *  E5 Sections 11.5, 11.5.1, 11.5.2, 11.5.3, 11.6, 11.6.1, 11.6.2, 11.6.3.
	 */

	duk_double_t d1, d2;
	duk_double_union du;
	duk_small_uint_fast_t opcode_shifted;
#if defined(DUK_USE_FASTINT) || !defined(DUK_USE_EXEC_PREFER_SIZE)
	duk_tval *tv_z;
#endif

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(tv_x != NULL); /* may be reg or const */
	DUK_ASSERT(tv_y != NULL); /* may be reg or const */
	DUK_ASSERT_DISABLE(idx_z >= 0); /* unsigned */
	DUK_ASSERT((duk_uint_t) idx_z < (duk_uint_t) duk_get_top(thr));

	opcode_shifted = opcode >> 2; /* Get base opcode without reg/const modifiers. */

#if defined(DUK_USE_FASTINT)
	if (DUK_TVAL_IS_FASTINT(tv_x) && DUK_TVAL_IS_FASTINT(tv_y)) {
		duk_int64_t v1, v2, v3;
		duk_int32_t v3_hi;

		v1 = DUK_TVAL_GET_FASTINT(tv_x);
		v2 = DUK_TVAL_GET_FASTINT(tv_y);

		switch (opcode_shifted) {
		case DUK_OP_SUB >> 2: {
			v3 = v1 - v2;
			break;
		}
		case DUK_OP_MUL >> 2: {
			/* Must ensure result is 64-bit (no overflow); a
			 * simple and sufficient fast path is to allow only
			 * 32-bit inputs.  Avoid zero inputs to avoid
			 * negative zero issues (-1 * 0 = -0, for instance).
			 */
			if (v1 >= DUK_I64_CONSTANT(-0x80000000) && v1 <= DUK_I64_CONSTANT(0x7fffffff) && v1 != 0 &&
			    v2 >= DUK_I64_CONSTANT(-0x80000000) && v2 <= DUK_I64_CONSTANT(0x7fffffff) && v2 != 0) {
				v3 = v1 * v2;
			} else {
				goto skip_fastint;
			}
			break;
		}
		case DUK_OP_DIV >> 2: {
			/* Don't allow a zero divisor.  Fast path check by
			 * "verifying" with multiplication.  Also avoid zero
			 * dividend to avoid negative zero issues (0 / -1 = -0
			 * for instance).
			 */
			if (v1 == 0 || v2 == 0) {
				goto skip_fastint;
			}
			v3 = v1 / v2;
			if (v3 * v2 != v1) {
				goto skip_fastint;
			}
			break;
		}
		case DUK_OP_MOD >> 2: {
			/* Don't allow a zero divisor.  Restrict both v1 and
			 * v2 to positive values to avoid compiler specific
			 * behavior.
			 */
			if (v1 < 1 || v2 < 1) {
				goto skip_fastint;
			}
			v3 = v1 % v2;
			DUK_ASSERT(v3 >= 0);
			DUK_ASSERT(v3 < v2);
			DUK_ASSERT(v1 - (v1 / v2) * v2 == v3);
			break;
		}
		default: {
			/* Possible with DUK_OP_EXP. */
			goto skip_fastint;
		}
		}

		v3_hi = (duk_int32_t) (v3 >> 32);
		if (DUK_LIKELY(v3_hi >= DUK_I64_CONSTANT(-0x8000) && v3_hi <= DUK_I64_CONSTANT(0x7fff))) {
			tv_z = thr->valstack_bottom + idx_z;
			DUK_TVAL_SET_FASTINT_UPDREF(thr, tv_z, v3); /* side effects */
			return;
		}
		/* fall through if overflow etc */
	}
skip_fastint:
#endif /* DUK_USE_FASTINT */

	if (DUK_TVAL_IS_NUMBER(tv_x) && DUK_TVAL_IS_NUMBER(tv_y)) {
		/* fast path */
		d1 = DUK_TVAL_GET_NUMBER(tv_x);
		d2 = DUK_TVAL_GET_NUMBER(tv_y);
	} else {
		duk_push_tval(thr, tv_x);
		duk_push_tval(thr, tv_y);
		d1 = duk_to_number_m2(thr); /* side effects */
		d2 = duk_to_number_m1(thr);
		DUK_ASSERT(duk_is_number(thr, -2));
		DUK_ASSERT(duk_is_number(thr, -1));
		DUK_ASSERT_DOUBLE_IS_NORMALIZED(d1);
		DUK_ASSERT_DOUBLE_IS_NORMALIZED(d2);
		duk_pop_2_unsafe(thr);
	}

	switch (opcode_shifted) {
	case DUK_OP_SUB >> 2: {
		du.d = d1 - d2;
		break;
	}
	case DUK_OP_MUL >> 2: {
		du.d = d1 * d2;
		break;
	}
	case DUK_OP_DIV >> 2: {
		/* Division-by-zero is undefined behavior, so
		 * rely on a helper.
		 */
		du.d = duk_double_div(d1, d2);
		break;
	}
	case DUK_OP_MOD >> 2: {
		du.d = duk__compute_mod(d1, d2);
		break;
	}
#if defined(DUK_USE_ES7_EXP_OPERATOR)
	case DUK_OP_EXP >> 2: {
		du.d = duk__compute_exp(d1, d2);
		break;
	}
#endif
	default: {
		DUK_UNREACHABLE();
		du.d = DUK_DOUBLE_NAN; /* should not happen */
		break;
	}
	}

#if defined(DUK_USE_EXEC_PREFER_SIZE)
	duk_push_number(thr, du.d); /* will NaN normalize result */
	duk_replace(thr, (duk_idx_t) idx_z);
#else /* DUK_USE_EXEC_PREFER_SIZE */
	/* important to use normalized NaN with 8-byte tagged types */
	DUK_DBLUNION_NORMALIZE_NAN_CHECK(&du);
	DUK_ASSERT(DUK_DBLUNION_IS_NORMALIZED(&du));
	tv_z = thr->valstack_bottom + idx_z;
	DUK_TVAL_SET_NUMBER_UPDREF(thr, tv_z, du.d); /* side effects */
#endif /* DUK_USE_EXEC_PREFER_SIZE */
}

DUK_LOCAL DUK_EXEC_ALWAYS_INLINE_PERF void duk__vm_bitwise_binary_op(duk_hthread *thr,
                                                                     duk_tval *tv_x,
                                                                     duk_tval *tv_y,
                                                                     duk_small_uint_fast_t idx_z,
                                                                     duk_small_uint_fast_t opcode) {
	/*
	 *  Binary bitwise operations use different coercions (ToInt32, ToUint32)
	 *  depending on the operation.  We coerce the arguments first using
	 *  ToInt32(), and then cast to an 32-bit value if necessary.  Note that
	 *  such casts must be correct even if there is no native 32-bit type
	 *  (e.g., duk_int32_t and duk_uint32_t are 64-bit).
	 *
	 *  E5 Sections 11.10, 11.7.1, 11.7.2, 11.7.3
	 */

	duk_int32_t i1, i2, i3;
	duk_uint32_t u1, u2, u3;
#if defined(DUK_USE_FASTINT)
	duk_int64_t fi3;
#else
	duk_double_t d3;
#endif
	duk_small_uint_fast_t opcode_shifted;
#if defined(DUK_USE_FASTINT) || !defined(DUK_USE_EXEC_PREFER_SIZE)
	duk_tval *tv_z;
#endif

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(tv_x != NULL); /* may be reg or const */
	DUK_ASSERT(tv_y != NULL); /* may be reg or const */
	DUK_ASSERT_DISABLE(idx_z >= 0); /* unsigned */
	DUK_ASSERT((duk_uint_t) idx_z < (duk_uint_t) duk_get_top(thr));

	opcode_shifted = opcode >> 2; /* Get base opcode without reg/const modifiers. */

#if defined(DUK_USE_FASTINT)
	if (DUK_TVAL_IS_FASTINT(tv_x) && DUK_TVAL_IS_FASTINT(tv_y)) {
		i1 = (duk_int32_t) DUK_TVAL_GET_FASTINT_I32(tv_x);
		i2 = (duk_int32_t) DUK_TVAL_GET_FASTINT_I32(tv_y);
	} else
#endif /* DUK_USE_FASTINT */
	{
		duk_push_tval(thr, tv_x);
		duk_push_tval(thr, tv_y);
		i1 = duk_to_int32(thr, -2);
		i2 = duk_to_int32(thr, -1);
		duk_pop_2_unsafe(thr);
	}

	switch (opcode_shifted) {
	case DUK_OP_BAND >> 2: {
		i3 = i1 & i2;
		break;
	}
	case DUK_OP_BOR >> 2: {
		i3 = i1 | i2;
		break;
	}
	case DUK_OP_BXOR >> 2: {
		i3 = i1 ^ i2;
		break;
	}
	case DUK_OP_BASL >> 2: {
		/* Signed shift, named "arithmetic" (asl) because the result
		 * is signed, e.g. 4294967295 << 1 -> -2.  Note that result
		 * must be masked.
		 */

		u2 = ((duk_uint32_t) i2) & 0xffffffffUL;
		i3 = (duk_int32_t) (((duk_uint32_t) i1) << (u2 & 0x1fUL)); /* E5 Section 11.7.1, steps 7 and 8 */
		i3 = i3 & ((duk_int32_t) 0xffffffffUL); /* Note: left shift, should mask */
		break;
	}
	case DUK_OP_BASR >> 2: {
		/* signed shift */

		u2 = ((duk_uint32_t) i2) & 0xffffffffUL;
		i3 = i1 >> (u2 & 0x1fUL); /* E5 Section 11.7.2, steps 7 and 8 */
		break;
	}
	case DUK_OP_BLSR >> 2: {
		/* unsigned shift */

		u1 = ((duk_uint32_t) i1) & 0xffffffffUL;
		u2 = ((duk_uint32_t) i2) & 0xffffffffUL;

		/* special result value handling */
		u3 = u1 >> (u2 & 0x1fUL); /* E5 Section 11.7.2, steps 7 and 8 */
#if defined(DUK_USE_FASTINT)
		fi3 = (duk_int64_t) u3;
		goto fastint_result_set;
#else
		d3 = (duk_double_t) u3;
		goto result_set;
#endif
	}
	default: {
		DUK_UNREACHABLE();
		i3 = 0; /* should not happen */
		break;
	}
	}

#if defined(DUK_USE_FASTINT)
	/* Result is always fastint compatible. */
	/* XXX: Set 32-bit result (but must then handle signed and
	 * unsigned results separately).
	 */
	fi3 = (duk_int64_t) i3;

fastint_result_set:
	tv_z = thr->valstack_bottom + idx_z;
	DUK_TVAL_SET_FASTINT_UPDREF(thr, tv_z, fi3); /* side effects */
#else /* DUK_USE_FASTINT */
	d3 = (duk_double_t) i3;

result_set:
	DUK_ASSERT(!DUK_ISNAN(d3)); /* 'd3' is never NaN, so no need to normalize */
	DUK_ASSERT_DOUBLE_IS_NORMALIZED(d3); /* always normalized */

#if defined(DUK_USE_EXEC_PREFER_SIZE)
	duk_push_number(thr, d3); /* would NaN normalize result, but unnecessary */
	duk_replace(thr, (duk_idx_t) idx_z);
#else /* DUK_USE_EXEC_PREFER_SIZE */
	tv_z = thr->valstack_bottom + idx_z;
	DUK_TVAL_SET_NUMBER_UPDREF(thr, tv_z, d3); /* side effects */
#endif /* DUK_USE_EXEC_PREFER_SIZE */
#endif /* DUK_USE_FASTINT */
}

/* In-place unary operation. */
DUK_LOCAL DUK_EXEC_ALWAYS_INLINE_PERF void duk__vm_arith_unary_op(duk_hthread *thr,
                                                                  duk_uint_fast_t idx_src,
                                                                  duk_uint_fast_t idx_dst,
                                                                  duk_small_uint_fast_t opcode) {
	/*
	 *  Arithmetic operations other than '+' have number-only semantics
	 *  and are implemented here.  The separate switch-case here means a
	 *  "double dispatch" of the arithmetic opcode, but saves code space.
	 *
	 *  E5 Sections 11.5, 11.5.1, 11.5.2, 11.5.3, 11.6, 11.6.1, 11.6.2, 11.6.3.
	 */

	duk_tval *tv;
	duk_double_t d1;
	duk_double_union du;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(opcode == DUK_OP_UNM || opcode == DUK_OP_UNP);
	DUK_ASSERT_DISABLE(idx_src >= 0);
	DUK_ASSERT_DISABLE(idx_dst >= 0);

	tv = DUK_GET_TVAL_POSIDX(thr, (duk_idx_t) idx_src);

#if defined(DUK_USE_FASTINT)
	if (DUK_TVAL_IS_FASTINT(tv)) {
		duk_int64_t v1, v2;

		v1 = DUK_TVAL_GET_FASTINT(tv);
		if (opcode == DUK_OP_UNM) {
			/* The smallest fastint is no longer 48-bit when
			 * negated.  Positive zero becames negative zero
			 * (cannot be represented) when negated.
			 */
			if (DUK_LIKELY(v1 != DUK_FASTINT_MIN && v1 != 0)) {
				v2 = -v1;
				tv = DUK_GET_TVAL_POSIDX(thr, (duk_idx_t) idx_dst);
				DUK_TVAL_SET_FASTINT_UPDREF(thr, tv, v2);
				return;
			}
		} else {
			/* ToNumber() for a fastint is a no-op. */
			DUK_ASSERT(opcode == DUK_OP_UNP);
			v2 = v1;
			tv = DUK_GET_TVAL_POSIDX(thr, (duk_idx_t) idx_dst);
			DUK_TVAL_SET_FASTINT_UPDREF(thr, tv, v2);
			return;
		}
		/* fall through if overflow etc */
	}
#endif /* DUK_USE_FASTINT */

	if (DUK_TVAL_IS_NUMBER(tv)) {
		d1 = DUK_TVAL_GET_NUMBER(tv);
	} else {
		d1 = duk_to_number_tval(thr, tv); /* side effects */
	}

	if (opcode == DUK_OP_UNP) {
		/* ToNumber() for a double is a no-op, but unary plus is
		 * used to force a fastint check so do that here.
		 */
		du.d = d1;
		DUK_ASSERT(DUK_DBLUNION_IS_NORMALIZED(&du));
#if defined(DUK_USE_FASTINT)
		tv = DUK_GET_TVAL_POSIDX(thr, (duk_idx_t) idx_dst);
		DUK_TVAL_SET_NUMBER_CHKFAST_UPDREF(thr, tv, du.d); /* always 'fast', i.e. inlined */
		return;
#endif
	} else {
		DUK_ASSERT(opcode == DUK_OP_UNM);
		du.d = -d1;
		DUK_DBLUNION_NORMALIZE_NAN_CHECK(&du); /* mandatory if du.d is a NaN */
		DUK_ASSERT(DUK_DBLUNION_IS_NORMALIZED(&du));
	}

	/* XXX: size optimize: push+replace? */
	tv = DUK_GET_TVAL_POSIDX(thr, (duk_idx_t) idx_dst);
	DUK_TVAL_SET_NUMBER_UPDREF(thr, tv, du.d);
}

DUK_LOCAL DUK_EXEC_ALWAYS_INLINE_PERF void duk__vm_bitwise_not(duk_hthread *thr, duk_uint_fast_t idx_src, duk_uint_fast_t idx_dst) {
	/*
	 *  E5 Section 11.4.8
	 */

	duk_tval *tv;
	duk_int32_t i1, i2;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT_DISABLE(idx_src >= 0);
	DUK_ASSERT_DISABLE(idx_dst >= 0);
	DUK_ASSERT((duk_uint_t) idx_src < (duk_uint_t) duk_get_top(thr));
	DUK_ASSERT((duk_uint_t) idx_dst < (duk_uint_t) duk_get_top(thr));

	tv = DUK_GET_TVAL_POSIDX(thr, (duk_idx_t) idx_src);

#if defined(DUK_USE_FASTINT)
	if (DUK_TVAL_IS_FASTINT(tv)) {
		i1 = (duk_int32_t) DUK_TVAL_GET_FASTINT_I32(tv);
	} else
#endif /* DUK_USE_FASTINT */
	{
		duk_push_tval(thr, tv);
		i1 = duk_to_int32(thr, -1); /* side effects */
		duk_pop_unsafe(thr);
	}

	/* Result is always fastint compatible. */
	i2 = ~i1;
	tv = DUK_GET_TVAL_POSIDX(thr, (duk_idx_t) idx_dst);
	DUK_TVAL_SET_I32_UPDREF(thr, tv, i2); /* side effects */
}

DUK_LOCAL DUK_EXEC_ALWAYS_INLINE_PERF void duk__vm_logical_not(duk_hthread *thr, duk_uint_fast_t idx_src, duk_uint_fast_t idx_dst) {
	/*
	 *  E5 Section 11.4.9
	 */

	duk_tval *tv;
	duk_bool_t res;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT_DISABLE(idx_src >= 0);
	DUK_ASSERT_DISABLE(idx_dst >= 0);
	DUK_ASSERT((duk_uint_t) idx_src < (duk_uint_t) duk_get_top(thr));
	DUK_ASSERT((duk_uint_t) idx_dst < (duk_uint_t) duk_get_top(thr));

	/* ToBoolean() does not require any operations with side effects so
	 * we can do it efficiently.  For footprint it would be better to use
	 * duk_js_toboolean() and then push+replace to the result slot.
	 */
	tv = DUK_GET_TVAL_POSIDX(thr, (duk_idx_t) idx_src);
	res = duk_js_toboolean(tv); /* does not modify 'tv' */
	DUK_ASSERT(res == 0 || res == 1);
	res ^= 1;
	tv = DUK_GET_TVAL_POSIDX(thr, (duk_idx_t) idx_dst);
	/* XXX: size optimize: push+replace? */
	DUK_TVAL_SET_BOOLEAN_UPDREF(thr, tv, res); /* side effects */
}

/* XXX: size optimized variant */
DUK_LOCAL DUK_EXEC_ALWAYS_INLINE_PERF void duk__prepost_incdec_reg_helper(duk_hthread *thr,
                                                                          duk_tval *tv_dst,
                                                                          duk_tval *tv_src,
                                                                          duk_small_uint_t op) {
	duk_double_t x, y, z;

	/* Two lowest bits of opcode are used to distinguish
	 * variants.  Bit 0 = inc(0)/dec(1), bit 1 = pre(0)/post(1).
	 */
	DUK_ASSERT((DUK_OP_PREINCR & 0x03) == 0x00);
	DUK_ASSERT((DUK_OP_PREDECR & 0x03) == 0x01);
	DUK_ASSERT((DUK_OP_POSTINCR & 0x03) == 0x02);
	DUK_ASSERT((DUK_OP_POSTDECR & 0x03) == 0x03);

#if defined(DUK_USE_FASTINT)
	if (DUK_TVAL_IS_FASTINT(tv_src)) {
		duk_int64_t x_fi, y_fi, z_fi;
		x_fi = DUK_TVAL_GET_FASTINT(tv_src);
		if (op & 0x01) {
			if (DUK_UNLIKELY(x_fi == DUK_FASTINT_MIN)) {
				goto skip_fastint;
			}
			y_fi = x_fi - 1;
		} else {
			if (DUK_UNLIKELY(x_fi == DUK_FASTINT_MAX)) {
				goto skip_fastint;
			}
			y_fi = x_fi + 1;
		}

		DUK_TVAL_SET_FASTINT(tv_src, y_fi); /* no need for refcount update */

		z_fi = (op & 0x02) ? x_fi : y_fi;
		DUK_TVAL_SET_FASTINT_UPDREF(thr, tv_dst, z_fi); /* side effects */
		return;
	}
skip_fastint:
#endif
	if (DUK_TVAL_IS_NUMBER(tv_src)) {
		/* Fast path for the case where the register
		 * is a number (e.g. loop counter).
		 */

		x = DUK_TVAL_GET_NUMBER(tv_src);
		if (op & 0x01) {
			y = x - 1.0;
		} else {
			y = x + 1.0;
		}

		DUK_TVAL_SET_NUMBER(tv_src, y); /* no need for refcount update */
	} else {
		/* Preserve duk_tval pointer(s) across a potential valstack
		 * resize by converting them into offsets temporarily.
		 */
		duk_idx_t bc;
		duk_size_t off_dst;

		off_dst = (duk_size_t) ((duk_uint8_t *) tv_dst - (duk_uint8_t *) thr->valstack_bottom);
		bc = (duk_idx_t) (tv_src - thr->valstack_bottom); /* XXX: pass index explicitly? */
		tv_src = NULL; /* no longer referenced */

		x = duk_to_number(thr, bc);
		if (op & 0x01) {
			y = x - 1.0;
		} else {
			y = x + 1.0;
		}

		duk_push_number(thr, y);
		duk_replace(thr, bc);

		tv_dst = (duk_tval *) (void *) (((duk_uint8_t *) thr->valstack_bottom) + off_dst);
	}

	z = (op & 0x02) ? x : y;
	DUK_TVAL_SET_NUMBER_UPDREF(thr, tv_dst, z); /* side effects */
}

DUK_LOCAL DUK_EXEC_ALWAYS_INLINE_PERF void duk__prepost_incdec_var_helper(duk_hthread *thr,
                                                                          duk_small_uint_t idx_dst,
                                                                          duk_tval *tv_id,
                                                                          duk_small_uint_t op,
                                                                          duk_small_uint_t is_strict) {
	duk_activation *act;
	duk_double_t x, y;
	duk_hstring *name;

	/* XXX: The pre/post inc/dec for an identifier lookup is
	 * missing the important fast path where the identifier
	 * has a storage location e.g. in a scope object so that
	 * it can be updated in-place.  In particular, the case
	 * where the identifier has a storage location AND the
	 * previous value is a number should be optimized because
	 * it's side effect free.
	 */

	/* Two lowest bits of opcode are used to distinguish
	 * variants.  Bit 0 = inc(0)/dec(1), bit 1 = pre(0)/post(1).
	 */
	DUK_ASSERT((DUK_OP_PREINCV & 0x03) == 0x00);
	DUK_ASSERT((DUK_OP_PREDECV & 0x03) == 0x01);
	DUK_ASSERT((DUK_OP_POSTINCV & 0x03) == 0x02);
	DUK_ASSERT((DUK_OP_POSTDECV & 0x03) == 0x03);

	DUK_ASSERT(DUK_TVAL_IS_STRING(tv_id));
	name = DUK_TVAL_GET_STRING(tv_id);
	DUK_ASSERT(name != NULL);
	act = thr->callstack_curr;
	(void) duk_js_getvar_activation(thr, act, name, 1 /*throw*/); /* -> [ ... val this ] */

	/* XXX: Fastint fast path would be useful here.  Also fastints
	 * now lose their fastint status in current handling which is
	 * not intuitive.
	 */

	x = duk_to_number_m2(thr);
	if (op & 0x01) {
		y = x - 1.0;
	} else {
		y = x + 1.0;
	}

	/* [... x this] */

	if (op & 0x02) {
		duk_push_number(thr, y); /* -> [ ... x this y ] */
		DUK_ASSERT(act == thr->callstack_curr);
		duk_js_putvar_activation(thr, act, name, DUK_GET_TVAL_NEGIDX(thr, -1), is_strict);
		duk_pop_2_unsafe(thr); /* -> [ ... x ] */
	} else {
		duk_pop_2_unsafe(thr); /* -> [ ... ] */
		duk_push_number(thr, y); /* -> [ ... y ] */
		DUK_ASSERT(act == thr->callstack_curr);
		duk_js_putvar_activation(thr, act, name, DUK_GET_TVAL_NEGIDX(thr, -1), is_strict);
	}

#if defined(DUK_USE_EXEC_PREFER_SIZE)
	duk_replace(thr, (duk_idx_t) idx_dst);
#else /* DUK_USE_EXEC_PREFER_SIZE */
	DUK__REPLACE_TO_TVPTR(thr, DUK_GET_TVAL_POSIDX(thr, (duk_idx_t) idx_dst));
#endif /* DUK_USE_EXEC_PREFER_SIZE */
}

/*
 *  Longjmp and other control flow transfer for the bytecode executor.
 *
 *  The longjmp handler can handle all longjmp types: error, yield, and
 *  resume (pseudotypes are never actually thrown).
 *
 *  Error policy for longjmp: should not ordinarily throw errors; if errors
 *  occur (e.g. due to out-of-memory) they bubble outwards rather than being
 *  handled recursively.
 */

#define DUK__LONGJMP_RESTART 0 /* state updated, restart bytecode execution */
#define DUK__LONGJMP_RETHROW 1 /* exit bytecode executor by rethrowing an error to caller */

#define DUK__RETHAND_RESTART  0 /* state updated, restart bytecode execution */
#define DUK__RETHAND_FINISHED 1 /* exit bytecode execution with return value */

/* XXX: optimize reconfig valstack operations so that resize, clamp, and setting
 * top are combined into one pass.
 */

/* Reconfigure value stack for return to an ECMAScript function at
 * callstack top (caller unwinds).
 */
DUK_LOCAL void duk__reconfig_valstack_ecma_return(duk_hthread *thr) {
	duk_activation *act;
	duk_hcompfunc *h_func;
	duk_idx_t clamp_top;

	DUK_ASSERT(thr != NULL);
	act = thr->callstack_curr;
	DUK_ASSERT(act != NULL);
	DUK_ASSERT(DUK_ACT_GET_FUNC(act) != NULL);
	DUK_ASSERT(DUK_HOBJECT_IS_COMPFUNC(DUK_ACT_GET_FUNC(act)));

	/* Clamp so that values at 'clamp_top' and above are wiped and won't
	 * retain reachable garbage.  Then extend to 'nregs' because we're
	 * returning to an ECMAScript function.
	 */

	h_func = (duk_hcompfunc *) DUK_ACT_GET_FUNC(act);

	thr->valstack_bottom = (duk_tval *) (void *) ((duk_uint8_t *) thr->valstack + act->bottom_byteoff);
	DUK_ASSERT(act->retval_byteoff >= act->bottom_byteoff);
	clamp_top =
	    (duk_idx_t) ((act->retval_byteoff - act->bottom_byteoff + sizeof(duk_tval)) / sizeof(duk_tval)); /* +1 = one retval */
	duk_set_top_and_wipe(thr, h_func->nregs, clamp_top);

	DUK_ASSERT((duk_uint8_t *) thr->valstack_end >= (duk_uint8_t *) thr->valstack + act->reserve_byteoff);
	thr->valstack_end = (duk_tval *) (void *) ((duk_uint8_t *) thr->valstack + act->reserve_byteoff);

	/* XXX: a best effort shrink check would be OK here */
}

/* Reconfigure value stack for an ECMAScript catcher.  Use topmost catcher
 * in 'act'.
 */
DUK_LOCAL void duk__reconfig_valstack_ecma_catcher(duk_hthread *thr, duk_activation *act) {
	duk_catcher *cat;
	duk_hcompfunc *h_func;
	duk_size_t idx_bottom;
	duk_idx_t clamp_top;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(act != NULL);
	DUK_ASSERT(DUK_ACT_GET_FUNC(act) != NULL);
	DUK_ASSERT(DUK_HOBJECT_IS_COMPFUNC(DUK_ACT_GET_FUNC(act)));
	cat = act->cat;
	DUK_ASSERT(cat != NULL);

	h_func = (duk_hcompfunc *) DUK_ACT_GET_FUNC(act);

	thr->valstack_bottom = (duk_tval *) (void *) ((duk_uint8_t *) thr->valstack + act->bottom_byteoff);
	idx_bottom = (duk_size_t) (thr->valstack_bottom - thr->valstack);
	DUK_ASSERT(cat->idx_base >= idx_bottom);
	clamp_top = (duk_idx_t) (cat->idx_base - idx_bottom + 2); /* +2 = catcher value, catcher lj_type */
	duk_set_top_and_wipe(thr, h_func->nregs, clamp_top);

	DUK_ASSERT((duk_uint8_t *) thr->valstack_end >= (duk_uint8_t *) thr->valstack + act->reserve_byteoff);
	thr->valstack_end = (duk_tval *) (void *) ((duk_uint8_t *) thr->valstack + act->reserve_byteoff);

	/* XXX: a best effort shrink check would be OK here */
}

/* Set catcher regs: idx_base+0 = value, idx_base+1 = lj_type.
 * No side effects.
 */
DUK_LOCAL void duk__set_catcher_regs_norz(duk_hthread *thr, duk_catcher *cat, duk_tval *tv_val_unstable, duk_small_uint_t lj_type) {
	duk_tval *tv1;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(tv_val_unstable != NULL);

	tv1 = thr->valstack + cat->idx_base;
	DUK_ASSERT(tv1 < thr->valstack_top);
	DUK_TVAL_SET_TVAL_UPDREF_NORZ(thr, tv1, tv_val_unstable);

	tv1++;
	DUK_ASSERT(tv1 == thr->valstack + cat->idx_base + 1);
	DUK_ASSERT(tv1 < thr->valstack_top);
	DUK_TVAL_SET_U32_UPDREF_NORZ(thr, tv1, (duk_uint32_t) lj_type);
}

DUK_LOCAL void duk__handle_catch_part1(duk_hthread *thr,
                                       duk_tval *tv_val_unstable,
                                       duk_small_uint_t lj_type,
                                       volatile duk_bool_t *out_delayed_catch_setup) {
	duk_activation *act;
	duk_catcher *cat;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(tv_val_unstable != NULL);

	act = thr->callstack_curr;
	DUK_ASSERT(act != NULL);
	DUK_DD(DUK_DDPRINT("handle catch, part 1; act=%!A, cat=%!C", act, act->cat));

	DUK_ASSERT(act->cat != NULL);
	DUK_ASSERT(DUK_CAT_GET_TYPE(act->cat) == DUK_CAT_TYPE_TCF);

	/* The part1/part2 split could also be made here at the very top
	 * of catch handling.  Value stack would be reconfigured inside
	 * part2's protection.  Value stack reconfiguration should be free
	 * of allocs, however.
	 */

	duk__set_catcher_regs_norz(thr, act->cat, tv_val_unstable, lj_type);

	DUK_ASSERT(thr->callstack_top >= 1);
	DUK_ASSERT(thr->callstack_curr != NULL);
	DUK_ASSERT(DUK_ACT_GET_FUNC(thr->callstack_curr) != NULL);
	DUK_ASSERT(DUK_HOBJECT_IS_COMPFUNC(DUK_ACT_GET_FUNC(thr->callstack_curr)));

	DUK_ASSERT(thr->callstack_top >= 1);
	DUK_ASSERT(act == thr->callstack_curr);
	DUK_ASSERT(act != NULL);
	duk__reconfig_valstack_ecma_catcher(thr, act);

	DUK_ASSERT(thr->callstack_top >= 1);
	DUK_ASSERT(act == thr->callstack_curr);
	DUK_ASSERT(act != NULL);
	cat = act->cat;
	DUK_ASSERT(cat != NULL);

	act->curr_pc = cat->pc_base + 0; /* +0 = catch */

	/*
	 *  If the catch block has an automatic catch variable binding,
	 *  we need to create a lexical environment for it which requires
	 *  allocations.  Move out of "error handling state" before the
	 *  allocations to avoid e.g. out-of-memory errors (leading to
	 *  GH-2022 or similar).
	 */

	if (DUK_CAT_HAS_CATCH_BINDING_ENABLED(cat)) {
		DUK_DDD(DUK_DDDPRINT("catcher has an automatic catch binding, handle in part 2"));
		*out_delayed_catch_setup = 1;
	} else {
		DUK_DDD(DUK_DDDPRINT("catcher has no catch binding"));
	}

	DUK_CAT_CLEAR_CATCH_ENABLED(cat);
}

DUK_LOCAL void duk__handle_catch_part2(duk_hthread *thr) {
	duk_activation *act;
	duk_catcher *cat;
	duk_hdecenv *new_env;

	DUK_ASSERT(thr != NULL);

	act = thr->callstack_curr;
	DUK_ASSERT(act != NULL);
	DUK_DD(DUK_DDPRINT("handle catch, part 2; act=%!A, cat=%!C", act, act->cat));

	DUK_ASSERT(act->cat != NULL);
	cat = act->cat;
	DUK_ASSERT(cat != NULL);
	DUK_ASSERT(DUK_CAT_GET_TYPE(cat) == DUK_CAT_TYPE_TCF);
	DUK_ASSERT(DUK_CAT_HAS_CATCH_BINDING_ENABLED(cat));
	DUK_ASSERT(thr->valstack + cat->idx_base < thr->valstack_top);

	/*
	 *  Create lexical environment for the catch clause, containing
	 *  a binding for the caught value.
	 *
	 *  The binding is mutable (= writable) but not deletable.
	 *  Step 4 for the catch production in E5 Section 12.14;
	 *  no value is given for CreateMutableBinding 'D' argument,
	 *  which implies the binding is not deletable.
	 */

	if (act->lex_env == NULL) {
		DUK_ASSERT(act->var_env == NULL);
		DUK_DDD(DUK_DDDPRINT("delayed environment initialization"));

		duk_js_init_activation_environment_records_delayed(thr, act);
		DUK_ASSERT(act == thr->callstack_curr);
		DUK_ASSERT(act != NULL);
	}
	DUK_ASSERT(act->lex_env != NULL);
	DUK_ASSERT(act->var_env != NULL);
	DUK_ASSERT(DUK_ACT_GET_FUNC(act) != NULL);

	new_env = duk_hdecenv_alloc(thr, DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_DECENV));
	DUK_ASSERT(new_env != NULL);
	duk_push_hobject(thr, (duk_hobject *) new_env);
	DUK_ASSERT(DUK_HOBJECT_GET_PROTOTYPE(thr->heap, (duk_hobject *) new_env) == NULL);
	DUK_DDD(DUK_DDDPRINT("new_env allocated: %!iO", (duk_heaphdr *) new_env));

	/* Note: currently the catch binding is handled without a register
	 * binding because we don't support dynamic register bindings (they
	 * must be fixed for an entire function).  So, there is no need to
	 * record regbases etc.
	 */

	/* [ ...env ] */

	DUK_ASSERT(cat->h_varname != NULL);
	duk_push_hstring(thr, cat->h_varname);
	DUK_ASSERT(thr->valstack + cat->idx_base < thr->valstack_top);
	duk_push_tval(thr, thr->valstack + cat->idx_base);
	duk_xdef_prop(thr, -3, DUK_PROPDESC_FLAGS_W); /* writable, not configurable */

	/* [ ... env ] */

	DUK_ASSERT(act == thr->callstack_curr);
	DUK_ASSERT(act != NULL);
	DUK_HOBJECT_SET_PROTOTYPE(thr->heap, (duk_hobject *) new_env, act->lex_env);
	act->lex_env = (duk_hobject *) new_env;
	DUK_HOBJECT_INCREF(thr, (duk_hobject *) new_env); /* reachable through activation */
	/* Net refcount change to act->lex_env is 0: incref for new_env's
	 * prototype, decref for act->lex_env overwrite.
	 */

	DUK_CAT_SET_LEXENV_ACTIVE(cat);

	duk_pop_unsafe(thr);

	DUK_DDD(DUK_DDDPRINT("new_env finished: %!iO", (duk_heaphdr *) new_env));
}

DUK_LOCAL void duk__handle_finally(duk_hthread *thr, duk_tval *tv_val_unstable, duk_small_uint_t lj_type) {
	duk_activation *act;
	duk_catcher *cat;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(tv_val_unstable != NULL);

	act = thr->callstack_curr;
	DUK_ASSERT(act != NULL);
	DUK_ASSERT(act->cat != NULL);
	DUK_ASSERT(DUK_CAT_GET_TYPE(act->cat) == DUK_CAT_TYPE_TCF);

	duk__set_catcher_regs_norz(thr, act->cat, tv_val_unstable, lj_type);

	DUK_ASSERT(thr->callstack_top >= 1);
	DUK_ASSERT(thr->callstack_curr != NULL);
	DUK_ASSERT(DUK_ACT_GET_FUNC(thr->callstack_curr) != NULL);
	DUK_ASSERT(DUK_HOBJECT_IS_COMPFUNC(DUK_ACT_GET_FUNC(thr->callstack_curr)));

	DUK_ASSERT(thr->callstack_top >= 1);
	DUK_ASSERT(act == thr->callstack_curr);
	DUK_ASSERT(act != NULL);
	duk__reconfig_valstack_ecma_catcher(thr, act);

	DUK_ASSERT(thr->callstack_top >= 1);
	DUK_ASSERT(act == thr->callstack_curr);
	DUK_ASSERT(act != NULL);
	cat = act->cat;
	DUK_ASSERT(cat != NULL);

	act->curr_pc = cat->pc_base + 1; /* +1 = finally */

	DUK_CAT_CLEAR_FINALLY_ENABLED(cat);
}

DUK_LOCAL void duk__handle_label(duk_hthread *thr, duk_small_uint_t lj_type) {
	duk_activation *act;
	duk_catcher *cat;

	DUK_ASSERT(thr != NULL);

	DUK_ASSERT(thr->callstack_top >= 1);
	act = thr->callstack_curr;
	DUK_ASSERT(act != NULL);
	DUK_ASSERT(DUK_ACT_GET_FUNC(act) != NULL);
	DUK_ASSERT(DUK_HOBJECT_HAS_COMPFUNC(DUK_ACT_GET_FUNC(act)));

	/* +0 = break, +1 = continue */
	cat = act->cat;
	DUK_ASSERT(cat != NULL);
	DUK_ASSERT(DUK_CAT_GET_TYPE(cat) == DUK_CAT_TYPE_LABEL);

	act->curr_pc = cat->pc_base + (lj_type == DUK_LJ_TYPE_CONTINUE ? 1 : 0);

	/* valstack should not need changes */
#if defined(DUK_USE_ASSERTIONS)
	DUK_ASSERT(thr->callstack_top >= 1);
	DUK_ASSERT(act == thr->callstack_curr);
	DUK_ASSERT(act != NULL);
	DUK_ASSERT((duk_size_t) (thr->valstack_top - thr->valstack_bottom) ==
	           (duk_size_t) ((duk_hcompfunc *) DUK_ACT_GET_FUNC(act))->nregs);
#endif
}

/* Called for handling both a longjmp() with type DUK_LJ_TYPE_YIELD and
 * when a RETURN opcode terminates a thread and yields to the resumer.
 * Caller unwinds so that top of callstack is the activation we return to.
 */
#if defined(DUK_USE_COROUTINE_SUPPORT)
DUK_LOCAL void duk__handle_yield(duk_hthread *thr, duk_hthread *resumer, duk_tval *tv_val_unstable) {
	duk_activation *act_resumer;
	duk_tval *tv1;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(resumer != NULL);
	DUK_ASSERT(tv_val_unstable != NULL);
	act_resumer = resumer->callstack_curr;
	DUK_ASSERT(act_resumer != NULL);
	DUK_ASSERT(DUK_ACT_GET_FUNC(act_resumer) != NULL);
	DUK_ASSERT(DUK_HOBJECT_IS_COMPFUNC(DUK_ACT_GET_FUNC(act_resumer))); /* resume caller must be an ECMAScript func */

	tv1 = (duk_tval *) (void *) ((duk_uint8_t *) resumer->valstack +
	                             act_resumer->retval_byteoff); /* return value from Duktape.Thread.resume() */
	DUK_TVAL_SET_TVAL_UPDREF(thr, tv1, tv_val_unstable); /* side effects */ /* XXX: avoid side effects */

	duk__reconfig_valstack_ecma_return(resumer);

	/* caller must change active thread, and set thr->resumer to NULL */
}
#endif /* DUK_USE_COROUTINE_SUPPORT */

DUK_LOCAL duk_small_uint_t duk__handle_longjmp(duk_hthread *thr,
                                               duk_activation *entry_act,
                                               volatile duk_bool_t *out_delayed_catch_setup) {
	duk_small_uint_t retval = DUK__LONGJMP_RESTART;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(entry_act != NULL);

	/* 'thr' is the current thread, as no-one resumes except us and we
	 * switch 'thr' in that case.
	 */
	DUK_ASSERT(thr == thr->heap->curr_thread);

	/*
	 *  (Re)try handling the longjmp.
	 *
	 *  A longjmp handler may convert the longjmp to a different type and
	 *  "virtually" rethrow by goto'ing to 'check_longjmp'.  Before the goto,
	 *  the following must be updated:
	 *    - the heap 'lj' state
	 *    - 'thr' must reflect the "throwing" thread
	 */

check_longjmp:

	DUK_DD(DUK_DDPRINT("handling longjmp: type=%ld, value1=%!T, value2=%!T, iserror=%ld, top=%ld",
	                   (long) thr->heap->lj.type,
	                   (duk_tval *) &thr->heap->lj.value1,
	                   (duk_tval *) &thr->heap->lj.value2,
	                   (long) thr->heap->lj.iserror,
	                   (long) duk_get_top(thr)));

	switch (thr->heap->lj.type) {
#if defined(DUK_USE_COROUTINE_SUPPORT)
	case DUK_LJ_TYPE_RESUME: {
		/*
		 *  Note: lj.value1 is 'value', lj.value2 is 'resumee'.
		 *  This differs from YIELD.
		 */

		duk_tval *tv;
		duk_tval *tv2;
		duk_hthread *resumee;

		/* duk_bi_duk_object_yield() and duk_bi_duk_object_resume() ensure all of these are met */

		DUK_ASSERT(thr->state == DUK_HTHREAD_STATE_RUNNING); /* unchanged by Duktape.Thread.resume() */
		DUK_ASSERT(thr->callstack_top >= 2); /* ECMAScript activation + Duktape.Thread.resume() activation */
		DUK_ASSERT(thr->callstack_curr != NULL);
		DUK_ASSERT(thr->callstack_curr->parent != NULL);
		DUK_ASSERT(DUK_ACT_GET_FUNC(thr->callstack_curr) != NULL &&
		           DUK_HOBJECT_IS_NATFUNC(DUK_ACT_GET_FUNC(thr->callstack_curr)) &&
		           ((duk_hnatfunc *) DUK_ACT_GET_FUNC(thr->callstack_curr))->func == duk_bi_thread_resume);

		tv = &thr->heap->lj.value2; /* resumee */
		DUK_ASSERT(DUK_TVAL_IS_OBJECT(tv));
		DUK_ASSERT(DUK_TVAL_GET_OBJECT(tv) != NULL);
		DUK_ASSERT(DUK_HOBJECT_IS_THREAD(DUK_TVAL_GET_OBJECT(tv)));
		resumee = (duk_hthread *) DUK_TVAL_GET_OBJECT(tv);

		DUK_ASSERT(resumee != NULL);
		DUK_ASSERT(resumee->resumer == NULL);
		DUK_ASSERT(resumee->state == DUK_HTHREAD_STATE_INACTIVE ||
		           resumee->state == DUK_HTHREAD_STATE_YIELDED); /* checked by Duktape.Thread.resume() */
		DUK_ASSERT(resumee->state != DUK_HTHREAD_STATE_YIELDED ||
		           resumee->callstack_top >= 2); /* YIELDED: ECMAScript activation + Duktape.Thread.yield() activation */
		DUK_ASSERT(resumee->state != DUK_HTHREAD_STATE_YIELDED ||
		           (DUK_ACT_GET_FUNC(resumee->callstack_curr) != NULL &&
		            DUK_HOBJECT_IS_NATFUNC(DUK_ACT_GET_FUNC(resumee->callstack_curr)) &&
		            ((duk_hnatfunc *) DUK_ACT_GET_FUNC(resumee->callstack_curr))->func == duk_bi_thread_yield));
		DUK_ASSERT(resumee->state != DUK_HTHREAD_STATE_INACTIVE ||
		           resumee->callstack_top == 0); /* INACTIVE: no activation, single function value on valstack */

		if (thr->heap->lj.iserror) {
			/*
			 *  Throw the error in the resumed thread's context; the
			 *  error value is pushed onto the resumee valstack.
			 *
			 *  Note: the callstack of the target may empty in this case
			 *  too (i.e. the target thread has never been resumed).  The
			 *  value stack will contain the initial function in that case,
			 *  which we simply ignore.
			 */

			DUK_ASSERT(resumee->resumer == NULL);
			resumee->resumer = thr;
			DUK_HTHREAD_INCREF(thr, thr);
			resumee->state = DUK_HTHREAD_STATE_RUNNING;
			thr->state = DUK_HTHREAD_STATE_RESUMED;
			DUK_HEAP_SWITCH_THREAD(thr->heap, resumee);
			thr = resumee;

			thr->heap->lj.type = DUK_LJ_TYPE_THROW;

			/* thr->heap->lj.value1 is already the value to throw */
			/* thr->heap->lj.value2 is 'thread', will be wiped out at the end */

			DUK_ASSERT(thr->heap->lj.iserror); /* already set */

			DUK_DD(DUK_DDPRINT("-> resume with an error, converted to a throw in the resumee, propagate"));
			goto check_longjmp;
		} else if (resumee->state == DUK_HTHREAD_STATE_YIELDED) {
			/* Unwind previous Duktape.Thread.yield() call.  The
			 * activation remaining must always be an ECMAScript
			 * call now (yield() accepts calls from ECMAScript
			 * only).
			 */
			duk_activation *act_resumee;

			DUK_ASSERT(resumee->callstack_top >= 2);
			act_resumee = resumee->callstack_curr; /* Duktape.Thread.yield() */
			DUK_ASSERT(act_resumee != NULL);
			act_resumee = act_resumee->parent; /* ECMAScript call site for yield() */
			DUK_ASSERT(act_resumee != NULL);

			tv = (duk_tval *) (void *) ((duk_uint8_t *) resumee->valstack +
			                            act_resumee->retval_byteoff); /* return value from Duktape.Thread.yield() */
			DUK_ASSERT(tv >= resumee->valstack && tv < resumee->valstack_top);
			tv2 = &thr->heap->lj.value1;
			DUK_TVAL_SET_TVAL_UPDREF(thr, tv, tv2); /* side effects */ /* XXX: avoid side effects */

			duk_hthread_activation_unwind_norz(resumee); /* unwind to 'yield' caller */
			/* no need to unwind catch stack */

			duk__reconfig_valstack_ecma_return(resumee);

			DUK_ASSERT(resumee->resumer == NULL);
			resumee->resumer = thr;
			DUK_HTHREAD_INCREF(thr, thr);
			resumee->state = DUK_HTHREAD_STATE_RUNNING;
			thr->state = DUK_HTHREAD_STATE_RESUMED;
			DUK_HEAP_SWITCH_THREAD(thr->heap, resumee);
#if 0
			thr = resumee;  /* not needed, as we exit right away */
#endif
			DUK_DD(DUK_DDPRINT("-> resume with a value, restart execution in resumee"));
			retval = DUK__LONGJMP_RESTART;
			goto wipe_and_return;
		} else {
			/* Initial resume call. */
			duk_small_uint_t call_flags;
			duk_int_t setup_rc;

			/* resumee: [... initial_func]  (currently actually: [initial_func]) */

			duk_push_undefined(resumee);
			tv = &thr->heap->lj.value1;
			duk_push_tval(resumee, tv);

			/* resumee: [... initial_func undefined(= this) resume_value ] */

			call_flags = DUK_CALL_FLAG_ALLOW_ECMATOECMA; /* not tailcall, ecma-to-ecma (assumed to succeed) */

			setup_rc = duk_handle_call_unprotected_nargs(resumee, 1 /*nargs*/, call_flags);
			if (setup_rc == 0) {
				/* This shouldn't happen; Duktape.Thread.resume()
				 * should make sure of that.  If it does happen
				 * this internal error will propagate out of the
				 * executor which can be quite misleading.
				 */
				DUK_ERROR_INTERNAL(thr);
				DUK_WO_NORETURN(return 0;);
			}

			DUK_ASSERT(resumee->resumer == NULL);
			resumee->resumer = thr;
			DUK_HTHREAD_INCREF(thr, thr);
			resumee->state = DUK_HTHREAD_STATE_RUNNING;
			thr->state = DUK_HTHREAD_STATE_RESUMED;
			DUK_HEAP_SWITCH_THREAD(thr->heap, resumee);
#if 0
			thr = resumee;  /* not needed, as we exit right away */
#endif
			DUK_DD(DUK_DDPRINT("-> resume with a value, restart execution in resumee"));
			retval = DUK__LONGJMP_RESTART;
			goto wipe_and_return;
		}
		DUK_UNREACHABLE();
		break; /* never here */
	}

	case DUK_LJ_TYPE_YIELD: {
		/*
		 *  Currently only allowed only if yielding thread has only
		 *  ECMAScript activations (except for the Duktape.Thread.yield()
		 *  call at the callstack top) and none of them constructor
		 *  calls.
		 *
		 *  This excludes the 'entry' thread which will always have
		 *  a preventcount > 0.
		 */

		duk_hthread *resumer;

		/* duk_bi_duk_object_yield() and duk_bi_duk_object_resume() ensure all of these are met */

#if 0 /* entry_thread not available for assert */
		DUK_ASSERT(thr != entry_thread);                                                                             /* Duktape.Thread.yield() should prevent */
#endif
		DUK_ASSERT(thr->state == DUK_HTHREAD_STATE_RUNNING); /* unchanged from Duktape.Thread.yield() */
		DUK_ASSERT(thr->callstack_top >= 2); /* ECMAScript activation + Duktape.Thread.yield() activation */
		DUK_ASSERT(thr->callstack_curr != NULL);
		DUK_ASSERT(thr->callstack_curr->parent != NULL);
		DUK_ASSERT(DUK_ACT_GET_FUNC(thr->callstack_curr) != NULL &&
		           DUK_HOBJECT_IS_NATFUNC(DUK_ACT_GET_FUNC(thr->callstack_curr)) &&
		           ((duk_hnatfunc *) DUK_ACT_GET_FUNC(thr->callstack_curr))->func == duk_bi_thread_yield);
		DUK_ASSERT(DUK_ACT_GET_FUNC(thr->callstack_curr->parent) != NULL &&
		           DUK_HOBJECT_IS_COMPFUNC(DUK_ACT_GET_FUNC(thr->callstack_curr->parent))); /* an ECMAScript function */

		resumer = thr->resumer;

		DUK_ASSERT(resumer != NULL);
		DUK_ASSERT(resumer->state == DUK_HTHREAD_STATE_RESUMED); /* written by a previous RESUME handling */
		DUK_ASSERT(resumer->callstack_top >= 2); /* ECMAScript activation + Duktape.Thread.resume() activation */
		DUK_ASSERT(resumer->callstack_curr != NULL);
		DUK_ASSERT(resumer->callstack_curr->parent != NULL);
		DUK_ASSERT(DUK_ACT_GET_FUNC(resumer->callstack_curr) != NULL &&
		           DUK_HOBJECT_IS_NATFUNC(DUK_ACT_GET_FUNC(resumer->callstack_curr)) &&
		           ((duk_hnatfunc *) DUK_ACT_GET_FUNC(resumer->callstack_curr))->func == duk_bi_thread_resume);
		DUK_ASSERT(DUK_ACT_GET_FUNC(resumer->callstack_curr->parent) != NULL &&
		           DUK_HOBJECT_IS_COMPFUNC(DUK_ACT_GET_FUNC(resumer->callstack_curr->parent))); /* an ECMAScript function */

		if (thr->heap->lj.iserror) {
			thr->state = DUK_HTHREAD_STATE_YIELDED;
			thr->resumer = NULL;
			DUK_HTHREAD_DECREF_NORZ(thr, resumer);
			resumer->state = DUK_HTHREAD_STATE_RUNNING;
			DUK_HEAP_SWITCH_THREAD(thr->heap, resumer);
			thr = resumer;

			thr->heap->lj.type = DUK_LJ_TYPE_THROW;
			/* lj.value1 is already set */
			DUK_ASSERT(thr->heap->lj.iserror); /* already set */

			DUK_DD(DUK_DDPRINT("-> yield an error, converted to a throw in the resumer, propagate"));
			goto check_longjmp;
		} else {
			/* When handling the yield, the last reference to
			 * 'thr' may disappear.
			 */

			DUK_GC_TORTURE(resumer->heap);
			duk_hthread_activation_unwind_norz(resumer);
			DUK_GC_TORTURE(resumer->heap);
			thr->state = DUK_HTHREAD_STATE_YIELDED;
			thr->resumer = NULL;
			DUK_HTHREAD_DECREF_NORZ(thr, resumer);
			resumer->state = DUK_HTHREAD_STATE_RUNNING;
			DUK_HEAP_SWITCH_THREAD(thr->heap, resumer);
			duk__handle_yield(thr, resumer, &thr->heap->lj.value1);
			thr = resumer;
			DUK_GC_TORTURE(resumer->heap);

			DUK_DD(DUK_DDPRINT("-> yield a value, restart execution in resumer"));
			retval = DUK__LONGJMP_RESTART;
			goto wipe_and_return;
		}
		DUK_UNREACHABLE();
		break; /* never here */
	}
#endif /* DUK_USE_COROUTINE_SUPPORT */

	case DUK_LJ_TYPE_THROW: {
		/*
		 *  Three possible outcomes:
		 *    * A try or finally catcher is found => resume there.
		 *      (or)
		 *    * The error propagates to the bytecode executor entry
		 *      level (and we're in the entry thread) => rethrow
		 *      with a new longjmp(), after restoring the previous
		 *      catchpoint.
		 *    * The error is not caught in the current thread, so
		 *      the thread finishes with an error.  This works like
		 *      a yielded error, except that the thread is finished
		 *      and can no longer be resumed.  (There is always a
		 *      resumer in this case.)
		 *
		 *  Note: until we hit the entry level, there can only be
		 *  ECMAScript activations.
		 */

		duk_activation *act;
		duk_catcher *cat;
		duk_hthread *resumer;

		for (;;) {
			act = thr->callstack_curr;
			if (act == NULL) {
				break;
			}

			for (;;) {
				cat = act->cat;
				if (cat == NULL) {
					break;
				}

				if (DUK_CAT_HAS_CATCH_ENABLED(cat)) {
					DUK_ASSERT(DUK_CAT_GET_TYPE(cat) == DUK_CAT_TYPE_TCF);

					DUK_DDD(DUK_DDDPRINT("before catch part 1: thr=%p, act=%p, cat=%p",
					                     (void *) thr,
					                     (void *) act,
					                     (void *) act->cat));
					duk__handle_catch_part1(thr,
					                        &thr->heap->lj.value1,
					                        DUK_LJ_TYPE_THROW,
					                        out_delayed_catch_setup);

					DUK_DD(DUK_DDPRINT("-> throw caught by a 'catch' clause, restart execution"));
					retval = DUK__LONGJMP_RESTART;
					goto wipe_and_return;
				}

				if (DUK_CAT_HAS_FINALLY_ENABLED(cat)) {
					DUK_ASSERT(DUK_CAT_GET_TYPE(cat) == DUK_CAT_TYPE_TCF);
					DUK_ASSERT(!DUK_CAT_HAS_CATCH_ENABLED(cat));

					duk__handle_finally(thr, &thr->heap->lj.value1, DUK_LJ_TYPE_THROW);

					DUK_DD(DUK_DDPRINT("-> throw caught by a 'finally' clause, restart execution"));
					retval = DUK__LONGJMP_RESTART;
					goto wipe_and_return;
				}

				duk_hthread_catcher_unwind_norz(thr, act);
			}

			if (act == entry_act) {
				/* Not caught by anything before entry level; rethrow and let the
				 * final catcher finish unwinding (esp. value stack).
				 */
				DUK_D(DUK_DPRINT("-> throw propagated up to entry level, rethrow and exit bytecode executor"));
				retval = DUK__LONGJMP_RETHROW;
				goto just_return;
			}

			duk_hthread_activation_unwind_norz(thr);
		}

		DUK_DD(DUK_DDPRINT("-> throw not caught by current thread, yield error to resumer and recheck longjmp"));

		/* Not caught by current thread, thread terminates (yield error to resumer);
		 * note that this may cause a cascade if the resumer terminates with an uncaught
		 * exception etc (this is OK, but needs careful testing).
		 */

		DUK_ASSERT(thr->resumer != NULL);
		DUK_ASSERT(thr->resumer->callstack_top >= 2); /* ECMAScript activation + Duktape.Thread.resume() activation */
		DUK_ASSERT(thr->resumer->callstack_curr != NULL);
		DUK_ASSERT(thr->resumer->callstack_curr->parent != NULL);
		DUK_ASSERT(
		    DUK_ACT_GET_FUNC(thr->resumer->callstack_curr->parent) != NULL &&
		    DUK_HOBJECT_IS_COMPFUNC(DUK_ACT_GET_FUNC(thr->resumer->callstack_curr->parent))); /* an ECMAScript function */

		resumer = thr->resumer;

		/* reset longjmp */

		DUK_ASSERT(thr->heap->lj.type == DUK_LJ_TYPE_THROW); /* already set */
		/* lj.value1 already set */

		duk_hthread_terminate(thr); /* updates thread state, minimizes its allocations */
		DUK_ASSERT(thr->state == DUK_HTHREAD_STATE_TERMINATED);

		thr->resumer = NULL;
		DUK_HTHREAD_DECREF_NORZ(thr, resumer);
		resumer->state = DUK_HTHREAD_STATE_RUNNING;
		DUK_HEAP_SWITCH_THREAD(thr->heap, resumer);
		thr = resumer;
		goto check_longjmp;
	}

	case DUK_LJ_TYPE_BREAK: /* pseudotypes, not used in actual longjmps */
	case DUK_LJ_TYPE_CONTINUE:
	case DUK_LJ_TYPE_RETURN:
	case DUK_LJ_TYPE_NORMAL:
	default: {
		/* should never happen, but be robust */
		DUK_D(DUK_DPRINT("caught unknown longjmp type %ld, treat as internal error", (long) thr->heap->lj.type));
		goto convert_to_internal_error;
	}

	} /* end switch */

	DUK_UNREACHABLE();

wipe_and_return:
	DUK_DD(DUK_DDPRINT("handling longjmp done, wipe-and-return, top=%ld", (long) duk_get_top(thr)));
	thr->heap->lj.type = DUK_LJ_TYPE_UNKNOWN;
	thr->heap->lj.iserror = 0;

	DUK_TVAL_SET_UNDEFINED_UPDREF(thr, &thr->heap->lj.value1); /* side effects */
	DUK_TVAL_SET_UNDEFINED_UPDREF(thr, &thr->heap->lj.value2); /* side effects */

	DUK_GC_TORTURE(thr->heap);

just_return:
	return retval;

convert_to_internal_error:
	/* This could also be thrown internally (set the error, goto check_longjmp),
	 * but it's better for internal errors to bubble outwards so that we won't
	 * infinite loop in this catchpoint.
	 */
	DUK_ERROR_INTERNAL(thr);
	DUK_WO_NORETURN(return 0;);
}

/* Handle a BREAK/CONTINUE opcode.  Avoid using longjmp() for BREAK/CONTINUE
 * handling because it has a measurable performance impact in ordinary
 * environments and an extreme impact in Emscripten (GH-342).
 */
DUK_LOCAL DUK_EXEC_NOINLINE_PERF void duk__handle_break_or_continue(duk_hthread *thr,
                                                                    duk_uint_t label_id,
                                                                    duk_small_uint_t lj_type) {
	duk_activation *act;
	duk_catcher *cat;

	DUK_ASSERT(thr != NULL);

	/* Find a matching label catcher or 'finally' catcher in
	 * the same function, unwinding catchers as we go.
	 *
	 * A label catcher must always exist and will match unless
	 * a 'finally' captures the break/continue first.  It is the
	 * compiler's responsibility to ensure that labels are used
	 * correctly.
	 */

	act = thr->callstack_curr;
	DUK_ASSERT(act != NULL);

	for (;;) {
		cat = act->cat;
		if (cat == NULL) {
			break;
		}

		DUK_DDD(DUK_DDDPRINT("considering catcher %p: type=%ld label=%ld",
		                     (void *) cat,
		                     (long) DUK_CAT_GET_TYPE(cat),
		                     (long) DUK_CAT_GET_LABEL(cat)));

		/* XXX: bit mask test; FINALLY <-> TCF, single bit mask would suffice? */

		if (DUK_CAT_GET_TYPE(cat) == DUK_CAT_TYPE_TCF && DUK_CAT_HAS_FINALLY_ENABLED(cat)) {
			duk_tval tv_tmp;

			DUK_TVAL_SET_U32(&tv_tmp, (duk_uint32_t) label_id);
			duk__handle_finally(thr, &tv_tmp, lj_type);

			DUK_DD(DUK_DDPRINT("-> break/continue caught by 'finally', restart execution"));
			return;
		}
		if (DUK_CAT_GET_TYPE(cat) == DUK_CAT_TYPE_LABEL && (duk_uint_t) DUK_CAT_GET_LABEL(cat) == label_id) {
			duk__handle_label(thr, lj_type);

			DUK_DD(
			    DUK_DDPRINT("-> break/continue caught by a label catcher (in the same function), restart execution"));
			return;
		}

		duk_hthread_catcher_unwind_norz(thr, act);
	}

	/* Should never happen, but be robust. */
	DUK_D(DUK_DPRINT(
	    "-> break/continue not caught by anything in the current function (should never happen), throw internal error"));
	DUK_ERROR_INTERNAL(thr);
	DUK_WO_NORETURN(return;);
}

/* Handle a RETURN opcode.  Avoid using longjmp() for return handling because
 * it has a measurable performance impact in ordinary environments and an extreme
 * impact in Emscripten (GH-342).  Return value is on value stack top.
 */
DUK_LOCAL duk_small_uint_t duk__handle_return(duk_hthread *thr, duk_activation *entry_act) {
	duk_tval *tv1;
	duk_tval *tv2;
#if defined(DUK_USE_COROUTINE_SUPPORT)
	duk_hthread *resumer;
#endif
	duk_activation *act;
	duk_catcher *cat;

	/* We can directly access value stack here. */

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(entry_act != NULL);
	DUK_ASSERT(thr->valstack_top - 1 >= thr->valstack_bottom);
	tv1 = thr->valstack_top - 1;
	DUK_TVAL_CHKFAST_INPLACE_FAST(tv1); /* fastint downgrade check for return values */

	/*
	 *  Four possible outcomes:
	 *
	 *    1. A 'finally' in the same function catches the 'return'.
	 *       It may continue to propagate when 'finally' is finished,
	 *       or it may be neutralized by 'finally' (both handled by
	 *       ENDFIN).
	 *
	 *    2. The return happens at the entry level of the bytecode
	 *       executor, so return from the executor (in C stack).
	 *
	 *    3. There is a calling (ECMAScript) activation in the call
	 *       stack => return to it, in the same executor instance.
	 *
	 *    4. There is no calling activation, and the thread is
	 *       terminated.  There is always a resumer in this case,
	 *       which gets the return value similarly to a 'yield'
	 *       (except that the current thread can no longer be
	 *       resumed).
	 */

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->callstack_top >= 1);

	act = thr->callstack_curr;
	DUK_ASSERT(act != NULL);

	for (;;) {
		cat = act->cat;
		if (cat == NULL) {
			break;
		}

		if (DUK_CAT_GET_TYPE(cat) == DUK_CAT_TYPE_TCF && DUK_CAT_HAS_FINALLY_ENABLED(cat)) {
			DUK_ASSERT(thr->valstack_top - 1 >= thr->valstack_bottom);
			duk__handle_finally(thr, thr->valstack_top - 1, DUK_LJ_TYPE_RETURN);

			DUK_DD(DUK_DDPRINT("-> return caught by 'finally', restart execution"));
			return DUK__RETHAND_RESTART;
		}

		duk_hthread_catcher_unwind_norz(thr, act);
	}

	if (act == entry_act) {
		/* Return to the bytecode executor caller who will unwind stacks
		 * and handle constructor post-processing.
		 * Return value is already on the stack top: [ ... retval ].
		 */

		DUK_DDD(DUK_DDDPRINT("-> return propagated up to entry level, exit bytecode executor"));
		return DUK__RETHAND_FINISHED;
	}

	if (thr->callstack_top >= 2) {
		/* There is a caller; it MUST be an ECMAScript caller (otherwise it would
		 * match entry_act check).
		 */
		DUK_DDD(DUK_DDDPRINT("return to ECMAScript caller, retval_byteoff=%ld, lj_value1=%!T",
		                     (long) (thr->callstack_curr->parent->retval_byteoff),
		                     (duk_tval *) &thr->heap->lj.value1));

		DUK_ASSERT(thr->callstack_curr != NULL);
		DUK_ASSERT(thr->callstack_curr->parent != NULL);
		DUK_ASSERT(DUK_HOBJECT_IS_COMPFUNC(DUK_ACT_GET_FUNC(thr->callstack_curr->parent))); /* must be ECMAScript */

#if defined(DUK_USE_ES6_PROXY)
		if (thr->callstack_curr->flags & (DUK_ACT_FLAG_CONSTRUCT | DUK_ACT_FLAG_CONSTRUCT_PROXY)) {
			duk_call_construct_postprocess(thr,
			                               thr->callstack_curr->flags &
			                                   DUK_ACT_FLAG_CONSTRUCT_PROXY); /* side effects */
		}
#else
		if (thr->callstack_curr->flags & DUK_ACT_FLAG_CONSTRUCT) {
			duk_call_construct_postprocess(thr, 0); /* side effects */
		}
#endif

		tv1 = (duk_tval *) (void *) ((duk_uint8_t *) thr->valstack + thr->callstack_curr->parent->retval_byteoff);
		DUK_ASSERT(thr->valstack_top - 1 >= thr->valstack_bottom);
		tv2 = thr->valstack_top - 1;
		DUK_TVAL_SET_TVAL_UPDREF(thr, tv1, tv2); /* side effects */

		/* Catch stack unwind happens inline in callstack unwind. */
		duk_hthread_activation_unwind_norz(thr);

		duk__reconfig_valstack_ecma_return(thr);

		DUK_DD(DUK_DDPRINT("-> return not intercepted, restart execution in caller"));
		return DUK__RETHAND_RESTART;
	}

#if defined(DUK_USE_COROUTINE_SUPPORT)
	DUK_DD(DUK_DDPRINT("no calling activation, thread finishes (similar to yield)"));

	DUK_ASSERT(thr->resumer != NULL);
	DUK_ASSERT(thr->resumer->callstack_top >= 2); /* ECMAScript activation + Duktape.Thread.resume() activation */
	DUK_ASSERT(thr->resumer->callstack_curr != NULL);
	DUK_ASSERT(thr->resumer->callstack_curr->parent != NULL);
	DUK_ASSERT(DUK_ACT_GET_FUNC(thr->resumer->callstack_curr) != NULL &&
	           DUK_HOBJECT_IS_NATFUNC(DUK_ACT_GET_FUNC(thr->resumer->callstack_curr)) &&
	           ((duk_hnatfunc *) DUK_ACT_GET_FUNC(thr->resumer->callstack_curr))->func ==
	               duk_bi_thread_resume); /* Duktape.Thread.resume() */
	DUK_ASSERT(DUK_ACT_GET_FUNC(thr->resumer->callstack_curr->parent) != NULL &&
	           DUK_HOBJECT_IS_COMPFUNC(DUK_ACT_GET_FUNC(thr->resumer->callstack_curr->parent))); /* an ECMAScript function */
	DUK_ASSERT(thr->state == DUK_HTHREAD_STATE_RUNNING);
	DUK_ASSERT(thr->resumer->state == DUK_HTHREAD_STATE_RESUMED);

	resumer = thr->resumer;

	/* Share yield longjmp handler.
	 *
	 * This sequence of steps is a bit fragile (see GH-1845):
	 * - We need the return value from 'thr' (resumed thread) value stack.
	 *   The termination unwinds its value stack, losing the value.
	 * - We need a refcounted reference for 'thr', which may only exist
	 *   in the caller value stack.  We can't unwind or reconfigure the
	 *   caller's value stack without potentially freeing 'thr'.
	 *
	 * Current approach is to capture the 'thr' return value and store
	 * a reference to 'thr' in the caller value stack temporarily.  This
	 * keeps 'thr' reachable until final yield/return handling which
	 * removes the references atomatically.
	 */

	DUK_ASSERT(thr->valstack_top - 1 >= thr->valstack_bottom);
	duk_hthread_activation_unwind_norz(resumer); /* May remove last reference to 'thr', but is NORZ. */
	duk_push_tval(resumer, thr->valstack_top - 1); /* Capture return value, side effect free. */
	duk_push_hthread(resumer, thr); /* Make 'thr' reachable again, before side effects. */

	duk_hthread_terminate(thr); /* Updates thread state, minimizes its allocations. */
	thr->resumer = NULL;
	DUK_HTHREAD_DECREF(thr, resumer);
	DUK_ASSERT(thr->state == DUK_HTHREAD_STATE_TERMINATED);

	resumer->state = DUK_HTHREAD_STATE_RUNNING;
	DUK_HEAP_SWITCH_THREAD(thr->heap, resumer);

	DUK_ASSERT(resumer->valstack_top - 2 >= resumer->valstack_bottom);
	duk__handle_yield(thr, resumer, resumer->valstack_top - 2);
	thr = NULL; /* 'thr' invalidated by call */

#if 0
	thr = resumer;  /* not needed */
#endif

	DUK_DD(DUK_DDPRINT("-> return not caught, thread terminated; handle like yield, restart execution in resumer"));
	return DUK__RETHAND_RESTART;
#else
	/* Without coroutine support this case should never happen. */
	DUK_ERROR_INTERNAL(thr);
	DUK_WO_NORETURN(return 0;);
#endif
}

/*
 *  Executor interrupt handling
 *
 *  The handler is called whenever the interrupt countdown reaches zero
 *  (or below).  The handler must perform whatever checks are activated,
 *  e.g. check for cumulative step count to impose an execution step
 *  limit or check for breakpoints or other debugger interaction.
 *
 *  When the actions are done, the handler must reinit the interrupt
 *  init and counter values.  The 'init' value must indicate how many
 *  bytecode instructions are executed before the next interrupt.  The
 *  counter must interface with the bytecode executor loop.  Concretely,
 *  the new init value is normally one higher than the new counter value.
 *  For instance, to execute exactly one bytecode instruction the init
 *  value is set to 1 and the counter to 0.  If an error is thrown by the
 *  interrupt handler, the counters are set to the same value (e.g. both
 *  to 0 to cause an interrupt when the next bytecode instruction is about
 *  to be executed after error handling).
 *
 *  Maintaining the init/counter value properly is important for accurate
 *  behavior.  For instance, executor step limit needs a cumulative step
 *  count which is simply computed as a sum of 'init' values.  This must
 *  work accurately even when single stepping.
 */

#if defined(DUK_USE_INTERRUPT_COUNTER)

#define DUK__INT_NOACTION 0 /* no specific action, resume normal execution */
#define DUK__INT_RESTART  1 /* must "goto restart_execution", e.g. breakpoints changed */

#if defined(DUK_USE_DEBUGGER_SUPPORT)
DUK_LOCAL void duk__interrupt_handle_debugger(duk_hthread *thr, duk_bool_t *out_immediate, duk_small_uint_t *out_interrupt_retval) {
	duk_activation *act;
	duk_breakpoint *bp;
	duk_breakpoint **bp_active;
	duk_uint_fast32_t line = 0;
	duk_bool_t process_messages;
	duk_bool_t processed_messages = 0;

	DUK_ASSERT(thr->heap->dbg_processing == 0); /* don't re-enter e.g. during Eval */

	act = thr->callstack_curr;
	DUK_ASSERT(act != NULL);

	/* It might seem that replacing 'thr->heap' with just 'heap' below
	 * might be a good idea, but it increases code size slightly
	 * (probably due to unnecessary spilling) at least on x64.
	 */

	/*
	 *  Single opcode step check
	 */

	if (thr->heap->dbg_pause_flags & DUK_PAUSE_FLAG_ONE_OPCODE_ACTIVE) {
		DUK_D(DUK_DPRINT("PAUSE TRIGGERED by one opcode step"));
		duk_debug_set_paused(thr->heap);
	}

	/*
	 *  Breakpoint and step state checks
	 */

	if (act->flags & DUK_ACT_FLAG_BREAKPOINT_ACTIVE || (thr->heap->dbg_pause_act == thr->callstack_curr)) {
		line = duk_debug_curr_line(thr);

		if (act->prev_line != line) {
			/* Stepped?  Step out is handled by callstack unwind. */
			if ((thr->heap->dbg_pause_flags & DUK_PAUSE_FLAG_LINE_CHANGE) &&
			    (thr->heap->dbg_pause_act == thr->callstack_curr) && (line != thr->heap->dbg_pause_startline)) {
				DUK_D(DUK_DPRINT("PAUSE TRIGGERED by line change, at line %ld", (long) line));
				duk_debug_set_paused(thr->heap);
			}

			/* Check for breakpoints only on line transition.
			 * Breakpoint is triggered when we enter the target
			 * line from a different line, and the previous line
			 * was within the same function.
			 *
			 * This condition is tricky: the condition used to be
			 * that transition to -or across- the breakpoint line
			 * triggered the breakpoint.  This seems intuitively
			 * better because it handles breakpoints on lines with
			 * no emitted opcodes; but this leads to the issue
			 * described in: https://github.com/svaarala/duktape/issues/263.
			 */
			bp_active = thr->heap->dbg_breakpoints_active;
			for (;;) {
				bp = *bp_active++;
				if (bp == NULL) {
					break;
				}

				DUK_ASSERT(bp->filename != NULL);
				if (act->prev_line != bp->line && line == bp->line) {
					DUK_D(DUK_DPRINT("PAUSE TRIGGERED by breakpoint at %!O:%ld",
					                 (duk_heaphdr *) bp->filename,
					                 (long) bp->line));
					duk_debug_set_paused(thr->heap);
				}
			}
		} else {
			;
		}

		act->prev_line = (duk_uint32_t) line;
	}

	/*
	 *  Rate limit check for sending status update or peeking into
	 *  the debug transport.  Both can be expensive operations that
	 *  we don't want to do on every opcode.
	 *
	 *  Making sure the interval remains reasonable on a wide variety
	 *  of targets and bytecode is difficult without a timestamp, so
	 *  we use a Date-provided timestamp for the rate limit check.
	 *  But since it's also expensive to get a timestamp, a bytecode
	 *  counter is used to rate limit getting timestamps.
	 */

	process_messages = 0;
	if (thr->heap->dbg_state_dirty || DUK_HEAP_HAS_DEBUGGER_PAUSED(thr->heap) || thr->heap->dbg_detaching) {
		/* Enter message processing loop for sending Status notifys and
		 * to finish a pending detach.
		 */
		process_messages = 1;
	}

	/* XXX: remove heap->dbg_exec_counter, use heap->inst_count_interrupt instead? */
	DUK_ASSERT(thr->interrupt_init >= 0);
	thr->heap->dbg_exec_counter += (duk_uint_t) thr->interrupt_init;
	if (thr->heap->dbg_exec_counter - thr->heap->dbg_last_counter >= DUK_HEAP_DBG_RATELIMIT_OPCODES) {
		/* Overflow of the execution counter is fine and doesn't break
		 * anything here.
		 */

		duk_double_t now, diff_last;

		thr->heap->dbg_last_counter = thr->heap->dbg_exec_counter;
		now = duk_time_get_monotonic_time(thr);

		diff_last = now - thr->heap->dbg_last_time;
		if (diff_last < 0.0 || diff_last >= (duk_double_t) DUK_HEAP_DBG_RATELIMIT_MILLISECS) {
			/* Monotonic time should not experience time jumps,
			 * but the provider may be missing and we're actually
			 * using ECMAScript time.  So, tolerate negative values
			 * so that a time jump works reasonably.
			 *
			 * Same interval is now used for status sending and
			 * peeking.
			 */

			thr->heap->dbg_last_time = now;
			thr->heap->dbg_state_dirty = 1;
			process_messages = 1;
		}
	}

	/*
	 *  Process messages and send status if necessary.
	 *
	 *  If we're paused, we'll block for new messages.  If we're not
	 *  paused, we'll process anything we can peek but won't block
	 *  for more.  Detach (and re-attach) handling is all localized
	 *  to duk_debug_process_messages() too.
	 *
	 *  Debugger writes outside the message loop may cause debugger
	 *  detach1 phase to run, after which dbg_read_cb == NULL and
	 *  dbg_detaching != 0.  The message loop will finish the detach
	 *  by running detach2 phase, so enter the message loop also when
	 *  detaching.
	 */

	if (process_messages) {
		DUK_ASSERT(thr->heap->dbg_processing == 0);
		processed_messages = duk_debug_process_messages(thr, 0 /*no_block*/);
		DUK_ASSERT(thr->heap->dbg_processing == 0);
	}

	/* Continue checked execution if there are breakpoints or we're stepping.
	 * Also use checked execution if paused flag is active - it shouldn't be
	 * because the debug message loop shouldn't terminate if it was.  Step out
	 * is handled by callstack unwind and doesn't need checked execution.
	 * Note that debugger may have detached due to error or explicit request
	 * above, so we must recheck attach status.
	 */

	if (duk_debug_is_attached(thr->heap)) {
		DUK_ASSERT(act == thr->callstack_curr);
		DUK_ASSERT(act != NULL);
		if (act->flags & DUK_ACT_FLAG_BREAKPOINT_ACTIVE || (thr->heap->dbg_pause_flags & DUK_PAUSE_FLAG_ONE_OPCODE) ||
		    ((thr->heap->dbg_pause_flags & DUK_PAUSE_FLAG_LINE_CHANGE) &&
		     thr->heap->dbg_pause_act == thr->callstack_curr) ||
		    DUK_HEAP_HAS_DEBUGGER_PAUSED(thr->heap)) {
			*out_immediate = 1;
		}

		/* If we processed any debug messages breakpoints may have
		 * changed; restart execution to re-check active breakpoints.
		 */
		if (processed_messages) {
			DUK_D(DUK_DPRINT("processed debug messages, restart execution to recheck possibly changed breakpoints"));
			*out_interrupt_retval = DUK__INT_RESTART;
		} else {
			if (thr->heap->dbg_pause_flags & DUK_PAUSE_FLAG_ONE_OPCODE) {
				/* Set 'pause after one opcode' active only when we're
				 * actually just about to execute code.
				 */
				thr->heap->dbg_pause_flags |= DUK_PAUSE_FLAG_ONE_OPCODE_ACTIVE;
			}
		}
	} else {
		DUK_D(DUK_DPRINT("debugger became detached, resume normal execution"));
	}
}
#endif /* DUK_USE_DEBUGGER_SUPPORT */

DUK_LOCAL DUK_EXEC_NOINLINE_PERF DUK_COLD duk_small_uint_t duk__executor_interrupt(duk_hthread *thr) {
	duk_int_t ctr;
	duk_activation *act;
	duk_hcompfunc *fun;
	duk_bool_t immediate = 0;
	duk_small_uint_t retval;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(thr->callstack_top > 0);

#if defined(DUK_USE_DEBUG)
	thr->heap->inst_count_interrupt += thr->interrupt_init;
	DUK_DD(DUK_DDPRINT("execution interrupt, counter=%ld, init=%ld, "
	                   "instruction counts: executor=%ld, interrupt=%ld",
	                   (long) thr->interrupt_counter,
	                   (long) thr->interrupt_init,
	                   (long) thr->heap->inst_count_exec,
	                   (long) thr->heap->inst_count_interrupt));
#endif

	retval = DUK__INT_NOACTION;
	ctr = DUK_HTHREAD_INTCTR_DEFAULT;

	/*
	 *  Avoid nested calls.  Concretely this happens during debugging, e.g.
	 *  when we eval() an expression.
	 *
	 *  Also don't interrupt if we're currently doing debug processing
	 *  (which can be initiated outside the bytecode executor) as this
	 *  may cause the debugger to be called recursively.  Check required
	 *  for correct operation of throw intercept and other "exotic" halting
	 * scenarios.
	 */

#if defined(DUK_USE_DEBUGGER_SUPPORT)
	if (DUK_HEAP_HAS_INTERRUPT_RUNNING(thr->heap) || thr->heap->dbg_processing) {
#else
	if (DUK_HEAP_HAS_INTERRUPT_RUNNING(thr->heap)) {
#endif
		DUK_DD(DUK_DDPRINT("nested executor interrupt, ignoring"));

		/* Set a high interrupt counter; the original executor
		 * interrupt invocation will rewrite before exiting.
		 */
		thr->interrupt_init = ctr;
		thr->interrupt_counter = ctr - 1;
		return DUK__INT_NOACTION;
	}
	DUK_HEAP_SET_INTERRUPT_RUNNING(thr->heap);

	act = thr->callstack_curr;
	DUK_ASSERT(act != NULL);

	fun = (duk_hcompfunc *) DUK_ACT_GET_FUNC(act);
	DUK_ASSERT(DUK_HOBJECT_HAS_COMPFUNC((duk_hobject *) fun));

	DUK_UNREF(fun);

#if defined(DUK_USE_EXEC_TIMEOUT_CHECK)
	/*
	 *  Execution timeout check
	 */

	if (DUK_USE_EXEC_TIMEOUT_CHECK(thr->heap->heap_udata)) {
		/* Keep throwing an error whenever we get here.  The unusual values
		 * are set this way because no instruction is ever executed, we just
		 * throw an error until all try/catch/finally and other catchpoints
		 * have been exhausted.  Duktape/C code gets control at each protected
		 * call but whenever it enters back into Duktape the RangeError gets
		 * raised.  User exec timeout check must consistently indicate a timeout
		 * until we've fully bubbled out of Duktape.
		 */
		DUK_D(DUK_DPRINT("execution timeout, throwing a RangeError"));
		thr->interrupt_init = 0;
		thr->interrupt_counter = 0;
		DUK_HEAP_CLEAR_INTERRUPT_RUNNING(thr->heap);
		DUK_ERROR_RANGE(thr, "execution timeout");
		DUK_WO_NORETURN(return 0;);
	}
#endif /* DUK_USE_EXEC_TIMEOUT_CHECK */

#if defined(DUK_USE_DEBUGGER_SUPPORT)
	if (!thr->heap->dbg_processing && (thr->heap->dbg_read_cb != NULL || thr->heap->dbg_detaching)) {
		/* Avoid recursive re-entry; enter when we're attached or
		 * detaching (to finish off the pending detach).
		 */
		duk__interrupt_handle_debugger(thr, &immediate, &retval);
		DUK_ASSERT(act == thr->callstack_curr);
	}
#endif /* DUK_USE_DEBUGGER_SUPPORT */

	/*
	 *  Update the interrupt counter
	 */

	if (immediate) {
		/* Cause an interrupt after executing one instruction. */
		ctr = 1;
	}

	/* The counter value is one less than the init value: init value should
	 * indicate how many instructions are executed before interrupt.  To
	 * execute 1 instruction (after interrupt handler return), counter must
	 * be 0.
	 */
	DUK_ASSERT(ctr >= 1);
	thr->interrupt_init = ctr;
	thr->interrupt_counter = ctr - 1;
	DUK_HEAP_CLEAR_INTERRUPT_RUNNING(thr->heap);

	return retval;
}
#endif /* DUK_USE_INTERRUPT_COUNTER */

/*
 *  Debugger handling for executor restart
 *
 *  Check for breakpoints, stepping, etc, and figure out if we should execute
 *  in checked or normal mode.  Note that we can't do this when an activation
 *  is created, because breakpoint status (and stepping status) may change
 *  later, so we must recheck every time we're executing an activation.
 *  This primitive should be side effect free to avoid changes during check.
 */

#if defined(DUK_USE_DEBUGGER_SUPPORT)
DUK_LOCAL void duk__executor_recheck_debugger(duk_hthread *thr, duk_activation *act, duk_hcompfunc *fun) {
	duk_heap *heap;
	duk_tval *tv_tmp;
	duk_hstring *filename;
	duk_small_uint_t bp_idx;
	duk_breakpoint **bp_active;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(act != NULL);
	DUK_ASSERT(fun != NULL);

	heap = thr->heap;
	bp_active = heap->dbg_breakpoints_active;
	act->flags &= ~DUK_ACT_FLAG_BREAKPOINT_ACTIVE;

	tv_tmp = duk_hobject_find_entry_tval_ptr_stridx(thr->heap, (duk_hobject *) fun, DUK_STRIDX_FILE_NAME);
	if (tv_tmp && DUK_TVAL_IS_STRING(tv_tmp)) {
		filename = DUK_TVAL_GET_STRING(tv_tmp);

		/* Figure out all active breakpoints.  A breakpoint is
		 * considered active if the current function's fileName
		 * matches the breakpoint's fileName, AND there is no
		 * inner function that has matching line numbers
		 * (otherwise a breakpoint would be triggered both
		 * inside and outside of the inner function which would
		 * be confusing).  Example:
		 *
		 *     function foo() {
		 *         print('foo');
		 *         function bar() {    <-.  breakpoints in these
		 *             print('bar');     |  lines should not affect
		 *         }                   <-'  foo() execution
		 *         bar();
		 *     }
		 *
		 * We need a few things that are only available when
		 * debugger support is enabled: (1) a line range for
		 * each function, and (2) access to the function
		 * template to access the inner functions (and their
		 * line ranges).
		 *
		 * It's important to have a narrow match for active
		 * breakpoints so that we don't enter checked execution
		 * when that's not necessary.  For instance, if we're
		 * running inside a certain function and there's
		 * breakpoint outside in (after the call site), we
		 * don't want to slow down execution of the function.
		 */

		for (bp_idx = 0; bp_idx < heap->dbg_breakpoint_count; bp_idx++) {
			duk_breakpoint *bp = heap->dbg_breakpoints + bp_idx;
			duk_hobject **funcs, **funcs_end;
			duk_hcompfunc *inner_fun;
			duk_bool_t bp_match;

			if (bp->filename == filename && bp->line >= fun->start_line && bp->line <= fun->end_line) {
				bp_match = 1;
				DUK_DD(DUK_DDPRINT("breakpoint filename and line match: "
				                   "%s:%ld vs. %s (line %ld vs. %ld-%ld)",
				                   DUK_HSTRING_GET_DATA(bp->filename),
				                   (long) bp->line,
				                   DUK_HSTRING_GET_DATA(filename),
				                   (long) bp->line,
				                   (long) fun->start_line,
				                   (long) fun->end_line));

				funcs = DUK_HCOMPFUNC_GET_FUNCS_BASE(thr->heap, fun);
				funcs_end = DUK_HCOMPFUNC_GET_FUNCS_END(thr->heap, fun);
				while (funcs != funcs_end) {
					inner_fun = (duk_hcompfunc *) *funcs;
					DUK_ASSERT(DUK_HOBJECT_IS_COMPFUNC((duk_hobject *) inner_fun));
					if (bp->line >= inner_fun->start_line && bp->line <= inner_fun->end_line) {
						DUK_DD(DUK_DDPRINT("inner function masks ('captures') breakpoint"));
						bp_match = 0;
						break;
					}
					funcs++;
				}

				if (bp_match) {
					/* No need to check for size of bp_active list,
					 * it's always larger than maximum number of
					 * breakpoints.
					 */
					act->flags |= DUK_ACT_FLAG_BREAKPOINT_ACTIVE;
					*bp_active = heap->dbg_breakpoints + bp_idx;
					bp_active++;
				}
			}
		}
	}

	*bp_active = NULL; /* terminate */

	DUK_DD(DUK_DDPRINT("ACTIVE BREAKPOINTS: %ld", (long) (bp_active - thr->heap->dbg_breakpoints_active)));

	/* Force pause if we were doing "step into" in another activation. */
	if ((thr->heap->dbg_pause_flags & DUK_PAUSE_FLAG_FUNC_ENTRY) && thr->heap->dbg_pause_act != thr->callstack_curr) {
		DUK_D(DUK_DPRINT("PAUSE TRIGGERED by function entry"));
		duk_debug_set_paused(thr->heap);
	}

	/* Force interrupt right away if we're paused or in "checked mode".
	 * Step out is handled by callstack unwind.
	 */
	if ((act->flags & DUK_ACT_FLAG_BREAKPOINT_ACTIVE) || DUK_HEAP_HAS_DEBUGGER_PAUSED(thr->heap) ||
	    ((thr->heap->dbg_pause_flags & DUK_PAUSE_FLAG_LINE_CHANGE) && thr->heap->dbg_pause_act == thr->callstack_curr)) {
		/* We'll need to interrupt early so recompute the init
		 * counter to reflect the number of bytecode instructions
		 * executed so that step counts for e.g. debugger rate
		 * limiting are accurate.
		 */
		DUK_ASSERT(thr->interrupt_counter <= thr->interrupt_init);
		thr->interrupt_init = thr->interrupt_init - thr->interrupt_counter;
		thr->interrupt_counter = 0;
	}
}
#endif /* DUK_USE_DEBUGGER_SUPPORT */

/*
 *  Opcode handlers for opcodes with a lot of code and which are relatively
 *  rare; NOINLINE to reduce amount of code in main bytecode dispatcher.
 */

DUK_LOCAL DUK_EXEC_NOINLINE_PERF void duk__handle_op_initset_initget(duk_hthread *thr, duk_uint_fast32_t ins) {
	duk_bool_t is_set = (DUK_DEC_OP(ins) == DUK_OP_INITSET);
	duk_uint_fast_t idx;
	duk_uint_t defprop_flags;

	/* A -> object register (acts as a source)
	 * BC -> BC+0 contains key, BC+1 closure (value)
	 */

	/* INITSET/INITGET are only used to initialize object literal keys.
	 * There may be a previous propery in ES2015 because duplicate property
	 * names are allowed.
	 */

	/* This could be made more optimal by accessing internals directly. */

	idx = (duk_uint_fast_t) DUK_DEC_BC(ins);
	duk_dup(thr, (duk_idx_t) (idx + 0)); /* key */
	duk_dup(thr, (duk_idx_t) (idx + 1)); /* getter/setter */
	if (is_set) {
		defprop_flags =
		    DUK_DEFPROP_HAVE_SETTER | DUK_DEFPROP_FORCE | DUK_DEFPROP_SET_ENUMERABLE | DUK_DEFPROP_SET_CONFIGURABLE;
	} else {
		defprop_flags =
		    DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_FORCE | DUK_DEFPROP_SET_ENUMERABLE | DUK_DEFPROP_SET_CONFIGURABLE;
	}
	duk_def_prop(thr, (duk_idx_t) DUK_DEC_A(ins), defprop_flags);
}

DUK_LOCAL DUK_EXEC_NOINLINE_PERF void duk__handle_op_trycatch(duk_hthread *thr, duk_uint_fast32_t ins, duk_instr_t *curr_pc) {
	duk_activation *act;
	duk_catcher *cat;
	duk_tval *tv1;
	duk_small_uint_fast_t a;
	duk_small_uint_fast_t bc;

	/* A -> flags
	 * BC -> reg_catch; base register for two registers used both during
	 *       trycatch setup and when catch is triggered
	 *
	 *      If DUK_BC_TRYCATCH_FLAG_CATCH_BINDING set:
	 *          reg_catch + 0: catch binding variable name (string).
	 *          Automatic declarative environment is established for
	 *          the duration of the 'catch' clause.
	 *
	 *      If DUK_BC_TRYCATCH_FLAG_WITH_BINDING set:
	 *          reg_catch + 0: with 'target value', which is coerced to
	 *          an object and then used as a bindind object for an
	 *          environment record.  The binding is initialized here, for
	 *          the 'try' clause.
	 *
	 * Note that a TRYCATCH generated for a 'with' statement has no
	 * catch or finally parts.
	 */

	/* XXX: TRYCATCH handling should be reworked to avoid creating
	 * an explicit scope unless it is actually needed (e.g. function
	 * instances or eval is executed inside the catch block).  This
	 * rework is not trivial because the compiler doesn't have an
	 * intermediate representation.  When the rework is done, the
	 * opcode format can also be made more straightforward.
	 */

	/* XXX: side effect handling is quite awkward here */

	DUK_DDD(DUK_DDDPRINT("TRYCATCH: reg_catch=%ld, have_catch=%ld, "
	                     "have_finally=%ld, catch_binding=%ld, with_binding=%ld (flags=0x%02lx)",
	                     (long) DUK_DEC_BC(ins),
	                     (long) (DUK_DEC_A(ins) & DUK_BC_TRYCATCH_FLAG_HAVE_CATCH ? 1 : 0),
	                     (long) (DUK_DEC_A(ins) & DUK_BC_TRYCATCH_FLAG_HAVE_FINALLY ? 1 : 0),
	                     (long) (DUK_DEC_A(ins) & DUK_BC_TRYCATCH_FLAG_CATCH_BINDING ? 1 : 0),
	                     (long) (DUK_DEC_A(ins) & DUK_BC_TRYCATCH_FLAG_WITH_BINDING ? 1 : 0),
	                     (unsigned long) DUK_DEC_A(ins)));

	a = DUK_DEC_A(ins);
	bc = DUK_DEC_BC(ins);

	/* Registers 'bc' and 'bc + 1' are written in longjmp handling
	 * and if their previous values (which are temporaries) become
	 * unreachable -and- have a finalizer, there'll be a function
	 * call during error handling which is not supported now (GH-287).
	 * Ensure that both 'bc' and 'bc + 1' have primitive values to
	 * guarantee no finalizer calls in error handling.  Scrubbing also
	 * ensures finalizers for the previous values run here rather than
	 * later.  Error handling related values are also written to 'bc'
	 * and 'bc + 1' but those values never become unreachable during
	 * error handling, so there's no side effect problem even if the
	 * error value has a finalizer.
	 */
	duk_dup(thr, (duk_idx_t) bc); /* Stabilize value. */
	duk_to_undefined(thr, (duk_idx_t) bc);
	duk_to_undefined(thr, (duk_idx_t) (bc + 1));

	/* Allocate catcher and populate it.  Doesn't have to
	 * be fully atomic, but the catcher must be in a
	 * consistent state if side effects (such as finalizer
	 * calls) occur.
	 */

	cat = duk_hthread_catcher_alloc(thr);
	DUK_ASSERT(cat != NULL);

	cat->flags = DUK_CAT_TYPE_TCF;
	cat->h_varname = NULL;
	cat->pc_base = (duk_instr_t *) curr_pc; /* pre-incremented, points to first jump slot */
	cat->idx_base = (duk_size_t) (thr->valstack_bottom - thr->valstack) + bc;

	act = thr->callstack_curr;
	DUK_ASSERT(act != NULL);
	cat->parent = act->cat;
	act->cat = cat;

	if (a & DUK_BC_TRYCATCH_FLAG_HAVE_CATCH) {
		cat->flags |= DUK_CAT_FLAG_CATCH_ENABLED;
	}
	if (a & DUK_BC_TRYCATCH_FLAG_HAVE_FINALLY) {
		cat->flags |= DUK_CAT_FLAG_FINALLY_ENABLED;
	}
	if (a & DUK_BC_TRYCATCH_FLAG_CATCH_BINDING) {
		DUK_DDD(DUK_DDDPRINT("catch binding flag set to catcher"));
		cat->flags |= DUK_CAT_FLAG_CATCH_BINDING_ENABLED;
		tv1 = DUK_GET_TVAL_NEGIDX(thr, -1);
		DUK_ASSERT(DUK_TVAL_IS_STRING(tv1));

		/* borrowed reference; although 'tv1' comes from a register,
		 * its value was loaded using LDCONST so the constant will
		 * also exist and be reachable.
		 */
		cat->h_varname = DUK_TVAL_GET_STRING(tv1);
	} else if (a & DUK_BC_TRYCATCH_FLAG_WITH_BINDING) {
		duk_hobjenv *env;
		duk_hobject *target;

		/* Delayed env initialization for activation (if needed). */
		DUK_ASSERT(thr->callstack_top >= 1);
		DUK_ASSERT(act == thr->callstack_curr);
		DUK_ASSERT(act != NULL);
		if (act->lex_env == NULL) {
			DUK_DDD(DUK_DDDPRINT("delayed environment initialization"));
			DUK_ASSERT(act->var_env == NULL);

			duk_js_init_activation_environment_records_delayed(thr, act);
			DUK_ASSERT(act == thr->callstack_curr);
			DUK_UNREF(act); /* 'act' is no longer accessed, scanbuild fix */
		}
		DUK_ASSERT(act->lex_env != NULL);
		DUK_ASSERT(act->var_env != NULL);

		/* Coerce 'with' target. */
		target = duk_to_hobject(thr, -1);
		DUK_ASSERT(target != NULL);

		/* Create an object environment; it is not pushed
		 * so avoid side effects very carefully until it is
		 * referenced.
		 */
		env = duk_hobjenv_alloc(thr, DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_OBJENV));
		DUK_ASSERT(env != NULL);
		DUK_ASSERT(DUK_HOBJECT_GET_PROTOTYPE(thr->heap, (duk_hobject *) env) == NULL);
		env->target = target; /* always provideThis=true */
		DUK_HOBJECT_INCREF(thr, target);
		env->has_this = 1;
		DUK_HOBJENV_ASSERT_VALID(env);
		DUK_DDD(DUK_DDDPRINT("environment for with binding: %!iO", env));

		DUK_ASSERT(act == thr->callstack_curr);
		DUK_ASSERT(DUK_HOBJECT_GET_PROTOTYPE(thr->heap, (duk_hobject *) env) == NULL);
		DUK_ASSERT(act->lex_env != NULL);
		DUK_HOBJECT_SET_PROTOTYPE(thr->heap, (duk_hobject *) env, act->lex_env);
		act->lex_env = (duk_hobject *) env; /* Now reachable. */
		DUK_HOBJECT_INCREF(thr, (duk_hobject *) env);
		/* Net refcount change to act->lex_env is 0: incref for env's
		 * prototype, decref for act->lex_env overwrite.
		 */

		/* Set catcher lex_env active (affects unwind)
		 * only when the whole setup is complete.
		 */
		cat = act->cat; /* XXX: better to relookup? not mandatory because 'cat' is stable */
		cat->flags |= DUK_CAT_FLAG_LEXENV_ACTIVE;
	} else {
		;
	}

	DUK_DDD(DUK_DDDPRINT("TRYCATCH catcher: flags=0x%08lx, pc_base=%ld, "
	                     "idx_base=%ld, h_varname=%!O",
	                     (unsigned long) cat->flags,
	                     (long) cat->pc_base,
	                     (long) cat->idx_base,
	                     (duk_heaphdr *) cat->h_varname));

	duk_pop_unsafe(thr);
}

DUK_LOCAL DUK_EXEC_NOINLINE_PERF duk_instr_t *duk__handle_op_endtry(duk_hthread *thr, duk_uint_fast32_t ins) {
	duk_activation *act;
	duk_catcher *cat;
	duk_tval *tv1;
	duk_instr_t *pc_base;

	DUK_UNREF(ins);

	DUK_ASSERT(thr->callstack_top >= 1);
	act = thr->callstack_curr;
	DUK_ASSERT(act != NULL);
	cat = act->cat;
	DUK_ASSERT(cat != NULL);
	DUK_ASSERT(DUK_CAT_GET_TYPE(act->cat) == DUK_CAT_TYPE_TCF);

	DUK_DDD(DUK_DDDPRINT("ENDTRY: clearing catch active flag (regardless of whether it was set or not)"));
	DUK_CAT_CLEAR_CATCH_ENABLED(cat);

	pc_base = cat->pc_base;

	if (DUK_CAT_HAS_FINALLY_ENABLED(cat)) {
		DUK_DDD(DUK_DDDPRINT("ENDTRY: finally part is active, jump through 2nd jump slot with 'normal continuation'"));

		tv1 = thr->valstack + cat->idx_base;
		DUK_ASSERT(tv1 >= thr->valstack && tv1 < thr->valstack_top);
		DUK_TVAL_SET_UNDEFINED_UPDREF(thr, tv1); /* side effects */
		tv1 = NULL;

		tv1 = thr->valstack + cat->idx_base + 1;
		DUK_ASSERT(tv1 >= thr->valstack && tv1 < thr->valstack_top);
		DUK_TVAL_SET_U32_UPDREF(thr, tv1, (duk_uint32_t) DUK_LJ_TYPE_NORMAL); /* side effects */
		tv1 = NULL;

		DUK_CAT_CLEAR_FINALLY_ENABLED(cat);
	} else {
		DUK_DDD(
		    DUK_DDDPRINT("ENDTRY: no finally part, dismantle catcher, jump through 2nd jump slot (to end of statement)"));

		duk_hthread_catcher_unwind_norz(thr, act); /* lexenv may be set for 'with' binding */
		/* no need to unwind callstack */
	}

	return pc_base + 1; /* new curr_pc value */
}

DUK_LOCAL DUK_EXEC_NOINLINE_PERF duk_instr_t *duk__handle_op_endcatch(duk_hthread *thr, duk_uint_fast32_t ins) {
	duk_activation *act;
	duk_catcher *cat;
	duk_tval *tv1;
	duk_instr_t *pc_base;

	DUK_UNREF(ins);

	DUK_ASSERT(thr->callstack_top >= 1);
	act = thr->callstack_curr;
	DUK_ASSERT(act != NULL);
	cat = act->cat;
	DUK_ASSERT(cat != NULL);
	DUK_ASSERT(!DUK_CAT_HAS_CATCH_ENABLED(cat)); /* cleared before entering catch part */

	if (DUK_CAT_HAS_LEXENV_ACTIVE(cat)) {
		duk_hobject *prev_env;

		/* 'with' binding has no catch clause, so can't be here unless a normal try-catch */
		DUK_ASSERT(DUK_CAT_HAS_CATCH_BINDING_ENABLED(cat));
		DUK_ASSERT(act->lex_env != NULL);

		DUK_DDD(DUK_DDDPRINT("ENDCATCH: popping catcher part lexical environment"));

		prev_env = act->lex_env;
		DUK_ASSERT(prev_env != NULL);
		act->lex_env = DUK_HOBJECT_GET_PROTOTYPE(thr->heap, prev_env);
		DUK_CAT_CLEAR_LEXENV_ACTIVE(cat);
		DUK_HOBJECT_INCREF(thr, act->lex_env);
		DUK_HOBJECT_DECREF(thr, prev_env); /* side effects */

		DUK_ASSERT(act == thr->callstack_curr);
		DUK_ASSERT(act != NULL);
	}

	pc_base = cat->pc_base;

	if (DUK_CAT_HAS_FINALLY_ENABLED(cat)) {
		DUK_DDD(DUK_DDDPRINT("ENDCATCH: finally part is active, jump through 2nd jump slot with 'normal continuation'"));

		tv1 = thr->valstack + cat->idx_base;
		DUK_ASSERT(tv1 >= thr->valstack && tv1 < thr->valstack_top);
		DUK_TVAL_SET_UNDEFINED_UPDREF(thr, tv1); /* side effects */
		tv1 = NULL;

		tv1 = thr->valstack + cat->idx_base + 1;
		DUK_ASSERT(tv1 >= thr->valstack && tv1 < thr->valstack_top);
		DUK_TVAL_SET_U32_UPDREF(thr, tv1, (duk_uint32_t) DUK_LJ_TYPE_NORMAL); /* side effects */
		tv1 = NULL;

		DUK_CAT_CLEAR_FINALLY_ENABLED(cat);
	} else {
		DUK_DDD(
		    DUK_DDDPRINT("ENDCATCH: no finally part, dismantle catcher, jump through 2nd jump slot (to end of statement)"));

		duk_hthread_catcher_unwind_norz(thr, act);
		/* no need to unwind callstack */
	}

	return pc_base + 1; /* new curr_pc value */
}

DUK_LOCAL DUK_EXEC_NOINLINE_PERF duk_small_uint_t duk__handle_op_endfin(duk_hthread *thr,
                                                                        duk_uint_fast32_t ins,
                                                                        duk_activation *entry_act) {
	duk_activation *act;
	duk_tval *tv1;
	duk_uint_t reg_catch;
	duk_small_uint_t cont_type;
	duk_small_uint_t ret_result;

	DUK_ASSERT(thr->ptr_curr_pc == NULL);
	DUK_ASSERT(thr->callstack_top >= 1);
	act = thr->callstack_curr;
	DUK_ASSERT(act != NULL);
	reg_catch = DUK_DEC_ABC(ins);

	/* CATCH flag may be enabled or disabled here; it may be enabled if
	 * the statement has a catch block but the try block does not throw
	 * an error.
	 */

	DUK_DDD(DUK_DDDPRINT("ENDFIN: completion value=%!T, type=%!T",
	                     (duk_tval *) (thr->valstack_bottom + reg_catch + 0),
	                     (duk_tval *) (thr->valstack_bottom + reg_catch + 1)));

	tv1 = thr->valstack_bottom + reg_catch + 1; /* type */
	DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv1));
#if defined(DUK_USE_FASTINT)
	DUK_ASSERT(DUK_TVAL_IS_FASTINT(tv1));
	cont_type = (duk_small_uint_t) DUK_TVAL_GET_FASTINT_U32(tv1);
#else
	cont_type = (duk_small_uint_t) DUK_TVAL_GET_NUMBER(tv1);
#endif

	tv1--; /* value */

	switch (cont_type) {
	case DUK_LJ_TYPE_NORMAL: {
		DUK_DDD(DUK_DDDPRINT("ENDFIN: finally part finishing with 'normal' (non-abrupt) completion -> "
		                     "dismantle catcher, resume execution after ENDFIN"));

		duk_hthread_catcher_unwind_norz(thr, act);
		/* no need to unwind callstack */
		return 0; /* restart execution */
	}
	case DUK_LJ_TYPE_RETURN: {
		DUK_DDD(DUK_DDDPRINT("ENDFIN: finally part finishing with 'return' complation -> dismantle "
		                     "catcher, handle return, lj.value1=%!T",
		                     tv1));

		/* Not necessary to unwind catch stack: return handling will
		 * do it.  The finally flag of 'cat' is no longer set.  The
		 * catch flag may be set, but it's not checked by return handling.
		 */

		duk_push_tval(thr, tv1);
		ret_result = duk__handle_return(thr, entry_act);
		if (ret_result == DUK__RETHAND_RESTART) {
			return 0; /* restart execution */
		}
		DUK_ASSERT(ret_result == DUK__RETHAND_FINISHED);

		DUK_DDD(DUK_DDDPRINT("exiting executor after ENDFIN and RETURN (pseudo) longjmp type"));
		return 1; /* exit executor */
	}
	case DUK_LJ_TYPE_BREAK:
	case DUK_LJ_TYPE_CONTINUE: {
		duk_uint_t label_id;
		duk_small_uint_t lj_type;

		/* Not necessary to unwind catch stack: break/continue
		 * handling will do it.  The finally flag of 'cat' is
		 * no longer set.  The catch flag may be set, but it's
		 * not checked by break/continue handling.
		 */

		DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv1));
#if defined(DUK_USE_FASTINT)
		DUK_ASSERT(DUK_TVAL_IS_FASTINT(tv1));
		label_id = (duk_small_uint_t) DUK_TVAL_GET_FASTINT_U32(tv1);
#else
		label_id = (duk_small_uint_t) DUK_TVAL_GET_NUMBER(tv1);
#endif
		lj_type = cont_type;
		duk__handle_break_or_continue(thr, label_id, lj_type);
		return 0; /* restart execution */
	}
	default: {
		DUK_DDD(DUK_DDDPRINT("ENDFIN: finally part finishing with abrupt completion, lj_type=%ld -> "
		                     "dismantle catcher, re-throw error",
		                     (long) cont_type));

		duk_err_setup_ljstate1(thr, (duk_small_uint_t) cont_type, tv1);
		/* No debugger Throw notify check on purpose (rethrow). */

		DUK_ASSERT(thr->heap->lj.jmpbuf_ptr != NULL); /* always in executor */
		duk_err_longjmp(thr);
		DUK_UNREACHABLE();
	}
	}

	DUK_UNREACHABLE();
	return 0;
}

DUK_LOCAL DUK_EXEC_NOINLINE_PERF void duk__handle_op_initenum(duk_hthread *thr, duk_uint_fast32_t ins) {
	duk_small_uint_t b;
	duk_small_uint_t c;

	/*
	 *  Enumeration semantics come from for-in statement, E5 Section 12.6.4.
	 *  If called with 'null' or 'undefined', this opcode returns 'null' as
	 *  the enumerator, which is special cased in NEXTENUM.  This simplifies
	 *  the compiler part
	 */

	/* B -> register for writing enumerator object
	 * C -> value to be enumerated (register)
	 */
	b = DUK_DEC_B(ins);
	c = DUK_DEC_C(ins);

	if (duk_is_null_or_undefined(thr, (duk_idx_t) c)) {
		duk_push_null(thr);
		duk_replace(thr, (duk_idx_t) b);
	} else {
		duk_dup(thr, (duk_idx_t) c);
		duk_to_object(thr, -1);
		duk_hobject_enumerator_create(thr, 0 /*enum_flags*/); /* [ ... val ] --> [ ... enum ] */
		duk_replace(thr, (duk_idx_t) b);
	}
}

DUK_LOCAL DUK_EXEC_NOINLINE_PERF duk_small_uint_t duk__handle_op_nextenum(duk_hthread *thr, duk_uint_fast32_t ins) {
	duk_small_uint_t b;
	duk_small_uint_t c;
	duk_small_uint_t pc_skip = 0;

	/*
	 *  NEXTENUM checks whether the enumerator still has unenumerated
	 *  keys.  If so, the next key is loaded to the target register
	 *  and the next instruction is skipped.  Otherwise the next instruction
	 *  will be executed, jumping out of the enumeration loop.
	 */

	/* B -> target register for next key
	 * C -> enum register
	 */
	b = DUK_DEC_B(ins);
	c = DUK_DEC_C(ins);

	DUK_DDD(DUK_DDDPRINT("NEXTENUM: b->%!T, c->%!T",
	                     (duk_tval *) duk_get_tval(thr, (duk_idx_t) b),
	                     (duk_tval *) duk_get_tval(thr, (duk_idx_t) c)));

	if (duk_is_object(thr, (duk_idx_t) c)) {
		/* XXX: assert 'c' is an enumerator */
		duk_dup(thr, (duk_idx_t) c);
		if (duk_hobject_enumerator_next(thr, 0 /*get_value*/)) {
			/* [ ... enum ] -> [ ... next_key ] */
			DUK_DDD(DUK_DDDPRINT("enum active, next key is %!T, skip jump slot ", (duk_tval *) duk_get_tval(thr, -1)));
			pc_skip = 1;
		} else {
			/* [ ... enum ] -> [ ... ] */
			DUK_DDD(DUK_DDDPRINT("enum finished, execute jump slot"));
			DUK_ASSERT(DUK_TVAL_IS_UNDEFINED(thr->valstack_top)); /* valstack policy */
			thr->valstack_top++;
		}
		duk_replace(thr, (duk_idx_t) b);
	} else {
		/* 'null' enumerator case -> behave as with an empty enumerator */
		DUK_ASSERT(duk_is_null(thr, (duk_idx_t) c));
		DUK_DDD(DUK_DDDPRINT("enum is null, execute jump slot"));
	}

	return pc_skip;
}

/*
 *  Call handling helpers.
 */

DUK_LOCAL duk_bool_t duk__executor_handle_call(duk_hthread *thr, duk_idx_t idx, duk_idx_t nargs, duk_small_uint_t call_flags) {
	duk_bool_t rc;

	duk_set_top_unsafe(thr, (duk_idx_t) (idx + nargs + 2)); /* [ ... func this arg1 ... argN ] */

	/* Attempt an Ecma-to-Ecma call setup.  If the call
	 * target is (directly or indirectly) Reflect.construct(),
	 * the call may change into a constructor call on the fly.
	 */
	rc = (duk_bool_t) duk_handle_call_unprotected(thr, idx, call_flags);
	if (rc != 0) {
		/* Ecma-to-ecma call possible, may or may not
		 * be a tail call.  Avoid C recursion by
		 * reusing current executor instance.
		 */
		DUK_DDD(DUK_DDDPRINT("ecma-to-ecma call setup possible, restart execution"));
		/* curr_pc synced by duk_handle_call_unprotected() */
		DUK_ASSERT(thr->ptr_curr_pc == NULL);
		return rc;
	} else {
		/* Call was handled inline. */
	}
	DUK_ASSERT(thr->ptr_curr_pc != NULL);
	return rc;
}

/*
 *  ECMAScript bytecode executor.
 *
 *  Resume execution for the current thread from its current activation.
 *  Returns when execution would return from the entry level activation,
 *  leaving a single return value on top of the stack.  Function calls
 *  and thread resumptions are handled internally.  If an error occurs,
 *  a longjmp() with type DUK_LJ_TYPE_THROW is called on the entry level
 *  setjmp() jmpbuf.
 *
 *  ECMAScript function calls and coroutine resumptions are handled
 *  internally (by the outer executor function) without recursive C calls.
 *  Other function calls are handled using duk_handle_call(), increasing
 *  C recursion depth.
 *
 *  Abrupt completions (= long control tranfers) are handled either
 *  directly by reconfiguring relevant stacks and restarting execution,
 *  or via a longjmp.  Longjmp-free handling is preferable for performance
 *  (especially Emscripten performance), and is used for: break, continue,
 *  and return.
 *
 *  For more detailed notes, see doc/execution.rst.
 *
 *  Also see doc/code-issues.rst for discussion of setjmp(), longjmp(),
 *  and volatile.
 */

/* Presence of 'fun' is config based, there's a marginal performance
 * difference and the best option is architecture dependent.
 */
#if defined(DUK_USE_EXEC_FUN_LOCAL)
#define DUK__FUN() fun
#else
#define DUK__FUN() ((duk_hcompfunc *) DUK_ACT_GET_FUNC((thr)->callstack_curr))
#endif

/* Strict flag. */
#define DUK__STRICT() ((duk_small_uint_t) DUK_HOBJECT_HAS_STRICT((duk_hobject *) DUK__FUN()))

/* Reg/const access macros: these are very footprint and performance sensitive
 * so modify with care.  Arguments are sometimes evaluated multiple times which
 * is not ideal.
 */
#define DUK__REG(x)    (*(thr->valstack_bottom + (x)))
#define DUK__REGP(x)   (thr->valstack_bottom + (x))
#define DUK__CONST(x)  (*(consts + (x)))
#define DUK__CONSTP(x) (consts + (x))

/* Reg/const access macros which take the 32-bit instruction and avoid an
 * explicit field decoding step by using shifts and masks.  These must be
 * kept in sync with duk_js_bytecode.h.  The shift/mask values are chosen
 * so that 'ins' can be shifted and masked and used as a -byte- offset
 * instead of a duk_tval offset which needs further shifting (which is an
 * issue on some, but not all, CPUs).
 */
#define DUK__RCBIT_B DUK_BC_REGCONST_B
#define DUK__RCBIT_C DUK_BC_REGCONST_C
#if defined(DUK_USE_EXEC_REGCONST_OPTIMIZE)
#if defined(DUK_USE_PACKED_TVAL)
#define DUK__TVAL_SHIFT 3 /* sizeof(duk_tval) == 8 */
#else
#define DUK__TVAL_SHIFT 4 /* sizeof(duk_tval) == 16; not always the case so also asserted for */
#endif
#define DUK__SHIFT_A         (DUK_BC_SHIFT_A - DUK__TVAL_SHIFT)
#define DUK__SHIFT_B         (DUK_BC_SHIFT_B - DUK__TVAL_SHIFT)
#define DUK__SHIFT_C         (DUK_BC_SHIFT_C - DUK__TVAL_SHIFT)
#define DUK__SHIFT_BC        (DUK_BC_SHIFT_BC - DUK__TVAL_SHIFT)
#define DUK__MASK_A          (DUK_BC_UNSHIFTED_MASK_A << DUK__TVAL_SHIFT)
#define DUK__MASK_B          (DUK_BC_UNSHIFTED_MASK_B << DUK__TVAL_SHIFT)
#define DUK__MASK_C          (DUK_BC_UNSHIFTED_MASK_C << DUK__TVAL_SHIFT)
#define DUK__MASK_BC         (DUK_BC_UNSHIFTED_MASK_BC << DUK__TVAL_SHIFT)
#define DUK__BYTEOFF_A(ins)  (((ins) >> DUK__SHIFT_A) & DUK__MASK_A)
#define DUK__BYTEOFF_B(ins)  (((ins) >> DUK__SHIFT_B) & DUK__MASK_B)
#define DUK__BYTEOFF_C(ins)  (((ins) >> DUK__SHIFT_C) & DUK__MASK_C)
#define DUK__BYTEOFF_BC(ins) (((ins) >> DUK__SHIFT_BC) & DUK__MASK_BC)

#define DUK__REGP_A(ins)    ((duk_tval *) (void *) ((duk_uint8_t *) thr->valstack_bottom + DUK__BYTEOFF_A((ins))))
#define DUK__REGP_B(ins)    ((duk_tval *) (void *) ((duk_uint8_t *) thr->valstack_bottom + DUK__BYTEOFF_B((ins))))
#define DUK__REGP_C(ins)    ((duk_tval *) (void *) ((duk_uint8_t *) thr->valstack_bottom + DUK__BYTEOFF_C((ins))))
#define DUK__REGP_BC(ins)   ((duk_tval *) (void *) ((duk_uint8_t *) thr->valstack_bottom + DUK__BYTEOFF_BC((ins))))
#define DUK__CONSTP_A(ins)  ((duk_tval *) (void *) ((duk_uint8_t *) consts + DUK__BYTEOFF_A((ins))))
#define DUK__CONSTP_B(ins)  ((duk_tval *) (void *) ((duk_uint8_t *) consts + DUK__BYTEOFF_B((ins))))
#define DUK__CONSTP_C(ins)  ((duk_tval *) (void *) ((duk_uint8_t *) consts + DUK__BYTEOFF_C((ins))))
#define DUK__CONSTP_BC(ins) ((duk_tval *) (void *) ((duk_uint8_t *) consts + DUK__BYTEOFF_BC((ins))))
#define DUK__REGCONSTP_B(ins) \
	((duk_tval *) (void *) ((duk_uint8_t *) (((ins) &DUK__RCBIT_B) ? consts : thr->valstack_bottom) + DUK__BYTEOFF_B((ins))))
#define DUK__REGCONSTP_C(ins) \
	((duk_tval *) (void *) ((duk_uint8_t *) (((ins) &DUK__RCBIT_C) ? consts : thr->valstack_bottom) + DUK__BYTEOFF_C((ins))))
#else /* DUK_USE_EXEC_REGCONST_OPTIMIZE */
/* Safe alternatives, no assumption about duk_tval size. */
#define DUK__REGP_A(ins)      DUK__REGP(DUK_DEC_A((ins)))
#define DUK__REGP_B(ins)      DUK__REGP(DUK_DEC_B((ins)))
#define DUK__REGP_C(ins)      DUK__REGP(DUK_DEC_C((ins)))
#define DUK__REGP_BC(ins)     DUK__REGP(DUK_DEC_BC((ins)))
#define DUK__CONSTP_A(ins)    DUK__CONSTP(DUK_DEC_A((ins)))
#define DUK__CONSTP_B(ins)    DUK__CONSTP(DUK_DEC_B((ins)))
#define DUK__CONSTP_C(ins)    DUK__CONSTP(DUK_DEC_C((ins)))
#define DUK__CONSTP_BC(ins)   DUK__CONSTP(DUK_DEC_BC((ins)))
#define DUK__REGCONSTP_B(ins) ((((ins) &DUK__RCBIT_B) ? consts : thr->valstack_bottom) + DUK_DEC_B((ins)))
#define DUK__REGCONSTP_C(ins) ((((ins) &DUK__RCBIT_C) ? consts : thr->valstack_bottom) + DUK_DEC_C((ins)))
#endif /* DUK_USE_EXEC_REGCONST_OPTIMIZE */

#if defined(DUK_USE_VERBOSE_EXECUTOR_ERRORS)
#define DUK__INTERNAL_ERROR(msg) \
	do { \
		DUK_ERROR_ERROR(thr, (msg)); \
		DUK_WO_NORETURN(return;); \
	} while (0)
#else
#define DUK__INTERNAL_ERROR(msg) \
	do { \
		goto internal_error; \
	} while (0)
#endif

#define DUK__SYNC_CURR_PC() \
	do { \
		duk_activation *duk__act; \
		duk__act = thr->callstack_curr; \
		duk__act->curr_pc = curr_pc; \
	} while (0)
#define DUK__SYNC_AND_NULL_CURR_PC() \
	do { \
		duk_activation *duk__act; \
		duk__act = thr->callstack_curr; \
		duk__act->curr_pc = curr_pc; \
		thr->ptr_curr_pc = NULL; \
	} while (0)

#if defined(DUK_USE_EXEC_PREFER_SIZE)
#define DUK__LOOKUP_INDIRECT(idx) \
	do { \
		(idx) = (duk_uint_fast_t) duk_get_uint(thr, (duk_idx_t) (idx)); \
	} while (0)
#elif defined(DUK_USE_FASTINT)
#define DUK__LOOKUP_INDIRECT(idx) \
	do { \
		duk_tval *tv_ind; \
		tv_ind = DUK__REGP((idx)); \
		DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv_ind)); \
		DUK_ASSERT(DUK_TVAL_IS_FASTINT(tv_ind)); /* compiler guarantees */ \
		(idx) = (duk_uint_fast_t) DUK_TVAL_GET_FASTINT_U32(tv_ind); \
	} while (0)
#else
#define DUK__LOOKUP_INDIRECT(idx) \
	do { \
		duk_tval *tv_ind; \
		tv_ind = DUK__REGP(idx); \
		DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv_ind)); \
		idx = (duk_uint_fast_t) DUK_TVAL_GET_NUMBER(tv_ind); \
	} while (0)
#endif

DUK_LOCAL void duk__handle_executor_error(duk_heap *heap,
                                          duk_activation *entry_act,
                                          duk_int_t entry_call_recursion_depth,
                                          duk_jmpbuf *entry_jmpbuf_ptr,
                                          volatile duk_bool_t *out_delayed_catch_setup) {
	duk_small_uint_t lj_ret;

	/* Longjmp callers are required to sync-and-null thr->ptr_curr_pc
	 * before longjmp.
	 */
	DUK_ASSERT(heap->curr_thread != NULL);
	DUK_ASSERT(heap->curr_thread->ptr_curr_pc == NULL);

	/* XXX: signalling the need to shrink check (only if unwound) */

	/* Must be restored here to handle e.g. yields properly. */
	heap->call_recursion_depth = entry_call_recursion_depth;

	/* Switch to caller's setjmp() catcher so that if an error occurs
	 * during error handling, it is always propagated outwards instead
	 * of causing an infinite loop in our own handler.
	 */
	heap->lj.jmpbuf_ptr = (duk_jmpbuf *) entry_jmpbuf_ptr;

	lj_ret = duk__handle_longjmp(heap->curr_thread, entry_act, out_delayed_catch_setup);

	/* Error handling complete, remove side effect protections.
	 */
#if defined(DUK_USE_ASSERTIONS)
	DUK_ASSERT(heap->error_not_allowed == 1);
	heap->error_not_allowed = 0;
#endif
	DUK_ASSERT(heap->pf_prevent_count > 0);
	heap->pf_prevent_count--;
	DUK_DD(DUK_DDPRINT("executor error handled, pf_prevent_count updated to %ld", (long) heap->pf_prevent_count));

	if (lj_ret == DUK__LONGJMP_RESTART) {
		/* Restart bytecode execution, possibly with a changed thread. */
		DUK_REFZERO_CHECK_SLOW(heap->curr_thread);
	} else {
		/* If an error is propagated, don't run refzero checks here.
		 * The next catcher will deal with that.  Pf_prevent_count
		 * will be re-bumped by the longjmp.
		 */

		DUK_ASSERT(lj_ret == DUK__LONGJMP_RETHROW); /* Rethrow error to calling state. */
		DUK_ASSERT(heap->lj.jmpbuf_ptr == entry_jmpbuf_ptr); /* Longjmp handling has restored jmpbuf_ptr. */

		/* Thread may have changed, e.g. YIELD converted to THROW. */
		duk_err_longjmp(heap->curr_thread);
		DUK_UNREACHABLE();
	}
}

/* Outer executor with setjmp/longjmp handling. */
DUK_INTERNAL void duk_js_execute_bytecode(duk_hthread *exec_thr) {
	/* Entry level info. */
	duk_hthread *entry_thread;
	duk_activation *entry_act;
	duk_int_t entry_call_recursion_depth;
	duk_jmpbuf *entry_jmpbuf_ptr;
	duk_jmpbuf our_jmpbuf;
	duk_heap *heap;
	volatile duk_bool_t delayed_catch_setup = 0;

	DUK_ASSERT(exec_thr != NULL);
	DUK_ASSERT(exec_thr->heap != NULL);
	DUK_ASSERT(exec_thr->heap->curr_thread != NULL);
	DUK_ASSERT_REFCOUNT_NONZERO_HEAPHDR((duk_heaphdr *) exec_thr);
	DUK_ASSERT(exec_thr->callstack_top >= 1); /* at least one activation, ours */
	DUK_ASSERT(exec_thr->callstack_curr != NULL);
	DUK_ASSERT(DUK_ACT_GET_FUNC(exec_thr->callstack_curr) != NULL);
	DUK_ASSERT(DUK_HOBJECT_IS_COMPFUNC(DUK_ACT_GET_FUNC(exec_thr->callstack_curr)));

	DUK_GC_TORTURE(exec_thr->heap);

	entry_thread = exec_thr;
	heap = entry_thread->heap;
	entry_act = entry_thread->callstack_curr;
	DUK_ASSERT(entry_act != NULL);
	entry_call_recursion_depth = entry_thread->heap->call_recursion_depth;
	entry_jmpbuf_ptr = entry_thread->heap->lj.jmpbuf_ptr;

	/*
	 *  Note: we currently assume that the setjmp() catchpoint is
	 *  not re-entrant (longjmp() cannot be called more than once
	 *  for a single setjmp()).
	 *
	 *  See doc/code-issues.rst for notes on variable assignment
	 *  before and after setjmp().
	 */

	for (;;) {
		heap->lj.jmpbuf_ptr = &our_jmpbuf;
		DUK_ASSERT(heap->lj.jmpbuf_ptr != NULL);

#if defined(DUK_USE_CPP_EXCEPTIONS)
		try {
#else
		DUK_ASSERT(heap->lj.jmpbuf_ptr == &our_jmpbuf);
		if (DUK_SETJMP(our_jmpbuf.jb) == 0) {
#endif
			DUK_DDD(DUK_DDDPRINT("after setjmp, delayed catch setup: %ld\n", (long) delayed_catch_setup));

			if (DUK_UNLIKELY(delayed_catch_setup != 0)) {
				duk_hthread *thr = entry_thread->heap->curr_thread;

				delayed_catch_setup = 0;
				duk__handle_catch_part2(thr);
				DUK_ASSERT(delayed_catch_setup == 0);
				DUK_DDD(DUK_DDDPRINT("top after delayed catch setup: %ld", (long) duk_get_top(entry_thread)));
			}

			/* Execute bytecode until returned or longjmp(). */
			duk__js_execute_bytecode_inner(entry_thread, entry_act);

			/* Successful return: restore jmpbuf and return to caller. */
			heap->lj.jmpbuf_ptr = entry_jmpbuf_ptr;

			return;
#if defined(DUK_USE_CPP_EXCEPTIONS)
		} catch (duk_internal_exception &exc) {
#else
		} else {
#endif
#if defined(DUK_USE_CPP_EXCEPTIONS)
			DUK_UNREF(exc);
#endif
			DUK_DDD(DUK_DDDPRINT("longjmp caught by bytecode executor"));
			DUK_STATS_INC(exec_thr->heap, stats_exec_throw);

			duk__handle_executor_error(heap,
			                           entry_act,
			                           entry_call_recursion_depth,
			                           entry_jmpbuf_ptr,
			                           &delayed_catch_setup);
		}
#if defined(DUK_USE_CPP_EXCEPTIONS)
		catch (duk_fatal_exception &exc) {
			DUK_D(DUK_DPRINT("rethrow duk_fatal_exception"));
			DUK_UNREF(exc);
			throw;
		} catch (std::exception &exc) {
			const char *what = exc.what();
			if (!what) {
				what = "unknown";
			}
			DUK_D(DUK_DPRINT("unexpected c++ std::exception (perhaps thrown by user code)"));
			DUK_STATS_INC(exec_thr->heap, stats_exec_throw);
			try {
				DUK_ASSERT(heap->curr_thread != NULL);
				DUK_ERROR_FMT1(heap->curr_thread,
				               DUK_ERR_TYPE_ERROR,
				               "caught invalid c++ std::exception '%s' (perhaps thrown by user code)",
				               what);
				DUK_WO_NORETURN(return;);
			} catch (duk_internal_exception exc) {
				DUK_D(DUK_DPRINT("caught api error thrown from unexpected c++ std::exception"));
				DUK_UNREF(exc);
				duk__handle_executor_error(heap,
				                           entry_act,
				                           entry_call_recursion_depth,
				                           entry_jmpbuf_ptr,
				                           &delayed_catch_setup);
			}
		} catch (...) {
			DUK_D(DUK_DPRINT("unexpected c++ exception (perhaps thrown by user code)"));
			DUK_STATS_INC(exec_thr->heap, stats_exec_throw);
			try {
				DUK_ASSERT(heap->curr_thread != NULL);
				DUK_ERROR_TYPE(heap->curr_thread, "caught invalid c++ exception (perhaps thrown by user code)");
				DUK_WO_NORETURN(return;);
			} catch (duk_internal_exception exc) {
				DUK_D(DUK_DPRINT("caught api error thrown from unexpected c++ exception"));
				DUK_UNREF(exc);
				duk__handle_executor_error(heap,
				                           entry_act,
				                           entry_call_recursion_depth,
				                           entry_jmpbuf_ptr,
				                           &delayed_catch_setup);
			}
		}
#endif
	}

	DUK_WO_NORETURN(return;);
}

/* Inner executor, performance critical. */
DUK_LOCAL DUK_NOINLINE DUK_HOT void duk__js_execute_bytecode_inner(duk_hthread *entry_thread, duk_activation *entry_act) {
	/* Current PC, accessed by other functions through thr->ptr_to_curr_pc.
	 * Critical for performance.  It would be safest to make this volatile,
	 * but that eliminates performance benefits; aliasing guarantees
	 * should be enough though.
	 */
	duk_instr_t *curr_pc; /* bytecode has a stable pointer */

	/* Hot variables for interpretation.  Critical for performance,
	 * but must add sparingly to minimize register shuffling.
	 */
	duk_hthread *thr; /* stable */
	duk_tval *consts; /* stable */
	duk_uint_fast32_t ins;
	/* 'funcs' is quite rarely used, so no local for it */
#if defined(DUK_USE_EXEC_FUN_LOCAL)
	duk_hcompfunc *fun;
#else
	/* 'fun' is quite rarely used, so no local for it */
#endif

#if defined(DUK_USE_INTERRUPT_COUNTER)
	duk_int_t int_ctr;
#endif

#if defined(DUK_USE_ASSERTIONS)
	duk_size_t valstack_top_base; /* valstack top, should match before interpreting each op (no leftovers) */
#endif

	/* Optimized reg/const access macros assume sizeof(duk_tval) to be
	 * either 8 or 16.  Heap allocation checks this even without asserts
	 * enabled now because it can't be autodetected in duk_config.h.
	 */
#if 1
#if defined(DUK_USE_PACKED_TVAL)
	DUK_ASSERT(sizeof(duk_tval) == 8);
#else
	DUK_ASSERT(sizeof(duk_tval) == 16);
#endif
#endif

	DUK_GC_TORTURE(entry_thread->heap);

	/*
	 *  Restart execution by reloading thread state.
	 *
	 *  Note that 'thr' and any thread configuration may have changed,
	 *  so all local variables are suspect and we need to reinitialize.
	 *
	 *  The number of local variables should be kept to a minimum: if
	 *  the variables are spilled, they will need to be loaded from
	 *  memory anyway.
	 *
	 *  Any 'goto restart_execution;' code path in opcode dispatch must
	 *  ensure 'curr_pc' is synced back to act->curr_pc before the goto
	 *  takes place.
	 *
	 *  The interpreter must be very careful with memory pointers, as
	 *  many pointers are not guaranteed to be 'stable' and may be
	 *  reallocated and relocated on-the-fly quite easily (e.g. by a
	 *  memory allocation or a property access).
	 *
	 *  The following are assumed to have stable pointers:
	 *    - the current thread
	 *    - the current function
	 *    - the bytecode, constant table, inner function table of the
	 *      current function (as they are a part of the function allocation)
	 *
	 *  The following are assumed to have semi-stable pointers:
	 *    - the current activation entry: stable as long as callstack
	 *      is not changed (reallocated by growing or shrinking), or
	 *      by any garbage collection invocation (through finalizers)
	 *    - Note in particular that ANY DECREF can invalidate the
	 *      activation pointer, so for the most part a fresh lookup
	 *      is required
	 *
	 *  The following are not assumed to have stable pointers at all:
	 *    - the value stack (registers) of the current thread
	 *
	 *  See execution.rst for discussion.
	 */

restart_execution:

	/* Lookup current thread; use the stable 'entry_thread' for this to
	 * avoid clobber warnings.  Any valid, reachable 'thr' value would be
	 * fine for this, so using 'entry_thread' is just to silence warnings.
	 */
	thr = entry_thread->heap->curr_thread;
	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->callstack_top >= 1);
	DUK_ASSERT(thr->callstack_curr != NULL);
	DUK_ASSERT(DUK_ACT_GET_FUNC(thr->callstack_curr) != NULL);
	DUK_ASSERT(DUK_HOBJECT_IS_COMPFUNC(DUK_ACT_GET_FUNC(thr->callstack_curr)));

	DUK_GC_TORTURE(thr->heap);

	thr->ptr_curr_pc = &curr_pc;

	/* Relookup and initialize dispatch loop variables.  Debugger check. */
	{
		duk_activation *act;
#if !defined(DUK_USE_EXEC_FUN_LOCAL)
		duk_hcompfunc *fun;
#endif

		/* Assume interrupt init/counter are properly initialized here. */
		/* Assume that thr->valstack_bottom has been set-up before getting here. */

		act = thr->callstack_curr;
		DUK_ASSERT(act != NULL);
		fun = (duk_hcompfunc *) DUK_ACT_GET_FUNC(act);
		DUK_ASSERT(fun != NULL);
		DUK_ASSERT(thr->valstack_top - thr->valstack_bottom == fun->nregs);
		consts = DUK_HCOMPFUNC_GET_CONSTS_BASE(thr->heap, fun);
		DUK_ASSERT(consts != NULL);

#if defined(DUK_USE_DEBUGGER_SUPPORT)
		if (DUK_UNLIKELY(duk_debug_is_attached(thr->heap) && !thr->heap->dbg_processing)) {
			duk__executor_recheck_debugger(thr, act, fun);
			DUK_ASSERT(act == thr->callstack_curr);
			DUK_ASSERT(act != NULL);
		}
#endif /* DUK_USE_DEBUGGER_SUPPORT */

#if defined(DUK_USE_ASSERTIONS)
		valstack_top_base = (duk_size_t) (thr->valstack_top - thr->valstack);
#endif

		/* Set up curr_pc for opcode dispatch. */
		curr_pc = act->curr_pc;
	}

	DUK_DD(DUK_DDPRINT("restarting execution, thr %p, act idx %ld, fun %p,"
	                   "consts %p, funcs %p, lev %ld, regbot %ld, regtop %ld, "
	                   "preventcount=%ld",
	                   (void *) thr,
	                   (long) (thr->callstack_top - 1),
	                   (void *) DUK__FUN(),
	                   (void *) DUK_HCOMPFUNC_GET_CONSTS_BASE(thr->heap, DUK__FUN()),
	                   (void *) DUK_HCOMPFUNC_GET_FUNCS_BASE(thr->heap, DUK__FUN()),
	                   (long) (thr->callstack_top - 1),
	                   (long) (thr->valstack_bottom - thr->valstack),
	                   (long) (thr->valstack_top - thr->valstack),
	                   (long) thr->callstack_preventcount));

	/* Dispatch loop. */

	for (;;) {
		duk_uint8_t op;

		DUK_ASSERT(thr->callstack_top >= 1);
		DUK_ASSERT(thr->valstack_top - thr->valstack_bottom == DUK__FUN()->nregs);
		DUK_ASSERT((duk_size_t) (thr->valstack_top - thr->valstack) == valstack_top_base);

		/* Executor interrupt counter check, used to implement breakpoints,
		 * debugging interface, execution timeouts, etc.  The counter is heap
		 * specific but is maintained in the current thread to make the check
		 * as fast as possible.  The counter is copied back to the heap struct
		 * whenever a thread switch occurs by the DUK_HEAP_SWITCH_THREAD() macro.
		 */
#if defined(DUK_USE_INTERRUPT_COUNTER)
		int_ctr = thr->interrupt_counter;
		if (DUK_LIKELY(int_ctr > 0)) {
			thr->interrupt_counter = int_ctr - 1;
		} else {
			/* Trigger at zero or below */
			duk_small_uint_t exec_int_ret;

			DUK_STATS_INC(thr->heap, stats_exec_interrupt);

			/* Write curr_pc back for the debugger. */
			{
				duk_activation *act;
				DUK_ASSERT(thr->callstack_top > 0);
				act = thr->callstack_curr;
				DUK_ASSERT(act != NULL);
				act->curr_pc = (duk_instr_t *) curr_pc;
			}

			/* Forced restart caused by a function return; must recheck
			 * debugger breakpoints before checking line transitions,
			 * see GH-303.  Restart and then handle interrupt_counter
			 * zero again.
			 */
#if defined(DUK_USE_DEBUGGER_SUPPORT)
			if (thr->heap->dbg_force_restart) {
				DUK_DD(DUK_DDPRINT("dbg_force_restart flag forced restart execution")); /* GH-303 */
				thr->heap->dbg_force_restart = 0;
				goto restart_execution;
			}
#endif

			exec_int_ret = duk__executor_interrupt(thr);
			if (exec_int_ret == DUK__INT_RESTART) {
				/* curr_pc synced back above */
				goto restart_execution;
			}
		}
#endif /* DUK_USE_INTERRUPT_COUNTER */
#if defined(DUK_USE_INTERRUPT_COUNTER) && defined(DUK_USE_DEBUG)
		/* For cross-checking during development: ensure dispatch count
		 * matches cumulative interrupt counter init value sums.
		 */
		thr->heap->inst_count_exec++;
#endif

#if defined(DUK_USE_ASSERTIONS) || defined(DUK_USE_DEBUG)
		{
			duk_activation *act;
			act = thr->callstack_curr;
			DUK_ASSERT(curr_pc >= DUK_HCOMPFUNC_GET_CODE_BASE(thr->heap, DUK__FUN()));
			DUK_ASSERT(curr_pc < DUK_HCOMPFUNC_GET_CODE_END(thr->heap, DUK__FUN()));
			DUK_UNREF(act); /* if debugging disabled */

			DUK_DDD(DUK_DDDPRINT(
			    "executing bytecode: pc=%ld, ins=0x%08lx, op=%ld, valstack_top=%ld/%ld, nregs=%ld  -->  %!I",
			    (long) (curr_pc - DUK_HCOMPFUNC_GET_CODE_BASE(thr->heap, DUK__FUN())),
			    (unsigned long) *curr_pc,
			    (long) DUK_DEC_OP(*curr_pc),
			    (long) (thr->valstack_top - thr->valstack),
			    (long) (thr->valstack_end - thr->valstack),
			    (long) (DUK__FUN() ? DUK__FUN()->nregs : -1),
			    (duk_instr_t) *curr_pc));
		}
#endif

#if defined(DUK_USE_ASSERTIONS)
		/* Quite heavy assert: check valstack policy.  Improper
		 * shuffle instructions can write beyond valstack_top/end
		 * so this check catches them in the act.
		 */
		{
			duk_tval *tv;
			tv = thr->valstack_top;
			while (tv != thr->valstack_end) {
				DUK_ASSERT(DUK_TVAL_IS_UNDEFINED(tv));
				tv++;
			}
		}
#endif

		ins = *curr_pc++;
		DUK_STATS_INC(thr->heap, stats_exec_opcodes);

		/* Typing: use duk_small_(u)int_fast_t when decoding small
		 * opcode fields (op, A, B, C, BC) which fit into 16 bits
		 * and duk_(u)int_fast_t when decoding larger fields (e.g.
		 * ABC).  Use unsigned variant by default, signed when the
		 * value is used in signed arithmetic.  Using variable names
		 * such as 'a', 'b', 'c', 'bc', etc makes it easier to spot
		 * typing mismatches.
		 */

		/* Switch based on opcode.  Cast to 8-bit unsigned value and
		 * use a fully populated case clauses so that the compiler
		 * will (at least usually) omit a bounds check.
		 */
		op = (duk_uint8_t) DUK_DEC_OP(ins);
		switch (op) {
			/* Some useful macros.  These access inner executor variables
			 * directly so they only apply within the executor.
			 */
#if defined(DUK_USE_EXEC_PREFER_SIZE)
#define DUK__REPLACE_TOP_A_BREAK() \
	{ goto replace_top_a; }
#define DUK__REPLACE_TOP_BC_BREAK() \
	{ goto replace_top_bc; }
#define DUK__REPLACE_BOOL_A_BREAK(bval) \
	{ \
		duk_bool_t duk__bval; \
		duk__bval = (bval); \
		DUK_ASSERT(duk__bval == 0 || duk__bval == 1); \
		duk_push_boolean(thr, duk__bval); \
		DUK__REPLACE_TOP_A_BREAK(); \
	}
#else
#define DUK__REPLACE_TOP_A_BREAK() \
	{ \
		DUK__REPLACE_TO_TVPTR(thr, DUK__REGP_A(ins)); \
		break; \
	}
#define DUK__REPLACE_TOP_BC_BREAK() \
	{ \
		DUK__REPLACE_TO_TVPTR(thr, DUK__REGP_BC(ins)); \
		break; \
	}
#define DUK__REPLACE_BOOL_A_BREAK(bval) \
	{ \
		duk_bool_t duk__bval; \
		duk_tval *duk__tvdst; \
		duk__bval = (bval); \
		DUK_ASSERT(duk__bval == 0 || duk__bval == 1); \
		duk__tvdst = DUK__REGP_A(ins); \
		DUK_TVAL_SET_BOOLEAN_UPDREF(thr, duk__tvdst, duk__bval); \
		break; \
	}
#endif

		/* XXX: 12 + 12 bit variant might make sense too, for both reg and
		 * const loads.
		 */

		/* For LDREG, STREG, LDCONST footprint optimized variants would just
		 * duk_dup() + duk_replace(), but because they're used quite a lot
		 * they're currently intentionally not size optimized.
		 */
		case DUK_OP_LDREG: {
			duk_tval *tv1, *tv2;

			tv1 = DUK__REGP_A(ins);
			tv2 = DUK__REGP_BC(ins);
			DUK_TVAL_SET_TVAL_UPDREF_FAST(thr, tv1, tv2); /* side effects */
			break;
		}

		case DUK_OP_STREG: {
			duk_tval *tv1, *tv2;

			tv1 = DUK__REGP_A(ins);
			tv2 = DUK__REGP_BC(ins);
			DUK_TVAL_SET_TVAL_UPDREF_FAST(thr, tv2, tv1); /* side effects */
			break;
		}

		case DUK_OP_LDCONST: {
			duk_tval *tv1, *tv2;

			tv1 = DUK__REGP_A(ins);
			tv2 = DUK__CONSTP_BC(ins);
			DUK_TVAL_SET_TVAL_UPDREF_FAST(thr, tv1, tv2); /* side effects */
			break;
		}

		/* LDINT and LDINTX are intended to load an arbitrary signed
		 * 32-bit value.  Only an LDINT+LDINTX sequence is supported.
		 * This also guarantees all values remain fastints.
		 */
#if defined(DUK_USE_EXEC_PREFER_SIZE)
		case DUK_OP_LDINT: {
			duk_int32_t val;

			val = (duk_int32_t) DUK_DEC_BC(ins) - (duk_int32_t) DUK_BC_LDINT_BIAS;
			duk_push_int(thr, val);
			DUK__REPLACE_TOP_A_BREAK();
		}
		case DUK_OP_LDINTX: {
			duk_int32_t val;

			val = (duk_int32_t) duk_get_int(thr, DUK_DEC_A(ins));
			val = (val << DUK_BC_LDINTX_SHIFT) + (duk_int32_t) DUK_DEC_BC(ins); /* no bias */
			duk_push_int(thr, val);
			DUK__REPLACE_TOP_A_BREAK();
		}
#else /* DUK_USE_EXEC_PREFER_SIZE */
		case DUK_OP_LDINT: {
			duk_tval *tv1;
			duk_int32_t val;

			val = (duk_int32_t) DUK_DEC_BC(ins) - (duk_int32_t) DUK_BC_LDINT_BIAS;
			tv1 = DUK__REGP_A(ins);
			DUK_TVAL_SET_I32_UPDREF(thr, tv1, val); /* side effects */
			break;
		}
		case DUK_OP_LDINTX: {
			duk_tval *tv1;
			duk_int32_t val;

			tv1 = DUK__REGP_A(ins);
			DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv1));
#if defined(DUK_USE_FASTINT)
			DUK_ASSERT(DUK_TVAL_IS_FASTINT(tv1));
			val = DUK_TVAL_GET_FASTINT_I32(tv1);
#else
			/* XXX: fast double-to-int conversion, we know number is integer in [-0x80000000,0xffffffff]. */
			val = (duk_int32_t) DUK_TVAL_GET_NUMBER(tv1);
#endif
			val =
			    (duk_int32_t) ((duk_uint32_t) val << DUK_BC_LDINTX_SHIFT) + (duk_int32_t) DUK_DEC_BC(ins); /* no bias */
			DUK_TVAL_SET_I32_UPDREF(thr, tv1, val); /* side effects */
			break;
		}
#endif /* DUK_USE_EXEC_PREFER_SIZE */

#if defined(DUK_USE_EXEC_PREFER_SIZE)
		case DUK_OP_LDTHIS: {
			duk_push_this(thr);
			DUK__REPLACE_TOP_BC_BREAK();
		}
		case DUK_OP_LDUNDEF: {
			duk_to_undefined(thr, (duk_idx_t) DUK_DEC_BC(ins));
			break;
		}
		case DUK_OP_LDNULL: {
			duk_to_null(thr, (duk_idx_t) DUK_DEC_BC(ins));
			break;
		}
		case DUK_OP_LDTRUE: {
			duk_push_true(thr);
			DUK__REPLACE_TOP_BC_BREAK();
		}
		case DUK_OP_LDFALSE: {
			duk_push_false(thr);
			DUK__REPLACE_TOP_BC_BREAK();
		}
#else /* DUK_USE_EXEC_PREFER_SIZE */
		case DUK_OP_LDTHIS: {
			/* Note: 'this' may be bound to any value, not just an object */
			duk_tval *tv1, *tv2;

			tv1 = DUK__REGP_BC(ins);
			tv2 = thr->valstack_bottom - 1; /* 'this binding' is just under bottom */
			DUK_ASSERT(tv2 >= thr->valstack);
			DUK_TVAL_SET_TVAL_UPDREF_FAST(thr, tv1, tv2); /* side effects */
			break;
		}
		case DUK_OP_LDUNDEF: {
			duk_tval *tv1;

			tv1 = DUK__REGP_BC(ins);
			DUK_TVAL_SET_UNDEFINED_UPDREF(thr, tv1); /* side effects */
			break;
		}
		case DUK_OP_LDNULL: {
			duk_tval *tv1;

			tv1 = DUK__REGP_BC(ins);
			DUK_TVAL_SET_NULL_UPDREF(thr, tv1); /* side effects */
			break;
		}
		case DUK_OP_LDTRUE: {
			duk_tval *tv1;

			tv1 = DUK__REGP_BC(ins);
			DUK_TVAL_SET_BOOLEAN_UPDREF(thr, tv1, 1); /* side effects */
			break;
		}
		case DUK_OP_LDFALSE: {
			duk_tval *tv1;

			tv1 = DUK__REGP_BC(ins);
			DUK_TVAL_SET_BOOLEAN_UPDREF(thr, tv1, 0); /* side effects */
			break;
		}
#endif /* DUK_USE_EXEC_PREFER_SIZE */

		case DUK_OP_BNOT: {
			duk__vm_bitwise_not(thr, DUK_DEC_BC(ins), DUK_DEC_A(ins));
			break;
		}

		case DUK_OP_LNOT: {
			duk__vm_logical_not(thr, DUK_DEC_BC(ins), DUK_DEC_A(ins));
			break;
		}

#if defined(DUK_USE_EXEC_PREFER_SIZE)
		case DUK_OP_UNM:
		case DUK_OP_UNP: {
			duk__vm_arith_unary_op(thr, DUK_DEC_BC(ins), DUK_DEC_A(ins), op);
			break;
		}
#else /* DUK_USE_EXEC_PREFER_SIZE */
		case DUK_OP_UNM: {
			duk__vm_arith_unary_op(thr, DUK_DEC_BC(ins), DUK_DEC_A(ins), DUK_OP_UNM);
			break;
		}
		case DUK_OP_UNP: {
			duk__vm_arith_unary_op(thr, DUK_DEC_BC(ins), DUK_DEC_A(ins), DUK_OP_UNP);
			break;
		}
#endif /* DUK_USE_EXEC_PREFER_SIZE */

#if defined(DUK_USE_EXEC_PREFER_SIZE)
		case DUK_OP_TYPEOF: {
			duk_small_uint_t stridx;

			stridx = duk_js_typeof_stridx(DUK__REGP_BC(ins));
			DUK_ASSERT_STRIDX_VALID(stridx);
			duk_push_hstring_stridx(thr, stridx);
			DUK__REPLACE_TOP_A_BREAK();
		}
#else /* DUK_USE_EXEC_PREFER_SIZE */
		case DUK_OP_TYPEOF: {
			duk_tval *tv;
			duk_small_uint_t stridx;
			duk_hstring *h_str;

			tv = DUK__REGP_BC(ins);
			stridx = duk_js_typeof_stridx(tv);
			DUK_ASSERT_STRIDX_VALID(stridx);
			h_str = DUK_HTHREAD_GET_STRING(thr, stridx);
			tv = DUK__REGP_A(ins);
			DUK_TVAL_SET_STRING_UPDREF(thr, tv, h_str);
			break;
		}
#endif /* DUK_USE_EXEC_PREFER_SIZE */

		case DUK_OP_TYPEOFID: {
			duk_small_uint_t stridx;
#if !defined(DUK_USE_EXEC_PREFER_SIZE)
			duk_hstring *h_str;
#endif
			duk_activation *act;
			duk_hstring *name;
			duk_tval *tv;

			/* A -> target register
			 * BC -> constant index of identifier name
			 */

			tv = DUK__CONSTP_BC(ins);
			DUK_ASSERT(DUK_TVAL_IS_STRING(tv));
			name = DUK_TVAL_GET_STRING(tv);
			tv = NULL; /* lookup has side effects */
			act = thr->callstack_curr;
			if (duk_js_getvar_activation(thr, act, name, 0 /*throw*/)) {
				/* -> [... val this] */
				tv = DUK_GET_TVAL_NEGIDX(thr, -2);
				stridx = duk_js_typeof_stridx(tv);
				tv = NULL; /* no longer needed */
				duk_pop_2_unsafe(thr);
			} else {
				/* unresolvable, no stack changes */
				stridx = DUK_STRIDX_LC_UNDEFINED;
			}
			DUK_ASSERT_STRIDX_VALID(stridx);
#if defined(DUK_USE_EXEC_PREFER_SIZE)
			duk_push_hstring_stridx(thr, stridx);
			DUK__REPLACE_TOP_A_BREAK();
#else /* DUK_USE_EXEC_PREFER_SIZE */
			h_str = DUK_HTHREAD_GET_STRING(thr, stridx);
			tv = DUK__REGP_A(ins);
			DUK_TVAL_SET_STRING_UPDREF(thr, tv, h_str);
			break;
#endif /* DUK_USE_EXEC_PREFER_SIZE */
		}

		/* Equality: E5 Sections 11.9.1, 11.9.3 */

#define DUK__EQ_BODY(barg, carg) \
	{ \
		duk_bool_t tmp; \
		tmp = duk_js_equals(thr, (barg), (carg)); \
		DUK_ASSERT(tmp == 0 || tmp == 1); \
		DUK__REPLACE_BOOL_A_BREAK(tmp); \
	}
#define DUK__NEQ_BODY(barg, carg) \
	{ \
		duk_bool_t tmp; \
		tmp = duk_js_equals(thr, (barg), (carg)); \
		DUK_ASSERT(tmp == 0 || tmp == 1); \
		tmp ^= 1; \
		DUK__REPLACE_BOOL_A_BREAK(tmp); \
	}
#define DUK__SEQ_BODY(barg, carg) \
	{ \
		duk_bool_t tmp; \
		tmp = duk_js_strict_equals((barg), (carg)); \
		DUK_ASSERT(tmp == 0 || tmp == 1); \
		DUK__REPLACE_BOOL_A_BREAK(tmp); \
	}
#define DUK__SNEQ_BODY(barg, carg) \
	{ \
		duk_bool_t tmp; \
		tmp = duk_js_strict_equals((barg), (carg)); \
		DUK_ASSERT(tmp == 0 || tmp == 1); \
		tmp ^= 1; \
		DUK__REPLACE_BOOL_A_BREAK(tmp); \
	}
#if defined(DUK_USE_EXEC_PREFER_SIZE)
		case DUK_OP_EQ_RR:
		case DUK_OP_EQ_CR:
		case DUK_OP_EQ_RC:
		case DUK_OP_EQ_CC:
			DUK__EQ_BODY(DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins));
		case DUK_OP_NEQ_RR:
		case DUK_OP_NEQ_CR:
		case DUK_OP_NEQ_RC:
		case DUK_OP_NEQ_CC:
			DUK__NEQ_BODY(DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins));
		case DUK_OP_SEQ_RR:
		case DUK_OP_SEQ_CR:
		case DUK_OP_SEQ_RC:
		case DUK_OP_SEQ_CC:
			DUK__SEQ_BODY(DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins));
		case DUK_OP_SNEQ_RR:
		case DUK_OP_SNEQ_CR:
		case DUK_OP_SNEQ_RC:
		case DUK_OP_SNEQ_CC:
			DUK__SNEQ_BODY(DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins));
#else /* DUK_USE_EXEC_PREFER_SIZE */
		case DUK_OP_EQ_RR:
			DUK__EQ_BODY(DUK__REGP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_EQ_CR:
			DUK__EQ_BODY(DUK__CONSTP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_EQ_RC:
			DUK__EQ_BODY(DUK__REGP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_EQ_CC:
			DUK__EQ_BODY(DUK__CONSTP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_NEQ_RR:
			DUK__NEQ_BODY(DUK__REGP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_NEQ_CR:
			DUK__NEQ_BODY(DUK__CONSTP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_NEQ_RC:
			DUK__NEQ_BODY(DUK__REGP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_NEQ_CC:
			DUK__NEQ_BODY(DUK__CONSTP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_SEQ_RR:
			DUK__SEQ_BODY(DUK__REGP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_SEQ_CR:
			DUK__SEQ_BODY(DUK__CONSTP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_SEQ_RC:
			DUK__SEQ_BODY(DUK__REGP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_SEQ_CC:
			DUK__SEQ_BODY(DUK__CONSTP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_SNEQ_RR:
			DUK__SNEQ_BODY(DUK__REGP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_SNEQ_CR:
			DUK__SNEQ_BODY(DUK__CONSTP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_SNEQ_RC:
			DUK__SNEQ_BODY(DUK__REGP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_SNEQ_CC:
			DUK__SNEQ_BODY(DUK__CONSTP_B(ins), DUK__CONSTP_C(ins));
#endif /* DUK_USE_EXEC_PREFER_SIZE */

#define DUK__COMPARE_BODY(arg1, arg2, flags) \
	{ \
		duk_bool_t tmp; \
		tmp = duk_js_compare_helper(thr, (arg1), (arg2), (flags)); \
		DUK_ASSERT(tmp == 0 || tmp == 1); \
		DUK__REPLACE_BOOL_A_BREAK(tmp); \
	}
#define DUK__GT_BODY(barg, carg) DUK__COMPARE_BODY((carg), (barg), 0)
#define DUK__GE_BODY(barg, carg) DUK__COMPARE_BODY((barg), (carg), DUK_COMPARE_FLAG_EVAL_LEFT_FIRST | DUK_COMPARE_FLAG_NEGATE)
#define DUK__LT_BODY(barg, carg) DUK__COMPARE_BODY((barg), (carg), DUK_COMPARE_FLAG_EVAL_LEFT_FIRST)
#define DUK__LE_BODY(barg, carg) DUK__COMPARE_BODY((carg), (barg), DUK_COMPARE_FLAG_NEGATE)
#if defined(DUK_USE_EXEC_PREFER_SIZE)
		case DUK_OP_GT_RR:
		case DUK_OP_GT_CR:
		case DUK_OP_GT_RC:
		case DUK_OP_GT_CC:
			DUK__GT_BODY(DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins));
		case DUK_OP_GE_RR:
		case DUK_OP_GE_CR:
		case DUK_OP_GE_RC:
		case DUK_OP_GE_CC:
			DUK__GE_BODY(DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins));
		case DUK_OP_LT_RR:
		case DUK_OP_LT_CR:
		case DUK_OP_LT_RC:
		case DUK_OP_LT_CC:
			DUK__LT_BODY(DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins));
		case DUK_OP_LE_RR:
		case DUK_OP_LE_CR:
		case DUK_OP_LE_RC:
		case DUK_OP_LE_CC:
			DUK__LE_BODY(DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins));
#else /* DUK_USE_EXEC_PREFER_SIZE */
		case DUK_OP_GT_RR:
			DUK__GT_BODY(DUK__REGP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_GT_CR:
			DUK__GT_BODY(DUK__CONSTP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_GT_RC:
			DUK__GT_BODY(DUK__REGP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_GT_CC:
			DUK__GT_BODY(DUK__CONSTP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_GE_RR:
			DUK__GE_BODY(DUK__REGP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_GE_CR:
			DUK__GE_BODY(DUK__CONSTP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_GE_RC:
			DUK__GE_BODY(DUK__REGP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_GE_CC:
			DUK__GE_BODY(DUK__CONSTP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_LT_RR:
			DUK__LT_BODY(DUK__REGP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_LT_CR:
			DUK__LT_BODY(DUK__CONSTP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_LT_RC:
			DUK__LT_BODY(DUK__REGP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_LT_CC:
			DUK__LT_BODY(DUK__CONSTP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_LE_RR:
			DUK__LE_BODY(DUK__REGP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_LE_CR:
			DUK__LE_BODY(DUK__CONSTP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_LE_RC:
			DUK__LE_BODY(DUK__REGP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_LE_CC:
			DUK__LE_BODY(DUK__CONSTP_B(ins), DUK__CONSTP_C(ins));
#endif /* DUK_USE_EXEC_PREFER_SIZE */

		/* No size optimized variant at present for IF. */
		case DUK_OP_IFTRUE_R: {
			if (duk_js_toboolean(DUK__REGP_BC(ins)) != 0) {
				curr_pc++;
			}
			break;
		}
		case DUK_OP_IFTRUE_C: {
			if (duk_js_toboolean(DUK__CONSTP_BC(ins)) != 0) {
				curr_pc++;
			}
			break;
		}
		case DUK_OP_IFFALSE_R: {
			if (duk_js_toboolean(DUK__REGP_BC(ins)) == 0) {
				curr_pc++;
			}
			break;
		}
		case DUK_OP_IFFALSE_C: {
			if (duk_js_toboolean(DUK__CONSTP_BC(ins)) == 0) {
				curr_pc++;
			}
			break;
		}

#if defined(DUK_USE_EXEC_PREFER_SIZE)
		case DUK_OP_ADD_RR:
		case DUK_OP_ADD_CR:
		case DUK_OP_ADD_RC:
		case DUK_OP_ADD_CC: {
			/* XXX: could leave value on stack top and goto replace_top_a; */
			duk__vm_arith_add(thr, DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins), DUK_DEC_A(ins));
			break;
		}
#else /* DUK_USE_EXEC_PREFER_SIZE */
		case DUK_OP_ADD_RR: {
			duk__vm_arith_add(thr, DUK__REGP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins));
			break;
		}
		case DUK_OP_ADD_CR: {
			duk__vm_arith_add(thr, DUK__CONSTP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins));
			break;
		}
		case DUK_OP_ADD_RC: {
			duk__vm_arith_add(thr, DUK__REGP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins));
			break;
		}
		case DUK_OP_ADD_CC: {
			duk__vm_arith_add(thr, DUK__CONSTP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins));
			break;
		}
#endif /* DUK_USE_EXEC_PREFER_SIZE */

#if defined(DUK_USE_EXEC_PREFER_SIZE)
		case DUK_OP_SUB_RR:
		case DUK_OP_SUB_CR:
		case DUK_OP_SUB_RC:
		case DUK_OP_SUB_CC:
		case DUK_OP_MUL_RR:
		case DUK_OP_MUL_CR:
		case DUK_OP_MUL_RC:
		case DUK_OP_MUL_CC:
		case DUK_OP_DIV_RR:
		case DUK_OP_DIV_CR:
		case DUK_OP_DIV_RC:
		case DUK_OP_DIV_CC:
		case DUK_OP_MOD_RR:
		case DUK_OP_MOD_CR:
		case DUK_OP_MOD_RC:
		case DUK_OP_MOD_CC:
#if defined(DUK_USE_ES7_EXP_OPERATOR)
		case DUK_OP_EXP_RR:
		case DUK_OP_EXP_CR:
		case DUK_OP_EXP_RC:
		case DUK_OP_EXP_CC:
#endif /* DUK_USE_ES7_EXP_OPERATOR */
		{
			/* XXX: could leave value on stack top and goto replace_top_a; */
			duk__vm_arith_binary_op(thr, DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins), DUK_DEC_A(ins), op);
			break;
		}
#else /* DUK_USE_EXEC_PREFER_SIZE */
		case DUK_OP_SUB_RR: {
			duk__vm_arith_binary_op(thr, DUK__REGP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_SUB);
			break;
		}
		case DUK_OP_SUB_CR: {
			duk__vm_arith_binary_op(thr, DUK__CONSTP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_SUB);
			break;
		}
		case DUK_OP_SUB_RC: {
			duk__vm_arith_binary_op(thr, DUK__REGP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_SUB);
			break;
		}
		case DUK_OP_SUB_CC: {
			duk__vm_arith_binary_op(thr, DUK__CONSTP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_SUB);
			break;
		}
		case DUK_OP_MUL_RR: {
			duk__vm_arith_binary_op(thr, DUK__REGP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_MUL);
			break;
		}
		case DUK_OP_MUL_CR: {
			duk__vm_arith_binary_op(thr, DUK__CONSTP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_MUL);
			break;
		}
		case DUK_OP_MUL_RC: {
			duk__vm_arith_binary_op(thr, DUK__REGP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_MUL);
			break;
		}
		case DUK_OP_MUL_CC: {
			duk__vm_arith_binary_op(thr, DUK__CONSTP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_MUL);
			break;
		}
		case DUK_OP_DIV_RR: {
			duk__vm_arith_binary_op(thr, DUK__REGP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_DIV);
			break;
		}
		case DUK_OP_DIV_CR: {
			duk__vm_arith_binary_op(thr, DUK__CONSTP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_DIV);
			break;
		}
		case DUK_OP_DIV_RC: {
			duk__vm_arith_binary_op(thr, DUK__REGP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_DIV);
			break;
		}
		case DUK_OP_DIV_CC: {
			duk__vm_arith_binary_op(thr, DUK__CONSTP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_DIV);
			break;
		}
		case DUK_OP_MOD_RR: {
			duk__vm_arith_binary_op(thr, DUK__REGP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_MOD);
			break;
		}
		case DUK_OP_MOD_CR: {
			duk__vm_arith_binary_op(thr, DUK__CONSTP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_MOD);
			break;
		}
		case DUK_OP_MOD_RC: {
			duk__vm_arith_binary_op(thr, DUK__REGP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_MOD);
			break;
		}
		case DUK_OP_MOD_CC: {
			duk__vm_arith_binary_op(thr, DUK__CONSTP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_MOD);
			break;
		}
#if defined(DUK_USE_ES7_EXP_OPERATOR)
		case DUK_OP_EXP_RR: {
			duk__vm_arith_binary_op(thr, DUK__REGP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_EXP);
			break;
		}
		case DUK_OP_EXP_CR: {
			duk__vm_arith_binary_op(thr, DUK__CONSTP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_EXP);
			break;
		}
		case DUK_OP_EXP_RC: {
			duk__vm_arith_binary_op(thr, DUK__REGP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_EXP);
			break;
		}
		case DUK_OP_EXP_CC: {
			duk__vm_arith_binary_op(thr, DUK__CONSTP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_EXP);
			break;
		}
#endif /* DUK_USE_ES7_EXP_OPERATOR */
#endif /* DUK_USE_EXEC_PREFER_SIZE */

#if defined(DUK_USE_EXEC_PREFER_SIZE)
		case DUK_OP_BAND_RR:
		case DUK_OP_BAND_CR:
		case DUK_OP_BAND_RC:
		case DUK_OP_BAND_CC:
		case DUK_OP_BOR_RR:
		case DUK_OP_BOR_CR:
		case DUK_OP_BOR_RC:
		case DUK_OP_BOR_CC:
		case DUK_OP_BXOR_RR:
		case DUK_OP_BXOR_CR:
		case DUK_OP_BXOR_RC:
		case DUK_OP_BXOR_CC:
		case DUK_OP_BASL_RR:
		case DUK_OP_BASL_CR:
		case DUK_OP_BASL_RC:
		case DUK_OP_BASL_CC:
		case DUK_OP_BLSR_RR:
		case DUK_OP_BLSR_CR:
		case DUK_OP_BLSR_RC:
		case DUK_OP_BLSR_CC:
		case DUK_OP_BASR_RR:
		case DUK_OP_BASR_CR:
		case DUK_OP_BASR_RC:
		case DUK_OP_BASR_CC: {
			/* XXX: could leave value on stack top and goto replace_top_a; */
			duk__vm_bitwise_binary_op(thr, DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins), DUK_DEC_A(ins), op);
			break;
		}
#else /* DUK_USE_EXEC_PREFER_SIZE */
		case DUK_OP_BAND_RR: {
			duk__vm_bitwise_binary_op(thr, DUK__REGP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_BAND);
			break;
		}
		case DUK_OP_BAND_CR: {
			duk__vm_bitwise_binary_op(thr, DUK__CONSTP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_BAND);
			break;
		}
		case DUK_OP_BAND_RC: {
			duk__vm_bitwise_binary_op(thr, DUK__REGP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_BAND);
			break;
		}
		case DUK_OP_BAND_CC: {
			duk__vm_bitwise_binary_op(thr, DUK__CONSTP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_BAND);
			break;
		}
		case DUK_OP_BOR_RR: {
			duk__vm_bitwise_binary_op(thr, DUK__REGP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_BOR);
			break;
		}
		case DUK_OP_BOR_CR: {
			duk__vm_bitwise_binary_op(thr, DUK__CONSTP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_BOR);
			break;
		}
		case DUK_OP_BOR_RC: {
			duk__vm_bitwise_binary_op(thr, DUK__REGP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_BOR);
			break;
		}
		case DUK_OP_BOR_CC: {
			duk__vm_bitwise_binary_op(thr, DUK__CONSTP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_BOR);
			break;
		}
		case DUK_OP_BXOR_RR: {
			duk__vm_bitwise_binary_op(thr, DUK__REGP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_BXOR);
			break;
		}
		case DUK_OP_BXOR_CR: {
			duk__vm_bitwise_binary_op(thr, DUK__CONSTP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_BXOR);
			break;
		}
		case DUK_OP_BXOR_RC: {
			duk__vm_bitwise_binary_op(thr, DUK__REGP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_BXOR);
			break;
		}
		case DUK_OP_BXOR_CC: {
			duk__vm_bitwise_binary_op(thr, DUK__CONSTP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_BXOR);
			break;
		}
		case DUK_OP_BASL_RR: {
			duk__vm_bitwise_binary_op(thr, DUK__REGP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_BASL);
			break;
		}
		case DUK_OP_BASL_CR: {
			duk__vm_bitwise_binary_op(thr, DUK__CONSTP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_BASL);
			break;
		}
		case DUK_OP_BASL_RC: {
			duk__vm_bitwise_binary_op(thr, DUK__REGP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_BASL);
			break;
		}
		case DUK_OP_BASL_CC: {
			duk__vm_bitwise_binary_op(thr, DUK__CONSTP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_BASL);
			break;
		}
		case DUK_OP_BLSR_RR: {
			duk__vm_bitwise_binary_op(thr, DUK__REGP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_BLSR);
			break;
		}
		case DUK_OP_BLSR_CR: {
			duk__vm_bitwise_binary_op(thr, DUK__CONSTP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_BLSR);
			break;
		}
		case DUK_OP_BLSR_RC: {
			duk__vm_bitwise_binary_op(thr, DUK__REGP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_BLSR);
			break;
		}
		case DUK_OP_BLSR_CC: {
			duk__vm_bitwise_binary_op(thr, DUK__CONSTP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_BLSR);
			break;
		}
		case DUK_OP_BASR_RR: {
			duk__vm_bitwise_binary_op(thr, DUK__REGP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_BASR);
			break;
		}
		case DUK_OP_BASR_CR: {
			duk__vm_bitwise_binary_op(thr, DUK__CONSTP_B(ins), DUK__REGP_C(ins), DUK_DEC_A(ins), DUK_OP_BASR);
			break;
		}
		case DUK_OP_BASR_RC: {
			duk__vm_bitwise_binary_op(thr, DUK__REGP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_BASR);
			break;
		}
		case DUK_OP_BASR_CC: {
			duk__vm_bitwise_binary_op(thr, DUK__CONSTP_B(ins), DUK__CONSTP_C(ins), DUK_DEC_A(ins), DUK_OP_BASR);
			break;
		}
#endif /* DUK_USE_EXEC_PREFER_SIZE */

		/* For INSTOF and IN, B is always a register. */
#define DUK__INSTOF_BODY(barg, carg) \
	{ \
		duk_bool_t tmp; \
		tmp = duk_js_instanceof(thr, (barg), (carg)); \
		DUK_ASSERT(tmp == 0 || tmp == 1); \
		DUK__REPLACE_BOOL_A_BREAK(tmp); \
	}
#define DUK__IN_BODY(barg, carg) \
	{ \
		duk_bool_t tmp; \
		tmp = duk_js_in(thr, (barg), (carg)); \
		DUK_ASSERT(tmp == 0 || tmp == 1); \
		DUK__REPLACE_BOOL_A_BREAK(tmp); \
	}
#if defined(DUK_USE_EXEC_PREFER_SIZE)
		case DUK_OP_INSTOF_RR:
		case DUK_OP_INSTOF_CR:
		case DUK_OP_INSTOF_RC:
		case DUK_OP_INSTOF_CC:
			DUK__INSTOF_BODY(DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins));
		case DUK_OP_IN_RR:
		case DUK_OP_IN_CR:
		case DUK_OP_IN_RC:
		case DUK_OP_IN_CC:
			DUK__IN_BODY(DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins));
#else /* DUK_USE_EXEC_PREFER_SIZE */
		case DUK_OP_INSTOF_RR:
			DUK__INSTOF_BODY(DUK__REGP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_INSTOF_CR:
			DUK__INSTOF_BODY(DUK__CONSTP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_INSTOF_RC:
			DUK__INSTOF_BODY(DUK__REGP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_INSTOF_CC:
			DUK__INSTOF_BODY(DUK__CONSTP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_IN_RR:
			DUK__IN_BODY(DUK__REGP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_IN_CR:
			DUK__IN_BODY(DUK__CONSTP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_IN_RC:
			DUK__IN_BODY(DUK__REGP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_IN_CC:
			DUK__IN_BODY(DUK__CONSTP_B(ins), DUK__CONSTP_C(ins));
#endif /* DUK_USE_EXEC_PREFER_SIZE */

			/* Pre/post inc/dec for register variables, important for loops. */
#if defined(DUK_USE_EXEC_PREFER_SIZE)
		case DUK_OP_PREINCR:
		case DUK_OP_PREDECR:
		case DUK_OP_POSTINCR:
		case DUK_OP_POSTDECR: {
			duk__prepost_incdec_reg_helper(thr, DUK__REGP_A(ins), DUK__REGP_BC(ins), op);
			break;
		}
		case DUK_OP_PREINCV:
		case DUK_OP_PREDECV:
		case DUK_OP_POSTINCV:
		case DUK_OP_POSTDECV: {
			duk__prepost_incdec_var_helper(thr, DUK_DEC_A(ins), DUK__CONSTP_BC(ins), op, DUK__STRICT());
			break;
		}
#else /* DUK_USE_EXEC_PREFER_SIZE */
		case DUK_OP_PREINCR: {
			duk__prepost_incdec_reg_helper(thr, DUK__REGP_A(ins), DUK__REGP_BC(ins), DUK_OP_PREINCR);
			break;
		}
		case DUK_OP_PREDECR: {
			duk__prepost_incdec_reg_helper(thr, DUK__REGP_A(ins), DUK__REGP_BC(ins), DUK_OP_PREDECR);
			break;
		}
		case DUK_OP_POSTINCR: {
			duk__prepost_incdec_reg_helper(thr, DUK__REGP_A(ins), DUK__REGP_BC(ins), DUK_OP_POSTINCR);
			break;
		}
		case DUK_OP_POSTDECR: {
			duk__prepost_incdec_reg_helper(thr, DUK__REGP_A(ins), DUK__REGP_BC(ins), DUK_OP_POSTDECR);
			break;
		}
		case DUK_OP_PREINCV: {
			duk__prepost_incdec_var_helper(thr, DUK_DEC_A(ins), DUK__CONSTP_BC(ins), DUK_OP_PREINCV, DUK__STRICT());
			break;
		}
		case DUK_OP_PREDECV: {
			duk__prepost_incdec_var_helper(thr, DUK_DEC_A(ins), DUK__CONSTP_BC(ins), DUK_OP_PREDECV, DUK__STRICT());
			break;
		}
		case DUK_OP_POSTINCV: {
			duk__prepost_incdec_var_helper(thr, DUK_DEC_A(ins), DUK__CONSTP_BC(ins), DUK_OP_POSTINCV, DUK__STRICT());
			break;
		}
		case DUK_OP_POSTDECV: {
			duk__prepost_incdec_var_helper(thr, DUK_DEC_A(ins), DUK__CONSTP_BC(ins), DUK_OP_POSTDECV, DUK__STRICT());
			break;
		}
#endif /* DUK_USE_EXEC_PREFER_SIZE */

		/* XXX: Move to separate helper, optimize for perf/size separately. */
		/* Preinc/predec for object properties. */
		case DUK_OP_PREINCP_RR:
		case DUK_OP_PREINCP_CR:
		case DUK_OP_PREINCP_RC:
		case DUK_OP_PREINCP_CC:
		case DUK_OP_PREDECP_RR:
		case DUK_OP_PREDECP_CR:
		case DUK_OP_PREDECP_RC:
		case DUK_OP_PREDECP_CC:
		case DUK_OP_POSTINCP_RR:
		case DUK_OP_POSTINCP_CR:
		case DUK_OP_POSTINCP_RC:
		case DUK_OP_POSTINCP_CC:
		case DUK_OP_POSTDECP_RR:
		case DUK_OP_POSTDECP_CR:
		case DUK_OP_POSTDECP_RC:
		case DUK_OP_POSTDECP_CC: {
			duk_tval *tv_obj;
			duk_tval *tv_key;
			duk_tval *tv_val;
			duk_bool_t rc;
			duk_double_t x, y, z;
#if !defined(DUK_USE_EXEC_PREFER_SIZE)
			duk_tval *tv_dst;
#endif /* DUK_USE_EXEC_PREFER_SIZE */

			/* A -> target reg
			 * B -> object reg/const (may be const e.g. in "'foo'[1]")
			 * C -> key reg/const
			 */

			/* Opcode bits 0-1 are used to distinguish reg/const variants.
			 * Opcode bits 2-3 are used to distinguish inc/dec variants:
			 * Bit 2 = inc(0)/dec(1), bit 3 = pre(0)/post(1).
			 */
			DUK_ASSERT((DUK_OP_PREINCP_RR & 0x0c) == 0x00);
			DUK_ASSERT((DUK_OP_PREDECP_RR & 0x0c) == 0x04);
			DUK_ASSERT((DUK_OP_POSTINCP_RR & 0x0c) == 0x08);
			DUK_ASSERT((DUK_OP_POSTDECP_RR & 0x0c) == 0x0c);

			tv_obj = DUK__REGCONSTP_B(ins);
			tv_key = DUK__REGCONSTP_C(ins);
			rc = duk_hobject_getprop(thr, tv_obj, tv_key); /* -> [val] */
			DUK_UNREF(rc); /* ignore */
			tv_obj = NULL; /* invalidated */
			tv_key = NULL; /* invalidated */

			/* XXX: Fastint fast path would be useful here.  Also fastints
			 * now lose their fastint status in current handling which is
			 * not intuitive.
			 */

			x = duk_to_number_m1(thr);
			duk_pop_unsafe(thr);
			if (ins & DUK_BC_INCDECP_FLAG_DEC) {
				y = x - 1.0;
			} else {
				y = x + 1.0;
			}

			duk_push_number(thr, y);
			tv_val = DUK_GET_TVAL_NEGIDX(thr, -1);
			DUK_ASSERT(tv_val != NULL);
			tv_obj = DUK__REGCONSTP_B(ins);
			tv_key = DUK__REGCONSTP_C(ins);
			rc = duk_hobject_putprop(thr, tv_obj, tv_key, tv_val, DUK__STRICT());
			DUK_UNREF(rc); /* ignore */
			tv_obj = NULL; /* invalidated */
			tv_key = NULL; /* invalidated */
			duk_pop_unsafe(thr);

			z = (ins & DUK_BC_INCDECP_FLAG_POST) ? x : y;
#if defined(DUK_USE_EXEC_PREFER_SIZE)
			duk_push_number(thr, z);
			DUK__REPLACE_TOP_A_BREAK();
#else
			tv_dst = DUK__REGP_A(ins);
			DUK_TVAL_SET_NUMBER_UPDREF(thr, tv_dst, z);
			break;
#endif
		}

		/* XXX: GETPROP where object is 'this', GETPROPT?
		 * Occurs relatively often in object oriented code.
		 */

#define DUK__GETPROP_BODY(barg, carg) \
	{ \
		/* A -> target reg \
		 * B -> object reg/const (may be const e.g. in "'foo'[1]") \
		 * C -> key reg/const \
		 */ \
		(void) duk_hobject_getprop(thr, (barg), (carg)); \
		DUK__REPLACE_TOP_A_BREAK(); \
	}
#define DUK__GETPROPC_BODY(barg, carg) \
	{ \
		/* Same as GETPROP but callability check for property-based calls. */ \
		duk_tval *tv__targ; \
		(void) duk_hobject_getprop(thr, (barg), (carg)); \
		DUK_GC_TORTURE(thr->heap); \
		tv__targ = DUK_GET_TVAL_NEGIDX(thr, -1); \
		if (DUK_UNLIKELY(!duk_is_callable_tval(thr, tv__targ))) { \
			/* Here we intentionally re-evaluate the macro \
			 * arguments to deal with potentially changed \
			 * valstack base pointer! \
			 */ \
			duk_call_setup_propcall_error(thr, (barg), (carg)); \
		} \
		DUK__REPLACE_TOP_A_BREAK(); \
	}
#define DUK__PUTPROP_BODY(aarg, barg, carg) \
	{ \
		/* A -> object reg \
		 * B -> key reg/const \
		 * C -> value reg/const \
		 * \
		 * Note: intentional difference to register arrangement \
		 * of e.g. GETPROP; 'A' must contain a register-only value. \
		 */ \
		(void) duk_hobject_putprop(thr, (aarg), (barg), (carg), DUK__STRICT()); \
		break; \
	}
#define DUK__DELPROP_BODY(barg, carg) \
	{ \
		/* A -> result reg \
		 * B -> object reg \
		 * C -> key reg/const \
		 */ \
		duk_bool_t rc; \
		rc = duk_hobject_delprop(thr, (barg), (carg), DUK__STRICT()); \
		DUK_ASSERT(rc == 0 || rc == 1); \
		DUK__REPLACE_BOOL_A_BREAK(rc); \
	}
#if defined(DUK_USE_EXEC_PREFER_SIZE)
		case DUK_OP_GETPROP_RR:
		case DUK_OP_GETPROP_CR:
		case DUK_OP_GETPROP_RC:
		case DUK_OP_GETPROP_CC:
			DUK__GETPROP_BODY(DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins));
#if defined(DUK_USE_VERBOSE_ERRORS)
		case DUK_OP_GETPROPC_RR:
		case DUK_OP_GETPROPC_CR:
		case DUK_OP_GETPROPC_RC:
		case DUK_OP_GETPROPC_CC:
			DUK__GETPROPC_BODY(DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins));
#endif
		case DUK_OP_PUTPROP_RR:
		case DUK_OP_PUTPROP_CR:
		case DUK_OP_PUTPROP_RC:
		case DUK_OP_PUTPROP_CC:
			DUK__PUTPROP_BODY(DUK__REGP_A(ins), DUK__REGCONSTP_B(ins), DUK__REGCONSTP_C(ins));
		case DUK_OP_DELPROP_RR:
		case DUK_OP_DELPROP_RC: /* B is always reg */
			DUK__DELPROP_BODY(DUK__REGP_B(ins), DUK__REGCONSTP_C(ins));
#else /* DUK_USE_EXEC_PREFER_SIZE */
		case DUK_OP_GETPROP_RR:
			DUK__GETPROP_BODY(DUK__REGP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_GETPROP_CR:
			DUK__GETPROP_BODY(DUK__CONSTP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_GETPROP_RC:
			DUK__GETPROP_BODY(DUK__REGP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_GETPROP_CC:
			DUK__GETPROP_BODY(DUK__CONSTP_B(ins), DUK__CONSTP_C(ins));
#if defined(DUK_USE_VERBOSE_ERRORS)
		case DUK_OP_GETPROPC_RR:
			DUK__GETPROPC_BODY(DUK__REGP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_GETPROPC_CR:
			DUK__GETPROPC_BODY(DUK__CONSTP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_GETPROPC_RC:
			DUK__GETPROPC_BODY(DUK__REGP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_GETPROPC_CC:
			DUK__GETPROPC_BODY(DUK__CONSTP_B(ins), DUK__CONSTP_C(ins));
#endif
		case DUK_OP_PUTPROP_RR:
			DUK__PUTPROP_BODY(DUK__REGP_A(ins), DUK__REGP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_PUTPROP_CR:
			DUK__PUTPROP_BODY(DUK__REGP_A(ins), DUK__CONSTP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_PUTPROP_RC:
			DUK__PUTPROP_BODY(DUK__REGP_A(ins), DUK__REGP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_PUTPROP_CC:
			DUK__PUTPROP_BODY(DUK__REGP_A(ins), DUK__CONSTP_B(ins), DUK__CONSTP_C(ins));
		case DUK_OP_DELPROP_RR: /* B is always reg */
			DUK__DELPROP_BODY(DUK__REGP_B(ins), DUK__REGP_C(ins));
		case DUK_OP_DELPROP_RC:
			DUK__DELPROP_BODY(DUK__REGP_B(ins), DUK__CONSTP_C(ins));
#endif /* DUK_USE_EXEC_PREFER_SIZE */

		/* No fast path for DECLVAR now, it's quite a rare instruction. */
		case DUK_OP_DECLVAR_RR:
		case DUK_OP_DECLVAR_CR:
		case DUK_OP_DECLVAR_RC:
		case DUK_OP_DECLVAR_CC: {
			duk_activation *act;
			duk_small_uint_fast_t a = DUK_DEC_A(ins);
			duk_tval *tv1;
			duk_hstring *name;
			duk_small_uint_t prop_flags;
			duk_bool_t is_func_decl;

			tv1 = DUK__REGCONSTP_B(ins);
			DUK_ASSERT(DUK_TVAL_IS_STRING(tv1));
			name = DUK_TVAL_GET_STRING(tv1);
			DUK_ASSERT(name != NULL);

			is_func_decl = ((a & DUK_BC_DECLVAR_FLAG_FUNC_DECL) != 0);

			/* XXX: declvar takes an duk_tval pointer, which is awkward and
			 * should be reworked.
			 */

			/* Compiler is responsible for selecting property flags (configurability,
			 * writability, etc).
			 */
			prop_flags = a & DUK_PROPDESC_FLAGS_MASK;

			if (is_func_decl) {
				duk_push_tval(thr, DUK__REGCONSTP_C(ins));
			} else {
				DUK_ASSERT(DUK_TVAL_IS_UNDEFINED(thr->valstack_top)); /* valstack policy */
				thr->valstack_top++;
			}
			tv1 = DUK_GET_TVAL_NEGIDX(thr, -1);

			act = thr->callstack_curr;
			if (duk_js_declvar_activation(thr, act, name, tv1, prop_flags, is_func_decl)) {
				if (is_func_decl) {
					/* Already declared, update value. */
					tv1 = DUK_GET_TVAL_NEGIDX(thr, -1);
					duk_js_putvar_activation(thr, act, name, tv1, DUK__STRICT());
				} else {
					/* Already declared but no initializer value
					 * (e.g. 'var xyz;'), no-op.
					 */
				}
			}

			duk_pop_unsafe(thr);
			break;
		}

#if defined(DUK_USE_REGEXP_SUPPORT)
		/* The compiler should never emit DUK_OP_REGEXP if there is no
		 * regexp support.
		 */
		case DUK_OP_REGEXP_RR:
		case DUK_OP_REGEXP_CR:
		case DUK_OP_REGEXP_RC:
		case DUK_OP_REGEXP_CC: {
			/* A -> target register
			 * B -> bytecode (also contains flags)
			 * C -> escaped source
			 */

			duk_push_tval(thr, DUK__REGCONSTP_C(ins));
			duk_push_tval(thr, DUK__REGCONSTP_B(ins)); /* -> [ ... escaped_source bytecode ] */
			duk_regexp_create_instance(thr); /* -> [ ... regexp_instance ] */
			DUK__REPLACE_TOP_A_BREAK();
		}
#endif /* DUK_USE_REGEXP_SUPPORT */

		/* XXX: 'c' is unused, use whole BC, etc. */
		case DUK_OP_CSVAR_RR:
		case DUK_OP_CSVAR_CR:
		case DUK_OP_CSVAR_RC:
		case DUK_OP_CSVAR_CC: {
			/* The speciality of calling through a variable binding is that the
			 * 'this' value may be provided by the variable lookup: E5 Section 6.b.i.
			 *
			 * The only (standard) case where the 'this' binding is non-null is when
			 *   (1) the variable is found in an object environment record, and
			 *   (2) that object environment record is a 'with' block.
			 */

			duk_activation *act;
			duk_uint_fast_t idx;
			duk_tval *tv1;
			duk_hstring *name;

			/* A -> target registers (A, A + 1) for call setup
			 * B -> identifier name, usually constant but can be a register due to shuffling
			 */

			tv1 = DUK__REGCONSTP_B(ins);
			DUK_ASSERT(DUK_TVAL_IS_STRING(tv1));
			name = DUK_TVAL_GET_STRING(tv1);
			DUK_ASSERT(name != NULL);
			act = thr->callstack_curr;
			(void) duk_js_getvar_activation(thr, act, name, 1 /*throw*/); /* -> [... val this] */

			idx = (duk_uint_fast_t) DUK_DEC_A(ins);

			/* Could add direct value stack handling. */
			duk_replace(thr, (duk_idx_t) (idx + 1)); /* 'this' binding */
			duk_replace(thr, (duk_idx_t) idx); /* variable value (function, we hope, not checked here) */
			break;
		}

		case DUK_OP_CLOSURE: {
			duk_activation *act;
			duk_hcompfunc *fun_act;
			duk_small_uint_fast_t bc = DUK_DEC_BC(ins);
			duk_hobject *fun_temp;

			/* A -> target reg
			 * BC -> inner function index
			 */

			DUK_DDD(DUK_DDDPRINT("CLOSURE to target register %ld, fnum %ld (count %ld)",
			                     (long) DUK_DEC_A(ins),
			                     (long) DUK_DEC_BC(ins),
			                     (long) DUK_HCOMPFUNC_GET_FUNCS_COUNT(thr->heap, DUK__FUN())));

			DUK_ASSERT_DISABLE(bc >= 0); /* unsigned */
			DUK_ASSERT((duk_uint_t) bc < (duk_uint_t) DUK_HCOMPFUNC_GET_FUNCS_COUNT(thr->heap, DUK__FUN()));

			act = thr->callstack_curr;
			fun_act = (duk_hcompfunc *) DUK_ACT_GET_FUNC(act);
			fun_temp = DUK_HCOMPFUNC_GET_FUNCS_BASE(thr->heap, fun_act)[bc];
			DUK_ASSERT(fun_temp != NULL);
			DUK_ASSERT(DUK_HOBJECT_IS_COMPFUNC(fun_temp));

			DUK_DDD(
			    DUK_DDDPRINT("CLOSURE: function template is: %p -> %!O", (void *) fun_temp, (duk_heaphdr *) fun_temp));

			if (act->lex_env == NULL) {
				DUK_ASSERT(act->var_env == NULL);
				duk_js_init_activation_environment_records_delayed(thr, act);
				act = thr->callstack_curr;
			}
			DUK_ASSERT(act->lex_env != NULL);
			DUK_ASSERT(act->var_env != NULL);

			/* functions always have a NEWENV flag, i.e. they get a
			 * new variable declaration environment, so only lex_env
			 * matters here.
			 */
			duk_js_push_closure(thr, (duk_hcompfunc *) fun_temp, act->var_env, act->lex_env, 1 /*add_auto_proto*/);
			DUK__REPLACE_TOP_A_BREAK();
		}

		case DUK_OP_GETVAR: {
			duk_activation *act;
			duk_tval *tv1;
			duk_hstring *name;

			tv1 = DUK__CONSTP_BC(ins);
			DUK_ASSERT(DUK_TVAL_IS_STRING(tv1));
			name = DUK_TVAL_GET_STRING(tv1);
			DUK_ASSERT(name != NULL);
			act = thr->callstack_curr;
			DUK_ASSERT(act != NULL);
			(void) duk_js_getvar_activation(thr, act, name, 1 /*throw*/); /* -> [... val this] */
			duk_pop_unsafe(thr); /* 'this' binding is not needed here */
			DUK__REPLACE_TOP_A_BREAK();
		}

		case DUK_OP_PUTVAR: {
			duk_activation *act;
			duk_tval *tv1;
			duk_hstring *name;

			tv1 = DUK__CONSTP_BC(ins);
			DUK_ASSERT(DUK_TVAL_IS_STRING(tv1));
			name = DUK_TVAL_GET_STRING(tv1);
			DUK_ASSERT(name != NULL);

			/* XXX: putvar takes a duk_tval pointer, which is awkward and
			 * should be reworked.
			 */

			tv1 = DUK__REGP_A(ins); /* val */
			act = thr->callstack_curr;
			duk_js_putvar_activation(thr, act, name, tv1, DUK__STRICT());
			break;
		}

		case DUK_OP_DELVAR: {
			duk_activation *act;
			duk_tval *tv1;
			duk_hstring *name;
			duk_bool_t rc;

			tv1 = DUK__CONSTP_BC(ins);
			DUK_ASSERT(DUK_TVAL_IS_STRING(tv1));
			name = DUK_TVAL_GET_STRING(tv1);
			DUK_ASSERT(name != NULL);
			act = thr->callstack_curr;
			rc = duk_js_delvar_activation(thr, act, name);
			DUK__REPLACE_BOOL_A_BREAK(rc);
		}

		case DUK_OP_JUMP: {
			/* Note: without explicit cast to signed, MSVC will
			 * apparently generate a large positive jump when the
			 * bias-corrected value would normally be negative.
			 */
			curr_pc += (duk_int_fast_t) DUK_DEC_ABC(ins) - (duk_int_fast_t) DUK_BC_JUMP_BIAS;
			break;
		}

#define DUK__RETURN_SHARED() \
	do { \
		duk_small_uint_t ret_result; \
		/* duk__handle_return() is guaranteed never to throw, except \
		 * for potential out-of-memory situations which will then \
		 * propagate out of the executor longjmp handler. \
		 */ \
		DUK_ASSERT(thr->ptr_curr_pc == NULL); \
		ret_result = duk__handle_return(thr, entry_act); \
		if (ret_result == DUK__RETHAND_RESTART) { \
			goto restart_execution; \
		} \
		DUK_ASSERT(ret_result == DUK__RETHAND_FINISHED); \
		return; \
	} while (0)
#if defined(DUK_USE_EXEC_PREFER_SIZE)
		case DUK_OP_RETREG:
		case DUK_OP_RETCONST:
		case DUK_OP_RETCONSTN:
		case DUK_OP_RETUNDEF: {
			/* BC -> return value reg/const */

			DUK__SYNC_AND_NULL_CURR_PC();

			if (op == DUK_OP_RETREG) {
				duk_push_tval(thr, DUK__REGP_BC(ins));
			} else if (op == DUK_OP_RETUNDEF) {
				DUK_ASSERT(DUK_TVAL_IS_UNDEFINED(thr->valstack_top)); /* valstack policy */
				thr->valstack_top++;
			} else {
				DUK_ASSERT(op == DUK_OP_RETCONST || op == DUK_OP_RETCONSTN);
				duk_push_tval(thr, DUK__CONSTP_BC(ins));
			}

			DUK__RETURN_SHARED();
		}
#else /* DUK_USE_EXEC_PREFER_SIZE */
		case DUK_OP_RETREG: {
			duk_tval *tv;

			DUK__SYNC_AND_NULL_CURR_PC();
			tv = DUK__REGP_BC(ins);
			DUK_TVAL_SET_TVAL(thr->valstack_top, tv);
			DUK_TVAL_INCREF(thr, tv);
			thr->valstack_top++;
			DUK__RETURN_SHARED();
		}
		/* This will be unused without refcounting. */
		case DUK_OP_RETCONST: {
			duk_tval *tv;

			DUK__SYNC_AND_NULL_CURR_PC();
			tv = DUK__CONSTP_BC(ins);
			DUK_TVAL_SET_TVAL(thr->valstack_top, tv);
			DUK_TVAL_INCREF(thr, tv);
			thr->valstack_top++;
			DUK__RETURN_SHARED();
		}
		case DUK_OP_RETCONSTN: {
			duk_tval *tv;

			DUK__SYNC_AND_NULL_CURR_PC();
			tv = DUK__CONSTP_BC(ins);
			DUK_TVAL_SET_TVAL(thr->valstack_top, tv);
#if defined(DUK_USE_REFERENCE_COUNTING)
			/* Without refcounting only RETCONSTN is used. */
			DUK_ASSERT(!DUK_TVAL_IS_HEAP_ALLOCATED(tv)); /* no INCREF for this constant */
#endif
			thr->valstack_top++;
			DUK__RETURN_SHARED();
		}
		case DUK_OP_RETUNDEF: {
			DUK__SYNC_AND_NULL_CURR_PC();
			thr->valstack_top++; /* value at valstack top is already undefined by valstack policy */
			DUK_ASSERT(DUK_TVAL_IS_UNDEFINED(thr->valstack_top));
			DUK__RETURN_SHARED();
		}
#endif /* DUK_USE_EXEC_PREFER_SIZE */

		case DUK_OP_LABEL: {
			duk_activation *act;
			duk_catcher *cat;
			duk_small_uint_fast_t bc = DUK_DEC_BC(ins);

			/* Allocate catcher and populate it (must be atomic). */

			cat = duk_hthread_catcher_alloc(thr);
			DUK_ASSERT(cat != NULL);

			cat->flags = (duk_uint32_t) (DUK_CAT_TYPE_LABEL | (bc << DUK_CAT_LABEL_SHIFT));
			cat->pc_base = (duk_instr_t *) curr_pc; /* pre-incremented, points to first jump slot */
			cat->idx_base = 0; /* unused for label */
			cat->h_varname = NULL;

			act = thr->callstack_curr;
			DUK_ASSERT(act != NULL);
			cat->parent = act->cat;
			act->cat = cat;

			DUK_DDD(DUK_DDDPRINT("LABEL catcher: flags=0x%08lx, pc_base=%ld, "
			                     "idx_base=%ld, h_varname=%!O, label_id=%ld",
			                     (long) cat->flags,
			                     (long) cat->pc_base,
			                     (long) cat->idx_base,
			                     (duk_heaphdr *) cat->h_varname,
			                     (long) DUK_CAT_GET_LABEL(cat)));

			curr_pc += 2; /* skip jump slots */
			break;
		}

		case DUK_OP_ENDLABEL: {
			duk_activation *act;
#if (defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 2)) || defined(DUK_USE_ASSERTIONS)
			duk_small_uint_fast_t bc = DUK_DEC_BC(ins);
#endif
#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 2)
			DUK_DDD(DUK_DDDPRINT("ENDLABEL %ld", (long) bc));
#endif

			act = thr->callstack_curr;
			DUK_ASSERT(act->cat != NULL);
			DUK_ASSERT(DUK_CAT_GET_TYPE(act->cat) == DUK_CAT_TYPE_LABEL);
			DUK_ASSERT((duk_uint_fast_t) DUK_CAT_GET_LABEL(act->cat) == bc);
			duk_hthread_catcher_unwind_nolexenv_norz(thr, act);

			/* no need to unwind callstack */
			break;
		}

		case DUK_OP_BREAK: {
			duk_small_uint_fast_t bc = DUK_DEC_BC(ins);

			DUK__SYNC_AND_NULL_CURR_PC();
			duk__handle_break_or_continue(thr, (duk_uint_t) bc, DUK_LJ_TYPE_BREAK);
			goto restart_execution;
		}

		case DUK_OP_CONTINUE: {
			duk_small_uint_fast_t bc = DUK_DEC_BC(ins);

			DUK__SYNC_AND_NULL_CURR_PC();
			duk__handle_break_or_continue(thr, (duk_uint_t) bc, DUK_LJ_TYPE_CONTINUE);
			goto restart_execution;
		}

		/* XXX: move to helper, too large to be inline here */
		case DUK_OP_TRYCATCH: {
			duk__handle_op_trycatch(thr, ins, curr_pc);
			curr_pc += 2; /* skip jump slots */
			break;
		}

		case DUK_OP_ENDTRY: {
			curr_pc = duk__handle_op_endtry(thr, ins);
			break;
		}

		case DUK_OP_ENDCATCH: {
			duk__handle_op_endcatch(thr, ins);
			break;
		}

		case DUK_OP_ENDFIN: {
			/* Sync and NULL early. */
			DUK__SYNC_AND_NULL_CURR_PC();

			if (duk__handle_op_endfin(thr, ins, entry_act) != 0) {
				return;
			}

			/* Must restart because we NULLed out curr_pc. */
			goto restart_execution;
		}

		case DUK_OP_THROW: {
			duk_small_uint_fast_t bc = DUK_DEC_BC(ins);

			/* Note: errors are augmented when they are created, not
			 * when they are thrown.  So, don't augment here, it would
			 * break re-throwing for instance.
			 */

			/* Sync so that augmentation sees up-to-date activations, NULL
			 * thr->ptr_curr_pc so that it's not used if side effects occur
			 * in augmentation or longjmp handling.
			 */
			DUK__SYNC_AND_NULL_CURR_PC();

			duk_dup(thr, (duk_idx_t) bc);
			DUK_DDD(DUK_DDDPRINT("THROW ERROR (BYTECODE): %!dT (before throw augment)",
			                     (duk_tval *) duk_get_tval(thr, -1)));
#if defined(DUK_USE_AUGMENT_ERROR_THROW)
			duk_err_augment_error_throw(thr);
			DUK_DDD(
			    DUK_DDDPRINT("THROW ERROR (BYTECODE): %!dT (after throw augment)", (duk_tval *) duk_get_tval(thr, -1)));
#endif

			duk_err_setup_ljstate1(thr, DUK_LJ_TYPE_THROW, DUK_GET_TVAL_NEGIDX(thr, -1));
#if defined(DUK_USE_DEBUGGER_SUPPORT)
			duk_err_check_debugger_integration(thr);
#endif

			DUK_ASSERT(thr->heap->lj.jmpbuf_ptr != NULL); /* always in executor */
			duk_err_longjmp(thr);
			DUK_UNREACHABLE();
			break;
		}

		case DUK_OP_CSREG: {
			/*
			 *  Assuming a register binds to a variable declared within this
			 *  function (a declarative binding), the 'this' for the call
			 *  setup is always 'undefined'.  E5 Section 10.2.1.1.6.
			 */

			duk_small_uint_fast_t a = DUK_DEC_A(ins);
			duk_small_uint_fast_t bc = DUK_DEC_BC(ins);

			/* A -> register containing target function (not type checked here)
			 * BC -> target registers (BC, BC + 1) for call setup
			 */

#if defined(DUK_USE_PREFER_SIZE)
			duk_dup(thr, (duk_idx_t) a);
			duk_replace(thr, (duk_idx_t) bc);
			duk_to_undefined(thr, (duk_idx_t) (bc + 1));
#else
			duk_tval *tv1;
			duk_tval *tv2;
			duk_tval *tv3;
			duk_tval tv_tmp1;
			duk_tval tv_tmp2;

			tv1 = DUK__REGP(bc);
			tv2 = tv1 + 1;
			DUK_TVAL_SET_TVAL(&tv_tmp1, tv1);
			DUK_TVAL_SET_TVAL(&tv_tmp2, tv2);
			tv3 = DUK__REGP(a);
			DUK_TVAL_SET_TVAL(tv1, tv3);
			DUK_TVAL_INCREF(thr, tv1); /* no side effects */
			DUK_TVAL_SET_UNDEFINED(tv2); /* no need for incref */
			DUK_TVAL_DECREF(thr, &tv_tmp1);
			DUK_TVAL_DECREF(thr, &tv_tmp2);
#endif
			break;
		}

			/* XXX: in some cases it's faster NOT to reuse the value
			 * stack but rather copy the arguments on top of the stack
			 * (mainly when the calling value stack is large and the value
			 * stack resize would be large).
			 */

		case DUK_OP_CALL0:
		case DUK_OP_CALL1:
		case DUK_OP_CALL2:
		case DUK_OP_CALL3:
		case DUK_OP_CALL4:
		case DUK_OP_CALL5:
		case DUK_OP_CALL6:
		case DUK_OP_CALL7: {
			/* Opcode packs 4 flag bits: 1 for indirect, 3 map
			 * 1:1 to three lowest call handling flags.
			 *
			 * A -> nargs or register with nargs (indirect)
			 * BC -> base register for call (base -> func, base+1 -> this, base+2 -> arg1 ... base+2+N-1 -> argN)
			 */

			duk_idx_t nargs;
			duk_idx_t idx;
			duk_small_uint_t call_flags;
#if !defined(DUK_USE_EXEC_FUN_LOCAL)
			duk_hcompfunc *fun;
#endif

			DUK_ASSERT((DUK_OP_CALL0 & 0x0fU) == 0);
			DUK_ASSERT((ins & DUK_BC_CALL_FLAG_INDIRECT) == 0);

			nargs = (duk_idx_t) DUK_DEC_A(ins);
			call_flags = (ins & 0x07U) | DUK_CALL_FLAG_ALLOW_ECMATOECMA;
			idx = (duk_idx_t) DUK_DEC_BC(ins);

			if (duk__executor_handle_call(thr, idx, nargs, call_flags)) {
				/* curr_pc synced by duk_handle_call_unprotected() */
				DUK_ASSERT(thr->ptr_curr_pc == NULL);
				goto restart_execution;
			}
			DUK_ASSERT(thr->ptr_curr_pc != NULL);

			/* duk_js_call.c is required to restore the stack reserve
			 * so we only need to reset the top.
			 */
#if !defined(DUK_USE_EXEC_FUN_LOCAL)
			fun = DUK__FUN();
#endif
			duk_set_top_unsafe(thr, (duk_idx_t) fun->nregs);

			/* No need to reinit setjmp() catchpoint, as call handling
			 * will store and restore our state.
			 *
			 * When debugger is enabled, we need to recheck the activation
			 * status after returning.  This is now handled by call handling
			 * and heap->dbg_force_restart.
			 */
			break;
		}

		case DUK_OP_CALL8:
		case DUK_OP_CALL9:
		case DUK_OP_CALL10:
		case DUK_OP_CALL11:
		case DUK_OP_CALL12:
		case DUK_OP_CALL13:
		case DUK_OP_CALL14:
		case DUK_OP_CALL15: {
			/* Indirect variant. */
			duk_uint_fast_t nargs;
			duk_idx_t idx;
			duk_small_uint_t call_flags;
#if !defined(DUK_USE_EXEC_FUN_LOCAL)
			duk_hcompfunc *fun;
#endif

			DUK_ASSERT((DUK_OP_CALL0 & 0x0fU) == 0);
			DUK_ASSERT((ins & DUK_BC_CALL_FLAG_INDIRECT) != 0);

			nargs = (duk_uint_fast_t) DUK_DEC_A(ins);
			DUK__LOOKUP_INDIRECT(nargs);
			call_flags = (ins & 0x07U) | DUK_CALL_FLAG_ALLOW_ECMATOECMA;
			idx = (duk_idx_t) DUK_DEC_BC(ins);

			if (duk__executor_handle_call(thr, idx, (duk_idx_t) nargs, call_flags)) {
				DUK_ASSERT(thr->ptr_curr_pc == NULL);
				goto restart_execution;
			}
			DUK_ASSERT(thr->ptr_curr_pc != NULL);

#if !defined(DUK_USE_EXEC_FUN_LOCAL)
			fun = DUK__FUN();
#endif
			duk_set_top_unsafe(thr, (duk_idx_t) fun->nregs);
			break;
		}

		case DUK_OP_NEWOBJ: {
			duk_push_object(thr);
#if defined(DUK_USE_ASSERTIONS)
			{
				duk_hobject *h;
				h = duk_require_hobject(thr, -1);
				DUK_ASSERT(DUK_HOBJECT_GET_ESIZE(h) == 0);
				DUK_ASSERT(DUK_HOBJECT_GET_ENEXT(h) == 0);
				DUK_ASSERT(DUK_HOBJECT_GET_ASIZE(h) == 0);
				DUK_ASSERT(DUK_HOBJECT_GET_HSIZE(h) == 0);
			}
#endif
#if !defined(DUK_USE_PREFER_SIZE)
			/* XXX: could do a direct props realloc, but need hash size */
			duk_hobject_resize_entrypart(thr, duk_known_hobject(thr, -1), DUK_DEC_A(ins));
#endif
			DUK__REPLACE_TOP_BC_BREAK();
		}

		case DUK_OP_NEWARR: {
			duk_push_array(thr);
#if defined(DUK_USE_ASSERTIONS)
			{
				duk_hobject *h;
				h = duk_require_hobject(thr, -1);
				DUK_ASSERT(DUK_HOBJECT_GET_ESIZE(h) == 0);
				DUK_ASSERT(DUK_HOBJECT_GET_ENEXT(h) == 0);
				DUK_ASSERT(DUK_HOBJECT_GET_ASIZE(h) == 0);
				DUK_ASSERT(DUK_HOBJECT_GET_HSIZE(h) == 0);
				DUK_ASSERT(DUK_HOBJECT_HAS_ARRAY_PART(h));
			}
#endif
#if !defined(DUK_USE_PREFER_SIZE)
			duk_hobject_realloc_props(thr,
			                          duk_known_hobject(thr, -1),
			                          0 /*new_e_size*/,
			                          DUK_DEC_A(ins) /*new_a_size*/,
			                          0 /*new_h_size*/,
			                          0 /*abandon_array*/);
#if 0
			duk_hobject_resize_arraypart(thr, duk_known_hobject(thr, -1), DUK_DEC_A(ins));
#endif
#endif
			DUK__REPLACE_TOP_BC_BREAK();
		}

		case DUK_OP_MPUTOBJ:
		case DUK_OP_MPUTOBJI: {
			duk_idx_t obj_idx;
			duk_uint_fast_t idx, idx_end;
			duk_small_uint_fast_t count;

			/* A -> register of target object
			 * B -> first register of key/value pair list
			 *      or register containing first register number if indirect
			 * C -> number of key/value pairs * 2
			 *      (= number of value stack indices used starting from 'B')
			 */

			obj_idx = DUK_DEC_A(ins);
			DUK_ASSERT(duk_is_object(thr, obj_idx));

			idx = (duk_uint_fast_t) DUK_DEC_B(ins);
			if (DUK_DEC_OP(ins) == DUK_OP_MPUTOBJI) {
				DUK__LOOKUP_INDIRECT(idx);
			}

			count = (duk_small_uint_fast_t) DUK_DEC_C(ins);
			DUK_ASSERT(count > 0); /* compiler guarantees */
			idx_end = idx + count;

#if defined(DUK_USE_EXEC_INDIRECT_BOUND_CHECK)
			if (DUK_UNLIKELY(idx_end > (duk_uint_fast_t) duk_get_top(thr))) {
				/* XXX: use duk_is_valid_index() instead? */
				/* XXX: improve check; check against nregs, not against top */
				DUK__INTERNAL_ERROR("MPUTOBJ out of bounds");
			}
#endif

			/* Use 'force' flag to duk_def_prop() to ensure that any
			 * inherited properties don't prevent the operation.
			 * With ES2015 duplicate properties are allowed, so that we
			 * must overwrite any previous data or accessor property.
			 *
			 * With ES2015 computed property names the literal keys
			 * may be arbitrary values and need to be ToPropertyKey()
			 * coerced at runtime.
			 */
			do {
				/* XXX: faster initialization (direct access or better primitives) */
				duk_dup(thr, (duk_idx_t) idx);
				duk_dup(thr, (duk_idx_t) (idx + 1));
				duk_def_prop(thr,
				             obj_idx,
				             DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_FORCE | DUK_DEFPROP_SET_WRITABLE |
				                 DUK_DEFPROP_SET_ENUMERABLE | DUK_DEFPROP_SET_CONFIGURABLE);
				idx += 2;
			} while (idx < idx_end);
			break;
		}

		case DUK_OP_INITSET:
		case DUK_OP_INITGET: {
			duk__handle_op_initset_initget(thr, ins);
			break;
		}

		case DUK_OP_MPUTARR:
		case DUK_OP_MPUTARRI: {
			duk_idx_t obj_idx;
			duk_uint_fast_t idx, idx_end;
			duk_small_uint_fast_t count;
			duk_tval *tv1;
			duk_uint32_t arr_idx;

			/* A -> register of target object
			 * B -> first register of value data (start_index, value1, value2, ..., valueN)
			 *      or register containing first register number if indirect
			 * C -> number of key/value pairs (N)
			 */

			obj_idx = DUK_DEC_A(ins);
			DUK_ASSERT(duk_is_object(thr, obj_idx));

			idx = (duk_uint_fast_t) DUK_DEC_B(ins);
			if (DUK_DEC_OP(ins) == DUK_OP_MPUTARRI) {
				DUK__LOOKUP_INDIRECT(idx);
			}

			count = (duk_small_uint_fast_t) DUK_DEC_C(ins);
			DUK_ASSERT(count > 0 + 1); /* compiler guarantees */
			idx_end = idx + count;

#if defined(DUK_USE_EXEC_INDIRECT_BOUND_CHECK)
			if (idx_end > (duk_uint_fast_t) duk_get_top(thr)) {
				/* XXX: use duk_is_valid_index() instead? */
				/* XXX: improve check; check against nregs, not against top */
				DUK__INTERNAL_ERROR("MPUTARR out of bounds");
			}
#endif

			tv1 = DUK__REGP(idx);
			DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv1));
#if defined(DUK_USE_FASTINT)
			DUK_ASSERT(DUK_TVAL_IS_FASTINT(tv1));
			arr_idx = (duk_uint32_t) DUK_TVAL_GET_FASTINT_U32(tv1);
#else
			arr_idx = (duk_uint32_t) DUK_TVAL_GET_NUMBER(tv1);
#endif
			idx++;

			do {
				/* duk_xdef_prop() will define an own property without any array
				 * special behaviors.  We'll need to set the array length explicitly
				 * in the end.  For arrays with elisions, the compiler will emit an
				 * explicit SETALEN which will update the length.
				 */

				/* XXX: because we're dealing with 'own' properties of a fresh array,
				 * the array initializer should just ensure that the array has a large
				 * enough array part and write the values directly into array part,
				 * and finally set 'length' manually in the end (as already happens now).
				 */

				duk_dup(thr, (duk_idx_t) idx);
				duk_xdef_prop_index_wec(thr, obj_idx, arr_idx);

				idx++;
				arr_idx++;
			} while (idx < idx_end);

			/* XXX: E5.1 Section 11.1.4 coerces the final length through
			 * ToUint32() which is odd but happens now as a side effect of
			 * 'arr_idx' type.
			 */
			duk_set_length(thr, obj_idx, (duk_size_t) (duk_uarridx_t) arr_idx);
			break;
		}

		case DUK_OP_SETALEN: {
			duk_tval *tv1;
			duk_hobject *h;
			duk_uint32_t len;

			tv1 = DUK__REGP_A(ins);
			DUK_ASSERT(DUK_TVAL_IS_OBJECT(tv1));
			h = DUK_TVAL_GET_OBJECT(tv1);
			DUK_ASSERT(DUK_HOBJECT_IS_ARRAY(h));

			tv1 = DUK__REGP_BC(ins);
			DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv1));
#if defined(DUK_USE_FASTINT)
			DUK_ASSERT(DUK_TVAL_IS_FASTINT(tv1));
			len = (duk_uint32_t) DUK_TVAL_GET_FASTINT_U32(tv1);
#else
			len = (duk_uint32_t) DUK_TVAL_GET_NUMBER(tv1);
#endif
			((duk_harray *) h)->length = len;
			break;
		}

		case DUK_OP_INITENUM: {
			duk__handle_op_initenum(thr, ins);
			break;
		}

		case DUK_OP_NEXTENUM: {
			curr_pc += duk__handle_op_nextenum(thr, ins);
			break;
		}

		case DUK_OP_INVLHS: {
			DUK_ERROR_REFERENCE(thr, DUK_STR_INVALID_LVALUE);
			DUK_WO_NORETURN(return;);
			break;
		}

		case DUK_OP_DEBUGGER: {
			/* Opcode only emitted by compiler when debugger
			 * support is enabled.  Ignore it silently without
			 * debugger support, in case it has been loaded
			 * from precompiled bytecode.
			 */
#if defined(DUK_USE_DEBUGGER_SUPPORT)
			if (duk_debug_is_attached(thr->heap)) {
				DUK_D(DUK_DPRINT("DEBUGGER statement encountered, halt execution"));
				DUK__SYNC_AND_NULL_CURR_PC();
				duk_debug_halt_execution(thr, 1 /*use_prev_pc*/);
				DUK_D(DUK_DPRINT("DEBUGGER statement finished, resume execution"));
				goto restart_execution;
			} else {
				DUK_D(DUK_DPRINT("DEBUGGER statement ignored, debugger not attached"));
			}
#else
			DUK_D(DUK_DPRINT("DEBUGGER statement ignored, no debugger support"));
#endif
			break;
		}

		case DUK_OP_NOP: {
			/* Nop, ignored, but ABC fields may carry a value e.g.
			 * for indirect opcode handling.
			 */
			break;
		}

		case DUK_OP_INVALID: {
			DUK_ERROR_FMT1(thr, DUK_ERR_ERROR, "INVALID opcode (%ld)", (long) DUK_DEC_ABC(ins));
			DUK_WO_NORETURN(return;);
			break;
		}

#if defined(DUK_USE_ES6)
		case DUK_OP_NEWTARGET: {
			duk_push_new_target(thr);
			DUK__REPLACE_TOP_BC_BREAK();
		}
#endif /* DUK_USE_ES6 */

#if !defined(DUK_USE_EXEC_PREFER_SIZE)
#if !defined(DUK_USE_ES7_EXP_OPERATOR)
		case DUK_OP_EXP_RR:
		case DUK_OP_EXP_CR:
		case DUK_OP_EXP_RC:
		case DUK_OP_EXP_CC:
#endif
#if !defined(DUK_USE_ES6)
		case DUK_OP_NEWTARGET:
#endif
#if !defined(DUK_USE_VERBOSE_ERRORS)
		case DUK_OP_GETPROPC_RR:
		case DUK_OP_GETPROPC_CR:
		case DUK_OP_GETPROPC_RC:
		case DUK_OP_GETPROPC_CC:
#endif
		case DUK_OP_UNUSED207:
		case DUK_OP_UNUSED212:
		case DUK_OP_UNUSED213:
		case DUK_OP_UNUSED214:
		case DUK_OP_UNUSED215:
		case DUK_OP_UNUSED216:
		case DUK_OP_UNUSED217:
		case DUK_OP_UNUSED218:
		case DUK_OP_UNUSED219:
		case DUK_OP_UNUSED220:
		case DUK_OP_UNUSED221:
		case DUK_OP_UNUSED222:
		case DUK_OP_UNUSED223:
		case DUK_OP_UNUSED224:
		case DUK_OP_UNUSED225:
		case DUK_OP_UNUSED226:
		case DUK_OP_UNUSED227:
		case DUK_OP_UNUSED228:
		case DUK_OP_UNUSED229:
		case DUK_OP_UNUSED230:
		case DUK_OP_UNUSED231:
		case DUK_OP_UNUSED232:
		case DUK_OP_UNUSED233:
		case DUK_OP_UNUSED234:
		case DUK_OP_UNUSED235:
		case DUK_OP_UNUSED236:
		case DUK_OP_UNUSED237:
		case DUK_OP_UNUSED238:
		case DUK_OP_UNUSED239:
		case DUK_OP_UNUSED240:
		case DUK_OP_UNUSED241:
		case DUK_OP_UNUSED242:
		case DUK_OP_UNUSED243:
		case DUK_OP_UNUSED244:
		case DUK_OP_UNUSED245:
		case DUK_OP_UNUSED246:
		case DUK_OP_UNUSED247:
		case DUK_OP_UNUSED248:
		case DUK_OP_UNUSED249:
		case DUK_OP_UNUSED250:
		case DUK_OP_UNUSED251:
		case DUK_OP_UNUSED252:
		case DUK_OP_UNUSED253:
		case DUK_OP_UNUSED254:
		case DUK_OP_UNUSED255:
			/* Force all case clauses to map to an actual handler
			 * so that the compiler can emit a jump without a bounds
			 * check: the switch argument is a duk_uint8_t so that
			 * the compiler may be able to figure it out.  This is
			 * a small detail and obviously compiler dependent.
			 */
			/* default: clause omitted on purpose */
#else /* DUK_USE_EXEC_PREFER_SIZE */
		default:
#endif /* DUK_USE_EXEC_PREFER_SIZE */
		{
			/* Default case catches invalid/unsupported opcodes. */
			DUK_D(DUK_DPRINT("invalid opcode: %ld - %!I", (long) op, ins));
			DUK__INTERNAL_ERROR("invalid opcode");
			break;
		}

		} /* end switch */

		continue;

		/* Some shared exit paths for opcode handling below.  These
		 * are mostly useful to reduce code footprint when multiple
		 * opcodes have a similar epilogue (like replacing stack top
		 * with index 'a').
		 */

#if defined(DUK_USE_EXEC_PREFER_SIZE)
	replace_top_a:
		DUK__REPLACE_TO_TVPTR(thr, DUK__REGP_A(ins));
		continue;
	replace_top_bc:
		DUK__REPLACE_TO_TVPTR(thr, DUK__REGP_BC(ins));
		continue;
#endif
	}
	DUK_WO_NORETURN(return;);

#if !defined(DUK_USE_VERBOSE_EXECUTOR_ERRORS)
internal_error:
	DUK_ERROR_INTERNAL(thr);
	DUK_WO_NORETURN(return;);
#endif
}
