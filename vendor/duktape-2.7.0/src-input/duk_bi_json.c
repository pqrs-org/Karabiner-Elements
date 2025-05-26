/*
 *  JSON built-ins.
 *
 *  See doc/json.rst.
 *
 *  Codepoints are handled as duk_uint_fast32_t to ensure that the full
 *  unsigned 32-bit range is supported.  This matters to e.g. JX.
 *
 *  Input parsing doesn't do an explicit end-of-input check at all.  This is
 *  safe: input string data is always NUL-terminated (0x00) and valid JSON
 *  inputs never contain plain NUL characters, so that as long as syntax checks
 *  are correct, we'll never read past the NUL.  This approach reduces code size
 *  and improves parsing performance, but it's critical that syntax checks are
 *  indeed correct!
 */

#include "duk_internal.h"

#if defined(DUK_USE_JSON_SUPPORT)

/*
 *  Local defines and forward declarations.
 */

#define DUK__JSON_DECSTR_BUFSIZE    128
#define DUK__JSON_DECSTR_CHUNKSIZE  64
#define DUK__JSON_ENCSTR_CHUNKSIZE  64
#define DUK__JSON_STRINGIFY_BUFSIZE 128
#define DUK__JSON_MAX_ESC_LEN       10 /* '\Udeadbeef' */

DUK_LOCAL_DECL void duk__json_dec_syntax_error(duk_json_dec_ctx *js_ctx);
DUK_LOCAL_DECL void duk__json_dec_eat_white(duk_json_dec_ctx *js_ctx);
#if defined(DUK_USE_JX)
DUK_LOCAL_DECL duk_uint8_t duk__json_dec_peek(duk_json_dec_ctx *js_ctx);
#endif
DUK_LOCAL_DECL duk_uint8_t duk__json_dec_get(duk_json_dec_ctx *js_ctx);
DUK_LOCAL_DECL duk_uint8_t duk__json_dec_get_nonwhite(duk_json_dec_ctx *js_ctx);
DUK_LOCAL_DECL duk_uint_fast32_t duk__json_dec_decode_hex_escape(duk_json_dec_ctx *js_ctx, duk_small_uint_t n);
DUK_LOCAL_DECL void duk__json_dec_req_stridx(duk_json_dec_ctx *js_ctx, duk_small_uint_t stridx);
DUK_LOCAL_DECL void duk__json_dec_string(duk_json_dec_ctx *js_ctx);
#if defined(DUK_USE_JX)
DUK_LOCAL_DECL void duk__json_dec_plain_string(duk_json_dec_ctx *js_ctx);
DUK_LOCAL_DECL void duk__json_dec_pointer(duk_json_dec_ctx *js_ctx);
DUK_LOCAL_DECL void duk__json_dec_buffer(duk_json_dec_ctx *js_ctx);
#endif
DUK_LOCAL_DECL void duk__json_dec_number(duk_json_dec_ctx *js_ctx);
DUK_LOCAL_DECL void duk__json_dec_objarr_entry(duk_json_dec_ctx *js_ctx);
DUK_LOCAL_DECL void duk__json_dec_objarr_exit(duk_json_dec_ctx *js_ctx);
DUK_LOCAL_DECL void duk__json_dec_object(duk_json_dec_ctx *js_ctx);
DUK_LOCAL_DECL void duk__json_dec_array(duk_json_dec_ctx *js_ctx);
DUK_LOCAL_DECL void duk__json_dec_value(duk_json_dec_ctx *js_ctx);
DUK_LOCAL_DECL void duk__json_dec_reviver_walk(duk_json_dec_ctx *js_ctx);

DUK_LOCAL_DECL void duk__emit_1(duk_json_enc_ctx *js_ctx, duk_uint_fast8_t ch);
DUK_LOCAL_DECL void duk__emit_2(duk_json_enc_ctx *js_ctx, duk_uint_fast8_t ch1, duk_uint_fast8_t ch2);
DUK_LOCAL_DECL void duk__unemit_1(duk_json_enc_ctx *js_ctx);
DUK_LOCAL_DECL void duk__emit_hstring(duk_json_enc_ctx *js_ctx, duk_hstring *h);
#if defined(DUK_USE_FASTINT)
DUK_LOCAL_DECL void duk__emit_cstring(duk_json_enc_ctx *js_ctx, const char *p);
#endif
DUK_LOCAL_DECL void duk__emit_stridx(duk_json_enc_ctx *js_ctx, duk_small_uint_t stridx);
DUK_LOCAL_DECL duk_uint8_t *duk__emit_esc_auto_fast(duk_json_enc_ctx *js_ctx, duk_uint_fast32_t cp, duk_uint8_t *q);
DUK_LOCAL_DECL void duk__json_enc_key_autoquote(duk_json_enc_ctx *js_ctx, duk_hstring *k);
DUK_LOCAL_DECL void duk__json_enc_quote_string(duk_json_enc_ctx *js_ctx, duk_hstring *h_str);
DUK_LOCAL_DECL void duk__json_enc_objarr_entry(duk_json_enc_ctx *js_ctx, duk_idx_t *entry_top);
DUK_LOCAL_DECL void duk__json_enc_objarr_exit(duk_json_enc_ctx *js_ctx, duk_idx_t *entry_top);
DUK_LOCAL_DECL void duk__json_enc_object(duk_json_enc_ctx *js_ctx);
DUK_LOCAL_DECL void duk__json_enc_array(duk_json_enc_ctx *js_ctx);
DUK_LOCAL_DECL duk_bool_t duk__json_enc_value(duk_json_enc_ctx *js_ctx, duk_idx_t idx_holder);
DUK_LOCAL_DECL duk_bool_t duk__json_enc_allow_into_proplist(duk_tval *tv);
DUK_LOCAL_DECL void duk__json_enc_double(duk_json_enc_ctx *js_ctx);
#if defined(DUK_USE_FASTINT)
DUK_LOCAL_DECL void duk__json_enc_fastint_tval(duk_json_enc_ctx *js_ctx, duk_tval *tv);
#endif
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
DUK_LOCAL_DECL void duk__json_enc_buffer_jx_jc(duk_json_enc_ctx *js_ctx, duk_hbuffer *h);
DUK_LOCAL_DECL void duk__json_enc_pointer(duk_json_enc_ctx *js_ctx, void *ptr);
#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_LOCAL_DECL void duk__json_enc_bufobj(duk_json_enc_ctx *js_ctx, duk_hbufobj *h_bufobj);
#endif
#endif
#if defined(DUK_USE_JSON_STRINGIFY_FASTPATH)
DUK_LOCAL_DECL void duk__json_enc_buffer_json_fastpath(duk_json_enc_ctx *js_ctx, duk_hbuffer *h);
#endif
DUK_LOCAL_DECL void duk__json_enc_newline_indent(duk_json_enc_ctx *js_ctx, duk_uint_t depth);

/*
 *  Helper tables
 */

#if defined(DUK_USE_JSON_QUOTESTRING_FASTPATH)
DUK_LOCAL const duk_uint8_t duk__json_quotestr_lookup[256] = {
	/* 0x00 ... 0x7f: as is
	 * 0x80: escape generically
	 * 0x81: slow path
	 * 0xa0 ... 0xff: backslash + one char
	 */

	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xe2, 0xf4, 0xee, 0x80, 0xe6, 0xf2, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x20, 0x21, 0xa2, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
	0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0xdc, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63,
	0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
	0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
	0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
	0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
	0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
	0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
	0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81
};
#else /* DUK_USE_JSON_QUOTESTRING_FASTPATH */
DUK_LOCAL const duk_uint8_t duk__json_quotestr_esc[14] = { DUK_ASC_NUL,  DUK_ASC_NUL, DUK_ASC_NUL,  DUK_ASC_NUL,  DUK_ASC_NUL,
	                                                   DUK_ASC_NUL,  DUK_ASC_NUL, DUK_ASC_NUL,  DUK_ASC_LC_B, DUK_ASC_LC_T,
	                                                   DUK_ASC_LC_N, DUK_ASC_NUL, DUK_ASC_LC_F, DUK_ASC_LC_R };
#endif /* DUK_USE_JSON_QUOTESTRING_FASTPATH */

#if defined(DUK_USE_JSON_DECSTRING_FASTPATH)
DUK_LOCAL const duk_uint8_t duk__json_decstr_lookup[256] = {
	/* 0x00: slow path
	 * other: as is
	 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x21, 0x00, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
	0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x00, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63,
	0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b,
	0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
	0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb,
	0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
#endif /* DUK_USE_JSON_DECSTRING_FASTPATH */

#if defined(DUK_USE_JSON_EATWHITE_FASTPATH)
DUK_LOCAL const duk_uint8_t duk__json_eatwhite_lookup[256] = {
	/* 0x00: finish (non-white)
	 * 0x01: continue
	 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#endif /* DUK_USE_JSON_EATWHITE_FASTPATH */

#if defined(DUK_USE_JSON_DECNUMBER_FASTPATH)
DUK_LOCAL const duk_uint8_t duk__json_decnumber_lookup[256] = {
	/* 0x00: finish (not part of number)
	 * 0x01: continue
	 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#endif /* DUK_USE_JSON_DECNUMBER_FASTPATH */

/*
 *  Parsing implementation.
 *
 *  JSON lexer is now separate from duk_lexer.c because there are numerous
 *  small differences making it difficult to share the lexer.
 *
 *  The parser here works with raw bytes directly; this works because all
 *  JSON delimiters are ASCII characters.  Invalid xUTF-8 encoded values
 *  inside strings will be passed on without normalization; this is not a
 *  compliance concern because compliant inputs will always be valid
 *  CESU-8 encodings.
 */

DUK_LOCAL void duk__json_dec_syntax_error(duk_json_dec_ctx *js_ctx) {
	/* Shared handler to minimize parser size.  Cause will be
	 * hidden, unfortunately, but we'll have an offset which
	 * is often quite enough.
	 */
	DUK_ERROR_FMT1(js_ctx->thr, DUK_ERR_SYNTAX_ERROR, DUK_STR_FMT_INVALID_JSON, (long) (js_ctx->p - js_ctx->p_start));
	DUK_WO_NORETURN(return;);
}

DUK_LOCAL void duk__json_dec_eat_white(duk_json_dec_ctx *js_ctx) {
	const duk_uint8_t *p;
	duk_uint8_t t;

	p = js_ctx->p;
	for (;;) {
		DUK_ASSERT(p <= js_ctx->p_end);
		t = *p;

#if defined(DUK_USE_JSON_EATWHITE_FASTPATH)
		/* This fast path is pretty marginal in practice.
		 * XXX: candidate for removal.
		 */
		DUK_ASSERT(duk__json_eatwhite_lookup[0x00] == 0x00); /* end-of-input breaks */
		if (duk__json_eatwhite_lookup[t] == 0) {
			break;
		}
#else /* DUK_USE_JSON_EATWHITE_FASTPATH */
		if (!(t == 0x20 || t == 0x0a || t == 0x0d || t == 0x09)) {
			/* NUL also comes here.  Comparison order matters, 0x20
			 * is most common whitespace.
			 */
			break;
		}
#endif /* DUK_USE_JSON_EATWHITE_FASTPATH */
		p++;
	}
	js_ctx->p = p;
}

#if defined(DUK_USE_JX)
DUK_LOCAL duk_uint8_t duk__json_dec_peek(duk_json_dec_ctx *js_ctx) {
	DUK_ASSERT(js_ctx->p <= js_ctx->p_end);
	return *js_ctx->p;
}
#endif

DUK_LOCAL duk_uint8_t duk__json_dec_get(duk_json_dec_ctx *js_ctx) {
	DUK_ASSERT(js_ctx->p <= js_ctx->p_end);
	return *js_ctx->p++;
}

DUK_LOCAL duk_uint8_t duk__json_dec_get_nonwhite(duk_json_dec_ctx *js_ctx) {
	duk__json_dec_eat_white(js_ctx);
	return duk__json_dec_get(js_ctx);
}

/* For JX, expressing the whole unsigned 32-bit range matters. */
DUK_LOCAL duk_uint_fast32_t duk__json_dec_decode_hex_escape(duk_json_dec_ctx *js_ctx, duk_small_uint_t n) {
	duk_small_uint_t i;
	duk_uint_fast32_t res = 0;
	duk_uint8_t x;
	duk_small_int_t t;

	for (i = 0; i < n; i++) {
		/* XXX: share helper from lexer; duk_lexer.c / hexval(). */

		x = duk__json_dec_get(js_ctx);
		DUK_DDD(DUK_DDDPRINT("decode_hex_escape: i=%ld, n=%ld, res=%ld, x=%ld", (long) i, (long) n, (long) res, (long) x));

		/* x == 0x00 (EOF) causes syntax_error */
		DUK_ASSERT(duk_hex_dectab[0] == -1);
		t = duk_hex_dectab[x & 0xff];
		if (DUK_LIKELY(t >= 0)) {
			res = (res * 16) + (duk_uint_fast32_t) t;
		} else {
			/* catches EOF and invalid digits */
			goto syntax_error;
		}
	}

	DUK_DDD(DUK_DDDPRINT("final hex decoded value: %ld", (long) res));
	return res;

syntax_error:
	duk__json_dec_syntax_error(js_ctx);
	DUK_UNREACHABLE();
	return 0;
}

DUK_LOCAL void duk__json_dec_req_stridx(duk_json_dec_ctx *js_ctx, duk_small_uint_t stridx) {
	duk_hstring *h;
	const duk_uint8_t *p;
	duk_uint8_t x, y;

	/* First character has already been eaten and checked by the caller.
	 * We can scan until a NUL in stridx string because no built-in strings
	 * have internal NULs.
	 */

	DUK_ASSERT_STRIDX_VALID(stridx);
	h = DUK_HTHREAD_GET_STRING(js_ctx->thr, stridx);
	DUK_ASSERT(h != NULL);

	p = (const duk_uint8_t *) DUK_HSTRING_GET_DATA(h) + 1;
	DUK_ASSERT(*(js_ctx->p - 1) == *(p - 1)); /* first character has been matched */

	for (;;) {
		x = *p;
		if (x == 0) {
			break;
		}
		y = duk__json_dec_get(js_ctx);
		if (x != y) {
			/* Catches EOF of JSON input. */
			goto syntax_error;
		}
		p++;
	}

	return;

syntax_error:
	duk__json_dec_syntax_error(js_ctx);
	DUK_UNREACHABLE();
}

DUK_LOCAL duk_small_int_t duk__json_dec_string_escape(duk_json_dec_ctx *js_ctx, duk_uint8_t **ext_p) {
	duk_uint_fast32_t cp;

	/* EOF (-1) will be cast to an unsigned value first
	 * and then re-cast for the switch.  In any case, it
	 * will match the default case (syntax error).
	 */
	cp = (duk_uint_fast32_t) duk__json_dec_get(js_ctx);
	switch (cp) {
	case DUK_ASC_BACKSLASH:
		break;
	case DUK_ASC_DOUBLEQUOTE:
		break;
	case DUK_ASC_SLASH:
		break;
	case DUK_ASC_LC_T:
		cp = 0x09;
		break;
	case DUK_ASC_LC_N:
		cp = 0x0a;
		break;
	case DUK_ASC_LC_R:
		cp = 0x0d;
		break;
	case DUK_ASC_LC_F:
		cp = 0x0c;
		break;
	case DUK_ASC_LC_B:
		cp = 0x08;
		break;
	case DUK_ASC_LC_U: {
		cp = duk__json_dec_decode_hex_escape(js_ctx, 4);
		break;
	}
#if defined(DUK_USE_JX)
	case DUK_ASC_UC_U: {
		if (js_ctx->flag_ext_custom) {
			cp = duk__json_dec_decode_hex_escape(js_ctx, 8);
		} else {
			return 1; /* syntax error */
		}
		break;
	}
	case DUK_ASC_LC_X: {
		if (js_ctx->flag_ext_custom) {
			cp = duk__json_dec_decode_hex_escape(js_ctx, 2);
		} else {
			return 1; /* syntax error */
		}
		break;
	}
#endif /* DUK_USE_JX */
	default:
		/* catches EOF (0x00) */
		return 1; /* syntax error */
	}

	DUK_RAW_WRITEINC_XUTF8(*ext_p, cp);

	return 0;
}

DUK_LOCAL void duk__json_dec_string(duk_json_dec_ctx *js_ctx) {
	duk_hthread *thr = js_ctx->thr;
	duk_bufwriter_ctx bw_alloc;
	duk_bufwriter_ctx *bw;
	duk_uint8_t *q;

	/* '"' was eaten by caller */

	/* Note that we currently parse -bytes-, not codepoints.
	 * All non-ASCII extended UTF-8 will encode to bytes >= 0x80,
	 * so they'll simply pass through (valid UTF-8 or not).
	 */

	bw = &bw_alloc;
	DUK_BW_INIT_PUSHBUF(js_ctx->thr, bw, DUK__JSON_DECSTR_BUFSIZE);
	q = DUK_BW_GET_PTR(js_ctx->thr, bw);

#if defined(DUK_USE_JSON_DECSTRING_FASTPATH)
	for (;;) {
		duk_small_uint_t safe;
		duk_uint8_t b, x;
		const duk_uint8_t *p;

		/* Select a safe loop count where no output checks are
		 * needed assuming we won't encounter escapes.  Input
		 * bound checks are not necessary as a NUL (guaranteed)
		 * will cause a SyntaxError before we read out of bounds.
		 */

		safe = DUK__JSON_DECSTR_CHUNKSIZE;

		/* Ensure space for 1:1 output plus one escape. */
		q = DUK_BW_ENSURE_RAW(js_ctx->thr, bw, safe + DUK_UNICODE_MAX_XUTF8_LENGTH, q);

		p = js_ctx->p; /* temp copy, write back for next loop */
		for (;;) {
			if (safe == 0) {
				js_ctx->p = p;
				break;
			}
			safe--;

			/* End of input (NUL) goes through slow path and causes SyntaxError. */
			DUK_ASSERT(duk__json_decstr_lookup[0] == 0x00);

			b = *p++;
			x = (duk_small_int_t) duk__json_decstr_lookup[b];
			if (DUK_LIKELY(x != 0)) {
				/* Fast path, decode as is. */
				*q++ = b;
			} else if (b == DUK_ASC_DOUBLEQUOTE) {
				js_ctx->p = p;
				goto found_quote;
			} else if (b == DUK_ASC_BACKSLASH) {
				/* We've ensured space for one escaped input; then
				 * bail out and recheck (this makes escape handling
				 * quite slow but it's uncommon).
				 */
				js_ctx->p = p;
				if (duk__json_dec_string_escape(js_ctx, &q) != 0) {
					goto syntax_error;
				}
				break;
			} else {
				js_ctx->p = p;
				goto syntax_error;
			}
		}
	}
found_quote:
#else /* DUK_USE_JSON_DECSTRING_FASTPATH */
	for (;;) {
		duk_uint8_t x;

		q = DUK_BW_ENSURE_RAW(js_ctx->thr, bw, DUK_UNICODE_MAX_XUTF8_LENGTH, q);

		x = duk__json_dec_get(js_ctx);

		if (x == DUK_ASC_DOUBLEQUOTE) {
			break;
		} else if (x == DUK_ASC_BACKSLASH) {
			if (duk__json_dec_string_escape(js_ctx, &q) != 0) {
				goto syntax_error;
			}
		} else if (x < 0x20) {
			/* catches EOF (NUL) */
			goto syntax_error;
		} else {
			*q++ = (duk_uint8_t) x;
		}
	}
#endif /* DUK_USE_JSON_DECSTRING_FASTPATH */

	DUK_BW_SETPTR_AND_COMPACT(js_ctx->thr, bw, q);
	(void) duk_buffer_to_string(thr, -1); /* Safe if input string is safe. */

	/* [ ... str ] */

	return;

syntax_error:
	duk__json_dec_syntax_error(js_ctx);
	DUK_UNREACHABLE();
}

#if defined(DUK_USE_JX)
/* Decode a plain string consisting entirely of identifier characters.
 * Used to parse plain keys (e.g. "foo: 123").
 */
DUK_LOCAL void duk__json_dec_plain_string(duk_json_dec_ctx *js_ctx) {
	duk_hthread *thr = js_ctx->thr;
	const duk_uint8_t *p;
	duk_small_int_t x;

	/* Caller has already eaten the first char so backtrack one byte. */

	js_ctx->p--; /* safe */
	p = js_ctx->p;

	/* Here again we parse bytes, and non-ASCII UTF-8 will cause end of
	 * parsing (which is correct except if there are non-shortest encodings).
	 * There is also no need to check explicitly for end of input buffer as
	 * the input is NUL padded and NUL will exit the parsing loop.
	 *
	 * Because no unescaping takes place, we can just scan to the end of the
	 * plain string and intern from the input buffer.
	 */

	for (;;) {
		x = *p;

		/* There is no need to check the first character specially here
		 * (i.e. reject digits): the caller only accepts valid initial
		 * characters and won't call us if the first character is a digit.
		 * This also ensures that the plain string won't be empty.
		 */

		if (!duk_unicode_is_identifier_part((duk_codepoint_t) x)) {
			break;
		}
		p++;
	}

	duk_push_lstring(thr, (const char *) js_ctx->p, (duk_size_t) (p - js_ctx->p));
	js_ctx->p = p;

	/* [ ... str ] */
}
#endif /* DUK_USE_JX */

#if defined(DUK_USE_JX)
DUK_LOCAL void duk__json_dec_pointer(duk_json_dec_ctx *js_ctx) {
	duk_hthread *thr = js_ctx->thr;
	const duk_uint8_t *p;
	duk_small_int_t x;
	void *voidptr;

	/* Caller has already eaten the first character ('(') which we don't need. */

	p = js_ctx->p;

	for (;;) {
		x = *p;

		/* Assume that the native representation never contains a closing
		 * parenthesis.
		 */

		if (x == DUK_ASC_RPAREN) {
			break;
		} else if (x <= 0) {
			/* NUL term or -1 (EOF), NUL check would suffice */
			goto syntax_error;
		}
		p++;
	}

	/* There is no need to NUL delimit the sscanf() call: trailing garbage is
	 * ignored and there is always a NUL terminator which will force an error
	 * if no error is encountered before it.  It's possible that the scan
	 * would scan further than between [js_ctx->p,p[ though and we'd advance
	 * by less than the scanned value.
	 *
	 * Because pointers are platform specific, a failure to scan a pointer
	 * results in a null pointer which is a better placeholder than a missing
	 * value or an error.
	 */

	voidptr = NULL;
	(void) DUK_SSCANF((const char *) js_ctx->p, DUK_STR_FMT_PTR, &voidptr);
	duk_push_pointer(thr, voidptr);
	js_ctx->p = p + 1; /* skip ')' */

	/* [ ... ptr ] */

	return;

syntax_error:
	duk__json_dec_syntax_error(js_ctx);
	DUK_UNREACHABLE();
}
#endif /* DUK_USE_JX */

#if defined(DUK_USE_JX)
DUK_LOCAL void duk__json_dec_buffer(duk_json_dec_ctx *js_ctx) {
	duk_hthread *thr = js_ctx->thr;
	const duk_uint8_t *p;
	duk_uint8_t *buf;
	duk_size_t src_len;
	duk_small_int_t x;

	/* Caller has already eaten the first character ('|') which we don't need. */

	p = js_ctx->p;

	/* XXX: Would be nice to share the fast path loop from duk_hex_decode()
	 * and avoid creating a temporary buffer.  However, there are some
	 * differences which prevent trivial sharing:
	 *
	 *   - Pipe char detection
	 *   - EOF detection
	 *   - Unknown length of input and output
	 *
	 * The best approach here would be a bufwriter and a reasonaly sized
	 * safe inner loop (e.g. 64 output bytes at a time).
	 */

	for (;;) {
		x = *p;

		/* This loop intentionally does not ensure characters are valid
		 * ([0-9a-fA-F]) because the hex decode call below will do that.
		 */
		if (x == DUK_ASC_PIPE) {
			break;
		} else if (x <= 0) {
			/* NUL term or -1 (EOF), NUL check would suffice */
			goto syntax_error;
		}
		p++;
	}

	/* XXX: this is not very nice; unnecessary copy is made. */
	src_len = (duk_size_t) (p - js_ctx->p);
	buf = (duk_uint8_t *) duk_push_fixed_buffer_nozero(thr, src_len);
	DUK_ASSERT(buf != NULL);
	duk_memcpy((void *) buf, (const void *) js_ctx->p, src_len);
	duk_hex_decode(thr, -1);

	js_ctx->p = p + 1; /* skip '|' */

	/* [ ... buf ] */

	return;

syntax_error:
	duk__json_dec_syntax_error(js_ctx);
	DUK_UNREACHABLE();
}
#endif /* DUK_USE_JX */

/* Parse a number, other than NaN or +/- Infinity */
DUK_LOCAL void duk__json_dec_number(duk_json_dec_ctx *js_ctx) {
	duk_hthread *thr = js_ctx->thr;
	const duk_uint8_t *p_start;
	const duk_uint8_t *p;
	duk_uint8_t x;
	duk_small_uint_t s2n_flags;

	DUK_DDD(DUK_DDDPRINT("parse_number"));

	p_start = js_ctx->p;

	/* First pass parse is very lenient (e.g. allows '1.2.3') and extracts a
	 * string for strict number parsing.
	 */

	p = js_ctx->p;
	for (;;) {
		x = *p;

		DUK_DDD(DUK_DDDPRINT("parse_number: p_start=%p, p=%p, p_end=%p, x=%ld",
		                     (const void *) p_start,
		                     (const void *) p,
		                     (const void *) js_ctx->p_end,
		                     (long) x));

#if defined(DUK_USE_JSON_DECNUMBER_FASTPATH)
		/* This fast path is pretty marginal in practice.
		 * XXX: candidate for removal.
		 */
		DUK_ASSERT(duk__json_decnumber_lookup[0x00] == 0x00); /* end-of-input breaks */
		if (duk__json_decnumber_lookup[x] == 0) {
			break;
		}
#else /* DUK_USE_JSON_DECNUMBER_FASTPATH */
		if (!((x >= DUK_ASC_0 && x <= DUK_ASC_9) ||
		      (x == DUK_ASC_PERIOD || x == DUK_ASC_LC_E || x == DUK_ASC_UC_E || x == DUK_ASC_MINUS || x == DUK_ASC_PLUS))) {
			/* Plus sign must be accepted for positive exponents
			 * (e.g. '1.5e+2').  This clause catches NULs.
			 */
			break;
		}
#endif /* DUK_USE_JSON_DECNUMBER_FASTPATH */
		p++; /* safe, because matched (NUL causes a break) */
	}
	js_ctx->p = p;

	DUK_ASSERT(js_ctx->p > p_start);
	duk_push_lstring(thr, (const char *) p_start, (duk_size_t) (p - p_start));

	s2n_flags = DUK_S2N_FLAG_ALLOW_EXP | DUK_S2N_FLAG_ALLOW_MINUS | /* but don't allow leading plus */
	            DUK_S2N_FLAG_ALLOW_FRAC;

	DUK_DDD(DUK_DDDPRINT("parse_number: string before parsing: %!T", (duk_tval *) duk_get_tval(thr, -1)));
	duk_numconv_parse(thr, 10 /*radix*/, s2n_flags);
	if (duk_is_nan(thr, -1)) {
		duk__json_dec_syntax_error(js_ctx);
	}
	DUK_ASSERT(duk_is_number(thr, -1));
	DUK_DDD(DUK_DDDPRINT("parse_number: final number: %!T", (duk_tval *) duk_get_tval(thr, -1)));

	/* [ ... num ] */
}

DUK_LOCAL void duk__json_dec_objarr_entry(duk_json_dec_ctx *js_ctx) {
	duk_hthread *thr = js_ctx->thr;
	duk_require_stack(thr, DUK_JSON_DEC_REQSTACK);

	/* c recursion check */

	duk_native_stack_check(thr);

	DUK_ASSERT_DISABLE(js_ctx->recursion_depth >= 0); /* unsigned */
	DUK_ASSERT(js_ctx->recursion_depth <= js_ctx->recursion_limit);
	if (js_ctx->recursion_depth >= js_ctx->recursion_limit) {
		DUK_ERROR_RANGE(thr, DUK_STR_DEC_RECLIMIT);
		DUK_WO_NORETURN(return;);
	}
	js_ctx->recursion_depth++;
}

DUK_LOCAL void duk__json_dec_objarr_exit(duk_json_dec_ctx *js_ctx) {
	/* c recursion check */

	DUK_ASSERT(js_ctx->recursion_depth > 0);
	DUK_ASSERT(js_ctx->recursion_depth <= js_ctx->recursion_limit);
	js_ctx->recursion_depth--;
}

DUK_LOCAL void duk__json_dec_object(duk_json_dec_ctx *js_ctx) {
	duk_hthread *thr = js_ctx->thr;
	duk_int_t key_count; /* XXX: a "first" flag would suffice */
	duk_uint8_t x;

	DUK_DDD(DUK_DDDPRINT("parse_object"));

	duk__json_dec_objarr_entry(js_ctx);

	duk_push_object(thr);

	/* Initial '{' has been checked and eaten by caller. */

	key_count = 0;
	for (;;) {
		x = duk__json_dec_get_nonwhite(js_ctx);

		DUK_DDD(DUK_DDDPRINT("parse_object: obj=%!T, x=%ld, key_count=%ld",
		                     (duk_tval *) duk_get_tval(thr, -1),
		                     (long) x,
		                     (long) key_count));

		/* handle comma and closing brace */

		if (x == DUK_ASC_COMMA && key_count > 0) {
			/* accept comma, expect new value */
			x = duk__json_dec_get_nonwhite(js_ctx);
		} else if (x == DUK_ASC_RCURLY) {
			/* eat closing brace */
			break;
		} else if (key_count == 0) {
			/* accept anything, expect first value (EOF will be
			 * caught by key parsing below.
			 */
			;
		} else {
			/* catches EOF (NUL) and initial comma */
			goto syntax_error;
		}

		/* parse key and value */

		if (x == DUK_ASC_DOUBLEQUOTE) {
			duk__json_dec_string(js_ctx);
#if defined(DUK_USE_JX)
		} else if (js_ctx->flag_ext_custom && duk_unicode_is_identifier_start((duk_codepoint_t) x)) {
			duk__json_dec_plain_string(js_ctx);
#endif
		} else {
			goto syntax_error;
		}

		/* [ ... obj key ] */

		x = duk__json_dec_get_nonwhite(js_ctx);
		if (x != DUK_ASC_COLON) {
			goto syntax_error;
		}

		duk__json_dec_value(js_ctx);

		/* [ ... obj key val ] */

		duk_xdef_prop_wec(thr, -3);

		/* [ ... obj ] */

		key_count++;
	}

	/* [ ... obj ] */

	DUK_DDD(DUK_DDDPRINT("parse_object: final object is %!T", (duk_tval *) duk_get_tval(thr, -1)));

	duk__json_dec_objarr_exit(js_ctx);
	return;

syntax_error:
	duk__json_dec_syntax_error(js_ctx);
	DUK_UNREACHABLE();
}

DUK_LOCAL void duk__json_dec_array(duk_json_dec_ctx *js_ctx) {
	duk_hthread *thr = js_ctx->thr;
	duk_uarridx_t arr_idx;
	duk_uint8_t x;

	DUK_DDD(DUK_DDDPRINT("parse_array"));

	duk__json_dec_objarr_entry(js_ctx);

	duk_push_array(thr);

	/* Initial '[' has been checked and eaten by caller. */

	arr_idx = 0;
	for (;;) {
		x = duk__json_dec_get_nonwhite(js_ctx);

		DUK_DDD(DUK_DDDPRINT("parse_array: arr=%!T, x=%ld, arr_idx=%ld",
		                     (duk_tval *) duk_get_tval(thr, -1),
		                     (long) x,
		                     (long) arr_idx));

		/* handle comma and closing bracket */

		if ((x == DUK_ASC_COMMA) && (arr_idx != 0)) {
			/* accept comma, expect new value */
			;
		} else if (x == DUK_ASC_RBRACKET) {
			/* eat closing bracket */
			break;
		} else if (arr_idx == 0) {
			/* accept anything, expect first value (EOF will be
			 * caught by duk__json_dec_value() below.
			 */
			js_ctx->p--; /* backtrack (safe) */
		} else {
			/* catches EOF (NUL) and initial comma */
			goto syntax_error;
		}

		/* parse value */

		duk__json_dec_value(js_ctx);

		/* [ ... arr val ] */

		duk_xdef_prop_index_wec(thr, -2, arr_idx);
		arr_idx++;
	}

	/* Must set 'length' explicitly when using duk_xdef_prop_xxx() to
	 * set the values.
	 */

	duk_set_length(thr, -1, arr_idx);

	/* [ ... arr ] */

	DUK_DDD(DUK_DDDPRINT("parse_array: final array is %!T", (duk_tval *) duk_get_tval(thr, -1)));

	duk__json_dec_objarr_exit(js_ctx);
	return;

syntax_error:
	duk__json_dec_syntax_error(js_ctx);
	DUK_UNREACHABLE();
}

DUK_LOCAL void duk__json_dec_value(duk_json_dec_ctx *js_ctx) {
	duk_hthread *thr = js_ctx->thr;
	duk_uint8_t x;

	x = duk__json_dec_get_nonwhite(js_ctx);

	DUK_DDD(DUK_DDDPRINT("parse_value: initial x=%ld", (long) x));

	/* Note: duk__json_dec_req_stridx() backtracks one char */

	if (x == DUK_ASC_DOUBLEQUOTE) {
		duk__json_dec_string(js_ctx);
	} else if ((x >= DUK_ASC_0 && x <= DUK_ASC_9) || (x == DUK_ASC_MINUS)) {
#if defined(DUK_USE_JX)
		if (js_ctx->flag_ext_custom && x == DUK_ASC_MINUS && duk__json_dec_peek(js_ctx) == DUK_ASC_UC_I) {
			duk__json_dec_req_stridx(js_ctx, DUK_STRIDX_MINUS_INFINITY); /* "-Infinity", '-' has been eaten */
			duk_push_number(thr, -DUK_DOUBLE_INFINITY);
		} else {
#else
		{ /* unconditional block */
#endif
			/* We already ate 'x', so backup one byte. */
			js_ctx->p--; /* safe */
			duk__json_dec_number(js_ctx);
		}
	} else if (x == DUK_ASC_LC_T) {
		duk__json_dec_req_stridx(js_ctx, DUK_STRIDX_TRUE);
		duk_push_true(thr);
	} else if (x == DUK_ASC_LC_F) {
		duk__json_dec_req_stridx(js_ctx, DUK_STRIDX_FALSE);
		duk_push_false(thr);
	} else if (x == DUK_ASC_LC_N) {
		duk__json_dec_req_stridx(js_ctx, DUK_STRIDX_LC_NULL);
		duk_push_null(thr);
#if defined(DUK_USE_JX)
	} else if (js_ctx->flag_ext_custom && x == DUK_ASC_LC_U) {
		duk__json_dec_req_stridx(js_ctx, DUK_STRIDX_LC_UNDEFINED);
		duk_push_undefined(thr);
	} else if (js_ctx->flag_ext_custom && x == DUK_ASC_UC_N) {
		duk__json_dec_req_stridx(js_ctx, DUK_STRIDX_NAN);
		duk_push_nan(thr);
	} else if (js_ctx->flag_ext_custom && x == DUK_ASC_UC_I) {
		duk__json_dec_req_stridx(js_ctx, DUK_STRIDX_INFINITY);
		duk_push_number(thr, DUK_DOUBLE_INFINITY);
	} else if (js_ctx->flag_ext_custom && x == DUK_ASC_LPAREN) {
		duk__json_dec_pointer(js_ctx);
	} else if (js_ctx->flag_ext_custom && x == DUK_ASC_PIPE) {
		duk__json_dec_buffer(js_ctx);
#endif
	} else if (x == DUK_ASC_LCURLY) {
		duk__json_dec_object(js_ctx);
	} else if (x == DUK_ASC_LBRACKET) {
		duk__json_dec_array(js_ctx);
	} else {
		/* catches EOF (NUL) */
		goto syntax_error;
	}

	duk__json_dec_eat_white(js_ctx);

	/* [ ... val ] */
	return;

syntax_error:
	duk__json_dec_syntax_error(js_ctx);
	DUK_UNREACHABLE();
}

/* Recursive value reviver, implements the Walk() algorithm.  The parsing
 * step ensures there is a reasonable depth limit to the input.  However,
 * the reviver may create more depth by editing object or array entries, so
 * we have both C recursion limit and native stack checks here.
 */
DUK_LOCAL void duk__json_dec_reviver_walk(duk_json_dec_ctx *js_ctx) {
	duk_hthread *thr = js_ctx->thr;
	duk_hobject *h;
	duk_uarridx_t i, arr_len;

	duk__json_dec_objarr_entry(js_ctx);

	DUK_DDD(DUK_DDDPRINT("walk: top=%ld, holder=%!T, name=%!T",
	                     (long) duk_get_top(thr),
	                     (duk_tval *) duk_get_tval(thr, -2),
	                     (duk_tval *) duk_get_tval(thr, -1)));

	duk_dup_top(thr);
	duk_get_prop(thr, -3); /* -> [ ... holder name val ] */

	h = duk_get_hobject(thr, -1);
	if (h != NULL) {
		if (duk_js_isarray_hobject(h)) {
			arr_len = (duk_uarridx_t) duk_get_length(thr, -1);
			for (i = 0; i < arr_len; i++) {
				/* [ ... holder name val ] */

				DUK_DDD(DUK_DDDPRINT("walk: array, top=%ld, i=%ld, arr_len=%ld, holder=%!T, name=%!T, val=%!T",
				                     (long) duk_get_top(thr),
				                     (long) i,
				                     (long) arr_len,
				                     (duk_tval *) duk_get_tval(thr, -3),
				                     (duk_tval *) duk_get_tval(thr, -2),
				                     (duk_tval *) duk_get_tval(thr, -1)));

				duk_dup_top(thr);
				(void) duk_push_uint_to_hstring(thr,
				                                (duk_uint_t) i); /* -> [ ... holder name val val ToString(i) ] */
				duk__json_dec_reviver_walk(js_ctx); /* -> [ ... holder name val new_elem ] */

				if (duk_is_undefined(thr, -1)) {
					duk_pop(thr);
					duk_del_prop_index(thr, -1, i);
				} else {
					/* XXX: duk_xdef_prop_index_wec() would be more appropriate
					 * here but it currently makes some assumptions that might
					 * not hold (e.g. that previous property is not an accessor).
					 */
					duk_put_prop_index(thr, -2, i);
				}
			}
		} else {
			/* [ ... holder name val ] */
			duk_enum(thr, -1, DUK_ENUM_OWN_PROPERTIES_ONLY /*flags*/);
			while (duk_next(thr, -1 /*enum_index*/, 0 /*get_value*/)) {
				DUK_DDD(DUK_DDDPRINT("walk: object, top=%ld, holder=%!T, name=%!T, val=%!T, enum=%!iT, obj_key=%!T",
				                     (long) duk_get_top(thr),
				                     (duk_tval *) duk_get_tval(thr, -5),
				                     (duk_tval *) duk_get_tval(thr, -4),
				                     (duk_tval *) duk_get_tval(thr, -3),
				                     (duk_tval *) duk_get_tval(thr, -2),
				                     (duk_tval *) duk_get_tval(thr, -1)));

				/* [ ... holder name val enum obj_key ] */
				duk_dup_m3(thr);
				duk_dup_m2(thr);

				/* [ ... holder name val enum obj_key val obj_key ] */
				duk__json_dec_reviver_walk(js_ctx);

				/* [ ... holder name val enum obj_key new_elem ] */
				if (duk_is_undefined(thr, -1)) {
					duk_pop(thr);
					duk_del_prop(thr, -3);
				} else {
					/* XXX: duk_xdef_prop_index_wec() would be more appropriate
					 * here but it currently makes some assumptions that might
					 * not hold (e.g. that previous property is not an accessor).
					 *
					 * Using duk_put_prop() works incorrectly with '__proto__'
					 * if the own property with that name has been deleted.  This
					 * does not happen normally, but a clever reviver can trigger
					 * that, see complex reviver case in: test-bug-json-parse-__proto__.js.
					 */
					duk_put_prop(thr, -4);
				}
			}
			duk_pop(thr); /* pop enum */
		}
	}

	/* [ ... holder name val ] */

	duk_dup(thr, js_ctx->idx_reviver);
	duk_insert(thr, -4); /* -> [ ... reviver holder name val ] */
	duk_call_method(thr, 2); /* -> [ ... res ] */

	duk__json_dec_objarr_exit(js_ctx);

	DUK_DDD(DUK_DDDPRINT("walk: top=%ld, result=%!T", (long) duk_get_top(thr), (duk_tval *) duk_get_tval(thr, -1)));
}

/*
 *  Stringify implementation.
 */

#define DUK__EMIT_1(js_ctx, ch)       duk__emit_1((js_ctx), (duk_uint_fast8_t) (ch))
#define DUK__EMIT_2(js_ctx, ch1, ch2) duk__emit_2((js_ctx), (duk_uint_fast8_t) (ch1), (duk_uint_fast8_t) (ch2))
#define DUK__EMIT_HSTR(js_ctx, h)     duk__emit_hstring((js_ctx), (h))
#if defined(DUK_USE_FASTINT) || defined(DUK_USE_JX) || defined(DUK_USE_JC)
#define DUK__EMIT_CSTR(js_ctx, p) duk__emit_cstring((js_ctx), (p))
#endif
#define DUK__EMIT_STRIDX(js_ctx, i) duk__emit_stridx((js_ctx), (i))
#define DUK__UNEMIT_1(js_ctx)       duk__unemit_1((js_ctx))

DUK_LOCAL void duk__emit_1(duk_json_enc_ctx *js_ctx, duk_uint_fast8_t ch) {
	DUK_BW_WRITE_ENSURE_U8(js_ctx->thr, &js_ctx->bw, ch);
}

DUK_LOCAL void duk__emit_2(duk_json_enc_ctx *js_ctx, duk_uint_fast8_t ch1, duk_uint_fast8_t ch2) {
	DUK_BW_WRITE_ENSURE_U8_2(js_ctx->thr, &js_ctx->bw, ch1, ch2);
}

DUK_LOCAL void duk__emit_hstring(duk_json_enc_ctx *js_ctx, duk_hstring *h) {
	DUK_BW_WRITE_ENSURE_HSTRING(js_ctx->thr, &js_ctx->bw, h);
}

#if defined(DUK_USE_FASTINT) || defined(DUK_USE_JX) || defined(DUK_USE_JC)
DUK_LOCAL void duk__emit_cstring(duk_json_enc_ctx *js_ctx, const char *str) {
	DUK_BW_WRITE_ENSURE_CSTRING(js_ctx->thr, &js_ctx->bw, str);
}
#endif

DUK_LOCAL void duk__emit_stridx(duk_json_enc_ctx *js_ctx, duk_small_uint_t stridx) {
	duk_hstring *h;

	DUK_ASSERT_STRIDX_VALID(stridx);
	h = DUK_HTHREAD_GET_STRING(js_ctx->thr, stridx);
	DUK_ASSERT(h != NULL);

	DUK_BW_WRITE_ENSURE_HSTRING(js_ctx->thr, &js_ctx->bw, h);
}

DUK_LOCAL void duk__unemit_1(duk_json_enc_ctx *js_ctx) {
	DUK_ASSERT(DUK_BW_GET_SIZE(js_ctx->thr, &js_ctx->bw) >= 1);
	DUK_BW_ADD_PTR(js_ctx->thr, &js_ctx->bw, -1);
}

#define DUK__MKESC(nybbles, esc1, esc2) \
	(((duk_uint_fast32_t) (nybbles)) << 16) | (((duk_uint_fast32_t) (esc1)) << 8) | ((duk_uint_fast32_t) (esc2))

DUK_LOCAL duk_uint8_t *duk__emit_esc_auto_fast(duk_json_enc_ctx *js_ctx, duk_uint_fast32_t cp, duk_uint8_t *q) {
	duk_uint_fast32_t tmp;
	duk_small_uint_t dig;

	DUK_UNREF(js_ctx);

	/* Caller ensures space for at least DUK__JSON_MAX_ESC_LEN. */

	/* Select appropriate escape format automatically, and set 'tmp' to a
	 * value encoding both the escape format character and the nybble count:
	 *
	 *   (nybble_count << 16) | (escape_char1) | (escape_char2)
	 */

#if defined(DUK_USE_JX)
	if (DUK_LIKELY(cp < 0x100UL)) {
		if (DUK_UNLIKELY(js_ctx->flag_ext_custom != 0U)) {
			tmp = DUK__MKESC(2, DUK_ASC_BACKSLASH, DUK_ASC_LC_X);
		} else {
			tmp = DUK__MKESC(4, DUK_ASC_BACKSLASH, DUK_ASC_LC_U);
		}
	} else
#endif
	    if (DUK_LIKELY(cp < 0x10000UL)) {
		tmp = DUK__MKESC(4, DUK_ASC_BACKSLASH, DUK_ASC_LC_U);
	} else {
#if defined(DUK_USE_JX)
		if (DUK_LIKELY(js_ctx->flag_ext_custom != 0U)) {
			tmp = DUK__MKESC(8, DUK_ASC_BACKSLASH, DUK_ASC_UC_U);
		} else
#endif
		{
			/* In compatible mode and standard JSON mode, output
			 * something useful for non-BMP characters.  This won't
			 * roundtrip but will still be more or less readable and
			 * more useful than an error.
			 */
			tmp = DUK__MKESC(8, DUK_ASC_UC_U, DUK_ASC_PLUS);
		}
	}

	*q++ = (duk_uint8_t) ((tmp >> 8) & 0xff);
	*q++ = (duk_uint8_t) (tmp & 0xff);

	tmp = tmp >> 16;
	while (tmp > 0) {
		tmp--;
		dig = (duk_small_uint_t) ((cp >> (4 * tmp)) & 0x0f);
		*q++ = duk_lc_digits[dig];
	}

	return q;
}

DUK_LOCAL void duk__json_enc_key_autoquote(duk_json_enc_ctx *js_ctx, duk_hstring *k) {
	const duk_int8_t *p, *p_start, *p_end; /* Note: intentionally signed. */
	duk_size_t k_len;
	duk_codepoint_t cp;

	DUK_ASSERT(k != NULL);

	/* Accept ASCII strings which conform to identifier requirements
	 * as being emitted without key quotes.  Since we only accept ASCII
	 * there's no need for actual decoding: 'p' is intentionally signed
	 * so that bytes >= 0x80 extend to negative values and are rejected
	 * as invalid identifier codepoints.
	 */

	if (js_ctx->flag_avoid_key_quotes) {
		k_len = DUK_HSTRING_GET_BYTELEN(k);
		p_start = (const duk_int8_t *) DUK_HSTRING_GET_DATA(k);
		p_end = p_start + k_len;
		p = p_start;

		if (p == p_end) {
			/* Zero length string is not accepted without quotes */
			goto quote_normally;
		}
		cp = (duk_codepoint_t) (*p++);
		if (DUK_UNLIKELY(!duk_unicode_is_identifier_start(cp))) {
			goto quote_normally;
		}
		while (p < p_end) {
			cp = (duk_codepoint_t) (*p++);
			if (DUK_UNLIKELY(!duk_unicode_is_identifier_part(cp))) {
				goto quote_normally;
			}
		}

		/* This seems faster than emitting bytes one at a time and
		 * then potentially rewinding.
		 */
		DUK__EMIT_HSTR(js_ctx, k);
		return;
	}

quote_normally:
	duk__json_enc_quote_string(js_ctx, k);
}

/* The Quote(value) operation: quote a string.
 *
 * Stack policy: [ ] -> [ ].
 */

DUK_LOCAL void duk__json_enc_quote_string(duk_json_enc_ctx *js_ctx, duk_hstring *h_str) {
	duk_hthread *thr = js_ctx->thr;
	const duk_uint8_t *p, *p_start, *p_end, *p_now, *p_tmp;
	duk_uint8_t *q;
	duk_ucodepoint_t cp; /* typed for duk_unicode_decode_xutf8() */

	DUK_DDD(DUK_DDDPRINT("duk__json_enc_quote_string: h_str=%!O", (duk_heaphdr *) h_str));

	DUK_ASSERT(h_str != NULL);
	p_start = DUK_HSTRING_GET_DATA(h_str);
	p_end = p_start + DUK_HSTRING_GET_BYTELEN(h_str);
	p = p_start;

	DUK__EMIT_1(js_ctx, DUK_ASC_DOUBLEQUOTE);

	/* Encode string in small chunks, estimating the maximum expansion so that
	 * there's no need to ensure space while processing the chunk.
	 */

	while (p < p_end) {
		duk_size_t left, now, space;

		left = (duk_size_t) (p_end - p);
		now = (left > DUK__JSON_ENCSTR_CHUNKSIZE ? DUK__JSON_ENCSTR_CHUNKSIZE : left);

		/* Maximum expansion per input byte is 6:
		 *   - invalid UTF-8 byte causes "\uXXXX" to be emitted (6/1 = 6).
		 *   - 2-byte UTF-8 encodes as "\uXXXX" (6/2 = 3).
		 *   - 4-byte UTF-8 encodes as "\Uxxxxxxxx" (10/4 = 2.5).
		 */
		space = now * 6;
		q = DUK_BW_ENSURE_GETPTR(thr, &js_ctx->bw, space);

		p_now = p + now;

		while (p < p_now) {
#if defined(DUK_USE_JSON_QUOTESTRING_FASTPATH)
			duk_uint8_t b;

			b = duk__json_quotestr_lookup[*p++];
			if (DUK_LIKELY(b < 0x80)) {
				/* Most input bytes go through here. */
				*q++ = b;
			} else if (b >= 0xa0) {
				*q++ = DUK_ASC_BACKSLASH;
				*q++ = (duk_uint8_t) (b - 0x80);
			} else if (b == 0x80) {
				cp = (duk_ucodepoint_t) (*(p - 1));
				q = duk__emit_esc_auto_fast(js_ctx, cp, q);
			} else if (b == 0x7f && js_ctx->flag_ascii_only) {
				/* 0x7F is special */
				DUK_ASSERT(b == 0x81);
				cp = (duk_ucodepoint_t) 0x7f;
				q = duk__emit_esc_auto_fast(js_ctx, cp, q);
			} else {
				DUK_ASSERT(b == 0x81);
				p--;

				/* slow path is shared */
#else /* DUK_USE_JSON_QUOTESTRING_FASTPATH */
			cp = *p;

			if (DUK_LIKELY(cp <= 0x7f)) {
				/* ascii fast path: avoid decoding utf-8 */
				p++;
				if (cp == 0x22 || cp == 0x5c) {
					/* double quote or backslash */
					*q++ = DUK_ASC_BACKSLASH;
					*q++ = (duk_uint8_t) cp;
				} else if (cp < 0x20) {
					duk_uint_fast8_t esc_char;

					/* This approach is a bit shorter than a straight
					 * if-else-ladder and also a bit faster.
					 */
					if (cp < (sizeof(duk__json_quotestr_esc) / sizeof(duk_uint8_t)) &&
					    (esc_char = duk__json_quotestr_esc[cp]) != 0) {
						*q++ = DUK_ASC_BACKSLASH;
						*q++ = (duk_uint8_t) esc_char;
					} else {
						q = duk__emit_esc_auto_fast(js_ctx, cp, q);
					}
				} else if (cp == 0x7f && js_ctx->flag_ascii_only) {
					q = duk__emit_esc_auto_fast(js_ctx, cp, q);
				} else {
					/* any other printable -> as is */
					*q++ = (duk_uint8_t) cp;
				}
			} else {
				/* slow path is shared */
#endif /* DUK_USE_JSON_QUOTESTRING_FASTPATH */

				/* slow path decode */

				/* If XUTF-8 decoding fails, treat the offending byte as a codepoint directly
				 * and go forward one byte.  This is of course very lossy, but allows some kind
				 * of output to be produced even for internal strings which don't conform to
				 * XUTF-8.  All standard ECMAScript strings are always CESU-8, so this behavior
				 * does not violate the ECMAScript specification.  The behavior is applied to
				 * all modes, including ECMAScript standard JSON.  Because the current XUTF-8
				 * decoding is not very strict, this behavior only really affects initial bytes
				 * and truncated codepoints.
				 *
				 * Another alternative would be to scan forwards to start of next codepoint
				 * (or end of input) and emit just one replacement codepoint.
				 */

				p_tmp = p;
				if (!duk_unicode_decode_xutf8(thr, &p, p_start, p_end, &cp)) {
					/* Decode failed. */
					cp = *p_tmp;
					p = p_tmp + 1;
				}

#if defined(DUK_USE_NONSTD_JSON_ESC_U2028_U2029)
				if (js_ctx->flag_ascii_only || cp == 0x2028 || cp == 0x2029) {
#else
				if (js_ctx->flag_ascii_only) {
#endif
					q = duk__emit_esc_auto_fast(js_ctx, cp, q);
				} else {
					/* as is */
					DUK_RAW_WRITEINC_XUTF8(q, cp);
				}
			}
		}

		DUK_BW_SET_PTR(thr, &js_ctx->bw, q);
	}

	DUK__EMIT_1(js_ctx, DUK_ASC_DOUBLEQUOTE);
}

/* Encode a double (checked by caller) from stack top.  Stack top may be
 * replaced by serialized string but is not popped (caller does that).
 */
DUK_LOCAL void duk__json_enc_double(duk_json_enc_ctx *js_ctx) {
	duk_hthread *thr;
	duk_tval *tv;
	duk_double_t d;
	duk_small_int_t c;
	duk_small_int_t s;
	duk_small_uint_t stridx;
	duk_small_uint_t n2s_flags;
	duk_hstring *h_str;

	DUK_ASSERT(js_ctx != NULL);
	thr = js_ctx->thr;
	DUK_ASSERT(thr != NULL);

	/* Caller must ensure 'tv' is indeed a double and not a fastint! */
	tv = DUK_GET_TVAL_NEGIDX(thr, -1);
	DUK_ASSERT(DUK_TVAL_IS_DOUBLE(tv));
	d = DUK_TVAL_GET_DOUBLE(tv);

	c = (duk_small_int_t) DUK_FPCLASSIFY(d);
	s = (duk_small_int_t) DUK_SIGNBIT(d);
	DUK_UNREF(s);

	if (DUK_LIKELY(!(c == DUK_FP_INFINITE || c == DUK_FP_NAN))) {
		DUK_ASSERT(DUK_ISFINITE(d));

#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
		/* Negative zero needs special handling in JX/JC because
		 * it would otherwise serialize to '0', not '-0'.
		 */
		if (DUK_UNLIKELY(c == DUK_FP_ZERO && s != 0 && (js_ctx->flag_ext_custom_or_compatible))) {
			duk_push_hstring_stridx(thr, DUK_STRIDX_MINUS_ZERO); /* '-0' */
		} else
#endif /* DUK_USE_JX || DUK_USE_JC */
		{
			n2s_flags = 0;
			/* [ ... number ] -> [ ... string ] */
			duk_numconv_stringify(thr, 10 /*radix*/, 0 /*digits*/, n2s_flags);
		}
		h_str = duk_known_hstring(thr, -1);
		DUK__EMIT_HSTR(js_ctx, h_str);
		return;
	}

#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
	if (!(js_ctx->flags & (DUK_JSON_FLAG_EXT_CUSTOM | DUK_JSON_FLAG_EXT_COMPATIBLE))) {
		stridx = DUK_STRIDX_LC_NULL;
	} else if (c == DUK_FP_NAN) {
		stridx = js_ctx->stridx_custom_nan;
	} else if (s == 0) {
		stridx = js_ctx->stridx_custom_posinf;
	} else {
		stridx = js_ctx->stridx_custom_neginf;
	}
#else
	stridx = DUK_STRIDX_LC_NULL;
#endif
	DUK__EMIT_STRIDX(js_ctx, stridx);
}

#if defined(DUK_USE_FASTINT)
/* Encode a fastint from duk_tval ptr, no value stack effects. */
DUK_LOCAL void duk__json_enc_fastint_tval(duk_json_enc_ctx *js_ctx, duk_tval *tv) {
	duk_int64_t v;

	/* Fastint range is signed 48-bit so longest value is -2^47 = -140737488355328
	 * (16 chars long), longest signed 64-bit value is -2^63 = -9223372036854775808
	 * (20 chars long).  Alloc space for 64-bit range to be safe.
	 */
	duk_uint8_t buf[20 + 1];

	/* Caller must ensure 'tv' is indeed a fastint! */
	DUK_ASSERT(DUK_TVAL_IS_FASTINT(tv));
	v = DUK_TVAL_GET_FASTINT(tv);

	/* XXX: There are no format strings in duk_config.h yet, could add
	 * one for formatting duk_int64_t.  For now, assumes "%lld" and that
	 * "long long" type exists.  Could also rely on C99 directly but that
	 * won't work for older MSVC.
	 */
	DUK_SPRINTF((char *) buf, "%lld", (long long) v);
	DUK__EMIT_CSTR(js_ctx, (const char *) buf);
}
#endif

#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
#if defined(DUK_USE_HEX_FASTPATH)
DUK_LOCAL duk_uint8_t *duk__json_enc_buffer_data_hex(const duk_uint8_t *src, duk_size_t src_len, duk_uint8_t *dst) {
	duk_uint8_t *q;
	duk_uint16_t *q16;
	duk_small_uint_t x;
	duk_size_t i, len_safe;
#if !defined(DUK_USE_UNALIGNED_ACCESSES_POSSIBLE)
	duk_bool_t shift_dst;
#endif

	/* Unlike in duk_hex_encode() 'dst' is not necessarily aligned by 2.
	 * For platforms where unaligned accesses are not allowed, shift 'dst'
	 * ahead by 1 byte to get alignment and then duk_memmove() the result
	 * in place.  The faster encoding loop makes up the difference.
	 * There's always space for one extra byte because a terminator always
	 * follows the hex data and that's been accounted for by the caller.
	 */

#if defined(DUK_USE_UNALIGNED_ACCESSES_POSSIBLE)
	q16 = (duk_uint16_t *) (void *) dst;
#else
	shift_dst = (duk_bool_t) (((duk_size_t) dst) & 0x01U);
	if (shift_dst) {
		DUK_DD(DUK_DDPRINT("unaligned accesses not possible, dst not aligned -> step to dst + 1"));
		q16 = (duk_uint16_t *) (void *) (dst + 1);
	} else {
		DUK_DD(DUK_DDPRINT("unaligned accesses not possible, dst is aligned"));
		q16 = (duk_uint16_t *) (void *) dst;
	}
	DUK_ASSERT((((duk_size_t) q16) & 0x01U) == 0);
#endif

	len_safe = src_len & ~0x03U;
	for (i = 0; i < len_safe; i += 4) {
		q16[0] = duk_hex_enctab[src[i]];
		q16[1] = duk_hex_enctab[src[i + 1]];
		q16[2] = duk_hex_enctab[src[i + 2]];
		q16[3] = duk_hex_enctab[src[i + 3]];
		q16 += 4;
	}
	q = (duk_uint8_t *) q16;

#if !defined(DUK_USE_UNALIGNED_ACCESSES_POSSIBLE)
	if (shift_dst) {
		q--;
		duk_memmove((void *) dst, (const void *) (dst + 1), 2 * len_safe);
		DUK_ASSERT(dst + 2 * len_safe == q);
	}
#endif

	for (; i < src_len; i++) {
		x = src[i];
		*q++ = duk_lc_digits[x >> 4];
		*q++ = duk_lc_digits[x & 0x0f];
	}

	return q;
}
#else /* DUK_USE_HEX_FASTPATH */
DUK_LOCAL duk_uint8_t *duk__json_enc_buffer_data_hex(const duk_uint8_t *src, duk_size_t src_len, duk_uint8_t *dst) {
	const duk_uint8_t *p;
	const duk_uint8_t *p_end;
	duk_uint8_t *q;
	duk_small_uint_t x;

	p = src;
	p_end = src + src_len;
	q = dst;
	while (p != p_end) {
		x = *p++;
		*q++ = duk_lc_digits[x >> 4];
		*q++ = duk_lc_digits[x & 0x0f];
	}

	return q;
}
#endif /* DUK_USE_HEX_FASTPATH */

DUK_LOCAL void duk__json_enc_buffer_data(duk_json_enc_ctx *js_ctx, duk_uint8_t *buf_data, duk_size_t buf_len) {
	duk_hthread *thr;
	duk_uint8_t *q;
	duk_size_t space;

	thr = js_ctx->thr;

	DUK_ASSERT(js_ctx->flag_ext_custom || js_ctx->flag_ext_compatible); /* caller checks */
	DUK_ASSERT(js_ctx->flag_ext_custom_or_compatible);

	/* Buffer values are encoded in (lowercase) hex to make the
	 * binary data readable.  Base64 or similar would be more
	 * compact but less readable, and the point of JX/JC
	 * variants is to be as useful to a programmer as possible.
	 */

	/* The #if defined() clutter here needs to handle the three
	 * cases: (1) JX+JC, (2) JX only, (3) JC only.
	 */

	/* Note: space must cater for both JX and JC. */
	space = 9 + buf_len * 2 + 2;
	DUK_ASSERT(DUK_HBUFFER_MAX_BYTELEN <= 0x7ffffffeUL);
	DUK_ASSERT((space - 2) / 2 >= buf_len); /* overflow not possible, buffer limits */
	q = DUK_BW_ENSURE_GETPTR(thr, &js_ctx->bw, space);

#if defined(DUK_USE_JX) && defined(DUK_USE_JC)
	if (js_ctx->flag_ext_custom)
#endif
#if defined(DUK_USE_JX)
	{
		*q++ = DUK_ASC_PIPE;
		q = duk__json_enc_buffer_data_hex(buf_data, buf_len, q);
		*q++ = DUK_ASC_PIPE;

	}
#endif
#if defined(DUK_USE_JX) && defined(DUK_USE_JC)
	else
#endif
#if defined(DUK_USE_JC)
	{
		DUK_ASSERT(js_ctx->flag_ext_compatible);
		duk_memcpy((void *) q, (const void *) "{\"_buf\":\"", 9); /* len: 9 */
		q += 9;
		q = duk__json_enc_buffer_data_hex(buf_data, buf_len, q);
		*q++ = DUK_ASC_DOUBLEQUOTE;
		*q++ = DUK_ASC_RCURLY;
	}
#endif

	DUK_BW_SET_PTR(thr, &js_ctx->bw, q);
}

DUK_LOCAL void duk__json_enc_buffer_jx_jc(duk_json_enc_ctx *js_ctx, duk_hbuffer *h) {
	duk__json_enc_buffer_data(js_ctx,
	                          (duk_uint8_t *) DUK_HBUFFER_GET_DATA_PTR(js_ctx->thr->heap, h),
	                          (duk_size_t) DUK_HBUFFER_GET_SIZE(h));
}
#endif /* DUK_USE_JX || DUK_USE_JC */

#if defined(DUK_USE_JSON_STRINGIFY_FASTPATH)
DUK_LOCAL void duk__json_enc_buffer_json_fastpath(duk_json_enc_ctx *js_ctx, duk_hbuffer *h) {
	duk_size_t i, n;
	const duk_uint8_t *buf;
	duk_uint8_t *q;

	n = DUK_HBUFFER_GET_SIZE(h);
	if (n == 0) {
		DUK__EMIT_2(js_ctx, DUK_ASC_LCURLY, DUK_ASC_RCURLY);
		return;
	}

	DUK__EMIT_1(js_ctx, DUK_ASC_LCURLY);

	/* Maximum encoded length with 32-bit index: 1 + 10 + 2 + 3 + 1 + 1 = 18,
	 * with 64-bit index: 1 + 20 + 2 + 3 + 1 + 1 = 28.  32 has some slack.
	 *
	 * Note that because the output buffer is reallocated from time to time,
	 * side effects (such as finalizers) affecting the buffer 'h' must be
	 * disabled.  This is the case in the JSON.stringify() fast path.
	 */

	buf = (const duk_uint8_t *) DUK_HBUFFER_GET_DATA_PTR(js_ctx->thr->heap, h);
	if (DUK_UNLIKELY(js_ctx->h_gap != NULL)) {
		for (i = 0; i < n; i++) {
			duk__json_enc_newline_indent(js_ctx, js_ctx->recursion_depth + 1);
			q = DUK_BW_ENSURE_GETPTR(js_ctx->thr, &js_ctx->bw, 32);
			q += DUK_SPRINTF((char *) q, "\"%lu\": %u,", (unsigned long) i, (unsigned int) buf[i]);
			DUK_BW_SET_PTR(js_ctx->thr, &js_ctx->bw, q);
		}
	} else {
		q = DUK_BW_GET_PTR(js_ctx->thr, &js_ctx->bw);
		for (i = 0; i < n; i++) {
			q = DUK_BW_ENSURE_RAW(js_ctx->thr, &js_ctx->bw, 32, q);
			q += DUK_SPRINTF((char *) q, "\"%lu\":%u,", (unsigned long) i, (unsigned int) buf[i]);
		}
		DUK_BW_SET_PTR(js_ctx->thr, &js_ctx->bw, q);
	}
	DUK__UNEMIT_1(js_ctx); /* eat trailing comma */

	if (DUK_UNLIKELY(js_ctx->h_gap != NULL)) {
		duk__json_enc_newline_indent(js_ctx, js_ctx->recursion_depth);
	}
	DUK__EMIT_1(js_ctx, DUK_ASC_RCURLY);
}
#endif /* DUK_USE_JSON_STRINGIFY_FASTPATH */

#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
DUK_LOCAL void duk__json_enc_pointer(duk_json_enc_ctx *js_ctx, void *ptr) {
	char buf[64]; /* XXX: how to figure correct size? */
	const char *fmt;

	DUK_ASSERT(js_ctx->flag_ext_custom || js_ctx->flag_ext_compatible); /* caller checks */
	DUK_ASSERT(js_ctx->flag_ext_custom_or_compatible);

	duk_memzero(buf, sizeof(buf));

	/* The #if defined() clutter here needs to handle the three
	 * cases: (1) JX+JC, (2) JX only, (3) JC only.
	 */
#if defined(DUK_USE_JX) && defined(DUK_USE_JC)
	if (js_ctx->flag_ext_custom)
#endif
#if defined(DUK_USE_JX)
	{
		fmt = ptr ? "(%p)" : "(null)";
	}
#endif
#if defined(DUK_USE_JX) && defined(DUK_USE_JC)
	else
#endif
#if defined(DUK_USE_JC)
	{
		DUK_ASSERT(js_ctx->flag_ext_compatible);
		fmt = ptr ? "{\"_ptr\":\"%p\"}" : "{\"_ptr\":\"null\"}";
	}
#endif

	/* When ptr == NULL, the format argument is unused. */
	DUK_SNPRINTF(buf, sizeof(buf) - 1, fmt, ptr); /* must not truncate */
	DUK__EMIT_CSTR(js_ctx, buf);
}
#endif /* DUK_USE_JX || DUK_USE_JC */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
DUK_LOCAL void duk__json_enc_bufobj(duk_json_enc_ctx *js_ctx, duk_hbufobj *h_bufobj) {
	DUK_HBUFOBJ_ASSERT_VALID(h_bufobj);

	if (h_bufobj->buf == NULL || !DUK_HBUFOBJ_VALID_SLICE(h_bufobj)) {
		DUK__EMIT_STRIDX(js_ctx, DUK_STRIDX_LC_NULL);
	} else {
		/* Handle both full and partial slice (as long as covered). */
		duk__json_enc_buffer_data(js_ctx,
		                          (duk_uint8_t *) DUK_HBUFOBJ_GET_SLICE_BASE(js_ctx->thr->heap, h_bufobj),
		                          (duk_size_t) h_bufobj->length);
	}
}
#endif /* DUK_USE_JX || DUK_USE_JC */
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */

/* Indent helper.  Calling code relies on js_ctx->recursion_depth also being
 * directly related to indent depth.
 */
#if defined(DUK_USE_PREFER_SIZE)
DUK_LOCAL void duk__json_enc_newline_indent(duk_json_enc_ctx *js_ctx, duk_uint_t depth) {
	DUK_ASSERT(js_ctx->h_gap != NULL);
	DUK_ASSERT(DUK_HSTRING_GET_BYTELEN(js_ctx->h_gap) > 0); /* caller guarantees */

	DUK__EMIT_1(js_ctx, 0x0a);
	while (depth-- > 0) {
		DUK__EMIT_HSTR(js_ctx, js_ctx->h_gap);
	}
}
#else /* DUK_USE_PREFER_SIZE */
DUK_LOCAL void duk__json_enc_newline_indent(duk_json_enc_ctx *js_ctx, duk_uint_t depth) {
	const duk_uint8_t *gap_data;
	duk_size_t gap_len;
	duk_size_t avail_bytes; /* bytes of indent available for copying */
	duk_size_t need_bytes; /* bytes of indent still needed */
	duk_uint8_t *p_start;
	duk_uint8_t *p;

	DUK_ASSERT(js_ctx->h_gap != NULL);
	DUK_ASSERT(DUK_HSTRING_GET_BYTELEN(js_ctx->h_gap) > 0); /* caller guarantees */

	DUK__EMIT_1(js_ctx, 0x0a);
	if (DUK_UNLIKELY(depth == 0)) {
		return;
	}

	/* To handle deeper indents efficiently, make use of copies we've
	 * already emitted.  In effect we can emit a sequence of 1, 2, 4,
	 * 8, etc copies, and then finish the last run.  Byte counters
	 * avoid multiply with gap_len on every loop.
	 */

	gap_data = (const duk_uint8_t *) DUK_HSTRING_GET_DATA(js_ctx->h_gap);
	gap_len = (duk_size_t) DUK_HSTRING_GET_BYTELEN(js_ctx->h_gap);
	DUK_ASSERT(gap_len > 0);

	need_bytes = gap_len * depth;
	p = DUK_BW_ENSURE_GETPTR(js_ctx->thr, &js_ctx->bw, need_bytes);
	p_start = p;

	duk_memcpy((void *) p, (const void *) gap_data, (size_t) gap_len);
	p += gap_len;
	avail_bytes = gap_len;
	DUK_ASSERT(need_bytes >= gap_len);
	need_bytes -= gap_len;

	while (need_bytes >= avail_bytes) {
		duk_memcpy((void *) p, (const void *) p_start, (size_t) avail_bytes);
		p += avail_bytes;
		need_bytes -= avail_bytes;
		avail_bytes <<= 1;
	}

	DUK_ASSERT(need_bytes < avail_bytes); /* need_bytes may be zero */
	duk_memcpy((void *) p, (const void *) p_start, (size_t) need_bytes);
	p += need_bytes;
	/*avail_bytes += need_bytes*/

	DUK_BW_SET_PTR(js_ctx->thr, &js_ctx->bw, p);
}
#endif /* DUK_USE_PREFER_SIZE */

/* Shared entry handling for object/array serialization. */
DUK_LOCAL void duk__json_enc_objarr_entry(duk_json_enc_ctx *js_ctx, duk_idx_t *entry_top) {
	duk_hthread *thr = js_ctx->thr;
	duk_hobject *h_target;
	duk_uint_fast32_t i, n;

	*entry_top = duk_get_top(thr);

	duk_native_stack_check(thr);
	duk_require_stack(thr, DUK_JSON_ENC_REQSTACK);

	/* Loop check using a hybrid approach: a fixed-size visited[] array
	 * with overflow in a loop check object.
	 */

	h_target = duk_known_hobject(thr, -1); /* object or array */

	n = js_ctx->recursion_depth;
	if (DUK_UNLIKELY(n > DUK_JSON_ENC_LOOPARRAY)) {
		n = DUK_JSON_ENC_LOOPARRAY;
	}
	for (i = 0; i < n; i++) {
		if (DUK_UNLIKELY(js_ctx->visiting[i] == h_target)) {
			DUK_DD(DUK_DDPRINT("slow path loop detect"));
			DUK_ERROR_TYPE(thr, DUK_STR_CYCLIC_INPUT);
			DUK_WO_NORETURN(return;);
		}
	}
	if (js_ctx->recursion_depth < DUK_JSON_ENC_LOOPARRAY) {
		js_ctx->visiting[js_ctx->recursion_depth] = h_target;
	} else {
		duk_push_sprintf(thr, DUK_STR_FMT_PTR, (void *) h_target);
		duk_dup_top(thr); /* -> [ ... voidp voidp ] */
		if (duk_has_prop(thr, js_ctx->idx_loop)) {
			DUK_ERROR_TYPE(thr, DUK_STR_CYCLIC_INPUT);
			DUK_WO_NORETURN(return;);
		}
		duk_push_true(thr); /* -> [ ... voidp true ] */
		duk_put_prop(thr, js_ctx->idx_loop); /* -> [ ... ] */
	}

	/* C recursion check. */

	DUK_ASSERT_DISABLE(js_ctx->recursion_depth >= 0); /* unsigned */
	DUK_ASSERT(js_ctx->recursion_depth <= js_ctx->recursion_limit);
	if (js_ctx->recursion_depth >= js_ctx->recursion_limit) {
		DUK_ERROR_RANGE(thr, DUK_STR_ENC_RECLIMIT);
		DUK_WO_NORETURN(return;);
	}
	js_ctx->recursion_depth++;

	DUK_DDD(DUK_DDDPRINT("shared entry finished: top=%ld, loop=%!T",
	                     (long) duk_get_top(thr),
	                     (duk_tval *) duk_get_tval(thr, js_ctx->idx_loop)));
}

/* Shared exit handling for object/array serialization. */
DUK_LOCAL void duk__json_enc_objarr_exit(duk_json_enc_ctx *js_ctx, duk_idx_t *entry_top) {
	duk_hthread *thr = js_ctx->thr;
	duk_hobject *h_target;

	/* C recursion check. */

	DUK_ASSERT(js_ctx->recursion_depth > 0);
	DUK_ASSERT(js_ctx->recursion_depth <= js_ctx->recursion_limit);
	js_ctx->recursion_depth--;

	/* Loop check. */

	h_target = duk_known_hobject(thr, *entry_top - 1); /* original target at entry_top - 1 */

	if (js_ctx->recursion_depth < DUK_JSON_ENC_LOOPARRAY) {
		/* Previous entry was inside visited[], nothing to do. */
	} else {
		duk_push_sprintf(thr, DUK_STR_FMT_PTR, (void *) h_target);
		duk_del_prop(thr, js_ctx->idx_loop); /* -> [ ... ] */
	}

	/* Restore stack top after unbalanced code paths. */
	duk_set_top(thr, *entry_top);

	DUK_DDD(DUK_DDDPRINT("shared entry finished: top=%ld, loop=%!T",
	                     (long) duk_get_top(thr),
	                     (duk_tval *) duk_get_tval(thr, js_ctx->idx_loop)));
}

/* The JO(value) operation: encode object.
 *
 * Stack policy: [ object ] -> [ object ].
 */
DUK_LOCAL void duk__json_enc_object(duk_json_enc_ctx *js_ctx) {
	duk_hthread *thr = js_ctx->thr;
	duk_hstring *h_key;
	duk_idx_t entry_top;
	duk_idx_t idx_obj;
	duk_idx_t idx_keys;
	duk_bool_t emitted;
	duk_uarridx_t arr_len, i;
	duk_size_t prev_size;

	DUK_DDD(DUK_DDDPRINT("duk__json_enc_object: obj=%!T", (duk_tval *) duk_get_tval(thr, -1)));

	duk__json_enc_objarr_entry(js_ctx, &entry_top);

	idx_obj = entry_top - 1;

	if (js_ctx->idx_proplist >= 0) {
		idx_keys = js_ctx->idx_proplist;
	} else {
		/* XXX: would be nice to enumerate an object at specified index */
		duk_dup(thr, idx_obj);
		(void) duk_hobject_get_enumerated_keys(
		    thr,
		    DUK_ENUM_OWN_PROPERTIES_ONLY /*flags*/); /* [ ... target ] -> [ ... target keys ] */
		idx_keys = duk_require_normalize_index(thr, -1);
		/* leave stack unbalanced on purpose */
	}

	DUK_DDD(DUK_DDDPRINT("idx_keys=%ld, h_keys=%!T", (long) idx_keys, (duk_tval *) duk_get_tval(thr, idx_keys)));

	/* Steps 8-10 have been merged to avoid a "partial" variable. */

	DUK__EMIT_1(js_ctx, DUK_ASC_LCURLY);

	/* XXX: keys is an internal object with all keys to be processed
	 * in its (gapless) array part.  Because nobody can touch the keys
	 * object, we could iterate its array part directly (keeping in mind
	 * that it can be reallocated).
	 */

	arr_len = (duk_uarridx_t) duk_get_length(thr, idx_keys);
	emitted = 0;
	for (i = 0; i < arr_len; i++) {
		duk_get_prop_index(thr, idx_keys, i); /* -> [ ... key ] */

		DUK_DDD(DUK_DDDPRINT("object property loop: holder=%!T, key=%!T",
		                     (duk_tval *) duk_get_tval(thr, idx_obj),
		                     (duk_tval *) duk_get_tval(thr, -1)));

		h_key = duk_known_hstring(thr, -1);
		DUK_ASSERT(h_key != NULL);
		DUK_ASSERT(!DUK_HSTRING_HAS_SYMBOL(h_key)); /* proplist filtering; enum options */

		prev_size = DUK_BW_GET_SIZE(js_ctx->thr, &js_ctx->bw);
		if (DUK_UNLIKELY(js_ctx->h_gap != NULL)) {
			duk__json_enc_newline_indent(js_ctx, js_ctx->recursion_depth);
			duk__json_enc_key_autoquote(js_ctx, h_key);
			DUK__EMIT_2(js_ctx, DUK_ASC_COLON, DUK_ASC_SPACE);
		} else {
			duk__json_enc_key_autoquote(js_ctx, h_key);
			DUK__EMIT_1(js_ctx, DUK_ASC_COLON);
		}

		/* [ ... key ] */

		if (DUK_UNLIKELY(duk__json_enc_value(js_ctx, idx_obj) == 0)) {
			/* Value would yield 'undefined', so skip key altogether.
			 * Side effects have already happened.
			 */
			DUK_BW_SET_SIZE(js_ctx->thr, &js_ctx->bw, prev_size);
		} else {
			DUK__EMIT_1(js_ctx, DUK_ASC_COMMA);
			emitted = 1;
		}

		/* [ ... ] */
	}

	if (emitted) {
		DUK_ASSERT(*((duk_uint8_t *) DUK_BW_GET_PTR(js_ctx->thr, &js_ctx->bw) - 1) == DUK_ASC_COMMA);
		DUK__UNEMIT_1(js_ctx); /* eat trailing comma */
		if (DUK_UNLIKELY(js_ctx->h_gap != NULL)) {
			DUK_ASSERT(js_ctx->recursion_depth >= 1);
			duk__json_enc_newline_indent(js_ctx, js_ctx->recursion_depth - 1U);
		}
	}
	DUK__EMIT_1(js_ctx, DUK_ASC_RCURLY);

	duk__json_enc_objarr_exit(js_ctx, &entry_top);

	DUK_ASSERT_TOP(thr, entry_top);
}

/* The JA(value) operation: encode array.
 *
 * Stack policy: [ array ] -> [ array ].
 */
DUK_LOCAL void duk__json_enc_array(duk_json_enc_ctx *js_ctx) {
	duk_hthread *thr = js_ctx->thr;
	duk_idx_t entry_top;
	duk_idx_t idx_arr;
	duk_bool_t emitted;
	duk_uarridx_t i, arr_len;

	DUK_DDD(DUK_DDDPRINT("duk__json_enc_array: array=%!T", (duk_tval *) duk_get_tval(thr, -1)));

	duk__json_enc_objarr_entry(js_ctx, &entry_top);

	idx_arr = entry_top - 1;

	/* Steps 8-10 have been merged to avoid a "partial" variable. */

	DUK__EMIT_1(js_ctx, DUK_ASC_LBRACKET);

	arr_len = (duk_uarridx_t) duk_get_length(thr, idx_arr);
	emitted = 0;
	for (i = 0; i < arr_len; i++) {
		DUK_DDD(DUK_DDDPRINT("array entry loop: array=%!T, index=%ld, arr_len=%ld",
		                     (duk_tval *) duk_get_tval(thr, idx_arr),
		                     (long) i,
		                     (long) arr_len));

		if (DUK_UNLIKELY(js_ctx->h_gap != NULL)) {
			DUK_ASSERT(js_ctx->recursion_depth >= 1);
			duk__json_enc_newline_indent(js_ctx, js_ctx->recursion_depth);
		}

		(void) duk_push_uint_to_hstring(thr, (duk_uint_t) i); /* -> [ ... key ] */

		/* [ ... key ] */

		if (DUK_UNLIKELY(duk__json_enc_value(js_ctx, idx_arr) == 0)) {
			/* Value would normally be omitted, replace with 'null'. */
			DUK__EMIT_STRIDX(js_ctx, DUK_STRIDX_LC_NULL);
		} else {
			;
		}

		/* [ ... ] */

		DUK__EMIT_1(js_ctx, DUK_ASC_COMMA);
		emitted = 1;
	}

	if (emitted) {
		DUK_ASSERT(*((duk_uint8_t *) DUK_BW_GET_PTR(js_ctx->thr, &js_ctx->bw) - 1) == DUK_ASC_COMMA);
		DUK__UNEMIT_1(js_ctx); /* eat trailing comma */
		if (DUK_UNLIKELY(js_ctx->h_gap != NULL)) {
			DUK_ASSERT(js_ctx->recursion_depth >= 1);
			duk__json_enc_newline_indent(js_ctx, js_ctx->recursion_depth - 1U);
		}
	}
	DUK__EMIT_1(js_ctx, DUK_ASC_RBRACKET);

	duk__json_enc_objarr_exit(js_ctx, &entry_top);

	DUK_ASSERT_TOP(thr, entry_top);
}

/* The Str(key, holder) operation.
 *
 * Stack policy: [ ... key ] -> [ ... ]
 */
DUK_LOCAL duk_bool_t duk__json_enc_value(duk_json_enc_ctx *js_ctx, duk_idx_t idx_holder) {
	duk_hthread *thr = js_ctx->thr;
	duk_tval *tv;
	duk_tval *tv_holder;
	duk_tval *tv_key;
	duk_small_int_t c;

	DUK_DDD(DUK_DDDPRINT("duk__json_enc_value: idx_holder=%ld, holder=%!T, key=%!T",
	                     (long) idx_holder,
	                     (duk_tval *) duk_get_tval(thr, idx_holder),
	                     (duk_tval *) duk_get_tval(thr, -1)));

	tv_holder = DUK_GET_TVAL_POSIDX(thr, idx_holder);
	DUK_ASSERT(DUK_TVAL_IS_OBJECT(tv_holder));
	tv_key = DUK_GET_TVAL_NEGIDX(thr, -1);
	DUK_ASSERT(DUK_TVAL_IS_STRING(tv_key));
	DUK_ASSERT(!DUK_HSTRING_HAS_SYMBOL(DUK_TVAL_GET_STRING(tv_key))); /* Caller responsible. */
	(void) duk_hobject_getprop(thr, tv_holder, tv_key);

	/* -> [ ... key val ] */

	DUK_DDD(DUK_DDDPRINT("value=%!T", (duk_tval *) duk_get_tval(thr, -1)));

	/* Standard JSON checks for .toJSON() only for actual objects; for
	 * example, setting Number.prototype.toJSON and then serializing a
	 * number won't invoke the .toJSON() method.  However, lightfuncs and
	 * plain buffers mimic objects so we check for their .toJSON() method.
	 */
	if (duk_check_type_mask(thr, -1, DUK_TYPE_MASK_OBJECT | DUK_TYPE_MASK_LIGHTFUNC | DUK_TYPE_MASK_BUFFER)) {
		duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_TO_JSON);
		if (duk_is_callable(thr, -1)) { /* toJSON() can also be a lightfunc */
			DUK_DDD(DUK_DDDPRINT("value is object, has callable toJSON() -> call it"));
			/* XXX: duk_dup_unvalidated(thr, -2) etc. */
			duk_dup_m2(thr); /* -> [ ... key val toJSON val ] */
			duk_dup_m4(thr); /* -> [ ... key val toJSON val key ] */
			duk_call_method(thr, 1); /* -> [ ... key val val' ] */
			duk_remove_m2(thr); /* -> [ ... key val' ] */
		} else {
			duk_pop(thr); /* -> [ ... key val ] */
		}
	}

	/* [ ... key val ] */

	DUK_DDD(DUK_DDDPRINT("value=%!T", (duk_tval *) duk_get_tval(thr, -1)));

	if (js_ctx->h_replacer) {
		/* XXX: Here a "slice copy" would be useful. */
		DUK_DDD(DUK_DDDPRINT("replacer is set, call replacer"));
		duk_push_hobject(thr, js_ctx->h_replacer); /* -> [ ... key val replacer ] */
		duk_dup(thr, idx_holder); /* -> [ ... key val replacer holder ] */
		duk_dup_m4(thr); /* -> [ ... key val replacer holder key ] */
		duk_dup_m4(thr); /* -> [ ... key val replacer holder key val ] */
		duk_call_method(thr, 2); /* -> [ ... key val val' ] */
		duk_remove_m2(thr); /* -> [ ... key val' ] */
	}

	/* [ ... key val ] */

	DUK_DDD(DUK_DDDPRINT("value=%!T", (duk_tval *) duk_get_tval(thr, -1)));

	tv = DUK_GET_TVAL_NEGIDX(thr, -1);
	if (DUK_TVAL_IS_OBJECT(tv)) {
		duk_hobject *h;

		h = DUK_TVAL_GET_OBJECT(tv);
		DUK_ASSERT(h != NULL);

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
		if (DUK_HOBJECT_IS_BUFOBJ(h) && js_ctx->flags & (DUK_JSON_FLAG_EXT_CUSTOM | DUK_JSON_FLAG_EXT_COMPATIBLE)) {
			/* With JX/JC a bufferobject gets serialized specially. */
			duk_hbufobj *h_bufobj;
			h_bufobj = (duk_hbufobj *) h;
			DUK_HBUFOBJ_ASSERT_VALID(h_bufobj);
			duk__json_enc_bufobj(js_ctx, h_bufobj);
			goto pop2_emitted;
		}
		/* Otherwise bufferobjects get serialized as normal objects. */
#endif /* JX || JC */
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */
		c = (duk_small_int_t) DUK_HOBJECT_GET_CLASS_NUMBER(h);
		switch (c) {
		case DUK_HOBJECT_CLASS_NUMBER: {
			DUK_DDD(DUK_DDDPRINT("value is a Number object -> coerce with ToNumber()"));
			duk_to_number_m1(thr);
			/* The coercion potentially invokes user .valueOf() and .toString()
			 * but can't result in a function value because ToPrimitive() would
			 * reject such a result: test-dev-json-stringify-coercion-1.js.
			 */
			DUK_ASSERT(!duk_is_callable(thr, -1));
			break;
		}
		case DUK_HOBJECT_CLASS_STRING: {
			DUK_DDD(DUK_DDDPRINT("value is a String object -> coerce with ToString()"));
			duk_to_string(thr, -1);
			/* Same coercion behavior as for Number. */
			DUK_ASSERT(!duk_is_callable(thr, -1));
			break;
		}
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
		case DUK_HOBJECT_CLASS_POINTER:
#endif
		case DUK_HOBJECT_CLASS_BOOLEAN: {
			DUK_DDD(DUK_DDDPRINT("value is a Boolean/Buffer/Pointer object -> get internal value"));
			duk_xget_owndataprop_stridx_short(thr, -1, DUK_STRIDX_INT_VALUE);
			duk_remove_m2(thr);
			break;
		}
		default: {
			/* Normal object which doesn't get automatically coerced to a
			 * primitive value.  Functions are checked for specially.  The
			 * primitive value coercions for Number, String, Pointer, and
			 * Boolean can't result in functions so suffices to check here.
			 * Symbol objects are handled like plain objects (their primitive
			 * value is NOT looked up like for e.g. String objects).
			 */
			DUK_ASSERT(h != NULL);
			if (DUK_HOBJECT_IS_CALLABLE(h)) {
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
				if (js_ctx->flags & (DUK_JSON_FLAG_EXT_CUSTOM | DUK_JSON_FLAG_EXT_COMPATIBLE)) {
					/* We only get here when doing non-standard JSON encoding */
					DUK_DDD(DUK_DDDPRINT("-> function allowed, serialize to custom format"));
					DUK_ASSERT(js_ctx->flag_ext_custom || js_ctx->flag_ext_compatible);
					DUK__EMIT_STRIDX(js_ctx, js_ctx->stridx_custom_function);
					goto pop2_emitted;
				} else {
					DUK_DDD(DUK_DDDPRINT("-> will result in undefined (function)"));
					goto pop2_undef;
				}
#else /* DUK_USE_JX || DUK_USE_JC */
				DUK_DDD(DUK_DDDPRINT("-> will result in undefined (function)"));
				goto pop2_undef;
#endif /* DUK_USE_JX || DUK_USE_JC */
			}
		}
		} /* end switch */
	}

	/* [ ... key val ] */

	DUK_DDD(DUK_DDDPRINT("value=%!T", (duk_tval *) duk_get_tval(thr, -1)));

	if (duk_check_type_mask(thr, -1, js_ctx->mask_for_undefined)) {
		/* will result in undefined */
		DUK_DDD(DUK_DDDPRINT("-> will result in undefined (type mask check)"));
		goto pop2_undef;
	}
	tv = DUK_GET_TVAL_NEGIDX(thr, -1);

	switch (DUK_TVAL_GET_TAG(tv)) {
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
	/* When JX/JC not in use, the type mask above will avoid this case if needed. */
	case DUK_TAG_UNDEFINED: {
		DUK__EMIT_STRIDX(js_ctx, js_ctx->stridx_custom_undefined);
		break;
	}
#endif
	case DUK_TAG_NULL: {
		DUK__EMIT_STRIDX(js_ctx, DUK_STRIDX_LC_NULL);
		break;
	}
	case DUK_TAG_BOOLEAN: {
		DUK__EMIT_STRIDX(js_ctx, DUK_TVAL_GET_BOOLEAN(tv) ? DUK_STRIDX_TRUE : DUK_STRIDX_FALSE);
		break;
	}
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
	/* When JX/JC not in use, the type mask above will avoid this case if needed. */
	case DUK_TAG_POINTER: {
		duk__json_enc_pointer(js_ctx, DUK_TVAL_GET_POINTER(tv));
		break;
	}
#endif /* DUK_USE_JX || DUK_USE_JC */
	case DUK_TAG_STRING: {
		duk_hstring *h = DUK_TVAL_GET_STRING(tv);
		DUK_ASSERT(h != NULL);
		if (DUK_UNLIKELY(DUK_HSTRING_HAS_SYMBOL(h))) {
			goto pop2_undef;
		}
		duk__json_enc_quote_string(js_ctx, h);
		break;
	}
	case DUK_TAG_OBJECT: {
		duk_hobject *h = DUK_TVAL_GET_OBJECT(tv);
		DUK_ASSERT(h != NULL);

		/* Function values are handled completely above (including
		 * coercion results):
		 */
		DUK_ASSERT(!DUK_HOBJECT_IS_CALLABLE(h));

		if (duk_js_isarray_hobject(h)) {
			duk__json_enc_array(js_ctx);
		} else {
			duk__json_enc_object(js_ctx);
		}
		break;
	}
	/* Because plain buffers mimics Uint8Array, they have enumerable
	 * index properties [0,byteLength[.  Because JSON only serializes
	 * enumerable own properties, no properties can be serialized for
	 * plain buffers (all virtual properties are non-enumerable).  However,
	 * there may be a .toJSON() method which was already handled above.
	 */
	case DUK_TAG_BUFFER: {
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
		if (js_ctx->flag_ext_custom_or_compatible) {
			duk__json_enc_buffer_jx_jc(js_ctx, DUK_TVAL_GET_BUFFER(tv));
			break;
		}
#endif

		/* Could implement a fastpath, but the fast path would need
		 * to handle realloc side effects correctly.
		 */
		duk_to_object(thr, -1);
		duk__json_enc_object(js_ctx);
		break;
	}
	case DUK_TAG_LIGHTFUNC: {
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
		/* We only get here when doing non-standard JSON encoding */
		DUK_ASSERT(js_ctx->flag_ext_custom || js_ctx->flag_ext_compatible);
		DUK__EMIT_STRIDX(js_ctx, js_ctx->stridx_custom_function);
#else
		/* Standard JSON omits functions */
		DUK_UNREACHABLE();
#endif
		break;
	}
#if defined(DUK_USE_FASTINT)
	case DUK_TAG_FASTINT:
		/* Number serialization has a significant impact relative to
		 * other fast path code, so careful fast path for fastints.
		 */
		duk__json_enc_fastint_tval(js_ctx, tv);
		break;
#endif
	default: {
		/* number */
		DUK_ASSERT(!DUK_TVAL_IS_UNUSED(tv));
		DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv));
		/* XXX: A fast path for usual integers would be useful when
		 * fastint support is not enabled.
		 */
		duk__json_enc_double(js_ctx);
		break;
	}
	}

#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
pop2_emitted:
#endif
	duk_pop_2(thr); /* [ ... key val ] -> [ ... ] */
	return 1; /* emitted */

pop2_undef:
	duk_pop_2(thr); /* [ ... key val ] -> [ ... ] */
	return 0; /* not emitted */
}

/* E5 Section 15.12.3, main algorithm, step 4.b.ii steps 1-4. */
DUK_LOCAL duk_bool_t duk__json_enc_allow_into_proplist(duk_tval *tv) {
	duk_small_int_t c;

	/* XXX: some kind of external internal type checker?
	 * - type mask; symbol flag; class mask
	 */
	DUK_ASSERT(tv != NULL);
	if (DUK_TVAL_IS_STRING(tv)) {
		duk_hstring *h;
		h = DUK_TVAL_GET_STRING(tv);
		DUK_ASSERT(h != NULL);
		if (DUK_UNLIKELY(DUK_HSTRING_HAS_SYMBOL(h))) {
			return 0;
		}
		return 1;
	} else if (DUK_TVAL_IS_NUMBER(tv)) {
		return 1;
	} else if (DUK_TVAL_IS_OBJECT(tv)) {
		duk_hobject *h;
		h = DUK_TVAL_GET_OBJECT(tv);
		DUK_ASSERT(h != NULL);
		c = (duk_small_int_t) DUK_HOBJECT_GET_CLASS_NUMBER(h);
		if (c == DUK_HOBJECT_CLASS_STRING || c == DUK_HOBJECT_CLASS_NUMBER) {
			return 1;
		}
	}

	return 0;
}

/*
 *  JSON.stringify() fast path
 *
 *  Otherwise supports full JSON, JX, and JC features, but bails out on any
 *  possible side effect which might change the value being serialized.  The
 *  fast path can take advantage of the fact that the value being serialized
 *  is unchanged so that we can walk directly through property tables etc.
 */

#if defined(DUK_USE_JSON_STRINGIFY_FASTPATH)
DUK_LOCAL duk_bool_t duk__json_stringify_fast_value(duk_json_enc_ctx *js_ctx, duk_tval *tv) {
	duk_uint_fast32_t i, n;

	DUK_DDD(DUK_DDDPRINT("stringify fast: %!T", tv));

	DUK_ASSERT(js_ctx != NULL);
	DUK_ASSERT(js_ctx->thr != NULL);

#if 0 /* disabled for now */
 restart_match:
#endif

	DUK_ASSERT(tv != NULL);

	switch (DUK_TVAL_GET_TAG(tv)) {
	case DUK_TAG_UNDEFINED: {
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
		if (js_ctx->flag_ext_custom || js_ctx->flag_ext_compatible) {
			DUK__EMIT_STRIDX(js_ctx, js_ctx->stridx_custom_undefined);
			break;
		} else {
			goto emit_undefined;
		}
#else
		goto emit_undefined;
#endif
	}
	case DUK_TAG_NULL: {
		DUK__EMIT_STRIDX(js_ctx, DUK_STRIDX_LC_NULL);
		break;
	}
	case DUK_TAG_BOOLEAN: {
		DUK__EMIT_STRIDX(js_ctx, DUK_TVAL_GET_BOOLEAN(tv) ? DUK_STRIDX_TRUE : DUK_STRIDX_FALSE);
		break;
	}
	case DUK_TAG_STRING: {
		duk_hstring *h;
		h = DUK_TVAL_GET_STRING(tv);
		DUK_ASSERT(h != NULL);
		if (DUK_UNLIKELY(DUK_HSTRING_HAS_SYMBOL(h))) {
			goto emit_undefined;
		}
		duk__json_enc_quote_string(js_ctx, h);
		break;
	}
	case DUK_TAG_OBJECT: {
		duk_hobject *obj;
		duk_tval *tv_val;
		duk_bool_t emitted = 0;
		duk_uint32_t c_bit, c_all, c_array, c_unbox, c_undef, c_func, c_bufobj, c_object, c_abort;

		/* For objects JSON.stringify() only looks for own, enumerable
		 * properties which is nice for the fast path here.
		 *
		 * For arrays JSON.stringify() uses [[Get]] so it will actually
		 * inherit properties during serialization!  This fast path
		 * supports gappy arrays as long as there's no actual inherited
		 * property (which might be a getter etc).
		 *
		 * Since recursion only happens for objects, we can have both
		 * recursion and loop checks here.  We use a simple, depth-limited
		 * loop check in the fast path because the object-based tracking
		 * is very slow (when tested, it accounted for 50% of fast path
		 * execution time for input data with a lot of small objects!).
		 */

		/* XXX: for real world code, could just ignore array inheritance
		 * and only look at array own properties.
		 */

		/* We rely on a few object flag / class number relationships here,
		 * assert for them.
		 */

		obj = DUK_TVAL_GET_OBJECT(tv);
		DUK_ASSERT(obj != NULL);
		DUK_HOBJECT_ASSERT_VALID(obj);

		/* Once recursion depth is increased, exit path must decrease
		 * it (though it's OK to abort the fast path).
		 */

		DUK_ASSERT_DISABLE(js_ctx->recursion_depth >= 0); /* unsigned */
		DUK_ASSERT(js_ctx->recursion_depth <= js_ctx->recursion_limit);
		if (js_ctx->recursion_depth >= js_ctx->recursion_limit) {
			DUK_DD(DUK_DDPRINT("fast path recursion limit"));
			DUK_ERROR_RANGE(js_ctx->thr, DUK_STR_DEC_RECLIMIT);
			DUK_WO_NORETURN(return 0;);
		}

		for (i = 0, n = (duk_uint_fast32_t) js_ctx->recursion_depth; i < n; i++) {
			if (DUK_UNLIKELY(js_ctx->visiting[i] == obj)) {
				DUK_DD(DUK_DDPRINT("fast path loop detect"));
				DUK_ERROR_TYPE(js_ctx->thr, DUK_STR_CYCLIC_INPUT);
				DUK_WO_NORETURN(return 0;);
			}
		}

		/* Guaranteed by recursion_limit setup so we don't have to
		 * check twice.
		 */
		DUK_ASSERT(js_ctx->recursion_depth < DUK_JSON_ENC_LOOPARRAY);
		js_ctx->visiting[js_ctx->recursion_depth] = obj;
		js_ctx->recursion_depth++;

		/* If object has a .toJSON() property, we can't be certain
		 * that it wouldn't mutate any value arbitrarily, so bail
		 * out of the fast path.
		 *
		 * If an object is a Proxy we also can't avoid side effects
		 * so abandon.
		 */
		/* XXX: non-callable .toJSON() doesn't need to cause an abort
		 * but does at the moment, probably not worth fixing.
		 */
		if (duk_hobject_hasprop_raw(js_ctx->thr, obj, DUK_HTHREAD_STRING_TO_JSON(js_ctx->thr)) ||
		    DUK_HOBJECT_IS_PROXY(obj)) {
			DUK_DD(DUK_DDPRINT("object has a .toJSON property or object is a Proxy, abort fast path"));
			goto abort_fastpath;
		}

		/* We could use a switch-case for the class number but it turns out
		 * a small if-else ladder on class masks is better.  The if-ladder
		 * should be in order of relevancy.
		 */

		/* XXX: move masks to js_ctx? they don't change during one
		 * fast path invocation.
		 */
		DUK_ASSERT(DUK_HOBJECT_CLASS_MAX <= 31);
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
		if (js_ctx->flag_ext_custom_or_compatible) {
			c_all = DUK_HOBJECT_CMASK_ALL;
			c_array = DUK_HOBJECT_CMASK_ARRAY;
			c_unbox = DUK_HOBJECT_CMASK_NUMBER | DUK_HOBJECT_CMASK_STRING | DUK_HOBJECT_CMASK_BOOLEAN |
			          DUK_HOBJECT_CMASK_POINTER; /* Symbols are not unboxed. */
			c_func = DUK_HOBJECT_CMASK_FUNCTION;
			c_bufobj = DUK_HOBJECT_CMASK_ALL_BUFOBJS;
			c_undef = 0;
			c_abort = 0;
			c_object = c_all & ~(c_array | c_unbox | c_func | c_bufobj | c_undef | c_abort);
		} else
#endif
		{
			c_all = DUK_HOBJECT_CMASK_ALL;
			c_array = DUK_HOBJECT_CMASK_ARRAY;
			c_unbox = DUK_HOBJECT_CMASK_NUMBER | DUK_HOBJECT_CMASK_STRING |
			          DUK_HOBJECT_CMASK_BOOLEAN; /* Symbols are not unboxed. */
			c_func = 0;
			c_bufobj = 0;
			c_undef = DUK_HOBJECT_CMASK_FUNCTION | DUK_HOBJECT_CMASK_POINTER;
			/* As the fast path doesn't currently properly support
			 * duk_hbufobj virtual properties, abort fast path if
			 * we encounter them in plain JSON mode.
			 */
			c_abort = DUK_HOBJECT_CMASK_ALL_BUFOBJS;
			c_object = c_all & ~(c_array | c_unbox | c_func | c_bufobj | c_undef | c_abort);
		}

		c_bit = (duk_uint32_t) DUK_HOBJECT_GET_CLASS_MASK(obj);
		if (c_bit & c_object) {
			/* All other object types. */
			DUK__EMIT_1(js_ctx, DUK_ASC_LCURLY);

			/* A non-Array object should not have an array part in practice.
			 * But since it is supported internally (and perhaps used at some
			 * point), check and abandon if that's the case.
			 */
			if (DUK_HOBJECT_HAS_ARRAY_PART(obj)) {
				DUK_DD(DUK_DDPRINT("non-Array object has array part, abort fast path"));
				goto abort_fastpath;
			}

			for (i = 0; i < (duk_uint_fast32_t) DUK_HOBJECT_GET_ENEXT(obj); i++) {
				duk_hstring *k;
				duk_size_t prev_size;

				k = DUK_HOBJECT_E_GET_KEY(js_ctx->thr->heap, obj, i);
				if (!k) {
					continue;
				}
				if (DUK_HSTRING_HAS_ARRIDX(k)) {
					/* If an object has array index keys we would need
					 * to sort them into the ES2015 enumeration order to
					 * be consistent with the slow path.  Abort the fast
					 * path and handle in the slow path for now.
					 */
					DUK_DD(DUK_DDPRINT("property key is an array index, abort fast path"));
					goto abort_fastpath;
				}
				if (!DUK_HOBJECT_E_SLOT_IS_ENUMERABLE(js_ctx->thr->heap, obj, i)) {
					continue;
				}
				if (DUK_HOBJECT_E_SLOT_IS_ACCESSOR(js_ctx->thr->heap, obj, i)) {
					/* Getter might have arbitrary side effects,
					 * so bail out.
					 */
					DUK_DD(DUK_DDPRINT("property is an accessor, abort fast path"));
					goto abort_fastpath;
				}
				if (DUK_UNLIKELY(DUK_HSTRING_HAS_SYMBOL(k))) {
					continue;
				}

				tv_val = DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(js_ctx->thr->heap, obj, i);

				prev_size = DUK_BW_GET_SIZE(js_ctx->thr, &js_ctx->bw);
				if (DUK_UNLIKELY(js_ctx->h_gap != NULL)) {
					duk__json_enc_newline_indent(js_ctx, js_ctx->recursion_depth);
					duk__json_enc_key_autoquote(js_ctx, k);
					DUK__EMIT_2(js_ctx, DUK_ASC_COLON, DUK_ASC_SPACE);
				} else {
					duk__json_enc_key_autoquote(js_ctx, k);
					DUK__EMIT_1(js_ctx, DUK_ASC_COLON);
				}

				if (duk__json_stringify_fast_value(js_ctx, tv_val) == 0) {
					DUK_DD(DUK_DDPRINT("prop value not supported, rewind key and colon"));
					DUK_BW_SET_SIZE(js_ctx->thr, &js_ctx->bw, prev_size);
				} else {
					DUK__EMIT_1(js_ctx, DUK_ASC_COMMA);
					emitted = 1;
				}
			}

			/* If any non-Array value had enumerable virtual own
			 * properties, they should be serialized here (actually,
			 * before the explicit properties).  Standard types don't.
			 */

			if (emitted) {
				DUK_ASSERT(*((duk_uint8_t *) DUK_BW_GET_PTR(js_ctx->thr, &js_ctx->bw) - 1) == DUK_ASC_COMMA);
				DUK__UNEMIT_1(js_ctx); /* eat trailing comma */
				if (DUK_UNLIKELY(js_ctx->h_gap != NULL)) {
					DUK_ASSERT(js_ctx->recursion_depth >= 1);
					duk__json_enc_newline_indent(js_ctx, js_ctx->recursion_depth - 1U);
				}
			}
			DUK__EMIT_1(js_ctx, DUK_ASC_RCURLY);
		} else if (c_bit & c_array) {
			duk_uint_fast32_t arr_len;
			duk_uint_fast32_t asize;

			DUK__EMIT_1(js_ctx, DUK_ASC_LBRACKET);

			/* Assume arrays are dense in the fast path. */
			if (!DUK_HOBJECT_HAS_ARRAY_PART(obj)) {
				DUK_DD(DUK_DDPRINT("Array object is sparse, abort fast path"));
				goto abort_fastpath;
			}

			arr_len = (duk_uint_fast32_t) ((duk_harray *) obj)->length;
			asize = (duk_uint_fast32_t) DUK_HOBJECT_GET_ASIZE(obj);
			/* Array part may be larger than 'length'; if so, iterate
			 * only up to array 'length'.  Array part may also be smaller
			 * than 'length' in some cases.
			 */
			for (i = 0; i < arr_len; i++) {
				duk_tval *tv_arrval;
				duk_hstring *h_tmp;
				duk_bool_t has_inherited;

				if (DUK_UNLIKELY(js_ctx->h_gap != NULL)) {
					duk__json_enc_newline_indent(js_ctx, js_ctx->recursion_depth);
				}

				if (DUK_LIKELY(i < asize)) {
					tv_arrval = DUK_HOBJECT_A_GET_VALUE_PTR(js_ctx->thr->heap, obj, i);
					if (DUK_LIKELY(!DUK_TVAL_IS_UNUSED(tv_arrval))) {
						/* Expected case: element is present. */
						if (duk__json_stringify_fast_value(js_ctx, tv_arrval) == 0) {
							DUK__EMIT_STRIDX(js_ctx, DUK_STRIDX_LC_NULL);
						}
						goto elem_done;
					}
				}

				/* Gap in array; check for inherited property,
				 * bail out if one exists.  This should be enough
				 * to support gappy arrays for all practical code.
				 */

				h_tmp = duk_push_uint_to_hstring(js_ctx->thr, (duk_uint_t) i);
				has_inherited = duk_hobject_hasprop_raw(js_ctx->thr, obj, h_tmp);
				duk_pop(js_ctx->thr);
				if (has_inherited) {
					DUK_D(DUK_DPRINT("gap in array, conflicting inherited property, abort fast path"));
					goto abort_fastpath;
				}

				/* Ordinary gap, undefined encodes to 'null' in
				 * standard JSON, but JX/JC use their form for
				 * undefined to better preserve the typing.
				 */
				DUK_D(DUK_DPRINT("gap in array, no conflicting inherited property, remain on fast path"));
#if defined(DUK_USE_JX)
				DUK__EMIT_STRIDX(js_ctx, js_ctx->stridx_custom_undefined);
#else
				DUK__EMIT_STRIDX(js_ctx, DUK_STRIDX_LC_NULL);
#endif
				/* fall through */

			elem_done:
				DUK__EMIT_1(js_ctx, DUK_ASC_COMMA);
				emitted = 1;
			}

			if (emitted) {
				DUK_ASSERT(*((duk_uint8_t *) DUK_BW_GET_PTR(js_ctx->thr, &js_ctx->bw) - 1) == DUK_ASC_COMMA);
				DUK__UNEMIT_1(js_ctx); /* eat trailing comma */
				if (DUK_UNLIKELY(js_ctx->h_gap != NULL)) {
					DUK_ASSERT(js_ctx->recursion_depth >= 1);
					duk__json_enc_newline_indent(js_ctx, js_ctx->recursion_depth - 1U);
				}
			}
			DUK__EMIT_1(js_ctx, DUK_ASC_RBRACKET);
		} else if (c_bit & c_unbox) {
			/* Certain boxed types are required to go through
			 * automatic unboxing.  Rely on internal value being
			 * sane (to avoid infinite recursion).
			 */
			DUK_ASSERT((c_bit & DUK_HOBJECT_CMASK_SYMBOL) == 0); /* Symbols are not unboxed. */

#if 1
			/* The code below is incorrect if .toString() or .valueOf() have
			 * have been overridden.  The correct approach would be to look up
			 * the method(s) and if they resolve to the built-in function we
			 * can safely bypass it and look up the internal value directly.
			 * Unimplemented for now, abort fast path for boxed values.
			 */
			goto abort_fastpath;
#else /* disabled */
			/* Disabled until fixed, see above. */
			duk_tval *tv_internal;

			DUK_DD(DUK_DDPRINT("auto unboxing in fast path"));

			tv_internal = duk_hobject_get_internal_value_tval_ptr(js_ctx->thr->heap, obj);
			DUK_ASSERT(tv_internal != NULL);
			DUK_ASSERT(DUK_TVAL_IS_STRING(tv_internal) || DUK_TVAL_IS_NUMBER(tv_internal) ||
			           DUK_TVAL_IS_BOOLEAN(tv_internal) || DUK_TVAL_IS_POINTER(tv_internal));

			tv = tv_internal;
			DUK_ASSERT(js_ctx->recursion_depth > 0);
			js_ctx->recursion_depth--; /* required to keep recursion depth correct */
			goto restart_match;
#endif /* disabled */
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
		} else if (c_bit & c_func) {
			DUK__EMIT_STRIDX(js_ctx, js_ctx->stridx_custom_function);
#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
		} else if (c_bit & c_bufobj) {
			duk__json_enc_bufobj(js_ctx, (duk_hbufobj *) obj);
#endif
#endif
		} else if (c_bit & c_abort) {
			DUK_DD(DUK_DDPRINT("abort fast path for unsupported type"));
			goto abort_fastpath;
		} else {
			DUK_ASSERT((c_bit & c_undef) != 0);

			/* Must decrease recursion depth before returning. */
			DUK_ASSERT(js_ctx->recursion_depth > 0);
			DUK_ASSERT(js_ctx->recursion_depth <= js_ctx->recursion_limit);
			js_ctx->recursion_depth--;
			goto emit_undefined;
		}

		DUK_ASSERT(js_ctx->recursion_depth > 0);
		DUK_ASSERT(js_ctx->recursion_depth <= js_ctx->recursion_limit);
		js_ctx->recursion_depth--;
		break;
	}
	case DUK_TAG_BUFFER: {
		/* Plain buffers are treated like Uint8Arrays: they have
		 * enumerable indices.  Other virtual properties are not
		 * enumerable, and inherited properties are not serialized.
		 * However, there can be a replacer (not relevant here) or
		 * a .toJSON() method (which we need to check for explicitly).
		 */

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
		if (duk_hobject_hasprop_raw(js_ctx->thr,
		                            js_ctx->thr->builtins[DUK_BIDX_UINT8ARRAY_PROTOTYPE],
		                            DUK_HTHREAD_STRING_TO_JSON(js_ctx->thr))) {
			DUK_DD(DUK_DDPRINT("value is a plain buffer and there's an inherited .toJSON, abort fast path"));
			goto abort_fastpath;
		}
#endif

#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
		if (js_ctx->flag_ext_custom_or_compatible) {
			duk__json_enc_buffer_jx_jc(js_ctx, DUK_TVAL_GET_BUFFER(tv));
			break;
		}
#endif

		/* Plain buffers mimic Uint8Arrays, and have enumerable index
		 * properties.
		 */
		duk__json_enc_buffer_json_fastpath(js_ctx, DUK_TVAL_GET_BUFFER(tv));
		break;
	}
	case DUK_TAG_POINTER: {
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
		if (js_ctx->flag_ext_custom_or_compatible) {
			duk__json_enc_pointer(js_ctx, DUK_TVAL_GET_POINTER(tv));
			break;
		} else {
			goto emit_undefined;
		}
#else
		goto emit_undefined;
#endif
	}
	case DUK_TAG_LIGHTFUNC: {
		/* A lightfunc might also inherit a .toJSON() so just bail out. */
		/* XXX: Could just lookup .toJSON() and continue in fast path,
		 * as it would almost never be defined.
		 */
		DUK_DD(DUK_DDPRINT("value is a lightfunc, abort fast path"));
		goto abort_fastpath;
	}
#if defined(DUK_USE_FASTINT)
	case DUK_TAG_FASTINT: {
		/* Number serialization has a significant impact relative to
		 * other fast path code, so careful fast path for fastints.
		 */
		duk__json_enc_fastint_tval(js_ctx, tv);
		break;
	}
#endif
	default: {
		/* XXX: A fast path for usual integers would be useful when
		 * fastint support is not enabled.
		 */
		DUK_ASSERT(!DUK_TVAL_IS_UNUSED(tv));
		DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv));

		/* XXX: Stack discipline is annoying, could be changed in numconv. */
		duk_push_tval(js_ctx->thr, tv);
		duk__json_enc_double(js_ctx);
		duk_pop(js_ctx->thr);

#if 0
		/* Could also rely on native sprintf(), but it will handle
		 * values like NaN, Infinity, -0, exponent notation etc in
		 * a JSON-incompatible way.
		 */
		duk_double_t d;
		char buf[64];

		DUK_ASSERT(DUK_TVAL_IS_DOUBLE(tv));
		d = DUK_TVAL_GET_DOUBLE(tv);
		DUK_SPRINTF(buf, "%lg", d);
		DUK__EMIT_CSTR(js_ctx, buf);
#endif
	}
	}
	return 1; /* not undefined */

emit_undefined:
	return 0; /* value was undefined/unsupported */

abort_fastpath:
	/* Error message doesn't matter: the error is ignored anyway. */
	DUK_DD(DUK_DDPRINT("aborting fast path"));
	DUK_ERROR_INTERNAL(js_ctx->thr);
	DUK_WO_NORETURN(return 0;);
}

DUK_LOCAL duk_ret_t duk__json_stringify_fast(duk_hthread *thr, void *udata) {
	duk_json_enc_ctx *js_ctx;
	duk_tval *tv;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(udata != NULL);

	js_ctx = (duk_json_enc_ctx *) udata;
	DUK_ASSERT(js_ctx != NULL);

	tv = DUK_GET_TVAL_NEGIDX(thr, -1);
	if (duk__json_stringify_fast_value(js_ctx, tv) == 0) {
		DUK_DD(DUK_DDPRINT("top level value not supported, fail fast path"));
		DUK_DCERROR_TYPE_INVALID_ARGS(thr); /* Error message is ignored, so doesn't matter. */
	}

	return 0;
}
#endif /* DUK_USE_JSON_STRINGIFY_FASTPATH */

/*
 *  Top level wrappers
 */

DUK_INTERNAL
void duk_bi_json_parse_helper(duk_hthread *thr, duk_idx_t idx_value, duk_idx_t idx_reviver, duk_small_uint_t flags) {
	duk_json_dec_ctx js_ctx_alloc;
	duk_json_dec_ctx *js_ctx = &js_ctx_alloc;
	duk_hstring *h_text;
#if defined(DUK_USE_ASSERTIONS)
	duk_idx_t entry_top = duk_get_top(thr);
#endif

	/* negative top-relative indices not allowed now */
	DUK_ASSERT(idx_value == DUK_INVALID_INDEX || idx_value >= 0);
	DUK_ASSERT(idx_reviver == DUK_INVALID_INDEX || idx_reviver >= 0);

	DUK_DDD(DUK_DDDPRINT("JSON parse start: text=%!T, reviver=%!T, flags=0x%08lx, stack_top=%ld",
	                     (duk_tval *) duk_get_tval(thr, idx_value),
	                     (duk_tval *) duk_get_tval(thr, idx_reviver),
	                     (unsigned long) flags,
	                     (long) duk_get_top(thr)));

	duk_memzero(&js_ctx_alloc, sizeof(js_ctx_alloc));
	js_ctx->thr = thr;
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	/* nothing now */
#endif
	js_ctx->recursion_limit = DUK_USE_JSON_DEC_RECLIMIT;
	DUK_ASSERT(js_ctx->recursion_depth == 0);

	/* Flag handling currently assumes that flags are consistent.  This is OK
	 * because the call sites are now strictly controlled.
	 */

	js_ctx->flags = flags;
#if defined(DUK_USE_JX)
	js_ctx->flag_ext_custom = flags & DUK_JSON_FLAG_EXT_CUSTOM;
#endif
#if defined(DUK_USE_JC)
	js_ctx->flag_ext_compatible = flags & DUK_JSON_FLAG_EXT_COMPATIBLE;
#endif
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
	js_ctx->flag_ext_custom_or_compatible = flags & (DUK_JSON_FLAG_EXT_CUSTOM | DUK_JSON_FLAG_EXT_COMPATIBLE);
#endif

	h_text = duk_to_hstring(thr, idx_value); /* coerce in-place; rejects Symbols */
	DUK_ASSERT(h_text != NULL);

	/* JSON parsing code is allowed to read [p_start,p_end]: p_end is
	 * valid and points to the string NUL terminator (which is always
	 * guaranteed for duk_hstrings.
	 */
	js_ctx->p_start = (const duk_uint8_t *) DUK_HSTRING_GET_DATA(h_text);
	js_ctx->p = js_ctx->p_start;
	js_ctx->p_end = ((const duk_uint8_t *) DUK_HSTRING_GET_DATA(h_text)) + DUK_HSTRING_GET_BYTELEN(h_text);
	DUK_ASSERT(*(js_ctx->p_end) == 0x00);

	duk__json_dec_value(js_ctx); /* -> [ ... value ] */
	DUK_ASSERT(js_ctx->recursion_depth == 0);

	/* Trailing whitespace has been eaten by duk__json_dec_value(), so if
	 * we're not at end of input here, it's a SyntaxError.
	 */

	if (js_ctx->p != js_ctx->p_end) {
		duk__json_dec_syntax_error(js_ctx);
	}

	if (duk_is_callable(thr, idx_reviver)) {
		DUK_DDD(DUK_DDDPRINT("applying reviver: %!T", (duk_tval *) duk_get_tval(thr, idx_reviver)));

		js_ctx->idx_reviver = idx_reviver;

		duk_push_object(thr);
		duk_dup_m2(thr); /* -> [ ... val root val ] */
		duk_put_prop_stridx_short(thr, -2, DUK_STRIDX_EMPTY_STRING); /* default attrs ok */
		duk_push_hstring_stridx(thr, DUK_STRIDX_EMPTY_STRING); /* -> [ ... val root "" ] */

		DUK_DDD(DUK_DDDPRINT("start reviver walk, root=%!T, name=%!T",
		                     (duk_tval *) duk_get_tval(thr, -2),
		                     (duk_tval *) duk_get_tval(thr, -1)));

		DUK_ASSERT(js_ctx->recursion_depth == 0);
		duk__json_dec_reviver_walk(js_ctx); /* [ ... val root "" ] -> [ ... val val' ] */
		DUK_ASSERT(js_ctx->recursion_depth == 0);
		duk_remove_m2(thr); /* -> [ ... val' ] */
	} else {
		DUK_DDD(
		    DUK_DDDPRINT("reviver does not exist or is not callable: %!T", (duk_tval *) duk_get_tval(thr, idx_reviver)));
	}

	/* Final result is at stack top. */

	DUK_DDD(DUK_DDDPRINT("JSON parse end: text=%!T, reviver=%!T, flags=0x%08lx, result=%!T, stack_top=%ld",
	                     (duk_tval *) duk_get_tval(thr, idx_value),
	                     (duk_tval *) duk_get_tval(thr, idx_reviver),
	                     (unsigned long) flags,
	                     (duk_tval *) duk_get_tval(thr, -1),
	                     (long) duk_get_top(thr)));

	DUK_ASSERT(duk_get_top(thr) == entry_top + 1);
}

DUK_INTERNAL
void duk_bi_json_stringify_helper(duk_hthread *thr,
                                  duk_idx_t idx_value,
                                  duk_idx_t idx_replacer,
                                  duk_idx_t idx_space,
                                  duk_small_uint_t flags) {
	duk_json_enc_ctx js_ctx_alloc;
	duk_json_enc_ctx *js_ctx = &js_ctx_alloc;
	duk_hobject *h;
	duk_idx_t idx_holder;
	duk_idx_t entry_top;

	/* negative top-relative indices not allowed now */
	DUK_ASSERT(idx_value == DUK_INVALID_INDEX || idx_value >= 0);
	DUK_ASSERT(idx_replacer == DUK_INVALID_INDEX || idx_replacer >= 0);
	DUK_ASSERT(idx_space == DUK_INVALID_INDEX || idx_space >= 0);

	DUK_DDD(DUK_DDDPRINT("JSON stringify start: value=%!T, replacer=%!T, space=%!T, flags=0x%08lx, stack_top=%ld",
	                     (duk_tval *) duk_get_tval(thr, idx_value),
	                     (duk_tval *) duk_get_tval(thr, idx_replacer),
	                     (duk_tval *) duk_get_tval(thr, idx_space),
	                     (unsigned long) flags,
	                     (long) duk_get_top(thr)));

	entry_top = duk_get_top(thr);

	/*
	 *  Context init
	 */

	duk_memzero(&js_ctx_alloc, sizeof(js_ctx_alloc));
	js_ctx->thr = thr;
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	js_ctx->h_replacer = NULL;
	js_ctx->h_gap = NULL;
#endif
	js_ctx->idx_proplist = -1;

	/* Flag handling currently assumes that flags are consistent.  This is OK
	 * because the call sites are now strictly controlled.
	 */

	js_ctx->flags = flags;
	js_ctx->flag_ascii_only = flags & DUK_JSON_FLAG_ASCII_ONLY;
	js_ctx->flag_avoid_key_quotes = flags & DUK_JSON_FLAG_AVOID_KEY_QUOTES;
#if defined(DUK_USE_JX)
	js_ctx->flag_ext_custom = flags & DUK_JSON_FLAG_EXT_CUSTOM;
#endif
#if defined(DUK_USE_JC)
	js_ctx->flag_ext_compatible = flags & DUK_JSON_FLAG_EXT_COMPATIBLE;
#endif
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
	js_ctx->flag_ext_custom_or_compatible = flags & (DUK_JSON_FLAG_EXT_CUSTOM | DUK_JSON_FLAG_EXT_COMPATIBLE);
#endif

	/* The #if defined() clutter here handles the JX/JC enable/disable
	 * combinations properly.
	 */
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
	js_ctx->stridx_custom_undefined = DUK_STRIDX_LC_NULL; /* standard JSON; array gaps */
#if defined(DUK_USE_JX)
	if (flags & DUK_JSON_FLAG_EXT_CUSTOM) {
		js_ctx->stridx_custom_undefined = DUK_STRIDX_LC_UNDEFINED;
		js_ctx->stridx_custom_nan = DUK_STRIDX_NAN;
		js_ctx->stridx_custom_neginf = DUK_STRIDX_MINUS_INFINITY;
		js_ctx->stridx_custom_posinf = DUK_STRIDX_INFINITY;
		js_ctx->stridx_custom_function =
		    (flags & DUK_JSON_FLAG_AVOID_KEY_QUOTES) ? DUK_STRIDX_JSON_EXT_FUNCTION2 : DUK_STRIDX_JSON_EXT_FUNCTION1;
	}
#endif /* DUK_USE_JX */
#if defined(DUK_USE_JX) && defined(DUK_USE_JC)
	else
#endif /* DUK_USE_JX && DUK_USE_JC */
#if defined(DUK_USE_JC)
	    if (js_ctx->flags & DUK_JSON_FLAG_EXT_COMPATIBLE) {
		js_ctx->stridx_custom_undefined = DUK_STRIDX_JSON_EXT_UNDEFINED;
		js_ctx->stridx_custom_nan = DUK_STRIDX_JSON_EXT_NAN;
		js_ctx->stridx_custom_neginf = DUK_STRIDX_JSON_EXT_NEGINF;
		js_ctx->stridx_custom_posinf = DUK_STRIDX_JSON_EXT_POSINF;
		js_ctx->stridx_custom_function = DUK_STRIDX_JSON_EXT_FUNCTION1;
	}
#endif /* DUK_USE_JC */
#endif /* DUK_USE_JX || DUK_USE_JC */

#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
	if (js_ctx->flags & (DUK_JSON_FLAG_EXT_CUSTOM | DUK_JSON_FLAG_EXT_COMPATIBLE)) {
		DUK_ASSERT(js_ctx->mask_for_undefined == 0); /* already zero */
	} else
#endif /* DUK_USE_JX || DUK_USE_JC */
	{
		/* Plain buffer is treated like ArrayBuffer and serialized.
		 * Lightfuncs are treated like objects, but JSON explicitly
		 * skips serializing Function objects so we can just reject
		 * lightfuncs here.
		 */
		js_ctx->mask_for_undefined = DUK_TYPE_MASK_UNDEFINED | DUK_TYPE_MASK_POINTER | DUK_TYPE_MASK_LIGHTFUNC;
	}

	DUK_BW_INIT_PUSHBUF(thr, &js_ctx->bw, DUK__JSON_STRINGIFY_BUFSIZE);

	js_ctx->idx_loop = duk_push_bare_object(thr);
	DUK_ASSERT(js_ctx->idx_loop >= 0);

	/* [ ... buf loop ] */

	/*
	 *  Process replacer/proplist (2nd argument to JSON.stringify)
	 */

	h = duk_get_hobject(thr, idx_replacer);
	if (h != NULL) {
		if (DUK_HOBJECT_IS_CALLABLE(h)) {
			js_ctx->h_replacer = h;
		} else if (duk_js_isarray_hobject(h)) {
			/* Here the specification requires correct array index enumeration
			 * which is a bit tricky for sparse arrays (it is handled by the
			 * enum setup code).  We now enumerate ancestors too, although the
			 * specification is not very clear on whether that is required.
			 */

			duk_uarridx_t plist_idx = 0;
			duk_small_uint_t enum_flags;

			js_ctx->idx_proplist = duk_push_bare_array(thr);

			enum_flags = DUK_ENUM_ARRAY_INDICES_ONLY | DUK_ENUM_SORT_ARRAY_INDICES; /* expensive flag */
			duk_enum(thr, idx_replacer, enum_flags);
			while (duk_next(thr, -1 /*enum_index*/, 1 /*get_value*/)) {
				/* [ ... proplist enum_obj key val ] */
				if (duk__json_enc_allow_into_proplist(duk_get_tval(thr, -1))) {
					/* XXX: duplicates should be eliminated here */
					DUK_DDD(DUK_DDDPRINT("proplist enum: key=%!T, val=%!T --> accept",
					                     (duk_tval *) duk_get_tval(thr, -2),
					                     (duk_tval *) duk_get_tval(thr, -1)));
					duk_to_string(thr, -1); /* extra coercion of strings is OK */
					duk_put_prop_index(thr, -4, plist_idx); /* -> [ ... proplist enum_obj key ] */
					plist_idx++;
					duk_pop(thr);
				} else {
					DUK_DDD(DUK_DDDPRINT("proplist enum: key=%!T, val=%!T --> reject",
					                     (duk_tval *) duk_get_tval(thr, -2),
					                     (duk_tval *) duk_get_tval(thr, -1)));
					duk_pop_2(thr);
				}
			}
			duk_pop(thr); /* pop enum */

			/* [ ... proplist ] */
		}
	}

	/* [ ... buf loop (proplist) ] */

	/*
	 *  Process space (3rd argument to JSON.stringify)
	 */

	h = duk_get_hobject(thr, idx_space);
	if (h != NULL) {
		duk_small_uint_t c = DUK_HOBJECT_GET_CLASS_NUMBER(h);
		if (c == DUK_HOBJECT_CLASS_NUMBER) {
			duk_to_number(thr, idx_space);
		} else if (c == DUK_HOBJECT_CLASS_STRING) {
			duk_to_string(thr, idx_space);
		}
	}

	if (duk_is_number(thr, idx_space)) {
		duk_small_int_t nspace;
		/* spaces[] must be static to allow initializer with old compilers like BCC */
		static const char spaces[10] = {
			DUK_ASC_SPACE, DUK_ASC_SPACE, DUK_ASC_SPACE, DUK_ASC_SPACE, DUK_ASC_SPACE,
			DUK_ASC_SPACE, DUK_ASC_SPACE, DUK_ASC_SPACE, DUK_ASC_SPACE, DUK_ASC_SPACE
		}; /* XXX: helper */

		/* ToInteger() coercion; NaN -> 0, infinities are clamped to 0 and 10 */
		nspace = (duk_small_int_t) duk_to_int_clamped(thr, idx_space, 0 /*minval*/, 10 /*maxval*/);
		DUK_ASSERT(nspace >= 0 && nspace <= 10);

		duk_push_lstring(thr, spaces, (duk_size_t) nspace);
		js_ctx->h_gap = duk_known_hstring(thr, -1);
		DUK_ASSERT(js_ctx->h_gap != NULL);
	} else if (duk_is_string_notsymbol(thr, idx_space)) {
		duk_dup(thr, idx_space);
		duk_substring(thr, -1, 0, 10); /* clamp to 10 chars */
		js_ctx->h_gap = duk_known_hstring(thr, -1);
	} else {
		/* nop */
	}

	if (js_ctx->h_gap != NULL) {
		/* If gap is empty, behave as if not given at all.  Check
		 * against byte length because character length is more
		 * expensive.
		 */
		if (DUK_HSTRING_GET_BYTELEN(js_ctx->h_gap) == 0) {
			js_ctx->h_gap = NULL;
		}
	}

	/* [ ... buf loop (proplist) (gap) ] */

	/*
	 *  Fast path: assume no mutation, iterate object property tables
	 *  directly; bail out if that assumption doesn't hold.
	 */

#if defined(DUK_USE_JSON_STRINGIFY_FASTPATH)
	if (js_ctx->h_replacer == NULL && /* replacer is a mutation risk */
	    js_ctx->idx_proplist == -1) { /* proplist is very rare */
		duk_int_t pcall_rc;
		duk_small_uint_t prev_ms_base_flags;

		DUK_DD(DUK_DDPRINT("try JSON.stringify() fast path"));

		/* Use recursion_limit to ensure we don't overwrite js_ctx->visiting[]
		 * array so we don't need two counter checks in the fast path.  The
		 * slow path has a much larger recursion limit which we'll use if
		 * necessary.
		 */
		DUK_ASSERT(DUK_USE_JSON_ENC_RECLIMIT >= DUK_JSON_ENC_LOOPARRAY);
		js_ctx->recursion_limit = DUK_JSON_ENC_LOOPARRAY;
		DUK_ASSERT(js_ctx->recursion_depth == 0);

		/* Execute the fast path in a protected call.  If any error is thrown,
		 * fall back to the slow path.  This includes e.g. recursion limit
		 * because the fast path has a smaller recursion limit (and simpler,
		 * limited loop detection).
		 */

		duk_dup(thr, idx_value);

		/* Must prevent finalizers which may have arbitrary side effects. */
		prev_ms_base_flags = thr->heap->ms_base_flags;
		thr->heap->ms_base_flags |= DUK_MS_FLAG_NO_OBJECT_COMPACTION; /* Avoid attempt to compact any objects. */
		thr->heap->pf_prevent_count++; /* Prevent finalizers. */
		DUK_ASSERT(thr->heap->pf_prevent_count != 0); /* Wrap. */

		pcall_rc = duk_safe_call(thr, duk__json_stringify_fast, (void *) js_ctx /*udata*/, 1 /*nargs*/, 0 /*nret*/);

		DUK_ASSERT(thr->heap->pf_prevent_count > 0);
		thr->heap->pf_prevent_count--;
		thr->heap->ms_base_flags = prev_ms_base_flags;

		if (pcall_rc == DUK_EXEC_SUCCESS) {
			DUK_DD(DUK_DDPRINT("fast path successful"));
			DUK_BW_PUSH_AS_STRING(thr, &js_ctx->bw);
			goto replace_finished;
		}

		/* We come here for actual aborts (like encountering .toJSON())
		 * but also for recursion/loop errors.  Bufwriter size can be
		 * kept because we'll probably need at least as much as we've
		 * allocated so far.
		 */
		DUK_D(DUK_DPRINT("fast path failed, serialize using slow path instead"));
		DUK_BW_RESET_SIZE(thr, &js_ctx->bw);
		js_ctx->recursion_depth = 0;
	}
#endif

	/*
	 *  Create wrapper object and serialize
	 */

	idx_holder = duk_push_object(thr);
	duk_dup(thr, idx_value);
	duk_put_prop_stridx_short(thr, -2, DUK_STRIDX_EMPTY_STRING);

	DUK_DDD(DUK_DDDPRINT("before: flags=0x%08lx, loop=%!T, replacer=%!O, "
	                     "proplist=%!T, gap=%!O, holder=%!T",
	                     (unsigned long) js_ctx->flags,
	                     (duk_tval *) duk_get_tval(thr, js_ctx->idx_loop),
	                     (duk_heaphdr *) js_ctx->h_replacer,
	                     (duk_tval *) (js_ctx->idx_proplist >= 0 ? duk_get_tval(thr, js_ctx->idx_proplist) : NULL),
	                     (duk_heaphdr *) js_ctx->h_gap,
	                     (duk_tval *) duk_get_tval(thr, -1)));

	/* serialize the wrapper with empty string key */

	duk_push_hstring_empty(thr);

	/* [ ... buf loop (proplist) (gap) holder "" ] */

	js_ctx->recursion_limit = DUK_USE_JSON_ENC_RECLIMIT;
	DUK_ASSERT(js_ctx->recursion_depth == 0);

	if (DUK_UNLIKELY(duk__json_enc_value(js_ctx, idx_holder) == 0)) { /* [ ... holder key ] -> [ ... holder ] */
		/* Result is undefined. */
		duk_push_undefined(thr);
	} else {
		/* Convert buffer to result string. */
		DUK_BW_PUSH_AS_STRING(thr, &js_ctx->bw);
	}

	DUK_DDD(DUK_DDDPRINT("after: flags=0x%08lx, loop=%!T, replacer=%!O, "
	                     "proplist=%!T, gap=%!O, holder=%!T",
	                     (unsigned long) js_ctx->flags,
	                     (duk_tval *) duk_get_tval(thr, js_ctx->idx_loop),
	                     (duk_heaphdr *) js_ctx->h_replacer,
	                     (duk_tval *) (js_ctx->idx_proplist >= 0 ? duk_get_tval(thr, js_ctx->idx_proplist) : NULL),
	                     (duk_heaphdr *) js_ctx->h_gap,
	                     (duk_tval *) duk_get_tval(thr, idx_holder)));

	/* The stack has a variable shape here, so force it to the
	 * desired one explicitly.
	 */

#if defined(DUK_USE_JSON_STRINGIFY_FASTPATH)
replace_finished:
#endif
	duk_replace(thr, entry_top);
	duk_set_top(thr, entry_top + 1);

	DUK_DDD(DUK_DDDPRINT("JSON stringify end: value=%!T, replacer=%!T, space=%!T, "
	                     "flags=0x%08lx, result=%!T, stack_top=%ld",
	                     (duk_tval *) duk_get_tval(thr, idx_value),
	                     (duk_tval *) duk_get_tval(thr, idx_replacer),
	                     (duk_tval *) duk_get_tval(thr, idx_space),
	                     (unsigned long) flags,
	                     (duk_tval *) duk_get_tval(thr, -1),
	                     (long) duk_get_top(thr)));

	DUK_ASSERT(duk_get_top(thr) == entry_top + 1);
}

#if defined(DUK_USE_JSON_BUILTIN)

/*
 *  Entry points
 */

DUK_INTERNAL duk_ret_t duk_bi_json_object_parse(duk_hthread *thr) {
	duk_bi_json_parse_helper(thr, 0 /*idx_value*/, 1 /*idx_replacer*/, 0 /*flags*/);
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_json_object_stringify(duk_hthread *thr) {
	duk_bi_json_stringify_helper(thr, 0 /*idx_value*/, 1 /*idx_replacer*/, 2 /*idx_space*/, 0 /*flags*/);
	return 1;
}

#endif /* DUK_USE_JSON_BUILTIN */

#endif /* DUK_USE_JSON_SUPPORT */
