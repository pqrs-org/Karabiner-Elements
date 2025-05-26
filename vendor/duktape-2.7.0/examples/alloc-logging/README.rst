======================
Allocator with logging
======================

Example allocator that writes all memory alloc/realloc/free calls into a
log file so that memory usage can replayed later.  This is useful to e.g.
optimize pool sizes.
