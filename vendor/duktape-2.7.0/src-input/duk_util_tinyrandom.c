/*
 *  A tiny random number generator used for Math.random() and other internals.
 *
 *  Default algorithm is xoroshiro128+: http://xoroshiro.di.unimi.it/xoroshiro128plus.c
 *  with SplitMix64 seed preparation: http://xorshift.di.unimi.it/splitmix64.c.
 *
 *  Low memory targets and targets without 64-bit types use a slightly smaller
 *  (but slower) algorithm by Adi Shamir:
 *  http://www.woodmann.com/forum/archive/index.php/t-3100.html.
 *
 */

#include "duk_internal.h"

#if !defined(DUK_USE_GET_RANDOM_DOUBLE)

#if defined(DUK_USE_PREFER_SIZE) || !defined(DUK_USE_64BIT_OPS)
#define DUK__RANDOM_SHAMIR3OP
#else
#define DUK__RANDOM_XOROSHIRO128PLUS
#endif

#if defined(DUK__RANDOM_SHAMIR3OP)
#define DUK__UPDATE_RND(rnd) \
	do { \
		(rnd) += ((rnd) * (rnd)) | 0x05UL; \
		(rnd) = ((rnd) &0xffffffffUL); /* if duk_uint32_t is exactly 32 bits, this is a NOP */ \
	} while (0)

#define DUK__RND_BIT(rnd) ((rnd) >> 31) /* only use the highest bit */

DUK_INTERNAL void duk_util_tinyrandom_prepare_seed(duk_hthread *thr) {
	DUK_UNREF(thr); /* Nothing now. */
}

DUK_INTERNAL duk_double_t duk_util_tinyrandom_get_double(duk_hthread *thr) {
	duk_double_t t;
	duk_small_int_t n;
	duk_uint32_t rnd;

	rnd = thr->heap->rnd_state;

	n = 53; /* enough to cover the whole mantissa */
	t = 0.0;

	do {
		DUK__UPDATE_RND(rnd);
		t += DUK__RND_BIT(rnd);
		t /= 2.0;
	} while (--n);

	thr->heap->rnd_state = rnd;

	DUK_ASSERT(t >= (duk_double_t) 0.0);
	DUK_ASSERT(t < (duk_double_t) 1.0);

	return t;
}
#endif /* DUK__RANDOM_SHAMIR3OP */

#if defined(DUK__RANDOM_XOROSHIRO128PLUS)
DUK_LOCAL DUK_ALWAYS_INLINE duk_uint64_t duk__rnd_splitmix64(duk_uint64_t *x) {
	duk_uint64_t z;
	z = (*x += DUK_U64_CONSTANT(0x9E3779B97F4A7C15));
	z = (z ^ (z >> 30U)) * DUK_U64_CONSTANT(0xBF58476D1CE4E5B9);
	z = (z ^ (z >> 27U)) * DUK_U64_CONSTANT(0x94D049BB133111EB);
	return z ^ (z >> 31U);
}

DUK_LOCAL DUK_ALWAYS_INLINE duk_uint64_t duk__rnd_rotl(const duk_uint64_t x, duk_small_uint_t k) {
	return (x << k) | (x >> (64U - k));
}

DUK_LOCAL DUK_ALWAYS_INLINE duk_uint64_t duk__xoroshiro128plus(duk_uint64_t *s) {
	duk_uint64_t s0;
	duk_uint64_t s1;
	duk_uint64_t res;

	s0 = s[0];
	s1 = s[1];
	res = s0 + s1;
	s1 ^= s0;
	s[0] = duk__rnd_rotl(s0, 55) ^ s1 ^ (s1 << 14U);
	s[1] = duk__rnd_rotl(s1, 36);

	return res;
}

DUK_INTERNAL void duk_util_tinyrandom_prepare_seed(duk_hthread *thr) {
	duk_small_uint_t i;
	duk_uint64_t x;

	/* Mix both halves of the initial seed with SplitMix64.  The intent
	 * is to ensure that very similar raw seeds (which is usually the case
	 * because current seed is Date.now()) result in different xoroshiro128+
	 * seeds.
	 */
	x = thr->heap->rnd_state[0]; /* Only [0] is used as input here. */
	for (i = 0; i < 64; i++) {
		thr->heap->rnd_state[i & 0x01] = duk__rnd_splitmix64(&x); /* Keep last 2 values. */
	}
}

DUK_INTERNAL duk_double_t duk_util_tinyrandom_get_double(duk_hthread *thr) {
	duk_uint64_t v;
	duk_double_union du;

	/* For big and little endian the integer and IEEE double byte order
	 * is the same so a direct assignment works.  For mixed endian the
	 * 32-bit parts must be swapped.
	 */
	v = (DUK_U64_CONSTANT(0x3ff) << 52U) | (duk__xoroshiro128plus((duk_uint64_t *) thr->heap->rnd_state) >> 12U);
	du.ull[0] = v;
#if defined(DUK_USE_DOUBLE_ME)
	do {
		duk_uint32_t tmp;
		tmp = du.ui[0];
		du.ui[0] = du.ui[1];
		du.ui[1] = tmp;
	} while (0);
#endif
	return du.d - 1.0;
}
#endif /* DUK__RANDOM_XOROSHIRO128PLUS */

#endif /* !DUK_USE_GET_RANDOM_DOUBLE */
