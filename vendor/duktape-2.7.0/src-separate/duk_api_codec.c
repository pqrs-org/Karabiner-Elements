/*
 *  Encoding and decoding basic formats: hex, base64.
 *
 *  These are in-place operations which may allow an optimized implementation.
 *
 *  Base-64: https://tools.ietf.org/html/rfc4648#section-4
 */

#include "duk_internal.h"

/*
 *  Misc helpers
 */

/* Shared handling for encode/decode argument.  Fast path handling for
 * buffer and string values because they're the most common.  In particular,
 * avoid creating a temporary string or buffer when possible.  Return value
 * is guaranteed to be non-NULL, even for zero length input.
 */
DUK_LOCAL const duk_uint8_t *duk__prep_codec_arg(duk_hthread *thr, duk_idx_t idx, duk_size_t *out_len) {
	const void *def_ptr = (const void *) out_len; /* Any non-NULL pointer will do. */
	const void *ptr;
	duk_bool_t isbuffer;

	DUK_ASSERT(out_len != NULL);
	DUK_ASSERT(def_ptr != NULL);
	DUK_ASSERT(duk_is_valid_index(thr, idx)); /* checked by caller */

	ptr = (const void *)
	    duk_get_buffer_data_raw(thr, idx, out_len, NULL /*def_ptr*/, 0 /*def_size*/, 0 /*throw_flag*/, &isbuffer);
	if (isbuffer) {
		DUK_ASSERT(ptr != NULL || *out_len == 0U);
		if (DUK_UNLIKELY(ptr == NULL)) {
			ptr = def_ptr;
		}
		DUK_ASSERT(ptr != NULL);
	} else {
		/* For strings a non-NULL pointer is always guaranteed because
		 * at least a NUL will be present.
		 */
		ptr = (const void *) duk_to_lstring(thr, idx, out_len);
		DUK_ASSERT(ptr != NULL);
	}
	DUK_ASSERT(ptr != NULL);
	return (const duk_uint8_t *) ptr;
}

/*
 *  Base64
 */

#if defined(DUK_USE_BASE64_SUPPORT)
/* Bytes emitted for number of padding characters in range [0,4]. */
DUK_LOCAL const duk_int8_t duk__base64_decode_nequal_step[5] = {
	3, /* #### -> 24 bits, emit 3 bytes */
	2, /* ###= -> 18 bits, emit 2 bytes */
	1, /* ##== -> 12 bits, emit 1 byte */
	-1, /* #=== -> 6 bits, error */
	0, /* ==== -> 0 bits, emit 0 bytes */
};

#if defined(DUK_USE_BASE64_FASTPATH)
DUK_LOCAL const duk_uint8_t duk__base64_enctab_fast[64] = {
	0x41U, 0x42U, 0x43U, 0x44U, 0x45U, 0x46U, 0x47U, 0x48U, 0x49U, 0x4aU, 0x4bU, 0x4cU, 0x4dU, 0x4eU, 0x4fU, 0x50U, /* A...P */
	0x51U, 0x52U, 0x53U, 0x54U, 0x55U, 0x56U, 0x57U, 0x58U, 0x59U, 0x5aU, 0x61U, 0x62U, 0x63U, 0x64U, 0x65U, 0x66U, /* Q...f */
	0x67U, 0x68U, 0x69U, 0x6aU, 0x6bU, 0x6cU, 0x6dU, 0x6eU, 0x6fU, 0x70U, 0x71U, 0x72U, 0x73U, 0x74U, 0x75U, 0x76U, /* g...v */
	0x77U, 0x78U, 0x79U, 0x7aU, 0x30U, 0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U, 0x39U, 0x2bU, 0x2fU /* w.../ */
};
#endif /* DUK_USE_BASE64_FASTPATH */

#if defined(DUK_USE_BASE64_FASTPATH)
/* Decode table for one byte of input:
 *   -1 = allowed whitespace
 *   -2 = padding
 *   -3 = error
 *    0...63 decoded bytes
 */
DUK_LOCAL const duk_int8_t duk__base64_dectab_fast[256] = {
	-3, -3, -3, -3, -3, -3, -3, -3, -3, -1, -1, -3, -3, -1, -3, -3, /* 0x00...0x0f */
	-3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, /* 0x10...0x1f */
	-1, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 62, -3, -3, -3, 63, /* 0x20...0x2f */
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -3, -3, -3, -2, -3, -3, /* 0x30...0x3f */
	-3, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, /* 0x40...0x4f */
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -3, -3, -3, -3, -3, /* 0x50...0x5f */
	-3, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 0x60...0x6f */
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -3, -3, -3, -3, -3, /* 0x70...0x7f */
	-3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, /* 0x80...0x8f */
	-3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, /* 0x90...0x9f */
	-3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, /* 0xa0...0xaf */
	-3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, /* 0xb0...0xbf */
	-3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, /* 0xc0...0xcf */
	-3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, /* 0xd0...0xdf */
	-3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, /* 0xe0...0xef */
	-3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3 /* 0xf0...0xff */
};
#endif /* DUK_USE_BASE64_FASTPATH */

#if defined(DUK_USE_BASE64_FASTPATH)
DUK_LOCAL DUK_ALWAYS_INLINE void duk__base64_encode_fast_3(const duk_uint8_t *src, duk_uint8_t *dst) {
	duk_uint_t t;

	t = (duk_uint_t) src[0];
	t = (t << 8) + (duk_uint_t) src[1];
	t = (t << 8) + (duk_uint_t) src[2];

	dst[0] = duk__base64_enctab_fast[t >> 18];
	dst[1] = duk__base64_enctab_fast[(t >> 12) & 0x3fU];
	dst[2] = duk__base64_enctab_fast[(t >> 6) & 0x3fU];
	dst[3] = duk__base64_enctab_fast[t & 0x3fU];

#if 0
	/* Tested: not faster on x64, most likely due to aliasing between
	 * output and input index computation.
	 */
	/* aaaaaabb bbbbcccc ccdddddd */
	dst[0] = duk__base64_enctab_fast[(src[0] >> 2) & 0x3fU];
	dst[1] = duk__base64_enctab_fast[((src[0] << 4) & 0x30U) | ((src[1] >> 4) & 0x0fU)];
	dst[2] = duk__base64_enctab_fast[((src[1] << 2) & 0x3fU) | ((src[2] >> 6) & 0x03U)];
	dst[3] = duk__base64_enctab_fast[src[2] & 0x3fU];
#endif
}

DUK_LOCAL DUK_ALWAYS_INLINE void duk__base64_encode_fast_2(const duk_uint8_t *src, duk_uint8_t *dst) {
	duk_uint_t t;

	t = (duk_uint_t) src[0];
	t = (t << 8) + (duk_uint_t) src[1];
	dst[0] = duk__base64_enctab_fast[t >> 10]; /* XXXXXX-- -------- */
	dst[1] = duk__base64_enctab_fast[(t >> 4) & 0x3fU]; /* ------XX XXXX---- */
	dst[2] = duk__base64_enctab_fast[(t << 2) & 0x3fU]; /* -------- ----XXXX */
	dst[3] = DUK_ASC_EQUALS;
}

DUK_LOCAL DUK_ALWAYS_INLINE void duk__base64_encode_fast_1(const duk_uint8_t *src, duk_uint8_t *dst) {
	duk_uint_t t;

	t = (duk_uint_t) src[0];
	dst[0] = duk__base64_enctab_fast[t >> 2]; /* XXXXXX-- */
	dst[1] = duk__base64_enctab_fast[(t << 4) & 0x3fU]; /* ------XX */
	dst[2] = DUK_ASC_EQUALS;
	dst[3] = DUK_ASC_EQUALS;
}

DUK_LOCAL void duk__base64_encode_helper(const duk_uint8_t *src, duk_size_t srclen, duk_uint8_t *dst) {
	duk_size_t n;
	const duk_uint8_t *p;
	duk_uint8_t *q;

	n = srclen;
	p = src;
	q = dst;

	if (n >= 16U) {
		/* Fast path, unrolled by 4, allows interleaving.  Process
		 * 12-byte input chunks which encode to 16-char output chunks.
		 * Only enter when at least one block is emitted (avoids div+mul
		 * for short inputs too).
		 */
		const duk_uint8_t *p_end_fast;

		p_end_fast = p + ((n / 12U) * 12U);
		DUK_ASSERT(p_end_fast >= p + 12);
		do {
			duk__base64_encode_fast_3(p, q);
			duk__base64_encode_fast_3(p + 3, q + 4);
			duk__base64_encode_fast_3(p + 6, q + 8);
			duk__base64_encode_fast_3(p + 9, q + 12);
			p += 12;
			q += 16;
		} while (DUK_LIKELY(p != p_end_fast));

		DUK_ASSERT(src + srclen >= p);
		n = (duk_size_t) (src + srclen - p);
		DUK_ASSERT(n < 12U);
	}

	/* Remainder. */
	while (n >= 3U) {
		duk__base64_encode_fast_3(p, q);
		p += 3;
		q += 4;
		n -= 3U;
	}
	DUK_ASSERT(n == 0U || n == 1U || n == 2U);
	if (n == 1U) {
		duk__base64_encode_fast_1(p, q);
#if 0 /* Unnecessary. */
		p += 1;
		q += 4;
		n -= 1U;
#endif
	} else if (n == 2U) {
		duk__base64_encode_fast_2(p, q);
#if 0 /* Unnecessary. */
		p += 2;
		q += 4;
		n -= 2U;
#endif
	} else {
		DUK_ASSERT(n == 0U); /* nothing to do */
		;
	}
}
#else /* DUK_USE_BASE64_FASTPATH */
DUK_LOCAL void duk__base64_encode_helper(const duk_uint8_t *src, duk_size_t srclen, duk_uint8_t *dst) {
	duk_small_uint_t i, npad;
	duk_uint_t t, x, y;
	const duk_uint8_t *p;
	const duk_uint8_t *p_end;
	duk_uint8_t *q;

	p = src;
	p_end = src + srclen;
	q = dst;
	npad = 0U;

	while (p < p_end) {
		/* Read 3 bytes into 't', padded by zero. */
		t = 0;
		for (i = 0; i < 3; i++) {
			t = t << 8;
			if (p < p_end) {
				t += (duk_uint_t) (*p++);
			} else {
				/* This only happens on the last loop and we're
				 * guaranteed to exit on the next loop.
				 */
				npad++;
			}
		}
		DUK_ASSERT(npad <= 2U);

		/* Emit 4 encoded characters.  If npad > 0, some of the
		 * chars will be incorrect (zero bits) but we fix up the
		 * padding after the loop.  A straightforward 64-byte
		 * lookup would be faster and cleaner, but this is shorter.
		 */
		for (i = 0; i < 4; i++) {
			x = ((t >> 18) & 0x3fU);
			t = t << 6;

			if (x <= 51U) {
				if (x <= 25) {
					y = x + DUK_ASC_UC_A;
				} else {
					y = x - 26 + DUK_ASC_LC_A;
				}
			} else {
				if (x <= 61U) {
					y = x - 52 + DUK_ASC_0;
				} else if (x == 62) {
					y = DUK_ASC_PLUS;
				} else {
					DUK_ASSERT(x == 63);
					y = DUK_ASC_SLASH;
				}
			}

			*q++ = (duk_uint8_t) y;
		}
	}

	/* Handle padding by rewriting 0-2 bogus characters at the end.
	 *
	 *  Missing bytes    npad     base64 example
	 *    0               0         ####
	 *    1               1         ###=
	 *    2               2         ##==
	 */
	DUK_ASSERT(npad <= 2U);
	while (npad > 0U) {
		*(q - npad) = DUK_ASC_EQUALS;
		npad--;
	}
}
#endif /* DUK_USE_BASE64_FASTPATH */

#if defined(DUK_USE_BASE64_FASTPATH)
DUK_LOCAL duk_bool_t duk__base64_decode_helper(const duk_uint8_t *src,
                                               duk_size_t srclen,
                                               duk_uint8_t *dst,
                                               duk_uint8_t **out_dst_final) {
	duk_int_t x;
	duk_uint_t t;
	duk_small_uint_t n_equal;
	duk_int8_t step;
	const duk_uint8_t *p;
	const duk_uint8_t *p_end;
	const duk_uint8_t *p_end_safe;
	duk_uint8_t *q;

	DUK_ASSERT(src != NULL); /* Required by pointer arithmetic below, which fails for NULL. */

	p = src;
	p_end = src + srclen;
	p_end_safe = p_end - 8; /* If 'src <= src_end_safe', safe to read 8 bytes. */
	q = dst;

	/* Alternate between a fast path which processes clean groups with no
	 * padding or whitespace, and a slow path which processes one arbitrary
	 * group and then re-enters the fast path.  This handles e.g. base64
	 * with newlines reasonably well because the majority of a line is in
	 * the fast path.
	 */
	for (;;) {
		/* Fast path, on each loop handle two 4-char input groups.
		 * If both are clean, emit 6 bytes and continue.  If first
		 * is clean, emit 3 bytes and drop out; otherwise emit
		 * nothing and drop out.  This approach could be extended to
		 * more groups per loop, but for inputs with e.g. periodic
		 * newlines (which are common) it might not be an improvement.
		 */
		while (DUK_LIKELY(p <= p_end_safe)) {
			duk_int_t t1, t2;

			/* The lookup byte is intentionally sign extended to
			 * (at least) 32 bits and then ORed.  This ensures
			 * that is at least 1 byte is negative, the highest
			 * bit of the accumulator will be set at the end and
			 * we don't need to check every byte.
			 *
			 * Read all input bytes first before writing output
			 * bytes to minimize aliasing.
			 */
			DUK_DDD(DUK_DDDPRINT("fast loop: p=%p, p_end_safe=%p, p_end=%p",
			                     (const void *) p,
			                     (const void *) p_end_safe,
			                     (const void *) p_end));

			t1 = (duk_int_t) duk__base64_dectab_fast[p[0]];
			t1 = (duk_int_t) ((duk_uint_t) t1 << 6) | (duk_int_t) duk__base64_dectab_fast[p[1]];
			t1 = (duk_int_t) ((duk_uint_t) t1 << 6) | (duk_int_t) duk__base64_dectab_fast[p[2]];
			t1 = (duk_int_t) ((duk_uint_t) t1 << 6) | (duk_int_t) duk__base64_dectab_fast[p[3]];

			t2 = (duk_int_t) duk__base64_dectab_fast[p[4]];
			t2 = (duk_int_t) ((duk_uint_t) t2 << 6) | (duk_int_t) duk__base64_dectab_fast[p[5]];
			t2 = (duk_int_t) ((duk_uint_t) t2 << 6) | (duk_int_t) duk__base64_dectab_fast[p[6]];
			t2 = (duk_int_t) ((duk_uint_t) t2 << 6) | (duk_int_t) duk__base64_dectab_fast[p[7]];

			q[0] = (duk_uint8_t) (((duk_uint_t) t1 >> 16) & 0xffU);
			q[1] = (duk_uint8_t) (((duk_uint_t) t1 >> 8) & 0xffU);
			q[2] = (duk_uint8_t) ((duk_uint_t) t1 & 0xffU);

			q[3] = (duk_uint8_t) (((duk_uint_t) t2 >> 16) & 0xffU);
			q[4] = (duk_uint8_t) (((duk_uint_t) t2 >> 8) & 0xffU);
			q[5] = (duk_uint8_t) ((duk_uint_t) t2 & 0xffU);

			/* Optimistic check using one branch. */
			if (DUK_LIKELY((t1 | t2) >= 0)) {
				p += 8;
				q += 6;
			} else if (t1 >= 0) {
				DUK_DDD(
				    DUK_DDDPRINT("fast loop first group was clean, second was not, process one slow path group"));
				DUK_ASSERT(t2 < 0);
				p += 4;
				q += 3;
				break;
			} else {
				DUK_DDD(DUK_DDDPRINT(
				    "fast loop first group was not clean, second does not matter, process one slow path group"));
				DUK_ASSERT(t1 < 0);
				break;
			}
		} /* fast path */

		/* Slow path step 1: try to scan a 4-character encoded group,
		 * end-of-input, or start-of-padding.  We exit with:
		 *   1. n_chars == 4: full group, no padding, no end-of-input.
		 *   2. n_chars < 4: partial group (may also be 0), encountered
		 *      padding or end of input.
		 *
		 * The accumulator is initialized to 1; this allows us to detect
		 * a full group by comparing >= 0x1000000 without an extra
		 * counter variable.
		 */
		t = 1UL;
		for (;;) {
			DUK_DDD(DUK_DDDPRINT("slow loop: p=%p, p_end=%p, t=%lu",
			                     (const void *) p,
			                     (const void *) p_end,
			                     (unsigned long) t));

			if (DUK_LIKELY(p < p_end)) {
				x = duk__base64_dectab_fast[*p++];
				if (DUK_LIKELY(x >= 0)) {
					DUK_ASSERT(x >= 0 && x <= 63);
					t = (t << 6) + (duk_uint_t) x;
					if (t >= 0x1000000UL) {
						break;
					}
				} else if (x == -1) {
					continue; /* allowed ascii whitespace */
				} else if (x == -2) {
					p--;
					break; /* start of padding */
				} else {
					DUK_ASSERT(x == -3);
					goto decode_error;
				}
			} else {
				break; /* end of input */
			}
		} /* slow path step 1 */

		/* Complete the padding by simulating pad characters,
		 * regardless of actual input padding chars.
		 */
		n_equal = 0;
		while (t < 0x1000000UL) {
			t = (t << 6) + 0U;
			n_equal++;
		}

		/* Slow path step 2: deal with full/partial group, padding,
		 * etc.  Note that for num chars in [0,3] we intentionally emit
		 * 3 bytes but don't step forward that much, buffer space is
		 * guaranteed in setup.
		 *
		 *  num chars:
		 *   0      ####   no output (= step 0)
		 *   1      #===   reject, 6 bits of data
		 *   2      ##==   12 bits of data, output 1 byte (= step 1)
		 *   3      ###=   18 bits of data, output 2 bytes (= step 2)
		 *   4      ####   24 bits of data, output 3 bytes (= step 3)
		 */
		q[0] = (duk_uint8_t) ((t >> 16) & 0xffU);
		q[1] = (duk_uint8_t) ((t >> 8) & 0xffU);
		q[2] = (duk_uint8_t) (t & 0xffU);

		DUK_ASSERT(n_equal <= 4);
		step = duk__base64_decode_nequal_step[n_equal];
		if (DUK_UNLIKELY(step < 0)) {
			goto decode_error;
		}
		q += step;

		/* Slow path step 3: read and ignore padding and whitespace
		 * until (a) next non-padding and non-whitespace character
		 * after which we resume the fast path, or (b) end of input.
		 * This allows us to accept missing, partial, full, and extra
		 * padding cases uniformly.  We also support concatenated
		 * base-64 documents because we resume scanning afterwards.
		 *
		 * Note that to support concatenated documents well, the '='
		 * padding found inside the input must also allow for 'extra'
		 * padding.  For example, 'Zm===' decodes to 'f' and has one
		 * extra padding char.  So, 'Zm===Zm' should decode 'ff', even
		 * though the standard break-up would be 'Zm==' + '=Zm' which
		 * doesn't make sense.
		 *
		 * We also accept prepended padding like '==Zm9', because it
		 * is equivalent to an empty document with extra padding ('==')
		 * followed by a valid document.
		 */

		for (;;) {
			if (DUK_UNLIKELY(p >= p_end)) {
				goto done;
			}
			x = duk__base64_dectab_fast[*p++];
			if (x == -1 || x == -2) {
				; /* padding or whitespace, keep eating */
			} else {
				p--;
				break; /* backtrack and go back to fast path, even for -1 */
			}
		} /* slow path step 3 */
	} /* outer fast+slow path loop */

done:
	DUK_DDD(DUK_DDDPRINT("done; p=%p, p_end=%p", (const void *) p, (const void *) p_end));

	DUK_ASSERT(p == p_end);

	*out_dst_final = q;
	return 1;

decode_error:
	return 0;
}
#else /* DUK_USE_BASE64_FASTPATH */
DUK_LOCAL duk_bool_t duk__base64_decode_helper(const duk_uint8_t *src,
                                               duk_size_t srclen,
                                               duk_uint8_t *dst,
                                               duk_uint8_t **out_dst_final) {
	duk_uint_t t, x;
	duk_int_t y;
	duk_int8_t step;
	const duk_uint8_t *p;
	const duk_uint8_t *p_end;
	duk_uint8_t *q;
	/* 0x09, 0x0a, or 0x0d */
	duk_uint32_t mask_white = (1U << 9) | (1U << 10) | (1U << 13);

	/* 't' tracks progress of the decoded group:
	 *
	 *  t == 1             no valid chars yet
	 *  t >= 0x40          1x6 = 6 bits shifted in
	 *  t >= 0x1000        2x6 = 12 bits shifted in
	 *  t >= 0x40000       3x6 = 18 bits shifted in
	 *  t >= 0x1000000     4x6 = 24 bits shifted in
	 *
	 * By initializing t=1 there's no need for a separate counter for
	 * the number of characters found so far.
	 */
	p = src;
	p_end = src + srclen;
	q = dst;
	t = 1UL;

	for (;;) {
		duk_small_uint_t n_equal;

		DUK_ASSERT(t >= 1U);
		if (p >= p_end) {
			/* End of input: if input exists, treat like
			 * start of padding, finish the block, then
			 * re-enter here to see we're done.
			 */
			if (t == 1U) {
				break;
			} else {
				goto simulate_padding;
			}
		}

		x = *p++;

		if (x >= 0x41U) {
			/* Valid: a-z and A-Z. */
			DUK_ASSERT(x >= 0x41U && x <= 0xffU);
			if (x >= 0x61U && x <= 0x7aU) {
				y = (duk_int_t) x - 0x61 + 26;
			} else if (x <= 0x5aU) {
				y = (duk_int_t) x - 0x41;
			} else {
				goto decode_error;
			}
		} else if (x >= 0x30U) {
			/* Valid: 0-9 and =. */
			DUK_ASSERT(x >= 0x30U && x <= 0x40U);
			if (x <= 0x39U) {
				y = (duk_int_t) x - 0x30 + 52;
			} else if (x == 0x3dU) {
				/* Skip padding and whitespace unless we're in the
				 * middle of a block.  Otherwise complete group by
				 * simulating shifting in the correct padding.
				 */
				if (t == 1U) {
					continue;
				}
				goto simulate_padding;
			} else {
				goto decode_error;
			}
		} else if (x >= 0x20U) {
			/* Valid: +, /, and 0x20 whitespace. */
			DUK_ASSERT(x >= 0x20U && x <= 0x2fU);
			if (x == 0x2bU) {
				y = 62;
			} else if (x == 0x2fU) {
				y = 63;
			} else if (x == 0x20U) {
				continue;
			} else {
				goto decode_error;
			}
		} else {
			/* Valid: whitespace. */
			duk_uint32_t m;
			DUK_ASSERT(x < 0x20U); /* 0x00 to 0x1f */
			m = (1U << x);
			if (mask_white & m) {
				/* Allow basic ASCII whitespace. */
				continue;
			} else {
				goto decode_error;
			}
		}

		DUK_ASSERT(y >= 0 && y <= 63);
		t = (t << 6) + (duk_uint_t) y;
		if (t < 0x1000000UL) {
			continue;
		}
		/* fall through; no padding will be added */

	simulate_padding:
		n_equal = 0;
		while (t < 0x1000000UL) {
			t = (t << 6) + 0U;
			n_equal++;
		}

		/* Output 3 bytes from 't' and advance as needed. */
		q[0] = (duk_uint8_t) ((t >> 16) & 0xffU);
		q[1] = (duk_uint8_t) ((t >> 8) & 0xffU);
		q[2] = (duk_uint8_t) (t & 0xffU);

		DUK_ASSERT(n_equal <= 4U);
		step = duk__base64_decode_nequal_step[n_equal];
		if (step < 0) {
			goto decode_error;
		}
		q += step;

		/* Re-enter loop.  The actual padding characters are skipped
		 * by the main loop.  This handles cases like missing, partial,
		 * full, and extra padding, and allows parsing of concatenated
		 * documents (with extra padding) like: Zm===Zm.  Also extra
		 * prepended padding is accepted: ===Zm9v.
		 */
		t = 1U;
	}
	DUK_ASSERT(t == 1UL);

	*out_dst_final = q;
	return 1;

decode_error:
	return 0;
}
#endif /* DUK_USE_BASE64_FASTPATH */

DUK_EXTERNAL const char *duk_base64_encode(duk_hthread *thr, duk_idx_t idx) {
	const duk_uint8_t *src;
	duk_size_t srclen;
	duk_size_t dstlen;
	duk_uint8_t *dst;
	const char *ret;

	DUK_ASSERT_API_ENTRY(thr);

	idx = duk_require_normalize_index(thr, idx);
	src = duk__prep_codec_arg(thr, idx, &srclen);
	DUK_ASSERT(src != NULL);

	/* Compute exact output length.  Computation must not wrap; this
	 * limit works for 32-bit size_t:
	 * >>> srclen = 3221225469
	 * >>> '%x' % ((srclen + 2) / 3 * 4)
	 * 'fffffffc'
	 */
	if (srclen > 3221225469UL) {
		goto type_error;
	}
	dstlen = (srclen + 2U) / 3U * 4U;
	dst = (duk_uint8_t *) duk_push_fixed_buffer_nozero(thr, dstlen);

	duk__base64_encode_helper((const duk_uint8_t *) src, srclen, dst);

	ret = duk_buffer_to_string(thr, -1); /* Safe, result is ASCII. */
	duk_replace(thr, idx);
	return ret;

type_error:
	DUK_ERROR_TYPE(thr, DUK_STR_BASE64_ENCODE_FAILED);
	DUK_WO_NORETURN(return NULL;);
}

DUK_EXTERNAL void duk_base64_decode(duk_hthread *thr, duk_idx_t idx) {
	const duk_uint8_t *src;
	duk_size_t srclen;
	duk_size_t dstlen;
	duk_uint8_t *dst;
	duk_uint8_t *dst_final;

	DUK_ASSERT_API_ENTRY(thr);

	idx = duk_require_normalize_index(thr, idx);
	src = duk__prep_codec_arg(thr, idx, &srclen);
	DUK_ASSERT(src != NULL);

	/* Round up and add safety margin.  Avoid addition before division to
	 * avoid possibility of wrapping.  Margin includes +3 for rounding up,
	 * and +3 for one extra group: the decoder may emit and then backtrack
	 * a full group (3 bytes) from zero-sized input for technical reasons.
	 * Similarly, 'xx' may ecause 1+3 = bytes to be emitted and then
	 * backtracked.
	 */
	dstlen = (srclen / 4) * 3 + 6; /* upper limit, assuming no whitespace etc */
	dst = (duk_uint8_t *) duk_push_dynamic_buffer(thr, dstlen);
	/* Note: for dstlen=0, dst may be NULL */

	if (!duk__base64_decode_helper((const duk_uint8_t *) src, srclen, dst, &dst_final)) {
		goto type_error;
	}

	/* XXX: convert to fixed buffer? */
	(void) duk_resize_buffer(thr, -1, (duk_size_t) (dst_final - dst));
	duk_replace(thr, idx);
	return;

type_error:
	DUK_ERROR_TYPE(thr, DUK_STR_BASE64_DECODE_FAILED);
	DUK_WO_NORETURN(return;);
}
#else /* DUK_USE_BASE64_SUPPORT */
DUK_EXTERNAL const char *duk_base64_encode(duk_hthread *thr, duk_idx_t idx) {
	DUK_UNREF(idx);
	DUK_ERROR_UNSUPPORTED(thr);
	DUK_WO_NORETURN(return NULL;);
}

DUK_EXTERNAL void duk_base64_decode(duk_hthread *thr, duk_idx_t idx) {
	DUK_UNREF(idx);
	DUK_ERROR_UNSUPPORTED(thr);
	DUK_WO_NORETURN(return;);
}
#endif /* DUK_USE_BASE64_SUPPORT */

/*
 *  Hex
 */

#if defined(DUK_USE_HEX_SUPPORT)
DUK_EXTERNAL const char *duk_hex_encode(duk_hthread *thr, duk_idx_t idx) {
	const duk_uint8_t *inp;
	duk_size_t len;
	duk_size_t i;
	duk_uint8_t *buf;
	const char *ret;
#if defined(DUK_USE_HEX_FASTPATH)
	duk_size_t len_safe;
	duk_uint16_t *p16;
#endif

	DUK_ASSERT_API_ENTRY(thr);

	idx = duk_require_normalize_index(thr, idx);
	inp = duk__prep_codec_arg(thr, idx, &len);
	DUK_ASSERT(inp != NULL);

	/* Fixed buffer, no zeroing because we'll fill all the data. */
	buf = (duk_uint8_t *) duk_push_fixed_buffer_nozero(thr, len * 2);
	DUK_ASSERT(buf != NULL);

#if defined(DUK_USE_HEX_FASTPATH)
	DUK_ASSERT((((duk_size_t) buf) & 0x01U) == 0); /* pointer is aligned, guaranteed for fixed buffer */
	p16 = (duk_uint16_t *) (void *) buf;
	len_safe = len & ~0x03U;
	for (i = 0; i < len_safe; i += 4) {
		p16[0] = duk_hex_enctab[inp[i]];
		p16[1] = duk_hex_enctab[inp[i + 1]];
		p16[2] = duk_hex_enctab[inp[i + 2]];
		p16[3] = duk_hex_enctab[inp[i + 3]];
		p16 += 4;
	}
	for (; i < len; i++) {
		*p16++ = duk_hex_enctab[inp[i]];
	}
#else /* DUK_USE_HEX_FASTPATH */
	for (i = 0; i < len; i++) {
		duk_small_uint_t t;
		t = (duk_small_uint_t) inp[i];
		buf[i * 2 + 0] = duk_lc_digits[t >> 4];
		buf[i * 2 + 1] = duk_lc_digits[t & 0x0f];
	}
#endif /* DUK_USE_HEX_FASTPATH */

	/* XXX: Using a string return value forces a string intern which is
	 * not always necessary.  As a rough performance measure, hex encode
	 * time for tests/perf/test-hex-encode.js dropped from ~35s to ~15s
	 * without string coercion.  Change to returning a buffer and let the
	 * caller coerce to string if necessary?
	 */

	ret = duk_buffer_to_string(thr, -1); /* Safe, result is ASCII. */
	duk_replace(thr, idx);
	return ret;
}

DUK_EXTERNAL void duk_hex_decode(duk_hthread *thr, duk_idx_t idx) {
	const duk_uint8_t *inp;
	duk_size_t len;
	duk_size_t i;
	duk_int_t t;
	duk_uint8_t *buf;
#if defined(DUK_USE_HEX_FASTPATH)
	duk_int_t chk;
	duk_uint8_t *p;
	duk_size_t len_safe;
#endif

	DUK_ASSERT_API_ENTRY(thr);

	idx = duk_require_normalize_index(thr, idx);
	inp = duk__prep_codec_arg(thr, idx, &len);
	DUK_ASSERT(inp != NULL);

	if (len & 0x01) {
		goto type_error;
	}

	/* Fixed buffer, no zeroing because we'll fill all the data. */
	buf = (duk_uint8_t *) duk_push_fixed_buffer_nozero(thr, len / 2);
	DUK_ASSERT(buf != NULL);

#if defined(DUK_USE_HEX_FASTPATH)
	p = buf;
	len_safe = len & ~0x07U;
	for (i = 0; i < len_safe; i += 8) {
		t = ((duk_int_t) duk_hex_dectab_shift4[inp[i]]) | ((duk_int_t) duk_hex_dectab[inp[i + 1]]);
		chk = t;
		p[0] = (duk_uint8_t) t;
		t = ((duk_int_t) duk_hex_dectab_shift4[inp[i + 2]]) | ((duk_int_t) duk_hex_dectab[inp[i + 3]]);
		chk |= t;
		p[1] = (duk_uint8_t) t;
		t = ((duk_int_t) duk_hex_dectab_shift4[inp[i + 4]]) | ((duk_int_t) duk_hex_dectab[inp[i + 5]]);
		chk |= t;
		p[2] = (duk_uint8_t) t;
		t = ((duk_int_t) duk_hex_dectab_shift4[inp[i + 6]]) | ((duk_int_t) duk_hex_dectab[inp[i + 7]]);
		chk |= t;
		p[3] = (duk_uint8_t) t;
		p += 4;

		/* Check if any lookup above had a negative result. */
		if (DUK_UNLIKELY(chk < 0)) {
			goto type_error;
		}
	}
	for (; i < len; i += 2) {
		/* First cast to duk_int_t to sign extend, second cast to
		 * duk_uint_t to avoid signed left shift, and final cast to
		 * duk_int_t result type.
		 */
		t = (duk_int_t) ((((duk_uint_t) (duk_int_t) duk_hex_dectab[inp[i]]) << 4U) |
		                 ((duk_uint_t) (duk_int_t) duk_hex_dectab[inp[i + 1]]));
		if (DUK_UNLIKELY(t < 0)) {
			goto type_error;
		}
		*p++ = (duk_uint8_t) t;
	}
#else /* DUK_USE_HEX_FASTPATH */
	for (i = 0; i < len; i += 2) {
		/* For invalid characters the value -1 gets extended to
		 * at least 16 bits.  If either nybble is invalid, the
		 * resulting 't' will be < 0.
		 */
		t = (duk_int_t) ((((duk_uint_t) (duk_int_t) duk_hex_dectab[inp[i]]) << 4U) |
		                 ((duk_uint_t) (duk_int_t) duk_hex_dectab[inp[i + 1]]));
		if (DUK_UNLIKELY(t < 0)) {
			goto type_error;
		}
		buf[i >> 1] = (duk_uint8_t) t;
	}
#endif /* DUK_USE_HEX_FASTPATH */

	duk_replace(thr, idx);
	return;

type_error:
	DUK_ERROR_TYPE(thr, DUK_STR_HEX_DECODE_FAILED);
	DUK_WO_NORETURN(return;);
}
#else /* DUK_USE_HEX_SUPPORT */
DUK_EXTERNAL const char *duk_hex_encode(duk_hthread *thr, duk_idx_t idx) {
	DUK_UNREF(idx);
	DUK_ERROR_UNSUPPORTED(thr);
	DUK_WO_NORETURN(return NULL;);
}
DUK_EXTERNAL void duk_hex_decode(duk_hthread *thr, duk_idx_t idx) {
	DUK_UNREF(idx);
	DUK_ERROR_UNSUPPORTED(thr);
	DUK_WO_NORETURN(return;);
}
#endif /* DUK_USE_HEX_SUPPORT */

/*
 *  JSON
 */

#if defined(DUK_USE_JSON_SUPPORT)
DUK_EXTERNAL const char *duk_json_encode(duk_hthread *thr, duk_idx_t idx) {
#if defined(DUK_USE_ASSERTIONS)
	duk_idx_t top_at_entry;
#endif
	const char *ret;

	DUK_ASSERT_API_ENTRY(thr);
#if defined(DUK_USE_ASSERTIONS)
	top_at_entry = duk_get_top(thr);
#endif

	idx = duk_require_normalize_index(thr, idx);
	duk_bi_json_stringify_helper(thr,
	                             idx /*idx_value*/,
	                             DUK_INVALID_INDEX /*idx_replacer*/,
	                             DUK_INVALID_INDEX /*idx_space*/,
	                             0 /*flags*/);
	DUK_ASSERT(duk_is_string(thr, -1));
	duk_replace(thr, idx);
	ret = duk_get_string(thr, idx);

	DUK_ASSERT(duk_get_top(thr) == top_at_entry);

	return ret;
}

DUK_EXTERNAL void duk_json_decode(duk_hthread *thr, duk_idx_t idx) {
#if defined(DUK_USE_ASSERTIONS)
	duk_idx_t top_at_entry;
#endif

	DUK_ASSERT_API_ENTRY(thr);
#if defined(DUK_USE_ASSERTIONS)
	top_at_entry = duk_get_top(thr);
#endif

	idx = duk_require_normalize_index(thr, idx);
	duk_bi_json_parse_helper(thr, idx /*idx_value*/, DUK_INVALID_INDEX /*idx_reviver*/, 0 /*flags*/);
	duk_replace(thr, idx);

	DUK_ASSERT(duk_get_top(thr) == top_at_entry);
}
#else /* DUK_USE_JSON_SUPPORT */
DUK_EXTERNAL const char *duk_json_encode(duk_hthread *thr, duk_idx_t idx) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_UNREF(idx);
	DUK_ERROR_UNSUPPORTED(thr);
	DUK_WO_NORETURN(return NULL;);
}

DUK_EXTERNAL void duk_json_decode(duk_hthread *thr, duk_idx_t idx) {
	DUK_ASSERT_API_ENTRY(thr);
	DUK_UNREF(idx);
	DUK_ERROR_UNSUPPORTED(thr);
	DUK_WO_NORETURN(return;);
}
#endif /* DUK_USE_JSON_SUPPORT */
