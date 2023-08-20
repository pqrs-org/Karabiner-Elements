/*
 *  Debugging macro calls.
 */

#include "duk_internal.h"

#if defined(DUK_USE_DEBUG)

/*
 *  Debugging enabled
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if !defined(DUK_USE_DEBUG_WRITE)
#error debugging enabled (DUK_USE_DEBUG) but DUK_USE_DEBUG_WRITE not defined
#endif

#define DUK__DEBUG_BUFSIZE DUK_USE_DEBUG_BUFSIZE

#if defined(DUK_USE_VARIADIC_MACROS)

DUK_INTERNAL void duk_debug_log(duk_int_t level, const char *file, duk_int_t line, const char *func, const char *fmt, ...) {
	va_list ap;
	long arg_level;
	const char *arg_file;
	long arg_line;
	const char *arg_func;
	const char *arg_msg;
	char buf[DUK__DEBUG_BUFSIZE];

	va_start(ap, fmt);

	duk_memzero((void *) buf, (size_t) DUK__DEBUG_BUFSIZE);
	duk_debug_vsnprintf(buf, DUK__DEBUG_BUFSIZE - 1, fmt, ap);

	arg_level = (long) level;
	arg_file = (const char *) file;
	arg_line = (long) line;
	arg_func = (const char *) func;
	arg_msg = (const char *) buf;
	DUK_USE_DEBUG_WRITE(arg_level, arg_file, arg_line, arg_func, arg_msg);

	va_end(ap);
}

#else /* DUK_USE_VARIADIC_MACROS */

DUK_INTERNAL char duk_debug_file_stash[DUK_DEBUG_STASH_SIZE];
DUK_INTERNAL duk_int_t duk_debug_line_stash;
DUK_INTERNAL char duk_debug_func_stash[DUK_DEBUG_STASH_SIZE];
DUK_INTERNAL duk_int_t duk_debug_level_stash;

DUK_INTERNAL void duk_debug_log(const char *fmt, ...) {
	va_list ap;
	long arg_level;
	const char *arg_file;
	long arg_line;
	const char *arg_func;
	const char *arg_msg;
	char buf[DUK__DEBUG_BUFSIZE];

	va_start(ap, fmt);

	duk_memzero((void *) buf, (size_t) DUK__DEBUG_BUFSIZE);
	duk_debug_vsnprintf(buf, DUK__DEBUG_BUFSIZE - 1, fmt, ap);

	arg_level = (long) duk_debug_level_stash;
	arg_file = (const char *) duk_debug_file_stash;
	arg_line = (long) duk_debug_line_stash;
	arg_func = (const char *) duk_debug_func_stash;
	arg_msg = (const char *) buf;
	DUK_USE_DEBUG_WRITE(arg_level, arg_file, arg_line, arg_func, arg_msg);

	va_end(ap);
}

#endif /* DUK_USE_VARIADIC_MACROS */

#else /* DUK_USE_DEBUG */

/*
 *  Debugging disabled
 */

#endif /* DUK_USE_DEBUG */
