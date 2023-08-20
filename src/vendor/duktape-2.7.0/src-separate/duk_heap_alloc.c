/*
 *  duk_heap allocation and freeing.
 */

#include "duk_internal.h"

#if defined(DUK_USE_ROM_STRINGS)
/* Fixed seed value used with ROM strings. */
#define DUK__FIXED_HASH_SEED 0xabcd1234
#endif

/*
 *  Free a heap object.
 *
 *  Free heap object and its internal (non-heap) pointers.  Assumes that
 *  caller has removed the object from heap allocated list or the string
 *  intern table, and any weak references (which strings may have) have
 *  been already dealt with.
 */

DUK_INTERNAL void duk_free_hobject(duk_heap *heap, duk_hobject *h) {
	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(h != NULL);

	DUK_FREE(heap, DUK_HOBJECT_GET_PROPS(heap, h));

	if (DUK_HOBJECT_IS_COMPFUNC(h)) {
		duk_hcompfunc *f = (duk_hcompfunc *) h;
		DUK_UNREF(f);
		/* Currently nothing to free; 'data' is a heap object */
	} else if (DUK_HOBJECT_IS_NATFUNC(h)) {
		duk_hnatfunc *f = (duk_hnatfunc *) h;
		DUK_UNREF(f);
		/* Currently nothing to free */
	} else if (DUK_HOBJECT_IS_THREAD(h)) {
		duk_hthread *t = (duk_hthread *) h;
		duk_activation *act;

		DUK_FREE(heap, t->valstack);

		/* Don't free h->resumer because it exists in the heap.
		 * Callstack entries also contain function pointers which
		 * are not freed for the same reason.  They are decref
		 * finalized and the targets are freed if necessary based
		 * on their refcount (or reachability).
		 */
		for (act = t->callstack_curr; act != NULL;) {
			duk_activation *act_next;
			duk_catcher *cat;

			for (cat = act->cat; cat != NULL;) {
				duk_catcher *cat_next;

				cat_next = cat->parent;
				DUK_FREE(heap, (void *) cat);
				cat = cat_next;
			}

			act_next = act->parent;
			DUK_FREE(heap, (void *) act);
			act = act_next;
		}

		/* XXX: with 'caller' property the callstack would need
		 * to be unwound to update the 'caller' properties of
		 * functions in the callstack.
		 */
	} else if (DUK_HOBJECT_IS_BOUNDFUNC(h)) {
		duk_hboundfunc *f = (duk_hboundfunc *) (void *) h;

		DUK_FREE(heap, f->args);
	}

	DUK_FREE(heap, (void *) h);
}

DUK_INTERNAL void duk_free_hbuffer(duk_heap *heap, duk_hbuffer *h) {
	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(h != NULL);

	if (DUK_HBUFFER_HAS_DYNAMIC(h) && !DUK_HBUFFER_HAS_EXTERNAL(h)) {
		duk_hbuffer_dynamic *g = (duk_hbuffer_dynamic *) h;
		DUK_DDD(DUK_DDDPRINT("free dynamic buffer %p", (void *) DUK_HBUFFER_DYNAMIC_GET_DATA_PTR(heap, g)));
		DUK_FREE(heap, DUK_HBUFFER_DYNAMIC_GET_DATA_PTR(heap, g));
	}
	DUK_FREE(heap, (void *) h);
}

DUK_INTERNAL void duk_free_hstring(duk_heap *heap, duk_hstring *h) {
	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(h != NULL);

	DUK_UNREF(heap);
	DUK_UNREF(h);

#if defined(DUK_USE_HSTRING_EXTDATA) && defined(DUK_USE_EXTSTR_FREE)
	if (DUK_HSTRING_HAS_EXTDATA(h)) {
		DUK_DDD(
		    DUK_DDDPRINT("free extstr: hstring %!O, extdata: %p", h, DUK_HSTRING_GET_EXTDATA((duk_hstring_external *) h)));
		DUK_USE_EXTSTR_FREE(heap->heap_udata, (const void *) DUK_HSTRING_GET_EXTDATA((duk_hstring_external *) h));
	}
#endif
	DUK_FREE(heap, (void *) h);
}

DUK_INTERNAL void duk_heap_free_heaphdr_raw(duk_heap *heap, duk_heaphdr *hdr) {
	DUK_ASSERT(heap);
	DUK_ASSERT(hdr);

	DUK_DDD(DUK_DDDPRINT("free heaphdr %p, htype %ld", (void *) hdr, (long) DUK_HEAPHDR_GET_TYPE(hdr)));

	switch (DUK_HEAPHDR_GET_TYPE(hdr)) {
	case DUK_HTYPE_STRING:
		duk_free_hstring(heap, (duk_hstring *) hdr);
		break;
	case DUK_HTYPE_OBJECT:
		duk_free_hobject(heap, (duk_hobject *) hdr);
		break;
	default:
		DUK_ASSERT(DUK_HEAPHDR_GET_TYPE(hdr) == DUK_HTYPE_BUFFER);
		duk_free_hbuffer(heap, (duk_hbuffer *) hdr);
	}
}

/*
 *  Free the heap.
 *
 *  Frees heap-related non-heap-tracked allocations such as the
 *  string intern table; then frees the heap allocated objects;
 *  and finally frees the heap structure itself.  Reference counts
 *  and GC markers are ignored (and not updated) in this process,
 *  and finalizers won't be called.
 *
 *  The heap pointer and heap object pointers must not be used
 *  after this call.
 */

#if defined(DUK_USE_CACHE_ACTIVATION)
DUK_LOCAL duk_size_t duk__heap_free_activation_freelist(duk_heap *heap) {
	duk_activation *act;
	duk_activation *act_next;
	duk_size_t count_act = 0;

	for (act = heap->activation_free; act != NULL;) {
		act_next = act->parent;
		DUK_FREE(heap, (void *) act);
		act = act_next;
#if defined(DUK_USE_DEBUG)
		count_act++;
#endif
	}
	heap->activation_free = NULL; /* needed when called from mark-and-sweep */
	return count_act;
}
#endif /* DUK_USE_CACHE_ACTIVATION */

#if defined(DUK_USE_CACHE_CATCHER)
DUK_LOCAL duk_size_t duk__heap_free_catcher_freelist(duk_heap *heap) {
	duk_catcher *cat;
	duk_catcher *cat_next;
	duk_size_t count_cat = 0;

	for (cat = heap->catcher_free; cat != NULL;) {
		cat_next = cat->parent;
		DUK_FREE(heap, (void *) cat);
		cat = cat_next;
#if defined(DUK_USE_DEBUG)
		count_cat++;
#endif
	}
	heap->catcher_free = NULL; /* needed when called from mark-and-sweep */

	return count_cat;
}
#endif /* DUK_USE_CACHE_CATCHER */

DUK_INTERNAL void duk_heap_free_freelists(duk_heap *heap) {
	duk_size_t count_act = 0;
	duk_size_t count_cat = 0;

#if defined(DUK_USE_CACHE_ACTIVATION)
	count_act = duk__heap_free_activation_freelist(heap);
#endif
#if defined(DUK_USE_CACHE_CATCHER)
	count_cat = duk__heap_free_catcher_freelist(heap);
#endif
	DUK_UNREF(heap);
	DUK_UNREF(count_act);
	DUK_UNREF(count_cat);

	DUK_D(
	    DUK_DPRINT("freed %ld activation freelist entries, %ld catcher freelist entries", (long) count_act, (long) count_cat));
}

DUK_LOCAL void duk__free_allocated(duk_heap *heap) {
	duk_heaphdr *curr;
	duk_heaphdr *next;

	curr = heap->heap_allocated;
	while (curr) {
		/* We don't log or warn about freeing zero refcount objects
		 * because they may happen with finalizer processing.
		 */

		DUK_DDD(DUK_DDDPRINT("FINALFREE (allocated): %!iO", (duk_heaphdr *) curr));
		next = DUK_HEAPHDR_GET_NEXT(heap, curr);
		duk_heap_free_heaphdr_raw(heap, curr);
		curr = next;
	}
}

#if defined(DUK_USE_FINALIZER_SUPPORT)
DUK_LOCAL void duk__free_finalize_list(duk_heap *heap) {
	duk_heaphdr *curr;
	duk_heaphdr *next;

	curr = heap->finalize_list;
	while (curr) {
		DUK_DDD(DUK_DDDPRINT("FINALFREE (finalize_list): %!iO", (duk_heaphdr *) curr));
		next = DUK_HEAPHDR_GET_NEXT(heap, curr);
		duk_heap_free_heaphdr_raw(heap, curr);
		curr = next;
	}
}
#endif /* DUK_USE_FINALIZER_SUPPORT */

DUK_LOCAL void duk__free_stringtable(duk_heap *heap) {
	/* strings are only tracked by stringtable */
	duk_heap_strtable_free(heap);
}

#if defined(DUK_USE_FINALIZER_SUPPORT)
DUK_LOCAL void duk__free_run_finalizers(duk_heap *heap) {
	duk_heaphdr *curr;
	duk_uint_t round_no;
	duk_size_t count_all;
	duk_size_t count_finalized;
	duk_size_t curr_limit;

	DUK_ASSERT(heap != NULL);

#if defined(DUK_USE_REFERENCE_COUNTING)
	DUK_ASSERT(heap->refzero_list == NULL); /* refzero not running -> must be empty */
#endif
	DUK_ASSERT(heap->finalize_list == NULL); /* mark-and-sweep last pass */

	if (heap->heap_thread == NULL) {
		/* May happen when heap allocation fails right off.  There
		 * cannot be any finalizable objects in this case.
		 */
		DUK_D(DUK_DPRINT("no heap_thread in heap destruct, assume no finalizable objects"));
		return;
	}

	/* Prevent finalize_list processing and mark-and-sweep entirely.
	 * Setting ms_running != 0 also prevents refzero handling from moving
	 * objects away from the heap_allocated list.  The flag name is a bit
	 * misleading here.
	 *
	 * Use a distinct value for ms_running here (== 2) so that assertions
	 * can detect this situation separate from the normal runtime
	 * mark-and-sweep case.  This allows better assertions (GH-2030).
	 */
	DUK_ASSERT(heap->pf_prevent_count == 0);
	DUK_ASSERT(heap->ms_running == 0);
	DUK_ASSERT(heap->ms_prevent_count == 0);
	heap->pf_prevent_count = 1;
	heap->ms_running = 2; /* Use distinguishable value. */
	heap->ms_prevent_count = 1; /* Bump, because mark-and-sweep assumes it's bumped when ms_running is set. */

	curr_limit = 0; /* suppress warning, not used */
	for (round_no = 0;; round_no++) {
		curr = heap->heap_allocated;
		count_all = 0;
		count_finalized = 0;
		while (curr) {
			count_all++;
			if (DUK_HEAPHDR_IS_OBJECT(curr)) {
				/* Only objects in heap_allocated may have finalizers.  Check that
				 * the object itself has a _Finalizer property (own or inherited)
				 * so that we don't execute finalizers for e.g. Proxy objects.
				 */
				DUK_ASSERT(curr != NULL);

				if (DUK_HOBJECT_HAS_FINALIZER_FAST(heap, (duk_hobject *) curr)) {
					if (!DUK_HEAPHDR_HAS_FINALIZED((duk_heaphdr *) curr)) {
						DUK_ASSERT(
						    DUK_HEAP_HAS_FINALIZER_NORESCUE(heap)); /* maps to finalizer 2nd argument */
						duk_heap_run_finalizer(heap, (duk_hobject *) curr);
						count_finalized++;
					}
				}
			}
			curr = DUK_HEAPHDR_GET_NEXT(heap, curr);
		}

		/* Each round of finalizer execution may spawn new finalizable objects
		 * which is normal behavior for some applications.  Allow multiple
		 * rounds of finalization, but use a shrinking limit based on the
		 * first round to detect the case where a runaway finalizer creates
		 * an unbounded amount of new finalizable objects.  Finalizer rescue
		 * is not supported: the semantics are unclear because most of the
		 * objects being finalized here are already reachable.  The finalizer
		 * is given a boolean to indicate that rescue is not possible.
		 *
		 * See discussion in: https://github.com/svaarala/duktape/pull/473
		 */

		if (round_no == 0) {
			/* Cannot wrap: each object is at least 8 bytes so count is
			 * at most 1/8 of that.
			 */
			curr_limit = count_all * 2;
		} else {
			curr_limit = (curr_limit * 3) / 4; /* Decrease by 25% every round */
		}
		DUK_D(DUK_DPRINT("finalizer round %ld complete, %ld objects, tried to execute %ld finalizers, current limit is %ld",
		                 (long) round_no,
		                 (long) count_all,
		                 (long) count_finalized,
		                 (long) curr_limit));

		if (count_finalized == 0) {
			DUK_D(DUK_DPRINT("no more finalizable objects, forced finalization finished"));
			break;
		}
		if (count_finalized >= curr_limit) {
			DUK_D(DUK_DPRINT("finalizer count above limit, potentially runaway finalizer; skip remaining finalizers"));
			break;
		}
	}

	DUK_ASSERT(heap->ms_running == 2);
	DUK_ASSERT(heap->pf_prevent_count == 1);
	heap->ms_running = 0;
	heap->pf_prevent_count = 0;
}
#endif /* DUK_USE_FINALIZER_SUPPORT */

DUK_INTERNAL void duk_heap_free(duk_heap *heap) {
	DUK_D(DUK_DPRINT("free heap: %p", (void *) heap));

#if defined(DUK_USE_DEBUG)
	duk_heap_strtable_dump(heap);
#endif

#if defined(DUK_USE_DEBUGGER_SUPPORT)
	/* Detach a debugger if attached (can be called multiple times)
	 * safely.
	 */
	/* XXX: Add a flag to reject an attempt to re-attach?  Otherwise
	 * the detached callback may immediately reattach.
	 */
	duk_debug_do_detach(heap);
#endif

	/* Execute finalizers before freeing the heap, even for reachable
	 * objects.  This gives finalizers the chance to free any native
	 * resources like file handles, allocations made outside Duktape,
	 * etc.  This is quite tricky to get right, so that all finalizer
	 * guarantees are honored.
	 *
	 * Run mark-and-sweep a few times just in case (unreachable object
	 * finalizers run already here).  The last round must rescue objects
	 * from the previous round without running any more finalizers.  This
	 * ensures rescued objects get their FINALIZED flag cleared so that
	 * their finalizer is called once more in forced finalization to
	 * satisfy finalizer guarantees.  However, we don't want to run any
	 * more finalizers because that'd required one more loop, and so on.
	 *
	 * XXX: this perhaps requires an execution time limit.
	 */
	DUK_D(DUK_DPRINT("execute finalizers before freeing heap"));
	DUK_ASSERT(heap->pf_skip_finalizers == 0);
	DUK_D(DUK_DPRINT("forced gc #1 in heap destruction"));
	duk_heap_mark_and_sweep(heap, 0);
	DUK_D(DUK_DPRINT("forced gc #2 in heap destruction"));
	duk_heap_mark_and_sweep(heap, 0);
	DUK_D(DUK_DPRINT("forced gc #3 in heap destruction (don't run finalizers)"));
	heap->pf_skip_finalizers = 1;
	duk_heap_mark_and_sweep(heap, 0); /* Skip finalizers; queue finalizable objects to heap_allocated. */

	/* There are never objects in refzero_list at this point, or at any
	 * point beyond a DECREF (even a DECREF_NORZ).  Since Duktape 2.1
	 * refzero_list processing is side effect free, so it is always
	 * processed to completion by a DECREF initially triggering a zero
	 * refcount.
	 */
#if defined(DUK_USE_REFERENCE_COUNTING)
	DUK_ASSERT(heap->refzero_list == NULL); /* Always processed to completion inline. */
#endif
#if defined(DUK_USE_FINALIZER_SUPPORT)
	DUK_ASSERT(heap->finalize_list == NULL); /* Last mark-and-sweep with skip_finalizers. */
#endif

#if defined(DUK_USE_FINALIZER_SUPPORT)
	DUK_D(DUK_DPRINT("run finalizers for remaining finalizable objects"));
	DUK_HEAP_SET_FINALIZER_NORESCUE(heap); /* Rescue no longer supported. */
	duk__free_run_finalizers(heap);
#endif /* DUK_USE_FINALIZER_SUPPORT */

	/* Note: heap->heap_thread, heap->curr_thread, and heap->heap_object
	 * are on the heap allocated list.
	 */

	DUK_D(DUK_DPRINT("freeing temporary freelists"));
	duk_heap_free_freelists(heap);

	DUK_D(DUK_DPRINT("freeing heap_allocated of heap: %p", (void *) heap));
	duk__free_allocated(heap);

#if defined(DUK_USE_REFERENCE_COUNTING)
	DUK_ASSERT(heap->refzero_list == NULL); /* Always processed to completion inline. */
#endif

#if defined(DUK_USE_FINALIZER_SUPPORT)
	DUK_D(DUK_DPRINT("freeing finalize_list of heap: %p", (void *) heap));
	duk__free_finalize_list(heap);
#endif

	DUK_D(DUK_DPRINT("freeing string table of heap: %p", (void *) heap));
	duk__free_stringtable(heap);

	DUK_D(DUK_DPRINT("freeing heap structure: %p", (void *) heap));
	heap->free_func(heap->heap_udata, heap);
}

/*
 *  Allocate a heap.
 *
 *  String table is initialized with built-in strings from genbuiltins.py,
 *  either by dynamically creating the strings or by referring to ROM strings.
 */

#if defined(DUK_USE_ROM_STRINGS)
DUK_LOCAL duk_bool_t duk__init_heap_strings(duk_heap *heap) {
#if defined(DUK_USE_ASSERTIONS)
	duk_small_uint_t i;
#endif

	DUK_UNREF(heap);

	/* With ROM-based strings, heap->strs[] and thr->strs[] are omitted
	 * so nothing to initialize for strs[].
	 */

#if defined(DUK_USE_ASSERTIONS)
	for (i = 0; i < sizeof(duk_rom_strings_lookup) / sizeof(const duk_hstring *); i++) {
		const duk_hstring *h;
		duk_uint32_t hash;

		h = duk_rom_strings_lookup[i];
		while (h != NULL) {
			hash = duk_heap_hashstring(heap, (const duk_uint8_t *) DUK_HSTRING_GET_DATA(h), DUK_HSTRING_GET_BYTELEN(h));
			DUK_DD(DUK_DDPRINT("duk_rom_strings_lookup[%d] -> hash 0x%08lx, computed 0x%08lx",
			                   (int) i,
			                   (unsigned long) DUK_HSTRING_GET_HASH(h),
			                   (unsigned long) hash));
			DUK_ASSERT(hash == (duk_uint32_t) DUK_HSTRING_GET_HASH(h));

			h = (const duk_hstring *) h->hdr.h_next;
		}
	}
#endif
	return 1;
}
#else /* DUK_USE_ROM_STRINGS */

DUK_LOCAL duk_bool_t duk__init_heap_strings(duk_heap *heap) {
	duk_bitdecoder_ctx bd_ctx;
	duk_bitdecoder_ctx *bd = &bd_ctx; /* convenience */
	duk_small_uint_t i;

	duk_memzero(&bd_ctx, sizeof(bd_ctx));
	bd->data = (const duk_uint8_t *) duk_strings_data;
	bd->length = (duk_size_t) DUK_STRDATA_DATA_LENGTH;

	for (i = 0; i < DUK_HEAP_NUM_STRINGS; i++) {
		duk_uint8_t tmp[DUK_STRDATA_MAX_STRLEN];
		duk_small_uint_t len;
		duk_hstring *h;

		len = duk_bd_decode_bitpacked_string(bd, tmp);

		/* No need to length check string: it will never exceed even
		 * the 16-bit length maximum.
		 */
		DUK_ASSERT(len <= 0xffffUL);
		DUK_DDD(DUK_DDDPRINT("intern built-in string %ld", (long) i));
		h = duk_heap_strtable_intern(heap, tmp, len);
		if (!h) {
			goto failed;
		}
		DUK_ASSERT(!DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) h));

		/* Special flags checks.  Since these strings are always
		 * reachable and a string cannot appear twice in the string
		 * table, there's no need to check/set these flags elsewhere.
		 * The 'internal' flag is set by string intern code.
		 */
		if (i == DUK_STRIDX_EVAL || i == DUK_STRIDX_LC_ARGUMENTS) {
			DUK_HSTRING_SET_EVAL_OR_ARGUMENTS(h);
		}
		if (i >= DUK_STRIDX_START_RESERVED && i < DUK_STRIDX_END_RESERVED) {
			DUK_HSTRING_SET_RESERVED_WORD(h);
			if (i >= DUK_STRIDX_START_STRICT_RESERVED) {
				DUK_HSTRING_SET_STRICT_RESERVED_WORD(h);
			}
		}

		DUK_DDD(DUK_DDDPRINT("interned: %!O", (duk_heaphdr *) h));

		/* XXX: The incref macro takes a thread pointer but doesn't
		 * use it right now.
		 */
		DUK_HSTRING_INCREF(_never_referenced_, h);

#if defined(DUK_USE_HEAPPTR16)
		heap->strs16[i] = DUK_USE_HEAPPTR_ENC16(heap->heap_udata, (void *) h);
#else
		heap->strs[i] = h;
#endif
	}

	return 1;

failed:
	return 0;
}
#endif /* DUK_USE_ROM_STRINGS */

DUK_LOCAL duk_bool_t duk__init_heap_thread(duk_heap *heap) {
	duk_hthread *thr;

	DUK_D(DUK_DPRINT("heap init: alloc heap thread"));
	thr = duk_hthread_alloc_unchecked(heap, DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_THREAD));
	if (thr == NULL) {
		DUK_D(DUK_DPRINT("failed to alloc heap_thread"));
		return 0;
	}
	thr->state = DUK_HTHREAD_STATE_INACTIVE;
#if defined(DUK_USE_ROM_STRINGS)
	/* No strs[] pointer. */
#else /* DUK_USE_ROM_STRINGS */
#if defined(DUK_USE_HEAPPTR16)
	thr->strs16 = heap->strs16;
#else
	thr->strs = heap->strs;
#endif
#endif /* DUK_USE_ROM_STRINGS */

	heap->heap_thread = thr;
	DUK_HTHREAD_INCREF(thr, thr); /* Note: first argument not really used */

	/* 'thr' is now reachable */

	DUK_D(DUK_DPRINT("heap init: init heap thread stacks"));
	if (!duk_hthread_init_stacks(heap, thr)) {
		return 0;
	}

	/* XXX: this may now fail, and is not handled correctly */
	duk_hthread_create_builtin_objects(thr);

	/* default prototype */
	DUK_HOBJECT_SET_PROTOTYPE_INIT_INCREF(thr, (duk_hobject *) thr, thr->builtins[DUK_BIDX_THREAD_PROTOTYPE]);

	return 1;
}

#if defined(DUK_USE_DEBUG)
#define DUK__DUMPSZ(t) \
	do { \
		DUK_D(DUK_DPRINT("" #t "=%ld", (long) sizeof(t))); \
	} while (0)

/* These is not 100% because format would need to be non-portable "long long".
 * Also print out as doubles to catch cases where the "long" type is not wide
 * enough; the limits will then not be printed accurately but the magnitude
 * will be correct.
 */
#define DUK__DUMPLM_SIGNED_RAW(t, a, b) \
	do { \
		DUK_D(DUK_DPRINT(t "=[%ld,%ld]=[%lf,%lf]", (long) (a), (long) (b), (double) (a), (double) (b))); \
	} while (0)
#define DUK__DUMPLM_UNSIGNED_RAW(t, a, b) \
	do { \
		DUK_D(DUK_DPRINT(t "=[%lu,%lu]=[%lf,%lf]", (unsigned long) (a), (unsigned long) (b), (double) (a), (double) (b))); \
	} while (0)
#define DUK__DUMPLM_SIGNED(t) \
	do { \
		DUK__DUMPLM_SIGNED_RAW("DUK_" #t "_{MIN,MAX}", DUK_##t##_MIN, DUK_##t##_MAX); \
	} while (0)
#define DUK__DUMPLM_UNSIGNED(t) \
	do { \
		DUK__DUMPLM_UNSIGNED_RAW("DUK_" #t "_{MIN,MAX}", DUK_##t##_MIN, DUK_##t##_MAX); \
	} while (0)

DUK_LOCAL void duk__dump_type_sizes(void) {
	DUK_D(DUK_DPRINT("sizeof()"));

	/* basic platform types */
	DUK__DUMPSZ(char);
	DUK__DUMPSZ(short);
	DUK__DUMPSZ(int);
	DUK__DUMPSZ(long);
	DUK__DUMPSZ(double);
	DUK__DUMPSZ(void *);
	DUK__DUMPSZ(size_t);

	/* basic types from duk_features.h */
	DUK__DUMPSZ(duk_uint8_t);
	DUK__DUMPSZ(duk_int8_t);
	DUK__DUMPSZ(duk_uint16_t);
	DUK__DUMPSZ(duk_int16_t);
	DUK__DUMPSZ(duk_uint32_t);
	DUK__DUMPSZ(duk_int32_t);
	DUK__DUMPSZ(duk_uint64_t);
	DUK__DUMPSZ(duk_int64_t);
	DUK__DUMPSZ(duk_uint_least8_t);
	DUK__DUMPSZ(duk_int_least8_t);
	DUK__DUMPSZ(duk_uint_least16_t);
	DUK__DUMPSZ(duk_int_least16_t);
	DUK__DUMPSZ(duk_uint_least32_t);
	DUK__DUMPSZ(duk_int_least32_t);
#if defined(DUK_USE_64BIT_OPS)
	DUK__DUMPSZ(duk_uint_least64_t);
	DUK__DUMPSZ(duk_int_least64_t);
#endif
	DUK__DUMPSZ(duk_uint_fast8_t);
	DUK__DUMPSZ(duk_int_fast8_t);
	DUK__DUMPSZ(duk_uint_fast16_t);
	DUK__DUMPSZ(duk_int_fast16_t);
	DUK__DUMPSZ(duk_uint_fast32_t);
	DUK__DUMPSZ(duk_int_fast32_t);
#if defined(DUK_USE_64BIT_OPS)
	DUK__DUMPSZ(duk_uint_fast64_t);
	DUK__DUMPSZ(duk_int_fast64_t);
#endif
	DUK__DUMPSZ(duk_uintptr_t);
	DUK__DUMPSZ(duk_intptr_t);
	DUK__DUMPSZ(duk_uintmax_t);
	DUK__DUMPSZ(duk_intmax_t);
	DUK__DUMPSZ(duk_double_t);

	/* important chosen base types */
	DUK__DUMPSZ(duk_int_t);
	DUK__DUMPSZ(duk_uint_t);
	DUK__DUMPSZ(duk_int_fast_t);
	DUK__DUMPSZ(duk_uint_fast_t);
	DUK__DUMPSZ(duk_small_int_t);
	DUK__DUMPSZ(duk_small_uint_t);
	DUK__DUMPSZ(duk_small_int_fast_t);
	DUK__DUMPSZ(duk_small_uint_fast_t);

	/* some derived types */
	DUK__DUMPSZ(duk_codepoint_t);
	DUK__DUMPSZ(duk_ucodepoint_t);
	DUK__DUMPSZ(duk_idx_t);
	DUK__DUMPSZ(duk_errcode_t);
	DUK__DUMPSZ(duk_uarridx_t);

	/* tval */
	DUK__DUMPSZ(duk_double_union);
	DUK__DUMPSZ(duk_tval);

	/* structs from duk_forwdecl.h */
	DUK__DUMPSZ(duk_jmpbuf); /* just one 'int' for C++ exceptions */
	DUK__DUMPSZ(duk_heaphdr);
	DUK__DUMPSZ(duk_heaphdr_string);
	DUK__DUMPSZ(duk_hstring);
	DUK__DUMPSZ(duk_hstring_external);
	DUK__DUMPSZ(duk_hobject);
	DUK__DUMPSZ(duk_harray);
	DUK__DUMPSZ(duk_hcompfunc);
	DUK__DUMPSZ(duk_hnatfunc);
	DUK__DUMPSZ(duk_hdecenv);
	DUK__DUMPSZ(duk_hobjenv);
	DUK__DUMPSZ(duk_hthread);
#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
	DUK__DUMPSZ(duk_hbufobj);
#endif
	DUK__DUMPSZ(duk_hproxy);
	DUK__DUMPSZ(duk_hbuffer);
	DUK__DUMPSZ(duk_hbuffer_fixed);
	DUK__DUMPSZ(duk_hbuffer_dynamic);
	DUK__DUMPSZ(duk_hbuffer_external);
	DUK__DUMPSZ(duk_propaccessor);
	DUK__DUMPSZ(duk_propvalue);
	DUK__DUMPSZ(duk_propdesc);
	DUK__DUMPSZ(duk_heap);
	DUK__DUMPSZ(duk_activation);
	DUK__DUMPSZ(duk_catcher);
	DUK__DUMPSZ(duk_strcache_entry);
	DUK__DUMPSZ(duk_litcache_entry);
	DUK__DUMPSZ(duk_ljstate);
	DUK__DUMPSZ(duk_fixedbuffer);
	DUK__DUMPSZ(duk_bitdecoder_ctx);
	DUK__DUMPSZ(duk_bitencoder_ctx);
	DUK__DUMPSZ(duk_token);
	DUK__DUMPSZ(duk_re_token);
	DUK__DUMPSZ(duk_lexer_point);
	DUK__DUMPSZ(duk_lexer_ctx);
	DUK__DUMPSZ(duk_compiler_instr);
	DUK__DUMPSZ(duk_compiler_func);
	DUK__DUMPSZ(duk_compiler_ctx);
	DUK__DUMPSZ(duk_re_matcher_ctx);
	DUK__DUMPSZ(duk_re_compiler_ctx);
}
DUK_LOCAL void duk__dump_type_limits(void) {
	DUK_D(DUK_DPRINT("limits"));

	/* basic types */
	DUK__DUMPLM_SIGNED(INT8);
	DUK__DUMPLM_UNSIGNED(UINT8);
	DUK__DUMPLM_SIGNED(INT_FAST8);
	DUK__DUMPLM_UNSIGNED(UINT_FAST8);
	DUK__DUMPLM_SIGNED(INT_LEAST8);
	DUK__DUMPLM_UNSIGNED(UINT_LEAST8);
	DUK__DUMPLM_SIGNED(INT16);
	DUK__DUMPLM_UNSIGNED(UINT16);
	DUK__DUMPLM_SIGNED(INT_FAST16);
	DUK__DUMPLM_UNSIGNED(UINT_FAST16);
	DUK__DUMPLM_SIGNED(INT_LEAST16);
	DUK__DUMPLM_UNSIGNED(UINT_LEAST16);
	DUK__DUMPLM_SIGNED(INT32);
	DUK__DUMPLM_UNSIGNED(UINT32);
	DUK__DUMPLM_SIGNED(INT_FAST32);
	DUK__DUMPLM_UNSIGNED(UINT_FAST32);
	DUK__DUMPLM_SIGNED(INT_LEAST32);
	DUK__DUMPLM_UNSIGNED(UINT_LEAST32);
#if defined(DUK_USE_64BIT_OPS)
	DUK__DUMPLM_SIGNED(INT64);
	DUK__DUMPLM_UNSIGNED(UINT64);
	DUK__DUMPLM_SIGNED(INT_FAST64);
	DUK__DUMPLM_UNSIGNED(UINT_FAST64);
	DUK__DUMPLM_SIGNED(INT_LEAST64);
	DUK__DUMPLM_UNSIGNED(UINT_LEAST64);
#endif
	DUK__DUMPLM_SIGNED(INTPTR);
	DUK__DUMPLM_UNSIGNED(UINTPTR);
	DUK__DUMPLM_SIGNED(INTMAX);
	DUK__DUMPLM_UNSIGNED(UINTMAX);

	/* derived types */
	DUK__DUMPLM_SIGNED(INT);
	DUK__DUMPLM_UNSIGNED(UINT);
	DUK__DUMPLM_SIGNED(INT_FAST);
	DUK__DUMPLM_UNSIGNED(UINT_FAST);
	DUK__DUMPLM_SIGNED(SMALL_INT);
	DUK__DUMPLM_UNSIGNED(SMALL_UINT);
	DUK__DUMPLM_SIGNED(SMALL_INT_FAST);
	DUK__DUMPLM_UNSIGNED(SMALL_UINT_FAST);
}

DUK_LOCAL void duk__dump_misc_options(void) {
	DUK_D(DUK_DPRINT("DUK_VERSION: %ld", (long) DUK_VERSION));
	DUK_D(DUK_DPRINT("DUK_GIT_DESCRIBE: %s", DUK_GIT_DESCRIBE));
	DUK_D(DUK_DPRINT("OS string: %s", DUK_USE_OS_STRING));
	DUK_D(DUK_DPRINT("architecture string: %s", DUK_USE_ARCH_STRING));
	DUK_D(DUK_DPRINT("compiler string: %s", DUK_USE_COMPILER_STRING));
	DUK_D(DUK_DPRINT("debug level: %ld", (long) DUK_USE_DEBUG_LEVEL));
#if defined(DUK_USE_PACKED_TVAL)
	DUK_D(DUK_DPRINT("DUK_USE_PACKED_TVAL: yes"));
#else
	DUK_D(DUK_DPRINT("DUK_USE_PACKED_TVAL: no"));
#endif
#if defined(DUK_USE_VARIADIC_MACROS)
	DUK_D(DUK_DPRINT("DUK_USE_VARIADIC_MACROS: yes"));
#else
	DUK_D(DUK_DPRINT("DUK_USE_VARIADIC_MACROS: no"));
#endif
#if defined(DUK_USE_INTEGER_LE)
	DUK_D(DUK_DPRINT("integer endianness: little"));
#elif defined(DUK_USE_INTEGER_ME)
	DUK_D(DUK_DPRINT("integer endianness: mixed"));
#elif defined(DUK_USE_INTEGER_BE)
	DUK_D(DUK_DPRINT("integer endianness: big"));
#else
	DUK_D(DUK_DPRINT("integer endianness: ???"));
#endif
#if defined(DUK_USE_DOUBLE_LE)
	DUK_D(DUK_DPRINT("IEEE double endianness: little"));
#elif defined(DUK_USE_DOUBLE_ME)
	DUK_D(DUK_DPRINT("IEEE double endianness: mixed"));
#elif defined(DUK_USE_DOUBLE_BE)
	DUK_D(DUK_DPRINT("IEEE double endianness: big"));
#else
	DUK_D(DUK_DPRINT("IEEE double endianness: ???"));
#endif
}
#endif /* DUK_USE_DEBUG */

DUK_INTERNAL
duk_heap *duk_heap_alloc(duk_alloc_function alloc_func,
                         duk_realloc_function realloc_func,
                         duk_free_function free_func,
                         void *heap_udata,
                         duk_fatal_function fatal_func) {
	duk_heap *res = NULL;
	duk_uint32_t st_initsize;

	DUK_D(DUK_DPRINT("allocate heap"));

	/*
	 *  Random config sanity asserts
	 */

	DUK_ASSERT(DUK_USE_STRTAB_MINSIZE >= 64);

	DUK_ASSERT((DUK_HTYPE_STRING & 0x01U) == 0);
	DUK_ASSERT((DUK_HTYPE_BUFFER & 0x01U) == 0);
	DUK_ASSERT((DUK_HTYPE_OBJECT & 0x01U) == 1); /* DUK_HEAPHDR_IS_OBJECT() relies ont his. */

	/*
	 *  Debug dump type sizes
	 */

#if defined(DUK_USE_DEBUG)
	duk__dump_misc_options();
	duk__dump_type_sizes();
	duk__dump_type_limits();
#endif

	/*
	 *  If selftests enabled, run them as early as possible.
	 */

#if defined(DUK_USE_SELF_TESTS)
	DUK_D(DUK_DPRINT("run self tests"));
	if (duk_selftest_run_tests(alloc_func, realloc_func, free_func, heap_udata) > 0) {
		fatal_func(heap_udata, "self test(s) failed");
	}
	DUK_D(DUK_DPRINT("self tests passed"));
#endif

	/*
	 *  Important assert-like checks that should be enabled even
	 *  when assertions are otherwise not enabled.
	 */

#if defined(DUK_USE_EXEC_REGCONST_OPTIMIZE)
	/* Can't check sizeof() using preprocessor so explicit check.
	 * This will be optimized away in practice; unfortunately a
	 * warning is generated on some compilers as a result.
	 */
#if defined(DUK_USE_PACKED_TVAL)
	if (sizeof(duk_tval) != 8) {
#else
	if (sizeof(duk_tval) != 16) {
#endif
		fatal_func(heap_udata, "sizeof(duk_tval) not 8 or 16, cannot use DUK_USE_EXEC_REGCONST_OPTIMIZE option");
	}
#endif /* DUK_USE_EXEC_REGCONST_OPTIMIZE */

	/*
	 *  Computed values (e.g. INFINITY)
	 */

#if defined(DUK_USE_COMPUTED_NAN)
	do {
		/* Workaround for some exotic platforms where NAN is missing
		 * and the expression (0.0 / 0.0) does NOT result in a NaN.
		 * Such platforms use the global 'duk_computed_nan' which must
		 * be initialized at runtime.  Use 'volatile' to ensure that
		 * the compiler will actually do the computation and not try
		 * to do constant folding which might result in the original
		 * problem.
		 */
		volatile double dbl1 = 0.0;
		volatile double dbl2 = 0.0;
		duk_computed_nan = dbl1 / dbl2;
	} while (0);
#endif

#if defined(DUK_USE_COMPUTED_INFINITY)
	do {
		/* Similar workaround for INFINITY. */
		volatile double dbl1 = 1.0;
		volatile double dbl2 = 0.0;
		duk_computed_infinity = dbl1 / dbl2;
	} while (0);
#endif

	/*
	 *  Allocate heap struct
	 *
	 *  Use a raw call, all macros expect the heap to be initialized
	 */

#if defined(DUK_USE_INJECT_HEAP_ALLOC_ERROR) && (DUK_USE_INJECT_HEAP_ALLOC_ERROR == 1)
	goto failed;
#endif
	DUK_D(DUK_DPRINT("alloc duk_heap object"));
	res = (duk_heap *) alloc_func(heap_udata, sizeof(duk_heap));
	if (!res) {
		goto failed;
	}

	/*
	 *  Zero the struct, and start initializing roughly in order
	 */

	duk_memzero(res, sizeof(*res));
#if defined(DUK_USE_ASSERTIONS)
	res->heap_initializing = 1;
#endif

	/* explicit NULL inits */
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	res->heap_udata = NULL;
	res->heap_allocated = NULL;
#if defined(DUK_USE_REFERENCE_COUNTING)
	res->refzero_list = NULL;
#endif
#if defined(DUK_USE_FINALIZER_SUPPORT)
	res->finalize_list = NULL;
#if defined(DUK_USE_ASSERTIONS)
	res->currently_finalizing = NULL;
#endif
#endif
#if defined(DUK_USE_CACHE_ACTIVATION)
	res->activation_free = NULL;
#endif
#if defined(DUK_USE_CACHE_CATCHER)
	res->catcher_free = NULL;
#endif
	res->heap_thread = NULL;
	res->curr_thread = NULL;
	res->heap_object = NULL;
#if defined(DUK_USE_STRTAB_PTRCOMP)
	res->strtable16 = NULL;
#else
	res->strtable = NULL;
#endif
#if defined(DUK_USE_ROM_STRINGS)
	/* no res->strs[] */
#else /* DUK_USE_ROM_STRINGS */
#if defined(DUK_USE_HEAPPTR16)
	/* res->strs16[] is zeroed and zero decodes to NULL, so no NULL inits. */
#else
	{
		duk_small_uint_t i;
		for (i = 0; i < DUK_HEAP_NUM_STRINGS; i++) {
			res->strs[i] = NULL;
		}
	}
#endif
#endif /* DUK_USE_ROM_STRINGS */
#if defined(DUK_USE_DEBUGGER_SUPPORT)
	res->dbg_read_cb = NULL;
	res->dbg_write_cb = NULL;
	res->dbg_peek_cb = NULL;
	res->dbg_read_flush_cb = NULL;
	res->dbg_write_flush_cb = NULL;
	res->dbg_request_cb = NULL;
	res->dbg_udata = NULL;
	res->dbg_pause_act = NULL;
#endif
#endif /* DUK_USE_EXPLICIT_NULL_INIT */

	res->alloc_func = alloc_func;
	res->realloc_func = realloc_func;
	res->free_func = free_func;
	res->heap_udata = heap_udata;
	res->fatal_func = fatal_func;

	/* XXX: for now there's a pointer packing zero assumption, i.e.
	 * NULL <=> compressed pointer 0.  If this is removed, may need
	 * to precompute e.g. null16 here.
	 */

	/* res->ms_trigger_counter == 0 -> now causes immediate GC; which is OK */

	/* Prevent mark-and-sweep and finalizer execution until heap is completely
	 * initialized.
	 */
	DUK_ASSERT(res->ms_prevent_count == 0);
	DUK_ASSERT(res->pf_prevent_count == 0);
	res->ms_prevent_count = 1;
	res->pf_prevent_count = 1;
	DUK_ASSERT(res->ms_running == 0);

	res->call_recursion_depth = 0;
	res->call_recursion_limit = DUK_USE_NATIVE_CALL_RECLIMIT;

	/* XXX: use the pointer as a seed for now: mix in time at least */

	/* The casts through duk_uintptr_t is to avoid the following GCC warning:
	 *
	 *   warning: cast from pointer to integer of different size [-Wpointer-to-int-cast]
	 *
	 * This still generates a /Wp64 warning on VS2010 when compiling for x86.
	 */
#if defined(DUK_USE_ROM_STRINGS)
	/* XXX: make a common DUK_USE_ option, and allow custom fixed seed? */
	DUK_D(DUK_DPRINT("using rom strings, force heap hash_seed to fixed value 0x%08lx", (long) DUK__FIXED_HASH_SEED));
	res->hash_seed = (duk_uint32_t) DUK__FIXED_HASH_SEED;
#else /* DUK_USE_ROM_STRINGS */
	res->hash_seed = (duk_uint32_t) (duk_uintptr_t) res;
#if !defined(DUK_USE_STRHASH_DENSE)
	res->hash_seed ^= 5381; /* Bernstein hash init value is normally 5381; XOR it in in case pointer low bits are 0 */
#endif
#endif /* DUK_USE_ROM_STRINGS */

#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	res->lj.jmpbuf_ptr = NULL;
#endif
	DUK_ASSERT(res->lj.type == DUK_LJ_TYPE_UNKNOWN); /* zero */
	DUK_ASSERT(res->lj.iserror == 0);
	DUK_TVAL_SET_UNDEFINED(&res->lj.value1);
	DUK_TVAL_SET_UNDEFINED(&res->lj.value2);

	DUK_ASSERT_LJSTATE_UNSET(res);

	/*
	 *  Init stringtable: fixed variant
	 */

	st_initsize = DUK_USE_STRTAB_MINSIZE;
#if defined(DUK_USE_STRTAB_PTRCOMP)
	res->strtable16 = (duk_uint16_t *) alloc_func(heap_udata, sizeof(duk_uint16_t) * st_initsize);
	if (res->strtable16 == NULL) {
		goto failed;
	}
#else
	res->strtable = (duk_hstring **) alloc_func(heap_udata, sizeof(duk_hstring *) * st_initsize);
	if (res->strtable == NULL) {
		goto failed;
	}
#endif
	res->st_size = st_initsize;
	res->st_mask = st_initsize - 1;
#if (DUK_USE_STRTAB_MINSIZE != DUK_USE_STRTAB_MAXSIZE)
	DUK_ASSERT(res->st_count == 0);
#endif

#if defined(DUK_USE_STRTAB_PTRCOMP)
	/* zero assumption */
	duk_memzero(res->strtable16, sizeof(duk_uint16_t) * st_initsize);
#else
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	{
		duk_uint32_t i;
		for (i = 0; i < st_initsize; i++) {
			res->strtable[i] = NULL;
		}
	}
#else
	duk_memzero(res->strtable, sizeof(duk_hstring *) * st_initsize);
#endif /* DUK_USE_EXPLICIT_NULL_INIT */
#endif /* DUK_USE_STRTAB_PTRCOMP */

	/*
	 *  Init stringcache
	 */

#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	{
		duk_uint_t i;
		for (i = 0; i < DUK_HEAP_STRCACHE_SIZE; i++) {
			res->strcache[i].h = NULL;
		}
	}
#endif

	/*
	 *  Init litcache
	 */
#if defined(DUK_USE_LITCACHE_SIZE)
	DUK_ASSERT(DUK_USE_LITCACHE_SIZE > 0);
	DUK_ASSERT(DUK_IS_POWER_OF_TWO((duk_uint_t) DUK_USE_LITCACHE_SIZE));
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	{
		duk_uint_t i;
		for (i = 0; i < DUK_USE_LITCACHE_SIZE; i++) {
			res->litcache[i].addr = NULL;
			res->litcache[i].h = NULL;
		}
	}
#endif
#endif /* DUK_USE_LITCACHE_SIZE */

	/* XXX: error handling is incomplete.  It would be cleanest if
	 * there was a setjmp catchpoint, so that all init code could
	 * freely throw errors.  If that were the case, the return code
	 * passing here could be removed.
	 */

	/*
	 *  Init built-in strings
	 */

#if defined(DUK_USE_INJECT_HEAP_ALLOC_ERROR) && (DUK_USE_INJECT_HEAP_ALLOC_ERROR == 2)
	goto failed;
#endif
	DUK_D(DUK_DPRINT("heap init: initialize heap strings"));
	if (!duk__init_heap_strings(res)) {
		goto failed;
	}

	/*
	 *  Init the heap thread
	 */

#if defined(DUK_USE_INJECT_HEAP_ALLOC_ERROR) && (DUK_USE_INJECT_HEAP_ALLOC_ERROR == 3)
	goto failed;
#endif
	DUK_D(DUK_DPRINT("heap init: initialize heap thread"));
	if (!duk__init_heap_thread(res)) {
		goto failed;
	}

	/*
	 *  Init the heap object
	 */

#if defined(DUK_USE_INJECT_HEAP_ALLOC_ERROR) && (DUK_USE_INJECT_HEAP_ALLOC_ERROR == 4)
	goto failed;
#endif
	DUK_D(DUK_DPRINT("heap init: initialize heap object"));
	DUK_ASSERT(res->heap_thread != NULL);
	res->heap_object = duk_hobject_alloc_unchecked(res,
	                                               DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_FASTREFS |
	                                                   DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_OBJECT));
	if (res->heap_object == NULL) {
		goto failed;
	}
	DUK_HOBJECT_INCREF(res->heap_thread, res->heap_object);

	/*
	 *  Odds and ends depending on the heap thread
	 */

#if !defined(DUK_USE_GET_RANDOM_DOUBLE)
#if defined(DUK_USE_PREFER_SIZE) || !defined(DUK_USE_64BIT_OPS)
	res->rnd_state = (duk_uint32_t) duk_time_get_ecmascript_time(res->heap_thread);
	duk_util_tinyrandom_prepare_seed(res->heap_thread);
#else
	res->rnd_state[0] = (duk_uint64_t) duk_time_get_ecmascript_time(res->heap_thread);
	DUK_ASSERT(res->rnd_state[1] == 0); /* Not filled here, filled in by seed preparation. */
#if 0 /* Manual test values matching misc/xoroshiro128plus_test.c. */
	res->rnd_state[0] = DUK_U64_CONSTANT(0xdeadbeef12345678);
	res->rnd_state[1] = DUK_U64_CONSTANT(0xcafed00d12345678);
#endif
	duk_util_tinyrandom_prepare_seed(res->heap_thread);
	/* Mix in heap pointer: this ensures that if two Duktape heaps are
	 * created on the same millisecond, they get a different PRNG
	 * sequence (unless e.g. virtual memory addresses cause also the
	 * heap object pointer to be the same).
	 */
	{
		duk_uint64_t tmp_u64;
		tmp_u64 = 0;
		duk_memcpy((void *) &tmp_u64,
		           (const void *) &res,
		           (size_t) (sizeof(void *) >= sizeof(duk_uint64_t) ? sizeof(duk_uint64_t) : sizeof(void *)));
		res->rnd_state[1] ^= tmp_u64;
	}
	do {
		duk_small_uint_t i;
		for (i = 0; i < 10; i++) {
			/* Throw away a few initial random numbers just in
			 * case.  Probably unnecessary due to SplitMix64
			 * preparation.
			 */
			(void) duk_util_tinyrandom_get_double(res->heap_thread);
		}
	} while (0);
#endif
#endif

	/*
	 *  Allow finalizer and mark-and-sweep processing.
	 */

	DUK_D(DUK_DPRINT("heap init: allow finalizer/mark-and-sweep processing"));
	DUK_ASSERT(res->ms_prevent_count == 1);
	DUK_ASSERT(res->pf_prevent_count == 1);
	res->ms_prevent_count = 0;
	res->pf_prevent_count = 0;
	DUK_ASSERT(res->ms_running == 0);
#if defined(DUK_USE_ASSERTIONS)
	res->heap_initializing = 0;
#endif

	/*
	 *  All done.
	 */

	DUK_D(DUK_DPRINT("allocated heap: %p", (void *) res));
	return res;

failed:
	DUK_D(DUK_DPRINT("heap allocation failed"));

	if (res != NULL) {
		/* Assumes that allocated pointers and alloc funcs are valid
		 * if res exists.
		 */
		DUK_ASSERT(res->ms_prevent_count == 1);
		DUK_ASSERT(res->pf_prevent_count == 1);
		DUK_ASSERT(res->ms_running == 0);
		if (res->heap_thread != NULL) {
			res->ms_prevent_count = 0;
			res->pf_prevent_count = 0;
		}
#if defined(DUK_USE_ASSERTIONS)
		res->heap_initializing = 0;
#endif

		DUK_ASSERT(res->alloc_func != NULL);
		DUK_ASSERT(res->realloc_func != NULL);
		DUK_ASSERT(res->free_func != NULL);
		duk_heap_free(res);
	}

	return NULL;
}
