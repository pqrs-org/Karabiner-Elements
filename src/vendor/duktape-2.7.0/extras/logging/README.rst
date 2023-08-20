========================================
Duktape 1.x compatible logging framework
========================================

The default ``Duktape.Logger`` object and the logging related C API calls
(``duk_log()``, ``duk_log_va()``) were removed in Duktape 2.x because they
depended on stdout/stderr and were thus a portability issue for some targets
(there were also other issues, such as the logging framework not always
matching user expectations).  This directory contains Duktape 1.x compatible
``Duktape.Logger`` object and logging API calls:

* Add ``duk_logging.c`` to list of C sources to compile.

* Ensure ``duk_logging.h`` is in the include path.

* Include the extra header in calling code and initialize the bindings::

      #include "duktape.h"
      #include "duk_logging.h"

      /* After initializing the Duktape heap or when creating a new
       * thread with a new global environment:
       */
      duk_logging_init(ctx, 0 /*flags*/);

  See ``duk_logging.h`` for available flags.

* After these steps, ``Duktape.Logger`` will be registered to the ``Duktape``
  object and is ready to use.

See https://github.com/svaarala/duktape/blob/master/doc/logging.rst and
http://wiki.duktape.org/HowtoLogging.html for more information on the
logging framework design and functionality.
