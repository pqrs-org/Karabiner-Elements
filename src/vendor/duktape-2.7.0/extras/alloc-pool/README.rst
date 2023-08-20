=====================================
Pool allocator for low memory targets
=====================================

A simple pool allocator which satisfies allocations from preallocated pools
containing blocks of a certain size.  The caller provides a continuous memory
region and a pool configuration when initializing the allocator.

The pool configuration specifies the block sizes used, and parameters to
control how many entries are allocated for each block size.  The parameters
are specified with respect to an arbitrary floating point scaling parameter
``t`` as follows::

    bytes = A*t + B
    count = floor(bytes / block_size)
          = floor((A*t + B) / block_size)

    A: constant which indicates how quickly more bytes are assigned for this
       block size as the total allocation grows

    B: constant which indicates the base allocation for this block size, i.e.
       the allocated needed by Duktape initialization

Pool initialization finds the largest floating point ``t`` which still fits in
the memory region provided.  Any leftover bytes are sprinkled to the pools to
minimize wasted space.

A pool configuration can be written manually (by trial and error) or using
some automatic tooling such as ``pool_simulator.py``.

When using pointer compression only a single global pool is supported.  This
reduces code footprint and is usually sufficient in low memory targets.

Pointer compression functions are defined as inline functions in
``duk_alloc_pool.h`` to allow the compiler to inline pointer compression when
appropriate.  As a side effect ``duk_config.h`` must include
``duk_alloc_pool.h`` so that the declarations are visible when compiling
Duktape.
