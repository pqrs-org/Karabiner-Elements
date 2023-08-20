/*
 *  Memory allocation handling.
 */

#include "duk_internal.h"

/*
 *  Allocate memory with garbage collection.
 */

/* Slow path: voluntary GC triggered, first alloc attempt failed, or zero size. */
DUK_LOCAL DUK_NOINLINE_PERF DUK_COLD void *duk__heap_mem_alloc_slowpath(duk_heap *heap, duk_size_t size) {
	void *res;
	duk_small_int_t i;

	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(heap->alloc_func != NULL);
	DUK_ASSERT_DISABLE(size >= 0);

	if (size == 0) {
		DUK_D(DUK_DPRINT("zero size alloc in slow path, return NULL"));
		return NULL;
	}

	DUK_D(DUK_DPRINT("first alloc attempt failed or voluntary GC limit reached, attempt to gc and retry"));

#if 0
	/*
	 *  If GC is already running there is no point in attempting a GC
	 *  because it will be skipped.  This could be checked for explicitly,
	 *  but it isn't actually needed: the loop below will eventually
	 *  fail resulting in a NULL.
	 */

	if (heap->ms_prevent_count != 0) {
		DUK_D(DUK_DPRINT("duk_heap_mem_alloc() failed, gc in progress (gc skipped), alloc size %ld", (long) size));
		return NULL;
	}
#endif

	/*
	 *  Retry with several GC attempts.  Initial attempts are made without
	 *  emergency mode; later attempts use emergency mode which minimizes
	 *  memory allocations forcibly.
	 */

	for (i = 0; i < DUK_HEAP_ALLOC_FAIL_MARKANDSWEEP_LIMIT; i++) {
		duk_small_uint_t flags;

		flags = 0;
		if (i >= DUK_HEAP_ALLOC_FAIL_MARKANDSWEEP_EMERGENCY_LIMIT - 1) {
			flags |= DUK_MS_FLAG_EMERGENCY;
		}

		duk_heap_mark_and_sweep(heap, flags);

		DUK_ASSERT(size > 0);
		res = heap->alloc_func(heap->heap_udata, size);
		if (res != NULL) {
			DUK_D(DUK_DPRINT("duk_heap_mem_alloc() succeeded after gc (pass %ld), alloc size %ld",
			                 (long) (i + 1),
			                 (long) size));
			return res;
		}
	}

	DUK_D(DUK_DPRINT("duk_heap_mem_alloc() failed even after gc, alloc size %ld", (long) size));
	return NULL;
}

DUK_INTERNAL DUK_INLINE_PERF DUK_HOT void *duk_heap_mem_alloc(duk_heap *heap, duk_size_t size) {
	void *res;

	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(heap->alloc_func != NULL);
	DUK_ASSERT_DISABLE(size >= 0);

#if defined(DUK_USE_VOLUNTARY_GC)
	/* Voluntary periodic GC (if enabled). */
	if (DUK_UNLIKELY(--(heap)->ms_trigger_counter < 0)) {
		goto slowpath;
	}
#endif

#if defined(DUK_USE_GC_TORTURE)
	/* Simulate alloc failure on every alloc, except when mark-and-sweep
	 * is running.
	 */
	if (heap->ms_prevent_count == 0) {
		DUK_DDD(DUK_DDDPRINT("gc torture enabled, pretend that first alloc attempt fails"));
		res = NULL;
		DUK_UNREF(res);
		goto slowpath;
	}
#endif

	/* Zero-size allocation should happen very rarely (if at all), so
	 * don't check zero size on NULL; handle it in the slow path
	 * instead.  This reduces size of inlined code.
	 */
	res = heap->alloc_func(heap->heap_udata, size);
	if (DUK_LIKELY(res != NULL)) {
		return res;
	}

slowpath:

	if (size == 0) {
		DUK_D(DUK_DPRINT("first alloc attempt returned NULL for zero size alloc, use slow path to deal with it"));
	} else {
		DUK_D(DUK_DPRINT("first alloc attempt failed, attempt to gc and retry"));
	}
	return duk__heap_mem_alloc_slowpath(heap, size);
}

DUK_INTERNAL DUK_INLINE_PERF DUK_HOT void *duk_heap_mem_alloc_zeroed(duk_heap *heap, duk_size_t size) {
	void *res;

	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(heap->alloc_func != NULL);
	DUK_ASSERT_DISABLE(size >= 0);

	res = DUK_ALLOC(heap, size);
	if (DUK_LIKELY(res != NULL)) {
		duk_memzero(res, size);
	}
	return res;
}

DUK_INTERNAL DUK_INLINE_PERF DUK_HOT void *duk_heap_mem_alloc_checked(duk_hthread *thr, duk_size_t size) {
	void *res;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(thr->heap->alloc_func != NULL);

	res = duk_heap_mem_alloc(thr->heap, size);
	if (DUK_LIKELY(res != NULL)) {
		return res;
	} else if (size == 0) {
		DUK_ASSERT(res == NULL);
		return res;
	}
	DUK_ERROR_ALLOC_FAILED(thr);
	DUK_WO_NORETURN(return NULL;);
}

DUK_INTERNAL DUK_INLINE_PERF DUK_HOT void *duk_heap_mem_alloc_checked_zeroed(duk_hthread *thr, duk_size_t size) {
	void *res;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(thr->heap->alloc_func != NULL);

	res = duk_heap_mem_alloc(thr->heap, size);
	if (DUK_LIKELY(res != NULL)) {
		duk_memzero(res, size);
		return res;
	} else if (size == 0) {
		DUK_ASSERT(res == NULL);
		return res;
	}
	DUK_ERROR_ALLOC_FAILED(thr);
	DUK_WO_NORETURN(return NULL;);
}

/*
 *  Reallocate memory with garbage collection.
 */

/* Slow path: voluntary GC triggered, first realloc attempt failed, or zero size. */
DUK_LOCAL DUK_NOINLINE_PERF DUK_COLD void *duk__heap_mem_realloc_slowpath(duk_heap *heap, void *ptr, duk_size_t newsize) {
	void *res;
	duk_small_int_t i;

	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(heap->realloc_func != NULL);
	/* ptr may be NULL */
	DUK_ASSERT_DISABLE(newsize >= 0);

	/* Unlike for malloc(), zero size NULL result check happens at the call site. */

	DUK_D(DUK_DPRINT("first realloc attempt failed, attempt to gc and retry"));

#if 0
	/*
	 *  Avoid a GC if GC is already running.  See duk_heap_mem_alloc().
	 */

	if (heap->ms_prevent_count != 0) {
		DUK_D(DUK_DPRINT("duk_heap_mem_realloc() failed, gc in progress (gc skipped), alloc size %ld", (long) newsize));
		return NULL;
	}
#endif

	/*
	 *  Retry with several GC attempts.  Initial attempts are made without
	 *  emergency mode; later attempts use emergency mode which minimizes
	 *  memory allocations forcibly.
	 */

	for (i = 0; i < DUK_HEAP_ALLOC_FAIL_MARKANDSWEEP_LIMIT; i++) {
		duk_small_uint_t flags;

		flags = 0;
		if (i >= DUK_HEAP_ALLOC_FAIL_MARKANDSWEEP_EMERGENCY_LIMIT - 1) {
			flags |= DUK_MS_FLAG_EMERGENCY;
		}

		duk_heap_mark_and_sweep(heap, flags);

		res = heap->realloc_func(heap->heap_udata, ptr, newsize);
		if (res != NULL || newsize == 0) {
			DUK_D(DUK_DPRINT("duk_heap_mem_realloc() succeeded after gc (pass %ld), alloc size %ld",
			                 (long) (i + 1),
			                 (long) newsize));
			return res;
		}
	}

	DUK_D(DUK_DPRINT("duk_heap_mem_realloc() failed even after gc, alloc size %ld", (long) newsize));
	return NULL;
}

DUK_INTERNAL DUK_INLINE_PERF DUK_HOT void *duk_heap_mem_realloc(duk_heap *heap, void *ptr, duk_size_t newsize) {
	void *res;

	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(heap->realloc_func != NULL);
	/* ptr may be NULL */
	DUK_ASSERT_DISABLE(newsize >= 0);

#if defined(DUK_USE_VOLUNTARY_GC)
	/* Voluntary periodic GC (if enabled). */
	if (DUK_UNLIKELY(--(heap)->ms_trigger_counter < 0)) {
		goto gc_retry;
	}
#endif

#if defined(DUK_USE_GC_TORTURE)
	/* Simulate alloc failure on every realloc, except when mark-and-sweep
	 * is running.
	 */
	if (heap->ms_prevent_count == 0) {
		DUK_DDD(DUK_DDDPRINT("gc torture enabled, pretend that first realloc attempt fails"));
		res = NULL;
		DUK_UNREF(res);
		goto gc_retry;
	}
#endif

	res = heap->realloc_func(heap->heap_udata, ptr, newsize);
	if (DUK_LIKELY(res != NULL) || newsize == 0) {
		if (res != NULL && newsize == 0) {
			DUK_DD(DUK_DDPRINT("first realloc attempt returned NULL for zero size realloc, accept and return NULL"));
		}
		return res;
	} else {
		goto gc_retry;
	}
	/* Never here. */

gc_retry:
	return duk__heap_mem_realloc_slowpath(heap, ptr, newsize);
}

/*
 *  Reallocate memory with garbage collection, using a callback to provide
 *  the current allocated pointer.  This variant is used when a mark-and-sweep
 *  (e.g. finalizers) might change the original pointer.
 */

/* Slow path: voluntary GC triggered, first realloc attempt failed, or zero size. */
DUK_LOCAL DUK_NOINLINE_PERF DUK_COLD void *duk__heap_mem_realloc_indirect_slowpath(duk_heap *heap,
                                                                                   duk_mem_getptr cb,
                                                                                   void *ud,
                                                                                   duk_size_t newsize) {
	void *res;
	duk_small_int_t i;

	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(heap->realloc_func != NULL);
	DUK_ASSERT_DISABLE(newsize >= 0);

	/* Unlike for malloc(), zero size NULL result check happens at the call site. */

	DUK_D(DUK_DPRINT("first indirect realloc attempt failed, attempt to gc and retry"));

#if 0
	/*
	 *  Avoid a GC if GC is already running.  See duk_heap_mem_alloc().
	 */

	if (heap->ms_prevent_count != 0) {
		DUK_D(DUK_DPRINT("duk_heap_mem_realloc_indirect() failed, gc in progress (gc skipped), alloc size %ld", (long) newsize));
		return NULL;
	}
#endif

	/*
	 *  Retry with several GC attempts.  Initial attempts are made without
	 *  emergency mode; later attempts use emergency mode which minimizes
	 *  memory allocations forcibly.
	 */

	for (i = 0; i < DUK_HEAP_ALLOC_FAIL_MARKANDSWEEP_LIMIT; i++) {
		duk_small_uint_t flags;

#if defined(DUK_USE_DEBUG)
		void *ptr_pre;
		void *ptr_post;
#endif

#if defined(DUK_USE_DEBUG)
		ptr_pre = cb(heap, ud);
#endif
		flags = 0;
		if (i >= DUK_HEAP_ALLOC_FAIL_MARKANDSWEEP_EMERGENCY_LIMIT - 1) {
			flags |= DUK_MS_FLAG_EMERGENCY;
		}

		duk_heap_mark_and_sweep(heap, flags);
#if defined(DUK_USE_DEBUG)
		ptr_post = cb(heap, ud);
		if (ptr_pre != ptr_post) {
			DUK_DD(DUK_DDPRINT("realloc base pointer changed by mark-and-sweep: %p -> %p",
			                   (void *) ptr_pre,
			                   (void *) ptr_post));
		}
#endif

		/* Note: key issue here is to re-lookup the base pointer on every attempt.
		 * The pointer being reallocated may change after every mark-and-sweep.
		 */

		res = heap->realloc_func(heap->heap_udata, cb(heap, ud), newsize);
		if (res != NULL || newsize == 0) {
			DUK_D(DUK_DPRINT("duk_heap_mem_realloc_indirect() succeeded after gc (pass %ld), alloc size %ld",
			                 (long) (i + 1),
			                 (long) newsize));
			return res;
		}
	}

	DUK_D(DUK_DPRINT("duk_heap_mem_realloc_indirect() failed even after gc, alloc size %ld", (long) newsize));
	return NULL;
}

DUK_INTERNAL DUK_INLINE_PERF DUK_HOT void *duk_heap_mem_realloc_indirect(duk_heap *heap,
                                                                         duk_mem_getptr cb,
                                                                         void *ud,
                                                                         duk_size_t newsize) {
	void *res;

	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(heap->realloc_func != NULL);
	DUK_ASSERT_DISABLE(newsize >= 0);

#if defined(DUK_USE_VOLUNTARY_GC)
	/* Voluntary periodic GC (if enabled). */
	if (DUK_UNLIKELY(--(heap)->ms_trigger_counter < 0)) {
		goto gc_retry;
	}
#endif

#if defined(DUK_USE_GC_TORTURE)
	/* Simulate alloc failure on every realloc, except when mark-and-sweep
	 * is running.
	 */
	if (heap->ms_prevent_count == 0) {
		DUK_DDD(DUK_DDDPRINT("gc torture enabled, pretend that first indirect realloc attempt fails"));
		res = NULL;
		DUK_UNREF(res);
		goto gc_retry;
	}
#endif

	res = heap->realloc_func(heap->heap_udata, cb(heap, ud), newsize);
	if (DUK_LIKELY(res != NULL) || newsize == 0) {
		if (res != NULL && newsize == 0) {
			DUK_DD(DUK_DDPRINT(
			    "first indirect realloc attempt returned NULL for zero size realloc, accept and return NULL"));
		}
		return res;
	} else {
		goto gc_retry;
	}
	/* Never here. */

gc_retry:
	return duk__heap_mem_realloc_indirect_slowpath(heap, cb, ud, newsize);
}

/*
 *  Free memory
 */

DUK_INTERNAL DUK_INLINE_PERF DUK_HOT void duk_heap_mem_free(duk_heap *heap, void *ptr) {
	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(heap->free_func != NULL);
	/* ptr may be NULL */

	/* Must behave like a no-op with NULL and any pointer returned from
	 * malloc/realloc with zero size.
	 */
	heap->free_func(heap->heap_udata, ptr);

	/* Never perform a GC (even voluntary) in a memory free, otherwise
	 * all call sites doing frees would need to deal with the side effects.
	 * No need to update voluntary GC counter either.
	 */
}
