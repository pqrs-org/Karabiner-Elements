=====================================
Node.js-like module loading framework
=====================================

This directory contains an example module resolution and loading framework and
``require()`` implementation based on the Node.js module system:

* https://nodejs.org/api/modules.html

The application needs only to provide the module resolution and loading logic:

* Add ``duk_module_node.c`` to list of C sources to compile.

* Ensure ``duk_module_node.h`` is in the include path.

* Include the extra header in calling code and initialize the bindings::

      #include "duktape.h"
      #include "duk_module_node.h"

      /* After initializing the Duktape heap or when creating a new
       * thread with a new global environment:
       */
      duk_push_object(ctx);
      duk_push_c_function(ctx, cb_resolve_module, DUK_VARARGS);
      duk_put_prop_string(ctx, -2, "resolve");
      duk_push_c_function(ctx, cb_load_module, DUK_VARARGS);
      duk_put_prop_string(ctx, -2, "load");
      duk_module_node_init(ctx);

  Do not call ``duk_module_node_init()`` more than once for the same global
  environment.  Doing so is undefined behavior and may put the module system
  in an inconsistent state.
  
  It is possible to replace the callbacks after initialization by setting the
  following internal properties on the global stash:
  
  - ``\xffmodResolve``
  
  - ``\xffmodLoad``

* The resolve callback is a Duktape/C function which takes the string passed
  to ``require()`` and resolves it to a canonical module ID (for Node.js this
  is usually the fully resolved filename of the module)::

      duk_ret_t cb_resolve_module(duk_context *ctx) {
          /*
           *  Entry stack: [ requested_id parent_id ]
           */

          const char *requested_id = duk_get_string(ctx, 0);
          const char *parent_id = duk_get_string(ctx, 1);  /* calling module */
          const char *resolved_id;

          /* Arrive at the canonical module ID somehow. */

          duk_push_string(ctx, resolved_id);
          return 1;  /*nrets*/
      }

  If the module ID cannot be resolved, the resolve callback should throw an
  error, which will propagate out of the ``require()`` call.  Note also that
  when the global ``require()`` is called, the parent ID is an empty string.

* The load callback is a Duktape/C function which takes the resolved module ID
  and: (1) returns the ECMAScript source code for the module or ``undefined``
  if there's no source code, e.g. for pure C modules, (2) can populate
  ``module.exports`` itself, and (3) can replace ``module.exports``::

      duk_ret_t cb_load_module(duk_context *ctx) {
          /*
           *  Entry stack: [ resolved_id exports module ]
           */

          /* Arrive at the JS source code for the module somehow. */

          duk_push_string(ctx, module_source);
          return 1;  /*nrets*/
      }

  As with the resolve callback, the load callback should throw an error if the
  module cannot be loaded for any reason.

* After these steps, ``require()`` will be registered to the global object and
  the module system is ready to use.

* The main module (file being evaluated) should be loaded using
  ``duk_module_node_peval_main()``.  This function registers the module in
  ``require.main`` and thus should only be called once.
