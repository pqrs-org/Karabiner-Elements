/*
 *  Prototypes for built-in functions not automatically covered by the
 *  header declarations emitted by genbuiltins.py.
 */

#if !defined(DUK_BUILTIN_PROTOS_H_INCLUDED)
#define DUK_BUILTIN_PROTOS_H_INCLUDED

/* Buffer size needed for ISO 8601 formatting.
 * Accurate value is 32 + 1 for NUL termination:
 *   >>> len('+123456-01-23T12:34:56.123+12:34')
 *   32
 * Include additional space to be safe.
 */
#define DUK_BI_DATE_ISO8601_BUFSIZE 40

/* Helpers exposed for internal use */
DUK_INTERNAL_DECL void duk_bi_date_timeval_to_parts(duk_double_t d, duk_int_t *parts, duk_double_t *dparts, duk_small_uint_t flags);
DUK_INTERNAL_DECL duk_double_t duk_bi_date_get_timeval_from_dparts(duk_double_t *dparts, duk_small_uint_t flags);
DUK_INTERNAL_DECL duk_bool_t duk_bi_date_is_leap_year(duk_int_t year);
DUK_INTERNAL_DECL duk_bool_t duk_bi_date_timeval_in_valid_range(duk_double_t x);
DUK_INTERNAL_DECL duk_bool_t duk_bi_date_year_in_valid_range(duk_double_t year);
DUK_INTERNAL_DECL duk_bool_t duk_bi_date_timeval_in_leeway_range(duk_double_t x);
/* Built-in providers */
#if defined(DUK_USE_DATE_NOW_GETTIMEOFDAY)
DUK_INTERNAL_DECL duk_double_t duk_bi_date_get_now_gettimeofday(void);
#endif
#if defined(DUK_USE_DATE_NOW_TIME)
DUK_INTERNAL_DECL duk_double_t duk_bi_date_get_now_time(void);
#endif
#if defined(DUK_USE_DATE_NOW_WINDOWS)
DUK_INTERNAL_DECL duk_double_t duk_bi_date_get_now_windows(void);
#endif
#if defined(DUK_USE_DATE_NOW_WINDOWS_SUBMS)
DUK_INTERNAL_DECL duk_double_t duk_bi_date_get_now_windows_subms(void);
#endif
#if defined(DUK_USE_DATE_TZO_GMTIME_R) || defined(DUK_USE_DATE_TZO_GMTIME_S) || defined(DUK_USE_DATE_TZO_GMTIME)
DUK_INTERNAL_DECL duk_int_t duk_bi_date_get_local_tzoffset_gmtime(duk_double_t d);
#endif
#if defined(DUK_USE_DATE_TZO_WINDOWS)
DUK_INTERNAL_DECL duk_int_t duk_bi_date_get_local_tzoffset_windows(duk_double_t d);
#endif
#if defined(DUK_USE_DATE_TZO_WINDOWS_NO_DST)
DUK_INTERNAL_DECL duk_int_t duk_bi_date_get_local_tzoffset_windows_no_dst(duk_double_t d);
#endif
#if defined(DUK_USE_DATE_PRS_STRPTIME)
DUK_INTERNAL_DECL duk_bool_t duk_bi_date_parse_string_strptime(duk_hthread *thr, const char *str);
#endif
#if defined(DUK_USE_DATE_PRS_GETDATE)
DUK_INTERNAL_DECL duk_bool_t duk_bi_date_parse_string_getdate(duk_hthread *thr, const char *str);
#endif
#if defined(DUK_USE_DATE_FMT_STRFTIME)
DUK_INTERNAL_DECL duk_bool_t duk_bi_date_format_parts_strftime(duk_hthread *thr,
                                                               duk_int_t *parts,
                                                               duk_int_t tzoffset,
                                                               duk_small_uint_t flags);
#endif

#if defined(DUK_USE_GET_MONOTONIC_TIME_CLOCK_GETTIME)
DUK_INTERNAL_DECL duk_double_t duk_bi_date_get_monotonic_time_clock_gettime(void);
#endif
#if defined(DUK_USE_GET_MONOTONIC_TIME_WINDOWS_QPC)
DUK_INTERNAL_DECL duk_double_t duk_bi_date_get_monotonic_time_windows_qpc(void);
#endif

DUK_INTERNAL_DECL
void duk_bi_json_parse_helper(duk_hthread *thr, duk_idx_t idx_value, duk_idx_t idx_reviver, duk_small_uint_t flags);
DUK_INTERNAL_DECL
void duk_bi_json_stringify_helper(duk_hthread *thr,
                                  duk_idx_t idx_value,
                                  duk_idx_t idx_replacer,
                                  duk_idx_t idx_space,
                                  duk_small_uint_t flags);

DUK_INTERNAL_DECL duk_ret_t duk_textdecoder_decode_utf8_nodejs(duk_hthread *thr);

#if defined(DUK_USE_ES6_PROXY)
DUK_INTERNAL_DECL void duk_proxy_ownkeys_postprocess(duk_hthread *thr, duk_hobject *h_proxy_target, duk_uint_t flags);
#endif

#endif /* DUK_BUILTIN_PROTOS_H_INCLUDED */
