/*
 *  ECMAScript compiler.
 *
 *  Parses an input string and generates a function template result.
 *  Compilation may happen in multiple contexts (global code, eval
 *  code, function code).
 *
 *  The parser uses a traditional top-down recursive parsing for the
 *  statement level, and an operator precedence based top-down approach
 *  for the expression level.  The attempt is to minimize the C stack
 *  depth.  Bytecode is generated directly without an intermediate
 *  representation (tree), at the cost of needing two (and sometimes
 *  three) passes over each function.
 *
 *  The top-down recursive parser functions are named "duk__parse_XXX".
 *
 *  Recursion limits are in key functions to prevent arbitrary C recursion:
 *  function body parsing, statement parsing, and expression parsing.
 *
 *  See doc/compiler.rst for discussion on the design.
 *
 *  A few typing notes:
 *
 *    - duk_regconst_t: signed, highest bit set (< 0) means constant,
 *      some call sites use -1 for "none" (equivalent to constant 0x7fffffff)
 *    - PC values: duk_int_t, negative values used as markers
 */

#include "duk_internal.h"

/* If highest bit of a register number is set, it refers to a constant instead.
 * When interpreted as a signed value, this means const values are always
 * negative (when interpreted as two's complement).  For example DUK__ISREG_TEMP()
 * uses this approach to avoid an explicit DUK__ISREG() check (the condition is
 * logically "'x' is a register AND 'x' >= temp_first").
 */
#define DUK__CONST_MARKER   DUK_REGCONST_CONST_MARKER
#define DUK__REMOVECONST(x) ((x) & ~DUK__CONST_MARKER)
#define DUK__ISREG(x)       ((x) >= 0)
#define DUK__ISCONST(x)     ((x) < 0)
#define DUK__ISREG_TEMP(comp_ctx, x) \
	((duk_int32_t) (x) >= \
	 (duk_int32_t) ((comp_ctx)->curr_func.temp_first)) /* Check for x >= temp_first && x >= 0 by comparing as signed. */
#define DUK__ISREG_NOTTEMP(comp_ctx, x) \
	((duk_uint32_t) (x) < \
	 (duk_uint32_t) ((comp_ctx)->curr_func.temp_first)) /* Check for x >= 0 && x < temp_first by interpreting as unsigned. */
#define DUK__GETTEMP(comp_ctx)             ((comp_ctx)->curr_func.temp_next)
#define DUK__SETTEMP(comp_ctx, x)          ((comp_ctx)->curr_func.temp_next = (x)) /* dangerous: must only lower (temp_max not updated) */
#define DUK__SETTEMP_CHECKMAX(comp_ctx, x) duk__settemp_checkmax((comp_ctx), (x))
#define DUK__ALLOCTEMP(comp_ctx)           duk__alloctemp((comp_ctx))
#define DUK__ALLOCTEMPS(comp_ctx, count)   duk__alloctemps((comp_ctx), (count))

/* Init value set size for array and object literals. */
#define DUK__MAX_ARRAY_INIT_VALUES 20
#define DUK__MAX_OBJECT_INIT_PAIRS 10

/* XXX: hack, remove when const lookup is not O(n) */
#define DUK__GETCONST_MAX_CONSTS_CHECK 256

/* These limits are based on bytecode limits.  Max temps is limited
 * by duk_hcompfunc nargs/nregs fields being 16 bits.
 */
#define DUK__MAX_CONSTS DUK_BC_BC_MAX
#define DUK__MAX_FUNCS  DUK_BC_BC_MAX
#define DUK__MAX_TEMPS  0xffffL

/* Initial bytecode size allocation. */
#if defined(DUK_USE_PREFER_SIZE)
#define DUK__BC_INITIAL_INSTS 16
#else
#define DUK__BC_INITIAL_INSTS 256
#endif

#define DUK__RECURSION_INCREASE(comp_ctx, thr) \
	do { \
		DUK_DDD(DUK_DDDPRINT("RECURSION INCREASE: %s:%ld", (const char *) DUK_FILE_MACRO, (long) DUK_LINE_MACRO)); \
		duk__comp_recursion_increase((comp_ctx)); \
	} while (0)

#define DUK__RECURSION_DECREASE(comp_ctx, thr) \
	do { \
		DUK_DDD(DUK_DDDPRINT("RECURSION DECREASE: %s:%ld", (const char *) DUK_FILE_MACRO, (long) DUK_LINE_MACRO)); \
		duk__comp_recursion_decrease((comp_ctx)); \
	} while (0)

/* Value stack slot limits: these are quite approximate right now, and
 * because they overlap in control flow, some could be eliminated.
 */
#define DUK__COMPILE_ENTRY_SLOTS         8
#define DUK__FUNCTION_INIT_REQUIRE_SLOTS 16
#define DUK__FUNCTION_BODY_REQUIRE_SLOTS 16
#define DUK__PARSE_STATEMENTS_SLOTS      16
#define DUK__PARSE_EXPR_SLOTS            16

/* Temporary structure used to pass a stack allocated region through
 * duk_safe_call().
 */
typedef struct {
	duk_small_uint_t flags;
	duk_compiler_ctx comp_ctx_alloc;
	duk_lexer_point lex_pt_alloc;
} duk__compiler_stkstate;

/*
 *  Prototypes
 */

/* lexing */
DUK_LOCAL_DECL void duk__advance_helper(duk_compiler_ctx *comp_ctx, duk_small_int_t expect);
DUK_LOCAL_DECL void duk__advance_expect(duk_compiler_ctx *comp_ctx, duk_small_int_t expect);
DUK_LOCAL_DECL void duk__advance(duk_compiler_ctx *ctx);

/* function helpers */
DUK_LOCAL_DECL void duk__init_func_valstack_slots(duk_compiler_ctx *comp_ctx);
DUK_LOCAL_DECL void duk__reset_func_for_pass2(duk_compiler_ctx *comp_ctx);
DUK_LOCAL_DECL void duk__init_varmap_and_prologue_for_pass2(duk_compiler_ctx *comp_ctx, duk_regconst_t *out_stmt_value_reg);
DUK_LOCAL_DECL void duk__convert_to_func_template(duk_compiler_ctx *comp_ctx);
DUK_LOCAL_DECL duk_int_t duk__cleanup_varmap(duk_compiler_ctx *comp_ctx);

/* code emission */
DUK_LOCAL_DECL duk_int_t duk__get_current_pc(duk_compiler_ctx *comp_ctx);
DUK_LOCAL_DECL duk_compiler_instr *duk__get_instr_ptr(duk_compiler_ctx *comp_ctx, duk_int_t pc);
DUK_LOCAL_DECL void duk__emit(duk_compiler_ctx *comp_ctx, duk_instr_t ins);
DUK_LOCAL_DECL void duk__emit_op_only(duk_compiler_ctx *comp_ctx, duk_small_uint_t op);
DUK_LOCAL_DECL void duk__emit_a_b_c(duk_compiler_ctx *comp_ctx,
                                    duk_small_uint_t op_flags,
                                    duk_regconst_t a,
                                    duk_regconst_t b,
                                    duk_regconst_t c);
DUK_LOCAL_DECL void duk__emit_a_b(duk_compiler_ctx *comp_ctx, duk_small_uint_t op_flags, duk_regconst_t a, duk_regconst_t b);
DUK_LOCAL_DECL void duk__emit_b_c(duk_compiler_ctx *comp_ctx, duk_small_uint_t op_flags, duk_regconst_t b, duk_regconst_t c);
#if 0 /* unused */
DUK_LOCAL_DECL void duk__emit_a(duk_compiler_ctx *comp_ctx, duk_small_uint_t op_flags, duk_regconst_t a);
DUK_LOCAL_DECL void duk__emit_b(duk_compiler_ctx *comp_ctx, duk_small_uint_t op_flags, duk_regconst_t b);
#endif
DUK_LOCAL_DECL void duk__emit_a_bc(duk_compiler_ctx *comp_ctx, duk_small_uint_t op_flags, duk_regconst_t a, duk_regconst_t bc);
DUK_LOCAL_DECL void duk__emit_bc(duk_compiler_ctx *comp_ctx, duk_small_uint_t op, duk_regconst_t bc);
DUK_LOCAL_DECL void duk__emit_abc(duk_compiler_ctx *comp_ctx, duk_small_uint_t op, duk_regconst_t abc);
DUK_LOCAL_DECL void duk__emit_load_int32(duk_compiler_ctx *comp_ctx, duk_regconst_t reg, duk_int32_t val);
DUK_LOCAL_DECL void duk__emit_load_int32_noshuffle(duk_compiler_ctx *comp_ctx, duk_regconst_t reg, duk_int32_t val);
DUK_LOCAL_DECL void duk__emit_jump(duk_compiler_ctx *comp_ctx, duk_int_t target_pc);
DUK_LOCAL_DECL duk_int_t duk__emit_jump_empty(duk_compiler_ctx *comp_ctx);
DUK_LOCAL_DECL void duk__insert_jump_entry(duk_compiler_ctx *comp_ctx, duk_int_t jump_pc);
DUK_LOCAL_DECL void duk__patch_jump(duk_compiler_ctx *comp_ctx, duk_int_t jump_pc, duk_int_t target_pc);
DUK_LOCAL_DECL void duk__patch_jump_here(duk_compiler_ctx *comp_ctx, duk_int_t jump_pc);
DUK_LOCAL_DECL void duk__patch_trycatch(duk_compiler_ctx *comp_ctx,
                                        duk_int_t ldconst_pc,
                                        duk_int_t trycatch_pc,
                                        duk_regconst_t reg_catch,
                                        duk_regconst_t const_varname,
                                        duk_small_uint_t flags);
DUK_LOCAL_DECL void duk__emit_if_false_skip(duk_compiler_ctx *comp_ctx, duk_regconst_t regconst);
DUK_LOCAL_DECL void duk__emit_if_true_skip(duk_compiler_ctx *comp_ctx, duk_regconst_t regconst);
DUK_LOCAL_DECL void duk__emit_invalid(duk_compiler_ctx *comp_ctx);

/* ivalue/ispec helpers */
DUK_LOCAL_DECL void duk__ivalue_regconst(duk_ivalue *x, duk_regconst_t regconst);
DUK_LOCAL_DECL void duk__ivalue_plain_fromstack(duk_compiler_ctx *comp_ctx, duk_ivalue *x);
DUK_LOCAL_DECL void duk__ivalue_var_fromstack(duk_compiler_ctx *comp_ctx, duk_ivalue *x);
DUK_LOCAL_DECL void duk__ivalue_var_hstring(duk_compiler_ctx *comp_ctx, duk_ivalue *x, duk_hstring *h);
DUK_LOCAL_DECL void duk__copy_ispec(duk_compiler_ctx *comp_ctx, duk_ispec *src, duk_ispec *dst);
DUK_LOCAL_DECL void duk__copy_ivalue(duk_compiler_ctx *comp_ctx, duk_ivalue *src, duk_ivalue *dst);
DUK_LOCAL_DECL duk_regconst_t duk__alloctemps(duk_compiler_ctx *comp_ctx, duk_small_int_t num);
DUK_LOCAL_DECL duk_regconst_t duk__alloctemp(duk_compiler_ctx *comp_ctx);
DUK_LOCAL_DECL void duk__settemp_checkmax(duk_compiler_ctx *comp_ctx, duk_regconst_t temp_next);
DUK_LOCAL_DECL duk_regconst_t duk__getconst(duk_compiler_ctx *comp_ctx);
DUK_LOCAL_DECL
duk_regconst_t duk__ispec_toregconst_raw(duk_compiler_ctx *comp_ctx,
                                         duk_ispec *x,
                                         duk_regconst_t forced_reg,
                                         duk_small_uint_t flags);
DUK_LOCAL_DECL void duk__ispec_toforcedreg(duk_compiler_ctx *comp_ctx, duk_ispec *x, duk_regconst_t forced_reg);
DUK_LOCAL_DECL void duk__ivalue_toplain_raw(duk_compiler_ctx *comp_ctx, duk_ivalue *x, duk_regconst_t forced_reg);
DUK_LOCAL_DECL void duk__ivalue_toplain(duk_compiler_ctx *comp_ctx, duk_ivalue *x);
DUK_LOCAL_DECL void duk__ivalue_toplain_ignore(duk_compiler_ctx *comp_ctx, duk_ivalue *x);
DUK_LOCAL_DECL
duk_regconst_t duk__ivalue_toregconst_raw(duk_compiler_ctx *comp_ctx,
                                          duk_ivalue *x,
                                          duk_regconst_t forced_reg,
                                          duk_small_uint_t flags);
DUK_LOCAL_DECL duk_regconst_t duk__ivalue_toreg(duk_compiler_ctx *comp_ctx, duk_ivalue *x);
#if 0 /* unused */
DUK_LOCAL_DECL duk_regconst_t duk__ivalue_totemp(duk_compiler_ctx *comp_ctx, duk_ivalue *x);
#endif
DUK_LOCAL_DECL void duk__ivalue_toforcedreg(duk_compiler_ctx *comp_ctx, duk_ivalue *x, duk_int_t forced_reg);
DUK_LOCAL_DECL duk_regconst_t duk__ivalue_toregconst(duk_compiler_ctx *comp_ctx, duk_ivalue *x);
DUK_LOCAL_DECL duk_regconst_t duk__ivalue_totempconst(duk_compiler_ctx *comp_ctx, duk_ivalue *x);

/* identifier handling */
DUK_LOCAL_DECL duk_regconst_t duk__lookup_active_register_binding(duk_compiler_ctx *comp_ctx);
DUK_LOCAL_DECL duk_bool_t duk__lookup_lhs(duk_compiler_ctx *ctx, duk_regconst_t *out_reg_varbind, duk_regconst_t *out_rc_varname);

/* label handling */
DUK_LOCAL_DECL void duk__add_label(duk_compiler_ctx *comp_ctx, duk_hstring *h_label, duk_int_t pc_label, duk_int_t label_id);
DUK_LOCAL_DECL void duk__update_label_flags(duk_compiler_ctx *comp_ctx, duk_int_t label_id, duk_small_uint_t flags);
DUK_LOCAL_DECL void duk__lookup_active_label(duk_compiler_ctx *comp_ctx,
                                             duk_hstring *h_label,
                                             duk_bool_t is_break,
                                             duk_int_t *out_label_id,
                                             duk_int_t *out_label_catch_depth,
                                             duk_int_t *out_label_pc,
                                             duk_bool_t *out_is_closest);
DUK_LOCAL_DECL void duk__reset_labels_to_length(duk_compiler_ctx *comp_ctx, duk_size_t len);

/* top-down expression parser */
DUK_LOCAL_DECL void duk__expr_nud(duk_compiler_ctx *comp_ctx, duk_ivalue *res);
DUK_LOCAL_DECL void duk__expr_led(duk_compiler_ctx *comp_ctx, duk_ivalue *left, duk_ivalue *res);
DUK_LOCAL_DECL duk_small_uint_t duk__expr_lbp(duk_compiler_ctx *comp_ctx);
DUK_LOCAL_DECL duk_bool_t duk__expr_is_empty(duk_compiler_ctx *comp_ctx);

/* exprtop is the top level variant which resets nud/led counts */
DUK_LOCAL_DECL void duk__expr(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags);
DUK_LOCAL_DECL void duk__exprtop(duk_compiler_ctx *ctx, duk_ivalue *res, duk_small_uint_t rbp_flags);

/* convenience helpers */
#if 0 /* unused */
DUK_LOCAL_DECL duk_regconst_t duk__expr_toreg(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags);
#endif
#if 0 /* unused */
DUK_LOCAL_DECL duk_regconst_t duk__expr_totemp(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags);
#endif
DUK_LOCAL_DECL void duk__expr_toforcedreg(duk_compiler_ctx *comp_ctx,
                                          duk_ivalue *res,
                                          duk_small_uint_t rbp_flags,
                                          duk_regconst_t forced_reg);
DUK_LOCAL_DECL duk_regconst_t duk__expr_toregconst(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags);
#if 0 /* unused */
DUK_LOCAL_DECL duk_regconst_t duk__expr_totempconst(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags);
#endif
DUK_LOCAL_DECL void duk__expr_toplain(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags);
DUK_LOCAL_DECL void duk__expr_toplain_ignore(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags);
DUK_LOCAL_DECL duk_regconst_t duk__exprtop_toreg(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags);
#if 0 /* unused */
DUK_LOCAL_DECL duk_regconst_t duk__exprtop_totemp(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags);
#endif
DUK_LOCAL_DECL void duk__exprtop_toforcedreg(duk_compiler_ctx *comp_ctx,
                                             duk_ivalue *res,
                                             duk_small_uint_t rbp_flags,
                                             duk_regconst_t forced_reg);
DUK_LOCAL_DECL duk_regconst_t duk__exprtop_toregconst(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags);
#if 0 /* unused */
DUK_LOCAL_DECL void duk__exprtop_toplain_ignore(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags);
#endif

/* expression parsing helpers */
DUK_LOCAL_DECL duk_int_t duk__parse_arguments(duk_compiler_ctx *comp_ctx, duk_ivalue *res);
DUK_LOCAL_DECL void duk__nud_array_literal(duk_compiler_ctx *comp_ctx, duk_ivalue *res);
DUK_LOCAL_DECL void duk__nud_object_literal(duk_compiler_ctx *comp_ctx, duk_ivalue *res);

/* statement parsing */
DUK_LOCAL_DECL void duk__parse_var_decl(duk_compiler_ctx *comp_ctx,
                                        duk_ivalue *res,
                                        duk_small_uint_t expr_flags,
                                        duk_regconst_t *out_reg_varbind,
                                        duk_regconst_t *out_rc_varname);
DUK_LOCAL_DECL void duk__parse_var_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t expr_flags);
DUK_LOCAL_DECL void duk__parse_for_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_int_t pc_label_site);
DUK_LOCAL_DECL void duk__parse_switch_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_int_t pc_label_site);
DUK_LOCAL_DECL void duk__parse_if_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res);
DUK_LOCAL_DECL void duk__parse_do_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_int_t pc_label_site);
DUK_LOCAL_DECL void duk__parse_while_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_int_t pc_label_site);
DUK_LOCAL_DECL void duk__parse_break_or_continue_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res);
DUK_LOCAL_DECL void duk__parse_return_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res);
DUK_LOCAL_DECL void duk__parse_throw_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res);
DUK_LOCAL_DECL void duk__parse_try_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res);
DUK_LOCAL_DECL void duk__parse_with_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res);
DUK_LOCAL_DECL void duk__parse_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_bool_t allow_source_elem);
DUK_LOCAL_DECL duk_int_t duk__stmt_label_site(duk_compiler_ctx *comp_ctx, duk_int_t label_id);
DUK_LOCAL_DECL void duk__parse_stmts(duk_compiler_ctx *comp_ctx,
                                     duk_bool_t allow_source_elem,
                                     duk_bool_t expect_eof,
                                     duk_bool_t regexp_after);

DUK_LOCAL_DECL void duk__parse_func_body(duk_compiler_ctx *comp_ctx,
                                         duk_bool_t expect_eof,
                                         duk_bool_t implicit_return_value,
                                         duk_bool_t regexp_after,
                                         duk_small_int_t expect_token);
DUK_LOCAL_DECL void duk__parse_func_formals(duk_compiler_ctx *comp_ctx);
DUK_LOCAL_DECL void duk__parse_func_like_raw(duk_compiler_ctx *comp_ctx, duk_small_uint_t flags);
DUK_LOCAL_DECL duk_int_t duk__parse_func_like_fnum(duk_compiler_ctx *comp_ctx, duk_small_uint_t flags);

#define DUK__FUNC_FLAG_DECL           (1 << 0) /* Parsing a function declaration. */
#define DUK__FUNC_FLAG_GETSET         (1 << 1) /* Parsing an object literal getter/setter. */
#define DUK__FUNC_FLAG_METDEF         (1 << 2) /* Parsing an object literal method definition shorthand. */
#define DUK__FUNC_FLAG_PUSHNAME_PASS1 (1 << 3) /* Push function name when creating template (first pass only). */
#define DUK__FUNC_FLAG_USE_PREVTOKEN  (1 << 4) /* Use prev_token to start function parsing (workaround for object literal). */

/*
 *  Parser control values for tokens.  The token table is ordered by the
 *  DUK_TOK_XXX defines.
 *
 *  The binding powers are for lbp() use (i.e. for use in led() context).
 *  Binding powers are positive for typing convenience, and bits at the
 *  top should be reserved for flags.  Binding power step must be higher
 *  than 1 so that binding power "lbp - 1" can be used for right associative
 *  operators.  Currently a step of 2 is used (which frees one more bit for
 *  flags).
 */

/* XXX: actually single step levels would work just fine, clean up */

/* binding power "levels" (see doc/compiler.rst) */
#define DUK__BP_INVALID        0 /* always terminates led() */
#define DUK__BP_EOF            2
#define DUK__BP_CLOSING        4 /* token closes expression, e.g. ')', ']' */
#define DUK__BP_FOR_EXPR       DUK__BP_CLOSING /* bp to use when parsing a top level Expression */
#define DUK__BP_COMMA          6
#define DUK__BP_ASSIGNMENT     8
#define DUK__BP_CONDITIONAL    10
#define DUK__BP_LOR            12
#define DUK__BP_LAND           14
#define DUK__BP_BOR            16
#define DUK__BP_BXOR           18
#define DUK__BP_BAND           20
#define DUK__BP_EQUALITY       22
#define DUK__BP_RELATIONAL     24
#define DUK__BP_SHIFT          26
#define DUK__BP_ADDITIVE       28
#define DUK__BP_MULTIPLICATIVE 30
#define DUK__BP_EXPONENTIATION 32
#define DUK__BP_POSTFIX        34
#define DUK__BP_CALL           36
#define DUK__BP_MEMBER         38

#define DUK__TOKEN_LBP_BP_MASK         0x1f
#define DUK__TOKEN_LBP_FLAG_NO_REGEXP  (1 << 5) /* regexp literal must not follow this token */
#define DUK__TOKEN_LBP_FLAG_TERMINATES (1 << 6) /* terminates expression; e.g. post-increment/-decrement */
#define DUK__TOKEN_LBP_FLAG_UNUSED     (1 << 7) /* unused */

#define DUK__TOKEN_LBP_GET_BP(x) ((duk_small_uint_t) (((x) &DUK__TOKEN_LBP_BP_MASK) * 2))

#define DUK__MK_LBP(bp)              ((bp) >> 1) /* bp is assumed to be even */
#define DUK__MK_LBP_FLAGS(bp, flags) (((bp) >> 1) | (flags))

DUK_LOCAL const duk_uint8_t duk__token_lbp[] = {
	DUK__MK_LBP(DUK__BP_EOF), /* DUK_TOK_EOF */
	DUK__MK_LBP_FLAGS(DUK__BP_INVALID, DUK__TOKEN_LBP_FLAG_NO_REGEXP), /* DUK_TOK_IDENTIFIER */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_BREAK */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_CASE */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_CATCH */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_CONTINUE */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_DEBUGGER */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_DEFAULT */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_DELETE */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_DO */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_ELSE */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_FINALLY */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_FOR */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_FUNCTION */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_IF */
	DUK__MK_LBP(DUK__BP_RELATIONAL), /* DUK_TOK_IN */
	DUK__MK_LBP(DUK__BP_RELATIONAL), /* DUK_TOK_INSTANCEOF */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_NEW */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_RETURN */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_SWITCH */
	DUK__MK_LBP_FLAGS(DUK__BP_INVALID, DUK__TOKEN_LBP_FLAG_NO_REGEXP), /* DUK_TOK_THIS */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_THROW */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_TRY */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_TYPEOF */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_VAR */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_CONST */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_VOID */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_WHILE */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_WITH */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_CLASS */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_ENUM */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_EXPORT */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_EXTENDS */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_IMPORT */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_SUPER */
	DUK__MK_LBP_FLAGS(DUK__BP_INVALID, DUK__TOKEN_LBP_FLAG_NO_REGEXP), /* DUK_TOK_NULL */
	DUK__MK_LBP_FLAGS(DUK__BP_INVALID, DUK__TOKEN_LBP_FLAG_NO_REGEXP), /* DUK_TOK_TRUE */
	DUK__MK_LBP_FLAGS(DUK__BP_INVALID, DUK__TOKEN_LBP_FLAG_NO_REGEXP), /* DUK_TOK_FALSE */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_GET */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_SET */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_IMPLEMENTS */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_INTERFACE */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_LET */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_PACKAGE */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_PRIVATE */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_PROTECTED */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_PUBLIC */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_STATIC */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_YIELD */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_LCURLY */
	DUK__MK_LBP_FLAGS(DUK__BP_INVALID, DUK__TOKEN_LBP_FLAG_NO_REGEXP), /* DUK_TOK_RCURLY */
	DUK__MK_LBP(DUK__BP_MEMBER), /* DUK_TOK_LBRACKET */
	DUK__MK_LBP_FLAGS(DUK__BP_CLOSING, DUK__TOKEN_LBP_FLAG_NO_REGEXP), /* DUK_TOK_RBRACKET */
	DUK__MK_LBP(DUK__BP_CALL), /* DUK_TOK_LPAREN */
	DUK__MK_LBP_FLAGS(DUK__BP_CLOSING, DUK__TOKEN_LBP_FLAG_NO_REGEXP), /* DUK_TOK_RPAREN */
	DUK__MK_LBP(DUK__BP_MEMBER), /* DUK_TOK_PERIOD */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_SEMICOLON */
	DUK__MK_LBP(DUK__BP_COMMA), /* DUK_TOK_COMMA */
	DUK__MK_LBP(DUK__BP_RELATIONAL), /* DUK_TOK_LT */
	DUK__MK_LBP(DUK__BP_RELATIONAL), /* DUK_TOK_GT */
	DUK__MK_LBP(DUK__BP_RELATIONAL), /* DUK_TOK_LE */
	DUK__MK_LBP(DUK__BP_RELATIONAL), /* DUK_TOK_GE */
	DUK__MK_LBP(DUK__BP_EQUALITY), /* DUK_TOK_EQ */
	DUK__MK_LBP(DUK__BP_EQUALITY), /* DUK_TOK_NEQ */
	DUK__MK_LBP(DUK__BP_EQUALITY), /* DUK_TOK_SEQ */
	DUK__MK_LBP(DUK__BP_EQUALITY), /* DUK_TOK_SNEQ */
	DUK__MK_LBP(DUK__BP_ADDITIVE), /* DUK_TOK_ADD */
	DUK__MK_LBP(DUK__BP_ADDITIVE), /* DUK_TOK_SUB */
	DUK__MK_LBP(DUK__BP_MULTIPLICATIVE), /* DUK_TOK_MUL */
	DUK__MK_LBP(DUK__BP_MULTIPLICATIVE), /* DUK_TOK_DIV */
	DUK__MK_LBP(DUK__BP_MULTIPLICATIVE), /* DUK_TOK_MOD */
	DUK__MK_LBP(DUK__BP_EXPONENTIATION), /* DUK_TOK_EXP */
	DUK__MK_LBP_FLAGS(DUK__BP_POSTFIX, DUK__TOKEN_LBP_FLAG_NO_REGEXP), /* DUK_TOK_INCREMENT */
	DUK__MK_LBP_FLAGS(DUK__BP_POSTFIX, DUK__TOKEN_LBP_FLAG_NO_REGEXP), /* DUK_TOK_DECREMENT */
	DUK__MK_LBP(DUK__BP_SHIFT), /* DUK_TOK_ALSHIFT */
	DUK__MK_LBP(DUK__BP_SHIFT), /* DUK_TOK_ARSHIFT */
	DUK__MK_LBP(DUK__BP_SHIFT), /* DUK_TOK_RSHIFT */
	DUK__MK_LBP(DUK__BP_BAND), /* DUK_TOK_BAND */
	DUK__MK_LBP(DUK__BP_BOR), /* DUK_TOK_BOR */
	DUK__MK_LBP(DUK__BP_BXOR), /* DUK_TOK_BXOR */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_LNOT */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_BNOT */
	DUK__MK_LBP(DUK__BP_LAND), /* DUK_TOK_LAND */
	DUK__MK_LBP(DUK__BP_LOR), /* DUK_TOK_LOR */
	DUK__MK_LBP(DUK__BP_CONDITIONAL), /* DUK_TOK_QUESTION */
	DUK__MK_LBP(DUK__BP_INVALID), /* DUK_TOK_COLON */
	DUK__MK_LBP(DUK__BP_ASSIGNMENT), /* DUK_TOK_EQUALSIGN */
	DUK__MK_LBP(DUK__BP_ASSIGNMENT), /* DUK_TOK_ADD_EQ */
	DUK__MK_LBP(DUK__BP_ASSIGNMENT), /* DUK_TOK_SUB_EQ */
	DUK__MK_LBP(DUK__BP_ASSIGNMENT), /* DUK_TOK_MUL_EQ */
	DUK__MK_LBP(DUK__BP_ASSIGNMENT), /* DUK_TOK_DIV_EQ */
	DUK__MK_LBP(DUK__BP_ASSIGNMENT), /* DUK_TOK_MOD_EQ */
	DUK__MK_LBP(DUK__BP_ASSIGNMENT), /* DUK_TOK_EXP_EQ */
	DUK__MK_LBP(DUK__BP_ASSIGNMENT), /* DUK_TOK_ALSHIFT_EQ */
	DUK__MK_LBP(DUK__BP_ASSIGNMENT), /* DUK_TOK_ARSHIFT_EQ */
	DUK__MK_LBP(DUK__BP_ASSIGNMENT), /* DUK_TOK_RSHIFT_EQ */
	DUK__MK_LBP(DUK__BP_ASSIGNMENT), /* DUK_TOK_BAND_EQ */
	DUK__MK_LBP(DUK__BP_ASSIGNMENT), /* DUK_TOK_BOR_EQ */
	DUK__MK_LBP(DUK__BP_ASSIGNMENT), /* DUK_TOK_BXOR_EQ */
	DUK__MK_LBP_FLAGS(DUK__BP_INVALID, DUK__TOKEN_LBP_FLAG_NO_REGEXP), /* DUK_TOK_NUMBER */
	DUK__MK_LBP_FLAGS(DUK__BP_INVALID, DUK__TOKEN_LBP_FLAG_NO_REGEXP), /* DUK_TOK_STRING */
	DUK__MK_LBP_FLAGS(DUK__BP_INVALID, DUK__TOKEN_LBP_FLAG_NO_REGEXP), /* DUK_TOK_REGEXP */
};

/*
 *  Misc helpers
 */

DUK_LOCAL void duk__comp_recursion_increase(duk_compiler_ctx *comp_ctx) {
	DUK_ASSERT(comp_ctx != NULL);
	DUK_ASSERT(comp_ctx->recursion_depth >= 0);
	if (comp_ctx->recursion_depth >= comp_ctx->recursion_limit) {
		DUK_ERROR_RANGE(comp_ctx->thr, DUK_STR_COMPILER_RECURSION_LIMIT);
		DUK_WO_NORETURN(return;);
	}
	comp_ctx->recursion_depth++;
}

DUK_LOCAL void duk__comp_recursion_decrease(duk_compiler_ctx *comp_ctx) {
	DUK_ASSERT(comp_ctx != NULL);
	DUK_ASSERT(comp_ctx->recursion_depth > 0);
	comp_ctx->recursion_depth--;
}

DUK_LOCAL duk_bool_t duk__hstring_is_eval_or_arguments(duk_compiler_ctx *comp_ctx, duk_hstring *h) {
	DUK_UNREF(comp_ctx);
	DUK_ASSERT(h != NULL);
	return DUK_HSTRING_HAS_EVAL_OR_ARGUMENTS(h);
}

DUK_LOCAL duk_bool_t duk__hstring_is_eval_or_arguments_in_strict_mode(duk_compiler_ctx *comp_ctx, duk_hstring *h) {
	DUK_ASSERT(h != NULL);
	return (comp_ctx->curr_func.is_strict && DUK_HSTRING_HAS_EVAL_OR_ARGUMENTS(h));
}

/*
 *  Parser duk__advance() token eating functions
 */

/* XXX: valstack handling is awkward.  Add a valstack helper which
 * avoids dup():ing; valstack_copy(src, dst)?
 */

DUK_LOCAL void duk__advance_helper(duk_compiler_ctx *comp_ctx, duk_small_int_t expect) {
	duk_hthread *thr = comp_ctx->thr;
	duk_bool_t regexp;

	DUK_ASSERT_DISABLE(comp_ctx->curr_token.t >= 0); /* unsigned */
	DUK_ASSERT(comp_ctx->curr_token.t <= DUK_TOK_MAXVAL); /* MAXVAL is inclusive */

	/*
	 *  Use current token to decide whether a RegExp can follow.
	 *
	 *  We can use either 't' or 't_nores'; the latter would not
	 *  recognize keywords.  Some keywords can be followed by a
	 *  RegExp (e.g. "return"), so using 't' is better.  This is
	 *  not trivial, see doc/compiler.rst.
	 */

	regexp = 1;
	if (duk__token_lbp[comp_ctx->curr_token.t] & DUK__TOKEN_LBP_FLAG_NO_REGEXP) {
		regexp = 0;
	}
	if (comp_ctx->curr_func.reject_regexp_in_adv) {
		comp_ctx->curr_func.reject_regexp_in_adv = 0;
		regexp = 0;
	}
	if (comp_ctx->curr_func.allow_regexp_in_adv) {
		comp_ctx->curr_func.allow_regexp_in_adv = 0;
		regexp = 1;
	}

	if (expect >= 0 && comp_ctx->curr_token.t != (duk_small_uint_t) expect) {
		DUK_D(DUK_DPRINT("parse error: expect=%ld, got=%ld", (long) expect, (long) comp_ctx->curr_token.t));
		DUK_ERROR_SYNTAX(thr, DUK_STR_PARSE_ERROR);
		DUK_WO_NORETURN(return;);
	}

	/* make current token the previous; need to fiddle with valstack "backing store" */
	duk_memcpy(&comp_ctx->prev_token, &comp_ctx->curr_token, sizeof(duk_token));
	duk_copy(thr, comp_ctx->tok11_idx, comp_ctx->tok21_idx);
	duk_copy(thr, comp_ctx->tok12_idx, comp_ctx->tok22_idx);

	/* parse new token */
	duk_lexer_parse_js_input_element(&comp_ctx->lex, &comp_ctx->curr_token, comp_ctx->curr_func.is_strict, regexp);

	DUK_DDD(DUK_DDDPRINT("advance: curr: tok=%ld/%ld,%ld,term=%ld,%!T,%!T "
	                     "prev: tok=%ld/%ld,%ld,term=%ld,%!T,%!T",
	                     (long) comp_ctx->curr_token.t,
	                     (long) comp_ctx->curr_token.t_nores,
	                     (long) comp_ctx->curr_token.start_line,
	                     (long) comp_ctx->curr_token.lineterm,
	                     (duk_tval *) duk_get_tval(thr, comp_ctx->tok11_idx),
	                     (duk_tval *) duk_get_tval(thr, comp_ctx->tok12_idx),
	                     (long) comp_ctx->prev_token.t,
	                     (long) comp_ctx->prev_token.t_nores,
	                     (long) comp_ctx->prev_token.start_line,
	                     (long) comp_ctx->prev_token.lineterm,
	                     (duk_tval *) duk_get_tval(thr, comp_ctx->tok21_idx),
	                     (duk_tval *) duk_get_tval(thr, comp_ctx->tok22_idx)));
}

/* advance, expecting current token to be a specific token; parse next token in regexp context */
DUK_LOCAL void duk__advance_expect(duk_compiler_ctx *comp_ctx, duk_small_int_t expect) {
	duk__advance_helper(comp_ctx, expect);
}

/* advance, whatever the current token is; parse next token in regexp context */
DUK_LOCAL void duk__advance(duk_compiler_ctx *comp_ctx) {
	duk__advance_helper(comp_ctx, -1);
}

/*
 *  Helpers for duk_compiler_func.
 */

/* init function state: inits valstack allocations */
DUK_LOCAL void duk__init_func_valstack_slots(duk_compiler_ctx *comp_ctx) {
	duk_compiler_func *func = &comp_ctx->curr_func;
	duk_hthread *thr = comp_ctx->thr;
	duk_idx_t entry_top;

	entry_top = duk_get_top(thr);

	duk_memzero(func, sizeof(*func)); /* intentional overlap with earlier memzero */
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	func->h_name = NULL;
	func->h_consts = NULL;
	func->h_funcs = NULL;
	func->h_decls = NULL;
	func->h_labelnames = NULL;
	func->h_labelinfos = NULL;
	func->h_argnames = NULL;
	func->h_varmap = NULL;
#endif

	duk_require_stack(thr, DUK__FUNCTION_INIT_REQUIRE_SLOTS);

	DUK_BW_INIT_PUSHBUF(thr, &func->bw_code, DUK__BC_INITIAL_INSTS * sizeof(duk_compiler_instr));
	/* code_idx = entry_top + 0 */

	duk_push_bare_array(thr);
	func->consts_idx = entry_top + 1;
	func->h_consts = DUK_GET_HOBJECT_POSIDX(thr, entry_top + 1);
	DUK_ASSERT(func->h_consts != NULL);

	duk_push_bare_array(thr);
	func->funcs_idx = entry_top + 2;
	func->h_funcs = DUK_GET_HOBJECT_POSIDX(thr, entry_top + 2);
	DUK_ASSERT(func->h_funcs != NULL);
	DUK_ASSERT(func->fnum_next == 0);

	duk_push_bare_array(thr);
	func->decls_idx = entry_top + 3;
	func->h_decls = DUK_GET_HOBJECT_POSIDX(thr, entry_top + 3);
	DUK_ASSERT(func->h_decls != NULL);

	duk_push_bare_array(thr);
	func->labelnames_idx = entry_top + 4;
	func->h_labelnames = DUK_GET_HOBJECT_POSIDX(thr, entry_top + 4);
	DUK_ASSERT(func->h_labelnames != NULL);

	duk_push_dynamic_buffer(thr, 0);
	func->labelinfos_idx = entry_top + 5;
	func->h_labelinfos = (duk_hbuffer_dynamic *) duk_known_hbuffer(thr, entry_top + 5);
	DUK_ASSERT(func->h_labelinfos != NULL);
	DUK_ASSERT(DUK_HBUFFER_HAS_DYNAMIC(func->h_labelinfos) && !DUK_HBUFFER_HAS_EXTERNAL(func->h_labelinfos));

	duk_push_bare_array(thr);
	func->argnames_idx = entry_top + 6;
	func->h_argnames = DUK_GET_HOBJECT_POSIDX(thr, entry_top + 6);
	DUK_ASSERT(func->h_argnames != NULL);

	duk_push_bare_object(thr);
	func->varmap_idx = entry_top + 7;
	func->h_varmap = DUK_GET_HOBJECT_POSIDX(thr, entry_top + 7);
	DUK_ASSERT(func->h_varmap != NULL);
}

/* reset function state (prepare for pass 2) */
DUK_LOCAL void duk__reset_func_for_pass2(duk_compiler_ctx *comp_ctx) {
	duk_compiler_func *func = &comp_ctx->curr_func;
	duk_hthread *thr = comp_ctx->thr;

	/* reset bytecode buffer but keep current size; pass 2 will
	 * require same amount or more.
	 */
	DUK_BW_RESET_SIZE(thr, &func->bw_code);

	duk_set_length(thr, func->consts_idx, 0);
	/* keep func->h_funcs; inner functions are not reparsed to avoid O(depth^2) parsing */
	func->fnum_next = 0;
	/* duk_set_length(thr, func->funcs_idx, 0); */
	duk_set_length(thr, func->labelnames_idx, 0);
	duk_hbuffer_reset(thr, func->h_labelinfos);
	/* keep func->h_argnames; it is fixed for all passes */

	/* truncated in case pass 3 needed */
	duk_push_bare_object(thr);
	duk_replace(thr, func->varmap_idx);
	func->h_varmap = DUK_GET_HOBJECT_POSIDX(thr, func->varmap_idx);
	DUK_ASSERT(func->h_varmap != NULL);
}

/* cleanup varmap from any null entries, compact it, etc; returns number
 * of final entries after cleanup.
 */
DUK_LOCAL duk_int_t duk__cleanup_varmap(duk_compiler_ctx *comp_ctx) {
	duk_hthread *thr = comp_ctx->thr;
	duk_hobject *h_varmap;
	duk_hstring *h_key;
	duk_tval *tv;
	duk_uint32_t i, e_next;
	duk_int_t ret;

	/* [ ... varmap ] */

	h_varmap = DUK_GET_HOBJECT_NEGIDX(thr, -1);
	DUK_ASSERT(h_varmap != NULL);

	ret = 0;
	e_next = DUK_HOBJECT_GET_ENEXT(h_varmap);
	for (i = 0; i < e_next; i++) {
		h_key = DUK_HOBJECT_E_GET_KEY(thr->heap, h_varmap, i);
		if (!h_key) {
			continue;
		}

		DUK_ASSERT(!DUK_HOBJECT_E_SLOT_IS_ACCESSOR(thr->heap, h_varmap, i));

		/* The entries can either be register numbers or 'null' values.
		 * Thus, no need to DECREF them and get side effects.  DECREF'ing
		 * the keys (strings) can cause memory to be freed but no side
		 * effects as strings don't have finalizers.  This is why we can
		 * rely on the object properties not changing from underneath us.
		 */

		tv = DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(thr->heap, h_varmap, i);
		if (!DUK_TVAL_IS_NUMBER(tv)) {
			DUK_ASSERT(!DUK_TVAL_IS_HEAP_ALLOCATED(tv));
			DUK_HOBJECT_E_SET_KEY(thr->heap, h_varmap, i, NULL);
			DUK_HSTRING_DECREF(thr, h_key);
			/* when key is NULL, value is garbage so no need to set */
		} else {
			ret++;
		}
	}

	duk_compact_m1(thr);

	return ret;
}

/* Convert duk_compiler_func into a function template, leaving the result
 * on top of stack.
 */
/* XXX: awkward and bloated asm -- use faster internal accesses */
DUK_LOCAL void duk__convert_to_func_template(duk_compiler_ctx *comp_ctx) {
	duk_compiler_func *func = &comp_ctx->curr_func;
	duk_hthread *thr = comp_ctx->thr;
	duk_hcompfunc *h_res;
	duk_hbuffer_fixed *h_data;
	duk_size_t consts_count;
	duk_size_t funcs_count;
	duk_size_t code_count;
	duk_size_t code_size;
	duk_size_t data_size;
	duk_size_t i;
	duk_tval *p_const;
	duk_hobject **p_func;
	duk_instr_t *p_instr;
	duk_compiler_instr *q_instr;
	duk_tval *tv;
	duk_bool_t keep_varmap;
	duk_bool_t keep_formals;
#if !defined(DUK_USE_DEBUGGER_SUPPORT)
	duk_size_t formals_length;
#endif

	DUK_DDD(DUK_DDDPRINT("converting duk_compiler_func to function/template"));

	/*
	 *  Push result object and init its flags
	 */

	/* Valstack should suffice here, required on function valstack init */

	h_res = duk_push_hcompfunc(thr);
	DUK_ASSERT(h_res != NULL);
	DUK_ASSERT(DUK_HOBJECT_GET_PROTOTYPE(thr->heap, (duk_hobject *) h_res) == thr->builtins[DUK_BIDX_FUNCTION_PROTOTYPE]);
	DUK_HOBJECT_SET_PROTOTYPE_UPDREF(thr, (duk_hobject *) h_res, NULL); /* Function templates are "bare objects". */

	if (func->is_function) {
		DUK_DDD(DUK_DDDPRINT("function -> set NEWENV"));
		DUK_HOBJECT_SET_NEWENV((duk_hobject *) h_res);

		if (!func->is_arguments_shadowed) {
			/* arguments object would be accessible; note that shadowing
			 * bindings are arguments or function declarations, neither
			 * of which are deletable, so this is safe.
			 */

			if (func->id_access_arguments || func->may_direct_eval) {
				DUK_DDD(DUK_DDDPRINT("function may access 'arguments' object directly or "
				                     "indirectly -> set CREATEARGS"));
				DUK_HOBJECT_SET_CREATEARGS((duk_hobject *) h_res);
			}
		}
	} else if (func->is_eval && func->is_strict) {
		DUK_DDD(DUK_DDDPRINT("strict eval code -> set NEWENV"));
		DUK_HOBJECT_SET_NEWENV((duk_hobject *) h_res);
	} else {
		/* non-strict eval: env is caller's env or global env (direct vs. indirect call)
		 * global code: env is is global env
		 */
		DUK_DDD(DUK_DDDPRINT("non-strict eval code or global code -> no NEWENV"));
		DUK_ASSERT(!DUK_HOBJECT_HAS_NEWENV((duk_hobject *) h_res));
	}

#if defined(DUK_USE_FUNC_NAME_PROPERTY)
	if (func->is_function && func->is_namebinding && func->h_name != NULL) {
		/* Object literal set/get functions have a name (property
		 * name) but must not have a lexical name binding, see
		 * test-bug-getset-func-name.js.
		 */
		DUK_DDD(DUK_DDDPRINT("function expression with a name -> set NAMEBINDING"));
		DUK_HOBJECT_SET_NAMEBINDING((duk_hobject *) h_res);
	}
#endif

	if (func->is_strict) {
		DUK_DDD(DUK_DDDPRINT("function is strict -> set STRICT"));
		DUK_HOBJECT_SET_STRICT((duk_hobject *) h_res);
	}

	if (func->is_notail) {
		DUK_DDD(DUK_DDDPRINT("function is notail -> set NOTAIL"));
		DUK_HOBJECT_SET_NOTAIL((duk_hobject *) h_res);
	}

	if (func->is_constructable) {
		DUK_DDD(DUK_DDDPRINT("function is constructable -> set CONSTRUCTABLE"));
		DUK_HOBJECT_SET_CONSTRUCTABLE((duk_hobject *) h_res);
	}

	/*
	 *  Build function fixed size 'data' buffer, which contains bytecode,
	 *  constants, and inner function references.
	 *
	 *  During the building phase 'data' is reachable but incomplete.
	 *  Only incref's occur during building (no refzero or GC happens),
	 *  so the building process is atomic.
	 */

	consts_count = duk_hobject_get_length(thr, func->h_consts);
	funcs_count = duk_hobject_get_length(thr, func->h_funcs) / 3;
	code_count = DUK_BW_GET_SIZE(thr, &func->bw_code) / sizeof(duk_compiler_instr);
	code_size = code_count * sizeof(duk_instr_t);

	data_size = consts_count * sizeof(duk_tval) + funcs_count * sizeof(duk_hobject *) + code_size;

	DUK_DDD(DUK_DDDPRINT("consts_count=%ld, funcs_count=%ld, code_size=%ld -> "
	                     "data_size=%ld*%ld + %ld*%ld + %ld = %ld",
	                     (long) consts_count,
	                     (long) funcs_count,
	                     (long) code_size,
	                     (long) consts_count,
	                     (long) sizeof(duk_tval),
	                     (long) funcs_count,
	                     (long) sizeof(duk_hobject *),
	                     (long) code_size,
	                     (long) data_size));

	duk_push_fixed_buffer_nozero(thr, data_size);
	h_data = (duk_hbuffer_fixed *) (void *) duk_known_hbuffer(thr, -1);

	DUK_HCOMPFUNC_SET_DATA(thr->heap, h_res, (duk_hbuffer *) h_data);
	DUK_HEAPHDR_INCREF(thr, h_data);

	p_const = (duk_tval *) (void *) DUK_HBUFFER_FIXED_GET_DATA_PTR(thr->heap, h_data);
	for (i = 0; i < consts_count; i++) {
		DUK_ASSERT(i <= DUK_UARRIDX_MAX); /* const limits */
		tv = duk_hobject_find_array_entry_tval_ptr(thr->heap, func->h_consts, (duk_uarridx_t) i);
		DUK_ASSERT(tv != NULL);
		DUK_TVAL_SET_TVAL(p_const, tv);
		p_const++;
		DUK_TVAL_INCREF(thr, tv); /* may be a string constant */

		DUK_DDD(DUK_DDDPRINT("constant: %!T", (duk_tval *) tv));
	}

	p_func = (duk_hobject **) p_const;
	DUK_HCOMPFUNC_SET_FUNCS(thr->heap, h_res, p_func);
	for (i = 0; i < funcs_count; i++) {
		duk_hobject *h;
		DUK_ASSERT(i * 3 <= DUK_UARRIDX_MAX); /* func limits */
		tv = duk_hobject_find_array_entry_tval_ptr(thr->heap, func->h_funcs, (duk_uarridx_t) (i * 3));
		DUK_ASSERT(tv != NULL);
		DUK_ASSERT(DUK_TVAL_IS_OBJECT(tv));
		h = DUK_TVAL_GET_OBJECT(tv);
		DUK_ASSERT(h != NULL);
		DUK_ASSERT(DUK_HOBJECT_IS_COMPFUNC(h));
		*p_func++ = h;
		DUK_HOBJECT_INCREF(thr, h);

		DUK_DDD(DUK_DDDPRINT("inner function: %p -> %!iO", (void *) h, (duk_heaphdr *) h));
	}

	p_instr = (duk_instr_t *) p_func;
	DUK_HCOMPFUNC_SET_BYTECODE(thr->heap, h_res, p_instr);

	/* copy bytecode instructions one at a time */
	q_instr = (duk_compiler_instr *) (void *) DUK_BW_GET_BASEPTR(thr, &func->bw_code);
	for (i = 0; i < code_count; i++) {
		p_instr[i] = q_instr[i].ins;
	}
	/* Note: 'q_instr' is still used below */

	DUK_ASSERT((duk_uint8_t *) (p_instr + code_count) == DUK_HBUFFER_FIXED_GET_DATA_PTR(thr->heap, h_data) + data_size);

	duk_pop(thr); /* 'data' (and everything in it) is reachable through h_res now */

	/*
	 *  Init non-property result fields
	 *
	 *  'nregs' controls how large a register frame is allocated.
	 *
	 *  'nargs' controls how many formal arguments are written to registers:
	 *  r0, ... r(nargs-1).  The remaining registers are initialized to
	 *  undefined.
	 */

	DUK_ASSERT(func->temp_max >= 0);
	h_res->nregs = (duk_uint16_t) func->temp_max;
	h_res->nargs = (duk_uint16_t) duk_hobject_get_length(thr, func->h_argnames);
	DUK_ASSERT(h_res->nregs >= h_res->nargs); /* pass2 allocation handles this */
#if defined(DUK_USE_DEBUGGER_SUPPORT)
	h_res->start_line = (duk_uint32_t) func->min_line;
	h_res->end_line = (duk_uint32_t) func->max_line;
#endif

	/*
	 *  Init object properties
	 *
	 *  Properties should be added in decreasing order of access frequency.
	 *  (Not very critical for function templates.)
	 */

	DUK_DDD(DUK_DDDPRINT("init function properties"));

	/* [ ... res ] */

	/* _Varmap: omitted if function is guaranteed not to do a slow path
	 * identifier access that might be caught by locally declared variables.
	 * The varmap can also be omitted if it turns out empty of actual
	 * register mappings after a cleanup.  When debugging is enabled, we
	 * always need the varmap to be able to lookup variables at any point.
	 */

#if defined(DUK_USE_DEBUGGER_SUPPORT)
	DUK_DD(DUK_DDPRINT("keeping _Varmap because debugger support is enabled"));
	keep_varmap = 1;
#else
	if (func->id_access_slow_own || /* directly uses slow accesses that may match own variables */
	    func->id_access_arguments || /* accesses 'arguments' directly */
	    func->may_direct_eval || /* may indirectly slow access through a direct eval */
	    funcs_count >
	        0) { /* has inner functions which may slow access (XXX: this can be optimized by looking at the inner functions) */
		DUK_DD(DUK_DDPRINT("keeping _Varmap because of direct eval, slow path access that may match local variables, or "
		                   "presence of inner functions"));
		keep_varmap = 1;
	} else {
		DUK_DD(DUK_DDPRINT("dropping _Varmap"));
		keep_varmap = 0;
	}
#endif

	if (keep_varmap) {
		duk_int_t num_used;
		duk_dup(thr, func->varmap_idx);
		num_used = duk__cleanup_varmap(comp_ctx);
		DUK_DDD(DUK_DDDPRINT("cleaned up varmap: %!T (num_used=%ld)", (duk_tval *) duk_get_tval(thr, -1), (long) num_used));

		if (num_used > 0) {
			duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_INT_VARMAP, DUK_PROPDESC_FLAGS_NONE);
		} else {
			DUK_DD(DUK_DDPRINT("varmap is empty after cleanup -> no need to add"));
			duk_pop(thr);
		}
	}

	/* _Formals: omitted if function is guaranteed not to need a (non-strict)
	 * arguments object, and _Formals.length matches nargs exactly.
	 *
	 * Non-arrow functions can't see an outer function's 'argument' binding
	 * (because they have their own), but arrow functions can.  When arrow
	 * functions are added, this condition would need to be added:
	 *     inner_arrow_funcs_count > 0   inner arrow functions may access 'arguments'
	 */
#if defined(DUK_USE_DEBUGGER_SUPPORT)
	DUK_DD(DUK_DDPRINT("keeping _Formals because debugger support is enabled"));
	keep_formals = 1;
#else
	formals_length = duk_get_length(thr, func->argnames_idx);
	if (formals_length != (duk_size_t) h_res->nargs) {
		/* Nargs not enough for closure .length: keep _Formals regardless
		 * of its length.  Shouldn't happen in practice at the moment.
		 */
		DUK_DD(DUK_DDPRINT("keeping _Formals because _Formals.length != nargs"));
		keep_formals = 1;
	} else if ((func->id_access_arguments || func->may_direct_eval) && (formals_length > 0)) {
		/* Direct eval (may access 'arguments') or accesses 'arguments'
		 * explicitly: keep _Formals unless it is zero length.
		 */
		DUK_DD(DUK_DDPRINT(
		    "keeping _Formals because of direct eval or explicit access to 'arguments', and _Formals.length != 0"));
		keep_formals = 1;
	} else {
		DUK_DD(DUK_DDPRINT("omitting _Formals, nargs matches _Formals.length, so no properties added"));
		keep_formals = 0;
	}
#endif

	if (keep_formals) {
		duk_dup(thr, func->argnames_idx);
		duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_INT_FORMALS, DUK_PROPDESC_FLAGS_NONE);
	}

	/* name */
#if defined(DUK_USE_FUNC_NAME_PROPERTY)
	if (func->h_name) {
		duk_push_hstring(thr, func->h_name);
		DUK_DD(DUK_DDPRINT("setting function template .name to %!T", duk_get_tval(thr, -1)));
		duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_NAME, DUK_PROPDESC_FLAGS_NONE);
	}
#endif /* DUK_USE_FUNC_NAME_PROPERTY */

	/* _Source */
#if defined(DUK_USE_NONSTD_FUNC_SOURCE_PROPERTY)
	if (0) {
		/* XXX: Currently function source code is not stored, as it is not
		 * required by the standard.  Source code should not be stored by
		 * default (user should enable it explicitly), and the source should
		 * probably be compressed with a trivial text compressor; average
		 * compression of 20-30% is quite easy to achieve even with a trivial
		 * compressor (RLE + backwards lookup).
		 *
		 * Debugging needs source code to be useful: sometimes input code is
		 * not found in files as it may be generated and then eval()'d, given
		 * by dynamic C code, etc.
		 *
		 * Other issues:
		 *
		 *   - Need tokenizer indices for start and end to substring
		 *   - Always normalize function declaration part?
		 *   - If we keep _Formals, only need to store body
		 */

		/*
		 *  For global or eval code this is straightforward.  For functions
		 *  created with the Function constructor we only get the source for
		 *  the body and must manufacture the "function ..." part.
		 *
		 *  For instance, for constructed functions (v8):
		 *
		 *    > a = new Function("foo", "bar", "print(foo)");
		 *    [Function]
		 *    > a.toString()
		 *    'function anonymous(foo,bar) {\nprint(foo)\n}'
		 *
		 *  Similarly for e.g. getters (v8):
		 *
		 *    > x = { get a(foo,bar) { print(foo); } }
		 *    { a: [Getter] }
		 *    > Object.getOwnPropertyDescriptor(x, 'a').get.toString()
		 *    'function a(foo,bar) { print(foo); }'
		 */

#if 0
		duk_push_literal(thr, "XXX");
		duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_INT_SOURCE, DUK_PROPDESC_FLAGS_NONE);
#endif
	}
#endif /* DUK_USE_NONSTD_FUNC_SOURCE_PROPERTY */

	/* _Pc2line */
#if defined(DUK_USE_PC2LINE)
	if (1) {
		/*
		 *  Size-optimized pc->line mapping.
		 */

		DUK_ASSERT(code_count <= DUK_COMPILER_MAX_BYTECODE_LENGTH);
		duk_hobject_pc2line_pack(thr, q_instr, (duk_uint_fast32_t) code_count); /* -> pushes fixed buffer */
		duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_INT_PC2LINE, DUK_PROPDESC_FLAGS_NONE);

		/* XXX: if assertions enabled, walk through all valid PCs
		 * and check line mapping.
		 */
	}
#endif /* DUK_USE_PC2LINE */

	/* fileName */
#if defined(DUK_USE_FUNC_FILENAME_PROPERTY)
	if (comp_ctx->h_filename) {
		/*
		 *  Source filename (or equivalent), for identifying thrown errors.
		 */

		duk_push_hstring(thr, comp_ctx->h_filename);
		duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_FILE_NAME, DUK_PROPDESC_FLAGS_NONE);
	}
#endif

	DUK_DD(DUK_DDPRINT("converted function: %!ixT", (duk_tval *) duk_get_tval(thr, -1)));

	/*
	 *  Compact the function template.
	 */

	duk_compact_m1(thr);

	/*
	 *  Debug dumping
	 */

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 2)
	{
		duk_hcompfunc *h;
		duk_instr_t *p, *p_start, *p_end;

		h = (duk_hcompfunc *) duk_get_hobject(thr, -1);
		p_start = (duk_instr_t *) DUK_HCOMPFUNC_GET_CODE_BASE(thr->heap, h);
		p_end = (duk_instr_t *) DUK_HCOMPFUNC_GET_CODE_END(thr->heap, h);

		p = p_start;
		while (p < p_end) {
			DUK_DDD(DUK_DDDPRINT("BC %04ld: %!I        ; 0x%08lx op=%ld (%!X) a=%ld b=%ld c=%ld",
			                     (long) (p - p_start),
			                     (duk_instr_t) (*p),
			                     (unsigned long) (*p),
			                     (long) DUK_DEC_OP(*p),
			                     (long) DUK_DEC_OP(*p),
			                     (long) DUK_DEC_A(*p),
			                     (long) DUK_DEC_B(*p),
			                     (long) DUK_DEC_C(*p)));
			p++;
		}
	}
#endif
}

/*
 *  Code emission helpers
 *
 *  Some emission helpers understand the range of target and source reg/const
 *  values and automatically emit shuffling code if necessary.  This is the
 *  case when the slot in question (A, B, C) is used in the standard way and
 *  for opcodes the emission helpers explicitly understand (like DUK_OP_MPUTOBJ).
 *
 *  The standard way is that:
 *    - slot A is a target register
 *    - slot B is a source register/constant
 *    - slot C is a source register/constant
 *
 *  If a slot is used in a non-standard way the caller must indicate this
 *  somehow.  If a slot is used as a target instead of a source (or vice
 *  versa), this can be indicated with a flag to trigger proper shuffling
 *  (e.g. DUK__EMIT_FLAG_B_IS_TARGET).  If the value in the slot is not
 *  register/const related at all, the caller must ensure that the raw value
 *  fits into the corresponding slot so as to not trigger shuffling.  The
 *  caller must set a "no shuffle" flag to ensure compilation fails if
 *  shuffling were to be triggered because of an internal error.
 *
 *  For slots B and C the raw slot size is 9 bits but one bit is reserved for
 *  the reg/const indicator.  To use the full 9-bit range for a raw value,
 *  shuffling must be disabled with the DUK__EMIT_FLAG_NO_SHUFFLE_{B,C} flag.
 *  Shuffling is only done for A, B, and C slots, not the larger BC or ABC slots.
 *
 *  There is call handling specific understanding in the A-B-C emitter to
 *  convert call setup and call instructions into indirect ones if necessary.
 */

/* Code emission flags, passed in the 'opcode' field.  Opcode + flags
 * fit into 16 bits for now, so use duk_small_uint_t.
 */
#define DUK__EMIT_FLAG_NO_SHUFFLE_A     (1 << 8)
#define DUK__EMIT_FLAG_NO_SHUFFLE_B     (1 << 9)
#define DUK__EMIT_FLAG_NO_SHUFFLE_C     (1 << 10)
#define DUK__EMIT_FLAG_A_IS_SOURCE      (1 << 11) /* slot A is a source (default: target) */
#define DUK__EMIT_FLAG_B_IS_TARGET      (1 << 12) /* slot B is a target (default: source) */
#define DUK__EMIT_FLAG_C_IS_TARGET      (1 << 13) /* slot C is a target (default: source) */
#define DUK__EMIT_FLAG_BC_REGCONST      (1 << 14) /* slots B and C are reg/const */
#define DUK__EMIT_FLAG_RESERVE_JUMPSLOT (1 << 15) /* reserve a jumpslot after instr before target spilling, used for NEXTENUM */

/* XXX: macro smaller than call? */
DUK_LOCAL duk_int_t duk__get_current_pc(duk_compiler_ctx *comp_ctx) {
	duk_compiler_func *func;
	func = &comp_ctx->curr_func;
	return (duk_int_t) (DUK_BW_GET_SIZE(comp_ctx->thr, &func->bw_code) / sizeof(duk_compiler_instr));
}

DUK_LOCAL duk_compiler_instr *duk__get_instr_ptr(duk_compiler_ctx *comp_ctx, duk_int_t pc) {
	DUK_ASSERT(pc >= 0);
	DUK_ASSERT((duk_size_t) pc <
	           (duk_size_t) (DUK_BW_GET_SIZE(comp_ctx->thr, &comp_ctx->curr_func.bw_code) / sizeof(duk_compiler_instr)));
	return ((duk_compiler_instr *) (void *) DUK_BW_GET_BASEPTR(comp_ctx->thr, &comp_ctx->curr_func.bw_code)) + pc;
}

/* emit instruction; could return PC but that's not needed in the majority
 * of cases.
 */
DUK_LOCAL void duk__emit(duk_compiler_ctx *comp_ctx, duk_instr_t ins) {
#if defined(DUK_USE_PC2LINE)
	duk_int_t line;
#endif
	duk_compiler_instr *instr;

	DUK_DDD(DUK_DDDPRINT("duk__emit: 0x%08lx curr_token.start_line=%ld prev_token.start_line=%ld pc=%ld --> %!I",
	                     (unsigned long) ins,
	                     (long) comp_ctx->curr_token.start_line,
	                     (long) comp_ctx->prev_token.start_line,
	                     (long) duk__get_current_pc(comp_ctx),
	                     (duk_instr_t) ins));

	instr = (duk_compiler_instr *) (void *) DUK_BW_ENSURE_GETPTR(comp_ctx->thr,
	                                                             &comp_ctx->curr_func.bw_code,
	                                                             sizeof(duk_compiler_instr));
	DUK_BW_ADD_PTR(comp_ctx->thr, &comp_ctx->curr_func.bw_code, sizeof(duk_compiler_instr));

#if defined(DUK_USE_PC2LINE)
	/* The line number tracking is a bit inconsistent right now, which
	 * affects debugger accuracy.  Mostly call sites emit opcodes when
	 * they have parsed a token (say a terminating semicolon) and called
	 * duk__advance().  In this case the line number of the previous
	 * token is the most accurate one (except in prologue where
	 * prev_token.start_line is 0).  This is probably not 100% correct
	 * right now.
	 */
	/* approximation, close enough */
	line = comp_ctx->prev_token.start_line;
	if (line == 0) {
		line = comp_ctx->curr_token.start_line;
	}
#endif

	instr->ins = ins;
#if defined(DUK_USE_PC2LINE)
	instr->line = (duk_uint32_t) line;
#endif
#if defined(DUK_USE_DEBUGGER_SUPPORT)
	if (line < comp_ctx->curr_func.min_line) {
		comp_ctx->curr_func.min_line = line;
	}
	if (line > comp_ctx->curr_func.max_line) {
		comp_ctx->curr_func.max_line = line;
	}
#endif

	/* Limit checks for bytecode byte size and line number. */
	if (DUK_UNLIKELY(DUK_BW_GET_SIZE(comp_ctx->thr, &comp_ctx->curr_func.bw_code) > DUK_USE_ESBC_MAX_BYTES)) {
		goto fail_bc_limit;
	}
#if defined(DUK_USE_PC2LINE) && defined(DUK_USE_ESBC_LIMITS)
#if defined(DUK_USE_BUFLEN16)
	/* Buffer length is bounded to 0xffff automatically, avoid compile warning. */
	if (DUK_UNLIKELY(line > DUK_USE_ESBC_MAX_LINENUMBER)) {
		goto fail_bc_limit;
	}
#else
	if (DUK_UNLIKELY(line > DUK_USE_ESBC_MAX_LINENUMBER)) {
		goto fail_bc_limit;
	}
#endif
#endif

	return;

fail_bc_limit:
	DUK_ERROR_RANGE(comp_ctx->thr, DUK_STR_BYTECODE_LIMIT);
	DUK_WO_NORETURN(return;);
}

/* Update function min/max line from current token.  Needed to improve
 * function line range information for debugging, so that e.g. opening
 * curly brace is covered by line range even when no opcodes are emitted
 * for the line containing the brace.
 */
DUK_LOCAL void duk__update_lineinfo_currtoken(duk_compiler_ctx *comp_ctx) {
#if defined(DUK_USE_DEBUGGER_SUPPORT)
	duk_int_t line;

	line = comp_ctx->curr_token.start_line;
	if (line == 0) {
		return;
	}
	if (line < comp_ctx->curr_func.min_line) {
		comp_ctx->curr_func.min_line = line;
	}
	if (line > comp_ctx->curr_func.max_line) {
		comp_ctx->curr_func.max_line = line;
	}
#else
	DUK_UNREF(comp_ctx);
#endif
}

DUK_LOCAL void duk__emit_op_only(duk_compiler_ctx *comp_ctx, duk_small_uint_t op) {
	duk__emit(comp_ctx, DUK_ENC_OP_ABC(op, 0));
}

/* Important main primitive. */
DUK_LOCAL void duk__emit_a_b_c(duk_compiler_ctx *comp_ctx,
                               duk_small_uint_t op_flags,
                               duk_regconst_t a,
                               duk_regconst_t b,
                               duk_regconst_t c) {
	duk_instr_t ins = 0;
	duk_int_t a_out = -1;
	duk_int_t b_out = -1;
	duk_int_t c_out = -1;
	duk_int_t tmp;
	duk_small_uint_t op = op_flags & 0xffU;

	DUK_DDD(DUK_DDDPRINT("emit: op_flags=%04lx, a=%ld, b=%ld, c=%ld", (unsigned long) op_flags, (long) a, (long) b, (long) c));

	/* We could rely on max temp/const checks: if they don't exceed BC
	 * limit, nothing here can either (just asserts would be enough).
	 * Currently we check for the limits, which provides additional
	 * protection against creating invalid bytecode due to compiler
	 * bugs.
	 */

	DUK_ASSERT_DISABLE((op_flags & 0xff) >= DUK_BC_OP_MIN); /* unsigned */
	DUK_ASSERT((op_flags & 0xff) <= DUK_BC_OP_MAX);
	DUK_ASSERT(DUK__ISREG(a));
	DUK_ASSERT(b != -1); /* Not 'none'. */
	DUK_ASSERT(c != -1); /* Not 'none'. */

	/* Input shuffling happens before the actual operation, while output
	 * shuffling happens afterwards.  Output shuffling decisions are still
	 * made at the same time to reduce branch clutter; output shuffle decisions
	 * are recorded into X_out variables.
	 */

	/* Slot A: currently no support for reg/const. */

#if defined(DUK_USE_SHUFFLE_TORTURE)
	if (a <= DUK_BC_A_MAX && (op_flags & DUK__EMIT_FLAG_NO_SHUFFLE_A)) {
#else
	if (a <= DUK_BC_A_MAX) {
#endif
		;
	} else if (op_flags & DUK__EMIT_FLAG_NO_SHUFFLE_A) {
		DUK_D(DUK_DPRINT("out of regs: 'a' (reg) needs shuffling but shuffle prohibited, a: %ld", (long) a));
		goto error_outofregs;
	} else if (a <= DUK_BC_BC_MAX) {
		comp_ctx->curr_func.needs_shuffle = 1;
		tmp = comp_ctx->curr_func.shuffle1;
		if (op_flags & DUK__EMIT_FLAG_A_IS_SOURCE) {
			duk__emit(comp_ctx, DUK_ENC_OP_A_BC(DUK_OP_LDREG, tmp, a));
		} else {
			/* Output shuffle needed after main operation */
			a_out = a;

			/* The DUK_OP_CSVAR output shuffle assumes shuffle registers are
			 * consecutive.
			 */
			DUK_ASSERT((comp_ctx->curr_func.shuffle1 == 0 && comp_ctx->curr_func.shuffle2 == 0) ||
			           (comp_ctx->curr_func.shuffle2 == comp_ctx->curr_func.shuffle1 + 1));
			if (op == DUK_OP_CSVAR) {
				/* For CSVAR the limit is one smaller because output shuffle
				 * must be able to express 'a + 1' in BC.
				 */
				if (a + 1 > DUK_BC_BC_MAX) {
					goto error_outofregs;
				}
			}
		}
		a = tmp;
	} else {
		DUK_D(DUK_DPRINT("out of regs: 'a' (reg) needs shuffling but does not fit into BC, a: %ld", (long) a));
		goto error_outofregs;
	}

	/* Slot B: reg/const support, mapped to bit 0 of opcode. */

	if ((b & DUK__CONST_MARKER) != 0) {
		DUK_ASSERT((op_flags & DUK__EMIT_FLAG_NO_SHUFFLE_B) == 0);
		DUK_ASSERT((op_flags & DUK__EMIT_FLAG_B_IS_TARGET) == 0);
		b = b & ~DUK__CONST_MARKER;
#if defined(DUK_USE_SHUFFLE_TORTURE)
		if (0) {
#else
		if (b <= 0xff) {
#endif
			if (op_flags & DUK__EMIT_FLAG_BC_REGCONST) {
				/* Opcode follows B/C reg/const convention. */
				DUK_ASSERT((op & 0x01) == 0);
				ins |= DUK_ENC_OP_A_B_C(0x01, 0, 0, 0); /* const flag for B */
			} else {
				DUK_D(DUK_DPRINT("B is const, opcode is not B/C reg/const: %x", op_flags));
			}
		} else if (b <= DUK_BC_BC_MAX) {
			comp_ctx->curr_func.needs_shuffle = 1;
			tmp = comp_ctx->curr_func.shuffle2;
			duk__emit(comp_ctx, DUK_ENC_OP_A_BC(DUK_OP_LDCONST, tmp, b));
			b = tmp;
		} else {
			DUK_D(DUK_DPRINT("out of regs: 'b' (const) needs shuffling but does not fit into BC, b: %ld", (long) b));
			goto error_outofregs;
		}
	} else {
#if defined(DUK_USE_SHUFFLE_TORTURE)
		if (b <= 0xff && (op_flags & DUK__EMIT_FLAG_NO_SHUFFLE_B)) {
#else
		if (b <= 0xff) {
#endif
			;
		} else if (op_flags & DUK__EMIT_FLAG_NO_SHUFFLE_B) {
			if (b > DUK_BC_B_MAX) {
				/* Note: 0xff != DUK_BC_B_MAX */
				DUK_D(
				    DUK_DPRINT("out of regs: 'b' (reg) needs shuffling but shuffle prohibited, b: %ld", (long) b));
				goto error_outofregs;
			}
		} else if (b <= DUK_BC_BC_MAX) {
			comp_ctx->curr_func.needs_shuffle = 1;
			tmp = comp_ctx->curr_func.shuffle2;
			if (op_flags & DUK__EMIT_FLAG_B_IS_TARGET) {
				/* Output shuffle needed after main operation */
				b_out = b;
			}
			if (!(op_flags & DUK__EMIT_FLAG_B_IS_TARGET)) {
				if (op == DUK_OP_MPUTOBJ || op == DUK_OP_MPUTARR) {
					/* Special handling for MPUTOBJ/MPUTARR shuffling.
					 * For each, slot B identifies the first register of a range
					 * of registers, so normal shuffling won't work.  Instead,
					 * an indirect version of the opcode is used.
					 */
					DUK_ASSERT((op_flags & DUK__EMIT_FLAG_B_IS_TARGET) == 0);
					duk__emit_load_int32_noshuffle(comp_ctx, tmp, b);
					DUK_ASSERT(DUK_OP_MPUTOBJI == DUK_OP_MPUTOBJ + 1);
					DUK_ASSERT(DUK_OP_MPUTARRI == DUK_OP_MPUTARR + 1);
					op_flags++; /* indirect opcode follows direct */
				} else {
					duk__emit(comp_ctx, DUK_ENC_OP_A_BC(DUK_OP_LDREG, tmp, b));
				}
			}
			b = tmp;
		} else {
			DUK_D(DUK_DPRINT("out of regs: 'b' (reg) needs shuffling but does not fit into BC, b: %ld", (long) b));
			goto error_outofregs;
		}
	}

	/* Slot C: reg/const support, mapped to bit 1 of opcode. */

	if ((c & DUK__CONST_MARKER) != 0) {
		DUK_ASSERT((op_flags & DUK__EMIT_FLAG_NO_SHUFFLE_C) == 0);
		DUK_ASSERT((op_flags & DUK__EMIT_FLAG_C_IS_TARGET) == 0);
		c = c & ~DUK__CONST_MARKER;
#if defined(DUK_USE_SHUFFLE_TORTURE)
		if (0) {
#else
		if (c <= 0xff) {
#endif
			if (op_flags & DUK__EMIT_FLAG_BC_REGCONST) {
				/* Opcode follows B/C reg/const convention. */
				DUK_ASSERT((op & 0x02) == 0);
				ins |= DUK_ENC_OP_A_B_C(0x02, 0, 0, 0); /* const flag for C */
			} else {
				DUK_D(DUK_DPRINT("C is const, opcode is not B/C reg/const: %x", op_flags));
			}
		} else if (c <= DUK_BC_BC_MAX) {
			comp_ctx->curr_func.needs_shuffle = 1;
			tmp = comp_ctx->curr_func.shuffle3;
			duk__emit(comp_ctx, DUK_ENC_OP_A_BC(DUK_OP_LDCONST, tmp, c));
			c = tmp;
		} else {
			DUK_D(DUK_DPRINT("out of regs: 'c' (const) needs shuffling but does not fit into BC, c: %ld", (long) c));
			goto error_outofregs;
		}
	} else {
#if defined(DUK_USE_SHUFFLE_TORTURE)
		if (c <= 0xff && (op_flags & DUK__EMIT_FLAG_NO_SHUFFLE_C)) {
#else
		if (c <= 0xff) {
#endif
			;
		} else if (op_flags & DUK__EMIT_FLAG_NO_SHUFFLE_C) {
			if (c > DUK_BC_C_MAX) {
				/* Note: 0xff != DUK_BC_C_MAX */
				DUK_D(
				    DUK_DPRINT("out of regs: 'c' (reg) needs shuffling but shuffle prohibited, c: %ld", (long) c));
				goto error_outofregs;
			}
		} else if (c <= DUK_BC_BC_MAX) {
			comp_ctx->curr_func.needs_shuffle = 1;
			tmp = comp_ctx->curr_func.shuffle3;
			if (op_flags & DUK__EMIT_FLAG_C_IS_TARGET) {
				/* Output shuffle needed after main operation */
				c_out = c;
			} else {
				duk__emit(comp_ctx, DUK_ENC_OP_A_BC(DUK_OP_LDREG, tmp, c));
			}
			c = tmp;
		} else {
			DUK_D(DUK_DPRINT("out of regs: 'c' (reg) needs shuffling but does not fit into BC, c: %ld", (long) c));
			goto error_outofregs;
		}
	}

	/* Main operation */

	DUK_ASSERT(a >= DUK_BC_A_MIN);
	DUK_ASSERT(a <= DUK_BC_A_MAX);
	DUK_ASSERT(b >= DUK_BC_B_MIN);
	DUK_ASSERT(b <= DUK_BC_B_MAX);
	DUK_ASSERT(c >= DUK_BC_C_MIN);
	DUK_ASSERT(c <= DUK_BC_C_MAX);

	ins |= DUK_ENC_OP_A_B_C(op_flags & 0xff, a, b, c);
	duk__emit(comp_ctx, ins);

	/* NEXTENUM needs a jump slot right after the main instruction.
	 * When the JUMP is taken, output spilling is not needed so this
	 * workaround is possible.  The jump slot PC is exceptionally
	 * plumbed through comp_ctx to minimize call sites.
	 */
	if (op_flags & DUK__EMIT_FLAG_RESERVE_JUMPSLOT) {
		comp_ctx->emit_jumpslot_pc = duk__get_current_pc(comp_ctx);
		duk__emit_abc(comp_ctx, DUK_OP_JUMP, 0);
	}

	/* Output shuffling: only one output register is realistically possible.
	 *
	 * (Zero would normally be an OK marker value: if the target register
	 * was zero, it would never be shuffled.  But with DUK_USE_SHUFFLE_TORTURE
	 * this is no longer true, so use -1 as a marker instead.)
	 */

	if (a_out >= 0) {
		DUK_ASSERT(b_out < 0);
		DUK_ASSERT(c_out < 0);
		duk__emit(comp_ctx, DUK_ENC_OP_A_BC(DUK_OP_STREG, a, a_out));

		if (op == DUK_OP_CSVAR) {
			/* Special handling for CSVAR shuffling.  The variable lookup
			 * results in a <value, this binding> pair in successive
			 * registers so use two shuffle registers and two output
			 * loads.  (In practice this is dead code because temp/const
			 * limit is reached first.)
			 */
			duk__emit(comp_ctx, DUK_ENC_OP_A_BC(DUK_OP_STREG, a + 1, a_out + 1));
		}
	} else if (b_out >= 0) {
		DUK_ASSERT(a_out < 0);
		DUK_ASSERT(c_out < 0);
		duk__emit(comp_ctx, DUK_ENC_OP_A_BC(DUK_OP_STREG, b, b_out));
	} else if (c_out >= 0) {
		DUK_ASSERT(b_out < 0);
		DUK_ASSERT(c_out < 0);
		duk__emit(comp_ctx, DUK_ENC_OP_A_BC(DUK_OP_STREG, c, c_out));
	}

	return;

error_outofregs:
	DUK_ERROR_RANGE(comp_ctx->thr, DUK_STR_REG_LIMIT);
	DUK_WO_NORETURN(return;);
}

/* For many of the helpers below it'd be technically correct to add
 * "no shuffle" flags for parameters passed in as zero.  For example,
 * duk__emit_a_b() should call duk__emit_a_b_c() with C set to 0, and
 * DUK__EMIT_FLAG_NO_SHUFFLE_C added to op_flags.  However, since the
 * C value is 0, it'll never get shuffled so adding the flag is just
 * unnecessary additional code.  This is unfortunately not true for
 * "shuffle torture" mode which needs special handling.
 */

DUK_LOCAL void duk__emit_a_b(duk_compiler_ctx *comp_ctx, duk_small_uint_t op_flags, duk_regconst_t a, duk_regconst_t b) {
#if defined(DUK_USE_SHUFFLE_TORTURE)
	op_flags |= DUK__EMIT_FLAG_NO_SHUFFLE_C;
#endif
	duk__emit_a_b_c(comp_ctx, op_flags, a, b, 0);
}

DUK_LOCAL void duk__emit_b_c(duk_compiler_ctx *comp_ctx, duk_small_uint_t op_flags, duk_regconst_t b, duk_regconst_t c) {
#if defined(DUK_USE_SHUFFLE_TORTURE)
	op_flags |= DUK__EMIT_FLAG_NO_SHUFFLE_A;
#endif
	duk__emit_a_b_c(comp_ctx, op_flags, 0, b, c);
}

#if 0 /* unused */
DUK_LOCAL void duk__emit_a(duk_compiler_ctx *comp_ctx, int op_flags, int a) {
#if defined(DUK_USE_SHUFFLE_TORTURE)
	op_flags |= DUK__EMIT_FLAG_NO_SHUFFLE_B | DUK__EMIT_FLAG_NO_SHUFFLE_C;
#endif
	duk__emit_a_b_c(comp_ctx, op_flags, a, 0, 0);
}
#endif

#if 0 /* unused */
DUK_LOCAL void duk__emit_b(duk_compiler_ctx *comp_ctx, duk_small_uint_t op_flags, duk_regconst_t b) {
#if defined(DUK_USE_SHUFFLE_TORTURE)
	op_flags |= DUK__EMIT_FLAG_NO_SHUFFLE_A | DUK__EMIT_FLAG_NO_SHUFFLE_C;
#endif
	duk__emit_a_b_c(comp_ctx, op_flags, 0, b, 0);
}
#endif

DUK_LOCAL void duk__emit_a_bc(duk_compiler_ctx *comp_ctx, duk_small_uint_t op_flags, duk_regconst_t a, duk_regconst_t bc) {
	duk_instr_t ins;
	duk_int_t tmp;

	/* allow caller to give a const number with the DUK__CONST_MARKER */
	DUK_ASSERT(bc != -1); /* Not 'none'. */
	bc = bc & (~DUK__CONST_MARKER);

	DUK_ASSERT_DISABLE((op_flags & 0xff) >= DUK_BC_OP_MIN); /* unsigned */
	DUK_ASSERT((op_flags & 0xff) <= DUK_BC_OP_MAX);
	DUK_ASSERT(bc >= DUK_BC_BC_MIN);
	DUK_ASSERT(bc <= DUK_BC_BC_MAX);
	DUK_ASSERT((bc & DUK__CONST_MARKER) == 0);

	if (bc <= DUK_BC_BC_MAX) {
		;
	} else {
		/* No BC shuffling now. */
		goto error_outofregs;
	}

#if defined(DUK_USE_SHUFFLE_TORTURE)
	if (a <= DUK_BC_A_MAX && (op_flags & DUK__EMIT_FLAG_NO_SHUFFLE_A)) {
#else
	if (a <= DUK_BC_A_MAX) {
#endif
		ins = DUK_ENC_OP_A_BC(op_flags & 0xff, a, bc);
		duk__emit(comp_ctx, ins);
	} else if (op_flags & DUK__EMIT_FLAG_NO_SHUFFLE_A) {
		goto error_outofregs;
	} else if ((op_flags & 0xf0U) == DUK_OP_CALL0) {
		comp_ctx->curr_func.needs_shuffle = 1;
		tmp = comp_ctx->curr_func.shuffle1;
		duk__emit_load_int32_noshuffle(comp_ctx, tmp, a);
		op_flags |= DUK_BC_CALL_FLAG_INDIRECT;
		ins = DUK_ENC_OP_A_BC(op_flags & 0xff, tmp, bc);
		duk__emit(comp_ctx, ins);
	} else if (a <= DUK_BC_BC_MAX) {
		comp_ctx->curr_func.needs_shuffle = 1;
		tmp = comp_ctx->curr_func.shuffle1;
		ins = DUK_ENC_OP_A_BC(op_flags & 0xff, tmp, bc);
		if (op_flags & DUK__EMIT_FLAG_A_IS_SOURCE) {
			duk__emit(comp_ctx, DUK_ENC_OP_A_BC(DUK_OP_LDREG, tmp, a));
			duk__emit(comp_ctx, ins);
		} else {
			duk__emit(comp_ctx, ins);
			duk__emit(comp_ctx, DUK_ENC_OP_A_BC(DUK_OP_STREG, tmp, a));
		}
	} else {
		goto error_outofregs;
	}
	return;

error_outofregs:
	DUK_ERROR_RANGE(comp_ctx->thr, DUK_STR_REG_LIMIT);
	DUK_WO_NORETURN(return;);
}

DUK_LOCAL void duk__emit_bc(duk_compiler_ctx *comp_ctx, duk_small_uint_t op, duk_regconst_t bc) {
#if defined(DUK_USE_SHUFFLE_TORTURE)
	op |= DUK__EMIT_FLAG_NO_SHUFFLE_A;
#endif
	duk__emit_a_bc(comp_ctx, op, 0, bc);
}

DUK_LOCAL void duk__emit_abc(duk_compiler_ctx *comp_ctx, duk_small_uint_t op, duk_regconst_t abc) {
	duk_instr_t ins;

	DUK_ASSERT_DISABLE(op >= DUK_BC_OP_MIN); /* unsigned */
	DUK_ASSERT(op <= DUK_BC_OP_MAX);
	DUK_ASSERT_DISABLE(abc >= DUK_BC_ABC_MIN); /* unsigned */
	DUK_ASSERT(abc <= DUK_BC_ABC_MAX);
	DUK_ASSERT((abc & DUK__CONST_MARKER) == 0);
	DUK_ASSERT(abc != -1); /* Not 'none'. */

	if (abc <= DUK_BC_ABC_MAX) {
		;
	} else {
		goto error_outofregs;
	}
	ins = DUK_ENC_OP_ABC(op, abc);
	DUK_DDD(DUK_DDDPRINT("duk__emit_abc: 0x%08lx line=%ld pc=%ld op=%ld (%!X) abc=%ld (%!I)",
	                     (unsigned long) ins,
	                     (long) comp_ctx->curr_token.start_line,
	                     (long) duk__get_current_pc(comp_ctx),
	                     (long) op,
	                     (long) op,
	                     (long) abc,
	                     (duk_instr_t) ins));
	duk__emit(comp_ctx, ins);
	return;

error_outofregs:
	DUK_ERROR_RANGE(comp_ctx->thr, DUK_STR_REG_LIMIT);
	DUK_WO_NORETURN(return;);
}

DUK_LOCAL void duk__emit_load_int32_raw(duk_compiler_ctx *comp_ctx,
                                        duk_regconst_t reg,
                                        duk_int32_t val,
                                        duk_small_uint_t op_flags) {
	/* XXX: Shuffling support could be implemented here so that LDINT+LDINTX
	 * would only shuffle once (instead of twice).  The current code works
	 * though, and has a smaller compiler footprint.
	 */

	if ((val >= (duk_int32_t) DUK_BC_BC_MIN - (duk_int32_t) DUK_BC_LDINT_BIAS) &&
	    (val <= (duk_int32_t) DUK_BC_BC_MAX - (duk_int32_t) DUK_BC_LDINT_BIAS)) {
		DUK_DDD(DUK_DDDPRINT("emit LDINT to reg %ld for %ld", (long) reg, (long) val));
		duk__emit_a_bc(comp_ctx, DUK_OP_LDINT | op_flags, reg, (duk_regconst_t) (val + (duk_int32_t) DUK_BC_LDINT_BIAS));
	} else {
		duk_int32_t hi = val >> DUK_BC_LDINTX_SHIFT;
		duk_int32_t lo = val & ((((duk_int32_t) 1) << DUK_BC_LDINTX_SHIFT) - 1);
		DUK_ASSERT(lo >= 0);
		DUK_DDD(DUK_DDDPRINT("emit LDINT+LDINTX to reg %ld for %ld -> hi %ld, lo %ld",
		                     (long) reg,
		                     (long) val,
		                     (long) hi,
		                     (long) lo));
		duk__emit_a_bc(comp_ctx, DUK_OP_LDINT | op_flags, reg, (duk_regconst_t) (hi + (duk_int32_t) DUK_BC_LDINT_BIAS));
		duk__emit_a_bc(comp_ctx, DUK_OP_LDINTX | op_flags, reg, (duk_regconst_t) lo);
	}
}

DUK_LOCAL void duk__emit_load_int32(duk_compiler_ctx *comp_ctx, duk_regconst_t reg, duk_int32_t val) {
	duk__emit_load_int32_raw(comp_ctx, reg, val, 0 /*op_flags*/);
}

#if defined(DUK_USE_SHUFFLE_TORTURE)
/* Used by duk__emit*() calls so that we don't shuffle the loadints that
 * are needed to handle indirect opcodes.
 */
DUK_LOCAL void duk__emit_load_int32_noshuffle(duk_compiler_ctx *comp_ctx, duk_regconst_t reg, duk_int32_t val) {
	duk__emit_load_int32_raw(comp_ctx, reg, val, DUK__EMIT_FLAG_NO_SHUFFLE_A /*op_flags*/);
}
#else
DUK_LOCAL void duk__emit_load_int32_noshuffle(duk_compiler_ctx *comp_ctx, duk_regconst_t reg, duk_int32_t val) {
	/* When torture not enabled, can just use the same helper because
	 * 'reg' won't get spilled.
	 */
	DUK_ASSERT(reg <= DUK_BC_A_MAX);
	duk__emit_load_int32(comp_ctx, reg, val);
}
#endif

DUK_LOCAL void duk__emit_jump(duk_compiler_ctx *comp_ctx, duk_int_t target_pc) {
	duk_int_t curr_pc;
	duk_int_t offset;

	curr_pc = (duk_int_t) (DUK_BW_GET_SIZE(comp_ctx->thr, &comp_ctx->curr_func.bw_code) / sizeof(duk_compiler_instr));
	offset = (duk_int_t) target_pc - (duk_int_t) curr_pc - 1;
	DUK_ASSERT(offset + DUK_BC_JUMP_BIAS >= DUK_BC_ABC_MIN);
	DUK_ASSERT(offset + DUK_BC_JUMP_BIAS <= DUK_BC_ABC_MAX);
	duk__emit_abc(comp_ctx, DUK_OP_JUMP, (duk_regconst_t) (offset + DUK_BC_JUMP_BIAS));
}

DUK_LOCAL duk_int_t duk__emit_jump_empty(duk_compiler_ctx *comp_ctx) {
	duk_int_t ret;

	ret = duk__get_current_pc(comp_ctx); /* useful for patching jumps later */
	duk__emit_op_only(comp_ctx, DUK_OP_JUMP);
	return ret;
}

/* Insert an empty jump in the middle of code emitted earlier.  This is
 * currently needed for compiling for-in.
 */
DUK_LOCAL void duk__insert_jump_entry(duk_compiler_ctx *comp_ctx, duk_int_t jump_pc) {
#if defined(DUK_USE_PC2LINE)
	duk_int_t line;
#endif
	duk_compiler_instr *instr;
	duk_size_t offset;

	DUK_ASSERT(jump_pc >= 0);
	offset = (duk_size_t) jump_pc * sizeof(duk_compiler_instr);
	instr = (duk_compiler_instr *) (void *)
	    DUK_BW_INSERT_ENSURE_AREA(comp_ctx->thr, &comp_ctx->curr_func.bw_code, offset, sizeof(duk_compiler_instr));

#if defined(DUK_USE_PC2LINE)
	line = comp_ctx->curr_token.start_line; /* approximation, close enough */
#endif
	instr->ins = DUK_ENC_OP_ABC(DUK_OP_JUMP, 0);
#if defined(DUK_USE_PC2LINE)
	instr->line = (duk_uint32_t) line;
#endif

	DUK_BW_ADD_PTR(comp_ctx->thr, &comp_ctx->curr_func.bw_code, sizeof(duk_compiler_instr));
	if (DUK_UNLIKELY(DUK_BW_GET_SIZE(comp_ctx->thr, &comp_ctx->curr_func.bw_code) > DUK_USE_ESBC_MAX_BYTES)) {
		goto fail_bc_limit;
	}
	return;

fail_bc_limit:
	DUK_ERROR_RANGE(comp_ctx->thr, DUK_STR_BYTECODE_LIMIT);
	DUK_WO_NORETURN(return;);
}

/* Does not assume that jump_pc contains a DUK_OP_JUMP previously; this is intentional
 * to allow e.g. an INVALID opcode be overwritten with a JUMP (label management uses this).
 */
DUK_LOCAL void duk__patch_jump(duk_compiler_ctx *comp_ctx, duk_int_t jump_pc, duk_int_t target_pc) {
	duk_compiler_instr *instr;
	duk_int_t offset;

	/* allow negative PCs, behave as a no-op */
	if (jump_pc < 0) {
		DUK_DDD(
		    DUK_DDDPRINT("duk__patch_jump(): nop call, jump_pc=%ld (<0), target_pc=%ld", (long) jump_pc, (long) target_pc));
		return;
	}
	DUK_ASSERT(jump_pc >= 0);

	/* XXX: range assert */
	instr = duk__get_instr_ptr(comp_ctx, jump_pc);
	DUK_ASSERT(instr != NULL);

	/* XXX: range assert */
	offset = target_pc - jump_pc - 1;

	instr->ins = DUK_ENC_OP_ABC(DUK_OP_JUMP, offset + DUK_BC_JUMP_BIAS);
	DUK_DDD(DUK_DDDPRINT("duk__patch_jump(): jump_pc=%ld, target_pc=%ld, offset=%ld",
	                     (long) jump_pc,
	                     (long) target_pc,
	                     (long) offset));
}

DUK_LOCAL void duk__patch_jump_here(duk_compiler_ctx *comp_ctx, duk_int_t jump_pc) {
	duk__patch_jump(comp_ctx, jump_pc, duk__get_current_pc(comp_ctx));
}

DUK_LOCAL void duk__patch_trycatch(duk_compiler_ctx *comp_ctx,
                                   duk_int_t ldconst_pc,
                                   duk_int_t trycatch_pc,
                                   duk_regconst_t reg_catch,
                                   duk_regconst_t const_varname,
                                   duk_small_uint_t flags) {
	duk_compiler_instr *instr;

	DUK_ASSERT(DUK__ISREG(reg_catch));

	instr = duk__get_instr_ptr(comp_ctx, ldconst_pc);
	DUK_ASSERT(DUK_DEC_OP(instr->ins) == DUK_OP_LDCONST);
	DUK_ASSERT(instr != NULL);
	if (const_varname & DUK__CONST_MARKER) {
		/* Have a catch variable. */
		const_varname = const_varname & (~DUK__CONST_MARKER);
		if (reg_catch > DUK_BC_BC_MAX || const_varname > DUK_BC_BC_MAX) {
			/* Catch attempts to use out-of-range reg/const.  Without this
			 * check Duktape 0.12.0 could generate invalid code which caused
			 * an assert failure on execution.  This error is triggered e.g.
			 * for functions with a lot of constants and a try-catch statement.
			 * Shuffling or opcode semantics change is needed to fix the issue.
			 * See: test-bug-trycatch-many-constants.js.
			 */
			DUK_D(DUK_DPRINT("failed to patch trycatch: flags=%ld, reg_catch=%ld, const_varname=%ld (0x%08lx)",
			                 (long) flags,
			                 (long) reg_catch,
			                 (long) const_varname,
			                 (long) const_varname));
			DUK_ERROR_RANGE(comp_ctx->thr, DUK_STR_REG_LIMIT);
			DUK_WO_NORETURN(return;);
		}
		instr->ins |= DUK_ENC_OP_A_BC(0, 0, const_varname);
	} else {
		/* No catch variable, e.g. a try-finally; replace LDCONST with
		 * NOP to avoid a bogus LDCONST.
		 */
		instr->ins = DUK_ENC_OP(DUK_OP_NOP);
	}

	instr = duk__get_instr_ptr(comp_ctx, trycatch_pc);
	DUK_ASSERT(instr != NULL);
	DUK_ASSERT_DISABLE(flags >= DUK_BC_A_MIN);
	DUK_ASSERT(flags <= DUK_BC_A_MAX);
	instr->ins = DUK_ENC_OP_A_BC(DUK_OP_TRYCATCH, flags, reg_catch);
}

DUK_LOCAL void duk__emit_if_false_skip(duk_compiler_ctx *comp_ctx, duk_regconst_t regconst) {
	duk_small_uint_t op;

	op = DUK__ISREG(regconst) ? DUK_OP_IFFALSE_R : DUK_OP_IFFALSE_C;
	duk__emit_bc(comp_ctx, op, regconst); /* helper will remove const flag */
}

DUK_LOCAL void duk__emit_if_true_skip(duk_compiler_ctx *comp_ctx, duk_regconst_t regconst) {
	duk_small_uint_t op;

	op = DUK__ISREG(regconst) ? DUK_OP_IFTRUE_R : DUK_OP_IFTRUE_C;
	duk__emit_bc(comp_ctx, op, regconst); /* helper will remove const flag */
}

DUK_LOCAL void duk__emit_invalid(duk_compiler_ctx *comp_ctx) {
	duk__emit_op_only(comp_ctx, DUK_OP_INVALID);
}

/*
 *  Peephole optimizer for finished bytecode.
 *
 *  Does not remove opcodes; currently only straightens out unconditional
 *  jump chains which are generated by several control structures.
 */

DUK_LOCAL void duk__peephole_optimize_bytecode(duk_compiler_ctx *comp_ctx) {
	duk_compiler_instr *bc;
	duk_small_uint_t iter;
	duk_int_t i, n;
	duk_int_t count_opt;

	bc = (duk_compiler_instr *) (void *) DUK_BW_GET_BASEPTR(comp_ctx->thr, &comp_ctx->curr_func.bw_code);
#if defined(DUK_USE_BUFLEN16)
	/* No need to assert, buffer size maximum is 0xffff. */
#else
	DUK_ASSERT((duk_size_t) DUK_BW_GET_SIZE(comp_ctx->thr, &comp_ctx->curr_func.bw_code) / sizeof(duk_compiler_instr) <=
	           (duk_size_t) DUK_INT_MAX); /* bytecode limits */
#endif
	n = (duk_int_t) (DUK_BW_GET_SIZE(comp_ctx->thr, &comp_ctx->curr_func.bw_code) / sizeof(duk_compiler_instr));

	for (iter = 0; iter < DUK_COMPILER_PEEPHOLE_MAXITER; iter++) {
		count_opt = 0;

		for (i = 0; i < n; i++) {
			duk_instr_t ins;
			duk_int_t target_pc1;
			duk_int_t target_pc2;

			ins = bc[i].ins;
			if (DUK_DEC_OP(ins) != DUK_OP_JUMP) {
				continue;
			}

			target_pc1 = i + 1 + (duk_int_t) DUK_DEC_ABC(ins) - (duk_int_t) DUK_BC_JUMP_BIAS;
			DUK_DDD(DUK_DDDPRINT("consider jump at pc %ld; target_pc=%ld", (long) i, (long) target_pc1));
			DUK_ASSERT(target_pc1 >= 0);
			DUK_ASSERT(target_pc1 < n);

			/* Note: if target_pc1 == i, we'll optimize a jump to itself.
			 * This does not need to be checked for explicitly; the case
			 * is rare and max iter breaks us out.
			 */

			ins = bc[target_pc1].ins;
			if (DUK_DEC_OP(ins) != DUK_OP_JUMP) {
				continue;
			}

			target_pc2 = target_pc1 + 1 + (duk_int_t) DUK_DEC_ABC(ins) - (duk_int_t) DUK_BC_JUMP_BIAS;

			DUK_DDD(DUK_DDDPRINT("optimizing jump at pc %ld; old target is %ld -> new target is %ld",
			                     (long) i,
			                     (long) target_pc1,
			                     (long) target_pc2));

			bc[i].ins = DUK_ENC_OP_ABC(DUK_OP_JUMP, target_pc2 - (i + 1) + DUK_BC_JUMP_BIAS);

			count_opt++;
		}

		DUK_DD(DUK_DDPRINT("optimized %ld jumps on peephole round %ld", (long) count_opt, (long) (iter + 1)));

		if (count_opt == 0) {
			break;
		}
	}
}

/*
 *  Intermediate value helpers
 */

/* Flags for intermediate value coercions.  A flag for using a forced reg
 * is not needed, the forced_reg argument suffices and generates better
 * code (it is checked as it is used).
 */
/* XXX: DUK__IVAL_FLAG_REQUIRE_SHORT is passed but not currently implemented
 * by ispec/ivalue operations.
 */
#define DUK__IVAL_FLAG_ALLOW_CONST   (1 << 0) /* allow a constant to be returned */
#define DUK__IVAL_FLAG_REQUIRE_TEMP  (1 << 1) /* require a (mutable) temporary as a result (or a const if allowed) */
#define DUK__IVAL_FLAG_REQUIRE_SHORT (1 << 2) /* require a short (8-bit) reg/const which fits into bytecode B/C slot */

/* XXX: some code might benefit from DUK__SETTEMP_IFTEMP(thr,x) */

#if 0 /* enable manually for dumping */
#define DUK__DUMP_ISPEC(compctx, ispec) \
	do { \
		duk__dump_ispec((compctx), (ispec)); \
	} while (0)
#define DUK__DUMP_IVALUE(compctx, ivalue) \
	do { \
		duk__dump_ivalue((compctx), (ivalue)); \
	} while (0)

DUK_LOCAL void duk__dump_ispec(duk_compiler_ctx *comp_ctx, duk_ispec *x) {
	DUK_D(DUK_DPRINT("ispec dump: t=%ld regconst=0x%08lx, valstack_idx=%ld, value=%!T",
	                 (long) x->t, (unsigned long) x->regconst, (long) x->valstack_idx,
	                 duk_get_tval(comp_ctx->thr, x->valstack_idx)));
}
DUK_LOCAL void duk__dump_ivalue(duk_compiler_ctx *comp_ctx, duk_ivalue *x) {
	DUK_D(DUK_DPRINT("ivalue dump: t=%ld op=%ld "
	                 "x1={t=%ld regconst=0x%08lx valstack_idx=%ld value=%!T} "
	                 "x2={t=%ld regconst=0x%08lx valstack_idx=%ld value=%!T}",
		         (long) x->t, (long) x->op,
	                 (long) x->x1.t, (unsigned long) x->x1.regconst, (long) x->x1.valstack_idx,
	                 duk_get_tval(comp_ctx->thr, x->x1.valstack_idx),
	                 (long) x->x2.t, (unsigned long) x->x2.regconst, (long) x->x2.valstack_idx,
	                 duk_get_tval(comp_ctx->thr, x->x2.valstack_idx)));
}
#else
#define DUK__DUMP_ISPEC(comp_ctx, x) \
	do { \
	} while (0)
#define DUK__DUMP_IVALUE(comp_ctx, x) \
	do { \
	} while (0)
#endif

DUK_LOCAL void duk__ivalue_regconst(duk_ivalue *x, duk_regconst_t regconst) {
	x->t = DUK_IVAL_PLAIN;
	x->x1.t = DUK_ISPEC_REGCONST;
	x->x1.regconst = regconst;
}

DUK_LOCAL void duk__ivalue_plain_fromstack(duk_compiler_ctx *comp_ctx, duk_ivalue *x) {
	x->t = DUK_IVAL_PLAIN;
	x->x1.t = DUK_ISPEC_VALUE;
	duk_replace(comp_ctx->thr, x->x1.valstack_idx);
}

DUK_LOCAL void duk__ivalue_var_fromstack(duk_compiler_ctx *comp_ctx, duk_ivalue *x) {
	x->t = DUK_IVAL_VAR;
	x->x1.t = DUK_ISPEC_VALUE;
	duk_replace(comp_ctx->thr, x->x1.valstack_idx);
}

DUK_LOCAL_DECL void duk__ivalue_var_hstring(duk_compiler_ctx *comp_ctx, duk_ivalue *x, duk_hstring *h) {
	DUK_ASSERT(h != NULL);
	duk_push_hstring(comp_ctx->thr, h);
	duk__ivalue_var_fromstack(comp_ctx, x);
}

DUK_LOCAL void duk__copy_ispec(duk_compiler_ctx *comp_ctx, duk_ispec *src, duk_ispec *dst) {
	dst->t = src->t;
	dst->regconst = src->regconst;
	duk_copy(comp_ctx->thr, src->valstack_idx, dst->valstack_idx);
}

DUK_LOCAL void duk__copy_ivalue(duk_compiler_ctx *comp_ctx, duk_ivalue *src, duk_ivalue *dst) {
	dst->t = src->t;
	dst->op = src->op;
	dst->x1.t = src->x1.t;
	dst->x1.regconst = src->x1.regconst;
	dst->x2.t = src->x2.t;
	dst->x2.regconst = src->x2.regconst;
	duk_copy(comp_ctx->thr, src->x1.valstack_idx, dst->x1.valstack_idx);
	duk_copy(comp_ctx->thr, src->x2.valstack_idx, dst->x2.valstack_idx);
}

DUK_LOCAL duk_regconst_t duk__alloctemps(duk_compiler_ctx *comp_ctx, duk_small_int_t num) {
	duk_regconst_t res;

	res = comp_ctx->curr_func.temp_next;
	comp_ctx->curr_func.temp_next += num;

	if (comp_ctx->curr_func.temp_next > DUK__MAX_TEMPS) { /* == DUK__MAX_TEMPS is OK */
		DUK_ERROR_RANGE(comp_ctx->thr, DUK_STR_TEMP_LIMIT);
		DUK_WO_NORETURN(return 0;);
	}

	/* maintain highest 'used' temporary, needed to figure out nregs of function */
	if (comp_ctx->curr_func.temp_next > comp_ctx->curr_func.temp_max) {
		comp_ctx->curr_func.temp_max = comp_ctx->curr_func.temp_next;
	}

	return res;
}

DUK_LOCAL duk_regconst_t duk__alloctemp(duk_compiler_ctx *comp_ctx) {
	return duk__alloctemps(comp_ctx, 1);
}

DUK_LOCAL void duk__settemp_checkmax(duk_compiler_ctx *comp_ctx, duk_regconst_t temp_next) {
	comp_ctx->curr_func.temp_next = temp_next;
	if (temp_next > comp_ctx->curr_func.temp_max) {
		comp_ctx->curr_func.temp_max = temp_next;
	}
}

/* get const for value at valstack top */
DUK_LOCAL duk_regconst_t duk__getconst(duk_compiler_ctx *comp_ctx) {
	duk_hthread *thr = comp_ctx->thr;
	duk_compiler_func *f = &comp_ctx->curr_func;
	duk_tval *tv1;
	duk_int_t i, n, n_check;

	n = (duk_int_t) duk_get_length(thr, f->consts_idx);

	tv1 = DUK_GET_TVAL_NEGIDX(thr, -1);
	DUK_ASSERT(tv1 != NULL);

#if defined(DUK_USE_FASTINT)
	/* Explicit check for fastint downgrade. */
	DUK_TVAL_CHKFAST_INPLACE_SLOW(tv1);
#endif

	/* Sanity workaround for handling functions with a large number of
	 * constants at least somewhat reasonably.  Otherwise checking whether
	 * we already have the constant would grow very slow (as it is O(N^2)).
	 */
	n_check = (n > DUK__GETCONST_MAX_CONSTS_CHECK ? DUK__GETCONST_MAX_CONSTS_CHECK : n);
	for (i = 0; i < n_check; i++) {
		duk_tval *tv2 = DUK_HOBJECT_A_GET_VALUE_PTR(thr->heap, f->h_consts, i);

		/* Strict equality is NOT enough, because we cannot use the same
		 * constant for e.g. +0 and -0.
		 */
		if (duk_js_samevalue(tv1, tv2)) {
			DUK_DDD(DUK_DDDPRINT("reused existing constant for %!T -> const index %ld", (duk_tval *) tv1, (long) i));
			duk_pop(thr);
			return (duk_regconst_t) i | (duk_regconst_t) DUK__CONST_MARKER;
		}
	}

	if (n > DUK__MAX_CONSTS) {
		DUK_ERROR_RANGE(comp_ctx->thr, DUK_STR_CONST_LIMIT);
		DUK_WO_NORETURN(return 0;);
	}

	DUK_DDD(DUK_DDDPRINT("allocating new constant for %!T -> const index %ld", (duk_tval *) tv1, (long) n));
	(void) duk_put_prop_index(thr, f->consts_idx, (duk_uarridx_t) n); /* invalidates tv1, tv2 */
	return (duk_regconst_t) n | (duk_regconst_t) DUK__CONST_MARKER;
}

DUK_LOCAL duk_bool_t duk__const_needs_refcount(duk_compiler_ctx *comp_ctx, duk_regconst_t rc) {
#if defined(DUK_USE_REFERENCE_COUNTING)
	duk_compiler_func *f = &comp_ctx->curr_func;
	duk_bool_t ret;

	DUK_ASSERT((rc & DUK__CONST_MARKER) == 0); /* caller removes const marker */
	(void) duk_get_prop_index(comp_ctx->thr, f->consts_idx, (duk_uarridx_t) rc);
	ret = !duk_is_number(comp_ctx->thr, -1); /* now only number/string, so conservative check */
	duk_pop(comp_ctx->thr);
	return ret;
#else
	DUK_UNREF(comp_ctx);
	DUK_UNREF(rc);
	DUK_ASSERT((rc & DUK__CONST_MARKER) == 0); /* caller removes const marker */
	return 0;
#endif
}

/* Get the value represented by an duk_ispec to a register or constant.
 * The caller can control the result by indicating whether or not:
 *
 *   (1) a constant is allowed (sometimes the caller needs the result to
 *       be in a register)
 *
 *   (2) a temporary register is required (usually when caller requires
 *       the register to be safely mutable; normally either a bound
 *       register or a temporary register are both OK)
 *
 *   (3) a forced register target needs to be used
 *
 * Bytecode may be emitted to generate the necessary value.  The return
 * value is either a register or a constant.
 */

DUK_LOCAL
duk_regconst_t duk__ispec_toregconst_raw(duk_compiler_ctx *comp_ctx,
                                         duk_ispec *x,
                                         duk_regconst_t forced_reg,
                                         duk_small_uint_t flags) {
	duk_hthread *thr = comp_ctx->thr;

	DUK_DDD(DUK_DDDPRINT("duk__ispec_toregconst_raw(): x={%ld:%ld:%!T}, "
	                     "forced_reg=%ld, flags 0x%08lx: allow_const=%ld require_temp=%ld require_short=%ld",
	                     (long) x->t,
	                     (long) x->regconst,
	                     (duk_tval *) duk_get_tval(thr, x->valstack_idx),
	                     (long) forced_reg,
	                     (unsigned long) flags,
	                     (long) ((flags & DUK__IVAL_FLAG_ALLOW_CONST) ? 1 : 0),
	                     (long) ((flags & DUK__IVAL_FLAG_REQUIRE_TEMP) ? 1 : 0),
	                     (long) ((flags & DUK__IVAL_FLAG_REQUIRE_SHORT) ? 1 : 0)));

	switch (x->t) {
	case DUK_ISPEC_VALUE: {
		duk_tval *tv;

		tv = DUK_GET_TVAL_POSIDX(thr, x->valstack_idx);
		DUK_ASSERT(tv != NULL);

		switch (DUK_TVAL_GET_TAG(tv)) {
		case DUK_TAG_UNDEFINED: {
			/* Note: although there is no 'undefined' literal, undefined
			 * values can occur during compilation as a result of e.g.
			 * the 'void' operator.
			 */
			duk_regconst_t dest = (forced_reg >= 0 ? forced_reg : DUK__ALLOCTEMP(comp_ctx));
			duk__emit_bc(comp_ctx, DUK_OP_LDUNDEF, dest);
			return dest;
		}
		case DUK_TAG_NULL: {
			duk_regconst_t dest = (forced_reg >= 0 ? forced_reg : DUK__ALLOCTEMP(comp_ctx));
			duk__emit_bc(comp_ctx, DUK_OP_LDNULL, dest);
			return dest;
		}
		case DUK_TAG_BOOLEAN: {
			duk_regconst_t dest = (forced_reg >= 0 ? forced_reg : DUK__ALLOCTEMP(comp_ctx));
			duk__emit_bc(comp_ctx, (DUK_TVAL_GET_BOOLEAN(tv) ? DUK_OP_LDTRUE : DUK_OP_LDFALSE), dest);
			return dest;
		}
		case DUK_TAG_POINTER: {
			DUK_UNREACHABLE();
			break;
		}
		case DUK_TAG_STRING: {
			duk_hstring *h;
			duk_regconst_t dest;
			duk_regconst_t constidx;

			h = DUK_TVAL_GET_STRING(tv);
			DUK_UNREF(h);
			DUK_ASSERT(h != NULL);

#if 0 /* XXX: to be implemented? */
			/* Use special opcodes to load short strings */
			if (DUK_HSTRING_GET_BYTELEN(h) <= 2) {
				/* Encode into a single opcode (18 bits can encode 1-2 bytes + length indicator) */
			} else if (DUK_HSTRING_GET_BYTELEN(h) <= 6) {
				/* Encode into a double constant (53 bits can encode 6*8 = 48 bits + 3-bit length */
			}
#endif
			duk_dup(thr, x->valstack_idx);
			constidx = duk__getconst(comp_ctx);

			if (flags & DUK__IVAL_FLAG_ALLOW_CONST) {
				return constidx;
			}

			dest = (forced_reg >= 0 ? forced_reg : DUK__ALLOCTEMP(comp_ctx));
			duk__emit_a_bc(comp_ctx, DUK_OP_LDCONST, dest, constidx);
			return dest;
		}
		case DUK_TAG_OBJECT: {
			DUK_UNREACHABLE();
			break;
		}
		case DUK_TAG_BUFFER: {
			DUK_UNREACHABLE();
			break;
		}
		case DUK_TAG_LIGHTFUNC: {
			DUK_UNREACHABLE();
			break;
		}
#if defined(DUK_USE_FASTINT)
		case DUK_TAG_FASTINT:
#endif
		default: {
			/* number */
			duk_regconst_t dest;
			duk_regconst_t constidx;
			duk_double_t dval;
			duk_int32_t ival;

			DUK_ASSERT(!DUK_TVAL_IS_UNUSED(tv));
			DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv));
			dval = DUK_TVAL_GET_NUMBER(tv);

			if (!(flags & DUK__IVAL_FLAG_ALLOW_CONST)) {
				/* A number can be loaded either through a constant, using
				 * LDINT, or using LDINT+LDINTX.  LDINT is always a size win,
				 * LDINT+LDINTX is not if the constant is used multiple times.
				 * Currently always prefer LDINT+LDINTX over a double constant.
				 */

				if (duk_is_whole_get_int32_nonegzero(dval, &ival)) {
					dest = (forced_reg >= 0 ? forced_reg : DUK__ALLOCTEMP(comp_ctx));
					duk__emit_load_int32(comp_ctx, dest, ival);
					return dest;
				}
			}

			duk_dup(thr, x->valstack_idx);
			constidx = duk__getconst(comp_ctx);

			if (flags & DUK__IVAL_FLAG_ALLOW_CONST) {
				return constidx;
			} else {
				dest = (forced_reg >= 0 ? forced_reg : DUK__ALLOCTEMP(comp_ctx));
				duk__emit_a_bc(comp_ctx, DUK_OP_LDCONST, dest, constidx);
				return dest;
			}
		}
		} /* end switch */
		goto fail_internal; /* never here */
	}
	case DUK_ISPEC_REGCONST: {
		if (forced_reg >= 0) {
			if (DUK__ISCONST(x->regconst)) {
				duk__emit_a_bc(comp_ctx, DUK_OP_LDCONST, forced_reg, x->regconst);
			} else if (x->regconst != forced_reg) {
				duk__emit_a_bc(comp_ctx, DUK_OP_LDREG, forced_reg, x->regconst);
			} else {
				; /* already in correct reg */
			}
			return forced_reg;
		}

		DUK_ASSERT(forced_reg < 0);
		if (DUK__ISCONST(x->regconst)) {
			if (!(flags & DUK__IVAL_FLAG_ALLOW_CONST)) {
				duk_regconst_t dest = DUK__ALLOCTEMP(comp_ctx);
				duk__emit_a_bc(comp_ctx, DUK_OP_LDCONST, dest, x->regconst);
				return dest;
			}
			return x->regconst;
		}

		DUK_ASSERT(forced_reg < 0 && !DUK__ISCONST(x->regconst));
		if ((flags & DUK__IVAL_FLAG_REQUIRE_TEMP) && !DUK__ISREG_TEMP(comp_ctx, x->regconst)) {
			duk_regconst_t dest = DUK__ALLOCTEMP(comp_ctx);
			duk__emit_a_bc(comp_ctx, DUK_OP_LDREG, dest, x->regconst);
			return dest;
		}
		return x->regconst;
	}
	default: {
		break; /* never here */
	}
	}

fail_internal:
	DUK_ERROR_INTERNAL(thr);
	DUK_WO_NORETURN(return 0;);
}

DUK_LOCAL void duk__ispec_toforcedreg(duk_compiler_ctx *comp_ctx, duk_ispec *x, duk_regconst_t forced_reg) {
	DUK_ASSERT(forced_reg >= 0);
	(void) duk__ispec_toregconst_raw(comp_ctx, x, forced_reg, 0 /*flags*/);
}

/* Coerce an duk_ivalue to a 'plain' value by generating the necessary
 * arithmetic operations, property access, or variable access bytecode.
 * The duk_ivalue argument ('x') is converted into a plain value as a
 * side effect.
 */
DUK_LOCAL void duk__ivalue_toplain_raw(duk_compiler_ctx *comp_ctx, duk_ivalue *x, duk_regconst_t forced_reg) {
	duk_hthread *thr = comp_ctx->thr;

	DUK_DDD(DUK_DDDPRINT("duk__ivalue_toplain_raw(): x={t=%ld,op=%ld,x1={%ld:%ld:%!T},x2={%ld:%ld:%!T}}, "
	                     "forced_reg=%ld",
	                     (long) x->t,
	                     (long) x->op,
	                     (long) x->x1.t,
	                     (long) x->x1.regconst,
	                     (duk_tval *) duk_get_tval(thr, x->x1.valstack_idx),
	                     (long) x->x2.t,
	                     (long) x->x2.regconst,
	                     (duk_tval *) duk_get_tval(thr, x->x2.valstack_idx),
	                     (long) forced_reg));

	switch (x->t) {
	case DUK_IVAL_PLAIN: {
		return;
	}
	/* XXX: support unary arithmetic ivalues (useful?) */
	case DUK_IVAL_ARITH: {
		duk_regconst_t arg1;
		duk_regconst_t arg2;
		duk_regconst_t dest;
		duk_tval *tv1;
		duk_tval *tv2;

		DUK_DDD(DUK_DDDPRINT("arith to plain conversion"));

		/* inline arithmetic check for constant values */
		/* XXX: use the exactly same arithmetic function here as in executor */
		if (x->x1.t == DUK_ISPEC_VALUE && x->x2.t == DUK_ISPEC_VALUE && x->t == DUK_IVAL_ARITH) {
			tv1 = DUK_GET_TVAL_POSIDX(thr, x->x1.valstack_idx);
			tv2 = DUK_GET_TVAL_POSIDX(thr, x->x2.valstack_idx);
			DUK_ASSERT(tv1 != NULL);
			DUK_ASSERT(tv2 != NULL);

			DUK_DDD(DUK_DDDPRINT("arith: tv1=%!T, tv2=%!T", (duk_tval *) tv1, (duk_tval *) tv2));

			if (DUK_TVAL_IS_NUMBER(tv1) && DUK_TVAL_IS_NUMBER(tv2)) {
				duk_double_t d1 = DUK_TVAL_GET_NUMBER(tv1);
				duk_double_t d2 = DUK_TVAL_GET_NUMBER(tv2);
				duk_double_t d3;
				duk_bool_t accept_fold = 1;

				DUK_DDD(DUK_DDDPRINT("arith inline check: d1=%lf, d2=%lf, op=%ld",
				                     (double) d1,
				                     (double) d2,
				                     (long) x->op));
				switch (x->op) {
				case DUK_OP_ADD: {
					d3 = d1 + d2;
					break;
				}
				case DUK_OP_SUB: {
					d3 = d1 - d2;
					break;
				}
				case DUK_OP_MUL: {
					d3 = d1 * d2;
					break;
				}
				case DUK_OP_DIV: {
					/* Division-by-zero is undefined
					 * behavior, so rely on a helper.
					 */
					d3 = duk_double_div(d1, d2);
					break;
				}
				case DUK_OP_EXP: {
					d3 = (duk_double_t) duk_js_arith_pow((double) d1, (double) d2);
					break;
				}
				default: {
					d3 = 0.0; /* Won't be used, but silence MSVC /W4 warning. */
					accept_fold = 0;
					break;
				}
				}

				if (accept_fold) {
					duk_double_union du;
					du.d = d3;
					DUK_DBLUNION_NORMALIZE_NAN_CHECK(&du);
					d3 = du.d;

					x->t = DUK_IVAL_PLAIN;
					DUK_ASSERT(x->x1.t == DUK_ISPEC_VALUE);
					DUK_TVAL_SET_NUMBER(tv1, d3); /* old value is number: no refcount */
					return;
				}
			} else if (x->op == DUK_OP_ADD && DUK_TVAL_IS_STRING(tv1) && DUK_TVAL_IS_STRING(tv2)) {
				/* Inline string concatenation.  No need to check for
				 * symbols, as all inputs are valid ECMAScript strings.
				 */
				duk_dup(thr, x->x1.valstack_idx);
				duk_dup(thr, x->x2.valstack_idx);
				duk_concat(thr, 2);
				duk_replace(thr, x->x1.valstack_idx);
				x->t = DUK_IVAL_PLAIN;
				DUK_ASSERT(x->x1.t == DUK_ISPEC_VALUE);
				return;
			}
		}

		arg1 = duk__ispec_toregconst_raw(comp_ctx,
		                                 &x->x1,
		                                 -1,
		                                 DUK__IVAL_FLAG_ALLOW_CONST | DUK__IVAL_FLAG_REQUIRE_SHORT /*flags*/);
		arg2 = duk__ispec_toregconst_raw(comp_ctx,
		                                 &x->x2,
		                                 -1,
		                                 DUK__IVAL_FLAG_ALLOW_CONST | DUK__IVAL_FLAG_REQUIRE_SHORT /*flags*/);

		/* If forced reg, use it as destination.  Otherwise try to
		 * use either coerced ispec if it is a temporary.
		 */
		if (forced_reg >= 0) {
			dest = forced_reg;
		} else if (DUK__ISREG_TEMP(comp_ctx, arg1)) {
			dest = arg1;
		} else if (DUK__ISREG_TEMP(comp_ctx, arg2)) {
			dest = arg2;
		} else {
			dest = DUK__ALLOCTEMP(comp_ctx);
		}

		DUK_ASSERT(DUK__ISREG(dest));
		duk__emit_a_b_c(comp_ctx, x->op | DUK__EMIT_FLAG_BC_REGCONST, dest, arg1, arg2);

		duk__ivalue_regconst(x, dest);
		return;
	}
	case DUK_IVAL_PROP: {
		/* XXX: very similar to DUK_IVAL_ARITH - merge? */
		duk_regconst_t arg1;
		duk_regconst_t arg2;
		duk_regconst_t dest;

		/* Need a short reg/const, does not have to be a mutable temp. */
		arg1 = duk__ispec_toregconst_raw(comp_ctx,
		                                 &x->x1,
		                                 -1,
		                                 DUK__IVAL_FLAG_ALLOW_CONST | DUK__IVAL_FLAG_REQUIRE_SHORT /*flags*/);
		arg2 = duk__ispec_toregconst_raw(comp_ctx,
		                                 &x->x2,
		                                 -1,
		                                 DUK__IVAL_FLAG_ALLOW_CONST | DUK__IVAL_FLAG_REQUIRE_SHORT /*flags*/);

		/* Pick a destination register.  If either base value or key
		 * happens to be a temp value, reuse it as the destination.
		 *
		 * XXX: The temp must be a "mutable" one, i.e. such that no
		 * other expression is using it anymore.  Here this should be
		 * the case because the value of a property access expression
		 * is neither the base nor the key, but the lookup result.
		 */

		if (forced_reg >= 0) {
			dest = forced_reg;
		} else if (DUK__ISREG_TEMP(comp_ctx, arg1)) {
			dest = arg1;
		} else if (DUK__ISREG_TEMP(comp_ctx, arg2)) {
			dest = arg2;
		} else {
			dest = DUK__ALLOCTEMP(comp_ctx);
		}

		duk__emit_a_b_c(comp_ctx, DUK_OP_GETPROP | DUK__EMIT_FLAG_BC_REGCONST, dest, arg1, arg2);

		duk__ivalue_regconst(x, dest);
		return;
	}
	case DUK_IVAL_VAR: {
		/* x1 must be a string */
		duk_regconst_t dest;
		duk_regconst_t reg_varbind;
		duk_regconst_t rc_varname;

		DUK_ASSERT(x->x1.t == DUK_ISPEC_VALUE);

		duk_dup(thr, x->x1.valstack_idx);
		if (duk__lookup_lhs(comp_ctx, &reg_varbind, &rc_varname)) {
			duk__ivalue_regconst(x, reg_varbind);
		} else {
			dest = (forced_reg >= 0 ? forced_reg : DUK__ALLOCTEMP(comp_ctx));
			duk__emit_a_bc(comp_ctx, DUK_OP_GETVAR, dest, rc_varname);
			duk__ivalue_regconst(x, dest);
		}
		return;
	}
	case DUK_IVAL_NONE:
	default: {
		DUK_D(DUK_DPRINT("invalid ivalue type: %ld", (long) x->t));
		break;
	}
	}

	DUK_ERROR_INTERNAL(thr);
	DUK_WO_NORETURN(return;);
}

/* evaluate to plain value, no forced register (temp/bound reg both ok) */
DUK_LOCAL void duk__ivalue_toplain(duk_compiler_ctx *comp_ctx, duk_ivalue *x) {
	duk__ivalue_toplain_raw(comp_ctx, x, -1 /*forced_reg*/);
}

/* evaluate to final form (e.g. coerce GETPROP to code), throw away temp */
DUK_LOCAL void duk__ivalue_toplain_ignore(duk_compiler_ctx *comp_ctx, duk_ivalue *x) {
	duk_regconst_t temp;

	/* If duk__ivalue_toplain_raw() allocates a temp, forget it and
	 * restore next temp state.
	 */
	temp = DUK__GETTEMP(comp_ctx);
	duk__ivalue_toplain_raw(comp_ctx, x, -1 /*forced_reg*/);
	DUK__SETTEMP(comp_ctx, temp);
}

/* Coerce an duk_ivalue to a register or constant; result register may
 * be a temp or a bound register.
 *
 * The duk_ivalue argument ('x') is converted into a regconst as a
 * side effect.
 */
DUK_LOCAL
duk_regconst_t duk__ivalue_toregconst_raw(duk_compiler_ctx *comp_ctx,
                                          duk_ivalue *x,
                                          duk_regconst_t forced_reg,
                                          duk_small_uint_t flags) {
	duk_hthread *thr = comp_ctx->thr;
	duk_regconst_t reg;
	DUK_UNREF(thr);

	DUK_DDD(DUK_DDDPRINT("duk__ivalue_toregconst_raw(): x={t=%ld,op=%ld,x1={%ld:%ld:%!T},x2={%ld:%ld:%!T}}, "
	                     "forced_reg=%ld, flags 0x%08lx: allow_const=%ld require_temp=%ld require_short=%ld",
	                     (long) x->t,
	                     (long) x->op,
	                     (long) x->x1.t,
	                     (long) x->x1.regconst,
	                     (duk_tval *) duk_get_tval(thr, x->x1.valstack_idx),
	                     (long) x->x2.t,
	                     (long) x->x2.regconst,
	                     (duk_tval *) duk_get_tval(thr, x->x2.valstack_idx),
	                     (long) forced_reg,
	                     (unsigned long) flags,
	                     (long) ((flags & DUK__IVAL_FLAG_ALLOW_CONST) ? 1 : 0),
	                     (long) ((flags & DUK__IVAL_FLAG_REQUIRE_TEMP) ? 1 : 0),
	                     (long) ((flags & DUK__IVAL_FLAG_REQUIRE_SHORT) ? 1 : 0)));

	/* first coerce to a plain value */
	duk__ivalue_toplain_raw(comp_ctx, x, forced_reg);
	DUK_ASSERT(x->t == DUK_IVAL_PLAIN);

	/* then to a register */
	reg = duk__ispec_toregconst_raw(comp_ctx, &x->x1, forced_reg, flags);
	duk__ivalue_regconst(x, reg);

	return reg;
}

DUK_LOCAL duk_regconst_t duk__ivalue_toreg(duk_compiler_ctx *comp_ctx, duk_ivalue *x) {
	return duk__ivalue_toregconst_raw(comp_ctx, x, -1, 0 /*flags*/);
}

#if 0 /* unused */
DUK_LOCAL duk_regconst_t duk__ivalue_totemp(duk_compiler_ctx *comp_ctx, duk_ivalue *x) {
	return duk__ivalue_toregconst_raw(comp_ctx, x, -1, DUK__IVAL_FLAG_REQUIRE_TEMP /*flags*/);
}
#endif

DUK_LOCAL void duk__ivalue_toforcedreg(duk_compiler_ctx *comp_ctx, duk_ivalue *x, duk_int_t forced_reg) {
	DUK_ASSERT(forced_reg >= 0);
	(void) duk__ivalue_toregconst_raw(comp_ctx, x, forced_reg, 0 /*flags*/);
}

DUK_LOCAL duk_regconst_t duk__ivalue_toregconst(duk_compiler_ctx *comp_ctx, duk_ivalue *x) {
	return duk__ivalue_toregconst_raw(comp_ctx, x, -1, DUK__IVAL_FLAG_ALLOW_CONST /*flags*/);
}

DUK_LOCAL duk_regconst_t duk__ivalue_totempconst(duk_compiler_ctx *comp_ctx, duk_ivalue *x) {
	return duk__ivalue_toregconst_raw(comp_ctx, x, -1, DUK__IVAL_FLAG_ALLOW_CONST | DUK__IVAL_FLAG_REQUIRE_TEMP /*flags*/);
}

/* The issues below can be solved with better flags */

/* XXX: many operations actually want toforcedtemp() -- brand new temp? */
/* XXX: need a toplain_ignore() which will only coerce a value to a temp
 * register if it might have a side effect.  Side-effect free values do not
 * need to be coerced.
 */

/*
 *  Identifier handling
 */

DUK_LOCAL duk_regconst_t duk__lookup_active_register_binding(duk_compiler_ctx *comp_ctx) {
	duk_hthread *thr = comp_ctx->thr;
	duk_hstring *h_varname;
	duk_regconst_t ret;

	DUK_DDD(DUK_DDDPRINT("resolving identifier reference to '%!T'", (duk_tval *) duk_get_tval(thr, -1)));

	/*
	 *  Special name handling
	 */

	h_varname = duk_known_hstring(thr, -1);

	if (h_varname == DUK_HTHREAD_STRING_LC_ARGUMENTS(thr)) {
		DUK_DDD(DUK_DDDPRINT("flagging function as accessing 'arguments'"));
		comp_ctx->curr_func.id_access_arguments = 1;
	}

	/*
	 *  Inside one or more 'with' statements fall back to slow path always.
	 *  (See e.g. test-stmt-with.js.)
	 */

	if (comp_ctx->curr_func.with_depth > 0) {
		DUK_DDD(DUK_DDDPRINT("identifier lookup inside a 'with' -> fall back to slow path"));
		goto slow_path_own;
	}

	/*
	 *  Any catch bindings ("catch (e)") also affect identifier binding.
	 *
	 *  Currently, the varmap is modified for the duration of the catch
	 *  clause to ensure any identifier accesses with the catch variable
	 *  name will use slow path.
	 */

	duk_get_prop(thr, comp_ctx->curr_func.varmap_idx);
	if (duk_is_number(thr, -1)) {
		ret = duk_to_int(thr, -1);
		duk_pop(thr);
	} else {
		duk_pop(thr);
		if (comp_ctx->curr_func.catch_depth > 0 || comp_ctx->curr_func.with_depth > 0) {
			DUK_DDD(DUK_DDDPRINT("slow path access from inside a try-catch or with needs _Varmap"));
			goto slow_path_own;
		} else {
			/* In this case we're doing a variable lookup that doesn't
			 * match our own variables, so _Varmap won't be needed at
			 * run time.
			 */
			DUK_DDD(DUK_DDDPRINT("slow path access outside of try-catch and with, no need for _Varmap"));
			goto slow_path_notown;
		}
	}

	DUK_DDD(DUK_DDDPRINT("identifier lookup -> reg %ld", (long) ret));
	return ret;

slow_path_notown:
	DUK_DDD(DUK_DDDPRINT("identifier lookup -> slow path, not own variable"));

	comp_ctx->curr_func.id_access_slow = 1;
	return (duk_regconst_t) -1;

slow_path_own:
	DUK_DDD(DUK_DDDPRINT("identifier lookup -> slow path, may be own variable"));

	comp_ctx->curr_func.id_access_slow = 1;
	comp_ctx->curr_func.id_access_slow_own = 1;
	return (duk_regconst_t) -1;
}

/* Lookup an identifier name in the current varmap, indicating whether the
 * identifier is register-bound and if not, allocating a constant for the
 * identifier name.  Returns 1 if register-bound, 0 otherwise.  Caller can
 * also check (out_reg_varbind >= 0) to check whether or not identifier is
 * register bound.  The caller must NOT use out_rc_varname at all unless
 * return code is 0 or out_reg_varbind is < 0; this is becuase out_rc_varname
 * is unsigned and doesn't have a "unused" / none value.
 */
DUK_LOCAL duk_bool_t duk__lookup_lhs(duk_compiler_ctx *comp_ctx, duk_regconst_t *out_reg_varbind, duk_regconst_t *out_rc_varname) {
	duk_hthread *thr = comp_ctx->thr;
	duk_regconst_t reg_varbind;
	duk_regconst_t rc_varname;

	/* [ ... varname ] */

	duk_dup_top(thr);
	reg_varbind = duk__lookup_active_register_binding(comp_ctx);

	if (reg_varbind >= 0) {
		*out_reg_varbind = reg_varbind;
		*out_rc_varname = 0; /* duk_regconst_t is unsigned, so use 0 as dummy value (ignored by caller) */
		duk_pop(thr);
		return 1;
	} else {
		rc_varname = duk__getconst(comp_ctx);
		*out_reg_varbind = -1;
		*out_rc_varname = rc_varname;
		return 0;
	}
}

/*
 *  Label handling
 *
 *  Labels are initially added with flags prohibiting both break and continue.
 *  When the statement type is finally uncovered (after potentially multiple
 *  labels), all the labels are updated to allow/prohibit break and continue.
 */

DUK_LOCAL void duk__add_label(duk_compiler_ctx *comp_ctx, duk_hstring *h_label, duk_int_t pc_label, duk_int_t label_id) {
	duk_hthread *thr = comp_ctx->thr;
	duk_size_t n;
	duk_size_t new_size;
	duk_uint8_t *p;
	duk_labelinfo *li_start, *li;

	/* Duplicate (shadowing) labels are not allowed, except for the empty
	 * labels (which are used as default labels for switch and iteration
	 * statements).
	 *
	 * We could also allow shadowing of non-empty pending labels without any
	 * other issues than breaking the required label shadowing requirements
	 * of the E5 specification, see Section 12.12.
	 */

	p = (duk_uint8_t *) DUK_HBUFFER_DYNAMIC_GET_DATA_PTR(thr->heap, comp_ctx->curr_func.h_labelinfos);
	li_start = (duk_labelinfo *) (void *) p;
	li = (duk_labelinfo *) (void *) (p + DUK_HBUFFER_GET_SIZE(comp_ctx->curr_func.h_labelinfos));
	n = (duk_size_t) (li - li_start);

	while (li > li_start) {
		li--;

		if (li->h_label == h_label && h_label != DUK_HTHREAD_STRING_EMPTY_STRING(thr)) {
			DUK_ERROR_SYNTAX(thr, DUK_STR_DUPLICATE_LABEL);
			DUK_WO_NORETURN(return;);
		}
	}

	duk_push_hstring(thr, h_label);
	DUK_ASSERT(n <= DUK_UARRIDX_MAX); /* label limits */
	(void) duk_put_prop_index(thr, comp_ctx->curr_func.labelnames_idx, (duk_uarridx_t) n);

	new_size = (n + 1) * sizeof(duk_labelinfo);
	duk_hbuffer_resize(thr, comp_ctx->curr_func.h_labelinfos, new_size);
	/* XXX: slack handling, slow now */

	/* relookup after possible realloc */
	p = (duk_uint8_t *) DUK_HBUFFER_DYNAMIC_GET_DATA_PTR(thr->heap, comp_ctx->curr_func.h_labelinfos);
	li_start = (duk_labelinfo *) (void *) p;
	DUK_UNREF(li_start); /* silence scan-build warning */
	li = (duk_labelinfo *) (void *) (p + DUK_HBUFFER_GET_SIZE(comp_ctx->curr_func.h_labelinfos));
	li--;

	/* Labels can be used for iteration statements but also for other statements,
	 * in particular a label can be used for a block statement.  All cases of a
	 * named label accept a 'break' so that flag is set here.  Iteration statements
	 * also allow 'continue', so that flag is updated when we figure out the
	 * statement type.
	 */

	li->flags = DUK_LABEL_FLAG_ALLOW_BREAK;
	li->label_id = label_id;
	li->h_label = h_label;
	li->catch_depth = comp_ctx->curr_func.catch_depth; /* catch depth from current func */
	li->pc_label = pc_label;

	DUK_DDD(DUK_DDDPRINT("registered label: flags=0x%08lx, id=%ld, name=%!O, catch_depth=%ld, pc_label=%ld",
	                     (unsigned long) li->flags,
	                     (long) li->label_id,
	                     (duk_heaphdr *) li->h_label,
	                     (long) li->catch_depth,
	                     (long) li->pc_label));
}

/* Update all labels with matching label_id. */
DUK_LOCAL void duk__update_label_flags(duk_compiler_ctx *comp_ctx, duk_int_t label_id, duk_small_uint_t flags) {
	duk_uint8_t *p;
	duk_labelinfo *li_start, *li;

	p = (duk_uint8_t *) DUK_HBUFFER_DYNAMIC_GET_DATA_PTR(comp_ctx->thr->heap, comp_ctx->curr_func.h_labelinfos);
	li_start = (duk_labelinfo *) (void *) p;
	li = (duk_labelinfo *) (void *) (p + DUK_HBUFFER_GET_SIZE(comp_ctx->curr_func.h_labelinfos));

	/* Match labels starting from latest; once label_id no longer matches, we can
	 * safely exit without checking the rest of the labels (only the topmost labels
	 * are ever updated).
	 */
	while (li > li_start) {
		li--;

		if (li->label_id != label_id) {
			break;
		}

		DUK_DDD(DUK_DDDPRINT("updating (overwriting) label flags for li=%p, label_id=%ld, flags=%ld",
		                     (void *) li,
		                     (long) label_id,
		                     (long) flags));

		li->flags = flags;
	}
}

/* Lookup active label information.  Break/continue distinction is necessary to handle switch
 * statement related labels correctly: a switch will only catch a 'break', not a 'continue'.
 *
 * An explicit label cannot appear multiple times in the active set, but empty labels (unlabelled
 * iteration and switch statements) can.  A break will match the closest unlabelled or labelled
 * statement.  A continue will match the closest unlabelled or labelled iteration statement.  It is
 * a syntax error if a continue matches a labelled switch statement; because an explicit label cannot
 * be duplicated, the continue cannot match any valid label outside the switch.
 *
 * A side effect of these rules is that a LABEL statement related to a switch should never actually
 * catch a continue abrupt completion at run-time.  Hence an INVALID opcode can be placed in the
 * continue slot of the switch's LABEL statement.
 */

/* XXX: awkward, especially the bunch of separate output values -> output struct? */
DUK_LOCAL void duk__lookup_active_label(duk_compiler_ctx *comp_ctx,
                                        duk_hstring *h_label,
                                        duk_bool_t is_break,
                                        duk_int_t *out_label_id,
                                        duk_int_t *out_label_catch_depth,
                                        duk_int_t *out_label_pc,
                                        duk_bool_t *out_is_closest) {
	duk_hthread *thr = comp_ctx->thr;
	duk_uint8_t *p;
	duk_labelinfo *li_start, *li_end, *li;
	duk_bool_t match = 0;

	DUK_DDD(DUK_DDDPRINT("looking up active label: label='%!O', is_break=%ld", (duk_heaphdr *) h_label, (long) is_break));

	DUK_UNREF(thr);

	p = (duk_uint8_t *) DUK_HBUFFER_DYNAMIC_GET_DATA_PTR(thr->heap, comp_ctx->curr_func.h_labelinfos);
	li_start = (duk_labelinfo *) (void *) p;
	li_end = (duk_labelinfo *) (void *) (p + DUK_HBUFFER_GET_SIZE(comp_ctx->curr_func.h_labelinfos));
	li = li_end;

	/* Match labels starting from latest label because there can be duplicate empty
	 * labels in the label set.
	 */
	while (li > li_start) {
		li--;

		if (li->h_label != h_label) {
			DUK_DDD(DUK_DDDPRINT("labelinfo[%ld] ->'%!O' != %!O",
			                     (long) (li - li_start),
			                     (duk_heaphdr *) li->h_label,
			                     (duk_heaphdr *) h_label));
			continue;
		}

		DUK_DDD(DUK_DDDPRINT("labelinfo[%ld] -> '%!O' label name matches (still need to check type)",
		                     (long) (li - li_start),
		                     (duk_heaphdr *) h_label));

		/* currently all labels accept a break, so no explicit check for it now */
		DUK_ASSERT(li->flags & DUK_LABEL_FLAG_ALLOW_BREAK);

		if (is_break) {
			/* break matches always */
			match = 1;
			break;
		} else if (li->flags & DUK_LABEL_FLAG_ALLOW_CONTINUE) {
			/* iteration statements allow continue */
			match = 1;
			break;
		} else {
			/* continue matched this label -- we can only continue if this is the empty
			 * label, for which duplication is allowed, and thus there is hope of
			 * finding a match deeper in the label stack.
			 */
			if (h_label != DUK_HTHREAD_STRING_EMPTY_STRING(thr)) {
				DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_LABEL);
				DUK_WO_NORETURN(return;);
			} else {
				DUK_DDD(DUK_DDDPRINT("continue matched an empty label which does not "
				                     "allow a continue -> continue lookup deeper in label stack"));
			}
		}
	}
	/* XXX: match flag is awkward, rework */
	if (!match) {
		DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_LABEL);
		DUK_WO_NORETURN(return;);
	}

	DUK_DDD(DUK_DDDPRINT("label match: %!O -> label_id %ld, catch_depth=%ld, pc_label=%ld",
	                     (duk_heaphdr *) h_label,
	                     (long) li->label_id,
	                     (long) li->catch_depth,
	                     (long) li->pc_label));

	*out_label_id = li->label_id;
	*out_label_catch_depth = li->catch_depth;
	*out_label_pc = li->pc_label;
	*out_is_closest = (li == li_end - 1);
}

DUK_LOCAL void duk__reset_labels_to_length(duk_compiler_ctx *comp_ctx, duk_size_t len) {
	duk_hthread *thr = comp_ctx->thr;

	duk_set_length(thr, comp_ctx->curr_func.labelnames_idx, len);
	duk_hbuffer_resize(thr, comp_ctx->curr_func.h_labelinfos, sizeof(duk_labelinfo) * len);
}

/*
 *  Expression parsing: duk__expr_nud(), duk__expr_led(), duk__expr_lbp(), and helpers.
 *
 *  - duk__expr_nud(): ("null denotation"): process prev_token as a "start" of an expression (e.g. literal)
 *  - duk__expr_led(): ("left denotation"): process prev_token in the "middle" of an expression (e.g. operator)
 *  - duk__expr_lbp(): ("left-binding power"): return left-binding power of curr_token
 */

/* object literal key tracking flags */
#define DUK__OBJ_LIT_KEY_PLAIN (1 << 0) /* key encountered as a plain property */
#define DUK__OBJ_LIT_KEY_GET   (1 << 1) /* key encountered as a getter */
#define DUK__OBJ_LIT_KEY_SET   (1 << 2) /* key encountered as a setter */

DUK_LOCAL void duk__nud_array_literal(duk_compiler_ctx *comp_ctx, duk_ivalue *res) {
	duk_hthread *thr = comp_ctx->thr;
	duk_regconst_t reg_obj; /* result reg */
	duk_regconst_t reg_temp; /* temp reg */
	duk_regconst_t temp_start; /* temp reg value for start of loop */
	duk_small_uint_t max_init_values; /* max # of values initialized in one MPUTARR set */
	duk_small_uint_t num_values; /* number of values in current MPUTARR set */
	duk_uarridx_t curr_idx; /* current (next) array index */
	duk_uarridx_t start_idx; /* start array index of current MPUTARR set */
	duk_uarridx_t init_idx; /* last array index explicitly initialized, +1 */
	duk_bool_t require_comma; /* next loop requires a comma */
#if !defined(DUK_USE_PREFER_SIZE)
	duk_int_t pc_newarr;
	duk_compiler_instr *instr;
#endif

	/* DUK_TOK_LBRACKET already eaten, current token is right after that */
	DUK_ASSERT(comp_ctx->prev_token.t == DUK_TOK_LBRACKET);

	max_init_values = DUK__MAX_ARRAY_INIT_VALUES; /* XXX: depend on available temps? */

	reg_obj = DUK__ALLOCTEMP(comp_ctx);
#if !defined(DUK_USE_PREFER_SIZE)
	pc_newarr = duk__get_current_pc(comp_ctx);
#endif
	duk__emit_bc(comp_ctx, DUK_OP_NEWARR, reg_obj); /* XXX: patch initial size hint afterwards? */
	temp_start = DUK__GETTEMP(comp_ctx);

	/*
	 *  Emit initializers in sets of maximum max_init_values.
	 *  Corner cases such as single value initializers do not have
	 *  special handling now.
	 *
	 *  Elided elements must not be emitted as 'undefined' values,
	 *  because such values would be enumerable (which is incorrect).
	 *  Also note that trailing elisions must be reflected in the
	 *  length of the final array but cause no elements to be actually
	 *  inserted.
	 */

	curr_idx = 0;
	init_idx = 0; /* tracks maximum initialized index + 1 */
	start_idx = 0;
	require_comma = 0;

	for (;;) {
		num_values = 0;
		DUK__SETTEMP(comp_ctx, temp_start);

		if (comp_ctx->curr_token.t == DUK_TOK_RBRACKET) {
			break;
		}

		for (;;) {
			if (comp_ctx->curr_token.t == DUK_TOK_RBRACKET) {
				/* the outer loop will recheck and exit */
				break;
			}

			/* comma check */
			if (require_comma) {
				if (comp_ctx->curr_token.t == DUK_TOK_COMMA) {
					/* comma after a value, expected */
					duk__advance(comp_ctx);
					require_comma = 0;
					continue;
				} else {
					goto syntax_error;
				}
			} else {
				if (comp_ctx->curr_token.t == DUK_TOK_COMMA) {
					/* elision - flush */
					curr_idx++;
					duk__advance(comp_ctx);
					/* if num_values > 0, MPUTARR emitted by outer loop after break */
					break;
				}
			}
			/* else an array initializer element */

			/* initial index */
			if (num_values == 0) {
				start_idx = curr_idx;
				reg_temp = DUK__ALLOCTEMP(comp_ctx);
				duk__emit_load_int32(comp_ctx, reg_temp, (duk_int32_t) start_idx);
			}

			reg_temp = DUK__ALLOCTEMP(comp_ctx); /* alloc temp just in case, to update max temp */
			DUK__SETTEMP(comp_ctx, reg_temp);
			duk__expr_toforcedreg(comp_ctx, res, DUK__BP_COMMA /*rbp_flags*/, reg_temp /*forced_reg*/);
			DUK__SETTEMP(comp_ctx, reg_temp + 1);

			num_values++;
			curr_idx++;
			require_comma = 1;

			if (num_values >= max_init_values) {
				/* MPUTARR emitted by outer loop */
				break;
			}
		}

		if (num_values > 0) {
			/* - A is a source register (it's not a write target, but used
			 *   to identify the target object) but can be shuffled.
			 * - B cannot be shuffled normally because it identifies a range
			 *   of registers, the emitter has special handling for this
			 *   (the "no shuffle" flag must not be set).
			 * - C is a non-register number and cannot be shuffled, but
			 *   never needs to be.
			 */
			duk__emit_a_b_c(comp_ctx,
			                DUK_OP_MPUTARR | DUK__EMIT_FLAG_NO_SHUFFLE_C | DUK__EMIT_FLAG_A_IS_SOURCE,
			                reg_obj,
			                temp_start,
			                (duk_regconst_t) (num_values + 1));
			init_idx = start_idx + num_values;

			/* num_values and temp_start reset at top of outer loop */
		}
	}

	/* Update initil size for NEWARR, doesn't need to be exact and is
	 * capped at A field limit.
	 */
#if !defined(DUK_USE_PREFER_SIZE)
	instr = duk__get_instr_ptr(comp_ctx, pc_newarr);
	instr->ins |= DUK_ENC_OP_A(0, curr_idx > DUK_BC_A_MAX ? DUK_BC_A_MAX : curr_idx);
#endif

	DUK_ASSERT(comp_ctx->curr_token.t == DUK_TOK_RBRACKET);
	duk__advance(comp_ctx);

	DUK_DDD(DUK_DDDPRINT("array literal done, curridx=%ld, initidx=%ld", (long) curr_idx, (long) init_idx));

	/* trailing elisions? */
	if (curr_idx > init_idx) {
		/* yes, must set array length explicitly */
		DUK_DDD(DUK_DDDPRINT("array literal has trailing elisions which affect its length"));
		reg_temp = DUK__ALLOCTEMP(comp_ctx);
		duk__emit_load_int32(comp_ctx, reg_temp, (duk_int_t) curr_idx);
		duk__emit_a_bc(comp_ctx, DUK_OP_SETALEN | DUK__EMIT_FLAG_A_IS_SOURCE, reg_obj, reg_temp);
	}

	DUK__SETTEMP(comp_ctx, temp_start);

	duk__ivalue_regconst(res, reg_obj);
	return;

syntax_error:
	DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_ARRAY_LITERAL);
	DUK_WO_NORETURN(return;);
}

typedef struct {
	duk_regconst_t reg_obj;
	duk_regconst_t temp_start;
	duk_small_uint_t num_pairs;
	duk_small_uint_t num_total_pairs;
} duk__objlit_state;

DUK_LOCAL void duk__objlit_flush_keys(duk_compiler_ctx *comp_ctx, duk__objlit_state *st) {
	if (st->num_pairs > 0) {
		/* - A is a source register (it's not a write target, but used
		 *   to identify the target object) but can be shuffled.
		 * - B cannot be shuffled normally because it identifies a range
		 *   of registers, the emitter has special handling for this
		 *   (the "no shuffle" flag must not be set).
		 * - C is a non-register number and cannot be shuffled, but
		 *   never needs to be.
		 */
		DUK_ASSERT(st->num_pairs > 0);
		duk__emit_a_b_c(comp_ctx,
		                DUK_OP_MPUTOBJ | DUK__EMIT_FLAG_NO_SHUFFLE_C | DUK__EMIT_FLAG_A_IS_SOURCE,
		                st->reg_obj,
		                st->temp_start,
		                (duk_regconst_t) (st->num_pairs * 2));
		st->num_total_pairs += st->num_pairs;
		st->num_pairs = 0;
	}
	DUK__SETTEMP(comp_ctx, st->temp_start);
}

DUK_LOCAL duk_bool_t duk__objlit_load_key(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_token *tok, duk_regconst_t reg_temp) {
	if (tok->t_nores == DUK_TOK_IDENTIFIER || tok->t_nores == DUK_TOK_STRING) {
		/* same handling for identifiers and strings */
		DUK_ASSERT(tok->str1 != NULL);
		duk_push_hstring(comp_ctx->thr, tok->str1);
	} else if (tok->t == DUK_TOK_NUMBER) {
		/* numbers can be loaded as numbers and coerced on the fly */
		duk_push_number(comp_ctx->thr, tok->num);
	} else {
		return 1; /* error */
	}

	duk__ivalue_plain_fromstack(comp_ctx, res);
	DUK__SETTEMP(comp_ctx, reg_temp + 1);
	duk__ivalue_toforcedreg(comp_ctx, res, reg_temp);
	DUK__SETTEMP(comp_ctx, reg_temp + 1);
	return 0;
}

DUK_LOCAL void duk__nud_object_literal(duk_compiler_ctx *comp_ctx, duk_ivalue *res) {
	duk_hthread *thr = comp_ctx->thr;
	duk__objlit_state st;
	duk_regconst_t reg_temp; /* temp reg */
	duk_small_uint_t max_init_pairs; /* max # of key-value pairs initialized in one MPUTOBJ set */
	duk_bool_t first; /* first value: comma must not precede the value */
	duk_bool_t is_set, is_get; /* temps */
#if !defined(DUK_USE_PREFER_SIZE)
	duk_int_t pc_newobj;
	duk_compiler_instr *instr;
#endif

	DUK_ASSERT(comp_ctx->prev_token.t == DUK_TOK_LCURLY);

	max_init_pairs = DUK__MAX_OBJECT_INIT_PAIRS; /* XXX: depend on available temps? */

	st.reg_obj = DUK__ALLOCTEMP(comp_ctx); /* target object */
	st.temp_start = DUK__GETTEMP(comp_ctx); /* start of MPUTOBJ argument list */
	st.num_pairs = 0; /* number of key/value pairs emitted for current MPUTOBJ set */
	st.num_total_pairs = 0; /* number of key/value pairs emitted overall */

#if !defined(DUK_USE_PREFER_SIZE)
	pc_newobj = duk__get_current_pc(comp_ctx);
#endif
	duk__emit_bc(comp_ctx, DUK_OP_NEWOBJ, st.reg_obj);

	/*
	 *  Emit initializers in sets of maximum max_init_pairs keys.
	 *  Setter/getter is handled separately and terminates the
	 *  current set of initializer values.  Corner cases such as
	 *  single value initializers do not have special handling now.
	 */

	first = 1;
	for (;;) {
		/*
		 *  ES5 and ES2015+ provide a lot of different PropertyDefinition
		 *  formats, see http://www.ecma-international.org/ecma-262/6.0/#sec-object-initializer.
		 *
		 *  PropertyName can be IdentifierName (includes reserved words), a string
		 *  literal, or a number literal.  Note that IdentifierName allows 'get' and
		 *  'set' too, so we need to look ahead to the next token to distinguish:
		 *
		 *     { get : 1 }
		 *
		 *  and
		 *
		 *     { get foo() { return 1 } }
		 *     { get get() { return 1 } }    // 'get' as getter propertyname
		 *
		 *  Finally, a trailing comma is allowed.
		 *
		 *  Key name is coerced to string at compile time (and ends up as a
		 *  a string constant) even for numeric keys (e.g. "{1:'foo'}").
		 *  These could be emitted using e.g. LDINT, but that seems hardly
		 *  worth the effort and would increase code size.
		 */

		DUK_DDD(DUK_DDDPRINT("object literal loop, curr_token->t = %ld", (long) comp_ctx->curr_token.t));

		if (comp_ctx->curr_token.t == DUK_TOK_RCURLY) {
			break;
		}

		if (first) {
			first = 0;
		} else {
			if (comp_ctx->curr_token.t != DUK_TOK_COMMA) {
				goto syntax_error;
			}
			duk__advance(comp_ctx);
			if (comp_ctx->curr_token.t == DUK_TOK_RCURLY) {
				/* trailing comma followed by rcurly */
				break;
			}
		}

		/* Advance to get one step of lookup. */
		duk__advance(comp_ctx);

		/* Flush current MPUTOBJ if enough many pairs gathered. */
		if (st.num_pairs >= max_init_pairs) {
			duk__objlit_flush_keys(comp_ctx, &st);
			DUK_ASSERT(st.num_pairs == 0);
		}

		/* Reset temp register state and reserve reg_temp and
		 * reg_temp + 1 for handling the current property.
		 */
		DUK__SETTEMP(comp_ctx, st.temp_start + 2 * (duk_regconst_t) st.num_pairs);
		reg_temp = DUK__ALLOCTEMPS(comp_ctx, 2);

		/* NOTE: "get" and "set" are not officially ReservedWords and the lexer
		 * currently treats them always like ordinary identifiers (DUK_TOK_GET
		 * and DUK_TOK_SET are unused).  They need to be detected based on the
		 * identifier string content.
		 */

		is_get = (comp_ctx->prev_token.t == DUK_TOK_IDENTIFIER && comp_ctx->prev_token.str1 == DUK_HTHREAD_STRING_GET(thr));
		is_set = (comp_ctx->prev_token.t == DUK_TOK_IDENTIFIER && comp_ctx->prev_token.str1 == DUK_HTHREAD_STRING_SET(thr));
		if ((is_get || is_set) && comp_ctx->curr_token.t != DUK_TOK_COLON) {
			/* getter/setter */
			duk_int_t fnum;

			duk__objlit_flush_keys(comp_ctx, &st);
			DUK_ASSERT(DUK__GETTEMP(comp_ctx) ==
			           st.temp_start); /* 2 regs are guaranteed to be allocated w.r.t. temp_max */
			reg_temp = DUK__ALLOCTEMPS(comp_ctx, 2);

			if (duk__objlit_load_key(comp_ctx, res, &comp_ctx->curr_token, reg_temp) != 0) {
				goto syntax_error;
			}

			/* curr_token = get/set name */
			fnum = duk__parse_func_like_fnum(comp_ctx, DUK__FUNC_FLAG_GETSET);

			duk__emit_a_bc(comp_ctx, DUK_OP_CLOSURE, st.temp_start + 1, (duk_regconst_t) fnum);

			/* Slot C is used in a non-standard fashion (range of regs),
			 * emitter code has special handling for it (must not set the
			 * "no shuffle" flag).
			 */
			duk__emit_a_bc(comp_ctx,
			               (is_get ? DUK_OP_INITGET : DUK_OP_INITSET) | DUK__EMIT_FLAG_A_IS_SOURCE,
			               st.reg_obj,
			               st.temp_start); /* temp_start+0 = key, temp_start+1 = closure */

			DUK_ASSERT(st.num_pairs == 0); /* temp state is reset on next loop */
#if defined(DUK_USE_ES6)
		} else if (comp_ctx->prev_token.t == DUK_TOK_IDENTIFIER &&
		           (comp_ctx->curr_token.t == DUK_TOK_COMMA || comp_ctx->curr_token.t == DUK_TOK_RCURLY)) {
			duk_bool_t load_rc;

			load_rc = duk__objlit_load_key(comp_ctx, res, &comp_ctx->prev_token, reg_temp);
			DUK_UNREF(load_rc);
			DUK_ASSERT(load_rc == 0); /* always succeeds because token is identifier */

			duk__ivalue_var_hstring(comp_ctx, res, comp_ctx->prev_token.str1);
			DUK_ASSERT(DUK__GETTEMP(comp_ctx) == reg_temp + 1);
			duk__ivalue_toforcedreg(comp_ctx, res, reg_temp + 1);

			st.num_pairs++;
		} else if ((comp_ctx->prev_token.t == DUK_TOK_IDENTIFIER || comp_ctx->prev_token.t == DUK_TOK_STRING ||
		            comp_ctx->prev_token.t == DUK_TOK_NUMBER) &&
		           comp_ctx->curr_token.t == DUK_TOK_LPAREN) {
			duk_int_t fnum;

			/* Parsing-wise there's a small hickup here: the token parsing
			 * state is one step too advanced for the function parse helper
			 * compared to other cases.  The current solution is an extra
			 * flag to indicate whether function parsing should use the
			 * current or the previous token to starting parsing from.
			 */

			if (duk__objlit_load_key(comp_ctx, res, &comp_ctx->prev_token, reg_temp) != 0) {
				goto syntax_error;
			}

			fnum = duk__parse_func_like_fnum(comp_ctx, DUK__FUNC_FLAG_USE_PREVTOKEN | DUK__FUNC_FLAG_METDEF);

			duk__emit_a_bc(comp_ctx, DUK_OP_CLOSURE, reg_temp + 1, (duk_regconst_t) fnum);

			st.num_pairs++;
#endif /* DUK_USE_ES6 */
		} else {
#if defined(DUK_USE_ES6)
			if (comp_ctx->prev_token.t == DUK_TOK_LBRACKET) {
				/* ES2015 computed property name.  Executor ToPropertyKey()
				 * coerces the key at runtime.
				 */
				DUK__SETTEMP(comp_ctx, reg_temp);
				duk__expr_toforcedreg(comp_ctx, res, DUK__BP_FOR_EXPR, reg_temp);
				duk__advance_expect(comp_ctx, DUK_TOK_RBRACKET);

				/* XXX: If next token is '(' we're dealing with
				 * the method shorthand with a computed name,
				 * e.g. { [Symbol.for('foo')](a,b) {} }.  This
				 * form is not yet supported and causes a
				 * SyntaxError on the DUK_TOK_COLON check below.
				 */
			} else
#endif /* DUK_USE_ES6 */
			{
				if (duk__objlit_load_key(comp_ctx, res, &comp_ctx->prev_token, reg_temp) != 0) {
					goto syntax_error;
				}
			}

			duk__advance_expect(comp_ctx, DUK_TOK_COLON);

			DUK__SETTEMP(comp_ctx, reg_temp + 1);
			duk__expr_toforcedreg(comp_ctx, res, DUK__BP_COMMA /*rbp_flags*/, reg_temp + 1 /*forced_reg*/);

			st.num_pairs++;
		}
	} /* property loop */

	/* Flush remaining properties. */
	duk__objlit_flush_keys(comp_ctx, &st);
	DUK_ASSERT(st.num_pairs == 0);
	DUK_ASSERT(DUK__GETTEMP(comp_ctx) == st.temp_start);

	/* Update initial size for NEWOBJ.  The init size doesn't need to be
	 * exact as the purpose is just to avoid object resizes in common
	 * cases.  The size is capped to field A limit, and will be too high
	 * if the object literal contains duplicate keys (this is harmless but
	 * increases memory traffic if the object is compacted later on).
	 */
#if !defined(DUK_USE_PREFER_SIZE)
	instr = duk__get_instr_ptr(comp_ctx, pc_newobj);
	instr->ins |= DUK_ENC_OP_A(0, st.num_total_pairs > DUK_BC_A_MAX ? DUK_BC_A_MAX : st.num_total_pairs);
#endif

	DUK_ASSERT(comp_ctx->curr_token.t == DUK_TOK_RCURLY);
	duk__advance(comp_ctx); /* No RegExp after object literal. */

	duk__ivalue_regconst(res, st.reg_obj);
	return;

syntax_error:
	DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_OBJECT_LITERAL);
	DUK_WO_NORETURN(return;);
}

/* Parse argument list.  Arguments are written to temps starting from
 * "next temp".  Returns number of arguments parsed.  Expects left paren
 * to be already eaten, and eats the right paren before returning.
 */
DUK_LOCAL duk_int_t duk__parse_arguments(duk_compiler_ctx *comp_ctx, duk_ivalue *res) {
	duk_int_t nargs = 0;
	duk_regconst_t reg_temp;

	/* Note: expect that caller has already eaten the left paren */

	DUK_DDD(DUK_DDDPRINT("start parsing arguments, prev_token.t=%ld, curr_token.t=%ld",
	                     (long) comp_ctx->prev_token.t,
	                     (long) comp_ctx->curr_token.t));

	for (;;) {
		if (comp_ctx->curr_token.t == DUK_TOK_RPAREN) {
			break;
		}
		if (nargs > 0) {
			duk__advance_expect(comp_ctx, DUK_TOK_COMMA);
		}

		/* We want the argument expression value to go to "next temp"
		 * without additional moves.  That should almost always be the
		 * case, but we double check after expression parsing.
		 *
		 * This is not the cleanest possible approach.
		 */

		reg_temp = DUK__ALLOCTEMP(comp_ctx); /* bump up "allocated" reg count, just in case */
		DUK__SETTEMP(comp_ctx, reg_temp);

		/* binding power must be high enough to NOT allow comma expressions directly */
		duk__expr_toforcedreg(comp_ctx,
		                      res,
		                      DUK__BP_COMMA /*rbp_flags*/,
		                      reg_temp); /* always allow 'in', coerce to 'tr' just in case */

		DUK__SETTEMP(comp_ctx, reg_temp + 1);
		nargs++;

		DUK_DDD(DUK_DDDPRINT("argument #%ld written into reg %ld", (long) nargs, (long) reg_temp));
	}

	/* eat the right paren */
	duk__advance_expect(comp_ctx, DUK_TOK_RPAREN); /* RegExp mode does not matter. */

	DUK_DDD(DUK_DDDPRINT("end parsing arguments"));

	return nargs;
}

DUK_LOCAL duk_bool_t duk__expr_is_empty(duk_compiler_ctx *comp_ctx) {
	/* empty expressions can be detected conveniently with nud/led counts */
	return (comp_ctx->curr_func.nud_count == 0) && (comp_ctx->curr_func.led_count == 0);
}

DUK_LOCAL void duk__expr_nud(duk_compiler_ctx *comp_ctx, duk_ivalue *res) {
	duk_hthread *thr = comp_ctx->thr;
	duk_token *tk;
	duk_regconst_t temp_at_entry;
	duk_small_uint_t tok;
	duk_uint32_t args; /* temp variable to pass constants and flags to shared code */

	/*
	 *  ctx->prev_token     token to process with duk__expr_nud()
	 *  ctx->curr_token     updated by caller
	 *
	 *  Note: the token in the switch below has already been eaten.
	 */

	temp_at_entry = DUK__GETTEMP(comp_ctx);

	comp_ctx->curr_func.nud_count++;

	tk = &comp_ctx->prev_token;
	tok = tk->t;
	res->t = DUK_IVAL_NONE;

	DUK_DDD(DUK_DDDPRINT("duk__expr_nud(), prev_token.t=%ld, allow_in=%ld, paren_level=%ld",
	                     (long) tk->t,
	                     (long) comp_ctx->curr_func.allow_in,
	                     (long) comp_ctx->curr_func.paren_level));

	switch (tok) {
		/* PRIMARY EXPRESSIONS */

	case DUK_TOK_THIS: {
		duk_regconst_t reg_temp;
		reg_temp = DUK__ALLOCTEMP(comp_ctx);
		duk__emit_bc(comp_ctx, DUK_OP_LDTHIS, reg_temp);
		duk__ivalue_regconst(res, reg_temp);
		return;
	}
	case DUK_TOK_IDENTIFIER: {
		duk__ivalue_var_hstring(comp_ctx, res, tk->str1);
		return;
	}
	case DUK_TOK_NULL: {
		duk_push_null(thr);
		goto plain_value;
	}
	case DUK_TOK_TRUE: {
		duk_push_true(thr);
		goto plain_value;
	}
	case DUK_TOK_FALSE: {
		duk_push_false(thr);
		goto plain_value;
	}
	case DUK_TOK_NUMBER: {
		duk_push_number(thr, tk->num);
		goto plain_value;
	}
	case DUK_TOK_STRING: {
		DUK_ASSERT(tk->str1 != NULL);
		duk_push_hstring(thr, tk->str1);
		goto plain_value;
	}
	case DUK_TOK_REGEXP: {
#if defined(DUK_USE_REGEXP_SUPPORT)
		duk_regconst_t reg_temp;
		duk_regconst_t rc_re_bytecode; /* const */
		duk_regconst_t rc_re_source; /* const */

		DUK_ASSERT(tk->str1 != NULL);
		DUK_ASSERT(tk->str2 != NULL);

		DUK_DDD(DUK_DDDPRINT("emitting regexp op, str1=%!O, str2=%!O", (duk_heaphdr *) tk->str1, (duk_heaphdr *) tk->str2));

		reg_temp = DUK__ALLOCTEMP(comp_ctx);
		duk_push_hstring(thr, tk->str1);
		duk_push_hstring(thr, tk->str2);

		/* [ ... pattern flags ] */

		duk_regexp_compile(thr);

		/* [ ... escaped_source bytecode ] */

		rc_re_bytecode = duk__getconst(comp_ctx);
		rc_re_source = duk__getconst(comp_ctx);

		duk__emit_a_b_c(comp_ctx,
		                DUK_OP_REGEXP | DUK__EMIT_FLAG_BC_REGCONST,
		                reg_temp /*a*/,
		                rc_re_bytecode /*b*/,
		                rc_re_source /*c*/);

		duk__ivalue_regconst(res, reg_temp);
		return;
#else /* DUK_USE_REGEXP_SUPPORT */
		goto syntax_error;
#endif /* DUK_USE_REGEXP_SUPPORT */
	}
	case DUK_TOK_LBRACKET: {
		DUK_DDD(DUK_DDDPRINT("parsing array literal"));
		duk__nud_array_literal(comp_ctx, res);
		return;
	}
	case DUK_TOK_LCURLY: {
		DUK_DDD(DUK_DDDPRINT("parsing object literal"));
		duk__nud_object_literal(comp_ctx, res);
		return;
	}
	case DUK_TOK_LPAREN: {
		duk_bool_t prev_allow_in;

		comp_ctx->curr_func.paren_level++;
		prev_allow_in = comp_ctx->curr_func.allow_in;
		comp_ctx->curr_func.allow_in = 1; /* reset 'allow_in' for parenthesized expression */

		duk__expr(comp_ctx, res, DUK__BP_FOR_EXPR /*rbp_flags*/); /* Expression, terminates at a ')' */

		duk__advance_expect(comp_ctx, DUK_TOK_RPAREN); /* No RegExp after parenthesized expression. */
		comp_ctx->curr_func.allow_in = prev_allow_in;
		comp_ctx->curr_func.paren_level--;
		return;
	}

		/* MEMBER/NEW/CALL EXPRESSIONS */

	case DUK_TOK_NEW: {
		/*
		 *  Parsing an expression starting with 'new' is tricky because
		 *  there are multiple possible productions deriving from
		 *  LeftHandSideExpression which begin with 'new'.
		 *
		 *  We currently resort to one-token lookahead to distinguish the
		 *  cases.  Hopefully this is correct.  The binding power must be
		 *  such that parsing ends at an LPAREN (CallExpression) but not at
		 *  a PERIOD or LBRACKET (MemberExpression).
		 *
		 *  See doc/compiler.rst for discussion on the parsing approach,
		 *  and testcases/test-dev-new.js for a bunch of documented tests.
		 */

		duk_regconst_t reg_target;
		duk_int_t nargs;

		DUK_DDD(DUK_DDDPRINT("begin parsing new expression"));

		reg_target = DUK__ALLOCTEMPS(comp_ctx, 2);

#if defined(DUK_USE_ES6)
		if (comp_ctx->curr_token.t == DUK_TOK_PERIOD) {
			/* new.target */
			DUK_DDD(DUK_DDDPRINT("new.target"));
			duk__advance(comp_ctx);
			if (comp_ctx->curr_token.t_nores != DUK_TOK_IDENTIFIER ||
			    !duk_hstring_equals_ascii_cstring(comp_ctx->curr_token.str1, "target")) {
				goto syntax_error_newtarget;
			}
			if (comp_ctx->curr_func.is_global) {
				goto syntax_error_newtarget;
			}
			duk__advance(comp_ctx);
			duk__emit_bc(comp_ctx, DUK_OP_NEWTARGET, reg_target);
			duk__ivalue_regconst(res, reg_target);
			return;
		}
#endif /* DUK_USE_ES6 */

		duk__expr_toforcedreg(comp_ctx, res, DUK__BP_CALL /*rbp_flags*/, reg_target /*forced_reg*/);
		duk__emit_bc(comp_ctx, DUK_OP_NEWOBJ, reg_target + 1); /* default instance */
		DUK__SETTEMP(comp_ctx, reg_target + 2);

		/* XXX: 'new obj.noSuch()' doesn't use GETPROPC now which
		 * makes the error message worse than for obj.noSuch().
		 */

		if (comp_ctx->curr_token.t == DUK_TOK_LPAREN) {
			/* 'new' MemberExpression Arguments */
			DUK_DDD(DUK_DDDPRINT("new expression has argument list"));
			duk__advance(comp_ctx);
			nargs = duk__parse_arguments(comp_ctx, res); /* parse args starting from "next temp", reg_target + 1 */
			/* right paren eaten */
		} else {
			/* 'new' MemberExpression */
			DUK_DDD(DUK_DDDPRINT("new expression has no argument list"));
			nargs = 0;
		}

		duk__emit_a_bc(comp_ctx, DUK_OP_CALL0 | DUK_BC_CALL_FLAG_CONSTRUCT, nargs /*num_args*/, reg_target /*target*/);

		DUK_DDD(DUK_DDDPRINT("end parsing new expression"));

		duk__ivalue_regconst(res, reg_target);
		return;
	}

		/* FUNCTION EXPRESSIONS */

	case DUK_TOK_FUNCTION: {
		/* Function expression.  Note that any statement beginning with 'function'
		 * is handled by the statement parser as a function declaration, or a
		 * non-standard function expression/statement (or a SyntaxError).  We only
		 * handle actual function expressions (occurring inside an expression) here.
		 *
		 * O(depth^2) parse count for inner functions is handled by recording a
		 * lexer offset on the first compilation pass, so that the function can
		 * be efficiently skipped on the second pass.  This is encapsulated into
		 * duk__parse_func_like_fnum().
		 */

		duk_regconst_t reg_temp;
		duk_int_t fnum;

		reg_temp = DUK__ALLOCTEMP(comp_ctx);

		/* curr_token follows 'function' */
		fnum = duk__parse_func_like_fnum(comp_ctx, 0 /*flags*/);
		DUK_DDD(DUK_DDDPRINT("parsed inner function -> fnum %ld", (long) fnum));

		duk__emit_a_bc(comp_ctx, DUK_OP_CLOSURE, reg_temp /*a*/, (duk_regconst_t) fnum /*bc*/);

		duk__ivalue_regconst(res, reg_temp);
		return;
	}

		/* UNARY EXPRESSIONS */

	case DUK_TOK_DELETE: {
		/* Delete semantics are a bit tricky.  The description in E5 specification
		 * is kind of confusing, because it distinguishes between resolvability of
		 * a reference (which is only known at runtime) seemingly at compile time
		 * (= SyntaxError throwing).
		 */
		duk__expr(comp_ctx, res, DUK__BP_MULTIPLICATIVE /*rbp_flags*/); /* UnaryExpression */
		if (res->t == DUK_IVAL_VAR) {
			/* not allowed in strict mode, regardless of whether resolves;
			 * in non-strict mode DELVAR handles both non-resolving and
			 * resolving cases (the specification description is a bit confusing).
			 */

			duk_regconst_t reg_temp;
			duk_regconst_t reg_varbind;
			duk_regconst_t rc_varname;

			if (comp_ctx->curr_func.is_strict) {
				DUK_ERROR_SYNTAX(thr, DUK_STR_CANNOT_DELETE_IDENTIFIER);
				DUK_WO_NORETURN(return;);
			}

			DUK__SETTEMP(comp_ctx, temp_at_entry);
			reg_temp = DUK__ALLOCTEMP(comp_ctx);

			duk_dup(thr, res->x1.valstack_idx);
			if (duk__lookup_lhs(comp_ctx, &reg_varbind, &rc_varname)) {
				/* register bound variables are non-configurable -> always false */
				duk__emit_bc(comp_ctx, DUK_OP_LDFALSE, reg_temp);
			} else {
				duk_dup(thr, res->x1.valstack_idx);
				rc_varname = duk__getconst(comp_ctx);
				duk__emit_a_bc(comp_ctx, DUK_OP_DELVAR, reg_temp, rc_varname);
			}
			duk__ivalue_regconst(res, reg_temp);
		} else if (res->t == DUK_IVAL_PROP) {
			duk_regconst_t reg_temp;
			duk_regconst_t reg_obj;
			duk_regconst_t rc_key;

			DUK__SETTEMP(comp_ctx, temp_at_entry);
			reg_temp = DUK__ALLOCTEMP(comp_ctx);
			reg_obj =
			    duk__ispec_toregconst_raw(comp_ctx, &res->x1, -1 /*forced_reg*/, 0 /*flags*/); /* don't allow const */
			rc_key =
			    duk__ispec_toregconst_raw(comp_ctx, &res->x2, -1 /*forced_reg*/, DUK__IVAL_FLAG_ALLOW_CONST /*flags*/);
			duk__emit_a_b_c(comp_ctx, DUK_OP_DELPROP | DUK__EMIT_FLAG_BC_REGCONST, reg_temp, reg_obj, rc_key);

			duk__ivalue_regconst(res, reg_temp);
		} else {
			/* non-Reference deletion is always 'true', even in strict mode */
			duk_push_true(thr);
			goto plain_value;
		}
		return;
	}
	case DUK_TOK_VOID: {
		duk__expr_toplain_ignore(comp_ctx, res, DUK__BP_MULTIPLICATIVE /*rbp_flags*/); /* UnaryExpression */
		duk_push_undefined(thr);
		goto plain_value;
	}
	case DUK_TOK_TYPEOF: {
		/* 'typeof' must handle unresolvable references without throwing
		 * a ReferenceError (E5 Section 11.4.3).  Register mapped values
		 * will never be unresolvable so special handling is only required
		 * when an identifier is a "slow path" one.
		 */
		duk__expr(comp_ctx, res, DUK__BP_MULTIPLICATIVE /*rbp_flags*/); /* UnaryExpression */

		if (res->t == DUK_IVAL_VAR) {
			duk_regconst_t reg_varbind;
			duk_regconst_t rc_varname;
			duk_regconst_t reg_temp;

			duk_dup(thr, res->x1.valstack_idx);
			if (!duk__lookup_lhs(comp_ctx, &reg_varbind, &rc_varname)) {
				DUK_DDD(DUK_DDDPRINT("typeof for an identifier name which could not be resolved "
				                     "at compile time, need to use special run-time handling"));
				reg_temp = DUK__ALLOCTEMP(comp_ctx);
				duk__emit_a_bc(comp_ctx, DUK_OP_TYPEOFID, reg_temp, rc_varname);
				duk__ivalue_regconst(res, reg_temp);
				return;
			}
		}

		args = DUK_OP_TYPEOF;
		goto unary;
	}
	case DUK_TOK_INCREMENT: {
		args = (DUK_OP_PREINCP << 8) + DUK_OP_PREINCR;
		goto preincdec;
	}
	case DUK_TOK_DECREMENT: {
		args = (DUK_OP_PREDECP << 8) + DUK_OP_PREDECR;
		goto preincdec;
	}
	case DUK_TOK_ADD: {
		/* unary plus */
		duk__expr(comp_ctx, res, DUK__BP_MULTIPLICATIVE /*rbp_flags*/); /* UnaryExpression */
		if (res->t == DUK_IVAL_PLAIN && res->x1.t == DUK_ISPEC_VALUE && duk_is_number(thr, res->x1.valstack_idx)) {
			/* unary plus of a number is identity */
			return;
		}
		args = DUK_OP_UNP;
		goto unary;
	}
	case DUK_TOK_SUB: {
		/* unary minus */
		duk__expr(comp_ctx, res, DUK__BP_MULTIPLICATIVE /*rbp_flags*/); /* UnaryExpression */
		if (res->t == DUK_IVAL_PLAIN && res->x1.t == DUK_ISPEC_VALUE && duk_is_number(thr, res->x1.valstack_idx)) {
			/* this optimization is important to handle negative literals
			 * (which are not directly provided by the lexical grammar)
			 */
			duk_tval *tv_num;
			duk_double_union du;

			tv_num = DUK_GET_TVAL_POSIDX(thr, res->x1.valstack_idx);
			DUK_ASSERT(tv_num != NULL);
			DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv_num));
			du.d = DUK_TVAL_GET_NUMBER(tv_num);
			du.d = -du.d;
			DUK_DBLUNION_NORMALIZE_NAN_CHECK(&du);
			DUK_TVAL_SET_NUMBER(tv_num, du.d);
			return;
		}
		args = DUK_OP_UNM;
		goto unary;
	}
	case DUK_TOK_BNOT: {
		duk__expr(comp_ctx, res, DUK__BP_MULTIPLICATIVE /*rbp_flags*/); /* UnaryExpression */
		args = DUK_OP_BNOT;
		goto unary;
	}
	case DUK_TOK_LNOT: {
		duk__expr(comp_ctx, res, DUK__BP_MULTIPLICATIVE /*rbp_flags*/); /* UnaryExpression */
		if (res->t == DUK_IVAL_PLAIN && res->x1.t == DUK_ISPEC_VALUE) {
			/* Very minimal inlining to handle common idioms '!0' and '!1',
			 * and also boolean arguments like '!false' and '!true'.
			 */
			duk_tval *tv_val;

			tv_val = DUK_GET_TVAL_POSIDX(thr, res->x1.valstack_idx);
			DUK_ASSERT(tv_val != NULL);
			if (DUK_TVAL_IS_NUMBER(tv_val)) {
				duk_double_t d;
				d = DUK_TVAL_GET_NUMBER(tv_val);
				if (duk_double_equals(d, 0.0)) {
					/* Matches both +0 and -0 on purpose. */
					DUK_DDD(DUK_DDDPRINT("inlined lnot: !0 -> true"));
					DUK_TVAL_SET_BOOLEAN_TRUE(tv_val);
					return;
				} else if (duk_double_equals(d, 1.0)) {
					DUK_DDD(DUK_DDDPRINT("inlined lnot: !1 -> false"));
					DUK_TVAL_SET_BOOLEAN_FALSE(tv_val);
					return;
				}
			} else if (DUK_TVAL_IS_BOOLEAN(tv_val)) {
				duk_small_uint_t v;
				v = DUK_TVAL_GET_BOOLEAN(tv_val);
				DUK_DDD(DUK_DDDPRINT("inlined lnot boolean: %ld", (long) v));
				DUK_ASSERT(v == 0 || v == 1);
				DUK_TVAL_SET_BOOLEAN(tv_val, v ^ 0x01);
				return;
			}
		}
		args = DUK_OP_LNOT;
		goto unary;
	}

	} /* end switch */

	DUK_ERROR_SYNTAX(thr, DUK_STR_PARSE_ERROR);
	DUK_WO_NORETURN(return;);

unary : {
	/* Unary opcodes use just the 'BC' register source because it
	 * matches current shuffle limits, and maps cleanly to 16 high
	 * bits of the opcode.
	 */

	duk_regconst_t reg_src, reg_res;

	reg_src = duk__ivalue_toregconst_raw(comp_ctx, res, -1 /*forced_reg*/, 0 /*flags*/);
	if (DUK__ISREG_TEMP(comp_ctx, reg_src)) {
		reg_res = reg_src;
	} else {
		reg_res = DUK__ALLOCTEMP(comp_ctx);
	}
	duk__emit_a_bc(comp_ctx, args, reg_res, reg_src);
	duk__ivalue_regconst(res, reg_res);
	return;
}

preincdec : {
	/* preincrement and predecrement */
	duk_regconst_t reg_res;
	duk_small_uint_t args_op1 = args & 0xff; /* DUK_OP_PREINCR/DUK_OP_PREDECR */
	duk_small_uint_t args_op2 = args >> 8; /* DUK_OP_PREINCP_RR/DUK_OP_PREDECP_RR */

	/* Specific assumptions for opcode numbering. */
	DUK_ASSERT(DUK_OP_PREINCR + 4 == DUK_OP_PREINCV);
	DUK_ASSERT(DUK_OP_PREDECR + 4 == DUK_OP_PREDECV);

	reg_res = DUK__ALLOCTEMP(comp_ctx);

	duk__expr(comp_ctx, res, DUK__BP_MULTIPLICATIVE /*rbp_flags*/); /* UnaryExpression */
	if (res->t == DUK_IVAL_VAR) {
		duk_hstring *h_varname;
		duk_regconst_t reg_varbind;
		duk_regconst_t rc_varname;

		h_varname = duk_known_hstring(thr, res->x1.valstack_idx);

		if (duk__hstring_is_eval_or_arguments_in_strict_mode(comp_ctx, h_varname)) {
			goto syntax_error;
		}

		duk_dup(thr, res->x1.valstack_idx);
		if (duk__lookup_lhs(comp_ctx, &reg_varbind, &rc_varname)) {
			duk__emit_a_bc(comp_ctx,
			               args_op1, /* e.g. DUK_OP_PREINCR */
			               reg_res,
			               reg_varbind);
		} else {
			duk__emit_a_bc(comp_ctx,
			               args_op1 + 4, /* e.g. DUK_OP_PREINCV */
			               reg_res,
			               rc_varname);
		}

		DUK_DDD(DUK_DDDPRINT("preincdec to '%!O' -> reg_varbind=%ld, rc_varname=%ld",
		                     (duk_heaphdr *) h_varname,
		                     (long) reg_varbind,
		                     (long) rc_varname));
	} else if (res->t == DUK_IVAL_PROP) {
		duk_regconst_t reg_obj; /* allocate to reg only (not const) */
		duk_regconst_t rc_key;
		reg_obj = duk__ispec_toregconst_raw(comp_ctx, &res->x1, -1 /*forced_reg*/, 0 /*flags*/); /* don't allow const */
		rc_key = duk__ispec_toregconst_raw(comp_ctx, &res->x2, -1 /*forced_reg*/, DUK__IVAL_FLAG_ALLOW_CONST /*flags*/);
		duk__emit_a_b_c(comp_ctx,
		                args_op2 | DUK__EMIT_FLAG_BC_REGCONST, /* e.g. DUK_OP_PREINCP */
		                reg_res,
		                reg_obj,
		                rc_key);
	} else {
		/* Technically return value is not needed because INVLHS will
		 * unconditially throw a ReferenceError.  Coercion is necessary
		 * for proper semantics (consider ToNumber() called for an object).
		 * Use DUK_OP_UNP with a dummy register to get ToNumber().
		 */

		duk__ivalue_toforcedreg(comp_ctx, res, reg_res);
		duk__emit_bc(comp_ctx, DUK_OP_UNP, reg_res); /* for side effects, result ignored */
		duk__emit_op_only(comp_ctx, DUK_OP_INVLHS);
	}
	DUK__SETTEMP(comp_ctx, reg_res + 1);
	duk__ivalue_regconst(res, reg_res);
	return;
}

plain_value : {
	/* Stack top contains plain value */
	duk__ivalue_plain_fromstack(comp_ctx, res);
	return;
}

#if defined(DUK_USE_ES6)
syntax_error_newtarget:
	DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_NEWTARGET);
	DUK_WO_NORETURN(return;);
#endif

syntax_error:
	DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_EXPRESSION);
	DUK_WO_NORETURN(return;);
}

/* XXX: add flag to indicate whether caller cares about return value; this
 * affects e.g. handling of assignment expressions.  This change needs API
 * changes elsewhere too.
 */
DUK_LOCAL void duk__expr_led(duk_compiler_ctx *comp_ctx, duk_ivalue *left, duk_ivalue *res) {
	duk_hthread *thr = comp_ctx->thr;
	duk_token *tk;
	duk_small_uint_t tok;
	duk_uint32_t args; /* temp variable to pass constants and flags to shared code */

	/*
	 *  ctx->prev_token     token to process with duk__expr_led()
	 *  ctx->curr_token     updated by caller
	 */

	comp_ctx->curr_func.led_count++;

	/* The token in the switch has already been eaten here */
	tk = &comp_ctx->prev_token;
	tok = tk->t;

	DUK_DDD(DUK_DDDPRINT("duk__expr_led(), prev_token.t=%ld, allow_in=%ld, paren_level=%ld",
	                     (long) tk->t,
	                     (long) comp_ctx->curr_func.allow_in,
	                     (long) comp_ctx->curr_func.paren_level));

	/* XXX: default priority for infix operators is duk__expr_lbp(tok) -> get it here? */

	switch (tok) {
		/* PRIMARY EXPRESSIONS */

	case DUK_TOK_PERIOD: {
		/* Property access expressions are critical for correct LHS ordering,
		 * see comments in duk__expr()!
		 *
		 * A conservative approach would be to use duk__ivalue_totempconst()
		 * for 'left'.  However, allowing a reg-bound variable seems safe here
		 * and is nice because "foo.bar" is a common expression.  If the ivalue
		 * is used in an expression a GETPROP will occur before any changes to
		 * the base value can occur.  If the ivalue is used as an assignment
		 * LHS, the assignment code will ensure the base value is safe from
		 * RHS mutation.
		 */

		/* XXX: This now coerces an identifier into a GETVAR to a temp, which
		 * causes an extra LDREG in call setup.  It's sufficient to coerce to a
		 * unary ivalue?
		 */
		duk__ivalue_toplain(comp_ctx, left);

		/* NB: must accept reserved words as property name */
		if (comp_ctx->curr_token.t_nores != DUK_TOK_IDENTIFIER) {
			DUK_ERROR_SYNTAX(thr, DUK_STR_EXPECTED_IDENTIFIER);
			DUK_WO_NORETURN(return;);
		}

		res->t = DUK_IVAL_PROP;
		duk__copy_ispec(comp_ctx, &left->x1, &res->x1); /* left.x1 -> res.x1 */
		DUK_ASSERT(comp_ctx->curr_token.str1 != NULL);
		duk_push_hstring(thr, comp_ctx->curr_token.str1);
		duk_replace(thr, res->x2.valstack_idx);
		res->x2.t = DUK_ISPEC_VALUE;

		/* special RegExp literal handling after IdentifierName */
		comp_ctx->curr_func.reject_regexp_in_adv = 1;

		duk__advance(comp_ctx);
		return;
	}
	case DUK_TOK_LBRACKET: {
		/* Property access expressions are critical for correct LHS ordering,
		 * see comments in duk__expr()!
		 */

		/* XXX: optimize temp reg use */
		/* XXX: similar coercion issue as in DUK_TOK_PERIOD */
		/* XXX: coerce to regs? it might be better for enumeration use, where the
		 * same PROP ivalue is used multiple times.  Or perhaps coerce PROP further
		 * there?
		 */
		/* XXX: for simple cases like x['y'] an unnecessary LDREG is
		 * emitted for the base value; could avoid it if we knew that
		 * the key expression is safe (e.g. just a single literal).
		 */

		/* The 'left' value must not be a register bound variable
		 * because it may be mutated during the rest of the expression
		 * and E5.1 Section 11.2.1 specifies the order of evaluation
		 * so that the base value is evaluated first.
		 * See: test-bug-nested-prop-mutate.js.
		 */
		duk__ivalue_totempconst(comp_ctx, left);
		duk__expr_toplain(comp_ctx, res, DUK__BP_FOR_EXPR /*rbp_flags*/); /* Expression, ']' terminates */
		duk__advance_expect(comp_ctx, DUK_TOK_RBRACKET);

		res->t = DUK_IVAL_PROP;
		duk__copy_ispec(comp_ctx, &res->x1, &res->x2); /* res.x1 -> res.x2 */
		duk__copy_ispec(comp_ctx, &left->x1, &res->x1); /* left.x1 -> res.x1 */
		return;
	}
	case DUK_TOK_LPAREN: {
		/* function call */
		duk_regconst_t reg_cs = DUK__ALLOCTEMPS(comp_ctx, 2);
		duk_int_t nargs;
		duk_small_uint_t call_op = DUK_OP_CALL0;

		/* XXX: attempt to get the call result to "next temp" whenever
		 * possible to avoid unnecessary register shuffles.
		 */

		/*
		 *  Setup call: target and 'this' binding.  Three cases:
		 *
		 *    1. Identifier base (e.g. "foo()")
		 *    2. Property base (e.g. "foo.bar()")
		 *    3. Register base (e.g. "foo()()"; i.e. when a return value is a function)
		 */

		if (left->t == DUK_IVAL_VAR) {
			duk_hstring *h_varname;
			duk_regconst_t reg_varbind;
			duk_regconst_t rc_varname;

			DUK_DDD(DUK_DDDPRINT("function call with identifier base"));

			h_varname = duk_known_hstring(thr, left->x1.valstack_idx);
			if (h_varname == DUK_HTHREAD_STRING_EVAL(thr)) {
				/* Potential direct eval call detected, flag the CALL
				 * so that a run-time "direct eval" check is made and
				 * special behavior may be triggered.  Note that this
				 * does not prevent 'eval' from being register bound.
				 */
				DUK_DDD(DUK_DDDPRINT("function call with identifier 'eval' "
				                     "-> using EVALCALL, marking function "
				                     "as may_direct_eval"));
				call_op |= DUK_BC_CALL_FLAG_CALLED_AS_EVAL;
				comp_ctx->curr_func.may_direct_eval = 1;
			}

			duk_dup(thr, left->x1.valstack_idx);
			if (duk__lookup_lhs(comp_ctx, &reg_varbind, &rc_varname)) {
				duk__emit_a_bc(comp_ctx, DUK_OP_CSREG | DUK__EMIT_FLAG_A_IS_SOURCE, reg_varbind, reg_cs + 0);
			} else {
				/* XXX: expand target register or constant field to
				 * reduce shuffling.
				 */
				DUK_ASSERT(DUK__ISCONST(rc_varname));
				duk__emit_a_b(comp_ctx, DUK_OP_CSVAR | DUK__EMIT_FLAG_BC_REGCONST, reg_cs + 0, rc_varname);
			}
		} else if (left->t == DUK_IVAL_PROP) {
			/* Call through a property lookup, E5 Section 11.2.3, step 6.a.i,
			 * E5 Section 10.4.3.  There used to be a separate CSPROP opcode
			 * but a typical call setup took 3 opcodes (e.g. LDREG, LDCONST,
			 * CSPROP) and the same can be achieved with ordinary loads.
			 */
#if defined(DUK_USE_VERBOSE_ERRORS)
			duk_regconst_t reg_key;
#endif

			DUK_DDD(DUK_DDDPRINT("function call with property base"));

			/* XXX: For Math.sin() this generates: LDCONST + LDREG +
			 * GETPROPC + call.  The LDREG is unnecessary because LDCONST
			 * could be loaded directly into reg_cs + 1.  This doesn't
			 * happen now because a variable cannot be in left->x1 of a
			 * DUK_IVAL_PROP.  We could notice that left->x1 is a temp
			 * and reuse, but it would still be in the wrong position
			 * (reg_cs + 0 rather than reg_cs + 1).
			 */
			duk__ispec_toforcedreg(comp_ctx, &left->x1, reg_cs + 1); /* base */
#if defined(DUK_USE_VERBOSE_ERRORS)
			reg_key = duk__ispec_toregconst_raw(comp_ctx, &left->x2, -1, DUK__IVAL_FLAG_ALLOW_CONST /*flags*/);
			duk__emit_a_b_c(comp_ctx, DUK_OP_GETPROPC | DUK__EMIT_FLAG_BC_REGCONST, reg_cs + 0, reg_cs + 1, reg_key);
#else
			duk__ivalue_toforcedreg(comp_ctx, left, reg_cs + 0); /* base[key] */
#endif
		} else {
			DUK_DDD(DUK_DDDPRINT("function call with register base"));

			duk__ivalue_toforcedreg(comp_ctx, left, reg_cs + 0);
#if 0
			duk__emit_a_bc(comp_ctx,
			               DUK_OP_CSREG | DUK__EMIT_FLAG_A_IS_SOURCE,
			               reg_cs + 0,
			               reg_cs + 0);  /* in-place setup */
#endif
			/* Because of in-place setup, REGCS is equivalent to
			 * just this LDUNDEF.
			 */
			duk__emit_bc(comp_ctx, DUK_OP_LDUNDEF, reg_cs + 1);
		}

		DUK__SETTEMP(comp_ctx, reg_cs + 2);
		nargs = duk__parse_arguments(comp_ctx, res); /* parse args starting from "next temp" */

		/* Tailcalls are handled by back-patching the already emitted opcode
		 * later in return statement parser.
		 */

		duk__emit_a_bc(comp_ctx, call_op, (duk_regconst_t) nargs /*numargs*/, reg_cs /*basereg*/);
		DUK__SETTEMP(comp_ctx, reg_cs + 1); /* result in csreg */

		duk__ivalue_regconst(res, reg_cs);
		return;
	}

		/* POSTFIX EXPRESSION */

	case DUK_TOK_INCREMENT: {
		args = (DUK_OP_POSTINCP_RR << 16) + (DUK_OP_POSTINCR << 8) + 0;
		goto postincdec;
	}
	case DUK_TOK_DECREMENT: {
		args = (DUK_OP_POSTDECP_RR << 16) + (DUK_OP_POSTDECR << 8) + 0;
		goto postincdec;
	}

	/* EXPONENTIATION EXPRESSION */

#if defined(DUK_USE_ES7_EXP_OPERATOR)
	case DUK_TOK_EXP: {
		args = (DUK_OP_EXP << 8) + DUK__BP_EXPONENTIATION - 1; /* UnaryExpression */
		goto binary;
	}
#endif

		/* MULTIPLICATIVE EXPRESSION */

	case DUK_TOK_MUL: {
		args = (DUK_OP_MUL << 8) + DUK__BP_MULTIPLICATIVE; /* ExponentiationExpression */
		goto binary;
	}
	case DUK_TOK_DIV: {
		args = (DUK_OP_DIV << 8) + DUK__BP_MULTIPLICATIVE; /* ExponentiationExpression */
		goto binary;
	}
	case DUK_TOK_MOD: {
		args = (DUK_OP_MOD << 8) + DUK__BP_MULTIPLICATIVE; /* ExponentiationExpression */
		goto binary;
	}

		/* ADDITIVE EXPRESSION */

	case DUK_TOK_ADD: {
		args = (DUK_OP_ADD << 8) + DUK__BP_ADDITIVE; /* MultiplicativeExpression */
		goto binary;
	}
	case DUK_TOK_SUB: {
		args = (DUK_OP_SUB << 8) + DUK__BP_ADDITIVE; /* MultiplicativeExpression */
		goto binary;
	}

		/* SHIFT EXPRESSION */

	case DUK_TOK_ALSHIFT: {
		/* << */
		args = (DUK_OP_BASL << 8) + DUK__BP_SHIFT;
		goto binary;
	}
	case DUK_TOK_ARSHIFT: {
		/* >> */
		args = (DUK_OP_BASR << 8) + DUK__BP_SHIFT;
		goto binary;
	}
	case DUK_TOK_RSHIFT: {
		/* >>> */
		args = (DUK_OP_BLSR << 8) + DUK__BP_SHIFT;
		goto binary;
	}

		/* RELATIONAL EXPRESSION */

	case DUK_TOK_LT: {
		/* < */
		args = (DUK_OP_LT << 8) + DUK__BP_RELATIONAL;
		goto binary;
	}
	case DUK_TOK_GT: {
		args = (DUK_OP_GT << 8) + DUK__BP_RELATIONAL;
		goto binary;
	}
	case DUK_TOK_LE: {
		args = (DUK_OP_LE << 8) + DUK__BP_RELATIONAL;
		goto binary;
	}
	case DUK_TOK_GE: {
		args = (DUK_OP_GE << 8) + DUK__BP_RELATIONAL;
		goto binary;
	}
	case DUK_TOK_INSTANCEOF: {
		args = (DUK_OP_INSTOF << 8) + DUK__BP_RELATIONAL;
		goto binary;
	}
	case DUK_TOK_IN: {
		args = (DUK_OP_IN << 8) + DUK__BP_RELATIONAL;
		goto binary;
	}

		/* EQUALITY EXPRESSION */

	case DUK_TOK_EQ: {
		args = (DUK_OP_EQ << 8) + DUK__BP_EQUALITY;
		goto binary;
	}
	case DUK_TOK_NEQ: {
		args = (DUK_OP_NEQ << 8) + DUK__BP_EQUALITY;
		goto binary;
	}
	case DUK_TOK_SEQ: {
		args = (DUK_OP_SEQ << 8) + DUK__BP_EQUALITY;
		goto binary;
	}
	case DUK_TOK_SNEQ: {
		args = (DUK_OP_SNEQ << 8) + DUK__BP_EQUALITY;
		goto binary;
	}

		/* BITWISE EXPRESSIONS */

	case DUK_TOK_BAND: {
		args = (DUK_OP_BAND << 8) + DUK__BP_BAND;
		goto binary;
	}
	case DUK_TOK_BXOR: {
		args = (DUK_OP_BXOR << 8) + DUK__BP_BXOR;
		goto binary;
	}
	case DUK_TOK_BOR: {
		args = (DUK_OP_BOR << 8) + DUK__BP_BOR;
		goto binary;
	}

		/* LOGICAL EXPRESSIONS */

	case DUK_TOK_LAND: {
		/* syntactically left-associative but parsed as right-associative */
		args = (1 << 8) + DUK__BP_LAND - 1;
		goto binary_logical;
	}
	case DUK_TOK_LOR: {
		/* syntactically left-associative but parsed as right-associative */
		args = (0 << 8) + DUK__BP_LOR - 1;
		goto binary_logical;
	}

		/* CONDITIONAL EXPRESSION */

	case DUK_TOK_QUESTION: {
		/* XXX: common reg allocation need is to reuse a sub-expression's temp reg,
		 * but only if it really is a temp.  Nothing fancy here now.
		 */
		duk_regconst_t reg_temp;
		duk_int_t pc_jump1;
		duk_int_t pc_jump2;

		reg_temp = DUK__ALLOCTEMP(comp_ctx);
		duk__ivalue_toforcedreg(comp_ctx, left, reg_temp);
		duk__emit_if_true_skip(comp_ctx, reg_temp);
		pc_jump1 = duk__emit_jump_empty(comp_ctx); /* jump to false */
		duk__expr_toforcedreg(comp_ctx,
		                      res,
		                      DUK__BP_COMMA /*rbp_flags*/,
		                      reg_temp /*forced_reg*/); /* AssignmentExpression */
		duk__advance_expect(comp_ctx, DUK_TOK_COLON);
		pc_jump2 = duk__emit_jump_empty(comp_ctx); /* jump to end */
		duk__patch_jump_here(comp_ctx, pc_jump1);
		duk__expr_toforcedreg(comp_ctx,
		                      res,
		                      DUK__BP_COMMA /*rbp_flags*/,
		                      reg_temp /*forced_reg*/); /* AssignmentExpression */
		duk__patch_jump_here(comp_ctx, pc_jump2);

		DUK__SETTEMP(comp_ctx, reg_temp + 1);
		duk__ivalue_regconst(res, reg_temp);
		return;
	}

		/* ASSIGNMENT EXPRESSION */

	case DUK_TOK_EQUALSIGN: {
		/*
		 *  Assignments are right associative, allows e.g.
		 *    a = 5;
		 *    a += b = 9;   // same as a += (b = 9)
		 *  -> expression value 14, a = 14, b = 9
		 *
		 *  Right associativiness is reflected in the BP for recursion,
		 *  "-1" ensures assignment operations are allowed.
		 *
		 *  XXX: just use DUK__BP_COMMA (i.e. no need for 2-step bp levels)?
		 */
		args = (DUK_OP_NONE << 8) + DUK__BP_ASSIGNMENT - 1; /* DUK_OP_NONE marks a 'plain' assignment */
		goto assign;
	}
	case DUK_TOK_ADD_EQ: {
		/* right associative */
		args = (DUK_OP_ADD << 8) + DUK__BP_ASSIGNMENT - 1;
		goto assign;
	}
	case DUK_TOK_SUB_EQ: {
		/* right associative */
		args = (DUK_OP_SUB << 8) + DUK__BP_ASSIGNMENT - 1;
		goto assign;
	}
	case DUK_TOK_MUL_EQ: {
		/* right associative */
		args = (DUK_OP_MUL << 8) + DUK__BP_ASSIGNMENT - 1;
		goto assign;
	}
	case DUK_TOK_DIV_EQ: {
		/* right associative */
		args = (DUK_OP_DIV << 8) + DUK__BP_ASSIGNMENT - 1;
		goto assign;
	}
	case DUK_TOK_MOD_EQ: {
		/* right associative */
		args = (DUK_OP_MOD << 8) + DUK__BP_ASSIGNMENT - 1;
		goto assign;
	}
#if defined(DUK_USE_ES7_EXP_OPERATOR)
	case DUK_TOK_EXP_EQ: {
		/* right associative */
		args = (DUK_OP_EXP << 8) + DUK__BP_ASSIGNMENT - 1;
		goto assign;
	}
#endif
	case DUK_TOK_ALSHIFT_EQ: {
		/* right associative */
		args = (DUK_OP_BASL << 8) + DUK__BP_ASSIGNMENT - 1;
		goto assign;
	}
	case DUK_TOK_ARSHIFT_EQ: {
		/* right associative */
		args = (DUK_OP_BASR << 8) + DUK__BP_ASSIGNMENT - 1;
		goto assign;
	}
	case DUK_TOK_RSHIFT_EQ: {
		/* right associative */
		args = (DUK_OP_BLSR << 8) + DUK__BP_ASSIGNMENT - 1;
		goto assign;
	}
	case DUK_TOK_BAND_EQ: {
		/* right associative */
		args = (DUK_OP_BAND << 8) + DUK__BP_ASSIGNMENT - 1;
		goto assign;
	}
	case DUK_TOK_BOR_EQ: {
		/* right associative */
		args = (DUK_OP_BOR << 8) + DUK__BP_ASSIGNMENT - 1;
		goto assign;
	}
	case DUK_TOK_BXOR_EQ: {
		/* right associative */
		args = (DUK_OP_BXOR << 8) + DUK__BP_ASSIGNMENT - 1;
		goto assign;
	}

		/* COMMA */

	case DUK_TOK_COMMA: {
		/* right associative */

		duk__ivalue_toplain_ignore(comp_ctx, left); /* need side effects, not value */
		duk__expr_toplain(comp_ctx, res, DUK__BP_COMMA - 1 /*rbp_flags*/);

		/* return 'res' (of right part) as our result */
		return;
	}

	default: {
		break;
	}
	}

	DUK_D(DUK_DPRINT("parse error: unexpected token: %ld", (long) tok));
	DUK_ERROR_SYNTAX(thr, DUK_STR_PARSE_ERROR);
	DUK_WO_NORETURN(return;);

#if 0
	/* XXX: shared handling for 'duk__expr_lhs'? */
	if (comp_ctx->curr_func.paren_level == 0 && XXX) {
		comp_ctx->curr_func.duk__expr_lhs = 0;
	}
#endif

binary:
	/*
	 *  Shared handling of binary operations
	 *
	 *  args = (opcode << 8) + rbp
	 */
	{
		duk__ivalue_toplain(comp_ctx, left);
		duk__expr_toplain(comp_ctx, res, args & 0xff /*rbp_flags*/);

		/* combine left->x1 and res->x1 (right->x1, really) -> (left->x1 OP res->x1) */
		DUK_ASSERT(left->t == DUK_IVAL_PLAIN);
		DUK_ASSERT(res->t == DUK_IVAL_PLAIN);

		res->t = DUK_IVAL_ARITH;
		res->op = (args >> 8) & 0xff;

		res->x2.t = res->x1.t;
		res->x2.regconst = res->x1.regconst;
		duk_copy(thr, res->x1.valstack_idx, res->x2.valstack_idx);

		res->x1.t = left->x1.t;
		res->x1.regconst = left->x1.regconst;
		duk_copy(thr, left->x1.valstack_idx, res->x1.valstack_idx);

		DUK_DDD(DUK_DDDPRINT("binary op, res: t=%ld, x1.t=%ld, x1.regconst=0x%08lx, x2.t=%ld, x2.regconst=0x%08lx",
		                     (long) res->t,
		                     (long) res->x1.t,
		                     (unsigned long) res->x1.regconst,
		                     (long) res->x2.t,
		                     (unsigned long) res->x2.regconst));
		return;
	}

binary_logical:
	/*
	 *  Shared handling for logical AND and logical OR.
	 *
	 *  args = (truthval << 8) + rbp
	 *
	 *  Truthval determines when to skip right-hand-side.
	 *  For logical AND truthval=1, for logical OR truthval=0.
	 *
	 *  See doc/compiler.rst for discussion on compiling logical
	 *  AND and OR expressions.  The approach here is very simplistic,
	 *  generating extra jumps and multiple evaluations of truth values,
	 *  but generates code on-the-fly with only local back-patching.
	 *
	 *  Both logical AND and OR are syntactically left-associated.
	 *  However, logical ANDs are compiled as right associative
	 *  expressions, i.e. "A && B && C" as "A && (B && C)", to allow
	 *  skip jumps to skip over the entire tail.  Similarly for logical OR.
	 */

	{
		duk_regconst_t reg_temp;
		duk_int_t pc_jump;
		duk_small_uint_t args_truthval = args >> 8;
		duk_small_uint_t args_rbp = args & 0xff;

		/* XXX: unoptimal use of temps, resetting */

		reg_temp = DUK__ALLOCTEMP(comp_ctx);

		duk__ivalue_toforcedreg(comp_ctx, left, reg_temp);
		DUK_ASSERT(DUK__ISREG(reg_temp));
		duk__emit_bc(comp_ctx,
		             (args_truthval ? DUK_OP_IFTRUE_R : DUK_OP_IFFALSE_R),
		             reg_temp); /* skip jump conditionally */
		pc_jump = duk__emit_jump_empty(comp_ctx);
		duk__expr_toforcedreg(comp_ctx, res, args_rbp /*rbp_flags*/, reg_temp /*forced_reg*/);
		duk__patch_jump_here(comp_ctx, pc_jump);

		duk__ivalue_regconst(res, reg_temp);
		return;
	}

assign:
	/*
	 *  Shared assignment expression handling
	 *
	 *  args = (opcode << 8) + rbp
	 *
	 *  If 'opcode' is DUK_OP_NONE, plain assignment without arithmetic.
	 *  Syntactically valid left-hand-side forms which are not accepted as
	 *  left-hand-side values (e.g. as in "f() = 1") must NOT cause a
	 *  SyntaxError, but rather a run-time ReferenceError.
	 *
	 *  When evaluating X <op>= Y, the LHS (X) is conceptually evaluated
	 *  to a temporary first.  The RHS is then evaluated.  Finally, the
	 *  <op> is applied to the initial value of RHS (not the value after
	 *  RHS evaluation), and written to X.  Doing so concretely generates
	 *  inefficient code so we'd like to avoid the temporary when possible.
	 *  See: https://github.com/svaarala/duktape/pull/992.
	 *
	 *  The expression value (final LHS value, written to RHS) is
	 *  conceptually copied into a fresh temporary so that it won't
	 *  change even if the LHS/RHS values change in outer expressions.
	 *  For example, it'd be generally incorrect for the expression value
	 *  to be the RHS register binding, unless there's a guarantee that it
	 *  won't change during further expression evaluation.  Using the
	 *  temporary concretely produces inefficient bytecode, so we try to
	 *  avoid the extra temporary for some known-to-be-safe cases.
	 *  Currently the only safe case we detect is a "top level assignment",
	 *  for example "x = y + z;", where the assignment expression value is
	 *  ignored.
	 *  See: test-dev-assign-expr.js and test-bug-assign-mutate-gh381.js.
	 */

	{
		duk_small_uint_t args_op = args >> 8;
		duk_small_uint_t args_rbp = args & 0xff;
		duk_bool_t toplevel_assign;

		/* XXX: here we need to know if 'left' is left-hand-side compatible.
		 * That information is no longer available from current expr parsing
		 * state; it would need to be carried into the 'left' ivalue or by
		 * some other means.
		 */

		/* A top-level assignment is e.g. "x = y;".  For these it's safe
		 * to use the RHS as-is as the expression value, even if the RHS
		 * is a reg-bound identifier.  The RHS ('res') is right associative
		 * so it has consumed all other assignment level operations; the
		 * only relevant lower binding power construct is comma operator
		 * which will ignore the expression value provided here.  Usually
		 * the top level assignment expression value is ignored, but it
		 * is relevant for e.g. eval code.
		 */
		toplevel_assign = (comp_ctx->curr_func.nud_count == 1 && /* one token before */
		                   comp_ctx->curr_func.led_count == 1); /* one operator (= assign) */
		DUK_DDD(DUK_DDDPRINT("assignment: nud_count=%ld, led_count=%ld, toplevel_assign=%ld",
		                     (long) comp_ctx->curr_func.nud_count,
		                     (long) comp_ctx->curr_func.led_count,
		                     (long) toplevel_assign));

		if (left->t == DUK_IVAL_VAR) {
			duk_hstring *h_varname;
			duk_regconst_t reg_varbind;
			duk_regconst_t rc_varname;

			DUK_ASSERT(left->x1.t == DUK_ISPEC_VALUE); /* LHS is already side effect free */

			h_varname = duk_known_hstring(thr, left->x1.valstack_idx);
			if (duk__hstring_is_eval_or_arguments_in_strict_mode(comp_ctx, h_varname)) {
				/* E5 Section 11.13.1 (and others for other assignments), step 4. */
				goto syntax_error_lvalue;
			}
			duk_dup(thr, left->x1.valstack_idx);
			(void) duk__lookup_lhs(comp_ctx, &reg_varbind, &rc_varname);

			if (args_op == DUK_OP_NONE) {
				duk__expr(comp_ctx, res, args_rbp /*rbp_flags*/);
				if (toplevel_assign) {
					/* Any 'res' will do. */
					DUK_DDD(DUK_DDDPRINT("plain assignment, toplevel assign, use as is"));
				} else {
					/* 'res' must be a plain ivalue, and not register-bound variable. */
					DUK_DDD(DUK_DDDPRINT(
					    "plain assignment, not toplevel assign, ensure not a reg-bound identifier"));
					if (res->t != DUK_IVAL_PLAIN ||
					    (res->x1.t == DUK_ISPEC_REGCONST && DUK__ISREG_NOTTEMP(comp_ctx, res->x1.regconst))) {
						duk__ivalue_totempconst(comp_ctx, res);
					}
				}
			} else {
				/* For X <op>= Y we need to evaluate the pre-op
				 * value of X before evaluating the RHS: the RHS
				 * can change X, but when we do <op> we must use
				 * the pre-op value.
				 */
				duk_regconst_t reg_temp;

				reg_temp = DUK__ALLOCTEMP(comp_ctx);

				if (reg_varbind >= 0) {
					duk_regconst_t reg_res;
					duk_regconst_t reg_src;
					duk_int_t pc_temp_load;
					duk_int_t pc_before_rhs;
					duk_int_t pc_after_rhs;

					if (toplevel_assign) {
						/* 'reg_varbind' is the operation result and can also
						 * become the expression value for top level assignments
						 * such as: "var x; x += y;".
						 */
						DUK_DD(DUK_DDPRINT("<op>= expression is top level, write directly to reg_varbind"));
						reg_res = reg_varbind;
					} else {
						/* Not safe to use 'reg_varbind' as assignment expression
						 * value, so go through a temp.
						 */
						DUK_DD(DUK_DDPRINT("<op>= expression is not top level, write to reg_temp"));
						reg_res = reg_temp; /* reg_res should be smallest possible */
						reg_temp = DUK__ALLOCTEMP(comp_ctx);
					}

					/* Try to optimize X <op>= Y for reg-bound
					 * variables.  Detect side-effect free RHS
					 * narrowly by seeing whether it emits code.
					 * If not, rewind the code emitter and overwrite
					 * the unnecessary temp reg load.
					 */

					pc_temp_load = duk__get_current_pc(comp_ctx);
					duk__emit_a_bc(comp_ctx, DUK_OP_LDREG, reg_temp, reg_varbind);

					pc_before_rhs = duk__get_current_pc(comp_ctx);
					duk__expr_toregconst(comp_ctx, res, args_rbp /*rbp_flags*/);
					DUK_ASSERT(res->t == DUK_IVAL_PLAIN && res->x1.t == DUK_ISPEC_REGCONST);
					pc_after_rhs = duk__get_current_pc(comp_ctx);

					DUK_DD(DUK_DDPRINT("pc_temp_load=%ld, pc_before_rhs=%ld, pc_after_rhs=%ld",
					                   (long) pc_temp_load,
					                   (long) pc_before_rhs,
					                   (long) pc_after_rhs));

					if (pc_after_rhs == pc_before_rhs) {
						/* Note: if the reg_temp load generated shuffling
						 * instructions, we may need to rewind more than
						 * one instruction, so use explicit PC computation.
						 */
						DUK_DD(DUK_DDPRINT("rhs is side effect free, rewind and avoid unnecessary temp for "
						                   "reg-based <op>="));
						DUK_BW_ADD_PTR(comp_ctx->thr,
						               &comp_ctx->curr_func.bw_code,
						               (pc_temp_load - pc_before_rhs) *
						                   (duk_int_t) sizeof(duk_compiler_instr));
						reg_src = reg_varbind;
					} else {
						DUK_DD(DUK_DDPRINT("rhs evaluation emitted code, not sure if rhs is side effect "
						                   "free; use temp reg for LHS"));
						reg_src = reg_temp;
					}

					duk__emit_a_b_c(comp_ctx,
					                args_op | DUK__EMIT_FLAG_BC_REGCONST,
					                reg_res,
					                reg_src,
					                res->x1.regconst);

					res->x1.regconst = reg_res;

					/* Ensure compact use of temps. */
					if (DUK__ISREG_TEMP(comp_ctx, reg_res)) {
						DUK__SETTEMP(comp_ctx, reg_res + 1);
					}
				} else {
					/* When LHS is not register bound, always go through a
					 * temporary.  No optimization for top level assignment.
					 */

					duk__emit_a_bc(comp_ctx, DUK_OP_GETVAR, reg_temp, rc_varname);

					duk__expr_toregconst(comp_ctx, res, args_rbp /*rbp_flags*/);
					DUK_ASSERT(res->t == DUK_IVAL_PLAIN && res->x1.t == DUK_ISPEC_REGCONST);

					duk__emit_a_b_c(comp_ctx,
					                args_op | DUK__EMIT_FLAG_BC_REGCONST,
					                reg_temp,
					                reg_temp,
					                res->x1.regconst);
					res->x1.regconst = reg_temp;
				}

				DUK_ASSERT(res->t == DUK_IVAL_PLAIN && res->x1.t == DUK_ISPEC_REGCONST);
			}

			/* At this point 'res' holds the potential expression value.
			 * It can be basically any ivalue here, including a reg-bound
			 * identifier (if code above deems it safe) or a unary/binary
			 * operation.  Operations must be resolved to a side effect free
			 * plain value, and the side effects must happen exactly once.
			 */

			if (reg_varbind >= 0) {
				if (res->t != DUK_IVAL_PLAIN) {
					/* Resolve 'res' directly into the LHS binding, and use
					 * that as the expression value if safe.  If not safe,
					 * resolve to a temp/const and copy to LHS.
					 */
					if (toplevel_assign) {
						duk__ivalue_toforcedreg(comp_ctx, res, (duk_int_t) reg_varbind);
					} else {
						duk__ivalue_totempconst(comp_ctx, res);
						duk__copy_ivalue(comp_ctx, res, left); /* use 'left' as a temp */
						duk__ivalue_toforcedreg(comp_ctx, left, (duk_int_t) reg_varbind);
					}
				} else {
					/* Use 'res' as the expression value (it's side effect
					 * free and may be a plain value, a register, or a
					 * constant) and write it to the LHS binding too.
					 */
					duk__copy_ivalue(comp_ctx, res, left); /* use 'left' as a temp */
					duk__ivalue_toforcedreg(comp_ctx, left, (duk_int_t) reg_varbind);
				}
			} else {
				/* Only a reg fits into 'A' so coerce 'res' into a register
				 * for PUTVAR.
				 *
				 * XXX: here the current A/B/C split is suboptimal: we could
				 * just use 9 bits for reg_res (and support constants) and 17
				 * instead of 18 bits for the varname const index.
				 */

				duk__ivalue_toreg(comp_ctx, res);
				duk__emit_a_bc(comp_ctx, DUK_OP_PUTVAR | DUK__EMIT_FLAG_A_IS_SOURCE, res->x1.regconst, rc_varname);
			}

			/* 'res' contains expression value */
		} else if (left->t == DUK_IVAL_PROP) {
			/* E5 Section 11.13.1 (and others) step 4 never matches for prop writes -> no check */
			duk_regconst_t reg_obj;
			duk_regconst_t rc_key;
			duk_regconst_t rc_res;
			duk_regconst_t reg_temp;

			/* Property access expressions ('a[b]') are critical to correct
			 * LHS evaluation ordering, see test-dev-assign-eval-order*.js.
			 * We must make sure that the LHS target slot (base object and
			 * key) don't change during RHS evaluation.  The only concrete
			 * problem is a register reference to a variable-bound register
			 * (i.e., non-temp).  Require temp regs for both key and base.
			 *
			 * Don't allow a constant for the object (even for a number
			 * etc), as it goes into the 'A' field of the opcode.
			 */

			reg_obj = duk__ispec_toregconst_raw(comp_ctx,
			                                    &left->x1,
			                                    -1 /*forced_reg*/,
			                                    DUK__IVAL_FLAG_REQUIRE_TEMP /*flags*/);

			rc_key = duk__ispec_toregconst_raw(comp_ctx,
			                                   &left->x2,
			                                   -1 /*forced_reg*/,
			                                   DUK__IVAL_FLAG_REQUIRE_TEMP | DUK__IVAL_FLAG_ALLOW_CONST /*flags*/);

			/* Evaluate RHS only when LHS is safe. */

			if (args_op == DUK_OP_NONE) {
				duk__expr_toregconst(comp_ctx, res, args_rbp /*rbp_flags*/);
				DUK_ASSERT(res->t == DUK_IVAL_PLAIN && res->x1.t == DUK_ISPEC_REGCONST);
				rc_res = res->x1.regconst;
			} else {
				reg_temp = DUK__ALLOCTEMP(comp_ctx);
				duk__emit_a_b_c(comp_ctx, DUK_OP_GETPROP | DUK__EMIT_FLAG_BC_REGCONST, reg_temp, reg_obj, rc_key);

				duk__expr_toregconst(comp_ctx, res, args_rbp /*rbp_flags*/);
				DUK_ASSERT(res->t == DUK_IVAL_PLAIN && res->x1.t == DUK_ISPEC_REGCONST);

				duk__emit_a_b_c(comp_ctx,
				                args_op | DUK__EMIT_FLAG_BC_REGCONST,
				                reg_temp,
				                reg_temp,
				                res->x1.regconst);
				rc_res = reg_temp;
			}

			duk__emit_a_b_c(comp_ctx,
			                DUK_OP_PUTPROP | DUK__EMIT_FLAG_A_IS_SOURCE | DUK__EMIT_FLAG_BC_REGCONST,
			                reg_obj,
			                rc_key,
			                rc_res);

			duk__ivalue_regconst(res, rc_res);
		} else {
			/* No support for lvalues returned from new or function call expressions.
			 * However, these must NOT cause compile-time SyntaxErrors, but run-time
			 * ReferenceErrors.  Both left and right sides of the assignment must be
			 * evaluated before throwing a ReferenceError.  For instance:
			 *
			 *     f() = g();
			 *
			 * must result in f() being evaluated, then g() being evaluated, and
			 * finally, a ReferenceError being thrown.  See E5 Section 11.13.1.
			 */

			duk_regconst_t rc_res;

			/* First evaluate LHS fully to ensure all side effects are out. */
			duk__ivalue_toplain_ignore(comp_ctx, left);

			/* Then evaluate RHS fully (its value becomes the expression value too).
			 * Technically we'd need the side effect safety check here too, but because
			 * we always throw using INVLHS the result doesn't matter.
			 */
			rc_res = duk__expr_toregconst(comp_ctx, res, args_rbp /*rbp_flags*/);

			duk__emit_op_only(comp_ctx, DUK_OP_INVLHS);

			duk__ivalue_regconst(res, rc_res);
		}

		return;
	}

postincdec : {
	/*
	 *  Post-increment/decrement will return the original value as its
	 *  result value.  However, even that value will be coerced using
	 *  ToNumber() which is quite awkward.  Specific bytecode opcodes
	 *  are used to handle these semantics.
	 *
	 *  Note that post increment/decrement has a "no LineTerminator here"
	 *  restriction.  This is handled by duk__expr_lbp(), which forcibly terminates
	 *  the previous expression if a LineTerminator occurs before '++'/'--'.
	 */

	duk_regconst_t reg_res;
	duk_small_uint_t args_op1 = (args >> 8) & 0xff; /* DUK_OP_POSTINCR/DUK_OP_POSTDECR */
	duk_small_uint_t args_op2 = args >> 16; /* DUK_OP_POSTINCP_RR/DUK_OP_POSTDECP_RR */

	/* Specific assumptions for opcode numbering. */
	DUK_ASSERT(DUK_OP_POSTINCR + 4 == DUK_OP_POSTINCV);
	DUK_ASSERT(DUK_OP_POSTDECR + 4 == DUK_OP_POSTDECV);

	reg_res = DUK__ALLOCTEMP(comp_ctx);

	if (left->t == DUK_IVAL_VAR) {
		duk_hstring *h_varname;
		duk_regconst_t reg_varbind;
		duk_regconst_t rc_varname;

		h_varname = duk_known_hstring(thr, left->x1.valstack_idx);

		if (duk__hstring_is_eval_or_arguments_in_strict_mode(comp_ctx, h_varname)) {
			goto syntax_error;
		}

		duk_dup(thr, left->x1.valstack_idx);
		if (duk__lookup_lhs(comp_ctx, &reg_varbind, &rc_varname)) {
			duk__emit_a_bc(comp_ctx,
			               args_op1, /* e.g. DUK_OP_POSTINCR */
			               reg_res,
			               reg_varbind);
		} else {
			duk__emit_a_bc(comp_ctx,
			               args_op1 + 4, /* e.g. DUK_OP_POSTINCV */
			               reg_res,
			               rc_varname);
		}

		DUK_DDD(DUK_DDDPRINT("postincdec to '%!O' -> reg_varbind=%ld, rc_varname=%ld",
		                     (duk_heaphdr *) h_varname,
		                     (long) reg_varbind,
		                     (long) rc_varname));
	} else if (left->t == DUK_IVAL_PROP) {
		duk_regconst_t reg_obj; /* allocate to reg only (not const) */
		duk_regconst_t rc_key;

		reg_obj = duk__ispec_toregconst_raw(comp_ctx, &left->x1, -1 /*forced_reg*/, 0 /*flags*/); /* don't allow const */
		rc_key = duk__ispec_toregconst_raw(comp_ctx, &left->x2, -1 /*forced_reg*/, DUK__IVAL_FLAG_ALLOW_CONST /*flags*/);
		duk__emit_a_b_c(comp_ctx,
		                args_op2 | DUK__EMIT_FLAG_BC_REGCONST, /* e.g. DUK_OP_POSTINCP */
		                reg_res,
		                reg_obj,
		                rc_key);
	} else {
		/* Technically return value is not needed because INVLHS will
		 * unconditially throw a ReferenceError.  Coercion is necessary
		 * for proper semantics (consider ToNumber() called for an object).
		 * Use DUK_OP_UNP with a dummy register to get ToNumber().
		 */
		duk__ivalue_toforcedreg(comp_ctx, left, reg_res);
		duk__emit_bc(comp_ctx, DUK_OP_UNP, reg_res); /* for side effects, result ignored */
		duk__emit_op_only(comp_ctx, DUK_OP_INVLHS);
	}

	DUK__SETTEMP(comp_ctx, reg_res + 1);
	duk__ivalue_regconst(res, reg_res);
	return;
}

syntax_error:
	DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_EXPRESSION);
	DUK_WO_NORETURN(return;);

syntax_error_lvalue:
	DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_LVALUE);
	DUK_WO_NORETURN(return;);
}

DUK_LOCAL duk_small_uint_t duk__expr_lbp(duk_compiler_ctx *comp_ctx) {
	duk_small_uint_t tok = comp_ctx->curr_token.t;

	DUK_ASSERT_DISABLE(tok >= DUK_TOK_MINVAL); /* unsigned */
	DUK_ASSERT(tok <= DUK_TOK_MAXVAL);
	DUK_ASSERT(sizeof(duk__token_lbp) == DUK_TOK_MAXVAL + 1);

	/* XXX: integrate support for this into led() instead?
	 * Similar issue as post-increment/post-decrement.
	 */

	/* prevent duk__expr_led() by using a binding power less than anything valid */
	if (tok == DUK_TOK_IN && !comp_ctx->curr_func.allow_in) {
		return 0;
	}

	if ((tok == DUK_TOK_DECREMENT || tok == DUK_TOK_INCREMENT) && (comp_ctx->curr_token.lineterm)) {
		/* '++' or '--' in a post-increment/decrement position,
		 * and a LineTerminator occurs between the operator and
		 * the preceding expression.  Force the previous expr
		 * to terminate, in effect treating e.g. "a,b\n++" as
		 * "a,b;++" (= SyntaxError).
		 */
		return 0;
	}

	return DUK__TOKEN_LBP_GET_BP(duk__token_lbp[tok]); /* format is bit packed */
}

/*
 *  Expression parsing.
 *
 *  Upon entry to 'expr' and its variants, 'curr_tok' is assumed to be the
 *  first token of the expression.  Upon exit, 'curr_tok' will be the first
 *  token not part of the expression (e.g. semicolon terminating an expression
 *  statement).
 */

#define DUK__EXPR_RBP_MASK          0xff
#define DUK__EXPR_FLAG_REJECT_IN    (1 << 8) /* reject 'in' token (used for for-in) */
#define DUK__EXPR_FLAG_ALLOW_EMPTY  (1 << 9) /* allow empty expression */
#define DUK__EXPR_FLAG_REQUIRE_INIT (1 << 10) /* require initializer for var/const */

/* main expression parser function */
DUK_LOCAL void duk__expr(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags) {
	duk_hthread *thr = comp_ctx->thr;
	duk_ivalue tmp_alloc; /* 'res' is used for "left", and 'tmp' for "right" */
	duk_ivalue *tmp = &tmp_alloc;
	duk_small_uint_t rbp;

	DUK__RECURSION_INCREASE(comp_ctx, thr);

	duk_require_stack(thr, DUK__PARSE_EXPR_SLOTS);

	/* filter out flags from exprtop rbp_flags here to save space */
	rbp = rbp_flags & DUK__EXPR_RBP_MASK;

	DUK_DDD(DUK_DDDPRINT("duk__expr(), rbp_flags=%ld, rbp=%ld, allow_in=%ld, paren_level=%ld",
	                     (long) rbp_flags,
	                     (long) rbp,
	                     (long) comp_ctx->curr_func.allow_in,
	                     (long) comp_ctx->curr_func.paren_level));

	duk_memzero(&tmp_alloc, sizeof(tmp_alloc));
	tmp->x1.valstack_idx = duk_get_top(thr);
	tmp->x2.valstack_idx = tmp->x1.valstack_idx + 1;
	duk_push_undefined(thr);
	duk_push_undefined(thr);

	/* XXX: where to release temp regs in intermediate expressions?
	 * e.g. 1+2+3 -> don't inflate temp register count when parsing this.
	 * that particular expression temp regs can be forced here.
	 */

	/* XXX: increase ctx->expr_tokens here for every consumed token
	 * (this would be a nice statistic)?
	 */

	if (comp_ctx->curr_token.t == DUK_TOK_SEMICOLON || comp_ctx->curr_token.t == DUK_TOK_RPAREN) {
		/* XXX: possibly incorrect handling of empty expression */
		DUK_DDD(DUK_DDDPRINT("empty expression"));
		if (!(rbp_flags & DUK__EXPR_FLAG_ALLOW_EMPTY)) {
			DUK_ERROR_SYNTAX(thr, DUK_STR_EMPTY_EXPR_NOT_ALLOWED);
			DUK_WO_NORETURN(return;);
		}
		duk_push_undefined(thr);
		duk__ivalue_plain_fromstack(comp_ctx, res);
		goto cleanup;
	}

	duk__advance(comp_ctx);
	duk__expr_nud(comp_ctx, res); /* reuse 'res' as 'left' */
	while (rbp < duk__expr_lbp(comp_ctx)) {
		duk__advance(comp_ctx);
		duk__expr_led(comp_ctx, res, tmp);
		duk__copy_ivalue(comp_ctx, tmp, res); /* tmp -> res */
	}

cleanup:
	/* final result is already in 'res' */

	duk_pop_2(thr);

	DUK__RECURSION_DECREASE(comp_ctx, thr);
}

DUK_LOCAL void duk__exprtop(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags) {
	duk_hthread *thr = comp_ctx->thr;

	/* Note: these variables must reside in 'curr_func' instead of the global
	 * context: when parsing function expressions, expression parsing is nested.
	 */
	comp_ctx->curr_func.nud_count = 0;
	comp_ctx->curr_func.led_count = 0;
	comp_ctx->curr_func.paren_level = 0;
	comp_ctx->curr_func.expr_lhs = 1;
	comp_ctx->curr_func.allow_in = (rbp_flags & DUK__EXPR_FLAG_REJECT_IN ? 0 : 1);

	duk__expr(comp_ctx, res, rbp_flags);

	if (!(rbp_flags & DUK__EXPR_FLAG_ALLOW_EMPTY) && duk__expr_is_empty(comp_ctx)) {
		DUK_ERROR_SYNTAX(thr, DUK_STR_EMPTY_EXPR_NOT_ALLOWED);
		DUK_WO_NORETURN(return;);
	}
}

/* A bunch of helpers (for size optimization) that combine duk__expr()/duk__exprtop()
 * and result conversions.
 *
 * Each helper needs at least 2-3 calls to make it worth while to wrap.
 */

#if 0 /* unused */
DUK_LOCAL duk_regconst_t duk__expr_toreg(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags) {
	duk__expr(comp_ctx, res, rbp_flags);
	return duk__ivalue_toreg(comp_ctx, res);
}
#endif

#if 0 /* unused */
DUK_LOCAL duk_regconst_t duk__expr_totemp(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags) {
	duk__expr(comp_ctx, res, rbp_flags);
	return duk__ivalue_totemp(comp_ctx, res);
}
#endif

DUK_LOCAL void duk__expr_toforcedreg(duk_compiler_ctx *comp_ctx,
                                     duk_ivalue *res,
                                     duk_small_uint_t rbp_flags,
                                     duk_regconst_t forced_reg) {
	DUK_ASSERT(forced_reg >= 0);
	duk__expr(comp_ctx, res, rbp_flags);
	duk__ivalue_toforcedreg(comp_ctx, res, forced_reg);
}

DUK_LOCAL duk_regconst_t duk__expr_toregconst(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags) {
	duk__expr(comp_ctx, res, rbp_flags);
	return duk__ivalue_toregconst(comp_ctx, res);
}

#if 0 /* unused */
DUK_LOCAL duk_regconst_t duk__expr_totempconst(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags) {
	duk__expr(comp_ctx, res, rbp_flags);
	return duk__ivalue_totempconst(comp_ctx, res);
}
#endif

DUK_LOCAL void duk__expr_toplain(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags) {
	duk__expr(comp_ctx, res, rbp_flags);
	duk__ivalue_toplain(comp_ctx, res);
}

DUK_LOCAL void duk__expr_toplain_ignore(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags) {
	duk__expr(comp_ctx, res, rbp_flags);
	duk__ivalue_toplain_ignore(comp_ctx, res);
}

DUK_LOCAL duk_regconst_t duk__exprtop_toreg(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags) {
	duk__exprtop(comp_ctx, res, rbp_flags);
	return duk__ivalue_toreg(comp_ctx, res);
}

#if 0 /* unused */
DUK_LOCAL duk_regconst_t duk__exprtop_totemp(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags) {
	duk__exprtop(comp_ctx, res, rbp_flags);
	return duk__ivalue_totemp(comp_ctx, res);
}
#endif

DUK_LOCAL void duk__exprtop_toforcedreg(duk_compiler_ctx *comp_ctx,
                                        duk_ivalue *res,
                                        duk_small_uint_t rbp_flags,
                                        duk_regconst_t forced_reg) {
	DUK_ASSERT(forced_reg >= 0);
	duk__exprtop(comp_ctx, res, rbp_flags);
	duk__ivalue_toforcedreg(comp_ctx, res, forced_reg);
}

DUK_LOCAL duk_regconst_t duk__exprtop_toregconst(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t rbp_flags) {
	duk__exprtop(comp_ctx, res, rbp_flags);
	return duk__ivalue_toregconst(comp_ctx, res);
}

#if 0 /* unused */
DUK_LOCAL void duk__exprtop_toplain_ignore(duk_compiler_ctx *comp_ctx, duk_ivalue *res, int rbp_flags) {
	duk__exprtop(comp_ctx, res, rbp_flags);
	duk__ivalue_toplain_ignore(comp_ctx, res);
}
#endif

/*
 *  Parse an individual source element (top level statement) or a statement.
 *
 *  Handles labeled statements automatically (peeling away labels before
 *  parsing an expression that follows the label(s)).
 *
 *  Upon entry, 'curr_tok' contains the first token of the statement (parsed
 *  in "allow regexp literal" mode).  Upon exit, 'curr_tok' contains the first
 *  token following the statement (if the statement has a terminator, this is
 *  the token after the terminator).
 */

#define DUK__HAS_VAL                (1 << 0) /* stmt has non-empty value */
#define DUK__HAS_TERM               (1 << 1) /* stmt has explicit/implicit semicolon terminator */
#define DUK__ALLOW_AUTO_SEMI_ALWAYS (1 << 2) /* allow automatic semicolon even without lineterm (compatibility) */
#define DUK__STILL_PROLOGUE         (1 << 3) /* statement does not terminate directive prologue */
#define DUK__IS_TERMINAL            (1 << 4) /* statement is guaranteed to be terminal (control doesn't flow to next statement) */

/* Parse a single variable declaration (e.g. "i" or "i=10").  A leading 'var'
 * has already been eaten.  These is no return value in 'res', it is used only
 * as a temporary.
 *
 * When called from 'for-in' statement parser, the initializer expression must
 * not allow the 'in' token.  The caller supply additional expression parsing
 * flags (like DUK__EXPR_FLAG_REJECT_IN) in 'expr_flags'.
 *
 * Finally, out_rc_varname and out_reg_varbind are updated to reflect where
 * the identifier is bound:
 *
 *    If register bound:      out_reg_varbind >= 0, out_rc_varname == 0 (ignore)
 *    If not register bound:  out_reg_varbind < 0, out_rc_varname >= 0
 *
 * These allow the caller to use the variable for further assignment, e.g.
 * as is done in 'for-in' parsing.
 */

DUK_LOCAL void duk__parse_var_decl(duk_compiler_ctx *comp_ctx,
                                   duk_ivalue *res,
                                   duk_small_uint_t expr_flags,
                                   duk_regconst_t *out_reg_varbind,
                                   duk_regconst_t *out_rc_varname) {
	duk_hthread *thr = comp_ctx->thr;
	duk_hstring *h_varname;
	duk_regconst_t reg_varbind;
	duk_regconst_t rc_varname;

	/* assume 'var' has been eaten */

	/* Note: Identifier rejects reserved words */
	if (comp_ctx->curr_token.t != DUK_TOK_IDENTIFIER) {
		goto syntax_error;
	}
	h_varname = comp_ctx->curr_token.str1;

	DUK_ASSERT(h_varname != NULL);

	/* strict mode restrictions (E5 Section 12.2.1) */
	if (duk__hstring_is_eval_or_arguments_in_strict_mode(comp_ctx, h_varname)) {
		goto syntax_error;
	}

	/* register declarations in first pass */
	if (comp_ctx->curr_func.in_scanning) {
		duk_uarridx_t n;
		DUK_DDD(DUK_DDDPRINT("register variable declaration %!O in pass 1", (duk_heaphdr *) h_varname));
		n = (duk_uarridx_t) duk_get_length(thr, comp_ctx->curr_func.decls_idx);
		duk_push_hstring(thr, h_varname);
		duk_put_prop_index(thr, comp_ctx->curr_func.decls_idx, n);
		duk_push_int(thr, DUK_DECL_TYPE_VAR + (0 << 8));
		duk_put_prop_index(thr, comp_ctx->curr_func.decls_idx, n + 1);
	}

	duk_push_hstring(thr, h_varname); /* push before advancing to keep reachable */

	/* register binding lookup is based on varmap (even in first pass) */
	duk_dup_top(thr);
	(void) duk__lookup_lhs(comp_ctx, &reg_varbind, &rc_varname);

	duk__advance(comp_ctx); /* eat identifier */

	if (comp_ctx->curr_token.t == DUK_TOK_EQUALSIGN) {
		duk__advance(comp_ctx);

		DUK_DDD(DUK_DDDPRINT("vardecl, assign to '%!O' -> reg_varbind=%ld, rc_varname=%ld",
		                     (duk_heaphdr *) h_varname,
		                     (long) reg_varbind,
		                     (long) rc_varname));

		duk__exprtop(comp_ctx, res, DUK__BP_COMMA | expr_flags /*rbp_flags*/); /* AssignmentExpression */

		if (reg_varbind >= 0) {
			duk__ivalue_toforcedreg(comp_ctx, res, reg_varbind);
		} else {
			duk_regconst_t reg_val;
			reg_val = duk__ivalue_toreg(comp_ctx, res);
			duk__emit_a_bc(comp_ctx, DUK_OP_PUTVAR | DUK__EMIT_FLAG_A_IS_SOURCE, reg_val, rc_varname);
		}
	} else {
		if (expr_flags & DUK__EXPR_FLAG_REQUIRE_INIT) {
			/* Used for minimal 'const': initializer required. */
			goto syntax_error;
		}
	}

	duk_pop(thr); /* pop varname */

	*out_rc_varname = rc_varname;
	*out_reg_varbind = reg_varbind;

	return;

syntax_error:
	DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_VAR_DECLARATION);
	DUK_WO_NORETURN(return;);
}

DUK_LOCAL void duk__parse_var_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_small_uint_t expr_flags) {
	duk_regconst_t reg_varbind;
	duk_regconst_t rc_varname;

	duk__advance(comp_ctx); /* eat 'var' */

	for (;;) {
		/* rc_varname and reg_varbind are ignored here */
		duk__parse_var_decl(comp_ctx, res, 0 | expr_flags, &reg_varbind, &rc_varname);

		if (comp_ctx->curr_token.t != DUK_TOK_COMMA) {
			break;
		}
		duk__advance(comp_ctx);
	}
}

DUK_LOCAL void duk__parse_for_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_int_t pc_label_site) {
	duk_hthread *thr = comp_ctx->thr;
	duk_int_t pc_v34_lhs; /* start variant 3/4 left-hand-side code (L1 in doc/compiler.rst example) */
	duk_regconst_t temp_reset; /* knock back "next temp" to this whenever possible */
	duk_regconst_t reg_temps; /* preallocated temporaries (2) for variants 3 and 4 */

	DUK_DDD(DUK_DDDPRINT("start parsing a for/for-in statement"));

	/* Two temporaries are preallocated here for variants 3 and 4 which need
	 * registers which are never clobbered by expressions in the loop
	 * (concretely: for the enumerator object and the next enumerated value).
	 * Variants 1 and 2 "release" these temps.
	 */

	reg_temps = DUK__ALLOCTEMPS(comp_ctx, 2);

	temp_reset = DUK__GETTEMP(comp_ctx);

	/*
	 *  For/for-in main variants are:
	 *
	 *    1. for (ExpressionNoIn_opt; Expression_opt; Expression_opt) Statement
	 *    2. for (var VariableDeclarationNoIn; Expression_opt; Expression_opt) Statement
	 *    3. for (LeftHandSideExpression in Expression) Statement
	 *    4. for (var VariableDeclarationNoIn in Expression) Statement
	 *
	 *  Parsing these without arbitrary lookahead or backtracking is relatively
	 *  tricky but we manage to do so for now.
	 *
	 *  See doc/compiler.rst for a detailed discussion of control flow
	 *  issues, evaluation order issues, etc.
	 */

	duk__advance(comp_ctx); /* eat 'for' */
	duk__advance_expect(comp_ctx, DUK_TOK_LPAREN);

	DUK_DDD(DUK_DDDPRINT("detecting for/for-in loop variant, pc=%ld", (long) duk__get_current_pc(comp_ctx)));

	/* a label site has been emitted by duk__parse_stmt() automatically
	 * (it will also emit the ENDLABEL).
	 */

	if (comp_ctx->curr_token.t == DUK_TOK_VAR) {
		/*
		 *  Variant 2 or 4
		 */

		duk_regconst_t reg_varbind; /* variable binding register if register-bound (otherwise < 0) */
		duk_regconst_t rc_varname; /* variable name reg/const, if variable not register-bound */

		duk__advance(comp_ctx); /* eat 'var' */
		duk__parse_var_decl(comp_ctx, res, DUK__EXPR_FLAG_REJECT_IN, &reg_varbind, &rc_varname);
		DUK__SETTEMP(comp_ctx, temp_reset);

		if (comp_ctx->curr_token.t == DUK_TOK_IN) {
			/*
			 *  Variant 4
			 */

			DUK_DDD(DUK_DDDPRINT("detected for variant 4: for (var VariableDeclarationNoIn in Expression) Statement"));
			pc_v34_lhs = duk__get_current_pc(comp_ctx); /* jump is inserted here */
			if (reg_varbind >= 0) {
				duk__emit_a_bc(comp_ctx, DUK_OP_LDREG, reg_varbind, reg_temps + 0);
			} else {
				duk__emit_a_bc(comp_ctx, DUK_OP_PUTVAR | DUK__EMIT_FLAG_A_IS_SOURCE, reg_temps + 0, rc_varname);
			}
			goto parse_3_or_4;
		} else {
			/*
			 *  Variant 2
			 */

			DUK_DDD(DUK_DDDPRINT(
			    "detected for variant 2: for (var VariableDeclarationNoIn; Expression_opt; Expression_opt) Statement"));
			for (;;) {
				/* more initializers */
				if (comp_ctx->curr_token.t != DUK_TOK_COMMA) {
					break;
				}
				DUK_DDD(DUK_DDDPRINT("variant 2 has another variable initializer"));

				duk__advance(comp_ctx); /* eat comma */
				duk__parse_var_decl(comp_ctx, res, DUK__EXPR_FLAG_REJECT_IN, &reg_varbind, &rc_varname);
			}
			goto parse_1_or_2;
		}
	} else {
		/*
		 *  Variant 1 or 3
		 */

		pc_v34_lhs = duk__get_current_pc(comp_ctx); /* jump is inserted here (variant 3) */

		/* Note that duk__exprtop() here can clobber any reg above current temp_next,
		 * so any loop variables (e.g. enumerator) must be "preallocated".
		 */

		/* don't coerce yet to a plain value (variant 3 needs special handling) */
		duk__exprtop(comp_ctx,
		             res,
		             DUK__BP_FOR_EXPR | DUK__EXPR_FLAG_REJECT_IN |
		                 DUK__EXPR_FLAG_ALLOW_EMPTY /*rbp_flags*/); /* Expression */
		if (comp_ctx->curr_token.t == DUK_TOK_IN) {
			/*
			 *  Variant 3
			 */

			/* XXX: need to determine LHS type, and check that it is LHS compatible */
			DUK_DDD(DUK_DDDPRINT("detected for variant 3: for (LeftHandSideExpression in Expression) Statement"));
			if (duk__expr_is_empty(comp_ctx)) {
				goto syntax_error; /* LeftHandSideExpression does not allow empty expression */
			}

			if (res->t == DUK_IVAL_VAR) {
				duk_regconst_t reg_varbind;
				duk_regconst_t rc_varname;

				duk_dup(thr, res->x1.valstack_idx);
				if (duk__lookup_lhs(comp_ctx, &reg_varbind, &rc_varname)) {
					duk__emit_a_bc(comp_ctx, DUK_OP_LDREG, reg_varbind, reg_temps + 0);
				} else {
					duk__emit_a_bc(comp_ctx,
					               DUK_OP_PUTVAR | DUK__EMIT_FLAG_A_IS_SOURCE,
					               reg_temps + 0,
					               rc_varname);
				}
			} else if (res->t == DUK_IVAL_PROP) {
				/* Don't allow a constant for the object (even for a number etc), as
				 * it goes into the 'A' field of the opcode.
				 */
				duk_regconst_t reg_obj;
				duk_regconst_t rc_key;
				reg_obj = duk__ispec_toregconst_raw(comp_ctx,
				                                    &res->x1,
				                                    -1 /*forced_reg*/,
				                                    0 /*flags*/); /* don't allow const */
				rc_key = duk__ispec_toregconst_raw(comp_ctx,
				                                   &res->x2,
				                                   -1 /*forced_reg*/,
				                                   DUK__IVAL_FLAG_ALLOW_CONST /*flags*/);
				duk__emit_a_b_c(comp_ctx,
				                DUK_OP_PUTPROP | DUK__EMIT_FLAG_A_IS_SOURCE | DUK__EMIT_FLAG_BC_REGCONST,
				                reg_obj,
				                rc_key,
				                reg_temps + 0);
			} else {
				duk__ivalue_toplain_ignore(comp_ctx, res); /* just in case */
				duk__emit_op_only(comp_ctx, DUK_OP_INVLHS);
			}
			goto parse_3_or_4;
		} else {
			/*
			 *  Variant 1
			 */

			DUK_DDD(DUK_DDDPRINT(
			    "detected for variant 1: for (ExpressionNoIn_opt; Expression_opt; Expression_opt) Statement"));
			duk__ivalue_toplain_ignore(comp_ctx, res);
			goto parse_1_or_2;
		}
	}

parse_1_or_2:
	/*
	 *  Parse variant 1 or 2.  The first part expression (which differs
	 *  in the variants) has already been parsed and its code emitted.
	 *
	 *  reg_temps + 0: unused
	 *  reg_temps + 1: unused
	 */
	{
		duk_regconst_t rc_cond;
		duk_int_t pc_l1, pc_l2, pc_l3, pc_l4;
		duk_int_t pc_jumpto_l3, pc_jumpto_l4;
		duk_bool_t expr_c_empty;

		DUK_DDD(DUK_DDDPRINT("shared code for parsing variants 1 and 2"));

		/* "release" preallocated temps since we won't need them */
		temp_reset = reg_temps + 0;
		DUK__SETTEMP(comp_ctx, temp_reset);

		duk__advance_expect(comp_ctx, DUK_TOK_SEMICOLON);

		pc_l1 = duk__get_current_pc(comp_ctx);
		duk__exprtop(comp_ctx, res, DUK__BP_FOR_EXPR | DUK__EXPR_FLAG_ALLOW_EMPTY /*rbp_flags*/); /* Expression_opt */
		if (duk__expr_is_empty(comp_ctx)) {
			/* no need to coerce */
			pc_jumpto_l3 = duk__emit_jump_empty(comp_ctx); /* to body */
			pc_jumpto_l4 = -1; /* omitted */
		} else {
			rc_cond = duk__ivalue_toregconst(comp_ctx, res);
			duk__emit_if_false_skip(comp_ctx, rc_cond);
			pc_jumpto_l3 = duk__emit_jump_empty(comp_ctx); /* to body */
			pc_jumpto_l4 = duk__emit_jump_empty(comp_ctx); /* to exit */
		}
		DUK__SETTEMP(comp_ctx, temp_reset);

		duk__advance_expect(comp_ctx, DUK_TOK_SEMICOLON);

		pc_l2 = duk__get_current_pc(comp_ctx);
		duk__exprtop(comp_ctx, res, DUK__BP_FOR_EXPR | DUK__EXPR_FLAG_ALLOW_EMPTY /*rbp_flags*/); /* Expression_opt */
		if (duk__expr_is_empty(comp_ctx)) {
			/* no need to coerce */
			expr_c_empty = 1;
			/* JUMP L1 omitted */
		} else {
			duk__ivalue_toplain_ignore(comp_ctx, res);
			expr_c_empty = 0;
			duk__emit_jump(comp_ctx, pc_l1);
		}
		DUK__SETTEMP(comp_ctx, temp_reset);

		comp_ctx->curr_func.allow_regexp_in_adv = 1;
		duk__advance_expect(comp_ctx, DUK_TOK_RPAREN); /* Allow RegExp as part of next stmt. */

		pc_l3 = duk__get_current_pc(comp_ctx);
		duk__parse_stmt(comp_ctx, res, 0 /*allow_source_elem*/);
		if (expr_c_empty) {
			duk__emit_jump(comp_ctx, pc_l1);
		} else {
			duk__emit_jump(comp_ctx, pc_l2);
		}
		/* temp reset is not necessary after duk__parse_stmt(), which already does it */

		pc_l4 = duk__get_current_pc(comp_ctx);

		DUK_DDD(DUK_DDDPRINT("patching jumps: jumpto_l3: %ld->%ld, jumpto_l4: %ld->%ld, "
		                     "break: %ld->%ld, continue: %ld->%ld",
		                     (long) pc_jumpto_l3,
		                     (long) pc_l3,
		                     (long) pc_jumpto_l4,
		                     (long) pc_l4,
		                     (long) (pc_label_site + 1),
		                     (long) pc_l4,
		                     (long) (pc_label_site + 2),
		                     (long) pc_l2));

		duk__patch_jump(comp_ctx, pc_jumpto_l3, pc_l3);
		duk__patch_jump(comp_ctx, pc_jumpto_l4, pc_l4);
		duk__patch_jump(comp_ctx, pc_label_site + 1, pc_l4); /* break jump */
		duk__patch_jump(comp_ctx, pc_label_site + 2, expr_c_empty ? pc_l1 : pc_l2); /* continue jump */
	}
	goto finished;

parse_3_or_4:
	/*
	 *  Parse variant 3 or 4.
	 *
	 *  For variant 3 (e.g. "for (A in C) D;") the code for A (except the
	 *  final property/variable write) has already been emitted.  The first
	 *  instruction of that code is at pc_v34_lhs; a JUMP needs to be inserted
	 *  there to satisfy control flow needs.
	 *
	 *  For variant 4, if the variable declaration had an initializer
	 *  (e.g. "for (var A = B in C) D;") the code for the assignment
	 *  (B) has already been emitted.
	 *
	 *  Variables set before entering here:
	 *
	 *    pc_v34_lhs:    insert a "JUMP L2" here (see doc/compiler.rst example).
	 *    reg_temps + 0: iteration target value (written to LHS)
	 *    reg_temps + 1: enumerator object
	 */
	{
		duk_int_t pc_l1, pc_l2, pc_l3, pc_l4, pc_l5;
		duk_int_t pc_jumpto_l2, pc_jumpto_l3, pc_jumpto_l4, pc_jumpto_l5;
		duk_regconst_t reg_target;

		DUK_DDD(DUK_DDDPRINT("shared code for parsing variants 3 and 4, pc_v34_lhs=%ld", (long) pc_v34_lhs));

		DUK__SETTEMP(comp_ctx, temp_reset);

		/* First we need to insert a jump in the middle of previously
		 * emitted code to get the control flow right.  No jumps can
		 * cross the position where the jump is inserted.  See doc/compiler.rst
		 * for discussion on the intricacies of control flow and side effects
		 * for variants 3 and 4.
		 */

		duk__insert_jump_entry(comp_ctx, pc_v34_lhs);
		pc_jumpto_l2 = pc_v34_lhs; /* inserted jump */
		pc_l1 = pc_v34_lhs + 1; /* +1, right after inserted jump */

		/* The code for writing reg_temps + 0 to the left hand side has already
		 * been emitted.
		 */

		pc_jumpto_l3 = duk__emit_jump_empty(comp_ctx); /* -> loop body */

		duk__advance(comp_ctx); /* eat 'in' */

		/* Parse enumeration target and initialize enumerator.  For 'null' and 'undefined',
		 * INITENUM will creates a 'null' enumerator which works like an empty enumerator
		 * (E5 Section 12.6.4, step 3).  Note that INITENUM requires the value to be in a
		 * register (constant not allowed).
		 */

		pc_l2 = duk__get_current_pc(comp_ctx);
		reg_target = duk__exprtop_toreg(comp_ctx, res, DUK__BP_FOR_EXPR /*rbp_flags*/); /* Expression */
		duk__emit_b_c(comp_ctx, DUK_OP_INITENUM | DUK__EMIT_FLAG_B_IS_TARGET, reg_temps + 1, reg_target);
		pc_jumpto_l4 = duk__emit_jump_empty(comp_ctx);
		DUK__SETTEMP(comp_ctx, temp_reset);

		comp_ctx->curr_func.allow_regexp_in_adv = 1;
		duk__advance_expect(comp_ctx, DUK_TOK_RPAREN); /* Allow RegExp as part of next stmt. */

		pc_l3 = duk__get_current_pc(comp_ctx);
		duk__parse_stmt(comp_ctx, res, 0 /*allow_source_elem*/);
		/* temp reset is not necessary after duk__parse_stmt(), which already does it */

		/* NEXTENUM needs a jump slot right after the main opcode.
		 * We need the code emitter to reserve the slot: if there's
		 * target shuffling, the target shuffle opcodes must happen
		 * after the jump slot (for NEXTENUM the shuffle opcodes are
		 * not needed if the enum is finished).
		 */
		pc_l4 = duk__get_current_pc(comp_ctx);
		duk__emit_b_c(comp_ctx,
		              DUK_OP_NEXTENUM | DUK__EMIT_FLAG_B_IS_TARGET | DUK__EMIT_FLAG_RESERVE_JUMPSLOT,
		              reg_temps + 0,
		              reg_temps + 1);
		pc_jumpto_l5 = comp_ctx->emit_jumpslot_pc; /* NEXTENUM jump slot: executed when enum finished */
		duk__emit_jump(comp_ctx, pc_l1); /* jump to next loop, using reg_v34_iter as iterated value */

		pc_l5 = duk__get_current_pc(comp_ctx);

		/* XXX: since the enumerator may be a memory expensive object,
		 * perhaps clear it explicitly here?  If so, break jump must
		 * go through this clearing operation.
		 */

		DUK_DDD(DUK_DDDPRINT("patching jumps: jumpto_l2: %ld->%ld, jumpto_l3: %ld->%ld, "
		                     "jumpto_l4: %ld->%ld, jumpto_l5: %ld->%ld, "
		                     "break: %ld->%ld, continue: %ld->%ld",
		                     (long) pc_jumpto_l2,
		                     (long) pc_l2,
		                     (long) pc_jumpto_l3,
		                     (long) pc_l3,
		                     (long) pc_jumpto_l4,
		                     (long) pc_l4,
		                     (long) pc_jumpto_l5,
		                     (long) pc_l5,
		                     (long) (pc_label_site + 1),
		                     (long) pc_l5,
		                     (long) (pc_label_site + 2),
		                     (long) pc_l4));

		duk__patch_jump(comp_ctx, pc_jumpto_l2, pc_l2);
		duk__patch_jump(comp_ctx, pc_jumpto_l3, pc_l3);
		duk__patch_jump(comp_ctx, pc_jumpto_l4, pc_l4);
		duk__patch_jump(comp_ctx, pc_jumpto_l5, pc_l5);
		duk__patch_jump(comp_ctx, pc_label_site + 1, pc_l5); /* break jump */
		duk__patch_jump(comp_ctx, pc_label_site + 2, pc_l4); /* continue jump */
	}
	goto finished;

finished:
	DUK_DDD(DUK_DDDPRINT("end parsing a for/for-in statement"));
	return;

syntax_error:
	DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_FOR);
	DUK_WO_NORETURN(return;);
}

DUK_LOCAL void duk__parse_switch_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_int_t pc_label_site) {
	duk_hthread *thr = comp_ctx->thr;
	duk_regconst_t temp_at_loop;
	duk_regconst_t rc_switch; /* reg/const for switch value */
	duk_regconst_t rc_case; /* reg/const for case value */
	duk_regconst_t reg_temp; /* general temp register */
	duk_int_t pc_prevcase = -1;
	duk_int_t pc_prevstmt = -1;
	duk_int_t pc_default = -1; /* -1 == not set, -2 == pending (next statement list) */

	/* Note: negative pc values are ignored when patching jumps, so no explicit checks needed */

	/*
	 *  Switch is pretty complicated because of several conflicting concerns:
	 *
	 *    - Want to generate code without an intermediate representation,
	 *      i.e., in one go
	 *
	 *    - Case selectors are expressions, not values, and may thus e.g. throw
	 *      exceptions (which causes evaluation order concerns)
	 *
	 *    - Evaluation semantics of case selectors and default clause need to be
	 *      carefully implemented to provide correct behavior even with case value
	 *      side effects
	 *
	 *    - Fall through case and default clauses; avoiding dead JUMPs if case
	 *      ends with an unconditional jump (a break or a continue)
	 *
	 *    - The same case value may occur multiple times, but evaluation rules
	 *      only process the first match before switching to a "propagation" mode
	 *      where case values are no longer evaluated
	 *
	 *  See E5 Section 12.11.  Also see doc/compiler.rst for compilation
	 *  discussion.
	 */

	duk__advance(comp_ctx);
	duk__advance_expect(comp_ctx, DUK_TOK_LPAREN);
	rc_switch = duk__exprtop_toregconst(comp_ctx, res, DUK__BP_FOR_EXPR /*rbp_flags*/);
	duk__advance_expect(comp_ctx, DUK_TOK_RPAREN); /* RegExp mode does not matter. */
	duk__advance_expect(comp_ctx, DUK_TOK_LCURLY);

	DUK_DDD(DUK_DDDPRINT("switch value in register %ld", (long) rc_switch));

	temp_at_loop = DUK__GETTEMP(comp_ctx);

	for (;;) {
		duk_int_t num_stmts;
		duk_small_uint_t tok;

		/* sufficient for keeping temp reg numbers in check */
		DUK__SETTEMP(comp_ctx, temp_at_loop);

		if (comp_ctx->curr_token.t == DUK_TOK_RCURLY) {
			break;
		}

		/*
		 *  Parse a case or default clause.
		 */

		if (comp_ctx->curr_token.t == DUK_TOK_CASE) {
			/*
			 *  Case clause.
			 *
			 *  Note: cannot use reg_case as a temp register (for SEQ target)
			 *  because it may be a constant.
			 */

			duk__patch_jump_here(comp_ctx, pc_prevcase); /* chain jumps for case
			                                              * evaluation and checking
			                                              */

			duk__advance(comp_ctx);
			rc_case = duk__exprtop_toregconst(comp_ctx, res, DUK__BP_FOR_EXPR /*rbp_flags*/);
			duk__advance_expect(comp_ctx, DUK_TOK_COLON);

			reg_temp = DUK__ALLOCTEMP(comp_ctx);
			duk__emit_a_b_c(comp_ctx, DUK_OP_SEQ | DUK__EMIT_FLAG_BC_REGCONST, reg_temp, rc_switch, rc_case);
			duk__emit_if_true_skip(comp_ctx, reg_temp);

			/* jump to next case clause */
			pc_prevcase = duk__emit_jump_empty(comp_ctx); /* no match, next case */

			/* statements go here (if any) on next loop */
		} else if (comp_ctx->curr_token.t == DUK_TOK_DEFAULT) {
			/*
			 *  Default clause.
			 */

			if (pc_default >= 0) {
				goto syntax_error;
			}
			duk__advance(comp_ctx);
			duk__advance_expect(comp_ctx, DUK_TOK_COLON);

			/* Fix for https://github.com/svaarala/duktape/issues/155:
			 * If 'default' is first clause (detected by pc_prevcase < 0)
			 * we need to ensure we stay in the matching chain.
			 */
			if (pc_prevcase < 0) {
				DUK_DD(DUK_DDPRINT("default clause is first, emit prevcase jump"));
				pc_prevcase = duk__emit_jump_empty(comp_ctx);
			}

			/* default clause matches next statement list (if any) */
			pc_default = -2;
		} else {
			/* Code is not accepted before the first case/default clause */
			goto syntax_error;
		}

		/*
		 *  Parse code after the clause.  Possible terminators are
		 *  'case', 'default', and '}'.
		 *
		 *  Note that there may be no code at all, not even an empty statement,
		 *  between case clauses.  This must be handled just like an empty statement
		 *  (omitting seemingly pointless JUMPs), to avoid situations like
		 *  test-bug-case-fallthrough.js.
		 */

		num_stmts = 0;
		if (pc_default == -2) {
			pc_default = duk__get_current_pc(comp_ctx);
		}

		/* Note: this is correct even for default clause statements:
		 * they participate in 'fall-through' behavior even if the
		 * default clause is in the middle.
		 */
		duk__patch_jump_here(comp_ctx, pc_prevstmt); /* chain jumps for 'fall-through'
		                                              * after a case matches.
		                                              */

		for (;;) {
			tok = comp_ctx->curr_token.t;
			if (tok == DUK_TOK_CASE || tok == DUK_TOK_DEFAULT || tok == DUK_TOK_RCURLY) {
				break;
			}
			num_stmts++;
			duk__parse_stmt(comp_ctx, res, 0 /*allow_source_elem*/);
		}

		/* fall-through jump to next code of next case (backpatched) */
		pc_prevstmt = duk__emit_jump_empty(comp_ctx);

		/* XXX: would be nice to omit this jump when the jump is not
		 * reachable, at least in the obvious cases (such as the case
		 * ending with a 'break'.
		 *
		 * Perhaps duk__parse_stmt() could provide some info on whether
		 * the statement is a "dead end"?
		 *
		 * If implemented, just set pc_prevstmt to -1 when not needed.
		 */
	}

	DUK_ASSERT(comp_ctx->curr_token.t == DUK_TOK_RCURLY);
	comp_ctx->curr_func.allow_regexp_in_adv = 1;
	duk__advance(comp_ctx); /* Allow RegExp as part of next stmt. */

	/* default case control flow patchup; note that if pc_prevcase < 0
	 * (i.e. no case clauses), control enters default case automatically.
	 */
	if (pc_default >= 0) {
		/* default case exists: go there if no case matches */
		duk__patch_jump(comp_ctx, pc_prevcase, pc_default);
	} else {
		/* default case does not exist, or no statements present
		 * after default case: finish case evaluation
		 */
		duk__patch_jump_here(comp_ctx, pc_prevcase);
	}

	/* fall-through control flow patchup; note that pc_prevstmt may be
	 * < 0 (i.e. no case clauses), in which case this is a no-op.
	 */
	duk__patch_jump_here(comp_ctx, pc_prevstmt);

	/* continue jump not patched, an INVALID opcode remains there */
	duk__patch_jump_here(comp_ctx, pc_label_site + 1); /* break jump */

	/* Note: 'fast' breaks will jump to pc_label_site + 1, which will
	 * then jump here.  The double jump will be eliminated by a
	 * peephole pass, resulting in an optimal jump here.  The label
	 * site jumps will remain in bytecode and will waste code size.
	 */

	return;

syntax_error:
	DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_SWITCH);
	DUK_WO_NORETURN(return;);
}

DUK_LOCAL void duk__parse_if_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res) {
	duk_regconst_t temp_reset;
	duk_regconst_t rc_cond;
	duk_int_t pc_jump_false;

	DUK_DDD(DUK_DDDPRINT("begin parsing if statement"));

	temp_reset = DUK__GETTEMP(comp_ctx);

	duk__advance(comp_ctx); /* eat 'if' */
	duk__advance_expect(comp_ctx, DUK_TOK_LPAREN);

	rc_cond = duk__exprtop_toregconst(comp_ctx, res, DUK__BP_FOR_EXPR /*rbp_flags*/);
	duk__emit_if_true_skip(comp_ctx, rc_cond);
	pc_jump_false = duk__emit_jump_empty(comp_ctx); /* jump to end or else part */
	DUK__SETTEMP(comp_ctx, temp_reset);

	comp_ctx->curr_func.allow_regexp_in_adv = 1;
	duk__advance_expect(comp_ctx, DUK_TOK_RPAREN); /* Allow RegExp as part of next stmt. */

	duk__parse_stmt(comp_ctx, res, 0 /*allow_source_elem*/);

	/* The 'else' ambiguity is resolved by 'else' binding to the innermost
	 * construct, so greedy matching is correct here.
	 */

	if (comp_ctx->curr_token.t == DUK_TOK_ELSE) {
		duk_int_t pc_jump_end;

		DUK_DDD(DUK_DDDPRINT("if has else part"));

		duk__advance(comp_ctx);

		pc_jump_end = duk__emit_jump_empty(comp_ctx); /* jump from true part to end */
		duk__patch_jump_here(comp_ctx, pc_jump_false);

		duk__parse_stmt(comp_ctx, res, 0 /*allow_source_elem*/);

		duk__patch_jump_here(comp_ctx, pc_jump_end);
	} else {
		DUK_DDD(DUK_DDDPRINT("if does not have else part"));

		duk__patch_jump_here(comp_ctx, pc_jump_false);
	}

	DUK_DDD(DUK_DDDPRINT("end parsing if statement"));
}

DUK_LOCAL void duk__parse_do_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_int_t pc_label_site) {
	duk_regconst_t rc_cond;
	duk_int_t pc_start;

	DUK_DDD(DUK_DDDPRINT("begin parsing do statement"));

	duk__advance(comp_ctx); /* Eat 'do'; allow RegExp as part of next stmt. */

	pc_start = duk__get_current_pc(comp_ctx);
	duk__parse_stmt(comp_ctx, res, 0 /*allow_source_elem*/);
	duk__patch_jump_here(comp_ctx, pc_label_site + 2); /* continue jump */

	duk__advance_expect(comp_ctx, DUK_TOK_WHILE);
	duk__advance_expect(comp_ctx, DUK_TOK_LPAREN);

	rc_cond = duk__exprtop_toregconst(comp_ctx, res, DUK__BP_FOR_EXPR /*rbp_flags*/);
	duk__emit_if_false_skip(comp_ctx, rc_cond);
	duk__emit_jump(comp_ctx, pc_start);
	/* no need to reset temps, as we're finished emitting code */

	comp_ctx->curr_func.allow_regexp_in_adv = 1; /* Allow RegExp as part of next stmt. */
	duk__advance_expect(comp_ctx, DUK_TOK_RPAREN);

	duk__patch_jump_here(comp_ctx, pc_label_site + 1); /* break jump */

	DUK_DDD(DUK_DDDPRINT("end parsing do statement"));
}

DUK_LOCAL void duk__parse_while_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_int_t pc_label_site) {
	duk_regconst_t temp_reset;
	duk_regconst_t rc_cond;
	duk_int_t pc_start;
	duk_int_t pc_jump_false;

	DUK_DDD(DUK_DDDPRINT("begin parsing while statement"));

	temp_reset = DUK__GETTEMP(comp_ctx);

	duk__advance(comp_ctx); /* eat 'while' */

	duk__advance_expect(comp_ctx, DUK_TOK_LPAREN);

	pc_start = duk__get_current_pc(comp_ctx);
	duk__patch_jump_here(comp_ctx, pc_label_site + 2); /* continue jump */

	rc_cond = duk__exprtop_toregconst(comp_ctx, res, DUK__BP_FOR_EXPR /*rbp_flags*/);
	duk__emit_if_true_skip(comp_ctx, rc_cond);
	pc_jump_false = duk__emit_jump_empty(comp_ctx);
	DUK__SETTEMP(comp_ctx, temp_reset);

	comp_ctx->curr_func.allow_regexp_in_adv = 1;
	duk__advance_expect(comp_ctx, DUK_TOK_RPAREN); /* Allow RegExp as part of next stmt. */

	duk__parse_stmt(comp_ctx, res, 0 /*allow_source_elem*/);
	duk__emit_jump(comp_ctx, pc_start);

	duk__patch_jump_here(comp_ctx, pc_jump_false);
	duk__patch_jump_here(comp_ctx, pc_label_site + 1); /* break jump */

	DUK_DDD(DUK_DDDPRINT("end parsing while statement"));
}

DUK_LOCAL void duk__parse_break_or_continue_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res) {
	duk_hthread *thr = comp_ctx->thr;
	duk_bool_t is_break = (comp_ctx->curr_token.t == DUK_TOK_BREAK);
	duk_int_t label_id;
	duk_int_t label_catch_depth;
	duk_int_t label_pc; /* points to LABEL; pc+1 = jump site for break; pc+2 = jump site for continue */
	duk_bool_t label_is_closest;

	DUK_UNREF(res);

	duk__advance(comp_ctx); /* eat 'break' or 'continue' */

	if (comp_ctx->curr_token.t == DUK_TOK_SEMICOLON || /* explicit semi follows */
	    comp_ctx->curr_token.lineterm || /* automatic semi will be inserted */
	    comp_ctx->curr_token.allow_auto_semi) { /* automatic semi will be inserted */
		/* break/continue without label */

		duk__lookup_active_label(comp_ctx,
		                         DUK_HTHREAD_STRING_EMPTY_STRING(thr),
		                         is_break,
		                         &label_id,
		                         &label_catch_depth,
		                         &label_pc,
		                         &label_is_closest);
	} else if (comp_ctx->curr_token.t == DUK_TOK_IDENTIFIER) {
		/* break/continue with label (label cannot be a reserved word, production is 'Identifier' */
		DUK_ASSERT(comp_ctx->curr_token.str1 != NULL);
		duk__lookup_active_label(comp_ctx,
		                         comp_ctx->curr_token.str1,
		                         is_break,
		                         &label_id,
		                         &label_catch_depth,
		                         &label_pc,
		                         &label_is_closest);
		duk__advance(comp_ctx);
	} else {
		DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_BREAK_CONT_LABEL);
		DUK_WO_NORETURN(return;);
	}

	/* Use a fast break/continue when possible.  A fast break/continue is
	 * just a jump to the LABEL break/continue jump slot, which then jumps
	 * to an appropriate place (for break, going through ENDLABEL correctly).
	 * The peephole optimizer will optimize the jump to a direct one.
	 */

	if (label_catch_depth == comp_ctx->curr_func.catch_depth && label_is_closest) {
		DUK_DDD(DUK_DDDPRINT("break/continue: is_break=%ld, label_id=%ld, label_is_closest=%ld, "
		                     "label_catch_depth=%ld, catch_depth=%ld "
		                     "-> use fast variant (direct jump)",
		                     (long) is_break,
		                     (long) label_id,
		                     (long) label_is_closest,
		                     (long) label_catch_depth,
		                     (long) comp_ctx->curr_func.catch_depth));

		duk__emit_jump(comp_ctx, label_pc + (is_break ? 1 : 2));
	} else {
		DUK_DDD(DUK_DDDPRINT("break/continue: is_break=%ld, label_id=%ld, label_is_closest=%ld, "
		                     "label_catch_depth=%ld, catch_depth=%ld "
		                     "-> use slow variant (longjmp)",
		                     (long) is_break,
		                     (long) label_id,
		                     (long) label_is_closest,
		                     (long) label_catch_depth,
		                     (long) comp_ctx->curr_func.catch_depth));

		duk__emit_bc(comp_ctx, is_break ? DUK_OP_BREAK : DUK_OP_CONTINUE, (duk_regconst_t) label_id);
	}
}

DUK_LOCAL void duk__parse_return_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res) {
	duk_hthread *thr = comp_ctx->thr;
	duk_regconst_t rc_val;

	duk__advance(comp_ctx); /* eat 'return' */

	/* A 'return' statement is only allowed inside an actual function body,
	 * not as part of eval or global code.
	 */
	if (!comp_ctx->curr_func.is_function) {
		DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_RETURN);
		DUK_WO_NORETURN(return;);
	}

	if (comp_ctx->curr_token.t == DUK_TOK_SEMICOLON || /* explicit semi follows */
	    comp_ctx->curr_token.lineterm || /* automatic semi will be inserted */
	    comp_ctx->curr_token.allow_auto_semi) { /* automatic semi will be inserted */
		DUK_DDD(DUK_DDDPRINT("empty return value -> undefined"));
		duk__emit_op_only(comp_ctx, DUK_OP_RETUNDEF);
	} else {
		duk_int_t pc_before_expr;
		duk_int_t pc_after_expr;

		DUK_DDD(DUK_DDDPRINT("return with a value"));

		DUK_UNREF(pc_before_expr);
		DUK_UNREF(pc_after_expr);

		pc_before_expr = duk__get_current_pc(comp_ctx);
		rc_val = duk__exprtop_toregconst(comp_ctx, res, DUK__BP_FOR_EXPR /*rbp_flags*/);
		pc_after_expr = duk__get_current_pc(comp_ctx);

		/* Tail call check: if last opcode emitted was CALL, and
		 * the context allows it, add a tailcall flag to the CALL.
		 * This doesn't guarantee that a tail call will be allowed at
		 * runtime, so the RETURN must still be emitted.  (Duktape
		 * 0.10.0 avoided this and simulated a RETURN if a tail call
		 * couldn't be used at runtime; but this didn't work
		 * correctly with a thread yield/resume, see
		 * test-bug-tailcall-thread-yield-resume.js for discussion.)
		 *
		 * In addition to the last opcode being CALL, we also need to
		 * be sure that 'rc_val' is the result register of the CALL.
		 * For instance, for the expression 'return 0, (function ()
		 * { return 1; }), 2' the last opcode emitted is CALL (no
		 * bytecode is emitted for '2') but 'rc_val' indicates
		 * constant '2'.  Similarly if '2' is replaced by a register
		 * bound variable, no opcodes are emitted but tail call would
		 * be incorrect.
		 *
		 * This is tricky and easy to get wrong.  It would be best to
		 * track enough expression metadata to check that 'rc_val' came
		 * from that last CALL instruction.  We don't have that metadata
		 * now, so we check that 'rc_val' is a temporary register result
		 * (not a constant or a register bound variable).  There should
		 * be no way currently for 'rc_val' to be a temporary for an
		 * expression following the CALL instruction without emitting
		 * some opcodes following the CALL.  This proxy check is used
		 * below.
		 *
		 * See: test-bug-comma-expr-gh131.js.
		 *
		 * The non-standard 'caller' property disables tail calls
		 * because they pose some special cases which haven't been
		 * fixed yet.
		 */

#if defined(DUK_USE_TAILCALL)
		if (comp_ctx->curr_func.catch_depth == 0 && /* no catchers */
		    pc_after_expr > pc_before_expr) { /* at least one opcode emitted */
			duk_compiler_instr *instr;
			duk_instr_t ins;
			duk_small_uint_t op;

			instr = duk__get_instr_ptr(comp_ctx, pc_after_expr - 1);
			DUK_ASSERT(instr != NULL);

			ins = instr->ins;
			op = (duk_small_uint_t) DUK_DEC_OP(ins);
			if ((op & ~0x0fU) == DUK_OP_CALL0 && DUK__ISREG_TEMP(comp_ctx, rc_val) /* see above */) {
				DUK_DDD(DUK_DDDPRINT("return statement detected a tail call opportunity: "
				                     "catch depth is 0, duk__exprtop() emitted >= 1 instructions, "
				                     "and last instruction is a CALL "
				                     "-> change to TAILCALL"));
				ins |= DUK_ENC_OP(DUK_BC_CALL_FLAG_TAILCALL);
				instr->ins = ins;
			}
		}
#endif /* DUK_USE_TAILCALL */

		if (DUK__ISREG(rc_val)) {
			duk__emit_bc(comp_ctx, DUK_OP_RETREG, rc_val);
		} else {
			rc_val = DUK__REMOVECONST(rc_val);
			if (duk__const_needs_refcount(comp_ctx, rc_val)) {
				duk__emit_bc(comp_ctx, DUK_OP_RETCONST, rc_val);
			} else {
				duk__emit_bc(comp_ctx, DUK_OP_RETCONSTN, rc_val);
			}
		}
	}
}

DUK_LOCAL void duk__parse_throw_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res) {
	duk_regconst_t reg_val;

	duk__advance(comp_ctx); /* eat 'throw' */

	/* Unlike break/continue, throw statement does not allow an empty value. */

	if (comp_ctx->curr_token.lineterm) {
		DUK_ERROR_SYNTAX(comp_ctx->thr, DUK_STR_INVALID_THROW);
		DUK_WO_NORETURN(return;);
	}

	reg_val = duk__exprtop_toreg(comp_ctx, res, DUK__BP_FOR_EXPR /*rbp_flags*/);
	duk__emit_bc(comp_ctx, DUK_OP_THROW, reg_val);
}

DUK_LOCAL void duk__parse_try_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res) {
	duk_hthread *thr = comp_ctx->thr;
	duk_regconst_t reg_catch; /* reg_catch+0 and reg_catch+1 are reserved for TRYCATCH */
	duk_regconst_t rc_varname = 0;
	duk_small_uint_t trycatch_flags = 0;
	duk_int_t pc_ldconst = -1;
	duk_int_t pc_trycatch = -1;
	duk_int_t pc_catch = -1;
	duk_int_t pc_finally = -1;

	DUK_UNREF(res);

	/*
	 *  See the following documentation for discussion:
	 *
	 *    doc/execution.rst: control flow details
	 *
	 *  Try, catch, and finally "parts" are Blocks, not Statements, so
	 *  they must always be delimited by curly braces.  This is unlike e.g.
	 *  the if statement, which accepts any Statement.  This eliminates any
	 *  questions of matching parts of nested try statements.  The Block
	 *  parsing is implemented inline here (instead of calling out).
	 *
	 *  Finally part has a 'let scoped' variable, which requires a few kinks
	 *  here.
	 */

	comp_ctx->curr_func.catch_depth++;

	duk__advance(comp_ctx); /* eat 'try' */

	reg_catch = DUK__ALLOCTEMPS(comp_ctx, 2);

	/* The target for this LDCONST may need output shuffling, but we assume
	 * that 'pc_ldconst' will be the LDCONST that we can patch later.  This
	 * should be the case because there's no input shuffling.  (If there's
	 * no catch clause, this LDCONST will be replaced with a NOP.)
	 */
	pc_ldconst = duk__get_current_pc(comp_ctx);
	duk__emit_a_bc(comp_ctx, DUK_OP_LDCONST, reg_catch, 0 /*patched later*/);

	pc_trycatch = duk__get_current_pc(comp_ctx);
	duk__emit_invalid(comp_ctx); /* TRYCATCH, cannot emit now (not enough info) */
	duk__emit_invalid(comp_ctx); /* jump for 'catch' case */
	duk__emit_invalid(comp_ctx); /* jump for 'finally' case or end (if no finally) */

	/* try part */
	duk__advance_expect(comp_ctx, DUK_TOK_LCURLY);
	duk__parse_stmts(comp_ctx, 0 /*allow_source_elem*/, 0 /*expect_eof*/, 1 /*regexp_after*/);
	/* the DUK_TOK_RCURLY is eaten by duk__parse_stmts() */
	duk__emit_op_only(comp_ctx, DUK_OP_ENDTRY);

	if (comp_ctx->curr_token.t == DUK_TOK_CATCH) {
		/*
		 *  The catch variable must be updated to reflect the new allocated
		 *  register for the duration of the catch clause.  We need to store
		 *  and restore the original value for the varmap entry (if any).
		 */

		/*
		 *  Note: currently register bindings must be fixed for the entire
		 *  function.  So, even though the catch variable is in a register
		 *  we know, we must use an explicit environment record and slow path
		 *  accesses to read/write the catch binding to make closures created
		 *  within the catch clause work correctly.  This restriction should
		 *  be fixable (at least in common cases) later.
		 *
		 *  See: test-bug-catch-binding-2.js.
		 *
		 *  XXX: improve to get fast path access to most catch clauses.
		 */

		duk_hstring *h_var;
		duk_int_t varmap_value; /* for storing/restoring the varmap binding for catch variable */

		DUK_DDD(DUK_DDDPRINT("stack top at start of catch clause: %ld", (long) duk_get_top(thr)));

		trycatch_flags |= DUK_BC_TRYCATCH_FLAG_HAVE_CATCH;

		pc_catch = duk__get_current_pc(comp_ctx);

		duk__advance(comp_ctx);
		duk__advance_expect(comp_ctx, DUK_TOK_LPAREN);

		if (comp_ctx->curr_token.t != DUK_TOK_IDENTIFIER) {
			/* Identifier, i.e. don't allow reserved words */
			goto syntax_error;
		}
		h_var = comp_ctx->curr_token.str1;
		DUK_ASSERT(h_var != NULL);

		duk_push_hstring(thr, h_var); /* keep in on valstack, use borrowed ref below */

		if (comp_ctx->curr_func.is_strict &&
		    ((h_var == DUK_HTHREAD_STRING_EVAL(thr)) || (h_var == DUK_HTHREAD_STRING_LC_ARGUMENTS(thr)))) {
			DUK_DDD(DUK_DDDPRINT("catch identifier 'eval' or 'arguments' in strict mode -> SyntaxError"));
			goto syntax_error;
		}

		duk_dup_top(thr);
		rc_varname = duk__getconst(comp_ctx);
		DUK_DDD(DUK_DDDPRINT("catch clause, rc_varname=0x%08lx (%ld)", (unsigned long) rc_varname, (long) rc_varname));

		duk__advance(comp_ctx);
		duk__advance_expect(comp_ctx, DUK_TOK_RPAREN);

		duk__advance_expect(comp_ctx, DUK_TOK_LCURLY);

		DUK_DDD(DUK_DDDPRINT("varmap before modifying for catch clause: %!iT",
		                     (duk_tval *) duk_get_tval(thr, comp_ctx->curr_func.varmap_idx)));

		duk_dup_top(thr);
		duk_get_prop(thr, comp_ctx->curr_func.varmap_idx);
		if (duk_is_undefined(thr, -1)) {
			varmap_value = -2;
		} else if (duk_is_null(thr, -1)) {
			varmap_value = -1;
		} else {
			DUK_ASSERT(duk_is_number(thr, -1));
			varmap_value = duk_get_int(thr, -1);
			DUK_ASSERT(varmap_value >= 0);
		}
		duk_pop(thr);

#if 0
		/* It'd be nice to do something like this - but it doesn't
		 * work for closures created inside the catch clause.
		 */
		duk_dup_top(thr);
		duk_push_int(thr, (duk_int_t) (reg_catch + 0));
		duk_put_prop(thr, comp_ctx->curr_func.varmap_idx);
#endif
		duk_dup_top(thr);
		duk_push_null(thr);
		duk_put_prop(thr, comp_ctx->curr_func.varmap_idx);

		duk__emit_a_bc(comp_ctx,
		               DUK_OP_PUTVAR | DUK__EMIT_FLAG_A_IS_SOURCE,
		               reg_catch + 0 /*value*/,
		               rc_varname /*varname*/);

		DUK_DDD(DUK_DDDPRINT("varmap before parsing catch clause: %!iT",
		                     (duk_tval *) duk_get_tval(thr, comp_ctx->curr_func.varmap_idx)));

		duk__parse_stmts(comp_ctx, 0 /*allow_source_elem*/, 0 /*expect_eof*/, 1 /*regexp_after*/);
		/* the DUK_TOK_RCURLY is eaten by duk__parse_stmts() */

		if (varmap_value == -2) {
			/* not present */
			duk_del_prop(thr, comp_ctx->curr_func.varmap_idx);
		} else {
			if (varmap_value == -1) {
				duk_push_null(thr);
			} else {
				DUK_ASSERT(varmap_value >= 0);
				duk_push_int(thr, varmap_value);
			}
			duk_put_prop(thr, comp_ctx->curr_func.varmap_idx);
		}
		/* varname is popped by above code */

		DUK_DDD(DUK_DDDPRINT("varmap after restore catch clause: %!iT",
		                     (duk_tval *) duk_get_tval(thr, comp_ctx->curr_func.varmap_idx)));

		duk__emit_op_only(comp_ctx, DUK_OP_ENDCATCH);

		/*
		 *  XXX: for now, indicate that an expensive catch binding
		 *  declarative environment is always needed.  If we don't
		 *  need it, we don't need the const_varname either.
		 */

		trycatch_flags |= DUK_BC_TRYCATCH_FLAG_CATCH_BINDING;

		DUK_DDD(DUK_DDDPRINT("stack top at end of catch clause: %ld", (long) duk_get_top(thr)));
	}

	if (comp_ctx->curr_token.t == DUK_TOK_FINALLY) {
		trycatch_flags |= DUK_BC_TRYCATCH_FLAG_HAVE_FINALLY;

		pc_finally = duk__get_current_pc(comp_ctx);

		duk__advance(comp_ctx);

		duk__advance_expect(comp_ctx, DUK_TOK_LCURLY);
		duk__parse_stmts(comp_ctx, 0 /*allow_source_elem*/, 0 /*expect_eof*/, 1 /*regexp_after*/);
		/* the DUK_TOK_RCURLY is eaten by duk__parse_stmts() */
		duk__emit_abc(comp_ctx, DUK_OP_ENDFIN, reg_catch); /* rethrow */
	}

	if (!(trycatch_flags & DUK_BC_TRYCATCH_FLAG_HAVE_CATCH) && !(trycatch_flags & DUK_BC_TRYCATCH_FLAG_HAVE_FINALLY)) {
		/* must have catch and/or finally */
		goto syntax_error;
	}

	/* If there's no catch block, rc_varname will be 0 and duk__patch_trycatch()
	 * will replace the LDCONST with a NOP.  For any actual constant (including
	 * constant 0) the DUK__CONST_MARKER flag will be set in rc_varname.
	 */

	duk__patch_trycatch(comp_ctx, pc_ldconst, pc_trycatch, reg_catch, rc_varname, trycatch_flags);

	if (trycatch_flags & DUK_BC_TRYCATCH_FLAG_HAVE_CATCH) {
		DUK_ASSERT(pc_catch >= 0);
		duk__patch_jump(comp_ctx, pc_trycatch + 1, pc_catch);
	}

	if (trycatch_flags & DUK_BC_TRYCATCH_FLAG_HAVE_FINALLY) {
		DUK_ASSERT(pc_finally >= 0);
		duk__patch_jump(comp_ctx, pc_trycatch + 2, pc_finally);
	} else {
		/* without finally, the second jump slot is used to jump to end of stmt */
		duk__patch_jump_here(comp_ctx, pc_trycatch + 2);
	}

	comp_ctx->curr_func.catch_depth--;
	return;

syntax_error:
	DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_TRY);
	DUK_WO_NORETURN(return;);
}

DUK_LOCAL void duk__parse_with_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res) {
	duk_int_t pc_trycatch;
	duk_int_t pc_finished;
	duk_regconst_t reg_catch;
	duk_small_uint_t trycatch_flags;

	if (comp_ctx->curr_func.is_strict) {
		DUK_ERROR_SYNTAX(comp_ctx->thr, DUK_STR_WITH_IN_STRICT_MODE);
		DUK_WO_NORETURN(return;);
	}

	comp_ctx->curr_func.catch_depth++;

	duk__advance(comp_ctx); /* eat 'with' */

	reg_catch = DUK__ALLOCTEMPS(comp_ctx, 2);

	duk__advance_expect(comp_ctx, DUK_TOK_LPAREN);
	duk__exprtop_toforcedreg(comp_ctx, res, DUK__BP_FOR_EXPR /*rbp_flags*/, reg_catch);
	comp_ctx->curr_func.allow_regexp_in_adv = 1;
	duk__advance_expect(comp_ctx, DUK_TOK_RPAREN); /* Allow RegExp as part of next stmt. */

	pc_trycatch = duk__get_current_pc(comp_ctx);
	trycatch_flags = DUK_BC_TRYCATCH_FLAG_WITH_BINDING;
	duk__emit_a_bc(comp_ctx,
	               DUK_OP_TRYCATCH | DUK__EMIT_FLAG_NO_SHUFFLE_A,
	               (duk_regconst_t) trycatch_flags /*a*/,
	               reg_catch /*bc*/);
	duk__emit_invalid(comp_ctx); /* catch jump */
	duk__emit_invalid(comp_ctx); /* finished jump */

	duk__parse_stmt(comp_ctx, res, 0 /*allow_source_elem*/);
	duk__emit_op_only(comp_ctx, DUK_OP_ENDTRY);

	pc_finished = duk__get_current_pc(comp_ctx);

	duk__patch_jump(comp_ctx, pc_trycatch + 2, pc_finished);

	comp_ctx->curr_func.catch_depth--;
}

DUK_LOCAL duk_int_t duk__stmt_label_site(duk_compiler_ctx *comp_ctx, duk_int_t label_id) {
	/* if a site already exists, nop: max one label site per statement */
	if (label_id >= 0) {
		return label_id;
	}

	label_id = comp_ctx->curr_func.label_next++;
	DUK_DDD(DUK_DDDPRINT("allocated new label id for label site: %ld", (long) label_id));

	duk__emit_bc(comp_ctx, DUK_OP_LABEL, (duk_regconst_t) label_id);
	duk__emit_invalid(comp_ctx);
	duk__emit_invalid(comp_ctx);

	return label_id;
}

/* Parse a single statement.
 *
 * Creates a label site (with an empty label) automatically for iteration
 * statements.  Also "peels off" any label statements for explicit labels.
 */
DUK_LOCAL void duk__parse_stmt(duk_compiler_ctx *comp_ctx, duk_ivalue *res, duk_bool_t allow_source_elem) {
	duk_hthread *thr = comp_ctx->thr;
	duk_bool_t dir_prol_at_entry; /* directive prologue status at entry */
	duk_regconst_t temp_at_entry;
	duk_size_t labels_len_at_entry;
	duk_int_t pc_at_entry; /* assumed to also be PC of "LABEL" */
	duk_int_t stmt_id;
	duk_small_uint_t stmt_flags = 0;
	duk_int_t label_id = -1;
	duk_small_uint_t tok;
	duk_bool_t test_func_decl;

	DUK__RECURSION_INCREASE(comp_ctx, thr);

	temp_at_entry = DUK__GETTEMP(comp_ctx);
	pc_at_entry = duk__get_current_pc(comp_ctx);
	labels_len_at_entry = duk_get_length(thr, comp_ctx->curr_func.labelnames_idx);
	stmt_id = comp_ctx->curr_func.stmt_next++;
	dir_prol_at_entry = comp_ctx->curr_func.in_directive_prologue;

	DUK_UNREF(stmt_id);

	DUK_DDD(DUK_DDDPRINT("parsing a statement, stmt_id=%ld, temp_at_entry=%ld, labels_len_at_entry=%ld, "
	                     "is_strict=%ld, in_directive_prologue=%ld, catch_depth=%ld",
	                     (long) stmt_id,
	                     (long) temp_at_entry,
	                     (long) labels_len_at_entry,
	                     (long) comp_ctx->curr_func.is_strict,
	                     (long) comp_ctx->curr_func.in_directive_prologue,
	                     (long) comp_ctx->curr_func.catch_depth));

	/* The directive prologue flag is cleared by default so that it is
	 * unset for any recursive statement parsing.  It is only "revived"
	 * if a directive is detected.  (We could also make directives only
	 * allowed if 'allow_source_elem' was true.)
	 */
	comp_ctx->curr_func.in_directive_prologue = 0;

retry_parse:

	DUK_DDD(DUK_DDDPRINT("try stmt parse, stmt_id=%ld, label_id=%ld, allow_source_elem=%ld, catch_depth=%ld",
	                     (long) stmt_id,
	                     (long) label_id,
	                     (long) allow_source_elem,
	                     (long) comp_ctx->curr_func.catch_depth));

	/*
	 *  Detect iteration statements; if encountered, establish an
	 *  empty label.
	 */

	tok = comp_ctx->curr_token.t;
	if (tok == DUK_TOK_FOR || tok == DUK_TOK_DO || tok == DUK_TOK_WHILE || tok == DUK_TOK_SWITCH) {
		DUK_DDD(DUK_DDDPRINT("iteration/switch statement -> add empty label"));

		label_id = duk__stmt_label_site(comp_ctx, label_id);
		duk__add_label(comp_ctx, DUK_HTHREAD_STRING_EMPTY_STRING(thr), pc_at_entry /*pc_label*/, label_id);
	}

	/*
	 *  Main switch for statement / source element type.
	 */

	switch (comp_ctx->curr_token.t) {
	case DUK_TOK_FUNCTION: {
		/*
		 *  Function declaration, function expression, or (non-standard)
		 *  function statement.
		 *
		 *  The E5 specification only allows function declarations at
		 *  the top level (in "source elements").  An ExpressionStatement
		 *  is explicitly not allowed to begin with a "function" keyword
		 *  (E5 Section 12.4).  Hence any non-error semantics for such
		 *  non-top-level statements are non-standard.  Duktape semantics
		 *  for function statements are modelled after V8, see
		 *  test-dev-func-decl-outside-top.js.
		 */
		test_func_decl = allow_source_elem;
#if defined(DUK_USE_NONSTD_FUNC_STMT)
		/* Lenient: allow function declarations outside top level in both
		 * strict and non-strict modes.  However, don't allow labelled
		 * function declarations in strict mode.
		 */
		test_func_decl = test_func_decl || !comp_ctx->curr_func.is_strict || label_id < 0;
#endif /* DUK_USE_NONSTD_FUNC_STMT */
		/* Strict: never allow function declarations outside top level. */
		if (test_func_decl) {
			/* FunctionDeclaration: not strictly a statement but handled as such.
			 *
			 * O(depth^2) parse count for inner functions is handled by recording a
			 * lexer offset on the first compilation pass, so that the function can
			 * be efficiently skipped on the second pass.  This is encapsulated into
			 * duk__parse_func_like_fnum().
			 */

			duk_int_t fnum;
#if defined(DUK_USE_ASSERTIONS)
			duk_idx_t top_before;
#endif

			DUK_DDD(DUK_DDDPRINT("function declaration statement"));

#if defined(DUK_USE_ASSERTIONS)
			top_before = duk_get_top(thr);
#endif

			duk__advance(comp_ctx); /* eat 'function' */
			fnum = duk__parse_func_like_fnum(comp_ctx, DUK__FUNC_FLAG_DECL | DUK__FUNC_FLAG_PUSHNAME_PASS1);

			/* The value stack convention here is a bit odd: the function
			 * name is only pushed on pass 1 (in_scanning), and is needed
			 * to process function declarations.
			 */
			if (comp_ctx->curr_func.in_scanning) {
				duk_uarridx_t n;

#if defined(DUK_USE_ASSERTIONS)
				DUK_ASSERT(duk_get_top(thr) == top_before + 1);
#endif
				DUK_DDD(DUK_DDDPRINT("register function declaration %!T in pass 1, fnum %ld",
				                     duk_get_tval(thr, -1),
				                     (long) fnum));
				n = (duk_uarridx_t) duk_get_length(thr, comp_ctx->curr_func.decls_idx);
				/* funcname is at index -1 */
				duk_put_prop_index(thr, comp_ctx->curr_func.decls_idx, n);
				duk_push_int(thr, (duk_int_t) (DUK_DECL_TYPE_FUNC + (fnum << 8)));
				duk_put_prop_index(thr, comp_ctx->curr_func.decls_idx, n + 1);
			} else {
#if defined(DUK_USE_ASSERTIONS)
				DUK_ASSERT(duk_get_top(thr) == top_before);
#endif
			}

			/* no statement value (unlike function expression) */
			stmt_flags = 0;
			break;
		} else {
			DUK_ERROR_SYNTAX(thr, DUK_STR_FUNC_STMT_NOT_ALLOWED);
			DUK_WO_NORETURN(return;);
		}
		break;
	}
	case DUK_TOK_LCURLY: {
		DUK_DDD(DUK_DDDPRINT("block statement"));
		duk__advance(comp_ctx);
		duk__parse_stmts(comp_ctx, 0 /*allow_source_elem*/, 0 /*expect_eof*/, 1 /*regexp_after*/);
		/* the DUK_TOK_RCURLY is eaten by duk__parse_stmts() */
		if (label_id >= 0) {
			duk__patch_jump_here(comp_ctx, pc_at_entry + 1); /* break jump */
		}
		stmt_flags = 0;
		break;
	}
	case DUK_TOK_CONST: {
		DUK_DDD(DUK_DDDPRINT("constant declaration statement"));
		duk__parse_var_stmt(comp_ctx, res, DUK__EXPR_FLAG_REQUIRE_INIT /*expr_flags*/);
		stmt_flags = DUK__HAS_TERM;
		break;
	}
	case DUK_TOK_VAR: {
		DUK_DDD(DUK_DDDPRINT("variable declaration statement"));
		duk__parse_var_stmt(comp_ctx, res, 0 /*expr_flags*/);
		stmt_flags = DUK__HAS_TERM;
		break;
	}
	case DUK_TOK_SEMICOLON: {
		/* empty statement with an explicit semicolon */
		DUK_DDD(DUK_DDDPRINT("empty statement"));
		stmt_flags = DUK__HAS_TERM;
		break;
	}
	case DUK_TOK_IF: {
		DUK_DDD(DUK_DDDPRINT("if statement"));
		duk__parse_if_stmt(comp_ctx, res);
		if (label_id >= 0) {
			duk__patch_jump_here(comp_ctx, pc_at_entry + 1); /* break jump */
		}
		stmt_flags = 0;
		break;
	}
	case DUK_TOK_DO: {
		/*
		 *  Do-while statement is mostly trivial, but there is special
		 *  handling for automatic semicolon handling (triggered by the
		 *  DUK__ALLOW_AUTO_SEMI_ALWAYS) flag related to a bug filed at:
		 *
		 *    https://bugs.ecmascript.org/show_bug.cgi?id=8
		 *
		 *  See doc/compiler.rst for details.
		 */
		DUK_DDD(DUK_DDDPRINT("do statement"));
		DUK_ASSERT(label_id >= 0);
		duk__update_label_flags(comp_ctx, label_id, DUK_LABEL_FLAG_ALLOW_BREAK | DUK_LABEL_FLAG_ALLOW_CONTINUE);
		duk__parse_do_stmt(comp_ctx, res, pc_at_entry);
		stmt_flags = DUK__HAS_TERM | DUK__ALLOW_AUTO_SEMI_ALWAYS; /* DUK__ALLOW_AUTO_SEMI_ALWAYS workaround */
		break;
	}
	case DUK_TOK_WHILE: {
		DUK_DDD(DUK_DDDPRINT("while statement"));
		DUK_ASSERT(label_id >= 0);
		duk__update_label_flags(comp_ctx, label_id, DUK_LABEL_FLAG_ALLOW_BREAK | DUK_LABEL_FLAG_ALLOW_CONTINUE);
		duk__parse_while_stmt(comp_ctx, res, pc_at_entry);
		stmt_flags = 0;
		break;
	}
	case DUK_TOK_FOR: {
		/*
		 *  For/for-in statement is complicated to parse because
		 *  determining the statement type (three-part for vs. a
		 *  for-in) requires potential backtracking.
		 *
		 *  See the helper for the messy stuff.
		 */
		DUK_DDD(DUK_DDDPRINT("for/for-in statement"));
		DUK_ASSERT(label_id >= 0);
		duk__update_label_flags(comp_ctx, label_id, DUK_LABEL_FLAG_ALLOW_BREAK | DUK_LABEL_FLAG_ALLOW_CONTINUE);
		duk__parse_for_stmt(comp_ctx, res, pc_at_entry);
		stmt_flags = 0;
		break;
	}
	case DUK_TOK_CONTINUE:
	case DUK_TOK_BREAK: {
		DUK_DDD(DUK_DDDPRINT("break/continue statement"));
		duk__parse_break_or_continue_stmt(comp_ctx, res);
		stmt_flags = DUK__HAS_TERM | DUK__IS_TERMINAL;
		break;
	}
	case DUK_TOK_RETURN: {
		DUK_DDD(DUK_DDDPRINT("return statement"));
		duk__parse_return_stmt(comp_ctx, res);
		stmt_flags = DUK__HAS_TERM | DUK__IS_TERMINAL;
		break;
	}
	case DUK_TOK_WITH: {
		DUK_DDD(DUK_DDDPRINT("with statement"));
		comp_ctx->curr_func.with_depth++;
		duk__parse_with_stmt(comp_ctx, res);
		if (label_id >= 0) {
			duk__patch_jump_here(comp_ctx, pc_at_entry + 1); /* break jump */
		}
		comp_ctx->curr_func.with_depth--;
		stmt_flags = 0;
		break;
	}
	case DUK_TOK_SWITCH: {
		/*
		 *  The switch statement is pretty messy to compile.
		 *  See the helper for details.
		 */
		DUK_DDD(DUK_DDDPRINT("switch statement"));
		DUK_ASSERT(label_id >= 0);
		duk__update_label_flags(comp_ctx, label_id, DUK_LABEL_FLAG_ALLOW_BREAK); /* don't allow continue */
		duk__parse_switch_stmt(comp_ctx, res, pc_at_entry);
		stmt_flags = 0;
		break;
	}
	case DUK_TOK_THROW: {
		DUK_DDD(DUK_DDDPRINT("throw statement"));
		duk__parse_throw_stmt(comp_ctx, res);
		stmt_flags = DUK__HAS_TERM | DUK__IS_TERMINAL;
		break;
	}
	case DUK_TOK_TRY: {
		DUK_DDD(DUK_DDDPRINT("try statement"));
		duk__parse_try_stmt(comp_ctx, res);
		stmt_flags = 0;
		break;
	}
	case DUK_TOK_DEBUGGER: {
		duk__advance(comp_ctx);
#if defined(DUK_USE_DEBUGGER_SUPPORT)
		DUK_DDD(DUK_DDDPRINT("debugger statement: debugging enabled, emit debugger opcode"));
		duk__emit_op_only(comp_ctx, DUK_OP_DEBUGGER);
#else
		DUK_DDD(DUK_DDDPRINT("debugger statement: ignored"));
#endif
		stmt_flags = DUK__HAS_TERM;
		break;
	}
	default: {
		/*
		 *  Else, must be one of:
		 *    - ExpressionStatement, possibly a directive (String)
		 *    - LabelledStatement (Identifier followed by ':')
		 *
		 *  Expressions beginning with 'function' keyword are covered by a case
		 *  above (such expressions are not allowed in standard E5 anyway).
		 *  Also expressions starting with '{' are interpreted as block
		 *  statements.  See E5 Section 12.4.
		 *
		 *  Directive detection is tricky; see E5 Section 14.1 on directive
		 *  prologue.  A directive is an expression statement with a single
		 *  string literal and an explicit or automatic semicolon.  Escape
		 *  characters are significant and no parens etc are allowed:
		 *
		 *    'use strict';          // valid 'use strict' directive
		 *    'use\u0020strict';     // valid directive, not a 'use strict' directive
		 *    ('use strict');        // not a valid directive
		 *
		 *  The expression is determined to consist of a single string literal
		 *  based on duk__expr_nud() and duk__expr_led() call counts.  The string literal
		 *  of a 'use strict' directive is determined to lack any escapes based
		 *  num_escapes count from the lexer.  Note that other directives may be
		 *  allowed to contain escapes, so a directive with escapes does not
		 *  terminate a directive prologue.
		 *
		 *  We rely on the fact that the expression parser will not emit any
		 *  code for a single token expression.  However, it will generate an
		 *  intermediate value which we will then successfully ignore.
		 *
		 *  A similar approach is used for labels.
		 */

		duk_bool_t single_token;

		DUK_DDD(DUK_DDDPRINT("expression statement"));
		duk__exprtop(comp_ctx, res, DUK__BP_FOR_EXPR /*rbp_flags*/);

		single_token = (comp_ctx->curr_func.nud_count == 1 && /* one token */
		                comp_ctx->curr_func.led_count == 0); /* no operators */

		if (single_token && comp_ctx->prev_token.t == DUK_TOK_IDENTIFIER && comp_ctx->curr_token.t == DUK_TOK_COLON) {
			/*
			 *  Detected label
			 */

			duk_hstring *h_lab;

			/* expected ival */
			DUK_ASSERT(res->t == DUK_IVAL_VAR);
			DUK_ASSERT(res->x1.t == DUK_ISPEC_VALUE);
			DUK_ASSERT(DUK_TVAL_IS_STRING(duk_get_tval(thr, res->x1.valstack_idx)));
			h_lab = comp_ctx->prev_token.str1;
			DUK_ASSERT(h_lab != NULL);

			DUK_DDD(DUK_DDDPRINT("explicit label site for label '%!O'", (duk_heaphdr *) h_lab));

			duk__advance(comp_ctx); /* eat colon */

			label_id = duk__stmt_label_site(comp_ctx, label_id);

			duk__add_label(comp_ctx, h_lab, pc_at_entry /*pc_label*/, label_id);

			/* a statement following a label cannot be a source element
			 * (a function declaration).
			 */
			allow_source_elem = 0;

			DUK_DDD(DUK_DDDPRINT("label handled, retry statement parsing"));
			goto retry_parse;
		}

		stmt_flags = 0;

		if (dir_prol_at_entry && /* still in prologue */
		    single_token && /* single string token */
		    comp_ctx->prev_token.t == DUK_TOK_STRING) {
			/*
			 *  Detected a directive
			 */
			duk_hstring *h_dir;

			/* expected ival */
			DUK_ASSERT(res->t == DUK_IVAL_PLAIN);
			DUK_ASSERT(res->x1.t == DUK_ISPEC_VALUE);
			DUK_ASSERT(DUK_TVAL_IS_STRING(duk_get_tval(thr, res->x1.valstack_idx)));
			h_dir = comp_ctx->prev_token.str1;
			DUK_ASSERT(h_dir != NULL);

			DUK_DDD(DUK_DDDPRINT("potential directive: %!O", h_dir));

			stmt_flags |= DUK__STILL_PROLOGUE;

			/* Note: escaped characters differentiate directives */

			if (comp_ctx->prev_token.num_escapes > 0) {
				DUK_DDD(DUK_DDDPRINT("directive contains escapes: valid directive "
				                     "but we ignore such directives"));
			} else {
				/*
				 * The length comparisons are present to handle
				 * strings like "use strict\u0000foo" as required.
				 */

				if (DUK_HSTRING_GET_BYTELEN(h_dir) == 10 &&
				    DUK_STRCMP((const char *) DUK_HSTRING_GET_DATA(h_dir), "use strict") == 0) {
#if defined(DUK_USE_STRICT_DECL)
					DUK_DDD(DUK_DDDPRINT("use strict directive detected: strict flag %ld -> %ld",
					                     (long) comp_ctx->curr_func.is_strict,
					                     (long) 1));
					comp_ctx->curr_func.is_strict = 1;
#else
					DUK_DDD(DUK_DDDPRINT("use strict detected but strict declarations disabled, ignoring"));
#endif
				} else if (DUK_HSTRING_GET_BYTELEN(h_dir) == 14 &&
				           DUK_STRCMP((const char *) DUK_HSTRING_GET_DATA(h_dir), "use duk notail") == 0) {
					DUK_DDD(DUK_DDDPRINT("use duk notail directive detected: notail flag %ld -> %ld",
					                     (long) comp_ctx->curr_func.is_notail,
					                     (long) 1));
					comp_ctx->curr_func.is_notail = 1;
				} else {
					DUK_DD(DUK_DDPRINT("unknown directive: '%!O', ignoring but not terminating "
					                   "directive prologue",
					                   (duk_hobject *) h_dir));
				}
			}
		} else {
			DUK_DDD(DUK_DDDPRINT("non-directive expression statement or no longer in prologue; "
			                     "prologue terminated if still active"));
		}

		stmt_flags |= DUK__HAS_VAL | DUK__HAS_TERM;
	}
	} /* end switch (tok) */

	/*
	 *  Statement value handling.
	 *
	 *  Global code and eval code has an implicit return value
	 *  which comes from the last statement with a value
	 *  (technically a non-"empty" continuation, which is
	 *  different from an empty statement).
	 *
	 *  Since we don't know whether a later statement will
	 *  override the value of the current statement, we need
	 *  to coerce the statement value to a register allocated
	 *  for implicit return values.  In other cases we need
	 *  to coerce the statement value to a plain value to get
	 *  any side effects out (consider e.g. "foo.bar;").
	 */

	/* XXX: what about statements which leave a half-cooked value in 'res'
	 * but have no stmt value?  Any such statements?
	 */

	if (stmt_flags & DUK__HAS_VAL) {
		duk_regconst_t reg_stmt_value = comp_ctx->curr_func.reg_stmt_value;
		if (reg_stmt_value >= 0) {
			duk__ivalue_toforcedreg(comp_ctx, res, reg_stmt_value);
		} else {
			duk__ivalue_toplain_ignore(comp_ctx, res);
		}
	} else {
		;
	}

	/*
	 *  Statement terminator check, including automatic semicolon
	 *  handling.  After this step, 'curr_tok' should be the first
	 *  token after a possible statement terminator.
	 */

	if (stmt_flags & DUK__HAS_TERM) {
		if (comp_ctx->curr_token.t == DUK_TOK_SEMICOLON) {
			DUK_DDD(DUK_DDDPRINT("explicit semicolon terminates statement"));
			duk__advance(comp_ctx);
		} else {
			if (comp_ctx->curr_token.allow_auto_semi) {
				DUK_DDD(DUK_DDDPRINT("automatic semicolon terminates statement"));
			} else if (stmt_flags & DUK__ALLOW_AUTO_SEMI_ALWAYS) {
				/* XXX: make this lenience dependent on flags or strictness? */
				DUK_DDD(DUK_DDDPRINT("automatic semicolon terminates statement (allowed for compatibility "
				                     "even though no lineterm present before next token)"));
			} else {
				DUK_ERROR_SYNTAX(thr, DUK_STR_UNTERMINATED_STMT);
				DUK_WO_NORETURN(return;);
			}
		}
	} else {
		DUK_DDD(DUK_DDDPRINT("statement has no terminator"));
	}

	/*
	 *  Directive prologue tracking.
	 */

	if (stmt_flags & DUK__STILL_PROLOGUE) {
		DUK_DDD(DUK_DDDPRINT("setting in_directive_prologue"));
		comp_ctx->curr_func.in_directive_prologue = 1;
	}

	/*
	 *  Cleanups (all statement parsing flows through here).
	 *
	 *  Pop label site and reset labels.  Reset 'next temp' to value at
	 *  entry to reuse temps.
	 */

	if (label_id >= 0) {
		duk__emit_bc(comp_ctx, DUK_OP_ENDLABEL, (duk_regconst_t) label_id);
	}

	DUK__SETTEMP(comp_ctx, temp_at_entry);

	duk__reset_labels_to_length(comp_ctx, labels_len_at_entry);

	/* XXX: return indication of "terminalness" (e.g. a 'throw' is terminal) */

	DUK__RECURSION_DECREASE(comp_ctx, thr);
}

/*
 *  Parse a statement list.
 *
 *  Handles automatic semicolon insertion and implicit return value.
 *
 *  Upon entry, 'curr_tok' should contain the first token of the first
 *  statement (parsed in the "allow regexp literal" mode).  Upon exit,
 *  'curr_tok' contains the token following the statement list terminator
 *  (EOF or closing brace).
 */

DUK_LOCAL void duk__parse_stmts(duk_compiler_ctx *comp_ctx,
                                duk_bool_t allow_source_elem,
                                duk_bool_t expect_eof,
                                duk_bool_t regexp_after) {
	duk_hthread *thr = comp_ctx->thr;
	duk_ivalue res_alloc;
	duk_ivalue *res = &res_alloc;

	/* Setup state.  Initial ivalue is 'undefined'. */

	duk_require_stack(thr, DUK__PARSE_STATEMENTS_SLOTS);

	/* XXX: 'res' setup can be moved to function body level; in fact, two 'res'
	 * intermediate values suffice for parsing of each function.  Nesting is needed
	 * for nested functions (which may occur inside expressions).
	 */

	duk_memzero(&res_alloc, sizeof(res_alloc));
	res->t = DUK_IVAL_PLAIN;
	res->x1.t = DUK_ISPEC_VALUE;
	res->x1.valstack_idx = duk_get_top(thr);
	res->x2.valstack_idx = res->x1.valstack_idx + 1;
	duk_push_undefined(thr);
	duk_push_undefined(thr);

	/* Parse statements until a closing token (EOF or '}') is found. */

	for (;;) {
		/* Check whether statement list ends. */

		if (expect_eof) {
			if (comp_ctx->curr_token.t == DUK_TOK_EOF) {
				break;
			}
		} else {
			if (comp_ctx->curr_token.t == DUK_TOK_RCURLY) {
				break;
			}
		}

		/* Check statement type based on the first token type.
		 *
		 * Note: expression parsing helpers expect 'curr_tok' to
		 * contain the first token of the expression upon entry.
		 */

		DUK_DDD(DUK_DDDPRINT("TOKEN %ld (non-whitespace, non-comment)", (long) comp_ctx->curr_token.t));

		duk__parse_stmt(comp_ctx, res, allow_source_elem);
	}

	/* RegExp is allowed / not allowed depending on context.  For function
	 * declarations RegExp is allowed because it follows a function
	 * declaration statement and may appear as part of the next statement.
	 * For function expressions RegExp is not allowed, and it's possible
	 * to do something like '(function () {} / 123)'.
	 */
	if (regexp_after) {
		comp_ctx->curr_func.allow_regexp_in_adv = 1;
	}
	duk__advance(comp_ctx);

	/* Tear down state. */

	duk_pop_2(thr);
}

/*
 *  Declaration binding instantiation conceptually happens when calling a
 *  function; for us it essentially means that function prologue.  The
 *  conceptual process is described in E5 Section 10.5.
 *
 *  We need to keep track of all encountered identifiers to (1) create an
 *  identifier-to-register map ("varmap"); and (2) detect duplicate
 *  declarations.  Identifiers which are not bound to registers still need
 *  to be tracked for detecting duplicates.  Currently such identifiers
 *  are put into the varmap with a 'null' value, which is later cleaned up.
 *
 *  To support functions with a large number of variable and function
 *  declarations, registers are not allocated beyond a certain limit;
 *  after that limit, variables and functions need slow path access.
 *  Arguments are currently always register bound, which imposes a hard
 *  (and relatively small) argument count limit.
 *
 *  Some bindings in E5 are not configurable (= deletable) and almost all
 *  are mutable (writable).  Exceptions are:
 *
 *    - The 'arguments' binding, established only if no shadowing argument
 *      or function declaration exists.  We handle 'arguments' creation
 *      and binding through an explicit slow path environment record.
 *
 *    - The "name" binding for a named function expression.  This is also
 *      handled through an explicit slow path environment record.
 */

/* XXX: add support for variables to not be register bound always, to
 * handle cases with a very large number of variables?
 */

DUK_LOCAL void duk__init_varmap_and_prologue_for_pass2(duk_compiler_ctx *comp_ctx, duk_regconst_t *out_stmt_value_reg) {
	duk_hthread *thr = comp_ctx->thr;
	duk_hstring *h_name;
	duk_bool_t configurable_bindings;
	duk_uarridx_t num_args;
	duk_uarridx_t num_decls;
	duk_regconst_t rc_name;
	duk_small_uint_t declvar_flags;
	duk_uarridx_t i;
#if defined(DUK_USE_ASSERTIONS)
	duk_idx_t entry_top;
#endif

#if defined(DUK_USE_ASSERTIONS)
	entry_top = duk_get_top(thr);
#endif

	/*
	 *  Preliminaries
	 */

	configurable_bindings = comp_ctx->curr_func.is_eval;
	DUK_DDD(DUK_DDDPRINT("configurable_bindings=%ld", (long) configurable_bindings));

	/* varmap is already in comp_ctx->curr_func.varmap_idx */

	/*
	 *  Function formal arguments, always bound to registers
	 *  (there's no support for shuffling them now).
	 */

	num_args = (duk_uarridx_t) duk_get_length(thr, comp_ctx->curr_func.argnames_idx);
	DUK_DDD(DUK_DDDPRINT("num_args=%ld", (long) num_args));
	/* XXX: check num_args */

	for (i = 0; i < num_args; i++) {
		duk_get_prop_index(thr, comp_ctx->curr_func.argnames_idx, i);
		h_name = duk_known_hstring(thr, -1);

		if (comp_ctx->curr_func.is_strict) {
			if (duk__hstring_is_eval_or_arguments(comp_ctx, h_name)) {
				DUK_DDD(DUK_DDDPRINT("arg named 'eval' or 'arguments' in strict mode -> SyntaxError"));
				goto error_argname;
			}
			duk_dup_top(thr);
			if (duk_has_prop(thr, comp_ctx->curr_func.varmap_idx)) {
				DUK_DDD(DUK_DDDPRINT("duplicate arg name in strict mode -> SyntaxError"));
				goto error_argname;
			}

			/* Ensure argument name is not a reserved word in current
			 * (final) strictness.  Formal argument parsing may not
			 * catch reserved names if strictness changes during
			 * parsing.
			 *
			 * We only need to do this in strict mode because non-strict
			 * keyword are always detected in formal argument parsing.
			 */

			if (DUK_HSTRING_HAS_STRICT_RESERVED_WORD(h_name)) {
				goto error_argname;
			}
		}

		/* overwrite any previous binding of the same name; the effect is
		 * that last argument of a certain name wins.
		 */

		/* only functions can have arguments */
		DUK_ASSERT(comp_ctx->curr_func.is_function);
		duk_push_uarridx(thr, i); /* -> [ ... name index ] */
		duk_put_prop(thr, comp_ctx->curr_func.varmap_idx); /* -> [ ... ] */

		/* no code needs to be emitted, the regs already have values */
	}

	/* use temp_next for tracking register allocations */
	DUK__SETTEMP_CHECKMAX(comp_ctx, (duk_regconst_t) num_args);

	/*
	 *  After arguments, allocate special registers (like shuffling temps)
	 */

	if (out_stmt_value_reg) {
		*out_stmt_value_reg = DUK__ALLOCTEMP(comp_ctx);
	}
	if (comp_ctx->curr_func.needs_shuffle) {
		duk_regconst_t shuffle_base = DUK__ALLOCTEMPS(comp_ctx, 3);
		comp_ctx->curr_func.shuffle1 = shuffle_base;
		comp_ctx->curr_func.shuffle2 = shuffle_base + 1;
		comp_ctx->curr_func.shuffle3 = shuffle_base + 2;
		DUK_D(DUK_DPRINT("shuffle registers needed by function, allocated: %ld %ld %ld",
		                 (long) comp_ctx->curr_func.shuffle1,
		                 (long) comp_ctx->curr_func.shuffle2,
		                 (long) comp_ctx->curr_func.shuffle3));
	}
	if (comp_ctx->curr_func.temp_next > 0x100) {
		DUK_D(DUK_DPRINT("not enough 8-bit regs: temp_next=%ld", (long) comp_ctx->curr_func.temp_next));
		goto error_outofregs;
	}

	/*
	 *  Function declarations
	 */

	num_decls = (duk_uarridx_t) duk_get_length(thr, comp_ctx->curr_func.decls_idx);
	DUK_DDD(
	    DUK_DDDPRINT("num_decls=%ld -> %!T", (long) num_decls, (duk_tval *) duk_get_tval(thr, comp_ctx->curr_func.decls_idx)));
	for (i = 0; i < num_decls; i += 2) {
		duk_int_t decl_type;
		duk_int_t fnum;

		duk_get_prop_index(thr, comp_ctx->curr_func.decls_idx, i + 1); /* decl type */
		decl_type = duk_to_int(thr, -1);
		fnum = decl_type >> 8; /* XXX: macros */
		decl_type = decl_type & 0xff;
		duk_pop(thr);

		if (decl_type != DUK_DECL_TYPE_FUNC) {
			continue;
		}

		duk_get_prop_index(thr, comp_ctx->curr_func.decls_idx, i); /* decl name */

		/* XXX: spilling */
		if (comp_ctx->curr_func.is_function) {
			duk_regconst_t reg_bind;
			duk_dup_top(thr);
			if (duk_has_prop(thr, comp_ctx->curr_func.varmap_idx)) {
				/* shadowed; update value */
				duk_dup_top(thr);
				duk_get_prop(thr, comp_ctx->curr_func.varmap_idx);
				reg_bind = duk_to_int(thr, -1); /* [ ... name reg_bind ] */
				duk__emit_a_bc(comp_ctx, DUK_OP_CLOSURE, reg_bind, (duk_regconst_t) fnum);
			} else {
				/* function: always register bound */
				reg_bind = DUK__ALLOCTEMP(comp_ctx);
				duk__emit_a_bc(comp_ctx, DUK_OP_CLOSURE, reg_bind, (duk_regconst_t) fnum);
				duk_push_int(thr, (duk_int_t) reg_bind);
			}
		} else {
			/* Function declaration for global/eval code is emitted even
			 * for duplicates, because of E5 Section 10.5, step 5.e of
			 * E5.1 (special behavior for variable bound to global object).
			 *
			 * DECLVAR will not re-declare a variable as such, but will
			 * update the binding value.
			 */

			duk_regconst_t reg_temp = DUK__ALLOCTEMP(comp_ctx);
			duk_dup_top(thr);
			rc_name = duk__getconst(comp_ctx);
			duk_push_null(thr);

			duk__emit_a_bc(comp_ctx, DUK_OP_CLOSURE, reg_temp, (duk_regconst_t) fnum);

			declvar_flags = DUK_PROPDESC_FLAG_WRITABLE | DUK_PROPDESC_FLAG_ENUMERABLE | DUK_BC_DECLVAR_FLAG_FUNC_DECL;

			if (configurable_bindings) {
				declvar_flags |= DUK_PROPDESC_FLAG_CONFIGURABLE;
			}

			duk__emit_a_b_c(comp_ctx,
			                DUK_OP_DECLVAR | DUK__EMIT_FLAG_NO_SHUFFLE_A | DUK__EMIT_FLAG_BC_REGCONST,
			                (duk_regconst_t) declvar_flags /*flags*/,
			                rc_name /*name*/,
			                reg_temp /*value*/);

			DUK__SETTEMP(comp_ctx, reg_temp); /* forget temp */
		}

		DUK_DDD(DUK_DDDPRINT("function declaration to varmap: %!T -> %!T",
		                     (duk_tval *) duk_get_tval(thr, -2),
		                     (duk_tval *) duk_get_tval(thr, -1)));

#if defined(DUK_USE_FASTINT)
		DUK_ASSERT(DUK_TVAL_IS_NULL(duk_get_tval(thr, -1)) || DUK_TVAL_IS_FASTINT(duk_get_tval(thr, -1)));
#endif
		duk_put_prop(thr, comp_ctx->curr_func.varmap_idx); /* [ ... name reg/null ] -> [ ... ] */
	}

	/*
	 *  'arguments' binding is special; if a shadowing argument or
	 *  function declaration exists, an arguments object will
	 *  definitely not be needed, regardless of whether the identifier
	 *  'arguments' is referenced inside the function body.
	 */

	if (duk_has_prop_stridx(thr, comp_ctx->curr_func.varmap_idx, DUK_STRIDX_LC_ARGUMENTS)) {
		DUK_DDD(DUK_DDDPRINT("'arguments' is shadowed by argument or function declaration "
		                     "-> arguments object creation can be skipped"));
		comp_ctx->curr_func.is_arguments_shadowed = 1;
	}

	/*
	 *  Variable declarations.
	 *
	 *  Unlike function declarations, variable declaration values don't get
	 *  assigned on entry.  If a binding of the same name already exists, just
	 *  ignore it silently.
	 */

	for (i = 0; i < num_decls; i += 2) {
		duk_int_t decl_type;

		duk_get_prop_index(thr, comp_ctx->curr_func.decls_idx, i + 1); /* decl type */
		decl_type = duk_to_int(thr, -1);
		decl_type = decl_type & 0xff;
		duk_pop(thr);

		if (decl_type != DUK_DECL_TYPE_VAR) {
			continue;
		}

		duk_get_prop_index(thr, comp_ctx->curr_func.decls_idx, i); /* decl name */

		if (duk_has_prop(thr, comp_ctx->curr_func.varmap_idx)) {
			/* shadowed, ignore */
		} else {
			duk_get_prop_index(thr, comp_ctx->curr_func.decls_idx, i); /* decl name */
			h_name = duk_known_hstring(thr, -1);

			if (h_name == DUK_HTHREAD_STRING_LC_ARGUMENTS(thr) && !comp_ctx->curr_func.is_arguments_shadowed) {
				/* E5 Section steps 7-8 */
				DUK_DDD(DUK_DDDPRINT("'arguments' not shadowed by a function declaration, "
				                     "but appears as a variable declaration -> treat as "
				                     "a no-op for variable declaration purposes"));
				duk_pop(thr);
				continue;
			}

			/* XXX: spilling */
			if (comp_ctx->curr_func.is_function) {
				duk_regconst_t reg_bind = DUK__ALLOCTEMP(comp_ctx);
				/* no need to init reg, it will be undefined on entry */
				duk_push_int(thr, (duk_int_t) reg_bind);
			} else {
				duk_dup_top(thr);
				rc_name = duk__getconst(comp_ctx);
				duk_push_null(thr);

				declvar_flags = DUK_PROPDESC_FLAG_WRITABLE | DUK_PROPDESC_FLAG_ENUMERABLE;
				if (configurable_bindings) {
					declvar_flags |= DUK_PROPDESC_FLAG_CONFIGURABLE;
				}

				duk__emit_a_b_c(comp_ctx,
				                DUK_OP_DECLVAR | DUK__EMIT_FLAG_NO_SHUFFLE_A | DUK__EMIT_FLAG_BC_REGCONST,
				                (duk_regconst_t) declvar_flags /*flags*/,
				                rc_name /*name*/,
				                0 /*value*/);
			}

			duk_put_prop(thr, comp_ctx->curr_func.varmap_idx); /* [ ... name reg/null ] -> [ ... ] */
		}
	}

	/*
	 *  Wrap up
	 */

	DUK_DDD(DUK_DDDPRINT("varmap: %!T, is_arguments_shadowed=%ld",
	                     (duk_tval *) duk_get_tval(thr, comp_ctx->curr_func.varmap_idx),
	                     (long) comp_ctx->curr_func.is_arguments_shadowed));

	DUK_ASSERT_TOP(thr, entry_top);
	return;

error_outofregs:
	DUK_ERROR_RANGE(thr, DUK_STR_REG_LIMIT);
	DUK_WO_NORETURN(return;);

error_argname:
	DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_ARG_NAME);
	DUK_WO_NORETURN(return;);
}

/*
 *  Parse a function-body-like expression (FunctionBody or Program
 *  in E5 grammar) using a two-pass parse.  The productions appear
 *  in the following contexts:
 *
 *    - function expression
 *    - function statement
 *    - function declaration
 *    - getter in object literal
 *    - setter in object literal
 *    - global code
 *    - eval code
 *    - Function constructor body
 *
 *  This function only parses the statement list of the body; the argument
 *  list and possible function name must be initialized by the caller.
 *  For instance, for Function constructor, the argument names are originally
 *  on the value stack.  The parsing of statements ends either at an EOF or
 *  a closing brace; this is controlled by an input flag.
 *
 *  Note that there are many differences affecting parsing and even code
 *  generation:
 *
 *    - Global and eval code have an implicit return value generated
 *      by the last statement; function code does not
 *
 *    - Global code, eval code, and Function constructor body end in
 *      an EOF, other bodies in a closing brace ('}')
 *
 *  Upon entry, 'curr_tok' is ignored and the function will pull in the
 *  first token on its own.  Upon exit, 'curr_tok' is the terminating
 *  token (EOF or closing brace).
 */

DUK_LOCAL void duk__parse_func_body(duk_compiler_ctx *comp_ctx,
                                    duk_bool_t expect_eof,
                                    duk_bool_t implicit_return_value,
                                    duk_bool_t regexp_after,
                                    duk_small_int_t expect_token) {
	duk_compiler_func *func;
	duk_hthread *thr;
	duk_regconst_t reg_stmt_value = -1;
	duk_lexer_point lex_pt;
	duk_regconst_t temp_first;
	duk_small_int_t compile_round = 1;

	DUK_ASSERT(comp_ctx != NULL);

	thr = comp_ctx->thr;
	DUK_ASSERT(thr != NULL);

	func = &comp_ctx->curr_func;
	DUK_ASSERT(func != NULL);

	DUK__RECURSION_INCREASE(comp_ctx, thr);

	duk_require_stack(thr, DUK__FUNCTION_BODY_REQUIRE_SLOTS);

	/*
	 *  Store lexer position for a later rewind
	 */

	DUK_LEXER_GETPOINT(&comp_ctx->lex, &lex_pt);

	/*
	 *  Program code (global and eval code) has an implicit return value
	 *  from the last statement value (e.g. eval("1; 2+3;") returns 3).
	 *  This is not the case with functions.  If implicit statement return
	 *  value is requested, all statements are coerced to a register
	 *  allocated here, and used in the implicit return statement below.
	 */

	/* XXX: this is pointless here because pass 1 is throw-away */
	if (implicit_return_value) {
		reg_stmt_value = DUK__ALLOCTEMP(comp_ctx);

		/* If an implicit return value is needed by caller, it must be
		 * initialized to 'undefined' because we don't know whether any
		 * non-empty (where "empty" is a continuation type, and different
		 * from an empty statement) statements will be executed.
		 *
		 * However, since 1st pass is a throwaway one, no need to emit
		 * it here.
		 */
#if 0
		duk__emit_bc(comp_ctx,
		             DUK_OP_LDUNDEF,
		             0);
#endif
	}

	/*
	 *  First pass.
	 *
	 *  Gather variable/function declarations needed for second pass.
	 *  Code generated is dummy and discarded.
	 */

	func->in_directive_prologue = 1;
	func->in_scanning = 1;
	func->may_direct_eval = 0;
	func->id_access_arguments = 0;
	func->id_access_slow = 0;
	func->id_access_slow_own = 0;
	func->reg_stmt_value = reg_stmt_value;
#if defined(DUK_USE_DEBUGGER_SUPPORT)
	func->min_line = DUK_INT_MAX;
	func->max_line = 0;
#endif

	/* duk__parse_stmts() expects curr_tok to be set; parse in "allow
	 * regexp literal" mode with current strictness.
	 */
	if (expect_token >= 0) {
		/* Eating a left curly; regexp mode is allowed by left curly
		 * based on duk__token_lbp[] automatically.
		 */
		DUK_ASSERT(expect_token == DUK_TOK_LCURLY);
		duk__update_lineinfo_currtoken(comp_ctx);
		duk__advance_expect(comp_ctx, expect_token);
	} else {
		/* Need to set curr_token.t because lexing regexp mode depends on current
		 * token type.  Zero value causes "allow regexp" mode.
		 */
		comp_ctx->curr_token.t = 0;
		duk__advance(comp_ctx);
	}

	DUK_DDD(DUK_DDDPRINT("begin 1st pass"));
	duk__parse_stmts(comp_ctx,
	                 1, /* allow source elements */
	                 expect_eof, /* expect EOF instead of } */
	                 regexp_after); /* regexp after */
	DUK_DDD(DUK_DDDPRINT("end 1st pass"));

	/*
	 *  Second (and possibly third) pass.
	 *
	 *  Generate actual code.  In most cases the need for shuffle
	 *  registers is detected during pass 1, but in some corner cases
	 *  we'll only detect it during pass 2 and a third pass is then
	 *  needed (see GH-115).
	 */

	for (;;) {
		duk_bool_t needs_shuffle_before = comp_ctx->curr_func.needs_shuffle;
		compile_round++;

		/*
		 *  Rewind lexer.
		 *
		 *  duk__parse_stmts() expects curr_tok to be set; parse in "allow regexp
		 *  literal" mode with current strictness.
		 *
		 *  curr_token line number info should be initialized for pass 2 before
		 *  generating prologue, to ensure prologue bytecode gets nice line numbers.
		 */

		DUK_DDD(DUK_DDDPRINT("rewind lexer"));
		DUK_LEXER_SETPOINT(&comp_ctx->lex, &lex_pt);
		comp_ctx->curr_token.t = 0; /* this is needed for regexp mode */
		comp_ctx->curr_token.start_line = 0; /* needed for line number tracking (becomes prev_token.start_line) */
		duk__advance(comp_ctx);

		/*
		 *  Reset function state and perform register allocation, which creates
		 *  'varmap' for second pass.  Function prologue for variable declarations,
		 *  binding value initializations etc is emitted as a by-product.
		 *
		 *  Strict mode restrictions for duplicate and invalid argument
		 *  names are checked here now that we know whether the function
		 *  is actually strict.  See: test-dev-strict-mode-boundary.js.
		 *
		 *  Inner functions are compiled during pass 1 and are not reset.
		 */

		duk__reset_func_for_pass2(comp_ctx);
		func->in_directive_prologue = 1;
		func->in_scanning = 0;

		/* must be able to emit code, alloc consts, etc. */

		duk__init_varmap_and_prologue_for_pass2(comp_ctx, (implicit_return_value ? &reg_stmt_value : NULL));
		func->reg_stmt_value = reg_stmt_value;

		temp_first = DUK__GETTEMP(comp_ctx);

		func->temp_first = temp_first;
		func->temp_next = temp_first;
		func->stmt_next = 0;
		func->label_next = 0;

		/* XXX: init or assert catch depth etc -- all values */
		func->id_access_arguments = 0;
		func->id_access_slow = 0;
		func->id_access_slow_own = 0;

		/*
		 *  Check function name validity now that we know strictness.
		 *  This only applies to function declarations and expressions,
		 *  not setter/getter name.
		 *
		 *  See: test-dev-strict-mode-boundary.js
		 */

		if (func->is_function && !func->is_setget && func->h_name != NULL) {
			if (func->is_strict) {
				if (duk__hstring_is_eval_or_arguments(comp_ctx, func->h_name)) {
					DUK_DDD(DUK_DDDPRINT("func name is 'eval' or 'arguments' in strict mode"));
					goto error_funcname;
				}
				if (DUK_HSTRING_HAS_STRICT_RESERVED_WORD(func->h_name)) {
					DUK_DDD(DUK_DDDPRINT("func name is a reserved word in strict mode"));
					goto error_funcname;
				}
			} else {
				if (DUK_HSTRING_HAS_RESERVED_WORD(func->h_name) &&
				    !DUK_HSTRING_HAS_STRICT_RESERVED_WORD(func->h_name)) {
					DUK_DDD(DUK_DDDPRINT("func name is a reserved word in non-strict mode"));
					goto error_funcname;
				}
			}
		}

		/*
		 *  Second pass parsing.
		 */

		if (implicit_return_value) {
			/* Default implicit return value. */
			duk__emit_bc(comp_ctx, DUK_OP_LDUNDEF, 0);
		}

		DUK_DDD(DUK_DDDPRINT("begin 2nd pass"));
		duk__parse_stmts(comp_ctx,
		                 1, /* allow source elements */
		                 expect_eof, /* expect EOF instead of } */
		                 regexp_after); /* regexp after */
		DUK_DDD(DUK_DDDPRINT("end 2nd pass"));

		duk__update_lineinfo_currtoken(comp_ctx);

		if (needs_shuffle_before == comp_ctx->curr_func.needs_shuffle) {
			/* Shuffle decision not changed. */
			break;
		}
		if (compile_round >= 3) {
			/* Should never happen but avoid infinite loop just in case. */
			DUK_D(DUK_DPRINT("more than 3 compile passes needed, should never happen"));
			DUK_ERROR_INTERNAL(thr);
			DUK_WO_NORETURN(return;);
		}
		DUK_D(DUK_DPRINT("need additional round to compile function, round now %d", (int) compile_round));
	}

	/*
	 *  Emit a final RETURN.
	 *
	 *  It would be nice to avoid emitting an unnecessary "return" opcode
	 *  if the current PC is not reachable.  However, this cannot be reliably
	 *  detected; even if the previous instruction is an unconditional jump,
	 *  there may be a previous jump which jumps to current PC (which is the
	 *  case for iteration and conditional statements, for instance).
	 */

	/* XXX: request a "last statement is terminal" from duk__parse_stmt() and duk__parse_stmts();
	 * we could avoid the last RETURN if we could ensure there is no way to get here
	 * (directly or via a jump)
	 */

	DUK_ASSERT(comp_ctx->curr_func.catch_depth == 0);
	if (reg_stmt_value >= 0) {
		DUK_ASSERT(DUK__ISREG(reg_stmt_value));
		duk__emit_bc(comp_ctx, DUK_OP_RETREG, reg_stmt_value /*reg*/);
	} else {
		duk__emit_op_only(comp_ctx, DUK_OP_RETUNDEF);
	}

	/*
	 *  Peephole optimize JUMP chains.
	 */

	duk__peephole_optimize_bytecode(comp_ctx);

	/*
	 *  comp_ctx->curr_func is now ready to be converted into an actual
	 *  function template.
	 */

	DUK__RECURSION_DECREASE(comp_ctx, thr);
	return;

error_funcname:
	DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_FUNC_NAME);
	DUK_WO_NORETURN(return;);
}

/*
 *  Parse a function-like expression:
 *
 *    - function expression
 *    - function declaration
 *    - function statement (non-standard)
 *    - setter/getter
 *
 *  Adds the function to comp_ctx->curr_func function table and returns the
 *  function number.
 *
 *  On entry, curr_token points to:
 *
 *    - the token after 'function' for function expression/declaration/statement
 *    - the token after 'set' or 'get' for setter/getter
 */

/* Parse formals. */
DUK_LOCAL void duk__parse_func_formals(duk_compiler_ctx *comp_ctx) {
	duk_hthread *thr = comp_ctx->thr;
	duk_bool_t first = 1;
	duk_uarridx_t n;

	for (;;) {
		if (comp_ctx->curr_token.t == DUK_TOK_RPAREN) {
			break;
		}

		if (first) {
			/* no comma */
			first = 0;
		} else {
			duk__advance_expect(comp_ctx, DUK_TOK_COMMA);
		}

		/* Note: when parsing a formal list in non-strict context, e.g.
		 * "implements" is parsed as an identifier.  When the function is
		 * later detected to be strict, the argument list must be rechecked
		 * against a larger set of reserved words (that of strict mode).
		 * This is handled by duk__parse_func_body().  Here we recognize
		 * whatever tokens are considered reserved in current strictness
		 * (which is not always enough).
		 */

		if (comp_ctx->curr_token.t != DUK_TOK_IDENTIFIER) {
			DUK_ERROR_SYNTAX(thr, DUK_STR_EXPECTED_IDENTIFIER);
			DUK_WO_NORETURN(return;);
		}
		DUK_ASSERT(comp_ctx->curr_token.t == DUK_TOK_IDENTIFIER);
		DUK_ASSERT(comp_ctx->curr_token.str1 != NULL);
		DUK_DDD(DUK_DDDPRINT("formal argument: %!O", (duk_heaphdr *) comp_ctx->curr_token.str1));

		/* XXX: append primitive */
		duk_push_hstring(thr, comp_ctx->curr_token.str1);
		n = (duk_uarridx_t) duk_get_length(thr, comp_ctx->curr_func.argnames_idx);
		duk_put_prop_index(thr, comp_ctx->curr_func.argnames_idx, n);

		duk__advance(comp_ctx); /* eat identifier */
	}
}

/* Parse a function-like expression, assuming that 'comp_ctx->curr_func' is
 * correctly set up.  Assumes that curr_token is just after 'function' (or
 * 'set'/'get' etc).
 */
DUK_LOCAL void duk__parse_func_like_raw(duk_compiler_ctx *comp_ctx, duk_small_uint_t flags) {
	duk_hthread *thr = comp_ctx->thr;
	duk_token *tok;
	duk_bool_t no_advance;

	DUK_ASSERT(comp_ctx->curr_func.num_formals == 0);
	DUK_ASSERT(comp_ctx->curr_func.is_function == 1);
	DUK_ASSERT(comp_ctx->curr_func.is_eval == 0);
	DUK_ASSERT(comp_ctx->curr_func.is_global == 0);
	DUK_ASSERT(comp_ctx->curr_func.is_setget == ((flags & DUK__FUNC_FLAG_GETSET) != 0));

	duk__update_lineinfo_currtoken(comp_ctx);

	/*
	 *  Function name (if any)
	 *
	 *  We don't check for prohibited names here, because we don't
	 *  yet know whether the function will be strict.  Function body
	 *  parsing handles this retroactively.
	 *
	 *  For function expressions and declarations function name must
	 *  be an Identifer (excludes reserved words).  For setter/getter
	 *  it is a PropertyName which allows reserved words and also
	 *  strings and numbers (e.g. "{ get 1() { ... } }").
	 *
	 *  Function parsing may start either from prev_token or curr_token
	 *  (object literal method definition uses prev_token for example).
	 *  This is dealt with for the initial token.
	 */

	no_advance = (flags & DUK__FUNC_FLAG_USE_PREVTOKEN);
	if (no_advance) {
		tok = &comp_ctx->prev_token;
	} else {
		tok = &comp_ctx->curr_token;
	}

	if (flags & DUK__FUNC_FLAG_GETSET) {
		/* PropertyName -> IdentifierName | StringLiteral | NumericLiteral */
		if (tok->t_nores == DUK_TOK_IDENTIFIER || tok->t == DUK_TOK_STRING) {
			duk_push_hstring(thr, tok->str1); /* keep in valstack */
		} else if (tok->t == DUK_TOK_NUMBER) {
			duk_push_number(thr, tok->num);
			duk_to_string(thr, -1);
		} else {
			DUK_ERROR_SYNTAX(thr, DUK_STR_INVALID_GETSET_NAME);
			DUK_WO_NORETURN(return;);
		}
		comp_ctx->curr_func.h_name = duk_known_hstring(thr, -1); /* borrowed reference */
	} else {
		/* Function name is an Identifier (not IdentifierName), but we get
		 * the raw name (not recognizing keywords) here and perform the name
		 * checks only after pass 1.
		 */
		if (tok->t_nores == DUK_TOK_IDENTIFIER) {
			duk_push_hstring(thr, tok->str1); /* keep in valstack */
			comp_ctx->curr_func.h_name = duk_known_hstring(thr, -1); /* borrowed reference */
		} else {
			/* valstack will be unbalanced, which is OK */
			DUK_ASSERT((flags & DUK__FUNC_FLAG_GETSET) == 0);
			DUK_ASSERT(comp_ctx->curr_func.h_name == NULL);
			no_advance = 1;
			if (flags & DUK__FUNC_FLAG_DECL) {
				DUK_ERROR_SYNTAX(thr, DUK_STR_FUNC_NAME_REQUIRED);
				DUK_WO_NORETURN(return;);
			}
		}
	}

	DUK_DD(DUK_DDPRINT("function name: %!O", (duk_heaphdr *) comp_ctx->curr_func.h_name));

	if (!no_advance) {
		duk__advance(comp_ctx);
	}

	/*
	 *  Formal argument list
	 *
	 *  We don't check for prohibited names or for duplicate argument
	 *  names here, becase we don't yet know whether the function will
	 *  be strict.  Function body parsing handles this retroactively.
	 */

	duk__advance_expect(comp_ctx, DUK_TOK_LPAREN);

	duk__parse_func_formals(comp_ctx);

	DUK_ASSERT(comp_ctx->curr_token.t == DUK_TOK_RPAREN);
	duk__advance(comp_ctx);

	/*
	 *  Parse function body
	 */

	duk__parse_func_body(comp_ctx,
	                     0, /* expect_eof */
	                     0, /* implicit_return_value */
	                     flags & DUK__FUNC_FLAG_DECL, /* regexp_after */
	                     DUK_TOK_LCURLY); /* expect_token */

	/*
	 *  Convert duk_compiler_func to a function template and add it
	 *  to the parent function table.
	 */

	duk__convert_to_func_template(comp_ctx); /* -> [ ... func ] */
}

/* Parse an inner function, adding the function template to the current function's
 * function table.  Return a function number to be used by the outer function.
 *
 * Avoiding O(depth^2) inner function parsing is handled here.  On the first pass,
 * compile and register the function normally into the 'funcs' array, also recording
 * a lexer point (offset/line) to the closing brace of the function.  On the second
 * pass, skip the function and return the same 'fnum' as on the first pass by using
 * a running counter.
 *
 * An unfortunate side effect of this is that when parsing the inner function, almost
 * nothing is known of the outer function, i.e. the inner function's scope.  We don't
 * need that information at the moment, but it would allow some optimizations if it
 * were used.
 */
DUK_LOCAL duk_int_t duk__parse_func_like_fnum(duk_compiler_ctx *comp_ctx, duk_small_uint_t flags) {
	duk_hthread *thr = comp_ctx->thr;
	duk_compiler_func old_func;
	duk_idx_t entry_top;
	duk_int_t fnum;

	/*
	 *  On second pass, skip the function.
	 */

	if (!comp_ctx->curr_func.in_scanning) {
		duk_lexer_point lex_pt;

		fnum = comp_ctx->curr_func.fnum_next++;
		duk_get_prop_index(thr, comp_ctx->curr_func.funcs_idx, (duk_uarridx_t) (fnum * 3 + 1));
		lex_pt.offset = (duk_size_t) duk_to_uint(thr, -1);
		duk_pop(thr);
		duk_get_prop_index(thr, comp_ctx->curr_func.funcs_idx, (duk_uarridx_t) (fnum * 3 + 2));
		lex_pt.line = duk_to_int(thr, -1);
		duk_pop(thr);

		DUK_DDD(
		    DUK_DDDPRINT("second pass of an inner func, skip the function, reparse closing brace; lex offset=%ld, line=%ld",
		                 (long) lex_pt.offset,
		                 (long) lex_pt.line));

		DUK_LEXER_SETPOINT(&comp_ctx->lex, &lex_pt);
		comp_ctx->curr_token.t = 0; /* this is needed for regexp mode */
		comp_ctx->curr_token.start_line = 0; /* needed for line number tracking (becomes prev_token.start_line) */
		duk__advance(comp_ctx);

		/* RegExp is not allowed after a function expression, e.g. in
		 * (function () {} / 123).  A RegExp *is* allowed after a
		 * function declaration!
		 */
		if (flags & DUK__FUNC_FLAG_DECL) {
			comp_ctx->curr_func.allow_regexp_in_adv = 1;
		}
		duk__advance_expect(comp_ctx, DUK_TOK_RCURLY);

		return fnum;
	}

	/*
	 *  On first pass, perform actual parsing.  Remember valstack top on entry
	 *  to restore it later, and switch to using a new function in comp_ctx.
	 */

	entry_top = duk_get_top(thr);
	DUK_DDD(DUK_DDDPRINT("before func: entry_top=%ld, curr_tok.start_offset=%ld",
	                     (long) entry_top,
	                     (long) comp_ctx->curr_token.start_offset));

	duk_memcpy(&old_func, &comp_ctx->curr_func, sizeof(duk_compiler_func));

	duk_memzero(&comp_ctx->curr_func, sizeof(duk_compiler_func));
	duk__init_func_valstack_slots(comp_ctx);
	DUK_ASSERT(comp_ctx->curr_func.num_formals == 0);

	/* inherit initial strictness from parent */
	comp_ctx->curr_func.is_strict = old_func.is_strict;

	/* XXX: It might be better to just store the flags into the curr_func
	 * struct and use them as is without this flag interpretation step
	 * here.
	 */
	DUK_ASSERT(comp_ctx->curr_func.is_notail == 0);
	comp_ctx->curr_func.is_function = 1;
	DUK_ASSERT(comp_ctx->curr_func.is_eval == 0);
	DUK_ASSERT(comp_ctx->curr_func.is_global == 0);
	comp_ctx->curr_func.is_setget = ((flags & DUK__FUNC_FLAG_GETSET) != 0);
	comp_ctx->curr_func.is_namebinding =
	    !(flags & (DUK__FUNC_FLAG_GETSET | DUK__FUNC_FLAG_METDEF |
	               DUK__FUNC_FLAG_DECL)); /* no name binding for: declarations, objlit getset, objlit method def */
	comp_ctx->curr_func.is_constructable =
	    !(flags & (DUK__FUNC_FLAG_GETSET | DUK__FUNC_FLAG_METDEF)); /* not constructable: objlit getset, objlit method def */

	/*
	 *  Parse inner function
	 */

	duk__parse_func_like_raw(comp_ctx, flags); /* pushes function template */

	/* prev_token.start_offset points to the closing brace here; when skipping
	 * we're going to reparse the closing brace to ensure semicolon insertion
	 * etc work as expected.
	 */
	DUK_DDD(DUK_DDDPRINT("after func: prev_tok.start_offset=%ld, curr_tok.start_offset=%ld",
	                     (long) comp_ctx->prev_token.start_offset,
	                     (long) comp_ctx->curr_token.start_offset));
	DUK_ASSERT(comp_ctx->lex.input[comp_ctx->prev_token.start_offset] == (duk_uint8_t) DUK_ASC_RCURLY);

	/* XXX: append primitive */
	DUK_ASSERT(duk_get_length(thr, old_func.funcs_idx) == (duk_size_t) (old_func.fnum_next * 3));
	fnum = old_func.fnum_next++;

	if (fnum > DUK__MAX_FUNCS) {
		DUK_ERROR_RANGE(comp_ctx->thr, DUK_STR_FUNC_LIMIT);
		DUK_WO_NORETURN(return 0;);
	}

	/* array writes autoincrement length */
	(void) duk_put_prop_index(thr, old_func.funcs_idx, (duk_uarridx_t) (fnum * 3));
	duk_push_size_t(thr, comp_ctx->prev_token.start_offset);
	(void) duk_put_prop_index(thr, old_func.funcs_idx, (duk_uarridx_t) (fnum * 3 + 1));
	duk_push_int(thr, comp_ctx->prev_token.start_line);
	(void) duk_put_prop_index(thr, old_func.funcs_idx, (duk_uarridx_t) (fnum * 3 + 2));

	/*
	 *  Cleanup: restore original function, restore valstack state.
	 *
	 *  Function declaration handling needs the function name to be pushed
	 *  on the value stack.
	 */

	if (flags & DUK__FUNC_FLAG_PUSHNAME_PASS1) {
		DUK_ASSERT(comp_ctx->curr_func.h_name != NULL);
		duk_push_hstring(thr, comp_ctx->curr_func.h_name);
		duk_replace(thr, entry_top);
		duk_set_top(thr, entry_top + 1);
	} else {
		duk_set_top(thr, entry_top);
	}
	duk_memcpy((void *) &comp_ctx->curr_func, (void *) &old_func, sizeof(duk_compiler_func));

	return fnum;
}

/*
 *  Compile input string into an executable function template without
 *  arguments.
 *
 *  The string is parsed as the "Program" production of ECMAScript E5.
 *  Compilation context can be either global code or eval code (see E5
 *  Sections 14 and 15.1.2.1).
 *
 *  Input stack:  [ ... filename ]
 *  Output stack: [ ... func_template ]
 */

/* XXX: source code property */

DUK_LOCAL duk_ret_t duk__js_compile_raw(duk_hthread *thr, void *udata) {
	duk_hstring *h_filename;
	duk__compiler_stkstate *comp_stk;
	duk_compiler_ctx *comp_ctx;
	duk_lexer_point *lex_pt;
	duk_compiler_func *func;
	duk_idx_t entry_top;
	duk_bool_t is_strict;
	duk_bool_t is_eval;
	duk_bool_t is_funcexpr;
	duk_small_uint_t flags;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(udata != NULL);

	/*
	 *  Arguments check
	 */

	entry_top = duk_get_top(thr);
	DUK_ASSERT(entry_top >= 1);

	comp_stk = (duk__compiler_stkstate *) udata;
	comp_ctx = &comp_stk->comp_ctx_alloc;
	lex_pt = &comp_stk->lex_pt_alloc;
	DUK_ASSERT(comp_ctx != NULL);
	DUK_ASSERT(lex_pt != NULL);

	flags = comp_stk->flags;
	is_eval = (flags & DUK_COMPILE_EVAL ? 1 : 0);
	is_strict = (flags & DUK_COMPILE_STRICT ? 1 : 0);
	is_funcexpr = (flags & DUK_COMPILE_FUNCEXPR ? 1 : 0);

	h_filename = duk_get_hstring(thr, -1); /* may be undefined */

	/*
	 *  Init compiler and lexer contexts
	 */

	func = &comp_ctx->curr_func;
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	comp_ctx->thr = NULL;
	comp_ctx->h_filename = NULL;
	comp_ctx->prev_token.str1 = NULL;
	comp_ctx->prev_token.str2 = NULL;
	comp_ctx->curr_token.str1 = NULL;
	comp_ctx->curr_token.str2 = NULL;
#endif

	duk_require_stack(thr, DUK__COMPILE_ENTRY_SLOTS);

	duk_push_dynamic_buffer(thr, 0); /* entry_top + 0 */
	duk_push_undefined(thr); /* entry_top + 1 */
	duk_push_undefined(thr); /* entry_top + 2 */
	duk_push_undefined(thr); /* entry_top + 3 */
	duk_push_undefined(thr); /* entry_top + 4 */

	comp_ctx->thr = thr;
	comp_ctx->h_filename = h_filename;
	comp_ctx->tok11_idx = entry_top + 1;
	comp_ctx->tok12_idx = entry_top + 2;
	comp_ctx->tok21_idx = entry_top + 3;
	comp_ctx->tok22_idx = entry_top + 4;
	comp_ctx->recursion_limit = DUK_USE_COMPILER_RECLIMIT;

	/* comp_ctx->lex has been pre-initialized by caller: it has been
	 * zeroed and input/input_length has been set.
	 */
	comp_ctx->lex.thr = thr;
	/* comp_ctx->lex.input and comp_ctx->lex.input_length filled by caller */
	comp_ctx->lex.slot1_idx = comp_ctx->tok11_idx;
	comp_ctx->lex.slot2_idx = comp_ctx->tok12_idx;
	comp_ctx->lex.buf_idx = entry_top + 0;
	comp_ctx->lex.buf = (duk_hbuffer_dynamic *) duk_known_hbuffer(thr, entry_top + 0);
	DUK_ASSERT(DUK_HBUFFER_HAS_DYNAMIC(comp_ctx->lex.buf) && !DUK_HBUFFER_HAS_EXTERNAL(comp_ctx->lex.buf));
	comp_ctx->lex.token_limit = DUK_COMPILER_TOKEN_LIMIT;

	lex_pt->offset = 0;
	lex_pt->line = 1;
	DUK_LEXER_SETPOINT(&comp_ctx->lex, lex_pt); /* fills window */
	comp_ctx->curr_token.start_line = 0; /* needed for line number tracking (becomes prev_token.start_line) */

	/*
	 *  Initialize function state for a zero-argument function
	 */

	duk__init_func_valstack_slots(comp_ctx);
	DUK_ASSERT(func->num_formals == 0);

	if (is_funcexpr) {
		/* Name will be filled from function expression, not by caller.
		 * This case is used by Function constructor and duk_compile()
		 * API with the DUK_COMPILE_FUNCTION option.
		 */
		DUK_ASSERT(func->h_name == NULL);
	} else {
		duk_push_hstring_stridx(thr, (is_eval ? DUK_STRIDX_EVAL : DUK_STRIDX_GLOBAL));
		func->h_name = duk_get_hstring(thr, -1);
	}

	/*
	 *  Parse a function body or a function-like expression, depending
	 *  on flags.
	 */

	DUK_ASSERT(func->is_setget == 0);
	func->is_strict = (duk_uint8_t) is_strict;
	DUK_ASSERT(func->is_notail == 0);

	if (is_funcexpr) {
		func->is_function = 1;
		DUK_ASSERT(func->is_eval == 0);
		DUK_ASSERT(func->is_global == 0);
		func->is_namebinding = 1;
		func->is_constructable = 1;

		duk__advance(comp_ctx); /* init 'curr_token' */
		duk__advance_expect(comp_ctx, DUK_TOK_FUNCTION);
		(void) duk__parse_func_like_raw(comp_ctx, 0 /*flags*/);
	} else {
		DUK_ASSERT(func->is_function == 0);
		DUK_ASSERT(is_eval == 0 || is_eval == 1);
		func->is_eval = (duk_uint8_t) is_eval;
		func->is_global = (duk_uint8_t) !is_eval;
		DUK_ASSERT(func->is_namebinding == 0);
		DUK_ASSERT(func->is_constructable == 0);

		duk__parse_func_body(comp_ctx,
		                     1, /* expect_eof */
		                     1, /* implicit_return_value */
		                     1, /* regexp_after (does not matter) */
		                     -1); /* expect_token */
	}

	/*
	 *  Convert duk_compiler_func to a function template
	 */

	duk__convert_to_func_template(comp_ctx);

	/*
	 *  Wrapping duk_safe_call() will mangle the stack, just return stack top
	 */

	/* [ ... filename (temps) func ] */

	return 1;
}

DUK_INTERNAL void duk_js_compile(duk_hthread *thr, const duk_uint8_t *src_buffer, duk_size_t src_length, duk_small_uint_t flags) {
	duk__compiler_stkstate comp_stk;
	duk_compiler_ctx *prev_ctx;
	duk_ret_t safe_rc;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(src_buffer != NULL);

	/* preinitialize lexer state partially */
	duk_memzero(&comp_stk, sizeof(comp_stk));
	comp_stk.flags = flags;
	DUK_LEXER_INITCTX(&comp_stk.comp_ctx_alloc.lex);
	comp_stk.comp_ctx_alloc.lex.input = src_buffer;
	comp_stk.comp_ctx_alloc.lex.input_length = src_length;
	comp_stk.comp_ctx_alloc.lex.flags = flags; /* Forward flags directly for now. */

	/* [ ... filename ] */

	prev_ctx = thr->compile_ctx;
	thr->compile_ctx = &comp_stk.comp_ctx_alloc; /* for duk_error_augment.c */
	safe_rc = duk_safe_call(thr, duk__js_compile_raw, (void *) &comp_stk /*udata*/, 1 /*nargs*/, 1 /*nrets*/);
	thr->compile_ctx = prev_ctx; /* must restore reliably before returning */

	if (safe_rc != DUK_EXEC_SUCCESS) {
		DUK_D(DUK_DPRINT("compilation failed: %!T", duk_get_tval(thr, -1)));
		(void) duk_throw(thr);
		DUK_WO_NORETURN(return;);
	}

	/* [ ... template ] */
}
