/*
 *  Bound function representation.
 */

#if !defined(DUK_HBOUNDFUNC_H_INCLUDED)
#define DUK_HBOUNDFUNC_H_INCLUDED

/* Artificial limit for args length.  Ensures arithmetic won't overflow
 * 32 bits when combining bound functions.
 */
#define DUK_HBOUNDFUNC_MAX_ARGS 0x20000000UL

#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL_DECL void duk_hboundfunc_assert_valid(duk_hboundfunc *h);
#define DUK_HBOUNDFUNC_ASSERT_VALID(h) \
	do { \
		duk_hboundfunc_assert_valid((h)); \
	} while (0)
#else
#define DUK_HBOUNDFUNC_ASSERT_VALID(h) \
	do { \
	} while (0)
#endif

struct duk_hboundfunc {
	/* Shared object part. */
	duk_hobject obj;

	/* Final target function, stored as duk_tval so that lightfunc can be
	 * represented too.
	 */
	duk_tval target;

	/* This binding. */
	duk_tval this_binding;

	/* Arguments to prepend. */
	duk_tval *args; /* Separate allocation. */
	duk_idx_t nargs;
};

#endif /* DUK_HBOUNDFUNC_H_INCLUDED */
