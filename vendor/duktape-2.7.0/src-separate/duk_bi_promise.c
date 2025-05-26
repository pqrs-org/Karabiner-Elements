/*
 *  Promise built-in
 */

#include "duk_internal.h"

#if defined(DUK_USE_PROMISE_BUILTIN)

DUK_INTERNAL duk_ret_t duk_bi_promise_constructor(duk_hthread *thr) {
	DUK_ERROR_TYPE(thr, "unimplemented");
	DUK_WO_NORETURN(return 0;);
}

DUK_INTERNAL duk_ret_t duk_bi_promise_all(duk_hthread *thr) {
	DUK_ERROR_TYPE(thr, "unimplemented");
	DUK_WO_NORETURN(return 0;);
}

DUK_INTERNAL duk_ret_t duk_bi_promise_race(duk_hthread *thr) {
	DUK_ERROR_TYPE(thr, "unimplemented");
	DUK_WO_NORETURN(return 0;);
}

DUK_INTERNAL duk_ret_t duk_bi_promise_reject(duk_hthread *thr) {
	DUK_ERROR_TYPE(thr, "unimplemented");
	DUK_WO_NORETURN(return 0;);
}

DUK_INTERNAL duk_ret_t duk_bi_promise_resolve(duk_hthread *thr) {
	DUK_ERROR_TYPE(thr, "unimplemented");
	DUK_WO_NORETURN(return 0;);
}

DUK_INTERNAL duk_ret_t duk_bi_promise_catch(duk_hthread *thr) {
	DUK_ERROR_TYPE(thr, "unimplemented");
	DUK_WO_NORETURN(return 0;);
}

DUK_INTERNAL duk_ret_t duk_bi_promise_then(duk_hthread *thr) {
	DUK_ERROR_TYPE(thr, "unimplemented");
	DUK_WO_NORETURN(return 0;);
}

#endif /* DUK_USE_PROMISE_BUILTIN */
