/*
 *  Reference counting implementation.
 *
 *  INCREF/DECREF, finalization and freeing of objects whose refcount reaches
 *  zero (refzero).  These operations are very performance sensitive, so
 *  various small tricks are used in an attempt to maximize speed.
 */

#include "duk_internal.h"

#if defined(DUK_USE_REFERENCE_COUNTING)

#if !defined(DUK_USE_DOUBLE_LINKED_HEAP)
#error internal error, reference counting requires a double linked heap
#endif

/*
 *  Heap object refcount finalization.
 *
 *  When an object is about to be freed, all other objects it refers to must
 *  be decref'd.  Refcount finalization does NOT free the object or its inner
 *  allocations (mark-and-sweep shares these helpers), it just manipulates
 *  the refcounts.
 *
 *  Note that any of the DECREFs may cause a refcount to drop to zero.  If so,
 *  the object won't be refzero processed inline, but will just be queued to
 *  refzero_list and processed by an earlier caller working on refzero_list,
 *  eliminating C recursion from even long refzero cascades.  If refzero
 *  finalization is triggered by mark-and-sweep, refzero conditions are ignored
 *  (objects are not even queued to refzero_list) because mark-and-sweep deals
 *  with them; refcounts are still updated so that they remain in sync with
 *  actual references.
 */

DUK_LOCAL void duk__decref_tvals_norz(duk_hthread *thr, duk_tval *tv, duk_idx_t count) {
	DUK_ASSERT(count == 0 || tv != NULL);

	while (count-- > 0) {
		DUK_TVAL_DECREF_NORZ(thr, tv);
		tv++;
	}
}

DUK_INTERNAL void duk_hobject_refcount_finalize_norz(duk_heap *heap, duk_hobject *h) {
	duk_hthread *thr;
	duk_uint_fast32_t i;
	duk_uint_fast32_t n;
	duk_propvalue *p_val;
	duk_tval *p_tv;
	duk_hstring **p_key;
	duk_uint8_t *p_flag;
	duk_hobject *h_proto;

	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(heap->heap_thread != NULL);
	DUK_ASSERT(h);
	DUK_ASSERT(DUK_HEAPHDR_GET_TYPE((duk_heaphdr *) h) == DUK_HTYPE_OBJECT);

	thr = heap->heap_thread;
	DUK_ASSERT(thr != NULL);

	p_key = DUK_HOBJECT_E_GET_KEY_BASE(heap, h);
	p_val = DUK_HOBJECT_E_GET_VALUE_BASE(heap, h);
	p_flag = DUK_HOBJECT_E_GET_FLAGS_BASE(heap, h);
	n = DUK_HOBJECT_GET_ENEXT(h);
	while (n-- > 0) {
		duk_hstring *key;

		key = p_key[n];
		if (DUK_UNLIKELY(key == NULL)) {
			continue;
		}
		DUK_HSTRING_DECREF_NORZ(thr, key);
		if (DUK_UNLIKELY(p_flag[n] & DUK_PROPDESC_FLAG_ACCESSOR)) {
			duk_hobject *h_getset;
			h_getset = p_val[n].a.get;
			DUK_ASSERT(h_getset == NULL || DUK_HEAPHDR_IS_OBJECT((duk_heaphdr *) h_getset));
			DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, h_getset);
			h_getset = p_val[n].a.set;
			DUK_ASSERT(h_getset == NULL || DUK_HEAPHDR_IS_OBJECT((duk_heaphdr *) h_getset));
			DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, h_getset);
		} else {
			duk_tval *tv_val;
			tv_val = &p_val[n].v;
			DUK_TVAL_DECREF_NORZ(thr, tv_val);
		}
	}

	p_tv = DUK_HOBJECT_A_GET_BASE(heap, h);
	n = DUK_HOBJECT_GET_ASIZE(h);
	while (n-- > 0) {
		duk_tval *tv_val;
		tv_val = p_tv + n;
		DUK_TVAL_DECREF_NORZ(thr, tv_val);
	}

	/* Hash part is a 'weak reference' and doesn't contribute to refcounts. */

	h_proto = (duk_hobject *) DUK_HOBJECT_GET_PROTOTYPE(heap, h);
	DUK_ASSERT(h_proto == NULL || DUK_HEAPHDR_IS_OBJECT((duk_heaphdr *) h_proto));
	DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, h_proto);

	/* XXX: Object subclass tests are quite awkward at present, ideally
	 * we should be able to switch-case here with a dense index (subtype
	 * number or something).  For now, fast path plain objects and arrays
	 * and bit test the rest individually.
	 */

	if (DUK_HOBJECT_HAS_FASTREFS(h)) {
		/* Plain object or array, nothing more to do.  While a
		 * duk_harray has additional fields, none of them need
		 * DECREF updates.
		 */
		DUK_ASSERT(DUK_HOBJECT_ALLOWS_FASTREFS(h));
		return;
	}
	DUK_ASSERT(DUK_HOBJECT_PROHIBITS_FASTREFS(h));

	/* Slow path: special object, start bit checks from most likely. */

	/* XXX: reorg, more common first */
	if (DUK_HOBJECT_IS_COMPFUNC(h)) {
		duk_hcompfunc *f = (duk_hcompfunc *) h;
		duk_tval *tv, *tv_end;
		duk_hobject **funcs, **funcs_end;

		DUK_HCOMPFUNC_ASSERT_VALID(f);

		if (DUK_LIKELY(DUK_HCOMPFUNC_GET_DATA(heap, f) != NULL)) {
			tv = DUK_HCOMPFUNC_GET_CONSTS_BASE(heap, f);
			tv_end = DUK_HCOMPFUNC_GET_CONSTS_END(heap, f);
			while (tv < tv_end) {
				DUK_TVAL_DECREF_NORZ(thr, tv);
				tv++;
			}

			funcs = DUK_HCOMPFUNC_GET_FUNCS_BASE(heap, f);
			funcs_end = DUK_HCOMPFUNC_GET_FUNCS_END(heap, f);
			while (funcs < funcs_end) {
				duk_hobject *h_func;
				h_func = *funcs;
				DUK_ASSERT(h_func != NULL);
				DUK_ASSERT(DUK_HEAPHDR_IS_OBJECT((duk_heaphdr *) h_func));
				DUK_HCOMPFUNC_DECREF_NORZ(thr, (duk_hcompfunc *) h_func);
				funcs++;
			}
		} else {
			/* May happen in some out-of-memory corner cases. */
			DUK_D(DUK_DPRINT("duk_hcompfunc 'data' is NULL, skipping decref"));
		}

		DUK_HEAPHDR_DECREF_ALLOWNULL(thr, (duk_heaphdr *) DUK_HCOMPFUNC_GET_LEXENV(heap, f));
		DUK_HEAPHDR_DECREF_ALLOWNULL(thr, (duk_heaphdr *) DUK_HCOMPFUNC_GET_VARENV(heap, f));
		DUK_HEAPHDR_DECREF_ALLOWNULL(thr, (duk_hbuffer *) DUK_HCOMPFUNC_GET_DATA(heap, f));
	} else if (DUK_HOBJECT_IS_DECENV(h)) {
		duk_hdecenv *e = (duk_hdecenv *) h;
		DUK_HDECENV_ASSERT_VALID(e);
		DUK_HTHREAD_DECREF_NORZ_ALLOWNULL(thr, e->thread);
		DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, e->varmap);
	} else if (DUK_HOBJECT_IS_OBJENV(h)) {
		duk_hobjenv *e = (duk_hobjenv *) h;
		DUK_HOBJENV_ASSERT_VALID(e);
		DUK_ASSERT(e->target != NULL); /* Required for object environments. */
		DUK_HOBJECT_DECREF_NORZ(thr, e->target);
#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
	} else if (DUK_HOBJECT_IS_BUFOBJ(h)) {
		duk_hbufobj *b = (duk_hbufobj *) h;
		DUK_HBUFOBJ_ASSERT_VALID(b);
		DUK_HBUFFER_DECREF_NORZ_ALLOWNULL(thr, (duk_hbuffer *) b->buf);
		DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, (duk_hobject *) b->buf_prop);
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */
	} else if (DUK_HOBJECT_IS_BOUNDFUNC(h)) {
		duk_hboundfunc *f = (duk_hboundfunc *) (void *) h;
		DUK_HBOUNDFUNC_ASSERT_VALID(f);
		DUK_TVAL_DECREF_NORZ(thr, &f->target);
		DUK_TVAL_DECREF_NORZ(thr, &f->this_binding);
		duk__decref_tvals_norz(thr, f->args, f->nargs);
#if defined(DUK_USE_ES6_PROXY)
	} else if (DUK_HOBJECT_IS_PROXY(h)) {
		duk_hproxy *p = (duk_hproxy *) h;
		DUK_HPROXY_ASSERT_VALID(p);
		DUK_HOBJECT_DECREF_NORZ(thr, p->target);
		DUK_HOBJECT_DECREF_NORZ(thr, p->handler);
#endif /* DUK_USE_ES6_PROXY */
	} else if (DUK_HOBJECT_IS_THREAD(h)) {
		duk_hthread *t = (duk_hthread *) h;
		duk_activation *act;
		duk_tval *tv;

		DUK_HTHREAD_ASSERT_VALID(t);

		tv = t->valstack;
		while (tv < t->valstack_top) {
			DUK_TVAL_DECREF_NORZ(thr, tv);
			tv++;
		}

		for (act = t->callstack_curr; act != NULL; act = act->parent) {
			DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, (duk_hobject *) DUK_ACT_GET_FUNC(act));
			DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, (duk_hobject *) act->var_env);
			DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, (duk_hobject *) act->lex_env);
#if defined(DUK_USE_NONSTD_FUNC_CALLER_PROPERTY)
			DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, (duk_hobject *) act->prev_caller);
#endif
#if 0 /* nothing now */
			for (cat = act->cat; cat != NULL; cat = cat->parent) {
			}
#endif
		}

		for (i = 0; i < DUK_NUM_BUILTINS; i++) {
			DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, (duk_hobject *) t->builtins[i]);
		}

		DUK_HTHREAD_DECREF_NORZ_ALLOWNULL(thr, (duk_hthread *) t->resumer);
	} else {
		/* We may come here if the object should have a FASTREFS flag
		 * but it's missing for some reason.  Assert for never getting
		 * here; however, other than performance, this is harmless.
		 */
		DUK_D(DUK_DPRINT("missing FASTREFS flag for: %!iO", h));
		DUK_ASSERT(0);
	}
}

DUK_INTERNAL void duk_heaphdr_refcount_finalize_norz(duk_heap *heap, duk_heaphdr *hdr) {
	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(heap->heap_thread != NULL);
	DUK_ASSERT(hdr != NULL);

	if (DUK_HEAPHDR_IS_OBJECT(hdr)) {
		duk_hobject_refcount_finalize_norz(heap, (duk_hobject *) hdr);
	}
	/* DUK_HTYPE_BUFFER: nothing to finalize */
	/* DUK_HTYPE_STRING: nothing to finalize */
}

/*
 *  Refzero processing for duk_hobject: queue a refzero'ed object to either
 *  finalize_list or refzero_list and process the relevent list(s) if
 *  necessary.
 *
 *  Refzero_list is single linked, with only 'prev' pointers set and valid.
 *  All 'next' pointers are intentionally left as garbage.  This doesn't
 *  matter because refzero_list is processed to completion before any other
 *  code (like mark-and-sweep) might walk the list.
 *
 *  In more detail:
 *
 *  - On first insert refzero_list is NULL and the new object becomes the
 *    first and only element on the list; duk__refcount_free_pending() is
 *    called and it starts processing the list from the initial element,
 *    i.e. the list tail.
 *
 *  - As each object is refcount finalized, new objects may be queued to
 *    refzero_list head.  Their 'next' pointers are left as garbage, but
 *    'prev' points are set correctly, with the element at refzero_list
 *    having a NULL 'prev' pointer.  The fact that refzero_list is non-NULL
 *    is used to reject (1) recursive duk__refcount_free_pending() and
 *    (2) finalize_list processing calls.
 *
 *  - When we're done with the current object, read its 'prev' pointer and
 *    free the object.  If 'prev' is NULL, we've reached head of list and are
 *    done: set refzero_list to NULL and process pending finalizers.  Otherwise
 *    continue processing the list.
 *
 *  A refzero cascade is free of side effects because it only involves
 *  queueing more objects and freeing memory; finalizer execution is blocked
 *  in the code path queueing objects to finalize_list.  As a result the
 *  initial refzero call (which triggers duk__refcount_free_pending()) must
 *  check finalize_list so that finalizers are executed snappily.
 *
 *  If finalize_list processing starts first, refzero may occur while we're
 *  processing finalizers.  That's fine: that particular refzero cascade is
 *  handled to completion without side effects.  Once the cascade is complete,
 *  we'll run pending finalizers but notice that we're already doing that and
 *  return.
 *
 *  This could be expanded to allow incremental freeing: just bail out
 *  early and resume at a future alloc/decref/refzero.  However, if that
 *  were done, the list structure would need to be kept consistent at all
 *  times, mark-and-sweep would need to handle refzero_list, etc.
 */

DUK_LOCAL void duk__refcount_free_pending(duk_heap *heap) {
	duk_heaphdr *curr;
#if defined(DUK_USE_DEBUG)
	duk_int_t count = 0;
#endif

	DUK_ASSERT(heap != NULL);

	curr = heap->refzero_list;
	DUK_ASSERT(curr != NULL);
	DUK_ASSERT(DUK_HEAPHDR_GET_PREV(heap, curr) == NULL); /* We're called on initial insert only. */
	/* curr->next is GARBAGE. */

	do {
		duk_heaphdr *prev;

		DUK_DDD(DUK_DDDPRINT("refzero processing %p: %!O", (void *) curr, (duk_heaphdr *) curr));

#if defined(DUK_USE_DEBUG)
		count++;
#endif

		DUK_ASSERT(curr != NULL);
		DUK_ASSERT(DUK_HEAPHDR_GET_TYPE(curr) == DUK_HTYPE_OBJECT); /* currently, always the case */
		/* FINALIZED may be set; don't care about flags here. */

		/* Refcount finalize 'curr'.  Refzero_list must be non-NULL
		 * here to prevent recursive entry to duk__refcount_free_pending().
		 */
		DUK_ASSERT(heap->refzero_list != NULL);
		duk_hobject_refcount_finalize_norz(heap, (duk_hobject *) curr);

		prev = DUK_HEAPHDR_GET_PREV(heap, curr);
		DUK_ASSERT((prev == NULL && heap->refzero_list == curr) || (prev != NULL && heap->refzero_list != curr));
		/* prev->next is intentionally not updated and is garbage. */

		duk_free_hobject(heap, (duk_hobject *) curr); /* Invalidates 'curr'. */

		curr = prev;
	} while (curr != NULL);

	heap->refzero_list = NULL;

	DUK_DD(DUK_DDPRINT("refzero processed %ld objects", (long) count));
}

DUK_LOCAL DUK_INLINE void duk__refcount_refzero_hobject(duk_heap *heap, duk_hobject *obj, duk_bool_t skip_free_pending) {
	duk_heaphdr *hdr;
	duk_heaphdr *root;

	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(heap->heap_thread != NULL);
	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(DUK_HEAPHDR_GET_TYPE((duk_heaphdr *) obj) == DUK_HTYPE_OBJECT);

	hdr = (duk_heaphdr *) obj;

	/* Refzero'd objects must be in heap_allocated.  They can't be in
	 * finalize_list because all objects on finalize_list have an
	 * artificial +1 refcount bump.
	 */
#if defined(DUK_USE_ASSERTIONS)
	DUK_ASSERT(duk_heap_in_heap_allocated(heap, (duk_heaphdr *) obj));
#endif

	DUK_HEAP_REMOVE_FROM_HEAP_ALLOCATED(heap, hdr);

#if defined(DUK_USE_FINALIZER_SUPPORT)
	/* This finalizer check MUST BE side effect free.  It should also be
	 * as fast as possible because it's applied to every object freed.
	 */
	if (DUK_UNLIKELY(DUK_HOBJECT_HAS_FINALIZER_FAST(heap, (duk_hobject *) hdr) != 0U)) {
		/* Special case: FINALIZED may be set if mark-and-sweep queued
		 * object for finalization, the finalizer was executed (and
		 * FINALIZED set), mark-and-sweep hasn't yet processed the
		 * object again, but its refcount drops to zero.  Free without
		 * running the finalizer again.
		 */
		if (DUK_HEAPHDR_HAS_FINALIZED(hdr)) {
			DUK_D(DUK_DPRINT("refzero'd object has finalizer and FINALIZED is set -> free"));
		} else {
			/* Set FINALIZABLE flag so that all objects on finalize_list
			 * will have it set and are thus detectable based on the
			 * flag alone.
			 */
			DUK_HEAPHDR_SET_FINALIZABLE(hdr);
			DUK_ASSERT(!DUK_HEAPHDR_HAS_FINALIZED(hdr));

#if defined(DUK_USE_REFERENCE_COUNTING)
			/* Bump refcount on finalize_list insert so that a
			 * refzero can never occur when an object is waiting
			 * for its finalizer call.  Refzero might otherwise
			 * now happen because we allow duk_push_heapptr() for
			 * objects pending finalization.
			 */
			DUK_HEAPHDR_PREINC_REFCOUNT(hdr);
#endif
			DUK_HEAP_INSERT_INTO_FINALIZE_LIST(heap, hdr);

			/* Process finalizers unless skipping is explicitly
			 * requested (NORZ) or refzero_list is being processed
			 * (avoids side effects during a refzero cascade).
			 * If refzero_list is processed, the initial refzero
			 * call will run pending finalizers when refzero_list
			 * is done.
			 */
			if (!skip_free_pending && heap->refzero_list == NULL) {
				duk_heap_process_finalize_list(heap);
			}
			return;
		}
	}
#endif /* DUK_USE_FINALIZER_SUPPORT */

	/* No need to finalize, free object via refzero_list. */

	root = heap->refzero_list;

	DUK_HEAPHDR_SET_PREV(heap, hdr, NULL);
	/* 'next' is left as GARBAGE. */
	heap->refzero_list = hdr;

	if (root == NULL) {
		/* Object is now queued.  Refzero_list was NULL so
		 * no-one is currently processing it; do it here.
		 * With refzero processing just doing a cascade of
		 * free calls, we can process it directly even when
		 * NORZ macros are used: there are no side effects.
		 */
		duk__refcount_free_pending(heap);
		DUK_ASSERT(heap->refzero_list == NULL);

		/* Process finalizers only after the entire cascade
		 * is finished.  In most cases there's nothing to
		 * finalize, so fast path check to avoid a call.
		 */
#if defined(DUK_USE_FINALIZER_SUPPORT)
		if (!skip_free_pending && DUK_UNLIKELY(heap->finalize_list != NULL)) {
			duk_heap_process_finalize_list(heap);
		}
#endif
	} else {
		DUK_ASSERT(DUK_HEAPHDR_GET_PREV(heap, root) == NULL);
		DUK_HEAPHDR_SET_PREV(heap, root, hdr);

		/* Object is now queued.  Because refzero_list was
		 * non-NULL, it's already being processed by someone
		 * in the C call stack, so we're done.
		 */
	}
}

#if defined(DUK_USE_FINALIZER_SUPPORT)
DUK_INTERNAL DUK_ALWAYS_INLINE void duk_refzero_check_fast(duk_hthread *thr) {
	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(thr->heap->refzero_list == NULL); /* Processed to completion inline. */

	if (DUK_UNLIKELY(thr->heap->finalize_list != NULL)) {
		duk_heap_process_finalize_list(thr->heap);
	}
}

DUK_INTERNAL void duk_refzero_check_slow(duk_hthread *thr) {
	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(thr->heap->refzero_list == NULL); /* Processed to completion inline. */

	if (DUK_UNLIKELY(thr->heap->finalize_list != NULL)) {
		duk_heap_process_finalize_list(thr->heap);
	}
}
#endif /* DUK_USE_FINALIZER_SUPPORT */

/*
 *  Refzero processing for duk_hstring.
 */

DUK_LOCAL DUK_INLINE void duk__refcount_refzero_hstring(duk_heap *heap, duk_hstring *str) {
	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(heap->heap_thread != NULL);
	DUK_ASSERT(str != NULL);
	DUK_ASSERT(DUK_HEAPHDR_GET_TYPE((duk_heaphdr *) str) == DUK_HTYPE_STRING);

	duk_heap_strcache_string_remove(heap, str);
	duk_heap_strtable_unlink(heap, str);
	duk_free_hstring(heap, str);
}

/*
 *  Refzero processing for duk_hbuffer.
 */

DUK_LOCAL DUK_INLINE void duk__refcount_refzero_hbuffer(duk_heap *heap, duk_hbuffer *buf) {
	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(heap->heap_thread != NULL);
	DUK_ASSERT(buf != NULL);
	DUK_ASSERT(DUK_HEAPHDR_GET_TYPE((duk_heaphdr *) buf) == DUK_HTYPE_BUFFER);

	DUK_HEAP_REMOVE_FROM_HEAP_ALLOCATED(heap, (duk_heaphdr *) buf);
	duk_free_hbuffer(heap, buf);
}

/*
 *  Incref and decref functions.
 *
 *  Decref may trigger immediate refzero handling, which may free and finalize
 *  an arbitrary number of objects (a "DECREF cascade").
 *
 *  Refzero handling is skipped entirely if (1) mark-and-sweep is running or
 *  (2) execution is paused in the debugger.  The objects are left in the heap,
 *  and will be freed by mark-and-sweep or eventual heap destruction.
 *
 *  This is necessary during mark-and-sweep because refcounts are also updated
 *  during the sweep phase (otherwise objects referenced by a swept object
 *  would have incorrect refcounts) which then calls here.  This could be
 *  avoided by using separate decref macros in mark-and-sweep; however,
 *  mark-and-sweep also calls finalizers which would use the ordinary decref
 *  macros anyway.
 *
 *  We can't process refzeros (= free objects) when the debugger is running
 *  as the debugger might make an object unreachable but still continue
 *  inspecting it (or even cause it to be pushed back).  So we must rely on
 *  mark-and-sweep to collect them.
 *
 *  The DUK__RZ_SUPPRESS_CHECK() condition is also used in heap destruction
 *  when running finalizers for remaining objects: the flag prevents objects
 *  from being moved around in heap linked lists while that's being done.
 *
 *  The suppress condition is important to performance.
 */

#define DUK__RZ_SUPPRESS_ASSERT1() \
	do { \
		DUK_ASSERT(thr != NULL); \
		DUK_ASSERT(thr->heap != NULL); \
		/* When mark-and-sweep runs, heap_thread must exist. */ \
		DUK_ASSERT(thr->heap->ms_running == 0 || thr->heap->heap_thread != NULL); \
		/* In normal operation finalizers are executed with ms_running == 0 \
		 * so we should never see ms_running == 1 and thr != heap_thread. \
		 * In heap destruction finalizers are executed with ms_running != 0 \
		 * to e.g. prevent refzero; a special value ms_running == 2 is used \
		 * in that case so it can be distinguished from the normal runtime \
		 * case, and allows a stronger assertion here (GH-2030). \
		 */ \
		DUK_ASSERT(!(thr->heap->ms_running == 1 && thr != thr->heap->heap_thread)); \
		/* We may be called when the heap is initializing and we process \
		 * refzeros normally, but mark-and-sweep and finalizers are prevented \
		 * if that's the case. \
		 */ \
		DUK_ASSERT(thr->heap->heap_initializing == 0 || thr->heap->ms_prevent_count > 0); \
		DUK_ASSERT(thr->heap->heap_initializing == 0 || thr->heap->pf_prevent_count > 0); \
	} while (0)

#if defined(DUK_USE_DEBUGGER_SUPPORT)
#define DUK__RZ_SUPPRESS_ASSERT2() \
	do { \
		/* When debugger is paused, ms_running is set. */ \
		DUK_ASSERT(!DUK_HEAP_HAS_DEBUGGER_PAUSED(thr->heap) || thr->heap->ms_running != 0); \
	} while (0)
#define DUK__RZ_SUPPRESS_COND() (heap->ms_running != 0)
#else
#define DUK__RZ_SUPPRESS_ASSERT2() \
	do { \
	} while (0)
#define DUK__RZ_SUPPRESS_COND() (heap->ms_running != 0)
#endif /* DUK_USE_DEBUGGER_SUPPORT */

#define DUK__RZ_SUPPRESS_CHECK() \
	do { \
		DUK__RZ_SUPPRESS_ASSERT1(); \
		DUK__RZ_SUPPRESS_ASSERT2(); \
		if (DUK_UNLIKELY(DUK__RZ_SUPPRESS_COND())) { \
			DUK_DDD( \
			    DUK_DDDPRINT("refzero handling suppressed (not even queued) when mark-and-sweep running, object: %p", \
			                 (void *) h)); \
			return; \
		} \
	} while (0)

#define DUK__RZ_STRING() \
	do { \
		duk__refcount_refzero_hstring(heap, (duk_hstring *) h); \
	} while (0)
#define DUK__RZ_BUFFER() \
	do { \
		duk__refcount_refzero_hbuffer(heap, (duk_hbuffer *) h); \
	} while (0)
#define DUK__RZ_OBJECT() \
	do { \
		duk__refcount_refzero_hobject(heap, (duk_hobject *) h, skip_free_pending); \
	} while (0)

/* XXX: test the effect of inlining here vs. NOINLINE in refzero helpers */
#if defined(DUK_USE_FAST_REFCOUNT_DEFAULT)
#define DUK__RZ_INLINE DUK_ALWAYS_INLINE
#else
#define DUK__RZ_INLINE /*nop*/
#endif

DUK_LOCAL DUK__RZ_INLINE void duk__hstring_refzero_helper(duk_hthread *thr, duk_hstring *h) {
	duk_heap *heap;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(h != NULL);
	heap = thr->heap;

	DUK__RZ_SUPPRESS_CHECK();
	DUK__RZ_STRING();
}

DUK_LOCAL DUK__RZ_INLINE void duk__hbuffer_refzero_helper(duk_hthread *thr, duk_hbuffer *h) {
	duk_heap *heap;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(h != NULL);
	heap = thr->heap;

	DUK__RZ_SUPPRESS_CHECK();
	DUK__RZ_BUFFER();
}

DUK_LOCAL DUK__RZ_INLINE void duk__hobject_refzero_helper(duk_hthread *thr, duk_hobject *h, duk_bool_t skip_free_pending) {
	duk_heap *heap;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(h != NULL);
	heap = thr->heap;

	DUK__RZ_SUPPRESS_CHECK();
	DUK__RZ_OBJECT();
}

DUK_LOCAL DUK__RZ_INLINE void duk__heaphdr_refzero_helper(duk_hthread *thr, duk_heaphdr *h, duk_bool_t skip_free_pending) {
	duk_heap *heap;
	duk_small_uint_t htype;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(h != NULL);
	heap = thr->heap;

	htype = (duk_small_uint_t) DUK_HEAPHDR_GET_TYPE(h);
	DUK_DDD(DUK_DDDPRINT("ms_running=%ld, heap_thread=%p", (long) thr->heap->ms_running, thr->heap->heap_thread));
	DUK__RZ_SUPPRESS_CHECK();

	switch (htype) {
	case DUK_HTYPE_STRING:
		/* Strings have no internal references but do have "weak"
		 * references in the string cache.  Also note that strings
		 * are not on the heap_allocated list like other heap
		 * elements.
		 */

		DUK__RZ_STRING();
		break;

	case DUK_HTYPE_OBJECT:
		/* Objects have internal references.  Must finalize through
		 * the "refzero" work list.
		 */

		DUK__RZ_OBJECT();
		break;

	default:
		/* Buffers have no internal references.  However, a dynamic
		 * buffer has a separate allocation for the buffer.  This is
		 * freed by duk_heap_free_heaphdr_raw().
		 */

		DUK_ASSERT(DUK_HEAPHDR_GET_TYPE(h) == DUK_HTYPE_BUFFER);
		DUK__RZ_BUFFER();
		break;
	}
}

DUK_INTERNAL DUK_NOINLINE void duk_heaphdr_refzero(duk_hthread *thr, duk_heaphdr *h) {
	duk__heaphdr_refzero_helper(thr, h, 0 /*skip_free_pending*/);
}

DUK_INTERNAL DUK_NOINLINE void duk_heaphdr_refzero_norz(duk_hthread *thr, duk_heaphdr *h) {
	duk__heaphdr_refzero_helper(thr, h, 1 /*skip_free_pending*/);
}

DUK_INTERNAL DUK_NOINLINE void duk_hstring_refzero(duk_hthread *thr, duk_hstring *h) {
	duk__hstring_refzero_helper(thr, h);
}

DUK_INTERNAL DUK_NOINLINE void duk_hbuffer_refzero(duk_hthread *thr, duk_hbuffer *h) {
	duk__hbuffer_refzero_helper(thr, h);
}

DUK_INTERNAL DUK_NOINLINE void duk_hobject_refzero(duk_hthread *thr, duk_hobject *h) {
	duk__hobject_refzero_helper(thr, h, 0 /*skip_free_pending*/);
}

DUK_INTERNAL DUK_NOINLINE void duk_hobject_refzero_norz(duk_hthread *thr, duk_hobject *h) {
	duk__hobject_refzero_helper(thr, h, 1 /*skip_free_pending*/);
}

#if !defined(DUK_USE_FAST_REFCOUNT_DEFAULT)
DUK_INTERNAL void duk_tval_incref(duk_tval *tv) {
	DUK_ASSERT(tv != NULL);

	if (DUK_TVAL_NEEDS_REFCOUNT_UPDATE(tv)) {
		duk_heaphdr *h = DUK_TVAL_GET_HEAPHDR(tv);
		DUK_ASSERT(h != NULL);
		DUK_ASSERT(DUK_HEAPHDR_HTYPE_VALID(h));
		DUK_ASSERT_DISABLE(h->h_refcount >= 0);
		DUK_HEAPHDR_PREINC_REFCOUNT(h);
		DUK_ASSERT(DUK_HEAPHDR_GET_REFCOUNT(h) != 0); /* No wrapping. */
	}
}

DUK_INTERNAL void duk_tval_decref(duk_hthread *thr, duk_tval *tv) {
	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(tv != NULL);

	if (DUK_TVAL_NEEDS_REFCOUNT_UPDATE(tv)) {
		duk_heaphdr *h = DUK_TVAL_GET_HEAPHDR(tv);
		DUK_ASSERT(h != NULL);
		DUK_ASSERT(DUK_HEAPHDR_HTYPE_VALID(h));
		DUK_ASSERT(DUK_HEAPHDR_GET_REFCOUNT(h) >= 1);
#if 0
		if (DUK_HEAPHDR_PREDEC_REFCOUNT(h) != 0) {
			return;
		}
		duk_heaphdr_refzero(thr, h);
#else
		duk_heaphdr_decref(thr, h);
#endif
	}
}

DUK_INTERNAL void duk_tval_decref_norz(duk_hthread *thr, duk_tval *tv) {
	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(tv != NULL);

	if (DUK_TVAL_NEEDS_REFCOUNT_UPDATE(tv)) {
		duk_heaphdr *h = DUK_TVAL_GET_HEAPHDR(tv);
		DUK_ASSERT(h != NULL);
		DUK_ASSERT(DUK_HEAPHDR_HTYPE_VALID(h));
		DUK_ASSERT(DUK_HEAPHDR_GET_REFCOUNT(h) >= 1);
#if 0
		if (DUK_HEAPHDR_PREDEC_REFCOUNT(h) != 0) {
			return;
		}
		duk_heaphdr_refzero_norz(thr, h);
#else
		duk_heaphdr_decref_norz(thr, h);
#endif
	}
}
#endif /* !DUK_USE_FAST_REFCOUNT_DEFAULT */

#define DUK__DECREF_ASSERTS() \
	do { \
		DUK_ASSERT(thr != NULL); \
		DUK_ASSERT(thr->heap != NULL); \
		DUK_ASSERT(h != NULL); \
		DUK_ASSERT(DUK_HEAPHDR_HTYPE_VALID((duk_heaphdr *) h)); \
		DUK_ASSERT(DUK_HEAPHDR_GET_REFCOUNT((duk_heaphdr *) h) >= 1); \
	} while (0)
#if defined(DUK_USE_ROM_OBJECTS)
#define DUK__INCREF_SHARED() \
	do { \
		if (DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) h)) { \
			return; \
		} \
		DUK_HEAPHDR_PREINC_REFCOUNT((duk_heaphdr *) h); \
		DUK_ASSERT(DUK_HEAPHDR_GET_REFCOUNT((duk_heaphdr *) h) != 0); /* No wrapping. */ \
	} while (0)
#define DUK__DECREF_SHARED() \
	do { \
		if (DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) h)) { \
			return; \
		} \
		if (DUK_HEAPHDR_PREDEC_REFCOUNT((duk_heaphdr *) h) != 0) { \
			return; \
		} \
	} while (0)
#else
#define DUK__INCREF_SHARED() \
	do { \
		DUK_HEAPHDR_PREINC_REFCOUNT((duk_heaphdr *) h); \
		DUK_ASSERT(DUK_HEAPHDR_GET_REFCOUNT((duk_heaphdr *) h) != 0); /* No wrapping. */ \
	} while (0)
#define DUK__DECREF_SHARED() \
	do { \
		if (DUK_HEAPHDR_PREDEC_REFCOUNT((duk_heaphdr *) h) != 0) { \
			return; \
		} \
	} while (0)
#endif

#if !defined(DUK_USE_FAST_REFCOUNT_DEFAULT)
/* This will in practice be inlined because it's just an INC instructions
 * and a bit test + INC when ROM objects are enabled.
 */
DUK_INTERNAL void duk_heaphdr_incref(duk_heaphdr *h) {
	DUK_ASSERT(h != NULL);
	DUK_ASSERT(DUK_HEAPHDR_HTYPE_VALID(h));
	DUK_ASSERT_DISABLE(DUK_HEAPHDR_GET_REFCOUNT(h) >= 0);

	DUK__INCREF_SHARED();
}

DUK_INTERNAL void duk_heaphdr_decref(duk_hthread *thr, duk_heaphdr *h) {
	DUK__DECREF_ASSERTS();
	DUK__DECREF_SHARED();
	duk_heaphdr_refzero(thr, h);

	/* Forced mark-and-sweep when GC torture enabled; this could happen
	 * on any DECREF (but not DECREF_NORZ).
	 */
	DUK_GC_TORTURE(thr->heap);
}
DUK_INTERNAL void duk_heaphdr_decref_norz(duk_hthread *thr, duk_heaphdr *h) {
	DUK__DECREF_ASSERTS();
	DUK__DECREF_SHARED();
	duk_heaphdr_refzero_norz(thr, h);
}
#endif /* !DUK_USE_FAST_REFCOUNT_DEFAULT */

#if 0 /* Not needed. */
DUK_INTERNAL void duk_hstring_decref(duk_hthread *thr, duk_hstring *h) {
	DUK__DECREF_ASSERTS();
	DUK__DECREF_SHARED();
	duk_hstring_refzero(thr, h);
}
DUK_INTERNAL void duk_hstring_decref_norz(duk_hthread *thr, duk_hstring *h) {
	DUK__DECREF_ASSERTS();
	DUK__DECREF_SHARED();
	duk_hstring_refzero_norz(thr, h);
}
DUK_INTERNAL void duk_hbuffer_decref(duk_hthread *thr, duk_hbuffer *h) {
	DUK__DECREF_ASSERTS();
	DUK__DECREF_SHARED();
	duk_hbuffer_refzero(thr, h);
}
DUK_INTERNAL void duk_hbuffer_decref_norz(duk_hthread *thr, duk_hbuffer *h) {
	DUK__DECREF_ASSERTS();
	DUK__DECREF_SHARED();
	duk_hbuffer_refzero_norz(thr, h);
}
DUK_INTERNAL void duk_hobject_decref(duk_hthread *thr, duk_hobject *h) {
	DUK__DECREF_ASSERTS();
	DUK__DECREF_SHARED();
	duk_hobject_refzero(thr, h);
}
DUK_INTERNAL void duk_hobject_decref_norz(duk_hthread *thr, duk_hobject *h) {
	DUK__DECREF_ASSERTS();
	DUK__DECREF_SHARED();
	duk_hobject_refzero_norz(thr, h);
}
#endif

#else /* DUK_USE_REFERENCE_COUNTING */

/* no refcounting */

#endif /* DUK_USE_REFERENCE_COUNTING */
