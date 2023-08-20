/*
 *  duk_hthread allocation and freeing.
 */

#include "duk_internal.h"

/*
 *  Allocate initial stacks for a thread.  Note that 'thr' must be reachable
 *  as a garbage collection may be triggered by the allocation attempts.
 *  Returns zero (without leaking memory) if init fails.
 */

DUK_INTERNAL duk_bool_t duk_hthread_init_stacks(duk_heap *heap, duk_hthread *thr) {
	duk_size_t alloc_size;
	duk_size_t i;

	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->valstack == NULL);
	DUK_ASSERT(thr->valstack_end == NULL);
	DUK_ASSERT(thr->valstack_alloc_end == NULL);
	DUK_ASSERT(thr->valstack_bottom == NULL);
	DUK_ASSERT(thr->valstack_top == NULL);
	DUK_ASSERT(thr->callstack_curr == NULL);

	/* valstack */
	DUK_ASSERT(DUK_VALSTACK_API_ENTRY_MINIMUM <= DUK_VALSTACK_INITIAL_SIZE);
	alloc_size = sizeof(duk_tval) * DUK_VALSTACK_INITIAL_SIZE;
	thr->valstack = (duk_tval *) DUK_ALLOC(heap, alloc_size);
	if (!thr->valstack) {
		goto fail;
	}
	duk_memzero(thr->valstack, alloc_size);
	thr->valstack_end = thr->valstack + DUK_VALSTACK_API_ENTRY_MINIMUM;
	thr->valstack_alloc_end = thr->valstack + DUK_VALSTACK_INITIAL_SIZE;
	thr->valstack_bottom = thr->valstack;
	thr->valstack_top = thr->valstack;

	for (i = 0; i < DUK_VALSTACK_INITIAL_SIZE; i++) {
		DUK_TVAL_SET_UNDEFINED(&thr->valstack[i]);
	}

	return 1;

fail:
	DUK_FREE(heap, thr->valstack);
	DUK_ASSERT(thr->callstack_curr == NULL);

	thr->valstack = NULL;
	return 0;
}

/* For indirect allocs. */

DUK_INTERNAL void *duk_hthread_get_valstack_ptr(duk_heap *heap, void *ud) {
	duk_hthread *thr = (duk_hthread *) ud;
	DUK_UNREF(heap);
	return (void *) thr->valstack;
}
