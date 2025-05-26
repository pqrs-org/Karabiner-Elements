/*
 *  Object built-ins
 */

#include "duk_internal.h"

/* Needed even when Object built-in disabled. */
DUK_INTERNAL duk_ret_t duk_bi_object_prototype_to_string(duk_hthread *thr) {
	duk_tval *tv;

	tv = DUK_HTHREAD_THIS_PTR(thr);
	duk_push_class_string_tval(thr, tv, 0 /*avoid_side_effects*/);
	return 1;
}

#if defined(DUK_USE_OBJECT_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_object_constructor(duk_hthread *thr) {
	duk_uint_t arg_mask;

	arg_mask = duk_get_type_mask(thr, 0);

	if (!duk_is_constructor_call(thr) && /* not a constructor call */
	    ((arg_mask & (DUK_TYPE_MASK_NULL | DUK_TYPE_MASK_UNDEFINED)) == 0)) { /* and argument not null or undefined */
		duk_to_object(thr, 0);
		return 1;
	}

	/* Pointer and buffer primitive values are treated like other
	 * primitives values which have a fully fledged object counterpart:
	 * promote to an object value.  Lightfuncs and plain buffers are
	 * coerced with ToObject() even they could also be returned as is.
	 */
	if (arg_mask & (DUK_TYPE_MASK_OBJECT | DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_BOOLEAN | DUK_TYPE_MASK_NUMBER |
	                DUK_TYPE_MASK_POINTER | DUK_TYPE_MASK_BUFFER | DUK_TYPE_MASK_LIGHTFUNC)) {
		/* For DUK_TYPE_OBJECT the coercion is a no-op and could
		 * be checked for explicitly, but Object(obj) calls are
		 * not very common so opt for minimal footprint.
		 */
		duk_to_object(thr, 0);
		return 1;
	}

	(void) duk_push_object_helper(thr,
	                              DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_FASTREFS |
	                                  DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_OBJECT),
	                              DUK_BIDX_OBJECT_PROTOTYPE);
	return 1;
}
#endif /* DUK_USE_OBJECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN) && defined(DUK_USE_ES6)
DUK_INTERNAL duk_ret_t duk_bi_object_constructor_assign(duk_hthread *thr) {
	duk_idx_t nargs;
	duk_int_t idx;

	nargs = duk_get_top_require_min(thr, 1 /*min_top*/);

	duk_to_object(thr, 0);
	for (idx = 1; idx < nargs; idx++) {
		/* E7 19.1.2.1 (step 4a) */
		if (duk_is_null_or_undefined(thr, idx)) {
			continue;
		}

		/* duk_enum() respects ES2015+ [[OwnPropertyKeys]] ordering, which is
		 * convenient here.
		 */
		duk_to_object(thr, idx);
		duk_enum(thr, idx, DUK_ENUM_OWN_PROPERTIES_ONLY);
		while (duk_next(thr, -1, 1 /*get_value*/)) {
			/* [ target ... enum key value ] */
			duk_put_prop(thr, 0);
			/* [ target ... enum ] */
		}
		/* Could pop enumerator, but unnecessary because of duk_set_top()
		 * below.
		 */
	}

	duk_set_top(thr, 1);
	return 1;
}
#endif

#if defined(DUK_USE_OBJECT_BUILTIN) && defined(DUK_USE_ES6)
DUK_INTERNAL duk_ret_t duk_bi_object_constructor_is(duk_hthread *thr) {
	DUK_ASSERT_TOP(thr, 2);
	duk_push_boolean(thr, duk_samevalue(thr, 0, 1));
	return 1;
}
#endif

#if defined(DUK_USE_OBJECT_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_object_constructor_create(duk_hthread *thr) {
	duk_hobject *proto;

	DUK_ASSERT_TOP(thr, 2);

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
	duk_hbufobj_promote_plain(thr, 0);
#endif
	proto = duk_require_hobject_accept_mask(thr, 0, DUK_TYPE_MASK_NULL);
	DUK_ASSERT(proto != NULL || duk_is_null(thr, 0));

	(void) duk_push_object_helper_proto(thr,
	                                    DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_FASTREFS |
	                                        DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_OBJECT),
	                                    proto);

	if (!duk_is_undefined(thr, 1)) {
		/* [ O Properties obj ] */

		duk_replace(thr, 0);

		/* [ obj Properties ] */

		/* Just call the "original" Object.defineProperties() to
		 * finish up.
		 */

		return duk_bi_object_constructor_define_properties(thr);
	}

	/* [ O Properties obj ] */

	return 1;
}
#endif /* DUK_USE_OBJECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_object_constructor_define_properties(duk_hthread *thr) {
	duk_small_uint_t pass;
	duk_uint_t defprop_flags;
	duk_hobject *obj;
	duk_idx_t idx_value;
	duk_hobject *get;
	duk_hobject *set;

	/* Lightfunc and plain buffer handling by ToObject() coercion. */
	obj = duk_require_hobject_promote_mask(thr, 0, DUK_TYPE_MASK_LIGHTFUNC | DUK_TYPE_MASK_BUFFER);
	DUK_ASSERT(obj != NULL);

	duk_to_object(thr, 1); /* properties object */

	DUK_DDD(DUK_DDDPRINT("target=%!iT, properties=%!iT", (duk_tval *) duk_get_tval(thr, 0), (duk_tval *) duk_get_tval(thr, 1)));

	/*
	 *  Two pass approach to processing the property descriptors.
	 *  On first pass validate and normalize all descriptors before
	 *  any changes are made to the target object.  On second pass
	 *  make the actual modifications to the target object.
	 *
	 *  Right now we'll just use the same normalize/validate helper
	 *  on both passes, ignoring its outputs on the first pass.
	 */

	for (pass = 0; pass < 2; pass++) {
		duk_set_top(thr, 2); /* -> [ hobject props ] */
		duk_enum(thr, 1, DUK_ENUM_OWN_PROPERTIES_ONLY | DUK_ENUM_INCLUDE_SYMBOLS /*enum_flags*/);

		for (;;) {
			duk_hstring *key;

			/* [ hobject props enum(props) ] */

			duk_set_top(thr, 3);

			if (!duk_next(thr, 2, 1 /*get_value*/)) {
				break;
			}

			DUK_DDD(DUK_DDDPRINT("-> key=%!iT, desc=%!iT",
			                     (duk_tval *) duk_get_tval(thr, -2),
			                     (duk_tval *) duk_get_tval(thr, -1)));

			/* [ hobject props enum(props) key desc ] */

			duk_hobject_prepare_property_descriptor(thr, 4 /*idx_desc*/, &defprop_flags, &idx_value, &get, &set);

			/* [ hobject props enum(props) key desc [multiple values] ] */

			if (pass == 0) {
				continue;
			}

			/* This allows symbols on purpose. */
			key = duk_known_hstring(thr, 3);
			DUK_ASSERT(key != NULL);

			duk_hobject_define_property_helper(thr, defprop_flags, obj, key, idx_value, get, set, 1 /*throw_flag*/);
		}
	}

	/*
	 *  Return target object
	 */

	duk_dup_0(thr);
	return 1;
}
#endif /* DUK_USE_OBJECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_object_constructor_seal_freeze_shared(duk_hthread *thr) {
	DUK_ASSERT_TOP(thr, 1);

	duk_seal_freeze_raw(thr, 0, (duk_bool_t) duk_get_current_magic(thr) /*is_freeze*/);
	return 1;
}
#endif /* DUK_USE_OBJECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_object_constructor_is_sealed_frozen_shared(duk_hthread *thr) {
	duk_hobject *h;
	duk_bool_t is_frozen;
	duk_uint_t mask;

	is_frozen = (duk_bool_t) duk_get_current_magic(thr);
	mask = duk_get_type_mask(thr, 0);
	if (mask & (DUK_TYPE_MASK_LIGHTFUNC | DUK_TYPE_MASK_BUFFER)) {
		DUK_ASSERT(is_frozen == 0 || is_frozen == 1);
		duk_push_boolean(thr,
		                 (mask & DUK_TYPE_MASK_LIGHTFUNC) ? 1 : /* lightfunc always frozen and sealed */
                                     (is_frozen ^ 1)); /* buffer sealed but not frozen (index props writable) */
	} else {
		/* ES2015 Sections 19.1.2.12, 19.1.2.13: anything other than an object
		 * is considered to be already sealed and frozen.
		 */
		h = duk_get_hobject(thr, 0);
		duk_push_boolean(thr, (h == NULL) || duk_hobject_object_is_sealed_frozen_helper(thr, h, is_frozen /*is_frozen*/));
	}
	return 1;
}
#endif /* DUK_USE_OBJECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_object_prototype_to_locale_string(duk_hthread *thr) {
	DUK_ASSERT_TOP(thr, 0);
	(void) duk_push_this_coercible_to_object(thr);
	duk_get_prop_stridx_short(thr, 0, DUK_STRIDX_TO_STRING);
#if 0 /* This is mentioned explicitly in the E5.1 spec, but duk_call_method() checks for it in practice. */
	duk_require_callable(thr, 1);
#endif
	duk_dup_0(thr); /* -> [ O toString O ] */
	duk_call_method(thr, 0); /* XXX: call method tail call? */
	return 1;
}
#endif /* DUK_USE_OBJECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_object_prototype_value_of(duk_hthread *thr) {
	/* For lightfuncs and plain buffers, returns Object() coerced. */
	(void) duk_push_this_coercible_to_object(thr);
	return 1;
}
#endif /* DUK_USE_OBJECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_object_prototype_is_prototype_of(duk_hthread *thr) {
	duk_hobject *h_v;
	duk_hobject *h_obj;

	DUK_ASSERT_TOP(thr, 1);

	h_v = duk_get_hobject(thr, 0);
	if (!h_v) {
		duk_push_false(thr); /* XXX: tail call: return duk_push_false(thr) */
		return 1;
	}

	h_obj = duk_push_this_coercible_to_object(thr);
	DUK_ASSERT(h_obj != NULL);

	/* E5.1 Section 15.2.4.6, step 3.a, lookup proto once before compare.
	 * Prototype loops should cause an error to be thrown.
	 */
	duk_push_boolean(
	    thr,
	    duk_hobject_prototype_chain_contains(thr, DUK_HOBJECT_GET_PROTOTYPE(thr->heap, h_v), h_obj, 0 /*ignore_loop*/));
	return 1;
}
#endif /* DUK_USE_OBJECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_object_prototype_has_own_property(duk_hthread *thr) {
	return (duk_ret_t) duk_hobject_object_ownprop_helper(thr, 0 /*required_desc_flags*/);
}
#endif /* DUK_USE_OBJECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_object_prototype_property_is_enumerable(duk_hthread *thr) {
	return (duk_ret_t) duk_hobject_object_ownprop_helper(thr, DUK_PROPDESC_FLAG_ENUMERABLE /*required_desc_flags*/);
}
#endif /* DUK_USE_OBJECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN) || defined(DUK_USE_REFLECT_BUILTIN)
/* Shared helper to implement Object.getPrototypeOf,
 * Object.prototype.__proto__ getter, and Reflect.getPrototypeOf.
 *
 * http://www.ecma-international.org/ecma-262/6.0/index.html#sec-get-object.prototype.__proto__
 */
DUK_INTERNAL duk_ret_t duk_bi_object_getprototype_shared(duk_hthread *thr) {
	/*
	 *  magic = 0: __proto__ getter
	 *  magic = 1: Object.getPrototypeOf()
	 *  magic = 2: Reflect.getPrototypeOf()
	 */

	duk_hobject *h;
	duk_hobject *proto;
	duk_tval *tv;
	duk_int_t magic;

	magic = duk_get_current_magic(thr);

	if (magic == 0) {
		DUK_ASSERT_TOP(thr, 0);
		duk_push_this_coercible_to_object(thr);
	}
	DUK_ASSERT(duk_get_top(thr) >= 1);
	if (magic < 2) {
		/* ES2015 Section 19.1.2.9, step 1 */
		duk_to_object(thr, 0);
	}
	tv = DUK_GET_TVAL_POSIDX(thr, 0);

	switch (DUK_TVAL_GET_TAG(tv)) {
	case DUK_TAG_BUFFER:
		proto = thr->builtins[DUK_BIDX_UINT8ARRAY_PROTOTYPE];
		break;
	case DUK_TAG_LIGHTFUNC:
		proto = thr->builtins[DUK_BIDX_FUNCTION_PROTOTYPE];
		break;
	case DUK_TAG_OBJECT:
		h = DUK_TVAL_GET_OBJECT(tv);
		proto = DUK_HOBJECT_GET_PROTOTYPE(thr->heap, h);
		break;
	default:
		/* This implicitly handles CheckObjectCoercible() caused
		 * TypeError.
		 */
		DUK_DCERROR_TYPE_INVALID_ARGS(thr);
	}
	if (proto != NULL) {
		duk_push_hobject(thr, proto);
	} else {
		duk_push_null(thr);
	}
	return 1;
}
#endif /* DUK_USE_OBJECT_BUILTIN || DUK_USE_REFLECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN) || defined(DUK_USE_REFLECT_BUILTIN)
/* Shared helper to implement ES2015 Object.setPrototypeOf,
 * Object.prototype.__proto__ setter, and Reflect.setPrototypeOf.
 *
 * http://www.ecma-international.org/ecma-262/6.0/index.html#sec-get-object.prototype.__proto__
 * http://www.ecma-international.org/ecma-262/6.0/index.html#sec-object.setprototypeof
 */
DUK_INTERNAL duk_ret_t duk_bi_object_setprototype_shared(duk_hthread *thr) {
	/*
	 *  magic = 0: __proto__ setter
	 *  magic = 1: Object.setPrototypeOf()
	 *  magic = 2: Reflect.setPrototypeOf()
	 */

	duk_hobject *h_obj;
	duk_hobject *h_new_proto;
	duk_hobject *h_curr;
	duk_ret_t ret_success = 1; /* retval for success path */
	duk_uint_t mask;
	duk_int_t magic;

	/* Preliminaries for __proto__ and setPrototypeOf (E6 19.1.2.18 steps 1-4). */
	magic = duk_get_current_magic(thr);
	if (magic == 0) {
		duk_push_this_check_object_coercible(thr);
		duk_insert(thr, 0);
		if (!duk_check_type_mask(thr, 1, DUK_TYPE_MASK_NULL | DUK_TYPE_MASK_OBJECT)) {
			return 0;
		}

		/* __proto__ setter returns 'undefined' on success unlike the
		 * setPrototypeOf() call which returns the target object.
		 */
		ret_success = 0;
	} else {
		if (magic == 1) {
			duk_require_object_coercible(thr, 0);
		} else {
			duk_require_hobject_accept_mask(thr, 0, DUK_TYPE_MASK_LIGHTFUNC | DUK_TYPE_MASK_BUFFER);
		}
		duk_require_type_mask(thr, 1, DUK_TYPE_MASK_NULL | DUK_TYPE_MASK_OBJECT);
	}

	h_new_proto = duk_get_hobject(thr, 1);
	/* h_new_proto may be NULL */

	mask = duk_get_type_mask(thr, 0);
	if (mask & (DUK_TYPE_MASK_LIGHTFUNC | DUK_TYPE_MASK_BUFFER)) {
		duk_hobject *curr_proto;
		curr_proto =
		    thr->builtins[(mask & DUK_TYPE_MASK_LIGHTFUNC) ? DUK_BIDX_FUNCTION_PROTOTYPE : DUK_BIDX_UINT8ARRAY_PROTOTYPE];
		if (h_new_proto == curr_proto) {
			goto skip;
		}
		goto fail_nonextensible;
	}
	h_obj = duk_get_hobject(thr, 0);
	if (h_obj == NULL) {
		goto skip;
	}
	DUK_ASSERT(h_obj != NULL);

	/* [[SetPrototypeOf]] standard behavior, E6 9.1.2. */
	/* TODO: implement Proxy object support here */

	if (h_new_proto == DUK_HOBJECT_GET_PROTOTYPE(thr->heap, h_obj)) {
		goto skip;
	}
	if (!DUK_HOBJECT_HAS_EXTENSIBLE(h_obj)) {
		goto fail_nonextensible;
	}
	for (h_curr = h_new_proto; h_curr != NULL; h_curr = DUK_HOBJECT_GET_PROTOTYPE(thr->heap, h_curr)) {
		/* Loop prevention. */
		if (h_curr == h_obj) {
			goto fail_loop;
		}
	}
	DUK_HOBJECT_SET_PROTOTYPE_UPDREF(thr, h_obj, h_new_proto);
	/* fall thru */

skip:
	duk_set_top(thr, 1);
	if (magic == 2) {
		duk_push_true(thr);
	}
	return ret_success;

fail_nonextensible:
fail_loop:
	if (magic != 2) {
		DUK_DCERROR_TYPE_INVALID_ARGS(thr);
	} else {
		duk_push_false(thr);
		return 1;
	}
}
#endif /* DUK_USE_OBJECT_BUILTIN || DUK_USE_REFLECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN) || defined(DUK_USE_REFLECT_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_object_constructor_define_property(duk_hthread *thr) {
	/*
	 *  magic = 0: Object.defineProperty()
	 *  magic = 1: Reflect.defineProperty()
	 */

	duk_hobject *obj;
	duk_hstring *key;
	duk_hobject *get;
	duk_hobject *set;
	duk_idx_t idx_value;
	duk_uint_t defprop_flags;
	duk_small_uint_t magic;
	duk_bool_t throw_flag;
	duk_bool_t ret;

	DUK_ASSERT(thr != NULL);

	DUK_DDD(DUK_DDDPRINT("Object.defineProperty(): ctx=%p obj=%!T key=%!T desc=%!T",
	                     (void *) thr,
	                     (duk_tval *) duk_get_tval(thr, 0),
	                     (duk_tval *) duk_get_tval(thr, 1),
	                     (duk_tval *) duk_get_tval(thr, 2)));

	/* [ obj key desc ] */

	magic = (duk_small_uint_t) duk_get_current_magic(thr);

	/* Lightfuncs are currently supported by coercing to a temporary
	 * Function object; changes will be allowed (the coerced value is
	 * extensible) but will be lost.  Same for plain buffers.
	 */
	obj = duk_require_hobject_promote_mask(thr, 0, DUK_TYPE_MASK_LIGHTFUNC | DUK_TYPE_MASK_BUFFER);
	DUK_ASSERT(obj != NULL);
	key = duk_to_property_key_hstring(thr, 1);
	(void) duk_require_hobject(thr, 2);

	DUK_ASSERT(obj != NULL);
	DUK_ASSERT(key != NULL);
	DUK_ASSERT(duk_get_hobject(thr, 2) != NULL);

	/*
	 *  Validate and convert argument property descriptor (an ECMAScript
	 *  object) into a set of defprop_flags and possibly property value,
	 *  getter, and/or setter values on the value stack.
	 *
	 *  Lightfunc set/get values are coerced to full Functions.
	 */

	duk_hobject_prepare_property_descriptor(thr, 2 /*idx_desc*/, &defprop_flags, &idx_value, &get, &set);

	/*
	 *  Use Object.defineProperty() helper for the actual operation.
	 */

	DUK_ASSERT(magic == 0U || magic == 1U);
	throw_flag = magic ^ 1U;
	ret = duk_hobject_define_property_helper(thr, defprop_flags, obj, key, idx_value, get, set, throw_flag);

	/* Ignore the normalize/validate helper outputs on the value stack,
	 * they're popped automatically.
	 */

	if (magic == 0U) {
		/* Object.defineProperty(): return target object. */
		duk_push_hobject(thr, obj);
	} else {
		/* Reflect.defineProperty(): return success/fail. */
		duk_push_boolean(thr, ret);
	}
	return 1;
}
#endif /* DUK_USE_OBJECT_BUILTIN || DUK_USE_REFLECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN) || defined(DUK_USE_REFLECT_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_object_constructor_get_own_property_descriptor(duk_hthread *thr) {
	DUK_ASSERT_TOP(thr, 2);

	/* ES2015 Section 19.1.2.6, step 1 */
	if (duk_get_current_magic(thr) == 0) {
		duk_to_object(thr, 0);
	}

	/* [ obj key ] */

	duk_hobject_object_get_own_property_descriptor(thr, -2);
	return 1;
}
#endif /* DUK_USE_OBJECT_BUILTIN || DUK_USE_REFLECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN) || defined(DUK_USE_REFLECT_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_object_constructor_is_extensible(duk_hthread *thr) {
	/*
	 *  magic = 0: Object.isExtensible()
	 *  magic = 1: Reflect.isExtensible()
	 */

	duk_hobject *h;

	if (duk_get_current_magic(thr) == 0) {
		h = duk_get_hobject(thr, 0);
	} else {
		/* Reflect.isExtensible(): throw if non-object, but we accept lightfuncs
		 * and plain buffers here because they pretend to be objects.
		 */
		h = duk_require_hobject_accept_mask(thr, 0, DUK_TYPE_MASK_LIGHTFUNC | DUK_TYPE_MASK_BUFFER);
	}

	duk_push_boolean(thr, (h != NULL) && DUK_HOBJECT_HAS_EXTENSIBLE(h));
	return 1;
}
#endif /* DUK_USE_OBJECT_BUILTIN || DUK_USE_REFLECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN) || defined(DUK_USE_REFLECT_BUILTIN)
/* Shared helper for various key/symbol listings, magic:
 * 0=Object.keys()
 * 1=Object.getOwnPropertyNames(),
 * 2=Object.getOwnPropertySymbols(),
 * 3=Reflect.ownKeys()
 */
DUK_LOCAL const duk_small_uint_t duk__object_keys_enum_flags[4] = {
	/* Object.keys() */
	DUK_ENUM_OWN_PROPERTIES_ONLY | DUK_ENUM_NO_PROXY_BEHAVIOR,

	/* Object.getOwnPropertyNames() */
	DUK_ENUM_INCLUDE_NONENUMERABLE | DUK_ENUM_OWN_PROPERTIES_ONLY | DUK_ENUM_NO_PROXY_BEHAVIOR,

	/* Object.getOwnPropertySymbols() */
	DUK_ENUM_INCLUDE_SYMBOLS | DUK_ENUM_OWN_PROPERTIES_ONLY | DUK_ENUM_EXCLUDE_STRINGS | DUK_ENUM_INCLUDE_NONENUMERABLE |
	    DUK_ENUM_NO_PROXY_BEHAVIOR,

	/* Reflect.ownKeys() */
	DUK_ENUM_INCLUDE_SYMBOLS | DUK_ENUM_OWN_PROPERTIES_ONLY | DUK_ENUM_INCLUDE_NONENUMERABLE | DUK_ENUM_NO_PROXY_BEHAVIOR
};

DUK_INTERNAL duk_ret_t duk_bi_object_constructor_keys_shared(duk_hthread *thr) {
	duk_hobject *obj;
#if defined(DUK_USE_ES6_PROXY)
	duk_hobject *h_proxy_target;
	duk_hobject *h_proxy_handler;
	duk_hobject *h_trap_result;
#endif
	duk_small_uint_t enum_flags;
	duk_int_t magic;

	DUK_ASSERT_TOP(thr, 1);

	magic = duk_get_current_magic(thr);
	if (magic == 3) {
		/* ES2015 Section 26.1.11 requires a TypeError for non-objects.  Lightfuncs
		 * and plain buffers pretend to be objects, so accept those too.
		 */
		obj = duk_require_hobject_promote_mask(thr, 0, DUK_TYPE_MASK_LIGHTFUNC | DUK_TYPE_MASK_BUFFER);
	} else {
		/* ES2015: ToObject coerce. */
		obj = duk_to_hobject(thr, 0);
	}
	DUK_ASSERT(obj != NULL);
	DUK_UNREF(obj);

	/* XXX: proxy chains */

#if defined(DUK_USE_ES6_PROXY)
	/* XXX: better sharing of code between proxy target call sites */
	if (DUK_LIKELY(!duk_hobject_proxy_check(obj, &h_proxy_target, &h_proxy_handler))) {
		goto skip_proxy;
	}

	duk_push_hobject(thr, h_proxy_handler);
	if (!duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_OWN_KEYS)) {
		/* Careful with reachability here: don't pop 'obj' before pushing
		 * proxy target.
		 */
		DUK_DDD(DUK_DDDPRINT("no ownKeys trap, get keys of target instead"));
		duk_pop_2(thr);
		duk_push_hobject(thr, h_proxy_target);
		duk_replace(thr, 0);
		DUK_ASSERT_TOP(thr, 1);
		goto skip_proxy;
	}

	/* [ obj handler trap ] */
	duk_insert(thr, -2);
	duk_push_hobject(thr, h_proxy_target); /* -> [ obj trap handler target ] */
	duk_call_method(thr, 1 /*nargs*/); /* -> [ obj trap_result ] */
	h_trap_result = duk_require_hobject(thr, -1);
	DUK_UNREF(h_trap_result);

	magic = duk_get_current_magic(thr);
	DUK_ASSERT(magic >= 0 && magic < (duk_int_t) (sizeof(duk__object_keys_enum_flags) / sizeof(duk_small_uint_t)));
	enum_flags = duk__object_keys_enum_flags[magic];

	duk_proxy_ownkeys_postprocess(thr, h_proxy_target, enum_flags);
	return 1;

skip_proxy:
#endif /* DUK_USE_ES6_PROXY */

	DUK_ASSERT_TOP(thr, 1);
	magic = duk_get_current_magic(thr);
	DUK_ASSERT(magic >= 0 && magic < (duk_int_t) (sizeof(duk__object_keys_enum_flags) / sizeof(duk_small_uint_t)));
	enum_flags = duk__object_keys_enum_flags[magic];
	return duk_hobject_get_enumerated_keys(thr, enum_flags);
}
#endif /* DUK_USE_OBJECT_BUILTIN || DUK_USE_REFLECT_BUILTIN */

#if defined(DUK_USE_OBJECT_BUILTIN) || defined(DUK_USE_REFLECT_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_object_constructor_prevent_extensions(duk_hthread *thr) {
	/*
	 *  magic = 0: Object.preventExtensions()
	 *  magic = 1: Reflect.preventExtensions()
	 */

	duk_hobject *h;
	duk_uint_t mask;
	duk_int_t magic;

	magic = duk_get_current_magic(thr);

	/* Silent success for lightfuncs and plain buffers always. */
	mask = DUK_TYPE_MASK_LIGHTFUNC | DUK_TYPE_MASK_BUFFER;

	/* Object.preventExtensions() silent success for non-object. */
	if (magic == 0) {
		mask |= DUK_TYPE_MASK_UNDEFINED | DUK_TYPE_MASK_NULL | DUK_TYPE_MASK_BOOLEAN | DUK_TYPE_MASK_NUMBER |
		        DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_POINTER;
	}

	if (duk_check_type_mask(thr, 0, mask)) {
		/* Not an object, already non-extensible so always success. */
		goto done;
	}
	h = duk_require_hobject(thr, 0);
	DUK_ASSERT(h != NULL);

	DUK_HOBJECT_CLEAR_EXTENSIBLE(h);

	/* A non-extensible object cannot gain any more properties,
	 * so this is a good time to compact.
	 */
	duk_hobject_compact_props(thr, h);

done:
	if (magic == 1) {
		duk_push_true(thr);
	}
	return 1;
}
#endif /* DUK_USE_OBJECT_BUILTIN || DUK_USE_REFLECT_BUILTIN */

/*
 *  __defineGetter__, __defineSetter__, __lookupGetter__, __lookupSetter__
 */

#if defined(DUK_USE_ES8)
DUK_INTERNAL duk_ret_t duk_bi_object_prototype_defineaccessor(duk_hthread *thr) {
	duk_push_this(thr);
	duk_insert(thr, 0);
	duk_to_object(thr, 0);
	duk_require_callable(thr, 2);

	/* [ ToObject(this) key getter/setter ] */

	/* ToPropertyKey() coercion is not needed, duk_def_prop() does it. */
	duk_def_prop(thr,
	             0,
	             DUK_DEFPROP_SET_ENUMERABLE | DUK_DEFPROP_SET_CONFIGURABLE |
	                 (duk_get_current_magic(thr) ? DUK_DEFPROP_HAVE_SETTER : DUK_DEFPROP_HAVE_GETTER));
	return 0;
}
DUK_INTERNAL duk_ret_t duk_bi_object_prototype_lookupaccessor(duk_hthread *thr) {
	duk_uint_t sanity;

	duk_push_this(thr);
	duk_to_object(thr, -1);

	/* XXX: Prototype walk (with sanity) should be a core property
	 * operation, could add a flag to e.g. duk_get_prop_desc().
	 */

	/* ToPropertyKey() coercion is not needed, duk_get_prop_desc() does it. */
	sanity = DUK_HOBJECT_PROTOTYPE_CHAIN_SANITY;
	while (!duk_is_undefined(thr, -1)) {
		/* [ key obj ] */
		duk_dup(thr, 0);
		duk_get_prop_desc(thr, 1, 0 /*flags*/);
		if (!duk_is_undefined(thr, -1)) {
			duk_get_prop_stridx(thr, -1, (duk_get_current_magic(thr) != 0 ? DUK_STRIDX_SET : DUK_STRIDX_GET));
			return 1;
		}
		duk_pop(thr);

		if (DUK_UNLIKELY(sanity-- == 0)) {
			DUK_ERROR_RANGE(thr, DUK_STR_PROTOTYPE_CHAIN_LIMIT);
			DUK_WO_NORETURN(return 0;);
		}

		duk_get_prototype(thr, -1);
		duk_remove(thr, -2);
	}
	return 1;
}
#endif /* DUK_USE_ES8 */
