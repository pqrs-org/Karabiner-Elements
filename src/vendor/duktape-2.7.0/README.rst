=======
Duktape
=======

Duktape is a small and portable ECMAScript E5/E5.1 implementation.  It is
intended to be easily embeddable into C programs, with a C API similar in
spirit to Lua's.

Duktape supports the full E5/E5.1 feature set (with some semantics updated
from ES2015+) including errors, Unicode strings, and regular expressions,
a subset of ECMAScript 2015 (E6) and ECMAScript 2016 (E7) features (e.g.
computed property names, Proxy objects, exponentiation operator, Reflect),
ES2015 ArrayBuffer/TypedView, Node.js Buffer, performance.now(), CBOR, and
WHATWG Encoding API living standard.

Duktape also provides a number of custom features such as error tracebacks,
additional data types for better C integration, combined reference counting
and mark-and sweep garbage collector, object finalizers, co-operative
threads a.k.a. coroutines, tail calls, a built-in debugger protocol, function
bytecode dump/load, and so on.  Bundled extra modules provide functionality
such as CommonJS module loading and a logging framework.

You can browse Duktape programmer's API and other documentation at:

* http://duktape.org/

In particular, you should read the getting started section:

* http://duktape.org/guide.html#gettingstarted

More examples and how-to articles are in the Duktape Wiki:

* http://wiki.duktape.org/

To build an example command line tool with interactive evaluation (REPL) and
the ability to run script files::

  $ cd <dist_root>
  $ make -f Makefile.cmdline
  [...]

  $ ./duk
  ((o) Duktape
  duk> print('Hello world!');
  Hello world!
  = undefined

  $ ./duk mandel.js
  [...]

To integrate Duktape into your program:

* Use ``tools/configure.py`` to prepare Duktape source and header files
  for build::

      # Duktape options can be customized via command line options.
      # In this example, enable "fastint" support and disable ES2015
      # Proxy support

      $ python2 tools/configure.py --output-directory duktape-src \
            -DDUK_USE_FASTINT -UDUK_USE_ES6_PROXY

* The output directory (duktape-src) will then contain ``duktape.c``,
  ``duktape.h``, and ``duk_config.h`` which you include in your build.

For more details, see:

* http://duktape.org/guide.html#compiling

* http://wiki.duktape.org/Configuring.html

This distributable contains:

* Pre-configured Duktape header and source files using the Duktape default
  configuration:

  * ``src/``: main Duktape library in a "single source file" format (duktape.c,
    duktape.h, and duk_config.h).

  * ``src-noline/``: contains a variant of ``src/duktape.c`` with no ``#line``
    directives which is preferable for some users.  See discussion in
    https://github.com/svaarala/duktape/pull/363.

  * ``src-separate/``: main Duktape library in multiple files format.

* ``src-input/``: raw input source files used by ``configure.py`` which
  recreates the combined/separate prepared sources with specific options.

* ``tools/``: various Python tools, such as ``configure.py`` for preparing
  a ``duk_config.h`` header and Duktape source files for compilation, see
  http://wiki.duktape.org/Configuring.html.

* ``config/``: configuration metadata for ``configure.py``.

* ``examples/``: further examples for using Duktape.  Although Duktape
  itself is widely portable, some of the examples are Linux only.
  For instance the ``eventloop`` example illustrates how ``setTimeout()``
  and other standard timer functions could be implemented on Unix/Linux.

* ``extras/``: utilities and modules which don't comfortably fit into the
  main Duktape library because of footprint or portability concerns.
  Extras are maintained and bug fixed code, but don't have the same version
  guarantees as the main Duktape library.

* ``polyfills/``: a few replacement suggestions for non-standard Javascript
  functions provided by other implementations.

* ``debugger/``: a debugger with a web UI, see ``debugger/README.rst`` and
  https://github.com/svaarala/duktape/blob/master/doc/debugger.rst for
  details on Duktape debugger support.  Also contains a JSON debug proxy
  (one written in Node.js and another in DukLuv) to make talking to the
  debug target easier.

* ``licenses/``: licensing information.

You can find release notes at:

* https://github.com/svaarala/duktape/blob/master/RELEASES.rst
  (summary of all versions)

* https://github.com/svaarala/duktape/blob/master/doc/release-notes-v2-7.rst
  (more detailed notes for this version)

This distributable contains Duktape version 2.7.0, created from git
commit 03d4d728f8365021de6955c649e6dcd05dcca99f (03d4d72-dirty).

Duktape is copyrighted by its authors (see ``AUTHORS.rst``) and licensed
under the MIT license (see ``LICENSE.txt``).  String hashing algorithms are
based on the algorithm from Lua (MIT license), djb2 hash, and Murmurhash2
(MIT license).  Pseudorandom number generator algorithms are based on
Adi Shamir's three-op algorithm, xoroshiro128+ (public domain), and SplitMix64
(public domain).  Duktape module loader is based on the CommonJS module
loading specification (without sharing any code), CommonJS is under the MIT
license.

Have fun!

Sami Vaarala (sami.vaarala@iki.fi)
