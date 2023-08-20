/*
 *  Hobject allocation.
 *
 *  Provides primitive allocation functions for all object types (plain object,
 *  compiled function, native function, thread).  The object return is not yet
 *  in "heap allocated" list and has a refcount of zero, so caller must careful.
 */

/* XXX: In most cases there's no need for plain allocation without pushing
 * to the value stack.  Maybe rework contract?
 */

#include "duk_internal.h"

/*
 *  Helpers.
 */

DUK_LOCAL void duk__init_object_parts(duk_heap *heap, duk_uint_t hobject_flags, duk_hobject *obj) {
	DUK_ASSERT(obj != NULL);
	/* Zeroed by caller. */

	obj->hdr.h_flags = hobject_flags | DUK_HTYPE_OBJECT;
	DUK_ASSERT(DUK_HEAPHDR_GET_TYPE(&obj->hdr) == DUK_HTYPE_OBJECT); /* Assume zero shift. */

#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	DUK_HOBJECT_SET_PROTOTYPE(heap, obj, NULL);
	DUK_HOBJECT_SET_PROPS(heap, obj, NULL);
#endif
#if defined(DUK_USE_HEAPPTR16)
	/* Zero encoded pointer is required to match NULL. */
	DUK_HEAPHDR_SET_NEXT(heap, &obj->hdr, NULL);
#if defined(DUK_USE_DOUBLE_LINKED_HEAP)
	DUK_HEAPHDR_SET_PREV(heap, &obj->hdr, NULL);
#endif
#endif
	DUK_HEAPHDR_ASSERT_LINKS(heap, &obj->hdr);
	DUK_HEAP_INSERT_INTO_HEAP_ALLOCATED(heap, &obj->hdr);

	/* obj->props is intentionally left as NULL, and duk_hobject_props.c must deal
	 * with this properly.  This is intentional: empty objects consume a minimum
	 * amount of memory.  Further, an initial allocation might fail and cause
	 * 'obj' to "leak" (require a mark-and-sweep) since it is not reachable yet.
	 */
}

DUK_LOCAL void *duk__hobject_alloc_init(duk_hthread *thr, duk_uint_t hobject_flags, duk_size_t size) {
	void *res;

	res = (void *) DUK_ALLOC_CHECKED_ZEROED(thr, size);
	DUK_ASSERT(res != NULL);
	duk__init_object_parts(thr->heap, hobject_flags, (duk_hobject *) res);
	return res;
}

/*
 *  Allocate an duk_hobject.
 *
 *  The allocated object has no allocation for properties; the caller may
 *  want to force a resize if a desired size is known.
 *
 *  The allocated object has zero reference count and is not reachable.
 *  The caller MUST make the object reachable and increase its reference
 *  count before invoking any operation that might require memory allocation.
 */

DUK_INTERNAL duk_hobject *duk_hobject_alloc_unchecked(duk_heap *heap, duk_uint_t hobject_flags) {
	duk_hobject *res;

	DUK_ASSERT(heap != NULL);

	/* different memory layout, alloc size, and init */
	DUK_ASSERT((hobject_flags & DUK_HOBJECT_FLAG_COMPFUNC) == 0);
	DUK_ASSERT((hobject_flags & DUK_HOBJECT_FLAG_NATFUNC) == 0);
	DUK_ASSERT((hobject_flags & DUK_HOBJECT_FLAG_BOUNDFUNC) == 0);

	res = (duk_hobject *) DUK_ALLOC_ZEROED(heap, sizeof(duk_hobject));
	if (DUK_UNLIKELY(res == NULL)) {
		return NULL;
	}
	DUK_ASSERT(!DUK_HOBJECT_IS_THREAD(res));

	duk__init_object_parts(heap, hobject_flags, res);

	DUK_ASSERT(!DUK_HOBJECT_IS_THREAD(res));
	return res;
}

DUK_INTERNAL duk_hobject *duk_hobject_alloc(duk_hthread *thr, duk_uint_t hobject_flags) {
	duk_hobject *res;

	res = (duk_hobject *) duk__hobject_alloc_init(thr, hobject_flags, sizeof(duk_hobject));
	return res;
}

DUK_INTERNAL duk_hcompfunc *duk_hcompfunc_alloc(duk_hthread *thr, duk_uint_t hobject_flags) {
	duk_hcompfunc *res;

	res = (duk_hcompfunc *) duk__hobject_alloc_init(thr, hobject_flags, sizeof(duk_hcompfunc));
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
#if defined(DUK_USE_HEAPPTR16)
	/* NULL pointer is required to encode to zero, so memset is enough. */
#else
	res->data = NULL;
	res->funcs = NULL;
	res->bytecode = NULL;
#endif
	res->lex_env = NULL;
	res->var_env = NULL;
#endif

	return res;
}

DUK_INTERNAL duk_hnatfunc *duk_hnatfunc_alloc(duk_hthread *thr, duk_uint_t hobject_flags) {
	duk_hnatfunc *res;

	res = (duk_hnatfunc *) duk__hobject_alloc_init(thr, hobject_flags, sizeof(duk_hnatfunc));
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	res->func = NULL;
#endif

	return res;
}

DUK_INTERNAL duk_hboundfunc *duk_hboundfunc_alloc(duk_heap *heap, duk_uint_t hobject_flags) {
	duk_hboundfunc *res;

	res = (duk_hboundfunc *) DUK_ALLOC(heap, sizeof(duk_hboundfunc));
	if (!res) {
		return NULL;
	}
	duk_memzero(res, sizeof(duk_hboundfunc));

	duk__init_object_parts(heap, hobject_flags, &res->obj);

	DUK_TVAL_SET_UNDEFINED(&res->target);
	DUK_TVAL_SET_UNDEFINED(&res->this_binding);

#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	res->args = NULL;
#endif

	return res;
}

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL duk_hbufobj *duk_hbufobj_alloc(duk_hthread *thr, duk_uint_t hobject_flags) {
	duk_hbufobj *res;

	res = (duk_hbufobj *) duk__hobject_alloc_init(thr, hobject_flags, sizeof(duk_hbufobj));
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	res->buf = NULL;
	res->buf_prop = NULL;
#endif

	DUK_HBUFOBJ_ASSERT_VALID(res);
	return res;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/* Allocate a new thread.
 *
 * Leaves the built-ins array uninitialized.  The caller must either
 * initialize a new global context or share existing built-ins from
 * another thread.
 */
DUK_INTERNAL duk_hthread *duk_hthread_alloc_unchecked(duk_heap *heap, duk_uint_t hobject_flags) {
	duk_hthread *res;

	res = (duk_hthread *) DUK_ALLOC(heap, sizeof(duk_hthread));
	if (DUK_UNLIKELY(res == NULL)) {
		return NULL;
	}
	duk_memzero(res, sizeof(duk_hthread));

	duk__init_object_parts(heap, hobject_flags, &res->obj);

#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	res->ptr_curr_pc = NULL;
	res->heap = NULL;
	res->valstack = NULL;
	res->valstack_end = NULL;
	res->valstack_alloc_end = NULL;
	res->valstack_bottom = NULL;
	res->valstack_top = NULL;
	res->callstack_curr = NULL;
	res->resumer = NULL;
	res->compile_ctx = NULL,
#if defined(DUK_USE_HEAPPTR16)
	res->strs16 = NULL;
#else
	res->strs = NULL;
#endif
	{
		duk_small_uint_t i;
		for (i = 0; i < DUK_NUM_BUILTINS; i++) {
			res->builtins[i] = NULL;
		}
	}
#endif
	/* When nothing is running, API calls are in non-strict mode. */
	DUK_ASSERT(res->strict == 0);

	res->heap = heap;

	/* XXX: Any reason not to merge duk_hthread_alloc.c here? */
	return res;
}

DUK_INTERNAL duk_hthread *duk_hthread_alloc(duk_hthread *thr, duk_uint_t hobject_flags) {
	duk_hthread *res;

	res = duk_hthread_alloc_unchecked(thr->heap, hobject_flags);
	if (res == NULL) {
		DUK_ERROR_ALLOC_FAILED(thr);
		DUK_WO_NORETURN(return NULL;);
	}
	return res;
}

DUK_INTERNAL duk_harray *duk_harray_alloc(duk_hthread *thr, duk_uint_t hobject_flags) {
	duk_harray *res;

	res = (duk_harray *) duk__hobject_alloc_init(thr, hobject_flags, sizeof(duk_harray));

	DUK_ASSERT(res->length == 0);

	return res;
}

DUK_INTERNAL duk_hdecenv *duk_hdecenv_alloc(duk_hthread *thr, duk_uint_t hobject_flags) {
	duk_hdecenv *res;

	res = (duk_hdecenv *) duk__hobject_alloc_init(thr, hobject_flags, sizeof(duk_hdecenv));
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	res->thread = NULL;
	res->varmap = NULL;
#endif

	DUK_ASSERT(res->thread == NULL);
	DUK_ASSERT(res->varmap == NULL);
	DUK_ASSERT(res->regbase_byteoff == 0);

	return res;
}

DUK_INTERNAL duk_hobjenv *duk_hobjenv_alloc(duk_hthread *thr, duk_uint_t hobject_flags) {
	duk_hobjenv *res;

	res = (duk_hobjenv *) duk__hobject_alloc_init(thr, hobject_flags, sizeof(duk_hobjenv));
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	res->target = NULL;
#endif

	DUK_ASSERT(res->target == NULL);

	return res;
}

DUK_INTERNAL duk_hproxy *duk_hproxy_alloc(duk_hthread *thr, duk_uint_t hobject_flags) {
	duk_hproxy *res;

	res = (duk_hproxy *) duk__hobject_alloc_init(thr, hobject_flags, sizeof(duk_hproxy));

	/* Leave ->target and ->handler uninitialized, as caller will always
	 * explicitly initialize them before any side effects are possible.
	 */

	return res;
}
