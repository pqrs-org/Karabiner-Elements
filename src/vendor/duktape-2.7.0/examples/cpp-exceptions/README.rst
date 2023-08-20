=========================================
C++ exceptions for long control transfers
=========================================

Normally Duktape uses ``setjmp()`` / ``longjmp()`` or their variants for
internal long control transfers.  One downside of these functions is that
C++ automatic destructors (scope-based resource management, SBRM, a special
case of RAII) in Duktape/C functions won't be executed which is awkward for
C++ programmers.

When ``DUK_USE_CPP_EXCEPTIONS`` is defined, and both Duktape and application
code is compiled using a C++ compiler, Duktape uses C++ ``try-catch`` and
``throw`` for internal long control transfers.  This allows automatic
destructors to run as expected.  The config option is not enabled by default
because C++ exceptions are sometimes disabled even when a C++ compiler is
used (e.g. for performance reasons).

The ``cpp_exceptions.cpp`` example illustrates how C++ exceptions can be
used in Duktape/C functions at the moment:

* Duktape uses C++ try/catch/throw internally; this is not visible to user
  code directly.

* Automatic destructors (scope-based resource management) work as expected.

* C++ exceptions can be used in Duktape/C functions normally, but user
  exceptions must be caught before they reach Duktape.  If this is not
  done, such exceptions are caught by Duktape and converted to API errors
  (in other words, they won't propagate "through" Duktape at the moment).
