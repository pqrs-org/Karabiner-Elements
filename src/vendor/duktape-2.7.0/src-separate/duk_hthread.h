/*
 *  Heap thread object representation.
 *
 *  duk_hthread is also the 'context' for public API functions via a
 *  different typedef.  Most API calls operate on the topmost frame
 *  of the value stack only.
 */

#if !defined(DUK_HTHREAD_H_INCLUDED)
#define DUK_HTHREAD_H_INCLUDED

/*
 *  Stack constants
 */

/* Initial valstack size, roughly 0.7kiB. */
#define DUK_VALSTACK_INITIAL_SIZE 96U

/* Internal extra elements assumed on function entry, always added to
 * user-defined 'extra' for e.g. the duk_check_stack() call.
 */
#define DUK_VALSTACK_INTERNAL_EXTRA 32U

/* Number of elements guaranteed to be user accessible (in addition to call
 * arguments) on Duktape/C function entry.  This is the major public API
 * commitment.
 */
#define DUK_VALSTACK_API_ENTRY_MINIMUM DUK_API_ENTRY_STACK

/*
 *  Activation defines
 */

#define DUK_ACT_FLAG_STRICT            (1U << 0) /* function executes in strict mode */
#define DUK_ACT_FLAG_TAILCALLED        (1U << 1) /* activation has tail called one or more times */
#define DUK_ACT_FLAG_CONSTRUCT         (1U << 2) /* function executes as a constructor (called via "new") */
#define DUK_ACT_FLAG_PREVENT_YIELD     (1U << 3) /* activation prevents yield (native call or "new") */
#define DUK_ACT_FLAG_DIRECT_EVAL       (1U << 4) /* activation is a direct eval call */
#define DUK_ACT_FLAG_CONSTRUCT_PROXY   (1U << 5) /* activation is for Proxy 'construct' call, special return value handling */
#define DUK_ACT_FLAG_BREAKPOINT_ACTIVE (1U << 6) /* activation has active breakpoint(s) */

#define DUK_ACT_GET_FUNC(act) ((act)->func)

/*
 *  Flags for __FILE__ / __LINE__ registered into tracedata
 */

#define DUK_TB_FLAG_NOBLAME_FILELINE (1U << 0) /* don't report __FILE__ / __LINE__ as fileName/lineNumber */

/*
 *  Catcher defines
 */

/* XXX: remove catcher type entirely */

/* flags field: LLLLLLFT, L = label (24 bits), F = flags (4 bits), T = type (4 bits) */
#define DUK_CAT_TYPE_MASK   0x0000000fUL
#define DUK_CAT_TYPE_BITS   4
#define DUK_CAT_LABEL_MASK  0xffffff00UL
#define DUK_CAT_LABEL_BITS  24
#define DUK_CAT_LABEL_SHIFT 8

#define DUK_CAT_FLAG_CATCH_ENABLED         (1U << 4) /* catch part will catch */
#define DUK_CAT_FLAG_FINALLY_ENABLED       (1U << 5) /* finally part will catch */
#define DUK_CAT_FLAG_CATCH_BINDING_ENABLED (1U << 6) /* request to create catch binding */
#define DUK_CAT_FLAG_LEXENV_ACTIVE         (1U << 7) /* catch or with binding is currently active */

#define DUK_CAT_TYPE_UNKNOWN 0
#define DUK_CAT_TYPE_TCF     1
#define DUK_CAT_TYPE_LABEL   2

#define DUK_CAT_GET_TYPE(c)  ((c)->flags & DUK_CAT_TYPE_MASK)
#define DUK_CAT_GET_LABEL(c) (((c)->flags & DUK_CAT_LABEL_MASK) >> DUK_CAT_LABEL_SHIFT)

#define DUK_CAT_HAS_CATCH_ENABLED(c)         ((c)->flags & DUK_CAT_FLAG_CATCH_ENABLED)
#define DUK_CAT_HAS_FINALLY_ENABLED(c)       ((c)->flags & DUK_CAT_FLAG_FINALLY_ENABLED)
#define DUK_CAT_HAS_CATCH_BINDING_ENABLED(c) ((c)->flags & DUK_CAT_FLAG_CATCH_BINDING_ENABLED)
#define DUK_CAT_HAS_LEXENV_ACTIVE(c)         ((c)->flags & DUK_CAT_FLAG_LEXENV_ACTIVE)

#define DUK_CAT_SET_CATCH_ENABLED(c) \
	do { \
		(c)->flags |= DUK_CAT_FLAG_CATCH_ENABLED; \
	} while (0)
#define DUK_CAT_SET_FINALLY_ENABLED(c) \
	do { \
		(c)->flags |= DUK_CAT_FLAG_FINALLY_ENABLED; \
	} while (0)
#define DUK_CAT_SET_CATCH_BINDING_ENABLED(c) \
	do { \
		(c)->flags |= DUK_CAT_FLAG_CATCH_BINDING_ENABLED; \
	} while (0)
#define DUK_CAT_SET_LEXENV_ACTIVE(c) \
	do { \
		(c)->flags |= DUK_CAT_FLAG_LEXENV_ACTIVE; \
	} while (0)

#define DUK_CAT_CLEAR_CATCH_ENABLED(c) \
	do { \
		(c)->flags &= ~DUK_CAT_FLAG_CATCH_ENABLED; \
	} while (0)
#define DUK_CAT_CLEAR_FINALLY_ENABLED(c) \
	do { \
		(c)->flags &= ~DUK_CAT_FLAG_FINALLY_ENABLED; \
	} while (0)
#define DUK_CAT_CLEAR_CATCH_BINDING_ENABLED(c) \
	do { \
		(c)->flags &= ~DUK_CAT_FLAG_CATCH_BINDING_ENABLED; \
	} while (0)
#define DUK_CAT_CLEAR_LEXENV_ACTIVE(c) \
	do { \
		(c)->flags &= ~DUK_CAT_FLAG_LEXENV_ACTIVE; \
	} while (0)

/*
 *  Thread defines
 */

#if defined(DUK_USE_ROM_STRINGS)
#define DUK_HTHREAD_GET_STRING(thr, idx) ((duk_hstring *) DUK_LOSE_CONST(duk_rom_strings_stridx[(idx)]))
#else /* DUK_USE_ROM_STRINGS */
#if defined(DUK_USE_HEAPPTR16)
#define DUK_HTHREAD_GET_STRING(thr, idx) ((duk_hstring *) DUK_USE_HEAPPTR_DEC16((thr)->heap->heap_udata, (thr)->strs16[(idx)]))
#else
#define DUK_HTHREAD_GET_STRING(thr, idx) ((thr)->strs[(idx)])
#endif
#endif /* DUK_USE_ROM_STRINGS */

/* values for the state field */
#define DUK_HTHREAD_STATE_INACTIVE   1 /* thread not currently running */
#define DUK_HTHREAD_STATE_RUNNING    2 /* thread currently running (only one at a time) */
#define DUK_HTHREAD_STATE_RESUMED    3 /* thread resumed another thread (active but not running) */
#define DUK_HTHREAD_STATE_YIELDED    4 /* thread has yielded */
#define DUK_HTHREAD_STATE_TERMINATED 5 /* thread has terminated */

/* Executor interrupt default interval when nothing else requires a
 * smaller value.  The default interval must be small enough to allow
 * for reasonable execution timeout checking but large enough to keep
 * impact on execution performance low.
 */
#if defined(DUK_USE_INTERRUPT_COUNTER)
#define DUK_HTHREAD_INTCTR_DEFAULT (256L * 1024L)
#endif

/*
 *  Assert context is valid: non-NULL pointer, fields look sane.
 *
 *  This is used by public API call entrypoints to catch invalid 'ctx' pointers
 *  as early as possible; invalid 'ctx' pointers cause very odd and difficult to
 *  diagnose behavior so it's worth checking even when the check is not 100%.
 */

#if defined(DUK_USE_ASSERTIONS)
/* Assertions for internals. */
DUK_INTERNAL_DECL void duk_hthread_assert_valid(duk_hthread *thr);
#define DUK_HTHREAD_ASSERT_VALID(thr) \
	do { \
		duk_hthread_assert_valid((thr)); \
	} while (0)

/* Assertions for public API calls; a bit stronger. */
DUK_INTERNAL_DECL void duk_ctx_assert_valid(duk_hthread *thr);
#define DUK_CTX_ASSERT_VALID(thr) \
	do { \
		duk_ctx_assert_valid((thr)); \
	} while (0)
#else
#define DUK_HTHREAD_ASSERT_VALID(thr) \
	do { \
	} while (0)
#define DUK_CTX_ASSERT_VALID(thr) \
	do { \
	} while (0)
#endif

/* Assertions for API call entry specifically.  Checks 'ctx' but also may
 * check internal state (e.g. not in a debugger transport callback).
 */
#define DUK_ASSERT_API_ENTRY(thr) \
	do { \
		DUK_CTX_ASSERT_VALID((thr)); \
		DUK_ASSERT((thr)->heap != NULL); \
		DUK_ASSERT((thr)->heap->dbg_calling_transport == 0); \
	} while (0)

/*
 *  Assertion helpers.
 */

#define DUK_ASSERT_STRIDX_VALID(val) DUK_ASSERT((duk_uint_t) (val) < DUK_HEAP_NUM_STRINGS)

#define DUK_ASSERT_BIDX_VALID(val) DUK_ASSERT((duk_uint_t) (val) < DUK_NUM_BUILTINS)

/*
 *  Misc
 */

/* Fast access to 'this' binding.  Assumes there's a call in progress. */
#define DUK_HTHREAD_THIS_PTR(thr) \
	(DUK_ASSERT_EXPR((thr) != NULL), DUK_ASSERT_EXPR((thr)->valstack_bottom > (thr)->valstack), (thr)->valstack_bottom - 1)

/*
 *  Struct defines
 */

/* Fields are ordered for alignment/packing. */
struct duk_activation {
	duk_tval tv_func; /* borrowed: full duk_tval for function being executed; for lightfuncs */
	duk_hobject *func; /* borrowed: function being executed; for bound function calls, this is the final, real function, NULL
	                      for lightfuncs */
	duk_activation *parent; /* previous (parent) activation (or NULL if none) */
	duk_hobject *var_env; /* current variable environment (may be NULL if delayed) */
	duk_hobject *lex_env; /* current lexical environment (may be NULL if delayed) */
	duk_catcher *cat; /* current catcher (or NULL) */

#if defined(DUK_USE_NONSTD_FUNC_CALLER_PROPERTY)
	/* Previous value of 'func' caller, restored when unwound.  Only in use
	 * when 'func' is non-strict.
	 */
	duk_hobject *prev_caller;
#endif

	duk_instr_t *curr_pc; /* next instruction to execute (points to 'func' bytecode, stable pointer), NULL for native calls */

	/* bottom_byteoff and retval_byteoff are only used for book-keeping
	 * of ECMAScript-initiated calls, to allow returning to an ECMAScript
	 * function properly.
	 */

	/* Bottom of valstack for this activation, used to reset
	 * valstack_bottom on return; offset is absolute.  There's
	 * no need to track 'top' because native call handling deals
	 * with that using locals, and for ECMAScript returns 'nregs'
	 * indicates the necessary top.
	 */
	duk_size_t bottom_byteoff;

	/* Return value when returning to this activation (points to caller
	 * reg, not callee reg); offset is absolute (only set if activation is
	 * not topmost).
	 *
	 * Note: bottom_byteoff is always set, while retval_byteoff is only
	 * applicable for activations below the topmost one.  Currently
	 * retval_byteoff for the topmost activation is considered garbage
	 * (and it not initialized on entry or cleared on return; may contain
	 * previous or garbage values).
	 */
	duk_size_t retval_byteoff;

	/* Current 'this' binding is the value just below bottom.
	 * Previously, 'this' binding was handled with an index to the
	 * (calling) valstack.  This works for everything except tail
	 * calls, which must not "accumulate" valstack temps.
	 */

	/* Value stack reserve (valstack_end) byte offset to be restored
	 * when returning to this activation.  Only used by the bytecode
	 * executor.
	 */
	duk_size_t reserve_byteoff;

#if defined(DUK_USE_DEBUGGER_SUPPORT)
	duk_uint32_t prev_line; /* needed for stepping */
#endif

	duk_small_uint_t flags;
};

struct duk_catcher {
	duk_catcher *parent; /* previous (parent) catcher (or NULL if none) */
	duk_hstring *h_varname; /* borrowed reference to catch variable name (or NULL if none) */
	/* (reference is valid as long activation exists) */
	duk_instr_t *pc_base; /* resume execution from pc_base or pc_base+1 (points to 'func' bytecode, stable pointer) */
	duk_size_t idx_base; /* idx_base and idx_base+1 get completion value and type */
	duk_uint32_t flags; /* type and control flags, label number */
	/* XXX: could pack 'flags' and 'idx_base' to same value in practice,
	 * on 32-bit targets this would make duk_catcher 16 bytes.
	 */
};

struct duk_hthread {
	/* Shared object part */
	duk_hobject obj;

	/* Pointer to bytecode executor's 'curr_pc' variable.  Used to copy
	 * the current PC back into the topmost activation when activation
	 * state is about to change (or "syncing" is otherwise needed).  This
	 * is rather awkward but important for performance, see execution.rst.
	 */
	duk_instr_t **ptr_curr_pc;

	/* Backpointers. */
	duk_heap *heap;

	/* Current strictness flag: affects API calls. */
	duk_uint8_t strict;

	/* Thread state. */
	duk_uint8_t state;
	duk_uint8_t unused1;
	duk_uint8_t unused2;

	/* XXX: Valstack and callstack are currently assumed to have non-NULL
	 * pointers.  Relaxing this would not lead to big benefits (except
	 * perhaps for terminated threads).
	 */

	/* Value stack: these are expressed as pointers for faster stack
	 * manipulation.  [valstack,valstack_top[ is GC-reachable,
	 * [valstack_top,valstack_alloc_end[ is not GC-reachable but kept
	 * initialized as 'undefined'.  [valstack,valstack_end[ is the
	 * guaranteed/reserved space and the valstack cannot be resized to
	 * a smaller size.  [valstack_end,valstack_alloc_end[ is currently
	 * allocated slack that can be used to grow the current guaranteed
	 * space but may be shrunk away without notice.
	 *
	 *
	 * <----------------------- guaranteed --->
	 *                                        <---- slack --->
	 *               <--- frame --->
	 * .-------------+=============+----------+--------------.
	 * |xxxxxxxxxxxxx|yyyyyyyyyyyyy|uuuuuuuuuu|uuuuuuuuuuuuuu|
	 * `-------------+=============+----------+--------------'
	 *
	 * ^             ^             ^          ^              ^
	 * |             |             |          |              |
	 * valstack      bottom        top        end            alloc_end
	 *
	 *     xxx = arbitrary values, below current frame
	 *     yyy = arbitrary values, inside current frame
	 *     uuu = outside active value stack, initialized to 'undefined'
	 */
	duk_tval *valstack; /* start of valstack allocation */
	duk_tval *valstack_end; /* end of valstack reservation/guarantee (exclusive) */
	duk_tval *valstack_alloc_end; /* end of valstack allocation */
	duk_tval *valstack_bottom; /* bottom of current frame */
	duk_tval *valstack_top; /* top of current frame (exclusive) */

	/* Call stack, represented as a linked list starting from the current
	 * activation (or NULL if nothing is active).
	 */
	duk_activation *callstack_curr; /* current activation (or NULL if none) */
	duk_size_t callstack_top; /* number of activation records in callstack (0 if none) */
	duk_size_t callstack_preventcount; /* number of activation records in callstack preventing a yield */

	/* Yield/resume book-keeping. */
	duk_hthread *resumer; /* who resumed us (if any) */

	/* Current compiler state (if any), used for augmenting SyntaxErrors. */
	duk_compiler_ctx *compile_ctx;

#if defined(DUK_USE_INTERRUPT_COUNTER)
	/* Interrupt counter for triggering a slow path check for execution
	 * timeout, debugger interaction such as breakpoints, etc.  The value
	 * is valid for the current running thread, and both the init and
	 * counter values are copied whenever a thread switch occurs.  It's
	 * important for the counter to be conveniently accessible for the
	 * bytecode executor inner loop for performance reasons.
	 */
	duk_int_t interrupt_counter; /* countdown state */
	duk_int_t interrupt_init; /* start value for current countdown */
#endif

	/* Builtin-objects; may or may not be shared with other threads,
	 * threads existing in different "compartments" will have different
	 * built-ins.  Must be stored on a per-thread basis because there
	 * is no intermediate structure for a thread group / compartment.
	 * This takes quite a lot of space, currently 43x4 = 172 bytes on
	 * 32-bit platforms.
	 *
	 * In some cases the builtins array could be ROM based, but it's
	 * sometimes edited (e.g. for sandboxing) so it's better to keep
	 * this array in RAM.
	 */
	duk_hobject *builtins[DUK_NUM_BUILTINS];

	/* Convenience copies from heap/vm for faster access. */
#if defined(DUK_USE_ROM_STRINGS)
	/* No field needed when strings are in ROM. */
#else
#if defined(DUK_USE_HEAPPTR16)
	duk_uint16_t *strs16;
#else
	duk_hstring **strs;
#endif
#endif
};

/*
 *  Prototypes
 */

DUK_INTERNAL_DECL void duk_hthread_copy_builtin_objects(duk_hthread *thr_from, duk_hthread *thr_to);
DUK_INTERNAL_DECL void duk_hthread_create_builtin_objects(duk_hthread *thr);
DUK_INTERNAL_DECL duk_bool_t duk_hthread_init_stacks(duk_heap *heap, duk_hthread *thr);
DUK_INTERNAL_DECL void duk_hthread_terminate(duk_hthread *thr);

DUK_INTERNAL_DECL duk_activation *duk_hthread_activation_alloc(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_hthread_activation_free(duk_hthread *thr, duk_activation *act);
DUK_INTERNAL_DECL void duk_hthread_activation_unwind_norz(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_hthread_activation_unwind_reuse_norz(duk_hthread *thr);
DUK_INTERNAL_DECL duk_activation *duk_hthread_get_activation_for_level(duk_hthread *thr, duk_int_t level);

DUK_INTERNAL_DECL duk_catcher *duk_hthread_catcher_alloc(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_hthread_catcher_free(duk_hthread *thr, duk_catcher *cat);
DUK_INTERNAL_DECL void duk_hthread_catcher_unwind_norz(duk_hthread *thr, duk_activation *act);
DUK_INTERNAL_DECL void duk_hthread_catcher_unwind_nolexenv_norz(duk_hthread *thr, duk_activation *act);

#if defined(DUK_USE_FINALIZER_TORTURE)
DUK_INTERNAL_DECL void duk_hthread_valstack_torture_realloc(duk_hthread *thr);
#endif

DUK_INTERNAL_DECL void *duk_hthread_get_valstack_ptr(duk_heap *heap, void *ud); /* indirect allocs */

#if defined(DUK_USE_DEBUGGER_SUPPORT)
DUK_INTERNAL_DECL duk_uint_fast32_t duk_hthread_get_act_curr_pc(duk_hthread *thr, duk_activation *act);
#endif
DUK_INTERNAL_DECL duk_uint_fast32_t duk_hthread_get_act_prev_pc(duk_hthread *thr, duk_activation *act);
DUK_INTERNAL_DECL void duk_hthread_sync_currpc(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_hthread_sync_and_null_currpc(duk_hthread *thr);

#endif /* DUK_HTHREAD_H_INCLUDED */
