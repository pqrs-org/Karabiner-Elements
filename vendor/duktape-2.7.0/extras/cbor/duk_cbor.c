/*
 *  CBOR bindings for Duktape.
 *
 *  https://tools.ietf.org/html/rfc7049
 */

#include <math.h>
#include <string.h>
#include "duktape.h"
#include "duk_cbor.h"

/* #define DUK_CBOR_DPRINT */
/* #define DUK_CBOR_STRESS */

#if 1
#define DUK_CBOR_ASSERT(x) do {} while (0)
#else
#include <stdio.h>
#include <stdlib.h>
#define DUK_CBOR_ASSERT(x) do { \
		if (!(x)) { \
			fprintf(stderr, "ASSERT FAILED on %s:%d\n", __FILE__, __LINE__); \
			fflush(stderr); \
			abort(); \
		} \
	} while (0)
#endif

#if 0
#define DUK_CBOR_LIKELY(x) __builtin_expect (!!(x), 1)
#define DUK_CBOR_UNLIKELY(x) __builtin_expect (!!(x), 0)
#define DUK_CBOR_INLINE inline
#define DUK_CBOR_NOINLINE __attribute__((noinline))
#else
#define DUK_CBOR_LIKELY(x) (x)
#define DUK_CBOR_UNLIKELY(x) (x)
#define DUK_CBOR_INLINE
#define DUK_CBOR_NOINLINE
#endif

/* #define DUK_CBOR_GCC_BUILTINS */

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
	duk_context *ctx;
	duk_uint8_t *ptr;
	duk_uint8_t *buf;
	duk_uint8_t *buf_end;
	duk_size_t len;
	duk_idx_t idx_buf;
} duk_cbor_encode_context;

typedef struct {
	duk_context *ctx;
	const duk_uint8_t *buf;
	duk_size_t off;
	duk_size_t len;
} duk_cbor_decode_context;

typedef union {
	duk_uint8_t x[8];
	duk_uint16_t s[4];
	duk_uint32_t i[2];
#if 0
	duk_uint64_t i64[1];
#endif
	double d;
} duk_cbor_dblunion;

typedef union {
	duk_uint8_t x[4];
	duk_uint16_t s[2];
	duk_uint32_t i[1];
	float f;
} duk_cbor_fltunion;

static void duk__cbor_encode_value(duk_cbor_encode_context *enc_ctx);
static void duk__cbor_decode_value(duk_cbor_decode_context *dec_ctx);

/*
 *  Misc
 */

/* XXX: These are sometimes portability concerns and would be nice to expose
 * from Duktape itself as portability helpers.
 */

static int duk__cbor_signbit(double d) {
	return signbit(d);
}

static int duk__cbor_fpclassify(double d) {
	return fpclassify(d);
}

static int duk__cbor_isnan(double d) {
	return isnan(d);
}

static int duk__cbor_isinf(double d) {
	return isinf(d);
}

static duk_uint32_t duk__cbor_double_to_uint32(double d) {
	/* Out of range casts are undefined behavior, so caller must avoid. */
	DUK_CBOR_ASSERT(d >= 0.0 && d <= 4294967295.0);
	return (duk_uint32_t) d;
}

/* Endian detection.  Technically happens at runtime, but in practice
 * resolves at compile time to a constant and gets inlined.
 */
#define DUK__CBOR_LITTLE_ENDIAN  1
#define DUK__CBOR_MIXED_ENDIAN   2
#define DUK__CBOR_BIG_ENDIAN     3

static int duk__cbor_check_endian(void) {
	duk_cbor_dblunion u;

	/* >>> struct.pack('>d', 1.23456789).encode('hex')
	 * '3ff3c0ca4283de1b'
	 */

	u.d = 1.23456789;
	if (u.x[0] == 0x1bU) {
		return DUK__CBOR_LITTLE_ENDIAN;
	} else if (u.x[0] == 0x3fU) {
		return DUK__CBOR_BIG_ENDIAN;
	} else if (u.x[0] == 0xcaU) {
		return DUK__CBOR_MIXED_ENDIAN;
	} else {
		DUK_CBOR_ASSERT(0);
	}
	return 0;
}

static DUK_CBOR_INLINE duk_uint16_t duk__cbor_bswap16(duk_uint16_t x) {
#if defined(DUK_CBOR_GCC_BUILTINS)
	return __builtin_bswap16(x);
#else
	/* XXX: matches, DUK_BSWAP16(), use that if exposed. */
	return (x >> 8) | (x << 8);
#endif
}

static DUK_CBOR_INLINE duk_uint32_t duk__cbor_bswap32(duk_uint32_t x) {
#if defined(DUK_CBOR_GCC_BUILTINS)
	return __builtin_bswap32(x);
#else
	/* XXX: matches, DUK_BSWAP32(), use that if exposed. */
	return (x >> 24) | ((x >> 8) & 0xff00UL) | ((x << 8) & 0xff0000UL) | (x << 24);
#endif
}

#if 0
static duk_uint64_t duk__cbor_bswap64(duk_uint64_t x) {
	/* XXX */
}
#endif

static DUK_CBOR_INLINE void duk__cbor_write_uint16_big(duk_uint8_t *p, duk_uint16_t x) {
#if 0
	*p++ = (duk_uint8_t) ((x >> 8) & 0xffU);
	*p++ = (duk_uint8_t) (x & 0xffU);
#endif
	duk_uint16_t a;

	switch (duk__cbor_check_endian()) {
	case DUK__CBOR_LITTLE_ENDIAN:
	case DUK__CBOR_MIXED_ENDIAN:
		a = duk__cbor_bswap16(x);
		(void) memcpy((void *) p, (const void *) &a, 2);
		break;
	case DUK__CBOR_BIG_ENDIAN:
		a = x;
		(void) memcpy((void *) p, (const void *) &a, 2);
		break;
	default:
		DUK_CBOR_ASSERT(0);
	}
}

static DUK_CBOR_INLINE duk_uint16_t duk__cbor_read_uint16_big(const duk_uint8_t *p) {
	duk_uint16_t a, x;

#if 0
	x = (((duk_uint16_t) p[0]) << 8U) +
	    ((duk_uint16_t) p[1]);
#endif
	switch (duk__cbor_check_endian()) {
	case DUK__CBOR_LITTLE_ENDIAN:
	case DUK__CBOR_MIXED_ENDIAN:
		(void) memcpy((void *) &a, (const void *) p, 2);
		x = duk__cbor_bswap16(a);
		break;
	case DUK__CBOR_BIG_ENDIAN:
		(void) memcpy((void *) &a, (const void *) p, 2);
		x = a;
		break;
	default:
		DUK_CBOR_ASSERT(0);
		x = 0;
	}
	return x;
}

static DUK_CBOR_INLINE void duk__cbor_write_uint32_big(duk_uint8_t *p, duk_uint32_t x) {
#if 0
	*p++ = (duk_uint8_t) ((x >> 24) & 0xffU);
	*p++ = (duk_uint8_t) ((x >> 16) & 0xffU);
	*p++ = (duk_uint8_t) ((x >> 8) & 0xffU);
	*p++ = (duk_uint8_t) (x & 0xffU);
#endif
	duk_uint32_t a;

	switch (duk__cbor_check_endian()) {
	case DUK__CBOR_LITTLE_ENDIAN:
	case DUK__CBOR_MIXED_ENDIAN:
		a = duk__cbor_bswap32(x);
		(void) memcpy((void *) p, (const void *) &a, 4);
		break;
	case DUK__CBOR_BIG_ENDIAN:
		a = x;
		(void) memcpy((void *) p, (const void *) &a, 4);
		break;
	default:
		DUK_CBOR_ASSERT(0);
	}
}

static DUK_CBOR_INLINE duk_uint32_t duk__cbor_read_uint32_big(const duk_uint8_t *p) {
	duk_uint32_t a, x;

#if 0
	x = (((duk_uint32_t) p[0]) << 24U) +
	    (((duk_uint32_t) p[1]) << 16U) +
	    (((duk_uint32_t) p[2]) << 8U) +
	    ((duk_uint32_t) p[3]);
#endif
	switch (duk__cbor_check_endian()) {
	case DUK__CBOR_LITTLE_ENDIAN:
	case DUK__CBOR_MIXED_ENDIAN:
		(void) memcpy((void *) &a, (const void *) p, 4);
		x = duk__cbor_bswap32(a);
		break;
	case DUK__CBOR_BIG_ENDIAN:
		(void) memcpy((void *) &a, (const void *) p, 4);
		x = a;
		break;
	default:
		DUK_CBOR_ASSERT(0);
		x = 0;
	}
	return x;
}

static DUK_CBOR_INLINE void duk__cbor_write_double_big(duk_uint8_t *p, double x) {
	duk_cbor_dblunion u;
	duk_uint32_t a, b;

	u.d = x;

	switch (duk__cbor_check_endian()) {
	case DUK__CBOR_LITTLE_ENDIAN:
#if 0
		u.i64[0] = duk__cbor_bswap64(u.i64[0]);
		(void) memcpy((void *) p, (const void *) u.x, 8);
#endif
		a = u.i[0];
		b = u.i[1];
		u.i[0] = duk__cbor_bswap32(b);
		u.i[1] = duk__cbor_bswap32(a);
		(void) memcpy((void *) p, (const void *) u.x, 8);
		break;
	case DUK__CBOR_MIXED_ENDIAN:
		a = u.i[0];
		b = u.i[1];
		u.i[0] = duk__cbor_bswap32(a);
		u.i[1] = duk__cbor_bswap32(b);
		(void) memcpy((void *) p, (const void *) u.x, 8);
		break;
	case DUK__CBOR_BIG_ENDIAN:
		(void) memcpy((void *) p, (const void *) u.x, 8);
		break;
	default:
		DUK_CBOR_ASSERT(0);
	}
}

static DUK_CBOR_INLINE void duk__cbor_write_float_big(duk_uint8_t *p, float x) {
	duk_cbor_fltunion u;
	duk_uint32_t a;

	u.f = x;
	switch (duk__cbor_check_endian()) {
	case DUK__CBOR_LITTLE_ENDIAN:
	case DUK__CBOR_MIXED_ENDIAN:
		a = u.i[0];
		u.i[0] = duk__cbor_bswap32(a);
		(void) memcpy((void *) p, (const void *) u.x, 4);
		break;
	case DUK__CBOR_BIG_ENDIAN:
		(void) memcpy((void *) p, (const void *) u.x, 4);
		break;
	default:
		DUK_CBOR_ASSERT(0);
	}
}

static DUK_CBOR_INLINE void duk__cbor_dblunion_host_to_little(duk_cbor_dblunion *u) {
	duk_uint32_t a, b;

	switch (duk__cbor_check_endian()) {
	case DUK__CBOR_LITTLE_ENDIAN:
		/* HGFEDCBA -> HGFEDCBA */
		break;
	case DUK__CBOR_MIXED_ENDIAN:
		/* DCBAHGFE -> HGFEDCBA */
		a = u->i[0];
		b = u->i[1];
		u->i[0] = b;
		u->i[1] = a;
		break;
	case DUK__CBOR_BIG_ENDIAN:
		/* ABCDEFGH -> HGFEDCBA */
#if 0
		u->i64[0] = duk__cbor_bswap64(u->i64[0]);
#endif
		a = u->i[0];
		b = u->i[1];
		u->i[0] = duk__cbor_bswap32(b);
		u->i[1] = duk__cbor_bswap32(a);
		break;
	}
}

static DUK_CBOR_INLINE void duk__cbor_dblunion_little_to_host(duk_cbor_dblunion *u) {
	duk__cbor_dblunion_host_to_little(u);
}

static DUK_CBOR_INLINE void duk__cbor_dblunion_host_to_big(duk_cbor_dblunion *u) {
	duk_uint32_t a, b;

	switch (duk__cbor_check_endian()) {
	case DUK__CBOR_LITTLE_ENDIAN:
		/* HGFEDCBA -> ABCDEFGH */
#if 0
		u->i64[0] = duk__cbor_bswap64(u->i64[0]);
#else
		a = u->i[0];
		b = u->i[1];
		u->i[0] = duk__cbor_bswap32(b);
		u->i[1] = duk__cbor_bswap32(a);
#endif
		break;
	case DUK__CBOR_MIXED_ENDIAN:
		/* DCBAHGFE -> ABCDEFGH */
		a = u->i[0];
		b = u->i[1];
		u->i[0] = duk__cbor_bswap32(a);
		u->i[1] = duk__cbor_bswap32(b);
		break;
	case DUK__CBOR_BIG_ENDIAN:
		/* ABCDEFGH -> ABCDEFGH */
		break;
	}
}

static DUK_CBOR_INLINE void duk__cbor_dblunion_big_to_host(duk_cbor_dblunion *u) {
	duk__cbor_dblunion_host_to_big(u);
}

static DUK_CBOR_INLINE void duk__cbor_fltunion_host_to_big(duk_cbor_fltunion *u) {
	switch (duk__cbor_check_endian()) {
	case DUK__CBOR_LITTLE_ENDIAN:
	case DUK__CBOR_MIXED_ENDIAN:
		/* DCBA -> ABCD */
		u->i[0] = duk__cbor_bswap32(u->i[0]);
		break;
	case DUK__CBOR_BIG_ENDIAN:
		/* ABCD -> ABCD */
		break;
	}
}

static DUK_CBOR_INLINE void duk__cbor_fltunion_big_to_host(duk_cbor_fltunion *u) {
	duk__cbor_fltunion_host_to_big(u);
}

/*
 *  Encoding
 */

static void duk__cbor_encode_error(duk_cbor_encode_context *enc_ctx) {
	(void) duk_type_error(enc_ctx->ctx, "cbor encode error");
}

/* Check whether a string is UTF-8 compatible or not. */
static int duk__cbor_is_utf8_compatible(const duk_uint8_t *buf, duk_size_t len) {
	duk_size_t i = 0;
#if !defined(DUK_CBOR_PREFER_SIZE)
	duk_size_t len_safe;
#endif

	/* Many practical strings are ASCII only, so use a fast path check
	 * to check chunks of bytes at once with minimal branch cost.
	 */
#if !defined(DUK_CBOR_PREFER_SIZE)
	len_safe = len & ~0x03UL;
	for (; i < len_safe; i += 4) {
		duk_uint8_t t = buf[i] | buf[i + 1] | buf[i + 2] | buf[i + 3];
		if (DUK_CBOR_UNLIKELY((t & 0x80U) != 0U)) {
			/* At least one byte was outside 0x00-0x7f, break
			 * out to slow path (and remain there).
			 *
			 * XXX: We could also deal with the problem character
			 * and resume fast path later.
			 */
			break;
		}
	}
#endif

	for (; i < len;) {
		duk_uint8_t t;
		duk_size_t left;
		duk_size_t ncont;
		duk_uint32_t cp;
		duk_uint32_t mincp;

		t = buf[i++];
		if (DUK_CBOR_LIKELY((t & 0x80U) == 0U)) {
			/* Fast path, ASCII. */
			continue;
		}

		/* Non-ASCII start byte, slow path.
		 *
		 * 10xx xxxx          -> continuation byte
		 * 110x xxxx + 1*CONT -> [0x80, 0x7ff]
		 * 1110 xxxx + 2*CONT -> [0x800, 0xffff], must reject [0xd800,0xdfff]
		 * 1111 0xxx + 3*CONT -> [0x10000, 0x10ffff]
		 */
		left = len - i;
		if (t <= 0xdfU) {  /* 1101 1111 = 0xdf */
			if (t <= 0xbfU) {  /* 1011 1111 = 0xbf */
				return 0;
			}
			ncont = 1;
			mincp = 0x80UL;
			cp = t & 0x1fU;
		} else if (t <= 0xefU) {  /* 1110 1111 = 0xef */
			ncont = 2;
			mincp = 0x800UL;
			cp = t & 0x0fU;
		} else if (t <= 0xf7U) {  /* 1111 0111 = 0xf7 */
			ncont = 3;
			mincp = 0x10000UL;
			cp = t & 0x07U;
		} else {
			return 0;
		}
		if (left < ncont) {
			return 0;
		}
		while (ncont > 0U) {
			t = buf[i++];
			if ((t & 0xc0U) != 0x80U) {  /* 10xx xxxx */
				return 0;
			}
			cp = (cp << 6) + (t & 0x3fU);
			ncont--;
		}
		if (cp < mincp || cp > 0x10ffffUL || (cp >= 0xd800UL && cp <= 0xdfffUL)) {
			return 0;
		}
	}

	return 1;
}

/* Check that a size_t is in uint32 range to avoid out-of-range casts. */
static void duk__cbor_encode_sizet_uint32_check(duk_cbor_encode_context *enc_ctx, duk_size_t len) {
	if (DUK_CBOR_UNLIKELY(sizeof(duk_size_t) > sizeof(duk_uint32_t) && len > (duk_size_t) DUK_UINT32_MAX)) {
		duk__cbor_encode_error(enc_ctx);
	}
}

static DUK_CBOR_NOINLINE void duk__cbor_encode_ensure_slowpath(duk_cbor_encode_context *enc_ctx, duk_size_t len) {
	duk_size_t oldlen;
	duk_size_t minlen;
	duk_size_t newlen;
	duk_uint8_t *p_new;
	duk_size_t old_data_len;

	DUK_CBOR_ASSERT(enc_ctx->ptr >= enc_ctx->buf);
	DUK_CBOR_ASSERT(enc_ctx->buf_end >= enc_ctx->ptr);
	DUK_CBOR_ASSERT(enc_ctx->buf_end >= enc_ctx->buf);

	/* Overflow check.
	 *
	 * Limit example: 0xffffffffUL / 2U = 0x7fffffffUL, we reject >= 0x80000000UL.
	 */
	oldlen = enc_ctx->len;
	minlen = oldlen + len;
	if (DUK_CBOR_UNLIKELY(oldlen > DUK_SIZE_MAX / 2U || minlen < oldlen)) {
		duk__cbor_encode_error(enc_ctx);
	}

#if defined(DUK_CBOR_STRESS)
	newlen = oldlen + 1U;
#else
	newlen = oldlen * 2U;
#endif
	DUK_CBOR_ASSERT(newlen >= oldlen);

	if (minlen > newlen) {
		newlen = minlen;
	}
	DUK_CBOR_ASSERT(newlen >= oldlen);
	DUK_CBOR_ASSERT(newlen >= minlen);
	DUK_CBOR_ASSERT(newlen > 0U);

#if defined(DUK_CBOR_DPRINT)
	fprintf(stderr, "cbor encode buffer resized to %ld\n", (long) newlen);
#endif

	p_new = (duk_uint8_t *) duk_resize_buffer(enc_ctx->ctx, enc_ctx->idx_buf, newlen);
	DUK_CBOR_ASSERT(p_new != NULL);
	old_data_len = (duk_size_t) (enc_ctx->ptr - enc_ctx->buf);
	enc_ctx->buf = p_new;
	enc_ctx->buf_end = p_new + newlen;
	enc_ctx->ptr = p_new + old_data_len;
	enc_ctx->len = newlen;
}

static DUK_CBOR_INLINE void duk__cbor_encode_ensure(duk_cbor_encode_context *enc_ctx, duk_size_t len) {
	if (DUK_CBOR_LIKELY((duk_size_t) (enc_ctx->buf_end - enc_ctx->ptr) >= len)) {
		return;
	}
	duk__cbor_encode_ensure_slowpath(enc_ctx, len);
}

static duk_size_t duk__cbor_get_reserve(duk_cbor_encode_context *enc_ctx) {
	DUK_CBOR_ASSERT(enc_ctx->ptr >= enc_ctx->buf);
	DUK_CBOR_ASSERT(enc_ctx->ptr <= enc_ctx->buf_end);
	return (duk_size_t) (enc_ctx->buf_end - enc_ctx->ptr);
}

static void duk__cbor_encode_uint32(duk_cbor_encode_context *enc_ctx, duk_uint32_t u, duk_uint8_t base) {
	duk_uint8_t *p;

	/* Caller must ensure space. */
	DUK_CBOR_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 4);

	p = enc_ctx->ptr;
	if (DUK_CBOR_LIKELY(u <= 23U)) {
		*p++ = (duk_uint8_t) (base + (duk_uint8_t) u);
	} else if (u <= 0xffUL) {
		*p++ = base + 0x18U;
		*p++ = (duk_uint8_t) u;
	} else if (u <= 0xffffUL) {
		*p++ = base + 0x19U;
		duk__cbor_write_uint16_big(p, (duk_uint16_t) u);
		p += 2;
	} else {
		*p++ = base + 0x1aU;
		duk__cbor_write_uint32_big(p, u);
		p += 4;
	}
	enc_ctx->ptr = p;
}

#if defined(DUK_CBOR_DOUBLE_AS_IS)
static void duk__cbor_encode_double(duk_cbor_encode_context *enc_ctx, double d) {
	duk_uint8_t *p;

	/* Caller must ensure space. */
	DUK_CBOR_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 8);

	p = enc_ctx->ptr;
	*p++ = 0xfbU;
	duk__cbor_write_double_big(p, d);
	p += 8;
	enc_ctx->ptr = p;
}
#else  /* DUK_CBOR_DOUBLE_AS_IS */
static void duk__cbor_encode_double_fp(duk_cbor_encode_context *enc_ctx, double d) {
	duk_cbor_dblunion u;
	duk_uint16_t u16;
	duk_int16_t exp;
	duk_uint8_t *p;

	DUK_CBOR_ASSERT(duk__cbor_fpclassify(d) != FP_ZERO);

	/* Caller must ensure space. */
	DUK_CBOR_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 8);

	/* Organize into little endian (no-op if platform is little endian). */
	u.d = d;
	duk__cbor_dblunion_host_to_little(&u);

	/* Check if 'd' can represented as a normal half-float.
	 * Denormal half-floats could also be used, but that check
	 * isn't done now (denormal half-floats are decoded of course).
	 * So just check exponent range and that at most 10 significant
	 * bits (excluding implicit leading 1) are used in 'd'.
	 */
	u16 = (((duk_uint16_t) u.x[7]) << 8) | ((duk_uint16_t) u.x[6]);
	exp = (duk_int16_t) ((u16 & 0x7ff0U) >> 4) - 1023;

	if (exp >= -14 && exp <= 15) {
		/* Half-float normal exponents (excl. denormals).
		 *
		 *          7        6        5        4        3        2        1        0  (LE index)
		 * double: seeeeeee eeeemmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm
		 * half:         seeeee mmmm mmmmmm00 00000000 00000000 00000000 00000000 00000000
		 */
		int use_half_float;

		use_half_float =
		    (u.x[0] == 0 && u.x[1] == 0 && u.x[2] == 0 && u.x[3] == 0 &&
		     u.x[4] == 0 && (u.x[5] & 0x03U) == 0);

		if (use_half_float) {
			duk_uint32_t t;

			exp += 15;
			t = (duk_uint32_t) (u.x[7] & 0x80U) << 8;
			t += (duk_uint32_t) exp << 10;
			t += ((duk_uint32_t) u.x[6] & 0x0fU) << 6;
			t += ((duk_uint32_t) u.x[5]) >> 2;

			/* seeeeemm mmmmmmmm */
			p = enc_ctx->ptr;
			*p++ = 0xf9U;
			duk__cbor_write_uint16_big(p, (duk_uint16_t) t);
			p += 2;
			enc_ctx->ptr = p;
			return;
		}
	}

	/* Same check for plain float.  Also no denormal support here. */
	if (exp >= -126 && exp <= 127) {
		/* Float normal exponents (excl. denormals).
		 *
		 * double: seeeeeee eeeemmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm
		 * float:     seeee eeeemmmm mmmmmmmm mmmmmmmm mmm00000 00000000 00000000 00000000
		 */
		int use_float;
		duk_float_t d_float;

		/* We could do this explicit mantissa check, but doing
		 * a double-float-double cast is fine because we've
		 * already verified that the exponent is in range so
		 * that the narrower cast is not undefined behavior.
		 */
#if 0
		use_float =
		    (u.x[0] == 0 && u.x[1] == 0 && u.x[2] == 0 && (u.x[3] & 0xe0U) == 0);
#endif
		d_float = (duk_float_t) d;
		use_float = ((duk_double_t) d_float == d);
		if (use_float) {
			p = enc_ctx->ptr;
			*p++ = 0xfaU;
			duk__cbor_write_float_big(p, d_float);
			p += 4;
			enc_ctx->ptr = p;
			return;
		}
	}

	/* Special handling for NaN and Inf which we want to encode as
	 * half-floats.  They share the same (maximum) exponent.
	 */
	if (exp == 1024) {
		DUK_CBOR_ASSERT(duk__cbor_isnan(d) || duk__cbor_isinf(d));
		p = enc_ctx->ptr;
		*p++ = 0xf9U;
		if (duk__cbor_isnan(d)) {
			/* Shortest NaN encoding is using a half-float.  Lose the
			 * exact NaN bits in the process.  IEEE double would be
			 * 7ff8 0000 0000 0000, i.e. a quiet NaN in most architectures
			 * (https://en.wikipedia.org/wiki/NaN#Encoding).  The
			 * equivalent half float is 7e00.
			 */
			*p++ = 0x7eU;
		} else {
			/* Shortest +/- Infinity encoding is using a half-float. */
			if (duk__cbor_signbit(d)) {
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
	duk__cbor_write_double_big(p, d);
	p += 8;
	enc_ctx->ptr = p;
}

static void duk__cbor_encode_double(duk_cbor_encode_context *enc_ctx, double d) {
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
	DUK_CBOR_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 8);

	/* Most important path is integers.  The floor() test will be true
	 * for Inf too (but not NaN).
	 */
	d_floor = floor(d);  /* identity if d is +/- 0.0, NaN, or +/- Infinity */
	if (DUK_CBOR_LIKELY(d_floor == d)) {
		DUK_CBOR_ASSERT(!duk__cbor_isnan(d));  /* NaN == NaN compares false. */
		if (duk__cbor_signbit(d)) {
			if (d >= -4294967296.0) {
				d = -1.0 - d;
				if (d >= 0.0) {
					DUK_CBOR_ASSERT(d >= 0.0);
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
				DUK_CBOR_ASSERT(d >= 0.0);
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

	DUK_CBOR_ASSERT(duk__cbor_fpclassify(d) != FP_ZERO);
	duk__cbor_encode_double_fp(enc_ctx, d);
}
#endif  /* DUK_CBOR_DOUBLE_AS_IS */

static void duk__cbor_encode_string_top(duk_cbor_encode_context *enc_ctx) {
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
	DUK_CBOR_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 8);

	str = (const duk_uint8_t *) duk_require_lstring(enc_ctx->ctx, -1, &len);
	if (duk_is_symbol(enc_ctx->ctx, -1)) {
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
	duk__cbor_encode_uint32(enc_ctx, (duk_uint32_t) len,
	                        (DUK_CBOR_LIKELY(duk__cbor_is_utf8_compatible(str, len)) ? 0x60U : 0x40U));
#endif
	duk__cbor_encode_ensure(enc_ctx, len);
	p = enc_ctx->ptr;
	(void) memcpy((void *) p, (const void *) str, len);
	p += len;
	enc_ctx->ptr = p;
}

static void duk__cbor_encode_object(duk_cbor_encode_context *enc_ctx) {
	duk_uint8_t *buf;
	duk_size_t len;
	duk_uint8_t *p;
	duk_size_t i;
	duk_size_t off_ib;
	duk_uint32_t count;

	/* Caller must ensure space. */
	DUK_CBOR_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 8);

	/* XXX: Support for specific built-ins like Date and RegExp. */
	if (duk_is_array(enc_ctx->ctx, -1)) {
		/* Shortest encoding for arrays >= 256 in length is actually
		 * the indefinite length one (3 or more bytes vs. 2 bytes).
		 * We still use the definite length version because it is
		 * more decoding friendly.
		 */
		len = duk_get_length(enc_ctx->ctx, -1);
		duk__cbor_encode_sizet_uint32_check(enc_ctx, len);
		duk__cbor_encode_uint32(enc_ctx, (duk_uint32_t) len, 0x80U);
		for (i = 0; i < len; i++) {
			duk_get_prop_index(enc_ctx->ctx, -1, (duk_uarridx_t) i);
			duk__cbor_encode_value(enc_ctx);
		}
	} else if (duk_is_buffer_data(enc_ctx->ctx, -1)) {
		/* XXX: Tag buffer data?
		 * XXX: Encode typed arrays as integer arrays rather
		 * than buffer data as is?
		 */
		buf = (duk_uint8_t *) duk_require_buffer_data(enc_ctx->ctx, -1, &len);
		duk__cbor_encode_sizet_uint32_check(enc_ctx, len);
		duk__cbor_encode_uint32(enc_ctx, (duk_uint32_t) len, 0x40U);
		duk__cbor_encode_ensure(enc_ctx, len);
		p = enc_ctx->ptr;
		(void) memcpy((void *) p, (const void *) buf, len);
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
		off_ib = (duk_size_t) (enc_ctx->ptr - enc_ctx->buf);  /* XXX: get_offset? */
		count = 0U;
		p = enc_ctx->ptr;
		*p++ = 0xa0U + 0x1fU;  /* indefinite length */
		enc_ctx->ptr = p;
		duk_enum(enc_ctx->ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
		while (duk_next(enc_ctx->ctx, -1, 1 /*get_value*/)) {
			duk_insert(enc_ctx->ctx, -2);  /* [ ... key value ] -> [ ... value key ] */
			duk__cbor_encode_value(enc_ctx);
			duk__cbor_encode_value(enc_ctx);
			count++;
			if (count == 0U) {
				duk__cbor_encode_error(enc_ctx);
			}
		}
		duk_pop(enc_ctx->ctx);
		if (count <= 0x17U) {
			DUK_CBOR_ASSERT(off_ib < enc_ctx->len);
			enc_ctx->buf[off_ib] = 0xa0U + (duk_uint8_t) count;
		} else {
			duk__cbor_encode_ensure(enc_ctx, 1);
			p = enc_ctx->ptr;
			*p++ = 0xffU;  /* break */
			enc_ctx->ptr = p;
		}
	}
}

static void duk__cbor_encode_buffer(duk_cbor_encode_context *enc_ctx) {
	duk_uint8_t *buf;
	duk_size_t len;
	duk_uint8_t *p;

	/* Caller must ensure space. */
	DUK_CBOR_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 8);

	/* Tag buffer data? */
	buf = (duk_uint8_t *) duk_require_buffer(enc_ctx->ctx, -1, &len);
	duk__cbor_encode_sizet_uint32_check(enc_ctx, len);
	duk__cbor_encode_uint32(enc_ctx, (duk_uint32_t) len, 0x40U);
	duk__cbor_encode_ensure(enc_ctx, len);
	p = enc_ctx->ptr;
	(void) memcpy((void *) p, (const void *) buf, len);
	p += len;
	enc_ctx->ptr = p;
}

static void duk__cbor_encode_pointer(duk_cbor_encode_context *enc_ctx) {
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

	ptr = duk_to_string(enc_ctx->ctx, -1);
	DUK_CBOR_ASSERT(ptr != NULL);
	duk_push_sprintf(enc_ctx->ctx, "(%s)", ptr);
	duk_remove(enc_ctx->ctx, -2);
	duk__cbor_encode_string_top(enc_ctx);
}

static void duk__cbor_encode_lightfunc(duk_cbor_encode_context *enc_ctx) {
	duk_uint8_t *p;

	/* Caller must ensure space. */
	DUK_CBOR_ASSERT(duk__cbor_get_reserve(enc_ctx) >= 1 + 8);

	/* For now encode as an empty object. */
	p = enc_ctx->ptr;
	*p++ = 0xa0U;
	enc_ctx->ptr = p;
}

static void duk__cbor_encode_value(duk_cbor_encode_context *enc_ctx) {
	duk_uint8_t *p;

	/* Encode/decode cycle currently loses some type information.
	 * This can be improved by registering custom tags with IANA.
	 */

	/* When working with deeply recursive structures, this is important
	 * to ensure there's no effective depth limit.
	 */
	duk_require_stack(enc_ctx->ctx, 4);

	/* Reserve space for up to 64-bit types (1 initial byte + 8
	 * followup bytes).  This allows encoding of integers, floats,
	 * string/buffer length fields, etc without separate checks
	 * in each code path.
	 */
	duk__cbor_encode_ensure(enc_ctx, 1 + 8);

	switch (duk_get_type(enc_ctx->ctx, -1)) {
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
		duk_uint8_t u8 = duk_get_boolean(enc_ctx->ctx, -1) ? 0xf5U : 0xf4U;
		p = enc_ctx->ptr;
		*p++ = u8;
		enc_ctx->ptr = p;
		break;
	}
	case DUK_TYPE_NUMBER: {
		duk__cbor_encode_double(enc_ctx, duk_get_number(enc_ctx->ctx, -1));
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

	duk_pop(enc_ctx->ctx);
	return;

 fail:
	duk__cbor_encode_error(enc_ctx);
}

/*
 *  Decoding
 */

static void duk__cbor_req_stack(duk_cbor_decode_context *dec_ctx) {
	duk_require_stack(dec_ctx->ctx, 4);
}

static void duk__cbor_decode_error(duk_cbor_decode_context *dec_ctx) {
	(void) duk_type_error(dec_ctx->ctx, "cbor decode error");
}

static duk_uint8_t duk__cbor_decode_readbyte(duk_cbor_decode_context *dec_ctx) {
	DUK_CBOR_ASSERT(dec_ctx->off <= dec_ctx->len);
	if (DUK_CBOR_UNLIKELY(dec_ctx->len - dec_ctx->off < 1U)) {
		duk__cbor_decode_error(dec_ctx);
	}
	return dec_ctx->buf[dec_ctx->off++];
}

static duk_uint16_t duk__cbor_decode_read_u16(duk_cbor_decode_context *dec_ctx) {
	duk_uint16_t res;

	if (DUK_CBOR_UNLIKELY(dec_ctx->len - dec_ctx->off < 2U)) {
		duk__cbor_decode_error(dec_ctx);
	}
	res = duk__cbor_read_uint16_big(dec_ctx->buf + dec_ctx->off);
	dec_ctx->off += 2;
	return res;
}

static duk_uint32_t duk__cbor_decode_read_u32(duk_cbor_decode_context *dec_ctx) {
	duk_uint32_t res;

	if (DUK_CBOR_UNLIKELY(dec_ctx->len - dec_ctx->off < 4U)) {
		duk__cbor_decode_error(dec_ctx);
	}
	res = duk__cbor_read_uint32_big(dec_ctx->buf + dec_ctx->off);
	dec_ctx->off += 4;
	return res;
}

static duk_uint8_t duk__cbor_decode_peekbyte(duk_cbor_decode_context *dec_ctx) {
	if (DUK_CBOR_UNLIKELY(dec_ctx->off >= dec_ctx->len)) {
		duk__cbor_decode_error(dec_ctx);
	}
	return dec_ctx->buf[dec_ctx->off];
}

static void duk__cbor_decode_rewind(duk_cbor_decode_context *dec_ctx, duk_size_t len) {
	DUK_CBOR_ASSERT(len <= dec_ctx->off);  /* Caller must ensure. */
	dec_ctx->off -= len;
}

#if 0
static void duk__cbor_decode_ensure(duk_cbor_decode_context *dec_ctx, duk_size_t len) {
	if (dec_ctx->off + len > dec_ctx->len) {
		duk__cbor_decode_error(dec_ctx);
	}
}
#endif

static const duk_uint8_t *duk__cbor_decode_consume(duk_cbor_decode_context *dec_ctx, duk_size_t len) {
	DUK_CBOR_ASSERT(dec_ctx->off <= dec_ctx->len);
	if (DUK_CBOR_LIKELY(dec_ctx->len - dec_ctx->off >= len)) {
		const duk_uint8_t *res = dec_ctx->buf + dec_ctx->off;
		dec_ctx->off += len;
		return res;
	}

	duk__cbor_decode_error(dec_ctx);  /* Not enough input. */
	return NULL;
}

static int duk__cbor_decode_checkbreak(duk_cbor_decode_context *dec_ctx) {
	if (duk__cbor_decode_peekbyte(dec_ctx) == 0xffU) {
		DUK_CBOR_ASSERT(dec_ctx->off < dec_ctx->len);
		dec_ctx->off++;
#if 0
		(void) duk__cbor_decode_readbyte(dec_ctx);
#endif
		return 1;
	}
	return 0;
}

static void duk__cbor_decode_push_aival_int(duk_cbor_decode_context *dec_ctx, duk_uint8_t ib, duk_bool_t negative) {
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
	case 0x18U:  /* 1 byte */
		t = (duk_uint32_t) duk__cbor_decode_readbyte(dec_ctx);
		goto shared_exit;
	case 0x19U:  /* 2 byte */
		t = (duk_uint32_t) duk__cbor_decode_read_u16(dec_ctx);
		goto shared_exit;
	case 0x1aU:  /* 4 byte */
		t = (duk_uint32_t) duk__cbor_decode_read_u32(dec_ctx);
		goto shared_exit;
	case 0x1bU:  /* 8 byte */
		/* For uint64 it's important to handle the -1.0 part before
		 * casting to double: otherwise the adjustment might be lost
		 * in the cast.  Uses: -1.0 - d <=> -(d + 1.0).
		 */
		t = (duk_uint32_t) duk__cbor_decode_read_u32(dec_ctx);
		t2 = t;
		t = (duk_uint32_t) duk__cbor_decode_read_u32(dec_ctx);
		t1 = t;
#if 0
		t3 = (duk_uint64_t) t2 * 0x100000000ULL + (duk_uint64_t) t1;
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
				return -((duk_double_t) (t3 + 1ULL));
			}
		} else {
			return (duk_double_t) t3;  /* XXX: cast helper */
		}
#endif
#if 0
		t3 = (duk_uint64_t) t2 * 0x100000000ULL + (duk_uint64_t) t1;
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
		d1 = (duk_double_t) t1;  /* XXX: cast helpers */
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
		duk_push_number(dec_ctx->ctx, d);
		return;
	}

	duk__cbor_decode_error(dec_ctx);
	return;

 shared_exit:
	if (negative) {
		/* XXX: a push and check for fastint API would be nice */
		if ((duk_uint_t) t <= (duk_uint_t) -(DUK_INT_MIN + 1)) {
			duk_push_int(dec_ctx->ctx, -1 - ((duk_int_t) t));
		} else {
			duk_push_number(dec_ctx->ctx, -1.0 - (duk_double_t) t);
		}
	} else {
		duk_push_uint(dec_ctx->ctx, (duk_uint_t) t);
	}
}

static void duk__cbor_decode_skip_aival_int(duk_cbor_decode_context *dec_ctx, duk_uint8_t ib) {
	const duk_int8_t skips[32] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 4, 8, -1, -1, -1, -1
	};
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

static duk_uint32_t duk__cbor_decode_aival_uint32(duk_cbor_decode_context *dec_ctx, duk_uint8_t ib) {
	duk_uint8_t ai;
	duk_uint32_t t;

	ai = ib & 0x1fU;
	if (ai <= 0x17U) {
		return (duk_uint32_t) ai;
	}

	switch (ai) {
	case 0x18U:  /* 1 byte */
		t = (duk_uint32_t) duk__cbor_decode_readbyte(dec_ctx);
		return t;
	case 0x19U:  /* 2 byte */
		t = (duk_uint32_t) duk__cbor_decode_read_u16(dec_ctx);
		return t;
	case 0x1aU:  /* 4 byte */
		t = (duk_uint32_t) duk__cbor_decode_read_u32(dec_ctx);
		return t;
	case 0x1bU:  /* 8 byte */
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

static void duk__cbor_decode_buffer(duk_cbor_decode_context *dec_ctx, duk_uint8_t expected_base) {
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
	buf = (duk_uint8_t *) duk_push_fixed_buffer(dec_ctx->ctx, (duk_size_t) len);
	(void) memcpy((void *) buf, (const void *) inp, (size_t) len);
}

static void duk__cbor_decode_join_buffers(duk_cbor_decode_context *dec_ctx, duk_idx_t count) {
	duk_size_t total_size = 0;
	duk_idx_t top = duk_get_top(dec_ctx->ctx);
	duk_idx_t base = top - count;  /* count is >= 1 */
	duk_idx_t idx;
	duk_uint8_t *p = NULL;

	DUK_CBOR_ASSERT(count >= 1);
	DUK_CBOR_ASSERT(top >= count);

	for (;;) {
		/* First round: compute total size.
		 * Second round: copy into place.
		 */
		for (idx = base; idx < top; idx++) {
			duk_uint8_t *buf_data;
			duk_size_t buf_size;

			buf_data = (duk_uint8_t *) duk_require_buffer(dec_ctx->ctx, idx, &buf_size);
			if (p != NULL) {
				if (buf_size > 0U) {
					(void) memcpy((void *) p, (const void *) buf_data, buf_size);
				}
				p += buf_size;
			} else {
				total_size += buf_size;
				if (DUK_CBOR_UNLIKELY(total_size < buf_size)) {  /* Wrap check. */
					duk__cbor_decode_error(dec_ctx);
				}
			}
		}

		if (p != NULL) {
			break;
		} else {
			p = (duk_uint8_t *) duk_push_fixed_buffer(dec_ctx->ctx, total_size);
			DUK_CBOR_ASSERT(p != NULL);
		}
	}

	duk_replace(dec_ctx->ctx, base);
	duk_pop_n(dec_ctx->ctx, count - 1);
}

static void duk__cbor_decode_and_join_strbuf(duk_cbor_decode_context *dec_ctx, duk_uint8_t expected_base) {
	duk_idx_t count = 0;
	for (;;) {
		if (duk__cbor_decode_checkbreak(dec_ctx)) {
			break;
		}
		duk_require_stack(dec_ctx->ctx, 1);
		duk__cbor_decode_buffer(dec_ctx, expected_base);
		count++;
		if (DUK_UNLIKELY(count <= 0)) {  /* Wrap check. */
			duk__cbor_decode_error(dec_ctx);
		}
	}
	if (count == 0) {
		(void) duk_push_fixed_buffer(dec_ctx->ctx, 0);
	} else if (count > 1) {
		duk__cbor_decode_join_buffers(dec_ctx, count);
	}
}

static duk_double_t duk__cbor_decode_half_float(duk_cbor_decode_context *dec_ctx) {
	duk_cbor_dblunion u;
	const duk_uint8_t *inp;
	duk_int_t exp;
	duk_uint_t u16;
	duk_uint_t tmp;
	duk_double_t res;

	inp = duk__cbor_decode_consume(dec_ctx, 2);
	u16 = ((duk_uint_t) inp[0] << 8) + (duk_uint_t) inp[1];
	exp = (duk_int_t) ((u16 >> 10) & 0x1fU) - 15;

	/* Reconstruct IEEE double into little endian order first, then convert
	 * to host order.
	 */

	memset((void *) &u, 0, sizeof(u));

	if (exp == -15) {
		/* Zero or denormal; but note that half float
		 * denormals become double normals.
		 */
		if ((u16 & 0x03ffU) == 0) {
			u.x[7] = inp[0] & 0x80U;
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
			u.x[7] = 0x3fU;
			u.x[6] = 0x10U + (duk_uint8_t) ((u16 >> 6) & 0x0fU);
			u.x[5] = (duk_uint8_t) ((u16 << 2) & 0xffU);  /* Mask is really 0xfcU */

			duk__cbor_dblunion_little_to_host(&u);
			res = u.d - 0.00006103515625;  /* 2^(-14) */
			if (u16 & 0x8000U) {
				res = -res;
			}
			return res;
		}
	} else if (exp == 16) {
		/* +/- Inf or NaN. */
		if ((u16 & 0x03ffU) == 0) {
			u.x[7] = (inp[0] & 0x80U) + 0x7fU;
			u.x[6] = 0xf0U;
		} else {
			/* Create a 'quiet NaN' with highest
			 * bit set (there are some platforms
			 * where the NaN payload convention is
			 * the opposite).  Keep sign.
			 */
			u.x[7] = (inp[0] & 0x80U) + 0x7fU;
			u.x[6] = 0xf8U;
		}
	} else {
		/* Normal. */
		tmp = (inp[0] & 0x80U) ? 0x80000000UL : 0UL;
		tmp += (duk_uint_t) (exp + 1023) << 20;
		tmp += (duk_uint_t) (inp[0] & 0x03U) << 18;
		tmp += (duk_uint_t) (inp[1] & 0xffU) << 10;
		u.x[7] = (tmp >> 24) & 0xffU;
		u.x[6] = (tmp >> 16) & 0xffU;
		u.x[5] = (tmp >> 8) & 0xffU;
		u.x[4] = (tmp >> 0) & 0xffU;
	}

	duk__cbor_dblunion_little_to_host(&u);
	return u.d;
}

static void duk__cbor_decode_string(duk_cbor_decode_context *dec_ctx, duk_uint8_t ib, duk_uint8_t ai) {
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
		buf_data = (duk_uint8_t *) duk_require_buffer(dec_ctx->ctx, -1, &buf_size);
		(void) duk_push_lstring(dec_ctx->ctx, (const char *) buf_data, buf_size);
		duk_remove(dec_ctx->ctx, -2);
	} else {
		duk_uint32_t len;
		const duk_uint8_t *inp;

		len = duk__cbor_decode_aival_uint32(dec_ctx, ib);
		inp = duk__cbor_decode_consume(dec_ctx, len);
		(void) duk_push_lstring(dec_ctx->ctx, (const char *) inp, (duk_size_t) len);
	}
	if (duk_is_symbol(dec_ctx->ctx, -1)) {
		/* Refuse to create Symbols when decoding. */
		duk__cbor_decode_error(dec_ctx);
	}

	/* XXX: Here a Duktape API call to convert input -> utf-8 with
	 * replacements would be nice.
	 */
}

static duk_bool_t duk__cbor_decode_array(duk_cbor_decode_context *dec_ctx, duk_uint8_t ib, duk_uint8_t ai) {
	duk_uint32_t idx, len;

	duk__cbor_req_stack(dec_ctx);

	/* Support arrays up to 0xfffffffeU in length.  0xffffffff is
	 * used as an indefinite length marker.
	 */
	if (ai == 0x1fU) {
		len = 0xffffffffUL;
	} else {
		len = duk__cbor_decode_aival_uint32(dec_ctx, ib);
		if (len == 0xffffffffUL) {
			return 0;
		}
	}

	/* XXX: use bare array? */
	duk_push_array(dec_ctx->ctx);
	for (idx = 0U; ;) {
		if (len == 0xffffffffUL && duk__cbor_decode_checkbreak(dec_ctx)) {
			break;
		}
		if (idx == len) {
			if (ai == 0x1fU) {
				return 0;
			}
			break;
		}
		duk__cbor_decode_value(dec_ctx);
		duk_put_prop_index(dec_ctx->ctx, -2, (duk_uarridx_t) idx);
		idx++;
		if (idx == 0U) {
			return 0;  /* wrapped */
		}
	}

	return 1;
}

static duk_bool_t duk__cbor_decode_map(duk_cbor_decode_context *dec_ctx, duk_uint8_t ib, duk_uint8_t ai) {
	duk_uint32_t count;

	duk__cbor_req_stack(dec_ctx);

	if (ai == 0x1fU) {
		count = 0xffffffffUL;
	} else {
		count = duk__cbor_decode_aival_uint32(dec_ctx, ib);
		if (count == 0xffffffffUL) {
			return 0;
		}
	}

	/* XXX: use bare object? */
	duk_push_object(dec_ctx->ctx);
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
		duk_put_prop(dec_ctx->ctx, -3);
	}

	return 1;
}

static duk_double_t duk__cbor_decode_float(duk_cbor_decode_context *dec_ctx) {
	duk_cbor_fltunion u;
	const duk_uint8_t *inp;
	inp = duk__cbor_decode_consume(dec_ctx, 4);
	(void) memcpy((void *) u.x, (const void *) inp, 4);
	duk__cbor_fltunion_big_to_host(&u);
	return (duk_double_t) u.f;
}

static duk_double_t duk__cbor_decode_double(duk_cbor_decode_context *dec_ctx) {
	duk_cbor_dblunion u;
	const duk_uint8_t *inp;
	inp = duk__cbor_decode_consume(dec_ctx, 8);
	(void) memcpy((void *) u.x, (const void *) inp, 8);
	duk__cbor_dblunion_big_to_host(&u);
	return u.d;
}

#if defined(DUK_CBOR_DECODE_FASTPATH)
#define DUK__CBOR_AI  (ib & 0x1fU)

static void duk__cbor_decode_value(duk_cbor_decode_context *dec_ctx) {
	duk_uint8_t ib;

	/* Any paths potentially recursing back to duk__cbor_decode_value()
	 * must perform a Duktape value stack growth check.  Avoid the check
	 * here for simple paths like primitive values.
	 */

 reread_initial_byte:
#if defined(DUK_CBOR_DPRINT)
	fprintf(stderr, "cbor decode off=%ld len=%ld\n", (long) dec_ctx->off, (long) dec_ctx->len);
#endif

	ib = duk__cbor_decode_readbyte(dec_ctx);

	/* Full initial byte switch, footprint cost over baseline is ~+1kB. */
	/* XXX: Force full switch with no range check. */

	switch (ib) {
	case 0x00U: case 0x01U: case 0x02U: case 0x03U: case 0x04U: case 0x05U: case 0x06U: case 0x07U:
	case 0x08U: case 0x09U: case 0x0aU: case 0x0bU: case 0x0cU: case 0x0dU: case 0x0eU: case 0x0fU:
	case 0x10U: case 0x11U: case 0x12U: case 0x13U: case 0x14U: case 0x15U: case 0x16U: case 0x17U:
		duk_push_uint(dec_ctx->ctx, ib);
		break;
	case 0x18U: case 0x19U: case 0x1aU: case 0x1bU:
		duk__cbor_decode_push_aival_int(dec_ctx, ib, 0 /*negative*/);
		break;
	case 0x1cU: case 0x1dU: case 0x1eU: case 0x1fU:
		goto format_error;
	case 0x20U: case 0x21U: case 0x22U: case 0x23U: case 0x24U: case 0x25U: case 0x26U: case 0x27U:
	case 0x28U: case 0x29U: case 0x2aU: case 0x2bU: case 0x2cU: case 0x2dU: case 0x2eU: case 0x2fU:
	case 0x30U: case 0x31U: case 0x32U: case 0x33U: case 0x34U: case 0x35U: case 0x36U: case 0x37U:
		duk_push_int(dec_ctx->ctx, -((duk_int_t) ((ib - 0x20U) + 1U)));
		break;
	case 0x38U: case 0x39U: case 0x3aU: case 0x3bU:
		duk__cbor_decode_push_aival_int(dec_ctx, ib, 1 /*negative*/);
		break;
	case 0x3cU: case 0x3dU: case 0x3eU: case 0x3fU:
		goto format_error;
	case 0x40U: case 0x41U: case 0x42U: case 0x43U: case 0x44U: case 0x45U: case 0x46U: case 0x47U:
	case 0x48U: case 0x49U: case 0x4aU: case 0x4bU: case 0x4cU: case 0x4dU: case 0x4eU: case 0x4fU:
	case 0x50U: case 0x51U: case 0x52U: case 0x53U: case 0x54U: case 0x55U: case 0x56U: case 0x57U:
		/* XXX: Avoid rewind, we know the length already. */
		DUK_CBOR_ASSERT(dec_ctx->off > 0U);
		dec_ctx->off--;
		duk__cbor_decode_buffer(dec_ctx, 0x40U);
		break;
	case 0x58U: case 0x59U: case 0x5aU: case 0x5bU:
		/* XXX: Avoid rewind, decode length inline. */
		DUK_CBOR_ASSERT(dec_ctx->off > 0U);
		dec_ctx->off--;
		duk__cbor_decode_buffer(dec_ctx, 0x40U);
		break;
	case 0x5cU: case 0x5dU: case 0x5eU:
		goto format_error;
	case 0x5fU:
		duk__cbor_decode_and_join_strbuf(dec_ctx, 0x40U);
		break;
	case 0x60U: case 0x61U: case 0x62U: case 0x63U: case 0x64U: case 0x65U: case 0x66U: case 0x67U:
	case 0x68U: case 0x69U: case 0x6aU: case 0x6bU: case 0x6cU: case 0x6dU: case 0x6eU: case 0x6fU:
	case 0x70U: case 0x71U: case 0x72U: case 0x73U: case 0x74U: case 0x75U: case 0x76U: case 0x77U:
		/* XXX: Avoid double decode of length. */
		duk__cbor_decode_string(dec_ctx, ib, DUK__CBOR_AI);
		break;
	case 0x78U: case 0x79U: case 0x7aU: case 0x7bU:
		/* XXX: Avoid double decode of length. */
		duk__cbor_decode_string(dec_ctx, ib, DUK__CBOR_AI);
		break;
	case 0x7cU: case 0x7dU: case 0x7eU:
		goto format_error;
	case 0x7fU:
		duk__cbor_decode_string(dec_ctx, ib, DUK__CBOR_AI);
		break;
	case 0x80U: case 0x81U: case 0x82U: case 0x83U: case 0x84U: case 0x85U: case 0x86U: case 0x87U:
	case 0x88U: case 0x89U: case 0x8aU: case 0x8bU: case 0x8cU: case 0x8dU: case 0x8eU: case 0x8fU:
	case 0x90U: case 0x91U: case 0x92U: case 0x93U: case 0x94U: case 0x95U: case 0x96U: case 0x97U:
		if (DUK_CBOR_UNLIKELY(duk__cbor_decode_array(dec_ctx, ib, DUK__CBOR_AI) == 0)) {
			goto format_error;
		}
		break;
	case 0x98U: case 0x99U: case 0x9aU: case 0x9bU:
		if (DUK_CBOR_UNLIKELY(duk__cbor_decode_array(dec_ctx, ib, DUK__CBOR_AI) == 0)) {
			goto format_error;
		}
		break;
	case 0x9cU: case 0x9dU: case 0x9eU:
		goto format_error;
	case 0x9fU:
		if (DUK_CBOR_UNLIKELY(duk__cbor_decode_array(dec_ctx, ib, DUK__CBOR_AI) == 0)) {
			goto format_error;
		}
		break;
	case 0xa0U: case 0xa1U: case 0xa2U: case 0xa3U: case 0xa4U: case 0xa5U: case 0xa6U: case 0xa7U:
	case 0xa8U: case 0xa9U: case 0xaaU: case 0xabU: case 0xacU: case 0xadU: case 0xaeU: case 0xafU:
	case 0xb0U: case 0xb1U: case 0xb2U: case 0xb3U: case 0xb4U: case 0xb5U: case 0xb6U: case 0xb7U:
		if (DUK_CBOR_UNLIKELY(duk__cbor_decode_map(dec_ctx, ib, DUK__CBOR_AI) == 0)) {
			goto format_error;
		}
		break;
	case 0xb8U: case 0xb9U: case 0xbaU: case 0xbbU:
		if (DUK_CBOR_UNLIKELY(duk__cbor_decode_map(dec_ctx, ib, DUK__CBOR_AI) == 0)) {
			goto format_error;
		}
		break;
	case 0xbcU: case 0xbdU: case 0xbeU:
		goto format_error;
	case 0xbfU:
		if (DUK_CBOR_UNLIKELY(duk__cbor_decode_map(dec_ctx, ib, DUK__CBOR_AI) == 0)) {
			goto format_error;
		}
		break;
	case 0xc0U: case 0xc1U: case 0xc2U: case 0xc3U: case 0xc4U: case 0xc5U: case 0xc6U: case 0xc7U:
	case 0xc8U: case 0xc9U: case 0xcaU: case 0xcbU: case 0xccU: case 0xcdU: case 0xceU: case 0xcfU:
	case 0xd0U: case 0xd1U: case 0xd2U: case 0xd3U: case 0xd4U: case 0xd5U: case 0xd6U: case 0xd7U:
		/* Tag 0-23: drop. */
		goto reread_initial_byte;
	case 0xd8U: case 0xd9U: case 0xdaU: case 0xdbU:
		duk__cbor_decode_skip_aival_int(dec_ctx, ib);
		goto reread_initial_byte;
	case 0xdcU: case 0xddU: case 0xdeU: case 0xdfU:
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
		duk_push_false(dec_ctx->ctx);
		break;
	case 0xf5U:
		duk_push_true(dec_ctx->ctx);
		break;
	case 0xf6U:
		duk_push_null(dec_ctx->ctx);
		break;
	case 0xf7U:
		duk_push_undefined(dec_ctx->ctx);
		break;
	case 0xf8U:
		/* Simple value 32-255, nothing defined yet, so reject. */
		goto format_error;
	case 0xf9U: {
		duk_double_t d;
		d = duk__cbor_decode_half_float(dec_ctx);
		duk_push_number(dec_ctx->ctx, d);
		break;
	}
	case 0xfaU: {
		duk_double_t d;
		d = duk__cbor_decode_float(dec_ctx);
		duk_push_number(dec_ctx->ctx, d);
		break;
	}
	case 0xfbU: {
		duk_double_t d;
		d = duk__cbor_decode_double(dec_ctx);
		duk_push_number(dec_ctx->ctx, d);
		break;
	}
	case 0xfcU:
	case 0xfdU:
	case 0xfeU:
	case 0xffU:
		goto format_error;
	}  /* end switch */

	return;

 format_error:
	duk__cbor_decode_error(dec_ctx);
}
#else  /* DUK_CBOR_DECODE_FASTPATH */
static void duk__cbor_decode_value(duk_cbor_decode_context *dec_ctx) {
	duk_uint8_t ib, mt, ai;

	/* Any paths potentially recursing back to duk__cbor_decode_value()
	 * must perform a Duktape value stack growth check.  Avoid the check
	 * here for simple paths like primitive values.
	 */

 reread_initial_byte:
#if defined(DUK_CBOR_DPRINT)
	fprintf(stderr, "cbor decode off=%ld len=%ld\n", (long) dec_ctx->off, (long) dec_ctx->len);
#endif

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
	case 0U: {  /* unsigned integer */
		duk__cbor_decode_push_aival_int(dec_ctx, ib, 0 /*negative*/);
		break;
	}
	case 1U: {  /* negative integer */
		duk__cbor_decode_push_aival_int(dec_ctx, ib, 1 /*negative*/);
		break;
	}
	case 2U: {  /* byte string */
		if (ai == 0x1fU) {
			duk__cbor_decode_and_join_strbuf(dec_ctx, 0x40U);
		} else {
			duk__cbor_decode_rewind(dec_ctx, 1U);
			duk__cbor_decode_buffer(dec_ctx, 0x40U);
		}
		break;
	}
	case 3U: {  /* text string */
		duk__cbor_decode_string(dec_ctx, ib, ai);
		break;
	}
	case 4U: {  /* array of data items */
		if (DUK_CBOR_UNLIKELY(duk__cbor_decode_array(dec_ctx, ib, ai) == 0)) {
			goto format_error;
		}
		break;
	}
	case 5U: {  /* map of pairs of data items */
		if (DUK_CBOR_UNLIKELY(duk__cbor_decode_map(dec_ctx, ib, ai) == 0)) {
			goto format_error;
		}
		break;
	}
	case 6U: {  /* semantic tagging */
		/* Tags are ignored now, re-read initial byte.  A tagged
		 * value may itself be tagged (an unlimited number of times)
		 * so keep on peeling away tags.
		 */
		duk__cbor_decode_skip_aival_int(dec_ctx, ib);
		goto reread_initial_byte;
	}
	case 7U: {  /* floating point numbers, simple data types, break; other */
		switch (ai) {
		case 0x14U: {
			duk_push_false(dec_ctx->ctx);
			break;
		}
		case 0x15U: {
			duk_push_true(dec_ctx->ctx);
			break;
		}
		case 0x16U: {
			duk_push_null(dec_ctx->ctx);
			break;
		}
		case 0x17U: {
			duk_push_undefined(dec_ctx->ctx);
			break;
		}
		case 0x18U: {  /* more simple values (1 byte) */
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
		case 0x19U: {  /* half-float (2 bytes) */
			duk_double_t d;
			d = duk__cbor_decode_half_float(dec_ctx);
			duk_push_number(dec_ctx->ctx, d);
			break;
		}
		case 0x1aU: {  /* float (4 bytes) */
			duk_double_t d;
			d = duk__cbor_decode_float(dec_ctx);
			duk_push_number(dec_ctx->ctx, d);
			break;
		}
		case 0x1bU: {  /* double (8 bytes) */
			duk_double_t d;
			d = duk__cbor_decode_double(dec_ctx);
			duk_push_number(dec_ctx->ctx, d);
			break;
		}
		case 0xffU:  /* unexpected break */
		default: {
			goto format_error;
		}
		}  /* end switch */
		break;
	}
	default: {
		goto format_error;  /* will never actually occur */
	}
	}  /* end switch */

	return;

 format_error:
	duk__cbor_decode_error(dec_ctx);
}
#endif  /* DUK_CBOR_DECODE_FASTPATH */

/*
 *  Public APIs
 */

static duk_ret_t duk__cbor_encode_binding(duk_context *ctx) {
	/* Produce an ArrayBuffer by first decoding into a plain buffer which
	 * mimics a Uint8Array and gettings its .buffer property.
	 */
	duk_cbor_encode(ctx, -1, 0);
	duk_get_prop_string(ctx, -1, "buffer");
	return 1;
}

static duk_ret_t duk__cbor_decode_binding(duk_context *ctx) {
	/* Lenient: accept any buffer like. */
	duk_cbor_decode(ctx, -1, 0);
	return 1;
}

void duk_cbor_init(duk_context *ctx, duk_uint_t flags) {
	(void) flags;
	duk_push_global_object(ctx);
	duk_push_string(ctx, "CBOR");
	duk_push_object(ctx);
	duk_push_string(ctx, "encode");
	duk_push_c_function(ctx, duk__cbor_encode_binding, 1);
	duk_def_prop(ctx, -3, DUK_DEFPROP_ATTR_WC | DUK_DEFPROP_HAVE_VALUE);
	duk_push_string(ctx, "decode");
	duk_push_c_function(ctx, duk__cbor_decode_binding, 1);
	duk_def_prop(ctx, -3, DUK_DEFPROP_ATTR_WC | DUK_DEFPROP_HAVE_VALUE);
	duk_def_prop(ctx, -3, DUK_DEFPROP_ATTR_WC | DUK_DEFPROP_HAVE_VALUE);
	duk_pop(ctx);
}

void duk_cbor_encode(duk_context *ctx, duk_idx_t idx, duk_uint_t encode_flags) {
	duk_cbor_encode_context enc_ctx;
	duk_uint8_t *buf;

	(void) encode_flags;

	idx = duk_require_normalize_index(ctx, idx);

	enc_ctx.ctx = ctx;
	enc_ctx.idx_buf = duk_get_top(ctx);

	enc_ctx.len = 64;
	buf = (duk_uint8_t *) duk_push_dynamic_buffer(ctx, enc_ctx.len);
	enc_ctx.ptr = buf;
	enc_ctx.buf = buf;
	enc_ctx.buf_end = buf + enc_ctx.len;

	duk_dup(ctx, idx);
	duk__cbor_encode_value(&enc_ctx);
	duk_resize_buffer(enc_ctx.ctx, enc_ctx.idx_buf, (duk_size_t) (enc_ctx.ptr - enc_ctx.buf));
	duk_replace(ctx, idx);
}

void duk_cbor_decode(duk_context *ctx, duk_idx_t idx, duk_uint_t decode_flags) {
	duk_cbor_decode_context dec_ctx;

	(void) decode_flags;

	/* Suppress compile warnings for functions only needed with e.g.
	 * asserts enabled.
	 */
	(void) duk__cbor_get_reserve;
	(void) duk__cbor_isinf;
	(void) duk__cbor_fpclassify;

	idx = duk_require_normalize_index(ctx, idx);

	dec_ctx.ctx = ctx;
	dec_ctx.buf = (const duk_uint8_t *) duk_require_buffer_data(ctx, idx, &dec_ctx.len);
	dec_ctx.off = 0;
	/* dec_ctx.len: set above */

	duk__cbor_req_stack(&dec_ctx);
	duk__cbor_decode_value(&dec_ctx);
	if (dec_ctx.off != dec_ctx.len) {
		(void) duk_type_error(ctx, "trailing garbage");
	}

	duk_replace(ctx, idx);
}
