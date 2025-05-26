/*
 *  Heap native function representation.
 */

#if !defined(DUK_HNATFUNC_H_INCLUDED)
#define DUK_HNATFUNC_H_INCLUDED

#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL_DECL void duk_hnatfunc_assert_valid(duk_hnatfunc *h);
#define DUK_HNATFUNC_ASSERT_VALID(h) \
	do { \
		duk_hnatfunc_assert_valid((h)); \
	} while (0)
#else
#define DUK_HNATFUNC_ASSERT_VALID(h) \
	do { \
	} while (0)
#endif

#define DUK_HNATFUNC_NARGS_VARARGS ((duk_int16_t) -1)
#define DUK_HNATFUNC_NARGS_MAX     ((duk_int16_t) 0x7fff)

struct duk_hnatfunc {
	/* shared object part */
	duk_hobject obj;

	duk_c_function func;
	duk_int16_t nargs;
	duk_int16_t magic;

	/* The 'magic' field allows an opaque 16-bit field to be accessed by the
	 * Duktape/C function.  This allows, for instance, the same native function
	 * to be used for a set of very similar functions, with the 'magic' field
	 * providing the necessary non-argument flags / values to guide the behavior
	 * of the native function.  The value is signed on purpose: it is easier to
	 * convert a signed value to unsigned (simply AND with 0xffff) than vice
	 * versa.
	 *
	 * Note: cannot place nargs/magic into the heaphdr flags, because
	 * duk_hobject takes almost all flags already.
	 */
};

#endif /* DUK_HNATFUNC_H_INCLUDED */
