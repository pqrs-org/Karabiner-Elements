==================
Eventloop examples
==================

Overview and usage
==================

A few examples on how an event loop can be implemented with Duktape, mainly
illlustrating how the Duktape interface works (not how event loops should be
built otherwise).

To test (Linux only, perhaps other Unix)::

  $ ./evloop timer-test.js     # run with ECMAScript eventloop
  $ ./evloop -c timer-test.js  # run with C eventloop

Implementation approaches
=========================

There are several approaches to implementation timers.  Here we demonstrate
two main approaches:

1. Using a C eventloop which calls into ECMAScript.  All the event loop state
   like timers, sockets, etc, is held in C structures.
   (See ``c_eventloop.c`` and ``c_eventloop.js``.)

2. Using an ECMAScript eventloop which never returns.  All the event loop state
   can be managed with ECMAScript code instead of C structures.  The ECMAScript
   eventloop calls a Duktape/C helper to do the lowest level poll() call.
   (See ``ecma_eventloop.js``.)

Services provided
=================

The event loop API provided by both examples is the same, and includes:

* Timers: setTimeout, clearTimeout, setInterval, clearInterval

* Sockets: simple network sockets

In addition there are a few synchronous API bindings which are not event loop
related:

* File I/O

Limitations
===========

This is **not** a production quality event loop.  This is on purpose, to
keep the example somewhat simple.  Some shortcomings include:

* A production quality event loop would track its internal state (active
  timers and sockets) much more efficiently.  In general memory usage and
  code footprint can be reduced.

* Buffer churn caused by allocating a new buffer for every socket read
  should be eliminated by reusing buffers where appropriate.  Although
  churn doesn't increase memory footprint with reference counting, it
  is slower than reusing buffers and might increase memory fragmentation.

* There is no way to suspend reading or writing in the example.  Adding
  them is straightforward: the poll set needs to be managed dynamically.

* The example uses poll() while one should use epoll() on Linux, kqueue()
  on BSD systems, etc.

* Timers are not very accurate, e.g. setInterval() does not try to guarantee
  a steady schedule.  Instead, the next interval is scheduled after the
  current callback has finished.  This is not the best behavior for some
  environments, but avoids bunching callbacks.

* Error handling is mostly missing.
