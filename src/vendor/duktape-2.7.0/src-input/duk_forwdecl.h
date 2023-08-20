/*
 *  Forward declarations for all Duktape structures.
 */

#if !defined(DUK_FORWDECL_H_INCLUDED)
#define DUK_FORWDECL_H_INCLUDED

/*
 *  Forward declarations
 */

#if defined(DUK_USE_CPP_EXCEPTIONS)
class duk_internal_exception;
#else
struct duk_jmpbuf;
#endif

/* duk_tval intentionally skipped */
struct duk_heaphdr;
struct duk_heaphdr_string;
struct duk_harray;
struct duk_hstring;
struct duk_hstring_external;
struct duk_hobject;
struct duk_hcompfunc;
struct duk_hnatfunc;
struct duk_hboundfunc;
struct duk_hthread;
struct duk_hbufobj;
struct duk_hdecenv;
struct duk_hobjenv;
struct duk_hproxy;
struct duk_hbuffer;
struct duk_hbuffer_fixed;
struct duk_hbuffer_dynamic;
struct duk_hbuffer_external;

struct duk_propaccessor;
union duk_propvalue;
struct duk_propdesc;

struct duk_heap;
struct duk_breakpoint;

struct duk_activation;
struct duk_catcher;
struct duk_ljstate;
struct duk_strcache_entry;
struct duk_litcache_entry;
struct duk_strtab_entry;

#if defined(DUK_USE_DEBUG)
struct duk_fixedbuffer;
#endif

struct duk_bitdecoder_ctx;
struct duk_bitencoder_ctx;
struct duk_bufwriter_ctx;

struct duk_token;
struct duk_re_token;
struct duk_lexer_point;
struct duk_lexer_ctx;
struct duk_lexer_codepoint;

struct duk_compiler_instr;
struct duk_compiler_func;
struct duk_compiler_ctx;

struct duk_re_matcher_ctx;
struct duk_re_compiler_ctx;

#if defined(DUK_USE_CPP_EXCEPTIONS)
/* no typedef */
#else
typedef struct duk_jmpbuf duk_jmpbuf;
#endif

/* duk_tval intentionally skipped */
typedef struct duk_heaphdr duk_heaphdr;
typedef struct duk_heaphdr_string duk_heaphdr_string;
typedef struct duk_harray duk_harray;
typedef struct duk_hstring duk_hstring;
typedef struct duk_hstring_external duk_hstring_external;
typedef struct duk_hobject duk_hobject;
typedef struct duk_hcompfunc duk_hcompfunc;
typedef struct duk_hnatfunc duk_hnatfunc;
typedef struct duk_hboundfunc duk_hboundfunc;
typedef struct duk_hthread duk_hthread;
typedef struct duk_hbufobj duk_hbufobj;
typedef struct duk_hdecenv duk_hdecenv;
typedef struct duk_hobjenv duk_hobjenv;
typedef struct duk_hproxy duk_hproxy;
typedef struct duk_hbuffer duk_hbuffer;
typedef struct duk_hbuffer_fixed duk_hbuffer_fixed;
typedef struct duk_hbuffer_dynamic duk_hbuffer_dynamic;
typedef struct duk_hbuffer_external duk_hbuffer_external;

typedef struct duk_propaccessor duk_propaccessor;
typedef union duk_propvalue duk_propvalue;
typedef struct duk_propdesc duk_propdesc;

typedef struct duk_heap duk_heap;
typedef struct duk_breakpoint duk_breakpoint;

typedef struct duk_activation duk_activation;
typedef struct duk_catcher duk_catcher;
typedef struct duk_ljstate duk_ljstate;
typedef struct duk_strcache_entry duk_strcache_entry;
typedef struct duk_litcache_entry duk_litcache_entry;
typedef struct duk_strtab_entry duk_strtab_entry;

#if defined(DUK_USE_DEBUG)
typedef struct duk_fixedbuffer duk_fixedbuffer;
#endif

typedef struct duk_bitdecoder_ctx duk_bitdecoder_ctx;
typedef struct duk_bitencoder_ctx duk_bitencoder_ctx;
typedef struct duk_bufwriter_ctx duk_bufwriter_ctx;

typedef struct duk_token duk_token;
typedef struct duk_re_token duk_re_token;
typedef struct duk_lexer_point duk_lexer_point;
typedef struct duk_lexer_ctx duk_lexer_ctx;
typedef struct duk_lexer_codepoint duk_lexer_codepoint;

typedef struct duk_compiler_instr duk_compiler_instr;
typedef struct duk_compiler_func duk_compiler_func;
typedef struct duk_compiler_ctx duk_compiler_ctx;

typedef struct duk_re_matcher_ctx duk_re_matcher_ctx;
typedef struct duk_re_compiler_ctx duk_re_compiler_ctx;

#endif /* DUK_FORWDECL_H_INCLUDED */
