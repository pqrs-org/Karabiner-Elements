/*
 *  Macro support functions for reading/writing raw data.
 *
 *  These are done using memcpy to ensure they're valid even for unaligned
 *  reads/writes on platforms where alignment counts.  On x86 at least gcc
 *  is able to compile these into a bswap+mov.  "Always inline" is used to
 *  ensure these macros compile to minimal code.
 */

#include "duk_internal.h"

union duk__u16_union {
	duk_uint8_t b[2];
	duk_uint16_t x;
};
typedef union duk__u16_union duk__u16_union;

union duk__u32_union {
	duk_uint8_t b[4];
	duk_uint32_t x;
};
typedef union duk__u32_union duk__u32_union;

#if defined(DUK_USE_64BIT_OPS)
union duk__u64_union {
	duk_uint8_t b[8];
	duk_uint64_t x;
};
typedef union duk__u64_union duk__u64_union;
#endif

DUK_INTERNAL DUK_ALWAYS_INLINE duk_uint16_t duk_raw_read_u16_be(const duk_uint8_t *p) {
	duk__u16_union u;
	duk_memcpy((void *) u.b, (const void *) p, (size_t) 2);
	u.x = DUK_NTOH16(u.x);
	return u.x;
}

DUK_INTERNAL DUK_ALWAYS_INLINE duk_uint32_t duk_raw_read_u32_be(const duk_uint8_t *p) {
	duk__u32_union u;
	duk_memcpy((void *) u.b, (const void *) p, (size_t) 4);
	u.x = DUK_NTOH32(u.x);
	return u.x;
}

DUK_INTERNAL DUK_ALWAYS_INLINE duk_float_t duk_raw_read_float_be(const duk_uint8_t *p) {
	duk_float_union fu;
	duk_memcpy((void *) fu.uc, (const void *) p, (size_t) 4);
	duk_fltunion_big_to_host(&fu);
	return fu.f;
}

DUK_INTERNAL DUK_ALWAYS_INLINE duk_double_t duk_raw_read_double_be(const duk_uint8_t *p) {
	duk_double_union du;
	duk_memcpy((void *) du.uc, (const void *) p, (size_t) 8);
	duk_dblunion_big_to_host(&du);
	return du.d;
}

DUK_INTERNAL DUK_ALWAYS_INLINE duk_uint16_t duk_raw_readinc_u16_be(const duk_uint8_t **p) {
	duk_uint16_t res = duk_raw_read_u16_be(*p);
	*p += 2;
	return res;
}

DUK_INTERNAL DUK_ALWAYS_INLINE duk_uint32_t duk_raw_readinc_u32_be(const duk_uint8_t **p) {
	duk_uint32_t res = duk_raw_read_u32_be(*p);
	*p += 4;
	return res;
}

DUK_INTERNAL DUK_ALWAYS_INLINE duk_float_t duk_raw_readinc_float_be(const duk_uint8_t **p) {
	duk_float_t res = duk_raw_read_float_be(*p);
	*p += 4;
	return res;
}

DUK_INTERNAL DUK_ALWAYS_INLINE duk_double_t duk_raw_readinc_double_be(const duk_uint8_t **p) {
	duk_double_t res = duk_raw_read_double_be(*p);
	*p += 8;
	return res;
}

DUK_INTERNAL DUK_ALWAYS_INLINE void duk_raw_write_u16_be(duk_uint8_t *p, duk_uint16_t val) {
	duk__u16_union u;
	u.x = DUK_HTON16(val);
	duk_memcpy((void *) p, (const void *) u.b, (size_t) 2);
}

DUK_INTERNAL DUK_ALWAYS_INLINE void duk_raw_write_u32_be(duk_uint8_t *p, duk_uint32_t val) {
	duk__u32_union u;
	u.x = DUK_HTON32(val);
	duk_memcpy((void *) p, (const void *) u.b, (size_t) 4);
}

DUK_INTERNAL DUK_ALWAYS_INLINE void duk_raw_write_float_be(duk_uint8_t *p, duk_float_t val) {
	duk_float_union fu;
	fu.f = val;
	duk_fltunion_host_to_big(&fu);
	duk_memcpy((void *) p, (const void *) fu.uc, (size_t) 4);
}

DUK_INTERNAL DUK_ALWAYS_INLINE void duk_raw_write_double_be(duk_uint8_t *p, duk_double_t val) {
	duk_double_union du;
	du.d = val;
	duk_dblunion_host_to_big(&du);
	duk_memcpy((void *) p, (const void *) du.uc, (size_t) 8);
}

DUK_INTERNAL duk_small_int_t duk_raw_write_xutf8(duk_uint8_t *p, duk_ucodepoint_t val) {
	duk_small_int_t len = duk_unicode_encode_xutf8(val, p);
	return len;
}

DUK_INTERNAL duk_small_int_t duk_raw_write_cesu8(duk_uint8_t *p, duk_ucodepoint_t val) {
	duk_small_int_t len = duk_unicode_encode_cesu8(val, p);
	return len;
}

DUK_INTERNAL DUK_ALWAYS_INLINE void duk_raw_writeinc_u16_be(duk_uint8_t **p, duk_uint16_t val) {
	duk_raw_write_u16_be(*p, val);
	*p += 2;
}

DUK_INTERNAL DUK_ALWAYS_INLINE void duk_raw_writeinc_u32_be(duk_uint8_t **p, duk_uint32_t val) {
	duk_raw_write_u32_be(*p, val);
	*p += 4;
}

DUK_INTERNAL DUK_ALWAYS_INLINE void duk_raw_writeinc_float_be(duk_uint8_t **p, duk_float_t val) {
	duk_raw_write_float_be(*p, val);
	*p += 4;
}

DUK_INTERNAL DUK_ALWAYS_INLINE void duk_raw_writeinc_double_be(duk_uint8_t **p, duk_double_t val) {
	duk_raw_write_double_be(*p, val);
	*p += 8;
}

DUK_INTERNAL void duk_raw_writeinc_xutf8(duk_uint8_t **p, duk_ucodepoint_t val) {
	duk_small_int_t len = duk_unicode_encode_xutf8(val, *p);
	*p += len;
}

DUK_INTERNAL void duk_raw_writeinc_cesu8(duk_uint8_t **p, duk_ucodepoint_t val) {
	duk_small_int_t len = duk_unicode_encode_cesu8(val, *p);
	*p += len;
}
