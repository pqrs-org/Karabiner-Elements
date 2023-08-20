/*
 *  duk_hbuffer allocation and freeing.
 */

#include "duk_internal.h"

/* Allocate a new duk_hbuffer of a certain type and return a pointer to it
 * (NULL on error).  Write buffer data pointer to 'out_bufdata' (only if
 * allocation successful).
 */
DUK_INTERNAL duk_hbuffer *duk_hbuffer_alloc(duk_heap *heap, duk_size_t size, duk_small_uint_t flags, void **out_bufdata) {
	duk_hbuffer *res = NULL;
	duk_size_t header_size;
	duk_size_t alloc_size;

	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(out_bufdata != NULL);

	DUK_DDD(DUK_DDDPRINT("allocate hbuffer"));

	/* Size sanity check.  Should not be necessary because caller is
	 * required to check this, but we don't want to cause a segfault
	 * if the size wraps either in duk_size_t computation or when
	 * storing the size in a 16-bit field.
	 */
	if (size > DUK_HBUFFER_MAX_BYTELEN) {
		DUK_D(DUK_DPRINT("hbuffer alloc failed: size too large: %ld", (long) size));
		return NULL; /* no need to write 'out_bufdata' */
	}

	if (flags & DUK_BUF_FLAG_EXTERNAL) {
		header_size = sizeof(duk_hbuffer_external);
		alloc_size = sizeof(duk_hbuffer_external);
	} else if (flags & DUK_BUF_FLAG_DYNAMIC) {
		header_size = sizeof(duk_hbuffer_dynamic);
		alloc_size = sizeof(duk_hbuffer_dynamic);
	} else {
		header_size = sizeof(duk_hbuffer_fixed);
		alloc_size = sizeof(duk_hbuffer_fixed) + size;
		DUK_ASSERT(alloc_size >= sizeof(duk_hbuffer_fixed)); /* no wrapping */
	}

	res = (duk_hbuffer *) DUK_ALLOC(heap, alloc_size);
	if (DUK_UNLIKELY(res == NULL)) {
		goto alloc_error;
	}

	/* zero everything unless requested not to do so */
#if defined(DUK_USE_ZERO_BUFFER_DATA)
	duk_memzero((void *) res, (flags & DUK_BUF_FLAG_NOZERO) ? header_size : alloc_size);
#else
	duk_memzero((void *) res, header_size);
#endif

	if (flags & DUK_BUF_FLAG_EXTERNAL) {
		duk_hbuffer_external *h;
		h = (duk_hbuffer_external *) res;
		DUK_UNREF(h);
		*out_bufdata = NULL;
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
#if defined(DUK_USE_HEAPPTR16)
/* the compressed pointer is zeroed which maps to NULL, so nothing to do. */
#else
		DUK_HBUFFER_EXTERNAL_SET_DATA_PTR(heap, h, NULL);
#endif
#endif
		DUK_ASSERT(DUK_HBUFFER_EXTERNAL_GET_DATA_PTR(heap, h) == NULL);
	} else if (flags & DUK_BUF_FLAG_DYNAMIC) {
		duk_hbuffer_dynamic *h = (duk_hbuffer_dynamic *) res;
		void *ptr;

		if (size > 0) {
			DUK_ASSERT(!(flags & DUK_BUF_FLAG_EXTERNAL)); /* alloc external with size zero */
			DUK_DDD(DUK_DDDPRINT("dynamic buffer with nonzero size, alloc actual buffer"));
#if defined(DUK_USE_ZERO_BUFFER_DATA)
			ptr = DUK_ALLOC_ZEROED(heap, size);
#else
			ptr = DUK_ALLOC(heap, size);
#endif
			if (DUK_UNLIKELY(ptr == NULL)) {
				/* Because size > 0, NULL check is correct */
				goto alloc_error;
			}
			*out_bufdata = ptr;

			DUK_HBUFFER_DYNAMIC_SET_DATA_PTR(heap, h, ptr);
		} else {
			*out_bufdata = NULL;
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
#if defined(DUK_USE_HEAPPTR16)
/* the compressed pointer is zeroed which maps to NULL, so nothing to do. */
#else
			DUK_HBUFFER_DYNAMIC_SET_DATA_PTR(heap, h, NULL);
#endif
#endif
			DUK_ASSERT(DUK_HBUFFER_DYNAMIC_GET_DATA_PTR(heap, h) == NULL);
		}
	} else {
		*out_bufdata = (void *) ((duk_hbuffer_fixed *) (void *) res + 1);
	}

	DUK_HBUFFER_SET_SIZE(res, size);

	DUK_HEAPHDR_SET_TYPE(&res->hdr, DUK_HTYPE_BUFFER);
	if (flags & DUK_BUF_FLAG_DYNAMIC) {
		DUK_HBUFFER_SET_DYNAMIC(res);
		if (flags & DUK_BUF_FLAG_EXTERNAL) {
			DUK_HBUFFER_SET_EXTERNAL(res);
		}
	} else {
		DUK_ASSERT(!(flags & DUK_BUF_FLAG_EXTERNAL));
	}
	DUK_HEAP_INSERT_INTO_HEAP_ALLOCATED(heap, &res->hdr);

	DUK_DDD(DUK_DDDPRINT("allocated hbuffer: %p", (void *) res));
	return res;

alloc_error:
	DUK_DD(DUK_DDPRINT("hbuffer allocation failed"));

	DUK_FREE(heap, res);
	return NULL; /* no need to write 'out_bufdata' */
}

/* For indirect allocs. */

DUK_INTERNAL void *duk_hbuffer_get_dynalloc_ptr(duk_heap *heap, void *ud) {
	duk_hbuffer_dynamic *buf = (duk_hbuffer_dynamic *) ud;
	DUK_UNREF(heap);
	return (void *) DUK_HBUFFER_DYNAMIC_GET_DATA_PTR(heap, buf);
}
