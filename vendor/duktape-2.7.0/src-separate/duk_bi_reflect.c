/*
 *  'Reflect' built-in (ES2016 Section 26.1)
 *  http://www.ecma-international.org/ecma-262/7.0/#sec-reflect-object
 *
 *  Many Reflect built-in functions are provided by shared helpers in
 *  duk_bi_object.c or duk_bi_function.c.
 */

#include "duk_internal.h"

#if defined(DUK_USE_REFLECT_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_reflect_object_delete_property(duk_hthread *thr) {
	duk_tval *tv_obj;
	duk_tval *tv_key;
	duk_bool_t ret;

	DUK_ASSERT_TOP(thr, 2);
	(void) duk_require_hobject(thr, 0);
	(void) duk_to_string(thr, 1);

	/* [ target key ] */

	DUK_ASSERT(thr != NULL);
	tv_obj = DUK_GET_TVAL_POSIDX(thr, 0);
	tv_key = DUK_GET_TVAL_POSIDX(thr, 1);
	ret = duk_hobject_delprop(thr, tv_obj, tv_key, 0 /*throw_flag*/);
	duk_push_boolean(thr, ret);
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_reflect_object_get(duk_hthread *thr) {
	duk_tval *tv_obj;
	duk_tval *tv_key;
	duk_idx_t nargs;

	DUK_ASSERT(thr != NULL);
	nargs = duk_get_top_require_min(thr, 2 /*min_top*/);
	(void) duk_require_hobject(thr, 0);
	(void) duk_to_string(thr, 1);
	if (nargs >= 3 && !duk_strict_equals(thr, 0, 2)) {
		/* XXX: [[Get]] receiver currently unsupported */
		DUK_ERROR_UNSUPPORTED(thr);
		DUK_WO_NORETURN(return 0;);
	}

	/* [ target key receiver? ...? ] */

	tv_obj = DUK_GET_TVAL_POSIDX(thr, 0);
	tv_key = DUK_GET_TVAL_POSIDX(thr, 1);
	(void) duk_hobject_getprop(thr, tv_obj, tv_key); /* This could also be a duk_get_prop(). */
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_reflect_object_has(duk_hthread *thr) {
	duk_tval *tv_obj;
	duk_tval *tv_key;
	duk_bool_t ret;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT_TOP(thr, 2);
	(void) duk_require_hobject(thr, 0);
	(void) duk_to_string(thr, 1);

	/* [ target key ] */

	tv_obj = DUK_GET_TVAL_POSIDX(thr, 0);
	tv_key = DUK_GET_TVAL_POSIDX(thr, 1);
	ret = duk_hobject_hasprop(thr, tv_obj, tv_key);
	duk_push_boolean(thr, ret);
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_reflect_object_set(duk_hthread *thr) {
	duk_tval *tv_obj;
	duk_tval *tv_key;
	duk_tval *tv_val;
	duk_idx_t nargs;
	duk_bool_t ret;

	DUK_ASSERT(thr != NULL);
	nargs = duk_get_top_require_min(thr, 3 /*min_top*/);
	(void) duk_require_hobject(thr, 0);
	(void) duk_to_string(thr, 1);
	if (nargs >= 4 && !duk_strict_equals(thr, 0, 3)) {
		/* XXX: [[Set]] receiver currently unsupported */
		DUK_ERROR_UNSUPPORTED(thr);
		DUK_WO_NORETURN(return 0;);
	}

	/* [ target key value receiver? ...? ] */

	tv_obj = DUK_GET_TVAL_POSIDX(thr, 0);
	tv_key = DUK_GET_TVAL_POSIDX(thr, 1);
	tv_val = DUK_GET_TVAL_POSIDX(thr, 2);
	ret = duk_hobject_putprop(thr, tv_obj, tv_key, tv_val, 0 /*throw_flag*/);
	duk_push_boolean(thr, ret);
	return 1;
}
#endif /* DUK_USE_REFLECT_BUILTIN */
