/*
 *  Date/time.
 */

#include "duk_internal.h"

DUK_INTERNAL duk_double_t duk_time_get_ecmascript_time(duk_hthread *thr) {
	/* ECMAScript time, with millisecond fractions.  Exposed via
	 * duk_get_now() for example.
	 */
	DUK_UNREF(thr);
	return (duk_double_t) DUK_USE_DATE_GET_NOW(thr);
}

DUK_INTERNAL duk_double_t duk_time_get_ecmascript_time_nofrac(duk_hthread *thr) {
	/* ECMAScript time without millisecond fractions.  Exposed via
	 * the Date built-in which doesn't allow fractions.
	 */
	DUK_UNREF(thr);
	return (duk_double_t) DUK_FLOOR(DUK_USE_DATE_GET_NOW(thr));
}

DUK_INTERNAL duk_double_t duk_time_get_monotonic_time(duk_hthread *thr) {
	DUK_UNREF(thr);
#if defined(DUK_USE_GET_MONOTONIC_TIME)
	return (duk_double_t) DUK_USE_GET_MONOTONIC_TIME(thr);
#else
	return (duk_double_t) DUK_USE_DATE_GET_NOW(thr);
#endif
}

DUK_EXTERNAL duk_double_t duk_get_now(duk_hthread *thr) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_UNREF(thr);

	/* This API intentionally allows millisecond fractions. */
	return duk_time_get_ecmascript_time(thr);
}

#if 0 /* XXX: worth exposing? */
DUK_EXTERNAL duk_double_t duk_get_monotonic_time(duk_hthread *thr) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_UNREF(thr);

	return duk_time_get_monotonic_time(thr);
}
#endif

DUK_EXTERNAL void duk_time_to_components(duk_hthread *thr, duk_double_t timeval, duk_time_components *comp) {
	duk_int_t parts[DUK_DATE_IDX_NUM_PARTS];
	duk_double_t dparts[DUK_DATE_IDX_NUM_PARTS];
	duk_uint_t flags;

	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(comp != NULL); /* XXX: or check? */
	DUK_UNREF(thr);

	/* Convert as one-based, but change month to zero-based to match the
	 * ECMAScript Date built-in behavior 1:1.
	 */
	flags = DUK_DATE_FLAG_ONEBASED | DUK_DATE_FLAG_NAN_TO_ZERO;

	duk_bi_date_timeval_to_parts(timeval, parts, dparts, flags);

	/* XXX: sub-millisecond accuracy for the API */

	DUK_ASSERT(dparts[DUK_DATE_IDX_MONTH] >= 1.0 && dparts[DUK_DATE_IDX_MONTH] <= 12.0);
	comp->year = dparts[DUK_DATE_IDX_YEAR];
	comp->month = dparts[DUK_DATE_IDX_MONTH] - 1.0;
	comp->day = dparts[DUK_DATE_IDX_DAY];
	comp->hours = dparts[DUK_DATE_IDX_HOUR];
	comp->minutes = dparts[DUK_DATE_IDX_MINUTE];
	comp->seconds = dparts[DUK_DATE_IDX_SECOND];
	comp->milliseconds = dparts[DUK_DATE_IDX_MILLISECOND];
	comp->weekday = dparts[DUK_DATE_IDX_WEEKDAY];
}

DUK_EXTERNAL duk_double_t duk_components_to_time(duk_hthread *thr, duk_time_components *comp) {
	duk_double_t d;
	duk_double_t dparts[DUK_DATE_IDX_NUM_PARTS];
	duk_uint_t flags;

	DUK_ASSERT_API_ENTRY(thr);
	DUK_ASSERT(comp != NULL); /* XXX: or check? */
	DUK_UNREF(thr);

	/* Match Date constructor behavior (with UTC time).  Month is given
	 * as zero-based.  Day-of-month is given as one-based so normalize
	 * it to zero-based as the internal conversion helpers expects all
	 * components to be zero-based.
	 */
	flags = 0;

	/* XXX: expensive conversion; use array format in API instead, or unify
	 * time provider and time API to use same struct?
	 */

	dparts[DUK_DATE_IDX_YEAR] = comp->year;
	dparts[DUK_DATE_IDX_MONTH] = comp->month;
	dparts[DUK_DATE_IDX_DAY] = comp->day - 1.0;
	dparts[DUK_DATE_IDX_HOUR] = comp->hours;
	dparts[DUK_DATE_IDX_MINUTE] = comp->minutes;
	dparts[DUK_DATE_IDX_SECOND] = comp->seconds;
	dparts[DUK_DATE_IDX_MILLISECOND] = comp->milliseconds;
	dparts[DUK_DATE_IDX_WEEKDAY] = 0; /* ignored */

	d = duk_bi_date_get_timeval_from_dparts(dparts, flags);

	return d;
}
