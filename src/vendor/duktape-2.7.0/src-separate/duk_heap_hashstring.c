/*
 *  String hash computation (interning).
 *
 *  String hashing is performance critical because a string hash is computed
 *  for all new strings which are candidates to be added to the string table.
 *  However, strings actually added to the string table go through a codepoint
 *  length calculation which dominates performance because it goes through
 *  every byte of the input string (but only for strings added).
 *
 *  The string hash algorithm should be fast, but on the other hand provide
 *  good enough hashes to ensure both string table and object property table
 *  hash tables work reasonably well (i.e., there aren't too many collisions
 *  with real world inputs).  Unless the hash is cryptographic, it's always
 *  possible to craft inputs with maximal hash collisions.
 *
 *  NOTE: The hash algorithms must match tools/dukutil.py:duk_heap_hashstring()
 *  for ROM string support!
 */

#include "duk_internal.h"

#if defined(DUK_USE_STRHASH_DENSE)
/* Constants for duk_hashstring(). */
#define DUK__STRHASH_SHORTSTRING  4096L
#define DUK__STRHASH_MEDIUMSTRING (256L * 1024L)
#define DUK__STRHASH_BLOCKSIZE    256L

DUK_INTERNAL duk_uint32_t duk_heap_hashstring(duk_heap *heap, const duk_uint8_t *str, duk_size_t len) {
	duk_uint32_t hash;

	/* Use Murmurhash2 directly for short strings, and use "block skipping"
	 * for long strings: hash an initial part and then sample the rest of
	 * the string with reasonably sized chunks.  An initial offset for the
	 * sampling is computed based on a hash of the initial part of the string;
	 * this is done to (usually) avoid the case where all long strings have
	 * certain offset ranges which are never sampled.
	 *
	 * Skip should depend on length and bound the total time to roughly
	 * logarithmic.  With current values:
	 *
	 *   1M string => 256 * 241 = 61696 bytes (0.06M) of hashing
	 *   1G string => 256 * 16321 = 4178176 bytes (3.98M) of hashing
	 *
	 * XXX: It would be better to compute the skip offset more "smoothly"
	 * instead of having a few boundary values.
	 */

	/* note: mixing len into seed improves hashing when skipping */
	duk_uint32_t str_seed = heap->hash_seed ^ ((duk_uint32_t) len);

	if (len <= DUK__STRHASH_SHORTSTRING) {
		hash = duk_util_hashbytes(str, len, str_seed);
	} else {
		duk_size_t off;
		duk_size_t skip;

		if (len <= DUK__STRHASH_MEDIUMSTRING) {
			skip = (duk_size_t) (16 * DUK__STRHASH_BLOCKSIZE + DUK__STRHASH_BLOCKSIZE);
		} else {
			skip = (duk_size_t) (256 * DUK__STRHASH_BLOCKSIZE + DUK__STRHASH_BLOCKSIZE);
		}

		hash = duk_util_hashbytes(str, (duk_size_t) DUK__STRHASH_SHORTSTRING, str_seed);
		off = DUK__STRHASH_SHORTSTRING + (skip * (hash % 256)) / 256;

		/* XXX: inefficient loop */
		while (off < len) {
			duk_size_t left = len - off;
			duk_size_t now = (duk_size_t) (left > DUK__STRHASH_BLOCKSIZE ? DUK__STRHASH_BLOCKSIZE : left);
			hash ^= duk_util_hashbytes(str + off, now, str_seed);
			off += skip;
		}
	}

#if defined(DUK_USE_STRHASH16)
	/* Truncate to 16 bits here, so that a computed hash can be compared
	 * against a hash stored in a 16-bit field.
	 */
	hash &= 0x0000ffffUL;
#endif
	return hash;
}
#else /* DUK_USE_STRHASH_DENSE */
DUK_INTERNAL duk_uint32_t duk_heap_hashstring(duk_heap *heap, const duk_uint8_t *str, duk_size_t len) {
	duk_uint32_t hash;
	duk_size_t step;
	duk_size_t off;

	/* Slightly modified "Bernstein hash" from:
	 *
	 *     http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx
	 *
	 * Modifications: string skipping and reverse direction similar to
	 * Lua 5.1.5, and different hash initializer.
	 *
	 * The reverse direction ensures last byte it always included in the
	 * hash which is a good default as changing parts of the string are
	 * more often in the suffix than in the prefix.
	 */

	hash = heap->hash_seed ^ ((duk_uint32_t) len); /* Bernstein hash init value is normally 5381 */
	step = (len >> DUK_USE_STRHASH_SKIP_SHIFT) + 1;
	for (off = len; off >= step; off -= step) {
		DUK_ASSERT(off >= 1); /* off >= step, and step >= 1 */
		hash = (hash * 33) + str[off - 1];
	}

#if defined(DUK_USE_STRHASH16)
	/* Truncate to 16 bits here, so that a computed hash can be compared
	 * against a hash stored in a 16-bit field.
	 */
	hash &= 0x0000ffffUL;
#endif
	return hash;
}
#endif /* DUK_USE_STRHASH_DENSE */
