/*
 *  Hash function duk_util_hashbytes().
 *
 *  Currently, 32-bit MurmurHash2.
 *
 *  Don't rely on specific hash values; hash function may be endianness
 *  dependent, for instance.
 */

#include "duk_internal.h"

#if defined(DUK_USE_STRHASH_DENSE)
/* 'magic' constants for Murmurhash2 */
#define DUK__MAGIC_M ((duk_uint32_t) 0x5bd1e995UL)
#define DUK__MAGIC_R 24

DUK_INTERNAL duk_uint32_t duk_util_hashbytes(const duk_uint8_t *data, duk_size_t len, duk_uint32_t seed) {
	duk_uint32_t h = seed ^ ((duk_uint32_t) len);

	while (len >= 4) {
		/* Portability workaround is required for platforms without
		 * unaligned access.  The replacement code emulates little
		 * endian access even on big endian architectures, which is
		 * OK as long as it is consistent for a build.
		 */
#if defined(DUK_USE_HASHBYTES_UNALIGNED_U32_ACCESS)
		duk_uint32_t k = *((const duk_uint32_t *) (const void *) data);
#else
		duk_uint32_t k = ((duk_uint32_t) data[0]) | (((duk_uint32_t) data[1]) << 8) | (((duk_uint32_t) data[2]) << 16) |
		                 (((duk_uint32_t) data[3]) << 24);
#endif

		k *= DUK__MAGIC_M;
		k ^= k >> DUK__MAGIC_R;
		k *= DUK__MAGIC_M;
		h *= DUK__MAGIC_M;
		h ^= k;
		data += 4;
		len -= 4;
	}

	switch (len) {
	case 3:
		h ^= data[2] << 16;
	case 2:
		h ^= data[1] << 8;
	case 1:
		h ^= data[0];
		h *= DUK__MAGIC_M;
	}

	h ^= h >> 13;
	h *= DUK__MAGIC_M;
	h ^= h >> 15;

	return h;
}
#endif /* DUK_USE_STRHASH_DENSE */
