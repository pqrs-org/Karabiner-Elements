==============================================
Minimal sprintf/sscanf replacement for Duktape
==============================================

The ``duk_minimal_printf.c`` provides a portable provider for sprintf()/scanf()
with a feature set matching minimally what Duktape internals need.  The provider
compiles to less than 1kB.  The functions provided are::

    sprintf()
    snprintf()
    vsnprintf()
    sscanf()

In addition to Duktape internals, the set of supported formats affects the
``duk_push_(v)sprintf()`` call which may have an impact on user code.

Assumptions:

* ``sizeof(void *) <= sizeof(long)``

* ``sizeof(long) <= 8``

Note that these assumptions don't hold e.g. on 64-bit Windows.  This printf
provider is mostly useful for low memory targets where these assumptions are
typically not an issue.  The limitations are easy to fix if one relies more
on platform typing.

Supported formatting
====================

sprintf()
---------

Duktape relies on a ``sprintf()`` provider which supports at least the
following (this list is from Duktape 1.5.0)::

    %c
    %s
    %p

    %02d
    %03d
    %ld
    %lld    (JSON fast path only)

    %lu

    %lx
    %02lx
    %08lx

This minimal provider supports the following slightly different, wider set.

* Character format ``%c``.

* String format ``%s``.

* Pointer format ``%p``.

* Integer formats ``%d``, ``%ld``, ``%lu`` with optional padding and
  length modifiers.

* Hex formats ``%x``, ``%lx`` with optional padding and length modifiers.

The wider set is useful to make ``duk_push_(v)sprintf()`` behave reasonably
for any user code call sites.

The ``%lld`` format is not supported to avoid depending on the ``long long``
type; this makes the replacement incompatible with the JSON fast path which
must thus be disabled.

sscanf()
--------

There's only one call site for ``sscanf()``, for JX parsing of pointers::

    duk_bi_json.c:        (void) DUK_SSCANF((const char *) js_ctx->p, DUK_STR_FMT_PTR, &voidptr);

The exact format string here is ``%p`` and nothing else needs to be supported.
Further, when the minimal printf/scanf providers are used together we only
need to parse what we produce.  In particular:

* Pointer prefix is ``0x``, no need to match ``0X`` for example.

* All digits are ``[0-9a-f]`` with no need to match uppercase.

Building "duk" with minimal printf/scanf
========================================

The necessary defines in ``duk_config.h`` can be given to genconfig, but you
can also just make the following manual additions to the bottom of the config
file::

    #include "duk_minimal_printf.h"

    #undef DUK_SPRINTF
    #define DUK_SPRINTF duk_minimal_sprintf
    #undef DUK_SNPRINTF
    #define DUK_SNPRINTF duk_minimal_snprintf
    #undef DUK_VSNPRINTF
    #define DUK_VSNPRINTF duk_minimal_vsnprintf
    #undef DUK_SSCANF
    #define DUK_SSCANF duk_minimal_sscanf

Then just add ``duk_minimal_printf.c`` to build and compile the application.

Future work
===========

* Add support for ``%lld`` (maybe conditional) to allow JSON fast path to
  be supported.

* Add support for platforms such as 64-bit Windows where
  ``sizeof(long) < sizeof(void *)``.  This can be achieved by using a few
  typedefs internally; typedef an integer type large enough to hold all
  formatted types.
