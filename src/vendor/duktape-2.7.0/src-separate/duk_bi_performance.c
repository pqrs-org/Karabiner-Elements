/*
 *  High resolution time API (performance.now() et al)
 *
 *  API specification: https://encoding.spec.whatwg.org/#ap://www.w3.org/TR/hr-time/
 */

#include "duk_internal.h"

#if defined(DUK_USE_PERFORMANCE_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_performance_now(duk_hthread *thr) {
	/* From API spec:
	 * The DOMHighResTimeStamp type is used to store a time value in
	 * milliseconds, measured relative from the time origin, global
	 * monotonic clock, or a time value that represents a duration
	 * between two DOMHighResTimeStamp's.
	 */
	duk_push_number(thr, duk_time_get_monotonic_time(thr));
	return 1;
}

#if 0 /* Missing until semantics decided. */
DUK_INTERNAL duk_ret_t duk_bi_performance_timeorigin_getter(duk_hthread *thr) {
	/* No decision yet how to handle timeOrigins, e.g. should one be
	 * initialized per heap, or per global object set.  See
	 * https://www.w3.org/TR/hr-time/#time-origin.
	 */
	duk_push_uint(thr, 0);
	return 1;
}
#endif /* 0 */
#endif /* DUK_USE_PERFORMANCE_BUILTIN */
