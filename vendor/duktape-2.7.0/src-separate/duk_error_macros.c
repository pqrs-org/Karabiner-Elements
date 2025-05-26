/*
 *  Error and fatal handling.
 */

#include "duk_internal.h"

#define DUK__ERRFMT_BUFSIZE 256 /* size for formatting buffers */

#if defined(DUK_USE_VERBOSE_ERRORS)

DUK_INTERNAL DUK_COLD void duk_err_handle_error_fmt(duk_hthread *thr,
                                                    const char *filename,
                                                    duk_uint_t line_and_code,
                                                    const char *fmt,
                                                    ...) {
	va_list ap;
	char msg[DUK__ERRFMT_BUFSIZE];
	va_start(ap, fmt);
	(void) DUK_VSNPRINTF(msg, sizeof(msg), fmt, ap);
	msg[sizeof(msg) - 1] = (char) 0;
	duk_err_create_and_throw(thr,
	                         (duk_errcode_t) (line_and_code >> 24),
	                         msg,
	                         filename,
	                         (duk_int_t) (line_and_code & 0x00ffffffL));
	va_end(ap); /* dead code, but ensures portability (see Linux man page notes) */
}

DUK_INTERNAL DUK_COLD void duk_err_handle_error(duk_hthread *thr, const char *filename, duk_uint_t line_and_code, const char *msg) {
	duk_err_create_and_throw(thr,
	                         (duk_errcode_t) (line_and_code >> 24),
	                         msg,
	                         filename,
	                         (duk_int_t) (line_and_code & 0x00ffffffL));
}

#else /* DUK_USE_VERBOSE_ERRORS */

DUK_INTERNAL DUK_COLD void duk_err_handle_error(duk_hthread *thr, duk_errcode_t code) {
	duk_err_create_and_throw(thr, code);
}

#endif /* DUK_USE_VERBOSE_ERRORS */

/*
 *  Error throwing helpers
 */

#if defined(DUK_USE_VERBOSE_ERRORS)
#if defined(DUK_USE_PARANOID_ERRORS)
DUK_INTERNAL DUK_COLD void duk_err_require_type_index(duk_hthread *thr,
                                                      const char *filename,
                                                      duk_int_t linenumber,
                                                      duk_idx_t idx,
                                                      const char *expect_name) {
	DUK_ERROR_RAW_FMT3(thr,
	                   filename,
	                   linenumber,
	                   DUK_ERR_TYPE_ERROR,
	                   "%s required, found %s (stack index %ld)",
	                   expect_name,
	                   duk_get_type_name(thr, idx),
	                   (long) idx);
}
#else
DUK_INTERNAL DUK_COLD void duk_err_require_type_index(duk_hthread *thr,
                                                      const char *filename,
                                                      duk_int_t linenumber,
                                                      duk_idx_t idx,
                                                      const char *expect_name) {
	DUK_ERROR_RAW_FMT3(thr,
	                   filename,
	                   linenumber,
	                   DUK_ERR_TYPE_ERROR,
	                   "%s required, found %s (stack index %ld)",
	                   expect_name,
	                   duk_push_string_readable(thr, idx),
	                   (long) idx);
}
#endif
DUK_INTERNAL DUK_COLD void duk_err_error_internal(duk_hthread *thr, const char *filename, duk_int_t linenumber) {
	DUK_ERROR_RAW(thr, filename, linenumber, DUK_ERR_ERROR, DUK_STR_INTERNAL_ERROR);
}
DUK_INTERNAL DUK_COLD void duk_err_error_alloc_failed(duk_hthread *thr, const char *filename, duk_int_t linenumber) {
	DUK_ERROR_RAW(thr, filename, linenumber, DUK_ERR_ERROR, DUK_STR_ALLOC_FAILED);
}
DUK_INTERNAL DUK_COLD void duk_err_error(duk_hthread *thr, const char *filename, duk_int_t linenumber, const char *message) {
	DUK_ERROR_RAW(thr, filename, linenumber, DUK_ERR_ERROR, message);
}
DUK_INTERNAL DUK_COLD void duk_err_range(duk_hthread *thr, const char *filename, duk_int_t linenumber, const char *message) {
	DUK_ERROR_RAW(thr, filename, linenumber, DUK_ERR_RANGE_ERROR, message);
}
DUK_INTERNAL DUK_COLD void duk_err_range_index(duk_hthread *thr, const char *filename, duk_int_t linenumber, duk_idx_t idx) {
	DUK_ERROR_RAW_FMT1(thr, filename, linenumber, DUK_ERR_RANGE_ERROR, "invalid stack index %ld", (long) (idx));
}
DUK_INTERNAL DUK_COLD void duk_err_range_push_beyond(duk_hthread *thr, const char *filename, duk_int_t linenumber) {
	DUK_ERROR_RAW(thr, filename, linenumber, DUK_ERR_RANGE_ERROR, DUK_STR_PUSH_BEYOND_ALLOC_STACK);
}
DUK_INTERNAL DUK_COLD void duk_err_type_invalid_args(duk_hthread *thr, const char *filename, duk_int_t linenumber) {
	DUK_ERROR_RAW(thr, filename, linenumber, DUK_ERR_TYPE_ERROR, DUK_STR_INVALID_ARGS);
}
DUK_INTERNAL DUK_COLD void duk_err_type_invalid_state(duk_hthread *thr, const char *filename, duk_int_t linenumber) {
	DUK_ERROR_RAW(thr, filename, linenumber, DUK_ERR_TYPE_ERROR, DUK_STR_INVALID_STATE);
}
DUK_INTERNAL DUK_COLD void duk_err_type_invalid_trap_result(duk_hthread *thr, const char *filename, duk_int_t linenumber) {
	DUK_ERROR_RAW(thr, filename, linenumber, DUK_ERR_TYPE_ERROR, DUK_STR_INVALID_TRAP_RESULT);
}
#else
/* The file/line arguments are NULL and 0, they're ignored by DUK_ERROR_RAW()
 * when non-verbose errors are used.
 */

DUK_NORETURN(DUK_LOCAL_DECL void duk__err_shared(duk_hthread *thr, duk_errcode_t code));
DUK_LOCAL void duk__err_shared(duk_hthread *thr, duk_errcode_t code) {
	DUK_ERROR_RAW(thr, NULL, 0, code, NULL);
}
DUK_INTERNAL DUK_COLD void duk_err_error(duk_hthread *thr) {
	duk__err_shared(thr, DUK_ERR_ERROR);
}
DUK_INTERNAL DUK_COLD void duk_err_range(duk_hthread *thr) {
	duk__err_shared(thr, DUK_ERR_RANGE_ERROR);
}
DUK_INTERNAL DUK_COLD void duk_err_eval(duk_hthread *thr) {
	duk__err_shared(thr, DUK_ERR_EVAL_ERROR);
}
DUK_INTERNAL DUK_COLD void duk_err_reference(duk_hthread *thr) {
	duk__err_shared(thr, DUK_ERR_REFERENCE_ERROR);
}
DUK_INTERNAL DUK_COLD void duk_err_syntax(duk_hthread *thr) {
	duk__err_shared(thr, DUK_ERR_SYNTAX_ERROR);
}
DUK_INTERNAL DUK_COLD void duk_err_type(duk_hthread *thr) {
	duk__err_shared(thr, DUK_ERR_TYPE_ERROR);
}
DUK_INTERNAL DUK_COLD void duk_err_uri(duk_hthread *thr) {
	duk__err_shared(thr, DUK_ERR_URI_ERROR);
}
#endif

/*
 *  Default fatal error handler
 */

DUK_INTERNAL DUK_COLD void duk_default_fatal_handler(void *udata, const char *msg) {
	DUK_UNREF(udata);
	DUK_UNREF(msg);

	msg = msg ? msg : "NULL";

#if defined(DUK_USE_FATAL_HANDLER)
	/* duk_config.h provided a custom default fatal handler. */
	DUK_D(DUK_DPRINT("custom default fatal error handler called: %s", msg));
	DUK_USE_FATAL_HANDLER(udata, msg);
#elif defined(DUK_USE_CPP_EXCEPTIONS)
	/* With C++ use a duk_fatal_exception which user code can catch in
	 * a natural way.
	 */
	DUK_D(DUK_DPRINT("built-in default C++ fatal error handler called: %s", msg));
	throw duk_fatal_exception(msg);
#else
	/* Default behavior is to abort() on error.  There's no printout
	 * which makes this awkward, so it's always recommended to use an
	 * explicit fatal error handler.
	 *
	 * ====================================================================
	 * NOTE: If you are seeing this, you are most likely dealing with an
	 * uncaught error.  You should provide a fatal error handler in Duktape
	 * heap creation, and should consider using a protected call as your
	 * first call into an empty Duktape context to properly handle errors.
	 * See:
	 *   - http://duktape.org/guide.html#error-handling
	 *   - http://wiki.duktape.org/HowtoFatalErrors.html
	 *   - http://duktape.org/api.html#taglist-protected
	 * ====================================================================
	 */
	DUK_D(DUK_DPRINT("built-in default fatal error handler called: %s", msg));
	DUK_ABORT();
#endif

	DUK_D(DUK_DPRINT("fatal error handler returned, enter forever loop"));
	for (;;) {
		/* Loop forever to ensure we don't return. */
	}
}
