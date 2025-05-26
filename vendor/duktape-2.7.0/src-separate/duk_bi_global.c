/*
 *  Global object built-ins
 */

#include "duk_internal.h"

/*
 *  Encoding/decoding helpers
 */

/* XXX: Could add fast path (for each transform callback) with direct byte
 * lookups (no shifting) and no explicit check for x < 0x80 before table
 * lookup.
 */

/* Macros for creating and checking bitmasks for character encoding.
 * Bit number is a bit counterintuitive, but minimizes code size.
 */
#define DUK__MKBITS(a, b, c, d, e, f, g, h) \
	((duk_uint8_t) (((a) << 0) | ((b) << 1) | ((c) << 2) | ((d) << 3) | ((e) << 4) | ((f) << 5) | ((g) << 6) | ((h) << 7)))
#define DUK__CHECK_BITMASK(table, cp) ((table)[(cp) >> 3] & (1 << ((cp) &0x07)))

/* E5.1 Section 15.1.3.3: uriReserved + uriUnescaped + '#' */
DUK_LOCAL const duk_uint8_t duk__encode_uriunescaped_table[16] = {
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x00-0x0f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x10-0x1f */
	DUK__MKBITS(0, 1, 0, 1, 1, 0, 1, 1), DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), /* 0x20-0x2f */
	DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), DUK__MKBITS(1, 1, 1, 1, 0, 1, 0, 1), /* 0x30-0x3f */
	DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), /* 0x40-0x4f */
	DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), DUK__MKBITS(1, 1, 1, 0, 0, 0, 0, 1), /* 0x50-0x5f */
	DUK__MKBITS(0, 1, 1, 1, 1, 1, 1, 1), DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), /* 0x60-0x6f */
	DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), DUK__MKBITS(1, 1, 1, 0, 0, 0, 1, 0), /* 0x70-0x7f */
};

/* E5.1 Section 15.1.3.4: uriUnescaped */
DUK_LOCAL const duk_uint8_t duk__encode_uricomponent_unescaped_table[16] = {
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x00-0x0f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x10-0x1f */
	DUK__MKBITS(0, 1, 0, 0, 0, 0, 0, 1), DUK__MKBITS(1, 1, 1, 0, 0, 1, 1, 0), /* 0x20-0x2f */
	DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), DUK__MKBITS(1, 1, 0, 0, 0, 0, 0, 0), /* 0x30-0x3f */
	DUK__MKBITS(0, 1, 1, 1, 1, 1, 1, 1), DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), /* 0x40-0x4f */
	DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), DUK__MKBITS(1, 1, 1, 0, 0, 0, 0, 1), /* 0x50-0x5f */
	DUK__MKBITS(0, 1, 1, 1, 1, 1, 1, 1), DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), /* 0x60-0x6f */
	DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), DUK__MKBITS(1, 1, 1, 0, 0, 0, 1, 0), /* 0x70-0x7f */
};

/* E5.1 Section 15.1.3.1: uriReserved + '#' */
DUK_LOCAL const duk_uint8_t duk__decode_uri_reserved_table[16] = {
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x00-0x0f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x10-0x1f */
	DUK__MKBITS(0, 0, 0, 1, 1, 0, 1, 0), DUK__MKBITS(0, 0, 0, 1, 1, 0, 0, 1), /* 0x20-0x2f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 1, 1, 0, 1, 0, 1), /* 0x30-0x3f */
	DUK__MKBITS(1, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x40-0x4f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x50-0x5f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x60-0x6f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x70-0x7f */
};

/* E5.1 Section 15.1.3.2: empty */
DUK_LOCAL const duk_uint8_t duk__decode_uri_component_reserved_table[16] = {
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x00-0x0f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x10-0x1f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x20-0x2f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x30-0x3f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x40-0x4f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x50-0x5f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x60-0x6f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x70-0x7f */
};

#if defined(DUK_USE_SECTION_B)
/* E5.1 Section B.2.2, step 7. */
DUK_LOCAL const duk_uint8_t duk__escape_unescaped_table[16] = {
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x00-0x0f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), /* 0x10-0x1f */
	DUK__MKBITS(0, 0, 0, 0, 0, 0, 0, 0), DUK__MKBITS(0, 0, 1, 1, 0, 1, 1, 1), /* 0x20-0x2f */
	DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), DUK__MKBITS(1, 1, 0, 0, 0, 0, 0, 0), /* 0x30-0x3f */
	DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), /* 0x40-0x4f */
	DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), DUK__MKBITS(1, 1, 1, 0, 0, 0, 0, 1), /* 0x50-0x5f */
	DUK__MKBITS(0, 1, 1, 1, 1, 1, 1, 1), DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), /* 0x60-0x6f */
	DUK__MKBITS(1, 1, 1, 1, 1, 1, 1, 1), DUK__MKBITS(1, 1, 1, 0, 0, 0, 0, 0) /* 0x70-0x7f */
};
#endif /* DUK_USE_SECTION_B */

typedef struct {
	duk_hthread *thr;
	duk_hstring *h_str;
	duk_bufwriter_ctx bw;
	const duk_uint8_t *p;
	const duk_uint8_t *p_start;
	const duk_uint8_t *p_end;
} duk__transform_context;

typedef void (*duk__transform_callback)(duk__transform_context *tfm_ctx, const void *udata, duk_codepoint_t cp);

/* XXX: refactor and share with other code */
DUK_LOCAL duk_small_int_t duk__decode_hex_escape(const duk_uint8_t *p, duk_small_int_t n) {
	duk_small_int_t ch;
	duk_small_int_t t = 0;

	while (n > 0) {
		t = t * 16;
		ch = (duk_small_int_t) duk_hex_dectab[*p++];
		if (DUK_LIKELY(ch >= 0)) {
			t += ch;
		} else {
			return -1;
		}
		n--;
	}
	return t;
}

DUK_LOCAL int duk__transform_helper(duk_hthread *thr, duk__transform_callback callback, const void *udata) {
	duk__transform_context tfm_ctx_alloc;
	duk__transform_context *tfm_ctx = &tfm_ctx_alloc;
	duk_codepoint_t cp;

	tfm_ctx->thr = thr;

	tfm_ctx->h_str = duk_to_hstring(thr, 0);
	DUK_ASSERT(tfm_ctx->h_str != NULL);

	DUK_BW_INIT_PUSHBUF(thr, &tfm_ctx->bw, DUK_HSTRING_GET_BYTELEN(tfm_ctx->h_str)); /* initial size guess */

	tfm_ctx->p_start = DUK_HSTRING_GET_DATA(tfm_ctx->h_str);
	tfm_ctx->p_end = tfm_ctx->p_start + DUK_HSTRING_GET_BYTELEN(tfm_ctx->h_str);
	tfm_ctx->p = tfm_ctx->p_start;

	while (tfm_ctx->p < tfm_ctx->p_end) {
		cp = (duk_codepoint_t) duk_unicode_decode_xutf8_checked(thr, &tfm_ctx->p, tfm_ctx->p_start, tfm_ctx->p_end);
		callback(tfm_ctx, udata, cp);
	}

	DUK_BW_COMPACT(thr, &tfm_ctx->bw);

	(void) duk_buffer_to_string(thr, -1); /* Safe if transform is safe. */
	return 1;
}

DUK_LOCAL void duk__transform_callback_encode_uri(duk__transform_context *tfm_ctx, const void *udata, duk_codepoint_t cp) {
	duk_uint8_t xutf8_buf[DUK_UNICODE_MAX_XUTF8_LENGTH];
	duk_small_int_t len;
	duk_codepoint_t cp1, cp2;
	duk_small_int_t i, t;
	const duk_uint8_t *unescaped_table = (const duk_uint8_t *) udata;

	/* UTF-8 encoded bytes escaped as %xx%xx%xx... -> 3 * nbytes.
	 * Codepoint range is restricted so this is a slightly too large
	 * but doesn't matter.
	 */
	DUK_BW_ENSURE(tfm_ctx->thr, &tfm_ctx->bw, 3 * DUK_UNICODE_MAX_XUTF8_LENGTH);

	if (cp < 0) {
		goto uri_error;
	} else if ((cp < 0x80L) && DUK__CHECK_BITMASK(unescaped_table, cp)) {
		DUK_BW_WRITE_RAW_U8(tfm_ctx->thr, &tfm_ctx->bw, (duk_uint8_t) cp);
		return;
	} else if (cp >= 0xdc00L && cp <= 0xdfffL) {
		goto uri_error;
	} else if (cp >= 0xd800L && cp <= 0xdbffL) {
		/* Needs lookahead */
		if (duk_unicode_decode_xutf8(tfm_ctx->thr,
		                             &tfm_ctx->p,
		                             tfm_ctx->p_start,
		                             tfm_ctx->p_end,
		                             (duk_ucodepoint_t *) &cp2) == 0) {
			goto uri_error;
		}
		if (!(cp2 >= 0xdc00L && cp2 <= 0xdfffL)) {
			goto uri_error;
		}
		cp1 = cp;
		cp = (duk_codepoint_t) (((cp1 - 0xd800L) << 10) + (cp2 - 0xdc00L) + 0x10000L);
	} else if (cp > 0x10ffffL) {
		/* Although we can allow non-BMP characters (they'll decode
		 * back into surrogate pairs), we don't allow extended UTF-8
		 * characters; they would encode to URIs which won't decode
		 * back because of strict UTF-8 checks in URI decoding.
		 * (However, we could just as well allow them here.)
		 */
		goto uri_error;
	} else {
		/* Non-BMP characters within valid UTF-8 range: encode as is.
		 * They'll decode back into surrogate pairs if the escaped
		 * output is decoded.
		 */
		;
	}

	len = duk_unicode_encode_xutf8((duk_ucodepoint_t) cp, xutf8_buf);
	for (i = 0; i < len; i++) {
		t = (duk_small_int_t) xutf8_buf[i];
		DUK_BW_WRITE_RAW_U8_3(tfm_ctx->thr,
		                      &tfm_ctx->bw,
		                      DUK_ASC_PERCENT,
		                      (duk_uint8_t) duk_uc_nybbles[t >> 4],
		                      (duk_uint8_t) duk_uc_nybbles[t & 0x0f]);
	}

	return;

uri_error:
	DUK_ERROR_URI(tfm_ctx->thr, DUK_STR_INVALID_INPUT);
	DUK_WO_NORETURN(return;);
}

DUK_LOCAL void duk__transform_callback_decode_uri(duk__transform_context *tfm_ctx, const void *udata, duk_codepoint_t cp) {
	const duk_uint8_t *reserved_table = (const duk_uint8_t *) udata;
	duk_small_uint_t utf8_blen;
	duk_codepoint_t min_cp;
	duk_small_int_t t; /* must be signed */
	duk_small_uint_t i;

	/* Maximum write size: XUTF8 path writes max DUK_UNICODE_MAX_XUTF8_LENGTH,
	 * percent escape path writes max two times CESU-8 encoded BMP length.
	 */
	DUK_BW_ENSURE(tfm_ctx->thr,
	              &tfm_ctx->bw,
	              (DUK_UNICODE_MAX_XUTF8_LENGTH >= 2 * DUK_UNICODE_MAX_CESU8_BMP_LENGTH ? DUK_UNICODE_MAX_XUTF8_LENGTH :
                                                                                              DUK_UNICODE_MAX_CESU8_BMP_LENGTH));

	if (cp == (duk_codepoint_t) '%') {
		const duk_uint8_t *p = tfm_ctx->p;
		duk_size_t left = (duk_size_t) (tfm_ctx->p_end - p); /* bytes left */

		DUK_DDD(DUK_DDDPRINT("percent encoding, left=%ld", (long) left));

		if (left < 2) {
			goto uri_error;
		}

		t = duk__decode_hex_escape(p, 2);
		DUK_DDD(DUK_DDDPRINT("first byte: %ld", (long) t));
		if (t < 0) {
			goto uri_error;
		}

		if (t < 0x80) {
			if (DUK__CHECK_BITMASK(reserved_table, t)) {
				/* decode '%xx' to '%xx' if decoded char in reserved set */
				DUK_ASSERT(tfm_ctx->p - 1 >= tfm_ctx->p_start);
				DUK_BW_WRITE_RAW_U8_3(tfm_ctx->thr, &tfm_ctx->bw, DUK_ASC_PERCENT, p[0], p[1]);
			} else {
				DUK_BW_WRITE_RAW_U8(tfm_ctx->thr, &tfm_ctx->bw, (duk_uint8_t) t);
			}
			tfm_ctx->p += 2;
			return;
		}

		/* Decode UTF-8 codepoint from a sequence of hex escapes.  The
		 * first byte of the sequence has been decoded to 't'.
		 *
		 * Note that UTF-8 validation must be strict according to the
		 * specification: E5.1 Section 15.1.3, decode algorithm step
		 * 4.d.vii.8.  URIError from non-shortest encodings is also
		 * specifically noted in the spec.
		 */

		DUK_ASSERT(t >= 0x80);
		if (t < 0xc0) {
			/* continuation byte */
			goto uri_error;
		} else if (t < 0xe0) {
			/* 110x xxxx; 2 bytes */
			utf8_blen = 2;
			min_cp = 0x80L;
			cp = t & 0x1f;
		} else if (t < 0xf0) {
			/* 1110 xxxx; 3 bytes */
			utf8_blen = 3;
			min_cp = 0x800L;
			cp = t & 0x0f;
		} else if (t < 0xf8) {
			/* 1111 0xxx; 4 bytes */
			utf8_blen = 4;
			min_cp = 0x10000L;
			cp = t & 0x07;
		} else {
			/* extended utf-8 not allowed for URIs */
			goto uri_error;
		}

		if (left < utf8_blen * 3 - 1) {
			/* '%xx%xx...%xx', p points to char after first '%' */
			goto uri_error;
		}

		p += 3;
		for (i = 1; i < utf8_blen; i++) {
			/* p points to digit part ('%xy', p points to 'x') */
			t = duk__decode_hex_escape(p, 2);
			DUK_DDD(DUK_DDDPRINT("i=%ld utf8_blen=%ld cp=%ld t=0x%02lx",
			                     (long) i,
			                     (long) utf8_blen,
			                     (long) cp,
			                     (unsigned long) t));
			if (t < 0) {
				goto uri_error;
			}
			if ((t & 0xc0) != 0x80) {
				goto uri_error;
			}
			cp = (cp << 6) + (t & 0x3f);
			p += 3;
		}
		p--; /* p overshoots */
		tfm_ctx->p = p;

		DUK_DDD(DUK_DDDPRINT("final cp=%ld, min_cp=%ld", (long) cp, (long) min_cp));

		if (cp < min_cp || cp > 0x10ffffL || (cp >= 0xd800L && cp <= 0xdfffL)) {
			goto uri_error;
		}

		/* The E5.1 algorithm checks whether or not a decoded codepoint
		 * is below 0x80 and perhaps may be in the "reserved" set.
		 * This seems pointless because the single byte UTF-8 case is
		 * handled separately, and non-shortest encodings are rejected.
		 * So, 'cp' cannot be below 0x80 here, and thus cannot be in
		 * the reserved set.
		 */

		/* utf-8 validation ensures these */
		DUK_ASSERT(cp >= 0x80L && cp <= 0x10ffffL);

		if (cp >= 0x10000L) {
			cp -= 0x10000L;
			DUK_ASSERT(cp < 0x100000L);

			DUK_BW_WRITE_RAW_XUTF8(tfm_ctx->thr, &tfm_ctx->bw, ((cp >> 10) + 0xd800L));
			DUK_BW_WRITE_RAW_XUTF8(tfm_ctx->thr, &tfm_ctx->bw, ((cp & 0x03ffL) + 0xdc00L));
		} else {
			DUK_BW_WRITE_RAW_XUTF8(tfm_ctx->thr, &tfm_ctx->bw, cp);
		}
	} else {
		DUK_BW_WRITE_RAW_XUTF8(tfm_ctx->thr, &tfm_ctx->bw, cp);
	}
	return;

uri_error:
	DUK_ERROR_URI(tfm_ctx->thr, DUK_STR_INVALID_INPUT);
	DUK_WO_NORETURN(return;);
}

#if defined(DUK_USE_SECTION_B)
DUK_LOCAL void duk__transform_callback_escape(duk__transform_context *tfm_ctx, const void *udata, duk_codepoint_t cp) {
	DUK_UNREF(udata);

	DUK_BW_ENSURE(tfm_ctx->thr, &tfm_ctx->bw, 6);

	if (cp < 0) {
		goto esc_error;
	} else if ((cp < 0x80L) && DUK__CHECK_BITMASK(duk__escape_unescaped_table, cp)) {
		DUK_BW_WRITE_RAW_U8(tfm_ctx->thr, &tfm_ctx->bw, (duk_uint8_t) cp);
	} else if (cp < 0x100L) {
		DUK_BW_WRITE_RAW_U8_3(tfm_ctx->thr,
		                      &tfm_ctx->bw,
		                      (duk_uint8_t) DUK_ASC_PERCENT,
		                      (duk_uint8_t) duk_uc_nybbles[cp >> 4],
		                      (duk_uint8_t) duk_uc_nybbles[cp & 0x0f]);
	} else if (cp < 0x10000L) {
		DUK_BW_WRITE_RAW_U8_6(tfm_ctx->thr,
		                      &tfm_ctx->bw,
		                      (duk_uint8_t) DUK_ASC_PERCENT,
		                      (duk_uint8_t) DUK_ASC_LC_U,
		                      (duk_uint8_t) duk_uc_nybbles[cp >> 12],
		                      (duk_uint8_t) duk_uc_nybbles[(cp >> 8) & 0x0f],
		                      (duk_uint8_t) duk_uc_nybbles[(cp >> 4) & 0x0f],
		                      (duk_uint8_t) duk_uc_nybbles[cp & 0x0f]);
	} else {
		/* Characters outside BMP cannot be escape()'d.  We could
		 * encode them as surrogate pairs (for codepoints inside
		 * valid UTF-8 range, but not extended UTF-8).  Because
		 * escape() and unescape() are legacy functions, we don't.
		 */
		goto esc_error;
	}

	return;

esc_error:
	DUK_ERROR_TYPE(tfm_ctx->thr, DUK_STR_INVALID_INPUT);
	DUK_WO_NORETURN(return;);
}

DUK_LOCAL void duk__transform_callback_unescape(duk__transform_context *tfm_ctx, const void *udata, duk_codepoint_t cp) {
	duk_small_int_t t;

	DUK_UNREF(udata);

	if (cp == (duk_codepoint_t) '%') {
		const duk_uint8_t *p = tfm_ctx->p;
		duk_size_t left = (duk_size_t) (tfm_ctx->p_end - p); /* bytes left */

		if (left >= 5 && p[0] == 'u' && ((t = duk__decode_hex_escape(p + 1, 4)) >= 0)) {
			cp = (duk_codepoint_t) t;
			tfm_ctx->p += 5;
		} else if (left >= 2 && ((t = duk__decode_hex_escape(p, 2)) >= 0)) {
			cp = (duk_codepoint_t) t;
			tfm_ctx->p += 2;
		}
	}

	DUK_BW_WRITE_ENSURE_XUTF8(tfm_ctx->thr, &tfm_ctx->bw, cp);
}
#endif /* DUK_USE_SECTION_B */

/*
 *  Eval
 *
 *  Eval needs to handle both a "direct eval" and an "indirect eval".
 *  Direct eval handling needs access to the caller's activation so that its
 *  lexical environment can be accessed.  A direct eval is only possible from
 *  ECMAScript code; an indirect eval call is possible also from C code.
 *  When an indirect eval call is made from C code, there may not be a
 *  calling activation at all which needs careful handling.
 */

DUK_INTERNAL duk_ret_t duk_bi_global_object_eval(duk_hthread *thr) {
	duk_hstring *h;
	duk_activation *act_caller;
	duk_activation *act_eval;
	duk_hcompfunc *func;
	duk_hobject *outer_lex_env;
	duk_hobject *outer_var_env;
	duk_bool_t this_to_global = 1;
	duk_small_uint_t comp_flags;
	duk_int_t level = -2;
	duk_small_uint_t call_flags;

	DUK_ASSERT(duk_get_top(thr) == 1 || duk_get_top(thr) == 2); /* 2 when called by debugger */
	DUK_ASSERT(thr->callstack_top >= 1); /* at least this function exists */
	DUK_ASSERT(thr->callstack_curr != NULL);
	DUK_ASSERT((thr->callstack_curr->flags & DUK_ACT_FLAG_DIRECT_EVAL) == 0 || /* indirect eval */
	           (thr->callstack_top >= 2)); /* if direct eval, calling activation must exist */

	/*
	 *  callstack_top - 1 --> this function
	 *  callstack_top - 2 --> caller (may not exist)
	 *
	 *  If called directly from C, callstack_top might be 1.  If calling
	 *  activation doesn't exist, call must be indirect.
	 */

	h = duk_get_hstring_notsymbol(thr, 0);
	if (!h) {
		/* Symbol must be returned as is, like any non-string values. */
		return 1; /* return arg as-is */
	}

#if defined(DUK_USE_DEBUGGER_SUPPORT)
	/* NOTE: level is used only by the debugger and should never be present
	 * for an ECMAScript eval().
	 */
	DUK_ASSERT(level == -2); /* by default, use caller's environment */
	if (duk_get_top(thr) >= 2 && duk_is_number(thr, 1)) {
		level = duk_get_int(thr, 1);
	}
	DUK_ASSERT(level <= -2); /* This is guaranteed by debugger code. */
#endif

	/* [ source ] */

	comp_flags = DUK_COMPILE_EVAL;
	act_eval = thr->callstack_curr; /* this function */
	DUK_ASSERT(act_eval != NULL);
	act_caller = duk_hthread_get_activation_for_level(thr, level);
	if (act_caller != NULL) {
		/* Have a calling activation, check for direct eval (otherwise
		 * assume indirect eval.
		 */
		if ((act_caller->flags & DUK_ACT_FLAG_STRICT) && (act_eval->flags & DUK_ACT_FLAG_DIRECT_EVAL)) {
			/* Only direct eval inherits strictness from calling code
			 * (E5.1 Section 10.1.1).
			 */
			comp_flags |= DUK_COMPILE_STRICT;
		}
	} else {
		DUK_ASSERT((act_eval->flags & DUK_ACT_FLAG_DIRECT_EVAL) == 0);
	}

	duk_push_hstring_stridx(thr, DUK_STRIDX_INPUT); /* XXX: copy from caller? */
	duk_js_compile(thr, (const duk_uint8_t *) DUK_HSTRING_GET_DATA(h), (duk_size_t) DUK_HSTRING_GET_BYTELEN(h), comp_flags);
	func = (duk_hcompfunc *) duk_known_hobject(thr, -1);
	DUK_ASSERT(DUK_HOBJECT_IS_COMPFUNC((duk_hobject *) func));

	/* [ source template ] */

	/* E5 Section 10.4.2 */

	if (act_eval->flags & DUK_ACT_FLAG_DIRECT_EVAL) {
		DUK_ASSERT(thr->callstack_top >= 2);
		DUK_ASSERT(act_caller != NULL);
		if (act_caller->lex_env == NULL) {
			DUK_ASSERT(act_caller->var_env == NULL);
			DUK_DDD(DUK_DDDPRINT("delayed environment initialization"));

			/* this may have side effects, so re-lookup act */
			duk_js_init_activation_environment_records_delayed(thr, act_caller);
		}
		DUK_ASSERT(act_caller->lex_env != NULL);
		DUK_ASSERT(act_caller->var_env != NULL);

		this_to_global = 0;

		if (DUK_HOBJECT_HAS_STRICT((duk_hobject *) func)) {
			duk_hdecenv *new_env;
			duk_hobject *act_lex_env;

			DUK_DDD(DUK_DDDPRINT("direct eval call to a strict function -> "
			                     "var_env and lex_env to a fresh env, "
			                     "this_binding to caller's this_binding"));

			act_lex_env = act_caller->lex_env;

			new_env =
			    duk_hdecenv_alloc(thr,
			                      DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_DECENV));
			DUK_ASSERT(new_env != NULL);
			duk_push_hobject(thr, (duk_hobject *) new_env);

			DUK_ASSERT(DUK_HOBJECT_GET_PROTOTYPE(thr->heap, (duk_hobject *) new_env) == NULL);
			DUK_HOBJECT_SET_PROTOTYPE(thr->heap, (duk_hobject *) new_env, act_lex_env);
			DUK_HOBJECT_INCREF_ALLOWNULL(thr, act_lex_env);
			DUK_DDD(DUK_DDDPRINT("new_env allocated: %!iO", (duk_heaphdr *) new_env));

			outer_lex_env = (duk_hobject *) new_env;
			outer_var_env = (duk_hobject *) new_env;

			duk_insert(thr, 0); /* stash to bottom of value stack to keep new_env reachable for duration of eval */

			/* compiler's responsibility */
			DUK_ASSERT(DUK_HOBJECT_HAS_NEWENV((duk_hobject *) func));
		} else {
			DUK_DDD(DUK_DDDPRINT("direct eval call to a non-strict function -> "
			                     "var_env and lex_env to caller's envs, "
			                     "this_binding to caller's this_binding"));

			outer_lex_env = act_caller->lex_env;
			outer_var_env = act_caller->var_env;

			/* compiler's responsibility */
			DUK_ASSERT(!DUK_HOBJECT_HAS_NEWENV((duk_hobject *) func));
		}
	} else {
		DUK_DDD(DUK_DDDPRINT("indirect eval call -> var_env and lex_env to "
		                     "global object, this_binding to global object"));

		this_to_global = 1;
		outer_lex_env = thr->builtins[DUK_BIDX_GLOBAL_ENV];
		outer_var_env = thr->builtins[DUK_BIDX_GLOBAL_ENV];
	}

	/* Eval code doesn't need an automatic .prototype object. */
	duk_js_push_closure(thr, func, outer_var_env, outer_lex_env, 0 /*add_auto_proto*/);

	/* [ env? source template closure ] */

	if (this_to_global) {
		DUK_ASSERT(thr->builtins[DUK_BIDX_GLOBAL] != NULL);
		duk_push_hobject_bidx(thr, DUK_BIDX_GLOBAL);
	} else {
		duk_tval *tv;
		DUK_ASSERT(thr->callstack_top >= 2);
		DUK_ASSERT(act_caller != NULL);
		tv = (duk_tval *) (void *) ((duk_uint8_t *) thr->valstack + act_caller->bottom_byteoff -
		                            sizeof(duk_tval)); /* this is just beneath bottom */
		DUK_ASSERT(tv >= thr->valstack);
		duk_push_tval(thr, tv);
	}

	DUK_DDD(DUK_DDDPRINT("eval -> lex_env=%!iO, var_env=%!iO, this_binding=%!T",
	                     (duk_heaphdr *) outer_lex_env,
	                     (duk_heaphdr *) outer_var_env,
	                     duk_get_tval(thr, -1)));

	/* [ env? source template closure this ] */

	call_flags = 0;
	if (act_eval->flags & DUK_ACT_FLAG_DIRECT_EVAL) {
		/* Set DIRECT_EVAL flag for the call; it's not strictly
		 * needed for the 'inner' eval call (the eval body) but
		 * current new.target implementation expects to find it
		 * so it can traverse direct eval chains up to the real
		 * calling function.
		 */
		call_flags |= DUK_CALL_FLAG_DIRECT_EVAL;
	}
	duk_handle_call_unprotected_nargs(thr, 0, call_flags);

	/* [ env? source template result ] */

	return 1;
}

/*
 *  Parsing of ints and floats
 */

#if defined(DUK_USE_GLOBAL_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_global_object_parse_int(duk_hthread *thr) {
	duk_int32_t radix;
	duk_small_uint_t s2n_flags;

	DUK_ASSERT_TOP(thr, 2);
	duk_to_string(thr, 0); /* Reject symbols. */

	radix = duk_to_int32(thr, 1);

	/* While parseInt() recognizes 0xdeadbeef, it doesn't recognize
	 * ES2015 0o123 or 0b10001.
	 */
	s2n_flags = DUK_S2N_FLAG_TRIM_WHITE | DUK_S2N_FLAG_ALLOW_GARBAGE | DUK_S2N_FLAG_ALLOW_PLUS | DUK_S2N_FLAG_ALLOW_MINUS |
	            DUK_S2N_FLAG_ALLOW_LEADING_ZERO | DUK_S2N_FLAG_ALLOW_AUTO_HEX_INT;

	/* Specification stripPrefix maps to DUK_S2N_FLAG_ALLOW_AUTO_HEX_INT.
	 *
	 * Don't autodetect octals (from leading zeroes), require user code to
	 * provide an explicit radix 8 for parsing octal.  See write-up from Mozilla:
	 * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/parseInt#ECMAScript_5_Removes_Octal_Interpretation
	 */

	if (radix != 0) {
		if (radix < 2 || radix > 36) {
			goto ret_nan;
		}
		if (radix != 16) {
			s2n_flags &= ~DUK_S2N_FLAG_ALLOW_AUTO_HEX_INT;
		}
	} else {
		radix = 10;
	}

	duk_dup_0(thr);
	duk_numconv_parse(thr, (duk_small_int_t) radix, s2n_flags);
	return 1;

ret_nan:
	duk_push_nan(thr);
	return 1;
}
#endif /* DUK_USE_GLOBAL_BUILTIN */

#if defined(DUK_USE_GLOBAL_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_global_object_parse_float(duk_hthread *thr) {
	duk_small_uint_t s2n_flags;

	DUK_ASSERT_TOP(thr, 1);
	duk_to_string(thr, 0); /* Reject symbols. */

	/* XXX: check flags */
	s2n_flags = DUK_S2N_FLAG_TRIM_WHITE | DUK_S2N_FLAG_ALLOW_EXP | DUK_S2N_FLAG_ALLOW_GARBAGE | DUK_S2N_FLAG_ALLOW_PLUS |
	            DUK_S2N_FLAG_ALLOW_MINUS | DUK_S2N_FLAG_ALLOW_INF | DUK_S2N_FLAG_ALLOW_FRAC | DUK_S2N_FLAG_ALLOW_NAKED_FRAC |
	            DUK_S2N_FLAG_ALLOW_EMPTY_FRAC | DUK_S2N_FLAG_ALLOW_LEADING_ZERO;

	duk_numconv_parse(thr, 10 /*radix*/, s2n_flags);
	return 1;
}
#endif /* DUK_USE_GLOBAL_BUILTIN */

/*
 *  Number checkers
 */

#if defined(DUK_USE_GLOBAL_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_global_object_is_nan(duk_hthread *thr) {
	duk_double_t d = duk_to_number(thr, 0);
	duk_push_boolean(thr, (duk_bool_t) DUK_ISNAN(d));
	return 1;
}
#endif /* DUK_USE_GLOBAL_BUILTIN */

#if defined(DUK_USE_GLOBAL_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_global_object_is_finite(duk_hthread *thr) {
	duk_double_t d = duk_to_number(thr, 0);
	duk_push_boolean(thr, (duk_bool_t) DUK_ISFINITE(d));
	return 1;
}
#endif /* DUK_USE_GLOBAL_BUILTIN */

/*
 *  URI handling
 */

#if defined(DUK_USE_GLOBAL_BUILTIN)
DUK_INTERNAL duk_ret_t duk_bi_global_object_decode_uri(duk_hthread *thr) {
	return duk__transform_helper(thr, duk__transform_callback_decode_uri, (const void *) duk__decode_uri_reserved_table);
}

DUK_INTERNAL duk_ret_t duk_bi_global_object_decode_uri_component(duk_hthread *thr) {
	return duk__transform_helper(thr,
	                             duk__transform_callback_decode_uri,
	                             (const void *) duk__decode_uri_component_reserved_table);
}

DUK_INTERNAL duk_ret_t duk_bi_global_object_encode_uri(duk_hthread *thr) {
	return duk__transform_helper(thr, duk__transform_callback_encode_uri, (const void *) duk__encode_uriunescaped_table);
}

DUK_INTERNAL duk_ret_t duk_bi_global_object_encode_uri_component(duk_hthread *thr) {
	return duk__transform_helper(thr,
	                             duk__transform_callback_encode_uri,
	                             (const void *) duk__encode_uricomponent_unescaped_table);
}

#if defined(DUK_USE_SECTION_B)
DUK_INTERNAL duk_ret_t duk_bi_global_object_escape(duk_hthread *thr) {
	return duk__transform_helper(thr, duk__transform_callback_escape, (const void *) NULL);
}

DUK_INTERNAL duk_ret_t duk_bi_global_object_unescape(duk_hthread *thr) {
	return duk__transform_helper(thr, duk__transform_callback_unescape, (const void *) NULL);
}
#endif /* DUK_USE_SECTION_B */
#endif /* DUK_USE_GLOBAL_BUILTIN */
