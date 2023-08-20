/*
 *  Object handling: property access and other support functions.
 */

#include "duk_internal.h"

/*
 *  Property handling
 *
 *  The API exposes only the most common property handling functions.
 *  The caller can invoke ECMAScript built-ins for full control (e.g.
 *  defineProperty, getOwnPropertyDescriptor).
 */

DUK_EXTERNAL duk_bool_t duk_get_prop(duk_hthread *thr, duk_idx_t obj_idx) {
	duk_tval *tv_obj;
	duk_tval *tv_key;
	duk_bool_t rc;

	DUK_ASSERT_API_ENTRY(thr);

	/* Note: copying tv_obj and tv_key to locals to shield against a valstack
	 * resize is not necessary for a property get right now.
	 */

	tv_obj = duk_require_tval(thr, obj_idx);
	tv_key = duk_require_tval(thr, -1);

	rc = duk_hobject_getprop(thr, tv_obj, tv_key);
	DUK_ASSERT(rc == 0 || rc == 1);
	/* a value is left on stack regardless of rc */

	duk_remove_m2(thr); /* remove key */
	DUK_ASSERT(duk_is_undefined(thr, -1) || rc == 1);
	return rc; /* 1 if property found, 0 otherwise */
}

DUK_EXTERNAL duk_bool_t duk_get_prop_string(duk_hthread *thr, duk_idx_t obj_idx, const char *key) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(key != NULL);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	(void) duk_push_string(thr, key);
	return duk_get_prop(thr, obj_idx);
}

DUK_EXTERNAL duk_bool_t duk_get_prop_lstring(duk_hthread *thr, duk_idx_t obj_idx, const char *key, duk_size_t key_len) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(key != NULL);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	(void) duk_push_lstring(thr, key, key_len);
	return duk_get_prop(thr, obj_idx);
}

#if !defined(DUK_USE_PREFER_SIZE)
DUK_EXTERNAL duk_bool_t duk_get_prop_literal_raw(duk_hthread *thr, duk_idx_t obj_idx, const char *key, duk_size_t key_len) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(key != NULL);
	DUK_ASSERT(key[key_len] == (char) 0);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	(void) duk_push_literal_raw(thr, key, key_len);
	return duk_get_prop(thr, obj_idx);
}
#endif

DUK_EXTERNAL duk_bool_t duk_get_prop_index(duk_hthread *thr, duk_idx_t obj_idx, duk_uarridx_t arr_idx) {
	DUK_ASSERT_API_ENTRY(thr);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	duk_push_uarridx(thr, arr_idx);
	return duk_get_prop(thr, obj_idx);
}

DUK_EXTERNAL duk_bool_t duk_get_prop_heapptr(duk_hthread *thr, duk_idx_t obj_idx, void *ptr) {
	DUK_ASSERT_API_ENTRY(thr);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	(void) duk_push_heapptr(thr, ptr); /* NULL -> 'undefined' */
	return duk_get_prop(thr, obj_idx);
}

DUK_INTERNAL duk_bool_t duk_get_prop_stridx(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t stridx) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT_STRIDX_VALID(stridx);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	(void) duk_push_hstring(thr, DUK_HTHREAD_GET_STRING(thr, stridx));
	return duk_get_prop(thr, obj_idx);
}

DUK_INTERNAL duk_bool_t duk_get_prop_stridx_short_raw(duk_hthread *thr, duk_uint_t packed_args) {
	return duk_get_prop_stridx(thr, (duk_idx_t) (duk_int16_t) (packed_args >> 16), (duk_small_uint_t) (packed_args & 0xffffUL));
}

DUK_INTERNAL duk_bool_t duk_get_prop_stridx_boolean(duk_hthread *thr,
                                                    duk_idx_t obj_idx,
                                                    duk_small_uint_t stridx,
                                                    duk_bool_t *out_has_prop) {
	duk_bool_t rc;

	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT_STRIDX_VALID(stridx);

	rc = duk_get_prop_stridx(thr, obj_idx, stridx);
	if (out_has_prop) {
		*out_has_prop = rc;
	}
	return duk_to_boolean_top_pop(thr);
}

/* This get variant is for internal use, it differs from standard
 * duk_get_prop() in that:
 *   - Object argument must be an object (primitive values not supported).
 *   - Key argument must be a string (no coercion).
 *   - Only own properties are checked (no inheritance).  Only "entry part"
 *     properties are checked (not array index properties).
 *   - Property must be a plain data property, not a getter.
 *   - Proxy traps are not triggered.
 */
DUK_INTERNAL duk_bool_t duk_xget_owndataprop(duk_hthread *thr, duk_idx_t obj_idx) {
	duk_hobject *h_obj;
	duk_hstring *h_key;
	duk_tval *tv_val;

	DUK_ASSERT_API_ENTRY(thr);

	/* Note: copying tv_obj and tv_key to locals to shield against a valstack
	 * resize is not necessary for a property get right now.
	 */

	h_obj = duk_get_hobject(thr, obj_idx);
	if (h_obj == NULL) {
		return 0;
	}
	h_key = duk_require_hstring(thr, -1);

	tv_val = duk_hobject_find_entry_tval_ptr(thr->heap, h_obj, h_key);
	if (tv_val == NULL) {
		return 0;
	}

	duk_push_tval(thr, tv_val);
	duk_remove_m2(thr); /* remove key */

	return 1;
}

DUK_INTERNAL duk_bool_t duk_xget_owndataprop_stridx(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t stridx) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT_STRIDX_VALID(stridx);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	(void) duk_push_hstring(thr, DUK_HTHREAD_GET_STRING(thr, stridx));
	return duk_xget_owndataprop(thr, obj_idx);
}

DUK_INTERNAL duk_bool_t duk_xget_owndataprop_stridx_short_raw(duk_hthread *thr, duk_uint_t packed_args) {
	return duk_xget_owndataprop_stridx(thr,
	                                   (duk_idx_t) (duk_int16_t) (packed_args >> 16),
	                                   (duk_small_uint_t) (packed_args & 0xffffUL));
}

DUK_LOCAL duk_bool_t duk__put_prop_shared(duk_hthread *thr, duk_idx_t obj_idx, duk_idx_t idx_key) {
	duk_tval *tv_obj;
	duk_tval *tv_key;
	duk_tval *tv_val;
	duk_bool_t throw_flag;
	duk_bool_t rc;

	/* Note: copying tv_obj and tv_key to locals to shield against a valstack
	 * resize is not necessary for a property put right now (putprop protects
	 * against it internally).
	 */

	/* Key and value indices are either (-2, -1) or (-1, -2).  Given idx_key,
	 * idx_val is always (idx_key ^ 0x01).
	 */
	DUK_ASSERT((idx_key == -2 && (idx_key ^ 1) == -1) || (idx_key == -1 && (idx_key ^ 1) == -2));
	/* XXX: Direct access; faster validation. */
	tv_obj = duk_require_tval(thr, obj_idx);
	tv_key = duk_require_tval(thr, idx_key);
	tv_val = duk_require_tval(thr, idx_key ^ 1);
	throw_flag = duk_is_strict_call(thr);

	rc = duk_hobject_putprop(thr, tv_obj, tv_key, tv_val, throw_flag);
	DUK_ASSERT(rc == 0 || rc == 1);

	duk_pop_2(thr); /* remove key and value */
	return rc; /* 1 if property found, 0 otherwise */
}

DUK_EXTERNAL duk_bool_t duk_put_prop(duk_hthread *thr, duk_idx_t obj_idx) {
	DUK_ASSERT_API_ENTRY(thr);
	return duk__put_prop_shared(thr, obj_idx, -2);
}

DUK_EXTERNAL duk_bool_t duk_put_prop_string(duk_hthread *thr, duk_idx_t obj_idx, const char *key) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(key != NULL);

	/* Careful here and with other duk_put_prop_xxx() helpers: the
	 * target object and the property value may be in the same value
	 * stack slot (unusual, but still conceptually clear).
	 */
	obj_idx = duk_normalize_index(thr, obj_idx);
	(void) duk_push_string(thr, key);
	return duk__put_prop_shared(thr, obj_idx, -1);
}

DUK_EXTERNAL duk_bool_t duk_put_prop_lstring(duk_hthread *thr, duk_idx_t obj_idx, const char *key, duk_size_t key_len) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(key != NULL);

	obj_idx = duk_normalize_index(thr, obj_idx);
	(void) duk_push_lstring(thr, key, key_len);
	return duk__put_prop_shared(thr, obj_idx, -1);
}

#if !defined(DUK_USE_PREFER_SIZE)
DUK_EXTERNAL duk_bool_t duk_put_prop_literal_raw(duk_hthread *thr, duk_idx_t obj_idx, const char *key, duk_size_t key_len) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(key != NULL);
	DUK_ASSERT(key[key_len] == (char) 0);

	obj_idx = duk_normalize_index(thr, obj_idx);
	(void) duk_push_literal_raw(thr, key, key_len);
	return duk__put_prop_shared(thr, obj_idx, -1);
}
#endif

DUK_EXTERNAL duk_bool_t duk_put_prop_index(duk_hthread *thr, duk_idx_t obj_idx, duk_uarridx_t arr_idx) {
	DUK_ASSERT_API_ENTRY(thr);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	duk_push_uarridx(thr, arr_idx);
	return duk__put_prop_shared(thr, obj_idx, -1);
}

DUK_EXTERNAL duk_bool_t duk_put_prop_heapptr(duk_hthread *thr, duk_idx_t obj_idx, void *ptr) {
	DUK_ASSERT_API_ENTRY(thr);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	(void) duk_push_heapptr(thr, ptr); /* NULL -> 'undefined' */
	return duk__put_prop_shared(thr, obj_idx, -1);
}

DUK_INTERNAL duk_bool_t duk_put_prop_stridx(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t stridx) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT_STRIDX_VALID(stridx);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	duk_push_hstring(thr, DUK_HTHREAD_GET_STRING(thr, stridx));
	return duk__put_prop_shared(thr, obj_idx, -1);
}

DUK_INTERNAL duk_bool_t duk_put_prop_stridx_short_raw(duk_hthread *thr, duk_uint_t packed_args) {
	return duk_put_prop_stridx(thr, (duk_idx_t) (duk_int16_t) (packed_args >> 16), (duk_small_uint_t) (packed_args & 0xffffUL));
}

DUK_EXTERNAL duk_bool_t duk_del_prop(duk_hthread *thr, duk_idx_t obj_idx) {
	duk_tval *tv_obj;
	duk_tval *tv_key;
	duk_bool_t throw_flag;
	duk_bool_t rc;

	DUK_ASSERT_API_ENTRY(thr);

	/* Note: copying tv_obj and tv_key to locals to shield against a valstack
	 * resize is not necessary for a property delete right now.
	 */

	tv_obj = duk_require_tval(thr, obj_idx);
	tv_key = duk_require_tval(thr, -1);
	throw_flag = duk_is_strict_call(thr);

	rc = duk_hobject_delprop(thr, tv_obj, tv_key, throw_flag);
	DUK_ASSERT(rc == 0 || rc == 1);

	duk_pop(thr); /* remove key */
	return rc;
}

DUK_EXTERNAL duk_bool_t duk_del_prop_string(duk_hthread *thr, duk_idx_t obj_idx, const char *key) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(key != NULL);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	(void) duk_push_string(thr, key);
	return duk_del_prop(thr, obj_idx);
}

DUK_EXTERNAL duk_bool_t duk_del_prop_lstring(duk_hthread *thr, duk_idx_t obj_idx, const char *key, duk_size_t key_len) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(key != NULL);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	(void) duk_push_lstring(thr, key, key_len);
	return duk_del_prop(thr, obj_idx);
}

#if !defined(DUK_USE_PREFER_SIZE)
DUK_EXTERNAL duk_bool_t duk_del_prop_literal_raw(duk_hthread *thr, duk_idx_t obj_idx, const char *key, duk_size_t key_len) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(key != NULL);
	DUK_ASSERT(key[key_len] == (char) 0);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	(void) duk_push_literal_raw(thr, key, key_len);
	return duk_del_prop(thr, obj_idx);
}
#endif

DUK_EXTERNAL duk_bool_t duk_del_prop_index(duk_hthread *thr, duk_idx_t obj_idx, duk_uarridx_t arr_idx) {
	DUK_ASSERT_API_ENTRY(thr);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	duk_push_uarridx(thr, arr_idx);
	return duk_del_prop(thr, obj_idx);
}

DUK_EXTERNAL duk_bool_t duk_del_prop_heapptr(duk_hthread *thr, duk_idx_t obj_idx, void *ptr) {
	DUK_ASSERT_API_ENTRY(thr);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	(void) duk_push_heapptr(thr, ptr); /* NULL -> 'undefined' */
	return duk_del_prop(thr, obj_idx);
}

DUK_INTERNAL duk_bool_t duk_del_prop_stridx(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t stridx) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT_STRIDX_VALID(stridx);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	duk_push_hstring(thr, DUK_HTHREAD_GET_STRING(thr, stridx));
	return duk_del_prop(thr, obj_idx);
}

#if 0
DUK_INTERNAL duk_bool_t duk_del_prop_stridx_short_raw(duk_hthread *thr, duk_uint_t packed_args) {
	return duk_del_prop_stridx(thr, (duk_idx_t) (duk_int16_t) (packed_args >> 16),
	                                (duk_small_uint_t) (packed_args & 0xffffUL));
}
#endif

DUK_EXTERNAL duk_bool_t duk_has_prop(duk_hthread *thr, duk_idx_t obj_idx) {
	duk_tval *tv_obj;
	duk_tval *tv_key;
	duk_bool_t rc;

	DUK_ASSERT_API_ENTRY(thr);

	/* Note: copying tv_obj and tv_key to locals to shield against a valstack
	 * resize is not necessary for a property existence check right now.
	 */

	tv_obj = duk_require_tval(thr, obj_idx);
	tv_key = duk_require_tval(thr, -1);

	rc = duk_hobject_hasprop(thr, tv_obj, tv_key);
	DUK_ASSERT(rc == 0 || rc == 1);

	duk_pop(thr); /* remove key */
	return rc; /* 1 if property found, 0 otherwise */
}

DUK_EXTERNAL duk_bool_t duk_has_prop_string(duk_hthread *thr, duk_idx_t obj_idx, const char *key) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(key != NULL);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	(void) duk_push_string(thr, key);
	return duk_has_prop(thr, obj_idx);
}

DUK_EXTERNAL duk_bool_t duk_has_prop_lstring(duk_hthread *thr, duk_idx_t obj_idx, const char *key, duk_size_t key_len) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(key != NULL);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	(void) duk_push_lstring(thr, key, key_len);
	return duk_has_prop(thr, obj_idx);
}

#if !defined(DUK_USE_PREFER_SIZE)
DUK_EXTERNAL duk_bool_t duk_has_prop_literal_raw(duk_hthread *thr, duk_idx_t obj_idx, const char *key, duk_size_t key_len) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(key != NULL);
	DUK_ASSERT(key[key_len] == (char) 0);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	(void) duk_push_literal_raw(thr, key, key_len);
	return duk_has_prop(thr, obj_idx);
}
#endif

DUK_EXTERNAL duk_bool_t duk_has_prop_index(duk_hthread *thr, duk_idx_t obj_idx, duk_uarridx_t arr_idx) {
	DUK_ASSERT_API_ENTRY(thr);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	duk_push_uarridx(thr, arr_idx);
	return duk_has_prop(thr, obj_idx);
}

DUK_EXTERNAL duk_bool_t duk_has_prop_heapptr(duk_hthread *thr, duk_idx_t obj_idx, void *ptr) {
	DUK_ASSERT_API_ENTRY(thr);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	(void) duk_push_heapptr(thr, ptr); /* NULL -> 'undefined' */
	return duk_has_prop(thr, obj_idx);
}

DUK_INTERNAL duk_bool_t duk_has_prop_stridx(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t stridx) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT_STRIDX_VALID(stridx);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	duk_push_hstring(thr, DUK_HTHREAD_GET_STRING(thr, stridx));
	return duk_has_prop(thr, obj_idx);
}

#if 0
DUK_INTERNAL duk_bool_t duk_has_prop_stridx_short_raw(duk_hthread *thr, duk_uint_t packed_args) {
	return duk_has_prop_stridx(thr, (duk_idx_t) (duk_int16_t) (packed_args >> 16),
	                                (duk_small_uint_t) (packed_args & 0xffffUL));
}
#endif

/* Define own property without inheritance lookups and such.  This differs from
 * [[DefineOwnProperty]] because special behaviors (like Array 'length') are
 * not invoked by this method.  The caller must be careful to invoke any such
 * behaviors if necessary.
 */
DUK_INTERNAL void duk_xdef_prop(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t desc_flags) {
	duk_hobject *obj;
	duk_hstring *key;

	DUK_ASSERT_API_ENTRY(thr);

	obj = duk_require_hobject(thr, obj_idx);
	DUK_ASSERT(obj != NULL);
	key = duk_to_property_key_hstring(thr, -2);
	DUK_ASSERT(key != NULL);
	DUK_ASSERT(duk_require_tval(thr, -1) != NULL);

	duk_hobject_define_property_internal(thr, obj, key, desc_flags);

	duk_pop(thr); /* pop key */
}

DUK_INTERNAL void duk_xdef_prop_index(duk_hthread *thr, duk_idx_t obj_idx, duk_uarridx_t arr_idx, duk_small_uint_t desc_flags) {
	duk_hobject *obj;

	DUK_ASSERT_API_ENTRY(thr);

	obj = duk_require_hobject(thr, obj_idx);
	DUK_ASSERT(obj != NULL);

	duk_hobject_define_property_internal_arridx(thr, obj, arr_idx, desc_flags);
	/* value popped by call */
}

DUK_INTERNAL void duk_xdef_prop_stridx(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t stridx, duk_small_uint_t desc_flags) {
	duk_hobject *obj;
	duk_hstring *key;

	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT_STRIDX_VALID(stridx);

	obj = duk_require_hobject(thr, obj_idx);
	DUK_ASSERT(obj != NULL);
	key = DUK_HTHREAD_GET_STRING(thr, stridx);
	DUK_ASSERT(key != NULL);
	DUK_ASSERT(duk_require_tval(thr, -1) != NULL);

	duk_hobject_define_property_internal(thr, obj, key, desc_flags);
	/* value popped by call */
}

DUK_INTERNAL void duk_xdef_prop_stridx_short_raw(duk_hthread *thr, duk_uint_t packed_args) {
	duk_xdef_prop_stridx(thr,
	                     (duk_idx_t) (duk_int8_t) (packed_args >> 24),
	                     (duk_small_uint_t) (packed_args >> 8) & 0xffffUL,
	                     (duk_small_uint_t) (packed_args & 0xffL));
}

#if 0 /*unused*/
DUK_INTERNAL void duk_xdef_prop_stridx_builtin(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t stridx, duk_small_int_t builtin_idx, duk_small_uint_t desc_flags) {
	duk_hobject *obj;
	duk_hstring *key;

	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT_STRIDX_VALID(stridx);
	DUK_ASSERT_BIDX_VALID(builtin_idx);

	obj = duk_require_hobject(thr, obj_idx);
	DUK_ASSERT(obj != NULL);
	key = DUK_HTHREAD_GET_STRING(thr, stridx);
	DUK_ASSERT(key != NULL);

	duk_push_hobject(thr, thr->builtins[builtin_idx]);
	duk_hobject_define_property_internal(thr, obj, key, desc_flags);
	/* value popped by call */
}
#endif

/* This is a rare property helper; it sets the global thrower (E5 Section 13.2.3)
 * setter/getter into an object property.  This is needed by the 'arguments'
 * object creation code, function instance creation code, and Function.prototype.bind().
 */

DUK_INTERNAL void duk_xdef_prop_stridx_thrower(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t stridx) {
	DUK_ASSERT_API_ENTRY(thr);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	duk_push_hstring_stridx(thr, stridx);
	duk_push_hobject_bidx(thr, DUK_BIDX_TYPE_ERROR_THROWER);
	duk_dup_top(thr);
	duk_def_prop(thr, obj_idx, DUK_DEFPROP_HAVE_SETTER | DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_FORCE); /* attributes always 0 */
}

/* Object.getOwnPropertyDescriptor() equivalent C binding. */
DUK_EXTERNAL void duk_get_prop_desc(duk_hthread *thr, duk_idx_t obj_idx, duk_uint_t flags) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_UNREF(flags); /* no flags defined yet */

	duk_hobject_object_get_own_property_descriptor(thr, obj_idx); /* [ ... key ] -> [ ... desc ] */
}

/* Object.defineProperty() equivalent C binding. */
DUK_EXTERNAL void duk_def_prop(duk_hthread *thr, duk_idx_t obj_idx, duk_uint_t flags) {
	duk_idx_t idx_base;
	duk_hobject *obj;
	duk_hstring *key;
	duk_idx_t idx_value;
	duk_hobject *get;
	duk_hobject *set;
	duk_uint_t is_data_desc;
	duk_uint_t is_acc_desc;

	DUK_ASSERT_API_ENTRY(thr);

	obj = duk_require_hobject(thr, obj_idx);

	is_data_desc = flags & (DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_HAVE_WRITABLE);
	is_acc_desc = flags & (DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER);
	if (is_data_desc && is_acc_desc) {
		/* "Have" flags must not be conflicting so that they would
		 * apply to both a plain property and an accessor at the same
		 * time.
		 */
		goto fail_invalid_desc;
	}

	idx_base = duk_get_top_index(thr);
	if (flags & DUK_DEFPROP_HAVE_SETTER) {
		duk_require_type_mask(thr, idx_base, DUK_TYPE_MASK_UNDEFINED | DUK_TYPE_MASK_OBJECT | DUK_TYPE_MASK_LIGHTFUNC);
		set = duk_get_hobject_promote_lfunc(thr, idx_base);
		if (set != NULL && !DUK_HOBJECT_IS_CALLABLE(set)) {
			goto fail_not_callable;
		}
		idx_base--;
	} else {
		set = NULL;
	}
	if (flags & DUK_DEFPROP_HAVE_GETTER) {
		duk_require_type_mask(thr, idx_base, DUK_TYPE_MASK_UNDEFINED | DUK_TYPE_MASK_OBJECT | DUK_TYPE_MASK_LIGHTFUNC);
		get = duk_get_hobject_promote_lfunc(thr, idx_base);
		if (get != NULL && !DUK_HOBJECT_IS_CALLABLE(get)) {
			goto fail_not_callable;
		}
		idx_base--;
	} else {
		get = NULL;
	}
	if (flags & DUK_DEFPROP_HAVE_VALUE) {
		idx_value = idx_base;
		idx_base--;
	} else {
		idx_value = (duk_idx_t) -1;
	}
	key = duk_to_property_key_hstring(thr, idx_base);
	DUK_ASSERT(key != NULL);

	duk_require_valid_index(thr, idx_base);

	duk_hobject_define_property_helper(thr, flags /*defprop_flags*/, obj, key, idx_value, get, set, 1 /*throw_flag*/);

	/* Clean up stack */

	duk_set_top(thr, idx_base);

	/* [ ... obj ... ] */

	return;

fail_invalid_desc:
	DUK_ERROR_TYPE(thr, DUK_STR_INVALID_DESCRIPTOR);
	DUK_WO_NORETURN(return;);

fail_not_callable:
	DUK_ERROR_TYPE(thr, DUK_STR_NOT_CALLABLE);
	DUK_WO_NORETURN(return;);
}

/*
 *  Object related
 */

DUK_EXTERNAL void duk_compact(duk_hthread *thr, duk_idx_t obj_idx) {
	duk_hobject *obj;

	DUK_ASSERT_API_ENTRY(thr);

	obj = duk_get_hobject(thr, obj_idx);
	if (obj) {
		/* Note: this may fail, caller should protect the call if necessary */
		duk_hobject_compact_props(thr, obj);
	}
}

DUK_INTERNAL void duk_compact_m1(duk_hthread *thr) {
	DUK_ASSERT_API_ENTRY(thr);

	duk_compact(thr, -1);
}

/* XXX: the duk_hobject_enum.c stack APIs should be reworked */

DUK_EXTERNAL void duk_enum(duk_hthread *thr, duk_idx_t obj_idx, duk_uint_t enum_flags) {
	DUK_ASSERT_API_ENTRY(thr);

	duk_dup(thr, obj_idx);
	duk_require_hobject_promote_mask(thr, -1, DUK_TYPE_MASK_LIGHTFUNC | DUK_TYPE_MASK_BUFFER);
	duk_hobject_enumerator_create(thr, enum_flags); /* [target] -> [enum] */
}

DUK_EXTERNAL duk_bool_t duk_next(duk_hthread *thr, duk_idx_t enum_index, duk_bool_t get_value) {
	DUK_ASSERT_API_ENTRY(thr);

	duk_require_hobject(thr, enum_index);
	duk_dup(thr, enum_index);
	return duk_hobject_enumerator_next(thr, get_value);
}

DUK_INTERNAL void duk_seal_freeze_raw(duk_hthread *thr, duk_idx_t obj_idx, duk_bool_t is_freeze) {
	duk_tval *tv;
	duk_hobject *h;

	DUK_ASSERT_API_ENTRY(thr);

	tv = duk_require_tval(thr, obj_idx);
	DUK_ASSERT(tv != NULL);

	/* Seal/freeze are quite rare in practice so it'd be nice to get the
	 * correct behavior simply via automatic promotion (at the cost of some
	 * memory churn).  However, the promoted objects don't behave the same,
	 * e.g. promoted lightfuncs are extensible.
	 */

	switch (DUK_TVAL_GET_TAG(tv)) {
	case DUK_TAG_BUFFER:
		/* Plain buffer: already sealed, but not frozen (and can't be frozen
		 * because index properties can't be made non-writable.
		 */
		if (is_freeze) {
			goto fail_cannot_freeze;
		}
		break;
	case DUK_TAG_LIGHTFUNC:
		/* Lightfunc: already sealed and frozen, success. */
		break;
	case DUK_TAG_OBJECT:
		h = DUK_TVAL_GET_OBJECT(tv);
		DUK_ASSERT(h != NULL);
		if (is_freeze && DUK_HOBJECT_IS_BUFOBJ(h)) {
			/* Buffer objects cannot be frozen because there's no internal
			 * support for making virtual array indices non-writable.
			 */
			DUK_DD(DUK_DDPRINT("cannot freeze a buffer object"));
			goto fail_cannot_freeze;
		}
		duk_hobject_object_seal_freeze_helper(thr, h, is_freeze);

		/* Sealed and frozen objects cannot gain any more properties,
		 * so this is a good time to compact them.
		 */
		duk_hobject_compact_props(thr, h);
		break;
	default:
		/* ES2015 Sections 19.1.2.5, 19.1.2.17 */
		break;
	}
	return;

fail_cannot_freeze:
	DUK_ERROR_TYPE_INVALID_ARGS(thr); /* XXX: proper error message */
	DUK_WO_NORETURN(return;);
}

DUK_EXTERNAL void duk_seal(duk_hthread *thr, duk_idx_t obj_idx) {
	DUK_ASSERT_API_ENTRY(thr);

	duk_seal_freeze_raw(thr, obj_idx, 0 /*is_freeze*/);
}

DUK_EXTERNAL void duk_freeze(duk_hthread *thr, duk_idx_t obj_idx) {
	DUK_ASSERT_API_ENTRY(thr);

	duk_seal_freeze_raw(thr, obj_idx, 1 /*is_freeze*/);
}

/*
 *  Helpers for writing multiple properties
 */

DUK_EXTERNAL void duk_put_function_list(duk_hthread *thr, duk_idx_t obj_idx, const duk_function_list_entry *funcs) {
	const duk_function_list_entry *ent = funcs;

	DUK_ASSERT_API_ENTRY(thr);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	if (ent != NULL) {
		while (ent->key != NULL) {
			duk_push_c_function(thr, ent->value, ent->nargs);
			duk_put_prop_string(thr, obj_idx, ent->key);
			ent++;
		}
	}
}

DUK_EXTERNAL void duk_put_number_list(duk_hthread *thr, duk_idx_t obj_idx, const duk_number_list_entry *numbers) {
	const duk_number_list_entry *ent = numbers;
	duk_tval *tv;

	DUK_ASSERT_API_ENTRY(thr);

	obj_idx = duk_require_normalize_index(thr, obj_idx);
	if (ent != NULL) {
		while (ent->key != NULL) {
			tv = thr->valstack_top++;
			DUK_ASSERT(DUK_TVAL_IS_UNDEFINED(tv)); /* value stack init policy */
			DUK_TVAL_SET_NUMBER_CHKFAST_SLOW(tv, ent->value); /* no need for decref/incref */
			duk_put_prop_string(thr, obj_idx, ent->key);
			ent++;
		}
	}
}

/*
 *  Shortcut for accessing global object properties
 */

DUK_EXTERNAL duk_bool_t duk_get_global_string(duk_hthread *thr, const char *key) {
	duk_bool_t ret;

	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(thr->builtins[DUK_BIDX_GLOBAL] != NULL);

	/* XXX: direct implementation */

	duk_push_hobject(thr, thr->builtins[DUK_BIDX_GLOBAL]);
	ret = duk_get_prop_string(thr, -1, key);
	duk_remove_m2(thr);
	return ret;
}

DUK_EXTERNAL duk_bool_t duk_get_global_lstring(duk_hthread *thr, const char *key, duk_size_t key_len) {
	duk_bool_t ret;

	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(thr->builtins[DUK_BIDX_GLOBAL] != NULL);

	/* XXX: direct implementation */

	duk_push_hobject(thr, thr->builtins[DUK_BIDX_GLOBAL]);
	ret = duk_get_prop_lstring(thr, -1, key, key_len);
	duk_remove_m2(thr);
	return ret;
}

#if !defined(DUK_USE_PREFER_SIZE)
DUK_EXTERNAL duk_bool_t duk_get_global_literal_raw(duk_hthread *thr, const char *key, duk_size_t key_len) {
	duk_bool_t ret;

	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(thr->builtins[DUK_BIDX_GLOBAL] != NULL);
	DUK_ASSERT(key[key_len] == (char) 0);

	/* XXX: direct implementation */

	duk_push_hobject(thr, thr->builtins[DUK_BIDX_GLOBAL]);
	ret = duk_get_prop_literal_raw(thr, -1, key, key_len);
	duk_remove_m2(thr);
	return ret;
}
#endif

DUK_EXTERNAL duk_bool_t duk_get_global_heapptr(duk_hthread *thr, void *ptr) {
	duk_bool_t ret;

	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(thr->builtins[DUK_BIDX_GLOBAL] != NULL);

	/* XXX: direct implementation */

	duk_push_hobject(thr, thr->builtins[DUK_BIDX_GLOBAL]);
	ret = duk_get_prop_heapptr(thr, -1, ptr);
	duk_remove_m2(thr);
	return ret;
}

DUK_EXTERNAL duk_bool_t duk_put_global_string(duk_hthread *thr, const char *key) {
	duk_bool_t ret;

	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(thr->builtins[DUK_BIDX_GLOBAL] != NULL);

	/* XXX: direct implementation */

	duk_push_hobject(thr, thr->builtins[DUK_BIDX_GLOBAL]);
	duk_insert(thr, -2);
	ret = duk_put_prop_string(thr, -2, key); /* [ ... global val ] -> [ ... global ] */
	duk_pop(thr);
	return ret;
}

DUK_EXTERNAL duk_bool_t duk_put_global_lstring(duk_hthread *thr, const char *key, duk_size_t key_len) {
	duk_bool_t ret;

	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(thr->builtins[DUK_BIDX_GLOBAL] != NULL);

	/* XXX: direct implementation */

	duk_push_hobject(thr, thr->builtins[DUK_BIDX_GLOBAL]);
	duk_insert(thr, -2);
	ret = duk_put_prop_lstring(thr, -2, key, key_len); /* [ ... global val ] -> [ ... global ] */
	duk_pop(thr);
	return ret;
}

#if !defined(DUK_USE_PREFER_SIZE)
DUK_EXTERNAL duk_bool_t duk_put_global_literal_raw(duk_hthread *thr, const char *key, duk_size_t key_len) {
	duk_bool_t ret;

	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(thr->builtins[DUK_BIDX_GLOBAL] != NULL);
	DUK_ASSERT(key[key_len] == (char) 0);

	/* XXX: direct implementation */

	duk_push_hobject(thr, thr->builtins[DUK_BIDX_GLOBAL]);
	duk_insert(thr, -2);
	ret = duk_put_prop_literal_raw(thr, -2, key, key_len); /* [ ... global val ] -> [ ... global ] */
	duk_pop(thr);
	return ret;
}
#endif

DUK_EXTERNAL duk_bool_t duk_put_global_heapptr(duk_hthread *thr, void *ptr) {
	duk_bool_t ret;

	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(thr->builtins[DUK_BIDX_GLOBAL] != NULL);

	/* XXX: direct implementation */

	duk_push_hobject(thr, thr->builtins[DUK_BIDX_GLOBAL]);
	duk_insert(thr, -2);
	ret = duk_put_prop_heapptr(thr, -2, ptr); /* [ ... global val ] -> [ ... global ] */
	duk_pop(thr);
	return ret;
}

/*
 *  ES2015 GetMethod()
 */

DUK_INTERNAL duk_bool_t duk_get_method_stridx(duk_hthread *thr, duk_idx_t idx, duk_small_uint_t stridx) {
	(void) duk_get_prop_stridx(thr, idx, stridx);
	if (duk_is_null_or_undefined(thr, -1)) {
		duk_pop_nodecref_unsafe(thr);
		return 0;
	}
	if (!duk_is_callable(thr, -1)) {
		DUK_ERROR_TYPE(thr, DUK_STR_NOT_CALLABLE);
		DUK_WO_NORETURN(return 0;);
	}
	return 1;
}

/*
 *  Object prototype
 */

DUK_EXTERNAL void duk_get_prototype(duk_hthread *thr, duk_idx_t idx) {
	duk_hobject *obj;
	duk_hobject *proto;

	DUK_ASSERT_API_ENTRY(thr);

	obj = duk_require_hobject(thr, idx);
	DUK_ASSERT(obj != NULL);

	/* XXX: shared helper for duk_push_hobject_or_undefined()? */
	proto = DUK_HOBJECT_GET_PROTOTYPE(thr->heap, obj);
	if (proto) {
		duk_push_hobject(thr, proto);
	} else {
		duk_push_undefined(thr);
	}
}

DUK_EXTERNAL void duk_set_prototype(duk_hthread *thr, duk_idx_t idx) {
	duk_hobject *obj;
	duk_hobject *proto;

	DUK_ASSERT_API_ENTRY(thr);

	obj = duk_require_hobject(thr, idx);
	DUK_ASSERT(obj != NULL);
	duk_require_type_mask(thr, -1, DUK_TYPE_MASK_UNDEFINED | DUK_TYPE_MASK_OBJECT);
	proto = duk_get_hobject(thr, -1);
	/* proto can also be NULL here (allowed explicitly) */

#if defined(DUK_USE_ROM_OBJECTS)
	if (DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) obj)) {
		DUK_ERROR_TYPE(thr, DUK_STR_NOT_CONFIGURABLE); /* XXX: "read only object"? */
		DUK_WO_NORETURN(return;);
	}
#endif

	DUK_HOBJECT_SET_PROTOTYPE_UPDREF(thr, obj, proto);

	duk_pop(thr);
}

DUK_INTERNAL void duk_clear_prototype(duk_hthread *thr, duk_idx_t idx) {
	duk_hobject *obj;

	DUK_ASSERT_API_ENTRY(thr);

	obj = duk_require_hobject(thr, idx);
	DUK_ASSERT(obj != NULL);

#if defined(DUK_USE_ROM_OBJECTS)
	if (DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) obj)) {
		DUK_ERROR_TYPE(thr, DUK_STR_NOT_CONFIGURABLE); /* XXX: "read only object"? */
		DUK_WO_NORETURN(return;);
	}
#endif

	DUK_HOBJECT_SET_PROTOTYPE_UPDREF(thr, obj, NULL);
}

DUK_INTERNAL duk_bool_t duk_is_bare_object(duk_hthread *thr, duk_idx_t idx) {
	duk_hobject *obj;
	duk_hobject *proto;

	DUK_ASSERT_API_ENTRY(thr);

	obj = duk_require_hobject(thr, idx);
	DUK_ASSERT(obj != NULL);

	proto = DUK_HOBJECT_GET_PROTOTYPE(thr->heap, obj);
	return (proto == NULL);
}

/*
 *  Object finalizer
 */

#if defined(DUK_USE_FINALIZER_SUPPORT)
/* XXX: these could be implemented as macros calling an internal function
 * directly.
 * XXX: same issue as with Duktape.fin: there's no way to delete the property
 * now (just set it to undefined).
 */
DUK_EXTERNAL void duk_get_finalizer(duk_hthread *thr, duk_idx_t idx) {
	DUK_ASSERT_API_ENTRY(thr);

	/* This get intentionally walks the inheritance chain at present,
	 * which matches how the effective finalizer property is also
	 * looked up in GC.
	 */
	duk_get_prop_stridx(thr, idx, DUK_STRIDX_INT_FINALIZER);
}

DUK_EXTERNAL void duk_set_finalizer(duk_hthread *thr, duk_idx_t idx) {
	duk_hobject *h;
	duk_bool_t callable;

	DUK_ASSERT_API_ENTRY(thr);

	h = duk_require_hobject(thr, idx); /* Get before 'put' so that 'idx' is correct. */
	callable = duk_is_callable(thr, -1);

	/* At present finalizer is stored as a hidden Symbol, with normal
	 * inheritance and access control.  As a result, finalizer cannot
	 * currently be set on a non-extensible (sealed or frozen) object.
	 * It might be useful to allow it.
	 */
	duk_put_prop_stridx(thr, idx, DUK_STRIDX_INT_FINALIZER);

	/* In addition to setting the finalizer property, keep a "have
	 * finalizer" flag in duk_hobject in sync so that refzero can do
	 * a very quick finalizer check by walking the prototype chain
	 * and checking the flag alone.  (Note that this means that just
	 * setting _Finalizer on an object won't affect finalizer checks.)
	 *
	 * NOTE: if the argument is a Proxy object, this flag will be set
	 * on the Proxy, not the target.  As a result, the target won't get
	 * a finalizer flag and the Proxy also won't be finalized as there's
	 * an explicit Proxy check in finalization now.
	 */
	if (callable) {
		DUK_HOBJECT_SET_HAVE_FINALIZER(h);
	} else {
		DUK_HOBJECT_CLEAR_HAVE_FINALIZER(h);
	}
}
#else /* DUK_USE_FINALIZER_SUPPORT */
DUK_EXTERNAL void duk_get_finalizer(duk_hthread *thr, duk_idx_t idx) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_UNREF(idx);
	DUK_ERROR_UNSUPPORTED(thr);
	DUK_WO_NORETURN(return;);
}

DUK_EXTERNAL void duk_set_finalizer(duk_hthread *thr, duk_idx_t idx) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_UNREF(idx);
	DUK_ERROR_UNSUPPORTED(thr);
	DUK_WO_NORETURN(return;);
}
#endif /* DUK_USE_FINALIZER_SUPPORT */
