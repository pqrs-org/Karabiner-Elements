/*
 *  Boolean built-ins
 */

#include "duk_internal.h"

#if defined(DUK_USE_BOOLEAN_BUILTIN)

/* Shared helper to provide toString() and valueOf().  Checks 'this', gets
 * the primitive value to stack top, and optionally coerces with ToString().
 */
DUK_INTERNAL duk_ret_t duk_bi_boolean_prototype_tostring_shared(duk_hthread *thr) {
	duk_tval *tv;
	duk_hobject *h;
	duk_small_int_t coerce_tostring = duk_get_current_magic(thr);

	/* XXX: there is room to use a shared helper here, many built-ins
	 * check the 'this' type, and if it's an object, check its class,
	 * then get its internal value, etc.
	 */

	duk_push_this(thr);
	tv = duk_get_tval(thr, -1);
	DUK_ASSERT(tv != NULL);

	if (DUK_TVAL_IS_BOOLEAN(tv)) {
		goto type_ok;
	} else if (DUK_TVAL_IS_OBJECT(tv)) {
		h = DUK_TVAL_GET_OBJECT(tv);
		DUK_ASSERT(h != NULL);

		if (DUK_HOBJECT_GET_CLASS_NUMBER(h) == DUK_HOBJECT_CLASS_BOOLEAN) {
			duk_xget_owndataprop_stridx_short(thr, -1, DUK_STRIDX_INT_VALUE);
			DUK_ASSERT(duk_is_boolean(thr, -1));
			goto type_ok;
		}
	}

	DUK_DCERROR_TYPE_INVALID_ARGS(thr);
	/* never here */

type_ok:
	if (coerce_tostring) {
		duk_to_string(thr, -1);
	}
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_boolean_constructor(duk_hthread *thr) {
	duk_hobject *h_this;

	duk_to_boolean(thr, 0);

	if (duk_is_constructor_call(thr)) {
		/* XXX: helper; rely on Boolean.prototype as being non-writable, non-configurable */
		duk_push_this(thr);
		h_this = duk_known_hobject(thr, -1);
		DUK_ASSERT(DUK_HOBJECT_GET_PROTOTYPE(thr->heap, h_this) == thr->builtins[DUK_BIDX_BOOLEAN_PROTOTYPE]);

		DUK_HOBJECT_SET_CLASS_NUMBER(h_this, DUK_HOBJECT_CLASS_BOOLEAN);

		duk_dup_0(thr); /* -> [ val obj val ] */
		duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_INT_VALUE, DUK_PROPDESC_FLAGS_NONE); /* XXX: proper flags? */
	} /* unbalanced stack */

	return 1;
}

#endif /* DUK_USE_BOOLEAN_BUILTIN */
