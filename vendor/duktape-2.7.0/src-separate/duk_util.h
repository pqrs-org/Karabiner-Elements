/*
 *  Utilities
 */

#if !defined(DUK_UTIL_H_INCLUDED)
#define DUK_UTIL_H_INCLUDED

/*
 *  Some useful constants
 */

#define DUK_DOUBLE_2TO32  4294967296.0
#define DUK_DOUBLE_2TO31  2147483648.0
#define DUK_DOUBLE_LOG2E  1.4426950408889634
#define DUK_DOUBLE_LOG10E 0.4342944819032518

/*
 *  Endian conversion
 */

#if defined(DUK_USE_INTEGER_LE)
#define DUK_HTON32(x) DUK_BSWAP32((x))
#define DUK_NTOH32(x) DUK_BSWAP32((x))
#define DUK_HTON16(x) DUK_BSWAP16((x))
#define DUK_NTOH16(x) DUK_BSWAP16((x))
#elif defined(DUK_USE_INTEGER_BE)
#define DUK_HTON32(x) (x)
#define DUK_NTOH32(x) (x)
#define DUK_HTON16(x) (x)
#define DUK_NTOH16(x) (x)
#else
#error internal error, endianness defines broken
#endif

/*
 *  Bitstream decoder
 */

struct duk_bitdecoder_ctx {
	const duk_uint8_t *data;
	duk_size_t offset;
	duk_size_t length;
	duk_uint32_t currval;
	duk_small_int_t currbits;
};

#define DUK_BD_BITPACKED_STRING_MAXLEN 256

/*
 *  Bitstream encoder
 */

struct duk_bitencoder_ctx {
	duk_uint8_t *data;
	duk_size_t offset;
	duk_size_t length;
	duk_uint32_t currval;
	duk_small_int_t currbits;
	duk_small_int_t truncated;
};

/*
 *  Raw write/read macros for big endian, unaligned basic values.
 *  Caller ensures there's enough space.  The INC macro variants
 *  update the pointer argument automatically.
 */

#define DUK_RAW_WRITE_U8(ptr, val) \
	do { \
		*(ptr) = (duk_uint8_t) (val); \
	} while (0)
#define DUK_RAW_WRITE_U16_BE(ptr, val)    duk_raw_write_u16_be((ptr), (duk_uint16_t) (val))
#define DUK_RAW_WRITE_U32_BE(ptr, val)    duk_raw_write_u32_be((ptr), (duk_uint32_t) (val))
#define DUK_RAW_WRITE_FLOAT_BE(ptr, val)  duk_raw_write_float_be((ptr), (duk_float_t) (val))
#define DUK_RAW_WRITE_DOUBLE_BE(ptr, val) duk_raw_write_double_be((ptr), (duk_double_t) (val))
#define DUK_RAW_WRITE_XUTF8(ptr, val)     duk_raw_write_xutf8((ptr), (duk_ucodepoint_t) (val))

#define DUK_RAW_WRITEINC_U8(ptr, val) \
	do { \
		*(ptr)++ = (duk_uint8_t) (val); \
	} while (0)
#define DUK_RAW_WRITEINC_U16_BE(ptr, val)    duk_raw_writeinc_u16_be(&(ptr), (duk_uint16_t) (val))
#define DUK_RAW_WRITEINC_U32_BE(ptr, val)    duk_raw_writeinc_u32_be(&(ptr), (duk_uint32_t) (val))
#define DUK_RAW_WRITEINC_FLOAT_BE(ptr, val)  duk_raw_writeinc_float_be(&(ptr), (duk_float_t) (val))
#define DUK_RAW_WRITEINC_DOUBLE_BE(ptr, val) duk_raw_writeinc_double_be(&(ptr), (duk_double_t) (val))
#define DUK_RAW_WRITEINC_XUTF8(ptr, val)     duk_raw_writeinc_xutf8(&(ptr), (duk_ucodepoint_t) (val))
#define DUK_RAW_WRITEINC_CESU8(ptr, val)     duk_raw_writeinc_cesu8(&(ptr), (duk_ucodepoint_t) (val))

#define DUK_RAW_READ_U8(ptr)        ((duk_uint8_t) (*(ptr)))
#define DUK_RAW_READ_U16_BE(ptr)    duk_raw_read_u16_be((ptr));
#define DUK_RAW_READ_U32_BE(ptr)    duk_raw_read_u32_be((ptr));
#define DUK_RAW_READ_DOUBLE_BE(ptr) duk_raw_read_double_be((ptr));

#define DUK_RAW_READINC_U8(ptr)        ((duk_uint8_t) (*(ptr)++))
#define DUK_RAW_READINC_U16_BE(ptr)    duk_raw_readinc_u16_be(&(ptr));
#define DUK_RAW_READINC_U32_BE(ptr)    duk_raw_readinc_u32_be(&(ptr));
#define DUK_RAW_READINC_DOUBLE_BE(ptr) duk_raw_readinc_double_be(&(ptr));

/*
 *  Double and float byte order operations.
 */

DUK_INTERNAL_DECL void duk_dblunion_host_to_little(duk_double_union *u);
DUK_INTERNAL_DECL void duk_dblunion_little_to_host(duk_double_union *u);
DUK_INTERNAL_DECL void duk_dblunion_host_to_big(duk_double_union *u);
DUK_INTERNAL_DECL void duk_dblunion_big_to_host(duk_double_union *u);
DUK_INTERNAL_DECL void duk_fltunion_host_to_big(duk_float_union *u);
DUK_INTERNAL_DECL void duk_fltunion_big_to_host(duk_float_union *u);

/*
 *  Buffer writer (dynamic buffer only)
 *
 *  Helper for writing to a dynamic buffer with a concept of a "slack" area
 *  to reduce resizes.  You can ensure there is enough space beforehand and
 *  then write for a while without further checks, relying on a stable data
 *  pointer.  Slack handling is automatic so call sites only indicate how
 *  much data they need right now.
 *
 *  There are several ways to write using bufwriter.  The best approach
 *  depends mainly on how much performance matters over code footprint.
 *  The key issues are (1) ensuring there is space and (2) keeping the
 *  pointers consistent.  Fast code should ensure space for multiple writes
 *  with one ensure call.  Fastest inner loop code can temporarily borrow
 *  the 'p' pointer but must write it back eventually.
 *
 *  Be careful to ensure all macro arguments (other than static pointers like
 *  'thr' and 'bw_ctx') are evaluated exactly once, using temporaries if
 *  necessary (if that's not possible, there should be a note near the macro).
 *  Buffer write arguments often contain arithmetic etc so this is
 *  particularly important here.
 */

/* XXX: Migrate bufwriter and other read/write helpers to its own header? */

struct duk_bufwriter_ctx {
	duk_uint8_t *p;
	duk_uint8_t *p_base;
	duk_uint8_t *p_limit;
	duk_hbuffer_dynamic *buf;
};

#if defined(DUK_USE_PREFER_SIZE)
#define DUK_BW_SLACK_ADD   64
#define DUK_BW_SLACK_SHIFT 4 /* 2^4 -> 1/16 = 6.25% slack */
#else
#define DUK_BW_SLACK_ADD   64
#define DUK_BW_SLACK_SHIFT 2 /* 2^2 -> 1/4 = 25% slack */
#endif

/* Initialization and finalization (compaction), converting to other types. */

#define DUK_BW_INIT_PUSHBUF(thr, bw_ctx, sz) \
	do { \
		duk_bw_init_pushbuf((thr), (bw_ctx), (sz)); \
	} while (0)
#define DUK_BW_INIT_WITHBUF(thr, bw_ctx, buf) \
	do { \
		duk_bw_init((thr), (bw_ctx), (buf)); \
	} while (0)
#define DUK_BW_COMPACT(thr, bw_ctx) \
	do { \
		/* Make underlying buffer compact to match DUK_BW_GET_SIZE(). */ \
		duk_bw_compact((thr), (bw_ctx)); \
	} while (0)
#define DUK_BW_PUSH_AS_STRING(thr, bw_ctx) \
	do { \
		duk_push_lstring((thr), (const char *) (bw_ctx)->p_base, (duk_size_t) ((bw_ctx)->p - (bw_ctx)->p_base)); \
	} while (0)

/* Pointers may be NULL for a while when 'buf' size is zero and before any
 * ENSURE calls have been made.  Once an ENSURE has been made, the pointers
 * are required to be non-NULL so that it's always valid to use memcpy() and
 * memmove(), even for zero size.
 */
#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL_DECL void duk_bw_assert_valid(duk_hthread *thr, duk_bufwriter_ctx *bw_ctx);
#define DUK_BW_ASSERT_VALID_EXPR(thr, bw_ctx) (duk_bw_assert_valid((thr), (bw_ctx)))
#define DUK_BW_ASSERT_VALID(thr, bw_ctx) \
	do { \
		duk_bw_assert_valid((thr), (bw_ctx)); \
	} while (0)
#else
#define DUK_BW_ASSERT_VALID_EXPR(thr, bw_ctx) DUK_ASSERT_EXPR(1)
#define DUK_BW_ASSERT_VALID(thr, bw_ctx) \
	do { \
	} while (0)
#endif

/* Working with the pointer and current size. */

#define DUK_BW_GET_PTR(thr, bw_ctx) ((bw_ctx)->p)
#define DUK_BW_SET_PTR(thr, bw_ctx, ptr) \
	do { \
		(bw_ctx)->p = (ptr); \
	} while (0)
#define DUK_BW_ADD_PTR(thr, bw_ctx, delta) \
	do { \
		(bw_ctx)->p += (delta); \
	} while (0)
#define DUK_BW_GET_BASEPTR(thr, bw_ctx)  ((bw_ctx)->p_base)
#define DUK_BW_GET_LIMITPTR(thr, bw_ctx) ((bw_ctx)->p_limit)
#define DUK_BW_GET_SIZE(thr, bw_ctx)     ((duk_size_t) ((bw_ctx)->p - (bw_ctx)->p_base))
#define DUK_BW_SET_SIZE(thr, bw_ctx, sz) \
	do { \
		DUK_ASSERT((duk_size_t) (sz) <= (duk_size_t) ((bw_ctx)->p - (bw_ctx)->p_base)); \
		(bw_ctx)->p = (bw_ctx)->p_base + (sz); \
	} while (0)
#define DUK_BW_RESET_SIZE(thr, bw_ctx) \
	do { \
		/* Reset to zero size, keep current limit. */ \
		(bw_ctx)->p = (bw_ctx)->p_base; \
	} while (0)
#define DUK_BW_GET_BUFFER(thr, bw_ctx) ((bw_ctx)->buf)

/* Ensuring (reserving) space. */

#define DUK_BW_ENSURE(thr, bw_ctx, sz) \
	do { \
		duk_size_t duk__sz, duk__space; \
		DUK_BW_ASSERT_VALID((thr), (bw_ctx)); \
		duk__sz = (sz); \
		duk__space = (duk_size_t) ((bw_ctx)->p_limit - (bw_ctx)->p); \
		if (duk__space < duk__sz) { \
			(void) duk_bw_resize((thr), (bw_ctx), duk__sz); \
		} \
	} while (0)
/* NOTE: Multiple evaluation of 'ptr' in this macro. */
/* XXX: Rework to use an always-inline function? */
#define DUK_BW_ENSURE_RAW(thr, bw_ctx, sz, ptr) \
	(((duk_size_t) ((bw_ctx)->p_limit - (ptr)) >= (sz)) ? (ptr) : ((bw_ctx)->p = (ptr), duk_bw_resize((thr), (bw_ctx), (sz))))
#define DUK_BW_ENSURE_GETPTR(thr, bw_ctx, sz) DUK_BW_ENSURE_RAW((thr), (bw_ctx), (sz), (bw_ctx)->p)
#define DUK_BW_ASSERT_SPACE_EXPR(thr, bw_ctx, sz) \
	(DUK_BW_ASSERT_VALID_EXPR((thr), (bw_ctx)), \
	 DUK_ASSERT_EXPR((duk_size_t) ((bw_ctx)->p_limit - (bw_ctx)->p) >= (duk_size_t) (sz)))
#define DUK_BW_ASSERT_SPACE(thr, bw_ctx, sz) \
	do { \
		DUK_BW_ASSERT_SPACE_EXPR((thr), (bw_ctx), (sz)); \
	} while (0)

/* Miscellaneous. */

#define DUK_BW_SETPTR_AND_COMPACT(thr, bw_ctx, ptr) \
	do { \
		(bw_ctx)->p = (ptr); \
		duk_bw_compact((thr), (bw_ctx)); \
	} while (0)

/* Fast write calls which assume you control the slack beforehand.
 * Multibyte write variants exist and use a temporary write pointer
 * because byte writes alias with anything: with a stored pointer
 * explicit pointer load/stores get generated (e.g. gcc -Os).
 */

#define DUK_BW_WRITE_RAW_U8(thr, bw_ctx, val) \
	do { \
		DUK_BW_ASSERT_SPACE((thr), (bw_ctx), 1); \
		*(bw_ctx)->p++ = (duk_uint8_t) (val); \
	} while (0)
#define DUK_BW_WRITE_RAW_U8_2(thr, bw_ctx, val1, val2) \
	do { \
		duk_uint8_t *duk__p; \
		DUK_BW_ASSERT_SPACE((thr), (bw_ctx), 2); \
		duk__p = (bw_ctx)->p; \
		*duk__p++ = (duk_uint8_t) (val1); \
		*duk__p++ = (duk_uint8_t) (val2); \
		(bw_ctx)->p = duk__p; \
	} while (0)
#define DUK_BW_WRITE_RAW_U8_3(thr, bw_ctx, val1, val2, val3) \
	do { \
		duk_uint8_t *duk__p; \
		DUK_BW_ASSERT_SPACE((thr), (bw_ctx), 3); \
		duk__p = (bw_ctx)->p; \
		*duk__p++ = (duk_uint8_t) (val1); \
		*duk__p++ = (duk_uint8_t) (val2); \
		*duk__p++ = (duk_uint8_t) (val3); \
		(bw_ctx)->p = duk__p; \
	} while (0)
#define DUK_BW_WRITE_RAW_U8_4(thr, bw_ctx, val1, val2, val3, val4) \
	do { \
		duk_uint8_t *duk__p; \
		DUK_BW_ASSERT_SPACE((thr), (bw_ctx), 4); \
		duk__p = (bw_ctx)->p; \
		*duk__p++ = (duk_uint8_t) (val1); \
		*duk__p++ = (duk_uint8_t) (val2); \
		*duk__p++ = (duk_uint8_t) (val3); \
		*duk__p++ = (duk_uint8_t) (val4); \
		(bw_ctx)->p = duk__p; \
	} while (0)
#define DUK_BW_WRITE_RAW_U8_5(thr, bw_ctx, val1, val2, val3, val4, val5) \
	do { \
		duk_uint8_t *duk__p; \
		DUK_BW_ASSERT_SPACE((thr), (bw_ctx), 5); \
		duk__p = (bw_ctx)->p; \
		*duk__p++ = (duk_uint8_t) (val1); \
		*duk__p++ = (duk_uint8_t) (val2); \
		*duk__p++ = (duk_uint8_t) (val3); \
		*duk__p++ = (duk_uint8_t) (val4); \
		*duk__p++ = (duk_uint8_t) (val5); \
		(bw_ctx)->p = duk__p; \
	} while (0)
#define DUK_BW_WRITE_RAW_U8_6(thr, bw_ctx, val1, val2, val3, val4, val5, val6) \
	do { \
		duk_uint8_t *duk__p; \
		DUK_BW_ASSERT_SPACE((thr), (bw_ctx), 6); \
		duk__p = (bw_ctx)->p; \
		*duk__p++ = (duk_uint8_t) (val1); \
		*duk__p++ = (duk_uint8_t) (val2); \
		*duk__p++ = (duk_uint8_t) (val3); \
		*duk__p++ = (duk_uint8_t) (val4); \
		*duk__p++ = (duk_uint8_t) (val5); \
		*duk__p++ = (duk_uint8_t) (val6); \
		(bw_ctx)->p = duk__p; \
	} while (0)
#define DUK_BW_WRITE_RAW_XUTF8(thr, bw_ctx, cp) \
	do { \
		duk_ucodepoint_t duk__cp; \
		duk_small_int_t duk__enc_len; \
		duk__cp = (duk_ucodepoint_t) (cp); \
		DUK_BW_ASSERT_SPACE((thr), (bw_ctx), duk_unicode_get_xutf8_length(duk__cp)); \
		duk__enc_len = duk_unicode_encode_xutf8(duk__cp, (bw_ctx)->p); \
		(bw_ctx)->p += duk__enc_len; \
	} while (0)
#define DUK_BW_WRITE_RAW_CESU8(thr, bw_ctx, cp) \
	do { \
		duk_ucodepoint_t duk__cp; \
		duk_small_int_t duk__enc_len; \
		duk__cp = (duk_ucodepoint_t) (cp); \
		DUK_BW_ASSERT_SPACE((thr), (bw_ctx), duk_unicode_get_cesu8_length(duk__cp)); \
		duk__enc_len = duk_unicode_encode_cesu8(duk__cp, (bw_ctx)->p); \
		(bw_ctx)->p += duk__enc_len; \
	} while (0)
/* XXX: add temporary duk__p pointer here too; sharing */
/* XXX: avoid unsafe variants */
#define DUK_BW_WRITE_RAW_BYTES(thr, bw_ctx, valptr, valsz) \
	do { \
		const void *duk__valptr; \
		duk_size_t duk__valsz; \
		duk__valptr = (const void *) (valptr); \
		duk__valsz = (duk_size_t) (valsz); \
		duk_memcpy_unsafe((void *) ((bw_ctx)->p), duk__valptr, duk__valsz); \
		(bw_ctx)->p += duk__valsz; \
	} while (0)
#define DUK_BW_WRITE_RAW_CSTRING(thr, bw_ctx, val) \
	do { \
		const duk_uint8_t *duk__val; \
		duk_size_t duk__val_len; \
		duk__val = (const duk_uint8_t *) (val); \
		duk__val_len = DUK_STRLEN((const char *) duk__val); \
		duk_memcpy_unsafe((void *) ((bw_ctx)->p), (const void *) duk__val, duk__val_len); \
		(bw_ctx)->p += duk__val_len; \
	} while (0)
#define DUK_BW_WRITE_RAW_HSTRING(thr, bw_ctx, val) \
	do { \
		duk_size_t duk__val_len; \
		duk__val_len = DUK_HSTRING_GET_BYTELEN((val)); \
		duk_memcpy_unsafe((void *) ((bw_ctx)->p), (const void *) DUK_HSTRING_GET_DATA((val)), duk__val_len); \
		(bw_ctx)->p += duk__val_len; \
	} while (0)
#define DUK_BW_WRITE_RAW_HBUFFER(thr, bw_ctx, val) \
	do { \
		duk_size_t duk__val_len; \
		duk__val_len = DUK_HBUFFER_GET_SIZE((val)); \
		duk_memcpy_unsafe((void *) ((bw_ctx)->p), \
		                  (const void *) DUK_HBUFFER_GET_DATA_PTR((thr)->heap, (val)), \
		                  duk__val_len); \
		(bw_ctx)->p += duk__val_len; \
	} while (0)
#define DUK_BW_WRITE_RAW_HBUFFER_FIXED(thr, bw_ctx, val) \
	do { \
		duk_size_t duk__val_len; \
		duk__val_len = DUK_HBUFFER_FIXED_GET_SIZE((val)); \
		duk_memcpy_unsafe((void *) ((bw_ctx)->p), \
		                  (const void *) DUK_HBUFFER_FIXED_GET_DATA_PTR((thr)->heap, (val)), \
		                  duk__val_len); \
		(bw_ctx)->p += duk__val_len; \
	} while (0)
#define DUK_BW_WRITE_RAW_HBUFFER_DYNAMIC(thr, bw_ctx, val) \
	do { \
		duk_size_t duk__val_len; \
		duk__val_len = DUK_HBUFFER_DYNAMIC_GET_SIZE((val)); \
		duk_memcpy_unsafe((void *) ((bw_ctx)->p), \
		                  (const void *) DUK_HBUFFER_DYNAMIC_GET_DATA_PTR((thr)->heap, (val)), \
		                  duk__val_len); \
		(bw_ctx)->p += duk__val_len; \
	} while (0)

/* Append bytes from a slice already in the buffer. */
#define DUK_BW_WRITE_RAW_SLICE(thr, bw, dst_off, dst_len) duk_bw_write_raw_slice((thr), (bw), (dst_off), (dst_len))

/* Insert bytes in the middle of the buffer from an external buffer. */
#define DUK_BW_INSERT_RAW_BYTES(thr, bw, dst_off, buf, len) duk_bw_insert_raw_bytes((thr), (bw), (dst_off), (buf), (len))

/* Insert bytes in the middle of the buffer from a slice already
 * in the buffer.  Source offset is interpreted "before" the operation.
 */
#define DUK_BW_INSERT_RAW_SLICE(thr, bw, dst_off, src_off, len) duk_bw_insert_raw_slice((thr), (bw), (dst_off), (src_off), (len))

/* Insert a reserved area somewhere in the buffer; caller fills it.
 * Evaluates to a (duk_uint_t *) pointing to the start of the reserved
 * area for convenience.
 */
#define DUK_BW_INSERT_RAW_AREA(thr, bw, off, len) duk_bw_insert_raw_area((thr), (bw), (off), (len))

/* Remove a slice from inside buffer. */
#define DUK_BW_REMOVE_RAW_SLICE(thr, bw, off, len) duk_bw_remove_raw_slice((thr), (bw), (off), (len))

/* Safe write calls which will ensure space first. */

#define DUK_BW_WRITE_ENSURE_U8(thr, bw_ctx, val) \
	do { \
		DUK_BW_ENSURE((thr), (bw_ctx), 1); \
		DUK_BW_WRITE_RAW_U8((thr), (bw_ctx), (val)); \
	} while (0)
#define DUK_BW_WRITE_ENSURE_U8_2(thr, bw_ctx, val1, val2) \
	do { \
		DUK_BW_ENSURE((thr), (bw_ctx), 2); \
		DUK_BW_WRITE_RAW_U8_2((thr), (bw_ctx), (val1), (val2)); \
	} while (0)
#define DUK_BW_WRITE_ENSURE_U8_3(thr, bw_ctx, val1, val2, val3) \
	do { \
		DUK_BW_ENSURE((thr), (bw_ctx), 3); \
		DUK_BW_WRITE_RAW_U8_3((thr), (bw_ctx), (val1), (val2), (val3)); \
	} while (0)
#define DUK_BW_WRITE_ENSURE_U8_4(thr, bw_ctx, val1, val2, val3, val4) \
	do { \
		DUK_BW_ENSURE((thr), (bw_ctx), 4); \
		DUK_BW_WRITE_RAW_U8_4((thr), (bw_ctx), (val1), (val2), (val3), (val4)); \
	} while (0)
#define DUK_BW_WRITE_ENSURE_U8_5(thr, bw_ctx, val1, val2, val3, val4, val5) \
	do { \
		DUK_BW_ENSURE((thr), (bw_ctx), 5); \
		DUK_BW_WRITE_RAW_U8_5((thr), (bw_ctx), (val1), (val2), (val3), (val4), (val5)); \
	} while (0)
#define DUK_BW_WRITE_ENSURE_U8_6(thr, bw_ctx, val1, val2, val3, val4, val5, val6) \
	do { \
		DUK_BW_ENSURE((thr), (bw_ctx), 6); \
		DUK_BW_WRITE_RAW_U8_6((thr), (bw_ctx), (val1), (val2), (val3), (val4), (val5), (val6)); \
	} while (0)
#define DUK_BW_WRITE_ENSURE_XUTF8(thr, bw_ctx, cp) \
	do { \
		DUK_BW_ENSURE((thr), (bw_ctx), DUK_UNICODE_MAX_XUTF8_LENGTH); \
		DUK_BW_WRITE_RAW_XUTF8((thr), (bw_ctx), (cp)); \
	} while (0)
#define DUK_BW_WRITE_ENSURE_CESU8(thr, bw_ctx, cp) \
	do { \
		DUK_BW_ENSURE((thr), (bw_ctx), DUK_UNICODE_MAX_CESU8_LENGTH); \
		DUK_BW_WRITE_RAW_CESU8((thr), (bw_ctx), (cp)); \
	} while (0)
/* XXX: add temporary duk__p pointer here too; sharing */
/* XXX: avoid unsafe */
#define DUK_BW_WRITE_ENSURE_BYTES(thr, bw_ctx, valptr, valsz) \
	do { \
		const void *duk__valptr; \
		duk_size_t duk__valsz; \
		duk__valptr = (const void *) (valptr); \
		duk__valsz = (duk_size_t) (valsz); \
		DUK_BW_ENSURE((thr), (bw_ctx), duk__valsz); \
		duk_memcpy_unsafe((void *) ((bw_ctx)->p), duk__valptr, duk__valsz); \
		(bw_ctx)->p += duk__valsz; \
	} while (0)
#define DUK_BW_WRITE_ENSURE_CSTRING(thr, bw_ctx, val) \
	do { \
		const duk_uint8_t *duk__val; \
		duk_size_t duk__val_len; \
		duk__val = (const duk_uint8_t *) (val); \
		duk__val_len = DUK_STRLEN((const char *) duk__val); \
		DUK_BW_ENSURE((thr), (bw_ctx), duk__val_len); \
		duk_memcpy_unsafe((void *) ((bw_ctx)->p), (const void *) duk__val, duk__val_len); \
		(bw_ctx)->p += duk__val_len; \
	} while (0)
#define DUK_BW_WRITE_ENSURE_HSTRING(thr, bw_ctx, val) \
	do { \
		duk_size_t duk__val_len; \
		duk__val_len = DUK_HSTRING_GET_BYTELEN((val)); \
		DUK_BW_ENSURE((thr), (bw_ctx), duk__val_len); \
		duk_memcpy_unsafe((void *) ((bw_ctx)->p), (const void *) DUK_HSTRING_GET_DATA((val)), duk__val_len); \
		(bw_ctx)->p += duk__val_len; \
	} while (0)
#define DUK_BW_WRITE_ENSURE_HBUFFER(thr, bw_ctx, val) \
	do { \
		duk_size_t duk__val_len; \
		duk__val_len = DUK_HBUFFER_GET_SIZE((val)); \
		DUK_BW_ENSURE((thr), (bw_ctx), duk__val_len); \
		duk_memcpy_unsafe((void *) ((bw_ctx)->p), \
		                  (const void *) DUK_HBUFFER_GET_DATA_PTR((thr)->heap, (val)), \
		                  duk__val_len); \
		(bw_ctx)->p += duk__val_len; \
	} while (0)
#define DUK_BW_WRITE_ENSURE_HBUFFER_FIXED(thr, bw_ctx, val) \
	do { \
		duk_size_t duk__val_len; \
		duk__val_len = DUK_HBUFFER_FIXED_GET_SIZE((val)); \
		DUK_BW_ENSURE((thr), (bw_ctx), duk__val_len); \
		duk_memcpy_unsafe((void *) ((bw_ctx)->p), \
		                  (const void *) DUK_HBUFFER_FIXED_GET_DATA_PTR((thr)->heap, (val)), \
		                  duk__val_len); \
		(bw_ctx)->p += duk__val_len; \
	} while (0)
#define DUK_BW_WRITE_ENSURE_HBUFFER_DYNAMIC(thr, bw_ctx, val) \
	do { \
		duk_size_t duk__val_len; \
		duk__val_len = DUK_HBUFFER_DYNAMIC_GET_SIZE((val)); \
		DUK_BW_ENSURE((thr), (bw_ctx), duk__val_len); \
		duk_memcpy_unsafe((void *) ((bw_ctx)->p), \
		                  (const void *) DUK_HBUFFER_DYNAMIC_GET_DATA_PTR((thr)->heap, (val)), \
		                  duk__val_len); \
		(bw_ctx)->p += duk__val_len; \
	} while (0)

#define DUK_BW_WRITE_ENSURE_SLICE(thr, bw, dst_off, dst_len)   duk_bw_write_ensure_slice((thr), (bw), (dst_off), (dst_len))
#define DUK_BW_INSERT_ENSURE_BYTES(thr, bw, dst_off, buf, len) duk_bw_insert_ensure_bytes((thr), (bw), (dst_off), (buf), (len))
#define DUK_BW_INSERT_ENSURE_SLICE(thr, bw, dst_off, src_off, len) \
	duk_bw_insert_ensure_slice((thr), (bw), (dst_off), (src_off), (len))
#define DUK_BW_INSERT_ENSURE_AREA(thr, bw, off, len) \
	/* Evaluates to (duk_uint8_t *) pointing to start of area. */ \
	duk_bw_insert_ensure_area((thr), (bw), (off), (len))
#define DUK_BW_REMOVE_ENSURE_SLICE(thr, bw, off, len) \
	/* No difference between raw/ensure because the buffer shrinks. */ \
	DUK_BW_REMOVE_RAW_SLICE((thr), (bw), (off), (len))

/*
 *  Externs and prototypes
 */

#if !defined(DUK_SINGLE_FILE)
DUK_INTERNAL_DECL const duk_uint8_t duk_lc_digits[36];
DUK_INTERNAL_DECL const duk_uint8_t duk_uc_nybbles[16];
DUK_INTERNAL_DECL const duk_int8_t duk_hex_dectab[256];
#if defined(DUK_USE_HEX_FASTPATH)
DUK_INTERNAL_DECL const duk_int16_t duk_hex_dectab_shift4[256];
DUK_INTERNAL_DECL const duk_uint16_t duk_hex_enctab[256];
#endif
#endif /* !DUK_SINGLE_FILE */

/* Note: assumes that duk_util_probe_steps size is 32 */
#if defined(DUK_USE_HOBJECT_HASH_PART)
#if !defined(DUK_SINGLE_FILE)
DUK_INTERNAL_DECL duk_uint8_t duk_util_probe_steps[32];
#endif /* !DUK_SINGLE_FILE */
#endif

#if defined(DUK_USE_STRHASH_DENSE)
DUK_INTERNAL_DECL duk_uint32_t duk_util_hashbytes(const duk_uint8_t *data, duk_size_t len, duk_uint32_t seed);
#endif

DUK_INTERNAL_DECL duk_uint32_t duk_bd_decode(duk_bitdecoder_ctx *ctx, duk_small_int_t bits);
DUK_INTERNAL_DECL duk_small_uint_t duk_bd_decode_flag(duk_bitdecoder_ctx *ctx);
DUK_INTERNAL_DECL duk_uint32_t duk_bd_decode_flagged(duk_bitdecoder_ctx *ctx, duk_small_int_t bits, duk_uint32_t def_value);
DUK_INTERNAL_DECL duk_int32_t duk_bd_decode_flagged_signed(duk_bitdecoder_ctx *ctx, duk_small_int_t bits, duk_int32_t def_value);
DUK_INTERNAL_DECL duk_uint32_t duk_bd_decode_varuint(duk_bitdecoder_ctx *ctx);
DUK_INTERNAL_DECL duk_small_uint_t duk_bd_decode_bitpacked_string(duk_bitdecoder_ctx *bd, duk_uint8_t *out);

DUK_INTERNAL_DECL void duk_be_encode(duk_bitencoder_ctx *ctx, duk_uint32_t data, duk_small_int_t bits);
DUK_INTERNAL_DECL void duk_be_finish(duk_bitencoder_ctx *ctx);

#if !defined(DUK_USE_GET_RANDOM_DOUBLE)
DUK_INTERNAL_DECL duk_double_t duk_util_tinyrandom_get_double(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_util_tinyrandom_prepare_seed(duk_hthread *thr);
#endif

DUK_INTERNAL_DECL void duk_bw_init(duk_hthread *thr, duk_bufwriter_ctx *bw_ctx, duk_hbuffer_dynamic *h_buf);
DUK_INTERNAL_DECL void duk_bw_init_pushbuf(duk_hthread *thr, duk_bufwriter_ctx *bw_ctx, duk_size_t buf_size);
DUK_INTERNAL_DECL duk_uint8_t *duk_bw_resize(duk_hthread *thr, duk_bufwriter_ctx *bw_ctx, duk_size_t sz);
DUK_INTERNAL_DECL void duk_bw_compact(duk_hthread *thr, duk_bufwriter_ctx *bw_ctx);
DUK_INTERNAL_DECL void duk_bw_write_raw_slice(duk_hthread *thr, duk_bufwriter_ctx *bw, duk_size_t src_off, duk_size_t len);
DUK_INTERNAL_DECL void duk_bw_write_ensure_slice(duk_hthread *thr, duk_bufwriter_ctx *bw, duk_size_t src_off, duk_size_t len);
DUK_INTERNAL_DECL void duk_bw_insert_raw_bytes(duk_hthread *thr,
                                               duk_bufwriter_ctx *bw,
                                               duk_size_t dst_off,
                                               const duk_uint8_t *buf,
                                               duk_size_t len);
DUK_INTERNAL_DECL void duk_bw_insert_ensure_bytes(duk_hthread *thr,
                                                  duk_bufwriter_ctx *bw,
                                                  duk_size_t dst_off,
                                                  const duk_uint8_t *buf,
                                                  duk_size_t len);
DUK_INTERNAL_DECL void duk_bw_insert_raw_slice(duk_hthread *thr,
                                               duk_bufwriter_ctx *bw,
                                               duk_size_t dst_off,
                                               duk_size_t src_off,
                                               duk_size_t len);
DUK_INTERNAL_DECL void duk_bw_insert_ensure_slice(duk_hthread *thr,
                                                  duk_bufwriter_ctx *bw,
                                                  duk_size_t dst_off,
                                                  duk_size_t src_off,
                                                  duk_size_t len);
DUK_INTERNAL_DECL duk_uint8_t *duk_bw_insert_raw_area(duk_hthread *thr, duk_bufwriter_ctx *bw, duk_size_t off, duk_size_t len);
DUK_INTERNAL_DECL duk_uint8_t *duk_bw_insert_ensure_area(duk_hthread *thr, duk_bufwriter_ctx *bw, duk_size_t off, duk_size_t len);
DUK_INTERNAL_DECL void duk_bw_remove_raw_slice(duk_hthread *thr, duk_bufwriter_ctx *bw, duk_size_t off, duk_size_t len);
/* No duk_bw_remove_ensure_slice(), functionality would be identical. */

DUK_INTERNAL_DECL duk_uint16_t duk_raw_read_u16_be(const duk_uint8_t *p);
DUK_INTERNAL_DECL duk_uint32_t duk_raw_read_u32_be(const duk_uint8_t *p);
DUK_INTERNAL_DECL duk_float_t duk_raw_read_float_be(const duk_uint8_t *p);
DUK_INTERNAL_DECL duk_double_t duk_raw_read_double_be(const duk_uint8_t *p);
DUK_INTERNAL_DECL duk_uint16_t duk_raw_readinc_u16_be(const duk_uint8_t **p);
DUK_INTERNAL_DECL duk_uint32_t duk_raw_readinc_u32_be(const duk_uint8_t **p);
DUK_INTERNAL_DECL duk_float_t duk_raw_readinc_float_be(const duk_uint8_t **p);
DUK_INTERNAL_DECL duk_double_t duk_raw_readinc_double_be(const duk_uint8_t **p);
DUK_INTERNAL_DECL void duk_raw_write_u16_be(duk_uint8_t *p, duk_uint16_t val);
DUK_INTERNAL_DECL void duk_raw_write_u32_be(duk_uint8_t *p, duk_uint32_t val);
DUK_INTERNAL_DECL void duk_raw_write_float_be(duk_uint8_t *p, duk_float_t val);
DUK_INTERNAL_DECL void duk_raw_write_double_be(duk_uint8_t *p, duk_double_t val);
DUK_INTERNAL_DECL duk_small_int_t duk_raw_write_xutf8(duk_uint8_t *p, duk_ucodepoint_t val);
DUK_INTERNAL_DECL duk_small_int_t duk_raw_write_cesu8(duk_uint8_t *p, duk_ucodepoint_t val);
DUK_INTERNAL_DECL void duk_raw_writeinc_u16_be(duk_uint8_t **p, duk_uint16_t val);
DUK_INTERNAL_DECL void duk_raw_writeinc_u32_be(duk_uint8_t **p, duk_uint32_t val);
DUK_INTERNAL_DECL void duk_raw_writeinc_float_be(duk_uint8_t **p, duk_float_t val);
DUK_INTERNAL_DECL void duk_raw_writeinc_double_be(duk_uint8_t **p, duk_double_t val);
DUK_INTERNAL_DECL void duk_raw_writeinc_xutf8(duk_uint8_t **p, duk_ucodepoint_t val);
DUK_INTERNAL_DECL void duk_raw_writeinc_cesu8(duk_uint8_t **p, duk_ucodepoint_t val);

#if defined(DUK_USE_DEBUGGER_SUPPORT) /* For now only needed by the debugger. */
DUK_INTERNAL_DECL void duk_byteswap_bytes(duk_uint8_t *p, duk_small_uint_t len);
#endif

DUK_INTERNAL_DECL duk_double_t duk_util_get_random_double(duk_hthread *thr);

/* memcpy(), memmove() etc wrappers.  The plain variants like duk_memcpy()
 * assume C99+ and 'src' and 'dst' pointers must be non-NULL even when the
 * operation size is zero.  The unsafe variants like duk_memcpy_safe() deal
 * with the zero size case explicitly, and allow NULL pointers in that case
 * (which is undefined behavior in C99+).  For the majority of actual targets
 * a NULL pointer with a zero length is fine in practice.  These wrappers are
 * macros to force inlining; because there are hundreds of call sites, even a
 * few extra bytes per call site adds up to ~1kB footprint.
 */
#if defined(DUK_USE_ALLOW_UNDEFINED_BEHAVIOR)
#define duk_memcpy(dst, src, len) \
	do { \
		void *duk__dst = (dst); \
		const void *duk__src = (src); \
		duk_size_t duk__len = (len); \
		DUK_ASSERT(duk__dst != NULL || duk__len == 0U); \
		DUK_ASSERT(duk__src != NULL || duk__len == 0U); \
		(void) DUK_MEMCPY(duk__dst, duk__src, (size_t) duk__len); \
	} while (0)
#define duk_memcpy_unsafe(dst, src, len) duk_memcpy((dst), (src), (len))
#define duk_memmove(dst, src, len) \
	do { \
		void *duk__dst = (dst); \
		const void *duk__src = (src); \
		duk_size_t duk__len = (len); \
		DUK_ASSERT(duk__dst != NULL || duk__len == 0U); \
		DUK_ASSERT(duk__src != NULL || duk__len == 0U); \
		(void) DUK_MEMMOVE(duk__dst, duk__src, (size_t) duk__len); \
	} while (0)
#define duk_memmove_unsafe(dst, src, len) duk_memmove((dst), (src), (len))
#define duk_memset(dst, val, len) \
	do { \
		void *duk__dst = (dst); \
		duk_small_int_t duk__val = (val); \
		duk_size_t duk__len = (len); \
		DUK_ASSERT(duk__dst != NULL || duk__len == 0U); \
		(void) DUK_MEMSET(duk__dst, duk__val, (size_t) duk__len); \
	} while (0)
#define duk_memset_unsafe(dst, val, len) duk_memset((dst), (val), (len))
#define duk_memzero(dst, len) \
	do { \
		void *duk__dst = (dst); \
		duk_size_t duk__len = (len); \
		DUK_ASSERT(duk__dst != NULL || duk__len == 0U); \
		(void) DUK_MEMZERO(duk__dst, (size_t) duk__len); \
	} while (0)
#define duk_memzero_unsafe(dst, len) duk_memzero((dst), (len))
#else /* DUK_USE_ALLOW_UNDEFINED_BEHAVIOR */
#define duk_memcpy(dst, src, len) \
	do { \
		void *duk__dst = (dst); \
		const void *duk__src = (src); \
		duk_size_t duk__len = (len); \
		DUK_ASSERT(duk__dst != NULL); \
		DUK_ASSERT(duk__src != NULL); \
		(void) DUK_MEMCPY(duk__dst, duk__src, (size_t) duk__len); \
	} while (0)
#define duk_memcpy_unsafe(dst, src, len) \
	do { \
		void *duk__dst = (dst); \
		const void *duk__src = (src); \
		duk_size_t duk__len = (len); \
		DUK_ASSERT(duk__dst != NULL || duk__len == 0U); \
		DUK_ASSERT(duk__src != NULL || duk__len == 0U); \
		if (DUK_LIKELY(duk__len > 0U)) { \
			DUK_ASSERT(duk__dst != NULL); \
			DUK_ASSERT(duk__src != NULL); \
			(void) DUK_MEMCPY(duk__dst, duk__src, (size_t) duk__len); \
		} \
	} while (0)
#define duk_memmove(dst, src, len) \
	do { \
		void *duk__dst = (dst); \
		const void *duk__src = (src); \
		duk_size_t duk__len = (len); \
		DUK_ASSERT(duk__dst != NULL); \
		DUK_ASSERT(duk__src != NULL); \
		(void) DUK_MEMMOVE(duk__dst, duk__src, (size_t) duk__len); \
	} while (0)
#define duk_memmove_unsafe(dst, src, len) \
	do { \
		void *duk__dst = (dst); \
		const void *duk__src = (src); \
		duk_size_t duk__len = (len); \
		DUK_ASSERT(duk__dst != NULL || duk__len == 0U); \
		DUK_ASSERT(duk__src != NULL || duk__len == 0U); \
		if (DUK_LIKELY(duk__len > 0U)) { \
			DUK_ASSERT(duk__dst != NULL); \
			DUK_ASSERT(duk__src != NULL); \
			(void) DUK_MEMMOVE(duk__dst, duk__src, (size_t) duk__len); \
		} \
	} while (0)
#define duk_memset(dst, val, len) \
	do { \
		void *duk__dst = (dst); \
		duk_small_int_t duk__val = (val); \
		duk_size_t duk__len = (len); \
		DUK_ASSERT(duk__dst != NULL); \
		(void) DUK_MEMSET(duk__dst, duk__val, (size_t) duk__len); \
	} while (0)
#define duk_memset_unsafe(dst, val, len) \
	do { \
		void *duk__dst = (dst); \
		duk_small_int_t duk__val = (val); \
		duk_size_t duk__len = (len); \
		DUK_ASSERT(duk__dst != NULL || duk__len == 0U); \
		if (DUK_LIKELY(duk__len > 0U)) { \
			DUK_ASSERT(duk__dst != NULL); \
			(void) DUK_MEMSET(duk__dst, duk__val, (size_t) duk__len); \
		} \
	} while (0)
#define duk_memzero(dst, len) \
	do { \
		void *duk__dst = (dst); \
		duk_size_t duk__len = (len); \
		DUK_ASSERT(duk__dst != NULL); \
		(void) DUK_MEMZERO(duk__dst, (size_t) duk__len); \
	} while (0)
#define duk_memzero_unsafe(dst, len) \
	do { \
		void *duk__dst = (dst); \
		duk_size_t duk__len = (len); \
		DUK_ASSERT(duk__dst != NULL || duk__len == 0U); \
		if (DUK_LIKELY(duk__len > 0U)) { \
			DUK_ASSERT(duk__dst != NULL); \
			(void) DUK_MEMZERO(duk__dst, (size_t) duk__len); \
		} \
	} while (0)
#endif /* DUK_USE_ALLOW_UNDEFINED_BEHAVIOR */

DUK_INTERNAL_DECL duk_small_int_t duk_memcmp(const void *s1, const void *s2, duk_size_t len);
DUK_INTERNAL_DECL duk_small_int_t duk_memcmp_unsafe(const void *s1, const void *s2, duk_size_t len);

DUK_INTERNAL_DECL duk_bool_t duk_is_whole_get_int32_nonegzero(duk_double_t x, duk_int32_t *ival);
DUK_INTERNAL_DECL duk_bool_t duk_is_whole_get_int32(duk_double_t x, duk_int32_t *ival);
DUK_INTERNAL_DECL duk_bool_t duk_double_is_anyinf(duk_double_t x);
DUK_INTERNAL_DECL duk_bool_t duk_double_is_posinf(duk_double_t x);
DUK_INTERNAL_DECL duk_bool_t duk_double_is_neginf(duk_double_t x);
DUK_INTERNAL_DECL duk_bool_t duk_double_is_nan(duk_double_t x);
DUK_INTERNAL_DECL duk_bool_t duk_double_is_nan_or_zero(duk_double_t x);
DUK_INTERNAL_DECL duk_bool_t duk_double_is_nan_or_inf(duk_double_t x);
DUK_INTERNAL_DECL duk_bool_t duk_double_is_nan_zero_inf(duk_double_t x);
DUK_INTERNAL_DECL duk_small_uint_t duk_double_signbit(duk_double_t x);
DUK_INTERNAL_DECL duk_double_t duk_double_trunc_towards_zero(duk_double_t x);
DUK_INTERNAL_DECL duk_bool_t duk_double_same_sign(duk_double_t x, duk_double_t y);
DUK_INTERNAL_DECL duk_double_t duk_double_fmin(duk_double_t x, duk_double_t y);
DUK_INTERNAL_DECL duk_double_t duk_double_fmax(duk_double_t x, duk_double_t y);
DUK_INTERNAL_DECL duk_bool_t duk_double_is_finite(duk_double_t x);
DUK_INTERNAL_DECL duk_bool_t duk_double_is_integer(duk_double_t x);
DUK_INTERNAL_DECL duk_bool_t duk_double_is_safe_integer(duk_double_t x);

DUK_INTERNAL_DECL duk_double_t duk_double_div(duk_double_t x, duk_double_t y);
DUK_INTERNAL_DECL duk_int_t duk_double_to_int_t(duk_double_t x);
DUK_INTERNAL_DECL duk_uint_t duk_double_to_uint_t(duk_double_t x);
DUK_INTERNAL_DECL duk_int32_t duk_double_to_int32_t(duk_double_t x);
DUK_INTERNAL_DECL duk_uint32_t duk_double_to_uint32_t(duk_double_t x);
DUK_INTERNAL_DECL duk_float_t duk_double_to_float_t(duk_double_t x);
DUK_INTERNAL_DECL duk_bool_t duk_double_equals(duk_double_t x, duk_double_t y);
DUK_INTERNAL_DECL duk_bool_t duk_float_equals(duk_float_t x, duk_float_t y);

/*
 *  Miscellaneous
 */

/* Example: x     = 0x10 = 0b00010000
 *          x - 1 = 0x0f = 0b00001111
 *          x & (x - 1) == 0
 *
 *          x     = 0x07 = 0b00000111
 *          x - 1 = 0x06 = 0b00000110
 *          x & (x - 1) != 0
 *
 * However, incorrectly true for x == 0 so check for that explicitly.
 */
#define DUK_IS_POWER_OF_TWO(x) ((x) != 0U && ((x) & ((x) -1U)) == 0U)

#endif /* DUK_UTIL_H_INCLUDED */
