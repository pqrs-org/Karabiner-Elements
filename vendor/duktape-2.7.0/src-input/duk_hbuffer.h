/*
 *  Heap buffer representation.
 *
 *  Heap allocated user data buffer which is either:
 *
 *    1. A fixed size buffer (data follows header statically)
 *    2. A dynamic size buffer (data pointer follows header)
 *
 *  The data pointer for a variable size buffer of zero size may be NULL.
 */

#if !defined(DUK_HBUFFER_H_INCLUDED)
#define DUK_HBUFFER_H_INCLUDED

/*
 *  Flags
 *
 *  Fixed buffer:     0
 *  Dynamic buffer:   DUK_HBUFFER_FLAG_DYNAMIC
 *  External buffer:  DUK_HBUFFER_FLAG_DYNAMIC | DUK_HBUFFER_FLAG_EXTERNAL
 */

#define DUK_HBUFFER_FLAG_DYNAMIC  DUK_HEAPHDR_USER_FLAG(0) /* buffer is behind a pointer, dynamic or external */
#define DUK_HBUFFER_FLAG_EXTERNAL DUK_HEAPHDR_USER_FLAG(1) /* buffer pointer is to an externally allocated buffer */

#define DUK_HBUFFER_HAS_DYNAMIC(x)  DUK_HEAPHDR_CHECK_FLAG_BITS(&(x)->hdr, DUK_HBUFFER_FLAG_DYNAMIC)
#define DUK_HBUFFER_HAS_EXTERNAL(x) DUK_HEAPHDR_CHECK_FLAG_BITS(&(x)->hdr, DUK_HBUFFER_FLAG_EXTERNAL)

#define DUK_HBUFFER_SET_DYNAMIC(x)  DUK_HEAPHDR_SET_FLAG_BITS(&(x)->hdr, DUK_HBUFFER_FLAG_DYNAMIC)
#define DUK_HBUFFER_SET_EXTERNAL(x) DUK_HEAPHDR_SET_FLAG_BITS(&(x)->hdr, DUK_HBUFFER_FLAG_EXTERNAL)

#define DUK_HBUFFER_CLEAR_DYNAMIC(x)  DUK_HEAPHDR_CLEAR_FLAG_BITS(&(x)->hdr, DUK_HBUFFER_FLAG_DYNAMIC)
#define DUK_HBUFFER_CLEAR_EXTERNAL(x) DUK_HEAPHDR_CLEAR_FLAG_BITS(&(x)->hdr, DUK_HBUFFER_FLAG_EXTERNAL)

/*
 *  Misc defines
 */

/* Impose a maximum buffer length for now.  Restricted artificially to
 * ensure resize computations or adding a heap header length won't
 * overflow size_t and that a signed duk_int_t can hold a buffer
 * length.  The limit should be synchronized with DUK_HSTRING_MAX_BYTELEN.
 */

#if defined(DUK_USE_BUFLEN16)
#define DUK_HBUFFER_MAX_BYTELEN (0x0000ffffUL)
#else
/* Intentionally not 0x7fffffffUL; at least JSON code expects that
 * 2*len + 2 fits in 32 bits.
 */
#define DUK_HBUFFER_MAX_BYTELEN (0x7ffffffeUL)
#endif

/*
 *  Field access
 */

#if defined(DUK_USE_BUFLEN16)
/* size stored in duk_heaphdr unused flag bits */
#define DUK_HBUFFER_GET_SIZE(x) ((x)->hdr.h_flags >> 16)
#define DUK_HBUFFER_SET_SIZE(x, v) \
	do { \
		duk_size_t duk__v; \
		duk__v = (v); \
		DUK_ASSERT(duk__v <= 0xffffUL); \
		(x)->hdr.h_flags = ((x)->hdr.h_flags & 0x0000ffffUL) | (((duk_uint32_t) duk__v) << 16); \
	} while (0)
#define DUK_HBUFFER_ADD_SIZE(x, dv) \
	do { \
		(x)->hdr.h_flags += ((dv) << 16); \
	} while (0)
#define DUK_HBUFFER_SUB_SIZE(x, dv) \
	do { \
		(x)->hdr.h_flags -= ((dv) << 16); \
	} while (0)
#else
#define DUK_HBUFFER_GET_SIZE(x) (((duk_hbuffer *) (x))->size)
#define DUK_HBUFFER_SET_SIZE(x, v) \
	do { \
		((duk_hbuffer *) (x))->size = (v); \
	} while (0)
#define DUK_HBUFFER_ADD_SIZE(x, dv) \
	do { \
		(x)->size += (dv); \
	} while (0)
#define DUK_HBUFFER_SUB_SIZE(x, dv) \
	do { \
		(x)->size -= (dv); \
	} while (0)
#endif

#define DUK_HBUFFER_FIXED_GET_SIZE(x)    DUK_HBUFFER_GET_SIZE((duk_hbuffer *) (x))
#define DUK_HBUFFER_FIXED_SET_SIZE(x, v) DUK_HBUFFER_SET_SIZE((duk_hbuffer *) (x))

#define DUK_HBUFFER_DYNAMIC_GET_SIZE(x)     DUK_HBUFFER_GET_SIZE((duk_hbuffer *) (x))
#define DUK_HBUFFER_DYNAMIC_SET_SIZE(x, v)  DUK_HBUFFER_SET_SIZE((duk_hbuffer *) (x), (v))
#define DUK_HBUFFER_DYNAMIC_ADD_SIZE(x, dv) DUK_HBUFFER_ADD_SIZE((duk_hbuffer *) (x), (dv))
#define DUK_HBUFFER_DYNAMIC_SUB_SIZE(x, dv) DUK_HBUFFER_SUB_SIZE((duk_hbuffer *) (x), (dv))

#define DUK_HBUFFER_EXTERNAL_GET_SIZE(x)    DUK_HBUFFER_GET_SIZE((duk_hbuffer *) (x))
#define DUK_HBUFFER_EXTERNAL_SET_SIZE(x, v) DUK_HBUFFER_SET_SIZE((duk_hbuffer *) (x), (v))

#define DUK_HBUFFER_FIXED_GET_DATA_PTR(heap, x) ((duk_uint8_t *) (((duk_hbuffer_fixed *) (void *) (x)) + 1))

#if defined(DUK_USE_HEAPPTR16)
#define DUK_HBUFFER_DYNAMIC_GET_DATA_PTR(heap, x) \
	((void *) DUK_USE_HEAPPTR_DEC16((heap)->heap_udata, ((duk_heaphdr *) (x))->h_extra16))
#define DUK_HBUFFER_DYNAMIC_SET_DATA_PTR(heap, x, v) \
	do { \
		((duk_heaphdr *) (x))->h_extra16 = DUK_USE_HEAPPTR_ENC16((heap)->heap_udata, (void *) (v)); \
	} while (0)
#define DUK_HBUFFER_DYNAMIC_SET_DATA_PTR_NULL(heap, x) \
	do { \
		((duk_heaphdr *) (x))->h_extra16 = 0; /* assume 0 <=> NULL */ \
	} while (0)
#else
#define DUK_HBUFFER_DYNAMIC_GET_DATA_PTR(heap, x) ((x)->curr_alloc)
#define DUK_HBUFFER_DYNAMIC_SET_DATA_PTR(heap, x, v) \
	do { \
		(x)->curr_alloc = (void *) (v); \
	} while (0)
#define DUK_HBUFFER_DYNAMIC_SET_DATA_PTR_NULL(heap, x) \
	do { \
		(x)->curr_alloc = (void *) NULL; \
	} while (0)
#endif

/* No pointer compression because pointer is potentially outside of
 * Duktape heap.
 */
#if defined(DUK_USE_HEAPPTR16)
#define DUK_HBUFFER_EXTERNAL_GET_DATA_PTR(heap, x) ((void *) (x)->curr_alloc)
#define DUK_HBUFFER_EXTERNAL_SET_DATA_PTR(heap, x, v) \
	do { \
		(x)->curr_alloc = (void *) (v); \
	} while (0)
#define DUK_HBUFFER_EXTERNAL_SET_DATA_PTR_NULL(heap, x) \
	do { \
		(x)->curr_alloc = (void *) NULL; \
	} while (0)
#else
#define DUK_HBUFFER_EXTERNAL_GET_DATA_PTR(heap, x) ((void *) (x)->curr_alloc)
#define DUK_HBUFFER_EXTERNAL_SET_DATA_PTR(heap, x, v) \
	do { \
		(x)->curr_alloc = (void *) (v); \
	} while (0)
#define DUK_HBUFFER_EXTERNAL_SET_DATA_PTR_NULL(heap, x) \
	do { \
		(x)->curr_alloc = (void *) NULL; \
	} while (0)
#endif

/* Get a pointer to the current buffer contents (matching current allocation
 * size).  May be NULL for zero size dynamic/external buffer.
 */
#if defined(DUK_USE_HEAPPTR16)
#define DUK_HBUFFER_GET_DATA_PTR(heap, x) \
	(DUK_HBUFFER_HAS_DYNAMIC((x)) ? \
             (DUK_HBUFFER_HAS_EXTERNAL((x)) ? DUK_HBUFFER_EXTERNAL_GET_DATA_PTR((heap), (duk_hbuffer_external *) (x)) : \
                                              DUK_HBUFFER_DYNAMIC_GET_DATA_PTR((heap), (duk_hbuffer_dynamic *) (x))) : \
             DUK_HBUFFER_FIXED_GET_DATA_PTR((heap), (duk_hbuffer_fixed *) (void *) (x)))
#else
/* Without heap pointer compression duk_hbuffer_dynamic and duk_hbuffer_external
 * have the same layout so checking for fixed vs. dynamic (or external) is enough.
 */
#define DUK_HBUFFER_GET_DATA_PTR(heap, x) \
	(DUK_HBUFFER_HAS_DYNAMIC((x)) ? DUK_HBUFFER_DYNAMIC_GET_DATA_PTR((heap), (duk_hbuffer_dynamic *) (x)) : \
                                        DUK_HBUFFER_FIXED_GET_DATA_PTR((heap), (duk_hbuffer_fixed *) (void *) (x)))
#endif

/* Validity assert. */
#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL_DECL void duk_hbuffer_assert_valid(duk_hbuffer *h);
#define DUK_HBUFFER_ASSERT_VALID(h) \
	do { \
		duk_hbuffer_assert_valid((h)); \
	} while (0)
#else
#define DUK_HBUFFER_ASSERT_VALID(h) \
	do { \
	} while (0)
#endif

/*
 *  Structs
 */

/* Shared prefix for all buffer types. */
struct duk_hbuffer {
	duk_heaphdr hdr;

	/* It's not strictly necessary to track the current size, but
	 * it is useful for writing robust native code.
	 */

	/* Current size. */
#if defined(DUK_USE_BUFLEN16)
	/* Stored in duk_heaphdr unused flags. */
#else
	duk_size_t size;
#endif

	/*
	 *  Data following the header depends on the DUK_HBUFFER_FLAG_DYNAMIC
	 *  flag.
	 *
	 *  If the flag is clear (the buffer is a fixed size one), the buffer
	 *  data follows the header directly, consisting of 'size' bytes.
	 *
	 *  If the flag is set, the actual buffer is allocated separately, and
	 *  a few control fields follow the header.  Specifically:
	 *
	 *    - a "void *" pointing to the current allocation
	 *    - a duk_size_t indicating the full allocated size (always >= 'size')
	 *
	 *  If DUK_HBUFFER_FLAG_EXTERNAL is set, the buffer has been allocated
	 *  by user code, so that Duktape won't be able to resize it and won't
	 *  free it.  This allows buffers to point to e.g. an externally
	 *  allocated structure such as a frame buffer.
	 *
	 *  Unlike strings, no terminator byte (NUL) is guaranteed after the
	 *  data.  This would be convenient, but would pad aligned user buffers
	 *  unnecessarily upwards in size.  For instance, if user code requested
	 *  a 64-byte dynamic buffer, 65 bytes would actually be allocated which
	 *  would then potentially round upwards to perhaps 68 or 72 bytes.
	 */
};

/* Fixed buffer; data follows struct, with proper alignment guaranteed by
 * struct size.
 */
#if (DUK_USE_ALIGN_BY == 8) && defined(DUK_USE_PACK_MSVC_PRAGMA)
#pragma pack(push, 8)
#endif
struct duk_hbuffer_fixed {
	/* A union is used here as a portable struct size / alignment trick:
	 * by adding a 32-bit or a 64-bit (unused) union member, the size of
	 * the struct is effectively forced to be a multiple of 4 or 8 bytes
	 * (respectively) without increasing the size of the struct unless
	 * necessary.
	 */
	union {
		struct {
			duk_heaphdr hdr;
#if defined(DUK_USE_BUFLEN16)
			/* Stored in duk_heaphdr unused flags. */
#else
			duk_size_t size;
#endif
		} s;
#if (DUK_USE_ALIGN_BY == 4)
		duk_uint32_t dummy_for_align4;
#elif (DUK_USE_ALIGN_BY == 8)
		duk_double_t dummy_for_align8_1;
#if defined(DUK_USE_64BIT_OPS)
		duk_uint64_t dummy_for_align8_2;
#endif
#elif (DUK_USE_ALIGN_BY == 1)
		/* no extra padding */
#else
#error invalid DUK_USE_ALIGN_BY
#endif
	} u;

	/*
	 *  Data follows the struct header.  The struct size is padded by the
	 *  compiler based on the struct members.  This guarantees that the
	 *  buffer data will be aligned-by-4 but not necessarily aligned-by-8.
	 *
	 *  On platforms where alignment does not matter, the struct padding
	 *  could be removed (if there is any).  On platforms where alignment
	 *  by 8 is required, the struct size must be forced to be a multiple
	 *  of 8 by some means.  Without it, some user code may break, and also
	 *  Duktape itself breaks (e.g. the compiler stores duk_tvals in a
	 *  dynamic buffer).
	 */
}
#if (DUK_USE_ALIGN_BY == 8) && defined(DUK_USE_PACK_GCC_ATTR)
__attribute__((aligned(8)))
#elif (DUK_USE_ALIGN_BY == 8) && defined(DUK_USE_PACK_CLANG_ATTR)
__attribute__((aligned(8)))
#endif
;
#if (DUK_USE_ALIGN_BY == 8) && defined(DUK_USE_PACK_MSVC_PRAGMA)
#pragma pack(pop)
#endif

/* Dynamic buffer with 'curr_alloc' pointing to a dynamic area allocated using
 * heap allocation primitives.  Also used for external buffers when low memory
 * options are not used.
 */
struct duk_hbuffer_dynamic {
	duk_heaphdr hdr;

#if defined(DUK_USE_BUFLEN16)
	/* Stored in duk_heaphdr unused flags. */
#else
	duk_size_t size;
#endif

#if defined(DUK_USE_HEAPPTR16)
	/* Stored in duk_heaphdr h_extra16. */
#else
	void *curr_alloc; /* may be NULL if alloc_size == 0 */
#endif

	/*
	 *  Allocation size for 'curr_alloc' is alloc_size.  There is no
	 *  automatic NUL terminator for buffers (see above for rationale).
	 *
	 *  'curr_alloc' is explicitly allocated with heap allocation
	 *  primitives and will thus always have alignment suitable for
	 *  e.g. duk_tval and an IEEE double.
	 */
};

/* External buffer with 'curr_alloc' managed by user code and pointing to an
 * arbitrary address.  When heap pointer compression is not used, this struct
 * has the same layout as duk_hbuffer_dynamic.
 */
struct duk_hbuffer_external {
	duk_heaphdr hdr;

#if defined(DUK_USE_BUFLEN16)
	/* Stored in duk_heaphdr unused flags. */
#else
	duk_size_t size;
#endif

	/* Cannot be compressed as a heap pointer because may point to
	 * an arbitrary address.
	 */
	void *curr_alloc; /* may be NULL if alloc_size == 0 */
};

/*
 *  Prototypes
 */

DUK_INTERNAL_DECL duk_hbuffer *duk_hbuffer_alloc(duk_heap *heap, duk_size_t size, duk_small_uint_t flags, void **out_bufdata);
DUK_INTERNAL_DECL void *duk_hbuffer_get_dynalloc_ptr(duk_heap *heap, void *ud); /* indirect allocs */

/* dynamic buffer ops */
DUK_INTERNAL_DECL void duk_hbuffer_resize(duk_hthread *thr, duk_hbuffer_dynamic *buf, duk_size_t new_size);
DUK_INTERNAL_DECL void duk_hbuffer_reset(duk_hthread *thr, duk_hbuffer_dynamic *buf);

#endif /* DUK_HBUFFER_H_INCLUDED */
