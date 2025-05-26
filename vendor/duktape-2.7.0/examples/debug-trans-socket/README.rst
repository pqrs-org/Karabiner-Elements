================================================
Debug transport using a simple socket connection
================================================

This example implements an example debug transport which uses a Linux or
Windows TCP server socket on the debug target.

Files:

* ``duk_trans_socket.h``: header file for the transport, used for both Linux
  and Windows socket variants.

* ``duk_trans_socket_unix.c``: implementation for Linux/Unix.

* ``duk_trans_socket_windows.c``: implementation for Windows.

Compile either Unix or Windows source file only.
