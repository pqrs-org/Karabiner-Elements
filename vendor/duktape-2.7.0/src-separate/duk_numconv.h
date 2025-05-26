/*
 *  Number-to-string conversion.  The semantics of these is very tightly
 *  bound with the ECMAScript semantics required for call sites.
 */

#if !defined(DUK_NUMCONV_H_INCLUDED)
#define DUK_NUMCONV_H_INCLUDED

/* Output a specified number of digits instead of using the shortest
 * form.  Used for toPrecision() and toFixed().
 */
#define DUK_N2S_FLAG_FIXED_FORMAT (1U << 0)

/* Force exponential format.  Used for toExponential(). */
#define DUK_N2S_FLAG_FORCE_EXP (1U << 1)

/* If number would need zero padding (for whole number part), use
 * exponential format instead.  E.g. if input number is 12300, 3
 * digits are generated ("123"), output "1.23e+4" instead of "12300".
 * Used for toPrecision().
 */
#define DUK_N2S_FLAG_NO_ZERO_PAD (1U << 2)

/* Digit count indicates number of fractions (i.e. an absolute
 * digit index instead of a relative one).  Used together with
 * DUK_N2S_FLAG_FIXED_FORMAT for toFixed().
 */
#define DUK_N2S_FLAG_FRACTION_DIGITS (1U << 3)

/*
 *  String-to-number conversion
 */

/* Maximum exponent value when parsing numbers.  This is not strictly
 * compliant as there should be no upper limit, but as we parse the
 * exponent without a bigint, impose some limit.  The limit should be
 * small enough that multiplying it (or limit-1 to be precise) won't
 * overflow signed 32-bit integer range.  Exponent is only parsed with
 * radix 10, but with maximum radix (36) a safe limit is:
 * (10000000*36).toString(16) -> '15752a00'
 */
#define DUK_S2N_MAX_EXPONENT 10000000L

/* Trim white space (= allow leading and trailing whitespace) */
#define DUK_S2N_FLAG_TRIM_WHITE (1U << 0)

/* Allow exponent */
#define DUK_S2N_FLAG_ALLOW_EXP (1U << 1)

/* Allow trailing garbage (e.g. treat "123foo" as "123) */
#define DUK_S2N_FLAG_ALLOW_GARBAGE (1U << 2)

/* Allow leading plus sign */
#define DUK_S2N_FLAG_ALLOW_PLUS (1U << 3)

/* Allow leading minus sign */
#define DUK_S2N_FLAG_ALLOW_MINUS (1U << 4)

/* Allow 'Infinity' */
#define DUK_S2N_FLAG_ALLOW_INF (1U << 5)

/* Allow fraction part */
#define DUK_S2N_FLAG_ALLOW_FRAC (1U << 6)

/* Allow naked fraction (e.g. ".123") */
#define DUK_S2N_FLAG_ALLOW_NAKED_FRAC (1U << 7)

/* Allow empty fraction (e.g. "123.") */
#define DUK_S2N_FLAG_ALLOW_EMPTY_FRAC (1U << 8)

/* Allow empty string to be interpreted as 0 */
#define DUK_S2N_FLAG_ALLOW_EMPTY_AS_ZERO (1U << 9)

/* Allow leading zeroes (e.g. "0123" -> "123") */
#define DUK_S2N_FLAG_ALLOW_LEADING_ZERO (1U << 10)

/* Allow automatic detection of hex base ("0x" or "0X" prefix),
 * overrides radix argument and forces integer mode.
 */
#define DUK_S2N_FLAG_ALLOW_AUTO_HEX_INT (1U << 11)

/* Allow automatic detection of legacy octal base ("0n"),
 * overrides radix argument and forces integer mode.
 */
#define DUK_S2N_FLAG_ALLOW_AUTO_LEGACY_OCT_INT (1U << 12)

/* Allow automatic detection of ES2015 octal base ("0o123"),
 * overrides radix argument and forces integer mode.
 */
#define DUK_S2N_FLAG_ALLOW_AUTO_OCT_INT (1U << 13)

/* Allow automatic detection of ES2015 binary base ("0b10001"),
 * overrides radix argument and forces integer mode.
 */
#define DUK_S2N_FLAG_ALLOW_AUTO_BIN_INT (1U << 14)

/*
 *  Prototypes
 */

DUK_INTERNAL_DECL void duk_numconv_stringify(duk_hthread *thr,
                                             duk_small_int_t radix,
                                             duk_small_int_t digits,
                                             duk_small_uint_t flags);
DUK_INTERNAL_DECL void duk_numconv_parse(duk_hthread *thr, duk_small_int_t radix, duk_small_uint_t flags);

#endif /* DUK_NUMCONV_H_INCLUDED */
