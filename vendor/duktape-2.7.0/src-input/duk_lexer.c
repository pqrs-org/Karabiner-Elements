/*
 *  Lexer for source files, ToNumber() string conversions, RegExp expressions,
 *  and JSON.
 *
 *  Provides a stream of ECMAScript tokens from an UTF-8/CESU-8 buffer.  The
 *  caller can also rewind the token stream into a certain position which is
 *  needed by the compiler part for multi-pass scanning.  Tokens are
 *  represented as duk_token structures, and contain line number information.
 *  Token types are identified with DUK_TOK_* defines.
 *
 *  Characters are decoded into a fixed size lookup window consisting of
 *  decoded Unicode code points, with window positions past the end of the
 *  input filled with an invalid codepoint (-1).  The tokenizer can thus
 *  perform multiple character lookups efficiently and with few sanity
 *  checks (such as access outside the end of the input), which keeps the
 *  tokenization code small at the cost of performance.
 *
 *  Character data in tokens, such as identifier names and string literals,
 *  is encoded into CESU-8 format on-the-fly while parsing the token in
 *  question.  The string data is made reachable to garbage collection by
 *  placing the token-related values in value stack entries allocated for
 *  this purpose by the caller.  The characters exist in Unicode code point
 *  form only in the fixed size lookup window, which keeps character data
 *  expansion (of especially ASCII data) low.
 *
 *  Token parsing supports the full range of Unicode characters as described
 *  in the E5 specification.  Parsing has been optimized for ASCII characters
 *  because ordinary ECMAScript code consists almost entirely of ASCII
 *  characters.  Matching of complex Unicode codepoint sets (such as in the
 *  IdentifierStart and IdentifierPart productions) is optimized for size,
 *  and is done using a linear scan of a bit-packed list of ranges.  This is
 *  very slow, but should never be entered unless the source code actually
 *  contains Unicode characters.
 *
 *  ECMAScript tokenization is partially context sensitive.  First,
 *  additional future reserved words are recognized in strict mode (see E5
 *  Section 7.6.1.2).  Second, a forward slash character ('/') can be
 *  recognized either as starting a RegExp literal or as a division operator,
 *  depending on context.  The caller must provide necessary context flags
 *  when requesting a new token.
 *
 *  Future work:
 *
 *    * Make line number tracking optional, as it consumes space.
 *
 *    * Add a feature flag for disabling UTF-8 decoding of input, as most
 *      source code is ASCII.  Because of Unicode escapes written in ASCII,
 *      this does not allow Unicode support to be removed from e.g.
 *      duk_unicode_is_identifier_start() nor does it allow removal of CESU-8
 *      encoding of e.g. string literals.
 *
 *    * Add a feature flag for disabling Unicode compliance of e.g. identifier
 *      names.  This allows for a build more than a kilobyte smaller, because
 *      Unicode ranges needed by duk_unicode_is_identifier_start() and
 *      duk_unicode_is_identifier_part() can be dropped.  String literals
 *      should still be allowed to contain escaped Unicode, so this still does
 *      not allow removal of CESU-8 encoding of e.g. string literals.
 *
 *    * Character lookup tables for codepoints above BMP could be stripped.
 *
 *    * Strictly speaking, E5 specification requires that source code consists
 *      of 16-bit code units, and if not, must be conceptually converted to
 *      that format first.  The current lexer processes Unicode code points
 *      and allows characters outside the BMP.  These should be converted to
 *      surrogate pairs while reading the source characters into the window,
 *      not after tokens have been formed (as is done now).  However, the fix
 *      is not trivial because two characters are decoded from one codepoint.
 *
 *    * Optimize for speed as well as size.  Large if-else ladders are (at
 *      least potentially) slow.
 */

#include "duk_internal.h"

/*
 *  Various defines and file specific helper macros
 */

#define DUK__MAX_RE_DECESC_DIGITS 9
#define DUK__MAX_RE_QUANT_DIGITS  9 /* Does not allow e.g. 2**31-1, but one more would allow overflows of u32. */

/* whether to use macros or helper function depends on call count */
#define DUK__ISDIGIT(x)    ((x) >= DUK_ASC_0 && (x) <= DUK_ASC_9)
#define DUK__ISHEXDIGIT(x) duk__is_hex_digit((x))
#define DUK__ISOCTDIGIT(x) ((x) >= DUK_ASC_0 && (x) <= DUK_ASC_7)
#define DUK__ISDIGIT03(x)  ((x) >= DUK_ASC_0 && (x) <= DUK_ASC_3)
#define DUK__ISDIGIT47(x)  ((x) >= DUK_ASC_4 && (x) <= DUK_ASC_7)

/* lexer character window helpers */
#define DUK__LOOKUP(lex_ctx, idx)           ((lex_ctx)->window[(idx)].codepoint)
#define DUK__ADVANCECHARS(lex_ctx, count)   duk__advance_chars((lex_ctx), (count))
#define DUK__ADVANCEBYTES(lex_ctx, count)   duk__advance_bytes((lex_ctx), (count))
#define DUK__INITBUFFER(lex_ctx)            duk__initbuffer((lex_ctx))
#define DUK__APPENDBUFFER(lex_ctx, x)       duk__appendbuffer((lex_ctx), (duk_codepoint_t) (x))
#define DUK__APPENDBUFFER_ASCII(lex_ctx, x) duk__appendbuffer_ascii((lex_ctx), (duk_codepoint_t) (x))

/* lookup shorthands (note: assume context variable is named 'lex_ctx') */
#define DUK__L0() DUK__LOOKUP(lex_ctx, 0)
#define DUK__L1() DUK__LOOKUP(lex_ctx, 1)
#define DUK__L2() DUK__LOOKUP(lex_ctx, 2)
#define DUK__L3() DUK__LOOKUP(lex_ctx, 3)
#define DUK__L4() DUK__LOOKUP(lex_ctx, 4)
#define DUK__L5() DUK__LOOKUP(lex_ctx, 5)

/* packed advance/token number macro used by multiple functions */
#define DUK__ADVTOK(advbytes, tok) ((((advbytes) * sizeof(duk_lexer_codepoint)) << 8) + (tok))

/*
 *  Advance lookup window by N characters, filling in new characters as
 *  necessary.  After returning caller is guaranteed a character window of
 *  at least DUK_LEXER_WINDOW_SIZE characters.
 *
 *  The main function duk__advance_bytes() is called at least once per every
 *  token so it has a major lexer/compiler performance impact.  There are two
 *  variants for the main duk__advance_bytes() algorithm: a sliding window
 *  approach which is slightly faster at the cost of larger code footprint,
 *  and a simple copying one.
 *
 *  Decoding directly from the source string would be another lexing option.
 *  But the lookup window based approach has the advantage of hiding the
 *  source string and its encoding effectively which gives more flexibility
 *  going forward to e.g. support chunked streaming of source from flash.
 *
 *  Decodes UTF-8/CESU-8 leniently with support for code points from U+0000 to
 *  U+10FFFF, causing an error if the input is unparseable.  Leniency means:
 *
 *    * Unicode code point validation is intentionally not performed,
 *      except to check that the codepoint does not exceed 0x10ffff.
 *
 *    * In particular, surrogate pairs are allowed and not combined, which
 *      allows source files to represent all SourceCharacters with CESU-8.
 *      Broken surrogate pairs are allowed, as ECMAScript does not mandate
 *      their validation.
 *
 *    * Allow non-shortest UTF-8 encodings.
 *
 *  Leniency here causes few security concerns because all character data is
 *  decoded into Unicode codepoints before lexer processing, and is then
 *  re-encoded into CESU-8.  The source can be parsed as strict UTF-8 with
 *  a compiler option.  However, ECMAScript source characters include -all-
 *  16-bit unsigned integer codepoints, so leniency seems to be appropriate.
 *
 *  Note that codepoints above the BMP are not strictly SourceCharacters,
 *  but the lexer still accepts them as such.  Before ending up in a string
 *  or an identifier name, codepoints above BMP are converted into surrogate
 *  pairs and then CESU-8 encoded, resulting in 16-bit Unicode data as
 *  expected by ECMAScript.
 *
 *  An alternative approach to dealing with invalid or partial sequences
 *  would be to skip them and replace them with e.g. the Unicode replacement
 *  character U+FFFD.  This has limited utility because a replacement character
 *  will most likely cause a parse error, unless it occurs inside a string.
 *  Further, ECMAScript source is typically pure ASCII.
 *
 *  See:
 *
 *     http://en.wikipedia.org/wiki/UTF-8
 *     http://en.wikipedia.org/wiki/CESU-8
 *     http://tools.ietf.org/html/rfc3629
 *     http://en.wikipedia.org/wiki/UTF-8#Invalid_byte_sequences
 *
 *  Future work:
 *
 *    * Reject other invalid Unicode sequences (see Wikipedia entry for examples)
 *      in strict UTF-8 mode.
 *
 *    * Size optimize.  An attempt to use a 16-byte lookup table for the first
 *      byte resulted in a code increase though.
 *
 *    * Is checking against maximum 0x10ffff really useful?  4-byte encoding
 *      imposes a certain limit anyway.
 *
 *    * Support chunked streaming of source code.  Can be implemented either
 *      by streaming chunks of bytes or chunks of codepoints.
 */

#if defined(DUK_USE_LEXER_SLIDING_WINDOW)
DUK_LOCAL void duk__fill_lexer_buffer(duk_lexer_ctx *lex_ctx, duk_small_uint_t start_offset_bytes) {
	duk_lexer_codepoint *cp, *cp_end;
	duk_ucodepoint_t x;
	duk_small_uint_t contlen;
	const duk_uint8_t *p, *p_end;
#if defined(DUK_USE_STRICT_UTF8_SOURCE)
	duk_ucodepoint_t mincp;
#endif
	duk_int_t input_line;

	/* Use temporaries and update lex_ctx only when finished. */
	input_line = lex_ctx->input_line;
	p = lex_ctx->input + lex_ctx->input_offset;
	p_end = lex_ctx->input + lex_ctx->input_length;

	cp = (duk_lexer_codepoint *) (void *) ((duk_uint8_t *) lex_ctx->buffer + start_offset_bytes);
	cp_end = lex_ctx->buffer + DUK_LEXER_BUFFER_SIZE;

	for (; cp != cp_end; cp++) {
		cp->offset = (duk_size_t) (p - lex_ctx->input);
		cp->line = input_line;

		/* XXX: potential issue with signed pointers, p_end < p. */
		if (DUK_UNLIKELY(p >= p_end)) {
			/* If input_offset were assigned a negative value, it would
			 * result in a large positive value.  Most likely it would be
			 * larger than input_length and be caught here.  In any case
			 * no memory unsafe behavior would happen.
			 */
			cp->codepoint = -1;
			continue;
		}

		x = (duk_ucodepoint_t) (*p++);

		/* Fast path. */

		if (DUK_LIKELY(x < 0x80UL)) {
			DUK_ASSERT(x != 0x2028UL && x != 0x2029UL); /* not LS/PS */
			if (DUK_UNLIKELY(x <= 0x000dUL)) {
				if ((x == 0x000aUL) || ((x == 0x000dUL) && (p >= p_end || *p != 0x000aUL))) {
					/* lookup for 0x000a above assumes shortest encoding now */

					/* E5 Section 7.3, treat the following as newlines:
					 *   LF
					 *   CR [not followed by LF]
					 *   LS
					 *   PS
					 *
					 * For CR LF, CR is ignored if it is followed by LF, and the LF will bump
					 * the line number.
					 */
					input_line++;
				}
			}

			cp->codepoint = (duk_codepoint_t) x;
			continue;
		}

		/* Slow path. */

		if (x < 0xc0UL) {
			/* 10xx xxxx -> invalid */
			goto error_encoding;
		} else if (x < 0xe0UL) {
			/* 110x xxxx   10xx xxxx  */
			contlen = 1;
#if defined(DUK_USE_STRICT_UTF8_SOURCE)
			mincp = 0x80UL;
#endif
			x = x & 0x1fUL;
		} else if (x < 0xf0UL) {
			/* 1110 xxxx   10xx xxxx   10xx xxxx */
			contlen = 2;
#if defined(DUK_USE_STRICT_UTF8_SOURCE)
			mincp = 0x800UL;
#endif
			x = x & 0x0fUL;
		} else if (x < 0xf8UL) {
			/* 1111 0xxx   10xx xxxx   10xx xxxx   10xx xxxx */
			contlen = 3;
#if defined(DUK_USE_STRICT_UTF8_SOURCE)
			mincp = 0x10000UL;
#endif
			x = x & 0x07UL;
		} else {
			/* no point in supporting encodings of 5 or more bytes */
			goto error_encoding;
		}

		DUK_ASSERT(p_end >= p);
		if ((duk_size_t) contlen > (duk_size_t) (p_end - p)) {
			goto error_clipped;
		}

		while (contlen > 0) {
			duk_small_uint_t y;
			y = *p++;
			if ((y & 0xc0U) != 0x80U) {
				/* check that byte has the form 10xx xxxx */
				goto error_encoding;
			}
			x = x << 6;
			x += y & 0x3fUL;
			contlen--;
		}

		/* check final character validity */

		if (x > 0x10ffffUL) {
			goto error_encoding;
		}
#if defined(DUK_USE_STRICT_UTF8_SOURCE)
		if (x < mincp || (x >= 0xd800UL && x <= 0xdfffUL) || x == 0xfffeUL) {
			goto error_encoding;
		}
#endif

		DUK_ASSERT(x != 0x000aUL && x != 0x000dUL);
		if ((x == 0x2028UL) || (x == 0x2029UL)) {
			input_line++;
		}

		cp->codepoint = (duk_codepoint_t) x;
	}

	lex_ctx->input_offset = (duk_size_t) (p - lex_ctx->input);
	lex_ctx->input_line = input_line;
	return;

error_clipped: /* clipped codepoint */
error_encoding: /* invalid codepoint encoding or codepoint */
	lex_ctx->input_offset = (duk_size_t) (p - lex_ctx->input);
	lex_ctx->input_line = input_line;

	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_SOURCE_DECODE_FAILED);
	DUK_WO_NORETURN(return;);
}

DUK_LOCAL void duk__advance_bytes(duk_lexer_ctx *lex_ctx, duk_small_uint_t count_bytes) {
	duk_small_uint_t used_bytes, avail_bytes;

	DUK_ASSERT_DISABLE(count_bytes >= 0); /* unsigned */
	DUK_ASSERT(count_bytes <= (duk_small_uint_t) (DUK_LEXER_WINDOW_SIZE * sizeof(duk_lexer_codepoint)));
	DUK_ASSERT(lex_ctx->window >= lex_ctx->buffer);
	DUK_ASSERT(lex_ctx->window < lex_ctx->buffer + DUK_LEXER_BUFFER_SIZE);
	DUK_ASSERT((duk_uint8_t *) lex_ctx->window + count_bytes <=
	           (duk_uint8_t *) lex_ctx->buffer + DUK_LEXER_BUFFER_SIZE * sizeof(duk_lexer_codepoint));

	/* Zero 'count' is also allowed to make call sites easier.
	 * Arithmetic in bytes generates better code in GCC.
	 */

	lex_ctx->window = (duk_lexer_codepoint *) (void *) ((duk_uint8_t *) lex_ctx->window + count_bytes); /* avoid multiply */
	used_bytes = (duk_small_uint_t) ((duk_uint8_t *) lex_ctx->window - (duk_uint8_t *) lex_ctx->buffer);
	avail_bytes = DUK_LEXER_BUFFER_SIZE * sizeof(duk_lexer_codepoint) - used_bytes;
	if (avail_bytes < (duk_small_uint_t) (DUK_LEXER_WINDOW_SIZE * sizeof(duk_lexer_codepoint))) {
		/* Not enough data to provide a full window, so "scroll" window to
		 * start of buffer and fill up the rest.
		 */
		duk_memmove((void *) lex_ctx->buffer, (const void *) lex_ctx->window, (size_t) avail_bytes);
		lex_ctx->window = lex_ctx->buffer;
		duk__fill_lexer_buffer(lex_ctx, avail_bytes);
	}
}

DUK_LOCAL void duk__init_lexer_window(duk_lexer_ctx *lex_ctx) {
	lex_ctx->window = lex_ctx->buffer;
	duk__fill_lexer_buffer(lex_ctx, 0);
}
#else /* DUK_USE_LEXER_SLIDING_WINDOW */
DUK_LOCAL duk_codepoint_t duk__read_char(duk_lexer_ctx *lex_ctx) {
	duk_ucodepoint_t x;
	duk_small_uint_t len;
	duk_small_uint_t i;
	const duk_uint8_t *p;
#if defined(DUK_USE_STRICT_UTF8_SOURCE)
	duk_ucodepoint_t mincp;
#endif
	duk_size_t input_offset;

	input_offset = lex_ctx->input_offset;
	if (DUK_UNLIKELY(input_offset >= lex_ctx->input_length)) {
		/* If input_offset were assigned a negative value, it would
		 * result in a large positive value.  Most likely it would be
		 * larger than input_length and be caught here.  In any case
		 * no memory unsafe behavior would happen.
		 */
		return -1;
	}

	p = lex_ctx->input + input_offset;
	x = (duk_ucodepoint_t) (*p);

	if (DUK_LIKELY(x < 0x80UL)) {
		/* 0xxx xxxx -> fast path */

		/* input offset tracking */
		lex_ctx->input_offset++;

		DUK_ASSERT(x != 0x2028UL && x != 0x2029UL); /* not LS/PS */
		if (DUK_UNLIKELY(x <= 0x000dUL)) {
			if ((x == 0x000aUL) || ((x == 0x000dUL) && (lex_ctx->input_offset >= lex_ctx->input_length ||
			                                            lex_ctx->input[lex_ctx->input_offset] != 0x000aUL))) {
				/* lookup for 0x000a above assumes shortest encoding now */

				/* E5 Section 7.3, treat the following as newlines:
				 *   LF
				 *   CR [not followed by LF]
				 *   LS
				 *   PS
				 *
				 * For CR LF, CR is ignored if it is followed by LF, and the LF will bump
				 * the line number.
				 */
				lex_ctx->input_line++;
			}
		}

		return (duk_codepoint_t) x;
	}

	/* Slow path. */

	if (x < 0xc0UL) {
		/* 10xx xxxx -> invalid */
		goto error_encoding;
	} else if (x < 0xe0UL) {
		/* 110x xxxx   10xx xxxx  */
		len = 2;
#if defined(DUK_USE_STRICT_UTF8_SOURCE)
		mincp = 0x80UL;
#endif
		x = x & 0x1fUL;
	} else if (x < 0xf0UL) {
		/* 1110 xxxx   10xx xxxx   10xx xxxx */
		len = 3;
#if defined(DUK_USE_STRICT_UTF8_SOURCE)
		mincp = 0x800UL;
#endif
		x = x & 0x0fUL;
	} else if (x < 0xf8UL) {
		/* 1111 0xxx   10xx xxxx   10xx xxxx   10xx xxxx */
		len = 4;
#if defined(DUK_USE_STRICT_UTF8_SOURCE)
		mincp = 0x10000UL;
#endif
		x = x & 0x07UL;
	} else {
		/* no point in supporting encodings of 5 or more bytes */
		goto error_encoding;
	}

	DUK_ASSERT(lex_ctx->input_length >= lex_ctx->input_offset);
	if ((duk_size_t) len > (duk_size_t) (lex_ctx->input_length - lex_ctx->input_offset)) {
		goto error_clipped;
	}

	p++;
	for (i = 1; i < len; i++) {
		duk_small_uint_t y;
		y = *p++;
		if ((y & 0xc0U) != 0x80U) {
			/* check that byte has the form 10xx xxxx */
			goto error_encoding;
		}
		x = x << 6;
		x += y & 0x3fUL;
	}

	/* check final character validity */

	if (x > 0x10ffffUL) {
		goto error_encoding;
	}
#if defined(DUK_USE_STRICT_UTF8_SOURCE)
	if (x < mincp || (x >= 0xd800UL && x <= 0xdfffUL) || x == 0xfffeUL) {
		goto error_encoding;
	}
#endif

	/* input offset tracking */
	lex_ctx->input_offset += len;

	/* line tracking */
	DUK_ASSERT(x != 0x000aUL && x != 0x000dUL);
	if ((x == 0x2028UL) || (x == 0x2029UL)) {
		lex_ctx->input_line++;
	}

	return (duk_codepoint_t) x;

error_clipped: /* clipped codepoint */
error_encoding: /* invalid codepoint encoding or codepoint */
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_SOURCE_DECODE_FAILED);
	DUK_WO_NORETURN(return 0;);
}

DUK_LOCAL void duk__advance_bytes(duk_lexer_ctx *lex_ctx, duk_small_uint_t count_bytes) {
	duk_small_uint_t keep_bytes;
	duk_lexer_codepoint *cp, *cp_end;

	DUK_ASSERT_DISABLE(count_bytes >= 0); /* unsigned */
	DUK_ASSERT(count_bytes <= (duk_small_uint_t) (DUK_LEXER_WINDOW_SIZE * sizeof(duk_lexer_codepoint)));

	/* Zero 'count' is also allowed to make call sites easier. */

	keep_bytes = DUK_LEXER_WINDOW_SIZE * sizeof(duk_lexer_codepoint) - count_bytes;
	duk_memmove((void *) lex_ctx->window, (const void *) ((duk_uint8_t *) lex_ctx->window + count_bytes), (size_t) keep_bytes);

	cp = (duk_lexer_codepoint *) ((duk_uint8_t *) lex_ctx->window + keep_bytes);
	cp_end = lex_ctx->window + DUK_LEXER_WINDOW_SIZE;
	for (; cp != cp_end; cp++) {
		cp->offset = lex_ctx->input_offset;
		cp->line = lex_ctx->input_line;
		cp->codepoint = duk__read_char(lex_ctx);
	}
}

DUK_LOCAL void duk__init_lexer_window(duk_lexer_ctx *lex_ctx) {
	/* Call with count == DUK_LEXER_WINDOW_SIZE to fill buffer initially. */
	duk__advance_bytes(lex_ctx, DUK_LEXER_WINDOW_SIZE * sizeof(duk_lexer_codepoint)); /* fill window */
}
#endif /* DUK_USE_LEXER_SLIDING_WINDOW */

DUK_LOCAL void duk__advance_chars(duk_lexer_ctx *lex_ctx, duk_small_uint_t count_chars) {
	duk__advance_bytes(lex_ctx, count_chars * sizeof(duk_lexer_codepoint));
}

/*
 *  (Re)initialize the temporary byte buffer.  May be called extra times
 *  with little impact.
 */

DUK_LOCAL void duk__initbuffer(duk_lexer_ctx *lex_ctx) {
	/* Reuse buffer as is unless buffer has grown large. */
	if (DUK_HBUFFER_DYNAMIC_GET_SIZE(lex_ctx->buf) < DUK_LEXER_TEMP_BUF_LIMIT) {
		/* Keep current size */
	} else {
		duk_hbuffer_resize(lex_ctx->thr, lex_ctx->buf, DUK_LEXER_TEMP_BUF_LIMIT);
	}

	DUK_BW_INIT_WITHBUF(lex_ctx->thr, &lex_ctx->bw, lex_ctx->buf);
}

/*
 *  Append a Unicode codepoint to the temporary byte buffer.  Performs
 *  CESU-8 surrogate pair encoding for codepoints above the BMP.
 *  Existing surrogate pairs are allowed and also encoded into CESU-8.
 */

DUK_LOCAL void duk__appendbuffer(duk_lexer_ctx *lex_ctx, duk_codepoint_t x) {
	/*
	 *  Since character data is only generated by decoding the source or by
	 *  the compiler itself, we rely on the input codepoints being correct
	 *  and avoid a check here.
	 *
	 *  Character data can also come here through decoding of Unicode
	 *  escapes ("\udead\ubeef") so all 16-but unsigned values can be
	 *  present, even when the source file itself is strict UTF-8.
	 */
	DUK_ASSERT(x >= 0 && x <= 0x10ffffL);

	DUK_BW_WRITE_ENSURE_CESU8(lex_ctx->thr, &lex_ctx->bw, (duk_ucodepoint_t) x);
}

DUK_LOCAL void duk__appendbuffer_ascii(duk_lexer_ctx *lex_ctx, duk_codepoint_t x) {
	/* ASCII characters can be emitted as a single byte without encoding
	 * which matters for some fast paths.
	 */
	DUK_ASSERT(x >= 0 && x <= 0x7f);

	DUK_BW_WRITE_ENSURE_U8(lex_ctx->thr, &lex_ctx->bw, (duk_uint8_t) x);
}

/*
 *  Intern the temporary byte buffer into a valstack slot
 *  (in practice, slot1 or slot2).
 */

DUK_LOCAL duk_hstring *duk__internbuffer(duk_lexer_ctx *lex_ctx, duk_idx_t valstack_idx) {
	DUK_ASSERT(valstack_idx == lex_ctx->slot1_idx || valstack_idx == lex_ctx->slot2_idx);

	DUK_BW_PUSH_AS_STRING(lex_ctx->thr, &lex_ctx->bw);
	duk_replace(lex_ctx->thr, valstack_idx);
	return duk_known_hstring(lex_ctx->thr, valstack_idx);
}

/*
 *  Init lexer context
 */

DUK_INTERNAL void duk_lexer_initctx(duk_lexer_ctx *lex_ctx) {
	DUK_ASSERT(lex_ctx != NULL);

	duk_memzero(lex_ctx, sizeof(*lex_ctx));
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
#if defined(DUK_USE_LEXER_SLIDING_WINDOW)
	lex_ctx->window = NULL;
#endif
	lex_ctx->thr = NULL;
	lex_ctx->input = NULL;
	lex_ctx->buf = NULL;
#endif
}

/*
 *  Set lexer input position and reinitialize lookup window.
 */

DUK_INTERNAL void duk_lexer_getpoint(duk_lexer_ctx *lex_ctx, duk_lexer_point *pt) {
	pt->offset = lex_ctx->window[0].offset;
	pt->line = lex_ctx->window[0].line;
}

DUK_INTERNAL void duk_lexer_setpoint(duk_lexer_ctx *lex_ctx, duk_lexer_point *pt) {
	DUK_ASSERT_DISABLE(pt->offset >= 0); /* unsigned */
	DUK_ASSERT(pt->line >= 1);
	lex_ctx->input_offset = pt->offset;
	lex_ctx->input_line = pt->line;
	duk__init_lexer_window(lex_ctx);
}

/*
 *  Lexing helpers
 */

/* Numeric value of a hex digit (also covers octal and decimal digits) or
 * -1 if not a valid hex digit.
 */
DUK_LOCAL duk_codepoint_t duk__hexval_validate(duk_codepoint_t x) {
	duk_small_int_t t;

	/* Here 'x' is a Unicode codepoint */
	if (DUK_LIKELY(x >= 0 && x <= 0xff)) {
		t = duk_hex_dectab[x];
		if (DUK_LIKELY(t >= 0)) {
			return t;
		}
	}

	return -1;
}

/* Just a wrapper for call sites where 'x' is known to be valid so
 * we assert for it before decoding.
 */
DUK_LOCAL duk_codepoint_t duk__hexval(duk_codepoint_t x) {
	duk_codepoint_t ret;

	DUK_ASSERT((x >= DUK_ASC_0 && x <= DUK_ASC_9) || (x >= DUK_ASC_LC_A && x <= DUK_ASC_LC_F) ||
	           (x >= DUK_ASC_UC_A && x <= DUK_ASC_UC_F));
	ret = duk__hexval_validate(x);
	DUK_ASSERT(ret >= 0 && ret <= 15);
	return ret;
}

/* having this as a separate function provided a size benefit */
DUK_LOCAL duk_bool_t duk__is_hex_digit(duk_codepoint_t x) {
	if (DUK_LIKELY(x >= 0 && x <= 0xff)) {
		return (duk_hex_dectab[x] >= 0);
	}
	return 0;
}

/* Parse a Unicode escape of the form \xHH, \uHHHH, or \u{H+}.  Shared by
 * source and RegExp parsing.
 */
DUK_LOCAL duk_codepoint_t duk__lexer_parse_escape(duk_lexer_ctx *lex_ctx, duk_bool_t allow_es6) {
	duk_small_int_t digits; /* Initial value 2 or 4 for fixed length escapes, 0 for ES2015 \u{H+}. */
	duk_codepoint_t escval;
	duk_codepoint_t x;
	duk_small_uint_t adv;

	DUK_ASSERT(DUK__L0() == DUK_ASC_BACKSLASH); /* caller responsibilities */
	DUK_ASSERT(DUK__L1() == DUK_ASC_LC_X || DUK__L1() == DUK_ASC_LC_U);
	DUK_UNREF(allow_es6);

	adv = 2;
	digits = 2;
	if (DUK__L1() == DUK_ASC_LC_U) {
		digits = 4;
#if defined(DUK_USE_ES6_UNICODE_ESCAPE)
		if (DUK__L2() == DUK_ASC_LCURLY && allow_es6) {
			digits = 0;
			adv = 3;
		}
#endif
	}
	DUK__ADVANCECHARS(lex_ctx, adv);

	escval = 0;
	for (;;) {
		/* One of the escape forms: \xHH, \uHHHH, \u{H+}.
		 * The 'digits' variable tracks parsing state and is
		 * initialized to:
		 *
		 *   \xHH     2
		 *   \uHH     4
		 *   \u{H+}   0 first time, updated to -1 to indicate
		 *            at least one digit has been parsed
		 *
		 * Octal parsing is handled separately because it can be
		 * done with fixed lookahead and also has validation
		 * rules which depend on the escape length (which is
		 * variable).
		 *
		 * We don't need a specific check for x < 0 (end of
		 * input) or duk_unicode_is_line_terminator(x)
		 * because the 'dig' decode will fail and lead to a
		 * SyntaxError.
		 */
		duk_codepoint_t dig;

		x = DUK__L0();
		DUK__ADVANCECHARS(lex_ctx, 1);

		dig = duk__hexval_validate(x);
		if (digits > 0) {
			digits--;
			if (dig < 0) {
				goto fail_escape;
			}
			DUK_ASSERT(dig >= 0x00 && dig <= 0x0f);
			escval = (escval << 4) + dig;
			if (digits == 0) {
				DUK_ASSERT(escval >= 0 && escval <= 0xffffL);
				break;
			}
		} else {
#if defined(DUK_USE_ES6_UNICODE_ESCAPE)
			DUK_ASSERT(digits == 0 /* first time */ || digits == -1 /* others */);
			if (dig >= 0) {
				DUK_ASSERT(dig >= 0x00 && dig <= 0x0f);
				escval = (escval << 4) + dig;
				if (escval > 0x10ffffL) {
					goto fail_escape;
				}
			} else if (x == DUK_ASC_RCURLY) {
				if (digits == 0) {
					/* Empty escape, \u{}. */
					goto fail_escape;
				}
				DUK_ASSERT(escval >= 0 && escval <= 0x10ffffL);
				break;
			} else {
				goto fail_escape;
			}
			digits = -1; /* Indicate we have at least one digit. */
#else /* DUK_USE_ES6_UNICODE_ESCAPE */
			DUK_ASSERT(0); /* Never happens if \u{H+} support disabled. */
#endif /* DUK_USE_ES6_UNICODE_ESCAPE */
		}
	}

	return escval;

fail_escape:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_INVALID_ESCAPE);
	DUK_WO_NORETURN(return 0;);
}

/* Parse legacy octal escape of the form \N{1,3}, e.g. \0, \5, \0377.  Maximum
 * allowed value is \0377 (U+00FF), longest match is used.  Used for both string
 * RegExp octal escape parsing.  Window[0] must be the slash '\' and the first
 * digit must already be validated to be in [0-9] by the caller.
 */
DUK_LOCAL duk_codepoint_t duk__lexer_parse_legacy_octal(duk_lexer_ctx *lex_ctx,
                                                        duk_small_uint_t *out_adv,
                                                        duk_bool_t reject_annex_b) {
	duk_codepoint_t cp;
	duk_small_uint_t lookup_idx;
	duk_small_uint_t adv;
	duk_codepoint_t tmp;

	DUK_ASSERT(out_adv != NULL);
	DUK_ASSERT(DUK__LOOKUP(lex_ctx, 0) == DUK_ASC_BACKSLASH);
	DUK_ASSERT(DUK__LOOKUP(lex_ctx, 1) >= DUK_ASC_0 && DUK__LOOKUP(lex_ctx, 1) <= DUK_ASC_9);

	cp = 0;
	tmp = 0;
	for (lookup_idx = 1; lookup_idx <= 3; lookup_idx++) {
		DUK_DDD(DUK_DDDPRINT("lookup_idx=%ld, cp=%ld", (long) lookup_idx, (long) cp));
		tmp = DUK__LOOKUP(lex_ctx, lookup_idx);
		if (tmp < DUK_ASC_0 || tmp > DUK_ASC_7) {
			/* No more valid digits. */
			break;
		}
		tmp = (cp << 3) + (tmp - DUK_ASC_0);
		if (tmp > 0xff) {
			/* Three digit octal escapes above \377 (= 0xff)
			 * are not allowed.
			 */
			break;
		}
		cp = tmp;
	}
	DUK_DDD(DUK_DDDPRINT("final lookup_idx=%ld, cp=%ld", (long) lookup_idx, (long) cp));

	adv = lookup_idx;
	if (lookup_idx == 1) {
		DUK_DDD(DUK_DDDPRINT("\\8 or \\9 -> treat as literal, accept in strict mode too"));
		DUK_ASSERT(tmp == DUK_ASC_8 || tmp == DUK_ASC_9);
		cp = tmp;
		adv++; /* correction to above, eat offending character */
	} else if (lookup_idx == 2 && cp == 0) {
		/* Note: 'foo\0bar' is OK in strict mode, but 'foo\00bar' is not.
		 * It won't be interpreted as 'foo\u{0}0bar' but as a SyntaxError.
		 */
		DUK_DDD(DUK_DDDPRINT("\\0 -> accept in strict mode too"));
	} else {
		/* This clause also handles non-shortest zero, e.g. \00. */
		if (reject_annex_b) {
			DUK_DDD(DUK_DDDPRINT("non-zero octal literal %ld -> reject in strict-mode", (long) cp));
			cp = -1;
		} else {
			DUK_DDD(DUK_DDDPRINT("non-zero octal literal %ld -> accepted", (long) cp));
			DUK_ASSERT(cp >= 0 && cp <= 0xff);
		}
	}

	*out_adv = adv;

	DUK_ASSERT((cp >= 0 && cp <= 0xff) || (cp == -1 && reject_annex_b));
	return cp;
}

/* XXX: move strict mode to lex_ctx? */
DUK_LOCAL void duk__lexer_parse_string_literal(duk_lexer_ctx *lex_ctx,
                                               duk_token *out_token,
                                               duk_small_int_t quote,
                                               duk_bool_t strict_mode) {
	duk_small_uint_t adv;

	for (adv = 1 /* initial quote */;;) {
		duk_codepoint_t x;

		DUK__ADVANCECHARS(lex_ctx, adv); /* eat opening quote on first loop */
		x = DUK__L0();

		adv = 1;
		if (x == quote) {
			DUK__ADVANCECHARS(lex_ctx, 1); /* eat closing quote */
			break;
		} else if (x == '\\') {
			/* DUK__L0        -> '\' char
			 * DUK__L1 ... DUK__L5 -> more lookup
			 */
			duk_small_int_t emitcp = -1;

			x = DUK__L1();

			/* How much to advance before next loop. */
			adv = 2; /* note: long live range */

			switch (x) {
			case '\'':
				emitcp = 0x0027;
				break;
			case '"':
				emitcp = 0x0022;
				break;
			case '\\':
				emitcp = 0x005c;
				break;
			case 'b':
				emitcp = 0x0008;
				break;
			case 'f':
				emitcp = 0x000c;
				break;
			case 'n':
				emitcp = 0x000a;
				break;
			case 'r':
				emitcp = 0x000d;
				break;
			case 't':
				emitcp = 0x0009;
				break;
			case 'v':
				emitcp = 0x000b;
				break;
			case 'x':
			case 'u': {
				duk_codepoint_t esc_cp;
				esc_cp = duk__lexer_parse_escape(lex_ctx, 1 /*allow_es6*/);
				DUK__APPENDBUFFER(lex_ctx, esc_cp);
				adv = 0;
				break;
			}
			default: {
				if (duk_unicode_is_line_terminator(x)) {
					/* line continuation */
					if (x == 0x000d && DUK__L2() == 0x000a) {
						/* CR LF again a special case */
						adv = 3; /* line terminator, CR, LF */
					}
				} else if (DUK__ISDIGIT(x)) {
					/*
					 *  Octal escape or zero escape:
					 *    \0                                     (lookahead not OctalDigit)
					 *    \1 ... \7                              (lookahead not OctalDigit)
					 *    \ZeroToThree OctalDigit                (lookahead not OctalDigit)
					 *    \FourToSeven OctalDigit                (no lookahead restrictions)
					 *    \ZeroToThree OctalDigit OctalDigit     (no lookahead restrictions)
					 *
					 *  Zero escape is part of the standard syntax.  Octal escapes are
					 *  defined in E5 Section B.1.2, and are only allowed in non-strict mode.
					 *  Any other productions starting with a decimal digit are invalid
					 *  but are in practice treated like identity escapes.
					 *
					 *  Parse octal (up to 3 digits) from the lookup window.
					 */

					emitcp = duk__lexer_parse_legacy_octal(lex_ctx, &adv, strict_mode /*reject_annex_b*/);
					if (emitcp < 0) {
						goto fail_escape;
					}
				} else if (x < 0) {
					goto fail_unterminated;
				} else {
					/* escaped NonEscapeCharacter */
					DUK__APPENDBUFFER(lex_ctx, x);
				}
			} /* end default clause */
			} /* end switch */

			/* Shared handling for single codepoint escapes. */
			if (emitcp >= 0) {
				DUK__APPENDBUFFER(lex_ctx, emitcp);
			}

			/* Track number of escapes; count not really needed but directive
			 * prologues need to detect whether there were any escapes or line
			 * continuations or not.
			 */
			out_token->num_escapes++;
		} else if (x >= 0x20 && x <= 0x7f) {
			/* Fast path for ASCII case, avoids line terminator
			 * check and CESU-8 encoding.
			 */
			DUK_ASSERT(x >= 0);
			DUK_ASSERT(!duk_unicode_is_line_terminator(x));
			DUK_ASSERT(x != quote);
			DUK_ASSERT(x != DUK_ASC_BACKSLASH);
			DUK__APPENDBUFFER_ASCII(lex_ctx, x);
		} else if (x < 0 || duk_unicode_is_line_terminator(x)) {
			goto fail_unterminated;
		} else {
			/* Character which is part of the string but wasn't handled
			 * by the fast path.
			 */
			DUK__APPENDBUFFER(lex_ctx, x);
		}
	} /* string parse loop */

	return;

fail_escape:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_INVALID_ESCAPE);
	DUK_WO_NORETURN(return;);

fail_unterminated:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_UNTERMINATED_STRING);
	DUK_WO_NORETURN(return;);
}

/* Skip to end-of-line (or end-of-file), used for single line comments. */
DUK_LOCAL void duk__lexer_skip_to_endofline(duk_lexer_ctx *lex_ctx) {
	for (;;) {
		duk_codepoint_t x;

		x = DUK__L0();
		if (x < 0 || duk_unicode_is_line_terminator(x)) {
			break;
		}
		DUK__ADVANCECHARS(lex_ctx, 1);
	}
}

/*
 *  Parse ECMAScript source InputElementDiv or InputElementRegExp
 *  (E5 Section 7), skipping whitespace, comments, and line terminators.
 *
 *  Possible results are:
 *    (1) a token
 *    (2) a line terminator (skipped)
 *    (3) a comment (skipped)
 *    (4) EOF
 *
 *  White space is automatically skipped from the current position (but
 *  not after the input element).  If input has already ended, returns
 *  DUK_TOK_EOF indefinitely.  If a parse error occurs, uses an DUK_ERROR()
 *  macro call (and hence a longjmp through current heap longjmp context).
 *  Comments and line terminator tokens are automatically skipped.
 *
 *  The input element being matched is determined by regexp_mode; if set,
 *  parses a InputElementRegExp, otherwise a InputElementDiv.  The
 *  difference between these are handling of productions starting with a
 *  forward slash.
 *
 *  If strict_mode is set, recognizes additional future reserved words
 *  specific to strict mode, and refuses to parse octal literals.
 *
 *  The matching strategy below is to (currently) use a six character
 *  lookup window to quickly determine which production is the -longest-
 *  matching one, and then parse that.  The top-level if-else clauses
 *  match the first character, and the code blocks for each clause
 *  handle -all- alternatives for that first character.  ECMAScript
 *  specification uses the "longest match wins" semantics, so the order
 *  of the if-clauses matters.
 *
 *  Misc notes:
 *
 *    * ECMAScript numeric literals do not accept a sign character.
 *      Consequently e.g. "-1.0" is parsed as two tokens: a negative
 *      sign and a positive numeric literal.  The compiler performs
 *      the negation during compilation, so this has no adverse impact.
 *
 *    * There is no token for "undefined": it is just a value available
 *      from the global object (or simply established by doing a reference
 *      to an undefined value).
 *
 *    * Some contexts want Identifier tokens, which are IdentifierNames
 *      excluding reserved words, while some contexts want IdentifierNames
 *      directly.  In the latter case e.g. "while" is interpreted as an
 *      identifier name, not a DUK_TOK_WHILE token.  The solution here is
 *      to provide both token types: DUK_TOK_WHILE goes to 't' while
 *      DUK_TOK_IDENTIFIER goes to 't_nores', and 'slot1' always contains
 *      the identifier / keyword name.
 *
 *    * Directive prologue needs to identify string literals such as
 *      "use strict" and 'use strict', which are sensitive to line
 *      continuations and escape sequences.  For instance, "use\u0020strict"
 *      is a valid directive but is distinct from "use strict".  The solution
 *      here is to decode escapes while tokenizing, but to keep track of the
 *      number of escapes.  Directive detection can then check that the
 *      number of escapes is zero.
 *
 *    * Multi-line comments with one or more internal LineTerminator are
 *      treated like a line terminator to comply with automatic semicolon
 *      insertion.
 */

DUK_INTERNAL
void duk_lexer_parse_js_input_element(duk_lexer_ctx *lex_ctx,
                                      duk_token *out_token,
                                      duk_bool_t strict_mode,
                                      duk_bool_t regexp_mode) {
	duk_codepoint_t x; /* temporary, must be signed and 32-bit to hold Unicode code points */
	duk_small_uint_t advtok = 0; /* (advance << 8) + token_type, updated at function end,
	                              * init is unnecessary but suppresses "may be used uninitialized" warnings.
	                              */
	duk_bool_t got_lineterm = 0; /* got lineterm preceding non-whitespace, non-lineterm token */

	if (++lex_ctx->token_count >= lex_ctx->token_limit) {
		goto fail_token_limit;
	}

	out_token->t = DUK_TOK_EOF;
	out_token->t_nores = DUK_TOK_INVALID; /* marker: copy t if not changed */
#if 0 /* not necessary to init, disabled for faster parsing */
	out_token->num = DUK_DOUBLE_NAN;
	out_token->str1 = NULL;
	out_token->str2 = NULL;
#endif
	out_token->num_escapes = 0;
	/* out_token->lineterm set by caller */

	/* This would be nice, but parsing is faster without resetting the
	 * value slots.  The only side effect is that references to temporary
	 * string values may linger until lexing is finished; they're then
	 * freed normally.
	 */
#if 0
	duk_to_undefined(lex_ctx->thr, lex_ctx->slot1_idx);
	duk_to_undefined(lex_ctx->thr, lex_ctx->slot2_idx);
#endif

	/* 'advtok' indicates how much to advance and which token id to assign
	 * at the end.  This shared functionality minimizes code size.  All
	 * code paths are required to set 'advtok' to some value, so no default
	 * init value is used.  Code paths calling DUK_ERROR() never return so
	 * they don't need to set advtok.
	 */

	/*
	 *  Matching order:
	 *
	 *    Punctuator first chars, also covers comments, regexps
	 *    LineTerminator
	 *    Identifier or reserved word, also covers null/true/false literals
	 *    NumericLiteral
	 *    StringLiteral
	 *    EOF
	 *
	 *  The order does not matter as long as the longest match is
	 *  always correctly identified.  There are order dependencies
	 *  in the clauses, so it's not trivial to convert to a switch.
	 */

restart_lineupdate:
	out_token->start_line = lex_ctx->window[0].line;

restart:
	out_token->start_offset = lex_ctx->window[0].offset;

	x = DUK__L0();

	switch (x) {
	case DUK_ASC_SPACE:
	case DUK_ASC_HT: /* fast paths for space and tab */
		DUK__ADVANCECHARS(lex_ctx, 1);
		goto restart;
	case DUK_ASC_LF: /* LF line terminator; CR LF and Unicode lineterms are handled in slow path */
		DUK__ADVANCECHARS(lex_ctx, 1);
		got_lineterm = 1;
		goto restart_lineupdate;
#if defined(DUK_USE_SHEBANG_COMMENTS)
	case DUK_ASC_HASH: /* '#' */
		if (DUK__L1() == DUK_ASC_EXCLAMATION && lex_ctx->window[0].offset == 0 && (lex_ctx->flags & DUK_COMPILE_SHEBANG)) {
			/* "Shebang" comment ('#! ...') on first line. */
			/* DUK__ADVANCECHARS(lex_ctx, 2) would be correct here, but not necessary */
			duk__lexer_skip_to_endofline(lex_ctx);
			goto restart; /* line terminator will be handled on next round */
		}
		goto fail_token;
#endif /* DUK_USE_SHEBANG_COMMENTS */
	case DUK_ASC_SLASH: /* '/' */
		if (DUK__L1() == DUK_ASC_SLASH) {
			/*
			 *  E5 Section 7.4, allow SourceCharacter (which is any 16-bit
			 *  code point).
			 */

			/* DUK__ADVANCECHARS(lex_ctx, 2) would be correct here, but not necessary */
			duk__lexer_skip_to_endofline(lex_ctx);
			goto restart; /* line terminator will be handled on next round */
		} else if (DUK__L1() == DUK_ASC_STAR) {
			/*
			 *  E5 Section 7.4.  If the multi-line comment contains a newline,
			 *  it is treated like a single line terminator for automatic
			 *  semicolon insertion.
			 */

			duk_bool_t last_asterisk = 0;
			DUK__ADVANCECHARS(lex_ctx, 2);
			for (;;) {
				x = DUK__L0();
				if (x < 0) {
					goto fail_unterm_comment;
				}
				DUK__ADVANCECHARS(lex_ctx, 1);
				if (last_asterisk && x == DUK_ASC_SLASH) {
					break;
				}
				if (duk_unicode_is_line_terminator(x)) {
					got_lineterm = 1;
				}
				last_asterisk = (x == DUK_ASC_STAR);
			}
			goto restart_lineupdate;
		} else if (regexp_mode) {
#if defined(DUK_USE_REGEXP_SUPPORT)
			/*
			 *  "/" followed by something in regexp mode.  See E5 Section 7.8.5.
			 *
			 *  RegExp parsing is a bit complex.  First, the regexp body is delimited
			 *  by forward slashes, but the body may also contain forward slashes as
			 *  part of an escape sequence or inside a character class (delimited by
			 *  square brackets).  A mini state machine is used to implement these.
			 *
			 *  Further, an early (parse time) error must be thrown if the regexp
			 *  would cause a run-time error when used in the expression new RegExp(...).
			 *  Parsing here simply extracts the (candidate) regexp, and also accepts
			 *  invalid regular expressions (which are delimited properly).  The caller
			 *  (compiler) must perform final validation and regexp compilation.
			 *
			 *  RegExp first char may not be '/' (single line comment) or '*' (multi-
			 *  line comment).  These have already been checked above, so there is no
			 *  need below for special handling of the first regexp character as in
			 *  the E5 productions.
			 *
			 *  About unicode escapes within regexp literals:
			 *
			 *      E5 Section 7.8.5 grammar does NOT accept \uHHHH escapes.
			 *      However, Section 6 states that regexps accept the escapes,
			 *      see paragraph starting with "In string literals...".
			 *      The regexp grammar, which sees the decoded regexp literal
			 *      (after lexical parsing) DOES have a \uHHHH unicode escape.
			 *      So, for instance:
			 *
			 *          /\u1234/
			 *
			 *      should first be parsed by the lexical grammar as:
			 *
			 *          '\' 'u'      RegularExpressionBackslashSequence
			 *          '1'          RegularExpressionNonTerminator
			 *          '2'          RegularExpressionNonTerminator
			 *          '3'          RegularExpressionNonTerminator
			 *          '4'          RegularExpressionNonTerminator
			 *
			 *      and the escape itself is then parsed by the regexp engine.
			 *      This is the current implementation.
			 *
			 *  Minor spec inconsistency:
			 *
			 *      E5 Section 7.8.5 RegularExpressionBackslashSequence is:
			 *
			 *         \ RegularExpressionNonTerminator
			 *
			 *      while Section A.1 RegularExpressionBackslashSequence is:
			 *
			 *         \ NonTerminator
			 *
			 *      The latter is not normative and a typo.
			 *
			 */

			/* first, parse regexp body roughly */

			duk_small_int_t state = 0; /* 0=base, 1=esc, 2=class, 3=class+esc */

			DUK__INITBUFFER(lex_ctx);
			for (;;) {
				DUK__ADVANCECHARS(lex_ctx, 1); /* skip opening slash on first loop */
				x = DUK__L0();
				if (x < 0 || duk_unicode_is_line_terminator(x)) {
					goto fail_unterm_regexp;
				}
				x = DUK__L0(); /* re-read to avoid spill / fetch */
				if (state == 0) {
					if (x == DUK_ASC_SLASH) {
						DUK__ADVANCECHARS(lex_ctx, 1); /* eat closing slash */
						break;
					} else if (x == DUK_ASC_BACKSLASH) {
						state = 1;
					} else if (x == DUK_ASC_LBRACKET) {
						state = 2;
					}
				} else if (state == 1) {
					state = 0;
				} else if (state == 2) {
					if (x == DUK_ASC_RBRACKET) {
						state = 0;
					} else if (x == DUK_ASC_BACKSLASH) {
						state = 3;
					}
				} else { /* state == 3 */
					state = 2;
				}
				DUK__APPENDBUFFER(lex_ctx, x);
			}
			out_token->str1 = duk__internbuffer(lex_ctx, lex_ctx->slot1_idx);

			/* second, parse flags */

			DUK__INITBUFFER(lex_ctx);
			for (;;) {
				x = DUK__L0();
				if (!duk_unicode_is_identifier_part(x)) {
					break;
				}
				x = DUK__L0(); /* re-read to avoid spill / fetch */
				DUK__APPENDBUFFER(lex_ctx, x);
				DUK__ADVANCECHARS(lex_ctx, 1);
			}
			out_token->str2 = duk__internbuffer(lex_ctx, lex_ctx->slot2_idx);

			DUK__INITBUFFER(lex_ctx); /* free some memory */

			/* validation of the regexp is caller's responsibility */

			advtok = DUK__ADVTOK(0, DUK_TOK_REGEXP);
#else /* DUK_USE_REGEXP_SUPPORT */
			goto fail_regexp_support;
#endif /* DUK_USE_REGEXP_SUPPORT */
		} else if (DUK__L1() == DUK_ASC_EQUALS) {
			/* "/=" and not in regexp mode */
			advtok = DUK__ADVTOK(2, DUK_TOK_DIV_EQ);
		} else {
			/* "/" and not in regexp mode */
			advtok = DUK__ADVTOK(1, DUK_TOK_DIV);
		}
		break;
	case DUK_ASC_LCURLY: /* '{' */
		advtok = DUK__ADVTOK(1, DUK_TOK_LCURLY);
		break;
	case DUK_ASC_RCURLY: /* '}' */
		advtok = DUK__ADVTOK(1, DUK_TOK_RCURLY);
		break;
	case DUK_ASC_LPAREN: /* '(' */
		advtok = DUK__ADVTOK(1, DUK_TOK_LPAREN);
		break;
	case DUK_ASC_RPAREN: /* ')' */
		advtok = DUK__ADVTOK(1, DUK_TOK_RPAREN);
		break;
	case DUK_ASC_LBRACKET: /* '[' */
		advtok = DUK__ADVTOK(1, DUK_TOK_LBRACKET);
		break;
	case DUK_ASC_RBRACKET: /* ']' */
		advtok = DUK__ADVTOK(1, DUK_TOK_RBRACKET);
		break;
	case DUK_ASC_PERIOD: /* '.' */
		if (DUK__ISDIGIT(DUK__L1())) {
			/* Period followed by a digit can only start DecimalLiteral
			 * (handled in slow path).  We could jump straight into the
			 * DecimalLiteral handling but should avoid goto to inside
			 * a block.
			 */
			goto slow_path;
		}
		advtok = DUK__ADVTOK(1, DUK_TOK_PERIOD);
		break;
	case DUK_ASC_SEMICOLON: /* ';' */
		advtok = DUK__ADVTOK(1, DUK_TOK_SEMICOLON);
		break;
	case DUK_ASC_COMMA: /* ',' */
		advtok = DUK__ADVTOK(1, DUK_TOK_COMMA);
		break;
	case DUK_ASC_LANGLE: /* '<' */
#if defined(DUK_USE_HTML_COMMENTS)
		if (DUK__L1() == DUK_ASC_EXCLAMATION && DUK__L2() == DUK_ASC_MINUS && DUK__L3() == DUK_ASC_MINUS) {
			/*
			 *  ES2015: B.1.3, handle "<!--" SingleLineHTMLOpenComment
			 */

			/* DUK__ADVANCECHARS(lex_ctx, 4) would be correct here, but not necessary */
			duk__lexer_skip_to_endofline(lex_ctx);
			goto restart; /* line terminator will be handled on next round */
		} else
#endif /* DUK_USE_HTML_COMMENTS */
		    if (DUK__L1() == DUK_ASC_LANGLE && DUK__L2() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(3, DUK_TOK_ALSHIFT_EQ);
		} else if (DUK__L1() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(2, DUK_TOK_LE);
		} else if (DUK__L1() == DUK_ASC_LANGLE) {
			advtok = DUK__ADVTOK(2, DUK_TOK_ALSHIFT);
		} else {
			advtok = DUK__ADVTOK(1, DUK_TOK_LT);
		}
		break;
	case DUK_ASC_RANGLE: /* '>' */
		if (DUK__L1() == DUK_ASC_RANGLE && DUK__L2() == DUK_ASC_RANGLE && DUK__L3() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(4, DUK_TOK_RSHIFT_EQ);
		} else if (DUK__L1() == DUK_ASC_RANGLE && DUK__L2() == DUK_ASC_RANGLE) {
			advtok = DUK__ADVTOK(3, DUK_TOK_RSHIFT);
		} else if (DUK__L1() == DUK_ASC_RANGLE && DUK__L2() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(3, DUK_TOK_ARSHIFT_EQ);
		} else if (DUK__L1() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(2, DUK_TOK_GE);
		} else if (DUK__L1() == DUK_ASC_RANGLE) {
			advtok = DUK__ADVTOK(2, DUK_TOK_ARSHIFT);
		} else {
			advtok = DUK__ADVTOK(1, DUK_TOK_GT);
		}
		break;
	case DUK_ASC_EQUALS: /* '=' */
		if (DUK__L1() == DUK_ASC_EQUALS && DUK__L2() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(3, DUK_TOK_SEQ);
		} else if (DUK__L1() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(2, DUK_TOK_EQ);
		} else {
			advtok = DUK__ADVTOK(1, DUK_TOK_EQUALSIGN);
		}
		break;
	case DUK_ASC_EXCLAMATION: /* '!' */
		if (DUK__L1() == DUK_ASC_EQUALS && DUK__L2() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(3, DUK_TOK_SNEQ);
		} else if (DUK__L1() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(2, DUK_TOK_NEQ);
		} else {
			advtok = DUK__ADVTOK(1, DUK_TOK_LNOT);
		}
		break;
	case DUK_ASC_PLUS: /* '+' */
		if (DUK__L1() == DUK_ASC_PLUS) {
			advtok = DUK__ADVTOK(2, DUK_TOK_INCREMENT);
		} else if (DUK__L1() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(2, DUK_TOK_ADD_EQ);
		} else {
			advtok = DUK__ADVTOK(1, DUK_TOK_ADD);
		}
		break;
	case DUK_ASC_MINUS: /* '-' */
#if defined(DUK_USE_HTML_COMMENTS)
		if (got_lineterm && DUK__L1() == DUK_ASC_MINUS && DUK__L2() == DUK_ASC_RANGLE) {
			/*
			 *  ES2015: B.1.3, handle "-->" SingleLineHTMLCloseComment
			 *  Only allowed:
			 *  - on new line
			 *  - preceded only by whitespace
			 *  - preceded by end of multiline comment and optional whitespace
			 *
			 * Since whitespace generates no tokens, and multiline comments
			 * are treated as a line ending, consulting `got_lineterm` is
			 * sufficient to test for these three options.
			 */

			/* DUK__ADVANCECHARS(lex_ctx, 3) would be correct here, but not necessary */
			duk__lexer_skip_to_endofline(lex_ctx);
			goto restart; /* line terminator will be handled on next round */
		} else
#endif /* DUK_USE_HTML_COMMENTS */
		    if (DUK__L1() == DUK_ASC_MINUS) {
			advtok = DUK__ADVTOK(2, DUK_TOK_DECREMENT);
		} else if (DUK__L1() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(2, DUK_TOK_SUB_EQ);
		} else {
			advtok = DUK__ADVTOK(1, DUK_TOK_SUB);
		}
		break;
	case DUK_ASC_STAR: /* '*' */
#if defined(DUK_USE_ES7_EXP_OPERATOR)
		if (DUK__L1() == DUK_ASC_STAR && DUK__L2() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(3, DUK_TOK_EXP_EQ);
		} else if (DUK__L1() == DUK_ASC_STAR) {
			advtok = DUK__ADVTOK(2, DUK_TOK_EXP);
		} else
#endif
		    if (DUK__L1() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(2, DUK_TOK_MUL_EQ);
		} else {
			advtok = DUK__ADVTOK(1, DUK_TOK_MUL);
		}
		break;
	case DUK_ASC_PERCENT: /* '%' */
		if (DUK__L1() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(2, DUK_TOK_MOD_EQ);
		} else {
			advtok = DUK__ADVTOK(1, DUK_TOK_MOD);
		}
		break;
	case DUK_ASC_AMP: /* '&' */
		if (DUK__L1() == DUK_ASC_AMP) {
			advtok = DUK__ADVTOK(2, DUK_TOK_LAND);
		} else if (DUK__L1() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(2, DUK_TOK_BAND_EQ);
		} else {
			advtok = DUK__ADVTOK(1, DUK_TOK_BAND);
		}
		break;
	case DUK_ASC_PIPE: /* '|' */
		if (DUK__L1() == DUK_ASC_PIPE) {
			advtok = DUK__ADVTOK(2, DUK_TOK_LOR);
		} else if (DUK__L1() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(2, DUK_TOK_BOR_EQ);
		} else {
			advtok = DUK__ADVTOK(1, DUK_TOK_BOR);
		}
		break;
	case DUK_ASC_CARET: /* '^' */
		if (DUK__L1() == DUK_ASC_EQUALS) {
			advtok = DUK__ADVTOK(2, DUK_TOK_BXOR_EQ);
		} else {
			advtok = DUK__ADVTOK(1, DUK_TOK_BXOR);
		}
		break;
	case DUK_ASC_TILDE: /* '~' */
		advtok = DUK__ADVTOK(1, DUK_TOK_BNOT);
		break;
	case DUK_ASC_QUESTION: /* '?' */
		advtok = DUK__ADVTOK(1, DUK_TOK_QUESTION);
		break;
	case DUK_ASC_COLON: /* ':' */
		advtok = DUK__ADVTOK(1, DUK_TOK_COLON);
		break;
	case DUK_ASC_DOUBLEQUOTE: /* '"' */
	case DUK_ASC_SINGLEQUOTE: { /* '\'' */
		DUK__INITBUFFER(lex_ctx);
		duk__lexer_parse_string_literal(lex_ctx, out_token, x /*quote*/, strict_mode);
		duk__internbuffer(lex_ctx, lex_ctx->slot1_idx);
		out_token->str1 = duk_known_hstring(lex_ctx->thr, lex_ctx->slot1_idx);

		DUK__INITBUFFER(lex_ctx); /* free some memory */

		advtok = DUK__ADVTOK(0, DUK_TOK_STRING);
		break;
	}
	default:
		goto slow_path;
	} /* switch */

	goto skip_slow_path;

slow_path:
	if (duk_unicode_is_line_terminator(x)) {
		if (x == 0x000d && DUK__L1() == 0x000a) {
			/*
			 *  E5 Section 7.3: CR LF is detected as a single line terminator for
			 *  line numbers.  Here we also detect it as a single line terminator
			 *  token.
			 */
			DUK__ADVANCECHARS(lex_ctx, 2);
		} else {
			DUK__ADVANCECHARS(lex_ctx, 1);
		}
		got_lineterm = 1;
		goto restart_lineupdate;
	} else if (duk_unicode_is_identifier_start(x) || x == DUK_ASC_BACKSLASH) {
		/*
		 *  Parse an identifier and then check whether it is:
		 *    - reserved word (keyword or other reserved word)
		 *    - "null"  (NullLiteral)
		 *    - "true"  (BooleanLiteral)
		 *    - "false" (BooleanLiteral)
		 *    - anything else => identifier
		 *
		 *  This does not follow the E5 productions cleanly, but is
		 *  useful and compact.
		 *
		 *  Note that identifiers may contain Unicode escapes,
		 *  see E5 Sections 6 and 7.6.  They must be decoded first,
		 *  and the result checked against allowed characters.
		 *  The above if-clause accepts an identifier start and an
		 *  '\' character -- no other token can begin with a '\'.
		 *
		 *  Note that "get" and "set" are not reserved words in E5
		 *  specification so they are recognized as plain identifiers
		 *  (the tokens DUK_TOK_GET and DUK_TOK_SET are actually not
		 *  used now).  The compiler needs to work around this.
		 *
		 *  Strictly speaking, following ECMAScript longest match
		 *  specification, an invalid escape for the first character
		 *  should cause a syntax error.  However, an invalid escape
		 *  for IdentifierParts should just terminate the identifier
		 *  early (longest match), and let the next tokenization
		 *  fail.  For instance Rhino croaks with 'foo\z' when
		 *  parsing the identifier.  This has little practical impact.
		 */

		duk_small_uint_t i, i_end;
		duk_bool_t first = 1;
		duk_hstring *str;

		DUK__INITBUFFER(lex_ctx);
		for (;;) {
			/* re-lookup first char on first loop */
			if (DUK__L0() == DUK_ASC_BACKSLASH) {
				duk_codepoint_t esc_cp;
				if (DUK__L1() != DUK_ASC_LC_U) {
					goto fail_escape;
				}
				esc_cp = duk__lexer_parse_escape(lex_ctx, 1 /*allow_es6*/);
				DUK__APPENDBUFFER(lex_ctx, esc_cp);

				/* IdentifierStart is stricter than IdentifierPart, so if the first
				 * character is escaped, must have a stricter check here.
				 */
				if (!(first ? duk_unicode_is_identifier_start(esc_cp) : duk_unicode_is_identifier_part(esc_cp))) {
					goto fail_escape;
				}

				/* Track number of escapes: necessary for proper keyword
				 * detection.
				 */
				out_token->num_escapes++;
			} else {
				/* Note: first character is checked against this.  But because
				 * IdentifierPart includes all IdentifierStart characters, and
				 * the first character (if unescaped) has already been checked
				 * in the if condition, this is OK.
				 */
				if (!duk_unicode_is_identifier_part(DUK__L0())) {
					break;
				}
				DUK__APPENDBUFFER(lex_ctx, DUK__L0());
				DUK__ADVANCECHARS(lex_ctx, 1);
			}
			first = 0;
		}

		out_token->str1 = duk__internbuffer(lex_ctx, lex_ctx->slot1_idx);
		str = out_token->str1;
		out_token->t_nores = DUK_TOK_IDENTIFIER;

		DUK__INITBUFFER(lex_ctx); /* free some memory */

		/*
		 *  Interned identifier is compared against reserved words, which are
		 *  currently interned into the heap context.  See genbuiltins.py.
		 *
		 *  Note that an escape in the identifier disables recognition of
		 *  keywords; e.g. "\u0069f = 1;" is a valid statement (assigns to
		 *  identifier named "if").  This is not necessarily compliant,
		 *  see test-dec-escaped-char-in-keyword.js.
		 *
		 *  Note: "get" and "set" are awkward.  They are not officially
		 *  ReservedWords (and indeed e.g. "var set = 1;" is valid), and
		 *  must come out as DUK_TOK_IDENTIFIER.  The compiler needs to
		 *  work around this a bit.
		 */

		/* XXX: optimize by adding the token numbers directly into the
		 * always interned duk_hstring objects (there should be enough
		 * flag bits free for that)?
		 */

		i_end = (strict_mode ? DUK_STRIDX_END_RESERVED : DUK_STRIDX_START_STRICT_RESERVED);

		advtok = DUK__ADVTOK(0, DUK_TOK_IDENTIFIER);
		if (out_token->num_escapes == 0) {
			for (i = DUK_STRIDX_START_RESERVED; i < i_end; i++) {
				DUK_ASSERT_DISABLE(i >= 0); /* unsigned */
				DUK_ASSERT(i < DUK_HEAP_NUM_STRINGS);
				if (DUK_HTHREAD_GET_STRING(lex_ctx->thr, i) == str) {
					advtok = DUK__ADVTOK(0, DUK_STRIDX_TO_TOK(i));
					break;
				}
			}
		}
	} else if (DUK__ISDIGIT(x) || (x == DUK_ASC_PERIOD)) {
		/* Note: decimal number may start with a period, but must be followed by a digit */

		/*
		 *  Pre-parsing for decimal, hex, octal (both legacy and ES2015),
		 *  and binary literals, followed by an actual parser step
		 *  provided by numconv.
		 *
		 *  Note: the leading sign character ('+' or '-') is -not- part of
		 *  the production in E5 grammar, and that the a DecimalLiteral
		 *  starting with a '0' must be followed by a non-digit.
		 *
		 *  XXX: the two step parsing process is quite awkward, it would
		 *  be more straightforward to allow numconv to parse the longest
		 *  valid prefix (it already does that, it only needs to indicate
		 *  where the input ended).  However, the lexer decodes characters
		 *  using a limited lookup window, so this is not a trivial change.
		 */

		/* XXX: because of the final check below (that the literal is not
		 * followed by a digit), this could maybe be simplified, if we bail
		 * out early from a leading zero (and if there are no periods etc).
		 * Maybe too complex.
		 */

		duk_double_t val;
		duk_bool_t legacy_oct = 0;
		duk_small_int_t state; /* 0=before period/exp,
		                        * 1=after period, before exp
		                        * 2=after exp, allow '+' or '-'
		                        * 3=after exp and exp sign
		                        */
		duk_small_uint_t s2n_flags;
		duk_codepoint_t y, z;
		duk_small_int_t s2n_radix = 10;
		duk_small_uint_t pre_adv = 0;

		DUK__INITBUFFER(lex_ctx);
		y = DUK__L1();

		if (x == DUK_ASC_0) {
			z = DUK_LOWERCASE_CHAR_ASCII(y);

			pre_adv = 2; /* default for 0xNNN, 0oNNN, 0bNNN. */
			if (z == DUK_ASC_LC_X) {
				s2n_radix = 16;
			} else if (z == DUK_ASC_LC_O) {
				s2n_radix = 8;
			} else if (z == DUK_ASC_LC_B) {
				s2n_radix = 2;
			} else {
				pre_adv = 0;
				if (DUK__ISDIGIT(y)) {
					if (strict_mode) {
						/* Reject octal like \07 but also octal-lookalike
						 * decimal like \08 in strict mode.
						 */
						goto fail_number_literal;
					} else {
						/* Legacy OctalIntegerLiteral or octal-lookalice
						 * decimal.  Deciding between the two happens below
						 * in digit scanning.
						 */
						DUK__APPENDBUFFER(lex_ctx, x);
						pre_adv = 1;
						legacy_oct = 1;
						s2n_radix = 8; /* tentative unless conflicting digits found */
					}
				}
			}
		}

		DUK__ADVANCECHARS(lex_ctx, pre_adv);

		/* XXX: we could parse integers here directly, and fall back
		 * to numconv only when encountering a fractional expression
		 * or when an octal literal turned out to be decimal (0778 etc).
		 */
		state = 0;
		for (;;) {
			x = DUK__L0(); /* re-lookup curr char on first round */
			if (DUK__ISDIGIT(x)) {
				/* Note: intentionally allow leading zeroes here, as the
				 * actual parser will check for them.
				 */
				if (state == 0 && legacy_oct && (x == DUK_ASC_8 || x == DUK_ASC_9)) {
					/* Started out as an octal-lookalike
					 * but interpreted as decimal, e.g.
					 * '0779' -> 779.  This also means
					 * that fractions are allowed, e.g.
					 * '0779.123' is allowed but '0777.123'
					 * is not!
					 */
					s2n_radix = 10;
				}
				if (state == 2) {
					state = 3;
				}
			} else if (s2n_radix == 16 && DUK__ISHEXDIGIT(x)) {
				/* Note: 'e' and 'E' are also accepted here. */
				;
			} else if (x == DUK_ASC_PERIOD) {
				if (state >= 1 || s2n_radix != 10) {
					break;
				} else {
					state = 1;
				}
			} else if (x == DUK_ASC_LC_E || x == DUK_ASC_UC_E) {
				if (state >= 2 || s2n_radix != 10) {
					break;
				} else {
					state = 2;
				}
			} else if (x == DUK_ASC_MINUS || x == DUK_ASC_PLUS) {
				if (state != 2) {
					break;
				} else {
					state = 3;
				}
			} else {
				break;
			}
			DUK__APPENDBUFFER(lex_ctx, x);
			DUK__ADVANCECHARS(lex_ctx, 1);
		}

		/* XXX: better coercion */
		(void) duk__internbuffer(lex_ctx, lex_ctx->slot1_idx);

		if (s2n_radix != 10) {
			/* For bases other than 10, integer only. */
			s2n_flags = DUK_S2N_FLAG_ALLOW_LEADING_ZERO;
		} else {
			s2n_flags = DUK_S2N_FLAG_ALLOW_EXP | DUK_S2N_FLAG_ALLOW_FRAC | DUK_S2N_FLAG_ALLOW_NAKED_FRAC |
			            DUK_S2N_FLAG_ALLOW_EMPTY_FRAC | DUK_S2N_FLAG_ALLOW_LEADING_ZERO;
		}

		duk_dup(lex_ctx->thr, lex_ctx->slot1_idx);
		duk_numconv_parse(lex_ctx->thr, s2n_radix, s2n_flags);
		val = duk_to_number_m1(lex_ctx->thr);
		if (DUK_ISNAN(val)) {
			goto fail_number_literal;
		}
		duk_replace(lex_ctx->thr, lex_ctx->slot1_idx); /* could also just pop? */

		DUK__INITBUFFER(lex_ctx); /* free some memory */

		/* Section 7.8.3 (note): NumericLiteral must be followed by something other than
		 * IdentifierStart or DecimalDigit.
		 */

		if (DUK__ISDIGIT(DUK__L0()) || duk_unicode_is_identifier_start(DUK__L0())) {
			goto fail_number_literal;
		}

		out_token->num = val;
		advtok = DUK__ADVTOK(0, DUK_TOK_NUMBER);
	} else if (duk_unicode_is_whitespace(DUK__LOOKUP(lex_ctx, 0))) {
		DUK__ADVANCECHARS(lex_ctx, 1);
		goto restart;
	} else if (x < 0) {
		advtok = DUK__ADVTOK(0, DUK_TOK_EOF);
	} else {
		goto fail_token;
	}
skip_slow_path:

	/*
	 *  Shared exit path
	 */

	DUK__ADVANCEBYTES(lex_ctx, advtok >> 8);
	out_token->t = advtok & 0xff;
	if (out_token->t_nores == DUK_TOK_INVALID) {
		out_token->t_nores = out_token->t;
	}
	out_token->lineterm = got_lineterm;

	/* Automatic semicolon insertion is allowed if a token is preceded
	 * by line terminator(s), or terminates a statement list (right curly
	 * or EOF).
	 */
	if (got_lineterm || out_token->t == DUK_TOK_RCURLY || out_token->t == DUK_TOK_EOF) {
		out_token->allow_auto_semi = 1;
	} else {
		out_token->allow_auto_semi = 0;
	}

	return;

fail_token_limit:
	DUK_ERROR_RANGE(lex_ctx->thr, DUK_STR_TOKEN_LIMIT);
	DUK_WO_NORETURN(return;);

fail_token:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_INVALID_TOKEN);
	DUK_WO_NORETURN(return;);

fail_number_literal:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_INVALID_NUMBER_LITERAL);
	DUK_WO_NORETURN(return;);

fail_escape:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_INVALID_ESCAPE);
	DUK_WO_NORETURN(return;);

fail_unterm_regexp:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_UNTERMINATED_REGEXP);
	DUK_WO_NORETURN(return;);

fail_unterm_comment:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_UNTERMINATED_COMMENT);
	DUK_WO_NORETURN(return;);

#if !defined(DUK_USE_REGEXP_SUPPORT)
fail_regexp_support:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_REGEXP_SUPPORT_DISABLED);
	DUK_WO_NORETURN(return;);
#endif
}

#if defined(DUK_USE_REGEXP_SUPPORT)

/*
 *  Parse a RegExp token.  The grammar is described in E5 Section 15.10.
 *  Terminal constructions (such as quantifiers) are parsed directly here.
 *
 *  0xffffffffU is used as a marker for "infinity" in quantifiers.  Further,
 *  DUK__MAX_RE_QUANT_DIGITS limits the maximum number of digits that
 *  will be accepted for a quantifier.
 */

DUK_INTERNAL void duk_lexer_parse_re_token(duk_lexer_ctx *lex_ctx, duk_re_token *out_token) {
	duk_small_uint_t advtok = 0; /* init is unnecessary but suppresses "may be used uninitialized" warnings */
	duk_codepoint_t x, y;

	if (++lex_ctx->token_count >= lex_ctx->token_limit) {
		goto fail_token_limit;
	}

	duk_memzero(out_token, sizeof(*out_token));

	x = DUK__L0();
	y = DUK__L1();

	DUK_DDD(DUK_DDDPRINT("parsing regexp token, L0=%ld, L1=%ld", (long) x, (long) y));

	switch (x) {
	case DUK_ASC_PIPE: {
		advtok = DUK__ADVTOK(1, DUK_RETOK_DISJUNCTION);
		break;
	}
	case DUK_ASC_CARET: {
		advtok = DUK__ADVTOK(1, DUK_RETOK_ASSERT_START);
		break;
	}
	case DUK_ASC_DOLLAR: {
		advtok = DUK__ADVTOK(1, DUK_RETOK_ASSERT_END);
		break;
	}
	case DUK_ASC_QUESTION: {
		out_token->qmin = 0;
		out_token->qmax = 1;
		if (y == DUK_ASC_QUESTION) {
			advtok = DUK__ADVTOK(2, DUK_RETOK_QUANTIFIER);
			out_token->greedy = 0;
		} else {
			advtok = DUK__ADVTOK(1, DUK_RETOK_QUANTIFIER);
			out_token->greedy = 1;
		}
		break;
	}
	case DUK_ASC_STAR: {
		out_token->qmin = 0;
		out_token->qmax = DUK_RE_QUANTIFIER_INFINITE;
		if (y == DUK_ASC_QUESTION) {
			advtok = DUK__ADVTOK(2, DUK_RETOK_QUANTIFIER);
			out_token->greedy = 0;
		} else {
			advtok = DUK__ADVTOK(1, DUK_RETOK_QUANTIFIER);
			out_token->greedy = 1;
		}
		break;
	}
	case DUK_ASC_PLUS: {
		out_token->qmin = 1;
		out_token->qmax = DUK_RE_QUANTIFIER_INFINITE;
		if (y == DUK_ASC_QUESTION) {
			advtok = DUK__ADVTOK(2, DUK_RETOK_QUANTIFIER);
			out_token->greedy = 0;
		} else {
			advtok = DUK__ADVTOK(1, DUK_RETOK_QUANTIFIER);
			out_token->greedy = 1;
		}
		break;
	}
	case DUK_ASC_LCURLY: {
		/* Production allows 'DecimalDigits', including leading zeroes */
		duk_uint32_t val1 = 0;
		duk_uint32_t val2 = DUK_RE_QUANTIFIER_INFINITE;
		duk_small_int_t digits = 0;
#if defined(DUK_USE_ES6_REGEXP_SYNTAX)
		duk_lexer_point lex_pt;
#endif

#if defined(DUK_USE_ES6_REGEXP_SYNTAX)
		/* Store lexer position, restoring if quantifier is invalid. */
		DUK_LEXER_GETPOINT(lex_ctx, &lex_pt);
#endif

		for (;;) {
			DUK__ADVANCECHARS(lex_ctx, 1); /* eat '{' on entry */
			x = DUK__L0();
			if (DUK__ISDIGIT(x)) {
				digits++;
				val1 = val1 * 10 + (duk_uint32_t) duk__hexval(x);
			} else if (x == DUK_ASC_COMMA) {
				if (digits > DUK__MAX_RE_QUANT_DIGITS) {
					goto invalid_quantifier;
				}
				if (val2 != DUK_RE_QUANTIFIER_INFINITE) {
					goto invalid_quantifier;
				}
				if (DUK__L1() == DUK_ASC_RCURLY) {
					/* form: { DecimalDigits , }, val1 = min count */
					if (digits == 0) {
						goto invalid_quantifier;
					}
					out_token->qmin = val1;
					out_token->qmax = DUK_RE_QUANTIFIER_INFINITE;
					DUK__ADVANCECHARS(lex_ctx, 2);
					break;
				}
				val2 = val1;
				val1 = 0;
				digits = 0; /* not strictly necessary because of lookahead '}' above */
			} else if (x == DUK_ASC_RCURLY) {
				if (digits > DUK__MAX_RE_QUANT_DIGITS) {
					goto invalid_quantifier;
				}
				if (digits == 0) {
					goto invalid_quantifier;
				}
				if (val2 != DUK_RE_QUANTIFIER_INFINITE) {
					/* val2 = min count, val1 = max count */
					out_token->qmin = val2;
					out_token->qmax = val1;
				} else {
					/* val1 = count */
					out_token->qmin = val1;
					out_token->qmax = val1;
				}
				DUK__ADVANCECHARS(lex_ctx, 1);
				break;
			} else {
				goto invalid_quantifier;
			}
		}
		if (DUK__L0() == DUK_ASC_QUESTION) {
			out_token->greedy = 0;
			DUK__ADVANCECHARS(lex_ctx, 1);
		} else {
			out_token->greedy = 1;
		}
		advtok = DUK__ADVTOK(0, DUK_RETOK_QUANTIFIER);
		break;
	invalid_quantifier:
#if defined(DUK_USE_ES6_REGEXP_SYNTAX)
		/* Failed to match the quantifier, restore lexer and parse
		 * opening brace as a literal.
		 */
		DUK_LEXER_SETPOINT(lex_ctx, &lex_pt);
		advtok = DUK__ADVTOK(1, DUK_RETOK_ATOM_CHAR);
		out_token->num = DUK_ASC_LCURLY;
#else
		goto fail_quantifier;
#endif
		break;
	}
	case DUK_ASC_PERIOD: {
		advtok = DUK__ADVTOK(1, DUK_RETOK_ATOM_PERIOD);
		break;
	}
	case DUK_ASC_BACKSLASH: {
		/* The E5.1 specification does not seem to allow IdentifierPart characters
		 * to be used as identity escapes.  Unfortunately this includes '$', which
		 * cannot be escaped as '\$'; it needs to be escaped e.g. as '\u0024'.
		 * Many other implementations (including V8 and Rhino, for instance) do
		 * accept '\$' as a valid identity escape, which is quite pragmatic, and
		 * ES2015 Annex B relaxes the rules to allow these (and other) real world forms.
		 */

		advtok = DUK__ADVTOK(2, DUK_RETOK_ATOM_CHAR); /* default: char escape (two chars) */
		if (y == DUK_ASC_LC_B) {
			advtok = DUK__ADVTOK(2, DUK_RETOK_ASSERT_WORD_BOUNDARY);
		} else if (y == DUK_ASC_UC_B) {
			advtok = DUK__ADVTOK(2, DUK_RETOK_ASSERT_NOT_WORD_BOUNDARY);
		} else if (y == DUK_ASC_LC_F) {
			out_token->num = 0x000c;
		} else if (y == DUK_ASC_LC_N) {
			out_token->num = 0x000a;
		} else if (y == DUK_ASC_LC_T) {
			out_token->num = 0x0009;
		} else if (y == DUK_ASC_LC_R) {
			out_token->num = 0x000d;
		} else if (y == DUK_ASC_LC_V) {
			out_token->num = 0x000b;
		} else if (y == DUK_ASC_LC_C) {
			x = DUK__L2();
			if ((x >= DUK_ASC_LC_A && x <= DUK_ASC_LC_Z) || (x >= DUK_ASC_UC_A && x <= DUK_ASC_UC_Z)) {
				out_token->num = (duk_uint32_t) (x % 32);
				advtok = DUK__ADVTOK(3, DUK_RETOK_ATOM_CHAR);
			} else {
				goto fail_escape;
			}
		} else if (y == DUK_ASC_LC_X || y == DUK_ASC_LC_U) {
			/* The token value is the Unicode codepoint without
			 * it being decode into surrogate pair characters
			 * here.  The \u{H+} is only allowed in Unicode mode
			 * which we don't support yet.
			 */
			out_token->num = (duk_uint32_t) duk__lexer_parse_escape(lex_ctx, 0 /*allow_es6*/);
			advtok = DUK__ADVTOK(0, DUK_RETOK_ATOM_CHAR);
		} else if (y == DUK_ASC_LC_D) {
			advtok = DUK__ADVTOK(2, DUK_RETOK_ATOM_DIGIT);
		} else if (y == DUK_ASC_UC_D) {
			advtok = DUK__ADVTOK(2, DUK_RETOK_ATOM_NOT_DIGIT);
		} else if (y == DUK_ASC_LC_S) {
			advtok = DUK__ADVTOK(2, DUK_RETOK_ATOM_WHITE);
		} else if (y == DUK_ASC_UC_S) {
			advtok = DUK__ADVTOK(2, DUK_RETOK_ATOM_NOT_WHITE);
		} else if (y == DUK_ASC_LC_W) {
			advtok = DUK__ADVTOK(2, DUK_RETOK_ATOM_WORD_CHAR);
		} else if (y == DUK_ASC_UC_W) {
			advtok = DUK__ADVTOK(2, DUK_RETOK_ATOM_NOT_WORD_CHAR);
		} else if (DUK__ISDIGIT(y)) {
			/* E5 Section 15.10.2.11 */
			if (y == DUK_ASC_0) {
				if (DUK__ISDIGIT(DUK__L2())) {
					goto fail_escape;
				}
				out_token->num = 0x0000;
				advtok = DUK__ADVTOK(2, DUK_RETOK_ATOM_CHAR);
			} else {
				/* XXX: shared parsing? */
				duk_uint32_t val = 0;
				duk_small_int_t i;
				for (i = 0;; i++) {
					if (i >= DUK__MAX_RE_DECESC_DIGITS) {
						goto fail_escape;
					}
					DUK__ADVANCECHARS(lex_ctx, 1); /* eat backslash on entry */
					x = DUK__L0();
					if (!DUK__ISDIGIT(x)) {
						break;
					}
					val = val * 10 + (duk_uint32_t) duk__hexval(x);
				}
				/* DUK__L0() cannot be a digit, because the loop doesn't terminate if it is */
				advtok = DUK__ADVTOK(0, DUK_RETOK_ATOM_BACKREFERENCE);
				out_token->num = val;
			}
#if defined(DUK_USE_ES6_REGEXP_SYNTAX)
		} else if (y >= 0) {
			/* For ES2015 Annex B, accept any source character as identity
			 * escape except 'c' which is used for control characters.
			 * http://www.ecma-international.org/ecma-262/6.0/#sec-regular-expressions-patterns
			 * Careful not to match end-of-buffer (<0) here.
			 * This is not yet full ES2015 Annex B because cases above
			 * (like hex escape) won't backtrack.
			 */
			DUK_ASSERT(y != DUK_ASC_LC_C); /* covered above */
#else /* DUK_USE_ES6_REGEXP_SYNTAX */
		} else if ((y >= 0 && !duk_unicode_is_identifier_part(y)) || y == DUK_UNICODE_CP_ZWNJ || y == DUK_UNICODE_CP_ZWJ) {
			/* For ES5.1 identity escapes are not allowed for identifier
			 * parts.  This conflicts with a lot of real world code as this
			 * doesn't e.g. allow escaping a dollar sign as /\$/, see
			 * test-regexp-identity-escape-dollar.js.
			 */
#endif /* DUK_USE_ES6_REGEXP_SYNTAX */
			out_token->num = (duk_uint32_t) y;
		} else {
			goto fail_escape;
		}
		break;
	}
	case DUK_ASC_LPAREN: {
		/* XXX: naming is inconsistent: ATOM_END_GROUP ends an ASSERT_START_LOOKAHEAD */

		if (y == DUK_ASC_QUESTION) {
			if (DUK__L2() == DUK_ASC_EQUALS) {
				/* (?= */
				advtok = DUK__ADVTOK(3, DUK_RETOK_ASSERT_START_POS_LOOKAHEAD);
			} else if (DUK__L2() == DUK_ASC_EXCLAMATION) {
				/* (?! */
				advtok = DUK__ADVTOK(3, DUK_RETOK_ASSERT_START_NEG_LOOKAHEAD);
			} else if (DUK__L2() == DUK_ASC_COLON) {
				/* (?: */
				advtok = DUK__ADVTOK(3, DUK_RETOK_ATOM_START_NONCAPTURE_GROUP);
			} else {
				goto fail_group;
			}
		} else {
			/* ( */
			advtok = DUK__ADVTOK(1, DUK_RETOK_ATOM_START_CAPTURE_GROUP);
		}
		break;
	}
	case DUK_ASC_RPAREN: {
		advtok = DUK__ADVTOK(1, DUK_RETOK_ATOM_END_GROUP);
		break;
	}
	case DUK_ASC_LBRACKET: {
		/*
		 *  To avoid creating a heavy intermediate value for the list of ranges,
		 *  only the start token ('[' or '[^') is parsed here.  The regexp
		 *  compiler parses the ranges itself.
		 */

		/* XXX: with DUK_USE_ES6_REGEXP_SYNTAX we should allow left bracket
		 * literal too, but it's not easy to parse without backtracking.
		 */

		advtok = DUK__ADVTOK(1, DUK_RETOK_ATOM_START_CHARCLASS);
		if (y == DUK_ASC_CARET) {
			advtok = DUK__ADVTOK(2, DUK_RETOK_ATOM_START_CHARCLASS_INVERTED);
		}
		break;
	}
#if !defined(DUK_USE_ES6_REGEXP_SYNTAX)
	case DUK_ASC_RCURLY:
	case DUK_ASC_RBRACKET: {
		/* Although these could be parsed as PatternCharacters unambiguously (here),
		 * E5 Section 15.10.1 grammar explicitly forbids these as PatternCharacters.
		 */
		goto fail_invalid_char;
		break;
	}
#endif
	case -1: {
		/* EOF */
		advtok = DUK__ADVTOK(0, DUK_TOK_EOF);
		break;
	}
	default: {
		/* PatternCharacter, all excluded characters are matched by cases above */
		advtok = DUK__ADVTOK(1, DUK_RETOK_ATOM_CHAR);
		out_token->num = (duk_uint32_t) x;
		break;
	}
	}

	/*
	 *  Shared exit path
	 */

	DUK__ADVANCEBYTES(lex_ctx, advtok >> 8);
	out_token->t = advtok & 0xff;
	return;

fail_token_limit:
	DUK_ERROR_RANGE(lex_ctx->thr, DUK_STR_TOKEN_LIMIT);
	DUK_WO_NORETURN(return;);

fail_escape:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_INVALID_REGEXP_ESCAPE);
	DUK_WO_NORETURN(return;);

fail_group:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_INVALID_REGEXP_GROUP);
	DUK_WO_NORETURN(return;);

#if !defined(DUK_USE_ES6_REGEXP_SYNTAX)
fail_invalid_char:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_INVALID_REGEXP_CHARACTER);
	DUK_WO_NORETURN(return;);

fail_quantifier:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_INVALID_QUANTIFIER);
	DUK_WO_NORETURN(return;);
#endif
}

/*
 *  Special parser for character classes; calls callback for every
 *  range parsed and returns the number of ranges present.
 */

/* XXX: this duplicates functionality in duk_regexp.c where a similar loop is
 * required anyway.  We could use that BUT we need to update the regexp compiler
 * 'nranges' too.  Work this out a bit more cleanly to save space.
 */

/* XXX: the handling of character range detection is a bit convoluted.
 * Try to simplify and make smaller.
 */

/* XXX: logic for handling character ranges is now incorrect, it will accept
 * e.g. [\d-z] whereas it should croak from it?  SMJS accepts this too, though.
 *
 * Needs a read through and a lot of additional tests.
 */

DUK_LOCAL
void duk__emit_u16_direct_ranges(duk_lexer_ctx *lex_ctx,
                                 duk_re_range_callback gen_range,
                                 void *userdata,
                                 const duk_uint16_t *ranges,
                                 duk_small_int_t num) {
	const duk_uint16_t *ranges_end;

	DUK_UNREF(lex_ctx);

	ranges_end = ranges + num;
	while (ranges < ranges_end) {
		/* mark range 'direct', bypass canonicalization (see Wiki) */
		gen_range(userdata, (duk_codepoint_t) ranges[0], (duk_codepoint_t) ranges[1], 1);
		ranges += 2;
	}
}

DUK_INTERNAL void duk_lexer_parse_re_ranges(duk_lexer_ctx *lex_ctx, duk_re_range_callback gen_range, void *userdata) {
	duk_codepoint_t start = -1;
	duk_codepoint_t ch;
	duk_codepoint_t x;
	duk_bool_t dash = 0;
	duk_small_uint_t adv = 0;

	DUK_DD(DUK_DDPRINT("parsing regexp ranges"));

	for (;;) {
		DUK__ADVANCECHARS(lex_ctx, adv);
		adv = 1;

		x = DUK__L0();

		ch = -1; /* not strictly necessary, but avoids "uninitialized variable" warnings */
		DUK_UNREF(ch);

		if (x < 0) {
			goto fail_unterm_charclass;
		} else if (x == DUK_ASC_RBRACKET) {
			if (start >= 0) {
				gen_range(userdata, start, start, 0);
			}
			DUK__ADVANCECHARS(lex_ctx, 1); /* eat ']' before finishing */
			break;
		} else if (x == DUK_ASC_MINUS) {
			if (start >= 0 && !dash && DUK__L1() != DUK_ASC_RBRACKET) {
				/* '-' as a range indicator */
				dash = 1;
				continue;
			} else {
				/* '-' verbatim */
				ch = x;
			}
		} else if (x == DUK_ASC_BACKSLASH) {
			/*
			 *  The escapes are same as outside a character class, except that \b has a
			 *  different meaning, and \B and backreferences are prohibited (see E5
			 *  Section 15.10.2.19).  However, it's difficult to share code because we
			 *  handle e.g. "\n" very differently: here we generate a single character
			 *  range for it.
			 */

			/* XXX: ES2015 surrogate pair handling. */

			x = DUK__L1();

			adv = 2;

			if (x == DUK_ASC_LC_B) {
				/* Note: '\b' in char class is different than outside (assertion),
				 * '\B' is not allowed and is caught by the duk_unicode_is_identifier_part()
				 * check below.
				 */
				ch = 0x0008;
			} else if (x == DUK_ASC_LC_F) {
				ch = 0x000c;
			} else if (x == DUK_ASC_LC_N) {
				ch = 0x000a;
			} else if (x == DUK_ASC_LC_T) {
				ch = 0x0009;
			} else if (x == DUK_ASC_LC_R) {
				ch = 0x000d;
			} else if (x == DUK_ASC_LC_V) {
				ch = 0x000b;
			} else if (x == DUK_ASC_LC_C) {
				x = DUK__L2();
				adv = 3;
				if ((x >= DUK_ASC_LC_A && x <= DUK_ASC_LC_Z) || (x >= DUK_ASC_UC_A && x <= DUK_ASC_UC_Z)) {
					ch = (x % 32);
				} else {
					goto fail_escape;
				}
			} else if (x == DUK_ASC_LC_X || x == DUK_ASC_LC_U) {
				/* The \u{H+} form is only allowed in Unicode mode which
				 * we don't support yet.
				 */
				ch = duk__lexer_parse_escape(lex_ctx, 0 /*allow_es6*/);
				adv = 0;
			} else if (x == DUK_ASC_LC_D) {
				duk__emit_u16_direct_ranges(lex_ctx,
				                            gen_range,
				                            userdata,
				                            duk_unicode_re_ranges_digit,
				                            sizeof(duk_unicode_re_ranges_digit) / sizeof(duk_uint16_t));
				ch = -1;
			} else if (x == DUK_ASC_UC_D) {
				duk__emit_u16_direct_ranges(lex_ctx,
				                            gen_range,
				                            userdata,
				                            duk_unicode_re_ranges_not_digit,
				                            sizeof(duk_unicode_re_ranges_not_digit) / sizeof(duk_uint16_t));
				ch = -1;
			} else if (x == DUK_ASC_LC_S) {
				duk__emit_u16_direct_ranges(lex_ctx,
				                            gen_range,
				                            userdata,
				                            duk_unicode_re_ranges_white,
				                            sizeof(duk_unicode_re_ranges_white) / sizeof(duk_uint16_t));
				ch = -1;
			} else if (x == DUK_ASC_UC_S) {
				duk__emit_u16_direct_ranges(lex_ctx,
				                            gen_range,
				                            userdata,
				                            duk_unicode_re_ranges_not_white,
				                            sizeof(duk_unicode_re_ranges_not_white) / sizeof(duk_uint16_t));
				ch = -1;
			} else if (x == DUK_ASC_LC_W) {
				duk__emit_u16_direct_ranges(lex_ctx,
				                            gen_range,
				                            userdata,
				                            duk_unicode_re_ranges_wordchar,
				                            sizeof(duk_unicode_re_ranges_wordchar) / sizeof(duk_uint16_t));
				ch = -1;
			} else if (x == DUK_ASC_UC_W) {
				duk__emit_u16_direct_ranges(lex_ctx,
				                            gen_range,
				                            userdata,
				                            duk_unicode_re_ranges_not_wordchar,
				                            sizeof(duk_unicode_re_ranges_not_wordchar) / sizeof(duk_uint16_t));
				ch = -1;
			} else if (DUK__ISDIGIT(x)) {
				/* DecimalEscape, only \0 is allowed, no leading
				 * zeroes are allowed.
				 *
				 * ES2015 Annex B also allows (maximal match) legacy
				 * octal escapes up to \377 and \8 and \9 are
				 * accepted as literal '8' and '9', also in strict mode.
				 */

#if defined(DUK_USE_ES6_REGEXP_SYNTAX)
				ch = duk__lexer_parse_legacy_octal(lex_ctx, &adv, 0 /*reject_annex_b*/);
				DUK_ASSERT(ch >= 0); /* no rejections */
#else
				if (x == DUK_ASC_0 && !DUK__ISDIGIT(DUK__L2())) {
					ch = 0x0000;
				} else {
					goto fail_escape;
				}
#endif
#if defined(DUK_USE_ES6_REGEXP_SYNTAX)
			} else if (x >= 0) {
				/* IdentityEscape: ES2015 Annex B allows almost all
				 * source characters here.  Match anything except
				 * EOF here.
				 */
				ch = x;
#else /* DUK_USE_ES6_REGEXP_SYNTAX */
			} else if (!duk_unicode_is_identifier_part(x)) {
				/* IdentityEscape: ES5.1 doesn't allow identity escape
				 * for identifier part characters, which conflicts with
				 * some real world code.  For example, it doesn't allow
				 * /[\$]/ which is awkward.
				 */
				ch = x;
#endif /* DUK_USE_ES6_REGEXP_SYNTAX */
			} else {
				goto fail_escape;
			}
		} else {
			/* character represents itself */
			ch = x;
		}

		/* ch is a literal character here or -1 if parsed entity was
		 * an escape such as "\s".
		 */

		if (ch < 0) {
			/* multi-character sets not allowed as part of ranges, see
			 * E5 Section 15.10.2.15, abstract operation CharacterRange.
			 */
			if (start >= 0) {
				if (dash) {
					goto fail_range;
				} else {
					gen_range(userdata, start, start, 0);
					start = -1;
					/* dash is already 0 */
				}
			}
		} else {
			if (start >= 0) {
				if (dash) {
					if (start > ch) {
						goto fail_range;
					}
					gen_range(userdata, start, ch, 0);
					start = -1;
					dash = 0;
				} else {
					gen_range(userdata, start, start, 0);
					start = ch;
					/* dash is already 0 */
				}
			} else {
				start = ch;
			}
		}
	}

	return;

fail_escape:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_INVALID_REGEXP_ESCAPE);
	DUK_WO_NORETURN(return;);

fail_range:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_INVALID_RANGE);
	DUK_WO_NORETURN(return;);

fail_unterm_charclass:
	DUK_ERROR_SYNTAX(lex_ctx->thr, DUK_STR_UNTERMINATED_CHARCLASS);
	DUK_WO_NORETURN(return;);
}

#endif /* DUK_USE_REGEXP_SUPPORT */
