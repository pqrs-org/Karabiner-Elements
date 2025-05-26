/*
 *  Exceptions for Duktape internal throws when C++ exceptions are used
 *  for long control transfers.
 */

#if !defined(DUK_EXCEPTION_H_INCLUDED)
#define DUK_EXCEPTION_H_INCLUDED

#if defined(DUK_USE_CPP_EXCEPTIONS)
/* Internal exception used as a setjmp-longjmp replacement.  User code should
 * NEVER see or catch this exception, so it doesn't inherit from any base
 * class which should minimize the chance of user code accidentally catching
 * the exception.
 */
class duk_internal_exception {
	/* intentionally empty */
};

/* Fatal error, thrown as a specific C++ exception with C++ exceptions
 * enabled.  It is unsafe to continue; doing so may cause crashes or memory
 * leaks.  This is intended to be either uncaught, or caught by user code
 * aware of the "unsafe to continue" semantics.
 */
class duk_fatal_exception : public virtual std::runtime_error {
      public:
	duk_fatal_exception(const char *message) : std::runtime_error(message) {
	}
};
#endif

#endif /* DUK_EXCEPTION_H_INCLUDED */
