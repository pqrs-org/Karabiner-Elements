=========================
Minimal 'console' binding
=========================

Duktape doesn't provide a ``console`` binding (for example ``console.log``)
by default because it would be a portability issue for some targets.  Instead,
an application should provide its own ``console`` binding.  This directory
contains an example binding:

* Add ``duk_console.c`` to list of C sources to compile.

* Ensure ``duk_console.h`` is in the include path.

* Include the extra header in calling code and initialize the bindings::

      #include "duktape.h"
      #include "duk_console.h"

      /* After initializing the Duktape heap or when creating a new
       * thread with a new global environment:
       */
      duk_console_init(ctx, 0 /*flags*/);

  Use the ``DUK_CONSOLE_PROXY_WRAPPER`` to enable a Proxy wrapper for the
  console object.  The wrapper allows all undefined methods (for example,
  ``console.foo``) to be handled as no-ops instead of throwing an error.
  See ``duk_console.h`` for full flags list.

* After these steps, ``console`` will be registered to the global object
  and is ready to use.

* By default the console object will use ``stdout`` for log levels up to
  info, and ``stderr`` for warning and above.  You can change this by
  providing either ``DUK_CONSOLE_STDOUT_ONLY`` or ``DUK_CONSOLE_STDERR_ONLY``
  in ``duk_console_init()`` flags.
