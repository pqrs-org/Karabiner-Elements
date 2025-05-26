/*
 *  ES2015 TypedArray and Node.js Buffer built-ins
 */

#include "duk_internal.h"

/*
 *  Helpers for buffer handling, enabled with DUK_USE_BUFFEROBJECT_SUPPORT.
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
/* Map class number (minus DUK_HOBJECT_CLASS_BUFOBJ_MIN) to a bidx for the
 * default internal prototype.
 */
static const duk_uint8_t duk__buffer_proto_from_classnum[] = {
	DUK_BIDX_ARRAYBUFFER_PROTOTYPE,  DUK_BIDX_DATAVIEW_PROTOTYPE,          DUK_BIDX_INT8ARRAY_PROTOTYPE,
	DUK_BIDX_UINT8ARRAY_PROTOTYPE,   DUK_BIDX_UINT8CLAMPEDARRAY_PROTOTYPE, DUK_BIDX_INT16ARRAY_PROTOTYPE,
	DUK_BIDX_UINT16ARRAY_PROTOTYPE,  DUK_BIDX_INT32ARRAY_PROTOTYPE,        DUK_BIDX_UINT32ARRAY_PROTOTYPE,
	DUK_BIDX_FLOAT32ARRAY_PROTOTYPE, DUK_BIDX_FLOAT64ARRAY_PROTOTYPE
};

/* Map DUK_HBUFOBJ_ELEM_xxx to duk_hobject class number.
 * Sync with duk_hbufobj.h and duk_hobject.h.
 */
static const duk_uint8_t duk__buffer_class_from_elemtype[9] = { DUK_HOBJECT_CLASS_UINT8ARRAY,  DUK_HOBJECT_CLASS_UINT8CLAMPEDARRAY,
	                                                        DUK_HOBJECT_CLASS_INT8ARRAY,   DUK_HOBJECT_CLASS_UINT16ARRAY,
	                                                        DUK_HOBJECT_CLASS_INT16ARRAY,  DUK_HOBJECT_CLASS_UINT32ARRAY,
	                                                        DUK_HOBJECT_CLASS_INT32ARRAY,  DUK_HOBJECT_CLASS_FLOAT32ARRAY,
	                                                        DUK_HOBJECT_CLASS_FLOAT64ARRAY };

/* Map DUK_HBUFOBJ_ELEM_xxx to prototype object built-in index.
 * Sync with duk_hbufobj.h.
 */
static const duk_uint8_t duk__buffer_proto_from_elemtype[9] = {
	DUK_BIDX_UINT8ARRAY_PROTOTYPE,  DUK_BIDX_UINT8CLAMPEDARRAY_PROTOTYPE, DUK_BIDX_INT8ARRAY_PROTOTYPE,
	DUK_BIDX_UINT16ARRAY_PROTOTYPE, DUK_BIDX_INT16ARRAY_PROTOTYPE,        DUK_BIDX_UINT32ARRAY_PROTOTYPE,
	DUK_BIDX_INT32ARRAY_PROTOTYPE,  DUK_BIDX_FLOAT32ARRAY_PROTOTYPE,      DUK_BIDX_FLOAT64ARRAY_PROTOTYPE
};

/* Map DUK__FLD_xxx to byte size. */
static const duk_uint8_t duk__buffer_nbytes_from_fldtype[6] = {
	1, /* DUK__FLD_8BIT */
	2, /* DUK__FLD_16BIT */
	4, /* DUK__FLD_32BIT */
	4, /* DUK__FLD_FLOAT */
	8, /* DUK__FLD_DOUBLE */
	0 /* DUK__FLD_VARINT; not relevant here */
};

/* Bitfield for each DUK_HBUFOBJ_ELEM_xxx indicating which element types
 * are compatible with a blind byte copy for the TypedArray set() method (also
 * used for TypedArray constructor).  Array index is target buffer elem type,
 * bitfield indicates compatible source types.  The types must have same byte
 * size and they must be coercion compatible.
 */
#if !defined(DUK_USE_PREFER_SIZE)
static duk_uint16_t duk__buffer_elemtype_copy_compatible[9] = {
	/* xxx -> DUK_HBUFOBJ_ELEM_UINT8 */
	(1U << DUK_HBUFOBJ_ELEM_UINT8) | (1U << DUK_HBUFOBJ_ELEM_UINT8CLAMPED) | (1U << DUK_HBUFOBJ_ELEM_INT8),

	/* xxx -> DUK_HBUFOBJ_ELEM_UINT8CLAMPED
	 * Note: INT8 is -not- copy compatible, e.g. -1 would coerce to 0x00.
	 */
	(1U << DUK_HBUFOBJ_ELEM_UINT8) | (1U << DUK_HBUFOBJ_ELEM_UINT8CLAMPED),

	/* xxx -> DUK_HBUFOBJ_ELEM_INT8 */
	(1U << DUK_HBUFOBJ_ELEM_UINT8) | (1U << DUK_HBUFOBJ_ELEM_UINT8CLAMPED) | (1U << DUK_HBUFOBJ_ELEM_INT8),

	/* xxx -> DUK_HBUFOBJ_ELEM_UINT16 */
	(1U << DUK_HBUFOBJ_ELEM_UINT16) | (1U << DUK_HBUFOBJ_ELEM_INT16),

	/* xxx -> DUK_HBUFOBJ_ELEM_INT16 */
	(1U << DUK_HBUFOBJ_ELEM_UINT16) | (1U << DUK_HBUFOBJ_ELEM_INT16),

	/* xxx -> DUK_HBUFOBJ_ELEM_UINT32 */
	(1U << DUK_HBUFOBJ_ELEM_UINT32) | (1U << DUK_HBUFOBJ_ELEM_INT32),

	/* xxx -> DUK_HBUFOBJ_ELEM_INT32 */
	(1U << DUK_HBUFOBJ_ELEM_UINT32) | (1U << DUK_HBUFOBJ_ELEM_INT32),

	/* xxx -> DUK_HBUFOBJ_ELEM_FLOAT32 */
	(1U << DUK_HBUFOBJ_ELEM_FLOAT32),

	/* xxx -> DUK_HBUFOBJ_ELEM_FLOAT64 */
	(1U << DUK_HBUFOBJ_ELEM_FLOAT64)
};
#endif /* !DUK_USE_PREFER_SIZE */

DUK_LOCAL duk_hbufobj *duk__hbufobj_promote_this(duk_hthread *thr) {
	duk_tval *tv_dst;
	duk_hbufobj *res;

	duk_push_this(thr);
	DUK_ASSERT(duk_is_buffer(thr, -1));
	res = (duk_hbufobj *) duk_to_hobject(thr, -1);
	DUK_HBUFOBJ_ASSERT_VALID(res);
	DUK_DD(DUK_DDPRINT("promoted 'this' automatically to an ArrayBuffer: %!iT", duk_get_tval(thr, -1)));

	tv_dst = duk_get_borrowed_this_tval(thr);
	DUK_TVAL_SET_OBJECT_UPDREF(thr, tv_dst, (duk_hobject *) res);
	duk_pop(thr);

	return res;
}

#define DUK__BUFOBJ_FLAG_THROW   (1 << 0)
#define DUK__BUFOBJ_FLAG_PROMOTE (1 << 1)

/* Shared helper.  When DUK__BUFOBJ_FLAG_PROMOTE is given, the return value is
 * always a duk_hbufobj *.  Without the flag the return value can also be a
 * plain buffer, and the caller must check for it using DUK_HEAPHDR_IS_BUFFER().
 */
DUK_LOCAL duk_heaphdr *duk__getrequire_bufobj_this(duk_hthread *thr, duk_small_uint_t flags) {
	duk_tval *tv;
	duk_hbufobj *h_this;

	DUK_ASSERT(thr != NULL);

	tv = duk_get_borrowed_this_tval(thr);
	DUK_ASSERT(tv != NULL);

	if (DUK_TVAL_IS_OBJECT(tv)) {
		h_this = (duk_hbufobj *) DUK_TVAL_GET_OBJECT(tv);
		DUK_ASSERT(h_this != NULL);
		if (DUK_HOBJECT_IS_BUFOBJ((duk_hobject *) h_this)) {
			DUK_HBUFOBJ_ASSERT_VALID(h_this);
			return (duk_heaphdr *) h_this;
		}
	} else if (DUK_TVAL_IS_BUFFER(tv)) {
		if (flags & DUK__BUFOBJ_FLAG_PROMOTE) {
			/* Promote a plain buffer to a Uint8Array.  This is very
			 * inefficient but allows plain buffer to be used wherever an
			 * Uint8Array is used with very small cost; hot path functions
			 * like index read/write calls should provide direct buffer
			 * support to avoid promotion.
			 */
			/* XXX: make this conditional to a flag if call sites need it? */
			h_this = duk__hbufobj_promote_this(thr);
			DUK_ASSERT(h_this != NULL);
			DUK_HBUFOBJ_ASSERT_VALID(h_this);
			return (duk_heaphdr *) h_this;
		} else {
			/* XXX: ugly, share return pointer for duk_hbuffer. */
			return (duk_heaphdr *) DUK_TVAL_GET_BUFFER(tv);
		}
	}

	if (flags & DUK__BUFOBJ_FLAG_THROW) {
		DUK_ERROR_TYPE(thr, DUK_STR_NOT_BUFFER);
		DUK_WO_NORETURN(return NULL;);
	}
	return NULL;
}

/* Check that 'this' is a duk_hbufobj and return a pointer to it. */
DUK_LOCAL duk_hbufobj *duk__get_bufobj_this(duk_hthread *thr) {
	return (duk_hbufobj *) duk__getrequire_bufobj_this(thr, DUK__BUFOBJ_FLAG_PROMOTE);
}

/* Check that 'this' is a duk_hbufobj and return a pointer to it
 * (NULL if not).
 */
DUK_LOCAL duk_hbufobj *duk__require_bufobj_this(duk_hthread *thr) {
	return (duk_hbufobj *) duk__getrequire_bufobj_this(thr, DUK__BUFOBJ_FLAG_THROW | DUK__BUFOBJ_FLAG_PROMOTE);
}

/* Check that value is a duk_hbufobj and return a pointer to it. */
DUK_LOCAL duk_hbufobj *duk__require_bufobj_value(duk_hthread *thr, duk_idx_t idx) {
	duk_tval *tv;
	duk_hbufobj *h_obj;

	/* Don't accept relative indices now. */
	DUK_ASSERT(idx >= 0);

	tv = duk_require_tval(thr, idx);
	DUK_ASSERT(tv != NULL);
	if (DUK_TVAL_IS_OBJECT(tv)) {
		h_obj = (duk_hbufobj *) DUK_TVAL_GET_OBJECT(tv);
		DUK_ASSERT(h_obj != NULL);
		if (DUK_HOBJECT_IS_BUFOBJ((duk_hobject *) h_obj)) {
			DUK_HBUFOBJ_ASSERT_VALID(h_obj);
			return h_obj;
		}
	} else if (DUK_TVAL_IS_BUFFER(tv)) {
		h_obj = (duk_hbufobj *) duk_to_hobject(thr, idx);
		DUK_ASSERT(h_obj != NULL);
		DUK_HBUFOBJ_ASSERT_VALID(h_obj);
		return h_obj;
	}

	DUK_ERROR_TYPE(thr, DUK_STR_NOT_BUFFER);
	DUK_WO_NORETURN(return NULL;);
}

DUK_LOCAL void duk__set_bufobj_buffer(duk_hthread *thr, duk_hbufobj *h_bufobj, duk_hbuffer *h_val) {
	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(h_bufobj != NULL);
	DUK_ASSERT(h_bufobj->buf == NULL); /* no need to decref */
	DUK_ASSERT(h_val != NULL);
	DUK_HBUFOBJ_ASSERT_VALID(h_bufobj);
	DUK_UNREF(thr);

	h_bufobj->buf = h_val;
	DUK_HBUFFER_INCREF(thr, h_val);
	h_bufobj->length = (duk_uint_t) DUK_HBUFFER_GET_SIZE(h_val);
	DUK_ASSERT(h_bufobj->shift == 0);
	DUK_ASSERT(h_bufobj->elem_type == DUK_HBUFOBJ_ELEM_UINT8);
	DUK_ASSERT(h_bufobj->is_typedarray == 0);

	DUK_HBUFOBJ_ASSERT_VALID(h_bufobj);
}

/* Shared offset/length coercion helper. */
DUK_LOCAL void duk__resolve_offset_opt_length(duk_hthread *thr,
                                              duk_hbufobj *h_bufarg,
                                              duk_idx_t idx_offset,
                                              duk_idx_t idx_length,
                                              duk_uint_t *out_offset,
                                              duk_uint_t *out_length,
                                              duk_bool_t throw_flag) {
	duk_int_t offset_signed;
	duk_int_t length_signed;
	duk_uint_t offset;
	duk_uint_t length;

	offset_signed = duk_to_int(thr, idx_offset);
	if (offset_signed < 0) {
		goto fail_range;
	}
	offset = (duk_uint_t) offset_signed;
	if (offset > h_bufarg->length) {
		goto fail_range;
	}
	DUK_ASSERT_DISABLE(offset >= 0); /* unsigned */
	DUK_ASSERT(offset <= h_bufarg->length);

	if (duk_is_undefined(thr, idx_length)) {
		DUK_ASSERT(h_bufarg->length >= offset);
		length = h_bufarg->length - offset; /* >= 0 */
	} else {
		length_signed = duk_to_int(thr, idx_length);
		if (length_signed < 0) {
			goto fail_range;
		}
		length = (duk_uint_t) length_signed;
		DUK_ASSERT(h_bufarg->length >= offset);
		if (length > h_bufarg->length - offset) {
			/* Unlike for negative arguments, some call sites
			 * want length to be clamped if it's positive.
			 */
			if (throw_flag) {
				goto fail_range;
			} else {
				length = h_bufarg->length - offset;
			}
		}
	}
	DUK_ASSERT_DISABLE(length >= 0); /* unsigned */
	DUK_ASSERT(offset + length <= h_bufarg->length);

	*out_offset = offset;
	*out_length = length;
	return;

fail_range:
	DUK_ERROR_RANGE(thr, DUK_STR_INVALID_ARGS);
	DUK_WO_NORETURN(return;);
}

/* Shared lenient buffer length clamping helper.  No negative indices, no
 * element/byte shifting.
 */
DUK_LOCAL void duk__clamp_startend_nonegidx_noshift(duk_hthread *thr,
                                                    duk_int_t buffer_length,
                                                    duk_idx_t idx_start,
                                                    duk_idx_t idx_end,
                                                    duk_int_t *out_start_offset,
                                                    duk_int_t *out_end_offset) {
	duk_int_t start_offset;
	duk_int_t end_offset;

	DUK_ASSERT(out_start_offset != NULL);
	DUK_ASSERT(out_end_offset != NULL);

	/* undefined coerces to zero which is correct */
	start_offset = duk_to_int_clamped(thr, idx_start, 0, buffer_length);
	if (duk_is_undefined(thr, idx_end)) {
		end_offset = buffer_length;
	} else {
		end_offset = duk_to_int_clamped(thr, idx_end, start_offset, buffer_length);
	}

	DUK_ASSERT(start_offset >= 0);
	DUK_ASSERT(start_offset <= buffer_length);
	DUK_ASSERT(end_offset >= 0);
	DUK_ASSERT(end_offset <= buffer_length);
	DUK_ASSERT(start_offset <= end_offset);

	*out_start_offset = start_offset;
	*out_end_offset = end_offset;
}

/* Shared lenient buffer length clamping helper.  Indices are treated as
 * element indices (though output values are byte offsets) which only
 * really matters for TypedArray views as other buffer object have a zero
 * shift.  Negative indices are counted from end of input slice; crossed
 * indices are clamped to zero length; and final indices are clamped
 * against input slice.  Used for e.g. ArrayBuffer slice().
 */
DUK_LOCAL void duk__clamp_startend_negidx_shifted(duk_hthread *thr,
                                                  duk_int_t buffer_length,
                                                  duk_uint8_t buffer_shift,
                                                  duk_idx_t idx_start,
                                                  duk_idx_t idx_end,
                                                  duk_int_t *out_start_offset,
                                                  duk_int_t *out_end_offset) {
	duk_int_t start_offset;
	duk_int_t end_offset;

	DUK_ASSERT(out_start_offset != NULL);
	DUK_ASSERT(out_end_offset != NULL);

	buffer_length >>= buffer_shift; /* as (full) elements */

	/* Resolve start/end offset as element indices first; arguments
	 * at idx_start/idx_end are element offsets.  Working with element
	 * indices first also avoids potential for wrapping.
	 */

	start_offset = duk_to_int(thr, idx_start);
	if (start_offset < 0) {
		start_offset = buffer_length + start_offset;
	}
	if (duk_is_undefined(thr, idx_end)) {
		end_offset = buffer_length;
	} else {
		end_offset = duk_to_int(thr, idx_end);
		if (end_offset < 0) {
			end_offset = buffer_length + end_offset;
		}
	}
	/* Note: start_offset/end_offset can still be < 0 here. */

	if (start_offset < 0) {
		start_offset = 0;
	} else if (start_offset > buffer_length) {
		start_offset = buffer_length;
	}
	if (end_offset < start_offset) {
		end_offset = start_offset;
	} else if (end_offset > buffer_length) {
		end_offset = buffer_length;
	}
	DUK_ASSERT(start_offset >= 0);
	DUK_ASSERT(start_offset <= buffer_length);
	DUK_ASSERT(end_offset >= 0);
	DUK_ASSERT(end_offset <= buffer_length);
	DUK_ASSERT(start_offset <= end_offset);

	/* Convert indices to byte offsets. */
	start_offset <<= buffer_shift;
	end_offset <<= buffer_shift;

	*out_start_offset = start_offset;
	*out_end_offset = end_offset;
}

DUK_INTERNAL void duk_hbufobj_promote_plain(duk_hthread *thr, duk_idx_t idx) {
	if (duk_is_buffer(thr, idx)) {
		duk_to_object(thr, idx);
	}
}

DUK_INTERNAL void duk_hbufobj_push_uint8array_from_plain(duk_hthread *thr, duk_hbuffer *h_buf) {
	/* Push Uint8Array which will share the same underlying buffer as
	 * the plain buffer argument.  Also create an ArrayBuffer with the
	 * same backing for the result .buffer property.
	 */

	duk_push_hbuffer(thr, h_buf);
	duk_push_buffer_object(thr, -1, 0, (duk_size_t) DUK_HBUFFER_GET_SIZE(h_buf), DUK_BUFOBJ_UINT8ARRAY);
	duk_remove_m2(thr);

#if 0
	/* More verbose equivalent; maybe useful if e.g. .buffer is omitted. */
	h_bufobj = duk_push_bufobj_raw(thr,
	                               DUK_HOBJECT_FLAG_EXTENSIBLE |
	                               DUK_HOBJECT_FLAG_BUFOBJ |
	                               DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_UINT8ARRAY),
	                               DUK_BIDX_UINT8ARRAY_PROTOTYPE);
	DUK_ASSERT(h_bufobj != NULL);
	duk__set_bufobj_buffer(thr, h_bufobj, h_buf);
	h_bufobj->is_typedarray = 1;
	DUK_HBUFOBJ_ASSERT_VALID(h_bufobj);

	h_arrbuf = duk_push_bufobj_raw(thr,
	                               DUK_HOBJECT_FLAG_EXTENSIBLE |
	                               DUK_HOBJECT_FLAG_BUFOBJ |
	                               DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_ARRAYBUFFER),
	                               DUK_BIDX_ARRAYBUFFER_PROTOTYPE);
	DUK_ASSERT(h_arrbuf != NULL);
	duk__set_bufobj_buffer(thr, h_arrbuf, h_buf);
	DUK_ASSERT(h_arrbuf->is_typedarray == 0);
	DUK_HBUFOBJ_ASSERT_VALID(h_arrbuf);

	DUK_ASSERT(h_bufobj->buf_prop == NULL);
	h_bufobj->buf_prop = (duk_hobject *) h_arrbuf;
	DUK_ASSERT(h_arrbuf != NULL);
	DUK_HBUFOBJ_INCREF(thr, h_arrbuf);
	duk_pop(thr);
#endif
}

/* Indexed read helper for buffer objects, also called from outside this file. */
DUK_INTERNAL void duk_hbufobj_push_validated_read(duk_hthread *thr,
                                                  duk_hbufobj *h_bufobj,
                                                  duk_uint8_t *p,
                                                  duk_small_uint_t elem_size) {
	duk_double_union du;

	DUK_ASSERT(elem_size > 0);
	duk_memcpy((void *) du.uc, (const void *) p, (size_t) elem_size);

	switch (h_bufobj->elem_type) {
	case DUK_HBUFOBJ_ELEM_UINT8:
	case DUK_HBUFOBJ_ELEM_UINT8CLAMPED:
		duk_push_uint(thr, (duk_uint_t) du.uc[0]);
		break;
	case DUK_HBUFOBJ_ELEM_INT8:
		duk_push_int(thr, (duk_int_t) (duk_int8_t) du.uc[0]);
		break;
	case DUK_HBUFOBJ_ELEM_UINT16:
		duk_push_uint(thr, (duk_uint_t) du.us[0]);
		break;
	case DUK_HBUFOBJ_ELEM_INT16:
		duk_push_int(thr, (duk_int_t) (duk_int16_t) du.us[0]);
		break;
	case DUK_HBUFOBJ_ELEM_UINT32:
		duk_push_uint(thr, (duk_uint_t) du.ui[0]);
		break;
	case DUK_HBUFOBJ_ELEM_INT32:
		duk_push_int(thr, (duk_int_t) (duk_int32_t) du.ui[0]);
		break;
	case DUK_HBUFOBJ_ELEM_FLOAT32:
		duk_push_number(thr, (duk_double_t) du.f[0]);
		break;
	case DUK_HBUFOBJ_ELEM_FLOAT64:
		duk_push_number(thr, (duk_double_t) du.d);
		break;
	default:
		DUK_UNREACHABLE();
	}
}

/* Indexed write helper for buffer objects, also called from outside this file. */
DUK_INTERNAL void duk_hbufobj_validated_write(duk_hthread *thr, duk_hbufobj *h_bufobj, duk_uint8_t *p, duk_small_uint_t elem_size) {
	duk_double_union du;

	/* NOTE! Caller must ensure that any side effects from the
	 * coercions below are safe.  If that cannot be guaranteed
	 * (which is normally the case), caller must coerce the
	 * argument using duk_to_number() before any pointer
	 * validations; the result of duk_to_number() always coerces
	 * without side effects here.
	 */

	switch (h_bufobj->elem_type) {
	case DUK_HBUFOBJ_ELEM_UINT8:
		du.uc[0] = (duk_uint8_t) duk_to_uint32(thr, -1);
		break;
	case DUK_HBUFOBJ_ELEM_UINT8CLAMPED:
		du.uc[0] = (duk_uint8_t) duk_to_uint8clamped(thr, -1);
		break;
	case DUK_HBUFOBJ_ELEM_INT8:
		du.uc[0] = (duk_uint8_t) duk_to_int32(thr, -1);
		break;
	case DUK_HBUFOBJ_ELEM_UINT16:
		du.us[0] = (duk_uint16_t) duk_to_uint32(thr, -1);
		break;
	case DUK_HBUFOBJ_ELEM_INT16:
		du.us[0] = (duk_uint16_t) duk_to_int32(thr, -1);
		break;
	case DUK_HBUFOBJ_ELEM_UINT32:
		du.ui[0] = (duk_uint32_t) duk_to_uint32(thr, -1);
		break;
	case DUK_HBUFOBJ_ELEM_INT32:
		du.ui[0] = (duk_uint32_t) duk_to_int32(thr, -1);
		break;
	case DUK_HBUFOBJ_ELEM_FLOAT32:
		/* A double-to-float cast is undefined behavior in C99 if
		 * the cast is out-of-range, so use a helper.  Example:
		 * runtime error: value -1e+100 is outside the range of representable values of type 'float'
		 */
		du.f[0] = duk_double_to_float_t(duk_to_number_m1(thr));
		break;
	case DUK_HBUFOBJ_ELEM_FLOAT64:
		du.d = (duk_double_t) duk_to_number_m1(thr);
		break;
	default:
		DUK_UNREACHABLE();
	}

	DUK_ASSERT(elem_size > 0);
	duk_memcpy((void *) p, (const void *) du.uc, (size_t) elem_size);
}

/* Helper to create a fixed buffer from argument value at index 0.
 * Node.js and allocPlain() compatible.
 */
DUK_LOCAL duk_hbuffer *duk__hbufobj_fixed_from_argvalue(duk_hthread *thr) {
	duk_int_t len;
	duk_int_t i;
	duk_size_t buf_size;
	duk_uint8_t *buf;

	switch (duk_get_type(thr, 0)) {
	case DUK_TYPE_NUMBER: {
		len = duk_to_int_clamped(thr, 0, 0, DUK_INT_MAX);
		(void) duk_push_fixed_buffer_zero(thr, (duk_size_t) len);
		break;
	}
	case DUK_TYPE_BUFFER: { /* Treat like Uint8Array. */
		goto slow_copy;
	}
	case DUK_TYPE_OBJECT: {
		duk_hobject *h;
		duk_hbufobj *h_bufobj;

		/* For Node.js Buffers "Passing an ArrayBuffer returns a Buffer
		 * that shares allocated memory with the given ArrayBuffer."
		 * https://nodejs.org/api/buffer.html#buffer_buffer_from_buffer_alloc_and_buffer_allocunsafe
		 */

		h = duk_known_hobject(thr, 0);
		if (DUK_HOBJECT_GET_CLASS_NUMBER(h) == DUK_HOBJECT_CLASS_ARRAYBUFFER) {
			DUK_ASSERT(DUK_HOBJECT_IS_BUFOBJ(h));
			h_bufobj = (duk_hbufobj *) h;
			if (DUK_UNLIKELY(h_bufobj->buf == NULL)) {
				DUK_ERROR_TYPE_INVALID_ARGS(thr);
				DUK_WO_NORETURN(return NULL;);
			}
			if (DUK_UNLIKELY(h_bufobj->offset != 0 || h_bufobj->length != DUK_HBUFFER_GET_SIZE(h_bufobj->buf))) {
				/* No support for ArrayBuffers with slice
				 * offset/length.
				 */
				DUK_ERROR_TYPE_INVALID_ARGS(thr);
				DUK_WO_NORETURN(return NULL;);
			}
			duk_push_hbuffer(thr, h_bufobj->buf);
			return h_bufobj->buf;
		}
		goto slow_copy;
	}
	case DUK_TYPE_STRING: {
		/* ignore encoding for now */
		duk_require_hstring_notsymbol(thr, 0);
		duk_dup_0(thr);
		(void) duk_to_buffer(thr, -1, &buf_size);
		break;
	}
	default:
		DUK_ERROR_TYPE_INVALID_ARGS(thr);
		DUK_WO_NORETURN(return NULL;);
	}

done:
	DUK_ASSERT(duk_is_buffer(thr, -1));
	return duk_known_hbuffer(thr, -1);

slow_copy:
	/* XXX: fast path for typed arrays and other buffer objects? */

	(void) duk_get_prop_stridx_short(thr, 0, DUK_STRIDX_LENGTH);
	len = duk_to_int_clamped(thr, -1, 0, DUK_INT_MAX);
	duk_pop(thr);
	buf = (duk_uint8_t *) duk_push_fixed_buffer_nozero(thr, (duk_size_t) len); /* no zeroing, all indices get initialized */
	for (i = 0; i < len; i++) {
		/* XXX: fast path for array or buffer arguments? */
		duk_get_prop_index(thr, 0, (duk_uarridx_t) i);
		buf[i] = (duk_uint8_t) (duk_to_uint32(thr, -1) & 0xffU);
		duk_pop(thr);
	}
	goto done;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Node.js Buffer constructor
 *
 *  Node.js Buffers are just Uint8Arrays with internal prototype set to
 *  Buffer.prototype so they're handled otherwise the same as Uint8Array.
 *  However, the constructor arguments are very different so a separate
 *  constructor entry point is used.
 */
#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_nodejs_buffer_constructor(duk_hthread *thr) {
	duk_hbuffer *h_buf;

	h_buf = duk__hbufobj_fixed_from_argvalue(thr);
	DUK_ASSERT(h_buf != NULL);

	duk_push_buffer_object(thr, -1, 0, DUK_HBUFFER_FIXED_GET_SIZE((duk_hbuffer_fixed *) (void *) h_buf), DUK_BUFOBJ_UINT8ARRAY);
	duk_push_hobject_bidx(thr, DUK_BIDX_NODEJS_BUFFER_PROTOTYPE);
	duk_set_prototype(thr, -2);

	/* XXX: a more direct implementation */

	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  ArrayBuffer, DataView, and TypedArray constructors
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_arraybuffer_constructor(duk_hthread *thr) {
	duk_hbufobj *h_bufobj;
	duk_hbuffer *h_val;
	duk_int_t len;

	DUK_CTX_ASSERT_VALID(thr);

	duk_require_constructor_call(thr);

	len = duk_to_int(thr, 0);
	if (len < 0) {
		goto fail_length;
	}
	(void) duk_push_fixed_buffer_zero(thr, (duk_size_t) len);
	h_val = (duk_hbuffer *) duk_known_hbuffer(thr, -1);

	h_bufobj = duk_push_bufobj_raw(thr,
	                               DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_BUFOBJ |
	                                   DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_ARRAYBUFFER),
	                               DUK_BIDX_ARRAYBUFFER_PROTOTYPE);
	DUK_ASSERT(h_bufobj != NULL);

	duk__set_bufobj_buffer(thr, h_bufobj, h_val);
	DUK_HBUFOBJ_ASSERT_VALID(h_bufobj);

	return 1;

fail_length:
	DUK_DCERROR_RANGE_INVALID_LENGTH(thr);
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/* Format of magic, bits:
 *   0...1: elem size shift (0-3)
 *   2...5: elem type (DUK_HBUFOBJ_ELEM_xxx)
 *
 * XXX: add prototype bidx explicitly to magic instead of using a mapping?
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_typedarray_constructor(duk_hthread *thr) {
	duk_tval *tv;
	duk_hobject *h_obj;
	duk_hbufobj *h_bufobj = NULL;
	duk_hbufobj *h_bufarg = NULL;
	duk_hbuffer *h_val;
	duk_small_uint_t magic;
	duk_small_uint_t shift;
	duk_small_uint_t elem_type;
	duk_small_uint_t elem_size;
	duk_small_uint_t class_num;
	duk_small_uint_t proto_bidx;
	duk_uint_t align_mask;
	duk_uint_t elem_length;
	duk_int_t elem_length_signed;
	duk_uint_t byte_length;
	duk_small_uint_t copy_mode;

	/* XXX: The same copy helpers could be shared with at least some
	 * buffer functions.
	 */

	duk_require_constructor_call(thr);

	/* We could fit built-in index into magic but that'd make the magic
	 * number dependent on built-in numbering (genbuiltins.py doesn't
	 * handle that yet).  So map both class and prototype from the
	 * element type.
	 */
	magic = (duk_small_uint_t) duk_get_current_magic(thr);
	shift = magic & 0x03U; /* bits 0...1: shift */
	elem_type = (magic >> 2) & 0x0fU; /* bits 2...5: type */
	elem_size = 1U << shift;
	align_mask = elem_size - 1;
	DUK_ASSERT(elem_type < sizeof(duk__buffer_proto_from_elemtype) / sizeof(duk_uint8_t));
	proto_bidx = duk__buffer_proto_from_elemtype[elem_type];
	DUK_ASSERT(proto_bidx < DUK_NUM_BUILTINS);
	DUK_ASSERT(elem_type < sizeof(duk__buffer_class_from_elemtype) / sizeof(duk_uint8_t));
	class_num = duk__buffer_class_from_elemtype[elem_type];

	DUK_DD(DUK_DDPRINT("typedarray constructor, magic=%d, shift=%d, elem_type=%d, "
	                   "elem_size=%d, proto_bidx=%d, class_num=%d",
	                   (int) magic,
	                   (int) shift,
	                   (int) elem_type,
	                   (int) elem_size,
	                   (int) proto_bidx,
	                   (int) class_num));

	/* Argument variants.  When the argument is an ArrayBuffer a view to
	 * the same buffer is created; otherwise a new ArrayBuffer is always
	 * created.
	 */

	/* XXX: initial iteration to treat a plain buffer like an ArrayBuffer:
	 * coerce to an ArrayBuffer object and use that as .buffer.  The underlying
	 * buffer will be the same but result .buffer !== inputPlainBuffer.
	 */
	duk_hbufobj_promote_plain(thr, 0);

	tv = duk_get_tval(thr, 0);
	DUK_ASSERT(tv != NULL); /* arg count */
	if (DUK_TVAL_IS_OBJECT(tv)) {
		h_obj = DUK_TVAL_GET_OBJECT(tv);
		DUK_ASSERT(h_obj != NULL);

		if (DUK_HOBJECT_GET_CLASS_NUMBER(h_obj) == DUK_HOBJECT_CLASS_ARRAYBUFFER) {
			/* ArrayBuffer: unlike any other argument variant, create
			 * a view into the existing buffer.
			 */

			duk_int_t byte_offset_signed;
			duk_uint_t byte_offset;

			h_bufarg = (duk_hbufobj *) h_obj;

			byte_offset_signed = duk_to_int(thr, 1);
			if (byte_offset_signed < 0) {
				goto fail_arguments;
			}
			byte_offset = (duk_uint_t) byte_offset_signed;
			if (byte_offset > h_bufarg->length || (byte_offset & align_mask) != 0) {
				/* Must be >= 0 and multiple of element size. */
				goto fail_arguments;
			}
			if (duk_is_undefined(thr, 2)) {
				DUK_ASSERT(h_bufarg->length >= byte_offset);
				byte_length = h_bufarg->length - byte_offset;
				if ((byte_length & align_mask) != 0) {
					/* Must be element size multiple from
					 * start offset to end of buffer.
					 */
					goto fail_arguments;
				}
				elem_length = (byte_length >> shift);
			} else {
				elem_length_signed = duk_to_int(thr, 2);
				if (elem_length_signed < 0) {
					goto fail_arguments;
				}
				elem_length = (duk_uint_t) elem_length_signed;
				byte_length = elem_length << shift;
				if ((byte_length >> shift) != elem_length) {
					/* Byte length would overflow. */
					/* XXX: easier check with less code? */
					goto fail_arguments;
				}
				DUK_ASSERT(h_bufarg->length >= byte_offset);
				if (byte_length > h_bufarg->length - byte_offset) {
					/* Not enough data. */
					goto fail_arguments;
				}
			}
			DUK_UNREF(elem_length);
			DUK_ASSERT_DISABLE(byte_offset >= 0);
			DUK_ASSERT(byte_offset <= h_bufarg->length);
			DUK_ASSERT_DISABLE(byte_length >= 0);
			DUK_ASSERT(byte_offset + byte_length <= h_bufarg->length);
			DUK_ASSERT((elem_length << shift) == byte_length);

			h_bufobj = duk_push_bufobj_raw(thr,
			                               DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_BUFOBJ |
			                                   DUK_HOBJECT_CLASS_AS_FLAGS(class_num),
			                               (duk_small_int_t) proto_bidx);
			h_val = h_bufarg->buf;
			if (h_val == NULL) {
				DUK_DCERROR_TYPE_INVALID_ARGS(thr);
			}
			h_bufobj->buf = h_val;
			DUK_HBUFFER_INCREF(thr, h_val);
			h_bufobj->offset = h_bufarg->offset + byte_offset;
			h_bufobj->length = byte_length;
			h_bufobj->shift = (duk_uint8_t) shift;
			h_bufobj->elem_type = (duk_uint8_t) elem_type;
			h_bufobj->is_typedarray = 1;
			DUK_HBUFOBJ_ASSERT_VALID(h_bufobj);

			/* Set .buffer to the argument ArrayBuffer. */
			DUK_ASSERT(h_bufobj->buf_prop == NULL);
			h_bufobj->buf_prop = (duk_hobject *) h_bufarg;
			DUK_ASSERT(h_bufarg != NULL);
			DUK_HBUFOBJ_INCREF(thr, h_bufarg);
			return 1;
		} else if (DUK_HOBJECT_IS_BUFOBJ(h_obj)) {
			/* TypedArray (or other non-ArrayBuffer duk_hbufobj).
			 * Conceptually same behavior as for an Array-like argument,
			 * with a few fast paths.
			 */

			h_bufarg = (duk_hbufobj *) h_obj;
			DUK_HBUFOBJ_ASSERT_VALID(h_bufarg);
			elem_length_signed = (duk_int_t) (h_bufarg->length >> h_bufarg->shift);
			if (h_bufarg->buf == NULL) {
				DUK_DCERROR_TYPE_INVALID_ARGS(thr);
			}

			/* Select copy mode.  Must take into account element
			 * compatibility and validity of the underlying source
			 * buffer.
			 */

			DUK_DDD(DUK_DDDPRINT("selecting copy mode for bufobj arg, "
			                     "src byte_length=%ld, src shift=%d, "
			                     "src/dst elem_length=%ld; "
			                     "dst shift=%d -> dst byte_length=%ld",
			                     (long) h_bufarg->length,
			                     (int) h_bufarg->shift,
			                     (long) elem_length_signed,
			                     (int) shift,
			                     (long) (elem_length_signed << shift)));

			copy_mode = 2; /* default is explicit index read/write copy */
#if !defined(DUK_USE_PREFER_SIZE)
			/* With a size optimized build copy_mode 2 is enough.
			 * Modes 0 and 1 are faster but conceptually the same.
			 */
			DUK_ASSERT(elem_type < sizeof(duk__buffer_elemtype_copy_compatible) / sizeof(duk_uint16_t));
			if (DUK_HBUFOBJ_VALID_SLICE(h_bufarg)) {
				if ((duk__buffer_elemtype_copy_compatible[elem_type] & (1 << h_bufarg->elem_type)) != 0) {
					DUK_DDD(DUK_DDDPRINT("source/target are copy compatible, memcpy"));
					DUK_ASSERT(shift == h_bufarg->shift); /* byte sizes will match */
					copy_mode = 0;
				} else {
					DUK_DDD(DUK_DDDPRINT("source/target not copy compatible but valid, fast copy"));
					copy_mode = 1;
				}
			}
#endif /* !DUK_USE_PREFER_SIZE */
		} else {
			/* Array or Array-like */
			elem_length_signed = (duk_int_t) duk_get_length(thr, 0);
			copy_mode = 2;
		}
	} else {
		/* Non-object argument is simply int coerced, matches
		 * V8 behavior (except for "null", which we coerce to
		 * 0 but V8 TypeErrors).
		 */
		elem_length_signed = duk_to_int(thr, 0);
		copy_mode = 3;
	}
	if (elem_length_signed < 0) {
		goto fail_arguments;
	}
	elem_length = (duk_uint_t) elem_length_signed;
	byte_length = (duk_uint_t) (elem_length << shift);
	if ((byte_length >> shift) != elem_length) {
		/* Byte length would overflow. */
		/* XXX: easier check with less code? */
		goto fail_arguments;
	}

	DUK_DDD(DUK_DDDPRINT("elem_length=%ld, byte_length=%ld", (long) elem_length, (long) byte_length));

	/* ArrayBuffer argument is handled specially above; the rest of the
	 * argument variants are handled by shared code below.
	 *
	 * ArrayBuffer in h_bufobj->buf_prop is intentionally left unset.
	 * It will be automatically created by the .buffer accessor on
	 * first access.
	 */

	/* Push the resulting view object on top of a plain fixed buffer. */
	(void) duk_push_fixed_buffer(thr, byte_length);
	h_val = duk_known_hbuffer(thr, -1);
	DUK_ASSERT(h_val != NULL);

	h_bufobj =
	    duk_push_bufobj_raw(thr,
	                        DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_BUFOBJ | DUK_HOBJECT_CLASS_AS_FLAGS(class_num),
	                        (duk_small_int_t) proto_bidx);

	h_bufobj->buf = h_val;
	DUK_HBUFFER_INCREF(thr, h_val);
	DUK_ASSERT(h_bufobj->offset == 0);
	h_bufobj->length = byte_length;
	h_bufobj->shift = (duk_uint8_t) shift;
	h_bufobj->elem_type = (duk_uint8_t) elem_type;
	h_bufobj->is_typedarray = 1;
	DUK_HBUFOBJ_ASSERT_VALID(h_bufobj);

	/* Copy values, the copy method depends on the arguments.
	 *
	 * Copy mode decision may depend on the validity of the underlying
	 * buffer of the source argument; there must be no harmful side effects
	 * from there to here for copy_mode to still be valid.
	 */
	DUK_DDD(DUK_DDDPRINT("copy mode: %d", (int) copy_mode));
	switch (copy_mode) {
		/* Copy modes 0 and 1 can be omitted in size optimized build,
		 * copy mode 2 handles them (but more slowly).
		 */
#if !defined(DUK_USE_PREFER_SIZE)
	case 0: {
		/* Use byte copy. */

		duk_uint8_t *p_src;
		duk_uint8_t *p_dst;

		DUK_ASSERT(h_bufobj != NULL);
		DUK_ASSERT(h_bufobj->buf != NULL);
		DUK_ASSERT(DUK_HBUFOBJ_VALID_SLICE(h_bufobj));
		DUK_ASSERT(h_bufarg != NULL);
		DUK_ASSERT(h_bufarg->buf != NULL);
		DUK_ASSERT(DUK_HBUFOBJ_VALID_SLICE(h_bufarg));

		p_dst = DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_bufobj);
		p_src = DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_bufarg);

		DUK_DDD(DUK_DDDPRINT("using memcpy: p_src=%p, p_dst=%p, byte_length=%ld",
		                     (void *) p_src,
		                     (void *) p_dst,
		                     (long) byte_length));

		duk_memcpy_unsafe((void *) p_dst, (const void *) p_src, (size_t) byte_length);
		break;
	}
	case 1: {
		/* Copy values through direct validated reads and writes. */

		duk_small_uint_t src_elem_size;
		duk_small_uint_t dst_elem_size;
		duk_uint8_t *p_src;
		duk_uint8_t *p_src_end;
		duk_uint8_t *p_dst;

		DUK_ASSERT(h_bufobj != NULL);
		DUK_ASSERT(h_bufobj->buf != NULL);
		DUK_ASSERT(DUK_HBUFOBJ_VALID_SLICE(h_bufobj));
		DUK_ASSERT(h_bufarg != NULL);
		DUK_ASSERT(h_bufarg->buf != NULL);
		DUK_ASSERT(DUK_HBUFOBJ_VALID_SLICE(h_bufarg));

		src_elem_size = (duk_small_uint_t) (1U << h_bufarg->shift);
		dst_elem_size = elem_size;

		p_src = DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_bufarg);
		p_dst = DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_bufobj);
		p_src_end = p_src + h_bufarg->length;

		DUK_DDD(DUK_DDDPRINT("using fast copy: p_src=%p, p_src_end=%p, p_dst=%p, "
		                     "src_elem_size=%d, dst_elem_size=%d",
		                     (void *) p_src,
		                     (void *) p_src_end,
		                     (void *) p_dst,
		                     (int) src_elem_size,
		                     (int) dst_elem_size));

		while (p_src != p_src_end) {
			DUK_DDD(DUK_DDDPRINT("fast path per element copy loop: "
			                     "p_src=%p, p_src_end=%p, p_dst=%p",
			                     (void *) p_src,
			                     (void *) p_src_end,
			                     (void *) p_dst));
			/* A validated read() is always a number, so it's write coercion
			 * is always side effect free an won't invalidate pointers etc.
			 */
			duk_hbufobj_push_validated_read(thr, h_bufarg, p_src, src_elem_size);
			duk_hbufobj_validated_write(thr, h_bufobj, p_dst, dst_elem_size);
			duk_pop(thr);
			p_src += src_elem_size;
			p_dst += dst_elem_size;
		}
		break;
	}
#endif /* !DUK_USE_PREFER_SIZE */
	case 2: {
		/* Copy values by index reads and writes.  Let virtual
		 * property handling take care of coercion.
		 */
		duk_uint_t i;

		DUK_DDD(DUK_DDDPRINT("using slow copy"));

		for (i = 0; i < elem_length; i++) {
			duk_get_prop_index(thr, 0, (duk_uarridx_t) i);
			duk_put_prop_index(thr, -2, (duk_uarridx_t) i);
		}
		break;
	}
	default:
	case 3: {
		/* No copy, leave zero bytes in the buffer.  There's no
		 * ambiguity with Float32/Float64 because zero bytes also
		 * represent 0.0.
		 */

		DUK_DDD(DUK_DDDPRINT("using no copy"));
		break;
	}
	}

	return 1;

fail_arguments:
	DUK_DCERROR_RANGE_INVALID_ARGS(thr);
}
#else /* DUK_USE_BUFFEROBJECT_SUPPORT */
/* When bufferobject support is disabled, new Uint8Array() could still be
 * supported to create a plain fixed buffer.  Disabled for now.
 */
#if 0
DUK_INTERNAL duk_ret_t duk_bi_typedarray_constructor(duk_hthread *thr) {
	duk_int_t elem_length_signed;
	duk_uint_t byte_length;

	/* XXX: The same copy helpers could be shared with at least some
	 * buffer functions.
	 */

	duk_require_constructor_call(thr);

	elem_length_signed = duk_require_int(thr, 0);
	if (elem_length_signed < 0) {
		goto fail_arguments;
	}
	byte_length = (duk_uint_t) elem_length_signed;

	(void) duk_push_fixed_buffer_zero(thr, (duk_size_t) byte_length);
	return 1;

 fail_arguments:
	DUK_DCERROR_RANGE_INVALID_ARGS(thr);
}
#endif /* 0 */
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_dataview_constructor(duk_hthread *thr) {
	duk_hbufobj *h_bufarg;
	duk_hbufobj *h_bufobj;
	duk_hbuffer *h_val;
	duk_uint_t offset;
	duk_uint_t length;

	duk_require_constructor_call(thr);

	h_bufarg = duk__require_bufobj_value(thr, 0);
	DUK_ASSERT(h_bufarg != NULL);
	if (DUK_HOBJECT_GET_CLASS_NUMBER((duk_hobject *) h_bufarg) != DUK_HOBJECT_CLASS_ARRAYBUFFER) {
		DUK_DCERROR_TYPE_INVALID_ARGS(thr);
	}

	duk__resolve_offset_opt_length(thr, h_bufarg, 1, 2, &offset, &length, 1 /*throw_flag*/);
	DUK_ASSERT(offset <= h_bufarg->length);
	DUK_ASSERT(offset + length <= h_bufarg->length);

	h_bufobj = duk_push_bufobj_raw(thr,
	                               DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_BUFOBJ |
	                                   DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_DATAVIEW),
	                               DUK_BIDX_DATAVIEW_PROTOTYPE);

	h_val = h_bufarg->buf;
	if (h_val == NULL) {
		DUK_DCERROR_TYPE_INVALID_ARGS(thr);
	}
	h_bufobj->buf = h_val;
	DUK_HBUFFER_INCREF(thr, h_val);
	h_bufobj->offset = h_bufarg->offset + offset;
	h_bufobj->length = length;
	DUK_ASSERT(h_bufobj->shift == 0);
	DUK_ASSERT(h_bufobj->elem_type == DUK_HBUFOBJ_ELEM_UINT8);
	DUK_ASSERT(h_bufobj->is_typedarray == 0);

	DUK_ASSERT(h_bufobj->buf_prop == NULL);
	h_bufobj->buf_prop = (duk_hobject *) h_bufarg;
	DUK_ASSERT(h_bufarg != NULL);
	DUK_HBUFOBJ_INCREF(thr, h_bufarg);

	DUK_HBUFOBJ_ASSERT_VALID(h_bufobj);
	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  ArrayBuffer.isView()
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_arraybuffer_isview(duk_hthread *thr) {
	duk_hobject *h_obj;
	duk_bool_t ret = 0;

	if (duk_is_buffer(thr, 0)) {
		ret = 1;
	} else {
		h_obj = duk_get_hobject(thr, 0);
		if (h_obj != NULL && DUK_HOBJECT_IS_BUFOBJ(h_obj)) {
			/* DataView needs special casing: ArrayBuffer.isView() is
			 * true, but ->is_typedarray is 0.
			 */
			ret = ((duk_hbufobj *) h_obj)->is_typedarray ||
			      (DUK_HOBJECT_GET_CLASS_NUMBER(h_obj) == DUK_HOBJECT_CLASS_DATAVIEW);
		}
	}
	duk_push_boolean(thr, ret);
	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Uint8Array.allocPlain()
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_uint8array_allocplain(duk_hthread *thr) {
	duk__hbufobj_fixed_from_argvalue(thr);
	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Uint8Array.plainOf()
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_uint8array_plainof(duk_hthread *thr) {
	duk_hbufobj *h_bufobj;

#if !defined(DUK_USE_PREFER_SIZE)
	/* Avoid churn if argument is already a plain buffer. */
	if (duk_is_buffer(thr, 0)) {
		return 1;
	}
#endif

	/* Promotes plain buffers to ArrayBuffers, so for a plain buffer
	 * argument we'll create a pointless temporary (but still work
	 * correctly).
	 */
	h_bufobj = duk__require_bufobj_value(thr, 0);
	if (h_bufobj->buf == NULL) {
		duk_push_undefined(thr);
	} else {
		duk_push_hbuffer(thr, h_bufobj->buf);
	}
	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Node.js Buffer: toString([encoding], [start], [end])
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_nodejs_buffer_tostring(duk_hthread *thr) {
	duk_hbufobj *h_this;
	duk_int_t start_offset, end_offset;
	duk_uint8_t *buf_slice;
	duk_size_t slice_length;

	h_this = duk__get_bufobj_this(thr);
	if (h_this == NULL) {
		/* XXX: happens e.g. when evaluating: String(Buffer.prototype). */
		duk_push_literal(thr, "[object Object]");
		return 1;
	}
	DUK_HBUFOBJ_ASSERT_VALID(h_this);

	/* Ignore encoding for now. */

	duk__clamp_startend_nonegidx_noshift(thr,
	                                     (duk_int_t) h_this->length,
	                                     1 /*idx_start*/,
	                                     2 /*idx_end*/,
	                                     &start_offset,
	                                     &end_offset);

	slice_length = (duk_size_t) (end_offset - start_offset);
	buf_slice = (duk_uint8_t *) duk_push_fixed_buffer_nozero(thr, slice_length); /* all bytes initialized below */
	DUK_ASSERT(buf_slice != NULL);

	/* Neutered or uncovered, TypeError. */
	if (h_this->buf == NULL || !DUK_HBUFOBJ_VALID_BYTEOFFSET_EXCL(h_this, (duk_size_t) start_offset + slice_length)) {
		DUK_DCERROR_TYPE_INVALID_ARGS(thr);
	}

	/* XXX: ideally we wouldn't make a copy but a view into the buffer for the
	 * decoding process.  Or the decoding helper could be changed to accept
	 * the slice info (a buffer pointer is NOT a good approach because guaranteeing
	 * its stability is difficult).
	 */

	DUK_ASSERT(DUK_HBUFOBJ_VALID_BYTEOFFSET_EXCL(h_this, (duk_size_t) start_offset + slice_length));
	duk_memcpy_unsafe((void *) buf_slice,
	                  (const void *) (DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_this) + start_offset),
	                  (size_t) slice_length);

	/* Use the equivalent of: new TextEncoder().encode(this) to convert the
	 * string.  Result will be valid UTF-8; non-CESU-8 inputs are currently
	 * interpreted loosely.  Value stack convention is a bit odd for now.
	 */
	duk_replace(thr, 0);
	duk_set_top(thr, 1);
	return duk_textdecoder_decode_utf8_nodejs(thr);
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Node.js Buffer.prototype: toJSON()
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_nodejs_buffer_tojson(duk_hthread *thr) {
	duk_hbufobj *h_this;
	duk_uint8_t *buf;
	duk_uint_t i, n;
	duk_tval *tv;

	h_this = duk__require_bufobj_this(thr);
	DUK_ASSERT(h_this != NULL);

	if (h_this->buf == NULL || !DUK_HBUFOBJ_VALID_SLICE(h_this)) {
		/* Serialize uncovered backing buffer as a null; doesn't
		 * really matter as long we're memory safe.
		 */
		duk_push_null(thr);
		return 1;
	}

	duk_push_object(thr);
	duk_push_hstring_stridx(thr, DUK_STRIDX_UC_BUFFER);
	duk_put_prop_stridx_short(thr, -2, DUK_STRIDX_TYPE);

	/* XXX: uninitialized would be OK */
	DUK_ASSERT_DISABLE((duk_size_t) h_this->length <= (duk_size_t) DUK_UINT32_MAX);
	tv = duk_push_harray_with_size_outptr(thr, (duk_uint32_t) h_this->length); /* XXX: needs revision with >4G buffers */
	DUK_ASSERT(!duk_is_bare_object(thr, -1));

	DUK_ASSERT(h_this->buf != NULL);
	buf = DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_this);
	for (i = 0, n = h_this->length; i < n; i++) {
		DUK_TVAL_SET_U32(tv + i, (duk_uint32_t) buf[i]); /* no need for decref or incref */
	}
	duk_put_prop_stridx_short(thr, -2, DUK_STRIDX_DATA);

	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Node.js Buffer.prototype.equals()
 *  Node.js Buffer.prototype.compare()
 *  Node.js Buffer.compare()
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_buffer_compare_shared(duk_hthread *thr) {
	duk_small_uint_t magic;
	duk_hbufobj *h_bufarg1;
	duk_hbufobj *h_bufarg2;
	duk_small_int_t comp_res;

	/* XXX: keep support for plain buffers and non-Node.js buffers? */

	magic = (duk_small_uint_t) duk_get_current_magic(thr);
	if (magic & 0x02U) {
		/* Static call style. */
		h_bufarg1 = duk__require_bufobj_value(thr, 0);
		h_bufarg2 = duk__require_bufobj_value(thr, 1);
	} else {
		h_bufarg1 = duk__require_bufobj_this(thr);
		h_bufarg2 = duk__require_bufobj_value(thr, 0);
	}
	DUK_ASSERT(h_bufarg1 != NULL);
	DUK_ASSERT(h_bufarg2 != NULL);

	/* We want to compare the slice/view areas of the arguments.
	 * If either slice/view is invalid (underlying buffer is shorter)
	 * ensure equals() is false, but otherwise the only thing that
	 * matters is to be memory safe.
	 */

	if (DUK_HBUFOBJ_VALID_SLICE(h_bufarg1) && DUK_HBUFOBJ_VALID_SLICE(h_bufarg2)) {
		comp_res = duk_js_data_compare(
		    (const duk_uint8_t *) DUK_HBUFFER_GET_DATA_PTR(thr->heap, h_bufarg1->buf) + h_bufarg1->offset,
		    (const duk_uint8_t *) DUK_HBUFFER_GET_DATA_PTR(thr->heap, h_bufarg2->buf) + h_bufarg2->offset,
		    (duk_size_t) h_bufarg1->length,
		    (duk_size_t) h_bufarg2->length);
	} else {
		comp_res = -1; /* either nonzero value is ok */
	}

	if (magic & 0x01U) {
		/* compare: similar to string comparison but for buffer data. */
		duk_push_int(thr, comp_res);
	} else {
		/* equals */
		duk_push_boolean(thr, (comp_res == 0));
	}

	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Node.js Buffer.prototype.fill()
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_nodejs_buffer_fill(duk_hthread *thr) {
	duk_hbufobj *h_this;
	const duk_uint8_t *fill_str_ptr;
	duk_size_t fill_str_len;
	duk_uint8_t fill_value;
	duk_int_t fill_offset;
	duk_int_t fill_end;
	duk_size_t fill_length;
	duk_uint8_t *p;

	h_this = duk__require_bufobj_this(thr);
	DUK_ASSERT(h_this != NULL);
	if (h_this->buf == NULL) {
		DUK_DCERROR_TYPE_INVALID_ARGS(thr);
	}

	/* [ value offset end ] */

	if (duk_is_string_notsymbol(thr, 0)) {
		fill_str_ptr = (const duk_uint8_t *) duk_get_lstring(thr, 0, &fill_str_len);
		DUK_ASSERT(fill_str_ptr != NULL);
	} else {
		/* Symbols get ToNumber() coerced and cause TypeError. */
		fill_value = (duk_uint8_t) duk_to_uint32(thr, 0);
		fill_str_ptr = (const duk_uint8_t *) &fill_value;
		fill_str_len = 1;
	}

	/* Fill offset handling is more lenient than in Node.js. */

	duk__clamp_startend_nonegidx_noshift(thr,
	                                     (duk_int_t) h_this->length,
	                                     1 /*idx_start*/,
	                                     2 /*idx_end*/,
	                                     &fill_offset,
	                                     &fill_end);

	DUK_DDD(DUK_DDDPRINT("fill: fill_value=%02x, fill_offset=%ld, fill_end=%ld, view length=%ld",
	                     (unsigned int) fill_value,
	                     (long) fill_offset,
	                     (long) fill_end,
	                     (long) h_this->length));

	DUK_ASSERT(fill_end - fill_offset >= 0);
	DUK_ASSERT(h_this->buf != NULL);

	p = (DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_this) + fill_offset);
	fill_length = (duk_size_t) (fill_end - fill_offset);
	if (fill_str_len == 1) {
		/* Handle single character fills as memset() even when
		 * the fill data comes from a one-char argument.
		 */
		duk_memset_unsafe((void *) p, (int) fill_str_ptr[0], (size_t) fill_length);
	} else if (fill_str_len > 1) {
		duk_size_t i, n, t;

		for (i = 0, n = (duk_size_t) (fill_end - fill_offset), t = 0; i < n; i++) {
			p[i] = fill_str_ptr[t++];
			if (t >= fill_str_len) {
				t = 0;
			}
		}
	} else {
		DUK_DDD(DUK_DDDPRINT("zero size fill pattern, ignore silently"));
	}

	/* Return the Buffer to allow chaining: b.fill(0x11).fill(0x22, 3, 5).toString() */
	duk_push_this(thr);
	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Node.js Buffer.prototype.write(string, [offset], [length], [encoding])
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_nodejs_buffer_write(duk_hthread *thr) {
	duk_hbufobj *h_this;
	duk_uint_t offset;
	duk_uint_t length;
	const duk_uint8_t *str_data;
	duk_size_t str_len;

	/* XXX: very inefficient support for plain buffers */
	h_this = duk__require_bufobj_this(thr);
	DUK_ASSERT(h_this != NULL);

	/* Argument must be a string, e.g. a buffer is not allowed. */
	str_data = (const duk_uint8_t *) duk_require_lstring_notsymbol(thr, 0, &str_len);

	duk__resolve_offset_opt_length(thr, h_this, 1, 2, &offset, &length, 0 /*throw_flag*/);
	DUK_ASSERT(offset <= h_this->length);
	DUK_ASSERT(offset + length <= h_this->length);

	/* XXX: encoding is ignored now. */

	if (length > str_len) {
		length = (duk_uint_t) str_len;
	}

	if (DUK_HBUFOBJ_VALID_SLICE(h_this)) {
		/* Cannot overlap. */
		duk_memcpy_unsafe((void *) (DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_this) + offset),
		                  (const void *) str_data,
		                  (size_t) length);
	} else {
		DUK_DDD(DUK_DDDPRINT("write() target buffer is not covered, silent ignore"));
	}

	duk_push_uint(thr, length);
	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Node.js Buffer.prototype.copy()
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_nodejs_buffer_copy(duk_hthread *thr) {
	duk_hbufobj *h_this;
	duk_hbufobj *h_bufarg;
	duk_int_t source_length;
	duk_int_t target_length;
	duk_int_t target_start, source_start, source_end;
	duk_uint_t target_ustart, source_ustart, source_uend;
	duk_uint_t copy_size = 0;

	/* [ targetBuffer targetStart sourceStart sourceEnd ] */

	h_this = duk__require_bufobj_this(thr);
	h_bufarg = duk__require_bufobj_value(thr, 0);
	DUK_ASSERT(h_this != NULL);
	DUK_ASSERT(h_bufarg != NULL);
	source_length = (duk_int_t) h_this->length;
	target_length = (duk_int_t) h_bufarg->length;

	target_start = duk_to_int(thr, 1);
	source_start = duk_to_int(thr, 2);
	if (duk_is_undefined(thr, 3)) {
		source_end = source_length;
	} else {
		source_end = duk_to_int(thr, 3);
	}

	DUK_DDD(DUK_DDDPRINT("checking copy args: target_start=%ld, target_length=%ld, "
	                     "source_start=%ld, source_end=%ld, source_length=%ld",
	                     (long) target_start,
	                     (long) h_bufarg->length,
	                     (long) source_start,
	                     (long) source_end,
	                     (long) source_length));

	/* This behavior mostly mimics Node.js now. */

	if (source_start < 0 || source_end < 0 || target_start < 0) {
		/* Negative offsets cause a RangeError. */
		goto fail_bounds;
	}
	source_ustart = (duk_uint_t) source_start;
	source_uend = (duk_uint_t) source_end;
	target_ustart = (duk_uint_t) target_start;
	if (source_ustart >= source_uend || /* crossed offsets or zero size */
	    source_ustart >= (duk_uint_t) source_length || /* source out-of-bounds (but positive) */
	    target_ustart >= (duk_uint_t) target_length) { /* target out-of-bounds (but positive) */
		goto silent_ignore;
	}
	if (source_uend >= (duk_uint_t) source_length) {
		/* Source end clamped silently to available length. */
		source_uend = (duk_uint_t) source_length;
	}
	copy_size = source_uend - source_ustart;
	if (target_ustart + copy_size > (duk_uint_t) target_length) {
		/* Clamp to target's end if too long.
		 *
		 * NOTE: there's no overflow possibility in the comparison;
		 * both target_ustart and copy_size are >= 0 and based on
		 * values in duk_int_t range.  Adding them as duk_uint_t
		 * values is then guaranteed not to overflow.
		 */
		DUK_ASSERT(target_ustart + copy_size >= target_ustart); /* no overflow */
		DUK_ASSERT(target_ustart + copy_size >= copy_size); /* no overflow */
		copy_size = (duk_uint_t) target_length - target_ustart;
	}

	DUK_DDD(DUK_DDDPRINT("making copy: target_ustart=%lu source_ustart=%lu copy_size=%lu",
	                     (unsigned long) target_ustart,
	                     (unsigned long) source_ustart,
	                     (unsigned long) copy_size));

	DUK_ASSERT(copy_size >= 1);
	DUK_ASSERT(source_ustart <= (duk_uint_t) source_length);
	DUK_ASSERT(source_ustart + copy_size <= (duk_uint_t) source_length);
	DUK_ASSERT(target_ustart <= (duk_uint_t) target_length);
	DUK_ASSERT(target_ustart + copy_size <= (duk_uint_t) target_length);

	/* Ensure copy is covered by underlying buffers. */
	DUK_ASSERT(h_bufarg->buf != NULL); /* length check */
	DUK_ASSERT(h_this->buf != NULL); /* length check */
	if (DUK_HBUFOBJ_VALID_BYTEOFFSET_EXCL(h_bufarg, target_ustart + copy_size) &&
	    DUK_HBUFOBJ_VALID_BYTEOFFSET_EXCL(h_this, source_ustart + copy_size)) {
		/* Must use memmove() because copy area may overlap (source and target
		 * buffer may be the same, or from different slices.
		 */
		duk_memmove_unsafe((void *) (DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_bufarg) + target_ustart),
		                   (const void *) (DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_this) + source_ustart),
		                   (size_t) copy_size);
	} else {
		DUK_DDD(DUK_DDDPRINT("buffer copy not covered by underlying buffer(s), ignoring"));
	}

silent_ignore:
	/* Return value is like write(), number of bytes written.
	 * The return value matters because of code like:
	 * "off += buf.copy(...)".
	 */
	duk_push_uint(thr, copy_size);
	return 1;

fail_bounds:
	DUK_DCERROR_RANGE_INVALID_ARGS(thr);
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  TypedArray.prototype.set()
 *
 *  TypedArray set() is pretty interesting to implement because:
 *
 *    - The source argument may be a plain array or a typedarray.  If the
 *      source is a TypedArray, values are decoded and re-encoded into the
 *      target (not as a plain byte copy).  This may happen even when the
 *      element byte size is the same, e.g. integer values may be re-encoded
 *      into floats.
 *
 *    - Source and target may refer to the same underlying buffer, so that
 *      the set() operation may overlap.  The specification requires that this
 *      must work as if a copy was made before the operation.  Note that this
 *      is NOT a simple memmove() situation because the source and target
 *      byte sizes may be different -- e.g. a 4-byte source (Int8Array) may
 *      expand to a 16-byte target (Uint32Array) so that the target overlaps
 *      the source both from beginning and the end (unlike in typical memmove).
 *
 *    - Even if 'buf' pointers of the source and target differ, there's no
 *      guarantee that their memory areas don't overlap.  This may be the
 *      case with external buffers.
 *
 *  Even so, it is nice to optimize for the common case:
 *
 *    - Source and target separate buffers or non-overlapping.
 *
 *    - Source and target have a compatible type so that a plain byte copy
 *      is possible.  Note that while e.g. uint8 and int8 are compatible
 *      (coercion one way or another doesn't change the byte representation),
 *      e.g. int8 and uint8clamped are NOT compatible when writing int8
 *      values into uint8clamped typedarray (-1 would clamp to 0 for instance).
 *
 *  See test-bi-typedarray-proto-set.js.
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_typedarray_set(duk_hthread *thr) {
	duk_hbufobj *h_this;
	duk_hobject *h_obj;
	duk_uarridx_t i, n;
	duk_int_t offset_signed;
	duk_uint_t offset_elems;
	duk_uint_t offset_bytes;

	h_this = duk__require_bufobj_this(thr);
	DUK_ASSERT(h_this != NULL);
	DUK_HBUFOBJ_ASSERT_VALID(h_this);

	if (h_this->buf == NULL) {
		DUK_DDD(DUK_DDDPRINT("source neutered, skip copy"));
		return 0;
	}

	duk_hbufobj_promote_plain(thr, 0);
	h_obj = duk_require_hobject(thr, 0);

	/* XXX: V8 throws a TypeError for negative values.  Would it
	 * be more useful to interpret negative offsets here from the
	 * end of the buffer too?
	 */
	offset_signed = duk_to_int(thr, 1);
	if (offset_signed < 0) {
		/* For some reason this is a TypeError (at least in V8). */
		DUK_DCERROR_TYPE_INVALID_ARGS(thr);
	}
	offset_elems = (duk_uint_t) offset_signed;
	offset_bytes = offset_elems << h_this->shift;
	if ((offset_bytes >> h_this->shift) != offset_elems) {
		/* Byte length would overflow. */
		/* XXX: easier check with less code? */
		goto fail_args;
	}
	if (offset_bytes > h_this->length) {
		/* Equality may be OK but >length not.  Checking
		 * this explicitly avoids some overflow cases
		 * below.
		 */
		goto fail_args;
	}
	DUK_ASSERT(offset_bytes <= h_this->length);

	/* Fast path: source is a TypedArray (or any bufobj). */

	if (DUK_HOBJECT_IS_BUFOBJ(h_obj)) {
		duk_hbufobj *h_bufarg;
#if !defined(DUK_USE_PREFER_SIZE)
		duk_uint16_t comp_mask;
#endif
		duk_small_int_t no_overlap = 0;
		duk_uint_t src_length;
		duk_uint_t dst_length;
		duk_uint_t dst_length_elems;
		duk_uint8_t *p_src_base;
		duk_uint8_t *p_src_end;
		duk_uint8_t *p_src;
		duk_uint8_t *p_dst_base;
		duk_uint8_t *p_dst;
		duk_small_uint_t src_elem_size;
		duk_small_uint_t dst_elem_size;

		h_bufarg = (duk_hbufobj *) h_obj;
		DUK_HBUFOBJ_ASSERT_VALID(h_bufarg);

		if (h_bufarg->buf == NULL) {
			DUK_DDD(DUK_DDDPRINT("target neutered, skip copy"));
			return 0;
		}

		/* Nominal size check. */
		src_length = h_bufarg->length; /* bytes in source */
		dst_length_elems = (src_length >> h_bufarg->shift); /* elems in source and dest */
		dst_length = dst_length_elems << h_this->shift; /* bytes in dest */
		if ((dst_length >> h_this->shift) != dst_length_elems) {
			/* Byte length would overflow. */
			/* XXX: easier check with less code? */
			goto fail_args;
		}
		DUK_DDD(DUK_DDDPRINT("nominal size check: src_length=%ld, dst_length=%ld", (long) src_length, (long) dst_length));
		DUK_ASSERT(offset_bytes <= h_this->length);
		if (dst_length > h_this->length - offset_bytes) {
			/* Overflow not an issue because subtraction is used on the right
			 * side and guaranteed to be >= 0.
			 */
			DUK_DDD(DUK_DDDPRINT("copy exceeds target buffer nominal length"));
			goto fail_args;
		}
		if (!DUK_HBUFOBJ_VALID_BYTEOFFSET_EXCL(h_this, offset_bytes + dst_length)) {
			DUK_DDD(DUK_DDDPRINT("copy not covered by underlying target buffer, ignore"));
			return 0;
		}

		p_src_base = DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_bufarg);
		p_dst_base = DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_this) + offset_bytes;

		/* Check actual underlying buffers for validity and that they
		 * cover the copy.  No side effects are allowed after the check
		 * so that the validity status doesn't change.
		 */
		if (!DUK_HBUFOBJ_VALID_SLICE(h_this) || !DUK_HBUFOBJ_VALID_SLICE(h_bufarg)) {
			/* The condition could be more narrow and check for the
			 * copy area only, but there's no need for fine grained
			 * behavior when the underlying buffer is misconfigured.
			 */
			DUK_DDD(DUK_DDDPRINT("source and/or target not covered by underlying buffer, skip copy"));
			return 0;
		}

		/* We want to do a straight memory copy if possible: this is
		 * an important operation because .set() is the TypedArray
		 * way to copy chunks of memory.  However, because set()
		 * conceptually works in terms of elements, not all views are
		 * compatible with direct byte copying.
		 *
		 * If we do manage a direct copy, the "overlap issue" handled
		 * below can just be solved using memmove() because the source
		 * and destination element sizes are necessarily equal.
		 */

#if !defined(DUK_USE_PREFER_SIZE)
		DUK_ASSERT(h_this->elem_type < sizeof(duk__buffer_elemtype_copy_compatible) / sizeof(duk_uint16_t));
		comp_mask = duk__buffer_elemtype_copy_compatible[h_this->elem_type];
		if (comp_mask & (1 << h_bufarg->elem_type)) {
			DUK_ASSERT(src_length == dst_length);

			DUK_DDD(DUK_DDDPRINT("fast path: able to use memmove() because views are compatible"));
			duk_memmove_unsafe((void *) p_dst_base, (const void *) p_src_base, (size_t) dst_length);
			return 0;
		}
		DUK_DDD(DUK_DDDPRINT("fast path: views are not compatible with a byte copy, copy by item"));
#endif /* !DUK_USE_PREFER_SIZE */

		/* We want to avoid making a copy to process set() but that's
		 * not always possible: the source and the target may overlap
		 * and because element sizes are different, the overlap cannot
		 * always be handled with a memmove() or choosing the copy
		 * direction in a certain way.  For example, if source type is
		 * uint8 and target type is uint32, the target area may exceed
		 * the source area from both ends!
		 *
		 * Note that because external buffers may point to the same
		 * memory areas, we must ultimately make this check using
		 * pointers.
		 *
		 * NOTE: careful with side effects: any side effect may cause
		 * a buffer resize (or external buffer pointer/length update)!
		 */

		DUK_DDD(DUK_DDDPRINT("overlap check: p_src_base=%p, src_length=%ld, "
		                     "p_dst_base=%p, dst_length=%ld",
		                     (void *) p_src_base,
		                     (long) src_length,
		                     (void *) p_dst_base,
		                     (long) dst_length));

		if (p_src_base >= p_dst_base + dst_length || /* source starts after dest ends */
		    p_src_base + src_length <= p_dst_base) { /* source ends before dest starts */
			no_overlap = 1;
		}

		if (!no_overlap) {
			/* There's overlap: the desired end result is that
			 * conceptually a copy is made to avoid "trampling"
			 * of source data by destination writes.  We make
			 * an actual temporary copy to handle this case.
			 */
			duk_uint8_t *p_src_copy;

			DUK_DDD(DUK_DDDPRINT("there is overlap, make a copy of the source"));
			p_src_copy = (duk_uint8_t *) duk_push_fixed_buffer_nozero(thr, src_length);
			DUK_ASSERT(p_src_copy != NULL);
			duk_memcpy_unsafe((void *) p_src_copy, (const void *) p_src_base, (size_t) src_length);

			p_src_base = p_src_copy; /* use p_src_base from now on */
		}
		/* Value stack intentionally mixed size here. */

		DUK_DDD(DUK_DDDPRINT("after overlap check: p_src_base=%p, src_length=%ld, "
		                     "p_dst_base=%p, dst_length=%ld, valstack top=%ld",
		                     (void *) p_src_base,
		                     (long) src_length,
		                     (void *) p_dst_base,
		                     (long) dst_length,
		                     (long) duk_get_top(thr)));

		/* Ready to make the copy.  We must proceed element by element
		 * and must avoid any side effects that might cause the buffer
		 * validity check above to become invalid.
		 *
		 * Although we work through the value stack here, only plain
		 * numbers are handled which should be side effect safe.
		 */

		src_elem_size = (duk_small_uint_t) (1U << h_bufarg->shift);
		dst_elem_size = (duk_small_uint_t) (1U << h_this->shift);
		p_src = p_src_base;
		p_dst = p_dst_base;
		p_src_end = p_src_base + src_length;

		while (p_src != p_src_end) {
			DUK_DDD(DUK_DDDPRINT("fast path per element copy loop: "
			                     "p_src=%p, p_src_end=%p, p_dst=%p",
			                     (void *) p_src,
			                     (void *) p_src_end,
			                     (void *) p_dst));
			/* A validated read() is always a number, so it's write coercion
			 * is always side effect free an won't invalidate pointers etc.
			 */
			duk_hbufobj_push_validated_read(thr, h_bufarg, p_src, src_elem_size);
			duk_hbufobj_validated_write(thr, h_this, p_dst, dst_elem_size);
			duk_pop(thr);
			p_src += src_elem_size;
			p_dst += dst_elem_size;
		}

		return 0;
	} else {
		/* Slow path: quite slow, but we save space by using the property code
		 * to write coerce target values.  We don't need to worry about overlap
		 * here because the source is not a TypedArray.
		 *
		 * We could use the bufobj write coercion helper but since the
		 * property read may have arbitrary side effects, full validity checks
		 * would be needed for every element anyway.
		 */

		n = (duk_uarridx_t) duk_get_length(thr, 0);
		DUK_ASSERT(offset_bytes <= h_this->length);
		if ((n << h_this->shift) > h_this->length - offset_bytes) {
			/* Overflow not an issue because subtraction is used on the right
			 * side and guaranteed to be >= 0.
			 */
			DUK_DDD(DUK_DDDPRINT("copy exceeds target buffer nominal length"));
			goto fail_args;
		}

		/* There's no need to check for buffer validity status for the
		 * target here: the property access code will do that for each
		 * element.  Moreover, if we did check the validity here, side
		 * effects from reading the source argument might invalidate
		 * the results anyway.
		 */

		DUK_ASSERT_TOP(thr, 2);
		duk_push_this(thr);

		for (i = 0; i < n; i++) {
			duk_get_prop_index(thr, 0, i);
			duk_put_prop_index(thr, 2, offset_elems + i);
		}
	}

	return 0;

fail_args:
	DUK_DCERROR_RANGE_INVALID_ARGS(thr);
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Node.js Buffer.prototype.slice([start], [end])
 *  ArrayBuffer.prototype.slice(begin, [end])
 *  TypedArray.prototype.subarray(begin, [end])
 *
 *  The API calls are almost identical; negative indices are counted from end
 *  of buffer, and final indices are clamped (allowing crossed indices).  Main
 *  differences:
 *
 *    - Copy/view behavior; Node.js .slice() and TypedArray .subarray() create
 *      views, ArrayBuffer .slice() creates a copy
 *
 *    - Resulting object has a different class and prototype depending on the
 *      call (or 'this' argument)
 *
 *    - TypedArray .subarray() arguments are element indices, not byte offsets
 *
 *    - Plain buffer argument creates a plain buffer slice
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_LOCAL void duk__arraybuffer_plain_slice(duk_hthread *thr, duk_hbuffer *h_val) {
	duk_int_t start_offset, end_offset;
	duk_uint_t slice_length;
	duk_uint8_t *p_copy;
	duk_size_t copy_length;

	duk__clamp_startend_negidx_shifted(thr,
	                                   (duk_int_t) DUK_HBUFFER_GET_SIZE(h_val),
	                                   0 /*buffer_shift*/,
	                                   0 /*idx_start*/,
	                                   1 /*idx_end*/,
	                                   &start_offset,
	                                   &end_offset);
	DUK_ASSERT(end_offset <= (duk_int_t) DUK_HBUFFER_GET_SIZE(h_val));
	DUK_ASSERT(start_offset >= 0);
	DUK_ASSERT(end_offset >= start_offset);
	slice_length = (duk_uint_t) (end_offset - start_offset);

	p_copy = (duk_uint8_t *) duk_push_fixed_buffer_nozero(thr, (duk_size_t) slice_length);
	DUK_ASSERT(p_copy != NULL);
	copy_length = slice_length;

	duk_memcpy_unsafe((void *) p_copy,
	                  (const void *) ((duk_uint8_t *) DUK_HBUFFER_GET_DATA_PTR(thr->heap, h_val) + start_offset),
	                  copy_length);
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
/* Shared helper for slice/subarray operation.
 * Magic: 0x01=isView, 0x02=copy, 0x04=Node.js Buffer special handling.
 */
DUK_INTERNAL duk_ret_t duk_bi_buffer_slice_shared(duk_hthread *thr) {
	duk_small_int_t magic;
	duk_small_uint_t res_class_num;
	duk_small_int_t res_proto_bidx;
	duk_hbufobj *h_this;
	duk_hbufobj *h_bufobj;
	duk_hbuffer *h_val;
	duk_int_t start_offset, end_offset;
	duk_uint_t slice_length;
	duk_tval *tv;

	/* [ start end ] */

	magic = duk_get_current_magic(thr);

	tv = duk_get_borrowed_this_tval(thr);
	DUK_ASSERT(tv != NULL);

	if (DUK_TVAL_IS_BUFFER(tv)) {
		/* For plain buffers return a plain buffer slice. */
		h_val = DUK_TVAL_GET_BUFFER(tv);
		DUK_ASSERT(h_val != NULL);

		if (magic & 0x02) {
			/* Make copy: ArrayBuffer.prototype.slice() uses this. */
			duk__arraybuffer_plain_slice(thr, h_val);
			return 1;
		} else {
			/* View into existing buffer: cannot be done if the
			 * result is a plain buffer because there's no slice
			 * info.  So return an ArrayBuffer instance; coerce
			 * the 'this' binding into an object and behave as if
			 * the original call was for an Object-coerced plain
			 * buffer (handled automatically by duk__require_bufobj_this()).
			 */

			DUK_DDD(DUK_DDDPRINT("slice() doesn't handle view into plain buffer, coerce 'this' to ArrayBuffer object"));
			/* fall through */
		}
	}
	tv = NULL; /* No longer valid nor needed. */

	h_this = duk__require_bufobj_this(thr);

	/* Slice offsets are element (not byte) offsets, which only matters
	 * for TypedArray views, Node.js Buffer and ArrayBuffer have shift
	 * zero so byte and element offsets are the same.  Negative indices
	 * are counted from end of slice, crossed indices are allowed (and
	 * result in zero length result), and final values are clamped
	 * against the current slice.  There's intentionally no check
	 * against the underlying buffer here.
	 */

	duk__clamp_startend_negidx_shifted(thr,
	                                   (duk_int_t) h_this->length,
	                                   (duk_uint8_t) h_this->shift,
	                                   0 /*idx_start*/,
	                                   1 /*idx_end*/,
	                                   &start_offset,
	                                   &end_offset);
	DUK_ASSERT(end_offset >= start_offset);
	DUK_ASSERT(start_offset >= 0);
	DUK_ASSERT(end_offset >= 0);
	slice_length = (duk_uint_t) (end_offset - start_offset);

	/* The resulting buffer object gets the same class and prototype as
	 * the buffer in 'this', e.g. if the input is a Uint8Array the
	 * result is a Uint8Array; if the input is a Float32Array, the
	 * result is a Float32Array.  The result internal prototype should
	 * be the default prototype for the class (e.g. initial value of
	 * Uint8Array.prototype), not copied from the argument (Duktape 1.x
	 * did that).
	 *
	 * Node.js Buffers have special handling: they're Uint8Arrays as far
	 * as the internal class is concerned, so the new Buffer should also
	 * be an Uint8Array but inherit from Buffer.prototype.
	 */
	res_class_num = DUK_HOBJECT_GET_CLASS_NUMBER((duk_hobject *) h_this);
	DUK_ASSERT(res_class_num >= DUK_HOBJECT_CLASS_BUFOBJ_MIN); /* type check guarantees */
	DUK_ASSERT(res_class_num <= DUK_HOBJECT_CLASS_BUFOBJ_MAX);
	res_proto_bidx = duk__buffer_proto_from_classnum[res_class_num - DUK_HOBJECT_CLASS_BUFOBJ_MIN];
	if (magic & 0x04) {
		res_proto_bidx = DUK_BIDX_NODEJS_BUFFER_PROTOTYPE;
	}
	h_bufobj =
	    duk_push_bufobj_raw(thr,
	                        DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_BUFOBJ | DUK_HOBJECT_CLASS_AS_FLAGS(res_class_num),
	                        res_proto_bidx);
	DUK_ASSERT(h_bufobj != NULL);

	DUK_ASSERT(h_bufobj->length == 0);
	h_bufobj->shift = h_this->shift; /* inherit */
	h_bufobj->elem_type = h_this->elem_type; /* inherit */
	h_bufobj->is_typedarray = magic & 0x01;
	DUK_ASSERT(h_bufobj->is_typedarray == 0 || h_bufobj->is_typedarray == 1);

	h_val = h_this->buf;
	if (h_val == NULL) {
		DUK_DCERROR_TYPE_INVALID_ARGS(thr);
	}

	if (magic & 0x02) {
		/* non-zero: make copy */
		duk_uint8_t *p_copy;
		duk_size_t copy_length;

		p_copy = (duk_uint8_t *) duk_push_fixed_buffer_zero(
		    thr,
		    (duk_size_t) slice_length); /* must be zeroed, not all bytes always copied */
		DUK_ASSERT(p_copy != NULL);

		/* Copy slice, respecting underlying buffer limits; remainder
		 * is left as zero.
		 */
		copy_length = DUK_HBUFOBJ_CLAMP_BYTELENGTH(h_this, slice_length);
		duk_memcpy_unsafe((void *) p_copy,
		                  (const void *) (DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_this) + start_offset),
		                  copy_length);

		h_val = duk_known_hbuffer(thr, -1);

		h_bufobj->buf = h_val;
		DUK_HBUFFER_INCREF(thr, h_val);
		h_bufobj->length = slice_length;
		DUK_ASSERT(h_bufobj->offset == 0);

		duk_pop(thr); /* reachable so pop OK */
	} else {
		h_bufobj->buf = h_val;
		DUK_HBUFFER_INCREF(thr, h_val);
		h_bufobj->length = slice_length;
		h_bufobj->offset = h_this->offset + (duk_uint_t) start_offset;

		/* Copy the .buffer property, needed for TypedArray.prototype.subarray().
		 *
		 * XXX: limit copy only for TypedArray classes specifically?
		 */

		DUK_ASSERT(h_bufobj->buf_prop == NULL);
		h_bufobj->buf_prop = h_this->buf_prop; /* may be NULL */
		DUK_HOBJECT_INCREF_ALLOWNULL(thr, (duk_hobject *) h_bufobj->buf_prop);
	}
	/* unbalanced stack on purpose */

	DUK_HBUFOBJ_ASSERT_VALID(h_bufobj);
	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Node.js Buffer.isEncoding()
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_nodejs_buffer_is_encoding(duk_hthread *thr) {
	const char *encoding;

	/* only accept lowercase 'utf8' now. */

	encoding = duk_to_string(thr, 0);
	DUK_ASSERT(duk_is_string(thr, 0)); /* guaranteed by duk_to_string() */
	duk_push_boolean(thr, DUK_STRCMP(encoding, "utf8") == 0);
	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Node.js Buffer.isBuffer()
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_nodejs_buffer_is_buffer(duk_hthread *thr) {
	duk_hobject *h;
	duk_hobject *h_proto;
	duk_bool_t ret = 0;

	DUK_ASSERT(duk_get_top(thr) >= 1); /* nargs */
	h = duk_get_hobject(thr, 0);
	if (h != NULL) {
		h_proto = thr->builtins[DUK_BIDX_NODEJS_BUFFER_PROTOTYPE];
		DUK_ASSERT(h_proto != NULL);

		h = DUK_HOBJECT_GET_PROTOTYPE(thr->heap, h);
		if (h != NULL) {
			ret = duk_hobject_prototype_chain_contains(thr, h, h_proto, 0 /*ignore_loop*/);
		}
	}

	duk_push_boolean(thr, ret);
	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Node.js Buffer.byteLength()
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_nodejs_buffer_byte_length(duk_hthread *thr) {
	const char *str;
	duk_size_t len;

	/* At the moment Buffer(<str>) will just use the string bytes as
	 * is (ignoring encoding), so we return the string length here
	 * unconditionally.
	 */

	/* XXX: to be revised; Old Node.js behavior just coerces any buffer
	 * values to string:
	 * $ node
	 * > Buffer.byteLength(new Uint32Array(10))
	 * 20
	 * > Buffer.byteLength(new Uint32Array(100))
	 * 20
	 * (The 20 comes from '[object Uint32Array]'.length
	 */

	str = duk_to_lstring(thr, 0, &len);
	DUK_UNREF(str);
	duk_push_size_t(thr, len);
	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Node.js Buffer.concat()
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_nodejs_buffer_concat(duk_hthread *thr) {
	duk_hobject *h_arg;
	duk_uint_t total_length;
	duk_hbufobj *h_bufobj;
	duk_hbufobj *h_bufres;
	duk_hbuffer *h_val;
	duk_uint_t i, n;
	duk_uint8_t *p;
	duk_size_t space_left;
	duk_size_t copy_size;

	/* Node.js accepts only actual Arrays. */
	h_arg = duk_require_hobject(thr, 0);
	if (DUK_HOBJECT_GET_CLASS_NUMBER(h_arg) != DUK_HOBJECT_CLASS_ARRAY) {
		DUK_DCERROR_TYPE_INVALID_ARGS(thr);
	}

	/* Compute result length and validate argument buffers. */
	n = (duk_uint_t) duk_get_length(thr, 0);
	total_length = 0;
	for (i = 0; i < n; i++) {
		/* Neutered checks not necessary here: neutered buffers have
		 * zero 'length' so we'll effectively skip them.
		 */
		DUK_ASSERT_TOP(thr, 2); /* [ array totalLength ] */
		duk_get_prop_index(thr, 0, (duk_uarridx_t) i); /* -> [ array totalLength buf ] */
		h_bufobj = duk__require_bufobj_value(thr, 2);
		DUK_ASSERT(h_bufobj != NULL);
		total_length += h_bufobj->length;
		if (DUK_UNLIKELY(total_length < h_bufobj->length)) {
			DUK_DCERROR_RANGE_INVALID_ARGS(thr); /* Wrapped. */
		}
		duk_pop(thr);
	}
	/* In Node.js v0.12.1 a 1-element array is special and won't create a
	 * copy, this was fixed later so an explicit check no longer needed.
	 */

	/* User totalLength overrides a computed length, but we'll check
	 * every copy in the copy loop.  Note that duk_to_int() can
	 * technically have arbitrary side effects so we need to recheck
	 * the buffers in the copy loop.
	 */
	if (!duk_is_undefined(thr, 1) && n > 0) {
		/* For n == 0, Node.js ignores totalLength argument and
		 * returns a zero length buffer.
		 */
		duk_int_t total_length_signed;
		total_length_signed = duk_to_int(thr, 1);
		if (total_length_signed < 0) {
			DUK_DCERROR_RANGE_INVALID_ARGS(thr);
		}
		total_length = (duk_uint_t) total_length_signed;
	}

	h_bufres = duk_push_bufobj_raw(thr,
	                               DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_BUFOBJ |
	                                   DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_UINT8ARRAY),
	                               DUK_BIDX_NODEJS_BUFFER_PROTOTYPE);
	DUK_ASSERT(h_bufres != NULL);

	p = (duk_uint8_t *) duk_push_fixed_buffer_zero(thr,
	                                               total_length); /* must be zeroed, all bytes not necessarily written over */
	DUK_ASSERT(p != NULL);
	space_left = (duk_size_t) total_length;

	for (i = 0; i < n; i++) {
		DUK_ASSERT_TOP(thr, 4); /* [ array totalLength bufres buf ] */

		duk_get_prop_index(thr, 0, (duk_uarridx_t) i);
		h_bufobj = duk__require_bufobj_value(thr, 4);
		DUK_ASSERT(h_bufobj != NULL);

		copy_size = h_bufobj->length;
		if (copy_size > space_left) {
			copy_size = space_left;
		}

		if (h_bufobj->buf != NULL && DUK_HBUFOBJ_VALID_SLICE(h_bufobj)) {
			duk_memcpy_unsafe((void *) p, (const void *) DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_bufobj), copy_size);
		} else {
			/* Just skip, leaving zeroes in the result. */
			;
		}
		p += copy_size;
		space_left -= copy_size;

		duk_pop(thr);
	}

	h_val = duk_known_hbuffer(thr, -1);

	duk__set_bufobj_buffer(thr, h_bufres, h_val);
	h_bufres->is_typedarray = 1;
	DUK_HBUFOBJ_ASSERT_VALID(h_bufres);

	duk_pop(thr); /* pop plain buffer, now reachable through h_bufres */

	return 1; /* return h_bufres */
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Shared readfield and writefield methods
 *
 *  The readfield/writefield methods need support for endianness and field
 *  types.  All offsets are byte based so no offset shifting is needed.
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
/* Format of magic, bits:
 *   0...1: field type; 0=uint8, 1=uint16, 2=uint32, 3=float, 4=double, 5=unused, 6=unused, 7=unused
 *       3: endianness: 0=little, 1=big
 *       4: signed: 1=yes, 0=no
 *       5: typedarray: 1=yes, 0=no
 */
#define DUK__FLD_8BIT       0
#define DUK__FLD_16BIT      1
#define DUK__FLD_32BIT      2
#define DUK__FLD_FLOAT      3
#define DUK__FLD_DOUBLE     4
#define DUK__FLD_VARINT     5
#define DUK__FLD_BIGENDIAN  (1 << 3)
#define DUK__FLD_SIGNED     (1 << 4)
#define DUK__FLD_TYPEDARRAY (1 << 5)

/* XXX: split into separate functions for each field type? */
DUK_INTERNAL duk_ret_t duk_bi_buffer_readfield(duk_hthread *thr) {
	duk_small_uint_t magic = (duk_small_uint_t) duk_get_current_magic(thr);
	duk_small_uint_t magic_ftype;
	duk_small_uint_t magic_bigendian;
	duk_small_uint_t magic_signed;
	duk_small_uint_t magic_typedarray;
	duk_small_uint_t endswap;
	duk_hbufobj *h_this;
	duk_bool_t no_assert;
	duk_int_t offset_signed;
	duk_uint_t offset;
	duk_uint_t buffer_length;
	duk_uint_t check_length;
	duk_uint8_t *buf;
	duk_double_union du;

	magic_ftype = magic & 0x0007U;
	magic_bigendian = magic & 0x0008U;
	magic_signed = magic & 0x0010U;
	magic_typedarray = magic & 0x0020U;

	h_this = duk__require_bufobj_this(thr); /* XXX: very inefficient for plain buffers */
	DUK_ASSERT(h_this != NULL);
	buffer_length = h_this->length;

	/* [ offset noAssert                 ], when ftype != DUK__FLD_VARINT */
	/* [ offset fieldByteLength noAssert ], when ftype == DUK__FLD_VARINT */
	/* [ offset littleEndian             ], when DUK__FLD_TYPEDARRAY (regardless of ftype) */

	/* Handle TypedArray vs. Node.js Buffer arg differences */
	if (magic_typedarray) {
		no_assert = 0;
#if defined(DUK_USE_INTEGER_LE)
		endswap = !duk_to_boolean(thr, 1); /* 1=little endian */
#else
		endswap = duk_to_boolean(thr, 1); /* 1=little endian */
#endif
	} else {
		no_assert = duk_to_boolean(thr, (magic_ftype == DUK__FLD_VARINT) ? 2 : 1);
#if defined(DUK_USE_INTEGER_LE)
		endswap = magic_bigendian;
#else
		endswap = !magic_bigendian;
#endif
	}

	/* Offset is coerced first to signed integer range and then to unsigned.
	 * This ensures we can add a small byte length (1-8) to the offset in
	 * bound checks and not wrap.
	 */
	offset_signed = duk_to_int(thr, 0);
	offset = (duk_uint_t) offset_signed;
	if (offset_signed < 0) {
		goto fail_bounds;
	}

	DUK_DDD(DUK_DDDPRINT("readfield, buffer_length=%ld, offset=%ld, no_assert=%d, "
	                     "magic=%04x, magic_fieldtype=%d, magic_bigendian=%d, magic_signed=%d, "
	                     "endswap=%u",
	                     (long) buffer_length,
	                     (long) offset,
	                     (int) no_assert,
	                     (unsigned int) magic,
	                     (int) magic_ftype,
	                     (int) (magic_bigendian >> 3),
	                     (int) (magic_signed >> 4),
	                     (int) endswap));

	/* Update 'buffer_length' to be the effective, safe limit which
	 * takes into account the underlying buffer.  This value will be
	 * potentially invalidated by any side effect.
	 */
	check_length = DUK_HBUFOBJ_CLAMP_BYTELENGTH(h_this, buffer_length);
	DUK_DDD(DUK_DDDPRINT("buffer_length=%ld, check_length=%ld", (long) buffer_length, (long) check_length));

	if (h_this->buf) {
		buf = DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_this);
	} else {
		/* Neutered.  We could go into the switch-case safely with
		 * buf == NULL because check_length == 0.  To avoid scanbuild
		 * warnings, fail directly instead.
		 */
		DUK_ASSERT(check_length == 0);
		goto fail_neutered;
	}
	DUK_ASSERT(buf != NULL);

	switch (magic_ftype) {
	case DUK__FLD_8BIT: {
		duk_uint8_t tmp;
		if (offset + 1U > check_length) {
			goto fail_bounds;
		}
		tmp = buf[offset];
		if (magic_signed) {
			duk_push_int(thr, (duk_int_t) ((duk_int8_t) tmp));
		} else {
			duk_push_uint(thr, (duk_uint_t) tmp);
		}
		break;
	}
	case DUK__FLD_16BIT: {
		duk_uint16_t tmp;
		if (offset + 2U > check_length) {
			goto fail_bounds;
		}
		duk_memcpy((void *) du.uc, (const void *) (buf + offset), 2);
		tmp = du.us[0];
		if (endswap) {
			tmp = DUK_BSWAP16(tmp);
		}
		if (magic_signed) {
			duk_push_int(thr, (duk_int_t) ((duk_int16_t) tmp));
		} else {
			duk_push_uint(thr, (duk_uint_t) tmp);
		}
		break;
	}
	case DUK__FLD_32BIT: {
		duk_uint32_t tmp;
		if (offset + 4U > check_length) {
			goto fail_bounds;
		}
		duk_memcpy((void *) du.uc, (const void *) (buf + offset), 4);
		tmp = du.ui[0];
		if (endswap) {
			tmp = DUK_BSWAP32(tmp);
		}
		if (magic_signed) {
			duk_push_int(thr, (duk_int_t) ((duk_int32_t) tmp));
		} else {
			duk_push_uint(thr, (duk_uint_t) tmp);
		}
		break;
	}
	case DUK__FLD_FLOAT: {
		duk_uint32_t tmp;
		if (offset + 4U > check_length) {
			goto fail_bounds;
		}
		duk_memcpy((void *) du.uc, (const void *) (buf + offset), 4);
		if (endswap) {
			tmp = du.ui[0];
			tmp = DUK_BSWAP32(tmp);
			du.ui[0] = tmp;
		}
		duk_push_number(thr, (duk_double_t) du.f[0]);
		break;
	}
	case DUK__FLD_DOUBLE: {
		if (offset + 8U > check_length) {
			goto fail_bounds;
		}
		duk_memcpy((void *) du.uc, (const void *) (buf + offset), 8);
		if (endswap) {
			DUK_DBLUNION_BSWAP64(&du);
		}
		duk_push_number(thr, (duk_double_t) du.d);
		break;
	}
	case DUK__FLD_VARINT: {
		/* Node.js Buffer variable width integer field.  We don't really
		 * care about speed here, so aim for shortest algorithm.
		 */
		duk_int_t field_bytelen;
		duk_int_t i, i_step, i_end;
#if defined(DUK_USE_64BIT_OPS)
		duk_int64_t tmp;
		duk_small_uint_t shift_tmp;
#else
		duk_double_t tmp;
		duk_small_int_t highbyte;
#endif
		const duk_uint8_t *p;

		field_bytelen = duk_get_int(thr, 1); /* avoid side effects! */
		if (field_bytelen < 1 || field_bytelen > 6) {
			goto fail_field_length;
		}
		if (offset + (duk_uint_t) field_bytelen > check_length) {
			goto fail_bounds;
		}
		p = (const duk_uint8_t *) (buf + offset);

		/* Slow gathering of value using either 64-bit arithmetic
		 * or IEEE doubles if 64-bit types not available.  Handling
		 * of negative numbers is a bit non-obvious in both cases.
		 */

		if (magic_bigendian) {
			/* Gather in big endian */
			i = 0;
			i_step = 1;
			i_end = field_bytelen; /* one i_step over */
		} else {
			/* Gather in little endian */
			i = field_bytelen - 1;
			i_step = -1;
			i_end = -1; /* one i_step over */
		}

#if defined(DUK_USE_64BIT_OPS)
		tmp = 0;
		do {
			DUK_ASSERT(i >= 0 && i < field_bytelen);
			tmp = (tmp << 8) + (duk_int64_t) p[i];
			i += i_step;
		} while (i != i_end);

		if (magic_signed) {
			/* Shift to sign extend.  Left shift must be unsigned
			 * to avoid undefined behavior; right shift must be
			 * signed to sign extend properly.
			 */
			shift_tmp = (duk_small_uint_t) (64U - (duk_small_uint_t) field_bytelen * 8U);
			tmp = (duk_int64_t) ((duk_uint64_t) tmp << shift_tmp) >> shift_tmp;
		}

		duk_push_i64(thr, tmp);
#else
		highbyte = p[i];
		if (magic_signed && (highbyte & 0x80) != 0) {
			/* 0xff => 255 - 256 = -1; 0x80 => 128 - 256 = -128 */
			tmp = (duk_double_t) (highbyte - 256);
		} else {
			tmp = (duk_double_t) highbyte;
		}
		for (;;) {
			i += i_step;
			if (i == i_end) {
				break;
			}
			DUK_ASSERT(i >= 0 && i < field_bytelen);
			tmp = (tmp * 256.0) + (duk_double_t) p[i];
		}

		duk_push_number(thr, tmp);
#endif
		break;
	}
	default: { /* should never happen but default here */
		goto fail_bounds;
	}
	}

	return 1;

fail_neutered:
fail_field_length:
fail_bounds:
	if (no_assert) {
		/* Node.js return value for noAssert out-of-bounds reads is
		 * usually (but not always) NaN.  Return NaN consistently.
		 */
		duk_push_nan(thr);
		return 1;
	}
	DUK_DCERROR_RANGE_INVALID_ARGS(thr);
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
/* XXX: split into separate functions for each field type? */
DUK_INTERNAL duk_ret_t duk_bi_buffer_writefield(duk_hthread *thr) {
	duk_small_uint_t magic = (duk_small_uint_t) duk_get_current_magic(thr);
	duk_small_uint_t magic_ftype;
	duk_small_uint_t magic_bigendian;
	duk_small_uint_t magic_signed;
	duk_small_uint_t magic_typedarray;
	duk_small_uint_t endswap;
	duk_hbufobj *h_this;
	duk_bool_t no_assert;
	duk_int_t offset_signed;
	duk_uint_t offset;
	duk_uint_t buffer_length;
	duk_uint_t check_length;
	duk_uint8_t *buf;
	duk_double_union du;
	duk_int_t nbytes = 0;

	magic_ftype = magic & 0x0007U;
	magic_bigendian = magic & 0x0008U;
	magic_signed = magic & 0x0010U;
	magic_typedarray = magic & 0x0020U;
	DUK_UNREF(magic_signed);

	h_this = duk__require_bufobj_this(thr); /* XXX: very inefficient for plain buffers */
	DUK_ASSERT(h_this != NULL);
	buffer_length = h_this->length;

	/* [ value  offset noAssert                 ], when ftype != DUK__FLD_VARINT */
	/* [ value  offset fieldByteLength noAssert ], when ftype == DUK__FLD_VARINT */
	/* [ offset value  littleEndian             ], when DUK__FLD_TYPEDARRAY (regardless of ftype) */

	/* Handle TypedArray vs. Node.js Buffer arg differences */
	if (magic_typedarray) {
		no_assert = 0;
#if defined(DUK_USE_INTEGER_LE)
		endswap = !duk_to_boolean(thr, 2); /* 1=little endian */
#else
		endswap = duk_to_boolean(thr, 2); /* 1=little endian */
#endif
		duk_swap(thr, 0, 1); /* offset/value order different from Node.js */
	} else {
		no_assert = duk_to_boolean(thr, (magic_ftype == DUK__FLD_VARINT) ? 3 : 2);
#if defined(DUK_USE_INTEGER_LE)
		endswap = magic_bigendian;
#else
		endswap = !magic_bigendian;
#endif
	}

	/* Offset is coerced first to signed integer range and then to unsigned.
	 * This ensures we can add a small byte length (1-8) to the offset in
	 * bound checks and not wrap.
	 */
	offset_signed = duk_to_int(thr, 1);
	offset = (duk_uint_t) offset_signed;

	/* We need 'nbytes' even for a failed offset; return value must be
	 * (offset + nbytes) even when write fails due to invalid offset.
	 */
	if (magic_ftype != DUK__FLD_VARINT) {
		DUK_ASSERT(magic_ftype < (duk_small_uint_t) (sizeof(duk__buffer_nbytes_from_fldtype) / sizeof(duk_uint8_t)));
		nbytes = duk__buffer_nbytes_from_fldtype[magic_ftype];
	} else {
		nbytes = duk_get_int(thr, 2);
		if (nbytes < 1 || nbytes > 6) {
			goto fail_field_length;
		}
	}
	DUK_ASSERT(nbytes >= 1 && nbytes <= 8);

	/* Now we can check offset validity. */
	if (offset_signed < 0) {
		goto fail_bounds;
	}

	DUK_DDD(DUK_DDDPRINT("writefield, value=%!T, buffer_length=%ld, offset=%ld, no_assert=%d, "
	                     "magic=%04x, magic_fieldtype=%d, magic_bigendian=%d, magic_signed=%d, "
	                     "endswap=%u",
	                     duk_get_tval(thr, 0),
	                     (long) buffer_length,
	                     (long) offset,
	                     (int) no_assert,
	                     (unsigned int) magic,
	                     (int) magic_ftype,
	                     (int) (magic_bigendian >> 3),
	                     (int) (magic_signed >> 4),
	                     (int) endswap));

	/* Coerce value to a number before computing check_length, so that
	 * the field type specific coercion below can't have side effects
	 * that would invalidate check_length.
	 */
	duk_to_number(thr, 0);

	/* Update 'buffer_length' to be the effective, safe limit which
	 * takes into account the underlying buffer.  This value will be
	 * potentially invalidated by any side effect.
	 */
	check_length = DUK_HBUFOBJ_CLAMP_BYTELENGTH(h_this, buffer_length);
	DUK_DDD(DUK_DDDPRINT("buffer_length=%ld, check_length=%ld", (long) buffer_length, (long) check_length));

	if (h_this->buf) {
		buf = DUK_HBUFOBJ_GET_SLICE_BASE(thr->heap, h_this);
	} else {
		/* Neutered.  We could go into the switch-case safely with
		 * buf == NULL because check_length == 0.  To avoid scanbuild
		 * warnings, fail directly instead.
		 */
		DUK_ASSERT(check_length == 0);
		goto fail_neutered;
	}
	DUK_ASSERT(buf != NULL);

	switch (magic_ftype) {
	case DUK__FLD_8BIT: {
		if (offset + 1U > check_length) {
			goto fail_bounds;
		}
		/* sign doesn't matter when writing */
		buf[offset] = (duk_uint8_t) duk_to_uint32(thr, 0);
		break;
	}
	case DUK__FLD_16BIT: {
		duk_uint16_t tmp;
		if (offset + 2U > check_length) {
			goto fail_bounds;
		}
		tmp = (duk_uint16_t) duk_to_uint32(thr, 0);
		if (endswap) {
			tmp = DUK_BSWAP16(tmp);
		}
		du.us[0] = tmp;
		/* sign doesn't matter when writing */
		duk_memcpy((void *) (buf + offset), (const void *) du.uc, 2);
		break;
	}
	case DUK__FLD_32BIT: {
		duk_uint32_t tmp;
		if (offset + 4U > check_length) {
			goto fail_bounds;
		}
		tmp = (duk_uint32_t) duk_to_uint32(thr, 0);
		if (endswap) {
			tmp = DUK_BSWAP32(tmp);
		}
		du.ui[0] = tmp;
		/* sign doesn't matter when writing */
		duk_memcpy((void *) (buf + offset), (const void *) du.uc, 4);
		break;
	}
	case DUK__FLD_FLOAT: {
		duk_uint32_t tmp;
		if (offset + 4U > check_length) {
			goto fail_bounds;
		}
		du.f[0] = (duk_float_t) duk_to_number(thr, 0);
		if (endswap) {
			tmp = du.ui[0];
			tmp = DUK_BSWAP32(tmp);
			du.ui[0] = tmp;
		}
		/* sign doesn't matter when writing */
		duk_memcpy((void *) (buf + offset), (const void *) du.uc, 4);
		break;
	}
	case DUK__FLD_DOUBLE: {
		if (offset + 8U > check_length) {
			goto fail_bounds;
		}
		du.d = (duk_double_t) duk_to_number(thr, 0);
		if (endswap) {
			DUK_DBLUNION_BSWAP64(&du);
		}
		/* sign doesn't matter when writing */
		duk_memcpy((void *) (buf + offset), (const void *) du.uc, 8);
		break;
	}
	case DUK__FLD_VARINT: {
		/* Node.js Buffer variable width integer field.  We don't really
		 * care about speed here, so aim for shortest algorithm.
		 */
		duk_int_t field_bytelen;
		duk_int_t i, i_step, i_end;
#if defined(DUK_USE_64BIT_OPS)
		duk_int64_t tmp;
#else
		duk_double_t tmp;
#endif
		duk_uint8_t *p;

		field_bytelen = (duk_int_t) nbytes;
		if (offset + (duk_uint_t) field_bytelen > check_length) {
			goto fail_bounds;
		}

		/* Slow writing of value using either 64-bit arithmetic
		 * or IEEE doubles if 64-bit types not available.  There's
		 * no special sign handling when writing varints.
		 */

		if (magic_bigendian) {
			/* Write in big endian */
			i = field_bytelen; /* one i_step added at top of loop */
			i_step = -1;
			i_end = 0;
		} else {
			/* Write in little endian */
			i = -1; /* one i_step added at top of loop */
			i_step = 1;
			i_end = field_bytelen - 1;
		}

		/* XXX: The duk_to_number() cast followed by integer coercion
		 * is platform specific so NaN, +/- Infinity, and out-of-bounds
		 * values result in platform specific output now.
		 * See: test-bi-nodejs-buffer-proto-varint-special.js
		 */

#if defined(DUK_USE_64BIT_OPS)
		tmp = (duk_int64_t) duk_to_number(thr, 0);
		p = (duk_uint8_t *) (buf + offset);
		do {
			i += i_step;
			DUK_ASSERT(i >= 0 && i < field_bytelen);
			p[i] = (duk_uint8_t) (tmp & 0xff);
			tmp = tmp >> 8; /* unnecessary shift for last byte */
		} while (i != i_end);
#else
		tmp = duk_to_number(thr, 0);
		p = (duk_uint8_t *) (buf + offset);
		do {
			i += i_step;
			tmp = DUK_FLOOR(tmp);
			DUK_ASSERT(i >= 0 && i < field_bytelen);
			p[i] = (duk_uint8_t) (DUK_FMOD(tmp, 256.0));
			tmp = tmp / 256.0; /* unnecessary div for last byte */
		} while (i != i_end);
#endif
		break;
	}
	default: { /* should never happen but default here */
		goto fail_bounds;
	}
	}

	/* Node.js Buffer: return offset + #bytes written (i.e. next
	 * write offset).
	 */
	if (magic_typedarray) {
		/* For TypedArrays 'undefined' return value is specified
		 * by ES2015 (matches V8).
		 */
		return 0;
	}
	duk_push_uint(thr, offset + (duk_uint_t) nbytes);
	return 1;

fail_neutered:
fail_field_length:
fail_bounds:
	if (no_assert) {
		/* Node.js return value for failed writes is offset + #bytes
		 * that would have been written.
		 */
		/* XXX: for negative input offsets, 'offset' will be a large
		 * positive value so the result here is confusing.
		 */
		if (magic_typedarray) {
			return 0;
		}
		duk_push_uint(thr, offset + (duk_uint_t) nbytes);
		return 1;
	}
	DUK_DCERROR_RANGE_INVALID_ARGS(thr);
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  Accessors for .buffer, .byteLength, .byteOffset
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_LOCAL duk_hbufobj *duk__autospawn_arraybuffer(duk_hthread *thr, duk_hbuffer *h_buf) {
	duk_hbufobj *h_res;

	h_res = duk_push_bufobj_raw(thr,
	                            DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_BUFOBJ |
	                                DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_ARRAYBUFFER),
	                            DUK_BIDX_ARRAYBUFFER_PROTOTYPE);
	DUK_ASSERT(h_res != NULL);
	DUK_UNREF(h_res);

	duk__set_bufobj_buffer(thr, h_res, h_buf);
	DUK_HBUFOBJ_ASSERT_VALID(h_res);
	DUK_ASSERT(h_res->buf_prop == NULL);
	return h_res;
}

DUK_INTERNAL duk_ret_t duk_bi_typedarray_buffer_getter(duk_hthread *thr) {
	duk_hbufobj *h_bufobj;

	h_bufobj = (duk_hbufobj *) duk__getrequire_bufobj_this(thr, DUK__BUFOBJ_FLAG_THROW /*flags*/);
	DUK_ASSERT(h_bufobj != NULL);
	if (DUK_HEAPHDR_IS_BUFFER((duk_heaphdr *) h_bufobj)) {
		DUK_DD(DUK_DDPRINT("autospawn ArrayBuffer for plain buffer"));
		(void) duk__autospawn_arraybuffer(thr, (duk_hbuffer *) h_bufobj);
		return 1;
	} else {
		if (h_bufobj->buf_prop == NULL &&
		    DUK_HOBJECT_GET_CLASS_NUMBER((duk_hobject *) h_bufobj) != DUK_HOBJECT_CLASS_ARRAYBUFFER &&
		    h_bufobj->buf != NULL) {
			duk_hbufobj *h_arrbuf;

			DUK_DD(DUK_DDPRINT("autospawn ArrayBuffer for typed array or DataView"));
			h_arrbuf = duk__autospawn_arraybuffer(thr, h_bufobj->buf);

			if (h_bufobj->buf_prop == NULL) {
				/* Must recheck buf_prop, in case ArrayBuffer
				 * alloc had a side effect which already filled
				 * it!
				 */

				/* Set ArrayBuffer's .byteOffset and .byteLength based
				 * on the view so that Arraybuffer[view.byteOffset]
				 * matches view[0].
				 */
				h_arrbuf->offset = 0;
				DUK_ASSERT(h_bufobj->offset + h_bufobj->length >= h_bufobj->offset); /* Wrap check on creation. */
				h_arrbuf->length = h_bufobj->offset + h_bufobj->length;
				DUK_ASSERT(h_arrbuf->buf_prop == NULL);

				DUK_ASSERT(h_bufobj->buf_prop == NULL);
				h_bufobj->buf_prop = (duk_hobject *) h_arrbuf;
				DUK_HBUFOBJ_INCREF(thr, h_arrbuf); /* Now reachable and accounted for. */
			}

			/* Left on stack; pushed for the second time below (OK). */
		}
		if (h_bufobj->buf_prop) {
			duk_push_hobject(thr, h_bufobj->buf_prop);
			return 1;
		}
	}
	return 0;
}

DUK_INTERNAL duk_ret_t duk_bi_typedarray_byteoffset_getter(duk_hthread *thr) {
	duk_hbufobj *h_bufobj;

	h_bufobj = (duk_hbufobj *) duk__getrequire_bufobj_this(thr, DUK__BUFOBJ_FLAG_THROW /*flags*/);
	DUK_ASSERT(h_bufobj != NULL);
	if (DUK_HEAPHDR_IS_BUFFER((duk_heaphdr *) h_bufobj)) {
		duk_push_uint(thr, 0);
	} else {
		/* If neutered must return 0; offset is zeroed during
		 * neutering.
		 */
		duk_push_uint(thr, h_bufobj->offset);
	}
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_typedarray_bytelength_getter(duk_hthread *thr) {
	duk_hbufobj *h_bufobj;

	h_bufobj = (duk_hbufobj *) duk__getrequire_bufobj_this(thr, DUK__BUFOBJ_FLAG_THROW /*flags*/);
	DUK_ASSERT(h_bufobj != NULL);
	if (DUK_HEAPHDR_IS_BUFFER((duk_heaphdr *) h_bufobj)) {
		duk_hbuffer *h_buf;

		h_buf = (duk_hbuffer *) h_bufobj;
		DUK_ASSERT(DUK_HBUFFER_GET_SIZE(h_buf) <= DUK_UINT_MAX); /* Buffer limits. */
		duk_push_uint(thr, (duk_uint_t) DUK_HBUFFER_GET_SIZE(h_buf));
	} else {
		/* If neutered must return 0; length is zeroed during
		 * neutering.
		 */
		duk_push_uint(thr, h_bufobj->length);
	}
	return 1;
}
#else /* DUK_USE_BUFFEROBJECT_SUPPORT */
/* No .buffer getter without ArrayBuffer support. */
#if 0
DUK_INTERNAL duk_ret_t duk_bi_typedarray_buffer_getter(duk_hthread *thr) {
	return 0;
}
#endif

DUK_INTERNAL duk_ret_t duk_bi_typedarray_byteoffset_getter(duk_hthread *thr) {
	duk_push_uint(thr, 0);
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_typedarray_bytelength_getter(duk_hthread *thr) {
	duk_hbuffer *h_buf;

	/* XXX: helper? */
	duk_push_this(thr);
	h_buf = duk_require_hbuffer(thr, -1);
	duk_push_uint(thr, DUK_HBUFFER_GET_SIZE(h_buf));
	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */
