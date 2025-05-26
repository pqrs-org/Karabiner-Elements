/*
 *  Environment object representation.
 */

#if !defined(DUK_HENV_H_INCLUDED)
#define DUK_HENV_H_INCLUDED

#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL_DECL void duk_hdecenv_assert_valid(duk_hdecenv *h);
DUK_INTERNAL_DECL void duk_hobjenv_assert_valid(duk_hobjenv *h);
#define DUK_HDECENV_ASSERT_VALID(h) \
	do { \
		duk_hdecenv_assert_valid((h)); \
	} while (0)
#define DUK_HOBJENV_ASSERT_VALID(h) \
	do { \
		duk_hobjenv_assert_valid((h)); \
	} while (0)
#else
#define DUK_HDECENV_ASSERT_VALID(h) \
	do { \
	} while (0)
#define DUK_HOBJENV_ASSERT_VALID(h) \
	do { \
	} while (0)
#endif

struct duk_hdecenv {
	/* Shared object part. */
	duk_hobject obj;

	/* These control variables provide enough information to access live
	 * variables for a closure that is still open.  If thread == NULL,
	 * the record is closed and the identifiers are in the property table.
	 */
	duk_hthread *thread;
	duk_hobject *varmap;
	duk_size_t regbase_byteoff;
};

struct duk_hobjenv {
	/* Shared object part. */
	duk_hobject obj;

	/* Target object and 'this' binding for object binding. */
	duk_hobject *target;

	/* The 'target' object is used as a this binding in only some object
	 * environments.  For example, the global environment does not provide
	 * a this binding, but a with statement does.
	 */
	duk_bool_t has_this;
};

#endif /* DUK_HENV_H_INCLUDED */
