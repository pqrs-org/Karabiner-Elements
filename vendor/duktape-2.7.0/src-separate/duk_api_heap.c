/*
 *  Heap creation and destruction
 */

#include "duk_internal.h"

typedef struct duk_internal_thread_state duk_internal_thread_state;

struct duk_internal_thread_state {
	duk_ljstate lj;
	duk_bool_t creating_error;
	duk_hthread *curr_thread;
	duk_uint8_t thread_state;
	duk_int_t call_recursion_depth;
};

DUK_EXTERNAL duk_hthread *duk_create_heap(duk_alloc_function alloc_func,
                                          duk_realloc_function realloc_func,
                                          duk_free_function free_func,
                                          void *heap_udata,
                                          duk_fatal_function fatal_handler) {
	duk_heap *heap = NULL;
	duk_hthread *thr;

	/* Assume that either all memory funcs are NULL or non-NULL, mixed
	 * cases will now be unsafe.
	 */

	/* XXX: just assert non-NULL values here and make caller arguments
	 * do the defaulting to the default implementations (smaller code)?
	 */

	if (!alloc_func) {
		DUK_ASSERT(realloc_func == NULL);
		DUK_ASSERT(free_func == NULL);
#if defined(DUK_USE_PROVIDE_DEFAULT_ALLOC_FUNCTIONS)
		alloc_func = duk_default_alloc_function;
		realloc_func = duk_default_realloc_function;
		free_func = duk_default_free_function;
#else
		DUK_D(DUK_DPRINT("no allocation functions given and no default providers"));
		return NULL;
#endif
	} else {
		DUK_ASSERT(realloc_func != NULL);
		DUK_ASSERT(free_func != NULL);
	}

	if (!fatal_handler) {
		fatal_handler = duk_default_fatal_handler;
	}

	DUK_ASSERT(alloc_func != NULL);
	DUK_ASSERT(realloc_func != NULL);
	DUK_ASSERT(free_func != NULL);
	DUK_ASSERT(fatal_handler != NULL);

	heap = duk_heap_alloc(alloc_func, realloc_func, free_func, heap_udata, fatal_handler);
	if (!heap) {
		return NULL;
	}
	thr = heap->heap_thread;
	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	return thr;
}

DUK_EXTERNAL void duk_destroy_heap(duk_hthread *thr) {
	duk_heap *heap;

	if (!thr) {
		return;
	}
	DUK_ASSERT_API_ENTRY(thr);
	heap = thr->heap;
	DUK_ASSERT(heap != NULL);

	duk_heap_free(heap);
}

DUK_EXTERNAL void duk_suspend(duk_hthread *thr, duk_thread_state *state) {
	duk_internal_thread_state *snapshot = (duk_internal_thread_state *) (void *) state;
	duk_heap *heap;
	duk_ljstate *lj;

	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(state != NULL); /* unvalidated */

	/* Currently not supported when called from within a finalizer.
	 * If that is done, the finalizer will remain running indefinitely,
	 * preventing other finalizers from executing.  The assert is a bit
	 * wider, checking that it would be OK to run pending finalizers.
	 */
	DUK_ASSERT(thr->heap->pf_prevent_count == 0);

	/* Currently not supported to duk_suspend() from an errCreate()
	 * call.
	 */
	DUK_ASSERT(thr->heap->creating_error == 0);

	heap = thr->heap;
	lj = &heap->lj;

	duk_push_tval(thr, &lj->value1);
	duk_push_tval(thr, &lj->value2);

	/* XXX: creating_error == 0 is asserted above, so no need to store. */
	duk_memcpy((void *) &snapshot->lj, (const void *) lj, sizeof(duk_ljstate));
	snapshot->creating_error = heap->creating_error;
	snapshot->curr_thread = heap->curr_thread;
	snapshot->thread_state = thr->state;
	snapshot->call_recursion_depth = heap->call_recursion_depth;

	lj->jmpbuf_ptr = NULL;
	lj->type = DUK_LJ_TYPE_UNKNOWN;
	DUK_TVAL_SET_UNDEFINED(&lj->value1);
	DUK_TVAL_SET_UNDEFINED(&lj->value2);
	heap->creating_error = 0;
	heap->curr_thread = NULL;
	heap->call_recursion_depth = 0;

	thr->state = DUK_HTHREAD_STATE_INACTIVE;
}

DUK_EXTERNAL void duk_resume(duk_hthread *thr, const duk_thread_state *state) {
	const duk_internal_thread_state *snapshot = (const duk_internal_thread_state *) (const void *) state;
	duk_heap *heap;

	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(state != NULL); /* unvalidated */

	/* Shouldn't be necessary if duk_suspend() is called before
	 * duk_resume(), but assert in case API sequence is incorrect.
	 */
	DUK_ASSERT(thr->heap->pf_prevent_count == 0);
	DUK_ASSERT(thr->heap->creating_error == 0);

	thr->state = snapshot->thread_state;

	heap = thr->heap;

	duk_memcpy((void *) &heap->lj, (const void *) &snapshot->lj, sizeof(duk_ljstate));
	heap->creating_error = snapshot->creating_error;
	heap->curr_thread = snapshot->curr_thread;
	heap->call_recursion_depth = snapshot->call_recursion_depth;

	duk_pop_2(thr);
}

/* XXX: better place for this */
DUK_EXTERNAL void duk_set_global_object(duk_hthread *thr) {
	duk_hobject *h_glob;
	duk_hobject *h_prev_glob;
	duk_hobjenv *h_env;
	duk_hobject *h_prev_env;

	DUK_ASSERT_API_ENTRY(thr);

	DUK_D(DUK_DPRINT("replace global object with: %!T", duk_get_tval(thr, -1)));

	h_glob = duk_require_hobject(thr, -1);
	DUK_ASSERT(h_glob != NULL);

	/*
	 *  Replace global object.
	 */

	h_prev_glob = thr->builtins[DUK_BIDX_GLOBAL];
	DUK_UNREF(h_prev_glob);
	thr->builtins[DUK_BIDX_GLOBAL] = h_glob;
	DUK_HOBJECT_INCREF(thr, h_glob);
	DUK_HOBJECT_DECREF_ALLOWNULL(thr, h_prev_glob); /* side effects, in theory (referenced by global env) */

	/*
	 *  Replace lexical environment for global scope
	 *
	 *  Create a new object environment for the global lexical scope.
	 *  We can't just reset the _Target property of the current one,
	 *  because the lexical scope is shared by other threads with the
	 *  same (initial) built-ins.
	 */

	h_env = duk_hobjenv_alloc(thr, DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_OBJENV));
	DUK_ASSERT(h_env != NULL);
	DUK_ASSERT(DUK_HOBJECT_GET_PROTOTYPE(thr->heap, (duk_hobject *) h_env) == NULL);

	DUK_ASSERT(h_env->target == NULL);
	DUK_ASSERT(h_glob != NULL);
	h_env->target = h_glob;
	DUK_HOBJECT_INCREF(thr, h_glob);
	DUK_ASSERT(h_env->has_this == 0);

	/* [ ... new_glob ] */

	h_prev_env = thr->builtins[DUK_BIDX_GLOBAL_ENV];
	thr->builtins[DUK_BIDX_GLOBAL_ENV] = (duk_hobject *) h_env;
	DUK_HOBJECT_INCREF(thr, (duk_hobject *) h_env);
	DUK_HOBJECT_DECREF_ALLOWNULL(thr, h_prev_env); /* side effects */
	DUK_UNREF(h_env); /* without refcounts */
	DUK_UNREF(h_prev_env);

	/* [ ... new_glob ] */

	duk_pop(thr);

	/* [ ... ] */
}
