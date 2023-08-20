/*
 *  Date built-ins
 *
 *  Unlike most built-ins, Date has some platform dependencies for getting
 *  UTC time, converting between UTC and local time, and parsing and
 *  formatting time values.  These are all abstracted behind DUK_USE_xxx
 *  config options.  There are built-in platform specific providers for
 *  POSIX and Windows, but external providers can also be used.
 *
 *  See doc/datetime.rst.
 *
 */

#include "duk_internal.h"

/* XXX: currently defines unnecessary symbols when DUK_USE_DATE_BUILTIN is disabled. */

/*
 *  Forward declarations
 */

DUK_LOCAL_DECL duk_double_t duk__push_this_get_timeval_tzoffset(duk_hthread *thr, duk_small_uint_t flags, duk_int_t *out_tzoffset);
DUK_LOCAL_DECL duk_double_t duk__push_this_get_timeval(duk_hthread *thr, duk_small_uint_t flags);
DUK_LOCAL_DECL void duk__twodigit_year_fixup(duk_hthread *thr, duk_idx_t idx_val);
DUK_LOCAL_DECL duk_ret_t duk__set_this_timeval_from_dparts(duk_hthread *thr, duk_double_t *dparts, duk_small_uint_t flags);

/*
 *  Other file level defines
 */

/* Debug macro to print all parts and dparts (used manually because of debug level). */
#define DUK__DPRINT_PARTS_AND_DPARTS(parts, dparts) \
	do { \
		DUK_D(DUK_DPRINT("parts: %ld %ld %ld %ld %ld %ld %ld %ld, dparts: %lf %lf %lf %lf %lf %lf %lf %lf", \
		                 (long) (parts)[0], \
		                 (long) (parts)[1], \
		                 (long) (parts)[2], \
		                 (long) (parts)[3], \
		                 (long) (parts)[4], \
		                 (long) (parts)[5], \
		                 (long) (parts)[6], \
		                 (long) (parts)[7], \
		                 (double) (dparts)[0], \
		                 (double) (dparts)[1], \
		                 (double) (dparts)[2], \
		                 (double) (dparts)[3], \
		                 (double) (dparts)[4], \
		                 (double) (dparts)[5], \
		                 (double) (dparts)[6], \
		                 (double) (dparts)[7])); \
	} while (0)
#define DUK__DPRINT_PARTS(parts) \
	do { \
		DUK_D(DUK_DPRINT("parts: %ld %ld %ld %ld %ld %ld %ld %ld", \
		                 (long) (parts)[0], \
		                 (long) (parts)[1], \
		                 (long) (parts)[2], \
		                 (long) (parts)[3], \
		                 (long) (parts)[4], \
		                 (long) (parts)[5], \
		                 (long) (parts)[6], \
		                 (long) (parts)[7])); \
	} while (0)
#define DUK__DPRINT_DPARTS(dparts) \
	do { \
		DUK_D(DUK_DPRINT("dparts: %lf %lf %lf %lf %lf %lf %lf %lf", \
		                 (double) (dparts)[0], \
		                 (double) (dparts)[1], \
		                 (double) (dparts)[2], \
		                 (double) (dparts)[3], \
		                 (double) (dparts)[4], \
		                 (double) (dparts)[5], \
		                 (double) (dparts)[6], \
		                 (double) (dparts)[7])); \
	} while (0)

/* Equivalent year for DST calculations outside [1970,2038[ range, see
 * E5 Section 15.9.1.8.  Equivalent year has the same leap-year-ness and
 * starts with the same weekday on Jan 1.
 * https://bugzilla.mozilla.org/show_bug.cgi?id=351066
 */
#define DUK__YEAR(x) ((duk_uint8_t) ((x) -1970))
DUK_LOCAL duk_uint8_t duk__date_equivyear[14] = {
#if 1
	/* This is based on V8 EquivalentYear() algorithm (see util/genequivyear.py):
	 * http://code.google.com/p/v8/source/browse/trunk/src/date.h#146
	 */

	/* non-leap year: sunday, monday, ... */
	DUK__YEAR(2023),
	DUK__YEAR(2035),
	DUK__YEAR(2019),
	DUK__YEAR(2031),
	DUK__YEAR(2015),
	DUK__YEAR(2027),
	DUK__YEAR(2011),

	/* leap year: sunday, monday, ... */
	DUK__YEAR(2012),
	DUK__YEAR(2024),
	DUK__YEAR(2008),
	DUK__YEAR(2020),
	DUK__YEAR(2032),
	DUK__YEAR(2016),
	DUK__YEAR(2028)
#endif

#if 0
	/* This is based on Rhino EquivalentYear() algorithm:
	 * https://github.com/mozilla/rhino/blob/f99cc11d616f0cdda2c42bde72b3484df6182947/src/org/mozilla/javascript/NativeDate.java
	 */

	/* non-leap year: sunday, monday, ... */
	DUK__YEAR(1978), DUK__YEAR(1973), DUK__YEAR(1985), DUK__YEAR(1986),
	DUK__YEAR(1981), DUK__YEAR(1971), DUK__YEAR(1977),

	/* leap year: sunday, monday, ... */
	DUK__YEAR(1984), DUK__YEAR(1996), DUK__YEAR(1980), DUK__YEAR(1992),
	DUK__YEAR(1976), DUK__YEAR(1988), DUK__YEAR(1972)
#endif
};

/*
 *  ISO 8601 subset parser.
 */

/* Parser part count. */
#define DUK__NUM_ISO8601_PARSER_PARTS 9

/* Parser part indices. */
#define DUK__PI_YEAR        0
#define DUK__PI_MONTH       1
#define DUK__PI_DAY         2
#define DUK__PI_HOUR        3
#define DUK__PI_MINUTE      4
#define DUK__PI_SECOND      5
#define DUK__PI_MILLISECOND 6
#define DUK__PI_TZHOUR      7
#define DUK__PI_TZMINUTE    8

/* Parser part masks. */
#define DUK__PM_YEAR        (1 << DUK__PI_YEAR)
#define DUK__PM_MONTH       (1 << DUK__PI_MONTH)
#define DUK__PM_DAY         (1 << DUK__PI_DAY)
#define DUK__PM_HOUR        (1 << DUK__PI_HOUR)
#define DUK__PM_MINUTE      (1 << DUK__PI_MINUTE)
#define DUK__PM_SECOND      (1 << DUK__PI_SECOND)
#define DUK__PM_MILLISECOND (1 << DUK__PI_MILLISECOND)
#define DUK__PM_TZHOUR      (1 << DUK__PI_TZHOUR)
#define DUK__PM_TZMINUTE    (1 << DUK__PI_TZMINUTE)

/* Parser separator indices. */
#define DUK__SI_PLUS   0
#define DUK__SI_MINUS  1
#define DUK__SI_T      2
#define DUK__SI_SPACE  3
#define DUK__SI_COLON  4
#define DUK__SI_PERIOD 5
#define DUK__SI_Z      6
#define DUK__SI_NUL    7

/* Parser separator masks. */
#define DUK__SM_PLUS   (1 << DUK__SI_PLUS)
#define DUK__SM_MINUS  (1 << DUK__SI_MINUS)
#define DUK__SM_T      (1 << DUK__SI_T)
#define DUK__SM_SPACE  (1 << DUK__SI_SPACE)
#define DUK__SM_COLON  (1 << DUK__SI_COLON)
#define DUK__SM_PERIOD (1 << DUK__SI_PERIOD)
#define DUK__SM_Z      (1 << DUK__SI_Z)
#define DUK__SM_NUL    (1 << DUK__SI_NUL)

/* Rule control flags. */
#define DUK__CF_NEG        (1 << 0) /* continue matching, set neg_tzoffset flag */
#define DUK__CF_ACCEPT     (1 << 1) /* accept string */
#define DUK__CF_ACCEPT_NUL (1 << 2) /* accept string if next char is NUL (otherwise reject) */

#define DUK__PACK_RULE(partmask, sepmask, nextpart, flags) \
	((duk_uint32_t) (partmask) + (((duk_uint32_t) (sepmask)) << 9) + (((duk_uint32_t) (nextpart)) << 17) + \
	 (((duk_uint32_t) (flags)) << 21))

#define DUK__UNPACK_RULE(rule, var_nextidx, var_flags) \
	do { \
		(var_nextidx) = (duk_small_uint_t) (((rule) >> 17) & 0x0f); \
		(var_flags) = (duk_small_uint_t) ((rule) >> 21); \
	} while (0)

#define DUK__RULE_MASK_PART_SEP 0x1ffffUL

/* Matching separator index is used in the control table */
DUK_LOCAL const duk_uint8_t duk__parse_iso8601_seps[] = {
	DUK_ASC_PLUS /*0*/,  DUK_ASC_MINUS /*1*/,  DUK_ASC_UC_T /*2*/, DUK_ASC_SPACE /*3*/,
	DUK_ASC_COLON /*4*/, DUK_ASC_PERIOD /*5*/, DUK_ASC_UC_Z /*6*/, DUK_ASC_NUL /*7*/
};

/* Rule table: first matching rule is used to determine what to do next. */
DUK_LOCAL const duk_uint32_t duk__parse_iso8601_control[] = {
	DUK__PACK_RULE(DUK__PM_YEAR, DUK__SM_MINUS, DUK__PI_MONTH, 0),
	DUK__PACK_RULE(DUK__PM_MONTH, DUK__SM_MINUS, DUK__PI_DAY, 0),
	DUK__PACK_RULE(DUK__PM_YEAR | DUK__PM_MONTH | DUK__PM_DAY, DUK__SM_T | DUK__SM_SPACE, DUK__PI_HOUR, 0),
	DUK__PACK_RULE(DUK__PM_HOUR, DUK__SM_COLON, DUK__PI_MINUTE, 0),
	DUK__PACK_RULE(DUK__PM_MINUTE, DUK__SM_COLON, DUK__PI_SECOND, 0),
	DUK__PACK_RULE(DUK__PM_SECOND, DUK__SM_PERIOD, DUK__PI_MILLISECOND, 0),
	DUK__PACK_RULE(DUK__PM_TZHOUR, DUK__SM_COLON, DUK__PI_TZMINUTE, 0),
	DUK__PACK_RULE(DUK__PM_YEAR | DUK__PM_MONTH | DUK__PM_DAY | DUK__PM_HOUR /*Note1*/ | DUK__PM_MINUTE | DUK__PM_SECOND |
	                   DUK__PM_MILLISECOND,
	               DUK__SM_PLUS,
	               DUK__PI_TZHOUR,
	               0),
	DUK__PACK_RULE(DUK__PM_YEAR | DUK__PM_MONTH | DUK__PM_DAY | DUK__PM_HOUR /*Note1*/ | DUK__PM_MINUTE | DUK__PM_SECOND |
	                   DUK__PM_MILLISECOND,
	               DUK__SM_MINUS,
	               DUK__PI_TZHOUR,
	               DUK__CF_NEG),
	DUK__PACK_RULE(DUK__PM_YEAR | DUK__PM_MONTH | DUK__PM_DAY | DUK__PM_HOUR /*Note1*/ | DUK__PM_MINUTE | DUK__PM_SECOND |
	                   DUK__PM_MILLISECOND,
	               DUK__SM_Z,
	               0,
	               DUK__CF_ACCEPT_NUL),
	DUK__PACK_RULE(DUK__PM_YEAR | DUK__PM_MONTH | DUK__PM_DAY | DUK__PM_HOUR /*Note1*/ | DUK__PM_MINUTE | DUK__PM_SECOND |
	                   DUK__PM_MILLISECOND | DUK__PM_TZHOUR /*Note2*/ | DUK__PM_TZMINUTE,
	               DUK__SM_NUL,
	               0,
	               DUK__CF_ACCEPT)

	/* Note1: the specification doesn't require matching a time form with
	 *        just hours ("HH"), but we accept it here, e.g. "2012-01-02T12Z".
	 *
	 * Note2: the specification doesn't require matching a timezone offset
	 *        with just hours ("HH"), but accept it here, e.g. "2012-01-02T03:04:05+02"
	 */
};

DUK_LOCAL duk_bool_t duk__parse_string_iso8601_subset(duk_hthread *thr, const char *str) {
	duk_int_t parts[DUK__NUM_ISO8601_PARSER_PARTS];
	duk_double_t dparts[DUK_DATE_IDX_NUM_PARTS];
	duk_double_t d;
	const duk_uint8_t *p;
	duk_small_uint_t part_idx = 0;
	duk_int_t accum = 0;
	duk_small_uint_t ndigits = 0;
	duk_bool_t neg_year = 0;
	duk_bool_t neg_tzoffset = 0;
	duk_uint_fast8_t ch;
	duk_small_uint_t i;

	/* During parsing, month and day are one-based; set defaults here. */
	duk_memzero(parts, sizeof(parts));
	DUK_ASSERT(parts[DUK_DATE_IDX_YEAR] == 0); /* don't care value, year is mandatory */
	parts[DUK_DATE_IDX_MONTH] = 1;
	parts[DUK_DATE_IDX_DAY] = 1;

	/* Special handling for year sign. */
	p = (const duk_uint8_t *) str;
	ch = p[0];
	if (ch == DUK_ASC_PLUS) {
		p++;
	} else if (ch == DUK_ASC_MINUS) {
		neg_year = 1;
		p++;
	}

	for (;;) {
		ch = *p++;
		DUK_DDD(DUK_DDDPRINT("parsing, part_idx=%ld, char=%ld ('%c')",
		                     (long) part_idx,
		                     (long) ch,
		                     (int) ((ch >= 0x20 && ch <= 0x7e) ? ch : DUK_ASC_QUESTION)));

		if (ch >= DUK_ASC_0 && ch <= DUK_ASC_9) {
			if (ndigits >= 9) {
				DUK_DDD(DUK_DDDPRINT("too many digits -> reject"));
				goto reject;
			}
			if (part_idx == DUK__PI_MILLISECOND && ndigits >= 3) {
				/* ignore millisecond fractions after 3 */
			} else {
				accum = accum * 10 + ((duk_int_t) ch) - ((duk_int_t) DUK_ASC_0) + 0x00;
				ndigits++;
			}
		} else {
			duk_uint_fast32_t match_val;
			duk_small_uint_t sep_idx;

			if (ndigits <= 0) {
				goto reject;
			}
			if (part_idx == DUK__PI_MILLISECOND) {
				/* complete the millisecond field */
				while (ndigits < 3) {
					accum *= 10;
					ndigits++;
				}
			}
			parts[part_idx] = accum;
			DUK_DDD(DUK_DDDPRINT("wrote part %ld -> value %ld", (long) part_idx, (long) accum));

			accum = 0;
			ndigits = 0;

			for (i = 0; i < (duk_small_uint_t) (sizeof(duk__parse_iso8601_seps) / sizeof(duk_uint8_t)); i++) {
				if (duk__parse_iso8601_seps[i] == ch) {
					break;
				}
			}
			if (i == (duk_small_uint_t) (sizeof(duk__parse_iso8601_seps) / sizeof(duk_uint8_t))) {
				DUK_DDD(DUK_DDDPRINT("separator character doesn't match -> reject"));
				goto reject;
			}

			sep_idx = i;
			match_val = (1UL << part_idx) + (1UL << (sep_idx + 9)); /* match against rule part/sep bits */

			for (i = 0; i < (duk_small_uint_t) (sizeof(duk__parse_iso8601_control) / sizeof(duk_uint32_t)); i++) {
				duk_uint_fast32_t rule = duk__parse_iso8601_control[i];
				duk_small_uint_t nextpart;
				duk_small_uint_t cflags;

				DUK_DDD(DUK_DDDPRINT("part_idx=%ld, sep_idx=%ld, match_val=0x%08lx, considering rule=0x%08lx",
				                     (long) part_idx,
				                     (long) sep_idx,
				                     (unsigned long) match_val,
				                     (unsigned long) rule));

				if ((rule & match_val) != match_val) {
					continue;
				}

				DUK__UNPACK_RULE(rule, nextpart, cflags);

				DUK_DDD(DUK_DDDPRINT("rule match -> part_idx=%ld, sep_idx=%ld, match_val=0x%08lx, "
				                     "rule=0x%08lx -> nextpart=%ld, cflags=0x%02lx",
				                     (long) part_idx,
				                     (long) sep_idx,
				                     (unsigned long) match_val,
				                     (unsigned long) rule,
				                     (long) nextpart,
				                     (unsigned long) cflags));

				if (cflags & DUK__CF_NEG) {
					neg_tzoffset = 1;
				}

				if (cflags & DUK__CF_ACCEPT) {
					goto accept;
				}

				if (cflags & DUK__CF_ACCEPT_NUL) {
					DUK_ASSERT(*(p - 1) != (char) 0);
					if (*p == DUK_ASC_NUL) {
						goto accept;
					}
					goto reject;
				}

				part_idx = nextpart;
				break;
			} /* rule match */

			if (i == (duk_small_uint_t) (sizeof(duk__parse_iso8601_control) / sizeof(duk_uint32_t))) {
				DUK_DDD(DUK_DDDPRINT("no rule matches -> reject"));
				goto reject;
			}

			if (ch == 0) {
				/* This shouldn't be necessary, but check just in case
				 * to avoid any chance of overruns.
				 */
				DUK_DDD(DUK_DDDPRINT("NUL after rule matching (should not happen) -> reject"));
				goto reject;
			}
		} /* if-digit-else-ctrl */
	} /* char loop */

	/* We should never exit the loop above. */
	DUK_UNREACHABLE();

reject:
	DUK_DDD(DUK_DDDPRINT("reject"));
	return 0;

accept:
	DUK_DDD(DUK_DDDPRINT("accept"));

	/* Apply timezone offset to get the main parts in UTC */
	if (neg_year) {
		parts[DUK__PI_YEAR] = -parts[DUK__PI_YEAR];
	}
	if (neg_tzoffset) {
		parts[DUK__PI_HOUR] += parts[DUK__PI_TZHOUR];
		parts[DUK__PI_MINUTE] += parts[DUK__PI_TZMINUTE];
	} else {
		parts[DUK__PI_HOUR] -= parts[DUK__PI_TZHOUR];
		parts[DUK__PI_MINUTE] -= parts[DUK__PI_TZMINUTE];
	}
	parts[DUK__PI_MONTH] -= 1; /* zero-based month */
	parts[DUK__PI_DAY] -= 1; /* zero-based day */

	/* Use double parts, they tolerate unnormalized time.
	 *
	 * Note: DUK_DATE_IDX_WEEKDAY is initialized with a bogus value (DUK__PI_TZHOUR)
	 * on purpose.  It won't be actually used by duk_bi_date_get_timeval_from_dparts(),
	 * but will make the value initialized just in case, and avoid any
	 * potential for Valgrind issues.
	 */
	for (i = 0; i < DUK_DATE_IDX_NUM_PARTS; i++) {
		DUK_DDD(DUK_DDDPRINT("part[%ld] = %ld", (long) i, (long) parts[i]));
		dparts[i] = parts[i];
	}

	d = duk_bi_date_get_timeval_from_dparts(dparts, 0 /*flags*/);
	duk_push_number(thr, d);
	return 1;
}

/*
 *  Date/time parsing helper.
 *
 *  Parse a datetime string into a time value.  We must first try to parse
 *  the input according to the standard format in E5.1 Section 15.9.1.15.
 *  If that fails, we can try to parse using custom parsing, which can
 *  either be platform neutral (custom code) or platform specific (using
 *  existing platform API calls).
 *
 *  Note in particular that we must parse whatever toString(), toUTCString(),
 *  and toISOString() can produce; see E5.1 Section 15.9.4.2.
 *
 *  Returns 1 to allow tail calling.
 *
 *  There is much room for improvement here with respect to supporting
 *  alternative datetime formats.  For instance, V8 parses '2012-01-01' as
 *  UTC and '2012/01/01' as local time.
 */

DUK_LOCAL duk_ret_t duk__parse_string(duk_hthread *thr, const char *str) {
	/* XXX: there is a small risk here: because the ISO 8601 parser is
	 * very loose, it may end up parsing some datetime values which
	 * would be better parsed with a platform specific parser.
	 */

	DUK_ASSERT(str != NULL);
	DUK_DDD(DUK_DDDPRINT("parse datetime from string '%s'", (const char *) str));

	if (duk__parse_string_iso8601_subset(thr, str) != 0) {
		return 1;
	}

#if defined(DUK_USE_DATE_PARSE_STRING)
	/* Contract, either:
	 * - Push value on stack and return 1
	 * - Don't push anything on stack and return 0
	 */

	if (DUK_USE_DATE_PARSE_STRING(thr, str) != 0) {
		return 1;
	}
#else
	/* No platform-specific parsing, this is not an error. */
#endif

	duk_push_nan(thr);
	return 1;
}

/*
 *  Calendar helpers
 *
 *  Some helpers are used for getters and can operate on normalized values
 *  which can be represented with 32-bit signed integers.  Other helpers are
 *  needed by setters and operate on un-normalized double values, must watch
 *  out for non-finite numbers etc.
 */

DUK_LOCAL duk_uint8_t duk__days_in_month[12] = { (duk_uint8_t) 31, (duk_uint8_t) 28, (duk_uint8_t) 31, (duk_uint8_t) 30,
	                                         (duk_uint8_t) 31, (duk_uint8_t) 30, (duk_uint8_t) 31, (duk_uint8_t) 31,
	                                         (duk_uint8_t) 30, (duk_uint8_t) 31, (duk_uint8_t) 30, (duk_uint8_t) 31 };

/* Maximum iteration count for computing UTC-to-local time offset when
 * creating an ECMAScript time value from local parts.
 */
#define DUK__LOCAL_TZOFFSET_MAXITER 4

/* Because 'day since epoch' can be negative and is used to compute weekday
 * using a modulo operation, add this multiple of 7 to avoid negative values
 * when year is below 1970 epoch.  ECMAScript time values are restricted to
 * +/- 100 million days from epoch, so this adder fits nicely into 32 bits.
 * Round to a multiple of 7 (= floor(100000000 / 7) * 7) and add margin.
 */
#define DUK__WEEKDAY_MOD_ADDER (20000000 * 7) /* 0x08583b00 */

DUK_INTERNAL duk_bool_t duk_bi_date_is_leap_year(duk_int_t year) {
	if ((year % 4) != 0) {
		return 0;
	}
	if ((year % 100) != 0) {
		return 1;
	}
	if ((year % 400) != 0) {
		return 0;
	}
	return 1;
}

DUK_INTERNAL duk_bool_t duk_bi_date_timeval_in_valid_range(duk_double_t x) {
	return (x >= -DUK_DATE_MSEC_100M_DAYS && x <= DUK_DATE_MSEC_100M_DAYS);
}

DUK_INTERNAL duk_bool_t duk_bi_date_timeval_in_leeway_range(duk_double_t x) {
	return (x >= -DUK_DATE_MSEC_100M_DAYS_LEEWAY && x <= DUK_DATE_MSEC_100M_DAYS_LEEWAY);
}

DUK_INTERNAL duk_bool_t duk_bi_date_year_in_valid_range(duk_double_t x) {
	return (x >= DUK_DATE_MIN_ECMA_YEAR && x <= DUK_DATE_MAX_ECMA_YEAR);
}

DUK_LOCAL duk_double_t duk__timeclip(duk_double_t x) {
	if (!DUK_ISFINITE(x)) {
		return DUK_DOUBLE_NAN;
	}

	if (!duk_bi_date_timeval_in_valid_range(x)) {
		return DUK_DOUBLE_NAN;
	}

	x = duk_js_tointeger_number(x);

	/* Here we'd have the option to normalize -0 to +0. */
	return x;
}

/* Integer division which floors also negative values correctly. */
DUK_LOCAL duk_int_t duk__div_floor(duk_int_t a, duk_int_t b) {
	DUK_ASSERT(b > 0);
	if (a >= 0) {
		return a / b;
	} else {
		/* e.g. a = -4, b = 5  -->  -4 - 5 + 1 / 5  -->  -8 / 5  -->  -1
		 *      a = -5, b = 5  -->  -5 - 5 + 1 / 5  -->  -9 / 5  -->  -1
		 *      a = -6, b = 5  -->  -6 - 5 + 1 / 5  -->  -10 / 5  -->  -2
		 */
		return (a - b + 1) / b;
	}
}

/* Compute day number of the first day of a given year. */
DUK_LOCAL duk_int_t duk__day_from_year(duk_int_t year) {
	/* Note: in integer arithmetic, (x / 4) is same as floor(x / 4) for non-negative
	 * values, but is incorrect for negative ones.
	 */
	return 365 * (year - 1970) + duk__div_floor(year - 1969, 4) - duk__div_floor(year - 1901, 100) +
	       duk__div_floor(year - 1601, 400);
}

/* Given a day number, determine year and day-within-year. */
DUK_LOCAL duk_int_t duk__year_from_day(duk_int_t day, duk_small_int_t *out_day_within_year) {
	duk_int_t year;
	duk_int_t diff_days;

	/* estimate year upwards (towards positive infinity), then back down;
	 * two iterations should be enough
	 */

	if (day >= 0) {
		year = 1970 + day / 365;
	} else {
		year = 1970 + day / 366;
	}

	for (;;) {
		diff_days = duk__day_from_year(year) - day;
		DUK_DDD(DUK_DDDPRINT("year=%ld day=%ld, diff_days=%ld", (long) year, (long) day, (long) diff_days));
		if (diff_days <= 0) {
			DUK_ASSERT(-diff_days < 366); /* fits into duk_small_int_t */
			*out_day_within_year = -diff_days;
			DUK_DDD(DUK_DDDPRINT("--> year=%ld, day-within-year=%ld", (long) year, (long) *out_day_within_year));
			DUK_ASSERT(*out_day_within_year >= 0);
			DUK_ASSERT(*out_day_within_year < (duk_bi_date_is_leap_year(year) ? 366 : 365));
			return year;
		}

		/* Note: this is very tricky; we must never 'overshoot' the
		 * correction downwards.
		 */
		year -= 1 + (diff_days - 1) / 366; /* conservative */
	}
}

/* Given a (year, month, day-within-month) triple, compute day number.
 * The input triple is un-normalized and may contain non-finite values.
 */
DUK_LOCAL duk_double_t duk__make_day(duk_double_t year, duk_double_t month, duk_double_t day) {
	duk_int_t day_num;
	duk_bool_t is_leap;
	duk_small_int_t i, n;

	/* Assume that year, month, day are all coerced to whole numbers.
	 * They may also be NaN or infinity, in which case this function
	 * must return NaN or infinity to ensure time value becomes NaN.
	 * If 'day' is NaN, the final return will end up returning a NaN,
	 * so it doesn't need to be checked here.
	 */

	if (!DUK_ISFINITE(year) || !DUK_ISFINITE(month)) {
		return DUK_DOUBLE_NAN;
	}

	year += DUK_FLOOR(month / 12.0);

	month = DUK_FMOD(month, 12.0);
	if (month < 0.0) {
		/* handle negative values */
		month += 12.0;
	}

	/* The algorithm in E5.1 Section 15.9.1.12 normalizes month, but
	 * does not normalize the day-of-month (nor check whether or not
	 * it is finite) because it's not necessary for finding the day
	 * number which matches the (year,month) pair.
	 *
	 * We assume that duk__day_from_year() is exact here.
	 *
	 * Without an explicit infinity / NaN check in the beginning,
	 * day_num would be a bogus integer here.
	 *
	 * It's possible for 'year' to be out of integer range here.
	 * If so, we need to return NaN without integer overflow.
	 * This fixes test-bug-setyear-overflow.js.
	 */

	if (!duk_bi_date_year_in_valid_range(year)) {
		DUK_DD(DUK_DDPRINT("year not in ecmascript valid range, avoid integer overflow: %lf", (double) year));
		return DUK_DOUBLE_NAN;
	}
	day_num = duk__day_from_year((duk_int_t) year);
	is_leap = duk_bi_date_is_leap_year((duk_int_t) year);

	n = (duk_small_int_t) month;
	for (i = 0; i < n; i++) {
		day_num += duk__days_in_month[i];
		if (i == 1 && is_leap) {
			day_num++;
		}
	}

	/* If 'day' is NaN, returns NaN. */
	return (duk_double_t) day_num + day;
}

/* Split time value into parts.  The time value may contain fractions (it may
 * come from duk_time_to_components() API call) which are truncated.  Possible
 * local time adjustment has already been applied when reading the time value.
 */
DUK_INTERNAL void duk_bi_date_timeval_to_parts(duk_double_t d, duk_int_t *parts, duk_double_t *dparts, duk_small_uint_t flags) {
	duk_double_t d1, d2;
	duk_int_t t1, t2;
	duk_int_t day_since_epoch;
	duk_int_t year; /* does not fit into 16 bits */
	duk_small_int_t day_in_year;
	duk_small_int_t month;
	duk_small_int_t day;
	duk_small_int_t dim;
	duk_int_t jan1_since_epoch;
	duk_small_int_t jan1_weekday;
	duk_int_t equiv_year;
	duk_small_uint_t i;
	duk_bool_t is_leap;
	duk_small_int_t arridx;

	DUK_ASSERT(DUK_ISFINITE(d)); /* caller checks */
	d = DUK_FLOOR(d); /* remove fractions if present */
	DUK_ASSERT(duk_double_equals(DUK_FLOOR(d), d));

	/* The timevalue must be in valid ECMAScript range, but since a local
	 * time offset can be applied, we need to allow a +/- 24h leeway to
	 * the value.  In other words, although the UTC time is within the
	 * ECMAScript range, the local part values can be just outside of it.
	 */
	DUK_UNREF(duk_bi_date_timeval_in_leeway_range);
	DUK_ASSERT(duk_bi_date_timeval_in_leeway_range(d));

	/* These computations are guaranteed to be exact for the valid
	 * E5 time value range, assuming milliseconds without fractions.
	 */
	d1 = (duk_double_t) DUK_FMOD(d, (double) DUK_DATE_MSEC_DAY);
	if (d1 < 0.0) {
		/* deal with negative values */
		d1 += (duk_double_t) DUK_DATE_MSEC_DAY;
	}
	d2 = DUK_FLOOR((double) (d / (duk_double_t) DUK_DATE_MSEC_DAY));
	DUK_ASSERT(duk_double_equals(d2 * ((duk_double_t) DUK_DATE_MSEC_DAY) + d1, d));
	/* now expected to fit into a 32-bit integer */
	t1 = (duk_int_t) d1;
	t2 = (duk_int_t) d2;
	day_since_epoch = t2;
	DUK_ASSERT(duk_double_equals((duk_double_t) t1, d1));
	DUK_ASSERT(duk_double_equals((duk_double_t) t2, d2));

	/* t1 = milliseconds within day (fits 32 bit)
	 * t2 = day number from epoch (fits 32 bit, may be negative)
	 */

	parts[DUK_DATE_IDX_MILLISECOND] = t1 % 1000;
	t1 /= 1000;
	parts[DUK_DATE_IDX_SECOND] = t1 % 60;
	t1 /= 60;
	parts[DUK_DATE_IDX_MINUTE] = t1 % 60;
	t1 /= 60;
	parts[DUK_DATE_IDX_HOUR] = t1;
	DUK_ASSERT(parts[DUK_DATE_IDX_MILLISECOND] >= 0 && parts[DUK_DATE_IDX_MILLISECOND] <= 999);
	DUK_ASSERT(parts[DUK_DATE_IDX_SECOND] >= 0 && parts[DUK_DATE_IDX_SECOND] <= 59);
	DUK_ASSERT(parts[DUK_DATE_IDX_MINUTE] >= 0 && parts[DUK_DATE_IDX_MINUTE] <= 59);
	DUK_ASSERT(parts[DUK_DATE_IDX_HOUR] >= 0 && parts[DUK_DATE_IDX_HOUR] <= 23);

	DUK_DDD(DUK_DDDPRINT("d=%lf, d1=%lf, d2=%lf, t1=%ld, t2=%ld, parts: hour=%ld min=%ld sec=%ld msec=%ld",
	                     (double) d,
	                     (double) d1,
	                     (double) d2,
	                     (long) t1,
	                     (long) t2,
	                     (long) parts[DUK_DATE_IDX_HOUR],
	                     (long) parts[DUK_DATE_IDX_MINUTE],
	                     (long) parts[DUK_DATE_IDX_SECOND],
	                     (long) parts[DUK_DATE_IDX_MILLISECOND]));

	/* This assert depends on the input parts representing time inside
	 * the ECMAScript range.
	 */
	DUK_ASSERT(t2 + DUK__WEEKDAY_MOD_ADDER >= 0);
	parts[DUK_DATE_IDX_WEEKDAY] = (t2 + 4 + DUK__WEEKDAY_MOD_ADDER) % 7; /* E5.1 Section 15.9.1.6 */
	DUK_ASSERT(parts[DUK_DATE_IDX_WEEKDAY] >= 0 && parts[DUK_DATE_IDX_WEEKDAY] <= 6);

	year = duk__year_from_day(t2, &day_in_year);
	day = day_in_year;
	is_leap = duk_bi_date_is_leap_year(year);
	for (month = 0; month < 12; month++) {
		dim = duk__days_in_month[month];
		if (month == 1 && is_leap) {
			dim++;
		}
		DUK_DDD(DUK_DDDPRINT("month=%ld, dim=%ld, day=%ld", (long) month, (long) dim, (long) day));
		if (day < dim) {
			break;
		}
		day -= dim;
	}
	DUK_DDD(DUK_DDDPRINT("final month=%ld", (long) month));
	DUK_ASSERT(month >= 0 && month <= 11);
	DUK_ASSERT(day >= 0 && day <= 31);

	/* Equivalent year mapping, used to avoid DST trouble when platform
	 * may fail to provide reasonable DST answers for dates outside the
	 * ordinary range (e.g. 1970-2038).  An equivalent year has the same
	 * leap-year-ness as the original year and begins on the same weekday
	 * (Jan 1).
	 *
	 * The year 2038 is avoided because there seem to be problems with it
	 * on some platforms.  The year 1970 is also avoided as there were
	 * practical problems with it; an equivalent year is used for it too,
	 * which breaks some DST computations for 1970 right now, see e.g.
	 * test-bi-date-tzoffset-brute-fi.js.
	 */
	if ((flags & DUK_DATE_FLAG_EQUIVYEAR) && (year < 1971 || year > 2037)) {
		DUK_ASSERT(is_leap == 0 || is_leap == 1);

		jan1_since_epoch = day_since_epoch - day_in_year; /* day number for Jan 1 since epoch */
		DUK_ASSERT(jan1_since_epoch + DUK__WEEKDAY_MOD_ADDER >= 0);
		jan1_weekday = (jan1_since_epoch + 4 + DUK__WEEKDAY_MOD_ADDER) % 7; /* E5.1 Section 15.9.1.6 */
		DUK_ASSERT(jan1_weekday >= 0 && jan1_weekday <= 6);
		arridx = jan1_weekday;
		if (is_leap) {
			arridx += 7;
		}
		DUK_ASSERT(arridx >= 0 && arridx < (duk_small_int_t) (sizeof(duk__date_equivyear) / sizeof(duk_uint8_t)));

		equiv_year = (duk_int_t) duk__date_equivyear[arridx] + 1970;
		year = equiv_year;
		DUK_DDD(DUK_DDDPRINT("equiv year mapping, year=%ld, day_in_year=%ld, day_since_epoch=%ld, "
		                     "jan1_since_epoch=%ld, jan1_weekday=%ld -> equiv year %ld",
		                     (long) year,
		                     (long) day_in_year,
		                     (long) day_since_epoch,
		                     (long) jan1_since_epoch,
		                     (long) jan1_weekday,
		                     (long) equiv_year));
	}

	parts[DUK_DATE_IDX_YEAR] = year;
	parts[DUK_DATE_IDX_MONTH] = month;
	parts[DUK_DATE_IDX_DAY] = day;

	if (flags & DUK_DATE_FLAG_ONEBASED) {
		parts[DUK_DATE_IDX_MONTH]++; /* zero-based -> one-based */
		parts[DUK_DATE_IDX_DAY]++; /* -""- */
	}

	if (dparts != NULL) {
		for (i = 0; i < DUK_DATE_IDX_NUM_PARTS; i++) {
			dparts[i] = (duk_double_t) parts[i];
		}
	}
}

/* Compute time value from (double) parts.  The parts can be either UTC
 * or local time; if local, they need to be (conceptually) converted into
 * UTC time.  The parts may represent valid or invalid time, and may be
 * wildly out of range (but may cancel each other and still come out in
 * the valid Date range).
 */
DUK_INTERNAL duk_double_t duk_bi_date_get_timeval_from_dparts(duk_double_t *dparts, duk_small_uint_t flags) {
#if defined(DUK_USE_PARANOID_DATE_COMPUTATION)
	/* See comments below on MakeTime why these are volatile. */
	volatile duk_double_t tmp_time;
	volatile duk_double_t tmp_day;
	volatile duk_double_t d;
#else
	duk_double_t tmp_time;
	duk_double_t tmp_day;
	duk_double_t d;
#endif
	duk_small_uint_t i;
	duk_int_t tzoff, tzoffprev1, tzoffprev2;

	/* Expects 'this' at top of stack on entry. */

	/* Coerce all finite parts with ToInteger().  ToInteger() must not
	 * be called for NaN/Infinity because it will convert e.g. NaN to
	 * zero.  If ToInteger() has already been called, this has no side
	 * effects and is idempotent.
	 *
	 * Don't read dparts[DUK_DATE_IDX_WEEKDAY]; it will cause Valgrind
	 * issues if the value is uninitialized.
	 */
	for (i = 0; i <= DUK_DATE_IDX_MILLISECOND; i++) {
		/* SCANBUILD: scan-build complains here about assigned value
		 * being garbage or undefined.  This is correct but operating
		 * on undefined values has no ill effect and is ignored by the
		 * caller in the case where this happens.
		 */
		d = dparts[i];
		if (DUK_ISFINITE(d)) {
			dparts[i] = duk_js_tointeger_number(d);
		}
	}

	/* Use explicit steps in computation to try to ensure that
	 * computation happens with intermediate results coerced to
	 * double values (instead of using something more accurate).
	 * E.g. E5.1 Section 15.9.1.11 requires use of IEEE 754
	 * rules (= ECMAScript '+' and '*' operators).
	 *
	 * Without 'volatile' even this approach fails on some platform
	 * and compiler combinations.  For instance, gcc 4.8.1 on Ubuntu
	 * 64-bit, with -m32 and without -std=c99, test-bi-date-canceling.js
	 * would fail because of some optimizations when computing tmp_time
	 * (MakeTime below).  Adding 'volatile' to tmp_time solved this
	 * particular problem (annoyingly, also adding debug prints or
	 * running the executable under valgrind hides it).
	 */

	/* MakeTime */
	tmp_time = 0.0;
	tmp_time += dparts[DUK_DATE_IDX_HOUR] * ((duk_double_t) DUK_DATE_MSEC_HOUR);
	tmp_time += dparts[DUK_DATE_IDX_MINUTE] * ((duk_double_t) DUK_DATE_MSEC_MINUTE);
	tmp_time += dparts[DUK_DATE_IDX_SECOND] * ((duk_double_t) DUK_DATE_MSEC_SECOND);
	tmp_time += dparts[DUK_DATE_IDX_MILLISECOND];

	/* MakeDay */
	tmp_day = duk__make_day(dparts[DUK_DATE_IDX_YEAR], dparts[DUK_DATE_IDX_MONTH], dparts[DUK_DATE_IDX_DAY]);

	/* MakeDate */
	d = tmp_day * ((duk_double_t) DUK_DATE_MSEC_DAY) + tmp_time;

	DUK_DDD(DUK_DDDPRINT("time=%lf day=%lf --> timeval=%lf", (double) tmp_time, (double) tmp_day, (double) d));

	/* Optional UTC conversion. */
	if (flags & DUK_DATE_FLAG_LOCALTIME) {
		/* DUK_USE_DATE_GET_LOCAL_TZOFFSET() needs to be called with a
		 * time value computed from UTC parts.  At this point we only
		 * have 'd' which is a time value computed from local parts, so
		 * it is off by the UTC-to-local time offset which we don't know
		 * yet.  The current solution for computing the UTC-to-local
		 * time offset is to iterate a few times and detect a fixed
		 * point or a two-cycle loop (or a sanity iteration limit),
		 * see test-bi-date-local-parts.js and test-bi-date-tzoffset-basic-fi.js.
		 *
		 * E5.1 Section 15.9.1.9:
		 * UTC(t) = t - LocalTZA - DaylightSavingTA(t - LocalTZA)
		 *
		 * For NaN/inf, DUK_USE_DATE_GET_LOCAL_TZOFFSET() returns 0.
		 */

#if 0
		/* Old solution: don't iterate, incorrect */
		tzoff = DUK_USE_DATE_GET_LOCAL_TZOFFSET(d);
		DUK_DDD(DUK_DDDPRINT("tzoffset w/o iteration, tzoff=%ld", (long) tzoff));
		d -= tzoff * 1000L;
		DUK_UNREF(tzoffprev1);
		DUK_UNREF(tzoffprev2);
#endif

		/* Iteration solution */
		tzoff = 0;
		tzoffprev1 = 999999999L; /* invalid value which never matches */
		for (i = 0; i < DUK__LOCAL_TZOFFSET_MAXITER; i++) {
			tzoffprev2 = tzoffprev1;
			tzoffprev1 = tzoff;
			tzoff = DUK_USE_DATE_GET_LOCAL_TZOFFSET(d - tzoff * 1000L);
			DUK_DDD(DUK_DDDPRINT("tzoffset iteration, i=%d, tzoff=%ld, tzoffprev1=%ld tzoffprev2=%ld",
			                     (int) i,
			                     (long) tzoff,
			                     (long) tzoffprev1,
			                     (long) tzoffprev2));
			if (tzoff == tzoffprev1) {
				DUK_DDD(DUK_DDDPRINT("tzoffset iteration finished, i=%d, tzoff=%ld, tzoffprev1=%ld, tzoffprev2=%ld",
				                     (int) i,
				                     (long) tzoff,
				                     (long) tzoffprev1,
				                     (long) tzoffprev2));
				break;
			} else if (tzoff == tzoffprev2) {
				/* Two value cycle, see e.g. test-bi-date-tzoffset-basic-fi.js.
				 * In these cases, favor a higher tzoffset to get a consistent
				 * result which is independent of iteration count.  Not sure if
				 * this is a generically correct solution.
				 */
				DUK_DDD(DUK_DDDPRINT(
				    "tzoffset iteration two-value cycle, i=%d, tzoff=%ld, tzoffprev1=%ld, tzoffprev2=%ld",
				    (int) i,
				    (long) tzoff,
				    (long) tzoffprev1,
				    (long) tzoffprev2));
				if (tzoffprev1 > tzoff) {
					tzoff = tzoffprev1;
				}
				break;
			}
		}
		DUK_DDD(DUK_DDDPRINT("tzoffset iteration, tzoff=%ld", (long) tzoff));
		d -= tzoff * 1000L;
	}

	/* TimeClip(), which also handles Infinity -> NaN conversion */
	d = duk__timeclip(d);

	return d;
}

/*
 *  API oriented helpers
 */

/* Push 'this' binding, check that it is a Date object; then push the
 * internal time value.  At the end, stack is: [ ... this timeval ].
 * Returns the time value.  Local time adjustment is done if requested.
 */
DUK_LOCAL duk_double_t duk__push_this_get_timeval_tzoffset(duk_hthread *thr, duk_small_uint_t flags, duk_int_t *out_tzoffset) {
	duk_hobject *h;
	duk_double_t d;
	duk_int_t tzoffset = 0;

	duk_push_this(thr);
	h = duk_get_hobject(thr, -1); /* XXX: getter with class check, useful in built-ins */
	if (h == NULL || DUK_HOBJECT_GET_CLASS_NUMBER(h) != DUK_HOBJECT_CLASS_DATE) {
		DUK_ERROR_TYPE(thr, "expected Date");
		DUK_WO_NORETURN(return 0.0;);
	}

	duk_xget_owndataprop_stridx_short(thr, -1, DUK_STRIDX_INT_VALUE);
	d = duk_to_number_m1(thr);
	duk_pop(thr);

	if (DUK_ISNAN(d)) {
		if (flags & DUK_DATE_FLAG_NAN_TO_ZERO) {
			d = 0.0;
		}
		if (flags & DUK_DATE_FLAG_NAN_TO_RANGE_ERROR) {
			DUK_ERROR_RANGE(thr, "Invalid Date");
			DUK_WO_NORETURN(return 0.0;);
		}
	}
	/* if no NaN handling flag, may still be NaN here, but not Inf */
	DUK_ASSERT(!DUK_ISINF(d));

	if (flags & DUK_DATE_FLAG_LOCALTIME) {
		/* Note: DST adjustment is determined using UTC time.
		 * If 'd' is NaN, tzoffset will be 0.
		 */
		tzoffset = DUK_USE_DATE_GET_LOCAL_TZOFFSET(d); /* seconds */
		d += tzoffset * 1000L;
	}
	if (out_tzoffset) {
		*out_tzoffset = tzoffset;
	}

	/* [ ... this ] */
	return d;
}

DUK_LOCAL duk_double_t duk__push_this_get_timeval(duk_hthread *thr, duk_small_uint_t flags) {
	return duk__push_this_get_timeval_tzoffset(thr, flags, NULL);
}

/* Set timeval to 'this' from dparts, push the new time value onto the
 * value stack and return 1 (caller can then tail call us).  Expects
 * the value stack to contain 'this' on the stack top.
 */
DUK_LOCAL duk_ret_t duk__set_this_timeval_from_dparts(duk_hthread *thr, duk_double_t *dparts, duk_small_uint_t flags) {
	duk_double_t d;

	/* [ ... this ] */

	d = duk_bi_date_get_timeval_from_dparts(dparts, flags);
	duk_push_number(thr, d); /* -> [ ... this timeval_new ] */
	duk_dup_top(thr); /* -> [ ... this timeval_new timeval_new ] */

	/* Must force write because e.g. .setYear() must work even when
	 * the Date instance is frozen.
	 */
	duk_xdef_prop_stridx_short(thr, -3, DUK_STRIDX_INT_VALUE, DUK_PROPDESC_FLAGS_W);

	/* Stack top: new time value, return 1 to allow tail calls. */
	return 1;
}

/* 'out_buf' must be at least DUK_BI_DATE_ISO8601_BUFSIZE long. */
DUK_LOCAL void duk__format_parts_iso8601(duk_int_t *parts, duk_int_t tzoffset, duk_small_uint_t flags, duk_uint8_t *out_buf) {
	char yearstr[8]; /* "-123456\0" */
	char tzstr[8]; /* "+11:22\0" */
	char sep = (flags & DUK_DATE_FLAG_SEP_T) ? DUK_ASC_UC_T : DUK_ASC_SPACE;

	DUK_ASSERT(parts[DUK_DATE_IDX_MONTH] >= 1 && parts[DUK_DATE_IDX_MONTH] <= 12);
	DUK_ASSERT(parts[DUK_DATE_IDX_DAY] >= 1 && parts[DUK_DATE_IDX_DAY] <= 31);
	DUK_ASSERT(parts[DUK_DATE_IDX_YEAR] >= -999999 && parts[DUK_DATE_IDX_YEAR] <= 999999);

	/* Note: %06d for positive value, %07d for negative value to include
	 * sign and 6 digits.
	 */
	DUK_SNPRINTF(yearstr,
	             sizeof(yearstr),
	             (parts[DUK_DATE_IDX_YEAR] >= 0 && parts[DUK_DATE_IDX_YEAR] <= 9999) ?
                         "%04ld" :
                         ((parts[DUK_DATE_IDX_YEAR] >= 0) ? "+%06ld" : "%07ld"),
	             (long) parts[DUK_DATE_IDX_YEAR]);
	yearstr[sizeof(yearstr) - 1] = (char) 0;

	if (flags & DUK_DATE_FLAG_LOCALTIME) {
		/* tzoffset seconds are dropped; 16 bits suffice for
		 * time offset in minutes
		 */
		const char *fmt;
		duk_small_int_t tmp, arg_hours, arg_minutes;

		if (tzoffset >= 0) {
			tmp = tzoffset;
			fmt = "+%02d:%02d";
		} else {
			tmp = -tzoffset;
			fmt = "-%02d:%02d";
		}
		tmp = tmp / 60;
		arg_hours = tmp / 60;
		arg_minutes = tmp % 60;
		DUK_ASSERT(arg_hours <= 24); /* Even less is actually guaranteed for a valid tzoffset. */
		arg_hours = arg_hours & 0x3f; /* For [0,24] this is a no-op, but fixes GCC 7 warning, see
		                                 https://github.com/svaarala/duktape/issues/1602. */

		DUK_SNPRINTF(tzstr, sizeof(tzstr), fmt, (int) arg_hours, (int) arg_minutes);
		tzstr[sizeof(tzstr) - 1] = (char) 0;
	} else {
		tzstr[0] = DUK_ASC_UC_Z;
		tzstr[1] = (char) 0;
	}

	/* Unlike year, the other parts fit into 16 bits so %d format
	 * is portable.
	 */
	if ((flags & DUK_DATE_FLAG_TOSTRING_DATE) && (flags & DUK_DATE_FLAG_TOSTRING_TIME)) {
		DUK_SPRINTF((char *) out_buf,
		            "%s-%02d-%02d%c%02d:%02d:%02d.%03d%s",
		            (const char *) yearstr,
		            (int) parts[DUK_DATE_IDX_MONTH],
		            (int) parts[DUK_DATE_IDX_DAY],
		            (int) sep,
		            (int) parts[DUK_DATE_IDX_HOUR],
		            (int) parts[DUK_DATE_IDX_MINUTE],
		            (int) parts[DUK_DATE_IDX_SECOND],
		            (int) parts[DUK_DATE_IDX_MILLISECOND],
		            (const char *) tzstr);
	} else if (flags & DUK_DATE_FLAG_TOSTRING_DATE) {
		DUK_SPRINTF((char *) out_buf,
		            "%s-%02d-%02d",
		            (const char *) yearstr,
		            (int) parts[DUK_DATE_IDX_MONTH],
		            (int) parts[DUK_DATE_IDX_DAY]);
	} else {
		DUK_ASSERT(flags & DUK_DATE_FLAG_TOSTRING_TIME);
		DUK_SPRINTF((char *) out_buf,
		            "%02d:%02d:%02d.%03d%s",
		            (int) parts[DUK_DATE_IDX_HOUR],
		            (int) parts[DUK_DATE_IDX_MINUTE],
		            (int) parts[DUK_DATE_IDX_SECOND],
		            (int) parts[DUK_DATE_IDX_MILLISECOND],
		            (const char *) tzstr);
	}
}

/* Helper for string conversion calls: check 'this' binding, get the
 * internal time value, and format date and/or time in a few formats.
 * Return value allows tail calls.
 */
DUK_LOCAL duk_ret_t duk__to_string_helper(duk_hthread *thr, duk_small_uint_t flags) {
	duk_double_t d;
	duk_int_t parts[DUK_DATE_IDX_NUM_PARTS];
	duk_int_t tzoffset; /* seconds, doesn't fit into 16 bits */
	duk_bool_t rc;
	duk_uint8_t buf[DUK_BI_DATE_ISO8601_BUFSIZE];

	DUK_UNREF(rc); /* unreferenced with some options */

	d = duk__push_this_get_timeval_tzoffset(thr, flags, &tzoffset);
	if (DUK_ISNAN(d)) {
		duk_push_hstring_stridx(thr, DUK_STRIDX_INVALID_DATE);
		return 1;
	}
	DUK_ASSERT(DUK_ISFINITE(d));

	/* formatters always get one-based month/day-of-month */
	duk_bi_date_timeval_to_parts(d, parts, NULL, DUK_DATE_FLAG_ONEBASED);
	DUK_ASSERT(parts[DUK_DATE_IDX_MONTH] >= 1 && parts[DUK_DATE_IDX_MONTH] <= 12);
	DUK_ASSERT(parts[DUK_DATE_IDX_DAY] >= 1 && parts[DUK_DATE_IDX_DAY] <= 31);

	if (flags & DUK_DATE_FLAG_TOSTRING_LOCALE) {
		/* try locale specific formatter; if it refuses to format the
		 * string, fall back to an ISO 8601 formatted value in local
		 * time.
		 */
#if defined(DUK_USE_DATE_FORMAT_STRING)
		/* Contract, either:
		 * - Push string to value stack and return 1
		 * - Don't push anything and return 0
		 */

		rc = DUK_USE_DATE_FORMAT_STRING(thr, parts, tzoffset, flags);
		if (rc != 0) {
			return 1;
		}
#else
		/* No locale specific formatter; this is OK, we fall back
		 * to ISO 8601.
		 */
#endif
	}

	/* Different calling convention than above used because the helper
	 * is shared.
	 */
	duk__format_parts_iso8601(parts, tzoffset, flags, buf);
	duk_push_string(thr, (const char *) buf);
	return 1;
}

/* Helper for component getter calls: check 'this' binding, get the
 * internal time value, split it into parts (either as UTC time or
 * local time), push a specified component as a return value to the
 * value stack and return 1 (caller can then tail call us).
 */
DUK_LOCAL duk_ret_t duk__get_part_helper(duk_hthread *thr, duk_small_uint_t flags_and_idx) {
	duk_double_t d;
	duk_int_t parts[DUK_DATE_IDX_NUM_PARTS];
	duk_small_uint_t idx_part = (duk_small_uint_t) (flags_and_idx >> DUK_DATE_FLAG_VALUE_SHIFT); /* unpack args */

	DUK_ASSERT_DISABLE(idx_part >= 0); /* unsigned */
	DUK_ASSERT(idx_part < DUK_DATE_IDX_NUM_PARTS);

	d = duk__push_this_get_timeval(thr, flags_and_idx);
	if (DUK_ISNAN(d)) {
		duk_push_nan(thr);
		return 1;
	}
	DUK_ASSERT(DUK_ISFINITE(d));

	duk_bi_date_timeval_to_parts(d, parts, NULL, flags_and_idx); /* no need to mask idx portion */

	/* Setter APIs detect special year numbers (0...99) and apply a +1900
	 * only in certain cases.  The legacy getYear() getter applies -1900
	 * unconditionally.
	 */
	duk_push_int(thr, (flags_and_idx & DUK_DATE_FLAG_SUB1900) ? parts[idx_part] - 1900 : parts[idx_part]);
	return 1;
}

/* Helper for component setter calls: check 'this' binding, get the
 * internal time value, split it into parts (either as UTC time or
 * local time), modify one or more components as specified, recompute
 * the time value, set it as the internal value.  Finally, push the
 * new time value as a return value to the value stack and return 1
 * (caller can then tail call us).
 */
DUK_LOCAL duk_ret_t duk__set_part_helper(duk_hthread *thr, duk_small_uint_t flags_and_maxnargs) {
	duk_double_t d;
	duk_int_t parts[DUK_DATE_IDX_NUM_PARTS];
	duk_double_t dparts[DUK_DATE_IDX_NUM_PARTS];
	duk_idx_t nargs;
	duk_small_uint_t maxnargs = (duk_small_uint_t) (flags_and_maxnargs >> DUK_DATE_FLAG_VALUE_SHIFT); /* unpack args */
	duk_small_uint_t idx_first, idx;
	duk_small_uint_t i;

	nargs = duk_get_top(thr);
	d = duk__push_this_get_timeval(thr, flags_and_maxnargs);
	DUK_ASSERT(DUK_ISFINITE(d) || DUK_ISNAN(d));

	if (DUK_ISFINITE(d)) {
		duk_bi_date_timeval_to_parts(d, parts, dparts, flags_and_maxnargs);
	} else {
		/* NaN timevalue: we need to coerce the arguments, but
		 * the resulting internal timestamp needs to remain NaN.
		 * This works but is not pretty: parts and dparts will
		 * be partially uninitialized, but we only write to them.
		 */
	}

	/*
	 *  Determining which datetime components to overwrite based on
	 *  stack arguments is a bit complicated, but important to factor
	 *  out from setters themselves for compactness.
	 *
	 *  If DUK_DATE_FLAG_TIMESETTER, maxnargs indicates setter type:
	 *
	 *   1 -> millisecond
	 *   2 -> second, [millisecond]
	 *   3 -> minute, [second], [millisecond]
	 *   4 -> hour, [minute], [second], [millisecond]
	 *
	 *  Else:
	 *
	 *   1 -> date
	 *   2 -> month, [date]
	 *   3 -> year, [month], [date]
	 *
	 *  By comparing nargs and maxnargs (and flags) we know which
	 *  components to override.  We rely on part index ordering.
	 */

	if (flags_and_maxnargs & DUK_DATE_FLAG_TIMESETTER) {
		DUK_ASSERT(maxnargs >= 1 && maxnargs <= 4);
		idx_first = DUK_DATE_IDX_MILLISECOND - (maxnargs - 1);
	} else {
		DUK_ASSERT(maxnargs >= 1 && maxnargs <= 3);
		idx_first = DUK_DATE_IDX_DAY - (maxnargs - 1);
	}
	DUK_ASSERT_DISABLE(idx_first >= 0); /* unsigned */
	DUK_ASSERT(idx_first < DUK_DATE_IDX_NUM_PARTS);

	for (i = 0; i < maxnargs; i++) {
		if ((duk_idx_t) i >= nargs) {
			/* no argument given -> leave components untouched */
			break;
		}
		idx = idx_first + i;
		DUK_ASSERT_DISABLE(idx >= 0); /* unsigned */
		DUK_ASSERT(idx < DUK_DATE_IDX_NUM_PARTS);

		if (idx == DUK_DATE_IDX_YEAR && (flags_and_maxnargs & DUK_DATE_FLAG_YEAR_FIXUP)) {
			duk__twodigit_year_fixup(thr, (duk_idx_t) i);
		}

		dparts[idx] = duk_to_number(thr, (duk_idx_t) i);

		if (idx == DUK_DATE_IDX_DAY) {
			/* Day-of-month is one-based in the API, but zero-based
			 * internally, so fix here.  Note that month is zero-based
			 * both in the API and internally.
			 */
			/* SCANBUILD: complains about use of uninitialized values.
			 * The complaint is correct, but operating in undefined
			 * values here is intentional in some cases and the caller
			 * ignores the results.
			 */
			dparts[idx] -= 1.0;
		}
	}

	/* Leaves new timevalue on stack top and returns 1, which is correct
	 * for part setters.
	 */
	if (DUK_ISFINITE(d)) {
		return duk__set_this_timeval_from_dparts(thr, dparts, flags_and_maxnargs);
	} else {
		/* Internal timevalue is already NaN, so don't touch it. */
		duk_push_nan(thr);
		return 1;
	}
}

/* Apply ToNumber() to specified index; if ToInteger(val) in [0,99], add
 * 1900 and replace value at idx_val.
 */
DUK_LOCAL void duk__twodigit_year_fixup(duk_hthread *thr, duk_idx_t idx_val) {
	duk_double_t d;

	/* XXX: idx_val would fit into 16 bits, but using duk_small_uint_t
	 * might not generate better code due to casting.
	 */

	/* E5 Sections 15.9.3.1, B.2.4, B.2.5 */
	duk_to_number(thr, idx_val);
	if (duk_is_nan(thr, idx_val)) {
		return;
	}
	duk_dup(thr, idx_val);
	duk_to_int(thr, -1);
	d = duk_get_number(thr, -1); /* get as double to handle huge numbers correctly */
	if (d >= 0.0 && d <= 99.0) {
		d += 1900.0;
		duk_push_number(thr, d);
		duk_replace(thr, idx_val);
	}
	duk_pop(thr);
}

/* Set datetime parts from stack arguments, defaulting any missing values.
 * Day-of-week is not set; it is not required when setting the time value.
 */
DUK_LOCAL void duk__set_parts_from_args(duk_hthread *thr, duk_double_t *dparts, duk_idx_t nargs) {
	duk_double_t d;
	duk_small_uint_t i;
	duk_small_uint_t idx;

	/* Causes a ToNumber() coercion, but doesn't break coercion order since
	 * year is coerced first anyway.
	 */
	duk__twodigit_year_fixup(thr, 0);

	/* There are at most 7 args, but we use 8 here so that also
	 * DUK_DATE_IDX_WEEKDAY gets initialized (to zero) to avoid the potential
	 * for any Valgrind gripes later.
	 */
	for (i = 0; i < 8; i++) {
		/* Note: rely on index ordering */
		idx = DUK_DATE_IDX_YEAR + i;
		if ((duk_idx_t) i < nargs) {
			d = duk_to_number(thr, (duk_idx_t) i);
			if (idx == DUK_DATE_IDX_DAY) {
				/* Convert day from one-based to zero-based (internal).  This may
				 * cause the day part to be negative, which is OK.
				 */
				d -= 1.0;
			}
		} else {
			/* All components default to 0 except day-of-month which defaults
			 * to 1.  However, because our internal day-of-month is zero-based,
			 * it also defaults to zero here.
			 */
			d = 0.0;
		}
		dparts[idx] = d;
	}

	DUK_DDD(DUK_DDDPRINT("parts from args -> %lf %lf %lf %lf %lf %lf %lf %lf",
	                     (double) dparts[0],
	                     (double) dparts[1],
	                     (double) dparts[2],
	                     (double) dparts[3],
	                     (double) dparts[4],
	                     (double) dparts[5],
	                     (double) dparts[6],
	                     (double) dparts[7]));
}

/*
 *  Indirect magic value lookup for Date methods.
 *
 *  Date methods don't put their control flags into the function magic value
 *  because they wouldn't fit into a LIGHTFUNC's magic field.  Instead, the
 *  magic value is set to an index pointing to the array of control flags
 *  below.
 *
 *  This must be kept in strict sync with genbuiltins.py!
 */

static duk_uint16_t duk__date_magics[] = {
	/* 0: toString */
	DUK_DATE_FLAG_TOSTRING_DATE + DUK_DATE_FLAG_TOSTRING_TIME + DUK_DATE_FLAG_LOCALTIME,

	/* 1: toDateString */
	DUK_DATE_FLAG_TOSTRING_DATE + DUK_DATE_FLAG_LOCALTIME,

	/* 2: toTimeString */
	DUK_DATE_FLAG_TOSTRING_TIME + DUK_DATE_FLAG_LOCALTIME,

	/* 3: toLocaleString */
	DUK_DATE_FLAG_TOSTRING_DATE + DUK_DATE_FLAG_TOSTRING_TIME + DUK_DATE_FLAG_TOSTRING_LOCALE + DUK_DATE_FLAG_LOCALTIME,

	/* 4: toLocaleDateString */
	DUK_DATE_FLAG_TOSTRING_DATE + DUK_DATE_FLAG_TOSTRING_LOCALE + DUK_DATE_FLAG_LOCALTIME,

	/* 5: toLocaleTimeString */
	DUK_DATE_FLAG_TOSTRING_TIME + DUK_DATE_FLAG_TOSTRING_LOCALE + DUK_DATE_FLAG_LOCALTIME,

	/* 6: toUTCString */
	DUK_DATE_FLAG_TOSTRING_DATE + DUK_DATE_FLAG_TOSTRING_TIME,

	/* 7: toISOString */
	DUK_DATE_FLAG_TOSTRING_DATE + DUK_DATE_FLAG_TOSTRING_TIME + DUK_DATE_FLAG_NAN_TO_RANGE_ERROR + DUK_DATE_FLAG_SEP_T,

	/* 8: getFullYear */
	DUK_DATE_FLAG_LOCALTIME + (DUK_DATE_IDX_YEAR << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 9: getUTCFullYear */
	0 + (DUK_DATE_IDX_YEAR << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 10: getMonth */
	DUK_DATE_FLAG_LOCALTIME + (DUK_DATE_IDX_MONTH << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 11: getUTCMonth */
	0 + (DUK_DATE_IDX_MONTH << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 12: getDate */
	DUK_DATE_FLAG_ONEBASED + DUK_DATE_FLAG_LOCALTIME + (DUK_DATE_IDX_DAY << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 13: getUTCDate */
	DUK_DATE_FLAG_ONEBASED + (DUK_DATE_IDX_DAY << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 14: getDay */
	DUK_DATE_FLAG_LOCALTIME + (DUK_DATE_IDX_WEEKDAY << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 15: getUTCDay */
	0 + (DUK_DATE_IDX_WEEKDAY << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 16: getHours */
	DUK_DATE_FLAG_LOCALTIME + (DUK_DATE_IDX_HOUR << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 17: getUTCHours */
	0 + (DUK_DATE_IDX_HOUR << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 18: getMinutes */
	DUK_DATE_FLAG_LOCALTIME + (DUK_DATE_IDX_MINUTE << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 19: getUTCMinutes */
	0 + (DUK_DATE_IDX_MINUTE << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 20: getSeconds */
	DUK_DATE_FLAG_LOCALTIME + (DUK_DATE_IDX_SECOND << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 21: getUTCSeconds */
	0 + (DUK_DATE_IDX_SECOND << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 22: getMilliseconds */
	DUK_DATE_FLAG_LOCALTIME + (DUK_DATE_IDX_MILLISECOND << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 23: getUTCMilliseconds */
	0 + (DUK_DATE_IDX_MILLISECOND << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 24: setMilliseconds */
	DUK_DATE_FLAG_TIMESETTER + DUK_DATE_FLAG_LOCALTIME + (1 << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 25: setUTCMilliseconds */
	DUK_DATE_FLAG_TIMESETTER + (1 << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 26: setSeconds */
	DUK_DATE_FLAG_TIMESETTER + DUK_DATE_FLAG_LOCALTIME + (2 << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 27: setUTCSeconds */
	DUK_DATE_FLAG_TIMESETTER + (2 << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 28: setMinutes */
	DUK_DATE_FLAG_TIMESETTER + DUK_DATE_FLAG_LOCALTIME + (3 << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 29: setUTCMinutes */
	DUK_DATE_FLAG_TIMESETTER + (3 << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 30: setHours */
	DUK_DATE_FLAG_TIMESETTER + DUK_DATE_FLAG_LOCALTIME + (4 << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 31: setUTCHours */
	DUK_DATE_FLAG_TIMESETTER + (4 << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 32: setDate */
	DUK_DATE_FLAG_LOCALTIME + (1 << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 33: setUTCDate */
	0 + (1 << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 34: setMonth */
	DUK_DATE_FLAG_LOCALTIME + (2 << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 35: setUTCMonth */
	0 + (2 << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 36: setFullYear */
	DUK_DATE_FLAG_NAN_TO_ZERO + DUK_DATE_FLAG_LOCALTIME + (3 << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 37: setUTCFullYear */
	DUK_DATE_FLAG_NAN_TO_ZERO + (3 << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 38: getYear */
	DUK_DATE_FLAG_LOCALTIME + DUK_DATE_FLAG_SUB1900 + (DUK_DATE_IDX_YEAR << DUK_DATE_FLAG_VALUE_SHIFT),

	/* 39: setYear */
	DUK_DATE_FLAG_NAN_TO_ZERO + DUK_DATE_FLAG_YEAR_FIXUP + (3 << DUK_DATE_FLAG_VALUE_SHIFT),
};

DUK_LOCAL duk_small_uint_t duk__date_get_indirect_magic(duk_hthread *thr) {
	duk_small_uint_t magicidx = (duk_small_uint_t) duk_get_current_magic(thr);
	DUK_ASSERT(magicidx < (duk_small_int_t) (sizeof(duk__date_magics) / sizeof(duk_uint16_t)));
	return (duk_small_uint_t) duk__date_magics[magicidx];
}

#if defined(DUK_USE_DATE_BUILTIN)
/*
 *  Constructor calls
 */

DUK_INTERNAL duk_ret_t duk_bi_date_constructor(duk_hthread *thr) {
	duk_idx_t nargs = duk_get_top(thr);
	duk_bool_t is_cons = duk_is_constructor_call(thr);
	duk_double_t dparts[DUK_DATE_IDX_NUM_PARTS];
	duk_double_t d;

	DUK_DDD(DUK_DDDPRINT("Date constructor, nargs=%ld, is_cons=%ld", (long) nargs, (long) is_cons));

	(void) duk_push_object_helper(thr,
	                              DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_FASTREFS |
	                                  DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_DATE),
	                              DUK_BIDX_DATE_PROTOTYPE);

	/* Unlike most built-ins, the internal [[PrimitiveValue]] of a Date
	 * is mutable.
	 */

	if (nargs == 0 || !is_cons) {
		d = duk__timeclip(duk_time_get_ecmascript_time_nofrac(thr));
		duk_push_number(thr, d);
		duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_INT_VALUE, DUK_PROPDESC_FLAGS_W);
		if (!is_cons) {
			/* called as a normal function: return new Date().toString() */
			duk_to_string(thr, -1);
		}
		return 1;
	} else if (nargs == 1) {
		const char *str;
		duk_to_primitive(thr, 0, DUK_HINT_NONE);
		str = duk_get_string_notsymbol(thr, 0);
		if (str) {
			duk__parse_string(thr, str);
			duk_replace(thr, 0); /* may be NaN */
		}
		d = duk__timeclip(duk_to_number(thr, 0)); /* symbols fail here */
		duk_push_number(thr, d);
		duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_INT_VALUE, DUK_PROPDESC_FLAGS_W);
		return 1;
	}

	duk__set_parts_from_args(thr, dparts, nargs);

	/* Parts are in local time, convert when setting. */

	(void) duk__set_this_timeval_from_dparts(thr, dparts, DUK_DATE_FLAG_LOCALTIME /*flags*/); /* -> [ ... this timeval ] */
	duk_pop(thr); /* -> [ ... this ] */
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_date_constructor_parse(duk_hthread *thr) {
	return duk__parse_string(thr, duk_to_string(thr, 0));
}

DUK_INTERNAL duk_ret_t duk_bi_date_constructor_utc(duk_hthread *thr) {
	duk_idx_t nargs = duk_get_top(thr);
	duk_double_t dparts[DUK_DATE_IDX_NUM_PARTS];
	duk_double_t d;

	/* Behavior for nargs < 2 is implementation dependent: currently we'll
	 * set a NaN time value (matching V8 behavior) in this case.
	 */

	if (nargs < 2) {
		duk_push_nan(thr);
	} else {
		duk__set_parts_from_args(thr, dparts, nargs);
		d = duk_bi_date_get_timeval_from_dparts(dparts, 0 /*flags*/);
		duk_push_number(thr, d);
	}
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_date_constructor_now(duk_hthread *thr) {
	duk_double_t d;

	d = duk_time_get_ecmascript_time_nofrac(thr);
	DUK_ASSERT(duk_double_equals(duk__timeclip(d), d)); /* TimeClip() should never be necessary */
	duk_push_number(thr, d);
	return 1;
}

/*
 *  String/JSON conversions
 *
 *  Human readable conversions are now basically ISO 8601 with a space
 *  (instead of 'T') as the date/time separator.  This is a good baseline
 *  and is platform independent.
 *
 *  A shared native helper to provide many conversions.  Magic value contains
 *  a set of flags.  The helper provides:
 *
 *    toString()
 *    toDateString()
 *    toTimeString()
 *    toLocaleString()
 *    toLocaleDateString()
 *    toLocaleTimeString()
 *    toUTCString()
 *    toISOString()
 *
 *  Notes:
 *
 *    - Date.prototype.toGMTString() and Date.prototype.toUTCString() are
 *      required to be the same ECMAScript function object (!), so it is
 *      omitted from here.
 *
 *    - Date.prototype.toUTCString(): E5.1 specification does not require a
 *      specific format, but result should be human readable.  The
 *      specification suggests using ISO 8601 format with a space (instead
 *      of 'T') separator if a more human readable format is not available.
 *
 *    - Date.prototype.toISOString(): unlike other conversion functions,
 *      toISOString() requires a RangeError for invalid date values.
 */

DUK_INTERNAL duk_ret_t duk_bi_date_prototype_tostring_shared(duk_hthread *thr) {
	duk_small_uint_t flags = duk__date_get_indirect_magic(thr);
	return duk__to_string_helper(thr, flags);
}

DUK_INTERNAL duk_ret_t duk_bi_date_prototype_value_of(duk_hthread *thr) {
	/* This native function is also used for Date.prototype.getTime()
	 * as their behavior is identical.
	 */

	duk_double_t d = duk__push_this_get_timeval(thr, 0 /*flags*/); /* -> [ this ] */
	DUK_ASSERT(DUK_ISFINITE(d) || DUK_ISNAN(d));
	duk_push_number(thr, d);
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_date_prototype_to_json(duk_hthread *thr) {
	/* Note: toJSON() is a generic function which works even if 'this'
	 * is not a Date.  The sole argument is ignored.
	 */

	duk_push_this(thr);
	duk_to_object(thr, -1);

	duk_dup_top(thr);
	duk_to_primitive(thr, -1, DUK_HINT_NUMBER);
	if (duk_is_number(thr, -1)) {
		duk_double_t d = duk_get_number(thr, -1);
		if (!DUK_ISFINITE(d)) {
			duk_push_null(thr);
			return 1;
		}
	}
	duk_pop(thr);

	duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_TO_ISO_STRING);
	duk_dup_m2(thr); /* -> [ O toIsoString O ] */
	duk_call_method(thr, 0);
	return 1;
}

/*
 *  Getters.
 *
 *  Implementing getters is quite easy.  The internal time value is either
 *  NaN, or represents milliseconds (without fractions) from Jan 1, 1970.
 *  The internal time value can be converted to integer parts, and each
 *  part will be normalized and will fit into a 32-bit signed integer.
 *
 *  A shared native helper to provide all getters.  Magic value contains
 *  a set of flags and also packs the date component index argument.  The
 *  helper provides:
 *
 *    getFullYear()
 *    getUTCFullYear()
 *    getMonth()
 *    getUTCMonth()
 *    getDate()
 *    getUTCDate()
 *    getDay()
 *    getUTCDay()
 *    getHours()
 *    getUTCHours()
 *    getMinutes()
 *    getUTCMinutes()
 *    getSeconds()
 *    getUTCSeconds()
 *    getMilliseconds()
 *    getUTCMilliseconds()
 *    getYear()
 *
 *  Notes:
 *
 *    - Date.prototype.getDate(): 'date' means day-of-month, and is
 *      zero-based in internal calculations but public API expects it to
 *      be one-based.
 *
 *    - Date.prototype.getTime() and Date.prototype.valueOf() have identical
 *      behavior.  They have separate function objects, but share the same C
 *      function (duk_bi_date_prototype_value_of).
 */

DUK_INTERNAL duk_ret_t duk_bi_date_prototype_get_shared(duk_hthread *thr) {
	duk_small_uint_t flags_and_idx = duk__date_get_indirect_magic(thr);
	return duk__get_part_helper(thr, flags_and_idx);
}

DUK_INTERNAL duk_ret_t duk_bi_date_prototype_get_timezone_offset(duk_hthread *thr) {
	/*
	 *  Return (t - LocalTime(t)) in minutes:
	 *
	 *    t - LocalTime(t) = t - (t + LocalTZA + DaylightSavingTA(t))
	 *                     = -(LocalTZA + DaylightSavingTA(t))
	 *
	 *  where DaylightSavingTA() is checked for time 't'.
	 *
	 *  Note that the sign of the result is opposite to common usage,
	 *  e.g. for EE(S)T which normally is +2h or +3h from UTC, this
	 *  function returns -120 or -180.
	 *
	 */

	duk_double_t d;
	duk_int_t tzoffset;

	/* Note: DST adjustment is determined using UTC time. */
	d = duk__push_this_get_timeval(thr, 0 /*flags*/);
	DUK_ASSERT(DUK_ISFINITE(d) || DUK_ISNAN(d));
	if (DUK_ISNAN(d)) {
		duk_push_nan(thr);
	} else {
		DUK_ASSERT(DUK_ISFINITE(d));
		tzoffset = DUK_USE_DATE_GET_LOCAL_TZOFFSET(d);
		duk_push_int(thr, -tzoffset / 60);
	}
	return 1;
}

/*
 *  Setters.
 *
 *  Setters are a bit more complicated than getters.  Component setters
 *  break down the current time value into its (normalized) component
 *  parts, replace one or more components with -unnormalized- new values,
 *  and the components are then converted back into a time value.  As an
 *  example of using unnormalized values:
 *
 *    var d = new Date(1234567890);
 *
 *  is equivalent to:
 *
 *    var d = new Date(0);
 *    d.setUTCMilliseconds(1234567890);
 *
 *  A shared native helper to provide almost all setters.  Magic value
 *  contains a set of flags and also packs the "maxnargs" argument.  The
 *  helper provides:
 *
 *    setMilliseconds()
 *    setUTCMilliseconds()
 *    setSeconds()
 *    setUTCSeconds()
 *    setMinutes()
 *    setUTCMinutes()
 *    setHours()
 *    setUTCHours()
 *    setDate()
 *    setUTCDate()
 *    setMonth()
 *    setUTCMonth()
 *    setFullYear()
 *    setUTCFullYear()
 *    setYear()
 *
 *  Notes:
 *
 *    - Date.prototype.setYear() (Section B addition): special year check
 *      is omitted.  NaN / Infinity will just flow through and ultimately
 *      result in a NaN internal time value.
 *
 *    - Date.prototype.setYear() does not have optional arguments for
 *      setting month and day-in-month (like setFullYear()), but we indicate
 *      'maxnargs' to be 3 to get the year written to the correct component
 *      index in duk__set_part_helper().  The function has nargs == 1, so only
 *      the year will be set regardless of actual argument count.
 */

DUK_INTERNAL duk_ret_t duk_bi_date_prototype_set_shared(duk_hthread *thr) {
	duk_small_uint_t flags_and_maxnargs = duk__date_get_indirect_magic(thr);
	return duk__set_part_helper(thr, flags_and_maxnargs);
}

DUK_INTERNAL duk_ret_t duk_bi_date_prototype_set_time(duk_hthread *thr) {
	duk_double_t d;

	(void) duk__push_this_get_timeval(thr, 0 /*flags*/); /* -> [ timeval this ] */
	d = duk__timeclip(duk_to_number(thr, 0));
	duk_push_number(thr, d);
	duk_dup_top(thr);
	/* Must force write because .setTime() must work even when
	 * the Date instance is frozen.
	 */
	duk_xdef_prop_stridx_short(thr, -3, DUK_STRIDX_INT_VALUE, DUK_PROPDESC_FLAGS_W);
	/* -> [ timeval this timeval ] */

	return 1;
}

/*
 *  Misc.
 */

#if defined(DUK_USE_SYMBOL_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_date_prototype_toprimitive(duk_hthread *thr) {
	duk_size_t hintlen;
	const char *hintstr;
	duk_int_t hint;

	/* Invokes OrdinaryToPrimitive() with suitable hint.  Note that the
	 * method is generic, and works on non-Date arguments too.
	 *
	 * https://www.ecma-international.org/ecma-262/6.0/#sec-date.prototype-@@toprimitive
	 */

	duk_push_this(thr);
	duk_require_object(thr, -1);
	DUK_ASSERT_TOP(thr, 2);

	hintstr = duk_require_lstring(thr, 0, &hintlen);
	if ((hintlen == 6 && DUK_STRCMP(hintstr, "string") == 0) || (hintlen == 7 && DUK_STRCMP(hintstr, "default") == 0)) {
		hint = DUK_HINT_STRING;
	} else if (hintlen == 6 && DUK_STRCMP(hintstr, "number") == 0) {
		hint = DUK_HINT_NUMBER;
	} else {
		DUK_DCERROR_TYPE_INVALID_ARGS(thr);
	}

	duk_to_primitive_ordinary(thr, -1, hint);
	return 1;
}
#endif /* DUK_USE_SYMBOL_BUILTIN */

#endif /* DUK_USE_DATE_BUILTIN */
