/*
 *  Support functions for duk_heap.
 */

#include "duk_internal.h"

DUK_INTERNAL void duk_heap_insert_into_heap_allocated(duk_heap *heap, duk_heaphdr *hdr) {
	duk_heaphdr *root;

	DUK_ASSERT(DUK_HEAPHDR_GET_TYPE(hdr) != DUK_HTYPE_STRING);

	root = heap->heap_allocated;
#if defined(DUK_USE_DOUBLE_LINKED_HEAP)
	if (root != NULL) {
		DUK_ASSERT(DUK_HEAPHDR_GET_PREV(heap, root) == NULL);
		DUK_HEAPHDR_SET_PREV(heap, root, hdr);
	}
	DUK_HEAPHDR_SET_PREV(heap, hdr, NULL);
#endif
	DUK_HEAPHDR_SET_NEXT(heap, hdr, root);
	DUK_HEAPHDR_ASSERT_LINKS(heap, hdr);
	DUK_HEAPHDR_ASSERT_LINKS(heap, root);
	heap->heap_allocated = hdr;
}

#if defined(DUK_USE_REFERENCE_COUNTING)
DUK_INTERNAL void duk_heap_remove_from_heap_allocated(duk_heap *heap, duk_heaphdr *hdr) {
	duk_heaphdr *prev;
	duk_heaphdr *next;

	/* Strings are in string table. */
	DUK_ASSERT(hdr != NULL);
	DUK_ASSERT(DUK_HEAPHDR_GET_TYPE(hdr) != DUK_HTYPE_STRING);

	/* Target 'hdr' must be in heap_allocated (not e.g. finalize_list).
	 * If not, heap lists will become corrupted so assert early for it.
	 */
#if defined(DUK_USE_ASSERTIONS)
	{
		duk_heaphdr *tmp;
		for (tmp = heap->heap_allocated; tmp != NULL; tmp = DUK_HEAPHDR_GET_NEXT(heap, tmp)) {
			if (tmp == hdr) {
				break;
			}
		}
		DUK_ASSERT(tmp == hdr);
	}
#endif

	/* Read/write only once to minimize pointer compression calls. */
	prev = DUK_HEAPHDR_GET_PREV(heap, hdr);
	next = DUK_HEAPHDR_GET_NEXT(heap, hdr);

	if (prev != NULL) {
		DUK_ASSERT(heap->heap_allocated != hdr);
		DUK_HEAPHDR_SET_NEXT(heap, prev, next);
	} else {
		DUK_ASSERT(heap->heap_allocated == hdr);
		heap->heap_allocated = next;
	}
	if (next != NULL) {
		DUK_HEAPHDR_SET_PREV(heap, next, prev);
	} else {
		;
	}
}
#endif /* DUK_USE_REFERENCE_COUNTING */

#if defined(DUK_USE_FINALIZER_SUPPORT)
DUK_INTERNAL void duk_heap_insert_into_finalize_list(duk_heap *heap, duk_heaphdr *hdr) {
	duk_heaphdr *root;

	root = heap->finalize_list;
#if defined(DUK_USE_DOUBLE_LINKED_HEAP)
	DUK_HEAPHDR_SET_PREV(heap, hdr, NULL);
	if (root != NULL) {
		DUK_ASSERT(DUK_HEAPHDR_GET_PREV(heap, root) == NULL);
		DUK_HEAPHDR_SET_PREV(heap, root, hdr);
	}
#endif
	DUK_HEAPHDR_SET_NEXT(heap, hdr, root);
	DUK_HEAPHDR_ASSERT_LINKS(heap, hdr);
	DUK_HEAPHDR_ASSERT_LINKS(heap, root);
	heap->finalize_list = hdr;
}
#endif /* DUK_USE_FINALIZER_SUPPORT */

#if defined(DUK_USE_FINALIZER_SUPPORT)
DUK_INTERNAL void duk_heap_remove_from_finalize_list(duk_heap *heap, duk_heaphdr *hdr) {
#if defined(DUK_USE_DOUBLE_LINKED_HEAP)
	duk_heaphdr *next;
	duk_heaphdr *prev;

	next = DUK_HEAPHDR_GET_NEXT(heap, hdr);
	prev = DUK_HEAPHDR_GET_PREV(heap, hdr);
	if (next != NULL) {
		DUK_ASSERT(DUK_HEAPHDR_GET_PREV(heap, next) == hdr);
		DUK_HEAPHDR_SET_PREV(heap, next, prev);
	}
	if (prev == NULL) {
		DUK_ASSERT(hdr == heap->finalize_list);
		heap->finalize_list = next;
	} else {
		DUK_ASSERT(hdr != heap->finalize_list);
		DUK_HEAPHDR_SET_NEXT(heap, prev, next);
	}
#else
	duk_heaphdr *next;
	duk_heaphdr *curr;

	/* Random removal is expensive: we need to locate the previous element
	 * because we don't have a 'prev' pointer.
	 */
	curr = heap->finalize_list;
	if (curr == hdr) {
		heap->finalize_list = DUK_HEAPHDR_GET_NEXT(heap, curr);
	} else {
		DUK_ASSERT(hdr != heap->finalize_list);
		for (;;) {
			DUK_ASSERT(curr != NULL); /* Caller responsibility. */

			next = DUK_HEAPHDR_GET_NEXT(heap, curr);
			if (next == hdr) {
				next = DUK_HEAPHDR_GET_NEXT(heap, hdr);
				DUK_HEAPHDR_SET_NEXT(heap, curr, next);
				break;
			}
		}
	}
#endif
}
#endif /* DUK_USE_FINALIZER_SUPPORT */

#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL duk_bool_t duk_heap_in_heap_allocated(duk_heap *heap, duk_heaphdr *ptr) {
	duk_heaphdr *curr;
	DUK_ASSERT(heap != NULL);

	for (curr = heap->heap_allocated; curr != NULL; curr = DUK_HEAPHDR_GET_NEXT(heap, curr)) {
		if (curr == ptr) {
			return 1;
		}
	}
	return 0;
}
#endif /* DUK_USE_ASSERTIONS */

#if defined(DUK_USE_INTERRUPT_COUNTER)
DUK_INTERNAL void duk_heap_switch_thread(duk_heap *heap, duk_hthread *new_thr) {
	duk_hthread *curr_thr;

	DUK_ASSERT(heap != NULL);

	if (new_thr != NULL) {
		curr_thr = heap->curr_thread;
		if (curr_thr == NULL) {
			/* For initial entry use default value; zero forces an
			 * interrupt before executing the first insturction.
			 */
			DUK_DD(DUK_DDPRINT("switch thread, initial entry, init default interrupt counter"));
			new_thr->interrupt_counter = 0;
			new_thr->interrupt_init = 0;
		} else {
			/* Copy interrupt counter/init value state to new thread (if any).
			 * It's OK for new_thr to be the same as curr_thr.
			 */
#if defined(DUK_USE_DEBUG)
			if (new_thr != curr_thr) {
				DUK_DD(DUK_DDPRINT("switch thread, not initial entry, copy interrupt counter"));
			}
#endif
			new_thr->interrupt_counter = curr_thr->interrupt_counter;
			new_thr->interrupt_init = curr_thr->interrupt_init;
		}
	} else {
		DUK_DD(DUK_DDPRINT("switch thread, new thread is NULL, no interrupt counter changes"));
	}

	heap->curr_thread = new_thr; /* may be NULL */
}
#endif /* DUK_USE_INTERRUPT_COUNTER */

#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL void duk_heap_assert_valid(duk_heap *heap) {
	DUK_ASSERT(heap != NULL);
}
#endif
