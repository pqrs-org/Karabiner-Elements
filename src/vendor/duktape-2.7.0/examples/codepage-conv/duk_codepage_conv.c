/*
 *  Convert an 8-bit input string (e.g. ISO-8859-1) into CESU-8.
 *  Calling code supplies the "code page" as a 256-entry array of
 *  codepoints for the conversion.
 *
 *  This is useful when input data is in non-UTF-8 format and must
 *  be converted at runtime, e.g. when compiling non-UTF-8 source
 *  code.  Another alternative is to use e.g. iconv.
 */

#include "duktape.h"

/* Decode an 8-bit string using 'codepage' into Unicode codepoints and
 * re-encode into CESU-8.  Codepage argument must point to a 256-entry
 * table.  Only supports BMP (codepoints U+0000 to U+FFFF).
 */
void duk_decode_string_codepage(duk_context *ctx, const char *str, size_t len, unsigned int *codepage) {
	unsigned char *tmp;
	size_t tmplen, i;
	unsigned char *p;
	unsigned int cp;

	tmplen = 3 * len;  /* max expansion is 1 input byte -> 3 output bytes */
	if (tmplen / 3 != len) {
		/* Temporary buffer length wraps. */
		(void) duk_error(ctx, DUK_ERR_RANGE_ERROR, "input string too long");
		return;
	}

	tmp = (unsigned char *) duk_push_fixed_buffer(ctx, tmplen);

	for (i = 0, p = tmp; i < len; i++) {
		cp = codepage[((unsigned char *) str)[i]] & 0xffffUL;
		if (cp < 0x80UL) {
			*p++ = (unsigned char) cp;
		} else if (cp < 0x800UL) {
			*p++ = (unsigned char) (0xc0 + ((cp >> 6) & 0x1f));
			*p++ = (unsigned char) (0x80 + (cp & 0x3f));
		} else {
			/* In CESU-8 all codepoints in [0x0000,0xFFFF] are
			 * allowed, including surrogates.
			 */
			*p++ = (unsigned char) (0xe0 + ((cp >> 12) & 0x0f));
			*p++ = (unsigned char) (0x80 + ((cp >> 6) & 0x3f));
			*p++ = (unsigned char) (0x80 + (cp & 0x3f));
		}
	}

	duk_push_lstring(ctx, (const char *) tmp, (duk_size_t) (p - tmp));

	/* [ ... tmp res ] */

	duk_remove(ctx, -2);
}
