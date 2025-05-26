/*
 *  duk_hobject property access functionality.
 *
 *  This is very central functionality for size, performance, and compliance.
 *  It is also rather intricate; see hobject-algorithms.rst for discussion on
 *  the algorithms and memory-management.rst for discussion on refcounts and
 *  side effect issues.
 *
 *  Notes:
 *
 *    - It might be tempting to assert "refcount nonzero" for objects
 *      being operated on, but that's not always correct: objects with
 *      a zero refcount may be operated on by the refcount implementation
 *      (finalization) for instance.  Hence, no refcount assertions are made.
 *
 *    - Many operations (memory allocation, identifier operations, etc)
 *      may cause arbitrary side effects (e.g. through GC and finalization).
 *      These side effects may invalidate duk_tval pointers which point to
 *      areas subject to reallocation (like value stack).  Heap objects
 *      themselves have stable pointers.  Holding heap object pointers or
 *      duk_tval copies is not problematic with respect to side effects;
 *      care must be taken when holding and using argument duk_tval pointers.
 *
 *    - If a finalizer is executed, it may operate on the the same object
 *      we're currently dealing with.  For instance, the finalizer might
 *      delete a certain property which has already been looked up and
 *      confirmed to exist.  Ideally finalizers would be disabled if GC
 *      happens during property access.  At the moment property table realloc
 *      disables finalizers, and all DECREFs may cause arbitrary changes so
 *      handle DECREF carefully.
 *
 *    - The order of operations for a DECREF matters.  When DECREF is executed,
 *      the entire object graph must be consistent; note that a refzero may
 *      lead to a mark-and-sweep through a refcount finalizer.  Use NORZ macros
 *      and an explicit DUK_REFZERO_CHECK_xxx() if achieving correct order is hard.
 */

/*
 *  XXX: array indices are mostly typed as duk_uint32_t here; duk_uarridx_t
 *  might be more appropriate.
 */

#include "duk_internal.h"

/*
 *  Local defines
 */

#define DUK__NO_ARRAY_INDEX DUK_HSTRING_NO_ARRAY_INDEX

/* Marker values for hash part. */
#define DUK__HASH_UNUSED  DUK_HOBJECT_HASHIDX_UNUSED
#define DUK__HASH_DELETED DUK_HOBJECT_HASHIDX_DELETED

/* Valstack space that suffices for all local calls, excluding any recursion
 * into ECMAScript or Duktape/C calls (Proxy, getters, etc).
 */
#define DUK__VALSTACK_SPACE 10

/* Valstack space allocated especially for proxy lookup which does a
 * recursive property lookup.
 */
#define DUK__VALSTACK_PROXY_LOOKUP 20

/*
 *  Local prototypes
 */

DUK_LOCAL_DECL duk_bool_t duk__check_arguments_map_for_get(duk_hthread *thr,
                                                           duk_hobject *obj,
                                                           duk_hstring *key,
                                                           duk_propdesc *temp_desc);
DUK_LOCAL_DECL void duk__check_arguments_map_for_put(duk_hthread *thr,
                                                     duk_hobject *obj,
                                                     duk_hstring *key,
                                                     duk_propdesc *temp_desc,
                                                     duk_bool_t throw_flag);
DUK_LOCAL_DECL void duk__check_arguments_map_for_delete(duk_hthread *thr,
                                                        duk_hobject *obj,
                                                        duk_hstring *key,
                                                        duk_propdesc *temp_desc);

DUK_LOCAL_DECL duk_bool_t duk__handle_put_array_length_smaller(duk_hthread *thr,
                                                               duk_hobject *obj,
                                                               duk_uint32_t old_len,
                                                               duk_uint32_t new_len,
                                                               duk_bool_t force_flag,
                                                               duk_uint32_t *out_result_len);
DUK_LOCAL_DECL duk_bool_t duk__handle_put_array_length(duk_hthread *thr, duk_hobject *obj);

DUK_LOCAL_DECL duk_bool_t
duk__get_propdesc(duk_hthread *thr, duk_hobject *obj, duk_hstring *key, duk_propdesc *out_desc, duk_small_uint_t flags);
DUK_LOCAL_DECL duk_bool_t duk__get_own_propdesc_raw(duk_hthread *thr,
                                                    duk_hobject *obj,
                                                    duk_hstring *key,
                                                    duk_uint32_t arr_idx,
                                                    duk_propdesc *out_desc,
                                                    duk_small_uint_t flags);

DUK_LOCAL_DECL void duk__abandon_array_part(duk_hthread *thr, duk_hobject *obj);
DUK_LOCAL_DECL void duk__grow_props_for_array_item(duk_hthread *thr, duk_hobject *obj, duk_uint32_t highest_arr_idx);

/*
 *  Misc helpers
 */

/* Convert a duk_tval number (caller checks) to a 32-bit index.  Returns
 * DUK__NO_ARRAY_INDEX if the number is not whole or not a valid array
 * index.
 */
/* XXX: for fastints, could use a variant which assumes a double duk_tval
 * (and doesn't need to check for fastint again).
 */
DUK_LOCAL duk_uint32_t duk__tval_number_to_arr_idx(duk_tval *tv) {
	duk_double_t dbl;
	duk_uint32_t idx;

	DUK_ASSERT(tv != NULL);
	DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv));

	/* -0 is accepted here as index 0 because ToString(-0) == "0" which is
	 * in canonical form and thus an array index.
	 */
	dbl = DUK_TVAL_GET_NUMBER(tv);
	idx = (duk_uint32_t) dbl;
	if (duk_double_equals((duk_double_t) idx, dbl)) {
		/* Is whole and within 32 bit range.  If the value happens to be 0xFFFFFFFF,
		 * it's not a valid array index but will then match DUK__NO_ARRAY_INDEX.
		 */
		return idx;
	}
	return DUK__NO_ARRAY_INDEX;
}

#if defined(DUK_USE_FASTINT)
/* Convert a duk_tval fastint (caller checks) to a 32-bit index. */
DUK_LOCAL duk_uint32_t duk__tval_fastint_to_arr_idx(duk_tval *tv) {
	duk_int64_t t;

	DUK_ASSERT(tv != NULL);
	DUK_ASSERT(DUK_TVAL_IS_FASTINT(tv));

	t = DUK_TVAL_GET_FASTINT(tv);
	if (((duk_uint64_t) t & ~DUK_U64_CONSTANT(0xffffffff)) != 0) {
		/* Catches >0x100000000 and negative values. */
		return DUK__NO_ARRAY_INDEX;
	}

	/* If the value happens to be 0xFFFFFFFF, it's not a valid array index
	 * but will then match DUK__NO_ARRAY_INDEX.
	 */
	return (duk_uint32_t) t;
}
#endif /* DUK_USE_FASTINT */

/* Convert a duk_tval on the value stack (in a trusted index we don't validate)
 * to a string or symbol using ES2015 ToPropertyKey():
 * http://www.ecma-international.org/ecma-262/6.0/#sec-topropertykey.
 *
 * Also check if it's a valid array index and return that (or DUK__NO_ARRAY_INDEX
 * if not).
 */
DUK_LOCAL duk_uint32_t duk__to_property_key(duk_hthread *thr, duk_idx_t idx, duk_hstring **out_h) {
	duk_uint32_t arr_idx;
	duk_hstring *h;
	duk_tval *tv_dst;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(out_h != NULL);
	DUK_ASSERT(duk_is_valid_index(thr, idx));
	DUK_ASSERT(idx < 0);

	/* XXX: The revised ES2015 ToPropertyKey() handling (ES5.1 was just
	 * ToString()) involves a ToPrimitive(), a symbol check, and finally
	 * a ToString().  Figure out the best way to have a good fast path
	 * but still be compliant and share code.
	 */

	tv_dst = DUK_GET_TVAL_NEGIDX(thr, idx); /* intentionally unvalidated */
	if (DUK_TVAL_IS_STRING(tv_dst)) {
		/* Most important path: strings and plain symbols are used as
		 * is.  For symbols the array index check below is unnecessary
		 * (they're never valid array indices) but checking that the
		 * string is a symbol would make the plain string path slower
		 * unnecessarily.
		 */
		h = DUK_TVAL_GET_STRING(tv_dst);
	} else {
		h = duk_to_property_key_hstring(thr, idx);
	}
	DUK_ASSERT(h != NULL);
	*out_h = h;

	arr_idx = DUK_HSTRING_GET_ARRIDX_FAST(h);
	return arr_idx;
}

DUK_LOCAL duk_uint32_t duk__push_tval_to_property_key(duk_hthread *thr, duk_tval *tv_key, duk_hstring **out_h) {
	duk_push_tval(thr, tv_key); /* XXX: could use an unsafe push here */
	return duk__to_property_key(thr, -1, out_h);
}

/* String is an own (virtual) property of a plain buffer. */
DUK_LOCAL duk_bool_t duk__key_is_plain_buf_ownprop(duk_hthread *thr, duk_hbuffer *buf, duk_hstring *key, duk_uint32_t arr_idx) {
	DUK_UNREF(thr);

	/* Virtual index properties.  Checking explicitly for
	 * 'arr_idx != DUK__NO_ARRAY_INDEX' is not necessary
	 * because DUK__NO_ARRAY_INDEXi is always larger than
	 * maximum allowed buffer size.
	 */
	DUK_ASSERT(DUK__NO_ARRAY_INDEX >= DUK_HBUFFER_GET_SIZE(buf));
	if (arr_idx < DUK_HBUFFER_GET_SIZE(buf)) {
		return 1;
	}

	/* Other virtual properties. */
	return (key == DUK_HTHREAD_STRING_LENGTH(thr));
}

/*
 *  Helpers for managing property storage size
 */

/* Get default hash part size for a certain entry part size. */
#if defined(DUK_USE_HOBJECT_HASH_PART)
DUK_LOCAL duk_uint32_t duk__get_default_h_size(duk_uint32_t e_size) {
	DUK_ASSERT(e_size <= DUK_HOBJECT_MAX_PROPERTIES);

	if (e_size >= DUK_USE_HOBJECT_HASH_PROP_LIMIT) {
		duk_uint32_t res;
		duk_uint32_t tmp;

		/* Hash size should be 2^N where N is chosen so that 2^N is
		 * larger than e_size.  Extra shifting is used to ensure hash
		 * is relatively sparse.
		 */
		tmp = e_size;
		res = 2; /* Result will be 2 ** (N + 1). */
		while (tmp >= 0x40) {
			tmp >>= 6;
			res <<= 6;
		}
		while (tmp != 0) {
			tmp >>= 1;
			res <<= 1;
		}
		DUK_ASSERT((DUK_HOBJECT_MAX_PROPERTIES << 2U) > DUK_HOBJECT_MAX_PROPERTIES); /* Won't wrap, even shifted by 2. */
		DUK_ASSERT(res > e_size);
		return res;
	} else {
		return 0;
	}
}
#endif /* USE_PROP_HASH_PART */

/* Get minimum entry part growth for a certain size. */
DUK_LOCAL duk_uint32_t duk__get_min_grow_e(duk_uint32_t e_size) {
	duk_uint32_t res;

	res = (e_size + DUK_USE_HOBJECT_ENTRY_MINGROW_ADD) / DUK_USE_HOBJECT_ENTRY_MINGROW_DIVISOR;
	DUK_ASSERT(res >= 1); /* important for callers */
	return res;
}

/* Get minimum array part growth for a certain size. */
DUK_LOCAL duk_uint32_t duk__get_min_grow_a(duk_uint32_t a_size) {
	duk_uint32_t res;

	res = (a_size + DUK_USE_HOBJECT_ARRAY_MINGROW_ADD) / DUK_USE_HOBJECT_ARRAY_MINGROW_DIVISOR;
	DUK_ASSERT(res >= 1); /* important for callers */
	return res;
}

/* Count actually used entry part entries (non-NULL keys). */
DUK_LOCAL duk_uint32_t duk__count_used_e_keys(duk_hthread *thr, duk_hobject *obj) {
	duk_uint_fast32_t i;
	duk_uint_fast32_t n = 0;
	duk_hstring **e;

	DUK_ASSERT(obj != NULL);
	DUK_UNREF(thr);

	e = DUK_HOBJECT_E_GET_KEY_BASE(thr->heap, obj);
	for (i = 0; i < DUK_HOBJECT_GET_ENEXT(obj); i++) {
		if (*e++) {
			n++;
		}
	}
	return (duk_uint32_t) n;
}

/* Count actually used array part entries and array minimum size.
 * NOTE: 'out_min_size' can be computed much faster by starting from the
 * end and breaking out early when finding first used entry, but this is
 * not needed now.
 */
DUK_LOCAL void duk__compute_a_stats(duk_hthread *thr, duk_hobject *obj, duk_uint32_t *out_used, duk_uint32_t *out_min_size) {
	duk_uint_fast32_t i;
	duk_uint_fast32_t used = 0;
	duk_uint_fast32_t highest_idx = (duk_uint_fast32_t) -1; /* see below */
	duk_tval *a;

	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(out_used != NULL);
	DUK_ASSERT(out_min_size != NULL);
	DUK_UNREF(thr);

	a = DUK_HOBJECT_A_GET_BASE(thr->heap, obj);
	for (i = 0; i < DUK_HOBJECT_GET_ASIZE(obj); i++) {
		duk_tval *tv = a++;
		if (!DUK_TVAL_IS_UNUSED(tv)) {
			used++;
			highest_idx = i;
		}
	}

	/* Initial value for highest_idx is -1 coerced to unsigned.  This
	 * is a bit odd, but (highest_idx + 1) will then wrap to 0 below
	 * for out_min_size as intended.
	 */

	*out_used = (duk_uint32_t) used;
	*out_min_size = (duk_uint32_t) (highest_idx + 1); /* 0 if no used entries */
}

/* Check array density and indicate whether or not the array part should be abandoned. */
DUK_LOCAL duk_bool_t duk__abandon_array_density_check(duk_uint32_t a_used, duk_uint32_t a_size) {
	/*
	 *  Array abandon check; abandon if:
	 *
	 *    new_used / new_size < limit
	 *    new_used < limit * new_size        || limit is 3 bits fixed point
	 *    new_used < limit' / 8 * new_size   || *8
	 *    8*new_used < limit' * new_size     || :8
	 *    new_used < limit' * (new_size / 8)
	 *
	 *  Here, new_used = a_used, new_size = a_size.
	 *
	 *  Note: some callers use approximate values for a_used and/or a_size
	 *  (e.g. dropping a '+1' term).  This doesn't affect the usefulness
	 *  of the check, but may confuse debugging.
	 */

	return (a_used < DUK_USE_HOBJECT_ARRAY_ABANDON_LIMIT * (a_size >> 3));
}

/* Fast check for extending array: check whether or not a slow density check is required. */
DUK_LOCAL duk_bool_t duk__abandon_array_slow_check_required(duk_uint32_t arr_idx, duk_uint32_t old_size) {
	duk_uint32_t new_size_min;

	/*
	 *  In a fast check we assume old_size equals old_used (i.e., existing
	 *  array is fully dense).
	 *
	 *  Slow check if:
	 *
	 *    (new_size - old_size) / old_size > limit
	 *    new_size - old_size > limit * old_size
	 *    new_size > (1 + limit) * old_size        || limit' is 3 bits fixed point
	 *    new_size > (1 + (limit' / 8)) * old_size || * 8
	 *    8 * new_size > (8 + limit') * old_size   || : 8
	 *    new_size > (8 + limit') * (old_size / 8)
	 *    new_size > limit'' * (old_size / 8)      || limit'' = 9 -> max 25% increase
	 *    arr_idx + 1 > limit'' * (old_size / 8)
	 *
	 *  This check doesn't work well for small values, so old_size is rounded
	 *  up for the check (and the '+ 1' of arr_idx can be ignored in practice):
	 *
	 *    arr_idx > limit'' * ((old_size + 7) / 8)
	 */

	new_size_min = arr_idx + 1;
	return (new_size_min >= DUK_USE_HOBJECT_ARRAY_ABANDON_MINSIZE) &&
	       (arr_idx > DUK_USE_HOBJECT_ARRAY_FAST_RESIZE_LIMIT * ((old_size + 7) >> 3));
}

DUK_LOCAL duk_bool_t duk__abandon_array_check(duk_hthread *thr, duk_uint32_t arr_idx, duk_hobject *obj) {
	duk_uint32_t min_size;
	duk_uint32_t old_used;
	duk_uint32_t old_size;

	if (!duk__abandon_array_slow_check_required(arr_idx, DUK_HOBJECT_GET_ASIZE(obj))) {
		DUK_DDD(DUK_DDDPRINT("=> fast resize is OK"));
		return 0;
	}

	duk__compute_a_stats(thr, obj, &old_used, &old_size);

	DUK_DDD(DUK_DDDPRINT("abandon check, array stats: old_used=%ld, old_size=%ld, arr_idx=%ld",
	                     (long) old_used,
	                     (long) old_size,
	                     (long) arr_idx));

	min_size = arr_idx + 1;
#if defined(DUK_USE_OBJSIZES16)
	if (min_size > DUK_UINT16_MAX) {
		goto do_abandon;
	}
#endif
	DUK_UNREF(min_size);

	/* Note: intentionally use approximations to shave a few instructions:
	 *   a_used = old_used  (accurate: old_used + 1)
	 *   a_size = arr_idx   (accurate: arr_idx + 1)
	 */
	if (duk__abandon_array_density_check(old_used, arr_idx)) {
		DUK_DD(DUK_DDPRINT("write to new array entry beyond current length, "
		                   "decided to abandon array part (would become too sparse)"));

		/* Abandoning requires a props allocation resize and
		 * 'rechecks' the valstack, invalidating any existing
		 * valstack value pointers.
		 */
		goto do_abandon;
	}

	DUK_DDD(DUK_DDDPRINT("=> decided to keep array part"));
	return 0;

do_abandon:
	duk__abandon_array_part(thr, obj);
	DUK_ASSERT(!DUK_HOBJECT_HAS_ARRAY_PART(obj));
	return 1;
}

DUK_LOCAL duk_tval *duk__obtain_arridx_slot_slowpath(duk_hthread *thr, duk_uint32_t arr_idx, duk_hobject *obj) {
	/*
	 *  Array needs to grow, but we don't want it becoming too sparse.
	 *  If it were to become sparse, abandon array part, moving all
	 *  array entries into the entries part (for good).
	 *
	 *  Since we don't keep track of actual density (used vs. size) of
	 *  the array part, we need to estimate somehow.  The check is made
	 *  in two parts:
	 *
	 *    - Check whether the resize need is small compared to the
	 *      current size (relatively); if so, resize without further
	 *      checking (essentially we assume that the original part is
	 *      "dense" so that the result would be dense enough).
	 *
	 *    - Otherwise, compute the resize using an actual density
	 *      measurement based on counting the used array entries.
	 */

	DUK_DDD(DUK_DDDPRINT("write to new array requires array resize, decide whether to do a "
	                     "fast resize without abandon check (arr_idx=%ld, old_size=%ld)",
	                     (long) arr_idx,
	                     (long) DUK_HOBJECT_GET_ASIZE(obj)));

	if (DUK_UNLIKELY(duk__abandon_array_check(thr, arr_idx, obj) != 0)) {
		DUK_ASSERT(!DUK_HOBJECT_HAS_ARRAY_PART(obj));
		return NULL;
	}

	DUK_DD(DUK_DDPRINT("write to new array entry beyond current length, "
	                   "decided to extend current allocation"));

	/* In principle it's possible to run out of memory extending the
	 * array but with the allocation going through if we were to abandon
	 * the array part and try again.  In practice this should be rare
	 * because abandoned arrays have a higher per-entry footprint.
	 */

	duk__grow_props_for_array_item(thr, obj, arr_idx);

	DUK_ASSERT(DUK_HOBJECT_HAS_ARRAY_PART(obj));
	DUK_ASSERT(arr_idx < DUK_HOBJECT_GET_ASIZE(obj));
	return DUK_HOBJECT_A_GET_VALUE_PTR(thr->heap, obj, arr_idx);
}

DUK_LOCAL DUK_INLINE duk_tval *duk__obtain_arridx_slot(duk_hthread *thr, duk_uint32_t arr_idx, duk_hobject *obj) {
	if (DUK_LIKELY(arr_idx < DUK_HOBJECT_GET_ASIZE(obj))) {
		return DUK_HOBJECT_A_GET_VALUE_PTR(thr->heap, obj, arr_idx);
	} else {
		return duk__obtain_arridx_slot_slowpath(thr, arr_idx, obj);
	}
}

/*
 *  Proxy helpers
 */

#if defined(DUK_USE_ES6_PROXY)
DUK_INTERNAL duk_bool_t duk_hobject_proxy_check(duk_hobject *obj, duk_hobject **out_target, duk_hobject **out_handler) {
	duk_hproxy *h_proxy;

	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(out_target != NULL);
	DUK_ASSERT(out_handler != NULL);

	/* Caller doesn't need to check exotic proxy behavior (but does so for
	 * some fast paths).
	 */
	if (DUK_LIKELY(!DUK_HOBJECT_IS_PROXY(obj))) {
		return 0;
	}
	h_proxy = (duk_hproxy *) obj;
	DUK_HPROXY_ASSERT_VALID(h_proxy);

	DUK_ASSERT(h_proxy->handler != NULL);
	DUK_ASSERT(h_proxy->target != NULL);
	*out_handler = h_proxy->handler;
	*out_target = h_proxy->target;

	return 1;
}
#endif /* DUK_USE_ES6_PROXY */

/* Get Proxy target object.  If the argument is not a Proxy, return it as is.
 * If a Proxy is revoked, an error is thrown.
 */
#if defined(DUK_USE_ES6_PROXY)
DUK_INTERNAL duk_hobject *duk_hobject_resolve_proxy_target(duk_hobject *obj) {
	DUK_ASSERT(obj != NULL);

	/* Resolve Proxy targets until Proxy chain ends.  No explicit check for
	 * a Proxy loop: user code cannot create such a loop (it would only be
	 * possible by editing duk_hproxy references directly).
	 */

	while (DUK_HOBJECT_IS_PROXY(obj)) {
		duk_hproxy *h_proxy;

		h_proxy = (duk_hproxy *) obj;
		DUK_HPROXY_ASSERT_VALID(h_proxy);
		obj = h_proxy->target;
		DUK_ASSERT(obj != NULL);
	}

	DUK_ASSERT(obj != NULL);
	return obj;
}
#endif /* DUK_USE_ES6_PROXY */

#if defined(DUK_USE_ES6_PROXY)
DUK_LOCAL duk_bool_t duk__proxy_check_prop(duk_hthread *thr,
                                           duk_hobject *obj,
                                           duk_small_uint_t stridx_trap,
                                           duk_tval *tv_key,
                                           duk_hobject **out_target) {
	duk_hobject *h_handler;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(tv_key != NULL);
	DUK_ASSERT(out_target != NULL);

	if (!duk_hobject_proxy_check(obj, out_target, &h_handler)) {
		return 0;
	}
	DUK_ASSERT(*out_target != NULL);
	DUK_ASSERT(h_handler != NULL);

	/* XXX: At the moment Duktape accesses internal keys like _Finalizer using a
	 * normal property set/get which would allow a proxy handler to interfere with
	 * such behavior and to get access to internal key strings.  This is not a problem
	 * as such because internal key strings can be created in other ways too (e.g.
	 * through buffers).  The best fix is to change Duktape internal lookups to
	 * skip proxy behavior.  Until that, internal property accesses bypass the
	 * proxy and are applied to the target (as if the handler did not exist).
	 * This has some side effects, see test-bi-proxy-internal-keys.js.
	 */

	if (DUK_TVAL_IS_STRING(tv_key)) {
		duk_hstring *h_key = (duk_hstring *) DUK_TVAL_GET_STRING(tv_key);
		DUK_ASSERT(h_key != NULL);
		if (DUK_HSTRING_HAS_HIDDEN(h_key)) {
			/* Symbol accesses must go through proxy lookup in ES2015.
			 * Hidden symbols behave like Duktape 1.x internal keys
			 * and currently won't.
			 */
			DUK_DDD(DUK_DDDPRINT("hidden key, skip proxy handler and apply to target"));
			return 0;
		}
	}

	/* The handler is looked up with a normal property lookup; it may be an
	 * accessor or the handler object itself may be a proxy object.  If the
	 * handler is a proxy, we need to extend the valstack as we make a
	 * recursive proxy check without a function call in between (in fact
	 * there is no limit to the potential recursion here).
	 *
	 * (For sanity, proxy creation rejects another proxy object as either
	 * the handler or the target at the moment so recursive proxy cases
	 * are not realized now.)
	 */

	/* XXX: C recursion limit if proxies are allowed as handler/target values */

	duk_require_stack(thr, DUK__VALSTACK_PROXY_LOOKUP);
	duk_push_hobject(thr, h_handler);
	if (duk_get_prop_stridx_short(thr, -1, stridx_trap)) {
		/* -> [ ... handler trap ] */
		duk_insert(thr, -2); /* -> [ ... trap handler ] */

		/* stack prepped for func call: [ ... trap handler ] */
		return 1;
	} else {
		duk_pop_2_unsafe(thr);
		return 0;
	}
}
#endif /* DUK_USE_ES6_PROXY */

/*
 *  Reallocate property allocation, moving properties to the new allocation.
 *
 *  Includes key compaction, rehashing, and can also optionally abandon
 *  the array part, 'migrating' array entries into the beginning of the
 *  new entry part.
 *
 *  There is no support for in-place reallocation or just compacting keys
 *  without resizing the property allocation.  This is intentional to keep
 *  code size minimal, but would be useful future work.
 *
 *  The implementation is relatively straightforward, except for the array
 *  abandonment process.  Array abandonment requires that new string keys
 *  are interned, which may trigger GC.  All keys interned so far must be
 *  reachable for GC at all times and correctly refcounted for; valstack is
 *  used for that now.
 *
 *  Also, a GC triggered during this reallocation process must not interfere
 *  with the object being resized.  This is currently controlled by preventing
 *  finalizers (as they may affect ANY object) and object compaction in
 *  mark-and-sweep.  It would suffice to protect only this particular object
 *  from compaction, however.  DECREF refzero cascades are side effect free
 *  and OK.
 *
 *  Note: because we need to potentially resize the valstack (as part
 *  of abandoning the array part), any tval pointers to the valstack
 *  will become invalid after this call.
 */

DUK_INTERNAL void duk_hobject_realloc_props(duk_hthread *thr,
                                            duk_hobject *obj,
                                            duk_uint32_t new_e_size,
                                            duk_uint32_t new_a_size,
                                            duk_uint32_t new_h_size,
                                            duk_bool_t abandon_array) {
	duk_small_uint_t prev_ms_base_flags;
	duk_uint32_t new_alloc_size;
	duk_uint32_t new_e_size_adjusted;
	duk_uint8_t *new_p;
	duk_hstring **new_e_k;
	duk_propvalue *new_e_pv;
	duk_uint8_t *new_e_f;
	duk_tval *new_a;
	duk_uint32_t *new_h;
	duk_uint32_t new_e_next;
	duk_uint_fast32_t i;
	duk_size_t array_copy_size;
#if defined(DUK_USE_ASSERTIONS)
	duk_bool_t prev_error_not_allowed;
#endif

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(!abandon_array || new_a_size == 0); /* if abandon_array, new_a_size must be 0 */
	DUK_ASSERT(DUK_HOBJECT_GET_PROPS(thr->heap, obj) != NULL ||
	           (DUK_HOBJECT_GET_ESIZE(obj) == 0 && DUK_HOBJECT_GET_ASIZE(obj) == 0));
	DUK_ASSERT(new_h_size == 0 || new_h_size >= new_e_size); /* required to guarantee success of rehashing,
	                                                          * intentionally use unadjusted new_e_size
	                                                          */
	DUK_ASSERT(!DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) obj));
	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	DUK_STATS_INC(thr->heap, stats_object_realloc_props);

	/*
	 *  Pre resize assertions.
	 */

#if defined(DUK_USE_ASSERTIONS)
	/* XXX: pre-checks (such as no duplicate keys) */
#endif

	/*
	 *  For property layout 1, tweak e_size to ensure that the whole entry
	 *  part (key + val + flags) is a suitable multiple for alignment
	 *  (platform specific).
	 *
	 *  Property layout 2 does not require this tweaking and is preferred
	 *  on low RAM platforms requiring alignment.
	 */

#if defined(DUK_USE_HOBJECT_LAYOUT_2) || defined(DUK_USE_HOBJECT_LAYOUT_3)
	DUK_DDD(DUK_DDDPRINT("using layout 2 or 3, no need to pad e_size: %ld", (long) new_e_size));
	new_e_size_adjusted = new_e_size;
#elif defined(DUK_USE_HOBJECT_LAYOUT_1) && (DUK_HOBJECT_ALIGN_TARGET == 1)
	DUK_DDD(DUK_DDDPRINT("using layout 1, but no need to pad e_size: %ld", (long) new_e_size));
	new_e_size_adjusted = new_e_size;
#elif defined(DUK_USE_HOBJECT_LAYOUT_1) && ((DUK_HOBJECT_ALIGN_TARGET == 4) || (DUK_HOBJECT_ALIGN_TARGET == 8))
	new_e_size_adjusted =
	    (new_e_size + (duk_uint32_t) DUK_HOBJECT_ALIGN_TARGET - 1U) & (~((duk_uint32_t) DUK_HOBJECT_ALIGN_TARGET - 1U));
	DUK_DDD(DUK_DDDPRINT("using layout 1, and alignment target is %ld, adjusted e_size: %ld -> %ld",
	                     (long) DUK_HOBJECT_ALIGN_TARGET,
	                     (long) new_e_size,
	                     (long) new_e_size_adjusted));
	DUK_ASSERT(new_e_size_adjusted >= new_e_size);
#else
#error invalid hobject layout defines
#endif

	/*
	 *  Debug logging after adjustment.
	 */

	DUK_DDD(DUK_DDDPRINT(
	    "attempt to resize hobject %p props (%ld -> %ld bytes), from {p=%p,e_size=%ld,e_next=%ld,a_size=%ld,h_size=%ld} to "
	    "{e_size=%ld,a_size=%ld,h_size=%ld}, abandon_array=%ld, unadjusted new_e_size=%ld",
	    (void *) obj,
	    (long) DUK_HOBJECT_P_COMPUTE_SIZE(DUK_HOBJECT_GET_ESIZE(obj), DUK_HOBJECT_GET_ASIZE(obj), DUK_HOBJECT_GET_HSIZE(obj)),
	    (long) DUK_HOBJECT_P_COMPUTE_SIZE(new_e_size_adjusted, new_a_size, new_h_size),
	    (void *) DUK_HOBJECT_GET_PROPS(thr->heap, obj),
	    (long) DUK_HOBJECT_GET_ESIZE(obj),
	    (long) DUK_HOBJECT_GET_ENEXT(obj),
	    (long) DUK_HOBJECT_GET_ASIZE(obj),
	    (long) DUK_HOBJECT_GET_HSIZE(obj),
	    (long) new_e_size_adjusted,
	    (long) new_a_size,
	    (long) new_h_size,
	    (long) abandon_array,
	    (long) new_e_size));

	/*
	 *  Property count check.  This is the only point where we ensure that
	 *  we don't get more (allocated) property space that we can handle.
	 *  There aren't hard limits as such, but some algorithms may fail
	 *  if we get too close to the 4G property limit.
	 *
	 *  Since this works based on allocation size (not actually used size),
	 *  the limit is a bit approximate but good enough in practice.
	 */

	if (new_e_size_adjusted + new_a_size > DUK_HOBJECT_MAX_PROPERTIES) {
		DUK_ERROR_ALLOC_FAILED(thr);
		DUK_WO_NORETURN(return;);
	}
#if defined(DUK_USE_OBJSIZES16)
	if (new_e_size_adjusted > DUK_UINT16_MAX || new_a_size > DUK_UINT16_MAX) {
		/* If caller gave us sizes larger than what we can store,
		 * fail memory safely with an internal error rather than
		 * truncating the sizes.
		 */
		DUK_ERROR_INTERNAL(thr);
		DUK_WO_NORETURN(return;);
	}
#endif

	/*
	 *  Compute new alloc size and alloc new area.
	 *
	 *  The new area is not tracked in the heap at all, so it's critical
	 *  we get to free/keep it in a controlled manner.
	 */

#if defined(DUK_USE_ASSERTIONS)
	/* Whole path must be error throw free, but we may be called from
	 * within error handling so can't assert for error_not_allowed == 0.
	 */
	prev_error_not_allowed = thr->heap->error_not_allowed;
	thr->heap->error_not_allowed = 1;
#endif
	prev_ms_base_flags = thr->heap->ms_base_flags;
	thr->heap->ms_base_flags |=
	    DUK_MS_FLAG_NO_OBJECT_COMPACTION; /* Avoid attempt to compact the current object (all objects really). */
	thr->heap->pf_prevent_count++; /* Avoid finalizers. */
	DUK_ASSERT(thr->heap->pf_prevent_count != 0); /* Wrap. */

	new_alloc_size = DUK_HOBJECT_P_COMPUTE_SIZE(new_e_size_adjusted, new_a_size, new_h_size);
	DUK_DDD(DUK_DDDPRINT("new hobject allocation size is %ld", (long) new_alloc_size));
	if (new_alloc_size == 0) {
		DUK_ASSERT(new_e_size_adjusted == 0);
		DUK_ASSERT(new_a_size == 0);
		DUK_ASSERT(new_h_size == 0);
		new_p = NULL;
	} else {
		/* Alloc may trigger mark-and-sweep but no compaction, and
		 * cannot throw.
		 */
#if 0 /* XXX: inject test */
		if (1) {
			new_p = NULL;
			goto alloc_failed;
		}
#endif
		new_p = (duk_uint8_t *) DUK_ALLOC(thr->heap, new_alloc_size);
		if (new_p == NULL) {
			/* NULL always indicates alloc failure because
			 * new_alloc_size > 0.
			 */
			goto alloc_failed;
		}
	}

	/* Set up pointers to the new property area: this is hidden behind a macro
	 * because it is memory layout specific.
	 */
	DUK_HOBJECT_P_SET_REALLOC_PTRS(new_p,
	                               new_e_k,
	                               new_e_pv,
	                               new_e_f,
	                               new_a,
	                               new_h,
	                               new_e_size_adjusted,
	                               new_a_size,
	                               new_h_size);
	DUK_UNREF(new_h); /* happens when hash part dropped */
	new_e_next = 0;

	/* if new_p == NULL, all of these pointers are NULL */
	DUK_ASSERT((new_p != NULL) || (new_e_k == NULL && new_e_pv == NULL && new_e_f == NULL && new_a == NULL && new_h == NULL));

	DUK_DDD(DUK_DDDPRINT("new alloc size %ld, new_e_k=%p, new_e_pv=%p, new_e_f=%p, new_a=%p, new_h=%p",
	                     (long) new_alloc_size,
	                     (void *) new_e_k,
	                     (void *) new_e_pv,
	                     (void *) new_e_f,
	                     (void *) new_a,
	                     (void *) new_h));

	/*
	 *  Migrate array part to start of entries if requested.
	 *
	 *  Note: from an enumeration perspective the order of entry keys matters.
	 *  Array keys should appear wherever they appeared before the array abandon
	 *  operation.  (This no longer matters much because keys are ES2015 sorted.)
	 */

	if (abandon_array) {
		/* Assuming new_a_size == 0, and that entry part contains
		 * no conflicting keys, refcounts do not need to be adjusted for
		 * the values, as they remain exactly the same.
		 *
		 * The keys, however, need to be interned, incref'd, and be
		 * reachable for GC.  Any intern attempt may trigger a GC and
		 * claim any non-reachable strings, so every key must be reachable
		 * at all times.  Refcounts must be correct to satisfy refcount
		 * assertions.
		 *
		 * A longjmp must not occur here, as the new_p allocation would
		 * leak.  Refcounts would come out correctly as the interned
		 * strings are valstack tracked.
		 */
		DUK_ASSERT(new_a_size == 0);

		DUK_STATS_INC(thr->heap, stats_object_abandon_array);

		for (i = 0; i < DUK_HOBJECT_GET_ASIZE(obj); i++) {
			duk_tval *tv1;
			duk_tval *tv2;
			duk_hstring *key;

			DUK_ASSERT(DUK_HOBJECT_GET_PROPS(thr->heap, obj) != NULL);

			tv1 = DUK_HOBJECT_A_GET_VALUE_PTR(thr->heap, obj, i);
			if (DUK_TVAL_IS_UNUSED(tv1)) {
				continue;
			}

			DUK_ASSERT(new_p != NULL && new_e_k != NULL && new_e_pv != NULL && new_e_f != NULL);

			/*
			 *  Intern key via the valstack to ensure reachability behaves
			 *  properly.  We must avoid longjmp's here so use non-checked
			 *  primitives.
			 *
			 *  Note: duk_check_stack() potentially reallocs the valstack,
			 *  invalidating any duk_tval pointers to valstack.  Callers
			 *  must be careful.
			 */

#if 0 /* XXX: inject test */
			if (1) {
				goto abandon_error;
			}
#endif
			/* Never shrinks; auto-adds DUK_VALSTACK_INTERNAL_EXTRA, which
			 * is generous.
			 */
			if (!duk_check_stack(thr, 1)) {
				goto abandon_error;
			}
			DUK_ASSERT_VALSTACK_SPACE(thr, 1);
			key = duk_heap_strtable_intern_u32(thr->heap, (duk_uint32_t) i);
			if (key == NULL) {
				goto abandon_error;
			}
			duk_push_hstring(thr, key); /* keep key reachable for GC etc; guaranteed not to fail */

			/* Key is now reachable in the valstack, don't INCREF
			 * the new allocation yet (we'll steal the refcounts
			 * from the value stack once all keys are done).
			 */

			new_e_k[new_e_next] = key;
			tv2 = &new_e_pv[new_e_next].v; /* array entries are all plain values */
			DUK_TVAL_SET_TVAL(tv2, tv1);
			new_e_f[new_e_next] =
			    DUK_PROPDESC_FLAG_WRITABLE | DUK_PROPDESC_FLAG_ENUMERABLE | DUK_PROPDESC_FLAG_CONFIGURABLE;
			new_e_next++;

			/* Note: new_e_next matches pushed temp key count, and nothing can
			 * fail above between the push and this point.
			 */
		}

		/* Steal refcounts from value stack. */
		DUK_DDD(DUK_DDDPRINT("abandon array: pop %ld key temps from valstack", (long) new_e_next));
		duk_pop_n_nodecref_unsafe(thr, (duk_idx_t) new_e_next);
	}

	/*
	 *  Copy keys and values in the entry part (compacting them at the same time).
	 */

	for (i = 0; i < DUK_HOBJECT_GET_ENEXT(obj); i++) {
		duk_hstring *key;

		DUK_ASSERT(DUK_HOBJECT_GET_PROPS(thr->heap, obj) != NULL);

		key = DUK_HOBJECT_E_GET_KEY(thr->heap, obj, i);
		if (key == NULL) {
			continue;
		}

		DUK_ASSERT(new_p != NULL && new_e_k != NULL && new_e_pv != NULL && new_e_f != NULL);

		new_e_k[new_e_next] = key;
		new_e_pv[new_e_next] = DUK_HOBJECT_E_GET_VALUE(thr->heap, obj, i);
		new_e_f[new_e_next] = DUK_HOBJECT_E_GET_FLAGS(thr->heap, obj, i);
		new_e_next++;
	}
	/* the entries [new_e_next, new_e_size_adjusted[ are left uninitialized on purpose (ok, not gc reachable) */

	/*
	 *  Copy array elements to new array part.  If the new array part is
	 *  larger, initialize the unused entries as UNUSED because they are
	 *  GC reachable.
	 */

#if defined(DUK_USE_ASSERTIONS)
	/* Caller must have decref'd values above new_a_size (if that is necessary). */
	if (!abandon_array) {
		for (i = new_a_size; i < DUK_HOBJECT_GET_ASIZE(obj); i++) {
			duk_tval *tv;
			tv = DUK_HOBJECT_A_GET_VALUE_PTR(thr->heap, obj, i);
			DUK_ASSERT(DUK_TVAL_IS_UNUSED(tv));
		}
	}
#endif
	if (new_a_size > DUK_HOBJECT_GET_ASIZE(obj)) {
		array_copy_size = sizeof(duk_tval) * DUK_HOBJECT_GET_ASIZE(obj);
	} else {
		array_copy_size = sizeof(duk_tval) * new_a_size;
	}

	DUK_ASSERT(new_a != NULL || array_copy_size == 0U);
	DUK_ASSERT(DUK_HOBJECT_GET_PROPS(thr->heap, obj) != NULL || array_copy_size == 0U);
	DUK_ASSERT(DUK_HOBJECT_GET_ASIZE(obj) > 0 || array_copy_size == 0U);
	duk_memcpy_unsafe((void *) new_a, (const void *) DUK_HOBJECT_A_GET_BASE(thr->heap, obj), array_copy_size);

	for (i = DUK_HOBJECT_GET_ASIZE(obj); i < new_a_size; i++) {
		duk_tval *tv = &new_a[i];
		DUK_TVAL_SET_UNUSED(tv);
	}

	/*
	 *  Rebuild the hash part always from scratch (guaranteed to finish
	 *  as long as caller gave consistent parameters).
	 *
	 *  Any resize of hash part requires rehashing.  In addition, by rehashing
	 *  get rid of any elements marked deleted (DUK__HASH_DELETED) which is critical
	 *  to ensuring the hash part never fills up.
	 */

#if defined(DUK_USE_HOBJECT_HASH_PART)
	if (new_h_size == 0) {
		DUK_DDD(DUK_DDDPRINT("no hash part, no rehash"));
	} else {
		duk_uint32_t mask;

		DUK_ASSERT(new_h != NULL);

		/* fill new_h with u32 0xff = UNUSED */
		DUK_ASSERT(new_h_size > 0);
		duk_memset(new_h, 0xff, sizeof(duk_uint32_t) * new_h_size);

		DUK_ASSERT(new_e_next <= new_h_size); /* equality not actually possible */

		mask = new_h_size - 1;
		for (i = 0; i < new_e_next; i++) {
			duk_hstring *key = new_e_k[i];
			duk_uint32_t j, step;

			DUK_ASSERT(key != NULL);
			j = DUK_HSTRING_GET_HASH(key) & mask;
			step = 1; /* Cache friendly but clustering prone. */

			for (;;) {
				DUK_ASSERT(new_h[j] != DUK__HASH_DELETED); /* should never happen */
				if (new_h[j] == DUK__HASH_UNUSED) {
					DUK_DDD(DUK_DDDPRINT("rebuild hit %ld -> %ld", (long) j, (long) i));
					new_h[j] = (duk_uint32_t) i;
					break;
				}
				DUK_DDD(DUK_DDDPRINT("rebuild miss %ld, step %ld", (long) j, (long) step));
				j = (j + step) & mask;

				/* Guaranteed to finish (hash is larger than #props). */
			}
		}
	}
#endif /* DUK_USE_HOBJECT_HASH_PART */

	/*
	 *  Nice debug log.
	 */

	DUK_DD(DUK_DDPRINT(
	    "resized hobject %p props (%ld -> %ld bytes), from {p=%p,e_size=%ld,e_next=%ld,a_size=%ld,h_size=%ld} to "
	    "{p=%p,e_size=%ld,e_next=%ld,a_size=%ld,h_size=%ld}, abandon_array=%ld, unadjusted new_e_size=%ld",
	    (void *) obj,
	    (long) DUK_HOBJECT_P_COMPUTE_SIZE(DUK_HOBJECT_GET_ESIZE(obj), DUK_HOBJECT_GET_ASIZE(obj), DUK_HOBJECT_GET_HSIZE(obj)),
	    (long) new_alloc_size,
	    (void *) DUK_HOBJECT_GET_PROPS(thr->heap, obj),
	    (long) DUK_HOBJECT_GET_ESIZE(obj),
	    (long) DUK_HOBJECT_GET_ENEXT(obj),
	    (long) DUK_HOBJECT_GET_ASIZE(obj),
	    (long) DUK_HOBJECT_GET_HSIZE(obj),
	    (void *) new_p,
	    (long) new_e_size_adjusted,
	    (long) new_e_next,
	    (long) new_a_size,
	    (long) new_h_size,
	    (long) abandon_array,
	    (long) new_e_size));

	/*
	 *  All done, switch properties ('p') allocation to new one.
	 */

	DUK_FREE_CHECKED(thr, DUK_HOBJECT_GET_PROPS(thr->heap, obj)); /* NULL obj->p is OK */
	DUK_HOBJECT_SET_PROPS(thr->heap, obj, new_p);
	DUK_HOBJECT_SET_ESIZE(obj, new_e_size_adjusted);
	DUK_HOBJECT_SET_ENEXT(obj, new_e_next);
	DUK_HOBJECT_SET_ASIZE(obj, new_a_size);
	DUK_HOBJECT_SET_HSIZE(obj, new_h_size);

	/* Clear array part flag only after switching. */
	if (abandon_array) {
		DUK_HOBJECT_CLEAR_ARRAY_PART(obj);
	}

	DUK_DDD(DUK_DDDPRINT("resize result: %!O", (duk_heaphdr *) obj));

	DUK_ASSERT(thr->heap->pf_prevent_count > 0);
	thr->heap->pf_prevent_count--;
	thr->heap->ms_base_flags = prev_ms_base_flags;
#if defined(DUK_USE_ASSERTIONS)
	DUK_ASSERT(thr->heap->error_not_allowed == 1);
	thr->heap->error_not_allowed = prev_error_not_allowed;
#endif

	/*
	 *  Post resize assertions.
	 */

#if defined(DUK_USE_ASSERTIONS)
	/* XXX: post-checks (such as no duplicate keys) */
#endif
	return;

	/*
	 *  Abandon array failed.  We don't need to DECREF anything
	 *  because the references in the new allocation are not
	 *  INCREF'd until abandon is complete.  The string interned
	 *  keys are on the value stack and are handled normally by
	 *  unwind.
	 */

abandon_error:
alloc_failed:
	DUK_D(DUK_DPRINT("object property table resize failed"));

	DUK_FREE_CHECKED(thr, new_p); /* OK for NULL. */

	thr->heap->pf_prevent_count--;
	thr->heap->ms_base_flags = prev_ms_base_flags;
#if defined(DUK_USE_ASSERTIONS)
	DUK_ASSERT(thr->heap->error_not_allowed == 1);
	thr->heap->error_not_allowed = prev_error_not_allowed;
#endif

	DUK_ERROR_ALLOC_FAILED(thr);
	DUK_WO_NORETURN(return;);
}

/*
 *  Helpers to resize properties allocation on specific needs.
 */

DUK_INTERNAL void duk_hobject_resize_entrypart(duk_hthread *thr, duk_hobject *obj, duk_uint32_t new_e_size) {
	duk_uint32_t old_e_size;
	duk_uint32_t new_a_size;
	duk_uint32_t new_h_size;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(obj != NULL);

	old_e_size = DUK_HOBJECT_GET_ESIZE(obj);
	if (old_e_size > new_e_size) {
		new_e_size = old_e_size;
	}
#if defined(DUK_USE_HOBJECT_HASH_PART)
	new_h_size = duk__get_default_h_size(new_e_size);
#else
	new_h_size = 0;
#endif
	new_a_size = DUK_HOBJECT_GET_ASIZE(obj);

	duk_hobject_realloc_props(thr, obj, new_e_size, new_a_size, new_h_size, 0);
}

/* Grow entry part allocation for one additional entry. */
DUK_LOCAL void duk__grow_props_for_new_entry_item(duk_hthread *thr, duk_hobject *obj) {
	duk_uint32_t old_e_used; /* actually used, non-NULL entries */
	duk_uint32_t new_e_size_minimum;
	duk_uint32_t new_e_size;
	duk_uint32_t new_a_size;
	duk_uint32_t new_h_size;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(obj != NULL);

	/* Duktape 0.11.0 and prior tried to optimize the resize by not
	 * counting the number of actually used keys prior to the resize.
	 * This worked mostly well but also caused weird leak-like behavior
	 * as in: test-bug-object-prop-alloc-unbounded.js.  So, now we count
	 * the keys explicitly to compute the new entry part size.
	 */

	old_e_used = duk__count_used_e_keys(thr, obj);
	new_e_size_minimum = old_e_used + 1;
	new_e_size = old_e_used + duk__get_min_grow_e(old_e_used);
#if defined(DUK_USE_HOBJECT_HASH_PART)
	new_h_size = duk__get_default_h_size(new_e_size);
#else
	new_h_size = 0;
#endif
	new_a_size = DUK_HOBJECT_GET_ASIZE(obj);

#if defined(DUK_USE_OBJSIZES16)
	if (new_e_size > DUK_UINT16_MAX) {
		new_e_size = DUK_UINT16_MAX;
	}
	if (new_h_size > DUK_UINT16_MAX) {
		new_h_size = DUK_UINT16_MAX;
	}
	if (new_a_size > DUK_UINT16_MAX) {
		new_a_size = DUK_UINT16_MAX;
	}
#endif
	DUK_ASSERT(new_h_size == 0 || new_h_size >= new_e_size);

	if (!(new_e_size >= new_e_size_minimum)) {
		DUK_ERROR_ALLOC_FAILED(thr);
		DUK_WO_NORETURN(return;);
	}

	duk_hobject_realloc_props(thr, obj, new_e_size, new_a_size, new_h_size, 0);
}

/* Grow array part for a new highest array index. */
DUK_LOCAL void duk__grow_props_for_array_item(duk_hthread *thr, duk_hobject *obj, duk_uint32_t highest_arr_idx) {
	duk_uint32_t new_e_size;
	duk_uint32_t new_a_size;
	duk_uint32_t new_a_size_minimum;
	duk_uint32_t new_h_size;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(highest_arr_idx >= DUK_HOBJECT_GET_ASIZE(obj));

	new_e_size = DUK_HOBJECT_GET_ESIZE(obj);
	new_h_size = DUK_HOBJECT_GET_HSIZE(obj);
	new_a_size_minimum = highest_arr_idx + 1;
	new_a_size = highest_arr_idx + duk__get_min_grow_a(highest_arr_idx);
	DUK_ASSERT(new_a_size >= highest_arr_idx + 1); /* duk__get_min_grow_a() is always >= 1 */

#if defined(DUK_USE_OBJSIZES16)
	if (new_e_size > DUK_UINT16_MAX) {
		new_e_size = DUK_UINT16_MAX;
	}
	if (new_h_size > DUK_UINT16_MAX) {
		new_h_size = DUK_UINT16_MAX;
	}
	if (new_a_size > DUK_UINT16_MAX) {
		new_a_size = DUK_UINT16_MAX;
	}
#endif

	if (!(new_a_size >= new_a_size_minimum)) {
		DUK_ERROR_ALLOC_FAILED(thr);
		DUK_WO_NORETURN(return;);
	}

	duk_hobject_realloc_props(thr, obj, new_e_size, new_a_size, new_h_size, 0);
}

/* Abandon array part, moving array entries into entries part.
 * This requires a props resize, which is a heavy operation.
 * We also compact the entries part while we're at it, although
 * this is not strictly required.
 */
DUK_LOCAL void duk__abandon_array_part(duk_hthread *thr, duk_hobject *obj) {
	duk_uint32_t new_e_size_minimum;
	duk_uint32_t new_e_size;
	duk_uint32_t new_a_size;
	duk_uint32_t new_h_size;
	duk_uint32_t e_used; /* actually used, non-NULL keys */
	duk_uint32_t a_used;
	duk_uint32_t a_size;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(obj != NULL);

	e_used = duk__count_used_e_keys(thr, obj);
	duk__compute_a_stats(thr, obj, &a_used, &a_size);

	/*
	 *  Must guarantee all actually used array entries will fit into
	 *  new entry part.  Add one growth step to ensure we don't run out
	 *  of space right away.
	 */

	new_e_size_minimum = e_used + a_used;
	new_e_size = new_e_size_minimum + duk__get_min_grow_e(new_e_size_minimum);
	new_a_size = 0;
#if defined(DUK_USE_HOBJECT_HASH_PART)
	new_h_size = duk__get_default_h_size(new_e_size);
#else
	new_h_size = 0;
#endif

#if defined(DUK_USE_OBJSIZES16)
	if (new_e_size > DUK_UINT16_MAX) {
		new_e_size = DUK_UINT16_MAX;
	}
	if (new_h_size > DUK_UINT16_MAX) {
		new_h_size = DUK_UINT16_MAX;
	}
	if (new_a_size > DUK_UINT16_MAX) {
		new_a_size = DUK_UINT16_MAX;
	}
#endif

	if (!(new_e_size >= new_e_size_minimum)) {
		DUK_ERROR_ALLOC_FAILED(thr);
		DUK_WO_NORETURN(return;);
	}

	DUK_DD(DUK_DDPRINT("abandon array part for hobject %p, "
	                   "array stats before: e_used=%ld, a_used=%ld, a_size=%ld; "
	                   "resize to e_size=%ld, a_size=%ld, h_size=%ld",
	                   (void *) obj,
	                   (long) e_used,
	                   (long) a_used,
	                   (long) a_size,
	                   (long) new_e_size,
	                   (long) new_a_size,
	                   (long) new_h_size));

	duk_hobject_realloc_props(thr, obj, new_e_size, new_a_size, new_h_size, 1);
}

/*
 *  Compact an object.  Minimizes allocation size for objects which are
 *  not likely to be extended.  This is useful for internal and non-
 *  extensible objects, but can also be called for non-extensible objects.
 *  May abandon the array part if it is computed to be too sparse.
 *
 *  This call is relatively expensive, as it needs to scan both the
 *  entries and the array part.
 *
 *  The call may fail due to allocation error.
 */

DUK_INTERNAL void duk_hobject_compact_props(duk_hthread *thr, duk_hobject *obj) {
	duk_uint32_t e_size; /* currently used -> new size */
	duk_uint32_t a_size; /* currently required */
	duk_uint32_t a_used; /* actually used */
	duk_uint32_t h_size;
	duk_bool_t abandon_array;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(obj != NULL);

#if defined(DUK_USE_ROM_OBJECTS)
	if (DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) obj)) {
		DUK_DD(DUK_DDPRINT("ignore attempt to compact a rom object"));
		return;
	}
#endif

	e_size = duk__count_used_e_keys(thr, obj);
	duk__compute_a_stats(thr, obj, &a_used, &a_size);

	DUK_DD(DUK_DDPRINT("compacting hobject, used e keys %ld, used a keys %ld, min a size %ld, "
	                   "resized array density would be: %ld/%ld = %lf",
	                   (long) e_size,
	                   (long) a_used,
	                   (long) a_size,
	                   (long) a_used,
	                   (long) a_size,
	                   (double) a_used / (double) a_size));

	if (duk__abandon_array_density_check(a_used, a_size)) {
		DUK_DD(DUK_DDPRINT("decided to abandon array during compaction, a_used=%ld, a_size=%ld",
		                   (long) a_used,
		                   (long) a_size));
		abandon_array = 1;
		e_size += a_used;
		a_size = 0;
	} else {
		DUK_DD(DUK_DDPRINT("decided to keep array during compaction"));
		abandon_array = 0;
	}

#if defined(DUK_USE_HOBJECT_HASH_PART)
	if (e_size >= DUK_USE_HOBJECT_HASH_PROP_LIMIT) {
		h_size = duk__get_default_h_size(e_size);
	} else {
		h_size = 0;
	}
#else
	h_size = 0;
#endif

	DUK_DD(DUK_DDPRINT("compacting hobject -> new e_size %ld, new a_size=%ld, new h_size=%ld, abandon_array=%ld",
	                   (long) e_size,
	                   (long) a_size,
	                   (long) h_size,
	                   (long) abandon_array));

	duk_hobject_realloc_props(thr, obj, e_size, a_size, h_size, abandon_array);
}

/*
 *  Find an existing key from entry part either by linear scan or by
 *  using the hash index (if it exists).
 *
 *  Sets entry index (and possibly the hash index) to output variables,
 *  which allows the caller to update the entry and hash entries in-place.
 *  If entry is not found, both values are set to -1.  If entry is found
 *  but there is no hash part, h_idx is set to -1.
 */

DUK_INTERNAL duk_bool_t
duk_hobject_find_entry(duk_heap *heap, duk_hobject *obj, duk_hstring *key, duk_int_t *e_idx, duk_int_t *h_idx) {
	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(key != NULL);
	DUK_ASSERT(e_idx != NULL);
	DUK_ASSERT(h_idx != NULL);
	DUK_UNREF(heap);

	if (DUK_LIKELY(DUK_HOBJECT_GET_HSIZE(obj) == 0)) {
		/* Linear scan: more likely because most objects are small.
		 * This is an important fast path.
		 *
		 * XXX: this might be worth inlining for property lookups.
		 */
		duk_uint_fast32_t i;
		duk_uint_fast32_t n;
		duk_hstring **h_keys_base;
		DUK_DDD(DUK_DDDPRINT("duk_hobject_find_entry() using linear scan for lookup"));

		h_keys_base = DUK_HOBJECT_E_GET_KEY_BASE(heap, obj);
		n = DUK_HOBJECT_GET_ENEXT(obj);
		for (i = 0; i < n; i++) {
			if (h_keys_base[i] == key) {
				*e_idx = (duk_int_t) i;
				*h_idx = -1;
				return 1;
			}
		}
	}
#if defined(DUK_USE_HOBJECT_HASH_PART)
	else {
		/* hash lookup */
		duk_uint32_t n;
		duk_uint32_t i, step;
		duk_uint32_t *h_base;
		duk_uint32_t mask;

		DUK_DDD(DUK_DDDPRINT("duk_hobject_find_entry() using hash part for lookup"));

		h_base = DUK_HOBJECT_H_GET_BASE(heap, obj);
		n = DUK_HOBJECT_GET_HSIZE(obj);
		mask = n - 1;
		i = DUK_HSTRING_GET_HASH(key) & mask;
		step = 1; /* Cache friendly but clustering prone. */

		for (;;) {
			duk_uint32_t t;

			DUK_ASSERT_DISABLE(i >= 0); /* unsigned */
			DUK_ASSERT(i < DUK_HOBJECT_GET_HSIZE(obj));
			t = h_base[i];
			DUK_ASSERT(t == DUK__HASH_UNUSED || t == DUK__HASH_DELETED ||
			           (t < DUK_HOBJECT_GET_ESIZE(obj))); /* t >= 0 always true, unsigned */

			if (t == DUK__HASH_UNUSED) {
				break;
			} else if (t == DUK__HASH_DELETED) {
				DUK_DDD(DUK_DDDPRINT("lookup miss (deleted) i=%ld, t=%ld", (long) i, (long) t));
			} else {
				DUK_ASSERT(t < DUK_HOBJECT_GET_ESIZE(obj));
				if (DUK_HOBJECT_E_GET_KEY(heap, obj, t) == key) {
					DUK_DDD(
					    DUK_DDDPRINT("lookup hit i=%ld, t=%ld -> key %p", (long) i, (long) t, (void *) key));
					*e_idx = (duk_int_t) t;
					*h_idx = (duk_int_t) i;
					return 1;
				}
				DUK_DDD(DUK_DDDPRINT("lookup miss i=%ld, t=%ld", (long) i, (long) t));
			}
			i = (i + step) & mask;

			/* Guaranteed to finish (hash is larger than #props). */
		}
	}
#endif /* DUK_USE_HOBJECT_HASH_PART */

	/* Not found, leave e_idx and h_idx unset. */
	return 0;
}

/* For internal use: get non-accessor entry value */
DUK_INTERNAL duk_tval *duk_hobject_find_entry_tval_ptr(duk_heap *heap, duk_hobject *obj, duk_hstring *key) {
	duk_int_t e_idx;
	duk_int_t h_idx;

	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(key != NULL);
	DUK_UNREF(heap);

	if (duk_hobject_find_entry(heap, obj, key, &e_idx, &h_idx)) {
		DUK_ASSERT(e_idx >= 0);
		if (!DUK_HOBJECT_E_SLOT_IS_ACCESSOR(heap, obj, e_idx)) {
			return DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(heap, obj, e_idx);
		}
	}
	return NULL;
}

DUK_INTERNAL duk_tval *duk_hobject_find_entry_tval_ptr_stridx(duk_heap *heap, duk_hobject *obj, duk_small_uint_t stridx) {
	return duk_hobject_find_entry_tval_ptr(heap, obj, DUK_HEAP_GET_STRING(heap, stridx));
}

/* For internal use: get non-accessor entry value and attributes */
DUK_INTERNAL duk_tval *duk_hobject_find_entry_tval_ptr_and_attrs(duk_heap *heap,
                                                                 duk_hobject *obj,
                                                                 duk_hstring *key,
                                                                 duk_uint_t *out_attrs) {
	duk_int_t e_idx;
	duk_int_t h_idx;

	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(key != NULL);
	DUK_ASSERT(out_attrs != NULL);
	DUK_UNREF(heap);

	if (duk_hobject_find_entry(heap, obj, key, &e_idx, &h_idx)) {
		DUK_ASSERT(e_idx >= 0);
		if (!DUK_HOBJECT_E_SLOT_IS_ACCESSOR(heap, obj, e_idx)) {
			*out_attrs = DUK_HOBJECT_E_GET_FLAGS(heap, obj, e_idx);
			return DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(heap, obj, e_idx);
		}
	}
	/* If not found, out_attrs is left unset. */
	return NULL;
}

/* For internal use: get array part value */
DUK_INTERNAL duk_tval *duk_hobject_find_array_entry_tval_ptr(duk_heap *heap, duk_hobject *obj, duk_uarridx_t i) {
	duk_tval *tv;

	DUK_ASSERT(obj != NULL);
	DUK_UNREF(heap);

	if (!DUK_HOBJECT_HAS_ARRAY_PART(obj)) {
		return NULL;
	}
	if (i >= DUK_HOBJECT_GET_ASIZE(obj)) {
		return NULL;
	}
	tv = DUK_HOBJECT_A_GET_VALUE_PTR(heap, obj, i);
	return tv;
}

/*
 *  Allocate and initialize a new entry, resizing the properties allocation
 *  if necessary.  Returns entry index (e_idx) or throws an error if alloc fails.
 *
 *  Sets the key of the entry (increasing the key's refcount), and updates
 *  the hash part if it exists.  Caller must set value and flags, and update
 *  the entry value refcount.  A decref for the previous value is not necessary.
 */

DUK_LOCAL duk_int_t duk__hobject_alloc_entry_checked(duk_hthread *thr, duk_hobject *obj, duk_hstring *key) {
	duk_uint32_t idx;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(key != NULL);
	DUK_ASSERT(DUK_HOBJECT_GET_ENEXT(obj) <= DUK_HOBJECT_GET_ESIZE(obj));

#if defined(DUK_USE_ASSERTIONS)
	/* key must not already exist in entry part */
	{
		duk_uint_fast32_t i;
		for (i = 0; i < DUK_HOBJECT_GET_ENEXT(obj); i++) {
			DUK_ASSERT(DUK_HOBJECT_E_GET_KEY(thr->heap, obj, i) != key);
		}
	}
#endif

	if (DUK_HOBJECT_GET_ENEXT(obj) >= DUK_HOBJECT_GET_ESIZE(obj)) {
		/* only need to guarantee 1 more slot, but allocation growth is in chunks */
		DUK_DDD(DUK_DDDPRINT("entry part full, allocate space for one more entry"));
		duk__grow_props_for_new_entry_item(thr, obj);
	}
	DUK_ASSERT(DUK_HOBJECT_GET_ENEXT(obj) < DUK_HOBJECT_GET_ESIZE(obj));
	idx = DUK_HOBJECT_POSTINC_ENEXT(obj);

	/* previous value is assumed to be garbage, so don't touch it */
	DUK_HOBJECT_E_SET_KEY(thr->heap, obj, idx, key);
	DUK_HSTRING_INCREF(thr, key);

#if defined(DUK_USE_HOBJECT_HASH_PART)
	if (DUK_UNLIKELY(DUK_HOBJECT_GET_HSIZE(obj) > 0)) {
		duk_uint32_t n, mask;
		duk_uint32_t i, step;
		duk_uint32_t *h_base = DUK_HOBJECT_H_GET_BASE(thr->heap, obj);

		n = DUK_HOBJECT_GET_HSIZE(obj);
		mask = n - 1;
		i = DUK_HSTRING_GET_HASH(key) & mask;
		step = 1; /* Cache friendly but clustering prone. */

		for (;;) {
			duk_uint32_t t = h_base[i];
			if (t == DUK__HASH_UNUSED || t == DUK__HASH_DELETED) {
				DUK_DDD(DUK_DDDPRINT("duk__hobject_alloc_entry_checked() inserted key into hash part, %ld -> %ld",
				                     (long) i,
				                     (long) idx));
				DUK_ASSERT_DISABLE(i >= 0); /* unsigned */
				DUK_ASSERT(i < DUK_HOBJECT_GET_HSIZE(obj));
				DUK_ASSERT_DISABLE(idx >= 0);
				DUK_ASSERT(idx < DUK_HOBJECT_GET_ESIZE(obj));
				h_base[i] = idx;
				break;
			}
			DUK_DDD(DUK_DDDPRINT("duk__hobject_alloc_entry_checked() miss %ld", (long) i));
			i = (i + step) & mask;

			/* Guaranteed to finish (hash is larger than #props). */
		}
	}
#endif /* DUK_USE_HOBJECT_HASH_PART */

	/* Note: we could return the hash index here too, but it's not
	 * needed right now.
	 */

	DUK_ASSERT_DISABLE(idx >= 0);
	DUK_ASSERT(idx < DUK_HOBJECT_GET_ESIZE(obj));
	DUK_ASSERT(idx < DUK_HOBJECT_GET_ENEXT(obj));
	return (duk_int_t) idx;
}

/*
 *  Object internal value
 *
 *  Returned value is guaranteed to be reachable / incref'd, caller does not need
 *  to incref OR decref.  No proxies or accessors are invoked, no prototype walk.
 */

DUK_INTERNAL duk_tval *duk_hobject_get_internal_value_tval_ptr(duk_heap *heap, duk_hobject *obj) {
	return duk_hobject_find_entry_tval_ptr_stridx(heap, obj, DUK_STRIDX_INT_VALUE);
}

DUK_LOCAL duk_heaphdr *duk_hobject_get_internal_value_heaphdr(duk_heap *heap, duk_hobject *obj) {
	duk_tval *tv;

	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(obj != NULL);

	tv = duk_hobject_get_internal_value_tval_ptr(heap, obj);
	if (tv != NULL) {
		duk_heaphdr *h = DUK_TVAL_GET_HEAPHDR(tv);
		DUK_ASSERT(h != NULL);
		return h;
	}

	return NULL;
}

DUK_INTERNAL duk_hstring *duk_hobject_get_internal_value_string(duk_heap *heap, duk_hobject *obj) {
	duk_hstring *h;

	h = (duk_hstring *) duk_hobject_get_internal_value_heaphdr(heap, obj);
	if (h != NULL) {
		DUK_ASSERT(DUK_HEAPHDR_IS_STRING((duk_heaphdr *) h));
	}
	return h;
}

DUK_LOCAL duk_hobject *duk__hobject_get_entry_object_stridx(duk_heap *heap, duk_hobject *obj, duk_small_uint_t stridx) {
	duk_tval *tv;
	duk_hobject *h;

	tv = duk_hobject_find_entry_tval_ptr_stridx(heap, obj, stridx);
	if (tv != NULL && DUK_TVAL_IS_OBJECT(tv)) {
		h = DUK_TVAL_GET_OBJECT(tv);
		DUK_ASSERT(h != NULL);
		return h;
	}
	return NULL;
}

DUK_INTERNAL duk_harray *duk_hobject_get_formals(duk_hthread *thr, duk_hobject *obj) {
	duk_harray *h;

	h = (duk_harray *) duk__hobject_get_entry_object_stridx(thr->heap, obj, DUK_STRIDX_INT_FORMALS);
	if (h != NULL) {
		DUK_ASSERT(DUK_HOBJECT_IS_ARRAY((duk_hobject *) h));
		DUK_ASSERT(h->length <= DUK_HOBJECT_GET_ASIZE((duk_hobject *) h));
	}
	return h;
}

DUK_INTERNAL duk_hobject *duk_hobject_get_varmap(duk_hthread *thr, duk_hobject *obj) {
	duk_hobject *h;

	h = duk__hobject_get_entry_object_stridx(thr->heap, obj, DUK_STRIDX_INT_VARMAP);
	return h;
}

/*
 *  Arguments handling helpers (argument map mainly).
 *
 *  An arguments object has exotic behavior for some numeric indices.
 *  Accesses may translate to identifier operations which may have
 *  arbitrary side effects (potentially invalidating any duk_tval
 *  pointers).
 */

/* Lookup 'key' from arguments internal 'map', perform a variable lookup
 * if mapped, and leave the result on top of stack (and return non-zero).
 * Used in E5 Section 10.6 algorithms [[Get]] and [[GetOwnProperty]].
 */
DUK_LOCAL
duk_bool_t duk__lookup_arguments_map(duk_hthread *thr,
                                     duk_hobject *obj,
                                     duk_hstring *key,
                                     duk_propdesc *temp_desc,
                                     duk_hobject **out_map,
                                     duk_hobject **out_varenv) {
	duk_hobject *map;
	duk_hobject *varenv;
	duk_bool_t rc;

	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	DUK_DDD(DUK_DDDPRINT("arguments map lookup: thr=%p, obj=%p, key=%p, temp_desc=%p "
	                     "(obj -> %!O, key -> %!O)",
	                     (void *) thr,
	                     (void *) obj,
	                     (void *) key,
	                     (void *) temp_desc,
	                     (duk_heaphdr *) obj,
	                     (duk_heaphdr *) key));

	if (!duk_hobject_get_own_propdesc(thr, obj, DUK_HTHREAD_STRING_INT_MAP(thr), temp_desc, DUK_GETDESC_FLAG_PUSH_VALUE)) {
		DUK_DDD(DUK_DDDPRINT("-> no 'map'"));
		return 0;
	}

	map = duk_require_hobject(thr, -1);
	DUK_ASSERT(map != NULL);
	duk_pop_unsafe(thr); /* map is reachable through obj */

	if (!duk_hobject_get_own_propdesc(thr, map, key, temp_desc, DUK_GETDESC_FLAG_PUSH_VALUE)) {
		DUK_DDD(DUK_DDDPRINT("-> 'map' exists, but key not in map"));
		return 0;
	}

	/* [... varname] */
	DUK_DDD(DUK_DDDPRINT("-> 'map' exists, and contains key, key is mapped to argument/variable binding %!T",
	                     (duk_tval *) duk_get_tval(thr, -1)));
	DUK_ASSERT(duk_is_string(thr, -1)); /* guaranteed when building arguments */

	/* get varenv for varname (callee's declarative lexical environment) */
	rc = duk_hobject_get_own_propdesc(thr, obj, DUK_HTHREAD_STRING_INT_VARENV(thr), temp_desc, DUK_GETDESC_FLAG_PUSH_VALUE);
	DUK_UNREF(rc);
	DUK_ASSERT(rc != 0); /* arguments MUST have an initialized lexical environment reference */
	varenv = duk_require_hobject(thr, -1);
	DUK_ASSERT(varenv != NULL);
	duk_pop_unsafe(thr); /* varenv remains reachable through 'obj' */

	DUK_DDD(DUK_DDDPRINT("arguments varenv is: %!dO", (duk_heaphdr *) varenv));

	/* success: leave varname in stack */
	*out_map = map;
	*out_varenv = varenv;
	return 1; /* [... varname] */
}

/* Lookup 'key' from arguments internal 'map', and leave replacement value
 * on stack top if mapped (and return non-zero).
 * Used in E5 Section 10.6 algorithm for [[GetOwnProperty]] (used by [[Get]]).
 */
DUK_LOCAL duk_bool_t duk__check_arguments_map_for_get(duk_hthread *thr,
                                                      duk_hobject *obj,
                                                      duk_hstring *key,
                                                      duk_propdesc *temp_desc) {
	duk_hobject *map;
	duk_hobject *varenv;
	duk_hstring *varname;

	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	if (!duk__lookup_arguments_map(thr, obj, key, temp_desc, &map, &varenv)) {
		DUK_DDD(DUK_DDDPRINT("arguments: key not mapped, no exotic get behavior"));
		return 0;
	}

	/* [... varname] */

	varname = duk_require_hstring(thr, -1);
	DUK_ASSERT(varname != NULL);
	duk_pop_unsafe(thr); /* varname is still reachable */

	DUK_DDD(DUK_DDDPRINT("arguments object automatic getvar for a bound variable; "
	                     "key=%!O, varname=%!O",
	                     (duk_heaphdr *) key,
	                     (duk_heaphdr *) varname));

	(void) duk_js_getvar_envrec(thr, varenv, varname, 1 /*throw*/);

	/* [... value this_binding] */

	duk_pop_unsafe(thr);

	/* leave result on stack top */
	return 1;
}

/* Lookup 'key' from arguments internal 'map', perform a variable write if mapped.
 * Used in E5 Section 10.6 algorithm for [[DefineOwnProperty]] (used by [[Put]]).
 * Assumes stack top contains 'put' value (which is NOT popped).
 */
DUK_LOCAL void duk__check_arguments_map_for_put(duk_hthread *thr,
                                                duk_hobject *obj,
                                                duk_hstring *key,
                                                duk_propdesc *temp_desc,
                                                duk_bool_t throw_flag) {
	duk_hobject *map;
	duk_hobject *varenv;
	duk_hstring *varname;

	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	if (!duk__lookup_arguments_map(thr, obj, key, temp_desc, &map, &varenv)) {
		DUK_DDD(DUK_DDDPRINT("arguments: key not mapped, no exotic put behavior"));
		return;
	}

	/* [... put_value varname] */

	varname = duk_require_hstring(thr, -1);
	DUK_ASSERT(varname != NULL);
	duk_pop_unsafe(thr); /* varname is still reachable */

	DUK_DDD(DUK_DDDPRINT("arguments object automatic putvar for a bound variable; "
	                     "key=%!O, varname=%!O, value=%!T",
	                     (duk_heaphdr *) key,
	                     (duk_heaphdr *) varname,
	                     (duk_tval *) duk_require_tval(thr, -1)));

	/* [... put_value] */

	/*
	 *  Note: although arguments object variable mappings are only established
	 *  for non-strict functions (and a call to a non-strict function created
	 *  the arguments object in question), an inner strict function may be doing
	 *  the actual property write.  Hence the throw_flag applied here comes from
	 *  the property write call.
	 */

	duk_js_putvar_envrec(thr, varenv, varname, duk_require_tval(thr, -1), throw_flag);

	/* [... put_value] */
}

/* Lookup 'key' from arguments internal 'map', delete mapping if found.
 * Used in E5 Section 10.6 algorithm for [[Delete]].  Note that the
 * variable/argument itself (where the map points) is not deleted.
 */
DUK_LOCAL void duk__check_arguments_map_for_delete(duk_hthread *thr, duk_hobject *obj, duk_hstring *key, duk_propdesc *temp_desc) {
	duk_hobject *map;

	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	if (!duk_hobject_get_own_propdesc(thr, obj, DUK_HTHREAD_STRING_INT_MAP(thr), temp_desc, DUK_GETDESC_FLAG_PUSH_VALUE)) {
		DUK_DDD(DUK_DDDPRINT("arguments: key not mapped, no exotic delete behavior"));
		return;
	}

	map = duk_require_hobject(thr, -1);
	DUK_ASSERT(map != NULL);
	duk_pop_unsafe(thr); /* map is reachable through obj */

	DUK_DDD(DUK_DDDPRINT("-> have 'map', delete key %!O from map (if exists)); ignore result", (duk_heaphdr *) key));

	/* Note: no recursion issue, we can trust 'map' to behave */
	DUK_ASSERT(!DUK_HOBJECT_HAS_EXOTIC_BEHAVIOR(map));
	DUK_DDD(DUK_DDDPRINT("map before deletion: %!O", (duk_heaphdr *) map));
	(void) duk_hobject_delprop_raw(thr, map, key, 0); /* ignore result */
	DUK_DDD(DUK_DDDPRINT("map after deletion: %!O", (duk_heaphdr *) map));
}

/*
 *  ECMAScript compliant [[GetOwnProperty]](P), for internal use only.
 *
 *  If property is found:
 *    - Fills descriptor fields to 'out_desc'
 *    - If DUK_GETDESC_FLAG_PUSH_VALUE is set, pushes a value related to the
 *      property onto the stack ('undefined' for accessor properties).
 *    - Returns non-zero
 *
 *  If property is not found:
 *    - 'out_desc' is left in untouched state (possibly garbage)
 *    - Nothing is pushed onto the stack (not even with DUK_GETDESC_FLAG_PUSH_VALUE
 *      set)
 *    - Returns zero
 *
 *  Notes:
 *
 *    - Getting a property descriptor may cause an allocation (and hence
 *      GC) to take place, hence reachability and refcount of all related
 *      values matter.  Reallocation of value stack, properties, etc may
 *      invalidate many duk_tval pointers (concretely, those which reside
 *      in memory areas subject to reallocation).  However, heap object
 *      pointers are never affected (heap objects have stable pointers).
 *
 *    - The value of a plain property is always reachable and has a non-zero
 *      reference count.
 *
 *    - The value of a virtual property is not necessarily reachable from
 *      elsewhere and may have a refcount of zero.  Hence we push it onto
 *      the valstack for the caller, which ensures it remains reachable
 *      while it is needed.
 *
 *    - There are no virtual accessor properties.  Hence, all getters and
 *      setters are always related to concretely stored properties, which
 *      ensures that the get/set functions in the resulting descriptor are
 *      reachable and have non-zero refcounts.  Should there be virtual
 *      accessor properties later, this would need to change.
 */

DUK_LOCAL duk_bool_t duk__get_own_propdesc_raw(duk_hthread *thr,
                                               duk_hobject *obj,
                                               duk_hstring *key,
                                               duk_uint32_t arr_idx,
                                               duk_propdesc *out_desc,
                                               duk_small_uint_t flags) {
	duk_tval *tv;

	DUK_DDD(DUK_DDDPRINT("duk_hobject_get_own_propdesc: thr=%p, obj=%p, key=%p, out_desc=%p, flags=%lx, "
	                     "arr_idx=%ld (obj -> %!O, key -> %!O)",
	                     (void *) thr,
	                     (void *) obj,
	                     (void *) key,
	                     (void *) out_desc,
	                     (long) flags,
	                     (long) arr_idx,
	                     (duk_heaphdr *) obj,
	                     (duk_heaphdr *) key));

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(key != NULL);
	DUK_ASSERT(out_desc != NULL);
	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	DUK_STATS_INC(thr->heap, stats_getownpropdesc_count);

	/* Each code path returning 1 (= found) must fill in all the output
	 * descriptor fields.  We don't do it beforehand because it'd be
	 * unnecessary work if the property isn't found and would happen
	 * multiple times for an inheritance chain.
	 */
	DUK_ASSERT_SET_GARBAGE(out_desc, sizeof(*out_desc));
#if 0
	out_desc->flags = 0;
	out_desc->get = NULL;
	out_desc->set = NULL;
	out_desc->e_idx = -1;
	out_desc->h_idx = -1;
	out_desc->a_idx = -1;
#endif

	/*
	 *  Try entries part first because it's the common case.
	 *
	 *  Array part lookups are usually handled by the array fast path, and
	 *  are not usually inherited.  Array and entry parts never contain the
	 *  same keys so the entry part vs. array part order doesn't matter.
	 */

	if (duk_hobject_find_entry(thr->heap, obj, key, &out_desc->e_idx, &out_desc->h_idx)) {
		duk_int_t e_idx = out_desc->e_idx;
		DUK_ASSERT(out_desc->e_idx >= 0);
		out_desc->a_idx = -1;
		out_desc->flags = DUK_HOBJECT_E_GET_FLAGS(thr->heap, obj, e_idx);
		out_desc->get = NULL;
		out_desc->set = NULL;
		if (DUK_UNLIKELY(out_desc->flags & DUK_PROPDESC_FLAG_ACCESSOR)) {
			DUK_DDD(DUK_DDDPRINT("-> found accessor property in entry part"));
			out_desc->get = DUK_HOBJECT_E_GET_VALUE_GETTER(thr->heap, obj, e_idx);
			out_desc->set = DUK_HOBJECT_E_GET_VALUE_SETTER(thr->heap, obj, e_idx);
			if (flags & DUK_GETDESC_FLAG_PUSH_VALUE) {
				/* a dummy undefined value is pushed to make valstack
				 * behavior uniform for caller
				 */
				duk_push_undefined(thr);
			}
		} else {
			DUK_DDD(DUK_DDDPRINT("-> found plain property in entry part"));
			tv = DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(thr->heap, obj, e_idx);
			if (flags & DUK_GETDESC_FLAG_PUSH_VALUE) {
				duk_push_tval(thr, tv);
			}
		}
		goto prop_found;
	}

	/*
	 *  Try array part.
	 */

	if (DUK_HOBJECT_HAS_ARRAY_PART(obj) && arr_idx != DUK__NO_ARRAY_INDEX) {
		if (arr_idx < DUK_HOBJECT_GET_ASIZE(obj)) {
			tv = DUK_HOBJECT_A_GET_VALUE_PTR(thr->heap, obj, arr_idx);
			if (!DUK_TVAL_IS_UNUSED(tv)) {
				DUK_DDD(DUK_DDDPRINT("-> found in array part"));
				if (flags & DUK_GETDESC_FLAG_PUSH_VALUE) {
					duk_push_tval(thr, tv);
				}
				/* implicit attributes */
				out_desc->flags =
				    DUK_PROPDESC_FLAG_WRITABLE | DUK_PROPDESC_FLAG_CONFIGURABLE | DUK_PROPDESC_FLAG_ENUMERABLE;
				out_desc->get = NULL;
				out_desc->set = NULL;
				out_desc->e_idx = -1;
				out_desc->h_idx = -1;
				out_desc->a_idx = (duk_int_t) arr_idx; /* XXX: limit 2G due to being signed */
				goto prop_found;
			}
		}
	}

	DUK_DDD(DUK_DDDPRINT("-> not found as a concrete property"));

	/*
	 *  Not found as a concrete property, check for virtual properties.
	 */

	if (!DUK_HOBJECT_HAS_VIRTUAL_PROPERTIES(obj)) {
		/* Quick skip. */
		goto prop_not_found;
	}

	if (DUK_HOBJECT_HAS_EXOTIC_ARRAY(obj)) {
		duk_harray *a;

		DUK_DDD(DUK_DDDPRINT("array object exotic property get for key: %!O, arr_idx: %ld",
		                     (duk_heaphdr *) key,
		                     (long) arr_idx));

		a = (duk_harray *) obj;
		DUK_HARRAY_ASSERT_VALID(a);

		if (key == DUK_HTHREAD_STRING_LENGTH(thr)) {
			DUK_DDD(DUK_DDDPRINT("-> found, key is 'length', length exotic behavior"));

			if (flags & DUK_GETDESC_FLAG_PUSH_VALUE) {
				duk_push_uint(thr, (duk_uint_t) a->length);
			}
			out_desc->flags = DUK_PROPDESC_FLAG_VIRTUAL;
			if (DUK_HARRAY_LENGTH_WRITABLE(a)) {
				out_desc->flags |= DUK_PROPDESC_FLAG_WRITABLE;
			}
			out_desc->get = NULL;
			out_desc->set = NULL;
			out_desc->e_idx = -1;
			out_desc->h_idx = -1;
			out_desc->a_idx = -1;

			DUK_ASSERT(!DUK_HOBJECT_HAS_EXOTIC_ARGUMENTS(obj));
			goto prop_found_noexotic; /* cannot be arguments exotic */
		}
	} else if (DUK_HOBJECT_HAS_EXOTIC_STRINGOBJ(obj)) {
		DUK_DDD(DUK_DDDPRINT("string object exotic property get for key: %!O, arr_idx: %ld",
		                     (duk_heaphdr *) key,
		                     (long) arr_idx));

		/* XXX: charlen; avoid multiple lookups? */

		if (arr_idx != DUK__NO_ARRAY_INDEX) {
			duk_hstring *h_val;

			DUK_DDD(DUK_DDDPRINT("array index exists"));

			h_val = duk_hobject_get_internal_value_string(thr->heap, obj);
			DUK_ASSERT(h_val);
			if (arr_idx < DUK_HSTRING_GET_CHARLEN(h_val)) {
				DUK_DDD(DUK_DDDPRINT("-> found, array index inside string"));
				if (flags & DUK_GETDESC_FLAG_PUSH_VALUE) {
					duk_push_hstring(thr, h_val);
					duk_substring(thr, -1, arr_idx, arr_idx + 1); /* [str] -> [substr] */
				}
				out_desc->flags = DUK_PROPDESC_FLAG_ENUMERABLE | /* E5 Section 15.5.5.2 */
				                  DUK_PROPDESC_FLAG_VIRTUAL;
				out_desc->get = NULL;
				out_desc->set = NULL;
				out_desc->e_idx = -1;
				out_desc->h_idx = -1;
				out_desc->a_idx = -1;

				DUK_ASSERT(!DUK_HOBJECT_HAS_EXOTIC_ARGUMENTS(obj));
				goto prop_found_noexotic; /* cannot be arguments exotic */
			} else {
				/* index is above internal string length -> property is fully normal */
				DUK_DDD(DUK_DDDPRINT("array index outside string -> normal property"));
			}
		} else if (key == DUK_HTHREAD_STRING_LENGTH(thr)) {
			duk_hstring *h_val;

			DUK_DDD(DUK_DDDPRINT("-> found, key is 'length', length exotic behavior"));

			h_val = duk_hobject_get_internal_value_string(thr->heap, obj);
			DUK_ASSERT(h_val != NULL);
			if (flags & DUK_GETDESC_FLAG_PUSH_VALUE) {
				duk_push_uint(thr, (duk_uint_t) DUK_HSTRING_GET_CHARLEN(h_val));
			}
			out_desc->flags = DUK_PROPDESC_FLAG_VIRTUAL; /* E5 Section 15.5.5.1 */
			out_desc->get = NULL;
			out_desc->set = NULL;
			out_desc->e_idx = -1;
			out_desc->h_idx = -1;
			out_desc->a_idx = -1;

			DUK_ASSERT(!DUK_HOBJECT_HAS_EXOTIC_ARGUMENTS(obj));
			goto prop_found_noexotic; /* cannot be arguments exotic */
		}
	}
#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
	else if (DUK_HOBJECT_IS_BUFOBJ(obj)) {
		duk_hbufobj *h_bufobj;
		duk_uint_t byte_off;
		duk_small_uint_t elem_size;

		h_bufobj = (duk_hbufobj *) obj;
		DUK_HBUFOBJ_ASSERT_VALID(h_bufobj);
		DUK_DDD(DUK_DDDPRINT("bufobj property get for key: %!O, arr_idx: %ld", (duk_heaphdr *) key, (long) arr_idx));

		if (arr_idx != DUK__NO_ARRAY_INDEX && DUK_HBUFOBJ_HAS_VIRTUAL_INDICES(h_bufobj)) {
			DUK_DDD(DUK_DDDPRINT("array index exists"));

			/* Careful with wrapping: arr_idx upshift may easily wrap, whereas
			 * length downshift won't.
			 */
			if (arr_idx < (h_bufobj->length >> h_bufobj->shift)) {
				byte_off = arr_idx << h_bufobj->shift; /* no wrap assuming h_bufobj->length is valid */
				elem_size = (duk_small_uint_t) (1U << h_bufobj->shift);
				if (flags & DUK_GETDESC_FLAG_PUSH_VALUE) {
					duk_uint8_t *data;

					if (h_bufobj->buf != NULL &&
					    DUK_HBUFOBJ_VALID_BYTEOFFSET_EXCL(h_bufobj, byte_off + elem_size)) {
						data = (duk_uint8_t *) DUK_HBUFFER_GET_DATA_PTR(thr->heap, h_bufobj->buf) +
						       h_bufobj->offset + byte_off;
						duk_hbufobj_push_validated_read(thr, h_bufobj, data, elem_size);
					} else {
						DUK_D(DUK_DPRINT("bufobj access out of underlying buffer, ignoring (read zero)"));
						duk_push_uint(thr, 0);
					}
				}
				out_desc->flags = DUK_PROPDESC_FLAG_WRITABLE | DUK_PROPDESC_FLAG_VIRTUAL;
				if (DUK_HOBJECT_GET_CLASS_NUMBER(obj) != DUK_HOBJECT_CLASS_ARRAYBUFFER) {
					/* ArrayBuffer indices are non-standard and are
					 * non-enumerable to avoid their serialization.
					 */
					out_desc->flags |= DUK_PROPDESC_FLAG_ENUMERABLE;
				}
				out_desc->get = NULL;
				out_desc->set = NULL;
				out_desc->e_idx = -1;
				out_desc->h_idx = -1;
				out_desc->a_idx = -1;

				DUK_ASSERT(!DUK_HOBJECT_HAS_EXOTIC_ARGUMENTS(obj));
				goto prop_found_noexotic; /* cannot be e.g. arguments exotic, since exotic 'traits' are mutually
				                             exclusive */
			} else {
				/* index is above internal buffer length -> property is fully normal */
				DUK_DDD(DUK_DDDPRINT("array index outside buffer -> normal property"));
			}
		} else if (key == DUK_HTHREAD_STRING_LENGTH(thr) && DUK_HBUFOBJ_HAS_VIRTUAL_INDICES(h_bufobj)) {
			DUK_DDD(DUK_DDDPRINT("-> found, key is 'length', length exotic behavior"));

			if (flags & DUK_GETDESC_FLAG_PUSH_VALUE) {
				/* Length in elements: take into account shift, but
				 * intentionally don't check the underlying buffer here.
				 */
				duk_push_uint(thr, h_bufobj->length >> h_bufobj->shift);
			}
			out_desc->flags = DUK_PROPDESC_FLAG_VIRTUAL;
			out_desc->get = NULL;
			out_desc->set = NULL;
			out_desc->e_idx = -1;
			out_desc->h_idx = -1;
			out_desc->a_idx = -1;

			DUK_ASSERT(!DUK_HOBJECT_HAS_EXOTIC_ARGUMENTS(obj));
			goto prop_found_noexotic; /* cannot be arguments exotic */
		}
	}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

	/* Array properties have exotic behavior but they are concrete,
	 * so no special handling here.
	 *
	 * Arguments exotic behavior (E5 Section 10.6, [[GetOwnProperty]]
	 * is only relevant as a post-check implemented below; hence no
	 * check here.
	 */

	/*
	 *  Not found as concrete or virtual.
	 */

prop_not_found:
	DUK_DDD(DUK_DDDPRINT("-> not found (virtual, entry part, or array part)"));
	DUK_STATS_INC(thr->heap, stats_getownpropdesc_miss);
	return 0;

	/*
	 *  Found.
	 *
	 *  Arguments object has exotic post-processing, see E5 Section 10.6,
	 *  description of [[GetOwnProperty]] variant for arguments.
	 */

prop_found:
	DUK_DDD(DUK_DDDPRINT("-> property found, checking for arguments exotic post-behavior"));

	/* Notes:
	 *  - Only numbered indices are relevant, so arr_idx fast reject is good
	 *    (this is valid unless there are more than 4**32-1 arguments).
	 *  - Since variable lookup has no side effects, this can be skipped if
	 *    DUK_GETDESC_FLAG_PUSH_VALUE is not set.
	 */

	if (DUK_UNLIKELY(DUK_HOBJECT_HAS_EXOTIC_ARGUMENTS(obj) && arr_idx != DUK__NO_ARRAY_INDEX &&
	                 (flags & DUK_GETDESC_FLAG_PUSH_VALUE))) {
		duk_propdesc temp_desc;

		/* Magically bound variable cannot be an accessor.  However,
		 * there may be an accessor property (or a plain property) in
		 * place with magic behavior removed.  This happens e.g. when
		 * a magic property is redefined with defineProperty().
		 * Cannot assert for "not accessor" here.
		 */

		/* replaces top of stack with new value if necessary */
		DUK_ASSERT((flags & DUK_GETDESC_FLAG_PUSH_VALUE) != 0);

		/* This can perform a variable lookup but only into a declarative
		 * environment which has no side effects.
		 */
		if (duk__check_arguments_map_for_get(thr, obj, key, &temp_desc)) {
			DUK_DDD(DUK_DDDPRINT("-> arguments exotic behavior overrides result: %!T -> %!T",
			                     (duk_tval *) duk_get_tval(thr, -2),
			                     (duk_tval *) duk_get_tval(thr, -1)));
			/* [... old_result result] -> [... result] */
			duk_remove_m2(thr);
		}
	}

prop_found_noexotic:
	DUK_STATS_INC(thr->heap, stats_getownpropdesc_hit);
	return 1;
}

DUK_INTERNAL duk_bool_t
duk_hobject_get_own_propdesc(duk_hthread *thr, duk_hobject *obj, duk_hstring *key, duk_propdesc *out_desc, duk_small_uint_t flags) {
	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(key != NULL);
	DUK_ASSERT(out_desc != NULL);
	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	return duk__get_own_propdesc_raw(thr, obj, key, DUK_HSTRING_GET_ARRIDX_SLOW(key), out_desc, flags);
}

/*
 *  ECMAScript compliant [[GetProperty]](P), for internal use only.
 *
 *  If property is found:
 *    - Fills descriptor fields to 'out_desc'
 *    - If DUK_GETDESC_FLAG_PUSH_VALUE is set, pushes a value related to the
 *      property onto the stack ('undefined' for accessor properties).
 *    - Returns non-zero
 *
 *  If property is not found:
 *    - 'out_desc' is left in untouched state (possibly garbage)
 *    - Nothing is pushed onto the stack (not even with DUK_GETDESC_FLAG_PUSH_VALUE
 *      set)
 *    - Returns zero
 *
 *  May cause arbitrary side effects and invalidate (most) duk_tval
 *  pointers.
 */

DUK_LOCAL duk_bool_t
duk__get_propdesc(duk_hthread *thr, duk_hobject *obj, duk_hstring *key, duk_propdesc *out_desc, duk_small_uint_t flags) {
	duk_hobject *curr;
	duk_uint32_t arr_idx;
	duk_uint_t sanity;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(key != NULL);
	DUK_ASSERT(out_desc != NULL);
	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	DUK_STATS_INC(thr->heap, stats_getpropdesc_count);

	arr_idx = DUK_HSTRING_GET_ARRIDX_FAST(key);

	DUK_DDD(DUK_DDDPRINT("duk__get_propdesc: thr=%p, obj=%p, key=%p, out_desc=%p, flags=%lx, "
	                     "arr_idx=%ld (obj -> %!O, key -> %!O)",
	                     (void *) thr,
	                     (void *) obj,
	                     (void *) key,
	                     (void *) out_desc,
	                     (long) flags,
	                     (long) arr_idx,
	                     (duk_heaphdr *) obj,
	                     (duk_heaphdr *) key));

	curr = obj;
	DUK_ASSERT(curr != NULL);
	sanity = DUK_HOBJECT_PROTOTYPE_CHAIN_SANITY;
	do {
		if (duk__get_own_propdesc_raw(thr, curr, key, arr_idx, out_desc, flags)) {
			/* stack contains value (if requested), 'out_desc' is set */
			DUK_STATS_INC(thr->heap, stats_getpropdesc_hit);
			return 1;
		}

		/* not found in 'curr', next in prototype chain; impose max depth */
		if (DUK_UNLIKELY(sanity-- == 0)) {
			if (flags & DUK_GETDESC_FLAG_IGNORE_PROTOLOOP) {
				/* treat like property not found */
				break;
			} else {
				DUK_ERROR_RANGE(thr, DUK_STR_PROTOTYPE_CHAIN_LIMIT);
				DUK_WO_NORETURN(return 0;);
			}
		}
		curr = DUK_HOBJECT_GET_PROTOTYPE(thr->heap, curr);
	} while (curr != NULL);

	/* out_desc is left untouched (possibly garbage), caller must use return
	 * value to determine whether out_desc can be looked up
	 */

	DUK_STATS_INC(thr->heap, stats_getpropdesc_miss);
	return 0;
}

/*
 *  Shallow fast path checks for accessing array elements with numeric
 *  indices.  The goal is to try to avoid coercing an array index to an
 *  (interned) string for the most common lookups, in particular, for
 *  standard Array objects.
 *
 *  Interning is avoided but only for a very narrow set of cases:
 *    - Object has array part, index is within array allocation, and
 *      value is not unused (= key exists)
 *    - Object has no interfering exotic behavior (e.g. arguments or
 *      string object exotic behaviors interfere, array exotic
 *      behavior does not).
 *
 *  Current shortcoming: if key does not exist (even if it is within
 *  the array allocation range) a slow path lookup with interning is
 *  always required.  This can probably be fixed so that there is a
 *  quick fast path for non-existent elements as well, at least for
 *  standard Array objects.
 */

#if defined(DUK_USE_ARRAY_PROP_FASTPATH)
DUK_LOCAL duk_tval *duk__getprop_shallow_fastpath_array_tval(duk_hthread *thr, duk_hobject *obj, duk_tval *tv_key) {
	duk_tval *tv;
	duk_uint32_t idx;

	DUK_UNREF(thr);

	if (!(DUK_HOBJECT_HAS_ARRAY_PART(obj) && !DUK_HOBJECT_HAS_EXOTIC_ARGUMENTS(obj) && !DUK_HOBJECT_HAS_EXOTIC_STRINGOBJ(obj) &&
	      !DUK_HOBJECT_IS_BUFOBJ(obj) && !DUK_HOBJECT_IS_PROXY(obj))) {
		/* Must have array part and no conflicting exotic behaviors.
		 * Doesn't need to have array special behavior, e.g. Arguments
		 * object has array part.
		 */
		return NULL;
	}

	/* Arrays never have other exotic behaviors. */

	DUK_DDD(DUK_DDDPRINT("fast path attempt (no exotic string/arguments/buffer "
	                     "behavior, object has array part)"));

#if defined(DUK_USE_FASTINT)
	if (DUK_TVAL_IS_FASTINT(tv_key)) {
		idx = duk__tval_fastint_to_arr_idx(tv_key);
	} else
#endif
	    if (DUK_TVAL_IS_DOUBLE(tv_key)) {
		idx = duk__tval_number_to_arr_idx(tv_key);
	} else {
		DUK_DDD(DUK_DDDPRINT("key is not a number"));
		return NULL;
	}

	/* If index is not valid, idx will be DUK__NO_ARRAY_INDEX which
	 * is 0xffffffffUL.  We don't need to check for that explicitly
	 * because 0xffffffffUL will never be inside object 'a_size'.
	 */

	if (idx >= DUK_HOBJECT_GET_ASIZE(obj)) {
		DUK_DDD(DUK_DDDPRINT("key is not an array index or outside array part"));
		return NULL;
	}
	DUK_ASSERT(idx != 0xffffffffUL);
	DUK_ASSERT(idx != DUK__NO_ARRAY_INDEX);

	/* XXX: for array instances we could take a shortcut here and assume
	 * Array.prototype doesn't contain an array index property.
	 */

	DUK_DDD(DUK_DDDPRINT("key is a valid array index and inside array part"));
	tv = DUK_HOBJECT_A_GET_VALUE_PTR(thr->heap, obj, idx);
	if (!DUK_TVAL_IS_UNUSED(tv)) {
		DUK_DDD(DUK_DDDPRINT("-> fast path successful"));
		return tv;
	}

	DUK_DDD(DUK_DDDPRINT("fast path attempt failed, fall back to slow path"));
	return NULL;
}

DUK_LOCAL duk_bool_t duk__putprop_shallow_fastpath_array_tval(duk_hthread *thr,
                                                              duk_hobject *obj,
                                                              duk_tval *tv_key,
                                                              duk_tval *tv_val) {
	duk_tval *tv;
	duk_harray *a;
	duk_uint32_t idx;
	duk_uint32_t old_len, new_len;

	if (!(DUK_HOBJECT_HAS_EXOTIC_ARRAY(obj) && DUK_HOBJECT_HAS_ARRAY_PART(obj) && DUK_HOBJECT_HAS_EXTENSIBLE(obj))) {
		return 0;
	}
	DUK_ASSERT(!DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) obj)); /* caller ensures */

	a = (duk_harray *) obj;
	DUK_HARRAY_ASSERT_VALID(a);

#if defined(DUK_USE_FASTINT)
	if (DUK_TVAL_IS_FASTINT(tv_key)) {
		idx = duk__tval_fastint_to_arr_idx(tv_key);
	} else
#endif
	    if (DUK_TVAL_IS_DOUBLE(tv_key)) {
		idx = duk__tval_number_to_arr_idx(tv_key);
	} else {
		DUK_DDD(DUK_DDDPRINT("key is not a number"));
		return 0;
	}

	/* If index is not valid, idx will be DUK__NO_ARRAY_INDEX which
	 * is 0xffffffffUL.  We don't need to check for that explicitly
	 * because 0xffffffffUL will never be inside object 'a_size'.
	 */

	if (idx >= DUK_HOBJECT_GET_ASIZE(obj)) { /* for resizing of array part, use slow path */
		return 0;
	}
	DUK_ASSERT(idx != 0xffffffffUL);
	DUK_ASSERT(idx != DUK__NO_ARRAY_INDEX);

	old_len = a->length;

	if (idx >= old_len) {
		DUK_DDD(DUK_DDDPRINT("write new array entry requires length update "
		                     "(arr_idx=%ld, old_len=%ld)",
		                     (long) idx,
		                     (long) old_len));
		if (DUK_HARRAY_LENGTH_NONWRITABLE(a)) {
			/* The correct behavior here is either a silent error
			 * or a TypeError, depending on strictness.  Fall back
			 * to the slow path to handle the situation.
			 */
			return 0;
		}
		new_len = idx + 1;

		((duk_harray *) obj)->length = new_len;
	}

	tv = DUK_HOBJECT_A_GET_VALUE_PTR(thr->heap, obj, idx);
	DUK_TVAL_SET_TVAL_UPDREF(thr, tv, tv_val); /* side effects */

	DUK_DDD(DUK_DDDPRINT("array fast path success for index %ld", (long) idx));
	return 1;
}
#endif /* DUK_USE_ARRAY_PROP_FASTPATH */

/*
 *  Fast path for bufobj getprop/putprop
 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_LOCAL duk_bool_t duk__getprop_fastpath_bufobj_tval(duk_hthread *thr, duk_hobject *obj, duk_tval *tv_key) {
	duk_uint32_t idx;
	duk_hbufobj *h_bufobj;
	duk_uint_t byte_off;
	duk_small_uint_t elem_size;
	duk_uint8_t *data;

	if (!DUK_HOBJECT_IS_BUFOBJ(obj)) {
		return 0;
	}
	h_bufobj = (duk_hbufobj *) obj;
	if (!DUK_HBUFOBJ_HAS_VIRTUAL_INDICES(h_bufobj)) {
		return 0;
	}

#if defined(DUK_USE_FASTINT)
	if (DUK_TVAL_IS_FASTINT(tv_key)) {
		idx = duk__tval_fastint_to_arr_idx(tv_key);
	} else
#endif
	    if (DUK_TVAL_IS_DOUBLE(tv_key)) {
		idx = duk__tval_number_to_arr_idx(tv_key);
	} else {
		return 0;
	}

	/* If index is not valid, idx will be DUK__NO_ARRAY_INDEX which
	 * is 0xffffffffUL.  We don't need to check for that explicitly
	 * because 0xffffffffUL will never be inside bufobj length.
	 */

	/* Careful with wrapping (left shifting idx would be unsafe). */
	if (idx >= (h_bufobj->length >> h_bufobj->shift)) {
		return 0;
	}
	DUK_ASSERT(idx != DUK__NO_ARRAY_INDEX);

	byte_off = idx << h_bufobj->shift; /* no wrap assuming h_bufobj->length is valid */
	elem_size = (duk_small_uint_t) (1U << h_bufobj->shift);

	if (h_bufobj->buf != NULL && DUK_HBUFOBJ_VALID_BYTEOFFSET_EXCL(h_bufobj, byte_off + elem_size)) {
		data = (duk_uint8_t *) DUK_HBUFFER_GET_DATA_PTR(thr->heap, h_bufobj->buf) + h_bufobj->offset + byte_off;
		duk_hbufobj_push_validated_read(thr, h_bufobj, data, elem_size);
	} else {
		DUK_D(DUK_DPRINT("bufobj access out of underlying buffer, ignoring (read zero)"));
		duk_push_uint(thr, 0);
	}

	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_LOCAL duk_bool_t duk__putprop_fastpath_bufobj_tval(duk_hthread *thr, duk_hobject *obj, duk_tval *tv_key, duk_tval *tv_val) {
	duk_uint32_t idx;
	duk_hbufobj *h_bufobj;
	duk_uint_t byte_off;
	duk_small_uint_t elem_size;
	duk_uint8_t *data;

	if (!(DUK_HOBJECT_IS_BUFOBJ(obj) && DUK_TVAL_IS_NUMBER(tv_val))) {
		return 0;
	}
	DUK_ASSERT(!DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) obj)); /* caller ensures; rom objects are never bufobjs now */

	h_bufobj = (duk_hbufobj *) obj;
	if (!DUK_HBUFOBJ_HAS_VIRTUAL_INDICES(h_bufobj)) {
		return 0;
	}

#if defined(DUK_USE_FASTINT)
	if (DUK_TVAL_IS_FASTINT(tv_key)) {
		idx = duk__tval_fastint_to_arr_idx(tv_key);
	} else
#endif
	    if (DUK_TVAL_IS_DOUBLE(tv_key)) {
		idx = duk__tval_number_to_arr_idx(tv_key);
	} else {
		return 0;
	}

	/* If index is not valid, idx will be DUK__NO_ARRAY_INDEX which
	 * is 0xffffffffUL.  We don't need to check for that explicitly
	 * because 0xffffffffUL will never be inside bufobj length.
	 */

	/* Careful with wrapping (left shifting idx would be unsafe). */
	if (idx >= (h_bufobj->length >> h_bufobj->shift)) {
		return 0;
	}
	DUK_ASSERT(idx != DUK__NO_ARRAY_INDEX);

	byte_off = idx << h_bufobj->shift; /* no wrap assuming h_bufobj->length is valid */
	elem_size = (duk_small_uint_t) (1U << h_bufobj->shift);

	/* Value is required to be a number in the fast path so there
	 * are no side effects in write coercion.
	 */
	duk_push_tval(thr, tv_val);
	DUK_ASSERT(duk_is_number(thr, -1));

	if (h_bufobj->buf != NULL && DUK_HBUFOBJ_VALID_BYTEOFFSET_EXCL(h_bufobj, byte_off + elem_size)) {
		data = (duk_uint8_t *) DUK_HBUFFER_GET_DATA_PTR(thr->heap, h_bufobj->buf) + h_bufobj->offset + byte_off;
		duk_hbufobj_validated_write(thr, h_bufobj, data, elem_size);
	} else {
		DUK_D(DUK_DPRINT("bufobj access out of underlying buffer, ignoring (write skipped)"));
	}

	duk_pop_unsafe(thr);
	return 1;
}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/*
 *  GETPROP: ECMAScript property read.
 */

DUK_INTERNAL duk_bool_t duk_hobject_getprop(duk_hthread *thr, duk_tval *tv_obj, duk_tval *tv_key) {
	duk_tval tv_obj_copy;
	duk_tval tv_key_copy;
	duk_hobject *curr = NULL;
	duk_hstring *key = NULL;
	duk_uint32_t arr_idx = DUK__NO_ARRAY_INDEX;
	duk_propdesc desc;
	duk_uint_t sanity;

	DUK_DDD(DUK_DDDPRINT("getprop: thr=%p, obj=%p, key=%p (obj -> %!T, key -> %!T)",
	                     (void *) thr,
	                     (void *) tv_obj,
	                     (void *) tv_key,
	                     (duk_tval *) tv_obj,
	                     (duk_tval *) tv_key));

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(tv_obj != NULL);
	DUK_ASSERT(tv_key != NULL);

	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	DUK_STATS_INC(thr->heap, stats_getprop_all);

	/*
	 *  Make a copy of tv_obj, tv_key, and tv_val to avoid any issues of
	 *  them being invalidated by a valstack resize.
	 *
	 *  XXX: this is now an overkill for many fast paths.  Rework this
	 *  to be faster (although switching to a valstack discipline might
	 *  be a better solution overall).
	 */

	DUK_TVAL_SET_TVAL(&tv_obj_copy, tv_obj);
	DUK_TVAL_SET_TVAL(&tv_key_copy, tv_key);
	tv_obj = &tv_obj_copy;
	tv_key = &tv_key_copy;

	/*
	 *  Coercion and fast path processing
	 */

	switch (DUK_TVAL_GET_TAG(tv_obj)) {
	case DUK_TAG_UNDEFINED:
	case DUK_TAG_NULL: {
		/* Note: unconditional throw */
		DUK_DDD(DUK_DDDPRINT("base object is undefined or null -> reject"));
#if defined(DUK_USE_PARANOID_ERRORS)
		DUK_ERROR_TYPE(thr, DUK_STR_INVALID_BASE);
#else
		DUK_ERROR_FMT2(thr,
		               DUK_ERR_TYPE_ERROR,
		               "cannot read property %s of %s",
		               duk_push_string_tval_readable(thr, tv_key),
		               duk_push_string_tval_readable(thr, tv_obj));
#endif
		DUK_WO_NORETURN(return 0;);
		break;
	}

	case DUK_TAG_BOOLEAN: {
		DUK_DDD(DUK_DDDPRINT("base object is a boolean, start lookup from boolean prototype"));
		curr = thr->builtins[DUK_BIDX_BOOLEAN_PROTOTYPE];
		break;
	}

	case DUK_TAG_STRING: {
		duk_hstring *h = DUK_TVAL_GET_STRING(tv_obj);
		duk_int_t pop_count;

		if (DUK_UNLIKELY(DUK_HSTRING_HAS_SYMBOL(h))) {
			/* Symbols (ES2015 or hidden) don't have virtual properties. */
			DUK_DDD(DUK_DDDPRINT("base object is a symbol, start lookup from symbol prototype"));
			curr = thr->builtins[DUK_BIDX_SYMBOL_PROTOTYPE];
			break;
		}

#if defined(DUK_USE_FASTINT)
		if (DUK_TVAL_IS_FASTINT(tv_key)) {
			arr_idx = duk__tval_fastint_to_arr_idx(tv_key);
			DUK_DDD(DUK_DDDPRINT("base object string, key is a fast-path fastint; arr_idx %ld", (long) arr_idx));
			pop_count = 0;
		} else
#endif
		    if (DUK_TVAL_IS_NUMBER(tv_key)) {
			arr_idx = duk__tval_number_to_arr_idx(tv_key);
			DUK_DDD(DUK_DDDPRINT("base object string, key is a fast-path number; arr_idx %ld", (long) arr_idx));
			pop_count = 0;
		} else {
			arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);
			DUK_ASSERT(key != NULL);
			DUK_DDD(DUK_DDDPRINT("base object string, key is a non-fast-path number; after "
			                     "coercion key is %!T, arr_idx %ld",
			                     (duk_tval *) duk_get_tval(thr, -1),
			                     (long) arr_idx));
			pop_count = 1;
		}

		if (arr_idx != DUK__NO_ARRAY_INDEX && arr_idx < DUK_HSTRING_GET_CHARLEN(h)) {
			duk_pop_n_unsafe(thr, pop_count);
			duk_push_hstring(thr, h);
			duk_substring(thr, -1, arr_idx, arr_idx + 1); /* [str] -> [substr] */

			DUK_STATS_INC(thr->heap, stats_getprop_stringidx);
			DUK_DDD(DUK_DDDPRINT("-> %!T (base is string, key is an index inside string length "
			                     "after coercion -> return char)",
			                     (duk_tval *) duk_get_tval(thr, -1)));
			return 1;
		}

		if (pop_count == 0) {
			/* This is a pretty awkward control flow, but we need to recheck the
			 * key coercion here.
			 */
			arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);
			DUK_ASSERT(key != NULL);
			DUK_DDD(DUK_DDDPRINT("base object string, key is a non-fast-path number; after "
			                     "coercion key is %!T, arr_idx %ld",
			                     (duk_tval *) duk_get_tval(thr, -1),
			                     (long) arr_idx));
		}

		if (key == DUK_HTHREAD_STRING_LENGTH(thr)) {
			duk_pop_unsafe(thr); /* [key] -> [] */
			duk_push_uint(thr, (duk_uint_t) DUK_HSTRING_GET_CHARLEN(h)); /* [] -> [res] */

			DUK_STATS_INC(thr->heap, stats_getprop_stringlen);
			DUK_DDD(DUK_DDDPRINT("-> %!T (base is string, key is 'length' after coercion -> "
			                     "return string length)",
			                     (duk_tval *) duk_get_tval(thr, -1)));
			return 1;
		}

		DUK_DDD(DUK_DDDPRINT("base object is a string, start lookup from string prototype"));
		curr = thr->builtins[DUK_BIDX_STRING_PROTOTYPE];
		goto lookup; /* avoid double coercion */
	}

	case DUK_TAG_OBJECT: {
#if defined(DUK_USE_ARRAY_PROP_FASTPATH)
		duk_tval *tmp;
#endif

		curr = DUK_TVAL_GET_OBJECT(tv_obj);
		DUK_ASSERT(curr != NULL);

		/* XXX: array .length fast path (important in e.g. loops)? */

#if defined(DUK_USE_ARRAY_PROP_FASTPATH)
		tmp = duk__getprop_shallow_fastpath_array_tval(thr, curr, tv_key);
		if (tmp) {
			duk_push_tval(thr, tmp);

			DUK_DDD(DUK_DDDPRINT("-> %!T (base is object, key is a number, array part "
			                     "fast path)",
			                     (duk_tval *) duk_get_tval(thr, -1)));
			DUK_STATS_INC(thr->heap, stats_getprop_arrayidx);
			return 1;
		}
#endif

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
		if (duk__getprop_fastpath_bufobj_tval(thr, curr, tv_key) != 0) {
			/* Read value pushed on stack. */
			DUK_DDD(DUK_DDDPRINT("-> %!T (base is bufobj, key is a number, bufobj "
			                     "fast path)",
			                     (duk_tval *) duk_get_tval(thr, -1)));
			DUK_STATS_INC(thr->heap, stats_getprop_bufobjidx);
			return 1;
		}
#endif

#if defined(DUK_USE_ES6_PROXY)
		if (DUK_UNLIKELY(DUK_HOBJECT_IS_PROXY(curr))) {
			duk_hobject *h_target;

			if (duk__proxy_check_prop(thr, curr, DUK_STRIDX_GET, tv_key, &h_target)) {
				/* -> [ ... trap handler ] */
				DUK_DDD(DUK_DDDPRINT("-> proxy object 'get' for key %!T", (duk_tval *) tv_key));
				DUK_STATS_INC(thr->heap, stats_getprop_proxy);
				duk_push_hobject(thr, h_target); /* target */
				duk_push_tval(thr, tv_key); /* P */
				duk_push_tval(thr, tv_obj); /* Receiver: Proxy object */
				duk_call_method(thr, 3 /*nargs*/);

				/* Target object must be checked for a conflicting
				 * non-configurable property.
				 */
				arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);
				DUK_ASSERT(key != NULL);

				if (duk__get_own_propdesc_raw(thr, h_target, key, arr_idx, &desc, DUK_GETDESC_FLAG_PUSH_VALUE)) {
					duk_tval *tv_hook = duk_require_tval(thr, -3); /* value from hook */
					duk_tval *tv_targ = duk_require_tval(thr, -1); /* value from target */
					duk_bool_t datadesc_reject;
					duk_bool_t accdesc_reject;

					DUK_DDD(DUK_DDDPRINT("proxy 'get': target has matching property %!O, check for "
					                     "conflicting property; tv_hook=%!T, tv_targ=%!T, desc.flags=0x%08lx, "
					                     "desc.get=%p, desc.set=%p",
					                     (duk_heaphdr *) key,
					                     (duk_tval *) tv_hook,
					                     (duk_tval *) tv_targ,
					                     (unsigned long) desc.flags,
					                     (void *) desc.get,
					                     (void *) desc.set));

					datadesc_reject = !(desc.flags & DUK_PROPDESC_FLAG_ACCESSOR) &&
					                  !(desc.flags & DUK_PROPDESC_FLAG_CONFIGURABLE) &&
					                  !(desc.flags & DUK_PROPDESC_FLAG_WRITABLE) &&
					                  !duk_js_samevalue(tv_hook, tv_targ);
					accdesc_reject = (desc.flags & DUK_PROPDESC_FLAG_ACCESSOR) &&
					                 !(desc.flags & DUK_PROPDESC_FLAG_CONFIGURABLE) && (desc.get == NULL) &&
					                 !DUK_TVAL_IS_UNDEFINED(tv_hook);
					if (datadesc_reject || accdesc_reject) {
						DUK_ERROR_TYPE(thr, DUK_STR_PROXY_REJECTED);
						DUK_WO_NORETURN(return 0;);
					}

					duk_pop_2_unsafe(thr);
				} else {
					duk_pop_unsafe(thr);
				}
				return 1; /* return value */
			}

			curr = h_target; /* resume lookup from target */
			DUK_TVAL_SET_OBJECT(tv_obj, curr);
		}
#endif /* DUK_USE_ES6_PROXY */

		if (DUK_HOBJECT_HAS_EXOTIC_ARGUMENTS(curr)) {
			arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);
			DUK_ASSERT(key != NULL);

			DUK_STATS_INC(thr->heap, stats_getprop_arguments);
			if (duk__check_arguments_map_for_get(thr, curr, key, &desc)) {
				DUK_DDD(DUK_DDDPRINT("-> %!T (base is object with arguments exotic behavior, "
				                     "key matches magically bound property -> skip standard "
				                     "Get with replacement value)",
				                     (duk_tval *) duk_get_tval(thr, -1)));

				/* no need for 'caller' post-check, because 'key' must be an array index */

				duk_remove_m2(thr); /* [key result] -> [result] */
				return 1;
			}

			goto lookup; /* avoid double coercion */
		}
		break;
	}

	/* Buffer has virtual properties similar to string, but indexed values
	 * are numbers, not 1-byte buffers/strings which would perform badly.
	 */
	case DUK_TAG_BUFFER: {
		duk_hbuffer *h = DUK_TVAL_GET_BUFFER(tv_obj);
		duk_int_t pop_count;

		/*
		 *  Because buffer values are often looped over, a number fast path
		 *  is important.
		 */

#if defined(DUK_USE_FASTINT)
		if (DUK_TVAL_IS_FASTINT(tv_key)) {
			arr_idx = duk__tval_fastint_to_arr_idx(tv_key);
			DUK_DDD(DUK_DDDPRINT("base object buffer, key is a fast-path fastint; arr_idx %ld", (long) arr_idx));
			pop_count = 0;
		} else
#endif
		    if (DUK_TVAL_IS_NUMBER(tv_key)) {
			arr_idx = duk__tval_number_to_arr_idx(tv_key);
			DUK_DDD(DUK_DDDPRINT("base object buffer, key is a fast-path number; arr_idx %ld", (long) arr_idx));
			pop_count = 0;
		} else {
			arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);
			DUK_ASSERT(key != NULL);
			DUK_DDD(DUK_DDDPRINT("base object buffer, key is a non-fast-path number; after "
			                     "coercion key is %!T, arr_idx %ld",
			                     (duk_tval *) duk_get_tval(thr, -1),
			                     (long) arr_idx));
			pop_count = 1;
		}

		if (arr_idx != DUK__NO_ARRAY_INDEX && arr_idx < DUK_HBUFFER_GET_SIZE(h)) {
			duk_pop_n_unsafe(thr, pop_count);
			duk_push_uint(thr, ((duk_uint8_t *) DUK_HBUFFER_GET_DATA_PTR(thr->heap, h))[arr_idx]);
			DUK_STATS_INC(thr->heap, stats_getprop_bufferidx);
			DUK_DDD(DUK_DDDPRINT("-> %!T (base is buffer, key is an index inside buffer length "
			                     "after coercion -> return byte as number)",
			                     (duk_tval *) duk_get_tval(thr, -1)));
			return 1;
		}

		if (pop_count == 0) {
			/* This is a pretty awkward control flow, but we need to recheck the
			 * key coercion here.
			 */
			arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);
			DUK_ASSERT(key != NULL);
			DUK_DDD(DUK_DDDPRINT("base object buffer, key is a non-fast-path number; after "
			                     "coercion key is %!T, arr_idx %ld",
			                     (duk_tval *) duk_get_tval(thr, -1),
			                     (long) arr_idx));
		}

		if (key == DUK_HTHREAD_STRING_LENGTH(thr)) {
			duk_pop_unsafe(thr); /* [key] -> [] */
			duk_push_uint(thr, (duk_uint_t) DUK_HBUFFER_GET_SIZE(h)); /* [] -> [res] */
			DUK_STATS_INC(thr->heap, stats_getprop_bufferlen);

			DUK_DDD(DUK_DDDPRINT("-> %!T (base is buffer, key is 'length' "
			                     "after coercion -> return buffer length)",
			                     (duk_tval *) duk_get_tval(thr, -1)));
			return 1;
		}

		DUK_DDD(DUK_DDDPRINT("base object is a buffer, start lookup from Uint8Array prototype"));
		curr = thr->builtins[DUK_BIDX_UINT8ARRAY_PROTOTYPE];
		goto lookup; /* avoid double coercion */
	}

	case DUK_TAG_POINTER: {
		DUK_DDD(DUK_DDDPRINT("base object is a pointer, start lookup from pointer prototype"));
		curr = thr->builtins[DUK_BIDX_POINTER_PROTOTYPE];
		break;
	}

	case DUK_TAG_LIGHTFUNC: {
		/* Lightfuncs inherit getter .name and .length from %NativeFunctionPrototype%. */
		DUK_DDD(DUK_DDDPRINT("base object is a lightfunc, start lookup from function prototype"));
		curr = thr->builtins[DUK_BIDX_NATIVE_FUNCTION_PROTOTYPE];
		break;
	}

#if defined(DUK_USE_FASTINT)
	case DUK_TAG_FASTINT:
#endif
	default: {
		/* number */
		DUK_DDD(DUK_DDDPRINT("base object is a number, start lookup from number prototype"));
		DUK_ASSERT(!DUK_TVAL_IS_UNUSED(tv_obj));
		DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv_obj));
		curr = thr->builtins[DUK_BIDX_NUMBER_PROTOTYPE];
		break;
	}
	}

	/* key coercion (unless already coerced above) */
	DUK_ASSERT(key == NULL);
	arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);
	DUK_ASSERT(key != NULL);
	/*
	 *  Property lookup
	 */

lookup:
	/* [key] (coerced) */
	DUK_ASSERT(curr != NULL);
	DUK_ASSERT(key != NULL);

	sanity = DUK_HOBJECT_PROTOTYPE_CHAIN_SANITY;
	do {
		if (!duk__get_own_propdesc_raw(thr, curr, key, arr_idx, &desc, DUK_GETDESC_FLAG_PUSH_VALUE)) {
			goto next_in_chain;
		}

		if (desc.get != NULL) {
			/* accessor with defined getter */
			DUK_ASSERT((desc.flags & DUK_PROPDESC_FLAG_ACCESSOR) != 0);

			duk_pop_unsafe(thr); /* [key undefined] -> [key] */
			duk_push_hobject(thr, desc.get);
			duk_push_tval(thr, tv_obj); /* note: original, uncoerced base */
#if defined(DUK_USE_NONSTD_GETTER_KEY_ARGUMENT)
			duk_dup_m3(thr);
			duk_call_method(thr, 1); /* [key getter this key] -> [key retval] */
#else
			duk_call_method(thr, 0); /* [key getter this] -> [key retval] */
#endif
		} else {
			/* [key value] or [key undefined] */

			/* data property or accessor without getter */
			DUK_ASSERT(((desc.flags & DUK_PROPDESC_FLAG_ACCESSOR) == 0) || (desc.get == NULL));

			/* if accessor without getter, return value is undefined */
			DUK_ASSERT(((desc.flags & DUK_PROPDESC_FLAG_ACCESSOR) == 0) || duk_is_undefined(thr, -1));

			/* Note: for an accessor without getter, falling through to
			 * check for "caller" exotic behavior is unnecessary as
			 * "undefined" will never activate the behavior.  But it does
			 * no harm, so we'll do it anyway.
			 */
		}

		goto found; /* [key result] */

	next_in_chain:
		/* XXX: option to pretend property doesn't exist if sanity limit is
		 * hit might be useful.
		 */
		if (DUK_UNLIKELY(sanity-- == 0)) {
			DUK_ERROR_RANGE(thr, DUK_STR_PROTOTYPE_CHAIN_LIMIT);
			DUK_WO_NORETURN(return 0;);
		}
		curr = DUK_HOBJECT_GET_PROTOTYPE(thr->heap, curr);
	} while (curr != NULL);

	/*
	 *  Not found
	 */

	duk_to_undefined(thr, -1); /* [key] -> [undefined] (default value) */

	DUK_DDD(DUK_DDDPRINT("-> %!T (not found)", (duk_tval *) duk_get_tval(thr, -1)));
	return 0;

	/*
	 *  Found; post-processing (Function and arguments objects)
	 */

found:
	/* [key result] */

#if !defined(DUK_USE_NONSTD_FUNC_CALLER_PROPERTY)
	/* Special behavior for 'caller' property of (non-bound) function objects
	 * and non-strict Arguments objects: if 'caller' -value- (!) is a strict
	 * mode function, throw a TypeError (E5 Sections 15.3.5.4, 10.6).
	 * Quite interestingly, a non-strict function with no formal arguments
	 * will get an arguments object -without- special 'caller' behavior!
	 *
	 * The E5.1 spec is a bit ambiguous if this special behavior applies when
	 * a bound function is the base value (not the 'caller' value): Section
	 * 15.3.4.5 (describing bind()) states that [[Get]] for bound functions
	 * matches that of Section 15.3.5.4 ([[Get]] for Function instances).
	 * However, Section 13.3.5.4 has "NOTE: Function objects created using
	 * Function.prototype.bind use the default [[Get]] internal method."
	 * The current implementation assumes this means that bound functions
	 * should not have the special [[Get]] behavior.
	 *
	 * The E5.1 spec is also a bit unclear if the TypeError throwing is
	 * applied if the 'caller' value is a strict bound function.  The
	 * current implementation will throw even for both strict non-bound
	 * and strict bound functions.
	 *
	 * See test-dev-strict-func-as-caller-prop-value.js for quite extensive
	 * tests.
	 *
	 * This exotic behavior is disabled when the non-standard 'caller' property
	 * is enabled, as it conflicts with the free use of 'caller'.
	 */
	if (key == DUK_HTHREAD_STRING_CALLER(thr) && DUK_TVAL_IS_OBJECT(tv_obj)) {
		duk_hobject *orig = DUK_TVAL_GET_OBJECT(tv_obj);
		DUK_ASSERT(orig != NULL);

		if (DUK_HOBJECT_IS_NONBOUND_FUNCTION(orig) || DUK_HOBJECT_HAS_EXOTIC_ARGUMENTS(orig)) {
			duk_hobject *h;

			/* XXX: The TypeError is currently not applied to bound
			 * functions because the 'strict' flag is not copied by
			 * bind().  This may or may not be correct, the specification
			 * only refers to the value being a "strict mode Function
			 * object" which is ambiguous.
			 */
			DUK_ASSERT(!DUK_HOBJECT_HAS_BOUNDFUNC(orig));

			h = duk_get_hobject(thr, -1); /* NULL if not an object */
			if (h && DUK_HOBJECT_IS_FUNCTION(h) && DUK_HOBJECT_HAS_STRICT(h)) {
				/* XXX: sufficient to check 'strict', assert for 'is function' */
				DUK_ERROR_TYPE(thr, DUK_STR_STRICT_CALLER_READ);
				DUK_WO_NORETURN(return 0;);
			}
		}
	}
#endif /* !DUK_USE_NONSTD_FUNC_CALLER_PROPERTY */

	duk_remove_m2(thr); /* [key result] -> [result] */

	DUK_DDD(DUK_DDDPRINT("-> %!T (found)", (duk_tval *) duk_get_tval(thr, -1)));
	return 1;
}

/*
 *  HASPROP: ECMAScript property existence check ("in" operator).
 *
 *  Interestingly, the 'in' operator does not do any coercion of
 *  the target object.
 */

DUK_INTERNAL duk_bool_t duk_hobject_hasprop(duk_hthread *thr, duk_tval *tv_obj, duk_tval *tv_key) {
	duk_tval tv_key_copy;
	duk_hobject *obj;
	duk_hstring *key;
	duk_uint32_t arr_idx;
	duk_bool_t rc;
	duk_propdesc desc;

	DUK_DDD(DUK_DDDPRINT("hasprop: thr=%p, obj=%p, key=%p (obj -> %!T, key -> %!T)",
	                     (void *) thr,
	                     (void *) tv_obj,
	                     (void *) tv_key,
	                     (duk_tval *) tv_obj,
	                     (duk_tval *) tv_key));

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(tv_obj != NULL);
	DUK_ASSERT(tv_key != NULL);
	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	DUK_TVAL_SET_TVAL(&tv_key_copy, tv_key);
	tv_key = &tv_key_copy;

	/*
	 *  The 'in' operator requires an object as its right hand side,
	 *  throwing a TypeError unconditionally if this is not the case.
	 *
	 *  However, lightfuncs need to behave like fully fledged objects
	 *  here to be maximally transparent, so we need to handle them
	 *  here.  Same goes for plain buffers which behave like ArrayBuffers.
	 */

	/* XXX: Refactor key coercion so that it's only called once.  It can't
	 * be trivially lifted here because the object must be type checked
	 * first.
	 */

	if (DUK_TVAL_IS_OBJECT(tv_obj)) {
		obj = DUK_TVAL_GET_OBJECT(tv_obj);
		DUK_ASSERT(obj != NULL);

		arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);
	} else if (DUK_TVAL_IS_BUFFER(tv_obj)) {
		arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);
		if (duk__key_is_plain_buf_ownprop(thr, DUK_TVAL_GET_BUFFER(tv_obj), key, arr_idx)) {
			rc = 1;
			goto pop_and_return;
		}
		obj = thr->builtins[DUK_BIDX_UINT8ARRAY_PROTOTYPE];
	} else if (DUK_TVAL_IS_LIGHTFUNC(tv_obj)) {
		arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);

		/* If not found, resume existence check from %NativeFunctionPrototype%.
		 * We can just substitute the value in this case; nothing will
		 * need the original base value (as would be the case with e.g.
		 * setters/getters.
		 */
		obj = thr->builtins[DUK_BIDX_NATIVE_FUNCTION_PROTOTYPE];
	} else {
		/* Note: unconditional throw */
		DUK_DDD(DUK_DDDPRINT("base object is not an object -> reject"));
		DUK_ERROR_TYPE(thr, DUK_STR_INVALID_BASE);
		DUK_WO_NORETURN(return 0;);
	}

	/* XXX: fast path for arrays? */

	DUK_ASSERT(key != NULL);
	DUK_ASSERT(obj != NULL);
	DUK_UNREF(arr_idx);

#if defined(DUK_USE_ES6_PROXY)
	if (DUK_UNLIKELY(DUK_HOBJECT_IS_PROXY(obj))) {
		duk_hobject *h_target;
		duk_bool_t tmp_bool;

		/* XXX: the key in 'key in obj' is string coerced before we're called
		 * (which is the required behavior in E5/E5.1/E6) so the key is a string
		 * here already.
		 */

		if (duk__proxy_check_prop(thr, obj, DUK_STRIDX_HAS, tv_key, &h_target)) {
			/* [ ... key trap handler ] */
			DUK_DDD(DUK_DDDPRINT("-> proxy object 'has' for key %!T", (duk_tval *) tv_key));
			duk_push_hobject(thr, h_target); /* target */
			duk_push_tval(thr, tv_key); /* P */
			duk_call_method(thr, 2 /*nargs*/);
			tmp_bool = duk_to_boolean_top_pop(thr);
			if (!tmp_bool) {
				/* Target object must be checked for a conflicting
				 * non-configurable property.
				 */

				if (duk__get_own_propdesc_raw(thr, h_target, key, arr_idx, &desc, 0 /*flags*/)) { /* don't push
					                                                                             value */
					DUK_DDD(DUK_DDDPRINT("proxy 'has': target has matching property %!O, check for "
					                     "conflicting property; desc.flags=0x%08lx, "
					                     "desc.get=%p, desc.set=%p",
					                     (duk_heaphdr *) key,
					                     (unsigned long) desc.flags,
					                     (void *) desc.get,
					                     (void *) desc.set));
					/* XXX: Extensibility check for target uses IsExtensible().  If we
					 * implemented the isExtensible trap and didn't reject proxies as
					 * proxy targets, it should be respected here.
					 */
					if (!((desc.flags & DUK_PROPDESC_FLAG_CONFIGURABLE) && /* property is configurable and */
					      DUK_HOBJECT_HAS_EXTENSIBLE(h_target))) { /* ... target is extensible */
						DUK_ERROR_TYPE(thr, DUK_STR_PROXY_REJECTED);
						DUK_WO_NORETURN(return 0;);
					}
				}
			}

			duk_pop_unsafe(thr); /* [ key ] -> [] */
			return tmp_bool;
		}

		obj = h_target; /* resume check from proxy target */
	}
#endif /* DUK_USE_ES6_PROXY */

	/* XXX: inline into a prototype walking loop? */

	rc = duk__get_propdesc(thr, obj, key, &desc, 0 /*flags*/); /* don't push value */
	/* fall through */

pop_and_return:
	duk_pop_unsafe(thr); /* [ key ] -> [] */
	return rc;
}

/*
 *  HASPROP variant used internally.
 *
 *  This primitive must never throw an error, callers rely on this.
 *  In particular, don't throw an error for prototype loops; instead,
 *  pretend like the property doesn't exist if a prototype sanity limit
 *  is reached.
 *
 *  Does not implement proxy behavior: if applied to a proxy object,
 *  returns key existence on the proxy object itself.
 */

DUK_INTERNAL duk_bool_t duk_hobject_hasprop_raw(duk_hthread *thr, duk_hobject *obj, duk_hstring *key) {
	duk_propdesc dummy;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(key != NULL);

	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	return duk__get_propdesc(thr, obj, key, &dummy, DUK_GETDESC_FLAG_IGNORE_PROTOLOOP); /* don't push value */
}

/*
 *  Helper: handle Array object 'length' write which automatically
 *  deletes properties, see E5 Section 15.4.5.1, step 3.  This is
 *  quite tricky to get right.
 *
 *  Used by duk_hobject_putprop().
 */

/* Coerce a new .length candidate to a number and check that it's a valid
 * .length.
 */
DUK_LOCAL duk_uint32_t duk__to_new_array_length_checked(duk_hthread *thr, duk_tval *tv) {
	duk_uint32_t res;
	duk_double_t d;

#if !defined(DUK_USE_PREFER_SIZE)
#if defined(DUK_USE_FASTINT)
	/* When fastints are enabled, the most interesting case is assigning
	 * a fastint to .length (e.g. arr.length = 0).
	 */
	if (DUK_TVAL_IS_FASTINT(tv)) {
		/* Very common case. */
		duk_int64_t fi;
		fi = DUK_TVAL_GET_FASTINT(tv);
		if (fi < 0 || fi > DUK_I64_CONSTANT(0xffffffff)) {
			goto fail_range;
		}
		return (duk_uint32_t) fi;
	}
#else /* DUK_USE_FASTINT */
	/* When fastints are not enabled, the most interesting case is any
	 * number.
	 */
	if (DUK_TVAL_IS_DOUBLE(tv)) {
		d = DUK_TVAL_GET_NUMBER(tv);
	}
#endif /* DUK_USE_FASTINT */
	else
#endif /* !DUK_USE_PREFER_SIZE */
	{
		/* In all other cases, and when doing a size optimized build,
		 * fall back to the comprehensive handler.
		 */
		d = duk_js_tonumber(thr, tv);
	}

	/* Refuse to update an Array's 'length' to a value outside the
	 * 32-bit range.  Negative zero is accepted as zero.
	 */
	res = duk_double_to_uint32_t(d);
	if (!duk_double_equals((duk_double_t) res, d)) {
		goto fail_range;
	}

	return res;

fail_range:
	DUK_ERROR_RANGE(thr, DUK_STR_INVALID_ARRAY_LENGTH);
	DUK_WO_NORETURN(return 0;);
}

/* Delete elements required by a smaller length, taking into account
 * potentially non-configurable elements.  Returns non-zero if all
 * elements could be deleted, and zero if all or some elements could
 * not be deleted.  Also writes final "target length" to 'out_result_len'.
 * This is the length value that should go into the 'length' property
 * (must be set by the caller).  Never throws an error.
 */
DUK_LOCAL
duk_bool_t duk__handle_put_array_length_smaller(duk_hthread *thr,
                                                duk_hobject *obj,
                                                duk_uint32_t old_len,
                                                duk_uint32_t new_len,
                                                duk_bool_t force_flag,
                                                duk_uint32_t *out_result_len) {
	duk_uint32_t target_len;
	duk_uint_fast32_t i;
	duk_uint32_t arr_idx;
	duk_hstring *key;
	duk_tval *tv;
	duk_bool_t rc;

	DUK_DDD(DUK_DDDPRINT("new array length smaller than old (%ld -> %ld), "
	                     "probably need to remove elements",
	                     (long) old_len,
	                     (long) new_len));

	/*
	 *  New length is smaller than old length, need to delete properties above
	 *  the new length.
	 *
	 *  If array part exists, this is straightforward: array entries cannot
	 *  be non-configurable so this is guaranteed to work.
	 *
	 *  If array part does not exist, array-indexed values are scattered
	 *  in the entry part, and some may not be configurable (preventing length
	 *  from becoming lower than their index + 1).  To handle the algorithm
	 *  in E5 Section 15.4.5.1, step l correctly, we scan the entire property
	 *  set twice.
	 */

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(new_len < old_len);
	DUK_ASSERT(out_result_len != NULL);
	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	DUK_ASSERT(DUK_HOBJECT_HAS_EXOTIC_ARRAY(obj));
	DUK_ASSERT(DUK_HOBJECT_IS_ARRAY(obj));

	if (DUK_HOBJECT_HAS_ARRAY_PART(obj)) {
		/*
		 *  All defined array-indexed properties are in the array part
		 *  (we assume the array part is comprehensive), and all array
		 *  entries are writable, configurable, and enumerable.  Thus,
		 *  nothing can prevent array entries from being deleted.
		 */

		DUK_DDD(DUK_DDDPRINT("have array part, easy case"));

		if (old_len < DUK_HOBJECT_GET_ASIZE(obj)) {
			/* XXX: assertion that entries >= old_len are already unused */
			i = old_len;
		} else {
			i = DUK_HOBJECT_GET_ASIZE(obj);
		}
		DUK_ASSERT(i <= DUK_HOBJECT_GET_ASIZE(obj));

		while (i > new_len) {
			i--;
			tv = DUK_HOBJECT_A_GET_VALUE_PTR(thr->heap, obj, i);
			DUK_TVAL_SET_UNUSED_UPDREF(thr, tv); /* side effects */
		}

		*out_result_len = new_len;
		return 1;
	} else {
		/*
		 *  Entries part is a bit more complex.
		 */

		/* Stage 1: find highest preventing non-configurable entry (if any).
		 * When forcing, ignore non-configurability.
		 */

		DUK_DDD(DUK_DDDPRINT("no array part, slow case"));

		DUK_DDD(DUK_DDDPRINT("array length write, no array part, stage 1: find target_len "
		                     "(highest preventing non-configurable entry (if any))"));

		target_len = new_len;
		if (force_flag) {
			DUK_DDD(DUK_DDDPRINT("array length write, no array part; force flag -> skip stage 1"));
			goto skip_stage1;
		}
		for (i = 0; i < DUK_HOBJECT_GET_ENEXT(obj); i++) {
			key = DUK_HOBJECT_E_GET_KEY(thr->heap, obj, i);
			if (!key) {
				DUK_DDD(DUK_DDDPRINT("skip entry index %ld: null key", (long) i));
				continue;
			}
			if (!DUK_HSTRING_HAS_ARRIDX(key)) {
				DUK_DDD(DUK_DDDPRINT("skip entry index %ld: key not an array index", (long) i));
				continue;
			}

			DUK_ASSERT(
			    DUK_HSTRING_HAS_ARRIDX(key)); /* XXX: macro checks for array index flag, which is unnecessary here */
			arr_idx = DUK_HSTRING_GET_ARRIDX_SLOW(key);
			DUK_ASSERT(arr_idx != DUK__NO_ARRAY_INDEX);
			DUK_ASSERT(arr_idx < old_len); /* consistency requires this */

			if (arr_idx < new_len) {
				DUK_DDD(DUK_DDDPRINT("skip entry index %ld: key is array index %ld, below new_len",
				                     (long) i,
				                     (long) arr_idx));
				continue;
			}
			if (DUK_HOBJECT_E_SLOT_IS_CONFIGURABLE(thr->heap, obj, i)) {
				DUK_DDD(DUK_DDDPRINT("skip entry index %ld: key is a relevant array index %ld, but configurable",
				                     (long) i,
				                     (long) arr_idx));
				continue;
			}

			/* relevant array index is non-configurable, blocks write */
			if (arr_idx >= target_len) {
				DUK_DDD(DUK_DDDPRINT("entry at index %ld has arr_idx %ld, is not configurable, "
				                     "update target_len %ld -> %ld",
				                     (long) i,
				                     (long) arr_idx,
				                     (long) target_len,
				                     (long) (arr_idx + 1)));
				target_len = arr_idx + 1;
			}
		}
	skip_stage1:

		/* stage 2: delete configurable entries above target length */

		DUK_DDD(
		    DUK_DDDPRINT("old_len=%ld, new_len=%ld, target_len=%ld", (long) old_len, (long) new_len, (long) target_len));

		DUK_DDD(DUK_DDDPRINT("array length write, no array part, stage 2: remove "
		                     "entries >= target_len"));

		for (i = 0; i < DUK_HOBJECT_GET_ENEXT(obj); i++) {
			key = DUK_HOBJECT_E_GET_KEY(thr->heap, obj, i);
			if (!key) {
				DUK_DDD(DUK_DDDPRINT("skip entry index %ld: null key", (long) i));
				continue;
			}
			if (!DUK_HSTRING_HAS_ARRIDX(key)) {
				DUK_DDD(DUK_DDDPRINT("skip entry index %ld: key not an array index", (long) i));
				continue;
			}

			DUK_ASSERT(
			    DUK_HSTRING_HAS_ARRIDX(key)); /* XXX: macro checks for array index flag, which is unnecessary here */
			arr_idx = DUK_HSTRING_GET_ARRIDX_SLOW(key);
			DUK_ASSERT(arr_idx != DUK__NO_ARRAY_INDEX);
			DUK_ASSERT(arr_idx < old_len); /* consistency requires this */

			if (arr_idx < target_len) {
				DUK_DDD(DUK_DDDPRINT("skip entry index %ld: key is array index %ld, below target_len",
				                     (long) i,
				                     (long) arr_idx));
				continue;
			}
			DUK_ASSERT(force_flag || DUK_HOBJECT_E_SLOT_IS_CONFIGURABLE(thr->heap, obj, i)); /* stage 1 guarantees */

			DUK_DDD(DUK_DDDPRINT("delete entry index %ld: key is array index %ld", (long) i, (long) arr_idx));

			/*
			 *  Slow delete, but we don't care as we're already in a very slow path.
			 *  The delete always succeeds: key has no exotic behavior, property
			 *  is configurable, and no resize occurs.
			 */
			rc = duk_hobject_delprop_raw(thr, obj, key, force_flag ? DUK_DELPROP_FLAG_FORCE : 0);
			DUK_UNREF(rc);
			DUK_ASSERT(rc != 0);
		}

		/* stage 3: update length (done by caller), decide return code */

		DUK_DDD(DUK_DDDPRINT("array length write, no array part, stage 3: update length (done by caller)"));

		*out_result_len = target_len;

		if (target_len == new_len) {
			DUK_DDD(DUK_DDDPRINT("target_len matches new_len, return success"));
			return 1;
		}
		DUK_DDD(DUK_DDDPRINT("target_len does not match new_len (some entry prevented "
		                     "full length adjustment), return error"));
		return 0;
	}

	DUK_UNREACHABLE();
}

/* XXX: is valstack top best place for argument? */
DUK_LOCAL duk_bool_t duk__handle_put_array_length(duk_hthread *thr, duk_hobject *obj) {
	duk_harray *a;
	duk_uint32_t old_len;
	duk_uint32_t new_len;
	duk_uint32_t result_len;
	duk_bool_t rc;

	DUK_DDD(DUK_DDDPRINT("handling a put operation to array 'length' exotic property, "
	                     "new val: %!T",
	                     (duk_tval *) duk_get_tval(thr, -1)));

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(obj != NULL);

	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	DUK_ASSERT(DUK_HOBJECT_HAS_EXOTIC_ARRAY(obj));
	DUK_ASSERT(DUK_HOBJECT_IS_ARRAY(obj));
	a = (duk_harray *) obj;
	DUK_HARRAY_ASSERT_VALID(a);

	DUK_ASSERT(duk_is_valid_index(thr, -1));

	/*
	 *  Get old and new length
	 */

	old_len = a->length;
	new_len = duk__to_new_array_length_checked(thr, DUK_GET_TVAL_NEGIDX(thr, -1));
	DUK_DDD(DUK_DDDPRINT("old_len=%ld, new_len=%ld", (long) old_len, (long) new_len));

	/*
	 *  Writability check
	 */

	if (DUK_HARRAY_LENGTH_NONWRITABLE(a)) {
		DUK_DDD(DUK_DDDPRINT("length is not writable, fail"));
		return 0;
	}

	/*
	 *  New length not lower than old length => no changes needed
	 *  (not even array allocation).
	 */

	if (new_len >= old_len) {
		DUK_DDD(DUK_DDDPRINT("new length is same or higher than old length, just update length, no deletions"));
		a->length = new_len;
		return 1;
	}

	DUK_DDD(DUK_DDDPRINT("new length is lower than old length, probably must delete entries"));

	/*
	 *  New length lower than old length => delete elements, then
	 *  update length.
	 *
	 *  Note: even though a bunch of elements have been deleted, the 'desc' is
	 *  still valid as properties haven't been resized (and entries compacted).
	 */

	rc = duk__handle_put_array_length_smaller(thr, obj, old_len, new_len, 0 /*force_flag*/, &result_len);
	DUK_ASSERT(result_len >= new_len && result_len <= old_len);

	a->length = result_len;

	/* XXX: shrink array allocation or entries compaction here? */

	return rc;
}

/*
 *  PUTPROP: ECMAScript property write.
 *
 *  Unlike ECMAScript primitive which returns nothing, returns 1 to indicate
 *  success and 0 to indicate failure (assuming throw is not set).
 *
 *  This is an extremely tricky function.  Some examples:
 *
 *    * Currently a decref may trigger a GC, which may compact an object's
 *      property allocation.  Consequently, any entry indices (e_idx) will
 *      be potentially invalidated by a decref.
 *
 *    * Exotic behaviors (strings, arrays, arguments object) require,
 *      among other things:
 *
 *      - Preprocessing before and postprocessing after an actual property
 *        write.  For example, array index write requires pre-checking the
 *        array 'length' property for access control, and may require an
 *        array 'length' update after the actual write has succeeded (but
 *        not if it fails).
 *
 *      - Deletion of multiple entries, as a result of array 'length' write.
 *
 *    * Input values are taken as pointers which may point to the valstack.
 *      If valstack is resized because of the put (this may happen at least
 *      when the array part is abandoned), the pointers can be invalidated.
 *      (We currently make a copy of all of the input values to avoid issues.)
 */

DUK_INTERNAL duk_bool_t
duk_hobject_putprop(duk_hthread *thr, duk_tval *tv_obj, duk_tval *tv_key, duk_tval *tv_val, duk_bool_t throw_flag) {
	duk_tval tv_obj_copy;
	duk_tval tv_key_copy;
	duk_tval tv_val_copy;
	duk_hobject *orig = NULL; /* NULL if tv_obj is primitive */
	duk_hobject *curr;
	duk_hstring *key = NULL;
	duk_propdesc desc;
	duk_tval *tv;
	duk_uint32_t arr_idx;
	duk_bool_t rc;
	duk_int_t e_idx;
	duk_uint_t sanity;
	duk_uint32_t new_array_length = 0; /* 0 = no update */

	DUK_DDD(DUK_DDDPRINT("putprop: thr=%p, obj=%p, key=%p, val=%p, throw=%ld "
	                     "(obj -> %!T, key -> %!T, val -> %!T)",
	                     (void *) thr,
	                     (void *) tv_obj,
	                     (void *) tv_key,
	                     (void *) tv_val,
	                     (long) throw_flag,
	                     (duk_tval *) tv_obj,
	                     (duk_tval *) tv_key,
	                     (duk_tval *) tv_val));

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(tv_obj != NULL);
	DUK_ASSERT(tv_key != NULL);
	DUK_ASSERT(tv_val != NULL);

	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	DUK_STATS_INC(thr->heap, stats_putprop_all);

	/*
	 *  Make a copy of tv_obj, tv_key, and tv_val to avoid any issues of
	 *  them being invalidated by a valstack resize.
	 *
	 *  XXX: this is an overkill for some paths, so optimize this later
	 *  (or maybe switch to a stack arguments model entirely).
	 */

	DUK_TVAL_SET_TVAL(&tv_obj_copy, tv_obj);
	DUK_TVAL_SET_TVAL(&tv_key_copy, tv_key);
	DUK_TVAL_SET_TVAL(&tv_val_copy, tv_val);
	tv_obj = &tv_obj_copy;
	tv_key = &tv_key_copy;
	tv_val = &tv_val_copy;

	/*
	 *  Coercion and fast path processing.
	 */

	switch (DUK_TVAL_GET_TAG(tv_obj)) {
	case DUK_TAG_UNDEFINED:
	case DUK_TAG_NULL: {
		/* Note: unconditional throw */
		DUK_DDD(DUK_DDDPRINT("base object is undefined or null -> reject (object=%!iT)", (duk_tval *) tv_obj));
#if defined(DUK_USE_PARANOID_ERRORS)
		DUK_ERROR_TYPE(thr, DUK_STR_INVALID_BASE);
#else
		DUK_ERROR_FMT2(thr,
		               DUK_ERR_TYPE_ERROR,
		               "cannot write property %s of %s",
		               duk_push_string_tval_readable(thr, tv_key),
		               duk_push_string_tval_readable(thr, tv_obj));
#endif
		DUK_WO_NORETURN(return 0;);
		break;
	}

	case DUK_TAG_BOOLEAN: {
		DUK_DDD(DUK_DDDPRINT("base object is a boolean, start lookup from boolean prototype"));
		curr = thr->builtins[DUK_BIDX_BOOLEAN_PROTOTYPE];
		break;
	}

	case DUK_TAG_STRING: {
		duk_hstring *h = DUK_TVAL_GET_STRING(tv_obj);

		/*
		 *  Note: currently no fast path for array index writes.
		 *  They won't be possible anyway as strings are immutable.
		 */

		DUK_ASSERT(key == NULL);
		arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);
		DUK_ASSERT(key != NULL);

		if (DUK_UNLIKELY(DUK_HSTRING_HAS_SYMBOL(h))) {
			/* Symbols (ES2015 or hidden) don't have virtual properties. */
			curr = thr->builtins[DUK_BIDX_SYMBOL_PROTOTYPE];
			goto lookup;
		}

		if (key == DUK_HTHREAD_STRING_LENGTH(thr)) {
			goto fail_not_writable;
		}

		if (arr_idx != DUK__NO_ARRAY_INDEX && arr_idx < DUK_HSTRING_GET_CHARLEN(h)) {
			goto fail_not_writable;
		}

		DUK_DDD(DUK_DDDPRINT("base object is a string, start lookup from string prototype"));
		curr = thr->builtins[DUK_BIDX_STRING_PROTOTYPE];
		goto lookup; /* avoid double coercion */
	}

	case DUK_TAG_OBJECT: {
		orig = DUK_TVAL_GET_OBJECT(tv_obj);
		DUK_ASSERT(orig != NULL);

#if defined(DUK_USE_ROM_OBJECTS)
		/* With this check in place fast paths won't need read-only
		 * object checks.  This is technically incorrect if there are
		 * setters that cause no writes to ROM objects, but current
		 * built-ins don't have such setters.
		 */
		if (DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) orig)) {
			DUK_DD(DUK_DDPRINT("attempt to putprop on read-only target object"));
			goto fail_not_writable_no_pop; /* Must avoid duk_pop() in exit path */
		}
#endif

		/* The fast path for array property put is not fully compliant:
		 * If one places conflicting number-indexed properties into
		 * Array.prototype (for example, a non-writable Array.prototype[7])
		 * the fast path will incorrectly ignore them.
		 *
		 * This fast path could be made compliant by falling through
		 * to the slow path if the previous value was UNUSED.  This would
		 * also remove the need to check for extensibility.  Right now a
		 * non-extensible array is slower than an extensible one as far
		 * as writes are concerned.
		 *
		 * The fast path behavior is documented in more detail here:
		 * tests/ecmascript/test-misc-array-fast-write.js
		 */

		/* XXX: array .length? */

#if defined(DUK_USE_ARRAY_PROP_FASTPATH)
		if (duk__putprop_shallow_fastpath_array_tval(thr, orig, tv_key, tv_val) != 0) {
			DUK_DDD(DUK_DDDPRINT("array fast path success"));
			DUK_STATS_INC(thr->heap, stats_putprop_arrayidx);
			return 1;
		}
#endif

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
		if (duk__putprop_fastpath_bufobj_tval(thr, orig, tv_key, tv_val) != 0) {
			DUK_DDD(DUK_DDDPRINT("base is bufobj, key is a number, bufobj fast path"));
			DUK_STATS_INC(thr->heap, stats_putprop_bufobjidx);
			return 1;
		}
#endif

#if defined(DUK_USE_ES6_PROXY)
		if (DUK_UNLIKELY(DUK_HOBJECT_IS_PROXY(orig))) {
			duk_hobject *h_target;
			duk_bool_t tmp_bool;

			if (duk__proxy_check_prop(thr, orig, DUK_STRIDX_SET, tv_key, &h_target)) {
				/* -> [ ... trap handler ] */
				DUK_DDD(DUK_DDDPRINT("-> proxy object 'set' for key %!T", (duk_tval *) tv_key));
				DUK_STATS_INC(thr->heap, stats_putprop_proxy);
				duk_push_hobject(thr, h_target); /* target */
				duk_push_tval(thr, tv_key); /* P */
				duk_push_tval(thr, tv_val); /* V */
				duk_push_tval(thr, tv_obj); /* Receiver: Proxy object */
				duk_call_method(thr, 4 /*nargs*/);
				tmp_bool = duk_to_boolean_top_pop(thr);
				if (!tmp_bool) {
					goto fail_proxy_rejected;
				}

				/* Target object must be checked for a conflicting
				 * non-configurable property.
				 */
				arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);
				DUK_ASSERT(key != NULL);

				if (duk__get_own_propdesc_raw(thr, h_target, key, arr_idx, &desc, DUK_GETDESC_FLAG_PUSH_VALUE)) {
					duk_tval *tv_targ = duk_require_tval(thr, -1);
					duk_bool_t datadesc_reject;
					duk_bool_t accdesc_reject;

					DUK_DDD(DUK_DDDPRINT("proxy 'set': target has matching property %!O, check for "
					                     "conflicting property; tv_val=%!T, tv_targ=%!T, desc.flags=0x%08lx, "
					                     "desc.get=%p, desc.set=%p",
					                     (duk_heaphdr *) key,
					                     (duk_tval *) tv_val,
					                     (duk_tval *) tv_targ,
					                     (unsigned long) desc.flags,
					                     (void *) desc.get,
					                     (void *) desc.set));

					datadesc_reject = !(desc.flags & DUK_PROPDESC_FLAG_ACCESSOR) &&
					                  !(desc.flags & DUK_PROPDESC_FLAG_CONFIGURABLE) &&
					                  !(desc.flags & DUK_PROPDESC_FLAG_WRITABLE) &&
					                  !duk_js_samevalue(tv_val, tv_targ);
					accdesc_reject = (desc.flags & DUK_PROPDESC_FLAG_ACCESSOR) &&
					                 !(desc.flags & DUK_PROPDESC_FLAG_CONFIGURABLE) && (desc.set == NULL);
					if (datadesc_reject || accdesc_reject) {
						DUK_ERROR_TYPE(thr, DUK_STR_PROXY_REJECTED);
						DUK_WO_NORETURN(return 0;);
					}

					duk_pop_2_unsafe(thr);
				} else {
					duk_pop_unsafe(thr);
				}
				return 1; /* success */
			}

			orig = h_target; /* resume write to target */
			DUK_TVAL_SET_OBJECT(tv_obj, orig);
		}
#endif /* DUK_USE_ES6_PROXY */

		curr = orig;
		break;
	}

	case DUK_TAG_BUFFER: {
		duk_hbuffer *h = DUK_TVAL_GET_BUFFER(tv_obj);
		duk_int_t pop_count = 0;

		/*
		 *  Because buffer values may be looped over and read/written
		 *  from, an array index fast path is important.
		 */

#if defined(DUK_USE_FASTINT)
		if (DUK_TVAL_IS_FASTINT(tv_key)) {
			arr_idx = duk__tval_fastint_to_arr_idx(tv_key);
			DUK_DDD(DUK_DDDPRINT("base object buffer, key is a fast-path fastint; arr_idx %ld", (long) arr_idx));
			pop_count = 0;
		} else
#endif
		    if (DUK_TVAL_IS_NUMBER(tv_key)) {
			arr_idx = duk__tval_number_to_arr_idx(tv_key);
			DUK_DDD(DUK_DDDPRINT("base object buffer, key is a fast-path number; arr_idx %ld", (long) arr_idx));
			pop_count = 0;
		} else {
			arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);
			DUK_ASSERT(key != NULL);
			DUK_DDD(DUK_DDDPRINT("base object buffer, key is a non-fast-path number; after "
			                     "coercion key is %!T, arr_idx %ld",
			                     (duk_tval *) duk_get_tval(thr, -1),
			                     (long) arr_idx));
			pop_count = 1;
		}

		if (arr_idx != DUK__NO_ARRAY_INDEX && arr_idx < DUK_HBUFFER_GET_SIZE(h)) {
			duk_uint8_t *data;
			DUK_DDD(DUK_DDDPRINT("writing to buffer data at index %ld", (long) arr_idx));
			data = (duk_uint8_t *) DUK_HBUFFER_GET_DATA_PTR(thr->heap, h);

			/* XXX: duk_to_int() ensures we'll get 8 lowest bits as
			 * as input is within duk_int_t range (capped outside it).
			 */
#if defined(DUK_USE_FASTINT)
			/* Buffer writes are often integers. */
			if (DUK_TVAL_IS_FASTINT(tv_val)) {
				data[arr_idx] = (duk_uint8_t) DUK_TVAL_GET_FASTINT_U32(tv_val);
			} else
#endif
			{
				duk_push_tval(thr, tv_val);
				data[arr_idx] = (duk_uint8_t) duk_to_uint32(thr, -1);
				pop_count++;
			}

			duk_pop_n_unsafe(thr, pop_count);
			DUK_DDD(DUK_DDDPRINT("result: success (buffer data write)"));
			DUK_STATS_INC(thr->heap, stats_putprop_bufferidx);
			return 1;
		}

		if (pop_count == 0) {
			/* This is a pretty awkward control flow, but we need to recheck the
			 * key coercion here.
			 */
			arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);
			DUK_ASSERT(key != NULL);
			DUK_DDD(DUK_DDDPRINT("base object buffer, key is a non-fast-path number; after "
			                     "coercion key is %!T, arr_idx %ld",
			                     (duk_tval *) duk_get_tval(thr, -1),
			                     (long) arr_idx));
		}

		if (key == DUK_HTHREAD_STRING_LENGTH(thr)) {
			goto fail_not_writable;
		}

		DUK_DDD(DUK_DDDPRINT("base object is a buffer, start lookup from Uint8Array prototype"));
		curr = thr->builtins[DUK_BIDX_UINT8ARRAY_PROTOTYPE];
		goto lookup; /* avoid double coercion */
	}

	case DUK_TAG_POINTER: {
		DUK_DDD(DUK_DDDPRINT("base object is a pointer, start lookup from pointer prototype"));
		curr = thr->builtins[DUK_BIDX_POINTER_PROTOTYPE];
		break;
	}

	case DUK_TAG_LIGHTFUNC: {
		/* Lightfuncs have no own properties and are considered non-extensible.
		 * However, the write may be captured by an inherited setter which
		 * means we can't stop the lookup here.
		 */
		DUK_DDD(DUK_DDDPRINT("base object is a lightfunc, start lookup from function prototype"));
		curr = thr->builtins[DUK_BIDX_NATIVE_FUNCTION_PROTOTYPE];
		break;
	}

#if defined(DUK_USE_FASTINT)
	case DUK_TAG_FASTINT:
#endif
	default: {
		/* number */
		DUK_DDD(DUK_DDDPRINT("base object is a number, start lookup from number prototype"));
		DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv_obj));
		curr = thr->builtins[DUK_BIDX_NUMBER_PROTOTYPE];
		break;
	}
	}

	DUK_ASSERT(key == NULL);
	arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);
	DUK_ASSERT(key != NULL);

lookup:

	/*
	 *  Check whether the property already exists in the prototype chain.
	 *  Note that the actual write goes into the original base object
	 *  (except if an accessor property captures the write).
	 */

	/* [key] */

	DUK_ASSERT(curr != NULL);
	sanity = DUK_HOBJECT_PROTOTYPE_CHAIN_SANITY;
	do {
		if (!duk__get_own_propdesc_raw(thr, curr, key, arr_idx, &desc, 0 /*flags*/)) { /* don't push value */
			goto next_in_chain;
		}

		if (desc.flags & DUK_PROPDESC_FLAG_ACCESSOR) {
			/*
			 *  Found existing accessor property (own or inherited).
			 *  Call setter with 'this' set to orig, and value as the only argument.
			 *  Setter calls are OK even for ROM objects.
			 *
			 *  Note: no exotic arguments object behavior, because [[Put]] never
			 *  calls [[DefineOwnProperty]] (E5 Section 8.12.5, step 5.b).
			 */

			duk_hobject *setter;

			DUK_DD(DUK_DDPRINT("put to an own or inherited accessor, calling setter"));

			setter = DUK_HOBJECT_E_GET_VALUE_SETTER(thr->heap, curr, desc.e_idx);
			if (!setter) {
				goto fail_no_setter;
			}
			duk_push_hobject(thr, setter);
			duk_push_tval(thr, tv_obj); /* note: original, uncoerced base */
			duk_push_tval(thr, tv_val); /* [key setter this val] */
#if defined(DUK_USE_NONSTD_SETTER_KEY_ARGUMENT)
			duk_dup_m4(thr);
			duk_call_method(thr, 2); /* [key setter this val key] -> [key retval] */
#else
			duk_call_method(thr, 1); /* [key setter this val] -> [key retval] */
#endif
			duk_pop_unsafe(thr); /* ignore retval -> [key] */
			goto success_no_arguments_exotic;
		}

		if (orig == NULL) {
			/*
			 *  Found existing own or inherited plain property, but original
			 *  base is a primitive value.
			 */
			DUK_DD(DUK_DDPRINT("attempt to create a new property in a primitive base object"));
			goto fail_base_primitive;
		}

		if (curr != orig) {
			/*
			 *  Found existing inherited plain property.
			 *  Do an access control check, and if OK, write
			 *  new property to 'orig'.
			 */
			if (!DUK_HOBJECT_HAS_EXTENSIBLE(orig)) {
				DUK_DD(
				    DUK_DDPRINT("found existing inherited plain property, but original object is not extensible"));
				goto fail_not_extensible;
			}
			if (!(desc.flags & DUK_PROPDESC_FLAG_WRITABLE)) {
				DUK_DD(DUK_DDPRINT("found existing inherited plain property, original object is extensible, but "
				                   "inherited property is not writable"));
				goto fail_not_writable;
			}
			DUK_DD(DUK_DDPRINT("put to new property, object extensible, inherited property found and is writable"));
			goto create_new;
		} else {
			/*
			 *  Found existing own (non-inherited) plain property.
			 *  Do an access control check and update in place.
			 */

			if (!(desc.flags & DUK_PROPDESC_FLAG_WRITABLE)) {
				DUK_DD(
				    DUK_DDPRINT("found existing own (non-inherited) plain property, but property is not writable"));
				goto fail_not_writable;
			}
			if (desc.flags & DUK_PROPDESC_FLAG_VIRTUAL) {
				DUK_DD(DUK_DDPRINT("found existing own (non-inherited) virtual property, property is writable"));

				if (DUK_HOBJECT_IS_ARRAY(curr)) {
					/*
					 *  Write to 'length' of an array is a very complex case
					 *  handled in a helper which updates both the array elements
					 *  and writes the new 'length'.  The write may result in an
					 *  unconditional RangeError or a partial write (indicated
					 *  by a return code).
					 *
					 *  Note: the helper has an unnecessary writability check
					 *  for 'length', we already know it is writable.
					 */
					DUK_ASSERT(key == DUK_HTHREAD_STRING_LENGTH(thr)); /* only virtual array property */

					DUK_DDD(DUK_DDDPRINT(
					    "writing existing 'length' property to array exotic, invoke complex helper"));

					/* XXX: the helper currently assumes stack top contains new
					 * 'length' value and the whole calling convention is not very
					 * compatible with what we need.
					 */

					duk_push_tval(thr, tv_val); /* [key val] */
					rc = duk__handle_put_array_length(thr, orig);
					duk_pop_unsafe(thr); /* [key val] -> [key] */
					if (!rc) {
						goto fail_array_length_partial;
					}

					/* key is 'length', cannot match argument exotic behavior */
					goto success_no_arguments_exotic;
				}
#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
				else if (DUK_HOBJECT_IS_BUFOBJ(curr)) {
					duk_hbufobj *h_bufobj;
					duk_uint_t byte_off;
					duk_small_uint_t elem_size;

					h_bufobj = (duk_hbufobj *) curr;
					DUK_HBUFOBJ_ASSERT_VALID(h_bufobj);

					DUK_DD(DUK_DDPRINT("writable virtual property is in buffer object"));

					/* Careful with wrapping: arr_idx upshift may easily wrap, whereas
					 * length downshift won't.
					 */
					if (arr_idx < (h_bufobj->length >> h_bufobj->shift) &&
					    DUK_HBUFOBJ_HAS_VIRTUAL_INDICES(h_bufobj)) {
						duk_uint8_t *data;
						DUK_DDD(DUK_DDDPRINT("writing to buffer data at index %ld", (long) arr_idx));

						DUK_ASSERT(arr_idx != DUK__NO_ARRAY_INDEX); /* index/length check guarantees */
						byte_off = arr_idx
						           << h_bufobj->shift; /* no wrap assuming h_bufobj->length is valid */
						elem_size = (duk_small_uint_t) (1U << h_bufobj->shift);

						/* Coerce to number before validating pointers etc so that the
						 * number coercions in duk_hbufobj_validated_write() are
						 * guaranteed to be side effect free and not invalidate the
						 * pointer checks we do here.
						 */
						duk_push_tval(thr, tv_val);
						(void) duk_to_number_m1(thr);

						if (h_bufobj->buf != NULL &&
						    DUK_HBUFOBJ_VALID_BYTEOFFSET_EXCL(h_bufobj, byte_off + elem_size)) {
							data = (duk_uint8_t *) DUK_HBUFFER_GET_DATA_PTR(thr->heap, h_bufobj->buf) +
							       h_bufobj->offset + byte_off;
							duk_hbufobj_validated_write(thr, h_bufobj, data, elem_size);
						} else {
							DUK_D(DUK_DPRINT(
							    "bufobj access out of underlying buffer, ignoring (write skipped)"));
						}
						duk_pop_unsafe(thr);
						goto success_no_arguments_exotic;
					}
				}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

				DUK_D(DUK_DPRINT("should not happen, key %!O", key));
				goto fail_internal; /* should not happen */
			}
			DUK_DD(DUK_DDPRINT("put to existing own plain property, property is writable"));
			goto update_old;
		}
		DUK_UNREACHABLE();

	next_in_chain:
		/* XXX: option to pretend property doesn't exist if sanity limit is
		 * hit might be useful.
		 */
		if (DUK_UNLIKELY(sanity-- == 0)) {
			DUK_ERROR_RANGE(thr, DUK_STR_PROTOTYPE_CHAIN_LIMIT);
			DUK_WO_NORETURN(return 0;);
		}
		curr = DUK_HOBJECT_GET_PROTOTYPE(thr->heap, curr);
	} while (curr != NULL);

	/*
	 *  Property not found in prototype chain.
	 */

	DUK_DDD(DUK_DDDPRINT("property not found in prototype chain"));

	if (orig == NULL) {
		DUK_DD(DUK_DDPRINT("attempt to create a new property in a primitive base object"));
		goto fail_base_primitive;
	}

	if (!DUK_HOBJECT_HAS_EXTENSIBLE(orig)) {
		DUK_DD(DUK_DDPRINT("put to a new property (not found in prototype chain), but original object not extensible"));
		goto fail_not_extensible;
	}

	goto create_new;

update_old:

	/*
	 *  Update an existing property of the base object.
	 */

	/* [key] */

	DUK_DDD(DUK_DDDPRINT("update an existing property of the original object"));

	DUK_ASSERT(orig != NULL);
#if defined(DUK_USE_ROM_OBJECTS)
	/* This should not happen because DUK_TAG_OBJECT case checks
	 * for this already, but check just in case.
	 */
	if (DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) orig)) {
		goto fail_not_writable;
	}
#endif

	/* Although there are writable virtual properties (e.g. plain buffer
	 * and buffer object number indices), they are handled before we come
	 * here.
	 */
	DUK_ASSERT((desc.flags & DUK_PROPDESC_FLAG_VIRTUAL) == 0);
	DUK_ASSERT(desc.a_idx >= 0 || desc.e_idx >= 0);

	/* Array own property .length is handled above. */
	DUK_ASSERT(!(DUK_HOBJECT_IS_ARRAY(orig) && key == DUK_HTHREAD_STRING_LENGTH(thr)));

	if (desc.e_idx >= 0) {
		tv = DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(thr->heap, orig, desc.e_idx);
		DUK_DDD(DUK_DDDPRINT("previous entry value: %!iT", (duk_tval *) tv));
		DUK_TVAL_SET_TVAL_UPDREF(thr, tv, tv_val); /* side effects; e_idx may be invalidated */
		/* don't touch property attributes or hash part */
		DUK_DD(DUK_DDPRINT("put to an existing entry at index %ld -> new value %!iT", (long) desc.e_idx, (duk_tval *) tv));
	} else {
		/* Note: array entries are always writable, so the writability check
		 * above is pointless for them.  The check could be avoided with some
		 * refactoring but is probably not worth it.
		 */

		DUK_ASSERT(desc.a_idx >= 0);
		tv = DUK_HOBJECT_A_GET_VALUE_PTR(thr->heap, orig, desc.a_idx);
		DUK_DDD(DUK_DDDPRINT("previous array value: %!iT", (duk_tval *) tv));
		DUK_TVAL_SET_TVAL_UPDREF(thr, tv, tv_val); /* side effects; a_idx may be invalidated */
		DUK_DD(DUK_DDPRINT("put to an existing array entry at index %ld -> new value %!iT",
		                   (long) desc.a_idx,
		                   (duk_tval *) tv));
	}

	/* Regardless of whether property is found in entry or array part,
	 * it may have arguments exotic behavior (array indices may reside
	 * in entry part for abandoned / non-existent array parts).
	 */
	goto success_with_arguments_exotic;

create_new:

	/*
	 *  Create a new property in the original object.
	 *
	 *  Exotic properties need to be reconsidered here from a write
	 *  perspective (not just property attributes perspective).
	 *  However, the property does not exist in the object already,
	 *  so this limits the kind of exotic properties that apply.
	 */

	/* [key] */

	DUK_DDD(DUK_DDDPRINT("create new property to original object"));

	DUK_ASSERT(orig != NULL);

	/* Array own property .length is handled above. */
	DUK_ASSERT(!(DUK_HOBJECT_IS_ARRAY(orig) && key == DUK_HTHREAD_STRING_LENGTH(thr)));

#if defined(DUK_USE_ROM_OBJECTS)
	/* This should not happen because DUK_TAG_OBJECT case checks
	 * for this already, but check just in case.
	 */
	if (DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) orig)) {
		goto fail_not_writable;
	}
#endif

	/* Not possible because array object 'length' is present
	 * from its creation and cannot be deleted, and is thus
	 * caught as an existing property above.
	 */
	DUK_ASSERT(!(DUK_HOBJECT_HAS_EXOTIC_ARRAY(orig) && key == DUK_HTHREAD_STRING_LENGTH(thr)));

	if (DUK_HOBJECT_HAS_EXOTIC_ARRAY(orig) && arr_idx != DUK__NO_ARRAY_INDEX) {
		/* automatic length update */
		duk_uint32_t old_len;
		duk_harray *a;

		a = (duk_harray *) orig;
		DUK_HARRAY_ASSERT_VALID(a);

		old_len = a->length;

		if (arr_idx >= old_len) {
			DUK_DDD(DUK_DDDPRINT("write new array entry requires length update "
			                     "(arr_idx=%ld, old_len=%ld)",
			                     (long) arr_idx,
			                     (long) old_len));

			if (DUK_HARRAY_LENGTH_NONWRITABLE(a)) {
				DUK_DD(DUK_DDPRINT("attempt to extend array, but array 'length' is not writable"));
				goto fail_not_writable;
			}

			/* Note: actual update happens once write has been completed
			 * without error below.  The write should always succeed
			 * from a specification viewpoint, but we may e.g. run out
			 * of memory.  It's safer in this order.
			 */

			DUK_ASSERT(arr_idx != 0xffffffffUL);
			new_array_length = arr_idx + 1; /* flag for later write */
		} else {
			DUK_DDD(DUK_DDDPRINT("write new array entry does not require length update "
			                     "(arr_idx=%ld, old_len=%ld)",
			                     (long) arr_idx,
			                     (long) old_len));
		}
	}

	/* write_to_array_part: */

	/*
	 *  Write to array part?
	 *
	 *  Note: array abandonding requires a property resize which uses
	 *  'rechecks' valstack for temporaries and may cause any existing
	 *  valstack pointers to be invalidated.  To protect against this,
	 *  tv_obj, tv_key, and tv_val are copies of the original inputs.
	 */

	if (arr_idx != DUK__NO_ARRAY_INDEX && DUK_HOBJECT_HAS_ARRAY_PART(orig)) {
		tv = duk__obtain_arridx_slot(thr, arr_idx, orig);
		if (tv == NULL) {
			DUK_ASSERT(!DUK_HOBJECT_HAS_ARRAY_PART(orig));
			goto write_to_entry_part;
		}

		/* prev value must be unused, no decref */
		DUK_ASSERT(DUK_TVAL_IS_UNUSED(tv));
		DUK_TVAL_SET_TVAL(tv, tv_val);
		DUK_TVAL_INCREF(thr, tv);
		DUK_DD(DUK_DDPRINT("put to new array entry: %ld -> %!T", (long) arr_idx, (duk_tval *) tv));

		/* Note: array part values are [[Writable]], [[Enumerable]],
		 * and [[Configurable]] which matches the required attributes
		 * here.
		 */
		goto entry_updated;
	}

write_to_entry_part:

	/*
	 *  Write to entry part
	 */

	/* entry allocation updates hash part and increases the key
	 * refcount; may need a props allocation resize but doesn't
	 * 'recheck' the valstack.
	 */
	e_idx = duk__hobject_alloc_entry_checked(thr, orig, key);
	DUK_ASSERT(e_idx >= 0);

	tv = DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(thr->heap, orig, e_idx);
	/* prev value can be garbage, no decref */
	DUK_TVAL_SET_TVAL(tv, tv_val);
	DUK_TVAL_INCREF(thr, tv);
	DUK_HOBJECT_E_SET_FLAGS(thr->heap, orig, e_idx, DUK_PROPDESC_FLAGS_WEC);
	goto entry_updated;

entry_updated:

	/*
	 *  Possible pending array length update, which must only be done
	 *  if the actual entry write succeeded.
	 */

	if (new_array_length > 0) {
		/* Note: zero works as a "no update" marker because the new length
		 * can never be zero after a new property is written.
		 */

		DUK_ASSERT(DUK_HOBJECT_HAS_EXOTIC_ARRAY(orig));

		DUK_DDD(DUK_DDDPRINT("write successful, pending array length update to: %ld", (long) new_array_length));

		((duk_harray *) orig)->length = new_array_length;
	}

	/*
	 *  Arguments exotic behavior not possible for new properties: all
	 *  magically bound properties are initially present in the arguments
	 *  object, and if they are deleted, the binding is also removed from
	 *  parameter map.
	 */

	goto success_no_arguments_exotic;

success_with_arguments_exotic:

	/*
	 *  Arguments objects have exotic [[DefineOwnProperty]] which updates
	 *  the internal 'map' of arguments for writes to currently mapped
	 *  arguments.  More conretely, writes to mapped arguments generate
	 *  a write to a bound variable.
	 *
	 *  The [[Put]] algorithm invokes [[DefineOwnProperty]] for existing
	 *  data properties and new properties, but not for existing accessors.
	 *  Hence, in E5 Section 10.6 ([[DefinedOwnProperty]] algorithm), we
	 *  have a Desc with 'Value' (and possibly other properties too), and
	 *  we end up in step 5.b.i.
	 */

	if (arr_idx != DUK__NO_ARRAY_INDEX && DUK_HOBJECT_HAS_EXOTIC_ARGUMENTS(orig)) {
		/* Note: only numbered indices are relevant, so arr_idx fast reject
		 * is good (this is valid unless there are more than 4**32-1 arguments).
		 */

		DUK_DDD(DUK_DDDPRINT("putprop successful, arguments exotic behavior needed"));

		/* Note: we can reuse 'desc' here */

		/* XXX: top of stack must contain value, which helper doesn't touch,
		 * rework to use tv_val directly?
		 */

		duk_push_tval(thr, tv_val);
		(void) duk__check_arguments_map_for_put(thr, orig, key, &desc, throw_flag);
		duk_pop_unsafe(thr);
	}
	/* fall thru */

success_no_arguments_exotic:
	/* shared exit path now */
	DUK_DDD(DUK_DDDPRINT("result: success"));
	duk_pop_unsafe(thr); /* remove key */
	return 1;

#if defined(DUK_USE_ES6_PROXY)
fail_proxy_rejected:
	DUK_DDD(DUK_DDDPRINT("result: error, proxy rejects"));
	if (throw_flag) {
		DUK_ERROR_TYPE(thr, DUK_STR_PROXY_REJECTED);
		DUK_WO_NORETURN(return 0;);
	}
	/* Note: no key on stack */
	return 0;
#endif

fail_base_primitive:
	DUK_DDD(DUK_DDDPRINT("result: error, base primitive"));
	if (throw_flag) {
#if defined(DUK_USE_PARANOID_ERRORS)
		DUK_ERROR_TYPE(thr, DUK_STR_INVALID_BASE);
#else
		DUK_ERROR_FMT2(thr,
		               DUK_ERR_TYPE_ERROR,
		               "cannot write property %s of %s",
		               duk_push_string_tval_readable(thr, tv_key),
		               duk_push_string_tval_readable(thr, tv_obj));
#endif
		DUK_WO_NORETURN(return 0;);
	}
	duk_pop_unsafe(thr); /* remove key */
	return 0;

fail_not_extensible:
	DUK_DDD(DUK_DDDPRINT("result: error, not extensible"));
	if (throw_flag) {
		DUK_ERROR_TYPE(thr, DUK_STR_NOT_EXTENSIBLE);
		DUK_WO_NORETURN(return 0;);
	}
	duk_pop_unsafe(thr); /* remove key */
	return 0;

fail_not_writable:
	DUK_DDD(DUK_DDDPRINT("result: error, not writable"));
	if (throw_flag) {
		DUK_ERROR_TYPE(thr, DUK_STR_NOT_WRITABLE);
		DUK_WO_NORETURN(return 0;);
	}
	duk_pop_unsafe(thr); /* remove key */
	return 0;

#if defined(DUK_USE_ROM_OBJECTS)
fail_not_writable_no_pop:
	DUK_DDD(DUK_DDDPRINT("result: error, not writable"));
	if (throw_flag) {
		DUK_ERROR_TYPE(thr, DUK_STR_NOT_WRITABLE);
		DUK_WO_NORETURN(return 0;);
	}
	return 0;
#endif

fail_array_length_partial:
	DUK_DD(DUK_DDPRINT("result: error, array length write only partially successful"));
	if (throw_flag) {
		DUK_ERROR_TYPE(thr, DUK_STR_NOT_CONFIGURABLE);
		DUK_WO_NORETURN(return 0;);
	}
	duk_pop_unsafe(thr); /* remove key */
	return 0;

fail_no_setter:
	DUK_DDD(DUK_DDDPRINT("result: error, accessor property without setter"));
	if (throw_flag) {
		DUK_ERROR_TYPE(thr, DUK_STR_SETTER_UNDEFINED);
		DUK_WO_NORETURN(return 0;);
	}
	duk_pop_unsafe(thr); /* remove key */
	return 0;

fail_internal:
	DUK_DDD(DUK_DDDPRINT("result: error, internal"));
	if (throw_flag) {
		DUK_ERROR_INTERNAL(thr);
		DUK_WO_NORETURN(return 0;);
	}
	duk_pop_unsafe(thr); /* remove key */
	return 0;
}

/*
 *  ECMAScript compliant [[Delete]](P, Throw).
 */

DUK_INTERNAL duk_bool_t duk_hobject_delprop_raw(duk_hthread *thr, duk_hobject *obj, duk_hstring *key, duk_small_uint_t flags) {
	duk_propdesc desc;
	duk_tval *tv;
	duk_uint32_t arr_idx;
	duk_bool_t throw_flag;
	duk_bool_t force_flag;

	throw_flag = (flags & DUK_DELPROP_FLAG_THROW);
	force_flag = (flags & DUK_DELPROP_FLAG_FORCE);

	DUK_DDD(DUK_DDDPRINT("delprop_raw: thr=%p, obj=%p, key=%p, throw=%ld, force=%ld (obj -> %!O, key -> %!O)",
	                     (void *) thr,
	                     (void *) obj,
	                     (void *) key,
	                     (long) throw_flag,
	                     (long) force_flag,
	                     (duk_heaphdr *) obj,
	                     (duk_heaphdr *) key));

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(key != NULL);

	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	arr_idx = DUK_HSTRING_GET_ARRIDX_FAST(key);

	/* 0 = don't push current value */
	if (!duk__get_own_propdesc_raw(thr, obj, key, arr_idx, &desc, 0 /*flags*/)) { /* don't push value */
		DUK_DDD(DUK_DDDPRINT("property not found, succeed always"));
		goto success;
	}

#if defined(DUK_USE_ROM_OBJECTS)
	if (DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) obj)) {
		DUK_DD(DUK_DDPRINT("attempt to delprop on read-only target object"));
		goto fail_not_configurable;
	}
#endif

	if ((desc.flags & DUK_PROPDESC_FLAG_CONFIGURABLE) == 0 && !force_flag) {
		goto fail_not_configurable;
	}
	if (desc.a_idx < 0 && desc.e_idx < 0) {
		/* Currently there are no deletable virtual properties, but
		 * with force_flag we might attempt to delete one.
		 */
		DUK_DD(DUK_DDPRINT("delete failed: property found, force flag, but virtual (and implicitly non-configurable)"));
		goto fail_virtual;
	}

	if (desc.a_idx >= 0) {
		DUK_ASSERT(desc.e_idx < 0);

		tv = DUK_HOBJECT_A_GET_VALUE_PTR(thr->heap, obj, desc.a_idx);
		DUK_TVAL_SET_UNUSED_UPDREF(thr, tv); /* side effects */
		goto success;
	} else {
		DUK_ASSERT(desc.a_idx < 0);

		/* remove hash entry (no decref) */
#if defined(DUK_USE_HOBJECT_HASH_PART)
		if (desc.h_idx >= 0) {
			duk_uint32_t *h_base = DUK_HOBJECT_H_GET_BASE(thr->heap, obj);

			DUK_DDD(DUK_DDDPRINT("removing hash entry at h_idx %ld", (long) desc.h_idx));
			DUK_ASSERT(DUK_HOBJECT_GET_HSIZE(obj) > 0);
			DUK_ASSERT((duk_uint32_t) desc.h_idx < DUK_HOBJECT_GET_HSIZE(obj));
			h_base[desc.h_idx] = DUK__HASH_DELETED;
		} else {
			DUK_ASSERT(DUK_HOBJECT_GET_HSIZE(obj) == 0);
		}
#else
		DUK_ASSERT(DUK_HOBJECT_GET_HSIZE(obj) == 0);
#endif

		/* Remove value.  This requires multiple writes so avoid side
		 * effects via no-refzero macros so that e_idx is not
		 * invalidated.
		 */
		DUK_DDD(DUK_DDDPRINT("before removing value, e_idx %ld, key %p, key at slot %p",
		                     (long) desc.e_idx,
		                     (void *) key,
		                     (void *) DUK_HOBJECT_E_GET_KEY(thr->heap, obj, desc.e_idx)));
		DUK_DDD(DUK_DDDPRINT("removing value at e_idx %ld", (long) desc.e_idx));
		if (DUK_HOBJECT_E_SLOT_IS_ACCESSOR(thr->heap, obj, desc.e_idx)) {
			duk_hobject *tmp;

			tmp = DUK_HOBJECT_E_GET_VALUE_GETTER(thr->heap, obj, desc.e_idx);
			DUK_HOBJECT_E_SET_VALUE_GETTER(thr->heap, obj, desc.e_idx, NULL);
			DUK_UNREF(tmp);
			DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, tmp);

			tmp = DUK_HOBJECT_E_GET_VALUE_SETTER(thr->heap, obj, desc.e_idx);
			DUK_HOBJECT_E_SET_VALUE_SETTER(thr->heap, obj, desc.e_idx, NULL);
			DUK_UNREF(tmp);
			DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, tmp);
		} else {
			tv = DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(thr->heap, obj, desc.e_idx);
			DUK_TVAL_SET_UNDEFINED_UPDREF_NORZ(thr, tv);
		}
#if 0
		/* Not strictly necessary because if key == NULL, flag MUST be ignored. */
		DUK_HOBJECT_E_SET_FLAGS(thr->heap, obj, desc.e_idx, 0);
#endif

		/* Remove key. */
		DUK_DDD(DUK_DDDPRINT("before removing key, e_idx %ld, key %p, key at slot %p",
		                     (long) desc.e_idx,
		                     (void *) key,
		                     (void *) DUK_HOBJECT_E_GET_KEY(thr->heap, obj, desc.e_idx)));
		DUK_DDD(DUK_DDDPRINT("removing key at e_idx %ld", (long) desc.e_idx));
		DUK_ASSERT(key == DUK_HOBJECT_E_GET_KEY(thr->heap, obj, desc.e_idx));
		DUK_HOBJECT_E_SET_KEY(thr->heap, obj, desc.e_idx, NULL);
		DUK_HSTRING_DECREF_NORZ(thr, key);

		/* Trigger refzero side effects only when we're done as a
		 * finalizer might operate on the object and affect the
		 * e_idx we're supposed to use.
		 */
		DUK_REFZERO_CHECK_SLOW(thr);
		goto success;
	}

	DUK_UNREACHABLE();

success:
	/*
	 *  Argument exotic [[Delete]] behavior (E5 Section 10.6) is
	 *  a post-check, keeping arguments internal 'map' in sync with
	 *  any successful deletes (note that property does not need to
	 *  exist for delete to 'succeed').
	 *
	 *  Delete key from 'map'.  Since 'map' only contains array index
	 *  keys, we can use arr_idx for a fast skip.
	 */

	DUK_DDD(DUK_DDDPRINT("delete successful, check for arguments exotic behavior"));

	if (arr_idx != DUK__NO_ARRAY_INDEX && DUK_HOBJECT_HAS_EXOTIC_ARGUMENTS(obj)) {
		/* Note: only numbered indices are relevant, so arr_idx fast reject
		 * is good (this is valid unless there are more than 4**32-1 arguments).
		 */

		DUK_DDD(DUK_DDDPRINT("delete successful, arguments exotic behavior needed"));

		/* Note: we can reuse 'desc' here */
		(void) duk__check_arguments_map_for_delete(thr, obj, key, &desc);
	}

	DUK_DDD(DUK_DDDPRINT("delete successful"));
	return 1;

fail_virtual: /* just use the same "not configurable" error message */
fail_not_configurable:
	DUK_DDD(DUK_DDDPRINT("delete failed: property found, not configurable"));

	if (throw_flag) {
		DUK_ERROR_TYPE(thr, DUK_STR_NOT_CONFIGURABLE);
		DUK_WO_NORETURN(return 0;);
	}
	return 0;
}

/*
 *  DELPROP: ECMAScript property deletion.
 */

DUK_INTERNAL duk_bool_t duk_hobject_delprop(duk_hthread *thr, duk_tval *tv_obj, duk_tval *tv_key, duk_bool_t throw_flag) {
	duk_hstring *key = NULL;
#if defined(DUK_USE_ES6_PROXY)
	duk_propdesc desc;
#endif
	duk_int_t entry_top;
	duk_uint32_t arr_idx = DUK__NO_ARRAY_INDEX;
	duk_bool_t rc;

	DUK_DDD(DUK_DDDPRINT("delprop: thr=%p, obj=%p, key=%p (obj -> %!T, key -> %!T)",
	                     (void *) thr,
	                     (void *) tv_obj,
	                     (void *) tv_key,
	                     (duk_tval *) tv_obj,
	                     (duk_tval *) tv_key));

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(tv_obj != NULL);
	DUK_ASSERT(tv_key != NULL);

	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	/* Storing the entry top is cheaper here to ensure stack is correct at exit,
	 * as there are several paths out.
	 */
	entry_top = duk_get_top(thr);

	if (DUK_TVAL_IS_UNDEFINED(tv_obj) || DUK_TVAL_IS_NULL(tv_obj)) {
		DUK_DDD(DUK_DDDPRINT("base object is undefined or null -> reject"));
		goto fail_invalid_base_uncond;
	}

	duk_push_tval(thr, tv_obj);
	duk_push_tval(thr, tv_key);

	tv_obj = DUK_GET_TVAL_NEGIDX(thr, -2);
	if (DUK_TVAL_IS_OBJECT(tv_obj)) {
		duk_hobject *obj = DUK_TVAL_GET_OBJECT(tv_obj);
		DUK_ASSERT(obj != NULL);

#if defined(DUK_USE_ES6_PROXY)
		if (DUK_UNLIKELY(DUK_HOBJECT_IS_PROXY(obj))) {
			duk_hobject *h_target;
			duk_bool_t tmp_bool;

			/* Note: proxy handling must happen before key is string coerced. */

			if (duk__proxy_check_prop(thr, obj, DUK_STRIDX_DELETE_PROPERTY, tv_key, &h_target)) {
				/* -> [ ... obj key trap handler ] */
				DUK_DDD(DUK_DDDPRINT("-> proxy object 'deleteProperty' for key %!T", (duk_tval *) tv_key));
				duk_push_hobject(thr, h_target); /* target */
				duk_dup_m4(thr); /* P */
				duk_call_method(thr, 2 /*nargs*/);
				tmp_bool = duk_to_boolean_top_pop(thr);
				if (!tmp_bool) {
					goto fail_proxy_rejected; /* retval indicates delete failed */
				}

				/* Target object must be checked for a conflicting
				 * non-configurable property.
				 */
				tv_key = DUK_GET_TVAL_NEGIDX(thr, -1);
				arr_idx = duk__push_tval_to_property_key(thr, tv_key, &key);
				DUK_ASSERT(key != NULL);

				if (duk__get_own_propdesc_raw(thr, h_target, key, arr_idx, &desc, 0 /*flags*/)) { /* don't push
					                                                                             value */
					duk_small_int_t desc_reject;

					DUK_DDD(DUK_DDDPRINT("proxy 'deleteProperty': target has matching property %!O, check for "
					                     "conflicting property; desc.flags=0x%08lx, "
					                     "desc.get=%p, desc.set=%p",
					                     (duk_heaphdr *) key,
					                     (unsigned long) desc.flags,
					                     (void *) desc.get,
					                     (void *) desc.set));

					desc_reject = !(desc.flags & DUK_PROPDESC_FLAG_CONFIGURABLE);
					if (desc_reject) {
						/* unconditional */
						DUK_ERROR_TYPE(thr, DUK_STR_PROXY_REJECTED);
						DUK_WO_NORETURN(return 0;);
					}
				}
				rc = 1; /* success */
				goto done_rc;
			}

			obj = h_target; /* resume delete to target */
		}
#endif /* DUK_USE_ES6_PROXY */

		arr_idx = duk__to_property_key(thr, -1, &key);
		DUK_ASSERT(key != NULL);

		rc = duk_hobject_delprop_raw(thr, obj, key, throw_flag ? DUK_DELPROP_FLAG_THROW : 0);
		goto done_rc;
	} else if (DUK_TVAL_IS_STRING(tv_obj)) {
		/* String has .length and array index virtual properties
		 * which can't be deleted.  No need for a symbol check;
		 * no offending virtual symbols exist.
		 */
		/* XXX: unnecessary string coercion for array indices,
		 * intentional to keep small.
		 */
		duk_hstring *h = DUK_TVAL_GET_STRING(tv_obj);
		DUK_ASSERT(h != NULL);

		arr_idx = duk__to_property_key(thr, -1, &key);
		DUK_ASSERT(key != NULL);

		if (key == DUK_HTHREAD_STRING_LENGTH(thr)) {
			goto fail_not_configurable;
		}

		if (arr_idx != DUK__NO_ARRAY_INDEX && arr_idx < DUK_HSTRING_GET_CHARLEN(h)) {
			goto fail_not_configurable;
		}
	} else if (DUK_TVAL_IS_BUFFER(tv_obj)) {
		/* XXX: unnecessary string coercion for array indices,
		 * intentional to keep small; some overlap with string
		 * handling.
		 */
		duk_hbuffer *h = DUK_TVAL_GET_BUFFER(tv_obj);
		DUK_ASSERT(h != NULL);

		arr_idx = duk__to_property_key(thr, -1, &key);
		DUK_ASSERT(key != NULL);

		if (key == DUK_HTHREAD_STRING_LENGTH(thr)) {
			goto fail_not_configurable;
		}

		if (arr_idx != DUK__NO_ARRAY_INDEX && arr_idx < DUK_HBUFFER_GET_SIZE(h)) {
			goto fail_not_configurable;
		}
	} else if (DUK_TVAL_IS_LIGHTFUNC(tv_obj)) {
		/* Lightfunc has no virtual properties since Duktape 2.2
		 * so success.  Still must coerce key for side effects.
		 */

		arr_idx = duk__to_property_key(thr, -1, &key);
		DUK_ASSERT(key != NULL);
		DUK_UNREF(key);
	}

	/* non-object base, no offending virtual property */
	rc = 1;
	goto done_rc;

done_rc:
	duk_set_top_unsafe(thr, entry_top);
	return rc;

fail_invalid_base_uncond:
	/* Note: unconditional throw */
	DUK_ASSERT(duk_get_top(thr) == entry_top);
#if defined(DUK_USE_PARANOID_ERRORS)
	DUK_ERROR_TYPE(thr, DUK_STR_INVALID_BASE);
#else
	DUK_ERROR_FMT2(thr,
	               DUK_ERR_TYPE_ERROR,
	               "cannot delete property %s of %s",
	               duk_push_string_tval_readable(thr, tv_key),
	               duk_push_string_tval_readable(thr, tv_obj));
#endif
	DUK_WO_NORETURN(return 0;);

#if defined(DUK_USE_ES6_PROXY)
fail_proxy_rejected:
	if (throw_flag) {
		DUK_ERROR_TYPE(thr, DUK_STR_PROXY_REJECTED);
		DUK_WO_NORETURN(return 0;);
	}
	duk_set_top_unsafe(thr, entry_top);
	return 0;
#endif

fail_not_configurable:
	if (throw_flag) {
		DUK_ERROR_TYPE(thr, DUK_STR_NOT_CONFIGURABLE);
		DUK_WO_NORETURN(return 0;);
	}
	duk_set_top_unsafe(thr, entry_top);
	return 0;
}

/*
 *  Internal helper to define a property with specific flags, ignoring
 *  normal semantics such as extensibility, write protection etc.
 *  Overwrites any existing value and attributes unless caller requests
 *  that value only be updated if it doesn't already exists.
 *
 *  Does not support:
 *    - virtual properties (error if write attempted)
 *    - getter/setter properties (error if write attempted)
 *    - non-default (!= WEC) attributes for array entries (error if attempted)
 *    - array abandoning: if array part exists, it is always extended
 *    - array 'length' updating
 *
 *  Stack: [... in_val] -> []
 *
 *  Used for e.g. built-in initialization and environment record
 *  operations.
 */

DUK_INTERNAL void duk_hobject_define_property_internal(duk_hthread *thr,
                                                       duk_hobject *obj,
                                                       duk_hstring *key,
                                                       duk_small_uint_t flags) {
	duk_propdesc desc;
	duk_uint32_t arr_idx;
	duk_int_t e_idx;
	duk_tval *tv1 = NULL;
	duk_tval *tv2 = NULL;
	duk_small_uint_t propflags = flags & DUK_PROPDESC_FLAGS_MASK; /* mask out flags not actually stored */

	DUK_DDD(DUK_DDDPRINT("define new property (internal): thr=%p, obj=%!O, key=%!O, flags=0x%02lx, val=%!T",
	                     (void *) thr,
	                     (duk_heaphdr *) obj,
	                     (duk_heaphdr *) key,
	                     (unsigned long) flags,
	                     (duk_tval *) duk_get_tval(thr, -1)));

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(key != NULL);
	DUK_ASSERT(!DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) obj));
	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);
	DUK_ASSERT(duk_is_valid_index(thr, -1)); /* contains value */

	arr_idx = DUK_HSTRING_GET_ARRIDX_SLOW(key);

	if (duk__get_own_propdesc_raw(thr, obj, key, arr_idx, &desc, 0 /*flags*/)) { /* don't push value */
		if (desc.e_idx >= 0) {
			if (flags & DUK_PROPDESC_FLAG_NO_OVERWRITE) {
				DUK_DDD(DUK_DDDPRINT("property already exists in the entry part -> skip as requested"));
				goto pop_exit;
			}
			DUK_DDD(DUK_DDDPRINT("property already exists in the entry part -> update value and attributes"));
			if (DUK_UNLIKELY(DUK_HOBJECT_E_SLOT_IS_ACCESSOR(thr->heap, obj, desc.e_idx))) {
				DUK_D(DUK_DPRINT("existing property is an accessor, not supported"));
				goto error_internal;
			}

			DUK_HOBJECT_E_SET_FLAGS(thr->heap, obj, desc.e_idx, propflags);
			tv1 = DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(thr->heap, obj, desc.e_idx);
		} else if (desc.a_idx >= 0) {
			if (flags & DUK_PROPDESC_FLAG_NO_OVERWRITE) {
				DUK_DDD(DUK_DDDPRINT("property already exists in the array part -> skip as requested"));
				goto pop_exit;
			}
			DUK_DDD(DUK_DDDPRINT("property already exists in the array part -> update value (assert attributes)"));
			if (propflags != DUK_PROPDESC_FLAGS_WEC) {
				DUK_D(DUK_DPRINT("existing property in array part, but propflags not WEC (0x%02lx)",
				                 (unsigned long) propflags));
				goto error_internal;
			}

			tv1 = DUK_HOBJECT_A_GET_VALUE_PTR(thr->heap, obj, desc.a_idx);
		} else {
			if (flags & DUK_PROPDESC_FLAG_NO_OVERWRITE) {
				DUK_DDD(DUK_DDDPRINT("property already exists but is virtual -> skip as requested"));
				goto pop_exit;
			}
			if (key == DUK_HTHREAD_STRING_LENGTH(thr) && DUK_HOBJECT_HAS_EXOTIC_ARRAY(obj)) {
				duk_uint32_t new_len;
#if defined(DUK_USE_DEBUG)
				duk_uint32_t prev_len;
				prev_len = ((duk_harray *) obj)->length;
#endif
				new_len = duk__to_new_array_length_checked(thr, DUK_GET_TVAL_NEGIDX(thr, -1));
				((duk_harray *) obj)->length = new_len;
				DUK_DD(DUK_DDPRINT("internal define property for array .length: %ld -> %ld",
				                   (long) prev_len,
				                   (long) ((duk_harray *) obj)->length));
				goto pop_exit;
			}
			DUK_DD(DUK_DDPRINT("property already exists but is virtual -> failure"));
			goto error_virtual;
		}

		goto write_value;
	}

	if (DUK_HOBJECT_HAS_ARRAY_PART(obj)) {
		if (arr_idx != DUK__NO_ARRAY_INDEX) {
			DUK_DDD(DUK_DDDPRINT("property does not exist, object has array part -> possibly extend array part and "
			                     "write value (assert attributes)"));
			DUK_ASSERT(propflags == DUK_PROPDESC_FLAGS_WEC);

			tv1 = duk__obtain_arridx_slot(thr, arr_idx, obj);
			if (tv1 == NULL) {
				DUK_ASSERT(!DUK_HOBJECT_HAS_ARRAY_PART(obj));
				goto write_to_entry_part;
			}

			tv1 = DUK_HOBJECT_A_GET_VALUE_PTR(thr->heap, obj, arr_idx);
			goto write_value;
		}
	}

write_to_entry_part:
	DUK_DDD(DUK_DDDPRINT(
	    "property does not exist, object belongs in entry part -> allocate new entry and write value and attributes"));
	e_idx = duk__hobject_alloc_entry_checked(thr, obj, key); /* increases key refcount */
	DUK_ASSERT(e_idx >= 0);
	DUK_HOBJECT_E_SET_FLAGS(thr->heap, obj, e_idx, propflags);
	tv1 = DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(thr->heap, obj, e_idx);
	/* new entry: previous value is garbage; set to undefined to share write_value */
	DUK_TVAL_SET_UNDEFINED(tv1);
	goto write_value;

write_value:
	/* tv1 points to value storage */

	tv2 = duk_require_tval(thr, -1); /* late lookup, avoid side effects */
	DUK_DDD(DUK_DDDPRINT("writing/updating value: %!T -> %!T", (duk_tval *) tv1, (duk_tval *) tv2));

	DUK_TVAL_SET_TVAL_UPDREF(thr, tv1, tv2); /* side effects */
	goto pop_exit;

pop_exit:
	duk_pop_unsafe(thr); /* remove in_val */
	return;

error_virtual: /* share error message */
error_internal:
	DUK_ERROR_INTERNAL(thr);
	DUK_WO_NORETURN(return;);
}

/*
 *  Fast path for defining array indexed values without interning the key.
 *  This is used by e.g. code for Array prototype and traceback creation so
 *  must avoid interning.
 */

DUK_INTERNAL void duk_hobject_define_property_internal_arridx(duk_hthread *thr,
                                                              duk_hobject *obj,
                                                              duk_uarridx_t arr_idx,
                                                              duk_small_uint_t flags) {
	duk_hstring *key;
	duk_tval *tv1, *tv2;

	DUK_DDD(DUK_DDDPRINT("define new property (internal) arr_idx fast path: thr=%p, obj=%!O, "
	                     "arr_idx=%ld, flags=0x%02lx, val=%!T",
	                     (void *) thr,
	                     obj,
	                     (long) arr_idx,
	                     (unsigned long) flags,
	                     (duk_tval *) duk_get_tval(thr, -1)));

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(!DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) obj));

	if (DUK_HOBJECT_HAS_ARRAY_PART(obj) && arr_idx != DUK__NO_ARRAY_INDEX && flags == DUK_PROPDESC_FLAGS_WEC) {
		DUK_ASSERT((flags & DUK_PROPDESC_FLAG_NO_OVERWRITE) == 0); /* covered by comparison */

		DUK_DDD(DUK_DDDPRINT("define property to array part (property may or may not exist yet)"));

		tv1 = duk__obtain_arridx_slot(thr, arr_idx, obj);
		if (tv1 == NULL) {
			DUK_ASSERT(!DUK_HOBJECT_HAS_ARRAY_PART(obj));
			goto write_slow;
		}
		tv2 = duk_require_tval(thr, -1);

		DUK_TVAL_SET_TVAL_UPDREF(thr, tv1, tv2); /* side effects */

		duk_pop_unsafe(thr); /* [ ...val ] -> [ ... ] */
		return;
	}

write_slow:
	DUK_DDD(DUK_DDDPRINT("define property fast path didn't work, use slow path"));

	key = duk_push_uint_to_hstring(thr, (duk_uint_t) arr_idx);
	DUK_ASSERT(key != NULL);
	duk_insert(thr, -2); /* [ ... val key ] -> [ ... key val ] */

	duk_hobject_define_property_internal(thr, obj, key, flags);

	duk_pop_unsafe(thr); /* [ ... key ] -> [ ... ] */
}

/*
 *  Internal helpers for managing object 'length'
 */

DUK_INTERNAL duk_size_t duk_hobject_get_length(duk_hthread *thr, duk_hobject *obj) {
	duk_double_t val;

	DUK_CTX_ASSERT_VALID(thr);
	DUK_ASSERT(obj != NULL);

	/* Fast path for Arrays. */
	if (DUK_HOBJECT_HAS_EXOTIC_ARRAY(obj)) {
		return ((duk_harray *) obj)->length;
	}

	/* Slow path, .length can be e.g. accessor, obj can be a Proxy, etc. */
	duk_push_hobject(thr, obj);
	duk_push_hstring_stridx(thr, DUK_STRIDX_LENGTH);
	(void) duk_hobject_getprop(thr, DUK_GET_TVAL_NEGIDX(thr, -2), DUK_GET_TVAL_NEGIDX(thr, -1));
	val = duk_to_number_m1(thr);
	duk_pop_3_unsafe(thr);

	/* This isn't part of ECMAScript semantics; return a value within
	 * duk_size_t range, or 0 otherwise.
	 */
	if (val >= 0.0 && val <= (duk_double_t) DUK_SIZE_MAX) {
		return (duk_size_t) val;
	}
	return 0;
}

/*
 *  Fast finalizer check for an object.  Walks the prototype chain, checking
 *  for finalizer presence using DUK_HOBJECT_FLAG_HAVE_FINALIZER which is kept
 *  in sync with the actual property when setting/removing the finalizer.
 */

#if defined(DUK_USE_HEAPPTR16)
DUK_INTERNAL duk_bool_t duk_hobject_has_finalizer_fast_raw(duk_heap *heap, duk_hobject *obj) {
#else
DUK_INTERNAL duk_bool_t duk_hobject_has_finalizer_fast_raw(duk_hobject *obj) {
#endif
	duk_uint_t sanity;

	DUK_ASSERT(obj != NULL);

	sanity = DUK_HOBJECT_PROTOTYPE_CHAIN_SANITY;
	do {
		if (DUK_UNLIKELY(DUK_HOBJECT_HAS_HAVE_FINALIZER(obj))) {
			return 1;
		}
		if (DUK_UNLIKELY(sanity-- == 0)) {
			DUK_D(DUK_DPRINT("prototype loop when checking for finalizer existence; returning false"));
			return 0;
		}
#if defined(DUK_USE_HEAPPTR16)
		DUK_ASSERT(heap != NULL);
		obj = DUK_HOBJECT_GET_PROTOTYPE(heap, obj);
#else
		obj = DUK_HOBJECT_GET_PROTOTYPE(NULL, obj); /* 'heap' arg ignored */
#endif
	} while (obj != NULL);

	return 0;
}

/*
 *  Object.getOwnPropertyDescriptor()  (E5 Sections 15.2.3.3, 8.10.4)
 *
 *  [ ... key ] -> [ ... desc/undefined ]
 */

DUK_INTERNAL void duk_hobject_object_get_own_property_descriptor(duk_hthread *thr, duk_idx_t obj_idx) {
	duk_hobject *obj;
	duk_hstring *key;
	duk_propdesc pd;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);

	obj = duk_require_hobject_promote_mask(thr, obj_idx, DUK_TYPE_MASK_LIGHTFUNC | DUK_TYPE_MASK_BUFFER);
	key = duk_to_property_key_hstring(thr, -1);
	DUK_ASSERT(key != NULL);

	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	if (!duk_hobject_get_own_propdesc(thr, obj, key, &pd, DUK_GETDESC_FLAG_PUSH_VALUE)) {
		duk_push_undefined(thr);
		duk_remove_m2(thr);
		return;
	}

	duk_push_object(thr);

	/* [ ... key value desc ] */

	if (DUK_PROPDESC_IS_ACCESSOR(&pd)) {
		/* If a setter/getter is missing (undefined), the descriptor must
		 * still have the property present with the value 'undefined'.
		 */
		if (pd.get) {
			duk_push_hobject(thr, pd.get);
		} else {
			duk_push_undefined(thr);
		}
		duk_put_prop_stridx_short(thr, -2, DUK_STRIDX_GET);
		if (pd.set) {
			duk_push_hobject(thr, pd.set);
		} else {
			duk_push_undefined(thr);
		}
		duk_put_prop_stridx_short(thr, -2, DUK_STRIDX_SET);
	} else {
		duk_dup_m2(thr);
		duk_put_prop_stridx_short(thr, -2, DUK_STRIDX_VALUE);
		duk_push_boolean(thr, DUK_PROPDESC_IS_WRITABLE(&pd));
		duk_put_prop_stridx_short(thr, -2, DUK_STRIDX_WRITABLE);
	}
	duk_push_boolean(thr, DUK_PROPDESC_IS_ENUMERABLE(&pd));
	duk_put_prop_stridx_short(thr, -2, DUK_STRIDX_ENUMERABLE);
	duk_push_boolean(thr, DUK_PROPDESC_IS_CONFIGURABLE(&pd));
	duk_put_prop_stridx_short(thr, -2, DUK_STRIDX_CONFIGURABLE);

	/* [ ... key value desc ] */

	duk_replace(thr, -3);
	duk_pop_unsafe(thr); /* -> [ ... desc ] */
}

/*
 *  NormalizePropertyDescriptor() related helper.
 *
 *  Internal helper which validates and normalizes a property descriptor
 *  represented as an ECMAScript object (e.g. argument to defineProperty()).
 *  The output of this conversion is a set of defprop_flags and possibly
 *  some values pushed on the value stack to (1) ensure borrowed pointers
 *  remain valid, and (2) avoid unnecessary pops for footprint reasons.
 *  Caller must manage stack top carefully because the number of values
 *  pushed depends on the input property descriptor.
 *
 *  The original descriptor object must not be altered in the process.
 */

/* XXX: very basic optimization -> duk_get_prop_stridx_top */

DUK_INTERNAL
void duk_hobject_prepare_property_descriptor(duk_hthread *thr,
                                             duk_idx_t idx_in,
                                             duk_uint_t *out_defprop_flags,
                                             duk_idx_t *out_idx_value,
                                             duk_hobject **out_getter,
                                             duk_hobject **out_setter) {
	duk_idx_t idx_value = -1;
	duk_hobject *getter = NULL;
	duk_hobject *setter = NULL;
	duk_bool_t is_data_desc = 0;
	duk_bool_t is_acc_desc = 0;
	duk_uint_t defprop_flags = 0;

	DUK_ASSERT(out_defprop_flags != NULL);
	DUK_ASSERT(out_idx_value != NULL);
	DUK_ASSERT(out_getter != NULL);
	DUK_ASSERT(out_setter != NULL);
	DUK_ASSERT(idx_in <= 0x7fffL); /* short variants would be OK, but not used to avoid shifts */

	/* Must be an object, otherwise TypeError (E5.1 Section 8.10.5, step 1). */
	idx_in = duk_require_normalize_index(thr, idx_in);
	(void) duk_require_hobject(thr, idx_in);

	/* The coercion order must match the ToPropertyDescriptor() algorithm
	 * so that side effects in coercion happen in the correct order.
	 * (This order also happens to be compatible with duk_def_prop(),
	 * although it doesn't matter in practice.)
	 */

	if (duk_get_prop_stridx(thr, idx_in, DUK_STRIDX_VALUE)) {
		is_data_desc = 1;
		defprop_flags |= DUK_DEFPROP_HAVE_VALUE;
		idx_value = duk_get_top_index(thr);
	}

	if (duk_get_prop_stridx(thr, idx_in, DUK_STRIDX_WRITABLE)) {
		is_data_desc = 1;
		if (duk_to_boolean_top_pop(thr)) {
			defprop_flags |= DUK_DEFPROP_HAVE_WRITABLE | DUK_DEFPROP_WRITABLE;
		} else {
			defprop_flags |= DUK_DEFPROP_HAVE_WRITABLE;
		}
	}

	if (duk_get_prop_stridx(thr, idx_in, DUK_STRIDX_GET)) {
		duk_tval *tv = duk_require_tval(thr, -1);
		duk_hobject *h_get;

		if (DUK_TVAL_IS_UNDEFINED(tv)) {
			/* undefined is accepted */
			DUK_ASSERT(getter == NULL);
		} else {
			/* NOTE: lightfuncs are coerced to full functions because
			 * lightfuncs don't fit into a property value slot.  This
			 * has some side effects, see test-dev-lightfunc-accessor.js.
			 */
			h_get = duk_get_hobject_promote_lfunc(thr, -1);
			if (h_get == NULL || !DUK_HOBJECT_IS_CALLABLE(h_get)) {
				goto type_error;
			}
			getter = h_get;
		}
		is_acc_desc = 1;
		defprop_flags |= DUK_DEFPROP_HAVE_GETTER;
	}

	if (duk_get_prop_stridx(thr, idx_in, DUK_STRIDX_SET)) {
		duk_tval *tv = duk_require_tval(thr, -1);
		duk_hobject *h_set;

		if (DUK_TVAL_IS_UNDEFINED(tv)) {
			/* undefined is accepted */
			DUK_ASSERT(setter == NULL);
		} else {
			/* NOTE: lightfuncs are coerced to full functions because
			 * lightfuncs don't fit into a property value slot.  This
			 * has some side effects, see test-dev-lightfunc-accessor.js.
			 */
			h_set = duk_get_hobject_promote_lfunc(thr, -1);
			if (h_set == NULL || !DUK_HOBJECT_IS_CALLABLE(h_set)) {
				goto type_error;
			}
			setter = h_set;
		}
		is_acc_desc = 1;
		defprop_flags |= DUK_DEFPROP_HAVE_SETTER;
	}

	if (duk_get_prop_stridx(thr, idx_in, DUK_STRIDX_ENUMERABLE)) {
		if (duk_to_boolean_top_pop(thr)) {
			defprop_flags |= DUK_DEFPROP_HAVE_ENUMERABLE | DUK_DEFPROP_ENUMERABLE;
		} else {
			defprop_flags |= DUK_DEFPROP_HAVE_ENUMERABLE;
		}
	}

	if (duk_get_prop_stridx(thr, idx_in, DUK_STRIDX_CONFIGURABLE)) {
		if (duk_to_boolean_top_pop(thr)) {
			defprop_flags |= DUK_DEFPROP_HAVE_CONFIGURABLE | DUK_DEFPROP_CONFIGURABLE;
		} else {
			defprop_flags |= DUK_DEFPROP_HAVE_CONFIGURABLE;
		}
	}

	if (is_data_desc && is_acc_desc) {
		goto type_error;
	}

	*out_defprop_flags = defprop_flags;
	*out_idx_value = idx_value;
	*out_getter = getter;
	*out_setter = setter;

	/* [ ... [multiple values] ] */
	return;

type_error:
	DUK_ERROR_TYPE(thr, DUK_STR_INVALID_DESCRIPTOR);
	DUK_WO_NORETURN(return;);
}

/*
 *  Object.defineProperty() related helper (E5 Section 15.2.3.6).
 *  Also handles ES2015 Reflect.defineProperty().
 *
 *  Inlines all [[DefineOwnProperty]] exotic behaviors.
 *
 *  Note: ECMAScript compliant [[DefineOwnProperty]](P, Desc, Throw) is not
 *  implemented directly, but Object.defineProperty() serves its purpose.
 *  We don't need the [[DefineOwnProperty]] internally and we don't have a
 *  property descriptor with 'missing values' so it's easier to avoid it
 *  entirely.
 *
 *  Note: this is only called for actual objects, not primitive values.
 *  This must support virtual properties for full objects (e.g. Strings)
 *  but not for plain values (e.g. strings).  Lightfuncs, even though
 *  primitive in a sense, are treated like objects and accepted as target
 *  values.
 */

/* XXX: this is a major target for size optimization */
DUK_INTERNAL
duk_bool_t duk_hobject_define_property_helper(duk_hthread *thr,
                                              duk_uint_t defprop_flags,
                                              duk_hobject *obj,
                                              duk_hstring *key,
                                              duk_idx_t idx_value,
                                              duk_hobject *get,
                                              duk_hobject *set,
                                              duk_bool_t throw_flag) {
	duk_uint32_t arr_idx;
	duk_tval tv;
	duk_bool_t has_enumerable;
	duk_bool_t has_configurable;
	duk_bool_t has_writable;
	duk_bool_t has_value;
	duk_bool_t has_get;
	duk_bool_t has_set;
	duk_bool_t is_enumerable;
	duk_bool_t is_configurable;
	duk_bool_t is_writable;
	duk_bool_t force_flag;
	duk_small_uint_t new_flags;
	duk_propdesc curr;
	duk_uint32_t arridx_new_array_length; /* != 0 => post-update for array 'length' (used when key is an array index) */
	duk_uint32_t arrlen_old_len;
	duk_uint32_t arrlen_new_len;
	duk_bool_t pending_write_protect;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(key != NULL);
	/* idx_value may be < 0 (no value), set and get may be NULL */

	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

	/* All the flags fit in 16 bits, so will fit into duk_bool_t. */

	has_writable = (defprop_flags & DUK_DEFPROP_HAVE_WRITABLE);
	has_enumerable = (defprop_flags & DUK_DEFPROP_HAVE_ENUMERABLE);
	has_configurable = (defprop_flags & DUK_DEFPROP_HAVE_CONFIGURABLE);
	has_value = (defprop_flags & DUK_DEFPROP_HAVE_VALUE);
	has_get = (defprop_flags & DUK_DEFPROP_HAVE_GETTER);
	has_set = (defprop_flags & DUK_DEFPROP_HAVE_SETTER);
	is_writable = (defprop_flags & DUK_DEFPROP_WRITABLE);
	is_enumerable = (defprop_flags & DUK_DEFPROP_ENUMERABLE);
	is_configurable = (defprop_flags & DUK_DEFPROP_CONFIGURABLE);
	force_flag = (defprop_flags & DUK_DEFPROP_FORCE);

	arr_idx = DUK_HSTRING_GET_ARRIDX_SLOW(key);

	arridx_new_array_length = 0;
	pending_write_protect = 0;
	arrlen_old_len = 0;
	arrlen_new_len = 0;

	DUK_DDD(DUK_DDDPRINT("has_enumerable=%ld is_enumerable=%ld "
	                     "has_configurable=%ld is_configurable=%ld "
	                     "has_writable=%ld is_writable=%ld "
	                     "has_value=%ld value=%!T "
	                     "has_get=%ld get=%p=%!O "
	                     "has_set=%ld set=%p=%!O "
	                     "arr_idx=%ld throw_flag=!%ld",
	                     (long) has_enumerable,
	                     (long) is_enumerable,
	                     (long) has_configurable,
	                     (long) is_configurable,
	                     (long) has_writable,
	                     (long) is_writable,
	                     (long) has_value,
	                     (duk_tval *) (idx_value >= 0 ? duk_get_tval(thr, idx_value) : NULL),
	                     (long) has_get,
	                     (void *) get,
	                     (duk_heaphdr *) get,
	                     (long) has_set,
	                     (void *) set,
	                     (duk_heaphdr *) set,
	                     (long) arr_idx,
	                     (long) throw_flag));

	/*
	 *  Array exotic behaviors can be implemented at this point.  The local variables
	 *  are essentially a 'value copy' of the input descriptor (Desc), which is modified
	 *  by the Array [[DefineOwnProperty]] (E5 Section 15.4.5.1).
	 */

	if (!DUK_HOBJECT_HAS_EXOTIC_ARRAY(obj)) {
		goto skip_array_exotic;
	}

	if (key == DUK_HTHREAD_STRING_LENGTH(thr)) {
		duk_harray *a;

		/* E5 Section 15.4.5.1, step 3, steps a - i are implemented here, j - n at the end */
		if (!has_value) {
			DUK_DDD(DUK_DDDPRINT("exotic array behavior for 'length', but no value in descriptor -> normal behavior"));
			goto skip_array_exotic;
		}

		DUK_DDD(DUK_DDDPRINT("exotic array behavior for 'length', value present in descriptor -> exotic behavior"));

		/*
		 *  Get old and new length
		 */

		a = (duk_harray *) obj;
		DUK_HARRAY_ASSERT_VALID(a);
		arrlen_old_len = a->length;

		DUK_ASSERT(idx_value >= 0);
		arrlen_new_len = duk__to_new_array_length_checked(thr, DUK_GET_TVAL_POSIDX(thr, idx_value));
		duk_push_u32(thr, arrlen_new_len);
		duk_replace(thr, idx_value); /* step 3.e: replace 'Desc.[[Value]]' */

		DUK_DDD(DUK_DDDPRINT("old_len=%ld, new_len=%ld", (long) arrlen_old_len, (long) arrlen_new_len));

		if (arrlen_new_len >= arrlen_old_len) {
			/* standard behavior, step 3.f.i */
			DUK_DDD(DUK_DDDPRINT("new length is same or higher as previous => standard behavior"));
			goto skip_array_exotic;
		}
		DUK_DDD(DUK_DDDPRINT("new length is smaller than previous => exotic post behavior"));

		/* XXX: consolidated algorithm step 15.f -> redundant? */
		if (DUK_HARRAY_LENGTH_NONWRITABLE(a) && !force_flag) {
			/* Array .length is always non-configurable; if it's also
			 * non-writable, don't allow it to be written.
			 */
			goto fail_not_configurable;
		}

		/* steps 3.h and 3.i */
		if (has_writable && !is_writable) {
			DUK_DDD(DUK_DDDPRINT("desc writable is false, force it back to true, and flag pending write protect"));
			is_writable = 1;
			pending_write_protect = 1;
		}

		/* remaining actual steps are carried out if standard DefineOwnProperty succeeds */
	} else if (arr_idx != DUK__NO_ARRAY_INDEX) {
		/* XXX: any chance of unifying this with the 'length' key handling? */

		/* E5 Section 15.4.5.1, step 4 */
		duk_uint32_t old_len;
		duk_harray *a;

		a = (duk_harray *) obj;
		DUK_HARRAY_ASSERT_VALID(a);

		old_len = a->length;

		if (arr_idx >= old_len) {
			DUK_DDD(DUK_DDDPRINT("defineProperty requires array length update "
			                     "(arr_idx=%ld, old_len=%ld)",
			                     (long) arr_idx,
			                     (long) old_len));

			if (DUK_HARRAY_LENGTH_NONWRITABLE(a) && !force_flag) {
				/* Array .length is always non-configurable, so
				 * if it's also non-writable, don't allow a value
				 * write.  With force flag allow writing.
				 */
				goto fail_not_configurable;
			}

			/* actual update happens once write has been completed without
			 * error below.
			 */
			DUK_ASSERT(arr_idx != 0xffffffffUL);
			arridx_new_array_length = arr_idx + 1;
		} else {
			DUK_DDD(DUK_DDDPRINT("defineProperty does not require length update "
			                     "(arr_idx=%ld, old_len=%ld) -> standard behavior",
			                     (long) arr_idx,
			                     (long) old_len));
		}
	}
skip_array_exotic:

	/* XXX: There is currently no support for writing buffer object
	 * indexed elements here.  Attempt to do so will succeed and
	 * write a concrete property into the buffer object.  This should
	 * be fixed at some point but because buffers are a custom feature
	 * anyway, this is relatively unimportant.
	 */

	/*
	 *  Actual Object.defineProperty() default algorithm.
	 */

	/*
	 *  First check whether property exists; if not, simple case.  This covers
	 *  steps 1-4.
	 */

	if (!duk__get_own_propdesc_raw(thr, obj, key, arr_idx, &curr, DUK_GETDESC_FLAG_PUSH_VALUE)) {
		DUK_DDD(DUK_DDDPRINT("property does not exist"));

		if (!DUK_HOBJECT_HAS_EXTENSIBLE(obj) && !force_flag) {
			goto fail_not_extensible;
		}

#if defined(DUK_USE_ROM_OBJECTS)
		/* ROM objects are never extensible but force flag may
		 * allow us to come here anyway.
		 */
		DUK_ASSERT(!DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) obj) || !DUK_HOBJECT_HAS_EXTENSIBLE(obj));
		if (DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) obj)) {
			DUK_D(DUK_DPRINT("attempt to define property on a read-only target object"));
			goto fail_not_configurable;
		}
#endif

		/* XXX: share final setting code for value and flags?  difficult because
		 * refcount code is different.  Share entry allocation?  But can't allocate
		 * until array index checked.
		 */

		/* steps 4.a and 4.b are tricky */
		if (has_set || has_get) {
			duk_int_t e_idx;

			DUK_DDD(DUK_DDDPRINT("create new accessor property"));

			DUK_ASSERT(has_set || set == NULL);
			DUK_ASSERT(has_get || get == NULL);
			DUK_ASSERT(!has_value);
			DUK_ASSERT(!has_writable);

			new_flags = DUK_PROPDESC_FLAG_ACCESSOR; /* defaults, E5 Section 8.6.1, Table 7 */
			if (has_enumerable && is_enumerable) {
				new_flags |= DUK_PROPDESC_FLAG_ENUMERABLE;
			}
			if (has_configurable && is_configurable) {
				new_flags |= DUK_PROPDESC_FLAG_CONFIGURABLE;
			}

			if (arr_idx != DUK__NO_ARRAY_INDEX && DUK_HOBJECT_HAS_ARRAY_PART(obj)) {
				DUK_DDD(DUK_DDDPRINT("accessor cannot go to array part, abandon array"));
				duk__abandon_array_part(thr, obj);
			}

			/* write to entry part */
			e_idx = duk__hobject_alloc_entry_checked(thr, obj, key);
			DUK_ASSERT(e_idx >= 0);

			DUK_HOBJECT_E_SET_VALUE_GETTER(thr->heap, obj, e_idx, get);
			DUK_HOBJECT_E_SET_VALUE_SETTER(thr->heap, obj, e_idx, set);
			DUK_HOBJECT_INCREF_ALLOWNULL(thr, get);
			DUK_HOBJECT_INCREF_ALLOWNULL(thr, set);

			DUK_HOBJECT_E_SET_FLAGS(thr->heap, obj, e_idx, new_flags);
			goto success_exotics;
		} else {
			duk_int_t e_idx;
			duk_tval *tv2;

			DUK_DDD(DUK_DDDPRINT("create new data property"));

			DUK_ASSERT(!has_set);
			DUK_ASSERT(!has_get);

			new_flags = 0; /* defaults, E5 Section 8.6.1, Table 7 */
			if (has_writable && is_writable) {
				new_flags |= DUK_PROPDESC_FLAG_WRITABLE;
			}
			if (has_enumerable && is_enumerable) {
				new_flags |= DUK_PROPDESC_FLAG_ENUMERABLE;
			}
			if (has_configurable && is_configurable) {
				new_flags |= DUK_PROPDESC_FLAG_CONFIGURABLE;
			}
			if (has_value) {
				duk_tval *tv_tmp = duk_require_tval(thr, idx_value);
				DUK_TVAL_SET_TVAL(&tv, tv_tmp);
			} else {
				DUK_TVAL_SET_UNDEFINED(&tv); /* default value */
			}

			if (arr_idx != DUK__NO_ARRAY_INDEX && DUK_HOBJECT_HAS_ARRAY_PART(obj)) {
				if (new_flags == DUK_PROPDESC_FLAGS_WEC) {
					DUK_DDD(DUK_DDDPRINT(
					    "new data property attributes match array defaults, attempt to write to array part"));
					tv2 = duk__obtain_arridx_slot(thr, arr_idx, obj);
					if (tv2 == NULL) {
						DUK_DDD(DUK_DDDPRINT("failed writing to array part, abandoned array"));
					} else {
						DUK_DDD(DUK_DDDPRINT("success in writing to array part"));
						DUK_ASSERT(DUK_HOBJECT_HAS_ARRAY_PART(obj));
						DUK_ASSERT(DUK_TVAL_IS_UNUSED(tv2));
						DUK_TVAL_SET_TVAL(tv2, &tv);
						DUK_TVAL_INCREF(thr, tv2);
						goto success_exotics;
					}
				} else {
					DUK_DDD(DUK_DDDPRINT("new data property cannot go to array part, abandon array"));
					duk__abandon_array_part(thr, obj);
				}
				/* fall through */
			}

			/* write to entry part */
			e_idx = duk__hobject_alloc_entry_checked(thr, obj, key);
			DUK_ASSERT(e_idx >= 0);
			tv2 = DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(thr->heap, obj, e_idx);
			DUK_TVAL_SET_TVAL(tv2, &tv);
			DUK_TVAL_INCREF(thr, tv2);

			DUK_HOBJECT_E_SET_FLAGS(thr->heap, obj, e_idx, new_flags);
			goto success_exotics;
		}
		DUK_UNREACHABLE();
	}

	/* we currently assume virtual properties are not configurable (as none of them are) */
	DUK_ASSERT((curr.e_idx >= 0 || curr.a_idx >= 0) || !(curr.flags & DUK_PROPDESC_FLAG_CONFIGURABLE));

	/* [obj key desc value get set curr_value] */

	/*
	 *  Property already exists.  Steps 5-6 detect whether any changes need
	 *  to be made.
	 */

	if (has_enumerable) {
		if (is_enumerable) {
			if (!(curr.flags & DUK_PROPDESC_FLAG_ENUMERABLE)) {
				goto need_check;
			}
		} else {
			if (curr.flags & DUK_PROPDESC_FLAG_ENUMERABLE) {
				goto need_check;
			}
		}
	}
	if (has_configurable) {
		if (is_configurable) {
			if (!(curr.flags & DUK_PROPDESC_FLAG_CONFIGURABLE)) {
				goto need_check;
			}
		} else {
			if (curr.flags & DUK_PROPDESC_FLAG_CONFIGURABLE) {
				goto need_check;
			}
		}
	}
	if (has_value) {
		duk_tval *tmp1;
		duk_tval *tmp2;

		/* attempt to change from accessor to data property */
		if (curr.flags & DUK_PROPDESC_FLAG_ACCESSOR) {
			goto need_check;
		}

		tmp1 = duk_require_tval(thr, -1); /* curr value */
		tmp2 = duk_require_tval(thr, idx_value); /* new value */
		if (!duk_js_samevalue(tmp1, tmp2)) {
			goto need_check;
		}
	}
	if (has_writable) {
		/* attempt to change from accessor to data property */
		if (curr.flags & DUK_PROPDESC_FLAG_ACCESSOR) {
			goto need_check;
		}

		if (is_writable) {
			if (!(curr.flags & DUK_PROPDESC_FLAG_WRITABLE)) {
				goto need_check;
			}
		} else {
			if (curr.flags & DUK_PROPDESC_FLAG_WRITABLE) {
				goto need_check;
			}
		}
	}
	if (has_set) {
		if (curr.flags & DUK_PROPDESC_FLAG_ACCESSOR) {
			if (set != curr.set) {
				goto need_check;
			}
		} else {
			goto need_check;
		}
	}
	if (has_get) {
		if (curr.flags & DUK_PROPDESC_FLAG_ACCESSOR) {
			if (get != curr.get) {
				goto need_check;
			}
		} else {
			goto need_check;
		}
	}

	/* property exists, either 'desc' is empty, or all values
	 * match (SameValue)
	 */
	goto success_no_exotics;

need_check:

	/*
	 *  Some change(s) need to be made.  Steps 7-11.
	 */

	/* shared checks for all descriptor types */
	if (!(curr.flags & DUK_PROPDESC_FLAG_CONFIGURABLE) && !force_flag) {
		if (has_configurable && is_configurable) {
			goto fail_not_configurable;
		}
		if (has_enumerable) {
			if (curr.flags & DUK_PROPDESC_FLAG_ENUMERABLE) {
				if (!is_enumerable) {
					goto fail_not_configurable;
				}
			} else {
				if (is_enumerable) {
					goto fail_not_configurable;
				}
			}
		}
	}

	/* Virtual properties don't have backing so they can't mostly be
	 * edited.  Some virtual properties are, however, writable: for
	 * example, virtual index properties of buffer objects and Array
	 * instance .length.  These are not configurable so the checks
	 * above mostly cover attempts to change them, except when the
	 * duk_def_prop() call is used with DUK_DEFPROP_FORCE; even in
	 * that case we can't forcibly change the property attributes
	 * because they don't have concrete backing.
	 */

	/* XXX: for ROM objects too it'd be best if value modify was
	 * allowed if the value matches SameValue.
	 */
	/* Reject attempt to change a read-only object. */
#if defined(DUK_USE_ROM_OBJECTS)
	if (DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) obj)) {
		DUK_DD(DUK_DDPRINT("attempt to define property on read-only target object"));
		goto fail_not_configurable;
	}
#endif

	/* descriptor type specific checks */
	if (has_set || has_get) {
		/* IsAccessorDescriptor(desc) == true */
		DUK_ASSERT(!has_writable);
		DUK_ASSERT(!has_value);

		if (curr.flags & DUK_PROPDESC_FLAG_ACCESSOR) {
			/* curr and desc are accessors */
			if (!(curr.flags & DUK_PROPDESC_FLAG_CONFIGURABLE) && !force_flag) {
				if (has_set && set != curr.set) {
					goto fail_not_configurable;
				}
				if (has_get && get != curr.get) {
					goto fail_not_configurable;
				}
			}
		} else {
			duk_bool_t rc;
			duk_tval *tv1;

			/* curr is data, desc is accessor */
			if (!(curr.flags & DUK_PROPDESC_FLAG_CONFIGURABLE) && !force_flag) {
				goto fail_not_configurable;
			}

			DUK_DDD(DUK_DDDPRINT("convert property to accessor property"));
			if (curr.a_idx >= 0) {
				DUK_DDD(
				    DUK_DDDPRINT("property to convert is stored in an array entry, abandon array and re-lookup"));
				duk__abandon_array_part(thr, obj);
				duk_pop_unsafe(thr); /* remove old value */
				rc = duk__get_own_propdesc_raw(thr, obj, key, arr_idx, &curr, DUK_GETDESC_FLAG_PUSH_VALUE);
				DUK_UNREF(rc);
				DUK_ASSERT(rc != 0);
				DUK_ASSERT(curr.e_idx >= 0 && curr.a_idx < 0);
			}
			if (curr.e_idx < 0) {
				DUK_ASSERT(curr.a_idx < 0 && curr.e_idx < 0);
				goto fail_virtual; /* safeguard for virtual property */
			}

			DUK_ASSERT(curr.e_idx >= 0);
			DUK_ASSERT(!DUK_HOBJECT_E_SLOT_IS_ACCESSOR(thr->heap, obj, curr.e_idx));

			tv1 = DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(thr->heap, obj, curr.e_idx);
			DUK_TVAL_SET_UNDEFINED_UPDREF_NORZ(thr, tv1); /* XXX: just decref */

			DUK_HOBJECT_E_SET_VALUE_GETTER(thr->heap, obj, curr.e_idx, NULL);
			DUK_HOBJECT_E_SET_VALUE_SETTER(thr->heap, obj, curr.e_idx, NULL);
			DUK_HOBJECT_E_SLOT_CLEAR_WRITABLE(thr->heap, obj, curr.e_idx);
			DUK_HOBJECT_E_SLOT_SET_ACCESSOR(thr->heap, obj, curr.e_idx);

			DUK_DDD(DUK_DDDPRINT("flags after data->accessor conversion: 0x%02lx",
			                     (unsigned long) DUK_HOBJECT_E_GET_FLAGS(thr->heap, obj, curr.e_idx)));
			/* Update curr.flags; faster than a re-lookup. */
			curr.flags &= ~DUK_PROPDESC_FLAG_WRITABLE;
			curr.flags |= DUK_PROPDESC_FLAG_ACCESSOR;
		}
	} else if (has_value || has_writable) {
		/* IsDataDescriptor(desc) == true */
		DUK_ASSERT(!has_set);
		DUK_ASSERT(!has_get);

		if (curr.flags & DUK_PROPDESC_FLAG_ACCESSOR) {
			duk_hobject *tmp;

			/* curr is accessor, desc is data */
			if (!(curr.flags & DUK_PROPDESC_FLAG_CONFIGURABLE) && !force_flag) {
				goto fail_not_configurable;
			}

			/* curr is accessor -> cannot be in array part. */
			DUK_ASSERT(curr.a_idx < 0);
			if (curr.e_idx < 0) {
				goto fail_virtual; /* safeguard; no virtual accessors now */
			}

			DUK_DDD(DUK_DDDPRINT("convert property to data property"));

			DUK_ASSERT(DUK_HOBJECT_E_SLOT_IS_ACCESSOR(thr->heap, obj, curr.e_idx));
			tmp = DUK_HOBJECT_E_GET_VALUE_GETTER(thr->heap, obj, curr.e_idx);
			DUK_UNREF(tmp);
			DUK_HOBJECT_E_SET_VALUE_GETTER(thr->heap, obj, curr.e_idx, NULL);
			DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, tmp);
			tmp = DUK_HOBJECT_E_GET_VALUE_SETTER(thr->heap, obj, curr.e_idx);
			DUK_UNREF(tmp);
			DUK_HOBJECT_E_SET_VALUE_SETTER(thr->heap, obj, curr.e_idx, NULL);
			DUK_HOBJECT_DECREF_NORZ_ALLOWNULL(thr, tmp);

			DUK_TVAL_SET_UNDEFINED(DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(thr->heap, obj, curr.e_idx));
			DUK_HOBJECT_E_SLOT_CLEAR_WRITABLE(thr->heap, obj, curr.e_idx);
			DUK_HOBJECT_E_SLOT_CLEAR_ACCESSOR(thr->heap, obj, curr.e_idx);

			DUK_DDD(DUK_DDDPRINT("flags after accessor->data conversion: 0x%02lx",
			                     (unsigned long) DUK_HOBJECT_E_GET_FLAGS(thr->heap, obj, curr.e_idx)));

			/* Update curr.flags; faster than a re-lookup. */
			curr.flags &= ~(DUK_PROPDESC_FLAG_WRITABLE | DUK_PROPDESC_FLAG_ACCESSOR);
		} else {
			/* curr and desc are data */
			if (!(curr.flags & DUK_PROPDESC_FLAG_CONFIGURABLE) && !force_flag) {
				if (!(curr.flags & DUK_PROPDESC_FLAG_WRITABLE) && has_writable && is_writable) {
					goto fail_not_configurable;
				}
				/* Note: changing from writable to non-writable is OK */
				if (!(curr.flags & DUK_PROPDESC_FLAG_WRITABLE) && has_value) {
					duk_tval *tmp1 = duk_require_tval(thr, -1); /* curr value */
					duk_tval *tmp2 = duk_require_tval(thr, idx_value); /* new value */
					if (!duk_js_samevalue(tmp1, tmp2)) {
						goto fail_not_configurable;
					}
				}
			}
		}
	} else {
		/* IsGenericDescriptor(desc) == true; this means in practice that 'desc'
		 * only has [[Enumerable]] or [[Configurable]] flag updates, which are
		 * allowed at this point.
		 */

		DUK_ASSERT(!has_value && !has_writable && !has_get && !has_set);
	}

	/*
	 *  Start doing property attributes updates.  Steps 12-13.
	 *
	 *  Start by computing new attribute flags without writing yet.
	 *  Property type conversion is done above if necessary.
	 */

	new_flags = curr.flags;

	if (has_enumerable) {
		if (is_enumerable) {
			new_flags |= DUK_PROPDESC_FLAG_ENUMERABLE;
		} else {
			new_flags &= ~DUK_PROPDESC_FLAG_ENUMERABLE;
		}
	}
	if (has_configurable) {
		if (is_configurable) {
			new_flags |= DUK_PROPDESC_FLAG_CONFIGURABLE;
		} else {
			new_flags &= ~DUK_PROPDESC_FLAG_CONFIGURABLE;
		}
	}
	if (has_writable) {
		if (is_writable) {
			new_flags |= DUK_PROPDESC_FLAG_WRITABLE;
		} else {
			new_flags &= ~DUK_PROPDESC_FLAG_WRITABLE;
		}
	}

	/* XXX: write protect after flag? -> any chance of handling it here? */

	DUK_DDD(DUK_DDDPRINT("new flags that we want to write: 0x%02lx", (unsigned long) new_flags));

	/*
	 *  Check whether we need to abandon an array part (if it exists)
	 */

	if (curr.a_idx >= 0) {
		duk_bool_t rc;

		DUK_ASSERT(curr.e_idx < 0);

		if (new_flags == DUK_PROPDESC_FLAGS_WEC) {
			duk_tval *tv1, *tv2;

			DUK_DDD(DUK_DDDPRINT("array index, new property attributes match array defaults, update in-place"));

			DUK_ASSERT(curr.flags == DUK_PROPDESC_FLAGS_WEC); /* must have been, since in array part */
			DUK_ASSERT(!has_set);
			DUK_ASSERT(!has_get);
			DUK_ASSERT(
			    idx_value >=
			    0); /* must be: if attributes match and we get here the value must differ (otherwise no change) */

			tv2 = duk_require_tval(thr, idx_value);
			tv1 = DUK_HOBJECT_A_GET_VALUE_PTR(thr->heap, obj, curr.a_idx);
			DUK_TVAL_SET_TVAL_UPDREF(thr, tv1, tv2); /* side effects; may invalidate a_idx */
			goto success_exotics;
		}

		DUK_DDD(
		    DUK_DDDPRINT("array index, new property attributes do not match array defaults, abandon array and re-lookup"));
		duk__abandon_array_part(thr, obj);
		duk_pop_unsafe(thr); /* remove old value */
		rc = duk__get_own_propdesc_raw(thr, obj, key, arr_idx, &curr, DUK_GETDESC_FLAG_PUSH_VALUE);
		DUK_UNREF(rc);
		DUK_ASSERT(rc != 0);
		DUK_ASSERT(curr.e_idx >= 0 && curr.a_idx < 0);
	}

	DUK_DDD(DUK_DDDPRINT("updating existing property in entry part"));

	/* Array case is handled comprehensively above: either in entry
	 * part or a virtual property.
	 */
	DUK_ASSERT(curr.a_idx < 0);

	DUK_DDD(DUK_DDDPRINT("update existing property attributes"));
	if (curr.e_idx >= 0) {
		DUK_HOBJECT_E_SET_FLAGS(thr->heap, obj, curr.e_idx, new_flags);
	} else {
		/* For Array .length the only allowed transition is for .length
		 * to become non-writable.
		 */
		if (key == DUK_HTHREAD_STRING_LENGTH(thr) && DUK_HOBJECT_HAS_EXOTIC_ARRAY(obj)) {
			duk_harray *a;
			a = (duk_harray *) obj;
			DUK_DD(DUK_DDPRINT("Object.defineProperty() attribute update for duk_harray .length -> %02lx",
			                   (unsigned long) new_flags));
			DUK_HARRAY_ASSERT_VALID(a);
			if ((new_flags & DUK_PROPDESC_FLAGS_EC) != (curr.flags & DUK_PROPDESC_FLAGS_EC)) {
				DUK_D(DUK_DPRINT("Object.defineProperty() attempt to change virtual array .length enumerable or "
				                 "configurable attribute, fail"));
				goto fail_virtual;
			}
			if (new_flags & DUK_PROPDESC_FLAG_WRITABLE) {
				DUK_HARRAY_SET_LENGTH_WRITABLE(a);
			} else {
				DUK_HARRAY_SET_LENGTH_NONWRITABLE(a);
			}
		}
	}

	if (has_set) {
		duk_hobject *tmp;

		/* Virtual properties are non-configurable but with a 'force'
		 * flag we might come here so check explicitly for virtual.
		 */
		if (curr.e_idx < 0) {
			goto fail_virtual;
		}

		DUK_DDD(DUK_DDDPRINT("update existing property setter"));
		DUK_ASSERT(DUK_HOBJECT_E_SLOT_IS_ACCESSOR(thr->heap, obj, curr.e_idx));

		tmp = DUK_HOBJECT_E_GET_VALUE_SETTER(thr->heap, obj, curr.e_idx);
		DUK_UNREF(tmp);
		DUK_HOBJECT_E_SET_VALUE_SETTER(thr->heap, obj, curr.e_idx, set);
		DUK_HOBJECT_INCREF_ALLOWNULL(thr, set);
		DUK_HOBJECT_DECREF_ALLOWNULL(thr, tmp); /* side effects; may invalidate e_idx */
	}
	if (has_get) {
		duk_hobject *tmp;

		if (curr.e_idx < 0) {
			goto fail_virtual;
		}

		DUK_DDD(DUK_DDDPRINT("update existing property getter"));
		DUK_ASSERT(DUK_HOBJECT_E_SLOT_IS_ACCESSOR(thr->heap, obj, curr.e_idx));

		tmp = DUK_HOBJECT_E_GET_VALUE_GETTER(thr->heap, obj, curr.e_idx);
		DUK_UNREF(tmp);
		DUK_HOBJECT_E_SET_VALUE_GETTER(thr->heap, obj, curr.e_idx, get);
		DUK_HOBJECT_INCREF_ALLOWNULL(thr, get);
		DUK_HOBJECT_DECREF_ALLOWNULL(thr, tmp); /* side effects; may invalidate e_idx */
	}
	if (has_value) {
		duk_tval *tv1, *tv2;

		DUK_DDD(DUK_DDDPRINT("update existing property value"));

		if (curr.e_idx >= 0) {
			DUK_ASSERT(!DUK_HOBJECT_E_SLOT_IS_ACCESSOR(thr->heap, obj, curr.e_idx));
			tv2 = duk_require_tval(thr, idx_value);
			tv1 = DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(thr->heap, obj, curr.e_idx);
			DUK_TVAL_SET_TVAL_UPDREF(thr, tv1, tv2); /* side effects; may invalidate e_idx */
		} else {
			DUK_ASSERT(curr.a_idx < 0); /* array part case handled comprehensively previously */

			DUK_DD(DUK_DDPRINT("Object.defineProperty(), value update for virtual property"));
			/* XXX: Uint8Array and other typed array virtual writes not currently
			 * handled.
			 */
			if (key == DUK_HTHREAD_STRING_LENGTH(thr) && DUK_HOBJECT_HAS_EXOTIC_ARRAY(obj)) {
				duk_harray *a;
				a = (duk_harray *) obj;
				DUK_DD(DUK_DDPRINT("Object.defineProperty() value update for duk_harray .length -> %ld",
				                   (long) arrlen_new_len));
				DUK_HARRAY_ASSERT_VALID(a);
				a->length = arrlen_new_len;
			} else {
				goto fail_virtual; /* should not happen */
			}
		}
	}

	/*
	 *  Standard algorithm succeeded without errors, check for exotic post-behaviors.
	 *
	 *  Arguments exotic behavior in E5 Section 10.6 occurs after the standard
	 *  [[DefineOwnProperty]] has completed successfully.
	 *
	 *  Array exotic behavior in E5 Section 15.4.5.1 is implemented partly
	 *  prior to the default [[DefineOwnProperty]], but:
	 *    - for an array index key (e.g. "10") the final 'length' update occurs here
	 *    - for 'length' key the element deletion and 'length' update occurs here
	 */

success_exotics:

	/* curr.a_idx or curr.e_idx may have been invalidated by side effects
	 * above.
	 */

	/* [obj key desc value get set curr_value] */

	if (DUK_HOBJECT_HAS_EXOTIC_ARRAY(obj)) {
		duk_harray *a;

		a = (duk_harray *) obj;
		DUK_HARRAY_ASSERT_VALID(a);

		if (arridx_new_array_length > 0) {
			/*
			 *  Note: zero works as a "no update" marker because the new length
			 *  can never be zero after a new property is written.
			 */

			/* E5 Section 15.4.5.1, steps 4.e.i - 4.e.ii */

			DUK_DDD(DUK_DDDPRINT("defineProperty successful, pending array length update to: %ld",
			                     (long) arridx_new_array_length));

			a->length = arridx_new_array_length;
		}

		if (key == DUK_HTHREAD_STRING_LENGTH(thr) && arrlen_new_len < arrlen_old_len) {
			/*
			 *  E5 Section 15.4.5.1, steps 3.k - 3.n.  The order at the end combines
			 *  the error case 3.l.iii and the success case 3.m-3.n.
			 */

			/* XXX: investigate whether write protect can be handled above, if we
			 * just update length here while ignoring its protected status
			 */

			duk_uint32_t result_len;
			duk_bool_t rc;

			DUK_DDD(DUK_DDDPRINT("defineProperty successful, key is 'length', exotic array behavior, "
			                     "doing array element deletion and length update"));

			rc =
			    duk__handle_put_array_length_smaller(thr, obj, arrlen_old_len, arrlen_new_len, force_flag, &result_len);

			/* update length (curr points to length, and we assume it's still valid) */
			DUK_ASSERT(result_len >= arrlen_new_len && result_len <= arrlen_old_len);

			a->length = result_len;

			if (pending_write_protect) {
				DUK_DDD(DUK_DDDPRINT("setting array length non-writable (pending writability update)"));
				DUK_HARRAY_SET_LENGTH_NONWRITABLE(a);
			}

			/* XXX: shrink array allocation or entries compaction here? */
			if (!rc) {
				DUK_DD(DUK_DDPRINT("array length write only partially successful"));
				goto fail_not_configurable;
			}
		}
	} else if (arr_idx != DUK__NO_ARRAY_INDEX && DUK_HOBJECT_HAS_EXOTIC_ARGUMENTS(obj)) {
		duk_hobject *map;
		duk_hobject *varenv;

		DUK_ASSERT(arridx_new_array_length == 0);
		DUK_ASSERT(!DUK_HOBJECT_HAS_EXOTIC_ARRAY(obj)); /* traits are separate; in particular, arguments not an array */

		map = NULL;
		varenv = NULL;
		if (!duk__lookup_arguments_map(thr, obj, key, &curr, &map, &varenv)) {
			goto success_no_exotics;
		}
		DUK_ASSERT(map != NULL);
		DUK_ASSERT(varenv != NULL);

		/* [obj key desc value get set curr_value varname] */

		if (has_set || has_get) {
			/* = IsAccessorDescriptor(Desc) */
			DUK_DDD(DUK_DDDPRINT("defineProperty successful, key mapped to arguments 'map' "
			                     "changed to an accessor, delete arguments binding"));

			(void) duk_hobject_delprop_raw(thr, map, key, 0); /* ignore result */
		} else {
			/* Note: this order matters (final value before deleting map entry must be done) */
			DUK_DDD(DUK_DDDPRINT("defineProperty successful, key mapped to arguments 'map', "
			                     "check for value update / binding deletion"));

			if (has_value) {
				duk_hstring *varname;

				DUK_DDD(DUK_DDDPRINT("defineProperty successful, key mapped to arguments 'map', "
				                     "update bound value (variable/argument)"));

				varname = duk_require_hstring(thr, -1);
				DUK_ASSERT(varname != NULL);

				DUK_DDD(DUK_DDDPRINT("arguments object automatic putvar for a bound variable; "
				                     "key=%!O, varname=%!O, value=%!T",
				                     (duk_heaphdr *) key,
				                     (duk_heaphdr *) varname,
				                     (duk_tval *) duk_require_tval(thr, idx_value)));

				/* strict flag for putvar comes from our caller (currently: fixed) */
				duk_js_putvar_envrec(thr, varenv, varname, duk_require_tval(thr, idx_value), 1 /*throw_flag*/);
			}
			if (has_writable && !is_writable) {
				DUK_DDD(DUK_DDDPRINT("defineProperty successful, key mapped to arguments 'map', "
				                     "changed to non-writable, delete arguments binding"));

				(void) duk_hobject_delprop_raw(thr, map, key, 0); /* ignore result */
			}
		}

		/* 'varname' is in stack in this else branch, leaving an unbalanced stack below,
		 * but this doesn't matter now.
		 */
	}

success_no_exotics:
	/* Some code paths use NORZ macros for simplicity, ensure refzero
	 * handling is completed.
	 */
	DUK_REFZERO_CHECK_SLOW(thr);
	return 1;

fail_not_extensible:
	if (throw_flag) {
		DUK_ERROR_TYPE(thr, DUK_STR_NOT_EXTENSIBLE);
		DUK_WO_NORETURN(return 0;);
	}
	return 0;

fail_virtual: /* just use the same "not configurable" error message" */
fail_not_configurable:
	if (throw_flag) {
		DUK_ERROR_TYPE(thr, DUK_STR_NOT_CONFIGURABLE);
		DUK_WO_NORETURN(return 0;);
	}
	return 0;
}

/*
 *  Object.prototype.hasOwnProperty() and Object.prototype.propertyIsEnumerable().
 */

DUK_INTERNAL duk_bool_t duk_hobject_object_ownprop_helper(duk_hthread *thr, duk_small_uint_t required_desc_flags) {
	duk_hstring *h_v;
	duk_hobject *h_obj;
	duk_propdesc desc;
	duk_bool_t ret;

	/* coercion order matters */
	h_v = duk_to_hstring_acceptsymbol(thr, 0);
	DUK_ASSERT(h_v != NULL);

	h_obj = duk_push_this_coercible_to_object(thr);
	DUK_ASSERT(h_obj != NULL);

	ret = duk_hobject_get_own_propdesc(thr, h_obj, h_v, &desc, 0 /*flags*/); /* don't push value */

	duk_push_boolean(thr, ret && ((desc.flags & required_desc_flags) == required_desc_flags));
	return 1;
}

/*
 *  Object.seal() and Object.freeze()  (E5 Sections 15.2.3.8 and 15.2.3.9)
 *
 *  Since the algorithms are similar, a helper provides both functions.
 *  Freezing is essentially sealing + making plain properties non-writable.
 *
 *  Note: virtual (non-concrete) properties which are non-configurable but
 *  writable would pose some problems, but such properties do not currently
 *  exist (all virtual properties are non-configurable and non-writable).
 *  If they did exist, the non-configurability does NOT prevent them from
 *  becoming non-writable.  However, this change should be recorded somehow
 *  so that it would turn up (e.g. when getting the property descriptor),
 *  requiring some additional flags in the object.
 */

DUK_INTERNAL void duk_hobject_object_seal_freeze_helper(duk_hthread *thr, duk_hobject *obj, duk_bool_t is_freeze) {
	duk_uint_fast32_t i;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(obj != NULL);

	DUK_ASSERT_VALSTACK_SPACE(thr, DUK__VALSTACK_SPACE);

#if defined(DUK_USE_ROM_OBJECTS)
	if (DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) obj)) {
		DUK_DD(DUK_DDPRINT("attempt to seal/freeze a readonly object, reject"));
		DUK_ERROR_TYPE(thr, DUK_STR_NOT_CONFIGURABLE);
		DUK_WO_NORETURN(return;);
	}
#endif

	/*
	 *  Abandon array part because all properties must become non-configurable.
	 *  Note that this is now done regardless of whether this is always the case
	 *  (skips check, but performance problem if caller would do this many times
	 *  for the same object; not likely).
	 */

	duk__abandon_array_part(thr, obj);
	DUK_ASSERT(DUK_HOBJECT_GET_ASIZE(obj) == 0);

	for (i = 0; i < DUK_HOBJECT_GET_ENEXT(obj); i++) {
		duk_uint8_t *fp;

		/* since duk__abandon_array_part() causes a resize, there should be no gaps in keys */
		DUK_ASSERT(DUK_HOBJECT_E_GET_KEY(thr->heap, obj, i) != NULL);

		/* avoid multiple computations of flags address; bypasses macros */
		fp = DUK_HOBJECT_E_GET_FLAGS_PTR(thr->heap, obj, i);
		if (is_freeze && !((*fp) & DUK_PROPDESC_FLAG_ACCESSOR)) {
			*fp &= ~(DUK_PROPDESC_FLAG_WRITABLE | DUK_PROPDESC_FLAG_CONFIGURABLE);
		} else {
			*fp &= ~DUK_PROPDESC_FLAG_CONFIGURABLE;
		}
	}

	DUK_HOBJECT_CLEAR_EXTENSIBLE(obj);

	/* no need to compact since we already did that in duk__abandon_array_part()
	 * (regardless of whether an array part existed or not.
	 */

	return;
}

/*
 *  Object.isSealed() and Object.isFrozen()  (E5 Sections 15.2.3.11, 15.2.3.13)
 *
 *  Since the algorithms are similar, a helper provides both functions.
 *  Freezing is essentially sealing + making plain properties non-writable.
 *
 *  Note: all virtual (non-concrete) properties are currently non-configurable
 *  and non-writable (and there are no accessor virtual properties), so they don't
 *  need to be considered here now.
 */

DUK_INTERNAL duk_bool_t duk_hobject_object_is_sealed_frozen_helper(duk_hthread *thr, duk_hobject *obj, duk_bool_t is_frozen) {
	duk_uint_fast32_t i;

	DUK_ASSERT(obj != NULL);
	DUK_UNREF(thr);

	/* Note: no allocation pressure, no need to check refcounts etc */

	/* must not be extensible */
	if (DUK_HOBJECT_HAS_EXTENSIBLE(obj)) {
		return 0;
	}

	/* all virtual properties are non-configurable and non-writable */

	/* entry part must not contain any configurable properties, or
	 * writable properties (if is_frozen).
	 */
	for (i = 0; i < DUK_HOBJECT_GET_ENEXT(obj); i++) {
		duk_small_uint_t flags;

		if (!DUK_HOBJECT_E_GET_KEY(thr->heap, obj, i)) {
			continue;
		}

		/* avoid multiple computations of flags address; bypasses macros */
		flags = (duk_small_uint_t) DUK_HOBJECT_E_GET_FLAGS(thr->heap, obj, i);

		if (flags & DUK_PROPDESC_FLAG_CONFIGURABLE) {
			return 0;
		}
		if (is_frozen && !(flags & DUK_PROPDESC_FLAG_ACCESSOR) && (flags & DUK_PROPDESC_FLAG_WRITABLE)) {
			return 0;
		}
	}

	/* array part must not contain any non-unused properties, as they would
	 * be configurable and writable.
	 */
	for (i = 0; i < DUK_HOBJECT_GET_ASIZE(obj); i++) {
		duk_tval *tv = DUK_HOBJECT_A_GET_VALUE_PTR(thr->heap, obj, i);
		if (!DUK_TVAL_IS_UNUSED(tv)) {
			return 0;
		}
	}

	return 1;
}

/*
 *  Object.preventExtensions() and Object.isExtensible()  (E5 Sections 15.2.3.10, 15.2.3.13)
 *
 *  Not needed, implemented by macros DUK_HOBJECT_{HAS,CLEAR,SET}_EXTENSIBLE
 *  and the Object built-in bindings.
 */
