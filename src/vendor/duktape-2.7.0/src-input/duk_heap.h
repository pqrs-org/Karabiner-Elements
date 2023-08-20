/*
 *  Heap structure.
 *
 *  Heap contains allocated heap objects, interned strings, and built-in
 *  strings for one or more threads.
 */

#if !defined(DUK_HEAP_H_INCLUDED)
#define DUK_HEAP_H_INCLUDED

/* alloc function typedefs in duktape.h */

/*
 *  Heap flags
 */

#define DUK_HEAP_FLAG_MARKANDSWEEP_RECLIMIT_REACHED \
	(1U << 0) /* mark-and-sweep marking reached a recursion limit and must use multi-pass marking */
#define DUK_HEAP_FLAG_INTERRUPT_RUNNING  (1U << 1) /* executor interrupt running (used to avoid nested interrupts) */
#define DUK_HEAP_FLAG_FINALIZER_NORESCUE (1U << 2) /* heap destruction ongoing, finalizer rescue no longer possible */
#define DUK_HEAP_FLAG_DEBUGGER_PAUSED    (1U << 3) /* debugger is paused: talk with debug client until step/resume */

#define DUK__HEAP_HAS_FLAGS(heap, bits) ((heap)->flags & (bits))
#define DUK__HEAP_SET_FLAGS(heap, bits) \
	do { \
		(heap)->flags |= (bits); \
	} while (0)
#define DUK__HEAP_CLEAR_FLAGS(heap, bits) \
	do { \
		(heap)->flags &= ~(bits); \
	} while (0)

#define DUK_HEAP_HAS_MARKANDSWEEP_RECLIMIT_REACHED(heap) DUK__HEAP_HAS_FLAGS((heap), DUK_HEAP_FLAG_MARKANDSWEEP_RECLIMIT_REACHED)
#define DUK_HEAP_HAS_INTERRUPT_RUNNING(heap)             DUK__HEAP_HAS_FLAGS((heap), DUK_HEAP_FLAG_INTERRUPT_RUNNING)
#define DUK_HEAP_HAS_FINALIZER_NORESCUE(heap)            DUK__HEAP_HAS_FLAGS((heap), DUK_HEAP_FLAG_FINALIZER_NORESCUE)
#define DUK_HEAP_HAS_DEBUGGER_PAUSED(heap)               DUK__HEAP_HAS_FLAGS((heap), DUK_HEAP_FLAG_DEBUGGER_PAUSED)

#define DUK_HEAP_SET_MARKANDSWEEP_RECLIMIT_REACHED(heap) DUK__HEAP_SET_FLAGS((heap), DUK_HEAP_FLAG_MARKANDSWEEP_RECLIMIT_REACHED)
#define DUK_HEAP_SET_INTERRUPT_RUNNING(heap)             DUK__HEAP_SET_FLAGS((heap), DUK_HEAP_FLAG_INTERRUPT_RUNNING)
#define DUK_HEAP_SET_FINALIZER_NORESCUE(heap)            DUK__HEAP_SET_FLAGS((heap), DUK_HEAP_FLAG_FINALIZER_NORESCUE)
#define DUK_HEAP_SET_DEBUGGER_PAUSED(heap)               DUK__HEAP_SET_FLAGS((heap), DUK_HEAP_FLAG_DEBUGGER_PAUSED)

#define DUK_HEAP_CLEAR_MARKANDSWEEP_RECLIMIT_REACHED(heap) \
	DUK__HEAP_CLEAR_FLAGS((heap), DUK_HEAP_FLAG_MARKANDSWEEP_RECLIMIT_REACHED)
#define DUK_HEAP_CLEAR_INTERRUPT_RUNNING(heap)  DUK__HEAP_CLEAR_FLAGS((heap), DUK_HEAP_FLAG_INTERRUPT_RUNNING)
#define DUK_HEAP_CLEAR_FINALIZER_NORESCUE(heap) DUK__HEAP_CLEAR_FLAGS((heap), DUK_HEAP_FLAG_FINALIZER_NORESCUE)
#define DUK_HEAP_CLEAR_DEBUGGER_PAUSED(heap)    DUK__HEAP_CLEAR_FLAGS((heap), DUK_HEAP_FLAG_DEBUGGER_PAUSED)

/*
 *  Longjmp types, also double as identifying continuation type for a rethrow (in 'finally')
 */

#define DUK_LJ_TYPE_UNKNOWN  0 /* unused */
#define DUK_LJ_TYPE_THROW    1 /* value1 -> error object */
#define DUK_LJ_TYPE_YIELD    2 /* value1 -> yield value, iserror -> error / normal */
#define DUK_LJ_TYPE_RESUME   3 /* value1 -> resume value, value2 -> resumee thread, iserror -> error/normal */
#define DUK_LJ_TYPE_BREAK    4 /* value1 -> label number, pseudo-type to indicate a break continuation (for ENDFIN) */
#define DUK_LJ_TYPE_CONTINUE 5 /* value1 -> label number, pseudo-type to indicate a continue continuation (for ENDFIN) */
#define DUK_LJ_TYPE_RETURN   6 /* value1 -> return value, pseudo-type to indicate a return continuation (for ENDFIN) */
#define DUK_LJ_TYPE_NORMAL   7 /* no value, pseudo-type to indicate a normal continuation (for ENDFIN) */

/*
 *  Mark-and-sweep flags
 *
 *  These are separate from heap level flags now but could be merged.
 *  The heap structure only contains a 'base mark-and-sweep flags'
 *  field and the GC caller can impose further flags.
 */

/* Emergency mark-and-sweep: try extra hard, even at the cost of
 * performance.
 */
#define DUK_MS_FLAG_EMERGENCY (1U << 0)

/* Postpone rescue decisions for reachable objects with FINALIZED set.
 * Used during finalize_list processing to avoid incorrect rescue
 * decisions due to finalize_list being a reachability root.
 */
#define DUK_MS_FLAG_POSTPONE_RESCUE (1U << 1)

/* Don't compact objects; needed during object property table resize
 * to prevent a recursive resize.  It would suffice to protect only the
 * current object being resized, but this is not yet implemented.
 */
#define DUK_MS_FLAG_NO_OBJECT_COMPACTION (1U << 2)

/*
 *  Thread switching
 *
 *  To switch heap->curr_thread, use the macro below so that interrupt counters
 *  get updated correctly.  The macro allows a NULL target thread because that
 *  happens e.g. in call handling.
 */

#if defined(DUK_USE_INTERRUPT_COUNTER)
#define DUK_HEAP_SWITCH_THREAD(heap, newthr) duk_heap_switch_thread((heap), (newthr))
#else
#define DUK_HEAP_SWITCH_THREAD(heap, newthr) \
	do { \
		(heap)->curr_thread = (newthr); \
	} while (0)
#endif

/*
 *  Stats
 */

#if defined(DUK_USE_DEBUG)
#define DUK_STATS_INC(heap, fieldname) \
	do { \
		(heap)->fieldname += 1; \
	} while (0)
#else
#define DUK_STATS_INC(heap, fieldname) \
	do { \
	} while (0)
#endif

/*
 *  Other heap related defines
 */

/* Mark-and-sweep interval is relative to combined count of objects and
 * strings kept in the heap during the latest mark-and-sweep pass.
 * Fixed point .8 multiplier and .0 adder.  Trigger count (interval) is
 * decreased by each (re)allocation attempt (regardless of size), and each
 * refzero processed object.
 *
 * 'SKIP' indicates how many (re)allocations to wait until a retry if
 * GC is skipped because there is no thread do it with yet (happens
 * only during init phases).
 */
#if defined(DUK_USE_REFERENCE_COUNTING)
#define DUK_HEAP_MARK_AND_SWEEP_TRIGGER_MULT 12800L /* 50x heap size */
#define DUK_HEAP_MARK_AND_SWEEP_TRIGGER_ADD  1024L
#define DUK_HEAP_MARK_AND_SWEEP_TRIGGER_SKIP 256L
#else
#define DUK_HEAP_MARK_AND_SWEEP_TRIGGER_MULT 256L /* 1x heap size */
#define DUK_HEAP_MARK_AND_SWEEP_TRIGGER_ADD  1024L
#define DUK_HEAP_MARK_AND_SWEEP_TRIGGER_SKIP 256L
#endif

/* GC torture. */
#if defined(DUK_USE_GC_TORTURE)
#define DUK_GC_TORTURE(heap) \
	do { \
		duk_heap_mark_and_sweep((heap), 0); \
	} while (0)
#else
#define DUK_GC_TORTURE(heap) \
	do { \
	} while (0)
#endif

/* Stringcache is used for speeding up char-offset-to-byte-offset
 * translations for non-ASCII strings.
 */
#define DUK_HEAP_STRCACHE_SIZE             4
#define DUK_HEAP_STRINGCACHE_NOCACHE_LIMIT 16 /* strings up to the this length are not cached */

/* Some list management macros. */
#define DUK_HEAP_INSERT_INTO_HEAP_ALLOCATED(heap, hdr) duk_heap_insert_into_heap_allocated((heap), (hdr))
#if defined(DUK_USE_REFERENCE_COUNTING)
#define DUK_HEAP_REMOVE_FROM_HEAP_ALLOCATED(heap, hdr) duk_heap_remove_from_heap_allocated((heap), (hdr))
#endif
#if defined(DUK_USE_FINALIZER_SUPPORT)
#define DUK_HEAP_INSERT_INTO_FINALIZE_LIST(heap, hdr) duk_heap_insert_into_finalize_list((heap), (hdr))
#define DUK_HEAP_REMOVE_FROM_FINALIZE_LIST(heap, hdr) duk_heap_remove_from_finalize_list((heap), (hdr))
#endif

/*
 *  Built-in strings
 */

/* heap string indices are autogenerated in duk_strings.h */
#if defined(DUK_USE_ROM_STRINGS)
#define DUK_HEAP_GET_STRING(heap, idx) ((duk_hstring *) DUK_LOSE_CONST(duk_rom_strings_stridx[(idx)]))
#else /* DUK_USE_ROM_STRINGS */
#if defined(DUK_USE_HEAPPTR16)
#define DUK_HEAP_GET_STRING(heap, idx) ((duk_hstring *) DUK_USE_HEAPPTR_DEC16((heap)->heap_udata, (heap)->strs16[(idx)]))
#else
#define DUK_HEAP_GET_STRING(heap, idx) ((heap)->strs[(idx)])
#endif
#endif /* DUK_USE_ROM_STRINGS */

/*
 *  Raw memory calls: relative to heap, but no GC interaction
 */

#define DUK_ALLOC_RAW(heap, size) ((heap)->alloc_func((heap)->heap_udata, (size)))

#define DUK_REALLOC_RAW(heap, ptr, newsize) ((heap)->realloc_func((heap)->heap_udata, (void *) (ptr), (newsize)))

#define DUK_FREE_RAW(heap, ptr) ((heap)->free_func((heap)->heap_udata, (void *) (ptr)))

/*
 *  Memory calls: relative to heap, GC interaction, but no error throwing.
 *
 *  XXX: Currently a mark-and-sweep triggered by memory allocation will run
 *  using the heap->heap_thread.  This thread is also used for running
 *  mark-and-sweep finalization; this is not ideal because it breaks the
 *  isolation between multiple global environments.
 *
 *  Notes:
 *
 *    - DUK_FREE() is required to ignore NULL and any other possible return
 *      value of a zero-sized alloc/realloc (same as ANSI C free()).
 *
 *    - There is no DUK_REALLOC_ZEROED because we don't assume to know the
 *      old size.  Caller must zero the reallocated memory.
 *
 *    - DUK_REALLOC_INDIRECT() must be used when a mark-and-sweep triggered
 *      by an allocation failure might invalidate the original 'ptr', thus
 *      causing a realloc retry to use an invalid pointer.  Example: we're
 *      reallocating the value stack and a finalizer resizes the same value
 *      stack during mark-and-sweep.  The indirect variant requests for the
 *      current location of the pointer being reallocated using a callback
 *      right before every realloc attempt; this circuitous approach is used
 *      to avoid strict aliasing issues in a more straightforward indirect
 *      pointer (void **) approach.  Note: the pointer in the storage
 *      location is read but is NOT updated; the caller must do that.
 */

/* callback for indirect reallocs, request for current pointer */
typedef void *(*duk_mem_getptr)(duk_heap *heap, void *ud);

#define DUK_ALLOC(heap, size)                       duk_heap_mem_alloc((heap), (size))
#define DUK_ALLOC_ZEROED(heap, size)                duk_heap_mem_alloc_zeroed((heap), (size))
#define DUK_REALLOC(heap, ptr, newsize)             duk_heap_mem_realloc((heap), (ptr), (newsize))
#define DUK_REALLOC_INDIRECT(heap, cb, ud, newsize) duk_heap_mem_realloc_indirect((heap), (cb), (ud), (newsize))
#define DUK_FREE(heap, ptr)                         duk_heap_mem_free((heap), (ptr))

/*
 *  Checked allocation, relative to a thread
 *
 *  DUK_FREE_CHECKED() doesn't actually throw, but accepts a 'thr' argument
 *  for convenience.
 */

#define DUK_ALLOC_CHECKED(thr, size)        duk_heap_mem_alloc_checked((thr), (size))
#define DUK_ALLOC_CHECKED_ZEROED(thr, size) duk_heap_mem_alloc_checked_zeroed((thr), (size))
#define DUK_FREE_CHECKED(thr, ptr)          duk_heap_mem_free((thr)->heap, (ptr))

/*
 *  Memory constants
 */

#define DUK_HEAP_ALLOC_FAIL_MARKANDSWEEP_LIMIT \
	10 /* Retry allocation after mark-and-sweep for this \
	    * many times.  A single mark-and-sweep round is \
	    * not guaranteed to free all unreferenced memory \
	    * because of finalization (in fact, ANY number of \
	    * rounds is strictly not enough). \
	    */

#define DUK_HEAP_ALLOC_FAIL_MARKANDSWEEP_EMERGENCY_LIMIT \
	3 /* Starting from this round, use emergency mode \
	   * for mark-and-sweep. \
	   */

/*
 *  Debugger support
 */

/* Maximum number of breakpoints.  Only breakpoints that are set are
 * consulted so increasing this has no performance impact.
 */
#define DUK_HEAP_MAX_BREAKPOINTS 16

/* Opcode interval for a Date-based status/peek rate limit check.  Only
 * relevant when debugger is attached.  Requesting a timestamp may be a
 * slow operation on some platforms so this shouldn't be too low.  On the
 * other hand a high value makes Duktape react to a pause request slowly.
 */
#define DUK_HEAP_DBG_RATELIMIT_OPCODES 4000

/* Milliseconds between status notify and transport peeks. */
#define DUK_HEAP_DBG_RATELIMIT_MILLISECS 200

/* Debugger pause flags. */
#define DUK_PAUSE_FLAG_ONE_OPCODE        (1U << 0) /* pause when a single opcode has been executed */
#define DUK_PAUSE_FLAG_ONE_OPCODE_ACTIVE (1U << 1) /* one opcode pause actually active; artifact of current implementation */
#define DUK_PAUSE_FLAG_LINE_CHANGE       (1U << 2) /* pause when current line number changes */
#define DUK_PAUSE_FLAG_FUNC_ENTRY        (1U << 3) /* pause when entering a function */
#define DUK_PAUSE_FLAG_FUNC_EXIT         (1U << 4) /* pause when exiting current function */
#define DUK_PAUSE_FLAG_CAUGHT_ERROR      (1U << 5) /* pause when about to throw an error that is caught */
#define DUK_PAUSE_FLAG_UNCAUGHT_ERROR    (1U << 6) /* pause when about to throw an error that won't be caught */

struct duk_breakpoint {
	duk_hstring *filename;
	duk_uint32_t line;
};

/*
 *  String cache should ideally be at duk_hthread level, but that would
 *  cause string finalization to slow down relative to the number of
 *  threads; string finalization must check the string cache for "weak"
 *  references to the string being finalized to avoid dead pointers.
 *
 *  Thus, string caches are now at the heap level now.
 */

struct duk_strcache_entry {
	duk_hstring *h;
	duk_uint32_t bidx;
	duk_uint32_t cidx;
};

/*
 *  Longjmp state, contains the information needed to perform a longjmp.
 *  Longjmp related values are written to value1, value2, and iserror.
 */

struct duk_ljstate {
	duk_jmpbuf *jmpbuf_ptr; /* current setjmp() catchpoint */
	duk_small_uint_t type; /* longjmp type */
	duk_bool_t iserror; /* isError flag for yield */
	duk_tval value1; /* 1st related value (type specific) */
	duk_tval value2; /* 2nd related value (type specific) */
};

#define DUK_ASSERT_LJSTATE_UNSET(heap) \
	do { \
		DUK_ASSERT(heap != NULL); \
		DUK_ASSERT(heap->lj.type == DUK_LJ_TYPE_UNKNOWN); \
		DUK_ASSERT(heap->lj.iserror == 0); \
		DUK_ASSERT(DUK_TVAL_IS_UNDEFINED(&heap->lj.value1)); \
		DUK_ASSERT(DUK_TVAL_IS_UNDEFINED(&heap->lj.value2)); \
	} while (0)
#define DUK_ASSERT_LJSTATE_SET(heap) \
	do { \
		DUK_ASSERT(heap != NULL); \
		DUK_ASSERT(heap->lj.type != DUK_LJ_TYPE_UNKNOWN); \
	} while (0)

/*
 *  Literal intern cache
 */

struct duk_litcache_entry {
	const duk_uint8_t *addr;
	duk_hstring *h;
};

/*
 *  Main heap structure
 */

#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL_DECL void duk_heap_assert_valid(duk_heap *heap);
#define DUK_HEAP_ASSERT_VALID(heap) \
	do { \
		duk_heap_assert_valid((heap)); \
	} while (0)
#else
#define DUK_HEAP_ASSERT_VALID(heap) \
	do { \
	} while (0)
#endif

struct duk_heap {
	duk_small_uint_t flags;

	/* Allocator functions. */
	duk_alloc_function alloc_func;
	duk_realloc_function realloc_func;
	duk_free_function free_func;

	/* Heap udata, used for allocator functions but also for other heap
	 * level callbacks like fatal function, pointer compression, etc.
	 */
	void *heap_udata;

	/* Fatal error handling, called e.g. when a longjmp() is needed but
	 * lj.jmpbuf_ptr is NULL.  fatal_func must never return; it's not
	 * declared as "noreturn" because doing that for typedefs is a bit
	 * challenging portability-wise.
	 */
	duk_fatal_function fatal_func;

	/* Main list of allocated heap objects.  Objects are either here,
	 * in finalize_list waiting for processing, or in refzero_list
	 * temporarily while a DECREF refzero cascade finishes.
	 */
	duk_heaphdr *heap_allocated;

	/* Temporary work list for freeing a cascade of objects when a DECREF
	 * (or DECREF_NORZ) encounters a zero refcount.  Using a work list
	 * allows fixed C stack size when refcounts go to zero for a chain of
	 * objects.  Outside of DECREF this is always a NULL because DECREF is
	 * processed without side effects (only memory free calls).
	 */
#if defined(DUK_USE_REFERENCE_COUNTING)
	duk_heaphdr *refzero_list;
#endif

#if defined(DUK_USE_FINALIZER_SUPPORT)
	/* Work list for objects to be finalized. */
	duk_heaphdr *finalize_list;
#if defined(DUK_USE_ASSERTIONS)
	/* Object whose finalizer is executing right now (no nesting). */
	duk_heaphdr *currently_finalizing;
#endif
#endif

	/* Freelist for duk_activations and duk_catchers. */
#if defined(DUK_USE_CACHE_ACTIVATION)
	duk_activation *activation_free;
#endif
#if defined(DUK_USE_CACHE_CATCHER)
	duk_catcher *catcher_free;
#endif

	/* Voluntary mark-and-sweep trigger counter.  Intentionally signed
	 * because we continue decreasing the value when voluntary GC cannot
	 * run.
	 */
#if defined(DUK_USE_VOLUNTARY_GC)
	duk_int_t ms_trigger_counter;
#endif

	/* Mark-and-sweep recursion control: too deep recursion causes
	 * multi-pass processing to avoid growing C stack without bound.
	 */
	duk_uint_t ms_recursion_depth;

	/* Mark-and-sweep flags automatically active (used for critical sections). */
	duk_small_uint_t ms_base_flags;

	/* Mark-and-sweep running flag.  Prevents re-entry, and also causes
	 * refzero events to be ignored (= objects won't be queued to refzero_list).
	 *
	 * 0: mark-and-sweep not running
	 * 1: mark-and-sweep is running
	 * 2: heap destruction active or debugger active, prevent mark-and-sweep
	 *    and refzero processing (but mark-and-sweep not itself running)
	 */
	duk_uint_t ms_running;

	/* Mark-and-sweep prevent count, stacking.  Used to avoid M&S side
	 * effects (besides finalizers which are controlled separately) such
	 * as compacting the string table or object property tables.  This
	 * is also bumped when ms_running is set to prevent recursive re-entry.
	 * Can also be bumped when mark-and-sweep is not running.
	 */
	duk_uint_t ms_prevent_count;

	/* Finalizer processing prevent count, stacking.  Bumped when finalizers
	 * are processed to prevent recursive finalizer processing (first call site
	 * processing finalizers handles all finalizers until the list is empty).
	 * Can also be bumped explicitly to prevent finalizer execution.
	 */
	duk_uint_t pf_prevent_count;

	/* When processing finalize_list, don't actually run finalizers but
	 * queue finalizable objects back to heap_allocated as is.  This is
	 * used during heap destruction to deal with finalizers that keep
	 * on creating more finalizable garbage.
	 */
	duk_uint_t pf_skip_finalizers;

#if defined(DUK_USE_ASSERTIONS)
	/* Set when we're in a critical path where an error throw would cause
	 * e.g. sandboxing/protected call violations or state corruption.  This
	 * is just used for asserts.
	 */
	duk_bool_t error_not_allowed;
#endif

#if defined(DUK_USE_ASSERTIONS)
	/* Set when heap is still being initialized, helps with writing
	 * some assertions.
	 */
	duk_bool_t heap_initializing;
#endif

	/* Marker for detecting internal "double faults", errors thrown when
	 * we're trying to create an error object, see duk_error_throw.c.
	 */
	duk_bool_t creating_error;

	/* Marker for indicating we're calling a user error augmentation
	 * (errCreate/errThrow) function.  Errors created/thrown during
	 * such a call are not augmented.
	 */
#if defined(DUK_USE_AUGMENT_ERROR_THROW) || defined(DUK_USE_AUGMENT_ERROR_CREATE)
	duk_bool_t augmenting_error;
#endif

	/* Longjmp state. */
	duk_ljstate lj;

	/* Heap thread, used internally and for finalization. */
	duk_hthread *heap_thread;

	/* Current running thread. */
	duk_hthread *curr_thread;

	/* Heap level "stash" object (e.g., various reachability roots). */
	duk_hobject *heap_object;

	/* duk_handle_call / duk_handle_safe_call recursion depth limiting */
	duk_int_t call_recursion_depth;
	duk_int_t call_recursion_limit;

	/* Mix-in value for computing string hashes; should be reasonably unpredictable. */
	duk_uint32_t hash_seed;

	/* Random number state for duk_util_tinyrandom.c. */
#if !defined(DUK_USE_GET_RANDOM_DOUBLE)
#if defined(DUK_USE_PREFER_SIZE) || !defined(DUK_USE_64BIT_OPS)
	duk_uint32_t rnd_state; /* State for Shamir's three-op algorithm */
#else
	duk_uint64_t rnd_state[2]; /* State for xoroshiro128+ */
#endif
#endif

	/* Counter for unique local symbol creation. */
	/* XXX: When 64-bit types are available, it would be more efficient to
	 * use a duk_uint64_t at least for incrementing but maybe also for
	 * string formatting in the Symbol constructor.
	 */
	duk_uint32_t sym_counter[2];

	/* For manual debugging: instruction count based on executor and
	 * interrupt counter book-keeping.  Inspect debug logs to see how
	 * they match up.
	 */
#if defined(DUK_USE_INTERRUPT_COUNTER) && defined(DUK_USE_DEBUG)
	duk_int_t inst_count_exec;
	duk_int_t inst_count_interrupt;
#endif

	/* Debugger state. */
#if defined(DUK_USE_DEBUGGER_SUPPORT)
	/* Callbacks and udata; dbg_read_cb != NULL is used to indicate attached state. */
	duk_debug_read_function dbg_read_cb; /* required, NULL implies detached */
	duk_debug_write_function dbg_write_cb; /* required */
	duk_debug_peek_function dbg_peek_cb;
	duk_debug_read_flush_function dbg_read_flush_cb;
	duk_debug_write_flush_function dbg_write_flush_cb;
	duk_debug_request_function dbg_request_cb;
	duk_debug_detached_function dbg_detached_cb;
	void *dbg_udata;

	/* The following are only relevant when debugger is attached. */
	duk_bool_t dbg_processing; /* currently processing messages or breakpoints: don't enter message processing recursively (e.g.
	                              no breakpoints when processing debugger eval) */
	duk_bool_t dbg_state_dirty; /* resend state next time executor is about to run */
	duk_bool_t
	    dbg_force_restart; /* force executor restart to recheck breakpoints; used to handle function returns (see GH-303) */
	duk_bool_t dbg_detaching; /* debugger detaching; used to avoid calling detach handler recursively */
	duk_small_uint_t dbg_pause_flags; /* flags for automatic pause behavior */
	duk_activation *dbg_pause_act; /* activation related to pause behavior (pause on line change, function entry/exit) */
	duk_uint32_t dbg_pause_startline; /* starting line number for line change related pause behavior */
	duk_breakpoint dbg_breakpoints[DUK_HEAP_MAX_BREAKPOINTS]; /* breakpoints: [0,breakpoint_count[ gc reachable */
	duk_small_uint_t dbg_breakpoint_count;
	duk_breakpoint
	    *dbg_breakpoints_active[DUK_HEAP_MAX_BREAKPOINTS + 1]; /* currently active breakpoints: NULL term, borrowed pointers */
	/* XXX: make active breakpoints actual copies instead of pointers? */

	/* These are for rate limiting Status notifications and transport peeking. */
	duk_uint_t dbg_exec_counter; /* cumulative opcode execution count (overflows are OK) */
	duk_uint_t dbg_last_counter; /* value of dbg_exec_counter when we last did a Date-based check */
	duk_double_t dbg_last_time; /* time when status/peek was last done (Date-based rate limit) */

	/* Used to support single-byte stream lookahead. */
	duk_bool_t dbg_have_next_byte;
	duk_uint8_t dbg_next_byte;
#endif /* DUK_USE_DEBUGGER_SUPPORT */
#if defined(DUK_USE_ASSERTIONS)
	duk_bool_t dbg_calling_transport; /* transport call in progress, calling into Duktape forbidden */
#endif

	/* String intern table (weak refs). */
#if defined(DUK_USE_STRTAB_PTRCOMP)
	duk_uint16_t *strtable16;
#else
	duk_hstring **strtable;
#endif
	duk_uint32_t st_mask; /* mask for lookup, st_size - 1 */
	duk_uint32_t st_size; /* stringtable size */
#if (DUK_USE_STRTAB_MINSIZE != DUK_USE_STRTAB_MAXSIZE)
	duk_uint32_t st_count; /* string count for resize load factor checks */
#endif
	duk_bool_t st_resizing; /* string table is being resized; avoid recursive resize */

	/* String access cache (codepoint offset -> byte offset) for fast string
	 * character looping; 'weak' reference which needs special handling in GC.
	 */
	duk_strcache_entry strcache[DUK_HEAP_STRCACHE_SIZE];

#if defined(DUK_USE_LITCACHE_SIZE)
	/* Literal intern cache.  When enabled, strings interned as literals
	 * (e.g. duk_push_literal()) will be pinned and cached for the lifetime
	 * of the heap.
	 */
	duk_litcache_entry litcache[DUK_USE_LITCACHE_SIZE];
#endif

	/* Built-in strings. */
#if defined(DUK_USE_ROM_STRINGS)
	/* No field needed when strings are in ROM. */
#else
#if defined(DUK_USE_HEAPPTR16)
	duk_uint16_t strs16[DUK_HEAP_NUM_STRINGS];
#else
	duk_hstring *strs[DUK_HEAP_NUM_STRINGS];
#endif
#endif

	/* Stats. */
#if defined(DUK_USE_DEBUG)
	duk_int_t stats_exec_opcodes;
	duk_int_t stats_exec_interrupt;
	duk_int_t stats_exec_throw;
	duk_int_t stats_call_all;
	duk_int_t stats_call_tailcall;
	duk_int_t stats_call_ecmatoecma;
	duk_int_t stats_safecall_all;
	duk_int_t stats_safecall_nothrow;
	duk_int_t stats_safecall_throw;
	duk_int_t stats_ms_try_count;
	duk_int_t stats_ms_skip_count;
	duk_int_t stats_ms_emergency_count;
	duk_int_t stats_strtab_intern_hit;
	duk_int_t stats_strtab_intern_miss;
	duk_int_t stats_strtab_resize_check;
	duk_int_t stats_strtab_resize_grow;
	duk_int_t stats_strtab_resize_shrink;
	duk_int_t stats_strtab_litcache_hit;
	duk_int_t stats_strtab_litcache_miss;
	duk_int_t stats_strtab_litcache_pin;
	duk_int_t stats_object_realloc_props;
	duk_int_t stats_object_abandon_array;
	duk_int_t stats_getownpropdesc_count;
	duk_int_t stats_getownpropdesc_hit;
	duk_int_t stats_getownpropdesc_miss;
	duk_int_t stats_getpropdesc_count;
	duk_int_t stats_getpropdesc_hit;
	duk_int_t stats_getpropdesc_miss;
	duk_int_t stats_getprop_all;
	duk_int_t stats_getprop_arrayidx;
	duk_int_t stats_getprop_bufobjidx;
	duk_int_t stats_getprop_bufferidx;
	duk_int_t stats_getprop_bufferlen;
	duk_int_t stats_getprop_stringidx;
	duk_int_t stats_getprop_stringlen;
	duk_int_t stats_getprop_proxy;
	duk_int_t stats_getprop_arguments;
	duk_int_t stats_putprop_all;
	duk_int_t stats_putprop_arrayidx;
	duk_int_t stats_putprop_bufobjidx;
	duk_int_t stats_putprop_bufferidx;
	duk_int_t stats_putprop_proxy;
	duk_int_t stats_getvar_all;
	duk_int_t stats_putvar_all;
	duk_int_t stats_envrec_delayedcreate;
	duk_int_t stats_envrec_create;
	duk_int_t stats_envrec_newenv;
	duk_int_t stats_envrec_oldenv;
	duk_int_t stats_envrec_pushclosure;
#endif
};

/*
 *  Prototypes
 */

DUK_INTERNAL_DECL
duk_heap *duk_heap_alloc(duk_alloc_function alloc_func,
                         duk_realloc_function realloc_func,
                         duk_free_function free_func,
                         void *heap_udata,
                         duk_fatal_function fatal_func);
DUK_INTERNAL_DECL void duk_heap_free(duk_heap *heap);
DUK_INTERNAL_DECL void duk_free_hobject(duk_heap *heap, duk_hobject *h);
DUK_INTERNAL_DECL void duk_free_hbuffer(duk_heap *heap, duk_hbuffer *h);
DUK_INTERNAL_DECL void duk_free_hstring(duk_heap *heap, duk_hstring *h);
DUK_INTERNAL_DECL void duk_heap_free_heaphdr_raw(duk_heap *heap, duk_heaphdr *hdr);

DUK_INTERNAL_DECL void duk_heap_insert_into_heap_allocated(duk_heap *heap, duk_heaphdr *hdr);
#if defined(DUK_USE_REFERENCE_COUNTING)
DUK_INTERNAL_DECL void duk_heap_remove_from_heap_allocated(duk_heap *heap, duk_heaphdr *hdr);
#endif
#if defined(DUK_USE_FINALIZER_SUPPORT)
DUK_INTERNAL_DECL void duk_heap_insert_into_finalize_list(duk_heap *heap, duk_heaphdr *hdr);
DUK_INTERNAL_DECL void duk_heap_remove_from_finalize_list(duk_heap *heap, duk_heaphdr *hdr);
#endif
#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL_DECL duk_bool_t duk_heap_in_heap_allocated(duk_heap *heap, duk_heaphdr *ptr);
#endif
#if defined(DUK_USE_INTERRUPT_COUNTER)
DUK_INTERNAL_DECL void duk_heap_switch_thread(duk_heap *heap, duk_hthread *new_thr);
#endif

DUK_INTERNAL_DECL duk_hstring *duk_heap_strtable_intern(duk_heap *heap, const duk_uint8_t *str, duk_uint32_t blen);
DUK_INTERNAL_DECL duk_hstring *duk_heap_strtable_intern_checked(duk_hthread *thr, const duk_uint8_t *str, duk_uint32_t len);
#if defined(DUK_USE_LITCACHE_SIZE)
DUK_INTERNAL_DECL duk_hstring *duk_heap_strtable_intern_literal_checked(duk_hthread *thr,
                                                                        const duk_uint8_t *str,
                                                                        duk_uint32_t blen);
#endif
DUK_INTERNAL_DECL duk_hstring *duk_heap_strtable_intern_u32(duk_heap *heap, duk_uint32_t val);
DUK_INTERNAL_DECL duk_hstring *duk_heap_strtable_intern_u32_checked(duk_hthread *thr, duk_uint32_t val);
#if defined(DUK_USE_REFERENCE_COUNTING)
DUK_INTERNAL_DECL void duk_heap_strtable_unlink(duk_heap *heap, duk_hstring *h);
#endif
DUK_INTERNAL_DECL void duk_heap_strtable_unlink_prev(duk_heap *heap, duk_hstring *h, duk_hstring *prev);
DUK_INTERNAL_DECL void duk_heap_strtable_force_resize(duk_heap *heap);
DUK_INTERNAL void duk_heap_strtable_free(duk_heap *heap);
#if defined(DUK_USE_DEBUG)
DUK_INTERNAL void duk_heap_strtable_dump(duk_heap *heap);
#endif

DUK_INTERNAL_DECL void duk_heap_strcache_string_remove(duk_heap *heap, duk_hstring *h);
DUK_INTERNAL_DECL duk_uint_fast32_t duk_heap_strcache_offset_char2byte(duk_hthread *thr,
                                                                       duk_hstring *h,
                                                                       duk_uint_fast32_t char_offset);

#if defined(DUK_USE_PROVIDE_DEFAULT_ALLOC_FUNCTIONS)
DUK_INTERNAL_DECL void *duk_default_alloc_function(void *udata, duk_size_t size);
DUK_INTERNAL_DECL void *duk_default_realloc_function(void *udata, void *ptr, duk_size_t newsize);
DUK_INTERNAL_DECL void duk_default_free_function(void *udata, void *ptr);
#endif

DUK_INTERNAL_DECL void *duk_heap_mem_alloc(duk_heap *heap, duk_size_t size);
DUK_INTERNAL_DECL void *duk_heap_mem_alloc_zeroed(duk_heap *heap, duk_size_t size);
DUK_INTERNAL_DECL void *duk_heap_mem_alloc_checked(duk_hthread *thr, duk_size_t size);
DUK_INTERNAL_DECL void *duk_heap_mem_alloc_checked_zeroed(duk_hthread *thr, duk_size_t size);
DUK_INTERNAL_DECL void *duk_heap_mem_realloc(duk_heap *heap, void *ptr, duk_size_t newsize);
DUK_INTERNAL_DECL void *duk_heap_mem_realloc_indirect(duk_heap *heap, duk_mem_getptr cb, void *ud, duk_size_t newsize);
DUK_INTERNAL_DECL void duk_heap_mem_free(duk_heap *heap, void *ptr);

DUK_INTERNAL_DECL void duk_heap_free_freelists(duk_heap *heap);

#if defined(DUK_USE_FINALIZER_SUPPORT)
DUK_INTERNAL_DECL void duk_heap_run_finalizer(duk_heap *heap, duk_hobject *obj);
DUK_INTERNAL_DECL void duk_heap_process_finalize_list(duk_heap *heap);
#endif /* DUK_USE_FINALIZER_SUPPORT */

DUK_INTERNAL_DECL void duk_heap_mark_and_sweep(duk_heap *heap, duk_small_uint_t flags);

DUK_INTERNAL_DECL duk_uint32_t duk_heap_hashstring(duk_heap *heap, const duk_uint8_t *str, duk_size_t len);

#endif /* DUK_HEAP_H_INCLUDED */
