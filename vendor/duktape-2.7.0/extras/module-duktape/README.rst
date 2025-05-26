===============================================
Duktape 1.x compatible module loading framework
===============================================

The default built-in module loading framework was removed in Duktape 2.x
because more flexibility was needed for module loading.  This directory
contains a Duktape 1.x compatible module loading framework which you can
add to your build:

* Add ``duk_module_duktape.c`` to list of C sources to compile.

* Ensure ``duk_module_duktape.h`` is in the include path.

* Include the extra header in calling code and initialize the bindings::

      #include "duktape.h"
      #include "duk_module_duktape.h"

      /* After initializing the Duktape heap or when creating a new
       * thread with a new global environment:
       */
      duk_module_duktape_init(ctx);

  Don't call ``duk_module_duktape_init()`` more than once for the same global
  environment.

* As usual in Duktape 1.x, you should define ``Duktape.modSearch()`` to provide
  environment specific module lookups.

* After these steps, ``require()`` will be registered to the global object and
  the module system is ready to use.
