/*
 *  WHATWG Encoding API built-ins
 *
 *  API specification: https://encoding.spec.whatwg.org/#api
 *  Web IDL: https://www.w3.org/TR/WebIDL/
 */

#include "duk_internal.h"

/*
 *  Data structures for encoding/decoding
 */

typedef struct {
	duk_uint8_t *out; /* where to write next byte(s) */
	duk_codepoint_t lead; /* lead surrogate */
} duk__encode_context;

typedef struct {
	/* UTF-8 decoding state */
	duk_codepoint_t codepoint; /* built up incrementally */
	duk_uint8_t upper; /* max value of next byte (decode error otherwise) */
	duk_uint8_t lower; /* min value of next byte (ditto) */
	duk_uint8_t needed; /* how many more bytes we need */
	duk_uint8_t bom_handled; /* BOM seen or no longer expected */

	/* Decoder configuration */
	duk_uint8_t fatal;
	duk_uint8_t ignore_bom;
} duk__decode_context;

/* The signed duk_codepoint_t type is used to signal a decoded codepoint
 * (>= 0) or various other states using negative values.
 */
#define DUK__CP_CONTINUE (-1) /* continue to next byte, no completed codepoint */
#define DUK__CP_ERROR    (-2) /* decoding error */
#define DUK__CP_RETRY    (-3) /* decoding error; retry last byte */

/*
 *  Raw helpers for encoding/decoding
 */

/* Emit UTF-8 (= CESU-8) encoded U+FFFD (replacement char), i.e. ef bf bd. */
DUK_LOCAL duk_uint8_t *duk__utf8_emit_repl(duk_uint8_t *ptr) {
	*ptr++ = 0xef;
	*ptr++ = 0xbf;
	*ptr++ = 0xbd;
	return ptr;
}

DUK_LOCAL void duk__utf8_decode_init(duk__decode_context *dec_ctx) {
	/* (Re)init the decoding state of 'dec_ctx' but leave decoder
	 * configuration fields untouched.
	 */
	dec_ctx->codepoint = 0x0000L;
	dec_ctx->upper = 0xbf;
	dec_ctx->lower = 0x80;
	dec_ctx->needed = 0;
	dec_ctx->bom_handled = 0;
}

DUK_LOCAL duk_codepoint_t duk__utf8_decode_next(duk__decode_context *dec_ctx, duk_uint8_t x) {
	/*
	 *  UTF-8 algorithm based on the Encoding specification:
	 *  https://encoding.spec.whatwg.org/#utf-8-decoder
	 *
	 *  Two main states: decoding initial byte vs. decoding continuation
	 *  bytes.  Shortest length encoding is validated by restricting the
	 *  allowed range of first continuation byte using 'lower' and 'upper'.
	 */

	if (dec_ctx->needed == 0) {
		/* process initial byte */
		if (x <= 0x7f) {
			/* U+0000-U+007F, 1 byte (ASCII) */
			return (duk_codepoint_t) x;
		} else if (x >= 0xc2 && x <= 0xdf) {
			/* U+0080-U+07FF, 2 bytes */
			dec_ctx->needed = 1;
			dec_ctx->codepoint = x & 0x1f;
			DUK_ASSERT(dec_ctx->lower == 0x80);
			DUK_ASSERT(dec_ctx->upper == 0xbf);
			return DUK__CP_CONTINUE;
		} else if (x >= 0xe0 && x <= 0xef) {
			/* U+0800-U+FFFF, 3 bytes */
			if (x == 0xe0) {
				dec_ctx->lower = 0xa0;
				DUK_ASSERT(dec_ctx->upper == 0xbf);
			} else if (x == 0xed) {
				DUK_ASSERT(dec_ctx->lower == 0x80);
				dec_ctx->upper = 0x9f;
			}
			dec_ctx->needed = 2;
			dec_ctx->codepoint = x & 0x0f;
			return DUK__CP_CONTINUE;
		} else if (x >= 0xf0 && x <= 0xf4) {
			/* U+010000-U+10FFFF, 4 bytes */
			if (x == 0xf0) {
				dec_ctx->lower = 0x90;
				DUK_ASSERT(dec_ctx->upper == 0xbf);
			} else if (x == 0xf4) {
				DUK_ASSERT(dec_ctx->lower == 0x80);
				dec_ctx->upper = 0x8f;
			}
			dec_ctx->needed = 3;
			dec_ctx->codepoint = x & 0x07;
			return DUK__CP_CONTINUE;
		} else {
			/* not a legal initial byte */
			return DUK__CP_ERROR;
		}
	} else {
		/* process continuation byte */
		if (x >= dec_ctx->lower && x <= dec_ctx->upper) {
			dec_ctx->lower = 0x80;
			dec_ctx->upper = 0xbf;
			dec_ctx->codepoint = (dec_ctx->codepoint << 6) | (x & 0x3f);
			if (--dec_ctx->needed > 0) {
				/* need more bytes */
				return DUK__CP_CONTINUE;
			} else {
				/* got a codepoint */
				duk_codepoint_t ret;
				DUK_ASSERT(dec_ctx->codepoint <= 0x10ffffL); /* Decoding rules guarantee. */
				ret = dec_ctx->codepoint;
				dec_ctx->codepoint = 0x0000L;
				dec_ctx->needed = 0;
				return ret;
			}
		} else {
			/* We just encountered an illegal UTF-8 continuation byte.  This might
			 * be the initial byte of the next character; if we return a plain
			 * error status and the decoder is in replacement mode, the character
			 * will be masked.  We still need to alert the caller to the error
			 * though.
			 */
			dec_ctx->codepoint = 0x0000L;
			dec_ctx->needed = 0;
			dec_ctx->lower = 0x80;
			dec_ctx->upper = 0xbf;
			return DUK__CP_RETRY;
		}
	}
}

#if defined(DUK_USE_ENCODING_BUILTINS)
DUK_LOCAL void duk__utf8_encode_char(void *udata, duk_codepoint_t codepoint) {
	duk__encode_context *enc_ctx;

	DUK_ASSERT(codepoint >= 0);
	enc_ctx = (duk__encode_context *) udata;
	DUK_ASSERT(enc_ctx != NULL);

#if !defined(DUK_USE_PREFER_SIZE)
	if (codepoint <= 0x7f && enc_ctx->lead == 0x0000L) {
		/* Fast path for ASCII. */
		*enc_ctx->out++ = (duk_uint8_t) codepoint;
		return;
	}
#endif

	if (DUK_UNLIKELY(codepoint > 0x10ffffL)) {
		/* cannot legally encode in UTF-8 */
		codepoint = DUK_UNICODE_CP_REPLACEMENT_CHARACTER;
	} else if (codepoint >= 0xd800L && codepoint <= 0xdfffL) {
		if (codepoint <= 0xdbffL) {
			/* high surrogate */
			duk_codepoint_t prev_lead = enc_ctx->lead;
			enc_ctx->lead = codepoint;
			if (prev_lead == 0x0000L) {
				/* high surrogate, no output */
				return;
			} else {
				/* consecutive high surrogates, consider first one unpaired */
				codepoint = DUK_UNICODE_CP_REPLACEMENT_CHARACTER;
			}
		} else {
			/* low surrogate */
			if (enc_ctx->lead != 0x0000L) {
				codepoint =
				    (duk_codepoint_t) (0x010000L + ((enc_ctx->lead - 0xd800L) << 10) + (codepoint - 0xdc00L));
				enc_ctx->lead = 0x0000L;
			} else {
				/* unpaired low surrogate */
				DUK_ASSERT(enc_ctx->lead == 0x0000L);
				codepoint = DUK_UNICODE_CP_REPLACEMENT_CHARACTER;
			}
		}
	} else {
		if (enc_ctx->lead != 0x0000L) {
			/* unpaired high surrogate: emit replacement character and the input codepoint */
			enc_ctx->lead = 0x0000L;
			enc_ctx->out = duk__utf8_emit_repl(enc_ctx->out);
		}
	}

	/* Codepoint may be original input, a decoded surrogate pair, or may
	 * have been replaced with U+FFFD.
	 */
	enc_ctx->out += duk_unicode_encode_xutf8((duk_ucodepoint_t) codepoint, enc_ctx->out);
}
#endif /* DUK_USE_ENCODING_BUILTINS */

/* Shared helper for buffer-to-string using a TextDecoder() compatible UTF-8
 * decoder.
 */
DUK_LOCAL duk_ret_t duk__decode_helper(duk_hthread *thr, duk__decode_context *dec_ctx) {
	const duk_uint8_t *input;
	duk_size_t len = 0;
	duk_size_t len_tmp;
	duk_bool_t stream = 0;
	duk_codepoint_t codepoint;
	duk_uint8_t *output;
	const duk_uint8_t *in;
	duk_uint8_t *out;

	DUK_ASSERT(dec_ctx != NULL);

	/* Careful with input buffer pointer: any side effects involving
	 * code execution (e.g. getters, coercion calls, and finalizers)
	 * may cause a resize and invalidate a pointer we've read.  This
	 * is why the pointer is actually looked up at the last minute.
	 * Argument validation must still happen first to match WHATWG
	 * required side effect order.
	 */

	if (duk_is_undefined(thr, 0)) {
		duk_push_fixed_buffer_nozero(thr, 0);
		duk_replace(thr, 0);
	}
	(void) duk_require_buffer_data(thr, 0, &len); /* Need 'len', avoid pointer. */

	if (duk_check_type_mask(thr, 1, DUK_TYPE_MASK_UNDEFINED | DUK_TYPE_MASK_NULL | DUK_TYPE_MASK_NONE)) {
		/* Use defaults, treat missing value like undefined. */
	} else {
		duk_require_type_mask(thr,
		                      1,
		                      DUK_TYPE_MASK_UNDEFINED | DUK_TYPE_MASK_NULL | DUK_TYPE_MASK_LIGHTFUNC |
		                          DUK_TYPE_MASK_BUFFER | DUK_TYPE_MASK_OBJECT);
		if (duk_get_prop_literal(thr, 1, "stream")) {
			stream = duk_to_boolean(thr, -1);
		}
	}

	/* Allowance is 3*len in the general case because all bytes may potentially
	 * become U+FFFD.  If the first byte completes a non-BMP codepoint it will
	 * decode to a CESU-8 surrogate pair (6 bytes) so we allow 3 extra bytes to
	 * compensate: (1*3)+3 = 6.  Non-BMP codepoints are safe otherwise because
	 * the 4->6 expansion is well under the 3x allowance.
	 *
	 * XXX: As with TextEncoder, need a better buffer allocation strategy here.
	 */
	if (len >= (DUK_HBUFFER_MAX_BYTELEN / 3) - 3) {
		DUK_ERROR_TYPE(thr, DUK_STR_RESULT_TOO_LONG);
		DUK_WO_NORETURN(return 0;);
	}
	output =
	    (duk_uint8_t *) duk_push_fixed_buffer_nozero(thr, 3 + (3 * len)); /* used parts will be always manually written over */

	input = (const duk_uint8_t *) duk_get_buffer_data(thr, 0, &len_tmp);
	DUK_ASSERT(input != NULL || len == 0);
	if (DUK_UNLIKELY(len != len_tmp)) {
		/* Very unlikely but possible: source buffer was resized by
		 * a side effect when fixed buffer was pushed.  Output buffer
		 * may not be large enough to hold output, so just fail if
		 * length has changed.
		 */
		DUK_D(DUK_DPRINT("input buffer resized by side effect, fail"));
		goto fail_type;
	}

	/* From this point onwards it's critical that no side effect occur
	 * which may disturb 'input': finalizer execution, property accesses,
	 * active coercions, etc.  Even an allocation related mark-and-sweep
	 * may affect the pointer because it may trigger a pending finalizer.
	 */

	in = input;
	out = output;
	while (in < input + len) {
		codepoint = duk__utf8_decode_next(dec_ctx, *in++);
		if (codepoint < 0) {
			if (codepoint == DUK__CP_CONTINUE) {
				continue;
			}

			/* Decoding error with or without retry. */
			DUK_ASSERT(codepoint == DUK__CP_ERROR || codepoint == DUK__CP_RETRY);
			if (codepoint == DUK__CP_RETRY) {
				--in; /* retry last byte */
			}
			/* replacement mode: replace with U+FFFD */
			codepoint = DUK_UNICODE_CP_REPLACEMENT_CHARACTER;
			if (dec_ctx->fatal) {
				/* fatal mode: throw a TypeError */
				goto fail_type;
			}
			/* Continue with 'codepoint', Unicode replacement. */
		}
		DUK_ASSERT(codepoint >= 0x0000L && codepoint <= 0x10ffffL);

		if (!dec_ctx->bom_handled) {
			dec_ctx->bom_handled = 1;
			if (codepoint == 0xfeffL && !dec_ctx->ignore_bom) {
				continue;
			}
		}

		out += duk_unicode_encode_cesu8((duk_ucodepoint_t) codepoint, out);
		DUK_ASSERT(out <= output + (3 + (3 * len)));
	}

	if (!stream) {
		if (dec_ctx->needed != 0) {
			/* truncated sequence at end of buffer */
			if (dec_ctx->fatal) {
				goto fail_type;
			} else {
				out += duk_unicode_encode_cesu8(DUK_UNICODE_CP_REPLACEMENT_CHARACTER, out);
				DUK_ASSERT(out <= output + (3 + (3 * len)));
			}
		}
		duk__utf8_decode_init(dec_ctx); /* Initialize decoding state for potential reuse. */
	}

	/* Output buffer is fixed and thus stable even if there had been
	 * side effects (which there shouldn't be).
	 */
	duk_push_lstring(thr, (const char *) output, (duk_size_t) (out - output));
	return 1;

fail_type:
	DUK_ERROR_TYPE(thr, DUK_STR_UTF8_DECODE_FAILED);
	DUK_WO_NORETURN(return 0;);
}

/*
 *  Built-in bindings
 */

#if defined(DUK_USE_ENCODING_BUILTINS)
DUK_INTERNAL duk_ret_t duk_bi_textencoder_constructor(duk_hthread *thr) {
	/* TextEncoder currently requires no persistent state, so the constructor
	 * does nothing on purpose.
	 */

	duk_require_constructor_call(thr);
	return 0;
}

DUK_INTERNAL duk_ret_t duk_bi_textencoder_prototype_encoding_getter(duk_hthread *thr) {
	duk_push_literal(thr, "utf-8");
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_textencoder_prototype_encode(duk_hthread *thr) {
	duk__encode_context enc_ctx;
	duk_size_t len;
	duk_size_t final_len;
	duk_uint8_t *output;

	DUK_ASSERT_TOP(thr, 1);
	if (duk_is_undefined(thr, 0)) {
		len = 0;
	} else {
		duk_hstring *h_input;

		h_input = duk_to_hstring(thr, 0);
		DUK_ASSERT(h_input != NULL);

		len = (duk_size_t) DUK_HSTRING_GET_CHARLEN(h_input);
		if (len >= DUK_HBUFFER_MAX_BYTELEN / 3) {
			DUK_ERROR_TYPE(thr, DUK_STR_RESULT_TOO_LONG);
			DUK_WO_NORETURN(return 0;);
		}
	}

	/* Allowance is 3*len because all bytes can potentially be replaced with
	 * U+FFFD -- which rather inconveniently encodes to 3 bytes in UTF-8.
	 * Rely on dynamic buffer data pointer stability: no other code has
	 * access to the data pointer.
	 *
	 * XXX: The buffer allocation strategy used here is rather inefficient.
	 * Maybe switch to a chunk-based strategy, or preprocess the string to
	 * figure out the space needed ahead of time?
	 */
	DUK_ASSERT(3 * len >= len);
	output = (duk_uint8_t *) duk_push_dynamic_buffer(thr, 3 * len);

	if (len > 0) {
		DUK_ASSERT(duk_is_string(thr, 0)); /* True if len > 0. */

		/* XXX: duk_decode_string() is used to process the input
		 * string.  For standard ECMAScript strings, represented
		 * internally as CESU-8, this is fine.  However, behavior
		 * beyond CESU-8 is not very strict: codepoints using an
		 * extended form of UTF-8 are also accepted, and invalid
		 * codepoint sequences (which are allowed in Duktape strings)
		 * are not handled as well as they could (e.g. invalid
		 * continuation bytes may mask following codepoints).
		 * This is how ECMAScript code would also see such strings.
		 * Maybe replace duk_decode_string() with an explicit strict
		 * CESU-8 decoder here?
		 */
		enc_ctx.lead = 0x0000L;
		enc_ctx.out = output;
		duk_decode_string(thr, 0, duk__utf8_encode_char, (void *) &enc_ctx);
		if (enc_ctx.lead != 0x0000L) {
			/* unpaired high surrogate at end of string */
			enc_ctx.out = duk__utf8_emit_repl(enc_ctx.out);
			DUK_ASSERT(enc_ctx.out <= output + (3 * len));
		}

		/* The output buffer is usually very much oversized, so shrink it to
		 * actually needed size.  Pointer stability assumed up to this point.
		 */
		DUK_ASSERT_TOP(thr, 2);
		DUK_ASSERT(output == (duk_uint8_t *) duk_get_buffer_data(thr, -1, NULL));

		final_len = (duk_size_t) (enc_ctx.out - output);
		duk_resize_buffer(thr, -1, final_len);
		/* 'output' and 'enc_ctx.out' are potentially invalidated by the resize. */
	} else {
		final_len = 0;
	}

	/* Standard WHATWG output is a Uint8Array.  Here the Uint8Array will
	 * be backed by a dynamic buffer which differs from e.g. Uint8Arrays
	 * created as 'new Uint8Array(N)'.  ECMAScript code won't see the
	 * difference but C code will.  When bufferobjects are not supported,
	 * returns a plain dynamic buffer.
	 */
#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
	duk_push_buffer_object(thr, -1, 0, final_len, DUK_BUFOBJ_UINT8ARRAY);
#endif
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_textdecoder_constructor(duk_hthread *thr) {
	duk__decode_context *dec_ctx;
	duk_bool_t fatal = 0;
	duk_bool_t ignore_bom = 0;

	DUK_ASSERT_TOP(thr, 2);
	duk_require_constructor_call(thr);
	if (!duk_is_undefined(thr, 0)) {
		/* XXX: For now ignore 'label' (encoding identifier). */
		duk_to_string(thr, 0);
	}
	if (!duk_is_null_or_undefined(thr, 1)) {
		if (duk_get_prop_literal(thr, 1, "fatal")) {
			fatal = duk_to_boolean(thr, -1);
		}
		if (duk_get_prop_literal(thr, 1, "ignoreBOM")) {
			ignore_bom = duk_to_boolean(thr, -1);
		}
	}

	duk_push_this(thr);

	/* The decode context is not assumed to be zeroed; all fields are
	 * initialized explicitly.
	 */
	dec_ctx = (duk__decode_context *) duk_push_fixed_buffer(thr, sizeof(duk__decode_context));
	dec_ctx->fatal = (duk_uint8_t) fatal;
	dec_ctx->ignore_bom = (duk_uint8_t) ignore_bom;
	duk__utf8_decode_init(dec_ctx); /* Initializes remaining fields. */

	duk_put_prop_literal(thr, -2, DUK_INTERNAL_SYMBOL("Context"));
	return 0;
}

/* Get TextDecoder context from 'this'; leaves garbage on stack. */
DUK_LOCAL duk__decode_context *duk__get_textdecoder_context(duk_hthread *thr) {
	duk__decode_context *dec_ctx;
	duk_push_this(thr);
	duk_get_prop_literal(thr, -1, DUK_INTERNAL_SYMBOL("Context"));
	dec_ctx = (duk__decode_context *) duk_require_buffer(thr, -1, NULL);
	DUK_ASSERT(dec_ctx != NULL);
	return dec_ctx;
}

DUK_INTERNAL duk_ret_t duk_bi_textdecoder_prototype_shared_getter(duk_hthread *thr) {
	duk__decode_context *dec_ctx;
	duk_int_t magic;

	dec_ctx = duk__get_textdecoder_context(thr);
	magic = duk_get_current_magic(thr);
	switch (magic) {
	case 0:
		/* Encoding is now fixed, so _Context lookup is only needed to
		 * validate the 'this' binding (TypeError if not TextDecoder-like).
		 */
		duk_push_literal(thr, "utf-8");
		break;
	case 1:
		duk_push_boolean(thr, dec_ctx->fatal);
		break;
	default:
		duk_push_boolean(thr, dec_ctx->ignore_bom);
		break;
	}

	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_textdecoder_prototype_decode(duk_hthread *thr) {
	duk__decode_context *dec_ctx;

	dec_ctx = duk__get_textdecoder_context(thr);
	return duk__decode_helper(thr, dec_ctx);
}
#endif /* DUK_USE_ENCODING_BUILTINS */

/*
 *  Internal helper for Node.js Buffer
 */

/* Internal helper used for Node.js Buffer .toString().  Value stack convention
 * is currently odd: it mimics TextDecoder .decode() so that argument must be at
 * index 0, and decode options (not present for Buffer) at index 1.  Return value
 * is a Duktape/C function return value.
 */
DUK_INTERNAL duk_ret_t duk_textdecoder_decode_utf8_nodejs(duk_hthread *thr) {
	duk__decode_context dec_ctx;

	dec_ctx.fatal = 0; /* use replacement chars */
	dec_ctx.ignore_bom = 1; /* ignore BOMs (matches Node.js Buffer .toString()) */
	duk__utf8_decode_init(&dec_ctx);

	return duk__decode_helper(thr, &dec_ctx);
}
