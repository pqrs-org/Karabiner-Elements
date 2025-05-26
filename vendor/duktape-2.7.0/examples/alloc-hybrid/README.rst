=====================
Hybrid pool allocator
=====================

Example allocator that tries to satisfy memory allocations for small sizes
from a set of fixed pools, but always falls back to malloc/realloc/free if
a larger size is requested or the pools have been exhausted.

This may be useful to reduce memory churn when the platform allocator does
not handle allocations for a lot of small memory areas efficiently.
