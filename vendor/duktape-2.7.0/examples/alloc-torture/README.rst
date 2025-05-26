==========================================
Allocator with memory wiping and red zones
==========================================

Example allocator that wipes memory on free and checks that no out-of-bounds
writes have been made to bytes just before and after the allocated area.

Valgrind is a better tool for detecting these memory issues, but it's not
available for all targets so you can use something like this to detect
memory lifecycle or out-of-bounds issues.
