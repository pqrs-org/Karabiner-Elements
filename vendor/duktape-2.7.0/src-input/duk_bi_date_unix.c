/*
 *  Unix-like Date providers
 *
 *  Generally useful Unix / POSIX / ANSI Date providers.
 */

#include "duk_internal.h"

/* The necessary #includes are in place in duk_config.h. */

/* Buffer sizes for some UNIX calls.  Larger than strictly necessary
 * to avoid Valgrind errors.
 */
#define DUK__STRPTIME_BUF_SIZE 64
#define DUK__STRFTIME_BUF_SIZE 64

#if defined(DUK_USE_DATE_NOW_GETTIMEOFDAY)
/* Get current ECMAScript time (= UNIX/Posix time, but in milliseconds). */
DUK_INTERNAL duk_double_t duk_bi_date_get_now_gettimeofday(void) {
	struct timeval tv;
	duk_double_t d;

	if (gettimeofday(&tv, NULL) != 0) {
		DUK_D(DUK_DPRINT("gettimeofday() failed"));
		return 0.0;
	}

	/* As of Duktape 2.2.0 allow fractions. */
	d = ((duk_double_t) tv.tv_sec) * 1000.0 + ((duk_double_t) tv.tv_usec) / 1000.0;

	return d;
}
#endif /* DUK_USE_DATE_NOW_GETTIMEOFDAY */

#if defined(DUK_USE_DATE_NOW_TIME)
/* Not a very good provider: only full seconds are available. */
DUK_INTERNAL duk_double_t duk_bi_date_get_now_time(void) {
	time_t t;

	t = time(NULL);
	if (t == (time_t) -1) {
		DUK_D(DUK_DPRINT("time() failed"));
		return 0.0;
	}
	return ((duk_double_t) t) * 1000.0;
}
#endif /* DUK_USE_DATE_NOW_TIME */

#if defined(DUK_USE_DATE_TZO_GMTIME) || defined(DUK_USE_DATE_TZO_GMTIME_R) || defined(DUK_USE_DATE_TZO_GMTIME_S)
/* Get local time offset (in seconds) for a certain (UTC) instant 'd'. */
DUK_INTERNAL duk_int_t duk_bi_date_get_local_tzoffset_gmtime(duk_double_t d) {
	time_t t, t1, t2;
	duk_int_t parts[DUK_DATE_IDX_NUM_PARTS];
	duk_double_t dparts[DUK_DATE_IDX_NUM_PARTS];
	struct tm tms[2];
#if defined(DUK_USE_DATE_TZO_GMTIME)
	struct tm *tm_ptr;
#endif

	/* For NaN/inf, the return value doesn't matter. */
	if (!DUK_ISFINITE(d)) {
		return 0;
	}

	/* If not within ECMAScript range, some integer time calculations
	 * won't work correctly (and some asserts will fail), so bail out
	 * if so.  This fixes test-bug-date-insane-setyear.js.  There is
	 * a +/- 24h leeway in this range check to avoid a test262 corner
	 * case documented in test-bug-date-timeval-edges.js.
	 */
	if (!duk_bi_date_timeval_in_leeway_range(d)) {
		DUK_DD(DUK_DDPRINT("timeval not within valid range, skip tzoffset computation to avoid integer overflows"));
		return 0;
	}

	/*
	 *  This is a bit tricky to implement portably.  The result depends
	 *  on the timestamp (specifically, DST depends on the timestamp).
	 *  If e.g. UNIX APIs are used, they'll have portability issues with
	 *  very small and very large years.
	 *
	 *  Current approach:
	 *
	 *  - Stay within portable UNIX limits by using equivalent year mapping.
	 *    Avoid year 1970 and 2038 as some conversions start to fail, at
	 *    least on some platforms.  Avoiding 1970 means that there are
	 *    currently DST discrepancies for 1970.
	 *
	 *  - Create a UTC and local time breakdowns from 't'.  Then create
	 *    a time_t using gmtime() and localtime() and compute the time
	 *    difference between the two.
	 *
	 *  Equivalent year mapping (E5 Section 15.9.1.8):
	 *
	 *    If the host environment provides functionality for determining
	 *    daylight saving time, the implementation of ECMAScript is free
	 *    to map the year in question to an equivalent year (same
	 *    leap-year-ness and same starting week day for the year) for which
	 *    the host environment provides daylight saving time information.
	 *    The only restriction is that all equivalent years should produce
	 *    the same result.
	 *
	 *  This approach is quite reasonable but not entirely correct, e.g.
	 *  the specification also states (E5 Section 15.9.1.8):
	 *
	 *    The implementation of ECMAScript should not try to determine
	 *    whether the exact time was subject to daylight saving time, but
	 *    just whether daylight saving time would have been in effect if
	 *    the _current daylight saving time algorithm_ had been used at the
	 *    time.  This avoids complications such as taking into account the
	 *    years that the locale observed daylight saving time year round.
	 *
	 *  Since we rely on the platform APIs for conversions between local
	 *  time and UTC, we can't guarantee the above.  Rather, if the platform
	 *  has historical DST rules they will be applied.  This seems to be the
	 *  general preferred direction in ECMAScript standardization (or at least
	 *  implementations) anyway, and even the equivalent year mapping should
	 *  be disabled if the platform is known to handle DST properly for the
	 *  full ECMAScript range.
	 *
	 *  The following has useful discussion and links:
	 *
	 *    https://bugzilla.mozilla.org/show_bug.cgi?id=351066
	 */

	duk_bi_date_timeval_to_parts(d, parts, dparts, DUK_DATE_FLAG_EQUIVYEAR /*flags*/);
	DUK_ASSERT(parts[DUK_DATE_IDX_YEAR] >= 1970 && parts[DUK_DATE_IDX_YEAR] <= 2038);

	d = duk_bi_date_get_timeval_from_dparts(dparts, 0 /*flags*/);
	DUK_ASSERT(d >= 0 && d < 2147483648.0 * 1000.0); /* unsigned 31-bit range */
	t = (time_t) (d / 1000.0);
	DUK_DDD(DUK_DDDPRINT("timeval: %lf -> time_t %ld", (double) d, (long) t));

	duk_memzero((void *) tms, sizeof(struct tm) * 2);

#if defined(DUK_USE_DATE_TZO_GMTIME_R)
	(void) gmtime_r(&t, &tms[0]);
	(void) localtime_r(&t, &tms[1]);
#elif defined(DUK_USE_DATE_TZO_GMTIME_S)
	(void) gmtime_s(&t, &tms[0]);
	(void) localtime_s(&t, &tms[1]);
#elif defined(DUK_USE_DATE_TZO_GMTIME)
	tm_ptr = gmtime(&t);
	duk_memcpy((void *) &tms[0], tm_ptr, sizeof(struct tm));
	tm_ptr = localtime(&t);
	duk_memcpy((void *) &tms[1], tm_ptr, sizeof(struct tm));
#else
#error internal error
#endif
	DUK_DDD(DUK_DDDPRINT("gmtime result: tm={sec:%ld,min:%ld,hour:%ld,mday:%ld,mon:%ld,year:%ld,"
	                     "wday:%ld,yday:%ld,isdst:%ld}",
	                     (long) tms[0].tm_sec,
	                     (long) tms[0].tm_min,
	                     (long) tms[0].tm_hour,
	                     (long) tms[0].tm_mday,
	                     (long) tms[0].tm_mon,
	                     (long) tms[0].tm_year,
	                     (long) tms[0].tm_wday,
	                     (long) tms[0].tm_yday,
	                     (long) tms[0].tm_isdst));
	DUK_DDD(DUK_DDDPRINT("localtime result: tm={sec:%ld,min:%ld,hour:%ld,mday:%ld,mon:%ld,year:%ld,"
	                     "wday:%ld,yday:%ld,isdst:%ld}",
	                     (long) tms[1].tm_sec,
	                     (long) tms[1].tm_min,
	                     (long) tms[1].tm_hour,
	                     (long) tms[1].tm_mday,
	                     (long) tms[1].tm_mon,
	                     (long) tms[1].tm_year,
	                     (long) tms[1].tm_wday,
	                     (long) tms[1].tm_yday,
	                     (long) tms[1].tm_isdst));

	/* tm_isdst is both an input and an output to mktime(), use 0 to
	 * avoid DST handling in mktime():
	 * - https://github.com/svaarala/duktape/issues/406
	 * - http://stackoverflow.com/questions/8558919/mktime-and-tm-isdst
	 */
	tms[0].tm_isdst = 0;
	tms[1].tm_isdst = 0;
	t1 = mktime(&tms[0]); /* UTC */
	t2 = mktime(&tms[1]); /* local */
	if (t1 == (time_t) -1 || t2 == (time_t) -1) {
		/* This check used to be for (t < 0) but on some platforms
		 * time_t is unsigned and apparently the proper way to detect
		 * an mktime() error return is the cast above.  See e.g.:
		 * http://pubs.opengroup.org/onlinepubs/009695299/functions/mktime.html
		 */
		goto mktime_error;
	}
	DUK_DDD(DUK_DDDPRINT("t1=%ld (utc), t2=%ld (local)", (long) t1, (long) t2));

	/* Compute final offset in seconds, positive if local time ahead of
	 * UTC (returned value is UTC-to-local offset).
	 *
	 * difftime() returns a double, so coercion to int generates quite
	 * a lot of code.  Direct subtraction is not portable, however.
	 * XXX: allow direct subtraction on known platforms.
	 */
#if 0
	return (duk_int_t) (t2 - t1);
#endif
	return (duk_int_t) difftime(t2, t1);

mktime_error:
	/* XXX: return something more useful, so that caller can throw? */
	DUK_D(DUK_DPRINT("mktime() failed, d=%lf", (double) d));
	return 0;
}
#endif /* DUK_USE_DATE_TZO_GMTIME */

#if defined(DUK_USE_DATE_PRS_STRPTIME)
DUK_INTERNAL duk_bool_t duk_bi_date_parse_string_strptime(duk_hthread *thr, const char *str) {
	struct tm tm;
	time_t t;
	char buf[DUK__STRPTIME_BUF_SIZE];

	/* Copy to buffer with slack to avoid Valgrind gripes from strptime. */
	DUK_ASSERT(str != NULL);
	duk_memzero(buf, sizeof(buf)); /* valgrind whine without this */
	DUK_SNPRINTF(buf, sizeof(buf), "%s", (const char *) str);
	buf[sizeof(buf) - 1] = (char) 0;

	DUK_DDD(DUK_DDDPRINT("parsing: '%s'", (const char *) buf));

	duk_memzero(&tm, sizeof(tm));
	if (strptime((const char *) buf, "%c", &tm) != NULL) {
		DUK_DDD(DUK_DDDPRINT("before mktime: tm={sec:%ld,min:%ld,hour:%ld,mday:%ld,mon:%ld,year:%ld,"
		                     "wday:%ld,yday:%ld,isdst:%ld}",
		                     (long) tm.tm_sec,
		                     (long) tm.tm_min,
		                     (long) tm.tm_hour,
		                     (long) tm.tm_mday,
		                     (long) tm.tm_mon,
		                     (long) tm.tm_year,
		                     (long) tm.tm_wday,
		                     (long) tm.tm_yday,
		                     (long) tm.tm_isdst));
		tm.tm_isdst = -1; /* negative: dst info not available */

		t = mktime(&tm);
		DUK_DDD(DUK_DDDPRINT("mktime() -> %ld", (long) t));
		if (t >= 0) {
			duk_push_number(thr, ((duk_double_t) t) * 1000.0);
			return 1;
		}
	}

	return 0;
}
#endif /* DUK_USE_DATE_PRS_STRPTIME */

#if defined(DUK_USE_DATE_PRS_GETDATE)
DUK_INTERNAL duk_bool_t duk_bi_date_parse_string_getdate(duk_hthread *thr, const char *str) {
	struct tm tm;
	duk_small_int_t rc;
	time_t t;

	/* For this to work, DATEMSK must be set, so this is not very
	 * convenient for an embeddable interpreter.
	 */

	duk_memzero(&tm, sizeof(struct tm));
	rc = (duk_small_int_t) getdate_r(str, &tm);
	DUK_DDD(DUK_DDDPRINT("getdate_r() -> %ld", (long) rc));

	if (rc == 0) {
		t = mktime(&tm);
		DUK_DDD(DUK_DDDPRINT("mktime() -> %ld", (long) t));
		if (t >= 0) {
			duk_push_number(thr, (duk_double_t) t);
			return 1;
		}
	}

	return 0;
}
#endif /* DUK_USE_DATE_PRS_GETDATE */

#if defined(DUK_USE_DATE_FMT_STRFTIME)
DUK_INTERNAL duk_bool_t duk_bi_date_format_parts_strftime(duk_hthread *thr,
                                                          duk_int_t *parts,
                                                          duk_int_t tzoffset,
                                                          duk_small_uint_t flags) {
	char buf[DUK__STRFTIME_BUF_SIZE];
	struct tm tm;
	const char *fmt;

	DUK_UNREF(tzoffset);

	/* If the platform doesn't support the entire ECMAScript range, we need
	 * to return 0 so that the caller can fall back to the default formatter.
	 *
	 * For now, assume that if time_t is 8 bytes or more, the whole ECMAScript
	 * range is supported.  For smaller time_t values (4 bytes in practice),
	 * assumes that the signed 32-bit range is supported.
	 *
	 * XXX: detect this more correctly per platform.  The size of time_t is
	 * probably not an accurate guarantee of strftime() supporting or not
	 * supporting a large time range (the full ECMAScript range).
	 */
	if (sizeof(time_t) < 8 && (parts[DUK_DATE_IDX_YEAR] < 1970 || parts[DUK_DATE_IDX_YEAR] > 2037)) {
		/* be paranoid for 32-bit time values (even avoiding negative ones) */
		return 0;
	}

	duk_memzero(&tm, sizeof(tm));
	tm.tm_sec = parts[DUK_DATE_IDX_SECOND];
	tm.tm_min = parts[DUK_DATE_IDX_MINUTE];
	tm.tm_hour = parts[DUK_DATE_IDX_HOUR];
	tm.tm_mday = parts[DUK_DATE_IDX_DAY]; /* already one-based */
	tm.tm_mon = parts[DUK_DATE_IDX_MONTH] - 1; /* one-based -> zero-based */
	tm.tm_year = parts[DUK_DATE_IDX_YEAR] - 1900;
	tm.tm_wday = parts[DUK_DATE_IDX_WEEKDAY];
	tm.tm_isdst = 0;

	duk_memzero(buf, sizeof(buf));
	if ((flags & DUK_DATE_FLAG_TOSTRING_DATE) && (flags & DUK_DATE_FLAG_TOSTRING_TIME)) {
		fmt = "%c";
	} else if (flags & DUK_DATE_FLAG_TOSTRING_DATE) {
		fmt = "%x";
	} else {
		DUK_ASSERT(flags & DUK_DATE_FLAG_TOSTRING_TIME);
		fmt = "%X";
	}
	(void) strftime(buf, sizeof(buf) - 1, fmt, &tm);
	DUK_ASSERT(buf[sizeof(buf) - 1] == 0);

	duk_push_string(thr, buf);
	return 1;
}
#endif /* DUK_USE_DATE_FMT_STRFTIME */

#if defined(DUK_USE_GET_MONOTONIC_TIME_CLOCK_GETTIME)
DUK_INTERNAL duk_double_t duk_bi_date_get_monotonic_time_clock_gettime(void) {
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		return (duk_double_t) ts.tv_sec * 1000.0 + (duk_double_t) ts.tv_nsec / 1000000.0;
	} else {
		DUK_D(DUK_DPRINT("clock_gettime(CLOCK_MONOTONIC) failed"));
		return 0.0;
	}
}
#endif
