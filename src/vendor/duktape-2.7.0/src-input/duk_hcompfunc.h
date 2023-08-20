/*
 *  Heap compiled function (ECMAScript function) representation.
 *
 *  There is a single data buffer containing the ECMAScript function's
 *  bytecode, constants, and inner functions.
 */

#if !defined(DUK_HCOMPFUNC_H_INCLUDED)
#define DUK_HCOMPFUNC_H_INCLUDED

/*
 *  Field accessor macros
 */

/* XXX: casts could be improved, especially for GET/SET DATA */

#if defined(DUK_USE_HEAPPTR16)
#define DUK_HCOMPFUNC_GET_DATA(heap, h) ((duk_hbuffer_fixed *) (void *) DUK_USE_HEAPPTR_DEC16((heap)->heap_udata, (h)->data16))
#define DUK_HCOMPFUNC_SET_DATA(heap, h, v) \
	do { \
		(h)->data16 = DUK_USE_HEAPPTR_ENC16((heap)->heap_udata, (void *) (v)); \
	} while (0)
#define DUK_HCOMPFUNC_GET_FUNCS(heap, h) ((duk_hobject **) (void *) (DUK_USE_HEAPPTR_DEC16((heap)->heap_udata, (h)->funcs16)))
#define DUK_HCOMPFUNC_SET_FUNCS(heap, h, v) \
	do { \
		(h)->funcs16 = DUK_USE_HEAPPTR_ENC16((heap)->heap_udata, (void *) (v)); \
	} while (0)
#define DUK_HCOMPFUNC_GET_BYTECODE(heap, h) ((duk_instr_t *) (void *) (DUK_USE_HEAPPTR_DEC16((heap)->heap_udata, (h)->bytecode16)))
#define DUK_HCOMPFUNC_SET_BYTECODE(heap, h, v) \
	do { \
		(h)->bytecode16 = DUK_USE_HEAPPTR_ENC16((heap)->heap_udata, (void *) (v)); \
	} while (0)
#define DUK_HCOMPFUNC_GET_LEXENV(heap, h) ((duk_hobject *) (void *) (DUK_USE_HEAPPTR_DEC16((heap)->heap_udata, (h)->lex_env16)))
#define DUK_HCOMPFUNC_SET_LEXENV(heap, h, v) \
	do { \
		(h)->lex_env16 = DUK_USE_HEAPPTR_ENC16((heap)->heap_udata, (void *) (v)); \
	} while (0)
#define DUK_HCOMPFUNC_GET_VARENV(heap, h) ((duk_hobject *) (void *) (DUK_USE_HEAPPTR_DEC16((heap)->heap_udata, (h)->var_env16)))
#define DUK_HCOMPFUNC_SET_VARENV(heap, h, v) \
	do { \
		(h)->var_env16 = DUK_USE_HEAPPTR_ENC16((heap)->heap_udata, (void *) (v)); \
	} while (0)
#else
#define DUK_HCOMPFUNC_GET_DATA(heap, h) ((duk_hbuffer_fixed *) (void *) (h)->data)
#define DUK_HCOMPFUNC_SET_DATA(heap, h, v) \
	do { \
		(h)->data = (duk_hbuffer *) (v); \
	} while (0)
#define DUK_HCOMPFUNC_GET_FUNCS(heap, h) ((h)->funcs)
#define DUK_HCOMPFUNC_SET_FUNCS(heap, h, v) \
	do { \
		(h)->funcs = (v); \
	} while (0)
#define DUK_HCOMPFUNC_GET_BYTECODE(heap, h) ((h)->bytecode)
#define DUK_HCOMPFUNC_SET_BYTECODE(heap, h, v) \
	do { \
		(h)->bytecode = (v); \
	} while (0)
#define DUK_HCOMPFUNC_GET_LEXENV(heap, h) ((h)->lex_env)
#define DUK_HCOMPFUNC_SET_LEXENV(heap, h, v) \
	do { \
		(h)->lex_env = (v); \
	} while (0)
#define DUK_HCOMPFUNC_GET_VARENV(heap, h) ((h)->var_env)
#define DUK_HCOMPFUNC_SET_VARENV(heap, h, v) \
	do { \
		(h)->var_env = (v); \
	} while (0)
#endif

/*
 *  Accessor macros for function specific data areas
 */

/* Note: assumes 'data' is always a fixed buffer */
#define DUK_HCOMPFUNC_GET_BUFFER_BASE(heap, h) DUK_HBUFFER_FIXED_GET_DATA_PTR((heap), DUK_HCOMPFUNC_GET_DATA((heap), (h)))

#define DUK_HCOMPFUNC_GET_CONSTS_BASE(heap, h) ((duk_tval *) (void *) DUK_HCOMPFUNC_GET_BUFFER_BASE((heap), (h)))

#define DUK_HCOMPFUNC_GET_FUNCS_BASE(heap, h) DUK_HCOMPFUNC_GET_FUNCS((heap), (h))

#define DUK_HCOMPFUNC_GET_CODE_BASE(heap, h) DUK_HCOMPFUNC_GET_BYTECODE((heap), (h))

#define DUK_HCOMPFUNC_GET_CONSTS_END(heap, h) ((duk_tval *) (void *) DUK_HCOMPFUNC_GET_FUNCS((heap), (h)))

#define DUK_HCOMPFUNC_GET_FUNCS_END(heap, h) ((duk_hobject **) (void *) DUK_HCOMPFUNC_GET_BYTECODE((heap), (h)))

/* XXX: double evaluation of DUK_HCOMPFUNC_GET_DATA() */
#define DUK_HCOMPFUNC_GET_CODE_END(heap, h) \
	((duk_instr_t *) (void *) (DUK_HBUFFER_FIXED_GET_DATA_PTR((heap), DUK_HCOMPFUNC_GET_DATA((heap), (h))) + \
	                           DUK_HBUFFER_GET_SIZE((duk_hbuffer *) DUK_HCOMPFUNC_GET_DATA((heap), h))))

#define DUK_HCOMPFUNC_GET_CONSTS_SIZE(heap, h) \
	((duk_size_t) (((const duk_uint8_t *) DUK_HCOMPFUNC_GET_CONSTS_END((heap), (h))) - \
	               ((const duk_uint8_t *) DUK_HCOMPFUNC_GET_CONSTS_BASE((heap), (h)))))

#define DUK_HCOMPFUNC_GET_FUNCS_SIZE(heap, h) \
	((duk_size_t) (((const duk_uint8_t *) DUK_HCOMPFUNC_GET_FUNCS_END((heap), (h))) - \
	               ((const duk_uint8_t *) DUK_HCOMPFUNC_GET_FUNCS_BASE((heap), (h)))))

#define DUK_HCOMPFUNC_GET_CODE_SIZE(heap, h) \
	((duk_size_t) (((const duk_uint8_t *) DUK_HCOMPFUNC_GET_CODE_END((heap), (h))) - \
	               ((const duk_uint8_t *) DUK_HCOMPFUNC_GET_CODE_BASE((heap), (h)))))

#define DUK_HCOMPFUNC_GET_CONSTS_COUNT(heap, h) ((duk_size_t) (DUK_HCOMPFUNC_GET_CONSTS_SIZE((heap), (h)) / sizeof(duk_tval)))

#define DUK_HCOMPFUNC_GET_FUNCS_COUNT(heap, h) ((duk_size_t) (DUK_HCOMPFUNC_GET_FUNCS_SIZE((heap), (h)) / sizeof(duk_hobject *)))

#define DUK_HCOMPFUNC_GET_CODE_COUNT(heap, h) ((duk_size_t) (DUK_HCOMPFUNC_GET_CODE_SIZE((heap), (h)) / sizeof(duk_instr_t)))

/*
 *  Validity assert
 */

#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL_DECL void duk_hcompfunc_assert_valid(duk_hcompfunc *h);
#define DUK_HCOMPFUNC_ASSERT_VALID(h) \
	do { \
		duk_hcompfunc_assert_valid((h)); \
	} while (0)
#else
#define DUK_HCOMPFUNC_ASSERT_VALID(h) \
	do { \
	} while (0)
#endif

/*
 *  Main struct
 */

struct duk_hcompfunc {
	/* shared object part */
	duk_hobject obj;

	/*
	 *  Pointers to function data area for faster access.  Function
	 *  data is a buffer shared between all closures of the same
	 *  "template" function.  The data buffer is always fixed (non-
	 *  dynamic, hence stable), with a layout as follows:
	 *
	 *    constants (duk_tval)
	 *    inner functions (duk_hobject *)
	 *    bytecode (duk_instr_t)
	 *
	 *  Note: bytecode end address can be computed from 'data' buffer
	 *  size.  It is not strictly necessary functionally, assuming
	 *  bytecode never jumps outside its allocated area.  However,
	 *  it's a safety/robustness feature for avoiding the chance of
	 *  executing random data as bytecode due to a compiler error.
	 *
	 *  Note: values in the data buffer must be incref'd (they will
	 *  be decref'd on release) for every compiledfunction referring
	 *  to the 'data' element.
	 */

	/* Data area, fixed allocation, stable data ptrs. */
#if defined(DUK_USE_HEAPPTR16)
	duk_uint16_t data16;
#else
	duk_hbuffer *data;
#endif

	/* No need for constants pointer (= same as data).
	 *
	 * When using 16-bit packing alignment to 4 is nice.  'funcs' will be
	 * 4-byte aligned because 'constants' are duk_tvals.  For now the
	 * inner function pointers are not compressed, so that 'bytecode' will
	 * also be 4-byte aligned.
	 */
#if defined(DUK_USE_HEAPPTR16)
	duk_uint16_t funcs16;
	duk_uint16_t bytecode16;
#else
	duk_hobject **funcs;
	duk_instr_t *bytecode;
#endif

	/* Lexenv: lexical environment of closure, NULL for templates.
	 * Varenv: variable environment of closure, NULL for templates.
	 */
#if defined(DUK_USE_HEAPPTR16)
	duk_uint16_t lex_env16;
	duk_uint16_t var_env16;
#else
	duk_hobject *lex_env;
	duk_hobject *var_env;
#endif

	/*
	 *  'nregs' registers are allocated on function entry, at most 'nargs'
	 *  are initialized to arguments, and the rest to undefined.  Arguments
	 *  above 'nregs' are not mapped to registers.  All registers in the
	 *  active stack range must be initialized because they are GC reachable.
	 *  'nargs' is needed so that if the function is given more than 'nargs'
	 *  arguments, the additional arguments do not 'clobber' registers
	 *  beyond 'nregs' which must be consistently initialized to undefined.
	 *
	 *  Usually there is no need to know which registers are mapped to
	 *  local variables.  Registers may be allocated to variable in any
	 *  way (even including gaps).  However, a register-variable mapping
	 *  must be the same for the duration of the function execution and
	 *  the register cannot be used for anything else.
	 *
	 *  When looking up variables by name, the '_Varmap' map is used.
	 *  When an activation closes, registers mapped to arguments are
	 *  copied into the environment record based on the same map.  The
	 *  reverse map (from register to variable) is not currently needed
	 *  at run time, except for debugging, so it is not maintained.
	 */

	duk_uint16_t nregs; /* regs to allocate */
	duk_uint16_t nargs; /* number of arguments allocated to regs */

	/*
	 *  Additional control information is placed into the object itself
	 *  as internal properties to avoid unnecessary fields for the
	 *  majority of functions.  The compiler tries to omit internal
	 *  control fields when possible.
	 *
	 *  Function templates:
	 *
	 *    {
	 *      name: "func",    // declaration, named function expressions
	 *      fileName: <debug info for creating nice errors>
	 *      _Varmap: { "arg1": 0, "arg2": 1, "varname": 2 },
	 *      _Formals: [ "arg1", "arg2" ],
	 *      _Source: "function func(arg1, arg2) { ... }",
	 *      _Pc2line: <debug info for pc-to-line mapping>,
	 *    }
	 *
	 *  Function instances:
	 *
	 *    {
	 *      length: 2,
	 *      prototype: { constructor: <func> },
	 *      caller: <thrower>,
	 *      arguments: <thrower>,
	 *      name: "func",    // declaration, named function expressions
	 *      fileName: <debug info for creating nice errors>
	 *      _Varmap: { "arg1": 0, "arg2": 1, "varname": 2 },
	 *      _Formals: [ "arg1", "arg2" ],
	 *      _Source: "function func(arg1, arg2) { ... }",
	 *      _Pc2line: <debug info for pc-to-line mapping>,
	 *    }
	 *
	 *  More detailed description of these properties can be found
	 *  in the documentation.
	 */

#if defined(DUK_USE_DEBUGGER_SUPPORT)
	/* Line number range for function.  Needed during debugging to
	 * determine active breakpoints.
	 */
	duk_uint32_t start_line;
	duk_uint32_t end_line;
#endif
};

#endif /* DUK_HCOMPFUNC_H_INCLUDED */
