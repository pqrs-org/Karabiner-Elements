/*
 *  duk_hstring assertion helpers.
 */

#include "duk_internal.h"

#if defined(DUK_USE_ASSERTIONS)

DUK_INTERNAL void duk_hstring_assert_valid(duk_hstring *h) {
	DUK_ASSERT(h != NULL);
}

#endif /* DUK_USE_ASSERTIONS */
