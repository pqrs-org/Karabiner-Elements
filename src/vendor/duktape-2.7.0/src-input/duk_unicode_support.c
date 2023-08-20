/*
 *  Various Unicode help functions for character classification predicates,
 *  case conversion, decoding, etc.
 */

#include "duk_internal.h"

/*
 *  Fast path tables
 */

#if defined(DUK_USE_IDCHAR_FASTPATH)
DUK_INTERNAL const duk_int8_t duk_is_idchar_tab[128] = {
	/* 0: not IdentifierStart or IdentifierPart
	 * 1: IdentifierStart and IdentifierPart
	 * -1: IdentifierPart only
	 */
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, /* 0x00...0x0f */
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, /* 0x10...0x1f */
	0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, /* 0x20...0x2f */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, /* 0x30...0x3f */
	0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, 1, 1, 1, 1, 1, /* 0x40...0x4f */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, 0, 0, 0, 0, 1, /* 0x50...0x5f */
	0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, 1, 1, 1, 1, 1, /* 0x60...0x6f */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, 0, 0, 0, 0, 0 /* 0x70...0x7f */
};
#endif

/*
 *  XUTF-8 and CESU-8 encoding/decoding
 */

DUK_INTERNAL duk_small_int_t duk_unicode_get_xutf8_length(duk_ucodepoint_t cp) {
	duk_uint_fast32_t x = (duk_uint_fast32_t) cp;
	if (x < 0x80UL) {
		/* 7 bits */
		return 1;
	} else if (x < 0x800UL) {
		/* 11 bits */
		return 2;
	} else if (x < 0x10000UL) {
		/* 16 bits */
		return 3;
	} else if (x < 0x200000UL) {
		/* 21 bits */
		return 4;
	} else if (x < 0x4000000UL) {
		/* 26 bits */
		return 5;
	} else if (x < (duk_ucodepoint_t) 0x80000000UL) {
		/* 31 bits */
		return 6;
	} else {
		/* 36 bits */
		return 7;
	}
}

#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL duk_small_int_t duk_unicode_get_cesu8_length(duk_ucodepoint_t cp) {
	duk_uint_fast32_t x = (duk_uint_fast32_t) cp;
	if (x < 0x80UL) {
		/* 7 bits */
		return 1;
	} else if (x < 0x800UL) {
		/* 11 bits */
		return 2;
	} else if (x < 0x10000UL) {
		/* 16 bits */
		return 3;
	} else {
		/* Encoded as surrogate pair, each encoding to 3 bytes for
		 * 6 bytes total.  Codepoints above U+10FFFF encode as 6 bytes
		 * too, see duk_unicode_encode_cesu8().
		 */
		return 3 + 3;
	}
}
#endif /* DUK_USE_ASSERTIONS */

DUK_INTERNAL const duk_uint8_t duk_unicode_xutf8_markers[7] = { 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe };

/* Encode to extended UTF-8; 'out' must have space for at least
 * DUK_UNICODE_MAX_XUTF8_LENGTH bytes.  Allows encoding of any
 * 32-bit (unsigned) codepoint.
 */
DUK_INTERNAL duk_small_int_t duk_unicode_encode_xutf8(duk_ucodepoint_t cp, duk_uint8_t *out) {
	duk_uint_fast32_t x = (duk_uint_fast32_t) cp;
	duk_small_int_t len;
	duk_uint8_t marker;
	duk_small_int_t i;

	len = duk_unicode_get_xutf8_length(cp);
	DUK_ASSERT(len > 0);

	marker = duk_unicode_xutf8_markers[len - 1]; /* 64-bit OK because always >= 0 */

	i = len;
	DUK_ASSERT(i > 0);
	do {
		i--;
		if (i > 0) {
			out[i] = (duk_uint8_t) (0x80 + (x & 0x3f));
			x >>= 6;
		} else {
			/* Note: masking of 'x' is not necessary because of
			 * range check and shifting -> no bits overlapping
			 * the marker should be set.
			 */
			out[0] = (duk_uint8_t) (marker + x);
		}
	} while (i > 0);

	return len;
}

/* Encode to CESU-8; 'out' must have space for at least
 * DUK_UNICODE_MAX_CESU8_LENGTH bytes; codepoints above U+10FFFF
 * will encode to garbage but won't overwrite the output buffer.
 */
DUK_INTERNAL duk_small_int_t duk_unicode_encode_cesu8(duk_ucodepoint_t cp, duk_uint8_t *out) {
	duk_uint_fast32_t x = (duk_uint_fast32_t) cp;
	duk_small_int_t len;

	if (x < 0x80UL) {
		out[0] = (duk_uint8_t) x;
		len = 1;
	} else if (x < 0x800UL) {
		out[0] = (duk_uint8_t) (0xc0 + ((x >> 6) & 0x1f));
		out[1] = (duk_uint8_t) (0x80 + (x & 0x3f));
		len = 2;
	} else if (x < 0x10000UL) {
		/* surrogate pairs get encoded here */
		out[0] = (duk_uint8_t) (0xe0 + ((x >> 12) & 0x0f));
		out[1] = (duk_uint8_t) (0x80 + ((x >> 6) & 0x3f));
		out[2] = (duk_uint8_t) (0x80 + (x & 0x3f));
		len = 3;
	} else {
		/*
		 *  Unicode codepoints above U+FFFF are encoded as surrogate
		 *  pairs here.  This ensures that all CESU-8 codepoints are
		 *  16-bit values as expected in ECMAScript.  The surrogate
		 *  pairs always get a 3-byte encoding (each) in CESU-8.
		 *  See: http://en.wikipedia.org/wiki/Surrogate_pair
		 *
		 *  20-bit codepoint, 10 bits (A and B) per surrogate pair:
		 *
		 *    x = 0b00000000 0000AAAA AAAAAABB BBBBBBBB
		 *  sp1 = 0b110110AA AAAAAAAA  (0xd800 + ((x >> 10) & 0x3ff))
		 *  sp2 = 0b110111BB BBBBBBBB  (0xdc00 + (x & 0x3ff))
		 *
		 *  Encoded into CESU-8:
		 *
		 *  sp1 -> 0b11101101  (0xe0 + ((sp1 >> 12) & 0x0f))
		 *      -> 0b1010AAAA  (0x80 + ((sp1 >> 6) & 0x3f))
		 *      -> 0b10AAAAAA  (0x80 + (sp1 & 0x3f))
		 *  sp2 -> 0b11101101  (0xe0 + ((sp2 >> 12) & 0x0f))
		 *      -> 0b1011BBBB  (0x80 + ((sp2 >> 6) & 0x3f))
		 *      -> 0b10BBBBBB  (0x80 + (sp2 & 0x3f))
		 *
		 *  Note that 0x10000 must be subtracted first.  The code below
		 *  avoids the sp1, sp2 temporaries which saves around 20 bytes
		 *  of code.
		 */

		x -= 0x10000UL;

		out[0] = (duk_uint8_t) (0xed);
		out[1] = (duk_uint8_t) (0xa0 + ((x >> 16) & 0x0f));
		out[2] = (duk_uint8_t) (0x80 + ((x >> 10) & 0x3f));
		out[3] = (duk_uint8_t) (0xed);
		out[4] = (duk_uint8_t) (0xb0 + ((x >> 6) & 0x0f));
		out[5] = (duk_uint8_t) (0x80 + (x & 0x3f));
		len = 6;
	}

	return len;
}

/* Decode helper.  Return zero on error. */
DUK_INTERNAL duk_small_int_t duk_unicode_decode_xutf8(duk_hthread *thr,
                                                      const duk_uint8_t **ptr,
                                                      const duk_uint8_t *ptr_start,
                                                      const duk_uint8_t *ptr_end,
                                                      duk_ucodepoint_t *out_cp) {
	const duk_uint8_t *p;
	duk_uint32_t res;
	duk_uint_fast8_t ch;
	duk_small_int_t n;

	DUK_UNREF(thr);

	p = *ptr;
	if (p < ptr_start || p >= ptr_end) {
		goto fail;
	}

	/*
	 *  UTF-8 decoder which accepts longer than standard byte sequences.
	 *  This allows full 32-bit code points to be used.
	 */

	ch = (duk_uint_fast8_t) (*p++);
	if (ch < 0x80) {
		/* 0xxx xxxx   [7 bits] */
		res = (duk_uint32_t) (ch & 0x7f);
		n = 0;
	} else if (ch < 0xc0) {
		/* 10xx xxxx -> invalid */
		goto fail;
	} else if (ch < 0xe0) {
		/* 110x xxxx   10xx xxxx   [11 bits] */
		res = (duk_uint32_t) (ch & 0x1f);
		n = 1;
	} else if (ch < 0xf0) {
		/* 1110 xxxx   10xx xxxx   10xx xxxx   [16 bits] */
		res = (duk_uint32_t) (ch & 0x0f);
		n = 2;
	} else if (ch < 0xf8) {
		/* 1111 0xxx   10xx xxxx   10xx xxxx   10xx xxxx   [21 bits] */
		res = (duk_uint32_t) (ch & 0x07);
		n = 3;
	} else if (ch < 0xfc) {
		/* 1111 10xx   10xx xxxx   10xx xxxx   10xx xxxx   10xx xxxx   [26 bits] */
		res = (duk_uint32_t) (ch & 0x03);
		n = 4;
	} else if (ch < 0xfe) {
		/* 1111 110x   10xx xxxx   10xx xxxx   10xx xxxx   10xx xxxx   10xx xxxx   [31 bits] */
		res = (duk_uint32_t) (ch & 0x01);
		n = 5;
	} else if (ch < 0xff) {
		/* 1111 1110   10xx xxxx   10xx xxxx   10xx xxxx   10xx xxxx   10xx xxxx   10xx xxxx   [36 bits] */
		res = (duk_uint32_t) (0);
		n = 6;
	} else {
		/* 8-byte format could be:
		 * 1111 1111   10xx xxxx   10xx xxxx   10xx xxxx   10xx xxxx   10xx xxxx   10xx xxxx   10xx xxxx   [41 bits]
		 *
		 * However, this format would not have a zero bit following the
		 * leading one bits and would not allow 0xFF to be used as an
		 * "invalid xutf-8" marker for internal keys.  Further, 8-byte
		 * encodings (up to 41 bit code points) are not currently needed.
		 */
		goto fail;
	}

	DUK_ASSERT(p >= ptr_start); /* verified at beginning */
	if (p + n > ptr_end) {
		/* check pointer at end */
		goto fail;
	}

	while (n > 0) {
		DUK_ASSERT(p >= ptr_start && p < ptr_end);
		ch = (duk_uint_fast8_t) (*p++);
#if 0
		if (ch & 0xc0 != 0x80) {
			/* not a continuation byte */
			p--;
			*ptr = p;
			*out_cp = DUK_UNICODE_CP_REPLACEMENT_CHARACTER;
			return 1;
		}
#endif
		res = (res << 6) + (duk_uint32_t) (ch & 0x3f);
		n--;
	}

	*ptr = p;
	*out_cp = res;
	return 1;

fail:
	return 0;
}

/* used by e.g. duk_regexp_executor.c, string built-ins */
DUK_INTERNAL duk_ucodepoint_t duk_unicode_decode_xutf8_checked(duk_hthread *thr,
                                                               const duk_uint8_t **ptr,
                                                               const duk_uint8_t *ptr_start,
                                                               const duk_uint8_t *ptr_end) {
	duk_ucodepoint_t cp;

	if (duk_unicode_decode_xutf8(thr, ptr, ptr_start, ptr_end, &cp)) {
		return cp;
	}
	DUK_ERROR_INTERNAL(thr);
	DUK_WO_NORETURN(return 0;);
}

/* Compute (extended) utf-8 length without codepoint encoding validation,
 * used for string interning.
 *
 * NOTE: This algorithm is performance critical, more so than string hashing
 * in some cases.  It is needed when interning a string and needs to scan
 * every byte of the string with no skipping.  Having an ASCII fast path
 * is useful if possible in the algorithm.  The current algorithms were
 * chosen from several variants, based on x64 gcc -O2 testing.  See:
 * https://github.com/svaarala/duktape/pull/422
 *
 * NOTE: must match tools/dukutil.py:duk_unicode_unvalidated_utf8_length().
 */

#if defined(DUK_USE_PREFER_SIZE)
/* Small variant; roughly 150 bytes smaller than the fast variant. */
DUK_INTERNAL duk_size_t duk_unicode_unvalidated_utf8_length(const duk_uint8_t *data, duk_size_t blen) {
	const duk_uint8_t *p;
	const duk_uint8_t *p_end;
	duk_size_t ncont;
	duk_size_t clen;

	p = data;
	p_end = data + blen;
	ncont = 0;
	while (p != p_end) {
		duk_uint8_t x;
		x = *p++;
		if (DUK_UNLIKELY(x >= 0x80 && x <= 0xbf)) {
			ncont++;
		}
	}

	DUK_ASSERT(ncont <= blen);
	clen = blen - ncont;
	DUK_ASSERT(clen <= blen);
	return clen;
}
#else /* DUK_USE_PREFER_SIZE */
/* This seems like a good overall approach.  Fast path for ASCII in 4 byte
 * blocks.
 */
DUK_INTERNAL duk_size_t duk_unicode_unvalidated_utf8_length(const duk_uint8_t *data, duk_size_t blen) {
	const duk_uint8_t *p;
	const duk_uint8_t *p_end;
	const duk_uint32_t *p32_end;
	const duk_uint32_t *p32;
	duk_size_t ncont;
	duk_size_t clen;

	ncont = 0; /* number of continuation (non-initial) bytes in [0x80,0xbf] */
	p = data;
	p_end = data + blen;
	if (blen < 16) {
		goto skip_fastpath;
	}

	/* Align 'p' to 4; the input data may have arbitrary alignment.
	 * End of string check not needed because blen >= 16.
	 */
	while (((duk_size_t) (const void *) p) & 0x03U) {
		duk_uint8_t x;
		x = *p++;
		if (DUK_UNLIKELY(x >= 0x80 && x <= 0xbf)) {
			ncont++;
		}
	}

	/* Full, aligned 4-byte reads. */
	p32_end = (const duk_uint32_t *) (const void *) (p + ((duk_size_t) (p_end - p) & (duk_size_t) (~0x03)));
	p32 = (const duk_uint32_t *) (const void *) p;
	while (p32 != (const duk_uint32_t *) p32_end) {
		duk_uint32_t x;
		x = *p32++;
		if (DUK_LIKELY((x & 0x80808080UL) == 0)) {
			; /* ASCII fast path */
		} else {
			/* Flip highest bit of each byte which changes
			 * the bit pattern 10xxxxxx into 00xxxxxx which
			 * allows an easy bit mask test.
			 */
			x ^= 0x80808080UL;
			if (DUK_UNLIKELY(!(x & 0xc0000000UL))) {
				ncont++;
			}
			if (DUK_UNLIKELY(!(x & 0x00c00000UL))) {
				ncont++;
			}
			if (DUK_UNLIKELY(!(x & 0x0000c000UL))) {
				ncont++;
			}
			if (DUK_UNLIKELY(!(x & 0x000000c0UL))) {
				ncont++;
			}
		}
	}
	p = (const duk_uint8_t *) p32;
	/* Fall through to handle the rest. */

skip_fastpath:
	while (p != p_end) {
		duk_uint8_t x;
		x = *p++;
		if (DUK_UNLIKELY(x >= 0x80 && x <= 0xbf)) {
			ncont++;
		}
	}

	DUK_ASSERT(ncont <= blen);
	clen = blen - ncont;
	DUK_ASSERT(clen <= blen);
	return clen;
}
#endif /* DUK_USE_PREFER_SIZE */

/* Check whether a string is UTF-8 compatible or not. */
DUK_INTERNAL duk_bool_t duk_unicode_is_utf8_compatible(const duk_uint8_t *buf, duk_size_t len) {
	duk_size_t i = 0;
#if !defined(DUK_USE_PREFER_SIZE)
	duk_size_t len_safe;
#endif

	/* Many practical strings are ASCII only, so use a fast path check
	 * to check chunks of bytes at once with minimal branch cost.
	 */
#if !defined(DUK_USE_PREFER_SIZE)
	len_safe = len & ~0x03UL;
	for (; i < len_safe; i += 4) {
		duk_uint8_t t = buf[i] | buf[i + 1] | buf[i + 2] | buf[i + 3];
		if (DUK_UNLIKELY((t & 0x80U) != 0U)) {
			/* At least one byte was outside 0x00-0x7f, break
			 * out to slow path (and remain there).
			 *
			 * XXX: We could also deal with the problem character
			 * and resume fast path later.
			 */
			break;
		}
	}
#endif

	for (; i < len;) {
		duk_uint8_t t;
		duk_size_t left;
		duk_size_t ncont;
		duk_uint32_t cp;
		duk_uint32_t mincp;

		t = buf[i++];
		if (DUK_LIKELY((t & 0x80U) == 0U)) {
			/* Fast path, ASCII. */
			continue;
		}

		/* Non-ASCII start byte, slow path.
		 *
		 * 10xx xxxx          -> continuation byte
		 * 110x xxxx + 1*CONT -> [0x80, 0x7ff]
		 * 1110 xxxx + 2*CONT -> [0x800, 0xffff], must reject [0xd800,0xdfff]
		 * 1111 0xxx + 3*CONT -> [0x10000, 0x10ffff]
		 */
		left = len - i;
		if (t <= 0xdfU) { /* 1101 1111 = 0xdf */
			if (t <= 0xbfU) { /* 1011 1111 = 0xbf */
				return 0;
			}
			ncont = 1;
			mincp = 0x80UL;
			cp = t & 0x1fU;
		} else if (t <= 0xefU) { /* 1110 1111 = 0xef */
			ncont = 2;
			mincp = 0x800UL;
			cp = t & 0x0fU;
		} else if (t <= 0xf7U) { /* 1111 0111 = 0xf7 */
			ncont = 3;
			mincp = 0x10000UL;
			cp = t & 0x07U;
		} else {
			return 0;
		}
		if (left < ncont) {
			return 0;
		}
		while (ncont > 0U) {
			t = buf[i++];
			if ((t & 0xc0U) != 0x80U) { /* 10xx xxxx */
				return 0;
			}
			cp = (cp << 6) + (t & 0x3fU);
			ncont--;
		}
		if (cp < mincp || cp > 0x10ffffUL || (cp >= 0xd800UL && cp <= 0xdfffUL)) {
			return 0;
		}
	}

	return 1;
}

/*
 *  Unicode range matcher
 *
 *  Matches a codepoint against a packed bitstream of character ranges.
 *  Used for slow path Unicode matching.
 */

/* Must match tools/extract_chars.py, generate_match_table3(). */
DUK_LOCAL duk_uint32_t duk__uni_decode_value(duk_bitdecoder_ctx *bd_ctx) {
	duk_uint32_t t;

	t = (duk_uint32_t) duk_bd_decode(bd_ctx, 4);
	if (t <= 0x0eU) {
		return t;
	}
	t = (duk_uint32_t) duk_bd_decode(bd_ctx, 8);
	if (t <= 0xfdU) {
		return t + 0x0f;
	}
	if (t == 0xfeU) {
		t = (duk_uint32_t) duk_bd_decode(bd_ctx, 12);
		return t + 0x0fU + 0xfeU;
	} else {
		t = (duk_uint32_t) duk_bd_decode(bd_ctx, 24);
		return t + 0x0fU + 0xfeU + 0x1000UL;
	}
}

DUK_LOCAL duk_small_int_t duk__uni_range_match(const duk_uint8_t *unitab, duk_size_t unilen, duk_codepoint_t cp) {
	duk_bitdecoder_ctx bd_ctx;
	duk_codepoint_t prev_re;

	duk_memzero(&bd_ctx, sizeof(bd_ctx));
	bd_ctx.data = (const duk_uint8_t *) unitab;
	bd_ctx.length = (duk_size_t) unilen;

	prev_re = 0;
	for (;;) {
		duk_codepoint_t r1, r2;
		r1 = (duk_codepoint_t) duk__uni_decode_value(&bd_ctx);
		if (r1 == 0) {
			break;
		}
		r2 = (duk_codepoint_t) duk__uni_decode_value(&bd_ctx);

		r1 = prev_re + r1;
		r2 = r1 + r2;
		prev_re = r2;

		/* [r1,r2] is the range */

		DUK_DDD(DUK_DDDPRINT("duk__uni_range_match: cp=%06lx range=[0x%06lx,0x%06lx]",
		                     (unsigned long) cp,
		                     (unsigned long) r1,
		                     (unsigned long) r2));
		if (cp >= r1 && cp <= r2) {
			return 1;
		}
	}

	return 0;
}

/*
 *  "WhiteSpace" production check.
 */

DUK_INTERNAL duk_small_int_t duk_unicode_is_whitespace(duk_codepoint_t cp) {
	/*
	 *  E5 Section 7.2 specifies six characters specifically as
	 *  white space:
	 *
	 *    0009;<control>;Cc;0;S;;;;;N;CHARACTER TABULATION;;;;
	 *    000B;<control>;Cc;0;S;;;;;N;LINE TABULATION;;;;
	 *    000C;<control>;Cc;0;WS;;;;;N;FORM FEED (FF);;;;
	 *    0020;SPACE;Zs;0;WS;;;;;N;;;;;
	 *    00A0;NO-BREAK SPACE;Zs;0;CS;<noBreak> 0020;;;;N;NON-BREAKING SPACE;;;;
	 *    FEFF;ZERO WIDTH NO-BREAK SPACE;Cf;0;BN;;;;;N;BYTE ORDER MARK;;;;
	 *
	 *  It also specifies any Unicode category 'Zs' characters as white
	 *  space.  These can be extracted with the "tools/extract_chars.py" script.
	 *  Current result:
	 *
	 *    RAW OUTPUT:
	 *    ===========
	 *    0020;SPACE;Zs;0;WS;;;;;N;;;;;
	 *    00A0;NO-BREAK SPACE;Zs;0;CS;<noBreak> 0020;;;;N;NON-BREAKING SPACE;;;;
	 *    1680;OGHAM SPACE MARK;Zs;0;WS;;;;;N;;;;;
	 *    180E;MONGOLIAN VOWEL SEPARATOR;Zs;0;WS;;;;;N;;;;;
	 *    2000;EN QUAD;Zs;0;WS;2002;;;;N;;;;;
	 *    2001;EM QUAD;Zs;0;WS;2003;;;;N;;;;;
	 *    2002;EN SPACE;Zs;0;WS;<compat> 0020;;;;N;;;;;
	 *    2003;EM SPACE;Zs;0;WS;<compat> 0020;;;;N;;;;;
	 *    2004;THREE-PER-EM SPACE;Zs;0;WS;<compat> 0020;;;;N;;;;;
	 *    2005;FOUR-PER-EM SPACE;Zs;0;WS;<compat> 0020;;;;N;;;;;
	 *    2006;SIX-PER-EM SPACE;Zs;0;WS;<compat> 0020;;;;N;;;;;
	 *    2007;FIGURE SPACE;Zs;0;WS;<noBreak> 0020;;;;N;;;;;
	 *    2008;PUNCTUATION SPACE;Zs;0;WS;<compat> 0020;;;;N;;;;;
	 *    2009;THIN SPACE;Zs;0;WS;<compat> 0020;;;;N;;;;;
	 *    200A;HAIR SPACE;Zs;0;WS;<compat> 0020;;;;N;;;;;
	 *    202F;NARROW NO-BREAK SPACE;Zs;0;CS;<noBreak> 0020;;;;N;;;;;
	 *    205F;MEDIUM MATHEMATICAL SPACE;Zs;0;WS;<compat> 0020;;;;N;;;;;
	 *    3000;IDEOGRAPHIC SPACE;Zs;0;WS;<wide> 0020;;;;N;;;;;
	 *
	 *    RANGES:
	 *    =======
	 *    0x0020
	 *    0x00a0
	 *    0x1680
	 *    0x180e
	 *    0x2000 ... 0x200a
	 *    0x202f
	 *    0x205f
	 *    0x3000
	 *
	 *  A manual decoder (below) is probably most compact for this.
	 */

	duk_uint_fast8_t lo;
	duk_uint_fast32_t hi;

	/* cp == -1 (EOF) never matches and causes return value 0 */

	lo = (duk_uint_fast8_t) (cp & 0xff);
	hi = (duk_uint_fast32_t) (cp >> 8); /* does not fit into an uchar */

	if (hi == 0x0000UL) {
		if (lo == 0x09U || lo == 0x0bU || lo == 0x0cU || lo == 0x20U || lo == 0xa0U) {
			return 1;
		}
	} else if (hi == 0x0020UL) {
		if (lo <= 0x0aU || lo == 0x2fU || lo == 0x5fU) {
			return 1;
		}
	} else if (cp == 0x1680L || cp == 0x180eL || cp == 0x3000L || cp == 0xfeffL) {
		return 1;
	}

	return 0;
}

/*
 *  "LineTerminator" production check.
 */

DUK_INTERNAL duk_small_int_t duk_unicode_is_line_terminator(duk_codepoint_t cp) {
	/*
	 *  E5 Section 7.3
	 *
	 *  A LineTerminatorSequence essentially merges <CR> <LF> sequences
	 *  into a single line terminator.  This must be handled by the caller.
	 */

	if (cp == 0x000aL || cp == 0x000dL || cp == 0x2028L || cp == 0x2029L) {
		return 1;
	}

	return 0;
}

/*
 *  "IdentifierStart" production check.
 */

DUK_INTERNAL duk_small_int_t duk_unicode_is_identifier_start(duk_codepoint_t cp) {
	/*
	 *  E5 Section 7.6:
	 *
	 *    IdentifierStart:
	 *      UnicodeLetter
	 *      $
	 *      _
	 *      \ UnicodeEscapeSequence
	 *
	 *  IdentifierStart production has one multi-character production:
	 *
	 *    \ UnicodeEscapeSequence
	 *
	 *  The '\' character is -not- matched by this function.  Rather, the caller
	 *  should decode the escape and then call this function to check whether the
	 *  decoded character is acceptable (see discussion in E5 Section 7.6).
	 *
	 *  The "UnicodeLetter" alternative of the production allows letters
	 *  from various Unicode categories.  These can be extracted with the
	 *  "tools/extract_chars.py" script.
	 *
	 *  Because the result has hundreds of Unicode codepoint ranges, matching
	 *  for any values >= 0x80 are done using a very slow range-by-range scan
	 *  and a packed range format.
	 *
	 *  The ASCII portion (codepoints 0x00 ... 0x7f) is fast-pathed below because
	 *  it matters the most.  The ASCII related ranges of IdentifierStart are:
	 *
	 *    0x0041 ... 0x005a     ['A' ... 'Z']
	 *    0x0061 ... 0x007a     ['a' ... 'z']
	 *    0x0024                ['$']
	 *    0x005f                ['_']
	 */

	/* ASCII (and EOF) fast path -- quick accept and reject */
	if (cp <= 0x7fL) {
#if defined(DUK_USE_IDCHAR_FASTPATH)
		return (cp >= 0) && (duk_is_idchar_tab[cp] > 0);
#else
		if ((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') || cp == '_' || cp == '$') {
			return 1;
		}
		return 0;
#endif
	}

	/* Non-ASCII slow path (range-by-range linear comparison), very slow */

#if defined(DUK_USE_SOURCE_NONBMP)
	if (duk__uni_range_match(duk_unicode_ids_noa, (duk_size_t) sizeof(duk_unicode_ids_noa), (duk_codepoint_t) cp)) {
		return 1;
	}
	return 0;
#else
	if (cp < 0x10000L) {
		if (duk__uni_range_match(duk_unicode_ids_noabmp, sizeof(duk_unicode_ids_noabmp), (duk_codepoint_t) cp)) {
			return 1;
		}
		return 0;
	} else {
		/* without explicit non-BMP support, assume non-BMP characters
		 * are always accepted as identifier characters.
		 */
		return 1;
	}
#endif
}

/*
 *  "IdentifierPart" production check.
 */

DUK_INTERNAL duk_small_int_t duk_unicode_is_identifier_part(duk_codepoint_t cp) {
	/*
	 *  E5 Section 7.6:
	 *
	 *    IdentifierPart:
	 *      IdentifierStart
	 *      UnicodeCombiningMark
	 *      UnicodeDigit
	 *      UnicodeConnectorPunctuation
	 *      <ZWNJ>  [U+200C]
	 *      <ZWJ>   [U+200D]
	 *
	 *  IdentifierPart production has one multi-character production
	 *  as part of its IdentifierStart alternative.  The '\' character
	 *  of an escape sequence is not matched here, see discussion in
	 *  duk_unicode_is_identifier_start().
	 *
	 *  To match non-ASCII characters (codepoints >= 0x80), a very slow
	 *  linear range-by-range scan is used.  The codepoint is first compared
	 *  to the IdentifierStart ranges, and if it doesn't match, then to a
	 *  set consisting of code points in IdentifierPart but not in
	 *  IdentifierStart.  This is done to keep the unicode range data small,
	 *  at the expense of speed.
	 *
	 *  The ASCII fast path consists of:
	 *
	 *    0x0030 ... 0x0039     ['0' ... '9', UnicodeDigit]
	 *    0x0041 ... 0x005a     ['A' ... 'Z', IdentifierStart]
	 *    0x0061 ... 0x007a     ['a' ... 'z', IdentifierStart]
	 *    0x0024                ['$', IdentifierStart]
	 *    0x005f                ['_', IdentifierStart and
	 *                                UnicodeConnectorPunctuation]
	 *
	 *  UnicodeCombiningMark has no code points <= 0x7f.
	 *
	 *  The matching code reuses the "identifier start" tables, and then
	 *  consults a separate range set for characters in "identifier part"
	 *  but not in "identifier start".  These can be extracted with the
	 *  "tools/extract_chars.py" script.
	 *
	 *  UnicodeCombiningMark -> categories Mn, Mc
	 *  UnicodeDigit -> categories Nd
	 *  UnicodeConnectorPunctuation -> categories Pc
	 */

	/* ASCII (and EOF) fast path -- quick accept and reject */
	if (cp <= 0x7fL) {
#if defined(DUK_USE_IDCHAR_FASTPATH)
		return (cp >= 0) && (duk_is_idchar_tab[cp] != 0);
#else
		if ((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') || (cp >= '0' && cp <= '9') || cp == '_' || cp == '$') {
			return 1;
		}
		return 0;
#endif
	}

	/* Non-ASCII slow path (range-by-range linear comparison), very slow */

#if defined(DUK_USE_SOURCE_NONBMP)
	if (duk__uni_range_match(duk_unicode_ids_noa, sizeof(duk_unicode_ids_noa), (duk_codepoint_t) cp) ||
	    duk__uni_range_match(duk_unicode_idp_m_ids_noa, sizeof(duk_unicode_idp_m_ids_noa), (duk_codepoint_t) cp)) {
		return 1;
	}
	return 0;
#else
	if (cp < 0x10000L) {
		if (duk__uni_range_match(duk_unicode_ids_noabmp, sizeof(duk_unicode_ids_noabmp), (duk_codepoint_t) cp) ||
		    duk__uni_range_match(duk_unicode_idp_m_ids_noabmp,
		                         sizeof(duk_unicode_idp_m_ids_noabmp),
		                         (duk_codepoint_t) cp)) {
			return 1;
		}
		return 0;
	} else {
		/* without explicit non-BMP support, assume non-BMP characters
		 * are always accepted as identifier characters.
		 */
		return 1;
	}
#endif
}

/*
 *  Unicode letter check.
 */

DUK_INTERNAL duk_small_int_t duk_unicode_is_letter(duk_codepoint_t cp) {
	/*
	 *  Unicode letter is now taken to be the categories:
	 *
	 *    Lu, Ll, Lt, Lm, Lo
	 *
	 *  (Not sure if this is exactly correct.)
	 *
	 *  The ASCII fast path consists of:
	 *
	 *    0x0041 ... 0x005a     ['A' ... 'Z']
	 *    0x0061 ... 0x007a     ['a' ... 'z']
	 */

	/* ASCII (and EOF) fast path -- quick accept and reject */
	if (cp <= 0x7fL) {
		if ((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z')) {
			return 1;
		}
		return 0;
	}

	/* Non-ASCII slow path (range-by-range linear comparison), very slow */

#if defined(DUK_USE_SOURCE_NONBMP)
	if (duk__uni_range_match(duk_unicode_ids_noa, sizeof(duk_unicode_ids_noa), (duk_codepoint_t) cp) &&
	    !duk__uni_range_match(duk_unicode_ids_m_let_noa, sizeof(duk_unicode_ids_m_let_noa), (duk_codepoint_t) cp)) {
		return 1;
	}
	return 0;
#else
	if (cp < 0x10000L) {
		if (duk__uni_range_match(duk_unicode_ids_noabmp, sizeof(duk_unicode_ids_noabmp), (duk_codepoint_t) cp) &&
		    !duk__uni_range_match(duk_unicode_ids_m_let_noabmp,
		                          sizeof(duk_unicode_ids_m_let_noabmp),
		                          (duk_codepoint_t) cp)) {
			return 1;
		}
		return 0;
	} else {
		/* without explicit non-BMP support, assume non-BMP characters
		 * are always accepted as letters.
		 */
		return 1;
	}
#endif
}

/*
 *  Complex case conversion helper which decodes a bit-packed conversion
 *  control stream generated by tools/extract_caseconv.py.  The conversion
 *  is very slow because it runs through the conversion data in a linear
 *  fashion to save space (which is why ASCII characters have a special
 *  fast path before arriving here).
 *
 *  The particular bit counts etc have been determined experimentally to
 *  be small but still sufficient, and must match the Python script
 *  (tools/extract_caseconv.py).
 *
 *  The return value is the case converted codepoint or -1 if the conversion
 *  results in multiple characters (this is useful for regexp Canonicalization
 *  operation).  If 'buf' is not NULL, the result codepoint(s) are also
 *  appended to the hbuffer.
 *
 *  Context and locale specific rules must be checked before consulting
 *  this function.
 */

DUK_LOCAL
duk_codepoint_t duk__slow_case_conversion(duk_hthread *thr, duk_bufwriter_ctx *bw, duk_codepoint_t cp, duk_bitdecoder_ctx *bd_ctx) {
	duk_small_int_t skip = 0;
	duk_small_int_t n;
	duk_small_int_t t;
	duk_small_int_t count;
	duk_codepoint_t tmp_cp;
	duk_codepoint_t start_i;
	duk_codepoint_t start_o;

	DUK_ASSERT(bd_ctx != NULL);
	DUK_UNREF(thr);

	DUK_DDD(DUK_DDDPRINT("slow case conversion for codepoint: %ld", (long) cp));

	/* range conversion with a "skip" */
	DUK_DDD(DUK_DDDPRINT("checking ranges"));
	for (;;) {
		skip++;
		n = (duk_small_int_t) duk_bd_decode(bd_ctx, 6);
		if (n == 0x3f) {
			/* end marker */
			break;
		}
		DUK_DDD(DUK_DDDPRINT("skip=%ld, n=%ld", (long) skip, (long) n));

		while (n--) {
			start_i = (duk_codepoint_t) duk_bd_decode(bd_ctx, 16);
			start_o = (duk_codepoint_t) duk_bd_decode(bd_ctx, 16);
			count = (duk_small_int_t) duk_bd_decode(bd_ctx, 7);
			DUK_DDD(DUK_DDDPRINT("range: start_i=%ld, start_o=%ld, count=%ld, skip=%ld",
			                     (long) start_i,
			                     (long) start_o,
			                     (long) count,
			                     (long) skip));

			if (cp >= start_i) {
				tmp_cp = cp - start_i; /* always >= 0 */
				if (tmp_cp < (duk_codepoint_t) count * (duk_codepoint_t) skip &&
				    (tmp_cp % (duk_codepoint_t) skip) == 0) {
					DUK_DDD(DUK_DDDPRINT("range matches input codepoint"));
					cp = start_o + tmp_cp;
					goto single;
				}
			}
		}
	}

	/* 1:1 conversion */
	n = (duk_small_int_t) duk_bd_decode(bd_ctx, 7);
	DUK_DDD(DUK_DDDPRINT("checking 1:1 conversions (count %ld)", (long) n));
	while (n--) {
		start_i = (duk_codepoint_t) duk_bd_decode(bd_ctx, 16);
		start_o = (duk_codepoint_t) duk_bd_decode(bd_ctx, 16);
		DUK_DDD(DUK_DDDPRINT("1:1 conversion %ld -> %ld", (long) start_i, (long) start_o));
		if (cp == start_i) {
			DUK_DDD(DUK_DDDPRINT("1:1 matches input codepoint"));
			cp = start_o;
			goto single;
		}
	}

	/* complex, multicharacter conversion */
	n = (duk_small_int_t) duk_bd_decode(bd_ctx, 7);
	DUK_DDD(DUK_DDDPRINT("checking 1:n conversions (count %ld)", (long) n));
	while (n--) {
		start_i = (duk_codepoint_t) duk_bd_decode(bd_ctx, 16);
		t = (duk_small_int_t) duk_bd_decode(bd_ctx, 2);
		DUK_DDD(DUK_DDDPRINT("1:n conversion %ld -> %ld chars", (long) start_i, (long) t));
		if (cp == start_i) {
			DUK_DDD(DUK_DDDPRINT("1:n matches input codepoint"));
			if (bw != NULL) {
				while (t--) {
					tmp_cp = (duk_codepoint_t) duk_bd_decode(bd_ctx, 16);
					DUK_BW_WRITE_RAW_XUTF8(thr, bw, (duk_ucodepoint_t) tmp_cp);
				}
			}
			return -1;
		} else {
			while (t--) {
				(void) duk_bd_decode(bd_ctx, 16);
			}
		}
	}

	/* default: no change */
	DUK_DDD(DUK_DDDPRINT("no rule matches, output is same as input"));
	/* fall through */

single:
	if (bw != NULL) {
		DUK_BW_WRITE_RAW_XUTF8(thr, bw, (duk_ucodepoint_t) cp);
	}
	return cp;
}

/*
 *  Case conversion helper, with context/local sensitivity.
 *  For proper case conversion, one needs to know the character
 *  and the preceding and following characters, as well as
 *  locale/language.
 */

/* XXX: add 'language' argument when locale/language sensitive rule
 * support added.
 */
DUK_LOCAL
duk_codepoint_t duk__case_transform_helper(duk_hthread *thr,
                                           duk_bufwriter_ctx *bw,
                                           duk_codepoint_t cp,
                                           duk_codepoint_t prev,
                                           duk_codepoint_t next,
                                           duk_bool_t uppercase) {
	duk_bitdecoder_ctx bd_ctx;

	/* fast path for ASCII */
	if (cp < 0x80L) {
		/* XXX: there are language sensitive rules for the ASCII range.
		 * If/when language/locale support is implemented, they need to
		 * be implemented here for the fast path.  There are no context
		 * sensitive rules for ASCII range.
		 */

		if (uppercase) {
			if (cp >= 'a' && cp <= 'z') {
				cp = cp - 'a' + 'A';
			}
		} else {
			if (cp >= 'A' && cp <= 'Z') {
				cp = cp - 'A' + 'a';
			}
		}

		if (bw != NULL) {
			DUK_BW_WRITE_RAW_U8(thr, bw, (duk_uint8_t) cp);
		}
		return cp;
	}

	/* context and locale specific rules which cannot currently be represented
	 * in the caseconv bitstream: hardcoded rules in C
	 */
	if (uppercase) {
		/* XXX: turkish / azeri */
	} else {
		/*
		 *  Final sigma context specific rule.  This is a rather tricky
		 *  rule and this handling is probably not 100% correct now.
		 *  The rule is not locale/language specific so it is supported.
		 */

		if (cp == 0x03a3L && /* U+03A3 = GREEK CAPITAL LETTER SIGMA */
		    duk_unicode_is_letter(prev) && /* prev exists and is not a letter */
		    !duk_unicode_is_letter(next)) { /* next does not exist or next is not a letter */
			/* Capital sigma occurred at "end of word", lowercase to
			 * U+03C2 = GREEK SMALL LETTER FINAL SIGMA.  Otherwise
			 * fall through and let the normal rules lowercase it to
			 * U+03C3 = GREEK SMALL LETTER SIGMA.
			 */
			cp = 0x03c2L;
			goto singlechar;
		}

		/* XXX: lithuanian not implemented */
		/* XXX: lithuanian, explicit dot rules */
		/* XXX: turkish / azeri, lowercase rules */
	}

	/* 1:1 or special conversions, but not locale/context specific: script generated rules */
	duk_memzero(&bd_ctx, sizeof(bd_ctx));
	if (uppercase) {
		bd_ctx.data = (const duk_uint8_t *) duk_unicode_caseconv_uc;
		bd_ctx.length = (duk_size_t) sizeof(duk_unicode_caseconv_uc);
	} else {
		bd_ctx.data = (const duk_uint8_t *) duk_unicode_caseconv_lc;
		bd_ctx.length = (duk_size_t) sizeof(duk_unicode_caseconv_lc);
	}
	return duk__slow_case_conversion(thr, bw, cp, &bd_ctx);

singlechar:
	if (bw != NULL) {
		DUK_BW_WRITE_RAW_XUTF8(thr, bw, (duk_ucodepoint_t) cp);
	}
	return cp;

	/* unused now, not needed until Turkish/Azeri */
#if 0
 nochar:
	return -1;
#endif
}

/*
 *  Replace valstack top with case converted version.
 */

DUK_INTERNAL void duk_unicode_case_convert_string(duk_hthread *thr, duk_bool_t uppercase) {
	duk_hstring *h_input;
	duk_bufwriter_ctx bw_alloc;
	duk_bufwriter_ctx *bw;
	const duk_uint8_t *p, *p_start, *p_end;
	duk_codepoint_t prev, curr, next;

	h_input = duk_require_hstring(thr, -1); /* Accept symbols. */
	DUK_ASSERT(h_input != NULL);

	bw = &bw_alloc;
	DUK_BW_INIT_PUSHBUF(thr, bw, DUK_HSTRING_GET_BYTELEN(h_input));

	/* [ ... input buffer ] */

	p_start = (const duk_uint8_t *) DUK_HSTRING_GET_DATA(h_input);
	p_end = p_start + DUK_HSTRING_GET_BYTELEN(h_input);
	p = p_start;

	prev = -1;
	DUK_UNREF(prev);
	curr = -1;
	next = -1;
	for (;;) {
		prev = curr;
		curr = next;
		next = -1;
		if (p < p_end) {
			next = (duk_codepoint_t) duk_unicode_decode_xutf8_checked(thr, &p, p_start, p_end);
		} else {
			/* end of input and last char has been processed */
			if (curr < 0) {
				break;
			}
		}

		/* on first round, skip */
		if (curr >= 0) {
			/* XXX: could add a fast path to process chunks of input codepoints,
			 * but relative benefit would be quite small.
			 */

			/* Ensure space for maximum multi-character result; estimate is overkill. */
			DUK_BW_ENSURE(thr, bw, 8 * DUK_UNICODE_MAX_XUTF8_LENGTH);

			duk__case_transform_helper(thr, bw, (duk_codepoint_t) curr, prev, next, uppercase);
		}
	}

	DUK_BW_COMPACT(thr, bw);
	(void) duk_buffer_to_string(thr, -1); /* Safe, output is encoded. */
	/* invalidates h_buf pointer */
	duk_remove_m2(thr);
}

#if defined(DUK_USE_REGEXP_SUPPORT)

/*
 *  Canonicalize() abstract operation needed for canonicalization of individual
 *  codepoints during regexp compilation and execution, see E5 Section 15.10.2.8.
 *  Note that codepoints are canonicalized one character at a time, so no context
 *  specific rules can apply.  Locale specific rules can apply, though.
 */

DUK_INTERNAL duk_codepoint_t duk_unicode_re_canonicalize_char(duk_hthread *thr, duk_codepoint_t cp) {
#if defined(DUK_USE_REGEXP_CANON_WORKAROUND)
	/* Fast canonicalization lookup at the cost of 128kB footprint. */
	DUK_ASSERT(cp >= 0);
	DUK_UNREF(thr);
	if (DUK_LIKELY(cp < 0x10000L)) {
		return (duk_codepoint_t) duk_unicode_re_canon_lookup[cp];
	}
	return cp;
#else /* DUK_USE_REGEXP_CANON_WORKAROUND */
	duk_codepoint_t y;

	y = duk__case_transform_helper(thr,
	                               NULL, /* NULL is allowed, no output */
	                               cp, /* curr char */
	                               -1, /* prev char */
	                               -1, /* next char */
	                               1); /* uppercase */

	if ((y < 0) || (cp >= 0x80 && y < 0x80)) {
		/* multiple codepoint conversion or non-ASCII mapped to ASCII
		 * --> leave as is.
		 */
		return cp;
	}

	return y;
#endif /* DUK_USE_REGEXP_CANON_WORKAROUND */
}

/*
 *  E5 Section 15.10.2.6 "IsWordChar" abstract operation.  Assume
 *  x < 0 for characters read outside the string.
 */

DUK_INTERNAL duk_small_int_t duk_unicode_re_is_wordchar(duk_codepoint_t x) {
	/*
	 *  Note: the description in E5 Section 15.10.2.6 has a typo, it
	 *  contains 'A' twice and lacks 'a'; the intent is [0-9a-zA-Z_].
	 */
	if ((x >= '0' && x <= '9') || (x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') || (x == '_')) {
		return 1;
	}
	return 0;
}

/*
 *  Regexp range tables
 */

/* exposed because lexer needs these too */
DUK_INTERNAL const duk_uint16_t duk_unicode_re_ranges_digit[2] = {
	(duk_uint16_t) 0x0030UL,
	(duk_uint16_t) 0x0039UL,
};
DUK_INTERNAL const duk_uint16_t duk_unicode_re_ranges_white[22] = {
	(duk_uint16_t) 0x0009UL, (duk_uint16_t) 0x000DUL, (duk_uint16_t) 0x0020UL, (duk_uint16_t) 0x0020UL, (duk_uint16_t) 0x00A0UL,
	(duk_uint16_t) 0x00A0UL, (duk_uint16_t) 0x1680UL, (duk_uint16_t) 0x1680UL, (duk_uint16_t) 0x180EUL, (duk_uint16_t) 0x180EUL,
	(duk_uint16_t) 0x2000UL, (duk_uint16_t) 0x200AUL, (duk_uint16_t) 0x2028UL, (duk_uint16_t) 0x2029UL, (duk_uint16_t) 0x202FUL,
	(duk_uint16_t) 0x202FUL, (duk_uint16_t) 0x205FUL, (duk_uint16_t) 0x205FUL, (duk_uint16_t) 0x3000UL, (duk_uint16_t) 0x3000UL,
	(duk_uint16_t) 0xFEFFUL, (duk_uint16_t) 0xFEFFUL,
};
DUK_INTERNAL const duk_uint16_t duk_unicode_re_ranges_wordchar[8] = {
	(duk_uint16_t) 0x0030UL, (duk_uint16_t) 0x0039UL, (duk_uint16_t) 0x0041UL, (duk_uint16_t) 0x005AUL,
	(duk_uint16_t) 0x005FUL, (duk_uint16_t) 0x005FUL, (duk_uint16_t) 0x0061UL, (duk_uint16_t) 0x007AUL,
};
DUK_INTERNAL const duk_uint16_t duk_unicode_re_ranges_not_digit[4] = {
	(duk_uint16_t) 0x0000UL,
	(duk_uint16_t) 0x002FUL,
	(duk_uint16_t) 0x003AUL,
	(duk_uint16_t) 0xFFFFUL,
};
DUK_INTERNAL const duk_uint16_t duk_unicode_re_ranges_not_white[24] = {
	(duk_uint16_t) 0x0000UL, (duk_uint16_t) 0x0008UL, (duk_uint16_t) 0x000EUL, (duk_uint16_t) 0x001FUL, (duk_uint16_t) 0x0021UL,
	(duk_uint16_t) 0x009FUL, (duk_uint16_t) 0x00A1UL, (duk_uint16_t) 0x167FUL, (duk_uint16_t) 0x1681UL, (duk_uint16_t) 0x180DUL,
	(duk_uint16_t) 0x180FUL, (duk_uint16_t) 0x1FFFUL, (duk_uint16_t) 0x200BUL, (duk_uint16_t) 0x2027UL, (duk_uint16_t) 0x202AUL,
	(duk_uint16_t) 0x202EUL, (duk_uint16_t) 0x2030UL, (duk_uint16_t) 0x205EUL, (duk_uint16_t) 0x2060UL, (duk_uint16_t) 0x2FFFUL,
	(duk_uint16_t) 0x3001UL, (duk_uint16_t) 0xFEFEUL, (duk_uint16_t) 0xFF00UL, (duk_uint16_t) 0xFFFFUL,
};
DUK_INTERNAL const duk_uint16_t duk_unicode_re_ranges_not_wordchar[10] = {
	(duk_uint16_t) 0x0000UL, (duk_uint16_t) 0x002FUL, (duk_uint16_t) 0x003AUL, (duk_uint16_t) 0x0040UL, (duk_uint16_t) 0x005BUL,
	(duk_uint16_t) 0x005EUL, (duk_uint16_t) 0x0060UL, (duk_uint16_t) 0x0060UL, (duk_uint16_t) 0x007BUL, (duk_uint16_t) 0xFFFFUL,
};

#endif /* DUK_USE_REGEXP_SUPPORT */
