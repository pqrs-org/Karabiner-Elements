/*
 *  Windows Date providers
 *
 *  Platform specific links:
 *
 *    - http://msdn.microsoft.com/en-us/library/windows/desktop/ms725473(v=vs.85).aspx
 */

#include "duk_internal.h"

/* The necessary #includes are in place in duk_config.h. */

#if defined(DUK_USE_DATE_NOW_WINDOWS) || defined(DUK_USE_DATE_TZO_WINDOWS)
/* Shared Windows helpers. */
DUK_LOCAL void duk__convert_systime_to_ularge(const SYSTEMTIME *st, ULARGE_INTEGER *res) {
	FILETIME ft;
	if (SystemTimeToFileTime(st, &ft) == 0) {
		DUK_D(DUK_DPRINT("SystemTimeToFileTime() failed, returning 0"));
		res->QuadPart = 0;
	} else {
		res->LowPart = ft.dwLowDateTime;
		res->HighPart = ft.dwHighDateTime;
	}
}

#if defined(DUK_USE_DATE_NOW_WINDOWS_SUBMS)
DUK_LOCAL void duk__convert_filetime_to_ularge(const FILETIME *ft, ULARGE_INTEGER *res) {
	res->LowPart = ft->dwLowDateTime;
	res->HighPart = ft->dwHighDateTime;
}
#endif /* DUK_USE_DATE_NOW_WINDOWS_SUBMS */

DUK_LOCAL void duk__set_systime_jan1970(SYSTEMTIME *st) {
	duk_memzero((void *) st, sizeof(*st));
	st->wYear = 1970;
	st->wMonth = 1;
	st->wDayOfWeek = 4; /* not sure whether or not needed; Thursday */
	st->wDay = 1;
	DUK_ASSERT(st->wHour == 0);
	DUK_ASSERT(st->wMinute == 0);
	DUK_ASSERT(st->wSecond == 0);
	DUK_ASSERT(st->wMilliseconds == 0);
}
#endif /* defined(DUK_USE_DATE_NOW_WINDOWS) || defined(DUK_USE_DATE_TZO_WINDOWS) */

#if defined(DUK_USE_DATE_NOW_WINDOWS)
DUK_INTERNAL duk_double_t duk_bi_date_get_now_windows(void) {
	/* Suggested step-by-step method from documentation of RtlTimeToSecondsSince1970:
	 * http://msdn.microsoft.com/en-us/library/windows/desktop/ms724928(v=vs.85).aspx
	 */
	SYSTEMTIME st1, st2;
	ULARGE_INTEGER tmp1, tmp2;

	GetSystemTime(&st1);
	duk__convert_systime_to_ularge((const SYSTEMTIME *) &st1, &tmp1);

	duk__set_systime_jan1970(&st2);
	duk__convert_systime_to_ularge((const SYSTEMTIME *) &st2, &tmp2);

	/* Difference is in 100ns units, convert to milliseconds, keeping
	 * fractions since Duktape 2.2.0.  This is only theoretical because
	 * SYSTEMTIME is limited to milliseconds.
	 */
	return (duk_double_t) ((LONGLONG) tmp1.QuadPart - (LONGLONG) tmp2.QuadPart) / 10000.0;
}
#endif /* DUK_USE_DATE_NOW_WINDOWS */

#if defined(DUK_USE_DATE_NOW_WINDOWS_SUBMS)
DUK_INTERNAL duk_double_t duk_bi_date_get_now_windows_subms(void) {
	/* Variant of the basic algorithm using GetSystemTimePreciseAsFileTime()
	 * for more accuracy.
	 */
	FILETIME ft1;
	SYSTEMTIME st2;
	ULARGE_INTEGER tmp1, tmp2;

	GetSystemTimePreciseAsFileTime(&ft1);
	duk__convert_filetime_to_ularge((const FILETIME *) &ft1, &tmp1);

	duk__set_systime_jan1970(&st2);
	duk__convert_systime_to_ularge((const SYSTEMTIME *) &st2, &tmp2);

	/* Difference is in 100ns units, convert to milliseconds, keeping
	 * fractions since Duktape 2.2.0.
	 */
	return (duk_double_t) ((LONGLONG) tmp1.QuadPart - (LONGLONG) tmp2.QuadPart) / 10000.0;
}
#endif /* DUK_USE_DATE_NOW_WINDOWS */

#if defined(DUK_USE_DATE_TZO_WINDOWS)
DUK_INTERNAL duk_int_t duk_bi_date_get_local_tzoffset_windows(duk_double_t d) {
	SYSTEMTIME st1;
	SYSTEMTIME st2;
	SYSTEMTIME st3;
	ULARGE_INTEGER tmp1;
	ULARGE_INTEGER tmp2;
	ULARGE_INTEGER tmp3;
	FILETIME ft1;

	/* XXX: handling of timestamps outside Windows supported range.
	 * How does Windows deal with dates before 1600?  Does windows
	 * support all ECMAScript years (like -200000 and +200000)?
	 * Should equivalent year mapping be used here too?  If so, use
	 * a shared helper (currently integrated into timeval-to-parts).
	 */

	/* Use the approach described in "Remarks" of FileTimeToLocalFileTime:
	 * http://msdn.microsoft.com/en-us/library/windows/desktop/ms724277(v=vs.85).aspx
	 */

	duk__set_systime_jan1970(&st1);
	duk__convert_systime_to_ularge((const SYSTEMTIME *) &st1, &tmp1);
	tmp2.QuadPart = (ULONGLONG) (d * 10000.0); /* millisec -> 100ns units since jan 1, 1970 */
	tmp2.QuadPart += tmp1.QuadPart; /* input 'd' in Windows UTC, 100ns units */

	ft1.dwLowDateTime = tmp2.LowPart;
	ft1.dwHighDateTime = tmp2.HighPart;
	if (FileTimeToSystemTime((const FILETIME *) &ft1, &st2) == 0) {
		DUK_D(DUK_DPRINT("FileTimeToSystemTime() failed, return tzoffset 0"));
		return 0;
	}
	if (SystemTimeToTzSpecificLocalTime((LPTIME_ZONE_INFORMATION) NULL, &st2, &st3) == 0) {
		DUK_D(DUK_DPRINT("SystemTimeToTzSpecificLocalTime() failed, return tzoffset 0"));
		return 0;
	}
	duk__convert_systime_to_ularge((const SYSTEMTIME *) &st3, &tmp3);

	/* Positive if local time ahead of UTC. */
	return (duk_int_t) (((LONGLONG) tmp3.QuadPart - (LONGLONG) tmp2.QuadPart) / DUK_I64_CONSTANT(10000000)); /* seconds */
}
#endif /* DUK_USE_DATE_TZO_WINDOWS */

#if defined(DUK_USE_DATE_TZO_WINDOWS_NO_DST)
DUK_INTERNAL duk_int_t duk_bi_date_get_local_tzoffset_windows_no_dst(duk_double_t d) {
	SYSTEMTIME st1;
	SYSTEMTIME st2;
	FILETIME ft1;
	FILETIME ft2;
	ULARGE_INTEGER tmp1;
	ULARGE_INTEGER tmp2;

	/* Do a similar computation to duk_bi_date_get_local_tzoffset_windows
	 * but without accounting for daylight savings time.  Use this on
	 * Windows platforms (like Durango) that don't support the
	 * SystemTimeToTzSpecificLocalTime() call.
	 */

	/* current time not needed for this computation */
	DUK_UNREF(d);

	duk__set_systime_jan1970(&st1);
	duk__convert_systime_to_ularge((const SYSTEMTIME *) &st1, &tmp1);

	ft1.dwLowDateTime = tmp1.LowPart;
	ft1.dwHighDateTime = tmp1.HighPart;
	if (FileTimeToLocalFileTime((const FILETIME *) &ft1, &ft2) == 0) {
		DUK_D(DUK_DPRINT("FileTimeToLocalFileTime() failed, return tzoffset 0"));
		return 0;
	}
	if (FileTimeToSystemTime((const FILETIME *) &ft2, &st2) == 0) {
		DUK_D(DUK_DPRINT("FileTimeToSystemTime() failed, return tzoffset 0"));
		return 0;
	}
	duk__convert_systime_to_ularge((const SYSTEMTIME *) &st2, &tmp2);

	return (duk_int_t) (((LONGLONG) tmp2.QuadPart - (LONGLONG) tmp1.QuadPart) / DUK_I64_CONSTANT(10000000)); /* seconds */
}
#endif /* DUK_USE_DATE_TZO_WINDOWS_NO_DST */

#if defined(DUK_USE_GET_MONOTONIC_TIME_WINDOWS_QPC)
DUK_INTERNAL duk_double_t duk_bi_date_get_monotonic_time_windows_qpc(void) {
	LARGE_INTEGER count, freq;

	/* There are legacy issues with QueryPerformanceCounter():
	 * - Potential jumps:
	 * https://support.microsoft.com/en-us/help/274323/performance-counter-value-may-unexpectedly-leap-forward
	 * - Differences between cores (XP):
	 * https://msdn.microsoft.com/en-us/library/windows/desktop/dn553408(v=vs.85).aspx#qpc_support_in_windows_versions
	 *
	 * We avoid these by enabling QPC by default only for Vista or later.
	 */

	if (QueryPerformanceCounter(&count) && QueryPerformanceFrequency(&freq)) {
		/* XXX: QueryPerformanceFrequency() can be cached */
		return (duk_double_t) count.QuadPart / (duk_double_t) freq.QuadPart * 1000.0;
	} else {
		/* MSDN: "On systems that run Windows XP or later, the function
		 * will always succeed and will thus never return zero."
		 * Provide minimal error path just in case user enables this
		 * feature in pre-XP Windows.
		 */
		return 0.0;
	}
}
#endif /* DUK_USE_GET_MONOTONIC_TIME_WINDOWS_QPC */
