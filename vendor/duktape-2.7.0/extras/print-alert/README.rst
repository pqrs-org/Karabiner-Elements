===================================================
Duktape 1.x compatible print() and alert() bindings
===================================================

The default ``print()`` and ``alert()`` built-ins were removed in Duktape 2.x
because they depended on stdout/stderr and were thus a portability issue for
some targets.  This directory contains Duktape 1.x compatible optional
print() and alert() bindings:

* Add ``duk_print_alert.c`` to list of C sources to compile.

* Ensure ``duk_print_alert.h`` is in the include path.

* Include the extra header in calling code and initialize the bindings::

      #include "duktape.h"
      #include "duk_print_alert.h"

      /* After initializing the Duktape heap or when creating a new
       * thread with a new global environment:
       */
      duk_print_alert_init(ctx, 0 /*flags*/);

  See ``duk_print_alert.h`` for available flags.

* After these steps, ``print()`` and ``alert()`` will be registered to the
  global object and are ready to use.
