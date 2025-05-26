/*
 *  Defines for JSON, especially duk_bi_json.c.
 */

#if !defined(DUK_JSON_H_INCLUDED)
#define DUK_JSON_H_INCLUDED

/* Encoding/decoding flags */
#define DUK_JSON_FLAG_ASCII_ONLY       (1U << 0) /* escape any non-ASCII characters */
#define DUK_JSON_FLAG_AVOID_KEY_QUOTES (1U << 1) /* avoid key quotes when key is an ASCII Identifier */
#define DUK_JSON_FLAG_EXT_CUSTOM       (1U << 2) /* extended types: custom encoding */
#define DUK_JSON_FLAG_EXT_COMPATIBLE   (1U << 3) /* extended types: compatible encoding */

/* How much stack to require on entry to object/array encode */
#define DUK_JSON_ENC_REQSTACK 32

/* How much stack to require on entry to object/array decode */
#define DUK_JSON_DEC_REQSTACK 32

/* How large a loop detection stack to use */
#define DUK_JSON_ENC_LOOPARRAY 64

/* Encoding state.  Heap object references are all borrowed. */
typedef struct {
	duk_hthread *thr;
	duk_bufwriter_ctx bw; /* output bufwriter */
	duk_hobject *h_replacer; /* replacer function */
	duk_hstring *h_gap; /* gap (if empty string, NULL) */
	duk_idx_t idx_proplist; /* explicit PropertyList */
	duk_idx_t idx_loop; /* valstack index of loop detection object */
	duk_small_uint_t flags;
	duk_small_uint_t flag_ascii_only;
	duk_small_uint_t flag_avoid_key_quotes;
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
	duk_small_uint_t flag_ext_custom;
	duk_small_uint_t flag_ext_compatible;
	duk_small_uint_t flag_ext_custom_or_compatible;
#endif
	duk_uint_t recursion_depth;
	duk_uint_t recursion_limit;
	duk_uint_t mask_for_undefined; /* type bit mask: types which certainly produce 'undefined' */
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
	duk_small_uint_t stridx_custom_undefined;
	duk_small_uint_t stridx_custom_nan;
	duk_small_uint_t stridx_custom_neginf;
	duk_small_uint_t stridx_custom_posinf;
	duk_small_uint_t stridx_custom_function;
#endif
	duk_hobject *visiting[DUK_JSON_ENC_LOOPARRAY]; /* indexed by recursion_depth */
} duk_json_enc_ctx;

typedef struct {
	duk_hthread *thr;
	const duk_uint8_t *p;
	const duk_uint8_t *p_start;
	const duk_uint8_t *p_end;
	duk_idx_t idx_reviver;
	duk_small_uint_t flags;
#if defined(DUK_USE_JX) || defined(DUK_USE_JC)
	duk_small_uint_t flag_ext_custom;
	duk_small_uint_t flag_ext_compatible;
	duk_small_uint_t flag_ext_custom_or_compatible;
#endif
	duk_int_t recursion_depth;
	duk_int_t recursion_limit;
} duk_json_dec_ctx;

#endif /* DUK_JSON_H_INCLUDED */
