/*
 *  Debugging macros, DUK_DPRINT() and its variants in particular.
 *
 *  DUK_DPRINT() allows formatted debug prints, and supports standard
 *  and Duktape specific formatters.  See duk_debug_vsnprintf.c for details.
 *
 *  DUK_D(x), DUK_DD(x), and DUK_DDD(x) are used together with log macros
 *  for technical reasons.  They are concretely used to hide 'x' from the
 *  compiler when the corresponding log level is disabled.  This allows
 *  clean builds on non-C99 compilers, at the cost of more verbose code.
 *  Examples:
 *
 *    DUK_D(DUK_DPRINT("foo"));
 *    DUK_DD(DUK_DDPRINT("foo"));
 *    DUK_DDD(DUK_DDDPRINT("foo"));
 *
 *  This approach is preferable to the old "double parentheses" hack because
 *  double parentheses make the C99 solution worse: __FILE__ and __LINE__ can
 *  no longer be added transparently without going through globals, which
 *  works poorly with threading.
 */

#if !defined(DUK_DEBUG_H_INCLUDED)
#define DUK_DEBUG_H_INCLUDED

#if defined(DUK_USE_DEBUG)

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 0)
#define DUK_D(x) x
#else
#define DUK_D(x) \
	do { \
	} while (0) /* omit */
#endif

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 1)
#define DUK_DD(x) x
#else
#define DUK_DD(x) \
	do { \
	} while (0) /* omit */
#endif

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 2)
#define DUK_DDD(x) x
#else
#define DUK_DDD(x) \
	do { \
	} while (0) /* omit */
#endif

/*
 *  Exposed debug macros: debugging enabled
 */

#if defined(DUK_USE_VARIADIC_MACROS)

/* Note: combining __FILE__, __LINE__, and __func__ into fmt would be
 * possible compile time, but waste some space with shared function names.
 */
#define DUK__DEBUG_LOG(lev, ...) \
	duk_debug_log((duk_int_t) (lev), DUK_FILE_MACRO, (duk_int_t) DUK_LINE_MACRO, DUK_FUNC_MACRO, __VA_ARGS__);

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 0)
#define DUK_DPRINT(...) DUK__DEBUG_LOG(DUK_LEVEL_DEBUG, __VA_ARGS__)
#else
#define DUK_DPRINT(...)
#endif

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 1)
#define DUK_DDPRINT(...) DUK__DEBUG_LOG(DUK_LEVEL_DDEBUG, __VA_ARGS__)
#else
#define DUK_DDPRINT(...)
#endif

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 2)
#define DUK_DDDPRINT(...) DUK__DEBUG_LOG(DUK_LEVEL_DDDEBUG, __VA_ARGS__)
#else
#define DUK_DDDPRINT(...)
#endif

#else /* DUK_USE_VARIADIC_MACROS */

#define DUK__DEBUG_STASH(lev) \
	(void) DUK_SNPRINTF(duk_debug_file_stash, DUK_DEBUG_STASH_SIZE, "%s", (const char *) DUK_FILE_MACRO), \
	    (void) (duk_debug_file_stash[DUK_DEBUG_STASH_SIZE - 1] = (char) 0), \
	    (void) (duk_debug_line_stash = (duk_int_t) DUK_LINE_MACRO), \
	    (void) DUK_SNPRINTF(duk_debug_func_stash, DUK_DEBUG_STASH_SIZE, "%s", (const char *) DUK_FUNC_MACRO), \
	    (void) (duk_debug_func_stash[DUK_DEBUG_STASH_SIZE - 1] = (char) 0), (void) (duk_debug_level_stash = (lev))

/* Without variadic macros resort to comma expression trickery to handle debug
 * prints.  This generates a lot of harmless warnings.  These hacks are not
 * needed normally because DUK_D() and friends will hide the entire debug log
 * statement from the compiler.
 */

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 0)
#define DUK_DPRINT DUK__DEBUG_STASH(DUK_LEVEL_DEBUG), (void) duk_debug_log /* args go here in parens */
#else
#define DUK_DPRINT 0 && /* args go here as a comma expression in parens */
#endif

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 1)
#define DUK_DDPRINT DUK__DEBUG_STASH(DUK_LEVEL_DDEBUG), (void) duk_debug_log /* args go here in parens */
#else
#define DUK_DDPRINT 0 && /* args */
#endif

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 2)
#define DUK_DDDPRINT DUK__DEBUG_STASH(DUK_LEVEL_DDDEBUG), (void) duk_debug_log /* args go here in parens */
#else
#define DUK_DDDPRINT 0 && /* args */
#endif

#endif /* DUK_USE_VARIADIC_MACROS */

#else /* DUK_USE_DEBUG */

/*
 *  Exposed debug macros: debugging disabled
 */

#define DUK_D(x) \
	do { \
	} while (0) /* omit */
#define DUK_DD(x) \
	do { \
	} while (0) /* omit */
#define DUK_DDD(x) \
	do { \
	} while (0) /* omit */

#if defined(DUK_USE_VARIADIC_MACROS)

#define DUK_DPRINT(...)
#define DUK_DDPRINT(...)
#define DUK_DDDPRINT(...)

#else /* DUK_USE_VARIADIC_MACROS */

#define DUK_DPRINT   0 && /* args go here as a comma expression in parens */
#define DUK_DDPRINT  0 && /* args */
#define DUK_DDDPRINT 0 && /* args */

#endif /* DUK_USE_VARIADIC_MACROS */

#endif /* DUK_USE_DEBUG */

/*
 *  Structs
 */

#if defined(DUK_USE_DEBUG)
struct duk_fixedbuffer {
	duk_uint8_t *buffer;
	duk_size_t length;
	duk_size_t offset;
	duk_bool_t truncated;
};
#endif

/*
 *  Prototypes
 */

#if defined(DUK_USE_DEBUG)
DUK_INTERNAL_DECL duk_int_t duk_debug_vsnprintf(char *str, duk_size_t size, const char *format, va_list ap);
#if 0 /*unused*/
DUK_INTERNAL_DECL duk_int_t duk_debug_snprintf(char *str, duk_size_t size, const char *format, ...);
#endif
DUK_INTERNAL_DECL void duk_debug_format_funcptr(char *buf, duk_size_t buf_size, duk_uint8_t *fptr, duk_size_t fptr_size);

#if defined(DUK_USE_VARIADIC_MACROS)
DUK_INTERNAL_DECL void duk_debug_log(duk_int_t level, const char *file, duk_int_t line, const char *func, const char *fmt, ...);
#else /* DUK_USE_VARIADIC_MACROS */
/* parameter passing, not thread safe */
#define DUK_DEBUG_STASH_SIZE 128
#if !defined(DUK_SINGLE_FILE)
DUK_INTERNAL_DECL char duk_debug_file_stash[DUK_DEBUG_STASH_SIZE];
DUK_INTERNAL_DECL duk_int_t duk_debug_line_stash;
DUK_INTERNAL_DECL char duk_debug_func_stash[DUK_DEBUG_STASH_SIZE];
DUK_INTERNAL_DECL duk_int_t duk_debug_level_stash;
#endif
DUK_INTERNAL_DECL void duk_debug_log(const char *fmt, ...);
#endif /* DUK_USE_VARIADIC_MACROS */

DUK_INTERNAL_DECL void duk_fb_put_bytes(duk_fixedbuffer *fb, const duk_uint8_t *buffer, duk_size_t length);
DUK_INTERNAL_DECL void duk_fb_put_byte(duk_fixedbuffer *fb, duk_uint8_t x);
DUK_INTERNAL_DECL void duk_fb_put_cstring(duk_fixedbuffer *fb, const char *x);
DUK_INTERNAL_DECL void duk_fb_sprintf(duk_fixedbuffer *fb, const char *fmt, ...);
DUK_INTERNAL_DECL void duk_fb_put_funcptr(duk_fixedbuffer *fb, duk_uint8_t *fptr, duk_size_t fptr_size);
DUK_INTERNAL_DECL duk_bool_t duk_fb_is_full(duk_fixedbuffer *fb);

#endif /* DUK_USE_DEBUG */

#endif /* DUK_DEBUG_H_INCLUDED */
