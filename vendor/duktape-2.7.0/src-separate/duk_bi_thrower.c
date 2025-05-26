/*
 *  Type error thrower, E5 Section 13.2.3.
 */

#include "duk_internal.h"

DUK_INTERNAL duk_ret_t duk_bi_type_error_thrower(duk_hthread *thr) {
	DUK_DCERROR_TYPE_INVALID_ARGS(thr);
}
