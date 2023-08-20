/*
 *  Regular expression structs, constants, and bytecode defines.
 */

#if !defined(DUK_REGEXP_H_INCLUDED)
#define DUK_REGEXP_H_INCLUDED

/* maximum bytecode copies for {n,m} quantifiers */
#define DUK_RE_MAX_ATOM_COPIES 1000

/* regexp compilation limits */
#define DUK_RE_COMPILE_TOKEN_LIMIT 100000000L /* 1e8 */

/* regexp execution limits */
#define DUK_RE_EXECUTE_STEPS_LIMIT 1000000000L /* 1e9 */

/* regexp opcodes */
#define DUK_REOP_MATCH                    1
#define DUK_REOP_CHAR                     2
#define DUK_REOP_PERIOD                   3
#define DUK_REOP_RANGES                   4
#define DUK_REOP_INVRANGES                5
#define DUK_REOP_JUMP                     6
#define DUK_REOP_SPLIT1                   7
#define DUK_REOP_SPLIT2                   8
#define DUK_REOP_SQMINIMAL                9
#define DUK_REOP_SQGREEDY                 10
#define DUK_REOP_SAVE                     11
#define DUK_REOP_WIPERANGE                12
#define DUK_REOP_LOOKPOS                  13
#define DUK_REOP_LOOKNEG                  14
#define DUK_REOP_BACKREFERENCE            15
#define DUK_REOP_ASSERT_START             16
#define DUK_REOP_ASSERT_END               17
#define DUK_REOP_ASSERT_WORD_BOUNDARY     18
#define DUK_REOP_ASSERT_NOT_WORD_BOUNDARY 19

/* flags */
#define DUK_RE_FLAG_GLOBAL      (1U << 0)
#define DUK_RE_FLAG_IGNORE_CASE (1U << 1)
#define DUK_RE_FLAG_MULTILINE   (1U << 2)

struct duk_re_matcher_ctx {
	duk_hthread *thr;

	duk_uint32_t re_flags;
	const duk_uint8_t *input;
	const duk_uint8_t *input_end;
	const duk_uint8_t *bytecode;
	const duk_uint8_t *bytecode_end;
	const duk_uint8_t **saved; /* allocated from valstack (fixed buffer) */
	duk_uint32_t nsaved;
	duk_uint32_t recursion_depth;
	duk_uint32_t recursion_limit;
	duk_uint32_t steps_count;
	duk_uint32_t steps_limit;
};

struct duk_re_compiler_ctx {
	duk_hthread *thr;

	duk_uint32_t re_flags;
	duk_lexer_ctx lex;
	duk_re_token curr_token;
	duk_bufwriter_ctx bw;
	duk_uint32_t captures; /* highest capture number emitted so far (used as: ++captures) */
	duk_uint32_t highest_backref;
	duk_uint32_t recursion_depth;
	duk_uint32_t recursion_limit;
	duk_uint32_t nranges; /* internal temporary value, used for char classes */
};

/*
 *  Prototypes
 */

#if defined(DUK_USE_REGEXP_SUPPORT)
DUK_INTERNAL_DECL void duk_regexp_compile(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_regexp_create_instance(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_regexp_match(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_regexp_match_force_global(duk_hthread *thr); /* hacky helper for String.prototype.split() */
#endif

#endif /* DUK_REGEXP_H_INCLUDED */
