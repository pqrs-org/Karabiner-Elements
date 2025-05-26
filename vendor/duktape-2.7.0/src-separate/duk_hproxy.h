/*
 *  Proxy object representation.
 */

#if !defined(DUK_HPROXY_H_INCLUDED)
#define DUK_HPROXY_H_INCLUDED

#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL_DECL void duk_hproxy_assert_valid(duk_hproxy *h);
#define DUK_HPROXY_ASSERT_VALID(h) \
	do { \
		duk_hproxy_assert_valid((h)); \
	} while (0)
#else
#define DUK_HPROXY_ASSERT_VALID(h) \
	do { \
	} while (0)
#endif

struct duk_hproxy {
	/* Shared object part. */
	duk_hobject obj;

	/* Proxy target object. */
	duk_hobject *target;

	/* Proxy handlers (traps). */
	duk_hobject *handler;
};

#endif /* DUK_HPROXY_H_INCLUDED */
