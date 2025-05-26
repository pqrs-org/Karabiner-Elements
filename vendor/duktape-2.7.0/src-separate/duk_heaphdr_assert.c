/*
 *  duk_heaphdr assertion helpers
 */

#include "duk_internal.h"

#if defined(DUK_USE_ASSERTIONS)

#if defined(DUK_USE_DOUBLE_LINKED_HEAP)
DUK_INTERNAL void duk_heaphdr_assert_links(duk_heap *heap, duk_heaphdr *h) {
	DUK_UNREF(heap);
	if (h != NULL) {
		duk_heaphdr *h_prev, *h_next;
		h_prev = DUK_HEAPHDR_GET_PREV(heap, h);
		h_next = DUK_HEAPHDR_GET_NEXT(heap, h);
		DUK_ASSERT(h_prev == NULL || (DUK_HEAPHDR_GET_NEXT(heap, h_prev) == h));
		DUK_ASSERT(h_next == NULL || (DUK_HEAPHDR_GET_PREV(heap, h_next) == h));
	}
}
#else
DUK_INTERNAL void duk_heaphdr_assert_links(duk_heap *heap, duk_heaphdr *h) {
	DUK_UNREF(heap);
	DUK_UNREF(h);
}
#endif

DUK_INTERNAL void duk_heaphdr_assert_valid(duk_heaphdr *h) {
	DUK_ASSERT(h != NULL);
	DUK_ASSERT(DUK_HEAPHDR_HTYPE_VALID(h));
}

/* Assert validity of a heaphdr, including all subclasses. */
DUK_INTERNAL void duk_heaphdr_assert_valid_subclassed(duk_heaphdr *h) {
	switch (DUK_HEAPHDR_GET_TYPE(h)) {
	case DUK_HTYPE_OBJECT: {
		duk_hobject *h_obj = (duk_hobject *) h;
		DUK_HOBJECT_ASSERT_VALID(h_obj);
		if (DUK_HOBJECT_IS_COMPFUNC(h_obj)) {
			DUK_HCOMPFUNC_ASSERT_VALID((duk_hcompfunc *) h_obj);
		} else if (DUK_HOBJECT_IS_NATFUNC(h_obj)) {
			DUK_HNATFUNC_ASSERT_VALID((duk_hnatfunc *) h_obj);
		} else if (DUK_HOBJECT_IS_DECENV(h_obj)) {
			DUK_HDECENV_ASSERT_VALID((duk_hdecenv *) h_obj);
		} else if (DUK_HOBJECT_IS_OBJENV(h_obj)) {
			DUK_HOBJENV_ASSERT_VALID((duk_hobjenv *) h_obj);
		} else if (DUK_HOBJECT_IS_BUFOBJ(h_obj)) {
#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
			DUK_HBUFOBJ_ASSERT_VALID((duk_hbufobj *) h_obj);
#endif
		} else if (DUK_HOBJECT_IS_BOUNDFUNC(h_obj)) {
			DUK_HBOUNDFUNC_ASSERT_VALID((duk_hboundfunc *) h_obj);
		} else if (DUK_HOBJECT_IS_PROXY(h_obj)) {
			DUK_HPROXY_ASSERT_VALID((duk_hproxy *) h_obj);
		} else if (DUK_HOBJECT_IS_THREAD(h_obj)) {
			DUK_HTHREAD_ASSERT_VALID((duk_hthread *) h_obj);
		} else {
			/* Just a plain object. */
			;
		}
		break;
	}
	case DUK_HTYPE_STRING: {
		duk_hstring *h_str = (duk_hstring *) h;
		DUK_HSTRING_ASSERT_VALID(h_str);
		break;
	}
	case DUK_HTYPE_BUFFER: {
		duk_hbuffer *h_buf = (duk_hbuffer *) h;
		DUK_HBUFFER_ASSERT_VALID(h_buf);
		break;
	}
	default: {
		DUK_ASSERT(0);
	}
	}
}

#endif /* DUK_USE_ASSERTIONS */
