/*
 *  Dummy Date provider
 *
 *  There are two minimally required macros which you must provide in
 *  duk_config.h:
 *
 *    extern duk_double_t dummy_get_now(void);
 *
 *    #define DUK_USE_DATE_GET_NOW(ctx) dummy_get_now()
 *    #define DUK_USE_DATE_GET_LOCAL_TZOFFSET(d)  0
 *
 *  Note that since the providers are macros, you don't need to use
 *  all arguments.  Similarly, you can "return" fixed values as
 *  constants.  Above, local timezone offset is always zero i.e.
 *  we're always in UTC.
 *
 *  You can also provide optional macros to parse and format timestamps
 *  in a platform specific format.  If not provided, Duktape will use
 *  ISO 8601 only (which is often good enough).
 */

#include "duktape.h"

duk_double_t dummy_get_now(void) {
	/* Return a fixed time here as a dummy example. */
	return -11504520000.0;
}
