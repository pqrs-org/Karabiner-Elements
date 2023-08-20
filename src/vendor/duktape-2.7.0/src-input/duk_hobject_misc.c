/*
 *  Misc support functions
 */

#include "duk_internal.h"

DUK_INTERNAL duk_bool_t duk_hobject_prototype_chain_contains(duk_hthread *thr,
                                                             duk_hobject *h,
                                                             duk_hobject *p,
                                                             duk_bool_t ignore_loop) {
	duk_uint_t sanity;

	DUK_ASSERT(thr != NULL);

	/* False if the object is NULL or the prototype 'p' is NULL.
	 * In particular, false if both are NULL (don't compare equal).
	 */
	if (h == NULL || p == NULL) {
		return 0;
	}

	sanity = DUK_HOBJECT_PROTOTYPE_CHAIN_SANITY;
	do {
		if (h == p) {
			return 1;
		}

		if (sanity-- == 0) {
			if (ignore_loop) {
				break;
			} else {
				DUK_ERROR_RANGE(thr, DUK_STR_PROTOTYPE_CHAIN_LIMIT);
				DUK_WO_NORETURN(return 0;);
			}
		}
		h = DUK_HOBJECT_GET_PROTOTYPE(thr->heap, h);
	} while (h);

	return 0;
}

DUK_INTERNAL void duk_hobject_set_prototype_updref(duk_hthread *thr, duk_hobject *h, duk_hobject *p) {
#if defined(DUK_USE_REFERENCE_COUNTING)
	duk_hobject *tmp;

	DUK_ASSERT(h);
	tmp = DUK_HOBJECT_GET_PROTOTYPE(thr->heap, h);
	DUK_HOBJECT_SET_PROTOTYPE(thr->heap, h, p);
	DUK_HOBJECT_INCREF_ALLOWNULL(thr, p); /* avoid problems if p == h->prototype */
	DUK_HOBJECT_DECREF_ALLOWNULL(thr, tmp);
#else
	DUK_ASSERT(h);
	DUK_UNREF(thr);
	DUK_HOBJECT_SET_PROTOTYPE(thr->heap, h, p);
#endif
}
