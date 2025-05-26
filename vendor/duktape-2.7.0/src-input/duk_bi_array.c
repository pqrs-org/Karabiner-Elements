/*
 *  Array built-ins
 *
 *  Most Array built-ins are intentionally generic in ECMAScript, and are
 *  intended to work even when the 'this' binding is not an Array instance.
 *  This ECMAScript feature is also used by much real world code.  For this
 *  reason the implementations here don't assume exotic Array behavior or
 *  e.g. presence of a .length property.  However, some algorithms have a
 *  fast path for duk_harray backed actual Array instances, enabled when
 *  footprint is not a concern.
 *
 *  XXX: the "Throw" flag should be set for (almost?) all [[Put]] and
 *  [[Delete]] operations, but it's currently false throughout.  Go through
 *  all put/delete cases and check throw flag use.  Need a new API primitive
 *  which allows throws flag to be specified.
 *
 *  XXX: array lengths above 2G won't work reliably.  There are many places
 *  where one needs a full signed 32-bit range ([-0xffffffff, 0xffffffff],
 *  i.e. -33- bits).  Although array 'length' cannot be written to be outside
 *  the unsigned 32-bit range (E5.1 Section 15.4.5.1 throws a RangeError if so)
 *  some intermediate values may be above 0xffffffff and this may not be always
 *  correctly handled now (duk_uint32_t is not enough for all algorithms).
 *  For instance, push() can legitimately write entries beyond length 0xffffffff
 *  and cause a RangeError only at the end.  To do this properly, the current
 *  push() implementation tracks the array index using a 'double' instead of a
 *  duk_uint32_t (which is somewhat awkward).  See test-bi-array-push-maxlen.js.
 *
 *  On using "put" vs. "def" prop
 *  =============================
 *
 *  Code below must be careful to use the appropriate primitive as it matters
 *  for compliance.  When using "put" there may be inherited properties in
 *  Array.prototype which cause side effects when values are written.  When
 *  using "define" there are no such side effects, and many test262 test cases
 *  check for this (for real world code, such side effects are very rare).
 *  Both "put" and "define" are used in the E5.1 specification; as a rule,
 *  "put" is used when modifying an existing array (or a non-array 'this'
 *  binding) and "define" for setting values into a fresh result array.
 */

#include "duk_internal.h"

/* Perform an intermediate join when this many elements have been pushed
 * on the value stack.
 */
#define DUK__ARRAY_MID_JOIN_LIMIT 4096

#if defined(DUK_USE_ARRAY_BUILTIN)

/*
 *  Shared helpers.
 */

/* Shared entry code for many Array built-ins: the 'this' binding is pushed
 * on the value stack and object coerced, and the current .length is returned.
 * Note that length is left on stack (it could be popped, but that's not
 * usually necessary because call handling will clean it up automatically).
 */
DUK_LOCAL duk_uint32_t duk__push_this_obj_len_u32(duk_hthread *thr) {
	duk_uint32_t len;

	/* XXX: push more directly? */
	(void) duk_push_this_coercible_to_object(thr);
	DUK_HOBJECT_ASSERT_VALID(duk_get_hobject(thr, -1));
	duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_LENGTH);
	len = duk_to_uint32(thr, -1);

	/* -> [ ... ToObject(this) ToUint32(length) ] */
	return len;
}

DUK_LOCAL duk_uint32_t duk__push_this_obj_len_u32_limited(duk_hthread *thr) {
	/* Range limited to [0, 0x7fffffff] range, i.e. range that can be
	 * represented with duk_int32_t.  Use this when the method doesn't
	 * handle the full 32-bit unsigned range correctly.
	 */
	duk_uint32_t ret = duk__push_this_obj_len_u32(thr);
	if (DUK_UNLIKELY(ret >= 0x80000000UL)) {
		DUK_ERROR_RANGE_INVALID_LENGTH(thr);
		DUK_WO_NORETURN(return 0U;);
	}
	return ret;
}

#if defined(DUK_USE_ARRAY_FASTPATH)
/* Check if 'this' binding is an Array instance (duk_harray) which satisfies
 * a few other guarantees for fast path operation.  The fast path doesn't
 * need to handle all operations, even for duk_harrays, but must handle a
 * significant fraction to improve performance.  Return a non-NULL duk_harray
 * pointer when all fast path criteria are met, NULL otherwise.
 */
DUK_LOCAL duk_harray *duk__arraypart_fastpath_this(duk_hthread *thr) {
	duk_tval *tv;
	duk_hobject *h;
	duk_uint_t flags_mask, flags_bits, flags_value;

	DUK_ASSERT(thr->valstack_bottom > thr->valstack); /* because call in progress */
	tv = DUK_GET_THIS_TVAL_PTR(thr);

	/* Fast path requires that 'this' is a duk_harray.  Read only arrays
	 * (ROM backed) are also rejected for simplicity.
	 */
	if (!DUK_TVAL_IS_OBJECT(tv)) {
		DUK_DD(DUK_DDPRINT("reject array fast path: not an object"));
		return NULL;
	}
	h = DUK_TVAL_GET_OBJECT(tv);
	DUK_ASSERT(h != NULL);
	flags_mask = DUK_HOBJECT_FLAG_ARRAY_PART | DUK_HOBJECT_FLAG_EXOTIC_ARRAY | DUK_HEAPHDR_FLAG_READONLY;
	flags_bits = DUK_HOBJECT_FLAG_ARRAY_PART | DUK_HOBJECT_FLAG_EXOTIC_ARRAY;
	flags_value = DUK_HEAPHDR_GET_FLAGS_RAW((duk_heaphdr *) h);
	if ((flags_value & flags_mask) != flags_bits) {
		DUK_DD(DUK_DDPRINT("reject array fast path: object flag check failed"));
		return NULL;
	}

	/* In some cases a duk_harray's 'length' may be larger than the
	 * current array part allocation.  Avoid the fast path in these
	 * cases, so that all fast path code can safely assume that all
	 * items in the range [0,length[ are backed by the current array
	 * part allocation.
	 */
	if (((duk_harray *) h)->length > DUK_HOBJECT_GET_ASIZE(h)) {
		DUK_DD(DUK_DDPRINT("reject array fast path: length > array part size"));
		return NULL;
	}

	/* Guarantees for fast path. */
	DUK_ASSERT(h != NULL);
	DUK_ASSERT(DUK_HOBJECT_GET_ASIZE(h) == 0 || DUK_HOBJECT_A_GET_BASE(thr->heap, h) != NULL);
	DUK_ASSERT(((duk_harray *) h)->length <= DUK_HOBJECT_GET_ASIZE(h));

	DUK_DD(DUK_DDPRINT("array fast path allowed for: %!O", (duk_heaphdr *) h));
	return (duk_harray *) h;
}
#endif /* DUK_USE_ARRAY_FASTPATH */

/*
 *  Constructor
 */

DUK_INTERNAL duk_ret_t duk_bi_array_constructor(duk_hthread *thr) {
	duk_idx_t nargs;
	duk_harray *a;
	duk_double_t d;
	duk_uint32_t len;
	duk_uint32_t len_prealloc;

	nargs = duk_get_top(thr);

	if (nargs == 1 && duk_is_number(thr, 0)) {
		/* XXX: expensive check (also shared elsewhere - so add a shared internal API call?) */
		d = duk_get_number(thr, 0);
		len = duk_to_uint32(thr, 0);
		if (!duk_double_equals((duk_double_t) len, d)) {
			DUK_DCERROR_RANGE_INVALID_LENGTH(thr);
		}

		/* For small lengths create a dense preallocated array.
		 * For large arrays preallocate an initial part.
		 */
		len_prealloc = len < 64 ? len : 64;
		a = duk_push_harray_with_size(thr, len_prealloc);
		DUK_ASSERT(a != NULL);
		DUK_ASSERT(!duk_is_bare_object(thr, -1));
		a->length = len;
		return 1;
	}

	duk_pack(thr, nargs);
	return 1;
}

/*
 *  isArray()
 */

DUK_INTERNAL duk_ret_t duk_bi_array_constructor_is_array(duk_hthread *thr) {
	DUK_ASSERT_TOP(thr, 1);
	duk_push_boolean(thr, duk_js_isarray(DUK_GET_TVAL_POSIDX(thr, 0)));
	return 1;
}

/*
 *  toString()
 */

DUK_INTERNAL duk_ret_t duk_bi_array_prototype_to_string(duk_hthread *thr) {
	(void) duk_push_this_coercible_to_object(thr);
	duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_JOIN);

	/* [ ... this func ] */
	if (!duk_is_callable(thr, -1)) {
		/* Fall back to the initial (original) Object.toString().  We don't
		 * currently have pointers to the built-in functions, only the top
		 * level global objects (like "Array") so this is now done in a bit
		 * of a hacky manner.  It would be cleaner to push the (original)
		 * function and use duk_call_method().
		 */

		/* XXX: 'this' will be ToObject() coerced twice, which is incorrect
		 * but should have no visible side effects.
		 */
		DUK_DDD(DUK_DDDPRINT("this.join is not callable, fall back to (original) Object.toString"));
		duk_set_top(thr, 0);
		return duk_bi_object_prototype_to_string(thr); /* has access to 'this' binding */
	}

	/* [ ... this func ] */

	duk_insert(thr, -2);

	/* [ ... func this ] */

	DUK_DDD(
	    DUK_DDDPRINT("calling: func=%!iT, this=%!iT", (duk_tval *) duk_get_tval(thr, -2), (duk_tval *) duk_get_tval(thr, -1)));
	duk_call_method(thr, 0);

	return 1;
}

/*
 *  concat()
 */

DUK_INTERNAL duk_ret_t duk_bi_array_prototype_concat(duk_hthread *thr) {
	duk_idx_t i, n;
	duk_uint32_t j, idx, len;
	duk_hobject *h;
	duk_size_t tmp_len;

	/* XXX: In ES2015 Array .length can be up to 2^53-1.  The current
	 * implementation is limited to 2^32-1.
	 */

	/* XXX: Fast path for array 'this' and array element. */

	/* XXX: The insert here is a bit expensive if there are a lot of items.
	 * It could also be special cased in the outermost for loop quite easily
	 * (as the element is dup()'d anyway).
	 */

	(void) duk_push_this_coercible_to_object(thr);
	duk_insert(thr, 0);
	n = duk_get_top(thr);
	duk_push_array(thr); /* -> [ ToObject(this) item1 ... itemN arr ] */

	/* NOTE: The Array special behaviors are NOT invoked by duk_xdef_prop_index()
	 * (which differs from the official algorithm).  If no error is thrown, this
	 * doesn't matter as the length is updated at the end.  However, if an error
	 * is thrown, the length will be unset.  That shouldn't matter because the
	 * caller won't get a reference to the intermediate value.
	 */

	idx = 0;
	for (i = 0; i < n; i++) {
		duk_bool_t spreadable;
		duk_bool_t need_has_check;

		DUK_ASSERT_TOP(thr, n + 1);

		/* [ ToObject(this) item1 ... itemN arr ] */

		h = duk_get_hobject(thr, i);

		if (h == NULL) {
			spreadable = 0;
		} else {
#if defined(DUK_USE_SYMBOL_BUILTIN)
			duk_get_prop_stridx(thr, i, DUK_STRIDX_WELLKNOWN_SYMBOL_IS_CONCAT_SPREADABLE);
			if (duk_is_undefined(thr, -1)) {
				spreadable = duk_js_isarray_hobject(h);
			} else {
				spreadable = duk_to_boolean(thr, -1);
			}
			duk_pop_nodecref_unsafe(thr);
#else
			spreadable = duk_js_isarray_hobject(h);
#endif
		}

		if (!spreadable) {
			duk_dup(thr, i);
			duk_xdef_prop_index_wec(thr, -2, idx);
			idx++;
			if (DUK_UNLIKELY(idx == 0U)) {
				/* Index after update is 0, and index written
				 * was 0xffffffffUL which is no longer a valid
				 * array index.
				 */
				goto fail_wrap;
			}
			continue;
		}

		DUK_ASSERT(duk_is_object(thr, i));
		need_has_check = (DUK_HOBJECT_IS_PROXY(h) != 0); /* Always 0 w/o Proxy support. */

		/* [ ToObject(this) item1 ... itemN arr ] */

		tmp_len = duk_get_length(thr, i);
		len = (duk_uint32_t) tmp_len;
		if (DUK_UNLIKELY(tmp_len != (duk_size_t) len)) {
			goto fail_wrap;
		}
		if (DUK_UNLIKELY(idx + len < idx)) {
			/* Result length must be at most 0xffffffffUL to be
			 * a valid 32-bit array index.
			 */
			goto fail_wrap;
		}
		for (j = 0; j < len; j++) {
			/* For a Proxy element, an explicit 'has' check is
			 * needed to allow the Proxy to present gaps.
			 */
			if (need_has_check) {
				if (duk_has_prop_index(thr, i, j)) {
					duk_get_prop_index(thr, i, j);
					duk_xdef_prop_index_wec(thr, -2, idx);
				}
			} else {
				if (duk_get_prop_index(thr, i, j)) {
					duk_xdef_prop_index_wec(thr, -2, idx);
				} else {
					duk_pop_undefined(thr);
				}
			}
			idx++;
			DUK_ASSERT(idx != 0U); /* Wrap check above. */
		}
	}

	/* ES5.1 has a specification "bug" in that nonexistent trailing
	 * elements don't affect the result .length.  Test262 and other
	 * engines disagree, and the specification bug was fixed in ES2015
	 * (see NOTE 1 in https://www.ecma-international.org/ecma-262/6.0/#sec-array.prototype.concat).
	 */
	duk_push_uarridx(thr, idx);
	duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_LENGTH, DUK_PROPDESC_FLAGS_W);

	DUK_ASSERT_TOP(thr, n + 1);
	return 1;

fail_wrap:
	DUK_ERROR_RANGE_INVALID_LENGTH(thr);
	DUK_WO_NORETURN(return 0;);
}

/*
 *  join(), toLocaleString()
 *
 *  Note: checking valstack is necessary, but only in the per-element loop.
 *
 *  Note: the trivial approach of pushing all the elements on the value stack
 *  and then calling duk_join() fails when the array contains a large number
 *  of elements.  This problem can't be offloaded to duk_join() because the
 *  elements to join must be handled here and have special handling.  Current
 *  approach is to do intermediate joins with very large number of elements.
 *  There is no fancy handling; the prefix gets re-joined multiple times.
 */

DUK_INTERNAL duk_ret_t duk_bi_array_prototype_join_shared(duk_hthread *thr) {
	duk_uint32_t len, count;
	duk_uint32_t idx;
	duk_small_int_t to_locale_string = duk_get_current_magic(thr);
	duk_idx_t valstack_required;

	/* For join(), nargs is 1.  For toLocaleString(), nargs is 0 and
	 * setting the top essentially pushes an undefined to the stack,
	 * thus defaulting to a comma separator.
	 */
	duk_set_top(thr, 1);
	if (duk_is_undefined(thr, 0)) {
		duk_pop_undefined(thr);
		duk_push_hstring_stridx(thr, DUK_STRIDX_COMMA);
	} else {
		duk_to_string(thr, 0);
	}

	len = duk__push_this_obj_len_u32(thr);

	/* [ sep ToObject(this) len ] */

	DUK_DDD(DUK_DDDPRINT("sep=%!T, this=%!T, len=%lu",
	                     (duk_tval *) duk_get_tval(thr, 0),
	                     (duk_tval *) duk_get_tval(thr, 1),
	                     (unsigned long) len));

	/* The extra (+4) is tight. */
	valstack_required = (duk_idx_t) ((len >= DUK__ARRAY_MID_JOIN_LIMIT ? DUK__ARRAY_MID_JOIN_LIMIT : len) + 4);
	duk_require_stack(thr, valstack_required);

	duk_dup_0(thr);

	/* [ sep ToObject(this) len sep ] */

	count = 0;
	idx = 0;
	for (;;) {
		DUK_DDD(DUK_DDDPRINT("join idx=%ld", (long) idx));
		if (count >= DUK__ARRAY_MID_JOIN_LIMIT || /* intermediate join to avoid valstack overflow */
		    idx >= len) { /* end of loop (careful with len==0) */
			/* [ sep ToObject(this) len sep str0 ... str(count-1) ] */
			DUK_DDD(DUK_DDDPRINT("mid/final join, count=%ld, idx=%ld, len=%ld", (long) count, (long) idx, (long) len));
			duk_join(thr, (duk_idx_t) count); /* -> [ sep ToObject(this) len str ] */
			duk_dup_0(thr); /* -> [ sep ToObject(this) len str sep ] */
			duk_insert(thr, -2); /* -> [ sep ToObject(this) len sep str ] */
			count = 1;
		}
		if (idx >= len) {
			/* if true, the stack already contains the final result */
			break;
		}

		duk_get_prop_index(thr, 1, (duk_uarridx_t) idx);
		if (duk_is_null_or_undefined(thr, -1)) {
			duk_pop_nodecref_unsafe(thr);
			duk_push_hstring_empty(thr);
		} else {
			if (to_locale_string) {
				duk_to_object(thr, -1);
				duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_TO_LOCALE_STRING);
				duk_insert(thr, -2); /* -> [ ... toLocaleString ToObject(val) ] */
				duk_call_method(thr, 0);
			}
			duk_to_string(thr, -1);
		}

		count++;
		idx++;
	}

	/* [ sep ToObject(this) len sep result ] */

	return 1;
}

/*
 *  pop(), push()
 */

#if defined(DUK_USE_ARRAY_FASTPATH)
DUK_LOCAL duk_ret_t duk__array_pop_fastpath(duk_hthread *thr, duk_harray *h_arr) {
	duk_tval *tv_arraypart;
	duk_tval *tv_val;
	duk_uint32_t len;

	tv_arraypart = DUK_HOBJECT_A_GET_BASE(thr->heap, (duk_hobject *) h_arr);
	len = h_arr->length;
	if (len <= 0) {
		/* nop, return undefined */
		return 0;
	}

	len--;
	h_arr->length = len;

	/* Fast path doesn't check for an index property inherited from
	 * Array.prototype.  This is quite often acceptable; if not,
	 * disable fast path.
	 */
	DUK_ASSERT_VS_SPACE(thr);
	tv_val = tv_arraypart + len;
	if (DUK_TVAL_IS_UNUSED(tv_val)) {
		/* No net refcount change.  Value stack already has
		 * 'undefined' based on value stack init policy.
		 */
		DUK_ASSERT(DUK_TVAL_IS_UNDEFINED(thr->valstack_top));
		DUK_ASSERT(DUK_TVAL_IS_UNUSED(tv_val));
	} else {
		/* No net refcount change. */
		DUK_TVAL_SET_TVAL(thr->valstack_top, tv_val);
		DUK_TVAL_SET_UNUSED(tv_val);
	}
	thr->valstack_top++;

	/* XXX: there's no shrink check in the fast path now */

	return 1;
}
#endif /* DUK_USE_ARRAY_FASTPATH */

DUK_INTERNAL duk_ret_t duk_bi_array_prototype_pop(duk_hthread *thr) {
	duk_uint32_t len;
	duk_uint32_t idx;
#if defined(DUK_USE_ARRAY_FASTPATH)
	duk_harray *h_arr;
#endif

	DUK_ASSERT_TOP(thr, 0);

#if defined(DUK_USE_ARRAY_FASTPATH)
	h_arr = duk__arraypart_fastpath_this(thr);
	if (h_arr) {
		return duk__array_pop_fastpath(thr, h_arr);
	}
#endif

	/* XXX: Merge fastpath check into a related call (push this, coerce length, etc)? */

	len = duk__push_this_obj_len_u32(thr);
	if (len == 0) {
		duk_push_int(thr, 0);
		duk_put_prop_stridx_short(thr, 0, DUK_STRIDX_LENGTH);
		return 0;
	}
	idx = len - 1;

	duk_get_prop_index(thr, 0, (duk_uarridx_t) idx);
	duk_del_prop_index(thr, 0, (duk_uarridx_t) idx);
	duk_push_u32(thr, idx);
	duk_put_prop_stridx_short(thr, 0, DUK_STRIDX_LENGTH);
	return 1;
}

#if defined(DUK_USE_ARRAY_FASTPATH)
DUK_LOCAL duk_ret_t duk__array_push_fastpath(duk_hthread *thr, duk_harray *h_arr) {
	duk_tval *tv_arraypart;
	duk_tval *tv_src;
	duk_tval *tv_dst;
	duk_uint32_t len;
	duk_idx_t i, n;

	len = h_arr->length;
	tv_arraypart = DUK_HOBJECT_A_GET_BASE(thr->heap, (duk_hobject *) h_arr);

	n = (duk_idx_t) (thr->valstack_top - thr->valstack_bottom);
	DUK_ASSERT(n >= 0);
	DUK_ASSERT((duk_uint32_t) n <= DUK_UINT32_MAX);
	if (DUK_UNLIKELY(len + (duk_uint32_t) n < len)) {
		DUK_D(DUK_DPRINT("Array.prototype.push() would go beyond 32-bit length, throw"));
		DUK_DCERROR_RANGE_INVALID_LENGTH(thr); /* != 0 return value returned as is by caller */
	}
	if (len + (duk_uint32_t) n > DUK_HOBJECT_GET_ASIZE((duk_hobject *) h_arr)) {
		/* Array part would need to be extended.  Rely on slow path
		 * for now.
		 *
		 * XXX: Rework hobject code a bit and add extend support.
		 */
		return 0;
	}

	tv_src = thr->valstack_bottom;
	tv_dst = tv_arraypart + len;
	for (i = 0; i < n; i++) {
		/* No net refcount change; reset value stack values to
		 * undefined to satisfy value stack init policy.
		 */
		DUK_TVAL_SET_TVAL(tv_dst, tv_src);
		DUK_TVAL_SET_UNDEFINED(tv_src);
		tv_src++;
		tv_dst++;
	}
	thr->valstack_top = thr->valstack_bottom;
	len += (duk_uint32_t) n;
	h_arr->length = len;

	DUK_ASSERT((duk_uint_t) len == len);
	duk_push_uint(thr, (duk_uint_t) len);
	return 1;
}
#endif /* DUK_USE_ARRAY_FASTPATH */

DUK_INTERNAL duk_ret_t duk_bi_array_prototype_push(duk_hthread *thr) {
	/* Note: 'this' is not necessarily an Array object.  The push()
	 * algorithm is supposed to work for other kinds of objects too,
	 * so the algorithm has e.g. an explicit update for the 'length'
	 * property which is normally "magical" in arrays.
	 */

	duk_uint32_t len;
	duk_idx_t i, n;
#if defined(DUK_USE_ARRAY_FASTPATH)
	duk_harray *h_arr;
#endif

#if defined(DUK_USE_ARRAY_FASTPATH)
	h_arr = duk__arraypart_fastpath_this(thr);
	if (h_arr) {
		duk_ret_t rc;
		rc = duk__array_push_fastpath(thr, h_arr);
		if (rc != 0) {
			return rc;
		}
		DUK_DD(DUK_DDPRINT("array push() fast path exited, resize case"));
	}
#endif

	n = duk_get_top(thr);
	len = duk__push_this_obj_len_u32(thr);

	/* [ arg1 ... argN obj length ] */

	/* Technically Array.prototype.push() can create an Array with length
	 * longer than 2^32-1, i.e. outside the 32-bit range.  The final length
	 * is *not* wrapped to 32 bits in the specification.
	 *
	 * This implementation tracks length with a uint32 because it's much
	 * more practical.
	 *
	 * See: test-bi-array-push-maxlen.js.
	 */

	if (len + (duk_uint32_t) n < len) {
		DUK_D(DUK_DPRINT("Array.prototype.push() would go beyond 32-bit length, throw"));
		DUK_DCERROR_RANGE_INVALID_LENGTH(thr);
	}

	for (i = 0; i < n; i++) {
		duk_dup(thr, i);
		duk_put_prop_index(thr, -3, (duk_uarridx_t) (len + (duk_uint32_t) i));
	}
	len += (duk_uint32_t) n;

	duk_push_u32(thr, len);
	duk_dup_top(thr);
	duk_put_prop_stridx_short(thr, -4, DUK_STRIDX_LENGTH);

	/* [ arg1 ... argN obj length new_length ] */
	return 1;
}

/*
 *  sort()
 *
 *  Currently qsort with random pivot.  This is now really, really slow,
 *  because there is no fast path for array parts.
 *
 *  Signed indices are used because qsort() leaves and degenerate cases
 *  may use a negative offset.
 */

DUK_LOCAL duk_small_int_t duk__array_sort_compare(duk_hthread *thr, duk_int_t idx1, duk_int_t idx2) {
	duk_bool_t have1, have2;
	duk_bool_t undef1, undef2;
	duk_small_int_t ret;
	duk_idx_t idx_obj = 1; /* fixed offsets in valstack */
	duk_idx_t idx_fn = 0;
	duk_hstring *h1, *h2;

	/* Fast exit if indices are identical.  This is valid for a non-existent property,
	 * for an undefined value, and almost always for ToString() coerced comparison of
	 * arbitrary values (corner cases where this is not the case include e.g. a an
	 * object with varying ToString() coercion).
	 *
	 * The specification does not prohibit "caching" of values read from the array, so
	 * assuming equality for comparing an index with itself falls into the category of
	 * "caching".
	 *
	 * Also, compareFn may be inconsistent, so skipping a call to compareFn here may
	 * have an effect on the final result.  The specification does not require any
	 * specific behavior for inconsistent compare functions, so again, this fast path
	 * is OK.
	 */

	if (idx1 == idx2) {
		DUK_DDD(DUK_DDDPRINT("duk__array_sort_compare: idx1=%ld, idx2=%ld -> indices identical, quick exit",
		                     (long) idx1,
		                     (long) idx2));
		return 0;
	}

	have1 = duk_get_prop_index(thr, idx_obj, (duk_uarridx_t) idx1);
	have2 = duk_get_prop_index(thr, idx_obj, (duk_uarridx_t) idx2);

	DUK_DDD(DUK_DDDPRINT("duk__array_sort_compare: idx1=%ld, idx2=%ld, have1=%ld, have2=%ld, val1=%!T, val2=%!T",
	                     (long) idx1,
	                     (long) idx2,
	                     (long) have1,
	                     (long) have2,
	                     (duk_tval *) duk_get_tval(thr, -2),
	                     (duk_tval *) duk_get_tval(thr, -1)));

	if (have1) {
		if (have2) {
			;
		} else {
			ret = -1;
			goto pop_ret;
		}
	} else {
		if (have2) {
			ret = 1;
			goto pop_ret;
		} else {
			ret = 0;
			goto pop_ret;
		}
	}

	undef1 = duk_is_undefined(thr, -2);
	undef2 = duk_is_undefined(thr, -1);
	if (undef1) {
		if (undef2) {
			ret = 0;
			goto pop_ret;
		} else {
			ret = 1;
			goto pop_ret;
		}
	} else {
		if (undef2) {
			ret = -1;
			goto pop_ret;
		} else {
			;
		}
	}

	if (!duk_is_undefined(thr, idx_fn)) {
		duk_double_t d;

		/* No need to check callable; duk_call() will do that. */
		duk_dup(thr, idx_fn); /* -> [ ... x y fn ] */
		duk_insert(thr, -3); /* -> [ ... fn x y ] */
		duk_call(thr, 2); /* -> [ ... res ] */

		/* ES5 is a bit vague about what to do if the return value is
		 * not a number.  ES2015 provides a concrete description:
		 * http://www.ecma-international.org/ecma-262/6.0/#sec-sortcompare.
		 */

		d = duk_to_number_m1(thr);
		if (d < 0.0) {
			ret = -1;
		} else if (d > 0.0) {
			ret = 1;
		} else {
			/* Because NaN compares to false, NaN is handled here
			 * without an explicit check above.
			 */
			ret = 0;
		}

		duk_pop_nodecref_unsafe(thr);
		DUK_DDD(DUK_DDDPRINT("-> result %ld (from comparefn, after coercion)", (long) ret));
		return ret;
	}

	/* string compare is the default (a bit oddly) */

	/* XXX: any special handling for plain array; causes repeated coercion now? */
	h1 = duk_to_hstring(thr, -2);
	h2 = duk_to_hstring_m1(thr);
	DUK_ASSERT(h1 != NULL);
	DUK_ASSERT(h2 != NULL);

	ret = duk_js_string_compare(h1, h2); /* retval is directly usable */
	goto pop_ret;

pop_ret:
	duk_pop_2_unsafe(thr);
	DUK_DDD(DUK_DDDPRINT("-> result %ld", (long) ret));
	return ret;
}

DUK_LOCAL void duk__array_sort_swap(duk_hthread *thr, duk_int_t l, duk_int_t r) {
	duk_bool_t have_l, have_r;
	duk_idx_t idx_obj = 1; /* fixed offset in valstack */

	if (l == r) {
		return;
	}

	/* swap elements; deal with non-existent elements correctly */
	have_l = duk_get_prop_index(thr, idx_obj, (duk_uarridx_t) l);
	have_r = duk_get_prop_index(thr, idx_obj, (duk_uarridx_t) r);

	if (have_r) {
		/* right exists, [[Put]] regardless whether or not left exists */
		duk_put_prop_index(thr, idx_obj, (duk_uarridx_t) l);
	} else {
		duk_del_prop_index(thr, idx_obj, (duk_uarridx_t) l);
		duk_pop_undefined(thr);
	}

	if (have_l) {
		duk_put_prop_index(thr, idx_obj, (duk_uarridx_t) r);
	} else {
		duk_del_prop_index(thr, idx_obj, (duk_uarridx_t) r);
		duk_pop_undefined(thr);
	}
}

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 2)
/* Debug print which visualizes the qsort partitioning process. */
DUK_LOCAL void duk__debuglog_qsort_state(duk_hthread *thr, duk_int_t lo, duk_int_t hi, duk_int_t pivot) {
	char buf[4096];
	char *ptr = buf;
	duk_int_t i, n;
	n = (duk_int_t) duk_get_length(thr, 1);
	if (n > 4000) {
		n = 4000;
	}
	*ptr++ = '[';
	for (i = 0; i < n; i++) {
		if (i == pivot) {
			*ptr++ = '|';
		} else if (i == lo) {
			*ptr++ = '<';
		} else if (i == hi) {
			*ptr++ = '>';
		} else if (i >= lo && i <= hi) {
			*ptr++ = '-';
		} else {
			*ptr++ = ' ';
		}
	}
	*ptr++ = ']';
	*ptr++ = '\0';

	DUK_DDD(DUK_DDDPRINT("%s   (lo=%ld, hi=%ld, pivot=%ld)", (const char *) buf, (long) lo, (long) hi, (long) pivot));
}
#endif

DUK_LOCAL void duk__array_qsort(duk_hthread *thr, duk_int_t lo, duk_int_t hi) {
	duk_int_t p, l, r;

	/* The lo/hi indices may be crossed and hi < 0 is possible at entry. */

	DUK_DDD(DUK_DDDPRINT("duk__array_qsort: lo=%ld, hi=%ld, obj=%!T", (long) lo, (long) hi, (duk_tval *) duk_get_tval(thr, 1)));

	DUK_ASSERT_TOP(thr, 3);

	/* In some cases it may be that lo > hi, or hi < 0; these
	 * degenerate cases happen e.g. for empty arrays, and in
	 * recursion leaves.
	 */

	/* trivial cases */
	if (hi - lo < 1) {
		DUK_DDD(DUK_DDDPRINT("degenerate case, return immediately"));
		return;
	}
	DUK_ASSERT(hi > lo);
	DUK_ASSERT(hi - lo + 1 >= 2);

	/* randomized pivot selection */
	p = lo + (duk_int_t) (duk_util_get_random_double(thr) * (duk_double_t) (hi - lo + 1));
	DUK_ASSERT(p >= lo && p <= hi);
	DUK_DDD(DUK_DDDPRINT("lo=%ld, hi=%ld, chose pivot p=%ld", (long) lo, (long) hi, (long) p));

	/* move pivot out of the way */
	duk__array_sort_swap(thr, p, lo);
	p = lo;
	DUK_DDD(DUK_DDDPRINT("pivot moved out of the way: %!T", (duk_tval *) duk_get_tval(thr, 1)));

	l = lo + 1;
	r = hi;
	for (;;) {
		/* find elements to swap */
		for (;;) {
			DUK_DDD(DUK_DDDPRINT("left scan: l=%ld, r=%ld, p=%ld", (long) l, (long) r, (long) p));
			if (l >= hi) {
				break;
			}
			if (duk__array_sort_compare(thr, l, p) >= 0) { /* !(l < p) */
				break;
			}
			l++;
		}
		for (;;) {
			DUK_DDD(DUK_DDDPRINT("right scan: l=%ld, r=%ld, p=%ld", (long) l, (long) r, (long) p));
			if (r <= lo) {
				break;
			}
			if (duk__array_sort_compare(thr, p, r) >= 0) { /* !(p < r) */
				break;
			}
			r--;
		}
		if (l >= r) {
			goto done;
		}
		DUK_ASSERT(l < r);

		DUK_DDD(DUK_DDDPRINT("swap %ld and %ld", (long) l, (long) r));

		duk__array_sort_swap(thr, l, r);

		DUK_DDD(DUK_DDDPRINT("after swap: %!T", (duk_tval *) duk_get_tval(thr, 1)));
		l++;
		r--;
	}
done:
	/* Note that 'l' and 'r' may cross, i.e. r < l */
	DUK_ASSERT(l >= lo && l <= hi);
	DUK_ASSERT(r >= lo && r <= hi);

	/* XXX: there's no explicit recursion bound here now.  For the average
	 * qsort recursion depth O(log n) that's not really necessary: e.g. for
	 * 2**32 recursion depth would be about 32 which is OK.  However, qsort
	 * worst case recursion depth is O(n) which may be a problem.
	 */

	/* move pivot to its final place */
	DUK_DDD(DUK_DDDPRINT("before final pivot swap: %!T", (duk_tval *) duk_get_tval(thr, 1)));
	duk__array_sort_swap(thr, lo, r);

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 2)
	duk__debuglog_qsort_state(thr, lo, hi, r);
#endif

	DUK_DDD(DUK_DDDPRINT("recurse: pivot=%ld, obj=%!T", (long) r, (duk_tval *) duk_get_tval(thr, 1)));
	duk__array_qsort(thr, lo, r - 1);
	duk__array_qsort(thr, r + 1, hi);
}

DUK_INTERNAL duk_ret_t duk_bi_array_prototype_sort(duk_hthread *thr) {
	duk_uint32_t len;

	/* XXX: len >= 0x80000000 won't work below because a signed type
	 * is needed by qsort.
	 */
	len = duk__push_this_obj_len_u32_limited(thr);

	/* stack[0] = compareFn
	 * stack[1] = ToObject(this)
	 * stack[2] = ToUint32(length)
	 */

	if (len > 0) {
		/* avoid degenerate cases, so that (len - 1) won't underflow */
		duk__array_qsort(thr, (duk_int_t) 0, (duk_int_t) (len - 1));
	}

	DUK_ASSERT_TOP(thr, 3);
	duk_pop_nodecref_unsafe(thr);
	return 1; /* return ToObject(this) */
}

/*
 *  splice()
 */

/* XXX: this compiles to over 500 bytes now, even without special handling
 * for an array part.  Uses signed ints so does not handle full array range correctly.
 */

/* XXX: can shift() / unshift() use the same helper?
 *   shift() is (close to?) <--> splice(0, 1)
 *   unshift is (close to?) <--> splice(0, 0, [items])?
 */

DUK_INTERNAL duk_ret_t duk_bi_array_prototype_splice(duk_hthread *thr) {
	duk_idx_t nargs;
	duk_uint32_t len_u32;
	duk_int_t len;
	duk_bool_t have_delcount;
	duk_int_t item_count;
	duk_int_t act_start;
	duk_int_t del_count;
	duk_int_t i, n;

	DUK_UNREF(have_delcount);

	nargs = duk_get_top(thr);
	if (nargs < 2) {
		duk_set_top(thr, 2);
		nargs = 2;
		have_delcount = 0;
	} else {
		have_delcount = 1;
	}

	/* XXX: len >= 0x80000000 won't work below because we need to be
	 * able to represent -len.
	 */
	len_u32 = duk__push_this_obj_len_u32_limited(thr);
	len = (duk_int_t) len_u32;
	DUK_ASSERT(len >= 0);

	act_start = duk_to_int_clamped(thr, 0, -len, len);
	if (act_start < 0) {
		act_start = len + act_start;
	}
	DUK_ASSERT(act_start >= 0 && act_start <= len);

#if defined(DUK_USE_NONSTD_ARRAY_SPLICE_DELCOUNT)
	if (have_delcount) {
#endif
		del_count = duk_to_int_clamped(thr, 1, 0, len - act_start);
#if defined(DUK_USE_NONSTD_ARRAY_SPLICE_DELCOUNT)
	} else {
		/* E5.1 standard behavior when deleteCount is not given would be
		 * to treat it just like if 'undefined' was given, which coerces
		 * ultimately to 0.  Real world behavior is to splice to the end
		 * of array, see test-bi-array-proto-splice-no-delcount.js.
		 */
		del_count = len - act_start;
	}
#endif

	DUK_ASSERT(nargs >= 2);
	item_count = (duk_int_t) (nargs - 2);

	DUK_ASSERT(del_count >= 0 && del_count <= len - act_start);
	DUK_ASSERT(del_count + act_start <= len);

	/* For now, restrict result array into 32-bit length range. */
	if (((duk_double_t) len) - ((duk_double_t) del_count) + ((duk_double_t) item_count) > (duk_double_t) DUK_UINT32_MAX) {
		DUK_D(DUK_DPRINT("Array.prototype.splice() would go beyond 32-bit length, throw"));
		DUK_DCERROR_RANGE_INVALID_LENGTH(thr);
	}

	duk_push_array(thr);

	/* stack[0] = start
	 * stack[1] = deleteCount
	 * stack[2...nargs-1] = items
	 * stack[nargs] = ToObject(this)               -3
	 * stack[nargs+1] = ToUint32(length)           -2
	 * stack[nargs+2] = result array               -1
	 */

	DUK_ASSERT_TOP(thr, nargs + 3);

	/* Step 9: copy elements-to-be-deleted into the result array */

	for (i = 0; i < del_count; i++) {
		if (duk_get_prop_index(thr, -3, (duk_uarridx_t) (act_start + i))) {
			duk_xdef_prop_index_wec(thr, -2, (duk_uarridx_t) i); /* throw flag irrelevant (false in std alg) */
		} else {
			duk_pop_undefined(thr);
		}
	}
	duk_push_u32(thr, (duk_uint32_t) del_count);
	duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_LENGTH, DUK_PROPDESC_FLAGS_W);

	/* Steps 12 and 13: reorganize elements to make room for itemCount elements */

	if (item_count < del_count) {
		/*    [ A B C D E F G H ]    rel_index = 2, del_count 3, item count 1
		 * -> [ A B F G H ]          (conceptual intermediate step)
		 * -> [ A B . F G H ]        (placeholder marked)
		 *    [ A B C F G H ]        (actual result at this point, C will be replaced)
		 */

		DUK_ASSERT_TOP(thr, nargs + 3);

		n = len - del_count;
		for (i = act_start; i < n; i++) {
			if (duk_get_prop_index(thr, -3, (duk_uarridx_t) (i + del_count))) {
				duk_put_prop_index(thr, -4, (duk_uarridx_t) (i + item_count));
			} else {
				duk_pop_undefined(thr);
				duk_del_prop_index(thr, -3, (duk_uarridx_t) (i + item_count));
			}
		}

		DUK_ASSERT_TOP(thr, nargs + 3);

		/* loop iterator init and limit changed from standard algorithm */
		n = len - del_count + item_count;
		for (i = len - 1; i >= n; i--) {
			duk_del_prop_index(thr, -3, (duk_uarridx_t) i);
		}

		DUK_ASSERT_TOP(thr, nargs + 3);
	} else if (item_count > del_count) {
		/*    [ A B C D E F G H ]    rel_index = 2, del_count 3, item count 4
		 * -> [ A B F G H ]          (conceptual intermediate step)
		 * -> [ A B . . . . F G H ]  (placeholder marked)
		 *    [ A B C D E F F G H ]  (actual result at this point)
		 */

		DUK_ASSERT_TOP(thr, nargs + 3);

		/* loop iterator init and limit changed from standard algorithm */
		for (i = len - del_count - 1; i >= act_start; i--) {
			if (duk_get_prop_index(thr, -3, (duk_uarridx_t) (i + del_count))) {
				duk_put_prop_index(thr, -4, (duk_uarridx_t) (i + item_count));
			} else {
				duk_pop_undefined(thr);
				duk_del_prop_index(thr, -3, (duk_uarridx_t) (i + item_count));
			}
		}

		DUK_ASSERT_TOP(thr, nargs + 3);
	} else {
		/*    [ A B C D E F G H ]    rel_index = 2, del_count 3, item count 3
		 * -> [ A B F G H ]          (conceptual intermediate step)
		 * -> [ A B . . . F G H ]    (placeholder marked)
		 *    [ A B C D E F G H ]    (actual result at this point)
		 */
	}
	DUK_ASSERT_TOP(thr, nargs + 3);

	/* Step 15: insert itemCount elements into the hole made above */

	for (i = 0; i < item_count; i++) {
		duk_dup(thr, i + 2); /* args start at index 2 */
		duk_put_prop_index(thr, -4, (duk_uarridx_t) (act_start + i));
	}

	/* Step 16: update length; note that the final length may be above 32 bit range
	 * (but we checked above that this isn't the case here)
	 */

	duk_push_u32(thr, (duk_uint32_t) (len - del_count + item_count));
	duk_put_prop_stridx_short(thr, -4, DUK_STRIDX_LENGTH);

	/* result array is already at the top of stack */
	DUK_ASSERT_TOP(thr, nargs + 3);
	return 1;
}

/*
 *  reverse()
 */

DUK_INTERNAL duk_ret_t duk_bi_array_prototype_reverse(duk_hthread *thr) {
	duk_uint32_t len;
	duk_uint32_t middle;
	duk_uint32_t lower, upper;
	duk_bool_t have_lower, have_upper;

	len = duk__push_this_obj_len_u32(thr);
	middle = len / 2;

	/* If len <= 1, middle will be 0 and for-loop bails out
	 * immediately (0 < 0 -> false).
	 */

	for (lower = 0; lower < middle; lower++) {
		DUK_ASSERT(len >= 2);
		DUK_ASSERT_TOP(thr, 2);

		DUK_ASSERT(len >= lower + 1);
		upper = len - lower - 1;

		have_lower = duk_get_prop_index(thr, -2, (duk_uarridx_t) lower);
		have_upper = duk_get_prop_index(thr, -3, (duk_uarridx_t) upper);

		/* [ ToObject(this) ToUint32(length) lowerValue upperValue ] */

		if (have_upper) {
			duk_put_prop_index(thr, -4, (duk_uarridx_t) lower);
		} else {
			duk_del_prop_index(thr, -4, (duk_uarridx_t) lower);
			duk_pop_undefined(thr);
		}

		if (have_lower) {
			duk_put_prop_index(thr, -3, (duk_uarridx_t) upper);
		} else {
			duk_del_prop_index(thr, -3, (duk_uarridx_t) upper);
			duk_pop_undefined(thr);
		}

		DUK_ASSERT_TOP(thr, 2);
	}

	DUK_ASSERT_TOP(thr, 2);
	duk_pop_unsafe(thr); /* -> [ ToObject(this) ] */
	return 1;
}

/*
 *  slice()
 */

DUK_INTERNAL duk_ret_t duk_bi_array_prototype_slice(duk_hthread *thr) {
	duk_uint32_t len_u32;
	duk_int_t len;
	duk_int_t start, end;
	duk_int_t i;
	duk_uarridx_t idx;
	duk_uint32_t res_length = 0;

	/* XXX: len >= 0x80000000 won't work below because we need to be
	 * able to represent -len.
	 */
	len_u32 = duk__push_this_obj_len_u32_limited(thr);
	len = (duk_int_t) len_u32;
	DUK_ASSERT(len >= 0);

	duk_push_array(thr);

	/* stack[0] = start
	 * stack[1] = end
	 * stack[2] = ToObject(this)
	 * stack[3] = ToUint32(length)
	 * stack[4] = result array
	 */

	start = duk_to_int_clamped(thr, 0, -len, len);
	if (start < 0) {
		start = len + start;
	}
	/* XXX: could duk_is_undefined() provide defaulting undefined to 'len'
	 * (the upper limit)?
	 */
	if (duk_is_undefined(thr, 1)) {
		end = len;
	} else {
		end = duk_to_int_clamped(thr, 1, -len, len);
		if (end < 0) {
			end = len + end;
		}
	}
	DUK_ASSERT(start >= 0 && start <= len);
	DUK_ASSERT(end >= 0 && end <= len);

	idx = 0;
	for (i = start; i < end; i++) {
		DUK_ASSERT_TOP(thr, 5);
		if (duk_get_prop_index(thr, 2, (duk_uarridx_t) i)) {
			duk_xdef_prop_index_wec(thr, 4, idx);
			res_length = idx + 1;
		} else {
			duk_pop_undefined(thr);
		}
		idx++;
		DUK_ASSERT_TOP(thr, 5);
	}

	duk_push_u32(thr, res_length);
	duk_xdef_prop_stridx_short(thr, 4, DUK_STRIDX_LENGTH, DUK_PROPDESC_FLAGS_W);

	DUK_ASSERT_TOP(thr, 5);
	return 1;
}

/*
 *  shift()
 */

DUK_INTERNAL duk_ret_t duk_bi_array_prototype_shift(duk_hthread *thr) {
	duk_uint32_t len;
	duk_uint32_t i;

	len = duk__push_this_obj_len_u32(thr);
	if (len == 0) {
		duk_push_int(thr, 0);
		duk_put_prop_stridx_short(thr, 0, DUK_STRIDX_LENGTH);
		return 0;
	}

	duk_get_prop_index(thr, 0, 0);

	/* stack[0] = object (this)
	 * stack[1] = ToUint32(length)
	 * stack[2] = elem at index 0 (retval)
	 */

	for (i = 1; i < len; i++) {
		DUK_ASSERT_TOP(thr, 3);
		if (duk_get_prop_index(thr, 0, (duk_uarridx_t) i)) {
			/* fromPresent = true */
			duk_put_prop_index(thr, 0, (duk_uarridx_t) (i - 1));
		} else {
			/* fromPresent = false */
			duk_del_prop_index(thr, 0, (duk_uarridx_t) (i - 1));
			duk_pop_undefined(thr);
		}
	}
	duk_del_prop_index(thr, 0, (duk_uarridx_t) (len - 1));

	duk_push_u32(thr, (duk_uint32_t) (len - 1));
	duk_put_prop_stridx_short(thr, 0, DUK_STRIDX_LENGTH);

	DUK_ASSERT_TOP(thr, 3);
	return 1;
}

/*
 *  unshift()
 */

DUK_INTERNAL duk_ret_t duk_bi_array_prototype_unshift(duk_hthread *thr) {
	duk_idx_t nargs;
	duk_uint32_t len;
	duk_uint32_t i;

	nargs = duk_get_top(thr);
	len = duk__push_this_obj_len_u32(thr);

	/* stack[0...nargs-1] = unshift args (vararg)
	 * stack[nargs] = ToObject(this)
	 * stack[nargs+1] = ToUint32(length)
	 */

	DUK_ASSERT_TOP(thr, nargs + 2);

	/* Note: unshift() may operate on indices above unsigned 32-bit range
	 * and the final length may be >= 2**32.  However, we restrict the
	 * final result to 32-bit range for practicality.
	 */

	if (len + (duk_uint32_t) nargs < len) {
		DUK_D(DUK_DPRINT("Array.prototype.unshift() would go beyond 32-bit length, throw"));
		DUK_DCERROR_RANGE_INVALID_LENGTH(thr);
	}

	i = len;
	while (i > 0) {
		DUK_ASSERT_TOP(thr, nargs + 2);
		i--;
		/* k+argCount-1; note that may be above 32-bit range */

		if (duk_get_prop_index(thr, -2, (duk_uarridx_t) i)) {
			/* fromPresent = true */
			/* [ ... ToObject(this) ToUint32(length) val ] */
			duk_put_prop_index(
			    thr,
			    -3,
			    (duk_uarridx_t) (i + (duk_uint32_t) nargs)); /* -> [ ... ToObject(this) ToUint32(length) ] */
		} else {
			/* fromPresent = false */
			/* [ ... ToObject(this) ToUint32(length) val ] */
			duk_pop_undefined(thr);
			duk_del_prop_index(
			    thr,
			    -2,
			    (duk_uarridx_t) (i + (duk_uint32_t) nargs)); /* -> [ ... ToObject(this) ToUint32(length) ] */
		}
		DUK_ASSERT_TOP(thr, nargs + 2);
	}

	for (i = 0; i < (duk_uint32_t) nargs; i++) {
		DUK_ASSERT_TOP(thr, nargs + 2);
		duk_dup(thr, (duk_idx_t) i); /* -> [ ... ToObject(this) ToUint32(length) arg[i] ] */
		duk_put_prop_index(thr, -3, (duk_uarridx_t) i);
		DUK_ASSERT_TOP(thr, nargs + 2);
	}

	DUK_ASSERT_TOP(thr, nargs + 2);
	duk_push_u32(thr, len + (duk_uint32_t) nargs);
	duk_dup_top(thr); /* -> [ ... ToObject(this) ToUint32(length) final_len final_len ] */
	duk_put_prop_stridx_short(thr, -4, DUK_STRIDX_LENGTH);
	return 1;
}

/*
 *  indexOf(), lastIndexOf()
 */

DUK_INTERNAL duk_ret_t duk_bi_array_prototype_indexof_shared(duk_hthread *thr) {
	duk_idx_t nargs;
	duk_int_t i, len;
	duk_int_t from_idx;
	duk_small_int_t idx_step = duk_get_current_magic(thr); /* idx_step is +1 for indexOf, -1 for lastIndexOf */

	/* lastIndexOf() needs to be a vararg function because we must distinguish
	 * between an undefined fromIndex and a "not given" fromIndex; indexOf() is
	 * made vararg for symmetry although it doesn't strictly need to be.
	 */

	nargs = duk_get_top(thr);
	duk_set_top(thr, 2);

	/* XXX: must be able to represent -len */
	len = (duk_int_t) duk__push_this_obj_len_u32_limited(thr);
	if (len == 0) {
		goto not_found;
	}

	/* Index clamping is a bit tricky, we must ensure that we'll only iterate
	 * through elements that exist and that the specific requirements from E5.1
	 * Sections 15.4.4.14 and 15.4.4.15 are fulfilled; especially:
	 *
	 *   - indexOf: clamp to [-len,len], negative handling -> [0,len],
	 *     if clamped result is len, for-loop bails out immediately
	 *
	 *   - lastIndexOf: clamp to [-len-1, len-1], negative handling -> [-1, len-1],
	 *     if clamped result is -1, for-loop bails out immediately
	 *
	 * If fromIndex is not given, ToInteger(undefined) = 0, which is correct
	 * for indexOf() but incorrect for lastIndexOf().  Hence special handling,
	 * and why lastIndexOf() needs to be a vararg function.
	 */

	if (nargs >= 2) {
		/* indexOf: clamp fromIndex to [-len, len]
		 * (if fromIndex == len, for-loop terminates directly)
		 *
		 * lastIndexOf: clamp fromIndex to [-len - 1, len - 1]
		 * (if clamped to -len-1 -> fromIndex becomes -1, terminates for-loop directly)
		 */
		from_idx = duk_to_int_clamped(thr, 1, (idx_step > 0 ? -len : -len - 1), (idx_step > 0 ? len : len - 1));
		if (from_idx < 0) {
			/* for lastIndexOf, result may be -1 (mark immediate termination) */
			from_idx = len + from_idx;
		}
	} else {
		/* for indexOf, ToInteger(undefined) would be 0, i.e. correct, but
		 * handle both indexOf and lastIndexOf specially here.
		 */
		if (idx_step > 0) {
			from_idx = 0;
		} else {
			from_idx = len - 1;
		}
	}

	/* stack[0] = searchElement
	 * stack[1] = fromIndex
	 * stack[2] = object
	 * stack[3] = length (not needed, but not popped above)
	 */

	for (i = from_idx; i >= 0 && i < len; i += idx_step) {
		DUK_ASSERT_TOP(thr, 4);

		if (duk_get_prop_index(thr, 2, (duk_uarridx_t) i)) {
			DUK_ASSERT_TOP(thr, 5);
			if (duk_strict_equals(thr, 0, 4)) {
				duk_push_int(thr, i);
				return 1;
			}
		}

		duk_pop_unsafe(thr);
	}

not_found:
	duk_push_int(thr, -1);
	return 1;
}

/*
 *  every(), some(), forEach(), map(), filter()
 */

#define DUK__ITER_EVERY   0
#define DUK__ITER_SOME    1
#define DUK__ITER_FOREACH 2
#define DUK__ITER_MAP     3
#define DUK__ITER_FILTER  4

/* XXX: This helper is a bit awkward because the handling for the different iteration
 * callers is quite different.  This now compiles to a bit less than 500 bytes, so with
 * 5 callers the net result is about 100 bytes / caller.
 */

DUK_INTERNAL duk_ret_t duk_bi_array_prototype_iter_shared(duk_hthread *thr) {
	duk_uint32_t len;
	duk_uint32_t i;
	duk_uarridx_t k;
	duk_bool_t bval;
	duk_small_int_t iter_type = duk_get_current_magic(thr);
	duk_uint32_t res_length = 0;

	/* each call this helper serves has nargs==2 */
	DUK_ASSERT_TOP(thr, 2);

	len = duk__push_this_obj_len_u32(thr);
	duk_require_callable(thr, 0);
	/* if thisArg not supplied, behave as if undefined was supplied */

	if (iter_type == DUK__ITER_MAP || iter_type == DUK__ITER_FILTER) {
		duk_push_array(thr);
	} else {
		duk_push_undefined(thr);
	}

	/* stack[0] = callback
	 * stack[1] = thisArg
	 * stack[2] = object
	 * stack[3] = ToUint32(length)  (unused, but avoid unnecessary pop)
	 * stack[4] = result array (or undefined)
	 */

	k = 0; /* result index for filter() */
	for (i = 0; i < len; i++) {
		DUK_ASSERT_TOP(thr, 5);

		if (!duk_get_prop_index(thr, 2, (duk_uarridx_t) i)) {
			/* For 'map' trailing missing elements don't invoke the
			 * callback but count towards the result length.
			 */
			if (iter_type == DUK__ITER_MAP) {
				res_length = i + 1;
			}
			duk_pop_undefined(thr);
			continue;
		}

		/* The original value needs to be preserved for filter(), hence
		 * this funny order.  We can't re-get the value because of side
		 * effects.
		 */

		duk_dup_0(thr);
		duk_dup_1(thr);
		duk_dup_m3(thr);
		duk_push_u32(thr, i);
		duk_dup_2(thr); /* [ ... val callback thisArg val i obj ] */
		duk_call_method(thr, 3); /* -> [ ... val retval ] */

		switch (iter_type) {
		case DUK__ITER_EVERY:
			bval = duk_to_boolean(thr, -1);
			if (!bval) {
				/* stack top contains 'false' */
				return 1;
			}
			break;
		case DUK__ITER_SOME:
			bval = duk_to_boolean(thr, -1);
			if (bval) {
				/* stack top contains 'true' */
				return 1;
			}
			break;
		case DUK__ITER_FOREACH:
			/* nop */
			break;
		case DUK__ITER_MAP:
			duk_dup_top(thr);
			duk_xdef_prop_index_wec(thr, 4, (duk_uarridx_t) i); /* retval to result[i] */
			res_length = i + 1;
			break;
		case DUK__ITER_FILTER:
			bval = duk_to_boolean(thr, -1);
			if (bval) {
				duk_dup_m2(thr); /* orig value */
				duk_xdef_prop_index_wec(thr, 4, (duk_uarridx_t) k);
				k++;
				res_length = k;
			}
			break;
		default:
			DUK_UNREACHABLE();
			break;
		}
		duk_pop_2_unsafe(thr);

		DUK_ASSERT_TOP(thr, 5);
	}

	switch (iter_type) {
	case DUK__ITER_EVERY:
		duk_push_true(thr);
		break;
	case DUK__ITER_SOME:
		duk_push_false(thr);
		break;
	case DUK__ITER_FOREACH:
		duk_push_undefined(thr);
		break;
	case DUK__ITER_MAP:
	case DUK__ITER_FILTER:
		DUK_ASSERT_TOP(thr, 5);
		DUK_ASSERT(duk_is_array(thr, -1)); /* topmost element is the result array already */
		duk_push_u32(thr, res_length);
		duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_LENGTH, DUK_PROPDESC_FLAGS_W);
		break;
	default:
		DUK_UNREACHABLE();
		break;
	}

	return 1;
}

/*
 *  reduce(), reduceRight()
 */

DUK_INTERNAL duk_ret_t duk_bi_array_prototype_reduce_shared(duk_hthread *thr) {
	duk_idx_t nargs;
	duk_bool_t have_acc;
	duk_uint32_t i, len;
	duk_small_int_t idx_step = duk_get_current_magic(thr); /* idx_step is +1 for reduce, -1 for reduceRight */

	/* We're a varargs function because we need to detect whether
	 * initialValue was given or not.
	 */
	nargs = duk_get_top(thr);
	DUK_DDD(DUK_DDDPRINT("nargs=%ld", (long) nargs));

	duk_set_top(thr, 2);
	len = duk__push_this_obj_len_u32(thr);
	duk_require_callable(thr, 0);

	/* stack[0] = callback fn
	 * stack[1] = initialValue
	 * stack[2] = object (coerced this)
	 * stack[3] = length (not needed, but not popped above)
	 * stack[4] = accumulator
	 */

	have_acc = 0;
	if (nargs >= 2) {
		duk_dup_1(thr);
		have_acc = 1;
	}
	DUK_DDD(DUK_DDDPRINT("have_acc=%ld, acc=%!T", (long) have_acc, (duk_tval *) duk_get_tval(thr, 3)));

	/* For len == 0, i is initialized to len - 1 which underflows.
	 * The condition (i < len) will then exit the for-loop on the
	 * first round which is correct.  Similarly, loop termination
	 * happens by i underflowing.
	 */

	for (i = (idx_step >= 0 ? 0 : len - 1); i < len; /* i >= 0 would always be true */
	     i += (duk_uint32_t) idx_step) {
		DUK_DDD(DUK_DDDPRINT("i=%ld, len=%ld, have_acc=%ld, top=%ld, acc=%!T",
		                     (long) i,
		                     (long) len,
		                     (long) have_acc,
		                     (long) duk_get_top(thr),
		                     (duk_tval *) duk_get_tval(thr, 4)));

		DUK_ASSERT((have_acc && duk_get_top(thr) == 5) || (!have_acc && duk_get_top(thr) == 4));

		if (!duk_has_prop_index(thr, 2, (duk_uarridx_t) i)) {
			continue;
		}

		if (!have_acc) {
			DUK_ASSERT_TOP(thr, 4);
			duk_get_prop_index(thr, 2, (duk_uarridx_t) i);
			have_acc = 1;
			DUK_ASSERT_TOP(thr, 5);
		} else {
			DUK_ASSERT_TOP(thr, 5);
			duk_dup_0(thr);
			duk_dup(thr, 4);
			duk_get_prop_index(thr, 2, (duk_uarridx_t) i);
			duk_push_u32(thr, i);
			duk_dup_2(thr);
			DUK_DDD(DUK_DDDPRINT("calling reduce function: func=%!T, prev=%!T, curr=%!T, idx=%!T, obj=%!T",
			                     (duk_tval *) duk_get_tval(thr, -5),
			                     (duk_tval *) duk_get_tval(thr, -4),
			                     (duk_tval *) duk_get_tval(thr, -3),
			                     (duk_tval *) duk_get_tval(thr, -2),
			                     (duk_tval *) duk_get_tval(thr, -1)));
			duk_call(thr, 4);
			DUK_DDD(DUK_DDDPRINT("-> result: %!T", (duk_tval *) duk_get_tval(thr, -1)));
			duk_replace(thr, 4);
			DUK_ASSERT_TOP(thr, 5);
		}
	}

	if (!have_acc) {
		DUK_DCERROR_TYPE_INVALID_ARGS(thr);
	}

	DUK_ASSERT_TOP(thr, 5);
	return 1;
}

#endif /* DUK_USE_ARRAY_BUILTIN */
