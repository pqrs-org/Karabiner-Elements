/*
 *  Tagged type definition (duk_tval) and accessor macros.
 *
 *  Access all fields through the accessor macros, as the representation
 *  is quite tricky.
 *
 *  There are two packed type alternatives: an 8-byte representation
 *  based on an IEEE double (preferred for compactness), and a 12-byte
 *  representation (portability).  The latter is needed also in e.g.
 *  64-bit environments (it usually pads to 16 bytes per value).
 *
 *  Selecting the tagged type format involves many trade-offs (memory
 *  use, size and performance of generated code, portability, etc).
 *
 *  NB: because macro arguments are often expressions, macros should
 *  avoid evaluating their argument more than once.
 */

#if !defined(DUK_TVAL_H_INCLUDED)
#define DUK_TVAL_H_INCLUDED

/* sanity */
#if !defined(DUK_USE_DOUBLE_LE) && !defined(DUK_USE_DOUBLE_ME) && !defined(DUK_USE_DOUBLE_BE)
#error unsupported: cannot determine byte order variant
#endif

#if defined(DUK_USE_PACKED_TVAL)
/* ======================================================================== */

/*
 *  Packed 8-byte representation
 */

/* use duk_double_union as duk_tval directly */
typedef union duk_double_union duk_tval;
typedef struct {
	duk_uint16_t a;
	duk_uint16_t b;
	duk_uint16_t c;
	duk_uint16_t d;
} duk_tval_unused;

/* tags */
#define DUK_TAG_NORMALIZED_NAN 0x7ff8UL /* the NaN variant we use */
/* avoid tag 0xfff0, no risk of confusion with negative infinity */
#define DUK_TAG_MIN            0xfff1UL
#if defined(DUK_USE_FASTINT)
#define DUK_TAG_FASTINT 0xfff1UL /* embed: integer value */
#endif
#define DUK_TAG_UNUSED    0xfff2UL /* marker; not actual tagged value */
#define DUK_TAG_UNDEFINED 0xfff3UL /* embed: nothing */
#define DUK_TAG_NULL      0xfff4UL /* embed: nothing */
#define DUK_TAG_BOOLEAN   0xfff5UL /* embed: 0 or 1 (false or true) */
/* DUK_TAG_NUMBER would logically go here, but it has multiple 'tags' */
#define DUK_TAG_POINTER   0xfff6UL /* embed: void ptr */
#define DUK_TAG_LIGHTFUNC 0xfff7UL /* embed: func ptr */
#define DUK_TAG_STRING    0xfff8UL /* embed: duk_hstring ptr */
#define DUK_TAG_OBJECT    0xfff9UL /* embed: duk_hobject ptr */
#define DUK_TAG_BUFFER    0xfffaUL /* embed: duk_hbuffer ptr */
#define DUK_TAG_MAX       0xfffaUL

/* for convenience */
#define DUK_XTAG_BOOLEAN_FALSE 0xfff50000UL
#define DUK_XTAG_BOOLEAN_TRUE  0xfff50001UL

#define DUK_TVAL_IS_VALID_TAG(tv) (DUK_TVAL_GET_TAG((tv)) - DUK_TAG_MIN <= DUK_TAG_MAX - DUK_TAG_MIN)

/* DUK_TVAL_UNUSED initializer for duk_tval_unused, works for any endianness. */
#define DUK_TVAL_UNUSED_INITIALIZER() \
	{ DUK_TAG_UNUSED, DUK_TAG_UNUSED, DUK_TAG_UNUSED, DUK_TAG_UNUSED }

/* two casts to avoid gcc warning: "warning: cast from pointer to integer of different size [-Wpointer-to-int-cast]" */
#if defined(DUK_USE_64BIT_OPS)
#if defined(DUK_USE_DOUBLE_ME)
#define DUK__TVAL_SET_TAGGEDPOINTER(tv, h, tag) \
	do { \
		(tv)->ull[DUK_DBL_IDX_ULL0] = (((duk_uint64_t) (tag)) << 16) | (((duk_uint64_t) (duk_uint32_t) (h)) << 32); \
	} while (0)
#else
#define DUK__TVAL_SET_TAGGEDPOINTER(tv, h, tag) \
	do { \
		(tv)->ull[DUK_DBL_IDX_ULL0] = (((duk_uint64_t) (tag)) << 48) | ((duk_uint64_t) (duk_uint32_t) (h)); \
	} while (0)
#endif
#else /* DUK_USE_64BIT_OPS */
#define DUK__TVAL_SET_TAGGEDPOINTER(tv, h, tag) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->ui[DUK_DBL_IDX_UI0] = ((duk_uint32_t) (tag)) << 16; \
		duk__tv->ui[DUK_DBL_IDX_UI1] = (duk_uint32_t) (h); \
	} while (0)
#endif /* DUK_USE_64BIT_OPS */

#if defined(DUK_USE_64BIT_OPS)
/* Double casting for pointer to avoid gcc warning (cast from pointer to integer of different size) */
#if defined(DUK_USE_DOUBLE_ME)
#define DUK__TVAL_SET_LIGHTFUNC(tv, fp, flags) \
	do { \
		(tv)->ull[DUK_DBL_IDX_ULL0] = (((duk_uint64_t) DUK_TAG_LIGHTFUNC) << 16) | ((duk_uint64_t) (flags)) | \
		                              (((duk_uint64_t) (duk_uint32_t) (fp)) << 32); \
	} while (0)
#else
#define DUK__TVAL_SET_LIGHTFUNC(tv, fp, flags) \
	do { \
		(tv)->ull[DUK_DBL_IDX_ULL0] = (((duk_uint64_t) DUK_TAG_LIGHTFUNC) << 48) | (((duk_uint64_t) (flags)) << 32) | \
		                              ((duk_uint64_t) (duk_uint32_t) (fp)); \
	} while (0)
#endif
#else /* DUK_USE_64BIT_OPS */
#define DUK__TVAL_SET_LIGHTFUNC(tv, fp, flags) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->ui[DUK_DBL_IDX_UI0] = (((duk_uint32_t) DUK_TAG_LIGHTFUNC) << 16) | ((duk_uint32_t) (flags)); \
		duk__tv->ui[DUK_DBL_IDX_UI1] = (duk_uint32_t) (fp); \
	} while (0)
#endif /* DUK_USE_64BIT_OPS */

#if defined(DUK_USE_FASTINT)
/* Note: masking is done for 'i' to deal with negative numbers correctly */
#if defined(DUK_USE_DOUBLE_ME)
#define DUK__TVAL_SET_I48(tv, i) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->ui[DUK_DBL_IDX_UI0] = \
		    ((duk_uint32_t) DUK_TAG_FASTINT) << 16 | (((duk_uint32_t) ((i) >> 32)) & 0x0000ffffUL); \
		duk__tv->ui[DUK_DBL_IDX_UI1] = (duk_uint32_t) (i); \
	} while (0)
#define DUK__TVAL_SET_U32(tv, i) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->ui[DUK_DBL_IDX_UI0] = ((duk_uint32_t) DUK_TAG_FASTINT) << 16; \
		duk__tv->ui[DUK_DBL_IDX_UI1] = (duk_uint32_t) (i); \
	} while (0)
#else
#define DUK__TVAL_SET_I48(tv, i) \
	do { \
		(tv)->ull[DUK_DBL_IDX_ULL0] = \
		    (((duk_uint64_t) DUK_TAG_FASTINT) << 48) | (((duk_uint64_t) (i)) & DUK_U64_CONSTANT(0x0000ffffffffffff)); \
	} while (0)
#define DUK__TVAL_SET_U32(tv, i) \
	do { \
		(tv)->ull[DUK_DBL_IDX_ULL0] = (((duk_uint64_t) DUK_TAG_FASTINT) << 48) | (duk_uint64_t) (i); \
	} while (0)
#endif

/* This needs to go through a cast because sign extension is needed. */
#define DUK__TVAL_SET_I32(tv, i) \
	do { \
		duk_int64_t duk__tmp = (duk_int64_t) (i); \
		DUK_TVAL_SET_I48((tv), duk__tmp); \
	} while (0)

/* XXX: Clumsy sign extend and masking of 16 topmost bits. */
#if defined(DUK_USE_DOUBLE_ME)
#define DUK__TVAL_GET_FASTINT(tv) \
	(((duk_int64_t) ((((duk_uint64_t) (tv)->ui[DUK_DBL_IDX_UI0]) << 32) | ((duk_uint64_t) (tv)->ui[DUK_DBL_IDX_UI1]))) \
	     << 16 >> \
	 16)
#else
#define DUK__TVAL_GET_FASTINT(tv) ((((duk_int64_t) (tv)->ull[DUK_DBL_IDX_ULL0]) << 16) >> 16)
#endif
#define DUK__TVAL_GET_FASTINT_U32(tv) ((tv)->ui[DUK_DBL_IDX_UI1])
#define DUK__TVAL_GET_FASTINT_I32(tv) ((duk_int32_t) (tv)->ui[DUK_DBL_IDX_UI1])
#endif /* DUK_USE_FASTINT */

#define DUK_TVAL_SET_UNDEFINED(tv) \
	do { \
		(tv)->us[DUK_DBL_IDX_US0] = (duk_uint16_t) DUK_TAG_UNDEFINED; \
	} while (0)
#define DUK_TVAL_SET_UNUSED(tv) \
	do { \
		(tv)->us[DUK_DBL_IDX_US0] = (duk_uint16_t) DUK_TAG_UNUSED; \
	} while (0)
#define DUK_TVAL_SET_NULL(tv) \
	do { \
		(tv)->us[DUK_DBL_IDX_US0] = (duk_uint16_t) DUK_TAG_NULL; \
	} while (0)

#define DUK_TVAL_SET_BOOLEAN(tv, val) \
	DUK_DBLUNION_SET_HIGH32((tv), (((duk_uint32_t) DUK_TAG_BOOLEAN) << 16) | ((duk_uint32_t) (val)))

#define DUK_TVAL_SET_NAN(tv) DUK_DBLUNION_SET_NAN_FULL((tv))

/* Assumes that caller has normalized NaNs, otherwise trouble ahead. */
#if defined(DUK_USE_FASTINT)
#define DUK_TVAL_SET_DOUBLE(tv, d) \
	do { \
		duk_double_t duk__dblval; \
		duk__dblval = (d); \
		DUK_ASSERT_DOUBLE_IS_NORMALIZED(duk__dblval); \
		DUK_DBLUNION_SET_DOUBLE((tv), duk__dblval); \
	} while (0)
#define DUK_TVAL_SET_I48(tv, i)                 DUK__TVAL_SET_I48((tv), (i))
#define DUK_TVAL_SET_I32(tv, i)                 DUK__TVAL_SET_I32((tv), (i))
#define DUK_TVAL_SET_U32(tv, i)                 DUK__TVAL_SET_U32((tv), (i))
#define DUK_TVAL_SET_NUMBER_CHKFAST_FAST(tv, d) duk_tval_set_number_chkfast_fast((tv), (d))
#define DUK_TVAL_SET_NUMBER_CHKFAST_SLOW(tv, d) duk_tval_set_number_chkfast_slow((tv), (d))
#define DUK_TVAL_SET_NUMBER(tv, d)              DUK_TVAL_SET_DOUBLE((tv), (d))
#define DUK_TVAL_CHKFAST_INPLACE_FAST(tv) \
	do { \
		duk_tval *duk__tv; \
		duk_double_t duk__d; \
		duk__tv = (tv); \
		if (DUK_TVAL_IS_DOUBLE(duk__tv)) { \
			duk__d = DUK_TVAL_GET_DOUBLE(duk__tv); \
			DUK_TVAL_SET_NUMBER_CHKFAST_FAST(duk__tv, duk__d); \
		} \
	} while (0)
#define DUK_TVAL_CHKFAST_INPLACE_SLOW(tv) \
	do { \
		duk_tval *duk__tv; \
		duk_double_t duk__d; \
		duk__tv = (tv); \
		if (DUK_TVAL_IS_DOUBLE(duk__tv)) { \
			duk__d = DUK_TVAL_GET_DOUBLE(duk__tv); \
			DUK_TVAL_SET_NUMBER_CHKFAST_SLOW(duk__tv, duk__d); \
		} \
	} while (0)
#else /* DUK_USE_FASTINT */
#define DUK_TVAL_SET_DOUBLE(tv, d) \
	do { \
		duk_double_t duk__dblval; \
		duk__dblval = (d); \
		DUK_ASSERT_DOUBLE_IS_NORMALIZED(duk__dblval); \
		DUK_DBLUNION_SET_DOUBLE((tv), duk__dblval); \
	} while (0)
#define DUK_TVAL_SET_I48(tv, i)                 DUK_TVAL_SET_DOUBLE((tv), (duk_double_t) (i)) /* XXX: fast int-to-double */
#define DUK_TVAL_SET_I32(tv, i)                 DUK_TVAL_SET_DOUBLE((tv), (duk_double_t) (i))
#define DUK_TVAL_SET_U32(tv, i)                 DUK_TVAL_SET_DOUBLE((tv), (duk_double_t) (i))
#define DUK_TVAL_SET_NUMBER_CHKFAST_FAST(tv, d) DUK_TVAL_SET_DOUBLE((tv), (d))
#define DUK_TVAL_SET_NUMBER_CHKFAST_SLOW(tv, d) DUK_TVAL_SET_DOUBLE((tv), (d))
#define DUK_TVAL_SET_NUMBER(tv, d)              DUK_TVAL_SET_DOUBLE((tv), (d))
#define DUK_TVAL_CHKFAST_INPLACE_FAST(tv) \
	do { \
	} while (0)
#define DUK_TVAL_CHKFAST_INPLACE_SLOW(tv) \
	do { \
	} while (0)
#endif /* DUK_USE_FASTINT */

#define DUK_TVAL_SET_FASTINT(tv, i) DUK_TVAL_SET_I48((tv), (i)) /* alias */

#define DUK_TVAL_SET_LIGHTFUNC(tv, fp, flags) DUK__TVAL_SET_LIGHTFUNC((tv), (fp), (flags))
#define DUK_TVAL_SET_STRING(tv, h)            DUK__TVAL_SET_TAGGEDPOINTER((tv), (h), DUK_TAG_STRING)
#define DUK_TVAL_SET_OBJECT(tv, h)            DUK__TVAL_SET_TAGGEDPOINTER((tv), (h), DUK_TAG_OBJECT)
#define DUK_TVAL_SET_BUFFER(tv, h)            DUK__TVAL_SET_TAGGEDPOINTER((tv), (h), DUK_TAG_BUFFER)
#define DUK_TVAL_SET_POINTER(tv, p)           DUK__TVAL_SET_TAGGEDPOINTER((tv), (p), DUK_TAG_POINTER)

#define DUK_TVAL_SET_TVAL(tv, x) \
	do { \
		*(tv) = *(x); \
	} while (0)

/* getters */
#define DUK_TVAL_GET_BOOLEAN(tv) ((duk_small_uint_t) (tv)->us[DUK_DBL_IDX_US1])
#if defined(DUK_USE_FASTINT)
#define DUK_TVAL_GET_DOUBLE(tv)      ((tv)->d)
#define DUK_TVAL_GET_FASTINT(tv)     DUK__TVAL_GET_FASTINT((tv))
#define DUK_TVAL_GET_FASTINT_U32(tv) DUK__TVAL_GET_FASTINT_U32((tv))
#define DUK_TVAL_GET_FASTINT_I32(tv) DUK__TVAL_GET_FASTINT_I32((tv))
#define DUK_TVAL_GET_NUMBER(tv)      duk_tval_get_number_packed((tv))
#else
#define DUK_TVAL_GET_NUMBER(tv) ((tv)->d)
#define DUK_TVAL_GET_DOUBLE(tv) ((tv)->d)
#endif
#define DUK_TVAL_GET_LIGHTFUNC(tv, out_fp, out_flags) \
	do { \
		(out_flags) = (tv)->ui[DUK_DBL_IDX_UI0] & 0xffffUL; \
		(out_fp) = (duk_c_function) (tv)->ui[DUK_DBL_IDX_UI1]; \
	} while (0)
#define DUK_TVAL_GET_LIGHTFUNC_FUNCPTR(tv) ((duk_c_function) ((tv)->ui[DUK_DBL_IDX_UI1]))
#define DUK_TVAL_GET_LIGHTFUNC_FLAGS(tv)   (((duk_small_uint_t) (tv)->ui[DUK_DBL_IDX_UI0]) & 0xffffUL)
#define DUK_TVAL_GET_STRING(tv)            ((duk_hstring *) (tv)->vp[DUK_DBL_IDX_VP1])
#define DUK_TVAL_GET_OBJECT(tv)            ((duk_hobject *) (tv)->vp[DUK_DBL_IDX_VP1])
#define DUK_TVAL_GET_BUFFER(tv)            ((duk_hbuffer *) (tv)->vp[DUK_DBL_IDX_VP1])
#define DUK_TVAL_GET_POINTER(tv)           ((void *) (tv)->vp[DUK_DBL_IDX_VP1])
#define DUK_TVAL_GET_HEAPHDR(tv)           ((duk_heaphdr *) (tv)->vp[DUK_DBL_IDX_VP1])

/* decoding */
#define DUK_TVAL_GET_TAG(tv) ((duk_small_uint_t) (tv)->us[DUK_DBL_IDX_US0])

#define DUK_TVAL_IS_UNDEFINED(tv)     (DUK_TVAL_GET_TAG((tv)) == DUK_TAG_UNDEFINED)
#define DUK_TVAL_IS_UNUSED(tv)        (DUK_TVAL_GET_TAG((tv)) == DUK_TAG_UNUSED)
#define DUK_TVAL_IS_NULL(tv)          (DUK_TVAL_GET_TAG((tv)) == DUK_TAG_NULL)
#define DUK_TVAL_IS_BOOLEAN(tv)       (DUK_TVAL_GET_TAG((tv)) == DUK_TAG_BOOLEAN)
#define DUK_TVAL_IS_BOOLEAN_TRUE(tv)  ((tv)->ui[DUK_DBL_IDX_UI0] == DUK_XTAG_BOOLEAN_TRUE)
#define DUK_TVAL_IS_BOOLEAN_FALSE(tv) ((tv)->ui[DUK_DBL_IDX_UI0] == DUK_XTAG_BOOLEAN_FALSE)
#define DUK_TVAL_IS_LIGHTFUNC(tv)     (DUK_TVAL_GET_TAG((tv)) == DUK_TAG_LIGHTFUNC)
#define DUK_TVAL_IS_STRING(tv)        (DUK_TVAL_GET_TAG((tv)) == DUK_TAG_STRING)
#define DUK_TVAL_IS_OBJECT(tv)        (DUK_TVAL_GET_TAG((tv)) == DUK_TAG_OBJECT)
#define DUK_TVAL_IS_BUFFER(tv)        (DUK_TVAL_GET_TAG((tv)) == DUK_TAG_BUFFER)
#define DUK_TVAL_IS_POINTER(tv)       (DUK_TVAL_GET_TAG((tv)) == DUK_TAG_POINTER)
#if defined(DUK_USE_FASTINT)
/* 0xfff0 is -Infinity */
#define DUK_TVAL_IS_DOUBLE(tv)  (DUK_TVAL_GET_TAG((tv)) <= 0xfff0UL)
#define DUK_TVAL_IS_FASTINT(tv) (DUK_TVAL_GET_TAG((tv)) == DUK_TAG_FASTINT)
#define DUK_TVAL_IS_NUMBER(tv)  (DUK_TVAL_GET_TAG((tv)) <= 0xfff1UL)
#else
#define DUK_TVAL_IS_NUMBER(tv) (DUK_TVAL_GET_TAG((tv)) <= 0xfff0UL)
#define DUK_TVAL_IS_DOUBLE(tv) DUK_TVAL_IS_NUMBER((tv))
#endif

/* This is performance critical because it appears in every DECREF. */
#define DUK_TVAL_IS_HEAP_ALLOCATED(tv) (DUK_TVAL_GET_TAG((tv)) >= DUK_TAG_STRING)

#if defined(DUK_USE_FASTINT)
DUK_INTERNAL_DECL duk_double_t duk_tval_get_number_packed(duk_tval *tv);
#endif

#else /* DUK_USE_PACKED_TVAL */
/* ======================================================================== */

/*
 *  Portable 12-byte representation
 */

/* Note: not initializing all bytes is normally not an issue: Duktape won't
 * read or use the uninitialized bytes so valgrind won't issue warnings.
 * In some special cases a harmless valgrind warning may be issued though.
 * For example, the DumpHeap debugger command writes out a compiled function's
 * 'data' area as is, including any uninitialized bytes, which causes a
 * valgrind warning.
 */

typedef struct duk_tval_struct duk_tval;

struct duk_tval_struct {
	duk_small_uint_t t;
	duk_small_uint_t v_extra;
	union {
		duk_double_t d;
		duk_small_int_t i;
#if defined(DUK_USE_FASTINT)
		duk_int64_t fi; /* if present, forces 16-byte duk_tval */
#endif
		void *voidptr;
		duk_hstring *hstring;
		duk_hobject *hobject;
		duk_hcompfunc *hcompfunc;
		duk_hnatfunc *hnatfunc;
		duk_hthread *hthread;
		duk_hbuffer *hbuffer;
		duk_heaphdr *heaphdr;
		duk_c_function lightfunc;
	} v;
};

typedef struct {
	duk_small_uint_t t;
	duk_small_uint_t v_extra;
	/* The rest of the fields don't matter except for debug dumps and such
	 * for which a partial initializer may trigger out-ot-bounds memory
	 * reads.  Include a double field which is usually as large or larger
	 * than pointers (not always however).
	 */
	duk_double_t d;
} duk_tval_unused;

#define DUK_TVAL_UNUSED_INITIALIZER() \
	{ DUK_TAG_UNUSED, 0, 0.0 }

#define DUK_TAG_MIN    0
#define DUK_TAG_NUMBER 0 /* DUK_TAG_NUMBER only defined for non-packed duk_tval */
#if defined(DUK_USE_FASTINT)
#define DUK_TAG_FASTINT 1
#endif
#define DUK_TAG_UNDEFINED 2
#define DUK_TAG_NULL      3
#define DUK_TAG_BOOLEAN   4
#define DUK_TAG_POINTER   5
#define DUK_TAG_LIGHTFUNC 6
#define DUK_TAG_UNUSED    7 /* marker; not actual tagged type */
#define DUK_TAG_STRING    8 /* first heap allocated, match bit boundary */
#define DUK_TAG_OBJECT    9
#define DUK_TAG_BUFFER    10
#define DUK_TAG_MAX       10

#define DUK_TVAL_IS_VALID_TAG(tv) (DUK_TVAL_GET_TAG((tv)) - DUK_TAG_MIN <= DUK_TAG_MAX - DUK_TAG_MIN)

/* DUK_TAG_NUMBER is intentionally first, as it is the default clause in code
 * to support the 8-byte representation.  Further, it is a non-heap-allocated
 * type so it should come before DUK_TAG_STRING.  Finally, it should not break
 * the tag value ranges covered by case-clauses in a switch-case.
 */

/* setters */
#define DUK_TVAL_SET_UNDEFINED(tv) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->t = DUK_TAG_UNDEFINED; \
	} while (0)

#define DUK_TVAL_SET_UNUSED(tv) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->t = DUK_TAG_UNUSED; \
	} while (0)

#define DUK_TVAL_SET_NULL(tv) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->t = DUK_TAG_NULL; \
	} while (0)

#define DUK_TVAL_SET_BOOLEAN(tv, val) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->t = DUK_TAG_BOOLEAN; \
		duk__tv->v.i = (duk_small_int_t) (val); \
	} while (0)

#if defined(DUK_USE_FASTINT)
#define DUK_TVAL_SET_DOUBLE(tv, val) \
	do { \
		duk_tval *duk__tv; \
		duk_double_t duk__dblval; \
		duk__dblval = (val); \
		DUK_ASSERT_DOUBLE_IS_NORMALIZED(duk__dblval); /* nop for unpacked duk_tval */ \
		duk__tv = (tv); \
		duk__tv->t = DUK_TAG_NUMBER; \
		duk__tv->v.d = duk__dblval; \
	} while (0)
#define DUK_TVAL_SET_I48(tv, val) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->t = DUK_TAG_FASTINT; \
		duk__tv->v.fi = (val); \
	} while (0)
#define DUK_TVAL_SET_U32(tv, val) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->t = DUK_TAG_FASTINT; \
		duk__tv->v.fi = (duk_int64_t) (val); \
	} while (0)
#define DUK_TVAL_SET_I32(tv, val) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->t = DUK_TAG_FASTINT; \
		duk__tv->v.fi = (duk_int64_t) (val); \
	} while (0)
#define DUK_TVAL_SET_NUMBER_CHKFAST_FAST(tv, d) duk_tval_set_number_chkfast_fast((tv), (d))
#define DUK_TVAL_SET_NUMBER_CHKFAST_SLOW(tv, d) duk_tval_set_number_chkfast_slow((tv), (d))
#define DUK_TVAL_SET_NUMBER(tv, val)            DUK_TVAL_SET_DOUBLE((tv), (val))
#define DUK_TVAL_CHKFAST_INPLACE_FAST(tv) \
	do { \
		duk_tval *duk__tv; \
		duk_double_t duk__d; \
		duk__tv = (tv); \
		if (DUK_TVAL_IS_DOUBLE(duk__tv)) { \
			duk__d = DUK_TVAL_GET_DOUBLE(duk__tv); \
			DUK_TVAL_SET_NUMBER_CHKFAST_FAST(duk__tv, duk__d); \
		} \
	} while (0)
#define DUK_TVAL_CHKFAST_INPLACE_SLOW(tv) \
	do { \
		duk_tval *duk__tv; \
		duk_double_t duk__d; \
		duk__tv = (tv); \
		if (DUK_TVAL_IS_DOUBLE(duk__tv)) { \
			duk__d = DUK_TVAL_GET_DOUBLE(duk__tv); \
			DUK_TVAL_SET_NUMBER_CHKFAST_SLOW(duk__tv, duk__d); \
		} \
	} while (0)
#else /* DUK_USE_FASTINT */
#define DUK_TVAL_SET_DOUBLE(tv, d) DUK_TVAL_SET_NUMBER((tv), (d))
#define DUK_TVAL_SET_I48(tv, val)  DUK_TVAL_SET_NUMBER((tv), (duk_double_t) (val)) /* XXX: fast int-to-double */
#define DUK_TVAL_SET_U32(tv, val)  DUK_TVAL_SET_NUMBER((tv), (duk_double_t) (val))
#define DUK_TVAL_SET_I32(tv, val)  DUK_TVAL_SET_NUMBER((tv), (duk_double_t) (val))
#define DUK_TVAL_SET_NUMBER(tv, val) \
	do { \
		duk_tval *duk__tv; \
		duk_double_t duk__dblval; \
		duk__dblval = (val); \
		DUK_ASSERT_DOUBLE_IS_NORMALIZED(duk__dblval); /* nop for unpacked duk_tval */ \
		duk__tv = (tv); \
		duk__tv->t = DUK_TAG_NUMBER; \
		duk__tv->v.d = duk__dblval; \
	} while (0)
#define DUK_TVAL_SET_NUMBER_CHKFAST_FAST(tv, d) DUK_TVAL_SET_NUMBER((tv), (d))
#define DUK_TVAL_SET_NUMBER_CHKFAST_SLOW(tv, d) DUK_TVAL_SET_NUMBER((tv), (d))
#define DUK_TVAL_CHKFAST_INPLACE_FAST(tv) \
	do { \
	} while (0)
#define DUK_TVAL_CHKFAST_INPLACE_SLOW(tv) \
	do { \
	} while (0)
#endif /* DUK_USE_FASTINT */

#define DUK_TVAL_SET_FASTINT(tv, i) DUK_TVAL_SET_I48((tv), (i)) /* alias */

#define DUK_TVAL_SET_POINTER(tv, hptr) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->t = DUK_TAG_POINTER; \
		duk__tv->v.voidptr = (hptr); \
	} while (0)

#define DUK_TVAL_SET_LIGHTFUNC(tv, fp, flags) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->t = DUK_TAG_LIGHTFUNC; \
		duk__tv->v_extra = (flags); \
		duk__tv->v.lightfunc = (duk_c_function) (fp); \
	} while (0)

#define DUK_TVAL_SET_STRING(tv, hptr) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->t = DUK_TAG_STRING; \
		duk__tv->v.hstring = (hptr); \
	} while (0)

#define DUK_TVAL_SET_OBJECT(tv, hptr) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->t = DUK_TAG_OBJECT; \
		duk__tv->v.hobject = (hptr); \
	} while (0)

#define DUK_TVAL_SET_BUFFER(tv, hptr) \
	do { \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->t = DUK_TAG_BUFFER; \
		duk__tv->v.hbuffer = (hptr); \
	} while (0)

#define DUK_TVAL_SET_NAN(tv) \
	do { \
		/* in non-packed representation we don't care about which NaN is used */ \
		duk_tval *duk__tv; \
		duk__tv = (tv); \
		duk__tv->t = DUK_TAG_NUMBER; \
		duk__tv->v.d = DUK_DOUBLE_NAN; \
	} while (0)

#define DUK_TVAL_SET_TVAL(tv, x) \
	do { \
		*(tv) = *(x); \
	} while (0)

/* getters */
#define DUK_TVAL_GET_BOOLEAN(tv) ((duk_small_uint_t) (tv)->v.i)
#if defined(DUK_USE_FASTINT)
#define DUK_TVAL_GET_DOUBLE(tv)      ((tv)->v.d)
#define DUK_TVAL_GET_FASTINT(tv)     ((tv)->v.fi)
#define DUK_TVAL_GET_FASTINT_U32(tv) ((duk_uint32_t) ((tv)->v.fi))
#define DUK_TVAL_GET_FASTINT_I32(tv) ((duk_int32_t) ((tv)->v.fi))
#if 0
#define DUK_TVAL_GET_NUMBER(tv) (DUK_TVAL_IS_FASTINT((tv)) ? (duk_double_t) DUK_TVAL_GET_FASTINT((tv)) : DUK_TVAL_GET_DOUBLE((tv)))
#define DUK_TVAL_GET_NUMBER(tv) duk_tval_get_number_unpacked((tv))
#else
/* This seems reasonable overall. */
#define DUK_TVAL_GET_NUMBER(tv) (DUK_TVAL_IS_FASTINT((tv)) ? duk_tval_get_number_unpacked_fastint((tv)) : DUK_TVAL_GET_DOUBLE((tv)))
#endif
#else
#define DUK_TVAL_GET_NUMBER(tv) ((tv)->v.d)
#define DUK_TVAL_GET_DOUBLE(tv) ((tv)->v.d)
#endif /* DUK_USE_FASTINT */
#define DUK_TVAL_GET_POINTER(tv) ((tv)->v.voidptr)
#define DUK_TVAL_GET_LIGHTFUNC(tv, out_fp, out_flags) \
	do { \
		(out_flags) = (duk_uint32_t) (tv)->v_extra; \
		(out_fp) = (tv)->v.lightfunc; \
	} while (0)
#define DUK_TVAL_GET_LIGHTFUNC_FUNCPTR(tv) ((tv)->v.lightfunc)
#define DUK_TVAL_GET_LIGHTFUNC_FLAGS(tv)   ((duk_small_uint_t) ((tv)->v_extra))
#define DUK_TVAL_GET_STRING(tv)            ((tv)->v.hstring)
#define DUK_TVAL_GET_OBJECT(tv)            ((tv)->v.hobject)
#define DUK_TVAL_GET_BUFFER(tv)            ((tv)->v.hbuffer)
#define DUK_TVAL_GET_HEAPHDR(tv)           ((tv)->v.heaphdr)

/* decoding */
#define DUK_TVAL_GET_TAG(tv)               ((tv)->t)
#define DUK_TVAL_IS_UNDEFINED(tv)          ((tv)->t == DUK_TAG_UNDEFINED)
#define DUK_TVAL_IS_UNUSED(tv)             ((tv)->t == DUK_TAG_UNUSED)
#define DUK_TVAL_IS_NULL(tv)               ((tv)->t == DUK_TAG_NULL)
#define DUK_TVAL_IS_BOOLEAN(tv)            ((tv)->t == DUK_TAG_BOOLEAN)
#define DUK_TVAL_IS_BOOLEAN_TRUE(tv)       (((tv)->t == DUK_TAG_BOOLEAN) && ((tv)->v.i != 0))
#define DUK_TVAL_IS_BOOLEAN_FALSE(tv)      (((tv)->t == DUK_TAG_BOOLEAN) && ((tv)->v.i == 0))
#if defined(DUK_USE_FASTINT)
#define DUK_TVAL_IS_DOUBLE(tv)  ((tv)->t == DUK_TAG_NUMBER)
#define DUK_TVAL_IS_FASTINT(tv) ((tv)->t == DUK_TAG_FASTINT)
#define DUK_TVAL_IS_NUMBER(tv)  ((tv)->t == DUK_TAG_NUMBER || (tv)->t == DUK_TAG_FASTINT)
#else
#define DUK_TVAL_IS_NUMBER(tv) ((tv)->t == DUK_TAG_NUMBER)
#define DUK_TVAL_IS_DOUBLE(tv) DUK_TVAL_IS_NUMBER((tv))
#endif /* DUK_USE_FASTINT */
#define DUK_TVAL_IS_POINTER(tv)   ((tv)->t == DUK_TAG_POINTER)
#define DUK_TVAL_IS_LIGHTFUNC(tv) ((tv)->t == DUK_TAG_LIGHTFUNC)
#define DUK_TVAL_IS_STRING(tv)    ((tv)->t == DUK_TAG_STRING)
#define DUK_TVAL_IS_OBJECT(tv)    ((tv)->t == DUK_TAG_OBJECT)
#define DUK_TVAL_IS_BUFFER(tv)    ((tv)->t == DUK_TAG_BUFFER)

/* This is performance critical because it's needed for every DECREF.
 * Take advantage of the fact that the first heap allocated tag is 8,
 * so that bit 3 is set for all heap allocated tags (and never set for
 * non-heap-allocated tags).
 */
#if 0
#define DUK_TVAL_IS_HEAP_ALLOCATED(tv) ((tv)->t >= DUK_TAG_STRING)
#endif
#define DUK_TVAL_IS_HEAP_ALLOCATED(tv) ((tv)->t & 0x08)

#if defined(DUK_USE_FASTINT)
#if 0
DUK_INTERNAL_DECL duk_double_t duk_tval_get_number_unpacked(duk_tval *tv);
#endif
DUK_INTERNAL_DECL duk_double_t duk_tval_get_number_unpacked_fastint(duk_tval *tv);
#endif

#endif /* DUK_USE_PACKED_TVAL */

/*
 *  Convenience (independent of representation)
 */

#define DUK_TVAL_SET_BOOLEAN_TRUE(tv)  DUK_TVAL_SET_BOOLEAN((tv), 1)
#define DUK_TVAL_SET_BOOLEAN_FALSE(tv) DUK_TVAL_SET_BOOLEAN((tv), 0)

#define DUK_TVAL_STRING_IS_SYMBOL(tv) DUK_HSTRING_HAS_SYMBOL(DUK_TVAL_GET_STRING((tv)))

/* Lightfunc flags packing and unpacking. */
/* Sign extend: 0x0000##00 -> 0x##000000 -> sign extend to 0xssssss##.
 * Avoid signed shifts due to portability limitations.
 */
#define DUK_LFUNC_FLAGS_GET_MAGIC(lf_flags)        ((duk_int32_t) (duk_int8_t) (((duk_uint16_t) (lf_flags)) >> 8))
#define DUK_LFUNC_FLAGS_GET_LENGTH(lf_flags)       (((lf_flags) >> 4) & 0x0fU)
#define DUK_LFUNC_FLAGS_GET_NARGS(lf_flags)        ((lf_flags) &0x0fU)
#define DUK_LFUNC_FLAGS_PACK(magic, length, nargs) ((((duk_small_uint_t) (magic)) & 0xffU) << 8) | ((length) << 4) | (nargs)

#define DUK_LFUNC_NARGS_VARARGS 0x0f /* varargs marker */
#define DUK_LFUNC_NARGS_MIN     0x00
#define DUK_LFUNC_NARGS_MAX     0x0e /* max, excl. varargs marker */
#define DUK_LFUNC_LENGTH_MIN    0x00
#define DUK_LFUNC_LENGTH_MAX    0x0f
#define DUK_LFUNC_MAGIC_MIN     (-0x80)
#define DUK_LFUNC_MAGIC_MAX     0x7f

/* fastint constants etc */
#if defined(DUK_USE_FASTINT)
#define DUK_FASTINT_MIN  (DUK_I64_CONSTANT(-0x800000000000))
#define DUK_FASTINT_MAX  (DUK_I64_CONSTANT(0x7fffffffffff))
#define DUK_FASTINT_BITS 48

DUK_INTERNAL_DECL void duk_tval_set_number_chkfast_fast(duk_tval *tv, duk_double_t x);
DUK_INTERNAL_DECL void duk_tval_set_number_chkfast_slow(duk_tval *tv, duk_double_t x);
#endif

#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL_DECL void duk_tval_assert_valid(duk_tval *tv);
#define DUK_TVAL_ASSERT_VALID(tv) \
	do { \
		duk_tval_assert_valid((tv)); \
	} while (0)
#else
#define DUK_TVAL_ASSERT_VALID(tv) \
	do { \
	} while (0)
#endif

#endif /* DUK_TVAL_H_INCLUDED */
