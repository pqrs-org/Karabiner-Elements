/*
 *  Heap header definition and assorted macros, including ref counting.
 *  Access all fields through the accessor macros.
 */

#if !defined(DUK_HEAPHDR_H_INCLUDED)
#define DUK_HEAPHDR_H_INCLUDED

/*
 *  Common heap header
 *
 *  All heap objects share the same flags and refcount fields.  Objects other
 *  than strings also need to have a single or double linked list pointers
 *  for insertion into the "heap allocated" list.  Strings have single linked
 *  list pointers for string table chaining.
 *
 *  Technically, 'h_refcount' must be wide enough to guarantee that it cannot
 *  wrap; otherwise objects might be freed incorrectly after wrapping.  The
 *  default refcount field is 32 bits even on 64-bit systems: while that's in
 *  theory incorrect, the Duktape heap needs to be larger than 64GB for the
 *  count to actually wrap (assuming 16-byte duk_tvals).  This is very unlikely
 *  to ever be an issue, but if it is, disabling DUK_USE_REFCOUNT32 causes
 *  Duktape to use size_t for refcounts which should always be safe.
 *
 *  Heap header size on 32-bit platforms: 8 bytes without reference counting,
 *  16 bytes with reference counting.
 *
 *  Note that 'raw' macros such as DUK_HEAPHDR_GET_REFCOUNT() are not
 *  defined without DUK_USE_REFERENCE_COUNTING, so caller must #if defined()
 *  around them.
 */

/* XXX: macro for shared header fields (avoids some padding issues) */

struct duk_heaphdr {
	duk_uint32_t h_flags;

#if defined(DUK_USE_REFERENCE_COUNTING)
#if defined(DUK_USE_ASSERTIONS)
	/* When assertions enabled, used by mark-and-sweep for refcount
	 * validation.  Largest reasonable type; also detects overflows.
	 */
	duk_size_t h_assert_refcount;
#endif
#if defined(DUK_USE_REFCOUNT16)
	duk_uint16_t h_refcount;
#elif defined(DUK_USE_REFCOUNT32)
	duk_uint32_t h_refcount;
#else
	duk_size_t h_refcount;
#endif
#endif /* DUK_USE_REFERENCE_COUNTING */

#if defined(DUK_USE_HEAPPTR16)
	duk_uint16_t h_next16;
#else
	duk_heaphdr *h_next;
#endif

#if defined(DUK_USE_DOUBLE_LINKED_HEAP)
	/* refcounting requires direct heap frees, which in turn requires a dual linked heap */
#if defined(DUK_USE_HEAPPTR16)
	duk_uint16_t h_prev16;
#else
	duk_heaphdr *h_prev;
#endif
#endif

	/* When DUK_USE_HEAPPTR16 (and DUK_USE_REFCOUNT16) is in use, the
	 * struct won't align nicely to 4 bytes.  This 16-bit extra field
	 * is added to make the alignment clean; the field can be used by
	 * heap objects when 16-bit packing is used.  This field is now
	 * conditional to DUK_USE_HEAPPTR16 only, but it is intended to be
	 * used with DUK_USE_REFCOUNT16 and DUK_USE_DOUBLE_LINKED_HEAP;
	 * this only matter to low memory environments anyway.
	 */
#if defined(DUK_USE_HEAPPTR16)
	duk_uint16_t h_extra16;
#endif
};

struct duk_heaphdr_string {
	/* 16 bits would be enough for shared heaphdr flags and duk_hstring
	 * flags.  The initial parts of duk_heaphdr_string and duk_heaphdr
	 * must match so changing the flags field size here would be quite
	 * awkward.  However, to minimize struct size, we can pack at least
	 * 16 bits of duk_hstring data into the flags field.
	 */
	duk_uint32_t h_flags;

#if defined(DUK_USE_REFERENCE_COUNTING)
#if defined(DUK_USE_ASSERTIONS)
	/* When assertions enabled, used by mark-and-sweep for refcount
	 * validation.  Largest reasonable type; also detects overflows.
	 */
	duk_size_t h_assert_refcount;
#endif
#if defined(DUK_USE_REFCOUNT16)
	duk_uint16_t h_refcount;
	duk_uint16_t h_strextra16; /* round out to 8 bytes */
#elif defined(DUK_USE_REFCOUNT32)
	duk_uint32_t h_refcount;
#else
	duk_size_t h_refcount;
#endif
#else
	duk_uint16_t h_strextra16;
#endif /* DUK_USE_REFERENCE_COUNTING */

	duk_hstring *h_next;
	/* No 'h_prev' pointer for strings. */
};

#define DUK_HEAPHDR_FLAGS_TYPE_MASK 0x00000003UL
#define DUK_HEAPHDR_FLAGS_FLAG_MASK (~DUK_HEAPHDR_FLAGS_TYPE_MASK)

/* 2 bits for heap type */
#define DUK_HEAPHDR_FLAGS_HEAP_START 2 /* 5 heap flags */
#define DUK_HEAPHDR_FLAGS_USER_START 7 /* 25 user flags */

#define DUK_HEAPHDR_HEAP_FLAG_NUMBER(n) (DUK_HEAPHDR_FLAGS_HEAP_START + (n))
#define DUK_HEAPHDR_USER_FLAG_NUMBER(n) (DUK_HEAPHDR_FLAGS_USER_START + (n))
#define DUK_HEAPHDR_HEAP_FLAG(n)        (1UL << (DUK_HEAPHDR_FLAGS_HEAP_START + (n)))
#define DUK_HEAPHDR_USER_FLAG(n)        (1UL << (DUK_HEAPHDR_FLAGS_USER_START + (n)))

#define DUK_HEAPHDR_FLAG_REACHABLE   DUK_HEAPHDR_HEAP_FLAG(0) /* mark-and-sweep: reachable */
#define DUK_HEAPHDR_FLAG_TEMPROOT    DUK_HEAPHDR_HEAP_FLAG(1) /* mark-and-sweep: children not processed */
#define DUK_HEAPHDR_FLAG_FINALIZABLE DUK_HEAPHDR_HEAP_FLAG(2) /* mark-and-sweep: finalizable (on current pass) */
#define DUK_HEAPHDR_FLAG_FINALIZED   DUK_HEAPHDR_HEAP_FLAG(3) /* mark-and-sweep: finalized (on previous pass) */
#define DUK_HEAPHDR_FLAG_READONLY    DUK_HEAPHDR_HEAP_FLAG(4) /* read-only object, in code section */

#define DUK_HTYPE_MIN    0
#define DUK_HTYPE_STRING 0
#define DUK_HTYPE_OBJECT 1
#define DUK_HTYPE_BUFFER 2
#define DUK_HTYPE_MAX    2

#if defined(DUK_USE_HEAPPTR16)
#define DUK_HEAPHDR_GET_NEXT(heap, h) ((duk_heaphdr *) DUK_USE_HEAPPTR_DEC16((heap)->heap_udata, (h)->h_next16))
#define DUK_HEAPHDR_SET_NEXT(heap, h, val) \
	do { \
		(h)->h_next16 = DUK_USE_HEAPPTR_ENC16((heap)->heap_udata, (void *) val); \
	} while (0)
#else
#define DUK_HEAPHDR_GET_NEXT(heap, h) ((h)->h_next)
#define DUK_HEAPHDR_SET_NEXT(heap, h, val) \
	do { \
		(h)->h_next = (val); \
	} while (0)
#endif

#if defined(DUK_USE_DOUBLE_LINKED_HEAP)
#if defined(DUK_USE_HEAPPTR16)
#define DUK_HEAPHDR_GET_PREV(heap, h) ((duk_heaphdr *) DUK_USE_HEAPPTR_DEC16((heap)->heap_udata, (h)->h_prev16))
#define DUK_HEAPHDR_SET_PREV(heap, h, val) \
	do { \
		(h)->h_prev16 = DUK_USE_HEAPPTR_ENC16((heap)->heap_udata, (void *) (val)); \
	} while (0)
#else
#define DUK_HEAPHDR_GET_PREV(heap, h) ((h)->h_prev)
#define DUK_HEAPHDR_SET_PREV(heap, h, val) \
	do { \
		(h)->h_prev = (val); \
	} while (0)
#endif
#endif

#if defined(DUK_USE_REFERENCE_COUNTING)
#define DUK_HEAPHDR_GET_REFCOUNT(h) ((h)->h_refcount)
#define DUK_HEAPHDR_SET_REFCOUNT(h, val) \
	do { \
		(h)->h_refcount = (val); \
		DUK_ASSERT((h)->h_refcount == (val)); /* No truncation. */ \
	} while (0)
#define DUK_HEAPHDR_PREINC_REFCOUNT(h) (++(h)->h_refcount) /* result: updated refcount */
#define DUK_HEAPHDR_PREDEC_REFCOUNT(h) (--(h)->h_refcount) /* result: updated refcount */
#else
/* refcount macros not defined without refcounting, caller must #if defined() now */
#endif /* DUK_USE_REFERENCE_COUNTING */

/*
 *  Note: type is treated as a field separate from flags, so some masking is
 *  involved in the macros below.
 */

#define DUK_HEAPHDR_GET_FLAGS_RAW(h) ((h)->h_flags)
#define DUK_HEAPHDR_SET_FLAGS_RAW(h, val) \
	do { \
		(h)->h_flags = (val); \
	} \
	}
#define DUK_HEAPHDR_GET_FLAGS(h) ((h)->h_flags & DUK_HEAPHDR_FLAGS_FLAG_MASK)
#define DUK_HEAPHDR_SET_FLAGS(h, val) \
	do { \
		(h)->h_flags = ((h)->h_flags & ~(DUK_HEAPHDR_FLAGS_FLAG_MASK)) | (val); \
	} while (0)
#define DUK_HEAPHDR_GET_TYPE(h) ((h)->h_flags & DUK_HEAPHDR_FLAGS_TYPE_MASK)
#define DUK_HEAPHDR_SET_TYPE(h, val) \
	do { \
		(h)->h_flags = ((h)->h_flags & ~(DUK_HEAPHDR_FLAGS_TYPE_MASK)) | (val); \
	} while (0)

/* Comparison for type >= DUK_HTYPE_MIN skipped; because DUK_HTYPE_MIN is zero
 * and the comparison is unsigned, it's always true and generates warnings.
 */
#define DUK_HEAPHDR_HTYPE_VALID(h) (DUK_HEAPHDR_GET_TYPE((h)) <= DUK_HTYPE_MAX)

#define DUK_HEAPHDR_SET_TYPE_AND_FLAGS(h, tval, fval) \
	do { \
		(h)->h_flags = ((tval) &DUK_HEAPHDR_FLAGS_TYPE_MASK) | ((fval) &DUK_HEAPHDR_FLAGS_FLAG_MASK); \
	} while (0)

#define DUK_HEAPHDR_SET_FLAG_BITS(h, bits) \
	do { \
		DUK_ASSERT(((bits) & ~(DUK_HEAPHDR_FLAGS_FLAG_MASK)) == 0); \
		(h)->h_flags |= (bits); \
	} while (0)

#define DUK_HEAPHDR_CLEAR_FLAG_BITS(h, bits) \
	do { \
		DUK_ASSERT(((bits) & ~(DUK_HEAPHDR_FLAGS_FLAG_MASK)) == 0); \
		(h)->h_flags &= ~((bits)); \
	} while (0)

#define DUK_HEAPHDR_CHECK_FLAG_BITS(h, bits) (((h)->h_flags & (bits)) != 0)

#define DUK_HEAPHDR_SET_REACHABLE(h)   DUK_HEAPHDR_SET_FLAG_BITS((h), DUK_HEAPHDR_FLAG_REACHABLE)
#define DUK_HEAPHDR_CLEAR_REACHABLE(h) DUK_HEAPHDR_CLEAR_FLAG_BITS((h), DUK_HEAPHDR_FLAG_REACHABLE)
#define DUK_HEAPHDR_HAS_REACHABLE(h)   DUK_HEAPHDR_CHECK_FLAG_BITS((h), DUK_HEAPHDR_FLAG_REACHABLE)

#define DUK_HEAPHDR_SET_TEMPROOT(h)   DUK_HEAPHDR_SET_FLAG_BITS((h), DUK_HEAPHDR_FLAG_TEMPROOT)
#define DUK_HEAPHDR_CLEAR_TEMPROOT(h) DUK_HEAPHDR_CLEAR_FLAG_BITS((h), DUK_HEAPHDR_FLAG_TEMPROOT)
#define DUK_HEAPHDR_HAS_TEMPROOT(h)   DUK_HEAPHDR_CHECK_FLAG_BITS((h), DUK_HEAPHDR_FLAG_TEMPROOT)

#define DUK_HEAPHDR_SET_FINALIZABLE(h)   DUK_HEAPHDR_SET_FLAG_BITS((h), DUK_HEAPHDR_FLAG_FINALIZABLE)
#define DUK_HEAPHDR_CLEAR_FINALIZABLE(h) DUK_HEAPHDR_CLEAR_FLAG_BITS((h), DUK_HEAPHDR_FLAG_FINALIZABLE)
#define DUK_HEAPHDR_HAS_FINALIZABLE(h)   DUK_HEAPHDR_CHECK_FLAG_BITS((h), DUK_HEAPHDR_FLAG_FINALIZABLE)

#define DUK_HEAPHDR_SET_FINALIZED(h)   DUK_HEAPHDR_SET_FLAG_BITS((h), DUK_HEAPHDR_FLAG_FINALIZED)
#define DUK_HEAPHDR_CLEAR_FINALIZED(h) DUK_HEAPHDR_CLEAR_FLAG_BITS((h), DUK_HEAPHDR_FLAG_FINALIZED)
#define DUK_HEAPHDR_HAS_FINALIZED(h)   DUK_HEAPHDR_CHECK_FLAG_BITS((h), DUK_HEAPHDR_FLAG_FINALIZED)

#define DUK_HEAPHDR_SET_READONLY(h)   DUK_HEAPHDR_SET_FLAG_BITS((h), DUK_HEAPHDR_FLAG_READONLY)
#define DUK_HEAPHDR_CLEAR_READONLY(h) DUK_HEAPHDR_CLEAR_FLAG_BITS((h), DUK_HEAPHDR_FLAG_READONLY)
#define DUK_HEAPHDR_HAS_READONLY(h)   DUK_HEAPHDR_CHECK_FLAG_BITS((h), DUK_HEAPHDR_FLAG_READONLY)

/* get or set a range of flags; m=first bit number, n=number of bits */
#define DUK_HEAPHDR_GET_FLAG_RANGE(h, m, n) (((h)->h_flags >> (m)) & ((1UL << (n)) - 1UL))

#define DUK_HEAPHDR_SET_FLAG_RANGE(h, m, n, v) \
	do { \
		(h)->h_flags = ((h)->h_flags & (~(((1UL << (n)) - 1UL) << (m)))) | ((v) << (m)); \
	} while (0)

/* init pointer fields to null */
#if defined(DUK_USE_DOUBLE_LINKED_HEAP)
#define DUK_HEAPHDR_INIT_NULLS(h) \
	do { \
		DUK_HEAPHDR_SET_NEXT((h), (void *) NULL); \
		DUK_HEAPHDR_SET_PREV((h), (void *) NULL); \
	} while (0)
#else
#define DUK_HEAPHDR_INIT_NULLS(h) \
	do { \
		DUK_HEAPHDR_SET_NEXT((h), (void *) NULL); \
	} while (0)
#endif

#define DUK_HEAPHDR_STRING_INIT_NULLS(h) \
	do { \
		(h)->h_next = NULL; \
	} while (0)

/*
 *  Type tests
 */

/* Take advantage of the fact that for DUK_HTYPE_xxx numbers the lowest bit
 * is only set for DUK_HTYPE_OBJECT (= 1).
 */
#if 0
#define DUK_HEAPHDR_IS_OBJECT(h) (DUK_HEAPHDR_GET_TYPE((h)) == DUK_HTYPE_OBJECT)
#endif
#define DUK_HEAPHDR_IS_OBJECT(h) ((h)->h_flags & 0x01UL)
#define DUK_HEAPHDR_IS_STRING(h) (DUK_HEAPHDR_GET_TYPE((h)) == DUK_HTYPE_STRING)
#define DUK_HEAPHDR_IS_BUFFER(h) (DUK_HEAPHDR_GET_TYPE((h)) == DUK_HTYPE_BUFFER)

/*
 *  Assert helpers
 */

/* Check that prev/next links are consistent: if e.g. h->prev is != NULL,
 * h->prev->next should point back to h.
 */
#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL_DECL void duk_heaphdr_assert_valid_subclassed(duk_heaphdr *h);
DUK_INTERNAL_DECL void duk_heaphdr_assert_links(duk_heap *heap, duk_heaphdr *h);
DUK_INTERNAL_DECL void duk_heaphdr_assert_valid(duk_heaphdr *h);
#define DUK_HEAPHDR_ASSERT_LINKS(heap, h) \
	do { \
		duk_heaphdr_assert_links((heap), (h)); \
	} while (0)
#define DUK_HEAPHDR_ASSERT_VALID(h) \
	do { \
		duk_heaphdr_assert_valid((h)); \
	} while (0)
#else
#define DUK_HEAPHDR_ASSERT_LINKS(heap, h) \
	do { \
	} while (0)
#define DUK_HEAPHDR_ASSERT_VALID(h) \
	do { \
	} while (0)
#endif

#endif /* DUK_HEAPHDR_H_INCLUDED */
