/*
 *  duk_hbuffer assertion helpers
 */

#include "duk_internal.h"

#if defined(DUK_USE_ASSERTIONS)

DUK_INTERNAL void duk_hbuffer_assert_valid(duk_hbuffer *h) {
	DUK_ASSERT(h != NULL);
}

#endif /* DUK_USE_ASSERTIONS */
