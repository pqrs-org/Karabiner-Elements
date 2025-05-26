/*
 *  Memory utils.
 */

#include "duk_internal.h"

#if defined(DUK_USE_ALLOW_UNDEFINED_BEHAVIOR)
DUK_INTERNAL DUK_INLINE duk_small_int_t duk_memcmp_unsafe(const void *s1, const void *s2, duk_size_t len) {
	DUK_ASSERT(s1 != NULL || len == 0U);
	DUK_ASSERT(s2 != NULL || len == 0U);
	return DUK_MEMCMP(s1, s2, (size_t) len);
}

DUK_INTERNAL DUK_INLINE duk_small_int_t duk_memcmp(const void *s1, const void *s2, duk_size_t len) {
	DUK_ASSERT(s1 != NULL);
	DUK_ASSERT(s2 != NULL);
	return DUK_MEMCMP(s1, s2, (size_t) len);
}
#else /* DUK_USE_ALLOW_UNDEFINED_BEHAVIOR */
DUK_INTERNAL DUK_INLINE duk_small_int_t duk_memcmp_unsafe(const void *s1, const void *s2, duk_size_t len) {
	DUK_ASSERT(s1 != NULL || len == 0U);
	DUK_ASSERT(s2 != NULL || len == 0U);
	if (DUK_UNLIKELY(len == 0U)) {
		return 0;
	}
	DUK_ASSERT(s1 != NULL);
	DUK_ASSERT(s2 != NULL);
	return duk_memcmp(s1, s2, len);
}

DUK_INTERNAL DUK_INLINE duk_small_int_t duk_memcmp(const void *s1, const void *s2, duk_size_t len) {
	DUK_ASSERT(s1 != NULL);
	DUK_ASSERT(s2 != NULL);
	return DUK_MEMCMP(s1, s2, (size_t) len);
}
#endif /* DUK_USE_ALLOW_UNDEFINED_BEHAVIOR */
