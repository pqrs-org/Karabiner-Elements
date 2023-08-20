/*
 *  Misc support functions
 */

#include "duk_internal.h"

/*
 *  duk_hstring charCodeAt, with and without surrogate awareness
 */

DUK_INTERNAL duk_ucodepoint_t duk_hstring_char_code_at_raw(duk_hthread *thr,
                                                           duk_hstring *h,
                                                           duk_uint_t pos,
                                                           duk_bool_t surrogate_aware) {
	duk_uint32_t boff;
	const duk_uint8_t *p, *p_start, *p_end;
	duk_ucodepoint_t cp1;
	duk_ucodepoint_t cp2;

	/* Caller must check character offset to be inside the string. */
	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(h != NULL);
	DUK_ASSERT_DISABLE(pos >= 0); /* unsigned */
	DUK_ASSERT(pos < (duk_uint_t) DUK_HSTRING_GET_CHARLEN(h));

	boff = (duk_uint32_t) duk_heap_strcache_offset_char2byte(thr, h, (duk_uint32_t) pos);
	DUK_DDD(DUK_DDDPRINT("charCodeAt: pos=%ld -> boff=%ld, str=%!O", (long) pos, (long) boff, (duk_heaphdr *) h));
	DUK_ASSERT_DISABLE(boff >= 0);
	DUK_ASSERT(boff < DUK_HSTRING_GET_BYTELEN(h));

	p_start = DUK_HSTRING_GET_DATA(h);
	p_end = p_start + DUK_HSTRING_GET_BYTELEN(h);
	p = p_start + boff;
	DUK_DDD(DUK_DDDPRINT("p_start=%p, p_end=%p, p=%p", (const void *) p_start, (const void *) p_end, (const void *) p));

	/* For invalid UTF-8 (never happens for standard ECMAScript strings)
	 * return U+FFFD replacement character.
	 */
	if (duk_unicode_decode_xutf8(thr, &p, p_start, p_end, &cp1)) {
		if (surrogate_aware && cp1 >= 0xd800UL && cp1 <= 0xdbffUL) {
			/* The decode helper is memory safe even if 'cp1' was
			 * decoded at the end of the string and 'p' is no longer
			 * within string memory range.
			 */
			cp2 = 0; /* If call fails, this is left untouched and won't match cp2 check. */
			(void) duk_unicode_decode_xutf8(thr, &p, p_start, p_end, &cp2);
			if (cp2 >= 0xdc00UL && cp2 <= 0xdfffUL) {
				cp1 = (duk_ucodepoint_t) (((cp1 - 0xd800UL) << 10) + (cp2 - 0xdc00UL) + 0x10000UL);
			}
		}
	} else {
		cp1 = DUK_UNICODE_CP_REPLACEMENT_CHARACTER;
	}

	return cp1;
}

/*
 *  duk_hstring charlen, when lazy charlen disabled
 */

#if !defined(DUK_USE_HSTRING_LAZY_CLEN)
#if !defined(DUK_USE_HSTRING_CLEN)
#error non-lazy duk_hstring charlen but DUK_USE_HSTRING_CLEN not set
#endif
DUK_INTERNAL void duk_hstring_init_charlen(duk_hstring *h) {
	duk_uint32_t clen;

	DUK_ASSERT(h != NULL);
	DUK_ASSERT(!DUK_HSTRING_HAS_ASCII(h));
	DUK_ASSERT(!DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) h));

	clen = duk_unicode_unvalidated_utf8_length(DUK_HSTRING_GET_DATA(h), DUK_HSTRING_GET_BYTELEN(h));
#if defined(DUK_USE_STRLEN16)
	DUK_ASSERT(clen <= 0xffffUL); /* Bytelength checked during interning. */
	h->clen16 = (duk_uint16_t) clen;
#else
	h->clen = (duk_uint32_t) clen;
#endif
	if (DUK_LIKELY(clen == DUK_HSTRING_GET_BYTELEN(h))) {
		DUK_HSTRING_SET_ASCII(h);
	}
}

DUK_INTERNAL DUK_HOT duk_size_t duk_hstring_get_charlen(duk_hstring *h) {
#if defined(DUK_USE_STRLEN16)
	return h->clen16;
#else
	return h->clen;
#endif
}
#endif /* !DUK_USE_HSTRING_LAZY_CLEN */

/*
 *  duk_hstring charlen, when lazy charlen enabled
 */

#if defined(DUK_USE_HSTRING_LAZY_CLEN)
#if defined(DUK_USE_HSTRING_CLEN)
DUK_LOCAL DUK_COLD duk_size_t duk__hstring_get_charlen_slowpath(duk_hstring *h) {
	duk_size_t res;

	DUK_ASSERT(h->clen == 0); /* Checked by caller. */

#if defined(DUK_USE_ROM_STRINGS)
	/* ROM strings have precomputed clen, but if the computed clen is zero
	 * we can still come here and can't write anything.
	 */
	if (DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) h)) {
		return 0;
	}
#endif

	res = duk_unicode_unvalidated_utf8_length(DUK_HSTRING_GET_DATA(h), DUK_HSTRING_GET_BYTELEN(h));
#if defined(DUK_USE_STRLEN16)
	DUK_ASSERT(res <= 0xffffUL); /* Bytelength checked during interning. */
	h->clen16 = (duk_uint16_t) res;
#else
	h->clen = (duk_uint32_t) res;
#endif
	if (DUK_LIKELY(res == DUK_HSTRING_GET_BYTELEN(h))) {
		DUK_HSTRING_SET_ASCII(h);
	}
	return res;
}
#else /* DUK_USE_HSTRING_CLEN */
DUK_LOCAL duk_size_t duk__hstring_get_charlen_slowpath(duk_hstring *h) {
	if (DUK_LIKELY(DUK_HSTRING_HAS_ASCII(h))) {
		/* Most practical strings will go here. */
		return DUK_HSTRING_GET_BYTELEN(h);
	} else {
		/* ASCII flag is lazy, so set it here. */
		duk_size_t res;

		/* XXX: here we could use the strcache to speed up the
		 * computation (matters for 'i < str.length' loops).
		 */

		res = duk_unicode_unvalidated_utf8_length(DUK_HSTRING_GET_DATA(h), DUK_HSTRING_GET_BYTELEN(h));

#if defined(DUK_USE_ROM_STRINGS)
		if (DUK_HEAPHDR_HAS_READONLY((duk_heaphdr *) h)) {
			/* For ROM strings, can't write anything; ASCII flag
			 * is preset so we don't need to update it.
			 */
			return res;
		}
#endif
		if (DUK_LIKELY(res == DUK_HSTRING_GET_BYTELEN(h))) {
			DUK_HSTRING_SET_ASCII(h);
		}
		return res;
	}
}
#endif /* DUK_USE_HSTRING_CLEN */

#if defined(DUK_USE_HSTRING_CLEN)
DUK_INTERNAL DUK_HOT duk_size_t duk_hstring_get_charlen(duk_hstring *h) {
#if defined(DUK_USE_STRLEN16)
	if (DUK_LIKELY(h->clen16 != 0)) {
		return h->clen16;
	}
#else
	if (DUK_LIKELY(h->clen != 0)) {
		return h->clen;
	}
#endif
	return duk__hstring_get_charlen_slowpath(h);
}
#else /* DUK_USE_HSTRING_CLEN */
DUK_INTERNAL DUK_HOT duk_size_t duk_hstring_get_charlen(duk_hstring *h) {
	/* Always use slow path. */
	return duk__hstring_get_charlen_slowpath(h);
}
#endif /* DUK_USE_HSTRING_CLEN */
#endif /* DUK_USE_HSTRING_LAZY_CLEN */

/*
 *  Compare duk_hstring to an ASCII cstring.
 */

DUK_INTERNAL duk_bool_t duk_hstring_equals_ascii_cstring(duk_hstring *h, const char *cstr) {
	duk_size_t len;

	DUK_ASSERT(h != NULL);
	DUK_ASSERT(cstr != NULL);

	len = DUK_STRLEN(cstr);
	if (len != DUK_HSTRING_GET_BYTELEN(h)) {
		return 0;
	}
	if (duk_memcmp((const void *) cstr, (const void *) DUK_HSTRING_GET_DATA(h), len) == 0) {
		return 1;
	}
	return 0;
}
