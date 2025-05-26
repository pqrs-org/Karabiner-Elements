=========================================
Duktape debug client and JSON debug proxy
=========================================

Overview
========

Debugger web UI which connects to the Duktape command line tool or any other
target supporting the example TCP transport (``examples/debug-trans-socket``)
on Unix and Windows.

Also provides a JSON debug proxy with a JSON mapping for the Duktape debug
protocol.

For detailed documentation of the debugger internals, see `debugger.rst`__.

__ https://github.com/svaarala/duktape/blob/master/doc/debugger.rst

Using the debugger web UI
=========================

Some prerequisites:

* You'll need Node.js v0.10.x or newer.  Older Node.js versions don't support
  the required packages.

Compile Duktape command line tool with debugger support:

* Enable ``DUK_USE_DEBUGGER_SUPPORT`` and ``DUK_USE_INTERRUPT_COUNTER`` for
  ``tools/configure.py``.

* Enable ``DUK_CMDLINE_DEBUGGER_SUPPORT`` on compiler command line for Duktape
  command line utility.

The source distributable contains a Makefile to build a "duk" command with
debugger support::

    $ cd <duktape dist directory>
    $ make -f Makefile.dukdebug

The Duktape Git repo "duk" target has debugger support enabled by default::

    $ make clean duk

Start Duktape command line tool so that it waits for a debugger connection::

    # For now we need to be in the directory containing the source files
    # executed so that the 'fileName' properties of functions will match
    # that on the debug client.

    # Using source distributable
    $ cd <duktape dist directory>
    $ ./duk --debugger mandel.js

    # Using Duktape Git repo
    $ cd <duktape checkout>/tests/ecmascript/
    $ ../../duk --debugger test-dev-mandel2-func.js

Start the web UI::

    # Must be in 'debugger' directory.

    $ cd debugger/
    $ make  # runs 'node duk_debug.js'

Once the required packages are installed, the NodeJS debug client will be
up and running.  Open the following in your browser and start debugging:

* http://localhost:9092/

The debug client automatically attaches to the debug target on startup.
If you start the debug target later, you'll need to click "Attach" in the
web UI.

Using the JSON debug proxy
==========================

There are two JSON debug proxy implementations: one implemented in DukLuv
and another in Node.js.

DukLuv JSON proxy
-----------------

DukLuv (https://github.com/creationix/dukluv) is a small and portable event
loop based on LibUV and Duktape with MIT license (like Duktape).  As such it's
easy to embed in a custom debug client: you just include the DukLuv executable
and the JSON proxy source file in your debug client.

Install DukLuv:

* Ensure ``cmake`` is installed

* ``git clone https://github.com/creationix/dukluv.git``

* ``cd dukluv``

* ``git submodule init; git submodule update``

* ``make``

* Binary should appear in:

  - ``./build/dukluv`` on Linux

  - ``.\build\Debug\dukluv.exe`` on Windows

Run the proxy::

    # Using Makefile; autogenerates duk_debug_meta.json
    # (You may need to edit DUKLUV in Makefile to point to your DukLuv)
    $ make runproxydukluv

    # Manually: see "dukluv duk_debug_proxy.js --help" for help
    $ .../path/to/dukluv duk_debug_proxy.js

Start Duktape command line (or whatever your target is)::

    $ cd <duktape checkout>/tests/ecmascript/
    $ ../../duk --debugger test-dev-mandel2-func.js

Now connect to the proxy using e.g. telnet::

    $ telnet localhost 9093

The proxy will then connect to the target and you can start issuing commands::

    $ telnet localhost 9093
    Trying 127.0.0.1...
    Connected to localhost.
    Escape character is '^]'.
    {"notify":"_TargetConnecting","args":["127.0.0.1",9091]}
    {"notify":"_TargetConnected","args":["1 10499 v1.4.0-140-gc9a6c7c duk command built from Duktape repo"]}
    {"notify":"Status","command":1,"args":[1,"test-dev-mandel2-func.js","global",58,0]}
    {"request":"BasicInfo"}
    {"reply":true,"args":[10499,"v1.4.0-140-gc9a6c7c","duk command built from Duktape repo",1]}
    {"request":"Eval","args":["print('Hello world!'); 123;"]}
    {"notify":"Print","command":2,"args":["Hello world!\n"]}
    {"reply":true,"args":[0,{"type":"number","data":"405ec00000000000"}]}
    [...]

The proxy log provides dumps both JSON and dvalue binary traffic which is
quite useful in development::

    $ make runproxydukluv
    Running Dukluv based debug proxy
    "dukluv" duk_debug_proxy.js --log-level 2 --metadata duk_debug_meta.json
    2016-02-17T13:59:42.308Z INF Proxy: Read proxy metadata from duk_debug_meta.json
    2016-02-17T13:59:42.325Z INF Proxy: Listening for incoming JSON debug connection on 0.0.0.0:9093, target is 127.0.0.1:9091
    2016-02-17T13:59:47.994Z INF Proxy: JSON proxy client connected
    2016-02-17T13:59:47.994Z INF Proxy: Connecting to debug target at 127.0.0.1:9091
    2016-02-17T13:59:47.994Z INF Proxy: PROXY --> CLIENT: {"notify":"_TargetConnecting","args":["127.0.0.1",9091]}
    2016-02-17T13:59:47.994Z INF Proxy: Connected to debug target at 127.0.0.1:9091
    2016-02-17T13:59:48.003Z INF Proxy: PROXY --> CLIENT: {"notify":"_TargetConnected","args":["1 10499 v1.4.0-140-gc9a6c7c duk command built from Duktape repo"]}
    2016-02-17T13:59:48.003Z INF Proxy: Target handshake: {"line":"1 10499 v1.4.0-140-gc9a6c7c duk command built from Duktape repo","protocolVersion":1,"text":"10499 v1.4.0-140-gc9a6c7c duk command built from Duktape repo","dukVersion":"1","dukGitDescribe":"10499","targetString":"v1.4.0-140-gc9a6c7c"}
    2016-02-17T13:59:48.151Z INF Proxy: PROXY <-- TARGET: |04|
    2016-02-17T13:59:48.152Z INF Proxy: PROXY <-- TARGET: |81|
    2016-02-17T13:59:48.152Z INF Proxy: PROXY <-- TARGET: |81|
    2016-02-17T13:59:48.160Z INF Proxy: PROXY <-- TARGET: |78746573742d6465762d6d616e64656c322d66756e632e6a73|
    2016-02-17T13:59:48.161Z INF Proxy: PROXY <-- TARGET: |66676c6f62616c|
    2016-02-17T13:59:48.165Z INF Proxy: PROXY <-- TARGET: |ba|
    2016-02-17T13:59:48.165Z INF Proxy: PROXY <-- TARGET: |80|
    2016-02-17T13:59:48.165Z INF Proxy: PROXY <-- TARGET: |00|
    2016-02-17T13:59:48.165Z INF Proxy: PROXY --> CLIENT: {"notify":"Status","command":1,"args":[1,"test-dev-mandel2-func.js","global",58,0]}
    2016-02-17T13:59:51.289Z INF Proxy: PROXY <-- CLIENT: {"request":"BasicInfo"}
    2016-02-17T13:59:51.289Z INF Proxy: PROXY --> TARGET: |01|
    2016-02-17T13:59:51.289Z INF Proxy: PROXY --> TARGET: |90|
    2016-02-17T13:59:51.289Z INF Proxy: PROXY --> TARGET: |00|
    2016-02-17T13:59:51.291Z INF Proxy: PROXY <-- TARGET: |02|
    2016-02-17T13:59:51.291Z INF Proxy: PROXY <-- TARGET: |e903|
    2016-02-17T13:59:51.292Z INF Proxy: PROXY <-- TARGET: |7376312e342e302d3134302d6763396136633763|
    2016-02-17T13:59:51.293Z INF Proxy: PROXY <-- TARGET: |12002364756b20636f6d6d616e64206275696c742066726f6d2044756b74617065207265706f|
    2016-02-17T13:59:51.293Z INF Proxy: PROXY <-- TARGET: |81|
    2016-02-17T13:59:51.293Z INF Proxy: PROXY <-- TARGET: |00|
    2016-02-17T13:59:51.293Z INF Proxy: PROXY --> CLIENT: {"reply":true,"args":[10499,"v1.4.0-140-gc9a6c7c","duk command built from Duktape repo",1]}
    2016-02-17T14:00:06.105Z INF Proxy: PROXY <-- CLIENT: {"request":"Eval","args":["print('Hello world!'); 123;"]}
    2016-02-17T14:00:06.105Z INF Proxy: PROXY --> TARGET: |01|
    2016-02-17T14:00:06.105Z INF Proxy: PROXY --> TARGET: |9e|
    2016-02-17T14:00:06.105Z INF Proxy: PROXY --> TARGET: |7b7072696e74282748656c6c6f20776f726c642127293b203132333b|
    2016-02-17T14:00:06.105Z INF Proxy: PROXY --> TARGET: |00|
    2016-02-17T14:00:06.167Z INF Proxy: PROXY <-- TARGET: |04|
    2016-02-17T14:00:06.167Z INF Proxy: PROXY <-- TARGET: |82|
    2016-02-17T14:00:06.167Z INF Proxy: PROXY <-- TARGET: |6d48656c6c6f20776f726c64210a|
    2016-02-17T14:00:06.168Z INF Proxy: PROXY <-- TARGET: |00|
    2016-02-17T14:00:06.168Z INF Proxy: PROXY --> CLIENT: {"notify":"Print","command":2,"args":["Hello world!\n"]}
    2016-02-17T14:00:06.171Z INF Proxy: PROXY <-- TARGET: |02|
    2016-02-17T14:00:06.171Z INF Proxy: PROXY <-- TARGET: |80|
    2016-02-17T14:00:06.173Z INF Proxy: PROXY <-- TARGET: |1a405ec00000000000|
    2016-02-17T14:00:06.173Z INF Proxy: PROXY <-- TARGET: |00|
    2016-02-17T14:00:06.174Z INF Proxy: PROXY --> CLIENT: {"reply":true,"args":[0,{"type":"number","data":"405ec00000000000"}]}
    [...]

Node.js JSON proxy
------------------

A Node.js-based JSON debug proxy is also provided by ``duk_debug.js``::

    # Same prerequisites as for running the debug client
    $ make runproxynodejs

Start Duktape command line (or whatever your target is)::

    $ cd <duktape checkout>/tests/ecmascript/
    $ ../../duk --debugger test-dev-mandel2-func.js

You can then connect to localhost:9093 and interact with the proxy.
Here's an example session using telnet and manually typed in commands
The ``-->`` (send) and ``<--`` (receiver) markers have been added for
readability and are not part of the stream::

    $ telnet localhost 9093
    Trying 127.0.0.1...
    Connected to localhost.
    Escape character is '^]'.
    <-- {"notify":"_TargetConnected","args":["1 10199 v1.1.0-275-gbd4d610-dirty duk command built from Duktape repo"]}
    <-- {"notify":"Status","command":1,"args":[1,"test-dev-mandel2-func.js","global",58,0]}
    --> {"request":"BasicInfo"}
    <-- {"reply":true,"args":[10199,"v1.1.0-275-gbd4d610-dirty","duk command built from Duktape repo",1]}
    --> {"request":"Eval", "args":[ "print(Math.PI)" ]}
    <-- {"notify":"Print","command":2,"args":["3.141592653589793\n"]}
    <-- {"reply":true,"args":[0,{"type":"undefined"}]}
    --> {"request":"Resume"}
    <-- {"reply":true,"args":[]}
    <-- {"notify":"Status","command":1,"args":[0,"test-dev-mandel2-func.js","global",58,0]}
    <-- {"notify":"Status","command":1,"args":[0,"test-dev-mandel2-func.js","global",58,0]}
    <-- {"notify":"Print","command":2,"args":["................................................................................\n"]}
    <-- {"notify":"Print","command":2,"args":["................................................................................\n"]}
    <-- {"notify":"Print","command":2,"args":["................................................................................\n"]}
    [...]
    <-- {"notify":"_Disconnecting"}

A telnet connection allows you to experiment with debug commands by simply
copy-pasting debug commands to the telnet session.  This is useful even if
you decide to implement the binary protocol directly.

The debug target used by the proxy can be configured with ``duk_debug.js``
command line options.

Source search path
==================

The NodeJS debug client needs to be able to find source code files matching
code running on the target ("duk" command line).  **The filenames used on the
target and on the debug client must match exactly**, because e.g. breakpoints
are targeted based on the 'fileName' property of Function objects.

The search path can be set using the ``--source-dirs`` option given to
``duk_debug.js``, with the default search paths including only
``../tests/ecmascript/``.

The default search path means that if a function on the target has fileName
``foo/bar.js`` it would be loaded from (relative to the duk_debug.js working
directory, ``debugger/``)::

    ../tests/ecmascript/foo/bar.js

Similarly, if the filesystem contained::

    ../tests/ecmascript/baz/quux.js

the web UI dropdown would show ``baz/quux.js``.  If you selected that file
and added a breakpoint, the breakpoint fileName sent to the debug target
would be ``baz/quux.js``.

.. note:: There's much to improve in the search path.  For instance, it'd
          be nice to add a certain path to search but exclude files based
          on paths and patterns, etc.

Architecture
============

::

    +-------------------+
    | Web browser       |  [debug UI]
    +-------------------+
          |
          | http (port 9092)
          | socket.io
          v
    +-------------------+
    | duk_debug.js      |  [debug client]
    +-------------------+
          |          /\
          |          ||
          +----------||---- [example tcp transport] (port 9091)
          |          ||     (application provides concrete transport)
          |          ||
          |          ||---- [debug protocol stream]
          |          ||     (between debug client and Duktape)
          |          ||
    + - - | - - - - -|| - - +
    :     v          ||     :
    :  +-------------||-+   :  [target]
    :  | application || |   :
    :  +-------------||-+   :
    :     ^          ||     :
    :     |          ||     :   [debug API]
    :     +----------||-------- debug transport callbacks
    :     |          ||     :   (read, write, peek, read/write flush)
    :     |          ||     :   implemented by application
    :     |          \/     :
    :  +----------------+   :
    :  | Duktape        |   :
    :  +----------------+   :
    + - - - - - - - - - - - +

The debug transport is application specific:

* Duktape command line ("duk") and this debug client use an **example** TCP
  transport as a concrete example.

* It is entirely up to the application to come up with the most suitable
  transport for its environment.  Different mechanisms will be needed for
  Wi-Fi, serial, etc.

The debug protocol running inside the transport is transport independent:

* The debug protocol is documented in ``doc/debugger.rst``.

* This debug client provides further concrete examples and clarifications
  on how the protocol can be used.

Using a custom transport
========================

Quite possibly your target device cannot use the example TCP transport and
you need to implement your own transport.  You'll need to implement your
custom transport both for the target device and for the debug client.

Target device
-------------

Implement the debug transport callbacks needed by ``duk_debugger_attach()``.

See ``doc/debugger.rst`` for details and ``examples/debug-trans-socket``
for example running code for a TCP transport.

Debug client alternative 1: duk_debug.js + custom TCP proxy
-----------------------------------------------------------

If you don't want to change ``duk_debug.js`` you can implement a TCP proxy
which accepts a TCP connection from ``duk_debug.js`` and then uses your
custom transport to talk to the target::

   +--------------+   TCP   +-------+   custom   +--------+
   | duk_debug.js | ------> | proxy | ---------> | target |
   +--------------+         +-------+            +--------+

This is a straightforward option and a proxy can be used with other debug
clients too (perhaps custom scripts talking to the target etc).

You could also use netcat and implement your proxy so that it talks to
``duk_debug.js`` using stdin/stdout.

Debug client alternative 2: duk_debug.js + custom NodeJS stream
---------------------------------------------------------------

To make ``duk_debug.js`` use a custom transport you need to:

* Implement your own transport as NodeJS stream.  You can add it directly to
  ``duk_debug.js`` but it's probably easiest to use a separate module so that
  the diff to ``duk_debug.js`` stays minimal.

* Change ``duk_debug.js`` to use the custom transport instead of a TCP
  stream.  Search for "CUSTOMTRANSPORT" in ``duk_debug.js``.

See:

* http://nodejs.org/api/stream.html

* https://github.com/substack/stream-handbook

Debug client alternative 3: custom debug client
-----------------------------------------------

You can also implement your own debug client and debug UI with support for
your custom transport.

You'll also need to implement the client part of the Duktape debugger
protocol.  See ``doc/debugger.rst`` for the specification and ``duk_debug.js``
for example running code which should illustrate the protocol in more detail.

The JSON debug proxy allows you to implement a debug client without needing
to implement the Duktape binary debug protocol.  The JSON protocol provides
a roughly 1:1 mapping to the binary protocol but with an easier syntax.
