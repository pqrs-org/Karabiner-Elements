/*
 *  Reference counting helper macros.  The macros take a thread argument
 *  and must thus always be executed in a specific thread context.  The
 *  thread argument is not really needed anymore: DECREF can operate with
 *  a heap pointer only, and INCREF needs neither.
 */

#if !defined(DUK_REFCOUNT_H_INCLUDED)
#define DUK_REFCOUNT_H_INCLUDED

#if defined(DUK_USE_REFERENCE_COUNTING)

#if defined(DUK_USE_ROM_OBJECTS)
/* With ROM objects "needs refcount update" is true when the value is
 * heap allocated and is not a ROM object.
 */
/* XXX: double evaluation for 'tv' argument. */
#define DUK_TVAL_NEEDS_REFCOUNT_UPDATE(tv) \
	(DUK_TVAL_IS_HEAP_ALLOCATED((tv)) && !DUK_HEAPHDR_HAS_READONLY(DUK_TVAL_GET_HEAPHDR((tv))))
#define DUK_HEAPHDR_NEEDS_REFCOUNT_UPDATE(h) (!DUK_HEAPHDR_HAS_READONLY((h)))
#else /* DUK_USE_ROM_OBJECTS */
/* Without ROM objects "needs refcount update" == is heap allocated. */
#define DUK_TVAL_NEEDS_REFCOUNT_UPDATE(tv)   DUK_TVAL_IS_HEAP_ALLOCATED((tv))
#define DUK_HEAPHDR_NEEDS_REFCOUNT_UPDATE(h) 1
#endif /* DUK_USE_ROM_OBJECTS */

/* Fast variants, inline refcount operations except for refzero handling.
 * Can be used explicitly when speed is always more important than size.
 * For a good compiler and a single file build, these are basically the
 * same as a forced inline.
 */
#define DUK_TVAL_INCREF_FAST(thr, tv) \
	do { \
		duk_tval *duk__tv = (tv); \
		DUK_ASSERT(duk__tv != NULL); \
		if (DUK_TVAL_NEEDS_REFCOUNT_UPDATE(duk__tv)) { \
			duk_heaphdr *duk__h = DUK_TVAL_GET_HEAPHDR(duk__tv); \
			DUK_ASSERT(duk__h != NULL); \
			DUK_ASSERT(DUK_HEAPHDR_HTYPE_VALID(duk__h)); \
			DUK_HEAPHDR_PREINC_REFCOUNT(duk__h); \
			DUK_ASSERT(DUK_HEAPHDR_GET_REFCOUNT(duk__h) != 0); /* No wrapping. */ \
		} \
	} while (0)
#define DUK_TVAL_DECREF_FAST(thr, tv) \
	do { \
		duk_tval *duk__tv = (tv); \
		DUK_ASSERT(duk__tv != NULL); \
		if (DUK_TVAL_NEEDS_REFCOUNT_UPDATE(duk__tv)) { \
			duk_heaphdr *duk__h = DUK_TVAL_GET_HEAPHDR(duk__tv); \
			DUK_ASSERT(duk__h != NULL); \
			DUK_ASSERT(DUK_HEAPHDR_HTYPE_VALID(duk__h)); \
			DUK_ASSERT(DUK_HEAPHDR_GET_REFCOUNT(duk__h) > 0); \
			if (DUK_HEAPHDR_PREDEC_REFCOUNT(duk__h) == 0) { \
				duk_heaphdr_refzero((thr), duk__h); \
			} \
		} \
	} while (0)
#define DUK_TVAL_DECREF_NORZ_FAST(thr, tv) \
	do { \
		duk_tval *duk__tv = (tv); \
		DUK_ASSERT(duk__tv != NULL); \
		if (DUK_TVAL_NEEDS_REFCOUNT_UPDATE(duk__tv)) { \
			duk_heaphdr *duk__h = DUK_TVAL_GET_HEAPHDR(duk__tv); \
			DUK_ASSERT(duk__h != NULL); \
			DUK_ASSERT(DUK_HEAPHDR_HTYPE_VALID(duk__h)); \
			DUK_ASSERT(DUK_HEAPHDR_GET_REFCOUNT(duk__h) > 0); \
			if (DUK_HEAPHDR_PREDEC_REFCOUNT(duk__h) == 0) { \
				duk_heaphdr_refzero_norz((thr), duk__h); \
			} \
		} \
	} while (0)
#define DUK_HEAPHDR_INCREF_FAST(thr, h) \
	do { \
		duk_heaphdr *duk__h = (duk_heaphdr *) (h); \
		DUK_ASSERT(duk__h != NULL); \
		DUK_ASSERT(DUK_HEAPHDR_HTYPE_VALID(duk__h)); \
		if (DUK_HEAPHDR_NEEDS_REFCOUNT_UPDATE(duk__h)) { \
			DUK_HEAPHDR_PREINC_REFCOUNT(duk__h); \
			DUK_ASSERT(DUK_HEAPHDR_GET_REFCOUNT(duk__h) != 0); /* No wrapping. */ \
		} \
	} while (0)
#define DUK_HEAPHDR_DECREF_FAST_RAW(thr, h, rzcall, rzcast) \
	do { \
		duk_heaphdr *duk__h = (duk_heaphdr *) (h); \
		DUK_ASSERT(duk__h != NULL); \
		DUK_ASSERT(DUK_HEAPHDR_HTYPE_VALID(duk__h)); \
		DUK_ASSERT(DUK_HEAPHDR_GET_REFCOUNT(duk__h) > 0); \
		if (DUK_HEAPHDR_NEEDS_REFCOUNT_UPDATE(duk__h)) { \
			if (DUK_HEAPHDR_PREDEC_REFCOUNT(duk__h) == 0) { \
				(rzcall)((thr), (rzcast) duk__h); \
			} \
		} \
	} while (0)
#define DUK_HEAPHDR_DECREF_FAST(thr, h)      DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_heaphdr_refzero, duk_heaphdr *)
#define DUK_HEAPHDR_DECREF_NORZ_FAST(thr, h) DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_heaphdr_refzero_norz, duk_heaphdr *)

/* Slow variants, call to a helper to reduce code size.
 * Can be used explicitly when size is always more important than speed.
 */
#define DUK_TVAL_INCREF_SLOW(thr, tv) \
	do { \
		duk_tval_incref((tv)); \
	} while (0)
#define DUK_TVAL_DECREF_SLOW(thr, tv) \
	do { \
		duk_tval_decref((thr), (tv)); \
	} while (0)
#define DUK_TVAL_DECREF_NORZ_SLOW(thr, tv) \
	do { \
		duk_tval_decref_norz((thr), (tv)); \
	} while (0)
#define DUK_HEAPHDR_INCREF_SLOW(thr, h) \
	do { \
		duk_heaphdr_incref((duk_heaphdr *) (h)); \
	} while (0)
#define DUK_HEAPHDR_DECREF_SLOW(thr, h) \
	do { \
		duk_heaphdr_decref((thr), (duk_heaphdr *) (h)); \
	} while (0)
#define DUK_HEAPHDR_DECREF_NORZ_SLOW(thr, h) \
	do { \
		duk_heaphdr_decref_norz((thr), (duk_heaphdr *) (h)); \
	} while (0)
#define DUK_HSTRING_INCREF_SLOW(thr, h) \
	do { \
		duk_heaphdr_incref((duk_heaphdr *) (h)); \
	} while (0)
#define DUK_HSTRING_DECREF_SLOW(thr, h) \
	do { \
		duk_heaphdr_decref((thr), (duk_heaphdr *) (h)); \
	} while (0)
#define DUK_HSTRING_DECREF_NORZ_SLOW(thr, h) \
	do { \
		duk_heaphdr_decref_norz((thr), (duk_heaphdr *) (h)); \
	} while (0)
#define DUK_HBUFFER_INCREF_SLOW(thr, h) \
	do { \
		duk_heaphdr_incref((duk_heaphdr *) (h)); \
	} while (0)
#define DUK_HBUFFER_DECREF_SLOW(thr, h) \
	do { \
		duk_heaphdr_decref((thr), (duk_heaphdr *) (h)); \
	} while (0)
#define DUK_HBUFFER_DECREF_NORZ_SLOW(thr, h) \
	do { \
		duk_heaphdr_decref_norz((thr), (duk_heaphdr *) (h)); \
	} while (0)
#define DUK_HOBJECT_INCREF_SLOW(thr, h) \
	do { \
		duk_heaphdr_incref((duk_heaphdr *) (h)); \
	} while (0)
#define DUK_HOBJECT_DECREF_SLOW(thr, h) \
	do { \
		duk_heaphdr_decref((thr), (duk_heaphdr *) (h)); \
	} while (0)
#define DUK_HOBJECT_DECREF_NORZ_SLOW(thr, h) \
	do { \
		duk_heaphdr_decref_norz((thr), (duk_heaphdr *) (h)); \
	} while (0)

/* Default variants.  Selection depends on speed/size preference.
 * Concretely: with gcc 4.8.1 -Os x64 the difference in final binary
 * is about +1kB for _FAST variants.
 */
#if defined(DUK_USE_FAST_REFCOUNT_DEFAULT)
/* XXX: It would be nice to specialize for specific duk_hobject subtypes
 * but current refzero queue handling prevents that.
 */
#define DUK_TVAL_INCREF(thr, tv)        DUK_TVAL_INCREF_FAST((thr), (tv))
#define DUK_TVAL_DECREF(thr, tv)        DUK_TVAL_DECREF_FAST((thr), (tv))
#define DUK_TVAL_DECREF_NORZ(thr, tv)   DUK_TVAL_DECREF_NORZ_FAST((thr), (tv))
#define DUK_HEAPHDR_INCREF(thr, h)      DUK_HEAPHDR_INCREF_FAST((thr), (h))
#define DUK_HEAPHDR_DECREF(thr, h)      DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_heaphdr_refzero, duk_heaphdr *)
#define DUK_HEAPHDR_DECREF_NORZ(thr, h) DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_heaphdr_refzero_norz, duk_heaphdr *)
#define DUK_HSTRING_INCREF(thr, h)      DUK_HEAPHDR_INCREF((thr), (duk_heaphdr *) (h))
#define DUK_HSTRING_DECREF(thr, h)      DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_hstring_refzero, duk_hstring *)
#define DUK_HSTRING_DECREF_NORZ(thr, h) \
	DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_hstring_refzero, duk_hstring *) /* no 'norz' variant */
#define DUK_HOBJECT_INCREF(thr, h)      DUK_HEAPHDR_INCREF((thr), (duk_heaphdr *) (h))
#define DUK_HOBJECT_DECREF(thr, h)      DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_hobject_refzero, duk_hobject *)
#define DUK_HOBJECT_DECREF_NORZ(thr, h) DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_hobject_refzero_norz, duk_hobject *)
#define DUK_HBUFFER_INCREF(thr, h)      DUK_HEAPHDR_INCREF((thr), (duk_heaphdr *) (h))
#define DUK_HBUFFER_DECREF(thr, h)      DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_hbuffer_refzero, duk_hbuffer *)
#define DUK_HBUFFER_DECREF_NORZ(thr, h) \
	DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_hbuffer_refzero, duk_hbuffer *) /* no 'norz' variant */
#define DUK_HCOMPFUNC_INCREF(thr, h)      DUK_HEAPHDR_INCREF((thr), (duk_heaphdr *) &(h)->obj)
#define DUK_HCOMPFUNC_DECREF(thr, h)      DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_hobject_refzero, duk_hobject *)
#define DUK_HCOMPFUNC_DECREF_NORZ(thr, h) DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_hobject_refzero_norz, duk_hobject *)
#define DUK_HNATFUNC_INCREF(thr, h)       DUK_HEAPHDR_INCREF((thr), (duk_heaphdr *) &(h)->obj)
#define DUK_HNATFUNC_DECREF(thr, h)       DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_hobject_refzero, duk_hobject *)
#define DUK_HNATFUNC_DECREF_NORZ(thr, h)  DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_hobject_refzero_norz, duk_hobject *)
#define DUK_HBUFOBJ_INCREF(thr, h)        DUK_HEAPHDR_INCREF((thr), (duk_heaphdr *) &(h)->obj)
#define DUK_HBUFOBJ_DECREF(thr, h)        DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_hobject_refzero, duk_hobject *)
#define DUK_HBUFOBJ_DECREF_NORZ(thr, h)   DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_hobject_refzero_norz, duk_hobject *)
#define DUK_HTHREAD_INCREF(thr, h)        DUK_HEAPHDR_INCREF((thr), (duk_heaphdr *) &(h)->obj)
#define DUK_HTHREAD_DECREF(thr, h)        DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_hobject_refzero, duk_hobject *)
#define DUK_HTHREAD_DECREF_NORZ(thr, h)   DUK_HEAPHDR_DECREF_FAST_RAW((thr), (h), duk_hobject_refzero_norz, duk_hobject *)
#else
#define DUK_TVAL_INCREF(thr, tv)          DUK_TVAL_INCREF_SLOW((thr), (tv))
#define DUK_TVAL_DECREF(thr, tv)          DUK_TVAL_DECREF_SLOW((thr), (tv))
#define DUK_TVAL_DECREF_NORZ(thr, tv)     DUK_TVAL_DECREF_NORZ_SLOW((thr), (tv))
#define DUK_HEAPHDR_INCREF(thr, h)        DUK_HEAPHDR_INCREF_SLOW((thr), (h))
#define DUK_HEAPHDR_DECREF(thr, h)        DUK_HEAPHDR_DECREF_SLOW((thr), (h))
#define DUK_HEAPHDR_DECREF_NORZ(thr, h)   DUK_HEAPHDR_DECREF_NORZ_SLOW((thr), (h))
#define DUK_HSTRING_INCREF(thr, h)        DUK_HEAPHDR_INCREF((thr), (duk_heaphdr *) (h))
#define DUK_HSTRING_DECREF(thr, h)        DUK_HSTRING_DECREF_SLOW((thr), (h))
#define DUK_HSTRING_DECREF_NORZ(thr, h)   DUK_HSTRING_DECREF_NORZ_SLOW((thr), (h))
#define DUK_HOBJECT_INCREF(thr, h)        DUK_HEAPHDR_INCREF((thr), (duk_heaphdr *) (h))
#define DUK_HOBJECT_DECREF(thr, h)        DUK_HOBJECT_DECREF_SLOW((thr), (h))
#define DUK_HOBJECT_DECREF_NORZ(thr, h)   DUK_HOBJECT_DECREF_NORZ_SLOW((thr), (h))
#define DUK_HBUFFER_INCREF(thr, h)        DUK_HEAPHDR_INCREF((thr), (duk_heaphdr *) (h))
#define DUK_HBUFFER_DECREF(thr, h)        DUK_HBUFFER_DECREF_SLOW((thr), (h))
#define DUK_HBUFFER_DECREF_NORZ(thr, h)   DUK_HBUFFER_DECREF_NORZ_SLOW((thr), (h))
#define DUK_HCOMPFUNC_INCREF(thr, h)      DUK_HEAPHDR_INCREF((thr), (duk_heaphdr *) &(h)->obj)
#define DUK_HCOMPFUNC_DECREF(thr, h)      DUK_HOBJECT_DECREF_SLOW((thr), (duk_hobject *) &(h)->obj)
#define DUK_HCOMPFUNC_DECREF_NORZ(thr, h) DUK_HOBJECT_DECREF_NORZ_SLOW((thr), (duk_hobject *) &(h)->obj)
#define DUK_HNATFUNC_INCREF(thr, h)       DUK_HEAPHDR_INCREF((thr), (duk_heaphdr *) &(h)->obj)
#define DUK_HNATFUNC_DECREF(thr, h)       DUK_HOBJECT_DECREF_SLOW((thr), (duk_hobject *) &(h)->obj)
#define DUK_HNATFUNC_DECREF_NORZ(thr, h)  DUK_HOBJECT_DECREF_NORZ_SLOW((thr), (duk_hobject *) &(h)->obj)
#define DUK_HBUFOBJ_INCREF(thr, h)        DUK_HEAPHDR_INCREF((thr), (duk_heaphdr *) &(h)->obj)
#define DUK_HBUFOBJ_DECREF(thr, h)        DUK_HOBJECT_DECREF_SLOW((thr), (duk_hobject *) &(h)->obj)
#define DUK_HBUFOB_DECREF_NORZ(thr, h)    DUK_HOBJECT_DECREF_NORZ_SLOW((thr), (duk_hobject *) &(h)->obj)
#define DUK_HTHREAD_INCREF(thr, h)        DUK_HEAPHDR_INCREF((thr), (duk_heaphdr *) &(h)->obj)
#define DUK_HTHREAD_DECREF(thr, h)        DUK_HOBJECT_DECREF_SLOW((thr), (duk_hobject *) &(h)->obj)
#define DUK_HTHREAD_DECREF_NORZ(thr, h)   DUK_HOBJECT_DECREF_NORZ_SLOW((thr), (duk_hobject *) &(h)->obj)
#endif

/* Convenience for some situations; the above macros don't allow NULLs
 * for performance reasons.  Macros cover only actually needed cases.
 */
#define DUK_HEAPHDR_INCREF_ALLOWNULL(thr, h) \
	do { \
		if ((h) != NULL) { \
			DUK_HEAPHDR_INCREF((thr), (duk_heaphdr *) (h)); \
		} \
	} while (0)
#define DUK_HEAPHDR_DECREF_ALLOWNULL(thr, h) \
	do { \
		if ((h) != NULL) { \
			DUK_HEAPHDR_DECREF((thr), (duk_heaphdr *) (h)); \
		} \
	} while (0)
#define DUK_HEAPHDR_DECREF_NORZ_ALLOWNULL(thr, h) \
	do { \
		if ((h) != NULL) { \
			DUK_HEAPHDR_DECREF_NORZ((thr), (duk_heaphdr *) (h)); \
		} \
	} while (0)
#define DUK_HOBJECT_INCREF_ALLOWNULL(thr, h) \
	do { \
		if ((h) != NULL) { \
			DUK_HOBJECT_INCREF((thr), (h)); \
		} \
	} while (0)
#define DUK_HOBJECT_DECREF_ALLOWNULL(thr, h) \
	do { \
		if ((h) != NULL) { \
			DUK_HOBJECT_DECREF((thr), (h)); \
		} \
	} while (0)
#define DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, h) \
	do { \
		if ((h) != NULL) { \
			DUK_HOBJECT_DECREF_NORZ((thr), (h)); \
		} \
	} while (0)
#define DUK_HBUFFER_INCREF_ALLOWNULL(thr, h) \
	do { \
		if ((h) != NULL) { \
			DUK_HBUFFER_INCREF((thr), (h)); \
		} \
	} while (0)
#define DUK_HBUFFER_DECREF_ALLOWNULL(thr, h) \
	do { \
		if ((h) != NULL) { \
			DUK_HBUFFER_DECREF((thr), (h)); \
		} \
	} while (0)
#define DUK_HBUFFER_DECREF_NORZ_ALLOWNULL(thr, h) \
	do { \
		if ((h) != NULL) { \
			DUK_HBUFFER_DECREF_NORZ((thr), (h)); \
		} \
	} while (0)
#define DUK_HTHREAD_INCREF_ALLOWNULL(thr, h) \
	do { \
		if ((h) != NULL) { \
			DUK_HTHREAD_INCREF((thr), (h)); \
		} \
	} while (0)
#define DUK_HTHREAD_DECREF_ALLOWNULL(thr, h) \
	do { \
		if ((h) != NULL) { \
			DUK_HTHREAD_DECREF((thr), (h)); \
		} \
	} while (0)
#define DUK_HTHREAD_DECREF_NORZ_ALLOWNULL(thr, h) \
	do { \
		if ((h) != NULL) { \
			DUK_HTHREAD_DECREF_NORZ((thr), (h)); \
		} \
	} while (0)

/* Called after one or more DECREF NORZ calls to handle pending side effects.
 * At present DECREF NORZ does freeing inline but doesn't execute finalizers,
 * so these macros check for pending finalizers and execute them.  The FAST
 * variant is performance critical.
 */
#if defined(DUK_USE_FINALIZER_SUPPORT)
#define DUK_REFZERO_CHECK_FAST(thr) \
	do { \
		duk_refzero_check_fast((thr)); \
	} while (0)
#define DUK_REFZERO_CHECK_SLOW(thr) \
	do { \
		duk_refzero_check_slow((thr)); \
	} while (0)
#else /* DUK_USE_FINALIZER_SUPPORT */
#define DUK_REFZERO_CHECK_FAST(thr) \
	do { \
	} while (0)
#define DUK_REFZERO_CHECK_SLOW(thr) \
	do { \
	} while (0)
#endif /* DUK_USE_FINALIZER_SUPPORT */

/*
 *  Macros to set a duk_tval and update refcount of the target (decref the
 *  old value and incref the new value if necessary).  This is both performance
 *  and footprint critical; any changes made should be measured for size/speed.
 */

#define DUK_TVAL_SET_UNDEFINED_UPDREF_ALT0(thr, tvptr_dst) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_UNDEFINED(tv__dst); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)

#define DUK_TVAL_SET_UNDEFINED_UPDREF_NORZ_ALT0(thr, tvptr_dst) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_UNDEFINED(tv__dst); \
		DUK_TVAL_DECREF_NORZ((thr), &tv__tmp); \
	} while (0)

#define DUK_TVAL_SET_UNUSED_UPDREF_ALT0(thr, tvptr_dst) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_UNUSED(tv__dst); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)

#define DUK_TVAL_SET_NULL_UPDREF_ALT0(thr, tvptr_dst) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_NULL(tv__dst); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)

#define DUK_TVAL_SET_BOOLEAN_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_BOOLEAN(tv__dst, (newval)); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)

#define DUK_TVAL_SET_NUMBER_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_NUMBER(tv__dst, (newval)); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)
#define DUK_TVAL_SET_NUMBER_CHKFAST_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_NUMBER_CHKFAST_FAST(tv__dst, (newval)); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)
#define DUK_TVAL_SET_DOUBLE_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_DOUBLE(tv__dst, (newval)); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)
#define DUK_TVAL_SET_NAN_UPDREF_ALT0(thr, tvptr_dst) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_NAN(tv__dst); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)
#if defined(DUK_USE_FASTINT)
#define DUK_TVAL_SET_I48_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_I48(tv__dst, (newval)); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)
#define DUK_TVAL_SET_I32_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_I32(tv__dst, (newval)); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)
#define DUK_TVAL_SET_U32_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_U32(tv__dst, (newval)); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)
#else
#define DUK_TVAL_SET_DOUBLE_CAST_UPDREF(thr, tvptr_dst, newval) \
	DUK_TVAL_SET_DOUBLE_UPDREF((thr), (tvptr_dst), (duk_double_t) (newval))
#endif /* DUK_USE_FASTINT */

#define DUK_TVAL_SET_LIGHTFUNC_UPDREF_ALT0(thr, tvptr_dst, lf_v, lf_fp, lf_flags) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_LIGHTFUNC(tv__dst, (lf_v), (lf_fp), (lf_flags)); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)

#define DUK_TVAL_SET_STRING_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_STRING(tv__dst, (newval)); \
		DUK_HSTRING_INCREF((thr), (newval)); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)

#define DUK_TVAL_SET_OBJECT_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_OBJECT(tv__dst, (newval)); \
		DUK_HOBJECT_INCREF((thr), (newval)); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)

#define DUK_TVAL_SET_BUFFER_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_BUFFER(tv__dst, (newval)); \
		DUK_HBUFFER_INCREF((thr), (newval)); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)

#define DUK_TVAL_SET_POINTER_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_POINTER(tv__dst, (newval)); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)

/* DUK_TVAL_SET_TVAL_UPDREF() is used a lot in executor, property lookups,
 * etc, so it's very important for performance.  Measure when changing.
 *
 * NOTE: the source and destination duk_tval pointers may be the same, and
 * the macros MUST deal with that correctly.
 */

/* Original idiom used, minimal code size. */
#define DUK_TVAL_SET_TVAL_UPDREF_ALT0(thr, tvptr_dst, tvptr_src) \
	do { \
		duk_tval *tv__dst, *tv__src; \
		duk_tval tv__tmp; \
		tv__dst = (tvptr_dst); \
		tv__src = (tvptr_src); \
		DUK_TVAL_SET_TVAL(&tv__tmp, tv__dst); \
		DUK_TVAL_SET_TVAL(tv__dst, tv__src); \
		DUK_TVAL_INCREF((thr), tv__src); \
		DUK_TVAL_DECREF((thr), &tv__tmp); /* side effects */ \
	} while (0)

/* Faster alternative: avoid making a temporary copy of tvptr_dst and use
 * fast incref/decref macros.
 */
#define DUK_TVAL_SET_TVAL_UPDREF_ALT1(thr, tvptr_dst, tvptr_src) \
	do { \
		duk_tval *tv__dst, *tv__src; \
		duk_heaphdr *h__obj; \
		tv__dst = (tvptr_dst); \
		tv__src = (tvptr_src); \
		DUK_TVAL_INCREF_FAST((thr), tv__src); \
		if (DUK_TVAL_NEEDS_REFCOUNT_UPDATE(tv__dst)) { \
			h__obj = DUK_TVAL_GET_HEAPHDR(tv__dst); \
			DUK_ASSERT(h__obj != NULL); \
			DUK_TVAL_SET_TVAL(tv__dst, tv__src); \
			DUK_HEAPHDR_DECREF_FAST((thr), h__obj); /* side effects */ \
		} else { \
			DUK_TVAL_SET_TVAL(tv__dst, tv__src); \
		} \
	} while (0)

/* XXX: no optimized variants yet */
#define DUK_TVAL_SET_UNDEFINED_UPDREF      DUK_TVAL_SET_UNDEFINED_UPDREF_ALT0
#define DUK_TVAL_SET_UNDEFINED_UPDREF_NORZ DUK_TVAL_SET_UNDEFINED_UPDREF_NORZ_ALT0
#define DUK_TVAL_SET_UNUSED_UPDREF         DUK_TVAL_SET_UNUSED_UPDREF_ALT0
#define DUK_TVAL_SET_NULL_UPDREF           DUK_TVAL_SET_NULL_UPDREF_ALT0
#define DUK_TVAL_SET_BOOLEAN_UPDREF        DUK_TVAL_SET_BOOLEAN_UPDREF_ALT0
#define DUK_TVAL_SET_NUMBER_UPDREF         DUK_TVAL_SET_NUMBER_UPDREF_ALT0
#define DUK_TVAL_SET_NUMBER_CHKFAST_UPDREF DUK_TVAL_SET_NUMBER_CHKFAST_UPDREF_ALT0
#define DUK_TVAL_SET_DOUBLE_UPDREF         DUK_TVAL_SET_DOUBLE_UPDREF_ALT0
#define DUK_TVAL_SET_NAN_UPDREF            DUK_TVAL_SET_NAN_UPDREF_ALT0
#if defined(DUK_USE_FASTINT)
#define DUK_TVAL_SET_I48_UPDREF DUK_TVAL_SET_I48_UPDREF_ALT0
#define DUK_TVAL_SET_I32_UPDREF DUK_TVAL_SET_I32_UPDREF_ALT0
#define DUK_TVAL_SET_U32_UPDREF DUK_TVAL_SET_U32_UPDREF_ALT0
#else
#define DUK_TVAL_SET_I48_UPDREF DUK_TVAL_SET_DOUBLE_CAST_UPDREF /* XXX: fast int-to-double */
#define DUK_TVAL_SET_I32_UPDREF DUK_TVAL_SET_DOUBLE_CAST_UPDREF
#define DUK_TVAL_SET_U32_UPDREF DUK_TVAL_SET_DOUBLE_CAST_UPDREF
#endif /* DUK_USE_FASTINT */
#define DUK_TVAL_SET_FASTINT_UPDREF   DUK_TVAL_SET_I48_UPDREF /* convenience */
#define DUK_TVAL_SET_LIGHTFUNC_UPDREF DUK_TVAL_SET_LIGHTFUNC_UPDREF_ALT0
#define DUK_TVAL_SET_STRING_UPDREF    DUK_TVAL_SET_STRING_UPDREF_ALT0
#define DUK_TVAL_SET_OBJECT_UPDREF    DUK_TVAL_SET_OBJECT_UPDREF_ALT0
#define DUK_TVAL_SET_BUFFER_UPDREF    DUK_TVAL_SET_BUFFER_UPDREF_ALT0
#define DUK_TVAL_SET_POINTER_UPDREF   DUK_TVAL_SET_POINTER_UPDREF_ALT0

#if defined(DUK_USE_FAST_REFCOUNT_DEFAULT)
/* Optimized for speed. */
#define DUK_TVAL_SET_TVAL_UPDREF      DUK_TVAL_SET_TVAL_UPDREF_ALT1
#define DUK_TVAL_SET_TVAL_UPDREF_FAST DUK_TVAL_SET_TVAL_UPDREF_ALT1
#define DUK_TVAL_SET_TVAL_UPDREF_SLOW DUK_TVAL_SET_TVAL_UPDREF_ALT0
#else
/* Optimized for size. */
#define DUK_TVAL_SET_TVAL_UPDREF      DUK_TVAL_SET_TVAL_UPDREF_ALT0
#define DUK_TVAL_SET_TVAL_UPDREF_FAST DUK_TVAL_SET_TVAL_UPDREF_ALT0
#define DUK_TVAL_SET_TVAL_UPDREF_SLOW DUK_TVAL_SET_TVAL_UPDREF_ALT0
#endif

#else /* DUK_USE_REFERENCE_COUNTING */

#define DUK_TVAL_NEEDS_REFCOUNT_UPDATE(tv)   0
#define DUK_HEAPHDR_NEEDS_REFCOUNT_UPDATE(h) 0

#define DUK_TVAL_INCREF_FAST(thr, v) \
	do { \
	} while (0) /* nop */
#define DUK_TVAL_DECREF_FAST(thr, v) \
	do { \
	} while (0) /* nop */
#define DUK_TVAL_DECREF_NORZ_FAST(thr, v) \
	do { \
	} while (0) /* nop */
#define DUK_TVAL_INCREF_SLOW(thr, v) \
	do { \
	} while (0) /* nop */
#define DUK_TVAL_DECREF_SLOW(thr, v) \
	do { \
	} while (0) /* nop */
#define DUK_TVAL_DECREF_NORZ_SLOW(thr, v) \
	do { \
	} while (0) /* nop */
#define DUK_TVAL_INCREF(thr, v) \
	do { \
	} while (0) /* nop */
#define DUK_TVAL_DECREF(thr, v) \
	do { \
	} while (0) /* nop */
#define DUK_TVAL_DECREF_NORZ(thr, v) \
	do { \
	} while (0) /* nop */
#define DUK_HEAPHDR_INCREF_FAST(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HEAPHDR_DECREF_FAST(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HEAPHDR_DECREF_NORZ_FAST(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HEAPHDR_INCREF_SLOW(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HEAPHDR_DECREF_SLOW(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HEAPHDR_DECREF_NORZ_SLOW(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HEAPHDR_INCREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HEAPHDR_DECREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HEAPHDR_DECREF_NORZ(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HSTRING_INCREF_FAST(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HSTRING_DECREF_FAST(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HSTRING_DECREF_NORZ_FAST(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HSTRING_INCREF_SLOW(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HSTRING_DECREF_SLOW(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HSTRING_DECREF_NORZ_SLOW(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HSTRING_INCREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HSTRING_DECREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HSTRING_DECREF_NORZ(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HOBJECT_INCREF_FAST(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HOBJECT_DECREF_FAST(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HOBJECT_DECREF_NORZ_FAST(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HOBJECT_INCREF_SLOW(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HOBJECT_DECREF_SLOW(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HOBJECT_DECREF_NORZ_SLOW(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HOBJECT_INCREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HOBJECT_DECREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HOBJECT_DECREF_NORZ(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HBUFFER_INCREF_FAST(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HBUFFER_DECREF_FAST(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HBUFFER_DECREF_NORZ_FAST(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HBUFFER_INCREF_SLOW(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HBUFFER_DECREF_SLOW(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HBUFFER_DECREF_NORZ_SLOW(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HBUFFER_INCREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HBUFFER_DECREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HBUFFER_DECREF_NORZ(thr, h) \
	do { \
	} while (0) /* nop */

#define DUK_HCOMPFUNC_INCREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HCOMPFUNC_DECREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HCOMPFUNC_DECREF_NORZ(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HNATFUNC_INCREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HNATFUNC_DECREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HNATFUNC_DECREF_NORZ(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HBUFOBJ_INCREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HBUFOBJ_DECREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HBUFOBJ_DECREF_NORZ(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HTHREAD_INCREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HTHREAD_DECREF(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HTHREAD_DECREF_NORZ(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HOBJECT_INCREF_ALLOWNULL(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HOBJECT_DECREF_ALLOWNULL(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HBUFFER_INCREF_ALLOWNULL(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HBUFFER_DECREF_ALLOWNULL(thr, h) \
	do { \
	} while (0) /* nop */
#define DUK_HBUFFER_DECREF_NORZ_ALLOWNULL(thr, h) \
	do { \
	} while (0) /* nop */

#define DUK_REFZERO_CHECK_FAST(thr) \
	do { \
	} while (0) /* nop */
#define DUK_REFZERO_CHECK_SLOW(thr) \
	do { \
	} while (0) /* nop */

#define DUK_TVAL_SET_UNDEFINED_UPDREF_ALT0(thr, tvptr_dst) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_UNDEFINED(tv__dst); \
		DUK_UNREF((thr)); \
	} while (0)

#define DUK_TVAL_SET_UNUSED_UPDREF_ALT0(thr, tvptr_dst) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_UNUSED(tv__dst); \
		DUK_UNREF((thr)); \
	} while (0)

#define DUK_TVAL_SET_NULL_UPDREF_ALT0(thr, tvptr_dst) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_NULL(tv__dst); \
		DUK_UNREF((thr)); \
	} while (0)

#define DUK_TVAL_SET_BOOLEAN_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_BOOLEAN(tv__dst, (newval)); \
		DUK_UNREF((thr)); \
	} while (0)

#define DUK_TVAL_SET_NUMBER_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_NUMBER(tv__dst, (newval)); \
		DUK_UNREF((thr)); \
	} while (0)
#define DUK_TVAL_SET_NUMBER_CHKFAST_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_NUMBER_CHKFAST_FAST(tv__dst, (newval)); \
		DUK_UNREF((thr)); \
	} while (0)
#define DUK_TVAL_SET_DOUBLE_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_DOUBLE(tv__dst, (newval)); \
		DUK_UNREF((thr)); \
	} while (0)
#define DUK_TVAL_SET_NAN_UPDREF_ALT0(thr, tvptr_dst) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_NAN(tv__dst); \
		DUK_UNREF((thr)); \
	} while (0)
#if defined(DUK_USE_FASTINT)
#define DUK_TVAL_SET_I48_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_I48(tv__dst, (newval)); \
		DUK_UNREF((thr)); \
	} while (0)
#define DUK_TVAL_SET_I32_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_I32(tv__dst, (newval)); \
		DUK_UNREF((thr)); \
	} while (0)
#define DUK_TVAL_SET_U32_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_U32(tv__dst, (newval)); \
		DUK_UNREF((thr)); \
	} while (0)
#else
#define DUK_TVAL_SET_DOUBLE_CAST_UPDREF(thr, tvptr_dst, newval) \
	DUK_TVAL_SET_DOUBLE_UPDREF((thr), (tvptr_dst), (duk_double_t) (newval))
#endif /* DUK_USE_FASTINT */

#define DUK_TVAL_SET_LIGHTFUNC_UPDREF_ALT0(thr, tvptr_dst, lf_v, lf_fp, lf_flags) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_LIGHTFUNC(tv__dst, (lf_v), (lf_fp), (lf_flags)); \
		DUK_UNREF((thr)); \
	} while (0)

#define DUK_TVAL_SET_STRING_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_STRING(tv__dst, (newval)); \
		DUK_UNREF((thr)); \
	} while (0)

#define DUK_TVAL_SET_OBJECT_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_OBJECT(tv__dst, (newval)); \
		DUK_UNREF((thr)); \
	} while (0)

#define DUK_TVAL_SET_BUFFER_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_BUFFER(tv__dst, (newval)); \
		DUK_UNREF((thr)); \
	} while (0)

#define DUK_TVAL_SET_POINTER_UPDREF_ALT0(thr, tvptr_dst, newval) \
	do { \
		duk_tval *tv__dst; \
		tv__dst = (tvptr_dst); \
		DUK_TVAL_SET_POINTER(tv__dst, (newval)); \
		DUK_UNREF((thr)); \
	} while (0)

#define DUK_TVAL_SET_TVAL_UPDREF_ALT0(thr, tvptr_dst, tvptr_src) \
	do { \
		duk_tval *tv__dst, *tv__src; \
		tv__dst = (tvptr_dst); \
		tv__src = (tvptr_src); \
		DUK_TVAL_SET_TVAL(tv__dst, tv__src); \
		DUK_UNREF((thr)); \
	} while (0)

#define DUK_TVAL_SET_UNDEFINED_UPDREF      DUK_TVAL_SET_UNDEFINED_UPDREF_ALT0
#define DUK_TVAL_SET_UNDEFINED_UPDREF_NORZ DUK_TVAL_SET_UNDEFINED_UPDREF_ALT0
#define DUK_TVAL_SET_UNUSED_UPDREF         DUK_TVAL_SET_UNUSED_UPDREF_ALT0
#define DUK_TVAL_SET_NULL_UPDREF           DUK_TVAL_SET_NULL_UPDREF_ALT0
#define DUK_TVAL_SET_BOOLEAN_UPDREF        DUK_TVAL_SET_BOOLEAN_UPDREF_ALT0
#define DUK_TVAL_SET_NUMBER_UPDREF         DUK_TVAL_SET_NUMBER_UPDREF_ALT0
#define DUK_TVAL_SET_NUMBER_CHKFAST_UPDREF DUK_TVAL_SET_NUMBER_CHKFAST_UPDREF_ALT0
#define DUK_TVAL_SET_DOUBLE_UPDREF         DUK_TVAL_SET_DOUBLE_UPDREF_ALT0
#define DUK_TVAL_SET_NAN_UPDREF            DUK_TVAL_SET_NAN_UPDREF_ALT0
#if defined(DUK_USE_FASTINT)
#define DUK_TVAL_SET_I48_UPDREF DUK_TVAL_SET_I48_UPDREF_ALT0
#define DUK_TVAL_SET_I32_UPDREF DUK_TVAL_SET_I32_UPDREF_ALT0
#define DUK_TVAL_SET_U32_UPDREF DUK_TVAL_SET_U32_UPDREF_ALT0
#else
#define DUK_TVAL_SET_I48_UPDREF DUK_TVAL_SET_DOUBLE_CAST_UPDREF /* XXX: fast-int-to-double */
#define DUK_TVAL_SET_I32_UPDREF DUK_TVAL_SET_DOUBLE_CAST_UPDREF
#define DUK_TVAL_SET_U32_UPDREF DUK_TVAL_SET_DOUBLE_CAST_UPDREF
#endif /* DUK_USE_FASTINT */
#define DUK_TVAL_SET_FASTINT_UPDREF   DUK_TVAL_SET_I48_UPDREF /* convenience */
#define DUK_TVAL_SET_LIGHTFUNC_UPDREF DUK_TVAL_SET_LIGHTFUNC_UPDREF_ALT0
#define DUK_TVAL_SET_STRING_UPDREF    DUK_TVAL_SET_STRING_UPDREF_ALT0
#define DUK_TVAL_SET_OBJECT_UPDREF    DUK_TVAL_SET_OBJECT_UPDREF_ALT0
#define DUK_TVAL_SET_BUFFER_UPDREF    DUK_TVAL_SET_BUFFER_UPDREF_ALT0
#define DUK_TVAL_SET_POINTER_UPDREF   DUK_TVAL_SET_POINTER_UPDREF_ALT0

#define DUK_TVAL_SET_TVAL_UPDREF      DUK_TVAL_SET_TVAL_UPDREF_ALT0
#define DUK_TVAL_SET_TVAL_UPDREF_FAST DUK_TVAL_SET_TVAL_UPDREF_ALT0
#define DUK_TVAL_SET_TVAL_UPDREF_SLOW DUK_TVAL_SET_TVAL_UPDREF_ALT0

#endif /* DUK_USE_REFERENCE_COUNTING */

/*
 *  Some convenience macros that don't have optimized implementations now.
 */

#define DUK_TVAL_SET_TVAL_UPDREF_NORZ(thr, tv_dst, tv_src) \
	do { \
		duk_hthread *duk__thr = (thr); \
		duk_tval *duk__dst = (tv_dst); \
		duk_tval *duk__src = (tv_src); \
		DUK_UNREF(duk__thr); \
		DUK_TVAL_DECREF_NORZ(thr, duk__dst); \
		DUK_TVAL_SET_TVAL(duk__dst, duk__src); \
		DUK_TVAL_INCREF(thr, duk__dst); \
	} while (0)

#define DUK_TVAL_SET_U32_UPDREF_NORZ(thr, tv_dst, val) \
	do { \
		duk_hthread *duk__thr = (thr); \
		duk_tval *duk__dst = (tv_dst); \
		duk_uint32_t duk__val = (duk_uint32_t) (val); \
		DUK_UNREF(duk__thr); \
		DUK_TVAL_DECREF_NORZ(thr, duk__dst); \
		DUK_TVAL_SET_U32(duk__dst, duk__val); \
	} while (0)

/*
 *  Prototypes
 */

#if defined(DUK_USE_REFERENCE_COUNTING)
#if defined(DUK_USE_FINALIZER_SUPPORT)
DUK_INTERNAL_DECL void duk_refzero_check_slow(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_refzero_check_fast(duk_hthread *thr);
#endif
DUK_INTERNAL_DECL void duk_heaphdr_refcount_finalize_norz(duk_heap *heap, duk_heaphdr *hdr);
DUK_INTERNAL_DECL void duk_hobject_refcount_finalize_norz(duk_heap *heap, duk_hobject *h);
#if 0 /* Not needed: fast path handles inline; slow path uses duk_heaphdr_decref() which is needed anyway. */
DUK_INTERNAL_DECL void duk_hstring_decref(duk_hthread *thr, duk_hstring *h);
DUK_INTERNAL_DECL void duk_hstring_decref_norz(duk_hthread *thr, duk_hstring *h);
DUK_INTERNAL_DECL void duk_hbuffer_decref(duk_hthread *thr, duk_hbuffer *h);
DUK_INTERNAL_DECL void duk_hbuffer_decref_norz(duk_hthread *thr, duk_hbuffer *h);
DUK_INTERNAL_DECL void duk_hobject_decref(duk_hthread *thr, duk_hobject *h);
DUK_INTERNAL_DECL void duk_hobject_decref_norz(duk_hthread *thr, duk_hobject *h);
#endif
DUK_INTERNAL_DECL void duk_heaphdr_refzero(duk_hthread *thr, duk_heaphdr *h);
DUK_INTERNAL_DECL void duk_heaphdr_refzero_norz(duk_hthread *thr, duk_heaphdr *h);
#if defined(DUK_USE_FAST_REFCOUNT_DEFAULT)
DUK_INTERNAL_DECL void duk_hstring_refzero(duk_hthread *thr, duk_hstring *h); /* no 'norz' variant */
DUK_INTERNAL_DECL void duk_hbuffer_refzero(duk_hthread *thr, duk_hbuffer *h); /* no 'norz' variant */
DUK_INTERNAL_DECL void duk_hobject_refzero(duk_hthread *thr, duk_hobject *h);
DUK_INTERNAL_DECL void duk_hobject_refzero_norz(duk_hthread *thr, duk_hobject *h);
#else
DUK_INTERNAL_DECL void duk_tval_incref(duk_tval *tv);
DUK_INTERNAL_DECL void duk_tval_decref(duk_hthread *thr, duk_tval *tv);
DUK_INTERNAL_DECL void duk_tval_decref_norz(duk_hthread *thr, duk_tval *tv);
DUK_INTERNAL_DECL void duk_heaphdr_incref(duk_heaphdr *h);
DUK_INTERNAL_DECL void duk_heaphdr_decref(duk_hthread *thr, duk_heaphdr *h);
DUK_INTERNAL_DECL void duk_heaphdr_decref_norz(duk_hthread *thr, duk_heaphdr *h);
#endif
#else /* DUK_USE_REFERENCE_COUNTING */
/* no refcounting */
#endif /* DUK_USE_REFERENCE_COUNTING */

#endif /* DUK_REFCOUNT_H_INCLUDED */
