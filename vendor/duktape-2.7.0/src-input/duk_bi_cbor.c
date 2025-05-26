/*
 *  CBOR bindings.
 *
 *  http://cbor.io/
 *  https://tools.ietf.org/html/rfc7049
 */

#include "duk_internal.h"

#if defined(DUK_USE_CBOR_SUPPORT)

/* #define DUK_CBOR_STRESS */

/* Default behavior for encoding strings: use CBOR text string if string
 * is UTF-8 compatible, otherwise use CBOR byte string.  These defines
 * can be used to force either type for all strings.  Using text strings
 * for non-UTF-8 data is technically invalid CBOR.
 */
/* #define DUK_CBOR_TEXT_STRINGS */
/* #define DUK_CBOR_BYTE_STRINGS */

/* Misc. defines. */
/* #define DUK_CBOR_PREFER_SIZE */
/* #define DUK_CBOR_DOUBLE_AS_IS */
/* #define DUK_CBOR_DECODE_FASTPATH */

typedef struct {
	duk_hthread *thr;
	duk_uint8_t *ptr;
	duk_uint8_t *buf;
	duk_uint8_t *buf_end;
	duk_size_t len;
	duk_idx_t idx_buf;
	duk_uint_t recursion_depth;
	duk_uint_t recursion_limit;
} duk_cbor_encode_context;

typedef struct {
	duk_hthread *thr;
	const duk_uint8_t *buf;
	duk_size_t off;
	duk_size_t len;
	duk_uint_t recursion_depth;
	duk_uint_t recursion_limit;
} duk_cbor_decode_context;

DUK_LOCAL void duk__cbor_encode_value(duk_cbor_encode_context *enc_ctx);
DUK_LOCAL void duk__cbor_decode_value(duk_cbor_decode_context *dec_ctx);

/*
 *  Misc
 */

DUK_LOCAL duk_uint32_t duk__cbor_double_to_uint32(double d) {
	/* Out of range casts are undefined behavior, so caller must avoid. */
	DUK_ASSERT(d >= 0.0 && d <= 4294967295.0);
	return (duk_uint32_t) d;
}

/*
 *  Encoding
 */

DUK_LOCAL void duk__cbor_encode_error(duk_cbor_encode_context *enc_ctx) {
	(void) duk_type_error(enc_ctx->thr, "cbor encode error");
}

DUK_LOCAL void duk__cbor_encode_req_stack(duk_cbor_encode_context *enc_ctx) {
	duk_require_stack(enc_ctx->thr, 4);
}

DUK_LOCAL void duk__cbor_encode_objarr_entry(duk_cbor_encode_context *enc_ctx) {
	duk_hthread *thr = enc_ctx->thr;

	/* Native stack check in object/array recursion. */
	duk_native_stack_check(thr);

	/* When working with deeply recursive structures, this is important
	 * to ensure there's no effective depth limit.
	 */
	duk__cbor_encode_req_stack(enc_ctx);

	DUK_ASSERT(enc_ctx->recursion_depth <= enc_ctx->recursion_limit);
	if (enc_ctx->recursion_depth >= enc_ctx->recursion_limit) {
		DUK_ERROR_RANGE(thr, DUK_STR_ENC_RECLIMIT);
		DUK_WO_NORETURN(return;);
	}
	enc_ctx->recursion_depth++;
}

DUK_LOCAL void duk__cbor_encode_objarr_exit(duk_cbor_encode_context *enc_ctx) {
	DUK_ASSERT(enc_ctx->recursion_depth > 0);
	enc_ctx->recursion_depth--;
}

/* Check that a size_t is in uint32 range to avoid out-of-range casts. */
DUK_LOCAL void duk__cbor_encode_sizet_uint32_check(duk_cbor_encode_context *enc_ctx, duk_size_t len) {
	if (DUK_UNLIKELY(sizeof(duk_size_t) > sizeof(duk_uint32_t) && len > (duk_size_t) DUK_UINT32_MAX)) {
		duk__cbor_encode_error(enc_ctx);
	}
}

DUK_LOCAL DUK_NOINLINE void duk__cbor_encode_ensure_slowpath(duk_cbor_encode_context *enc_ctx, duk_size_t len) {
	duk_size_t oldlen;
	duk_size_t minlen;
	duk_size_t newlen;
	duk_uint8_t *p_new;
	duk_size_t old_data_len;

	DUK_ASSERT(enc_ctx->ptr >= enc_ctx->buf);
	DUK_ASSERT(enc_ctx->buf_end >= enc_ctx->ptr);
	DUK_ASSERT(enc_ctx->buf_end >= enc_ctx->buf);

	/* Overflow check.
	 *
	 * Limit example: 0xffffffffUL / 2U = 0x7fffffffUL, we reject >= 0x80000000UL.
	 */
	oldlen = enc_ctx->len;
	minlen = oldlen + len;
	if (DUK_UNLIKELY(oldlen > DUK_SIZE_MAX / 2U || minlen < oldlen)) {
		duk__cbor_encode_error(enc_ctx);
	}

#if defined(DUK_CBOR_STRESS)
	newlen = oldlen + 1U;
#else
	newlen = oldlen * 2U;
#endif
	DUK_ASSERT(newlen >= oldlen);

	if (minlen > newlen) {
		newlen = minlen;
	}
	DUK_ASSERT(newlen >= oldlen);
	DUK_ASSERT(newlen >= minlen);
	DUK_ASSERT(newlen > 0U);

	DUK_DD(DUK_DDPRINT("cbor encode buffer resized to %ld", (long) newlen));

	p_new = (duk_uint8_t *) duk_resize_buffer(enc_ctx->thr, enc_ctx->idx_buf, newlen);
	DUK_ASSERT(p_new != NULL);
	old_data_len = (duk_size_t) (enc_ctx->ptr - enc_ctx->buf);
	enc_ctx->buf = p_new;
	enc_ctx->buf_end = p_new + newlen;
	enc_ctx->ptr = p_new + old_data_len;
	enc_ctx->len = newlen;
}

DUK_LOCAL DUK_INLINE void duk__cbor_encode_ensure(duk_cbor_encode_context *enc_ctx, duk_size_t len) {
	if (DUK_LIKELY((duk_size_t) (enc_ctx->buf_end - enc_ctx->ptr) >= len)) {
		return;
	}
	duk__cbor_encode_ensure_slowpath(enc_ctx, len);
}

DUK_LOCAL duk_size_t duk__cbor_get_reserve(duk_cbor_encode_context *enc_ctx) {
	DUK_ASSERT(enc_ctx->ptr >= enc_ctx->buf);
	DUK_ASSERT(enc_ctx->ptr <= enc_ctx->buf_end);
	return (duk_size_t) (enc_ctx->buf_end - enc_ctx->ptr);
}

DUK_LOCAL void duk__cbor_encode_uint32(duk_cbor_encode_context *enc_ctx, duk_uint32_t u, duk_uint8_t base) {
	duk_uint8_t *p;

	/* Caller must ensure space. */
	DUK_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 4);

	p = enc_ctx->ptr;
	if (DUK_LIKELY(u <= 23U)) {
		*p++ = (duk_uint8_t) (base + (duk_uint8_t) u);
	} else if (u <= 0xffUL) {
		*p++ = base + 0x18U;
		*p++ = (duk_uint8_t) u;
	} else if (u <= 0xffffUL) {
		*p++ = base + 0x19U;
		DUK_RAW_WRITEINC_U16_BE(p, (duk_uint16_t) u);
	} else {
		*p++ = base + 0x1aU;
		DUK_RAW_WRITEINC_U32_BE(p, u);
	}
	enc_ctx->ptr = p;
}

#if defined(DUK_CBOR_DOUBLE_AS_IS)
DUK_LOCAL void duk__cbor_encode_double(duk_cbor_encode_context *enc_ctx, double d) {
	duk_uint8_t *p;

	/* Caller must ensure space. */
	DUK_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 8);

	p = enc_ctx->ptr;
	*p++ = 0xfbU;
	DUK_RAW_WRITEINC_DOUBLE_BE(p, d);
	p += 8;
	enc_ctx->ptr = p;
}
#else /* DUK_CBOR_DOUBLE_AS_IS */
DUK_LOCAL void duk__cbor_encode_double_fp(duk_cbor_encode_context *enc_ctx, double d) {
	duk_double_union u;
	duk_uint16_t u16;
	duk_int16_t expt;
	duk_uint8_t *p;

	DUK_ASSERT(DUK_FPCLASSIFY(d) != DUK_FP_ZERO);

	/* Caller must ensure space. */
	DUK_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 8);

	/* Organize into little endian (no-op if platform is little endian). */
	u.d = d;
	duk_dblunion_host_to_little(&u);

	/* Check if 'd' can represented as a normal half-float.
	 * Denormal half-floats could also be used, but that check
	 * isn't done now (denormal half-floats are decoded of course).
	 * So just check exponent range and that at most 10 significant
	 * bits (excluding implicit leading 1) are used in 'd'.
	 */
	u16 = (((duk_uint16_t) u.uc[7]) << 8) | ((duk_uint16_t) u.uc[6]);
	expt = (duk_int16_t) ((u16 & 0x7ff0U) >> 4) - 1023;

	if (expt >= -14 && expt <= 15) {
		/* Half-float normal exponents (excl. denormals).
		 *
		 *          7        6        5        4        3        2        1        0  (LE index)
		 * double: seeeeeee eeeemmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm
		 * half:         seeeee mmmm mmmmmm00 00000000 00000000 00000000 00000000 00000000
		 */
		duk_bool_t use_half_float;

		use_half_float =
		    (u.uc[0] == 0 && u.uc[1] == 0 && u.uc[2] == 0 && u.uc[3] == 0 && u.uc[4] == 0 && (u.uc[5] & 0x03U) == 0);

		if (use_half_float) {
			duk_uint32_t t;

			expt += 15;
			t = (duk_uint32_t) (u.uc[7] & 0x80U) << 8;
			t += (duk_uint32_t) expt << 10;
			t += ((duk_uint32_t) u.uc[6] & 0x0fU) << 6;
			t += ((duk_uint32_t) u.uc[5]) >> 2;

			/* seeeeemm mmmmmmmm */
			p = enc_ctx->ptr;
			*p++ = 0xf9U;
			DUK_RAW_WRITEINC_U16_BE(p, (duk_uint16_t) t);
			enc_ctx->ptr = p;
			return;
		}
	}

	/* Same check for plain float.  Also no denormal support here. */
	if (expt >= -126 && expt <= 127) {
		/* Float normal exponents (excl. denormals).
		 *
		 * double: seeeeeee eeeemmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm
		 * float:     seeee eeeemmmm mmmmmmmm mmmmmmmm mmm00000 00000000 00000000 00000000
		 */
		duk_bool_t use_float;
		duk_float_t d_float;

		/* We could do this explicit mantissa check, but doing
		 * a double-float-double cast is fine because we've
		 * already verified that the exponent is in range so
		 * that the narrower cast is not undefined behavior.
		 */
#if 0
		use_float =
		    (u.uc[0] == 0 && u.uc[1] == 0 && u.uc[2] == 0 && (u.uc[3] & 0xe0U) == 0);
#endif
		d_float = (duk_float_t) d;
		use_float = duk_double_equals((duk_double_t) d_float, d);
		if (use_float) {
			p = enc_ctx->ptr;
			*p++ = 0xfaU;
			DUK_RAW_WRITEINC_FLOAT_BE(p, d_float);
			enc_ctx->ptr = p;
			return;
		}
	}

	/* Special handling for NaN and Inf which we want to encode as
	 * half-floats.  They share the same (maximum) exponent.
	 */
	if (expt == 1024) {
		DUK_ASSERT(DUK_ISNAN(d) || DUK_ISINF(d));
		p = enc_ctx->ptr;
		*p++ = 0xf9U;
		if (DUK_ISNAN(d)) {
			/* Shortest NaN encoding is using a half-float.  Lose the
			 * exact NaN bits in the process.  IEEE double would be
			 * 7ff8 0000 0000 0000, i.e. a quiet NaN in most architectures
			 * (https://en.wikipedia.org/wiki/NaN#Encoding).  The
			 * equivalent half float is 7e00.
			 */
			*p++ = 0x7eU;
		} else {
			/* Shortest +/- Infinity encoding is using a half-float. */
			if (DUK_SIGNBIT(d)) {
				*p++ = 0xfcU;
			} else {
				*p++ = 0x7cU;
			}
		}
		*p++ = 0x00U;
		enc_ctx->ptr = p;
		return;
	}

	/* Cannot use half-float or float, encode as full IEEE double. */
	p = enc_ctx->ptr;
	*p++ = 0xfbU;
	DUK_RAW_WRITEINC_DOUBLE_BE(p, d);
	enc_ctx->ptr = p;
}

DUK_LOCAL void duk__cbor_encode_double(duk_cbor_encode_context *enc_ctx, double d) {
	duk_uint8_t *p;
	double d_floor;

	/* Integers and floating point values of all types are conceptually
	 * equivalent in CBOR.  Try to always choose the shortest encoding
	 * which is not always immediately obvious.  For example, NaN and Inf
	 * can be most compactly represented as a half-float (assuming NaN
	 * bits are not preserved), and 0x1'0000'0000 as a single precision
	 * float.  Shortest forms in preference order (prefer integer over
	 * float when equal length):
	 *
	 *   uint        1 byte    [0,23] (not -0)
	 *   sint        1 byte    [-24,-1]
	 *   uint+1      2 bytes   [24,255]
	 *   sint+1      2 bytes   [-256,-25]
	 *   uint+2      3 bytes   [256,65535]
	 *   sint+2      3 bytes   [-65536,-257]
	 *   half-float  3 bytes   -0, NaN, +/- Infinity, range [-65504,65504]
	 *   uint+4      5 bytes   [65536,4294967295]
	 *   sint+4      5 bytes   [-4294967296,-258]
	 *   float       5 bytes   range [-(1 - 2^(-24)) * 2^128, (1 - 2^(-24)) * 2^128]
	 *   uint+8      9 bytes   [4294967296,18446744073709551615]
	 *   sint+8      9 bytes   [-18446744073709551616,-4294967297]
	 *   double      9 bytes
	 *
	 * For whole numbers (compatible with integers):
	 *   - 1-byte or 2-byte uint/sint representation is preferred for
	 *     [-256,255].
	 *   - 3-byte uint/sint is preferred for [-65536,65535].  Half floats
	 *     are never preferred because they have the same length.
	 *   - 5-byte uint/sint is preferred for [-4294967296,4294967295].
	 *     Single precision floats are never preferred, and half-floats
	 *     don't reach above the 3-byte uint/sint range so they're never
	 *     preferred.
	 *   - So, for all integers up to signed/unsigned 32-bit range the
	 *     preferred encoding is always an integer uint/sint.
	 *   - For integers above 32 bits the situation is more complicated.
	 *     Half-floats are never useful for them because of their limited
	 *     range, but IEEE single precision floats (5 bytes encoded) can
	 *     represent some integers between the 32-bit and 64-bit ranges
	 *     which require 9 bytes as a uint/sint.
	 *
	 * For floating point values not compatible with integers, the
	 * preferred encoding is quite clear:
	 *   - For +Inf/-Inf use half-float.
	 *   - For NaN use a half-float, assuming NaN bits ("payload") is
	 *     not worth preserving.  Duktape doesn't in general guarantee
	 *     preservation of the NaN payload so using a half-float seems
	 *     consistent with that.
	 *   - For remaining values, prefer the shortest form which doesn't
	 *     lose any precision.  For normal half-floats and single precision
	 *     floats this is simple: just check exponent and mantissa bits
	 *     using a fixed mask.  For denormal half-floats and single
	 *     precision floats the check is a bit more complicated: a normal
	 *     IEEE double can sometimes be represented as a denormal
	 *     half-float or single precision float.
	 *
	 * https://en.wikipedia.org/wiki/Half-precision_floating-point_format#IEEE_754_half-precision_binary_floating-point_format:_binary16
	 */

	/* Caller must ensure space. */
	DUK_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 8);

	/* Most important path is integers.  The floor() test will be true
	 * for Inf too (but not NaN).
	 */
	d_floor = DUK_FLOOR(d); /* identity if d is +/- 0.0, NaN, or +/- Infinity */
	if (DUK_LIKELY(duk_double_equals(d_floor, d) != 0)) {
		DUK_ASSERT(!DUK_ISNAN(d)); /* NaN == NaN compares false. */
		if (DUK_SIGNBIT(d)) {
			if (d >= -4294967296.0) {
				d = -1.0 - d;
				if (d >= 0.0) {
					DUK_ASSERT(d >= 0.0);
					duk__cbor_encode_uint32(enc_ctx, duk__cbor_double_to_uint32(d), 0x20U);
					return;
				}

				/* Input was negative zero, d == -1.0 < 0.0.
				 * Shortest -0 is using half-float.
				 */
				p = enc_ctx->ptr;
				*p++ = 0xf9U;
				*p++ = 0x80U;
				*p++ = 0x00U;
				enc_ctx->ptr = p;
				return;
			}
		} else {
			if (d <= 4294967295.0) {
				/* Positive zero needs no special handling. */
				DUK_ASSERT(d >= 0.0);
				duk__cbor_encode_uint32(enc_ctx, duk__cbor_double_to_uint32(d), 0x00U);
				return;
			}
		}
	}

	/* 64-bit integers are not supported at present.  So
	 * we also don't need to deal with choosing between a
	 * 64-bit uint/sint representation vs. IEEE double or
	 * float.
	 */

	DUK_ASSERT(DUK_FPCLASSIFY(d) != DUK_FP_ZERO);
	duk__cbor_encode_double_fp(enc_ctx, d);
}
#endif /* DUK_CBOR_DOUBLE_AS_IS */

DUK_LOCAL void duk__cbor_encode_string_top(duk_cbor_encode_context *enc_ctx) {
	const duk_uint8_t *str;
	duk_size_t len;
	duk_uint8_t *p;

	/* CBOR differentiates between UTF-8 text strings and byte strings.
	 * Text strings MUST be valid UTF-8, so not all Duktape strings can
	 * be encoded as valid CBOR text strings.  Possible behaviors:
	 *
	 *   1. Use text string when input is valid UTF-8, otherwise use
	 *      byte string (maybe tagged to indicate it was an extended
	 *      UTF-8 string).
	 *   2. Always use text strings, but sanitize input string so that
	 *      invalid UTF-8 is replaced with U+FFFD for example.  Combine
	 *      surrogates whenever possible.
	 *   3. Always use byte strings.  This is simple and produces valid
	 *      CBOR, but isn't ideal for interoperability.
	 *   4. Always use text strings, even for invalid UTF-8 such as
	 *      codepoints in the surrogate pair range.  This is simple but
	 *      produces technically invalid CBOR for non-UTF-8 strings which
	 *      may affect interoperability.
	 *
	 * Current default is 1; can be changed with defines.
	 */

	/* Caller must ensure space. */
	DUK_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 8);

	str = (const duk_uint8_t *) duk_require_lstring(enc_ctx->thr, -1, &len);
	if (duk_is_symbol(enc_ctx->thr, -1)) {
		/* Symbols, encode as an empty table for now.  This matches
		 * the behavior of cbor-js.
		 *
		 * XXX: Maybe encode String() coercion with a tag?
		 * XXX: Option to keep enough information to recover
		 * Symbols when decoding (this is not always desirable).
		 */
		p = enc_ctx->ptr;
		*p++ = 0xa0U;
		enc_ctx->ptr = p;
		return;
	}

	duk__cbor_encode_sizet_uint32_check(enc_ctx, len);
#if defined(DUK_CBOR_TEXT_STRINGS)
	duk__cbor_encode_uint32(enc_ctx, (duk_uint32_t) len, 0x60U);
#elif defined(DUK_CBOR_BYTE_STRINGS)
	duk__cbor_encode_uint32(enc_ctx, (duk_uint32_t) len, 0x40U);
#else
	duk__cbor_encode_uint32(enc_ctx,
	                        (duk_uint32_t) len,
	                        (DUK_LIKELY(duk_unicode_is_utf8_compatible(str, len) != 0) ? 0x60U : 0x40U));
#endif
	duk__cbor_encode_ensure(enc_ctx, len);
	p = enc_ctx->ptr;
	duk_memcpy((void *) p, (const void *) str, len);
	p += len;
	enc_ctx->ptr = p;
}

DUK_LOCAL void duk__cbor_encode_object(duk_cbor_encode_context *enc_ctx) {
	duk_uint8_t *buf;
	duk_size_t len;
	duk_uint8_t *p;
	duk_size_t i;
	duk_size_t off_ib;
	duk_uint32_t count;

	/* Caller must ensure space. */
	DUK_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 8);

	duk__cbor_encode_objarr_entry(enc_ctx);

	/* XXX: Support for specific built-ins like Date and RegExp. */
	if (duk_is_array(enc_ctx->thr, -1)) {
		/* Shortest encoding for arrays >= 256 in length is actually
		 * the indefinite length one (3 or more bytes vs. 2 bytes).
		 * We still use the definite length version because it is
		 * more decoding friendly.
		 */
		len = duk_get_length(enc_ctx->thr, -1);
		duk__cbor_encode_sizet_uint32_check(enc_ctx, len);
		duk__cbor_encode_uint32(enc_ctx, (duk_uint32_t) len, 0x80U);
		for (i = 0; i < len; i++) {
			duk_get_prop_index(enc_ctx->thr, -1, (duk_uarridx_t) i);
			duk__cbor_encode_value(enc_ctx);
		}
	} else if (duk_is_buffer_data(enc_ctx->thr, -1)) {
		/* XXX: Tag buffer data?
		 * XXX: Encode typed arrays as integer arrays rather
		 * than buffer data as is?
		 */
		buf = (duk_uint8_t *) duk_require_buffer_data(enc_ctx->thr, -1, &len);
		duk__cbor_encode_sizet_uint32_check(enc_ctx, len);
		duk__cbor_encode_uint32(enc_ctx, (duk_uint32_t) len, 0x40U);
		duk__cbor_encode_ensure(enc_ctx, len);
		p = enc_ctx->ptr;
		duk_memcpy_unsafe((void *) p, (const void *) buf, len);
		p += len;
		enc_ctx->ptr = p;
	} else {
		/* We don't know the number of properties in advance
		 * but would still like to encode at least small
		 * objects without indefinite length.  Emit an
		 * indefinite length byte initially, and if the final
		 * property count is small enough to also fit in one
		 * byte, backpatch it later.  Otherwise keep the
		 * indefinite length.  This works well up to 23
		 * properties which is practical and good enough.
		 */
		off_ib = (duk_size_t) (enc_ctx->ptr - enc_ctx->buf); /* XXX: get_offset? */
		count = 0U;
		p = enc_ctx->ptr;
		*p++ = 0xa0U + 0x1fU; /* indefinite length */
		enc_ctx->ptr = p;
		duk_enum(enc_ctx->thr, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
		while (duk_next(enc_ctx->thr, -1, 1 /*get_value*/)) {
			duk_insert(enc_ctx->thr, -2); /* [ ... key value ] -> [ ... value key ] */
			duk__cbor_encode_value(enc_ctx);
			duk__cbor_encode_value(enc_ctx);
			count++;
			if (count == 0U) {
				duk__cbor_encode_error(enc_ctx);
			}
		}
		duk_pop(enc_ctx->thr);
		if (count <= 0x17U) {
			DUK_ASSERT(off_ib < enc_ctx->len);
			enc_ctx->buf[off_ib] = 0xa0U + (duk_uint8_t) count;
		} else {
			duk__cbor_encode_ensure(enc_ctx, 1);
			p = enc_ctx->ptr;
			*p++ = 0xffU; /* break */
			enc_ctx->ptr = p;
		}
	}

	duk__cbor_encode_objarr_exit(enc_ctx);
}

DUK_LOCAL void duk__cbor_encode_buffer(duk_cbor_encode_context *enc_ctx) {
	duk_uint8_t *buf;
	duk_size_t len;
	duk_uint8_t *p;

	/* Caller must ensure space. */
	DUK_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 8);

	/* Tag buffer data? */
	buf = (duk_uint8_t *) duk_require_buffer(enc_ctx->thr, -1, &len);
	duk__cbor_encode_sizet_uint32_check(enc_ctx, len);
	duk__cbor_encode_uint32(enc_ctx, (duk_uint32_t) len, 0x40U);
	duk__cbor_encode_ensure(enc_ctx, len);
	p = enc_ctx->ptr;
	duk_memcpy_unsafe((void *) p, (const void *) buf, len);
	p += len;
	enc_ctx->ptr = p;
}

DUK_LOCAL void duk__cbor_encode_pointer(duk_cbor_encode_context *enc_ctx) {
	/* Pointers (void *) are challenging to encode.  They can't
	 * be relied to be even 64-bit integer compatible (there are
	 * pointer models larger than that), nor can floats encode
	 * them.  They could be encoded as strings (%p format) but
	 * that's not portable.  They could be encoded as direct memory
	 * representations.  Recovering pointers is non-portable in any
	 * case but it would be nice to be able to detect and recover
	 * compatible pointers.
	 *
	 * For now, encode as "(%p)" string, matching JX.  There doesn't
	 * seem to be an appropriate tag, so pointers don't currently
	 * survive a CBOR encode/decode roundtrip intact.
	 */
	const char *ptr;

	ptr = duk_to_string(enc_ctx->thr, -1);
	DUK_ASSERT(ptr != NULL);
	duk_push_sprintf(enc_ctx->thr, "(%s)", ptr);
	duk_remove(enc_ctx->thr, -2);
	duk__cbor_encode_string_top(enc_ctx);
}

DUK_LOCAL void duk__cbor_encode_lightfunc(duk_cbor_encode_context *enc_ctx) {
	duk_uint8_t *p;

	/* Caller must ensure space. */
	DUK_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 8);

	/* For now encode as an empty object. */
	p = enc_ctx->ptr;
	*p++ = 0xa0U;
	enc_ctx->ptr = p;
}

DUK_LOCAL void duk__cbor_encode_value(duk_cbor_encode_context *enc_ctx) {
	duk_uint8_t *p;

	/* Encode/decode cycle currently loses some type information.
	 * This can be improved by registering custom tags with IANA.
	 */

	/* Reserve space for up to 64-bit types (1 initial byte + 8
	 * followup bytes).  This allows encoding of integers, floats,
	 * string/buffer length fields, etc without separate checks
	 * in each code path.
	 */
	duk__cbor_encode_ensure(enc_ctx, 1 + 8);

	switch (duk_get_type(enc_ctx->thr, -1)) {
	case DUK_TYPE_UNDEFINED: {
		p = enc_ctx->ptr;
		*p++ = 0xf7;
		enc_ctx->ptr = p;
		break;
	}
	case DUK_TYPE_NULL: {
		p = enc_ctx->ptr;
		*p++ = 0xf6;
		enc_ctx->ptr = p;
		break;
	}
	case DUK_TYPE_BOOLEAN: {
		duk_uint8_t u8 = duk_get_boolean(enc_ctx->thr, -1) ? 0xf5U : 0xf4U;
		p = enc_ctx->ptr;
		*p++ = u8;
		enc_ctx->ptr = p;
		break;
	}
	case DUK_TYPE_NUMBER: {
		duk__cbor_encode_double(enc_ctx, duk_get_number(enc_ctx->thr, -1));
		break;
	}
	case DUK_TYPE_STRING: {
		duk__cbor_encode_string_top(enc_ctx);
		break;
	}
	case DUK_TYPE_OBJECT: {
		duk__cbor_encode_object(enc_ctx);
		break;
	}
	case DUK_TYPE_BUFFER: {
		duk__cbor_encode_buffer(enc_ctx);
		break;
	}
	case DUK_TYPE_POINTER: {
		duk__cbor_encode_pointer(enc_ctx);
		break;
	}
	case DUK_TYPE_LIGHTFUNC: {
		duk__cbor_encode_lightfunc(enc_ctx);
		break;
	}
	case DUK_TYPE_NONE:
	default:
		goto fail;
	}

	duk_pop(enc_ctx->thr);
	return;

fail:
	duk__cbor_encode_error(enc_ctx);
}

/*
 *  Decoding
 */

DUK_LOCAL void duk__cbor_decode_error(duk_cbor_decode_context *dec_ctx) {
	(void) duk_type_error(dec_ctx->thr, "cbor decode error");
}

DUK_LOCAL void duk__cbor_decode_req_stack(duk_cbor_decode_context *dec_ctx) {
	duk_require_stack(dec_ctx->thr, 4);
}

DUK_LOCAL void duk__cbor_decode_objarr_entry(duk_cbor_decode_context *dec_ctx) {
	duk_hthread *thr = dec_ctx->thr;

	/* Native stack check in object/array recursion. */
	duk_native_stack_check(thr);

	duk__cbor_decode_req_stack(dec_ctx);

	DUK_ASSERT(dec_ctx->recursion_depth <= dec_ctx->recursion_limit);
	if (dec_ctx->recursion_depth >= dec_ctx->recursion_limit) {
		DUK_ERROR_RANGE(thr, DUK_STR_DEC_RECLIMIT);
		DUK_WO_NORETURN(return;);
	}
	dec_ctx->recursion_depth++;
}

DUK_LOCAL void duk__cbor_decode_objarr_exit(duk_cbor_decode_context *dec_ctx) {
	DUK_ASSERT(dec_ctx->recursion_depth > 0);
	dec_ctx->recursion_depth--;
}

DUK_LOCAL duk_uint8_t duk__cbor_decode_readbyte(duk_cbor_decode_context *dec_ctx) {
	DUK_ASSERT(dec_ctx->off <= dec_ctx->len);
	if (DUK_UNLIKELY(dec_ctx->len - dec_ctx->off < 1U)) {
		duk__cbor_decode_error(dec_ctx);
	}
	return dec_ctx->buf[dec_ctx->off++];
}

DUK_LOCAL duk_uint16_t duk__cbor_decode_read_u16(duk_cbor_decode_context *dec_ctx) {
	duk_uint16_t res;

	DUK_ASSERT(dec_ctx->off <= dec_ctx->len);
	if (DUK_UNLIKELY(dec_ctx->len - dec_ctx->off < 2U)) {
		duk__cbor_decode_error(dec_ctx);
	}
	res = DUK_RAW_READ_U16_BE(dec_ctx->buf + dec_ctx->off);
	dec_ctx->off += 2;
	return res;
}

DUK_LOCAL duk_uint32_t duk__cbor_decode_read_u32(duk_cbor_decode_context *dec_ctx) {
	duk_uint32_t res;

	DUK_ASSERT(dec_ctx->off <= dec_ctx->len);
	if (DUK_UNLIKELY(dec_ctx->len - dec_ctx->off < 4U)) {
		duk__cbor_decode_error(dec_ctx);
	}
	res = DUK_RAW_READ_U32_BE(dec_ctx->buf + dec_ctx->off);
	dec_ctx->off += 4;
	return res;
}

DUK_LOCAL duk_uint8_t duk__cbor_decode_peekbyte(duk_cbor_decode_context *dec_ctx) {
	if (DUK_UNLIKELY(dec_ctx->off >= dec_ctx->len)) {
		duk__cbor_decode_error(dec_ctx);
	}
	return dec_ctx->buf[dec_ctx->off];
}

DUK_LOCAL void duk__cbor_decode_rewind(duk_cbor_decode_context *dec_ctx, duk_size_t len) {
	DUK_ASSERT(len <= dec_ctx->off); /* Caller must ensure. */
	dec_ctx->off -= len;
}

#if 0
DUK_LOCAL void duk__cbor_decode_ensure(duk_cbor_decode_context *dec_ctx, duk_size_t len) {
	if (dec_ctx->off + len > dec_ctx->len) {
		duk__cbor_decode_error(dec_ctx);
	}
}
#endif

DUK_LOCAL const duk_uint8_t *duk__cbor_decode_consume(duk_cbor_decode_context *dec_ctx, duk_size_t len) {
	DUK_ASSERT(dec_ctx->off <= dec_ctx->len);
	if (DUK_LIKELY(dec_ctx->len - dec_ctx->off >= len)) {
		const duk_uint8_t *res = dec_ctx->buf + dec_ctx->off;
		dec_ctx->off += len;
		return res;
	}

	duk__cbor_decode_error(dec_ctx); /* Not enough input. */
	return NULL;
}

DUK_LOCAL int duk__cbor_decode_checkbreak(duk_cbor_decode_context *dec_ctx) {
	if (duk__cbor_decode_peekbyte(dec_ctx) == 0xffU) {
		DUK_ASSERT(dec_ctx->off < dec_ctx->len);
		dec_ctx->off++;
#if 0
		(void) duk__cbor_decode_readbyte(dec_ctx);
#endif
		return 1;
	}
	return 0;
}

DUK_LOCAL void duk__cbor_decode_push_aival_int(duk_cbor_decode_context *dec_ctx, duk_uint8_t ib, duk_bool_t negative) {
	duk_uint8_t ai;
	duk_uint32_t t, t1, t2;
#if 0
	duk_uint64_t t3;
#endif
	duk_double_t d1, d2;
	duk_double_t d;

	ai = ib & 0x1fU;
	if (ai <= 0x17U) {
		t = ai;
		goto shared_exit;
	}

	switch (ai) {
	case 0x18U: /* 1 byte */
		t = (duk_uint32_t) duk__cbor_decode_readbyte(dec_ctx);
		goto shared_exit;
	case 0x19U: /* 2 byte */
		t = (duk_uint32_t) duk__cbor_decode_read_u16(dec_ctx);
		goto shared_exit;
	case 0x1aU: /* 4 byte */
		t = (duk_uint32_t) duk__cbor_decode_read_u32(dec_ctx);
		goto shared_exit;
	case 0x1bU: /* 8 byte */
		/* For uint64 it's important to handle the -1.0 part before
		 * casting to double: otherwise the adjustment might be lost
		 * in the cast.  Uses: -1.0 - d <=> -(d + 1.0).
		 */
		t = (duk_uint32_t) duk__cbor_decode_read_u32(dec_ctx);
		t2 = t;
		t = (duk_uint32_t) duk__cbor_decode_read_u32(dec_ctx);
		t1 = t;
#if 0
		t3 = (duk_uint64_t) t2 * DUK_U64_CONSTANT(0x100000000) + (duk_uint64_t) t1;
		if (negative) {
			if (t3 == DUK_UINT64_MAX) {
				/* -(0xffff'ffff'ffff'ffffULL + 1) =
				 * -0x1'0000'0000'0000'0000
				 *
				 * >>> -0x10000000000000000
				 * -18446744073709551616L
				 */
				return -18446744073709551616.0;
			} else {
				return -((duk_double_t) (t3 + DUK_U64_CONSTANT(1)));
			}
		} else {
			return (duk_double_t) t3;  /* XXX: cast helper */
		}
#endif
#if 0
		t3 = (duk_uint64_t) t2 * DUK_U64_CONSTANT(0x100000000) + (duk_uint64_t) t1;
		if (negative) {
			/* Simpler version: take advantage of the fact that
			 * 0xffff'ffff'ffff'ffff and 0x1'0000'0000'0000'0000
			 * both round to 0x1'0000'0000'0000'0000:
			 * > (0xffffffffffffffff).toString(16)
			 * '10000000000000000'
			 * > (0x10000000000000000).toString(16)
			 * '10000000000000000'
			 *
			 * For the DUK_UINT64_MAX case we just skip the +1
			 * increment to avoid wrapping; the result still
			 * comes out right for an IEEE double cast.
			 */
			if (t3 != DUK_UINT64_MAX) {
				t3++;
			}
			return -((duk_double_t) t3);
		} else {
			return (duk_double_t) t3;  /* XXX: cast helper */
		}
#endif
#if 1
		/* Use two double parts, avoids dependency on 64-bit type.
		 * Avoid precision loss carefully, especially when dealing
		 * with the required +1 for negative values.
		 *
		 * No fastint check for this path at present.
		 */
		d1 = (duk_double_t) t1; /* XXX: cast helpers */
		d2 = (duk_double_t) t2 * 4294967296.0;
		if (negative) {
			d1 += 1.0;
		}
		d = d2 + d1;
		if (negative) {
			d = -d;
		}
#endif
		/* XXX: a push and check for fastint API would be nice */
		duk_push_number(dec_ctx->thr, d);
		return;
	}

	duk__cbor_decode_error(dec_ctx);
	return;

shared_exit:
	if (negative) {
		/* XXX: a push and check for fastint API would be nice */
		if ((duk_uint_t) t <= (duk_uint_t) - (DUK_INT_MIN + 1)) {
			duk_push_int(dec_ctx->thr, -1 - ((duk_int_t) t));
		} else {
			duk_push_number(dec_ctx->thr, -1.0 - (duk_double_t) t);
		}
	} else {
		duk_push_uint(dec_ctx->thr, (duk_uint_t) t);
	}
}

DUK_LOCAL void duk__cbor_decode_skip_aival_int(duk_cbor_decode_context *dec_ctx, duk_uint8_t ib) {
	const duk_int8_t skips[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,
		                       0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 4, 8, -1, -1, -1, -1 };
	duk_uint8_t ai;
	duk_int8_t skip;

	ai = ib & 0x1fU;
	skip = skips[ai];
	if (DUK_UNLIKELY(skip < 0)) {
		duk__cbor_decode_error(dec_ctx);
	}
	duk__cbor_decode_consume(dec_ctx, (duk_size_t) skip);
	return;
}

DUK_LOCAL duk_uint32_t duk__cbor_decode_aival_uint32(duk_cbor_decode_context *dec_ctx, duk_uint8_t ib) {
	duk_uint8_t ai;
	duk_uint32_t t;

	ai = ib & 0x1fU;
	if (ai <= 0x17U) {
		return (duk_uint32_t) ai;
	}

	switch (ai) {
	case 0x18U: /* 1 byte */
		t = (duk_uint32_t) duk__cbor_decode_readbyte(dec_ctx);
		return t;
	case 0x19U: /* 2 byte */
		t = (duk_uint32_t) duk__cbor_decode_read_u16(dec_ctx);
		return t;
	case 0x1aU: /* 4 byte */
		t = (duk_uint32_t) duk__cbor_decode_read_u32(dec_ctx);
		return t;
	case 0x1bU: /* 8 byte */
		t = (duk_uint32_t) duk__cbor_decode_read_u32(dec_ctx);
		if (t != 0U) {
			break;
		}
		t = (duk_uint32_t) duk__cbor_decode_read_u32(dec_ctx);
		return t;
	}

	duk__cbor_decode_error(dec_ctx);
	return 0U;
}

DUK_LOCAL void duk__cbor_decode_buffer(duk_cbor_decode_context *dec_ctx, duk_uint8_t expected_base) {
	duk_uint32_t len;
	duk_uint8_t *buf;
	const duk_uint8_t *inp;
	duk_uint8_t ib;

	ib = duk__cbor_decode_readbyte(dec_ctx);
	if ((ib & 0xe0U) != expected_base) {
		duk__cbor_decode_error(dec_ctx);
	}
	/* Indefinite format is rejected by the following on purpose. */
	len = duk__cbor_decode_aival_uint32(dec_ctx, ib);
	inp = duk__cbor_decode_consume(dec_ctx, len);
	/* XXX: duk_push_fixed_buffer_with_data() would be a nice API addition. */
	buf = (duk_uint8_t *) duk_push_fixed_buffer(dec_ctx->thr, (duk_size_t) len);
	duk_memcpy((void *) buf, (const void *) inp, (size_t) len);
}

DUK_LOCAL void duk__cbor_decode_join_buffers(duk_cbor_decode_context *dec_ctx, duk_idx_t count) {
	duk_size_t total_size = 0;
	duk_idx_t top = duk_get_top(dec_ctx->thr);
	duk_idx_t base = top - count; /* count is >= 1 */
	duk_idx_t idx;
	duk_uint8_t *p = NULL;

	DUK_ASSERT(count >= 1);
	DUK_ASSERT(top >= count);

	for (;;) {
		/* First round: compute total size.
		 * Second round: copy into place.
		 */
		for (idx = base; idx < top; idx++) {
			duk_uint8_t *buf_data;
			duk_size_t buf_size;

			buf_data = (duk_uint8_t *) duk_require_buffer(dec_ctx->thr, idx, &buf_size);
			if (p != NULL) {
				duk_memcpy_unsafe((void *) p, (const void *) buf_data, buf_size);
				p += buf_size;
			} else {
				total_size += buf_size;
				if (DUK_UNLIKELY(total_size < buf_size)) { /* Wrap check. */
					duk__cbor_decode_error(dec_ctx);
				}
			}
		}

		if (p != NULL) {
			break;
		} else {
			p = (duk_uint8_t *) duk_push_fixed_buffer(dec_ctx->thr, total_size);
			DUK_ASSERT(p != NULL);
		}
	}

	duk_replace(dec_ctx->thr, base);
	duk_pop_n(dec_ctx->thr, count - 1);
}

DUK_LOCAL void duk__cbor_decode_and_join_strbuf(duk_cbor_decode_context *dec_ctx, duk_uint8_t expected_base) {
	duk_idx_t count = 0;
	for (;;) {
		if (duk__cbor_decode_checkbreak(dec_ctx)) {
			break;
		}
		duk_require_stack(dec_ctx->thr, 1);
		duk__cbor_decode_buffer(dec_ctx, expected_base);
		count++;
		if (DUK_UNLIKELY(count <= 0)) { /* Wrap check. */
			duk__cbor_decode_error(dec_ctx);
		}
	}
	if (count == 0) {
		(void) duk_push_fixed_buffer(dec_ctx->thr, 0);
	} else if (count > 1) {
		duk__cbor_decode_join_buffers(dec_ctx, count);
	}
}

DUK_LOCAL duk_double_t duk__cbor_decode_half_float(duk_cbor_decode_context *dec_ctx) {
	duk_double_union u;
	const duk_uint8_t *inp;
	duk_int_t expt;
	duk_uint_t u16;
	duk_uint_t tmp;
	duk_double_t res;

	inp = duk__cbor_decode_consume(dec_ctx, 2);
	u16 = ((duk_uint_t) inp[0] << 8) + (duk_uint_t) inp[1];
	expt = (duk_int_t) ((u16 >> 10) & 0x1fU) - 15;

	/* Reconstruct IEEE double into little endian order first, then convert
	 * to host order.
	 */

	duk_memzero((void *) &u, sizeof(u));

	if (expt == -15) {
		/* Zero or denormal; but note that half float
		 * denormals become double normals.
		 */
		if ((u16 & 0x03ffU) == 0) {
			u.uc[7] = inp[0] & 0x80U;
		} else {
			/* Create denormal by first creating a double that
			 * contains the denormal bits and a leading implicit
			 * 1-bit.  Then subtract away the implicit 1-bit.
			 *
			 *    0.mmmmmmmmmm * 2^-14
			 *    1.mmmmmmmmmm 0.... * 2^-14
			 *   -1.0000000000 0.... * 2^-14
			 *
			 * Double exponent: -14 + 1023 = 0x3f1
			 */
			u.uc[7] = 0x3fU;
			u.uc[6] = 0x10U + (duk_uint8_t) ((u16 >> 6) & 0x0fU);
			u.uc[5] = (duk_uint8_t) ((u16 << 2) & 0xffU); /* Mask is really 0xfcU */

			duk_dblunion_little_to_host(&u);
			res = u.d - 0.00006103515625; /* 2^(-14) */
			if (u16 & 0x8000U) {
				res = -res;
			}
			return res;
		}
	} else if (expt == 16) {
		/* +/- Inf or NaN. */
		if ((u16 & 0x03ffU) == 0) {
			u.uc[7] = (inp[0] & 0x80U) + 0x7fU;
			u.uc[6] = 0xf0U;
		} else {
			/* Create a 'quiet NaN' with highest
			 * bit set (there are some platforms
			 * where the NaN payload convention is
			 * the opposite).  Keep sign.
			 */
			u.uc[7] = (inp[0] & 0x80U) + 0x7fU;
			u.uc[6] = 0xf8U;
		}
	} else {
		/* Normal. */
		tmp = (inp[0] & 0x80U) ? 0x80000000UL : 0UL;
		tmp += (duk_uint_t) (expt + 1023) << 20;
		tmp += (duk_uint_t) (inp[0] & 0x03U) << 18;
		tmp += (duk_uint_t) (inp[1] & 0xffU) << 10;
		u.uc[7] = (tmp >> 24) & 0xffU;
		u.uc[6] = (tmp >> 16) & 0xffU;
		u.uc[5] = (tmp >> 8) & 0xffU;
		u.uc[4] = (tmp >> 0) & 0xffU;
	}

	duk_dblunion_little_to_host(&u);
	return u.d;
}

DUK_LOCAL void duk__cbor_decode_string(duk_cbor_decode_context *dec_ctx, duk_uint8_t ib, duk_uint8_t ai) {
	/* If the CBOR string data is not valid UTF-8 it is technically
	 * invalid CBOR.  Possible behaviors at least:
	 *
	 *   1. Reject the input, i.e. throw TypeError.
	 *
	 *   2. Accept the input, but sanitize non-UTF-8 data into UTF-8
	 *      using U+FFFD replacements.  Also it might make sense to
	 *      decode non-BMP codepoints into surrogates for better
	 *      ECMAScript compatibility.
	 *
	 *   3. Accept the input as a Duktape string (which are not always
	 *      valid UTF-8), but reject any input that would create a
	 *      Symbol representation.
	 *
	 * Current behavior is 3.
	 */

	if (ai == 0x1fU) {
		duk_uint8_t *buf_data;
		duk_size_t buf_size;

		duk__cbor_decode_and_join_strbuf(dec_ctx, 0x60U);
		buf_data = (duk_uint8_t *) duk_require_buffer(dec_ctx->thr, -1, &buf_size);
		(void) duk_push_lstring(dec_ctx->thr, (const char *) buf_data, buf_size);
		duk_remove(dec_ctx->thr, -2);
	} else {
		duk_uint32_t len;
		const duk_uint8_t *inp;

		len = duk__cbor_decode_aival_uint32(dec_ctx, ib);
		inp = duk__cbor_decode_consume(dec_ctx, len);
		(void) duk_push_lstring(dec_ctx->thr, (const char *) inp, (duk_size_t) len);
	}
	if (duk_is_symbol(dec_ctx->thr, -1)) {
		/* Refuse to create Symbols when decoding. */
		duk__cbor_decode_error(dec_ctx);
	}

	/* XXX: Here a Duktape API call to convert input -> utf-8 with
	 * replacements would be nice.
	 */
}

DUK_LOCAL duk_bool_t duk__cbor_decode_array(duk_cbor_decode_context *dec_ctx, duk_uint8_t ib, duk_uint8_t ai) {
	duk_uint32_t idx, len;

	duk__cbor_decode_objarr_entry(dec_ctx);

	/* Support arrays up to 0xfffffffeU in length.  0xffffffff is
	 * used as an indefinite length marker.
	 */
	if (ai == 0x1fU) {
		len = 0xffffffffUL;
	} else {
		len = duk__cbor_decode_aival_uint32(dec_ctx, ib);
		if (len == 0xffffffffUL) {
			goto failure;
		}
	}

	/* XXX: use bare array? */
	duk_push_array(dec_ctx->thr);
	for (idx = 0U;;) {
		if (len == 0xffffffffUL && duk__cbor_decode_checkbreak(dec_ctx)) {
			break;
		}
		if (idx == len) {
			if (ai == 0x1fU) {
				goto failure;
			}
			break;
		}
		duk__cbor_decode_value(dec_ctx);
		duk_put_prop_index(dec_ctx->thr, -2, (duk_uarridx_t) idx);
		idx++;
		if (idx == 0U) {
			goto failure; /* wrapped */
		}
	}

#if 0
 success:
#endif
	duk__cbor_decode_objarr_exit(dec_ctx);
	return 1;

failure:
	/* No need to unwind recursion checks, caller will throw. */
	return 0;
}

DUK_LOCAL duk_bool_t duk__cbor_decode_map(duk_cbor_decode_context *dec_ctx, duk_uint8_t ib, duk_uint8_t ai) {
	duk_uint32_t count;

	duk__cbor_decode_objarr_entry(dec_ctx);

	if (ai == 0x1fU) {
		count = 0xffffffffUL;
	} else {
		count = duk__cbor_decode_aival_uint32(dec_ctx, ib);
		if (count == 0xffffffffUL) {
			goto failure;
		}
	}

	/* XXX: use bare object? */
	duk_push_object(dec_ctx->thr);
	for (;;) {
		if (count == 0xffffffffUL) {
			if (duk__cbor_decode_checkbreak(dec_ctx)) {
				break;
			}
		} else {
			if (count == 0UL) {
				break;
			}
			count--;
		}

		/* Non-string keys are coerced to strings,
		 * possibly leading to overwriting previous
		 * keys.  Last key of a certain coerced name
		 * wins.  If key is an object, it will coerce
		 * to '[object Object]' which is consistent
		 * but potentially misleading.  One alternative
		 * would be to skip non-string keys.
		 */
		duk__cbor_decode_value(dec_ctx);
		duk__cbor_decode_value(dec_ctx);
		duk_put_prop(dec_ctx->thr, -3);
	}

#if 0
 success:
#endif
	duk__cbor_decode_objarr_exit(dec_ctx);
	return 1;

failure:
	/* No need to unwind recursion checks, caller will throw. */
	return 0;
}

DUK_LOCAL duk_double_t duk__cbor_decode_float(duk_cbor_decode_context *dec_ctx) {
	duk_float_union u;
	const duk_uint8_t *inp;
	inp = duk__cbor_decode_consume(dec_ctx, 4);
	duk_memcpy((void *) u.uc, (const void *) inp, 4);
	duk_fltunion_big_to_host(&u);
	return (duk_double_t) u.f;
}

DUK_LOCAL duk_double_t duk__cbor_decode_double(duk_cbor_decode_context *dec_ctx) {
	duk_double_union u;
	const duk_uint8_t *inp;
	inp = duk__cbor_decode_consume(dec_ctx, 8);
	duk_memcpy((void *) u.uc, (const void *) inp, 8);
	duk_dblunion_big_to_host(&u);
	return u.d;
}

#if defined(DUK_CBOR_DECODE_FASTPATH)
#define DUK__CBOR_AI (ib & 0x1fU)

DUK_LOCAL void duk__cbor_decode_value(duk_cbor_decode_context *dec_ctx) {
	duk_uint8_t ib;

	/* Any paths potentially recursing back to duk__cbor_decode_value()
	 * must perform a Duktape value stack growth check.  Avoid the check
	 * here for simple paths like primitive values.
	 */

reread_initial_byte:
	DUK_DDD(DUK_DDDPRINT("cbor decode off=%ld len=%ld", (long) dec_ctx->off, (long) dec_ctx->len));

	ib = duk__cbor_decode_readbyte(dec_ctx);

	/* Full initial byte switch, footprint cost over baseline is ~+1kB. */
	/* XXX: Force full switch with no range check. */

	switch (ib) {
	case 0x00U:
	case 0x01U:
	case 0x02U:
	case 0x03U:
	case 0x04U:
	case 0x05U:
	case 0x06U:
	case 0x07U:
	case 0x08U:
	case 0x09U:
	case 0x0aU:
	case 0x0bU:
	case 0x0cU:
	case 0x0dU:
	case 0x0eU:
	case 0x0fU:
	case 0x10U:
	case 0x11U:
	case 0x12U:
	case 0x13U:
	case 0x14U:
	case 0x15U:
	case 0x16U:
	case 0x17U:
		duk_push_uint(dec_ctx->thr, ib);
		break;
	case 0x18U:
	case 0x19U:
	case 0x1aU:
	case 0x1bU:
		duk__cbor_decode_push_aival_int(dec_ctx, ib, 0 /*negative*/);
		break;
	case 0x1cU:
	case 0x1dU:
	case 0x1eU:
	case 0x1fU:
		goto format_error;
	case 0x20U:
	case 0x21U:
	case 0x22U:
	case 0x23U:
	case 0x24U:
	case 0x25U:
	case 0x26U:
	case 0x27U:
	case 0x28U:
	case 0x29U:
	case 0x2aU:
	case 0x2bU:
	case 0x2cU:
	case 0x2dU:
	case 0x2eU:
	case 0x2fU:
	case 0x30U:
	case 0x31U:
	case 0x32U:
	case 0x33U:
	case 0x34U:
	case 0x35U:
	case 0x36U:
	case 0x37U:
		duk_push_int(dec_ctx->thr, -((duk_int_t) ((ib - 0x20U) + 1U)));
		break;
	case 0x38U:
	case 0x39U:
	case 0x3aU:
	case 0x3bU:
		duk__cbor_decode_push_aival_int(dec_ctx, ib, 1 /*negative*/);
		break;
	case 0x3cU:
	case 0x3dU:
	case 0x3eU:
	case 0x3fU:
		goto format_error;
	case 0x40U:
	case 0x41U:
	case 0x42U:
	case 0x43U:
	case 0x44U:
	case 0x45U:
	case 0x46U:
	case 0x47U:
	case 0x48U:
	case 0x49U:
	case 0x4aU:
	case 0x4bU:
	case 0x4cU:
	case 0x4dU:
	case 0x4eU:
	case 0x4fU:
	case 0x50U:
	case 0x51U:
	case 0x52U:
	case 0x53U:
	case 0x54U:
	case 0x55U:
	case 0x56U:
	case 0x57U:
		/* XXX: Avoid rewind, we know the length already. */
		DUK_ASSERT(dec_ctx->off > 0U);
		dec_ctx->off--;
		duk__cbor_decode_buffer(dec_ctx, 0x40U);
		break;
	case 0x58U:
	case 0x59U:
	case 0x5aU:
	case 0x5bU:
		/* XXX: Avoid rewind, decode length inline. */
		DUK_ASSERT(dec_ctx->off > 0U);
		dec_ctx->off--;
		duk__cbor_decode_buffer(dec_ctx, 0x40U);
		break;
	case 0x5cU:
	case 0x5dU:
	case 0x5eU:
		goto format_error;
	case 0x5fU:
		duk__cbor_decode_and_join_strbuf(dec_ctx, 0x40U);
		break;
	case 0x60U:
	case 0x61U:
	case 0x62U:
	case 0x63U:
	case 0x64U:
	case 0x65U:
	case 0x66U:
	case 0x67U:
	case 0x68U:
	case 0x69U:
	case 0x6aU:
	case 0x6bU:
	case 0x6cU:
	case 0x6dU:
	case 0x6eU:
	case 0x6fU:
	case 0x70U:
	case 0x71U:
	case 0x72U:
	case 0x73U:
	case 0x74U:
	case 0x75U:
	case 0x76U:
	case 0x77U:
		/* XXX: Avoid double decode of length. */
		duk__cbor_decode_string(dec_ctx, ib, DUK__CBOR_AI);
		break;
	case 0x78U:
	case 0x79U:
	case 0x7aU:
	case 0x7bU:
		/* XXX: Avoid double decode of length. */
		duk__cbor_decode_string(dec_ctx, ib, DUK__CBOR_AI);
		break;
	case 0x7cU:
	case 0x7dU:
	case 0x7eU:
		goto format_error;
	case 0x7fU:
		duk__cbor_decode_string(dec_ctx, ib, DUK__CBOR_AI);
		break;
	case 0x80U:
	case 0x81U:
	case 0x82U:
	case 0x83U:
	case 0x84U:
	case 0x85U:
	case 0x86U:
	case 0x87U:
	case 0x88U:
	case 0x89U:
	case 0x8aU:
	case 0x8bU:
	case 0x8cU:
	case 0x8dU:
	case 0x8eU:
	case 0x8fU:
	case 0x90U:
	case 0x91U:
	case 0x92U:
	case 0x93U:
	case 0x94U:
	case 0x95U:
	case 0x96U:
	case 0x97U:
		if (DUK_UNLIKELY(duk__cbor_decode_array(dec_ctx, ib, DUK__CBOR_AI) == 0)) {
			goto format_error;
		}
		break;
	case 0x98U:
	case 0x99U:
	case 0x9aU:
	case 0x9bU:
		if (DUK_UNLIKELY(duk__cbor_decode_array(dec_ctx, ib, DUK__CBOR_AI) == 0)) {
			goto format_error;
		}
		break;
	case 0x9cU:
	case 0x9dU:
	case 0x9eU:
		goto format_error;
	case 0x9fU:
		if (DUK_UNLIKELY(duk__cbor_decode_array(dec_ctx, ib, DUK__CBOR_AI) == 0)) {
			goto format_error;
		}
		break;
	case 0xa0U:
	case 0xa1U:
	case 0xa2U:
	case 0xa3U:
	case 0xa4U:
	case 0xa5U:
	case 0xa6U:
	case 0xa7U:
	case 0xa8U:
	case 0xa9U:
	case 0xaaU:
	case 0xabU:
	case 0xacU:
	case 0xadU:
	case 0xaeU:
	case 0xafU:
	case 0xb0U:
	case 0xb1U:
	case 0xb2U:
	case 0xb3U:
	case 0xb4U:
	case 0xb5U:
	case 0xb6U:
	case 0xb7U:
		if (DUK_UNLIKELY(duk__cbor_decode_map(dec_ctx, ib, DUK__CBOR_AI) == 0)) {
			goto format_error;
		}
		break;
	case 0xb8U:
	case 0xb9U:
	case 0xbaU:
	case 0xbbU:
		if (DUK_UNLIKELY(duk__cbor_decode_map(dec_ctx, ib, DUK__CBOR_AI) == 0)) {
			goto format_error;
		}
		break;
	case 0xbcU:
	case 0xbdU:
	case 0xbeU:
		goto format_error;
	case 0xbfU:
		if (DUK_UNLIKELY(duk__cbor_decode_map(dec_ctx, ib, DUK__CBOR_AI) == 0)) {
			goto format_error;
		}
		break;
	case 0xc0U:
	case 0xc1U:
	case 0xc2U:
	case 0xc3U:
	case 0xc4U:
	case 0xc5U:
	case 0xc6U:
	case 0xc7U:
	case 0xc8U:
	case 0xc9U:
	case 0xcaU:
	case 0xcbU:
	case 0xccU:
	case 0xcdU:
	case 0xceU:
	case 0xcfU:
	case 0xd0U:
	case 0xd1U:
	case 0xd2U:
	case 0xd3U:
	case 0xd4U:
	case 0xd5U:
	case 0xd6U:
	case 0xd7U:
		/* Tag 0-23: drop. */
		goto reread_initial_byte;
	case 0xd8U:
	case 0xd9U:
	case 0xdaU:
	case 0xdbU:
		duk__cbor_decode_skip_aival_int(dec_ctx, ib);
		goto reread_initial_byte;
	case 0xdcU:
	case 0xddU:
	case 0xdeU:
	case 0xdfU:
		goto format_error;
	case 0xe0U:
		goto format_error;
	case 0xe1U:
		goto format_error;
	case 0xe2U:
		goto format_error;
	case 0xe3U:
		goto format_error;
	case 0xe4U:
		goto format_error;
	case 0xe5U:
		goto format_error;
	case 0xe6U:
		goto format_error;
	case 0xe7U:
		goto format_error;
	case 0xe8U:
		goto format_error;
	case 0xe9U:
		goto format_error;
	case 0xeaU:
		goto format_error;
	case 0xebU:
		goto format_error;
	case 0xecU:
		goto format_error;
	case 0xedU:
		goto format_error;
	case 0xeeU:
		goto format_error;
	case 0xefU:
		goto format_error;
	case 0xf0U:
		goto format_error;
	case 0xf1U:
		goto format_error;
	case 0xf2U:
		goto format_error;
	case 0xf3U:
		goto format_error;
	case 0xf4U:
		duk_push_false(dec_ctx->thr);
		break;
	case 0xf5U:
		duk_push_true(dec_ctx->thr);
		break;
	case 0xf6U:
		duk_push_null(dec_ctx->thr);
		break;
	case 0xf7U:
		duk_push_undefined(dec_ctx->thr);
		break;
	case 0xf8U:
		/* Simple value 32-255, nothing defined yet, so reject. */
		goto format_error;
	case 0xf9U: {
		duk_double_t d;
		d = duk__cbor_decode_half_float(dec_ctx);
		duk_push_number(dec_ctx->thr, d);
		break;
	}
	case 0xfaU: {
		duk_double_t d;
		d = duk__cbor_decode_float(dec_ctx);
		duk_push_number(dec_ctx->thr, d);
		break;
	}
	case 0xfbU: {
		duk_double_t d;
		d = duk__cbor_decode_double(dec_ctx);
		duk_push_number(dec_ctx->thr, d);
		break;
	}
	case 0xfcU:
	case 0xfdU:
	case 0xfeU:
	case 0xffU:
		goto format_error;
	} /* end switch */

	return;

format_error:
	duk__cbor_decode_error(dec_ctx);
}
#else /* DUK_CBOR_DECODE_FASTPATH */
DUK_LOCAL void duk__cbor_decode_value(duk_cbor_decode_context *dec_ctx) {
	duk_uint8_t ib, mt, ai;

	/* Any paths potentially recursing back to duk__cbor_decode_value()
	 * must perform a Duktape value stack growth check.  Avoid the check
	 * here for simple paths like primitive values.
	 */

reread_initial_byte:
	DUK_DDD(DUK_DDDPRINT("cbor decode off=%ld len=%ld", (long) dec_ctx->off, (long) dec_ctx->len));

	ib = duk__cbor_decode_readbyte(dec_ctx);
	mt = ib >> 5U;
	ai = ib & 0x1fU;

	/* Additional information in [24,27] = [0x18,0x1b] has relatively
	 * uniform handling for all major types: read 1/2/4/8 additional
	 * bytes.  For major type 7 the 1-byte value is a 'simple type', and
	 * 2/4/8-byte values are floats.  For other major types the 1/2/4/8
	 * byte values are integers.  The lengths are uniform, but the typing
	 * is not.
	 */

	switch (mt) {
	case 0U: { /* unsigned integer */
		duk__cbor_decode_push_aival_int(dec_ctx, ib, 0 /*negative*/);
		break;
	}
	case 1U: { /* negative integer */
		duk__cbor_decode_push_aival_int(dec_ctx, ib, 1 /*negative*/);
		break;
	}
	case 2U: { /* byte string */
		if (ai == 0x1fU) {
			duk__cbor_decode_and_join_strbuf(dec_ctx, 0x40U);
		} else {
			duk__cbor_decode_rewind(dec_ctx, 1U);
			duk__cbor_decode_buffer(dec_ctx, 0x40U);
		}
		break;
	}
	case 3U: { /* text string */
		duk__cbor_decode_string(dec_ctx, ib, ai);
		break;
	}
	case 4U: { /* array of data items */
		if (DUK_UNLIKELY(duk__cbor_decode_array(dec_ctx, ib, ai) == 0)) {
			goto format_error;
		}
		break;
	}
	case 5U: { /* map of pairs of data items */
		if (DUK_UNLIKELY(duk__cbor_decode_map(dec_ctx, ib, ai) == 0)) {
			goto format_error;
		}
		break;
	}
	case 6U: { /* semantic tagging */
		/* Tags are ignored now, re-read initial byte.  A tagged
		 * value may itself be tagged (an unlimited number of times)
		 * so keep on peeling away tags.
		 */
		duk__cbor_decode_skip_aival_int(dec_ctx, ib);
		goto reread_initial_byte;
	}
	case 7U: { /* floating point numbers, simple data types, break; other */
		switch (ai) {
		case 0x14U: {
			duk_push_false(dec_ctx->thr);
			break;
		}
		case 0x15U: {
			duk_push_true(dec_ctx->thr);
			break;
		}
		case 0x16U: {
			duk_push_null(dec_ctx->thr);
			break;
		}
		case 0x17U: {
			duk_push_undefined(dec_ctx->thr);
			break;
		}
		case 0x18U: { /* more simple values (1 byte) */
			/* Simple value encoded in additional byte (none
			 * are defined so far).  RFC 7049 states that the
			 * follow-up byte must be 32-255 to minimize
			 * confusion.  So, a non-shortest encoding like
			 * f815 (= true, shortest encoding f5) must be
			 * rejected.  cbor.me tester rejects f815, but
			 * e.g. Python CBOR binding decodes it as true.
			 */
			goto format_error;
		}
		case 0x19U: { /* half-float (2 bytes) */
			duk_double_t d;
			d = duk__cbor_decode_half_float(dec_ctx);
			duk_push_number(dec_ctx->thr, d);
			break;
		}
		case 0x1aU: { /* float (4 bytes) */
			duk_double_t d;
			d = duk__cbor_decode_float(dec_ctx);
			duk_push_number(dec_ctx->thr, d);
			break;
		}
		case 0x1bU: { /* double (8 bytes) */
			duk_double_t d;
			d = duk__cbor_decode_double(dec_ctx);
			duk_push_number(dec_ctx->thr, d);
			break;
		}
		case 0xffU: /* unexpected break */
		default: {
			goto format_error;
		}
		} /* end switch */
		break;
	}
	default: {
		goto format_error; /* will never actually occur */
	}
	} /* end switch */

	return;

format_error:
	duk__cbor_decode_error(dec_ctx);
}
#endif /* DUK_CBOR_DECODE_FASTPATH */

DUK_LOCAL void duk__cbor_encode(duk_hthread *thr, duk_idx_t idx, duk_uint_t encode_flags) {
	duk_cbor_encode_context enc_ctx;
	duk_uint8_t *buf;

	DUK_UNREF(encode_flags);

	idx = duk_require_normalize_index(thr, idx);

	enc_ctx.thr = thr;
	enc_ctx.idx_buf = duk_get_top(thr);

	enc_ctx.len = 64;
	buf = (duk_uint8_t *) duk_push_dynamic_buffer(thr, enc_ctx.len);
	enc_ctx.ptr = buf;
	enc_ctx.buf = buf;
	enc_ctx.buf_end = buf + enc_ctx.len;

	enc_ctx.recursion_depth = 0;
	enc_ctx.recursion_limit = DUK_USE_CBOR_ENC_RECLIMIT;

	duk_dup(thr, idx);
	duk__cbor_encode_req_stack(&enc_ctx);
	duk__cbor_encode_value(&enc_ctx);
	DUK_ASSERT(enc_ctx.recursion_depth == 0);
	duk_resize_buffer(enc_ctx.thr, enc_ctx.idx_buf, (duk_size_t) (enc_ctx.ptr - enc_ctx.buf));
	duk_replace(thr, idx);
}

DUK_LOCAL void duk__cbor_decode(duk_hthread *thr, duk_idx_t idx, duk_uint_t decode_flags) {
	duk_cbor_decode_context dec_ctx;

	DUK_UNREF(decode_flags);

	/* Suppress compile warnings for functions only needed with e.g.
	 * asserts enabled.
	 */
	DUK_UNREF(duk__cbor_get_reserve);

	idx = duk_require_normalize_index(thr, idx);

	dec_ctx.thr = thr;
	dec_ctx.buf = (const duk_uint8_t *) duk_require_buffer_data(thr, idx, &dec_ctx.len);
	dec_ctx.off = 0;
	/* dec_ctx.len: set above */

	dec_ctx.recursion_depth = 0;
	dec_ctx.recursion_limit = DUK_USE_CBOR_DEC_RECLIMIT;

	duk__cbor_decode_req_stack(&dec_ctx);
	duk__cbor_decode_value(&dec_ctx);
	DUK_ASSERT(dec_ctx.recursion_depth == 0);
	if (dec_ctx.off != dec_ctx.len) {
		(void) duk_type_error(thr, "trailing garbage");
	}

	duk_replace(thr, idx);
}

#else /* DUK_USE_CBOR_SUPPORT */

DUK_LOCAL void duk__cbor_encode(duk_hthread *thr, duk_idx_t idx, duk_uint_t encode_flags) {
	DUK_UNREF(idx);
	DUK_UNREF(encode_flags);
	DUK_ERROR_UNSUPPORTED(thr);
}

DUK_LOCAL void duk__cbor_decode(duk_hthread *thr, duk_idx_t idx, duk_uint_t decode_flags) {
	DUK_UNREF(idx);
	DUK_UNREF(decode_flags);
	DUK_ERROR_UNSUPPORTED(thr);
}

#endif /* DUK_USE_CBOR_SUPPORT */

/*
 *  Public APIs
 */

DUK_EXTERNAL void duk_cbor_encode(duk_hthread *thr, duk_idx_t idx, duk_uint_t encode_flags) {
	DUK_ASSERT_API_ENTRY(thr);
	duk__cbor_encode(thr, idx, encode_flags);
}
DUK_EXTERNAL void duk_cbor_decode(duk_hthread *thr, duk_idx_t idx, duk_uint_t decode_flags) {
	DUK_ASSERT_API_ENTRY(thr);
	duk__cbor_decode(thr, idx, decode_flags);
}

#if defined(DUK_USE_CBOR_BUILTIN)
#if defined(DUK_USE_CBOR_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_cbor_encode(duk_hthread *thr) {
	DUK_ASSERT_TOP(thr, 1);

	duk__cbor_encode(thr, -1, 0 /*flags*/);

	/* Produce an ArrayBuffer by first decoding into a plain buffer which
	 * mimics a Uint8Array and gettings its .buffer property.
	 */
	/* XXX: shortcut */
	(void) duk_get_prop_stridx(thr, -1, DUK_STRIDX_LC_BUFFER);
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_cbor_decode(duk_hthread *thr) {
	DUK_ASSERT_TOP(thr, 1);

	duk__cbor_decode(thr, -1, 0 /*flags*/);
	return 1;
}
#else /* DUK_USE_CBOR_SUPPORT */
DUK_INTERNAL duk_ret_t duk_bi_cbor_encode(duk_hthread *thr) {
	DUK_ERROR_UNSUPPORTED(thr);
	DUK_WO_NORETURN(return 0;);
}
DUK_INTERNAL duk_ret_t duk_bi_cbor_decode(duk_hthread *thr) {
	DUK_ERROR_UNSUPPORTED(thr);
	DUK_WO_NORETURN(return 0;);
}
#endif /* DUK_USE_CBOR_SUPPORT */
#endif /* DUK_USE_CBOR_BUILTIN */
