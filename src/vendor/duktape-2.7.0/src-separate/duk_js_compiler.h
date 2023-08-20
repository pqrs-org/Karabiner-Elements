/*
 *  ECMAScript compiler.
 */

#if !defined(DUK_JS_COMPILER_H_INCLUDED)
#define DUK_JS_COMPILER_H_INCLUDED

/* ECMAScript compiler limits */
#define DUK_COMPILER_TOKEN_LIMIT 100000000L /* 1e8: protects against deeply nested inner functions */

/* maximum loopcount for peephole optimization */
#define DUK_COMPILER_PEEPHOLE_MAXITER 3

/* maximum bytecode length in instructions */
#define DUK_COMPILER_MAX_BYTECODE_LENGTH (256L * 1024L * 1024L) /* 1 GB */

/*
 *  Compiler intermediate values
 *
 *  Intermediate values describe either plain values (e.g. strings or
 *  numbers) or binary operations which have not yet been coerced into
 *  either a left-hand-side or right-hand-side role (e.g. object property).
 */

#define DUK_IVAL_NONE  0 /* no value */
#define DUK_IVAL_PLAIN 1 /* register, constant, or value */
#define DUK_IVAL_ARITH 2 /* binary arithmetic; DUK_OP_ADD, DUK_OP_EQ, other binary ops */
#define DUK_IVAL_PROP  3 /* property access */
#define DUK_IVAL_VAR   4 /* variable access */

#define DUK_ISPEC_NONE     0 /* no value */
#define DUK_ISPEC_VALUE    1 /* value resides in 'valstack_idx' */
#define DUK_ISPEC_REGCONST 2 /* value resides in a register or constant */

/* Bit mask which indicates that a regconst is a constant instead of a register.
 * Chosen so that when a regconst is cast to duk_int32_t, all consts are
 * negative values.
 */
#define DUK_REGCONST_CONST_MARKER DUK_INT32_MIN /* = -0x80000000 */

/* Type to represent a reg/const reference during compilation, with <0
 * indicating a constant.  Some call sites also use -1 to indicate 'none'.
 */
typedef duk_int32_t duk_regconst_t;

typedef struct {
	duk_small_uint_t t; /* DUK_ISPEC_XXX */
	duk_regconst_t regconst;
	duk_idx_t valstack_idx; /* always set; points to a reserved valstack slot */
} duk_ispec;

typedef struct {
	/*
	 *  PLAIN: x1
	 *  ARITH: x1 <op> x2
	 *  PROP: x1.x2
	 *  VAR: x1 (name)
	 */

	/* XXX: can be optimized for smaller footprint esp. on 32-bit environments */
	duk_small_uint_t t; /* DUK_IVAL_XXX */
	duk_small_uint_t op; /* bytecode opcode for binary ops */
	duk_ispec x1;
	duk_ispec x2;
} duk_ivalue;

/*
 *  Bytecode instruction representation during compilation
 *
 *  Contains the actual instruction and (optionally) debug info.
 */

struct duk_compiler_instr {
	duk_instr_t ins;
#if defined(DUK_USE_PC2LINE)
	duk_uint32_t line;
#endif
};

/*
 *  Compiler state
 */

#define DUK_LABEL_FLAG_ALLOW_BREAK    (1U << 0)
#define DUK_LABEL_FLAG_ALLOW_CONTINUE (1U << 1)

#define DUK_DECL_TYPE_VAR  0
#define DUK_DECL_TYPE_FUNC 1

/* XXX: optimize to 16 bytes */
typedef struct {
	duk_small_uint_t flags;
	duk_int_t label_id; /* numeric label_id (-1 reserved as marker) */
	duk_hstring *h_label; /* borrowed label name */
	duk_int_t catch_depth; /* catch depth at point of definition */
	duk_int_t pc_label; /* pc of label statement:
	                     * pc+1: break jump site
	                     * pc+2: continue jump site
	                     */

	/* Fast jumps (which avoid longjmp) jump directly to the jump sites
	 * which are always known even while the iteration/switch statement
	 * is still being parsed.  A final peephole pass "straightens out"
	 * the jumps.
	 */
} duk_labelinfo;

/* Compiling state of one function, eventually converted to duk_hcompfunc */
struct duk_compiler_func {
	/* These pointers are at the start of the struct so that they pack
	 * nicely.  Mixing pointers and integer values is bad on some
	 * platforms (e.g. if int is 32 bits and pointers are 64 bits).
	 */

	duk_bufwriter_ctx bw_code; /* bufwriter for code */

	duk_hstring *h_name; /* function name (borrowed reference), ends up in _name */
	/* h_code: held in bw_code */
	duk_hobject *h_consts; /* array */
	duk_hobject *h_funcs; /* array of function templates: [func1, offset1, line1, func2, offset2, line2]
	                       * offset/line points to closing brace to allow skipping on pass 2
	                       */
	duk_hobject *h_decls; /* array of declarations: [ name1, val1, name2, val2, ... ]
	                       * valN = (typeN) | (fnum << 8), where fnum is inner func number (0 for vars)
	                       * record function and variable declarations in pass 1
	                       */
	duk_hobject *h_labelnames; /* array of active label names */
	duk_hbuffer_dynamic *h_labelinfos; /* C array of duk_labelinfo */
	duk_hobject *h_argnames; /* array of formal argument names (-> _Formals) */
	duk_hobject *h_varmap; /* variable map for pass 2 (identifier -> register number or null (unmapped)) */

	/* Value stack indices for tracking objects. */
	/* code_idx: not needed */
	duk_idx_t consts_idx;
	duk_idx_t funcs_idx;
	duk_idx_t decls_idx;
	duk_idx_t labelnames_idx;
	duk_idx_t labelinfos_idx;
	duk_idx_t argnames_idx;
	duk_idx_t varmap_idx;

	/* Temp reg handling. */
	duk_regconst_t temp_first; /* first register that is a temporary (below: variables) */
	duk_regconst_t temp_next; /* next temporary register to allocate */
	duk_regconst_t temp_max; /* highest value of temp_reg (temp_max - 1 is highest used reg) */

	/* Shuffle registers if large number of regs/consts. */
	duk_regconst_t shuffle1;
	duk_regconst_t shuffle2;
	duk_regconst_t shuffle3;

	/* Stats for current expression being parsed. */
	duk_int_t nud_count;
	duk_int_t led_count;
	duk_int_t paren_level; /* parenthesis count, 0 = top level */
	duk_bool_t expr_lhs; /* expression is left-hand-side compatible */
	duk_bool_t allow_in; /* current paren level allows 'in' token */

	/* Misc. */
	duk_int_t stmt_next; /* statement id allocation (running counter) */
	duk_int_t label_next; /* label id allocation (running counter) */
	duk_int_t catch_depth; /* catch stack depth */
	duk_int_t with_depth; /* with stack depth (affects identifier lookups) */
	duk_int_t fnum_next; /* inner function numbering */
	duk_int_t num_formals; /* number of formal arguments */
	duk_regconst_t
	    reg_stmt_value; /* register for writing value of 'non-empty' statements (global or eval code), -1 is marker */
#if defined(DUK_USE_DEBUGGER_SUPPORT)
	duk_int_t min_line; /* XXX: typing (duk_hcompfunc has duk_uint32_t) */
	duk_int_t max_line;
#endif

	/* Status booleans. */
	duk_uint8_t is_function; /* is an actual function (not global/eval code) */
	duk_uint8_t is_eval; /* is eval code */
	duk_uint8_t is_global; /* is global code */
	duk_uint8_t is_namebinding; /* needs a name binding */
	duk_uint8_t is_constructable; /* result is constructable */
	duk_uint8_t is_setget; /* is a setter/getter */
	duk_uint8_t is_strict; /* function is strict */
	duk_uint8_t is_notail; /* function must not be tail called */
	duk_uint8_t in_directive_prologue; /* parsing in "directive prologue", recognize directives */
	duk_uint8_t in_scanning; /* parsing in "scanning" phase (first pass) */
	duk_uint8_t may_direct_eval; /* function may call direct eval */
	duk_uint8_t id_access_arguments; /* function refers to 'arguments' identifier */
	duk_uint8_t id_access_slow; /* function makes one or more slow path accesses that won't match own static variables */
	duk_uint8_t id_access_slow_own; /* function makes one or more slow path accesses that may match own static variables */
	duk_uint8_t is_arguments_shadowed; /* argument/function declaration shadows 'arguments' */
	duk_uint8_t needs_shuffle; /* function needs shuffle registers */
	duk_uint8_t
	    reject_regexp_in_adv; /* reject RegExp literal on next advance() call; needed for handling IdentifierName productions */
	duk_uint8_t allow_regexp_in_adv; /* allow RegExp literal on next advance() call */
};

struct duk_compiler_ctx {
	duk_hthread *thr;

	/* filename being compiled (ends up in functions' '_filename' property) */
	duk_hstring *h_filename; /* borrowed reference */

	/* lexing (tokenization) state (contains two valstack slot indices) */
	duk_lexer_ctx lex;

	/* current and previous token for parsing */
	duk_token prev_token;
	duk_token curr_token;
	duk_idx_t tok11_idx; /* curr_token slot1 (matches 'lex' slot1_idx) */
	duk_idx_t tok12_idx; /* curr_token slot2 (matches 'lex' slot2_idx) */
	duk_idx_t tok21_idx; /* prev_token slot1 */
	duk_idx_t tok22_idx; /* prev_token slot2 */

	/* recursion limit */
	duk_int_t recursion_depth;
	duk_int_t recursion_limit;

	/* code emission temporary */
	duk_int_t emit_jumpslot_pc;

	/* current function being compiled (embedded instead of pointer for more compact access) */
	duk_compiler_func curr_func;
};

/*
 *  Prototypes
 */

DUK_INTERNAL_DECL void duk_js_compile(duk_hthread *thr,
                                      const duk_uint8_t *src_buffer,
                                      duk_size_t src_length,
                                      duk_small_uint_t flags);

#endif /* DUK_JS_COMPILER_H_INCLUDED */
