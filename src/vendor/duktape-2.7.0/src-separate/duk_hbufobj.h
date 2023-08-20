/*
 *  Heap Buffer object representation.  Used for all Buffer variants.
 */

#if !defined(DUK_HBUFOBJ_H_INCLUDED)
#define DUK_HBUFOBJ_H_INCLUDED

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)

/* All element accessors are host endian now (driven by TypedArray spec). */
#define DUK_HBUFOBJ_ELEM_UINT8        0
#define DUK_HBUFOBJ_ELEM_UINT8CLAMPED 1
#define DUK_HBUFOBJ_ELEM_INT8         2
#define DUK_HBUFOBJ_ELEM_UINT16       3
#define DUK_HBUFOBJ_ELEM_INT16        4
#define DUK_HBUFOBJ_ELEM_UINT32       5
#define DUK_HBUFOBJ_ELEM_INT32        6
#define DUK_HBUFOBJ_ELEM_FLOAT32      7
#define DUK_HBUFOBJ_ELEM_FLOAT64      8
#define DUK_HBUFOBJ_ELEM_MAX          8

#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL_DECL void duk_hbufobj_assert_valid(duk_hbufobj *h);
#define DUK_HBUFOBJ_ASSERT_VALID(h) \
	do { \
		duk_hbufobj_assert_valid((h)); \
	} while (0)
#else
#define DUK_HBUFOBJ_ASSERT_VALID(h) \
	do { \
	} while (0)
#endif

/* Get the current data pointer (caller must ensure buf != NULL) as a
 * duk_uint8_t ptr.  Note that the result may be NULL if the underlying
 * buffer has zero size and is not a fixed buffer.
 */
#define DUK_HBUFOBJ_GET_SLICE_BASE(heap, h) \
	(DUK_ASSERT_EXPR((h) != NULL), \
	 DUK_ASSERT_EXPR((h)->buf != NULL), \
	 (((duk_uint8_t *) DUK_HBUFFER_GET_DATA_PTR((heap), (h)->buf)) + (h)->offset))

/* True if slice is full, i.e. offset is zero and length covers the entire
 * buffer.  This status may change independently of the duk_hbufobj if
 * the underlying buffer is dynamic and changes without the hbufobj
 * being changed.
 */
#define DUK_HBUFOBJ_FULL_SLICE(h) \
	(DUK_ASSERT_EXPR((h) != NULL), \
	 DUK_ASSERT_EXPR((h)->buf != NULL), \
	 ((h)->offset == 0 && (h)->length == DUK_HBUFFER_GET_SIZE((h)->buf)))

/* Validate that the whole slice [0,length[ is contained in the underlying
 * buffer.  Caller must ensure 'buf' != NULL.
 */
#define DUK_HBUFOBJ_VALID_SLICE(h) \
	(DUK_ASSERT_EXPR((h) != NULL), \
	 DUK_ASSERT_EXPR((h)->buf != NULL), \
	 ((h)->offset + (h)->length <= DUK_HBUFFER_GET_SIZE((h)->buf)))

/* Validate byte read/write for virtual 'offset', i.e. check that the
 * offset, taking into account h->offset, is within the underlying
 * buffer size.  This is a safety check which is needed to ensure
 * that even a misconfigured duk_hbufobj never causes memory unsafe
 * behavior (e.g. if an underlying dynamic buffer changes after being
 * setup).  Caller must ensure 'buf' != NULL.
 */
#define DUK_HBUFOBJ_VALID_BYTEOFFSET_INCL(h, off) \
	(DUK_ASSERT_EXPR((h) != NULL), DUK_ASSERT_EXPR((h)->buf != NULL), ((h)->offset + (off) < DUK_HBUFFER_GET_SIZE((h)->buf)))

#define DUK_HBUFOBJ_VALID_BYTEOFFSET_EXCL(h, off) \
	(DUK_ASSERT_EXPR((h) != NULL), DUK_ASSERT_EXPR((h)->buf != NULL), ((h)->offset + (off) <= DUK_HBUFFER_GET_SIZE((h)->buf)))

/* Clamp an input byte length (already assumed to be within the nominal
 * duk_hbufobj 'length') to the current dynamic buffer limits to yield
 * a byte length limit that's safe for memory accesses.  This value can
 * be invalidated by any side effect because it may trigger a user
 * callback that resizes the underlying buffer.
 */
#define DUK_HBUFOBJ_CLAMP_BYTELENGTH(h, len) (DUK_ASSERT_EXPR((h) != NULL), duk_hbufobj_clamp_bytelength((h), (len)))

/* Typed arrays have virtual indices, ArrayBuffer and DataView do not. */
#define DUK_HBUFOBJ_HAS_VIRTUAL_INDICES(h) ((h)->is_typedarray)

struct duk_hbufobj {
	/* Shared object part. */
	duk_hobject obj;

	/* Underlying buffer (refcounted), may be NULL. */
	duk_hbuffer *buf;

	/* .buffer reference to an ArrayBuffer, may be NULL. */
	duk_hobject *buf_prop;

	/* Slice and accessor information.
	 *
	 * Because the underlying buffer may be dynamic, these may be
	 * invalidated by the buffer being modified so that both offset
	 * and length should be validated before every access.  Behavior
	 * when the underlying buffer has changed doesn't need to be clean:
	 * virtual 'length' doesn't need to be affected, reads can return
	 * zero/NaN, and writes can be ignored.
	 *
	 * Note that a data pointer cannot be precomputed because 'buf' may
	 * be dynamic and its pointer unstable.
	 */

	duk_uint_t offset; /* byte offset to buf */
	duk_uint_t length; /* byte index limit for element access, exclusive */
	duk_uint8_t shift; /* element size shift:
	                    *   0 = u8/i8
	                    *   1 = u16/i16
	                    *   2 = u32/i32/float
	                    *   3 = double
	                    */
	duk_uint8_t elem_type; /* element type */
	duk_uint8_t is_typedarray;
};

DUK_INTERNAL_DECL duk_uint_t duk_hbufobj_clamp_bytelength(duk_hbufobj *h_bufobj, duk_uint_t len);
DUK_INTERNAL_DECL void duk_hbufobj_push_uint8array_from_plain(duk_hthread *thr, duk_hbuffer *h_buf);
DUK_INTERNAL_DECL void duk_hbufobj_push_validated_read(duk_hthread *thr,
                                                       duk_hbufobj *h_bufobj,
                                                       duk_uint8_t *p,
                                                       duk_small_uint_t elem_size);
DUK_INTERNAL_DECL void duk_hbufobj_validated_write(duk_hthread *thr,
                                                   duk_hbufobj *h_bufobj,
                                                   duk_uint8_t *p,
                                                   duk_small_uint_t elem_size);
DUK_INTERNAL_DECL void duk_hbufobj_promote_plain(duk_hthread *thr, duk_idx_t idx);

#else /* DUK_USE_BUFFEROBJECT_SUPPORT */

/* nothing */

#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */
#endif /* DUK_HBUFOBJ_H_INCLUDED */
