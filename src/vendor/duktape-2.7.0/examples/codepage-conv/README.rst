Codepage conversion example
===========================

Example of how to convert an 8-bit input string (e.g. ISO-8859-1 or Windows
codepage 1252) into CESU-8 without using an external library like iconv.

This is useful e.g. when compiling non-UTF-8 source code which cannot be
converted to UTF-8 (CESU-8) at build time.
