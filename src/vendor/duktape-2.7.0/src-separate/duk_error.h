/*
 *  Error handling macros, assertion macro, error codes.
 *
 *  There are three types of 'errors':
 *
 *    1. Ordinary errors relative to a thread, cause a longjmp, catchable.
 *    2. Fatal errors relative to a heap, cause fatal handler to be called.
 *    3. Fatal errors without context, cause the default (not heap specific)
 *       fatal handler to be called.
 *
 *  Fatal errors without context are used by debug code such as assertions.
 *  By providing a fatal error handler for a Duktape heap, user code can
 *  avoid fatal errors without context in non-debug builds.
 */

#if !defined(DUK_ERROR_H_INCLUDED)
#define DUK_ERROR_H_INCLUDED

/*
 *  Error codes: defined in duktape.h
 *
 *  Error codes are used as a shorthand to throw exceptions from inside
 *  the implementation.  The appropriate ECMAScript object is constructed
 *  based on the code.  ECMAScript code throws objects directly.  The error
 *  codes are defined in the public API header because they are also used
 *  by calling code.
 */

/*
 *  Normal error
 *
 *  Normal error is thrown with a longjmp() through the current setjmp()
 *  catchpoint record in the duk_heap.  The 'curr_thread' of the duk_heap
 *  identifies the throwing thread.
 *
 *  Error formatting is usually unnecessary.  The error macros provide a
 *  zero argument version (no formatting) and separate macros for small
 *  argument counts.  Variadic macros are not used to avoid portability
 *  issues and avoid the need for stash-based workarounds when they're not
 *  available.  Vararg calls are avoided for non-formatted error calls
 *  because vararg call sites are larger than normal, and there are a lot
 *  of call sites with no formatting.
 *
 *  Note that special formatting provided by debug macros is NOT available.
 *
 *  The _RAW variants allow the caller to specify file and line.  This makes
 *  it easier to write checked calls which want to use the call site of the
 *  checked function, not the error macro call inside the checked function.
 */

#if defined(DUK_USE_VERBOSE_ERRORS)

/* Because there are quite many call sites, pack error code (require at most
 * 8-bit) into a single argument.
 */
#define DUK_ERROR(thr, err, msg) \
	do { \
		duk_errcode_t duk__err = (err); \
		duk_int_t duk__line = (duk_int_t) DUK_LINE_MACRO; \
		DUK_ASSERT(duk__err >= 0 && duk__err <= 0xff); \
		DUK_ASSERT(duk__line >= 0 && duk__line <= 0x00ffffffL); \
		duk_err_handle_error((thr), DUK_FILE_MACRO, (((duk_uint_t) duk__err) << 24) | ((duk_uint_t) duk__line), (msg)); \
	} while (0)
#define DUK_ERROR_RAW(thr, file, line, err, msg) \
	do { \
		duk_errcode_t duk__err = (err); \
		duk_int_t duk__line = (duk_int_t) (line); \
		DUK_ASSERT(duk__err >= 0 && duk__err <= 0xff); \
		DUK_ASSERT(duk__line >= 0 && duk__line <= 0x00ffffffL); \
		duk_err_handle_error((thr), (file), (((duk_uint_t) duk__err) << 24) | ((duk_uint_t) duk__line), (msg)); \
	} while (0)

#define DUK_ERROR_FMT1(thr, err, fmt, arg1) \
	do { \
		duk_errcode_t duk__err = (err); \
		duk_int_t duk__line = (duk_int_t) DUK_LINE_MACRO; \
		DUK_ASSERT(duk__err >= 0 && duk__err <= 0xff); \
		DUK_ASSERT(duk__line >= 0 && duk__line <= 0x00ffffffL); \
		duk_err_handle_error_fmt((thr), \
		                         DUK_FILE_MACRO, \
		                         (((duk_uint_t) duk__err) << 24) | ((duk_uint_t) duk__line), \
		                         (fmt), \
		                         (arg1)); \
	} while (0)
#define DUK_ERROR_RAW_FMT1(thr, file, line, err, fmt, arg1) \
	do { \
		duk_errcode_t duk__err = (err); \
		duk_int_t duk__line = (duk_int_t) (line); \
		DUK_ASSERT(duk__err >= 0 && duk__err <= 0xff); \
		DUK_ASSERT(duk__line >= 0 && duk__line <= 0x00ffffffL); \
		duk_err_handle_error_fmt((thr), \
		                         (file), \
		                         (((duk_uint_t) duk__err) << 24) | ((duk_uint_t) duk__line), \
		                         (fmt), \
		                         (arg1)); \
	} while (0)

#define DUK_ERROR_FMT2(thr, err, fmt, arg1, arg2) \
	do { \
		duk_errcode_t duk__err = (err); \
		duk_int_t duk__line = (duk_int_t) DUK_LINE_MACRO; \
		DUK_ASSERT(duk__err >= 0 && duk__err <= 0xff); \
		DUK_ASSERT(duk__line >= 0 && duk__line <= 0x00ffffffL); \
		duk_err_handle_error_fmt((thr), \
		                         DUK_FILE_MACRO, \
		                         (((duk_uint_t) duk__err) << 24) | ((duk_uint_t) duk__line), \
		                         (fmt), \
		                         (arg1), \
		                         (arg2)); \
	} while (0)
#define DUK_ERROR_RAW_FMT2(thr, file, line, err, fmt, arg1, arg2) \
	do { \
		duk_errcode_t duk__err = (err); \
		duk_int_t duk__line = (duk_int_t) (line); \
		DUK_ASSERT(duk__err >= 0 && duk__err <= 0xff); \
		DUK_ASSERT(duk__line >= 0 && duk__line <= 0x00ffffffL); \
		duk_err_handle_error_fmt((thr), \
		                         (file), \
		                         (((duk_uint_t) duk__err) << 24) | ((duk_uint_t) duk__line), \
		                         (fmt), \
		                         (arg1), \
		                         (arg2)); \
	} while (0)

#define DUK_ERROR_FMT3(thr, err, fmt, arg1, arg2, arg3) \
	do { \
		duk_errcode_t duk__err = (err); \
		duk_int_t duk__line = (duk_int_t) DUK_LINE_MACRO; \
		DUK_ASSERT(duk__err >= 0 && duk__err <= 0xff); \
		DUK_ASSERT(duk__line >= 0 && duk__line <= 0x00ffffffL); \
		duk_err_handle_error_fmt((thr), \
		                         DUK_FILE_MACRO, \
		                         (((duk_uint_t) duk__err) << 24) | ((duk_uint_t) duk__line), \
		                         (fmt), \
		                         (arg1), \
		                         (arg2), \
		                         (arg3)); \
	} while (0)
#define DUK_ERROR_RAW_FMT3(thr, file, line, err, fmt, arg1, arg2, arg3) \
	do { \
		duk_errcode_t duk__err = (err); \
		duk_int_t duk__line = (duk_int_t) (line); \
		DUK_ASSERT(duk__err >= 0 && duk__err <= 0xff); \
		DUK_ASSERT(duk__line >= 0 && duk__line <= 0x00ffffffL); \
		duk_err_handle_error_fmt((thr), \
		                         (file), \
		                         (((duk_uint_t) duk__err) << 24) | ((duk_uint_t) duk__line), \
		                         (fmt), \
		                         (arg1), \
		                         (arg2), \
		                         (arg3)); \
	} while (0)

#define DUK_ERROR_FMT4(thr, err, fmt, arg1, arg2, arg3, arg4) \
	do { \
		duk_errcode_t duk__err = (err); \
		duk_int_t duk__line = (duk_int_t) DUK_LINE_MACRO; \
		DUK_ASSERT(duk__err >= 0 && duk__err <= 0xff); \
		DUK_ASSERT(duk__line >= 0 && duk__line <= 0x00ffffffL); \
		duk_err_handle_error_fmt((thr), \
		                         DUK_FILE_MACRO, \
		                         (((duk_uint_t) duk__err) << 24) | ((duk_uint_t) duk__line), \
		                         (fmt), \
		                         (arg1), \
		                         (arg2), \
		                         (arg3), \
		                         (arg4)); \
	} while (0)
#define DUK_ERROR_RAW_FMT4(thr, file, line, err, fmt, arg1, arg2, arg3, arg4) \
	do { \
		duk_errcode_t duk__err = (err); \
		duk_int_t duk__line = (duk_int_t) (line); \
		DUK_ASSERT(duk__err >= 0 && duk__err <= 0xff); \
		DUK_ASSERT(duk__line >= 0 && duk__line <= 0x00ffffffL); \
		duk_err_handle_error_fmt((thr), \
		                         (file), \
		                         (((duk_uint_t) duk__err) << 24) | ((duk_uint_t) duk__line), \
		                         (fmt), \
		                         (arg1), \
		                         (arg2), \
		                         (arg3), \
		                         (arg4)); \
	} while (0)

#else /* DUK_USE_VERBOSE_ERRORS */

#define DUK_ERROR(thr, err, msg)                 duk_err_handle_error((thr), (err))
#define DUK_ERROR_RAW(thr, file, line, err, msg) duk_err_handle_error((thr), (err))

#define DUK_ERROR_FMT1(thr, err, fmt, arg1)                 DUK_ERROR((thr), (err), (fmt))
#define DUK_ERROR_RAW_FMT1(thr, file, line, err, fmt, arg1) DUK_ERROR_RAW((thr), (file), (line), (err), (fmt))

#define DUK_ERROR_FMT2(thr, err, fmt, arg1, arg2)                 DUK_ERROR((thr), (err), (fmt))
#define DUK_ERROR_RAW_FMT2(thr, file, line, err, fmt, arg1, arg2) DUK_ERROR_RAW((thr), (file), (line), (err), (fmt))

#define DUK_ERROR_FMT3(thr, err, fmt, arg1, arg2, arg3)                 DUK_ERROR((thr), (err), (fmt))
#define DUK_ERROR_RAW_FMT3(thr, file, line, err, fmt, arg1, arg2, arg3) DUK_ERROR_RAW((thr), (file), (line), (err), (fmt))

#define DUK_ERROR_FMT4(thr, err, fmt, arg1, arg2, arg3, arg4)                 DUK_ERROR((thr), (err), (fmt))
#define DUK_ERROR_RAW_FMT4(thr, file, line, err, fmt, arg1, arg2, arg3, arg4) DUK_ERROR_RAW((thr), (file), (line), (err), (fmt))

#endif /* DUK_USE_VERBOSE_ERRORS */

/*
 *  Fatal error without context
 *
 *  The macro is an expression to make it compatible with DUK_ASSERT_EXPR().
 */

#define DUK_FATAL_WITHOUT_CONTEXT(msg) duk_default_fatal_handler(NULL, (msg))

/*
 *  Error throwing helpers
 *
 *  The goal is to provide verbose and configurable error messages.  Call
 *  sites should be clean in source code and compile to a small footprint.
 *  Small footprint is also useful for performance because small cold paths
 *  reduce code cache pressure.  Adding macros here only makes sense if there
 *  are enough call sites to get concrete benefits.
 *
 *  DUK_ERROR_xxx() macros are generic and can be used anywhere.
 *
 *  DUK_DCERROR_xxx() macros can only be used in Duktape/C functions where
 *  the "return DUK_RET_xxx;" shorthand is available for low memory targets.
 *  The DUK_DCERROR_xxx() macros always either throw or perform a
 *  'return DUK_RET_xxx' from the calling function.
 */

#if defined(DUK_USE_VERBOSE_ERRORS)
/* Verbose errors with key/value summaries (non-paranoid) or without key/value
 * summaries (paranoid, for some security sensitive environments), the paranoid
 * vs. non-paranoid distinction affects only a few specific errors.
 */
#if defined(DUK_USE_PARANOID_ERRORS)
#define DUK_ERROR_REQUIRE_TYPE_INDEX(thr, idx, expectname, lowmemstr) \
	do { \
		duk_err_require_type_index((thr), DUK_FILE_MACRO, (duk_int_t) DUK_LINE_MACRO, (idx), (expectname)); \
	} while (0)
#else /* DUK_USE_PARANOID_ERRORS */
#define DUK_ERROR_REQUIRE_TYPE_INDEX(thr, idx, expectname, lowmemstr) \
	do { \
		duk_err_require_type_index((thr), DUK_FILE_MACRO, (duk_int_t) DUK_LINE_MACRO, (idx), (expectname)); \
	} while (0)
#endif /* DUK_USE_PARANOID_ERRORS */

#define DUK_ERROR_INTERNAL(thr) \
	do { \
		duk_err_error_internal((thr), DUK_FILE_MACRO, (duk_int_t) DUK_LINE_MACRO); \
	} while (0)
#define DUK_DCERROR_INTERNAL(thr) \
	do { \
		DUK_ERROR_INTERNAL((thr)); \
		return 0; \
	} while (0)
#define DUK_ERROR_ALLOC_FAILED(thr) \
	do { \
		duk_err_error_alloc_failed((thr), DUK_FILE_MACRO, (duk_int_t) DUK_LINE_MACRO); \
	} while (0)
#define DUK_ERROR_UNSUPPORTED(thr) \
	do { \
		DUK_ERROR((thr), DUK_ERR_ERROR, DUK_STR_UNSUPPORTED); \
	} while (0)
#define DUK_DCERROR_UNSUPPORTED(thr) \
	do { \
		DUK_ERROR_UNSUPPORTED((thr)); \
		return 0; \
	} while (0)
#define DUK_ERROR_ERROR(thr, msg) \
	do { \
		duk_err_error((thr), DUK_FILE_MACRO, (duk_int_t) DUK_LINE_MACRO, (msg)); \
	} while (0)
#define DUK_ERROR_RANGE_INDEX(thr, idx) \
	do { \
		duk_err_range_index((thr), DUK_FILE_MACRO, (duk_int_t) DUK_LINE_MACRO, (idx)); \
	} while (0)
#define DUK_ERROR_RANGE_PUSH_BEYOND(thr) \
	do { \
		duk_err_range_push_beyond((thr), DUK_FILE_MACRO, (duk_int_t) DUK_LINE_MACRO); \
	} while (0)
#define DUK_ERROR_RANGE_INVALID_ARGS(thr) \
	do { \
		DUK_ERROR_RANGE((thr), DUK_STR_INVALID_ARGS); \
	} while (0)
#define DUK_DCERROR_RANGE_INVALID_ARGS(thr) \
	do { \
		DUK_ERROR_RANGE_INVALID_ARGS((thr)); \
		return 0; \
	} while (0)
#define DUK_ERROR_RANGE_INVALID_COUNT(thr) \
	do { \
		DUK_ERROR_RANGE((thr), DUK_STR_INVALID_COUNT); \
	} while (0)
#define DUK_DCERROR_RANGE_INVALID_COUNT(thr) \
	do { \
		DUK_ERROR_RANGE_INVALID_COUNT((thr)); \
		return 0; \
	} while (0)
#define DUK_ERROR_RANGE_INVALID_LENGTH(thr) \
	do { \
		DUK_ERROR_RANGE((thr), DUK_STR_INVALID_LENGTH); \
	} while (0)
#define DUK_DCERROR_RANGE_INVALID_LENGTH(thr) \
	do { \
		DUK_ERROR_RANGE_INVALID_LENGTH((thr)); \
		return 0; \
	} while (0)
#define DUK_ERROR_RANGE(thr, msg) \
	do { \
		duk_err_range((thr), DUK_FILE_MACRO, (duk_int_t) DUK_LINE_MACRO, (msg)); \
	} while (0)
#define DUK_ERROR_EVAL(thr, msg) \
	do { \
		DUK_ERROR((thr), DUK_ERR_EVAL_ERROR, (msg)); \
	} while (0)
#define DUK_ERROR_REFERENCE(thr, msg) \
	do { \
		DUK_ERROR((thr), DUK_ERR_REFERENCE_ERROR, (msg)); \
	} while (0)
#define DUK_ERROR_SYNTAX(thr, msg) \
	do { \
		DUK_ERROR((thr), DUK_ERR_SYNTAX_ERROR, (msg)); \
	} while (0)
#define DUK_ERROR_TYPE_INVALID_ARGS(thr) \
	do { \
		duk_err_type_invalid_args((thr), DUK_FILE_MACRO, (duk_int_t) DUK_LINE_MACRO); \
	} while (0)
#define DUK_DCERROR_TYPE_INVALID_ARGS(thr) \
	do { \
		DUK_ERROR_TYPE_INVALID_ARGS((thr)); \
		return 0; \
	} while (0)
#define DUK_ERROR_TYPE_INVALID_STATE(thr) \
	do { \
		duk_err_type_invalid_state((thr), DUK_FILE_MACRO, (duk_int_t) DUK_LINE_MACRO); \
	} while (0)
#define DUK_DCERROR_TYPE_INVALID_STATE(thr) \
	do { \
		DUK_ERROR_TYPE_INVALID_STATE((thr)); \
		return 0; \
	} while (0)
#define DUK_ERROR_TYPE_INVALID_TRAP_RESULT(thr) \
	do { \
		duk_err_type_invalid_trap_result((thr), DUK_FILE_MACRO, (duk_int_t) DUK_LINE_MACRO); \
	} while (0)
#define DUK_DCERROR_TYPE_INVALID_TRAP_RESULT(thr) \
	do { \
		DUK_ERROR_TYPE((thr), DUK_STR_INVALID_TRAP_RESULT); \
	} while (0)
#define DUK_ERROR_TYPE(thr, msg) \
	do { \
		DUK_ERROR((thr), DUK_ERR_TYPE_ERROR, (msg)); \
	} while (0)
#define DUK_ERROR_URI(thr, msg) \
	do { \
		DUK_ERROR((thr), DUK_ERR_URI_ERROR, (msg)); \
	} while (0)
#else /* DUK_USE_VERBOSE_ERRORS */
/* Non-verbose errors for low memory targets: no file, line, or message. */

#define DUK_ERROR_REQUIRE_TYPE_INDEX(thr, idx, expectname, lowmemstr) \
	do { \
		duk_err_type((thr)); \
	} while (0)

#define DUK_ERROR_INTERNAL(thr) \
	do { \
		duk_err_error((thr)); \
	} while (0)
#define DUK_DCERROR_INTERNAL(thr) \
	do { \
		DUK_UNREF((thr)); \
		return DUK_RET_ERROR; \
	} while (0)
#define DUK_ERROR_ALLOC_FAILED(thr) \
	do { \
		duk_err_error((thr)); \
	} while (0)
#define DUK_ERROR_UNSUPPORTED(thr) \
	do { \
		duk_err_error((thr)); \
	} while (0)
#define DUK_DCERROR_UNSUPPORTED(thr) \
	do { \
		DUK_UNREF((thr)); \
		return DUK_RET_ERROR; \
	} while (0)
#define DUK_ERROR_ERROR(thr, msg) \
	do { \
		duk_err_error((thr)); \
	} while (0)
#define DUK_ERROR_RANGE_INDEX(thr, idx) \
	do { \
		duk_err_range((thr)); \
	} while (0)
#define DUK_ERROR_RANGE_PUSH_BEYOND(thr) \
	do { \
		duk_err_range((thr)); \
	} while (0)
#define DUK_ERROR_RANGE_INVALID_ARGS(thr) \
	do { \
		duk_err_range((thr)); \
	} while (0)
#define DUK_DCERROR_RANGE_INVALID_ARGS(thr) \
	do { \
		DUK_UNREF((thr)); \
		return DUK_RET_RANGE_ERROR; \
	} while (0)
#define DUK_ERROR_RANGE_INVALID_COUNT(thr) \
	do { \
		duk_err_range((thr)); \
	} while (0)
#define DUK_DCERROR_RANGE_INVALID_COUNT(thr) \
	do { \
		DUK_UNREF((thr)); \
		return DUK_RET_RANGE_ERROR; \
	} while (0)
#define DUK_ERROR_RANGE_INVALID_LENGTH(thr) \
	do { \
		duk_err_range((thr)); \
	} while (0)
#define DUK_DCERROR_RANGE_INVALID_LENGTH(thr) \
	do { \
		DUK_UNREF((thr)); \
		return DUK_RET_RANGE_ERROR; \
	} while (0)
#define DUK_ERROR_RANGE(thr, msg) \
	do { \
		duk_err_range((thr)); \
	} while (0)
#define DUK_ERROR_EVAL(thr, msg) \
	do { \
		duk_err_eval((thr)); \
	} while (0)
#define DUK_ERROR_REFERENCE(thr, msg) \
	do { \
		duk_err_reference((thr)); \
	} while (0)
#define DUK_ERROR_SYNTAX(thr, msg) \
	do { \
		duk_err_syntax((thr)); \
	} while (0)
#define DUK_ERROR_TYPE_INVALID_ARGS(thr) \
	do { \
		duk_err_type((thr)); \
	} while (0)
#define DUK_DCERROR_TYPE_INVALID_ARGS(thr) \
	do { \
		DUK_UNREF((thr)); \
		return DUK_RET_TYPE_ERROR; \
	} while (0)
#define DUK_ERROR_TYPE_INVALID_STATE(thr) \
	do { \
		duk_err_type((thr)); \
	} while (0)
#define DUK_DCERROR_TYPE_INVALID_STATE(thr) \
	do { \
		duk_err_type((thr)); \
	} while (0)
#define DUK_ERROR_TYPE_INVALID_TRAP_RESULT(thr) \
	do { \
		duk_err_type((thr)); \
	} while (0)
#define DUK_DCERROR_TYPE_INVALID_TRAP_RESULT(thr) \
	do { \
		DUK_UNREF((thr)); \
		return DUK_RET_TYPE_ERROR; \
	} while (0)
#define DUK_ERROR_TYPE_INVALID_TRAP_RESULT(thr) \
	do { \
		duk_err_type((thr)); \
	} while (0)
#define DUK_ERROR_TYPE(thr, msg) \
	do { \
		duk_err_type((thr)); \
	} while (0)
#define DUK_ERROR_URI(thr, msg) \
	do { \
		duk_err_uri((thr)); \
	} while (0)
#endif /* DUK_USE_VERBOSE_ERRORS */

/*
 *  Assert macro: failure causes a fatal error.
 *
 *  NOTE: since the assert macro doesn't take a heap/context argument, there's
 *  no way to look up a heap/context specific fatal error handler which may have
 *  been given by the application.  Instead, assertion failures always use the
 *  internal default fatal error handler; it can be replaced via duk_config.h
 *  and then applies to all Duktape heaps.
 */

#if defined(DUK_USE_ASSERTIONS)

/* The message should be a compile time constant without formatting (less risk);
 * we don't care about assertion text size because they're not used in production
 * builds.
 */
#define DUK_ASSERT(x) \
	do { \
		if (!(x)) { \
			DUK_FATAL_WITHOUT_CONTEXT("assertion failed: " #x " (" DUK_FILE_MACRO \
			                          ":" DUK_MACRO_STRINGIFY(DUK_LINE_MACRO) ")"); \
		} \
	} while (0)

/* Assertion compatible inside a comma expression, evaluates to void. */
#define DUK_ASSERT_EXPR(x) \
	((void) ((x) ? 0 : \
                       (DUK_FATAL_WITHOUT_CONTEXT("assertion failed: " #x " (" DUK_FILE_MACRO \
	                                          ":" DUK_MACRO_STRINGIFY(DUK_LINE_MACRO) ")"), \
	                0)))

#else /* DUK_USE_ASSERTIONS */

#define DUK_ASSERT(x) \
	do { /* assertion omitted */ \
	} while (0)

#define DUK_ASSERT_EXPR(x) ((void) 0)

#endif /* DUK_USE_ASSERTIONS */

/* this variant is used when an assert would generate a compile warning by
 * being always true (e.g. >= 0 comparison for an unsigned value
 */
#define DUK_ASSERT_DISABLE(x) \
	do { /* assertion disabled */ \
	} while (0)

/*
 *  Assertion helpers
 */

#if defined(DUK_USE_ASSERTIONS) && defined(DUK_USE_REFERENCE_COUNTING)
#define DUK_ASSERT_REFCOUNT_NONZERO_HEAPHDR(h) \
	do { \
		DUK_ASSERT((h) == NULL || DUK_HEAPHDR_GET_REFCOUNT((duk_heaphdr *) (h)) > 0); \
	} while (0)
#define DUK_ASSERT_REFCOUNT_NONZERO_TVAL(tv) \
	do { \
		if ((tv) != NULL && DUK_TVAL_IS_HEAP_ALLOCATED((tv))) { \
			DUK_ASSERT(DUK_HEAPHDR_GET_REFCOUNT(DUK_TVAL_GET_HEAPHDR((tv))) > 0); \
		} \
	} while (0)
#else
#define DUK_ASSERT_REFCOUNT_NONZERO_HEAPHDR(h) /* no refcount check */
#define DUK_ASSERT_REFCOUNT_NONZERO_TVAL(tv)   /* no refcount check */
#endif

#define DUK_ASSERT_TOP(ctx, n) DUK_ASSERT((duk_idx_t) duk_get_top((ctx)) == (duk_idx_t) (n))

#if defined(DUK_USE_ASSERTIONS) && defined(DUK_USE_PACKED_TVAL)
#define DUK_ASSERT_DOUBLE_IS_NORMALIZED(dval) \
	do { \
		duk_double_union duk__assert_tmp_du; \
		duk__assert_tmp_du.d = (dval); \
		DUK_ASSERT(DUK_DBLUNION_IS_NORMALIZED(&duk__assert_tmp_du)); \
	} while (0)
#else
#define DUK_ASSERT_DOUBLE_IS_NORMALIZED(dval) /* nop */
#endif

#define DUK_ASSERT_VS_SPACE(thr) DUK_ASSERT(thr->valstack_top < thr->valstack_end)

/*
 *  Helper to initialize a memory area (e.g. struct) with garbage when
 *  assertions enabled.
 */

#if defined(DUK_USE_ASSERTIONS)
#define DUK_ASSERT_SET_GARBAGE(ptr, size) \
	do { \
		duk_memset_unsafe((void *) (ptr), 0x5a, size); \
	} while (0)
#else
#define DUK_ASSERT_SET_GARBAGE(ptr, size) \
	do { \
	} while (0)
#endif

/*
 *  Helper for valstack space
 *
 *  Caller of DUK_ASSERT_VALSTACK_SPACE() estimates the number of free stack entries
 *  required for its own use, and any child calls which are not (a) Duktape API calls
 *  or (b) Duktape calls which involve extending the valstack (e.g. getter call).
 */

#define DUK_VALSTACK_ASSERT_EXTRA \
	5 /* this is added to checks to allow for Duktape \
	   * API calls in addition to function's own use \
	   */
#if defined(DUK_USE_ASSERTIONS)
#define DUK_ASSERT_VALSTACK_SPACE(thr, n) \
	do { \
		DUK_ASSERT((thr) != NULL); \
		DUK_ASSERT((thr)->valstack_end - (thr)->valstack_top >= (n) + DUK_VALSTACK_ASSERT_EXTRA); \
	} while (0)
#else
#define DUK_ASSERT_VALSTACK_SPACE(thr, n) /* no valstack space check */
#endif

/*
 *  Prototypes
 */

#if defined(DUK_USE_VERBOSE_ERRORS)
DUK_NORETURN(
    DUK_INTERNAL_DECL void duk_err_handle_error(duk_hthread *thr, const char *filename, duk_uint_t line_and_code, const char *msg));
DUK_NORETURN(DUK_INTERNAL_DECL void
                 duk_err_handle_error_fmt(duk_hthread *thr, const char *filename, duk_uint_t line_and_code, const char *fmt, ...));
#else /* DUK_USE_VERBOSE_ERRORS */
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_handle_error(duk_hthread *thr, duk_errcode_t code));
#endif /* DUK_USE_VERBOSE_ERRORS */

#if defined(DUK_USE_VERBOSE_ERRORS)
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_create_and_throw(duk_hthread *thr,
                                                             duk_errcode_t code,
                                                             const char *msg,
                                                             const char *filename,
                                                             duk_int_t line));
#else
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_create_and_throw(duk_hthread *thr, duk_errcode_t code));
#endif

DUK_NORETURN(DUK_INTERNAL_DECL void duk_error_throw_from_negative_rc(duk_hthread *thr, duk_ret_t rc));

#define DUK_AUGMENT_FLAG_NOBLAME_FILELINE (1U << 0) /* if set, don't blame C file/line for .fileName and .lineNumber */
#define DUK_AUGMENT_FLAG_SKIP_ONE         (1U << 1) /* if set, skip topmost activation in traceback construction */

#if defined(DUK_USE_AUGMENT_ERROR_CREATE)
DUK_INTERNAL_DECL void duk_err_augment_error_create(duk_hthread *thr,
                                                    duk_hthread *thr_callstack,
                                                    const char *filename,
                                                    duk_int_t line,
                                                    duk_small_uint_t flags);
#endif
#if defined(DUK_USE_AUGMENT_ERROR_THROW)
DUK_INTERNAL_DECL void duk_err_augment_error_throw(duk_hthread *thr);
#endif

#if defined(DUK_USE_VERBOSE_ERRORS)
#if defined(DUK_USE_PARANOID_ERRORS)
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_require_type_index(duk_hthread *thr,
                                                               const char *filename,
                                                               duk_int_t linenumber,
                                                               duk_idx_t idx,
                                                               const char *expect_name));
#else
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_require_type_index(duk_hthread *thr,
                                                               const char *filename,
                                                               duk_int_t linenumber,
                                                               duk_idx_t idx,
                                                               const char *expect_name));
#endif
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_error_internal(duk_hthread *thr, const char *filename, duk_int_t linenumber));
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_error_alloc_failed(duk_hthread *thr, const char *filename, duk_int_t linenumber));
DUK_NORETURN(
    DUK_INTERNAL_DECL void duk_err_error(duk_hthread *thr, const char *filename, duk_int_t linenumber, const char *message));
DUK_NORETURN(
    DUK_INTERNAL_DECL void duk_err_range_index(duk_hthread *thr, const char *filename, duk_int_t linenumber, duk_idx_t idx));
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_range_push_beyond(duk_hthread *thr, const char *filename, duk_int_t linenumber));
DUK_NORETURN(
    DUK_INTERNAL_DECL void duk_err_range(duk_hthread *thr, const char *filename, duk_int_t linenumber, const char *message));
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_type_invalid_args(duk_hthread *thr, const char *filename, duk_int_t linenumber));
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_type_invalid_state(duk_hthread *thr, const char *filename, duk_int_t linenumber));
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_type_invalid_trap_result(duk_hthread *thr, const char *filename, duk_int_t linenumber));
#else /* DUK_VERBOSE_ERRORS */
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_error(duk_hthread *thr));
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_range(duk_hthread *thr));
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_eval(duk_hthread *thr));
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_reference(duk_hthread *thr));
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_syntax(duk_hthread *thr));
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_type(duk_hthread *thr));
DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_uri(duk_hthread *thr));
#endif /* DUK_VERBOSE_ERRORS */

DUK_NORETURN(DUK_INTERNAL_DECL void duk_err_longjmp(duk_hthread *thr));

DUK_NORETURN(DUK_INTERNAL_DECL void duk_default_fatal_handler(void *udata, const char *msg));

DUK_INTERNAL_DECL void duk_err_setup_ljstate1(duk_hthread *thr, duk_small_uint_t lj_type, duk_tval *tv_val);
#if defined(DUK_USE_DEBUGGER_SUPPORT)
DUK_INTERNAL_DECL void duk_err_check_debugger_integration(duk_hthread *thr);
#endif

DUK_INTERNAL_DECL duk_hobject *duk_error_prototype_from_code(duk_hthread *thr, duk_errcode_t err_code);

#endif /* DUK_ERROR_H_INCLUDED */
