/*
 *  Pointer built-ins
 */

#include "duk_internal.h"

/*
 *  Constructor
 */

DUK_INTERNAL duk_ret_t duk_bi_pointer_constructor(duk_hthread *thr) {
	/* XXX: this behavior is quite useless now; it would be nice to be able
	 * to create pointer values from e.g. numbers or strings.  Numbers are
	 * problematic on 64-bit platforms though.  Hex encoded strings?
	 */
	if (duk_get_top(thr) == 0) {
		duk_push_pointer(thr, NULL);
	} else {
		duk_to_pointer(thr, 0);
	}
	DUK_ASSERT(duk_is_pointer(thr, 0));
	duk_set_top(thr, 1);

	if (duk_is_constructor_call(thr)) {
		(void) duk_push_object_helper(thr,
		                              DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_FASTREFS |
		                                  DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_POINTER),
		                              DUK_BIDX_POINTER_PROTOTYPE);

		/* Pointer object internal value is immutable. */
		duk_dup_0(thr);
		duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_INT_VALUE, DUK_PROPDESC_FLAGS_NONE);
	}
	/* Note: unbalanced stack on purpose */

	return 1;
}

/*
 *  toString(), valueOf()
 */

DUK_INTERNAL duk_ret_t duk_bi_pointer_prototype_tostring_shared(duk_hthread *thr) {
	duk_tval *tv;
	duk_small_int_t to_string = duk_get_current_magic(thr);

	duk_push_this(thr);
	tv = duk_require_tval(thr, -1);
	DUK_ASSERT(tv != NULL);

	if (DUK_TVAL_IS_POINTER(tv)) {
		/* nop */
	} else if (DUK_TVAL_IS_OBJECT(tv)) {
		duk_hobject *h = DUK_TVAL_GET_OBJECT(tv);
		DUK_ASSERT(h != NULL);

		/* Must be a "pointer object", i.e. class "Pointer" */
		if (DUK_HOBJECT_GET_CLASS_NUMBER(h) != DUK_HOBJECT_CLASS_POINTER) {
			goto type_error;
		}

		duk_xget_owndataprop_stridx_short(thr, -1, DUK_STRIDX_INT_VALUE);
	} else {
		goto type_error;
	}

	if (to_string) {
		duk_to_string(thr, -1);
	}
	return 1;

type_error:
	DUK_DCERROR_TYPE_INVALID_ARGS(thr);
}
