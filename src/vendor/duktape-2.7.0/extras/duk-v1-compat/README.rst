================================
Duktape V1 compatibility helpers
================================

Provides helpers for migrating from Duktape 1.x to 2.x:

* Add ``duk_v1_compat.c`` to list of C sources to compile.

* Ensure ``duk_v1_compat.h`` is in the include path.

* Include the extra header in calling code::

      #include "duktape.h"
      #include "duk_v1_compat.h"

      /* ... */

      duk_dump_context_stdout(ctx);  /* Removed in Duktape 2.x. */

The helpers don't restore full 1.x compatibility because some API calls such
as ``duk_safe_call()`` have changed in an incompatible manner.

The old APIs are documented in:

* http://duktape.org/1.5.0/api.html
