/*
 *  Number-to-string and string-to-number conversions.
 *
 *  Slow path number-to-string and string-to-number conversion is based on
 *  a Dragon4 variant, with fast paths for small integers.  Big integer
 *  arithmetic is needed for guaranteeing that the conversion is correct
 *  and uses a minimum number of digits.  The big number arithmetic has a
 *  fixed maximum size and does not require dynamic allocations.
 *
 *  See: doc/number-conversion.rst.
 */

#include "duk_internal.h"

#define DUK__IEEE_DOUBLE_EXP_BIAS 1023
#define DUK__IEEE_DOUBLE_EXP_MIN  (-1022) /* biased exp == 0 -> denormal, exp -1022 */

#define DUK__DIGITCHAR(x) duk_lc_digits[(x)]

/*
 *  Tables generated with util/gennumdigits.py.
 *
 *  duk__str2num_digits_for_radix indicates, for each radix, how many input
 *  digits should be considered significant for string-to-number conversion.
 *  The input is also padded to this many digits to give the Dragon4
 *  conversion enough (apparent) precision to work with.
 *
 *  duk__str2num_exp_limits indicates, for each radix, the radix-specific
 *  minimum/maximum exponent values (for a Dragon4 integer mantissa)
 *  below and above which the number is guaranteed to underflow to zero
 *  or overflow to Infinity.  This allows parsing to keep bigint values
 *  bounded.
 */

DUK_LOCAL const duk_uint8_t duk__str2num_digits_for_radix[] = {
	69, 44, 35, 30, 27, 25, 23, 22, 20, 20, /* 2 to 11 */
	20, 19, 19, 18, 18, 17, 17, 17, 16, 16, /* 12 to 21 */
	16, 16, 16, 15, 15, 15, 15, 15, 15, 14, /* 22 to 31 */
	14, 14, 14, 14, 14 /* 31 to 36 */
};

typedef struct {
	duk_int16_t upper;
	duk_int16_t lower;
} duk__exp_limits;

DUK_LOCAL const duk__exp_limits duk__str2num_exp_limits[] = {
	{ 957, -1147 }, { 605, -725 }, { 479, -575 }, { 414, -496 }, { 372, -446 }, { 342, -411 }, { 321, -384 },
	{ 304, -364 },  { 291, -346 }, { 279, -334 }, { 268, -323 }, { 260, -312 }, { 252, -304 }, { 247, -296 },
	{ 240, -289 },  { 236, -283 }, { 231, -278 }, { 227, -273 }, { 223, -267 }, { 220, -263 }, { 216, -260 },
	{ 213, -256 },  { 210, -253 }, { 208, -249 }, { 205, -246 }, { 203, -244 }, { 201, -241 }, { 198, -239 },
	{ 196, -237 },  { 195, -234 }, { 193, -232 }, { 191, -230 }, { 190, -228 }, { 188, -226 }, { 187, -225 },
};

/*
 *  Limited functionality bigint implementation.
 *
 *  Restricted to non-negative numbers with less than 32 * DUK__BI_MAX_PARTS bits,
 *  with the caller responsible for ensuring this is never exceeded.  No memory
 *  allocation (except stack) is needed for bigint computation.  Operations
 *  have been tailored for number conversion needs.
 *
 *  Argument order is "assignment order", i.e. target first, then arguments:
 *  x <- y * z  -->  duk__bi_mul(x, y, z);
 */

/* This upper value has been experimentally determined; debug build will check
 * bigint size with assertions.
 */
#define DUK__BI_MAX_PARTS 37 /* 37x32 = 1184 bits */

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 2)
#define DUK__BI_PRINT(name, x) duk__bi_print((name), (x))
#else
#define DUK__BI_PRINT(name, x)
#endif

/* Current size is about 152 bytes. */
typedef struct {
	duk_small_int_t n;
	duk_uint32_t v[DUK__BI_MAX_PARTS]; /* low to high */
} duk__bigint;

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 2)
DUK_LOCAL void duk__bi_print(const char *name, duk__bigint *x) {
	/* Overestimate required size; debug code so not critical to be tight. */
	char buf[DUK__BI_MAX_PARTS * 9 + 64];
	char *p = buf;
	duk_small_int_t i;

	/* No NUL term checks in this debug code. */
	p += DUK_SPRINTF(p, "%p n=%ld", (void *) x, (long) x->n);
	if (x->n == 0) {
		p += DUK_SPRINTF(p, " 0");
	}
	for (i = x->n - 1; i >= 0; i--) {
		p += DUK_SPRINTF(p, " %08lx", (unsigned long) x->v[i]);
	}

	DUK_DDD(DUK_DDDPRINT("%s: %s", (const char *) name, (const char *) buf));
}
#endif

#if defined(DUK_USE_ASSERTIONS)
DUK_LOCAL duk_small_int_t duk__bi_is_valid(duk__bigint *x) {
	return (duk_small_int_t) (((x->n >= 0) && (x->n <= DUK__BI_MAX_PARTS)) /* is valid size */ &&
	                          ((x->n == 0) || (x->v[x->n - 1] != 0)) /* is normalized */);
}
#endif

DUK_LOCAL void duk__bi_normalize(duk__bigint *x) {
	duk_small_int_t i;

	for (i = x->n - 1; i >= 0; i--) {
		if (x->v[i] != 0) {
			break;
		}
	}

	/* Note: if 'x' is zero, x->n becomes 0 here */
	x->n = i + 1;
	DUK_ASSERT(duk__bi_is_valid(x));
}

/* x <- y */
DUK_LOCAL void duk__bi_copy(duk__bigint *x, duk__bigint *y) {
	duk_small_int_t n;

	n = y->n;
	x->n = n;
	/* No need to special case n == 0. */
	duk_memcpy((void *) x->v, (const void *) y->v, (size_t) (sizeof(duk_uint32_t) * (size_t) n));
}

DUK_LOCAL void duk__bi_set_small(duk__bigint *x, duk_uint32_t v) {
	if (v == 0U) {
		x->n = 0;
	} else {
		x->n = 1;
		x->v[0] = v;
	}
	DUK_ASSERT(duk__bi_is_valid(x));
}

/* Return value: <0  <=>  x < y
 *                0  <=>  x == y
 *               >0  <=>  x > y
 */
DUK_LOCAL int duk__bi_compare(duk__bigint *x, duk__bigint *y) {
	duk_small_int_t i, nx, ny;
	duk_uint32_t tx, ty;

	DUK_ASSERT(duk__bi_is_valid(x));
	DUK_ASSERT(duk__bi_is_valid(y));

	nx = x->n;
	ny = y->n;
	if (nx > ny) {
		goto ret_gt;
	}
	if (nx < ny) {
		goto ret_lt;
	}
	for (i = nx - 1; i >= 0; i--) {
		tx = x->v[i];
		ty = y->v[i];

		if (tx > ty) {
			goto ret_gt;
		}
		if (tx < ty) {
			goto ret_lt;
		}
	}

	return 0;

ret_gt:
	return 1;

ret_lt:
	return -1;
}

/* x <- y + z */
#if defined(DUK_USE_64BIT_OPS)
DUK_LOCAL void duk__bi_add(duk__bigint *x, duk__bigint *y, duk__bigint *z) {
	duk_uint64_t tmp;
	duk_small_int_t i, ny, nz;

	DUK_ASSERT(duk__bi_is_valid(y));
	DUK_ASSERT(duk__bi_is_valid(z));

	if (z->n > y->n) {
		duk__bigint *t;
		t = y;
		y = z;
		z = t;
	}
	DUK_ASSERT(y->n >= z->n);

	ny = y->n;
	nz = z->n;
	tmp = 0U;
	for (i = 0; i < ny; i++) {
		DUK_ASSERT(i < DUK__BI_MAX_PARTS);
		tmp += y->v[i];
		if (i < nz) {
			tmp += z->v[i];
		}
		x->v[i] = (duk_uint32_t) (tmp & 0xffffffffUL);
		tmp = tmp >> 32;
	}
	if (tmp != 0U) {
		DUK_ASSERT(i < DUK__BI_MAX_PARTS);
		x->v[i++] = (duk_uint32_t) tmp;
	}
	x->n = i;
	DUK_ASSERT(x->n <= DUK__BI_MAX_PARTS);

	/* no need to normalize */
	DUK_ASSERT(duk__bi_is_valid(x));
}
#else /* DUK_USE_64BIT_OPS */
DUK_LOCAL void duk__bi_add(duk__bigint *x, duk__bigint *y, duk__bigint *z) {
	duk_uint32_t carry, tmp1, tmp2;
	duk_small_int_t i, ny, nz;

	DUK_ASSERT(duk__bi_is_valid(y));
	DUK_ASSERT(duk__bi_is_valid(z));

	if (z->n > y->n) {
		duk__bigint *t;
		t = y;
		y = z;
		z = t;
	}
	DUK_ASSERT(y->n >= z->n);

	ny = y->n;
	nz = z->n;
	carry = 0U;
	for (i = 0; i < ny; i++) {
		/* Carry is detected based on wrapping which relies on exact 32-bit
		 * types.
		 */
		DUK_ASSERT(i < DUK__BI_MAX_PARTS);
		tmp1 = y->v[i];
		tmp2 = tmp1;
		if (i < nz) {
			tmp2 += z->v[i];
		}

		/* Careful with carry condition:
		 *  - If carry not added: 0x12345678 + 0 + 0xffffffff = 0x12345677 (< 0x12345678)
		 *  - If carry added:     0x12345678 + 1 + 0xffffffff = 0x12345678 (== 0x12345678)
		 */
		if (carry) {
			tmp2++;
			carry = (tmp2 <= tmp1 ? 1U : 0U);
		} else {
			carry = (tmp2 < tmp1 ? 1U : 0U);
		}

		x->v[i] = tmp2;
	}
	if (carry) {
		DUK_ASSERT(i < DUK__BI_MAX_PARTS);
		DUK_ASSERT(carry == 1U);
		x->v[i++] = carry;
	}
	x->n = i;
	DUK_ASSERT(x->n <= DUK__BI_MAX_PARTS);

	/* no need to normalize */
	DUK_ASSERT(duk__bi_is_valid(x));
}
#endif /* DUK_USE_64BIT_OPS */

/* x <- y + z */
DUK_LOCAL void duk__bi_add_small(duk__bigint *x, duk__bigint *y, duk_uint32_t z) {
	duk__bigint tmp;

	DUK_ASSERT(duk__bi_is_valid(y));

	/* XXX: this could be optimized; there is only one call site now though */
	duk__bi_set_small(&tmp, z);
	duk__bi_add(x, y, &tmp);

	DUK_ASSERT(duk__bi_is_valid(x));
}

#if 0 /* unused */
/* x <- x + y, use t as temp */
DUK_LOCAL void duk__bi_add_copy(duk__bigint *x, duk__bigint *y, duk__bigint *t) {
	duk__bi_add(t, x, y);
	duk__bi_copy(x, t);
}
#endif

/* x <- y - z, require x >= y => z >= 0, i.e. y >= z */
#if defined(DUK_USE_64BIT_OPS)
DUK_LOCAL void duk__bi_sub(duk__bigint *x, duk__bigint *y, duk__bigint *z) {
	duk_small_int_t i, ny, nz;
	duk_uint32_t ty, tz;
	duk_int64_t tmp;

	DUK_ASSERT(duk__bi_is_valid(y));
	DUK_ASSERT(duk__bi_is_valid(z));
	DUK_ASSERT(duk__bi_compare(y, z) >= 0);
	DUK_ASSERT(y->n >= z->n);

	ny = y->n;
	nz = z->n;
	tmp = 0;
	for (i = 0; i < ny; i++) {
		ty = y->v[i];
		if (i < nz) {
			tz = z->v[i];
		} else {
			tz = 0;
		}
		tmp = (duk_int64_t) ty - (duk_int64_t) tz + tmp;
		x->v[i] = (duk_uint32_t) ((duk_uint64_t) tmp & 0xffffffffUL);
		tmp = tmp >> 32; /* 0 or -1 */
	}
	DUK_ASSERT(tmp == 0);

	x->n = i;
	duk__bi_normalize(x); /* need to normalize, may even cancel to 0 */
	DUK_ASSERT(duk__bi_is_valid(x));
}
#else
DUK_LOCAL void duk__bi_sub(duk__bigint *x, duk__bigint *y, duk__bigint *z) {
	duk_small_int_t i, ny, nz;
	duk_uint32_t tmp1, tmp2, borrow;

	DUK_ASSERT(duk__bi_is_valid(y));
	DUK_ASSERT(duk__bi_is_valid(z));
	DUK_ASSERT(duk__bi_compare(y, z) >= 0);
	DUK_ASSERT(y->n >= z->n);

	ny = y->n;
	nz = z->n;
	borrow = 0U;
	for (i = 0; i < ny; i++) {
		/* Borrow is detected based on wrapping which relies on exact 32-bit
		 * types.
		 */
		tmp1 = y->v[i];
		tmp2 = tmp1;
		if (i < nz) {
			tmp2 -= z->v[i];
		}

		/* Careful with borrow condition:
		 *  - If borrow not subtracted: 0x12345678 - 0 - 0xffffffff = 0x12345679 (> 0x12345678)
		 *  - If borrow subtracted:     0x12345678 - 1 - 0xffffffff = 0x12345678 (== 0x12345678)
		 */
		if (borrow) {
			tmp2--;
			borrow = (tmp2 >= tmp1 ? 1U : 0U);
		} else {
			borrow = (tmp2 > tmp1 ? 1U : 0U);
		}

		x->v[i] = tmp2;
	}
	DUK_ASSERT(borrow == 0U);

	x->n = i;
	duk__bi_normalize(x); /* need to normalize, may even cancel to 0 */
	DUK_ASSERT(duk__bi_is_valid(x));
}
#endif

#if 0 /* unused */
/* x <- y - z */
DUK_LOCAL void duk__bi_sub_small(duk__bigint *x, duk__bigint *y, duk_uint32_t z) {
	duk__bigint tmp;

	DUK_ASSERT(duk__bi_is_valid(y));

	/* XXX: this could be optimized */
	duk__bi_set_small(&tmp, z);
	duk__bi_sub(x, y, &tmp);

	DUK_ASSERT(duk__bi_is_valid(x));
}
#endif

/* x <- x - y, use t as temp */
DUK_LOCAL void duk__bi_sub_copy(duk__bigint *x, duk__bigint *y, duk__bigint *t) {
	duk__bi_sub(t, x, y);
	duk__bi_copy(x, t);
}

/* x <- y * z */
DUK_LOCAL void duk__bi_mul(duk__bigint *x, duk__bigint *y, duk__bigint *z) {
	duk_small_int_t i, j, nx, nz;

	DUK_ASSERT(duk__bi_is_valid(y));
	DUK_ASSERT(duk__bi_is_valid(z));

	nx = y->n + z->n; /* max possible */
	DUK_ASSERT(nx <= DUK__BI_MAX_PARTS);

	if (nx == 0) {
		/* Both inputs are zero; cases where only one is zero can go
		 * through main algorithm.
		 */
		x->n = 0;
		return;
	}

	duk_memzero((void *) x->v, (size_t) (sizeof(duk_uint32_t) * (size_t) nx));
	x->n = nx;

	nz = z->n;
	for (i = 0; i < y->n; i++) {
#if defined(DUK_USE_64BIT_OPS)
		duk_uint64_t tmp = 0U;
		for (j = 0; j < nz; j++) {
			tmp += (duk_uint64_t) y->v[i] * (duk_uint64_t) z->v[j] + x->v[i + j];
			x->v[i + j] = (duk_uint32_t) (tmp & 0xffffffffUL);
			tmp = tmp >> 32;
		}
		if (tmp > 0) {
			DUK_ASSERT(i + j < nx);
			DUK_ASSERT(i + j < DUK__BI_MAX_PARTS);
			DUK_ASSERT(x->v[i + j] == 0U);
			x->v[i + j] = (duk_uint32_t) tmp;
		}
#else
		/*
		 *  Multiply + add + carry for 32-bit components using only 16x16->32
		 *  multiplies and carry detection based on unsigned overflow.
		 *
		 *    1st mult, 32-bit: (A*2^16 + B)
		 *    2nd mult, 32-bit: (C*2^16 + D)
		 *    3rd add, 32-bit: E
		 *    4th add, 32-bit: F
		 *
		 *      (AC*2^16 + B) * (C*2^16 + D) + E + F
		 *    = AC*2^32 + AD*2^16 + BC*2^16 + BD + E + F
		 *    = AC*2^32 + (AD + BC)*2^16 + (BD + E + F)
		 *    = AC*2^32 + AD*2^16 + BC*2^16 + (BD + E + F)
		 */
		duk_uint32_t a, b, c, d, e, f;
		duk_uint32_t r, s, t;

		a = y->v[i];
		b = a & 0xffffUL;
		a = a >> 16;

		f = 0;
		for (j = 0; j < nz; j++) {
			c = z->v[j];
			d = c & 0xffffUL;
			c = c >> 16;
			e = x->v[i + j];

			/* build result as: (r << 32) + s: start with (BD + E + F) */
			r = 0;
			s = b * d;

			/* add E */
			t = s + e;
			if (t < s) {
				r++;
			} /* carry */
			s = t;

			/* add F */
			t = s + f;
			if (t < s) {
				r++;
			} /* carry */
			s = t;

			/* add BC*2^16 */
			t = b * c;
			r += (t >> 16);
			t = s + ((t & 0xffffUL) << 16);
			if (t < s) {
				r++;
			} /* carry */
			s = t;

			/* add AD*2^16 */
			t = a * d;
			r += (t >> 16);
			t = s + ((t & 0xffffUL) << 16);
			if (t < s) {
				r++;
			} /* carry */
			s = t;

			/* add AC*2^32 */
			t = a * c;
			r += t;

			DUK_DDD(DUK_DDDPRINT("ab=%08lx cd=%08lx ef=%08lx -> rs=%08lx %08lx",
			                     (unsigned long) y->v[i],
			                     (unsigned long) z->v[j],
			                     (unsigned long) x->v[i + j],
			                     (unsigned long) r,
			                     (unsigned long) s));

			x->v[i + j] = s;
			f = r;
		}
		if (f > 0U) {
			DUK_ASSERT(i + j < nx);
			DUK_ASSERT(i + j < DUK__BI_MAX_PARTS);
			DUK_ASSERT(x->v[i + j] == 0U);
			x->v[i + j] = (duk_uint32_t) f;
		}
#endif /* DUK_USE_64BIT_OPS */
	}

	duk__bi_normalize(x);
	DUK_ASSERT(duk__bi_is_valid(x));
}

/* x <- y * z */
DUK_LOCAL void duk__bi_mul_small(duk__bigint *x, duk__bigint *y, duk_uint32_t z) {
	duk__bigint tmp;

	DUK_ASSERT(duk__bi_is_valid(y));

	/* XXX: this could be optimized */
	duk__bi_set_small(&tmp, z);
	duk__bi_mul(x, y, &tmp);

	DUK_ASSERT(duk__bi_is_valid(x));
}

/* x <- x * y, use t as temp */
DUK_LOCAL void duk__bi_mul_copy(duk__bigint *x, duk__bigint *y, duk__bigint *t) {
	duk__bi_mul(t, x, y);
	duk__bi_copy(x, t);
}

/* x <- x * y, use t as temp */
DUK_LOCAL void duk__bi_mul_small_copy(duk__bigint *x, duk_uint32_t y, duk__bigint *t) {
	duk__bi_mul_small(t, x, y);
	duk__bi_copy(x, t);
}

DUK_LOCAL int duk__bi_is_even(duk__bigint *x) {
	DUK_ASSERT(duk__bi_is_valid(x));
	return (x->n == 0) || ((x->v[0] & 0x01) == 0);
}

DUK_LOCAL int duk__bi_is_zero(duk__bigint *x) {
	DUK_ASSERT(duk__bi_is_valid(x));
	return (x->n == 0); /* this is the case for normalized numbers */
}

/* Bigint is 2^52.  Used to detect normalized IEEE double mantissa values
 * which are at the lowest edge (next floating point value downwards has
 * a different exponent).  The lowest mantissa has the form:
 *
 *     1000........000    (52 zeroes; only "hidden bit" is set)
 */
DUK_LOCAL duk_small_int_t duk__bi_is_2to52(duk__bigint *x) {
	DUK_ASSERT(duk__bi_is_valid(x));
	return (duk_small_int_t) (x->n == 2) && (x->v[0] == 0U) && (x->v[1] == (1U << (52 - 32)));
}

/* x <- (1<<y) */
DUK_LOCAL void duk__bi_twoexp(duk__bigint *x, duk_small_int_t y) {
	duk_small_int_t n, r;

	n = (y / 32) + 1;
	DUK_ASSERT(n > 0);
	r = y % 32;
	duk_memzero((void *) x->v, sizeof(duk_uint32_t) * (size_t) n);
	x->n = n;
	x->v[n - 1] = (((duk_uint32_t) 1) << r);
}

/* x <- b^y; use t1 and t2 as temps */
DUK_LOCAL void duk__bi_exp_small(duk__bigint *x, duk_small_int_t b, duk_small_int_t y, duk__bigint *t1, duk__bigint *t2) {
	/* Fast path the binary case */

	DUK_ASSERT(x != t1 && x != t2 && t1 != t2); /* distinct bignums, easy mistake to make */
	DUK_ASSERT(b >= 0);
	DUK_ASSERT(y >= 0);

	if (b == 2) {
		duk__bi_twoexp(x, y);
		return;
	}

	/* http://en.wikipedia.org/wiki/Exponentiation_by_squaring */

	DUK_DDD(DUK_DDDPRINT("exp_small: b=%ld, y=%ld", (long) b, (long) y));

	duk__bi_set_small(x, 1);
	duk__bi_set_small(t1, (duk_uint32_t) b);
	for (;;) {
		/* Loop structure ensures that we don't compute t1^2 unnecessarily
		 * on the final round, as that might create a bignum exceeding the
		 * current DUK__BI_MAX_PARTS limit.
		 */
		if (y & 0x01) {
			duk__bi_mul_copy(x, t1, t2);
		}
		y = y >> 1;
		if (y == 0) {
			break;
		}
		duk__bi_mul_copy(t1, t1, t2);
	}

	DUK__BI_PRINT("exp_small result", x);
}

/*
 *  A Dragon4 number-to-string variant, based on:
 *
 *    Guy L. Steele Jr., Jon L. White: "How to Print Floating-Point Numbers
 *    Accurately"
 *
 *    Robert G. Burger, R. Kent Dybvig: "Printing Floating-Point Numbers
 *    Quickly and Accurately"
 *
 *  The current algorithm is based on Figure 1 of the Burger-Dybvig paper,
 *  i.e. the base implementation without logarithm estimation speedups
 *  (these would increase code footprint considerably).  Fixed-format output
 *  does not follow the suggestions in the paper; instead, we generate an
 *  extra digit and round-with-carry.
 *
 *  The same algorithm is used for number parsing (with b=10 and B=2)
 *  by generating one extra digit and doing rounding manually.
 *
 *  See doc/number-conversion.rst for limitations.
 */

/* Maximum number of digits generated. */
#define DUK__MAX_OUTPUT_DIGITS 1040 /* (Number.MAX_VALUE).toString(2).length == 1024, + slack */

/* Maximum number of characters in formatted value. */
#define DUK__MAX_FORMATTED_LENGTH 1040 /* (-Number.MAX_VALUE).toString(2).length == 1025, + slack */

/* Number and (minimum) size of bigints in the nc_ctx structure. */
#define DUK__NUMCONV_CTX_NUM_BIGINTS  7
#define DUK__NUMCONV_CTX_BIGINTS_SIZE (sizeof(duk__bigint) * DUK__NUMCONV_CTX_NUM_BIGINTS)

typedef struct {
	/* Currently about 7*152 = 1064 bytes.  The space for these
	 * duk__bigints is used also as a temporary buffer for generating
	 * the final string.  This is a bit awkard; a union would be
	 * more correct.
	 */
	duk__bigint f, r, s, mp, mm, t1, t2;

	duk_small_int_t is_s2n; /* if 1, doing a string-to-number; else doing a number-to-string */
	duk_small_int_t is_fixed; /* if 1, doing a fixed format output (not free format) */
	duk_small_int_t req_digits; /* requested number of output digits; 0 = free-format */
	duk_small_int_t abs_pos; /* digit position is absolute, not relative */
	duk_small_int_t e; /* exponent for 'f' */
	duk_small_int_t b; /* input radix */
	duk_small_int_t B; /* output radix */
	duk_small_int_t k; /* see algorithm */
	duk_small_int_t low_ok; /* see algorithm */
	duk_small_int_t high_ok; /* see algorithm */
	duk_small_int_t unequal_gaps; /* m+ != m- (very rarely) */

	/* Buffer used for generated digits, values are in the range [0,B-1]. */
	duk_uint8_t digits[DUK__MAX_OUTPUT_DIGITS];
	duk_small_int_t count; /* digit count */
} duk__numconv_stringify_ctx;

/* Note: computes with 'idx' in assertions, so caller beware.
 * 'idx' is preincremented, i.e. '1' on first call, because it
 * is more convenient for the caller.
 */
#define DUK__DRAGON4_OUTPUT_PREINC(nc_ctx, preinc_idx, x) \
	do { \
		DUK_ASSERT((preinc_idx) -1 >= 0); \
		DUK_ASSERT((preinc_idx) -1 < DUK__MAX_OUTPUT_DIGITS); \
		((nc_ctx)->digits[(preinc_idx) -1]) = (duk_uint8_t) (x); \
	} while (0)

DUK_LOCAL duk_size_t duk__dragon4_format_uint32(duk_uint8_t *buf, duk_uint32_t x, duk_small_int_t radix) {
	duk_uint8_t *p;
	duk_size_t len;
	duk_small_int_t dig;
	duk_uint32_t t;

	DUK_ASSERT(buf != NULL);
	DUK_ASSERT(radix >= 2 && radix <= 36);

	/* A 32-bit unsigned integer formats to at most 32 digits (the
	 * worst case happens with radix == 2).  Output the digits backwards,
	 * and use a memmove() to get them in the right place.
	 */

	p = buf + 32;
	for (;;) {
		t = x / (duk_uint32_t) radix;
		dig = (duk_small_int_t) (x - t * (duk_uint32_t) radix);
		x = t;

		DUK_ASSERT(dig >= 0 && dig < 36);
		*(--p) = DUK__DIGITCHAR(dig);

		if (x == 0) {
			break;
		}
	}
	len = (duk_size_t) ((buf + 32) - p);

	duk_memmove((void *) buf, (const void *) p, (size_t) len);

	return len;
}

DUK_LOCAL void duk__dragon4_prepare(duk__numconv_stringify_ctx *nc_ctx) {
	duk_small_int_t lowest_mantissa;

#if 1
	/* Assume IEEE round-to-even, so that shorter encoding can be used
	 * when round-to-even would produce correct result.  By removing
	 * this check (and having low_ok == high_ok == 0) the results would
	 * still be accurate but in some cases longer than necessary.
	 */
	if (duk__bi_is_even(&nc_ctx->f)) {
		DUK_DDD(DUK_DDDPRINT("f is even"));
		nc_ctx->low_ok = 1;
		nc_ctx->high_ok = 1;
	} else {
		DUK_DDD(DUK_DDDPRINT("f is odd"));
		nc_ctx->low_ok = 0;
		nc_ctx->high_ok = 0;
	}
#else
	/* Note: not honoring round-to-even should work but now generates incorrect
	 * results.  For instance, 1e23 serializes to "a000...", i.e. the first digit
	 * equals the radix (10).  Scaling stops one step too early in this case.
	 * Don't know why this is the case, but since this code path is unused, it
	 * doesn't matter.
	 */
	nc_ctx->low_ok = 0;
	nc_ctx->high_ok = 0;
#endif

	/* For string-to-number, pretend we never have the lowest mantissa as there
	 * is no natural "precision" for inputs.  Having lowest_mantissa == 0, we'll
	 * fall into the base cases for both e >= 0 and e < 0.
	 */
	if (nc_ctx->is_s2n) {
		lowest_mantissa = 0;
	} else {
		lowest_mantissa = duk__bi_is_2to52(&nc_ctx->f);
	}

	nc_ctx->unequal_gaps = 0;
	if (nc_ctx->e >= 0) {
		/* exponent non-negative (and thus not minimum exponent) */

		if (lowest_mantissa) {
			/* (>= e 0) AND (= f (expt b (- p 1)))
			 *
			 * be <- (expt b e) == b^e
			 * be1 <- (* be b) == (expt b (+ e 1)) == b^(e+1)
			 * r <- (* f be1 2) == 2 * f * b^(e+1)    [if b==2 -> f * b^(e+2)]
			 * s <- (* b 2)                           [if b==2 -> 4]
			 * m+ <- be1 == b^(e+1)
			 * m- <- be == b^e
			 * k <- 0
			 * B <- B
			 * low_ok <- round
			 * high_ok <- round
			 */

			DUK_DDD(DUK_DDDPRINT("non-negative exponent (not smallest exponent); "
			                     "lowest mantissa value for this exponent -> "
			                     "unequal gaps"));

			duk__bi_exp_small(&nc_ctx->mm, nc_ctx->b, nc_ctx->e, &nc_ctx->t1, &nc_ctx->t2); /* mm <- b^e */
			duk__bi_mul_small(&nc_ctx->mp, &nc_ctx->mm, (duk_uint32_t) nc_ctx->b); /* mp <- b^(e+1) */
			duk__bi_mul_small(&nc_ctx->t1, &nc_ctx->f, 2);
			duk__bi_mul(&nc_ctx->r, &nc_ctx->t1, &nc_ctx->mp); /* r <- (2 * f) * b^(e+1) */
			duk__bi_set_small(&nc_ctx->s, (duk_uint32_t) (nc_ctx->b * 2)); /* s <- 2 * b */
			nc_ctx->unequal_gaps = 1;
		} else {
			/* (>= e 0) AND (not (= f (expt b (- p 1))))
			 *
			 * be <- (expt b e) == b^e
			 * r <- (* f be 2) == 2 * f * b^e    [if b==2 -> f * b^(e+1)]
			 * s <- 2
			 * m+ <- be == b^e
			 * m- <- be == b^e
			 * k <- 0
			 * B <- B
			 * low_ok <- round
			 * high_ok <- round
			 */

			DUK_DDD(DUK_DDDPRINT("non-negative exponent (not smallest exponent); "
			                     "not lowest mantissa for this exponent -> "
			                     "equal gaps"));

			duk__bi_exp_small(&nc_ctx->mm, nc_ctx->b, nc_ctx->e, &nc_ctx->t1, &nc_ctx->t2); /* mm <- b^e */
			duk__bi_copy(&nc_ctx->mp, &nc_ctx->mm); /* mp <- b^e */
			duk__bi_mul_small(&nc_ctx->t1, &nc_ctx->f, 2);
			duk__bi_mul(&nc_ctx->r, &nc_ctx->t1, &nc_ctx->mp); /* r <- (2 * f) * b^e */
			duk__bi_set_small(&nc_ctx->s, 2); /* s <- 2 */
		}
	} else {
		/* When doing string-to-number, lowest_mantissa is always 0 so
		 * the exponent check, while incorrect, won't matter.
		 */
		if (nc_ctx->e > DUK__IEEE_DOUBLE_EXP_MIN /*not minimum exponent*/ &&
		    lowest_mantissa /* lowest mantissa for this exponent*/) {
			/* r <- (* f b 2)                                [if b==2 -> (* f 4)]
			 * s <- (* (expt b (- 1 e)) 2) == b^(1-e) * 2    [if b==2 -> b^(2-e)]
			 * m+ <- b == 2
			 * m- <- 1
			 * k <- 0
			 * B <- B
			 * low_ok <- round
			 * high_ok <- round
			 */

			DUK_DDD(DUK_DDDPRINT("negative exponent; not minimum exponent and "
			                     "lowest mantissa for this exponent -> "
			                     "unequal gaps"));

			duk__bi_mul_small(&nc_ctx->r, &nc_ctx->f, (duk_uint32_t) (nc_ctx->b * 2)); /* r <- (2 * b) * f */
			duk__bi_exp_small(&nc_ctx->t1,
			                  nc_ctx->b,
			                  1 - nc_ctx->e,
			                  &nc_ctx->s,
			                  &nc_ctx->t2); /* NB: use 's' as temp on purpose */
			duk__bi_mul_small(&nc_ctx->s, &nc_ctx->t1, 2); /* s <- b^(1-e) * 2 */
			duk__bi_set_small(&nc_ctx->mp, 2);
			duk__bi_set_small(&nc_ctx->mm, 1);
			nc_ctx->unequal_gaps = 1;
		} else {
			/* r <- (* f 2)
			 * s <- (* (expt b (- e)) 2) == b^(-e) * 2    [if b==2 -> b^(1-e)]
			 * m+ <- 1
			 * m- <- 1
			 * k <- 0
			 * B <- B
			 * low_ok <- round
			 * high_ok <- round
			 */

			DUK_DDD(DUK_DDDPRINT("negative exponent; minimum exponent or not "
			                     "lowest mantissa for this exponent -> "
			                     "equal gaps"));

			duk__bi_mul_small(&nc_ctx->r, &nc_ctx->f, 2); /* r <- 2 * f */
			duk__bi_exp_small(&nc_ctx->t1,
			                  nc_ctx->b,
			                  -nc_ctx->e,
			                  &nc_ctx->s,
			                  &nc_ctx->t2); /* NB: use 's' as temp on purpose */
			duk__bi_mul_small(&nc_ctx->s, &nc_ctx->t1, 2); /* s <- b^(-e) * 2 */
			duk__bi_set_small(&nc_ctx->mp, 1);
			duk__bi_set_small(&nc_ctx->mm, 1);
		}
	}
}

DUK_LOCAL void duk__dragon4_scale(duk__numconv_stringify_ctx *nc_ctx) {
	duk_small_int_t k = 0;

	/* This is essentially the 'scale' algorithm, with recursion removed.
	 * Note that 'k' is either correct immediately, or will move in one
	 * direction in the loop.  There's no need to do the low/high checks
	 * on every round (like the Scheme algorithm does).
	 *
	 * The scheme algorithm finds 'k' and updates 's' simultaneously,
	 * while the logical algorithm finds 'k' with 's' having its initial
	 * value, after which 's' is updated separately (see the Burger-Dybvig
	 * paper, Section 3.1, steps 2 and 3).
	 *
	 * The case where m+ == m- (almost always) is optimized for, because
	 * it reduces the bigint operations considerably and almost always
	 * applies.  The scale loop only needs to work with m+, so this works.
	 */

	/* XXX: this algorithm could be optimized quite a lot by using e.g.
	 * a logarithm based estimator for 'k' and performing B^n multiplication
	 * using a lookup table or using some bit-representation based exp
	 * algorithm.  Currently we just loop, with significant performance
	 * impact for very large and very small numbers.
	 */

	DUK_DDD(
	    DUK_DDDPRINT("scale: B=%ld, low_ok=%ld, high_ok=%ld", (long) nc_ctx->B, (long) nc_ctx->low_ok, (long) nc_ctx->high_ok));
	DUK__BI_PRINT("r(init)", &nc_ctx->r);
	DUK__BI_PRINT("s(init)", &nc_ctx->s);
	DUK__BI_PRINT("mp(init)", &nc_ctx->mp);
	DUK__BI_PRINT("mm(init)", &nc_ctx->mm);

	for (;;) {
		DUK_DDD(DUK_DDDPRINT("scale loop (inc k), k=%ld", (long) k));
		DUK__BI_PRINT("r", &nc_ctx->r);
		DUK__BI_PRINT("s", &nc_ctx->s);
		DUK__BI_PRINT("m+", &nc_ctx->mp);
		DUK__BI_PRINT("m-", &nc_ctx->mm);

		duk__bi_add(&nc_ctx->t1, &nc_ctx->r, &nc_ctx->mp); /* t1 = (+ r m+) */
		if (duk__bi_compare(&nc_ctx->t1, &nc_ctx->s) >= (nc_ctx->high_ok ? 0 : 1)) {
			DUK_DDD(DUK_DDDPRINT("k is too low"));
			/* r <- r
			 * s <- (* s B)
			 * m+ <- m+
			 * m- <- m-
			 * k <- (+ k 1)
			 */

			duk__bi_mul_small_copy(&nc_ctx->s, (duk_uint32_t) nc_ctx->B, &nc_ctx->t1);
			k++;
		} else {
			break;
		}
	}

	/* k > 0 -> k was too low, and cannot be too high */
	if (k > 0) {
		goto skip_dec_k;
	}

	for (;;) {
		DUK_DDD(DUK_DDDPRINT("scale loop (dec k), k=%ld", (long) k));
		DUK__BI_PRINT("r", &nc_ctx->r);
		DUK__BI_PRINT("s", &nc_ctx->s);
		DUK__BI_PRINT("m+", &nc_ctx->mp);
		DUK__BI_PRINT("m-", &nc_ctx->mm);

		duk__bi_add(&nc_ctx->t1, &nc_ctx->r, &nc_ctx->mp); /* t1 = (+ r m+) */
		duk__bi_mul_small(&nc_ctx->t2, &nc_ctx->t1, (duk_uint32_t) nc_ctx->B); /* t2 = (* (+ r m+) B) */
		if (duk__bi_compare(&nc_ctx->t2, &nc_ctx->s) <= (nc_ctx->high_ok ? -1 : 0)) {
			DUK_DDD(DUK_DDDPRINT("k is too high"));
			/* r <- (* r B)
			 * s <- s
			 * m+ <- (* m+ B)
			 * m- <- (* m- B)
			 * k <- (- k 1)
			 */
			duk__bi_mul_small_copy(&nc_ctx->r, (duk_uint32_t) nc_ctx->B, &nc_ctx->t1);
			duk__bi_mul_small_copy(&nc_ctx->mp, (duk_uint32_t) nc_ctx->B, &nc_ctx->t1);
			if (nc_ctx->unequal_gaps) {
				DUK_DDD(DUK_DDDPRINT("m+ != m- -> need to update m- too"));
				duk__bi_mul_small_copy(&nc_ctx->mm, (duk_uint32_t) nc_ctx->B, &nc_ctx->t1);
			}
			k--;
		} else {
			break;
		}
	}

skip_dec_k:

	if (!nc_ctx->unequal_gaps) {
		DUK_DDD(DUK_DDDPRINT("equal gaps, copy m- from m+"));
		duk__bi_copy(&nc_ctx->mm, &nc_ctx->mp); /* mm <- mp */
	}
	nc_ctx->k = k;

	DUK_DDD(DUK_DDDPRINT("final k: %ld", (long) k));
	DUK__BI_PRINT("r(final)", &nc_ctx->r);
	DUK__BI_PRINT("s(final)", &nc_ctx->s);
	DUK__BI_PRINT("mp(final)", &nc_ctx->mp);
	DUK__BI_PRINT("mm(final)", &nc_ctx->mm);
}

DUK_LOCAL void duk__dragon4_generate(duk__numconv_stringify_ctx *nc_ctx) {
	duk_small_int_t tc1, tc2; /* terminating conditions */
	duk_small_int_t d; /* current digit */
	duk_small_int_t count = 0; /* digit count */

	/*
	 *  Digit generation loop.
	 *
	 *  Different termination conditions:
	 *
	 *    1. Free format output.  Terminate when shortest accurate
	 *       representation found.
	 *
	 *    2. Fixed format output, with specific number of digits.
	 *       Ignore termination conditions, terminate when digits
	 *       generated.  Caller requests an extra digit and rounds.
	 *
	 *    3. Fixed format output, with a specific absolute cut-off
	 *       position (e.g. 10 digits after decimal point).  Note
	 *       that we always generate at least one digit, even if
	 *       the digit is below the cut-off point already.
	 */

	for (;;) {
		DUK_DDD(DUK_DDDPRINT("generate loop, count=%ld, k=%ld, B=%ld, low_ok=%ld, high_ok=%ld",
		                     (long) count,
		                     (long) nc_ctx->k,
		                     (long) nc_ctx->B,
		                     (long) nc_ctx->low_ok,
		                     (long) nc_ctx->high_ok));
		DUK__BI_PRINT("r", &nc_ctx->r);
		DUK__BI_PRINT("s", &nc_ctx->s);
		DUK__BI_PRINT("m+", &nc_ctx->mp);
		DUK__BI_PRINT("m-", &nc_ctx->mm);

		/* (quotient-remainder (* r B) s) using a dummy subtraction loop */
		duk__bi_mul_small(&nc_ctx->t1, &nc_ctx->r, (duk_uint32_t) nc_ctx->B); /* t1 <- (* r B) */
		d = 0;
		for (;;) {
			if (duk__bi_compare(&nc_ctx->t1, &nc_ctx->s) < 0) {
				break;
			}
			duk__bi_sub_copy(&nc_ctx->t1, &nc_ctx->s, &nc_ctx->t2); /* t1 <- t1 - s */
			d++;
		}
		duk__bi_copy(&nc_ctx->r, &nc_ctx->t1); /* r <- (remainder (* r B) s) */
		/* d <- (quotient (* r B) s)   (in range 0...B-1) */
		DUK_DDD(DUK_DDDPRINT("-> d(quot)=%ld", (long) d));
		DUK__BI_PRINT("r(rem)", &nc_ctx->r);

		duk__bi_mul_small_copy(&nc_ctx->mp, (duk_uint32_t) nc_ctx->B, &nc_ctx->t2); /* m+ <- (* m+ B) */
		duk__bi_mul_small_copy(&nc_ctx->mm, (duk_uint32_t) nc_ctx->B, &nc_ctx->t2); /* m- <- (* m- B) */
		DUK__BI_PRINT("mp(upd)", &nc_ctx->mp);
		DUK__BI_PRINT("mm(upd)", &nc_ctx->mm);

		/* Terminating conditions.  For fixed width output, we just ignore the
		 * terminating conditions (and pretend that tc1 == tc2 == false).  The
		 * the current shortcut for fixed-format output is to generate a few
		 * extra digits and use rounding (with carry) to finish the output.
		 */

		if (nc_ctx->is_fixed == 0) {
			/* free-form */
			tc1 = (duk__bi_compare(&nc_ctx->r, &nc_ctx->mm) <= (nc_ctx->low_ok ? 0 : -1));

			duk__bi_add(&nc_ctx->t1, &nc_ctx->r, &nc_ctx->mp); /* t1 <- (+ r m+) */
			tc2 = (duk__bi_compare(&nc_ctx->t1, &nc_ctx->s) >= (nc_ctx->high_ok ? 0 : 1));

			DUK_DDD(DUK_DDDPRINT("tc1=%ld, tc2=%ld", (long) tc1, (long) tc2));
		} else {
			/* fixed-format */
			tc1 = 0;
			tc2 = 0;
		}

		/* Count is incremented before DUK__DRAGON4_OUTPUT_PREINC() call
		 * on purpose, which is taken into account by the macro.
		 */
		count++;

		if (tc1) {
			if (tc2) {
				/* tc1 = true, tc2 = true */
				duk__bi_mul_small(&nc_ctx->t1, &nc_ctx->r, 2);
				if (duk__bi_compare(&nc_ctx->t1, &nc_ctx->s) < 0) { /* (< (* r 2) s) */
					DUK_DDD(DUK_DDDPRINT("tc1=true, tc2=true, 2r > s: output d --> %ld (k=%ld)",
					                     (long) d,
					                     (long) nc_ctx->k));
					DUK__DRAGON4_OUTPUT_PREINC(nc_ctx, count, d);
				} else {
					DUK_DDD(DUK_DDDPRINT("tc1=true, tc2=true, 2r <= s: output d+1 --> %ld (k=%ld)",
					                     (long) (d + 1),
					                     (long) nc_ctx->k));
					DUK__DRAGON4_OUTPUT_PREINC(nc_ctx, count, d + 1);
				}
				break;
			} else {
				/* tc1 = true, tc2 = false */
				DUK_DDD(DUK_DDDPRINT("tc1=true, tc2=false: output d --> %ld (k=%ld)", (long) d, (long) nc_ctx->k));
				DUK__DRAGON4_OUTPUT_PREINC(nc_ctx, count, d);
				break;
			}
		} else {
			if (tc2) {
				/* tc1 = false, tc2 = true */
				DUK_DDD(DUK_DDDPRINT("tc1=false, tc2=true: output d+1 --> %ld (k=%ld)",
				                     (long) (d + 1),
				                     (long) nc_ctx->k));
				DUK__DRAGON4_OUTPUT_PREINC(nc_ctx, count, d + 1);
				break;
			} else {
				/* tc1 = false, tc2 = false */
				DUK_DDD(DUK_DDDPRINT("tc1=false, tc2=false: output d --> %ld (k=%ld)", (long) d, (long) nc_ctx->k));
				DUK__DRAGON4_OUTPUT_PREINC(nc_ctx, count, d);

				/* r <- r    (updated above: r <- (remainder (* r B) s)
				 * s <- s
				 * m+ <- m+  (updated above: m+ <- (* m+ B)
				 * m- <- m-  (updated above: m- <- (* m- B)
				 * B, low_ok, high_ok are fixed
				 */

				/* fall through and continue for-loop */
			}
		}

		/* fixed-format termination conditions */
		if (nc_ctx->is_fixed) {
			if (nc_ctx->abs_pos) {
				int pos = nc_ctx->k - count + 1; /* count is already incremented, take into account */
				DUK_DDD(DUK_DDDPRINT("fixed format, absolute: abs pos=%ld, k=%ld, count=%ld, req=%ld",
				                     (long) pos,
				                     (long) nc_ctx->k,
				                     (long) count,
				                     (long) nc_ctx->req_digits));
				if (pos <= nc_ctx->req_digits) {
					DUK_DDD(DUK_DDDPRINT("digit position reached req_digits, end generate loop"));
					break;
				}
			} else {
				DUK_DDD(DUK_DDDPRINT("fixed format, relative: k=%ld, count=%ld, req=%ld",
				                     (long) nc_ctx->k,
				                     (long) count,
				                     (long) nc_ctx->req_digits));
				if (count >= nc_ctx->req_digits) {
					DUK_DDD(DUK_DDDPRINT("digit count reached req_digits, end generate loop"));
					break;
				}
			}
		}
	} /* for */

	nc_ctx->count = count;

	DUK_DDD(DUK_DDDPRINT("generate finished"));

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 2)
	{
		duk_uint8_t buf[2048];
		duk_small_int_t i, t;
		duk_memzero(buf, sizeof(buf));
		for (i = 0; i < nc_ctx->count; i++) {
			t = nc_ctx->digits[i];
			if (t < 0 || t > 36) {
				buf[i] = (duk_uint8_t) '?';
			} else {
				buf[i] = (duk_uint8_t) DUK__DIGITCHAR(t);
			}
		}
		DUK_DDD(DUK_DDDPRINT("-> generated digits; k=%ld, digits='%s'", (long) nc_ctx->k, (const char *) buf));
	}
#endif
}

/* Round up digits to a given position.  If position is out-of-bounds,
 * does nothing.  If carry propagates over the first digit, a '1' is
 * prepended to digits and 'k' will be updated.  Return value indicates
 * whether carry propagated over the first digit.
 *
 * Note that nc_ctx->count is NOT updated based on the rounding position
 * (it is updated only if carry overflows over the first digit and an
 * extra digit is prepended).
 */
DUK_LOCAL duk_small_int_t duk__dragon4_fixed_format_round(duk__numconv_stringify_ctx *nc_ctx, duk_small_int_t round_idx) {
	duk_small_int_t t;
	duk_uint8_t *p;
	duk_uint8_t roundup_limit;
	duk_small_int_t ret = 0;

	/*
	 *  round_idx points to the digit which is considered for rounding; the
	 *  digit to its left is the final digit of the rounded value.  If round_idx
	 *  is zero, rounding will be performed; the result will either be an empty
	 *  rounded value or if carry happens a '1' digit is generated.
	 */

	if (round_idx >= nc_ctx->count) {
		DUK_DDD(DUK_DDDPRINT("round_idx out of bounds (%ld >= %ld (count)) -> no rounding",
		                     (long) round_idx,
		                     (long) nc_ctx->count));
		return 0;
	} else if (round_idx < 0) {
		DUK_DDD(DUK_DDDPRINT("round_idx out of bounds (%ld < 0) -> no rounding", (long) round_idx));
		return 0;
	}

	/*
	 *  Round-up limit.
	 *
	 *  For even values, divides evenly, e.g. 10 -> roundup_limit=5.
	 *
	 *  For odd values, rounds up, e.g. 3 -> roundup_limit=2.
	 *  If radix is 3, 0/3 -> down, 1/3 -> down, 2/3 -> up.
	 */
	roundup_limit = (duk_uint8_t) ((nc_ctx->B + 1) / 2);

	p = &nc_ctx->digits[round_idx];
	if (*p >= roundup_limit) {
		DUK_DDD(DUK_DDDPRINT("fixed-format rounding carry required"));
		/* carry */
		for (;;) {
			*p = 0;
			if (p == &nc_ctx->digits[0]) {
				DUK_DDD(DUK_DDDPRINT("carry propagated to first digit -> special case handling"));
				duk_memmove((void *) (&nc_ctx->digits[1]),
				            (const void *) (&nc_ctx->digits[0]),
				            (size_t) (sizeof(char) * (size_t) nc_ctx->count));
				nc_ctx->digits[0] = 1; /* don't increase 'count' */
				nc_ctx->k++; /* position of highest digit changed */
				nc_ctx->count++; /* number of digits changed */
				ret = 1;
				break;
			}

			DUK_DDD(DUK_DDDPRINT("fixed-format rounding carry: B=%ld, roundup_limit=%ld, p=%p, digits=%p",
			                     (long) nc_ctx->B,
			                     (long) roundup_limit,
			                     (void *) p,
			                     (void *) nc_ctx->digits));
			p--;
			t = *p;
			DUK_DDD(DUK_DDDPRINT("digit before carry: %ld", (long) t));
			if (++t < nc_ctx->B) {
				DUK_DDD(DUK_DDDPRINT("rounding carry terminated"));
				*p = (duk_uint8_t) t;
				break;
			}

			DUK_DDD(DUK_DDDPRINT("wraps, carry to next digit"));
		}
	}

	return ret;
}

#define DUK__NO_EXP (65536) /* arbitrary marker, outside valid exp range */

DUK_LOCAL void duk__dragon4_convert_and_push(duk__numconv_stringify_ctx *nc_ctx,
                                             duk_hthread *thr,
                                             duk_small_int_t radix,
                                             duk_small_int_t digits,
                                             duk_small_uint_t flags,
                                             duk_small_int_t neg) {
	duk_small_int_t k;
	duk_small_int_t pos, pos_end;
	duk_small_int_t expt;
	duk_small_int_t dig;
	duk_uint8_t *q;
	duk_uint8_t *buf;

	/*
	 *  The string conversion here incorporates all the necessary ECMAScript
	 *  semantics without attempting to be generic.  nc_ctx->digits contains
	 *  nc_ctx->count digits (>= 1), with the topmost digit's 'position'
	 *  indicated by nc_ctx->k as follows:
	 *
	 *    digits="123" count=3 k=0   -->   0.123
	 *    digits="123" count=3 k=1   -->   1.23
	 *    digits="123" count=3 k=5   -->   12300
	 *    digits="123" count=3 k=-1  -->   0.0123
	 *
	 *  Note that the identifier names used for format selection are different
	 *  in Burger-Dybvig paper and ECMAScript specification (quite confusingly
	 *  so, because e.g. 'k' has a totally different meaning in each).  See
	 *  documentation for discussion.
	 *
	 *  ECMAScript doesn't specify any specific behavior for format selection
	 *  (e.g. when to use exponent notation) for non-base-10 numbers.
	 *
	 *  The bigint space in the context is reused for string output, as there
	 *  is more than enough space for that (>1kB at the moment), and we avoid
	 *  allocating even more stack.
	 */

	DUK_ASSERT(DUK__NUMCONV_CTX_BIGINTS_SIZE >= DUK__MAX_FORMATTED_LENGTH);
	DUK_ASSERT(nc_ctx->count >= 1);

	k = nc_ctx->k;
	buf = (duk_uint8_t *) &nc_ctx->f; /* XXX: union would be more correct */
	q = buf;

	/* Exponent handling: if exponent format is used, record exponent value and
	 * fake k such that one leading digit is generated (e.g. digits=123 -> "1.23").
	 *
	 * toFixed() prevents exponent use; otherwise apply a set of criteria to
	 * match the other API calls (toString(), toPrecision, etc).
	 */

	expt = DUK__NO_EXP;
	if (!nc_ctx->abs_pos /* toFixed() */) {
		if ((flags & DUK_N2S_FLAG_FORCE_EXP) || /* exponential notation forced */
		    ((flags & DUK_N2S_FLAG_NO_ZERO_PAD) && /* fixed precision and zero padding would be required */
		     (k - digits >= 1)) || /* (e.g. k=3, digits=2 -> "12X") */
		    ((k > 21 || k <= -6) && (radix == 10))) { /* toString() conditions */
			DUK_DDD(DUK_DDDPRINT("use exponential notation: k=%ld -> expt=%ld", (long) k, (long) (k - 1)));
			expt = k - 1; /* e.g. 12.3 -> digits="123" k=2 -> 1.23e1 */
			k = 1; /* generate mantissa with a single leading whole number digit */
		}
	}

	if (neg) {
		*q++ = '-';
	}

	/* Start position (inclusive) and end position (exclusive) */
	pos = (k >= 1 ? k : 1);
	if (nc_ctx->is_fixed) {
		if (nc_ctx->abs_pos) {
			/* toFixed() */
			pos_end = -digits;
		} else {
			pos_end = k - digits;
		}
	} else {
		pos_end = k - nc_ctx->count;
	}
	if (pos_end > 0) {
		pos_end = 0;
	}

	DUK_DDD(DUK_DDDPRINT("expt=%ld, k=%ld, count=%ld, pos=%ld, pos_end=%ld, is_fixed=%ld, "
	                     "digits=%ld, abs_pos=%ld",
	                     (long) expt,
	                     (long) k,
	                     (long) nc_ctx->count,
	                     (long) pos,
	                     (long) pos_end,
	                     (long) nc_ctx->is_fixed,
	                     (long) digits,
	                     (long) nc_ctx->abs_pos));

	/* Digit generation */
	while (pos > pos_end) {
		DUK_DDD(DUK_DDDPRINT("digit generation: pos=%ld, pos_end=%ld", (long) pos, (long) pos_end));
		if (pos == 0) {
			*q++ = (duk_uint8_t) '.';
		}
		if (pos > k) {
			*q++ = (duk_uint8_t) '0';
		} else if (pos <= k - nc_ctx->count) {
			*q++ = (duk_uint8_t) '0';
		} else {
			dig = nc_ctx->digits[k - pos];
			DUK_ASSERT(dig >= 0 && dig < nc_ctx->B);
			*q++ = (duk_uint8_t) DUK__DIGITCHAR(dig);
		}

		pos--;
	}
	DUK_ASSERT(pos <= 1);

	/* Exponent */
	if (expt != DUK__NO_EXP) {
		/*
		 *  Exponent notation for non-base-10 numbers isn't specified in ECMAScript
		 *  specification, as it never explicitly turns up: non-decimal numbers can
		 *  only be formatted with Number.prototype.toString([radix]) and for that,
		 *  behavior is not explicitly specified.
		 *
		 *  Logical choices include formatting the exponent as decimal (e.g. binary
		 *  100000 as 1e+5) or in current radix (e.g. binary 100000 as 1e+101).
		 *  The Dragon4 algorithm (in the original paper) prints the exponent value
		 *  in the target radix B.  However, for radix values 15 and above, the
		 *  exponent separator 'e' is no longer easily parseable.  Consider, for
		 *  instance, the number "1.faecee+1c".
		 */

		duk_size_t len;
		char expt_sign;

		*q++ = 'e';
		if (expt >= 0) {
			expt_sign = '+';
		} else {
			expt_sign = '-';
			expt = -expt;
		}
		*q++ = (duk_uint8_t) expt_sign;
		len = duk__dragon4_format_uint32(q, (duk_uint32_t) expt, radix);
		q += len;
	}

	duk_push_lstring(thr, (const char *) buf, (size_t) (q - buf));
}

/*
 *  Conversion helpers
 */

DUK_LOCAL void duk__dragon4_double_to_ctx(duk__numconv_stringify_ctx *nc_ctx, duk_double_t x) {
	duk_double_union u;
	duk_uint32_t tmp;
	duk_small_int_t expt;

	/*
	 *    seeeeeee eeeeffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff
	 *       A        B        C        D        E        F        G        H
	 *
	 *    s       sign bit
	 *    eee...  exponent field
	 *    fff...  fraction
	 *
	 *    ieee value = 1.ffff... * 2^(e - 1023)  (normal)
	 *               = 0.ffff... * 2^(-1022)     (denormal)
	 *
	 *    algorithm v = f * b^e
	 */

	DUK_DBLUNION_SET_DOUBLE(&u, x);

	nc_ctx->f.n = 2;

	tmp = DUK_DBLUNION_GET_LOW32(&u);
	nc_ctx->f.v[0] = tmp;
	tmp = DUK_DBLUNION_GET_HIGH32(&u);
	nc_ctx->f.v[1] = tmp & 0x000fffffUL;
	expt = (duk_small_int_t) ((tmp >> 20) & 0x07ffUL);

	if (expt == 0) {
		/* denormal */
		expt = DUK__IEEE_DOUBLE_EXP_MIN - 52;
		duk__bi_normalize(&nc_ctx->f);
	} else {
		/* normal: implicit leading 1-bit */
		nc_ctx->f.v[1] |= 0x00100000UL;
		expt = expt - DUK__IEEE_DOUBLE_EXP_BIAS - 52;
		DUK_ASSERT(duk__bi_is_valid(&nc_ctx->f)); /* true, because v[1] has at least one bit set */
	}

	DUK_ASSERT(duk__bi_is_valid(&nc_ctx->f));

	nc_ctx->e = expt;
}

DUK_LOCAL void duk__dragon4_ctx_to_double(duk__numconv_stringify_ctx *nc_ctx, duk_double_t *x) {
	duk_double_union u;
	duk_small_int_t expt;
	duk_small_int_t i;
	duk_small_int_t bitstart;
	duk_small_int_t bitround;
	duk_small_int_t bitidx;
	duk_small_int_t skip_round;
	duk_uint32_t t, v;

	DUK_ASSERT(nc_ctx->count == 53 + 1);

	/* Sometimes this assert is not true right now; it will be true after
	 * rounding.  See: test-bug-numconv-mantissa-assert.js.
	 */
	DUK_ASSERT_DISABLE(nc_ctx->digits[0] == 1); /* zero handled by caller */

	/* Should not be required because the code below always sets both high
	 * and low parts, but at least gcc-4.4.5 fails to deduce this correctly
	 * (perhaps because the low part is set (seemingly) conditionally in a
	 * loop), so this is here to avoid the bogus warning.
	 */
	duk_memzero((void *) &u, sizeof(u));

	/*
	 *  Figure out how generated digits match up with the mantissa,
	 *  and then perform rounding.  If mantissa overflows, need to
	 *  recompute the exponent (it is bumped and may overflow to
	 *  infinity).
	 *
	 *  For normal numbers the leading '1' is hidden and ignored,
	 *  and the last bit is used for rounding:
	 *
	 *                          rounding pt
	 *       <--------52------->|
	 *     1 x x x x ... x x x x|y  ==>  x x x x ... x x x x
	 *
	 *  For denormals, the leading '1' is included in the number,
	 *  and the rounding point is different:
	 *
	 *                      rounding pt
	 *     <--52 or less--->|
	 *     1 x x x x ... x x|x x y  ==>  0 0 ... 1 x x ... x x
	 *
	 *  The largest denormals will have a mantissa beginning with
	 *  a '1' (the explicit leading bit); smaller denormals will
	 *  have leading zero bits.
	 *
	 *  If the exponent would become too high, the result becomes
	 *  Infinity.  If the exponent is so small that the entire
	 *  mantissa becomes zero, the result becomes zero.
	 *
	 *  Note: the Dragon4 'k' is off-by-one with respect to the IEEE
	 *  exponent.  For instance, k==0 indicates that the leading '1'
	 *  digit is at the first binary fraction position (0.1xxx...);
	 *  the corresponding IEEE exponent would be -1.
	 */

	skip_round = 0;

recheck_exp:

	expt = nc_ctx->k - 1; /* IEEE exp without bias */
	if (expt > 1023) {
		/* Infinity */
		bitstart = -255; /* needed for inf: causes mantissa to become zero,
		                  * and rounding to be skipped.
		                  */
		expt = 2047;
	} else if (expt >= -1022) {
		/* normal */
		bitstart = 1; /* skip leading digit */
		expt += DUK__IEEE_DOUBLE_EXP_BIAS;
		DUK_ASSERT(expt >= 1 && expt <= 2046);
	} else {
		/* denormal or zero */
		bitstart = 1023 + expt; /* expt==-1023 -> bitstart=0 (leading 1);
		                         * expt==-1024 -> bitstart=-1 (one left of leading 1), etc
		                         */
		expt = 0;
	}
	bitround = bitstart + 52;

	DUK_DDD(DUK_DDDPRINT("ieee expt=%ld, bitstart=%ld, bitround=%ld", (long) expt, (long) bitstart, (long) bitround));

	if (!skip_round) {
		if (duk__dragon4_fixed_format_round(nc_ctx, bitround)) {
			/* Corner case: see test-numconv-parse-mant-carry.js.  We could
			 * just bump the exponent and update bitstart, but it's more robust
			 * to recompute (but avoid rounding twice).
			 */
			DUK_DDD(DUK_DDDPRINT("rounding caused exponent to be bumped, recheck exponent"));
			skip_round = 1;
			goto recheck_exp;
		}
	}

	/*
	 *  Create mantissa
	 */

	t = 0;
	for (i = 0; i < 52; i++) {
		bitidx = bitstart + 52 - 1 - i;
		if (bitidx >= nc_ctx->count) {
			v = 0;
		} else if (bitidx < 0) {
			v = 0;
		} else {
			v = nc_ctx->digits[bitidx];
		}
		DUK_ASSERT(v == 0 || v == 1);
		t += v << (i % 32);
		if (i == 31) {
			/* low 32 bits is complete */
			DUK_DBLUNION_SET_LOW32(&u, t);
			t = 0;
		}
	}
	/* t has high mantissa */

	DUK_DDD(DUK_DDDPRINT("mantissa is complete: %08lx %08lx", (unsigned long) t, (unsigned long) DUK_DBLUNION_GET_LOW32(&u)));

	DUK_ASSERT(expt >= 0 && expt <= 0x7ffL);
	t += ((duk_uint32_t) expt) << 20;
#if 0 /* caller handles sign change */
	if (negative) {
		t |= 0x80000000U;
	}
#endif
	DUK_DBLUNION_SET_HIGH32(&u, t);

	DUK_DDD(DUK_DDDPRINT("number is complete: %08lx %08lx",
	                     (unsigned long) DUK_DBLUNION_GET_HIGH32(&u),
	                     (unsigned long) DUK_DBLUNION_GET_LOW32(&u)));

	*x = DUK_DBLUNION_GET_DOUBLE(&u);
}

/*
 *  Exposed number-to-string API
 *
 *  Input: [ number ]
 *  Output: [ string ]
 */

DUK_LOCAL DUK_NOINLINE void duk__numconv_stringify_raw(duk_hthread *thr,
                                                       duk_small_int_t radix,
                                                       duk_small_int_t digits,
                                                       duk_small_uint_t flags) {
	duk_double_t x;
	duk_small_int_t c;
	duk_small_int_t neg;
	duk_uint32_t uval;
	duk__numconv_stringify_ctx nc_ctx_alloc; /* large context; around 2kB now */
	duk__numconv_stringify_ctx *nc_ctx = &nc_ctx_alloc;

	x = (duk_double_t) duk_require_number(thr, -1);
	duk_pop(thr);

	/*
	 *  Handle special cases (NaN, infinity, zero).
	 */

	c = (duk_small_int_t) DUK_FPCLASSIFY(x);
	if (DUK_SIGNBIT((double) x)) {
		x = -x;
		neg = 1;
	} else {
		neg = 0;
	}

	/* NaN sign bit is platform specific with unpacked, un-normalized NaNs */
	DUK_ASSERT(c == DUK_FP_NAN || DUK_SIGNBIT((double) x) == 0);

	if (c == DUK_FP_NAN) {
		duk_push_hstring_stridx(thr, DUK_STRIDX_NAN);
		return;
	} else if (c == DUK_FP_INFINITE) {
		if (neg) {
			/* -Infinity */
			duk_push_hstring_stridx(thr, DUK_STRIDX_MINUS_INFINITY);
		} else {
			/* Infinity */
			duk_push_hstring_stridx(thr, DUK_STRIDX_INFINITY);
		}
		return;
	} else if (c == DUK_FP_ZERO) {
		/* We can't shortcut zero here if it goes through special formatting
		 * (such as forced exponential notation).
		 */
		;
	}

	/*
	 *  Handle integers in 32-bit range (that is, [-(2**32-1),2**32-1])
	 *  specially, as they're very likely for embedded programs.  This
	 *  is now done for all radix values.  We must be careful not to use
	 *  the fast path when special formatting (e.g. forced exponential)
	 *  is in force.
	 *
	 *  XXX: could save space by supporting radix 10 only and using
	 *  sprintf "%lu" for the fast path and for exponent formatting.
	 */

	uval = duk_double_to_uint32_t(x);
	if (duk_double_equals((double) uval, x) && /* integer number in range */
	    flags == 0) { /* no special formatting */
		/* use bigint area as a temp */
		duk_uint8_t *buf = (duk_uint8_t *) (&nc_ctx->f);
		duk_uint8_t *p = buf;

		DUK_ASSERT(DUK__NUMCONV_CTX_BIGINTS_SIZE >= 32 + 1); /* max size: radix=2 + sign */
		if (neg && uval != 0) {
			/* no negative sign for zero */
			*p++ = (duk_uint8_t) '-';
		}
		p += duk__dragon4_format_uint32(p, uval, radix);
		duk_push_lstring(thr, (const char *) buf, (duk_size_t) (p - buf));
		return;
	}

	/*
	 *  Dragon4 setup.
	 *
	 *  Convert double from IEEE representation for conversion;
	 *  normal finite values have an implicit leading 1-bit.  The
	 *  slow path algorithm doesn't handle zero, so zero is special
	 *  cased here but still creates a valid nc_ctx, and goes
	 *  through normal formatting in case special formatting has
	 *  been requested (e.g. forced exponential format: 0 -> "0e+0").
	 */

	/* Would be nice to bulk clear the allocation, but the context
	 * is 1-2 kilobytes and nothing should rely on it being zeroed.
	 */
#if 0
	duk_memzero((void *) nc_ctx, sizeof(*nc_ctx));  /* slow init, do only for slow path cases */
#endif

	nc_ctx->is_s2n = 0;
	nc_ctx->b = 2;
	nc_ctx->B = radix;
	nc_ctx->abs_pos = 0;
	if (flags & DUK_N2S_FLAG_FIXED_FORMAT) {
		nc_ctx->is_fixed = 1;
		if (flags & DUK_N2S_FLAG_FRACTION_DIGITS) {
			/* absolute req_digits; e.g. digits = 1 -> last digit is 0,
			 * but add an extra digit for rounding.
			 */
			nc_ctx->abs_pos = 1;
			nc_ctx->req_digits = (-digits + 1) - 1;
		} else {
			nc_ctx->req_digits = digits + 1;
		}
	} else {
		nc_ctx->is_fixed = 0;
		nc_ctx->req_digits = 0;
	}

	if (c == DUK_FP_ZERO) {
		/* Zero special case: fake requested number of zero digits; ensure
		 * no sign bit is printed.  Relative and absolute fixed format
		 * require separate handling.
		 */
		duk_small_int_t count;
		if (nc_ctx->is_fixed) {
			if (nc_ctx->abs_pos) {
				count = digits + 2; /* lead zero + 'digits' fractions + 1 for rounding */
			} else {
				count = digits + 1; /* + 1 for rounding */
			}
		} else {
			count = 1;
		}
		DUK_DDD(DUK_DDDPRINT("count=%ld", (long) count));
		DUK_ASSERT(count >= 1);
		duk_memzero((void *) nc_ctx->digits, (size_t) count);
		nc_ctx->count = count;
		nc_ctx->k = 1; /* 0.000... */
		neg = 0;
		goto zero_skip;
	}

	duk__dragon4_double_to_ctx(nc_ctx, x); /* -> sets 'f' and 'e' */
	DUK__BI_PRINT("f", &nc_ctx->f);
	DUK_DDD(DUK_DDDPRINT("e=%ld", (long) nc_ctx->e));

	/*
	 *  Dragon4 slow path digit generation.
	 */

	duk__dragon4_prepare(nc_ctx); /* setup many variables in nc_ctx */

	DUK_DDD(DUK_DDDPRINT("after prepare:"));
	DUK__BI_PRINT("r", &nc_ctx->r);
	DUK__BI_PRINT("s", &nc_ctx->s);
	DUK__BI_PRINT("mp", &nc_ctx->mp);
	DUK__BI_PRINT("mm", &nc_ctx->mm);

	duk__dragon4_scale(nc_ctx);

	DUK_DDD(DUK_DDDPRINT("after scale; k=%ld", (long) nc_ctx->k));
	DUK__BI_PRINT("r", &nc_ctx->r);
	DUK__BI_PRINT("s", &nc_ctx->s);
	DUK__BI_PRINT("mp", &nc_ctx->mp);
	DUK__BI_PRINT("mm", &nc_ctx->mm);

	duk__dragon4_generate(nc_ctx);

	/*
	 *  Convert and push final string.
	 */

zero_skip:

	if (flags & DUK_N2S_FLAG_FIXED_FORMAT) {
		/* Perform fixed-format rounding. */
		duk_small_int_t roundpos;
		if (flags & DUK_N2S_FLAG_FRACTION_DIGITS) {
			/* 'roundpos' is relative to nc_ctx->k and increases to the right
			 * (opposite of how 'k' changes).
			 */
			roundpos = -digits; /* absolute position for digit considered for rounding */
			roundpos = nc_ctx->k - roundpos;
		} else {
			roundpos = digits;
		}
		DUK_DDD(DUK_DDDPRINT("rounding: k=%ld, count=%ld, digits=%ld, roundpos=%ld",
		                     (long) nc_ctx->k,
		                     (long) nc_ctx->count,
		                     (long) digits,
		                     (long) roundpos));
		(void) duk__dragon4_fixed_format_round(nc_ctx, roundpos);

		/* Note: 'count' is currently not adjusted by rounding (i.e. the
		 * digits are not "chopped off".  That shouldn't matter because
		 * the digit position (absolute or relative) is passed on to the
		 * convert-and-push function.
		 */
	}

	duk__dragon4_convert_and_push(nc_ctx, thr, radix, digits, flags, neg);
}

DUK_INTERNAL void duk_numconv_stringify(duk_hthread *thr, duk_small_int_t radix, duk_small_int_t digits, duk_small_uint_t flags) {
	duk_native_stack_check(thr);
	duk__numconv_stringify_raw(thr, radix, digits, flags);
}

/*
 *  Exposed string-to-number API
 *
 *  Input: [ string ]
 *  Output: [ number ]
 *
 *  If number parsing fails, a NaN is pushed as the result.  If number parsing
 *  fails due to an internal error, an InternalError is thrown.
 */

DUK_LOCAL DUK_NOINLINE void duk__numconv_parse_raw(duk_hthread *thr, duk_small_int_t radix, duk_small_uint_t flags) {
	duk__numconv_stringify_ctx nc_ctx_alloc; /* large context; around 2kB now */
	duk__numconv_stringify_ctx *nc_ctx = &nc_ctx_alloc;
	duk_double_t res;
	duk_hstring *h_str;
	duk_int_t expt;
	duk_bool_t expt_neg;
	duk_small_int_t expt_adj;
	duk_small_int_t neg;
	duk_small_int_t dig;
	duk_small_int_t dig_whole;
	duk_small_int_t dig_lzero;
	duk_small_int_t dig_frac;
	duk_small_int_t dig_expt;
	duk_small_int_t dig_prec;
	const duk__exp_limits *explim;
	const duk_uint8_t *p;
	duk_small_int_t ch;

	DUK_DDD(DUK_DDDPRINT("parse number: %!T, radix=%ld, flags=0x%08lx",
	                     (duk_tval *) duk_get_tval(thr, -1),
	                     (long) radix,
	                     (unsigned long) flags));

	DUK_ASSERT(radix >= 2 && radix <= 36);
	DUK_ASSERT(radix - 2 < (duk_small_int_t) sizeof(duk__str2num_digits_for_radix));

	/*
	 *  Preliminaries: trim, sign, Infinity check
	 *
	 *  We rely on the interned string having a NUL terminator, which will
	 *  cause a parse failure wherever it is encountered.  As a result, we
	 *  don't need separate pointer checks.
	 *
	 *  There is no special parsing for 'NaN' in the specification although
	 *  'Infinity' (with an optional sign) is allowed in some contexts.
	 *  Some contexts allow plus/minus sign, while others only allow the
	 *  minus sign (like JSON.parse()).
	 *
	 *  Automatic hex number detection (leading '0x' or '0X') and octal
	 *  number detection (leading '0' followed by at least one octal digit)
	 *  is done here too.
	 *
	 *  Symbols are not explicitly rejected here (that's up to the caller).
	 *  If a symbol were passed here, it should ultimately safely fail
	 *  parsing due to a syntax error.
	 */

	if (flags & DUK_S2N_FLAG_TRIM_WHITE) {
		/* Leading / trailing whitespace is sometimes accepted and
		 * sometimes not.  After white space trimming, all valid input
		 * characters are pure ASCII.
		 */
		duk_trim(thr, -1);
	}
	h_str = duk_require_hstring(thr, -1);
	DUK_ASSERT(h_str != NULL);
	p = (const duk_uint8_t *) DUK_HSTRING_GET_DATA(h_str);

	neg = 0;
	ch = *p;
	if (ch == (duk_small_int_t) '+') {
		if ((flags & DUK_S2N_FLAG_ALLOW_PLUS) == 0) {
			DUK_DDD(DUK_DDDPRINT("parse failed: leading plus sign not allowed"));
			goto parse_fail;
		}
		p++;
	} else if (ch == (duk_small_int_t) '-') {
		if ((flags & DUK_S2N_FLAG_ALLOW_MINUS) == 0) {
			DUK_DDD(DUK_DDDPRINT("parse failed: leading minus sign not allowed"));
			goto parse_fail;
		}
		p++;
		neg = 1;
	}

	if ((flags & DUK_S2N_FLAG_ALLOW_INF) && DUK_STRNCMP((const char *) p, "Infinity", 8) == 0) {
		/* Don't check for Infinity unless the context allows it.
		 * 'Infinity' is a valid integer literal in e.g. base-36:
		 *
		 *   parseInt('Infinity', 36)
		 *   1461559270678
		 */

		if ((flags & DUK_S2N_FLAG_ALLOW_GARBAGE) == 0 && p[8] != DUK_ASC_NUL) {
			DUK_DDD(DUK_DDDPRINT("parse failed: trailing garbage after matching 'Infinity' not allowed"));
			goto parse_fail;
		} else {
			res = DUK_DOUBLE_INFINITY;
			goto negcheck_and_ret;
		}
	}
	ch = *p;
	if (ch == (duk_small_int_t) '0') {
		duk_small_int_t detect_radix = 0;
		ch = DUK_LOWERCASE_CHAR_ASCII(p[1]); /* 'x' or 'X' -> 'x' */
		if ((flags & DUK_S2N_FLAG_ALLOW_AUTO_HEX_INT) && ch == DUK_ASC_LC_X) {
			DUK_DDD(DUK_DDDPRINT("detected 0x/0X hex prefix, changing radix and preventing fractions and exponent"));
			detect_radix = 16;
#if 0
		} else if ((flags & DUK_S2N_FLAG_ALLOW_AUTO_LEGACY_OCT_INT) &&
		           (ch >= (duk_small_int_t) '0' && ch <= (duk_small_int_t) '9')) {
			DUK_DDD(DUK_DDDPRINT("detected 0n oct prefix, changing radix and preventing fractions and exponent"));
			detect_radix = 8;

			/* NOTE: if this legacy octal case is added back, it has
			 * different flags and 'p' advance so this needs to be
			 * reworked.
			 */
			flags |= DUK_S2N_FLAG_ALLOW_EMPTY_AS_ZERO;  /* interpret e.g. '09' as '0', not NaN */
			p += 1;
#endif
		} else if ((flags & DUK_S2N_FLAG_ALLOW_AUTO_OCT_INT) && ch == DUK_ASC_LC_O) {
			DUK_DDD(DUK_DDDPRINT("detected 0o oct prefix, changing radix and preventing fractions and exponent"));
			detect_radix = 8;
		} else if ((flags & DUK_S2N_FLAG_ALLOW_AUTO_BIN_INT) && ch == DUK_ASC_LC_B) {
			DUK_DDD(DUK_DDDPRINT("detected 0b bin prefix, changing radix and preventing fractions and exponent"));
			detect_radix = 2;
		}
		if (detect_radix > 0) {
			radix = detect_radix;
			/* Clear empty as zero flag: interpret e.g. '0x' and '0xg' as a NaN (= parse error) */
			flags &= ~(DUK_S2N_FLAG_ALLOW_EXP | DUK_S2N_FLAG_ALLOW_EMPTY_FRAC | DUK_S2N_FLAG_ALLOW_FRAC |
			           DUK_S2N_FLAG_ALLOW_NAKED_FRAC | DUK_S2N_FLAG_ALLOW_EMPTY_AS_ZERO);
			flags |= DUK_S2N_FLAG_ALLOW_LEADING_ZERO; /* allow e.g. '0x0009' and '0b00010001' */
			p += 2;
		}
	}

	/*
	 *  Scan number and setup for Dragon4.
	 *
	 *  The fast path case is detected during setup: an integer which
	 *  can be converted without rounding, no net exponent.  The fast
	 *  path could be implemented as a separate scan, but may not really
	 *  be worth it: the multiplications for building 'f' are not
	 *  expensive when 'f' is small.
	 *
	 *  The significand ('f') must contain enough bits of (apparent)
	 *  accuracy, so that Dragon4 will generate enough binary output digits.
	 *  For decimal numbers, this means generating a 20-digit significand,
	 *  which should yield enough practical accuracy to parse IEEE doubles.
	 *  In fact, the ECMAScript specification explicitly allows an
	 *  implementation to treat digits beyond 20 as zeroes (and even
	 *  to round the 20th digit upwards).  For non-decimal numbers, the
	 *  appropriate number of digits has been precomputed for comparable
	 *  accuracy.
	 *
	 *  Digit counts:
	 *
	 *    [ dig_lzero ]
	 *      |
	 *     .+-..---[ dig_prec ]----.
	 *     |  ||                   |
	 *     0000123.456789012345678901234567890e+123456
	 *     |     | |                         |  |    |
	 *     `--+--' `------[ dig_frac ]-------'  `-+--'
	 *        |                                   |
	 *    [ dig_whole ]                       [ dig_expt ]
	 *
	 *    dig_frac and dig_expt are -1 if not present
	 *    dig_lzero is only computed for whole number part
	 *
	 *  Parsing state
	 *
	 *     Parsing whole part      dig_frac < 0 AND dig_expt < 0
	 *     Parsing fraction part   dig_frac >= 0 AND dig_expt < 0
	 *     Parsing exponent part   dig_expt >= 0   (dig_frac may be < 0 or >= 0)
	 *
	 *  Note: in case we hit an implementation limit (like exponent range),
	 *  we should throw an error, NOT return NaN or Infinity.  Even with
	 *  very large exponent (or significand) values the final result may be
	 *  finite, so NaN/Infinity would be incorrect.
	 */

	duk__bi_set_small(&nc_ctx->f, 0);
	dig_prec = 0;
	dig_lzero = 0;
	dig_whole = 0;
	dig_frac = -1;
	dig_expt = -1;
	expt = 0;
	expt_adj = 0; /* essentially tracks digit position of lowest 'f' digit */
	expt_neg = 0;
	for (;;) {
		ch = *p++;

		DUK_DDD(DUK_DDDPRINT("parse digits: p=%p, ch='%c' (%ld), expt=%ld, expt_adj=%ld, "
		                     "dig_whole=%ld, dig_frac=%ld, dig_expt=%ld, dig_lzero=%ld, dig_prec=%ld",
		                     (const void *) p,
		                     (int) ((ch >= 0x20 && ch <= 0x7e) ? ch : '?'),
		                     (long) ch,
		                     (long) expt,
		                     (long) expt_adj,
		                     (long) dig_whole,
		                     (long) dig_frac,
		                     (long) dig_expt,
		                     (long) dig_lzero,
		                     (long) dig_prec));
		DUK__BI_PRINT("f", &nc_ctx->f);

		/* Most common cases first. */
		if (ch >= (duk_small_int_t) '0' && ch <= (duk_small_int_t) '9') {
			dig = (duk_small_int_t) ch - '0' + 0;
		} else if (ch == (duk_small_int_t) '.') {
			/* A leading digit is not required in some cases, e.g. accept ".123".
			 * In other cases (JSON.parse()) a leading digit is required.  This
			 * is checked for after the loop.
			 */
			if (dig_frac >= 0 || dig_expt >= 0) {
				if (flags & DUK_S2N_FLAG_ALLOW_GARBAGE) {
					DUK_DDD(DUK_DDDPRINT("garbage termination (invalid period)"));
					break;
				} else {
					DUK_DDD(DUK_DDDPRINT("parse failed: period not allowed"));
					goto parse_fail;
				}
			}

			if ((flags & DUK_S2N_FLAG_ALLOW_FRAC) == 0) {
				/* Some contexts don't allow fractions at all; this can't be a
				 * post-check because the state ('f' and expt) would be incorrect.
				 */
				if (flags & DUK_S2N_FLAG_ALLOW_GARBAGE) {
					DUK_DDD(DUK_DDDPRINT("garbage termination (invalid first period)"));
					break;
				} else {
					DUK_DDD(DUK_DDDPRINT("parse failed: fraction part not allowed"));
				}
			}

			DUK_DDD(DUK_DDDPRINT("start fraction part"));
			dig_frac = 0;
			continue;
		} else if (ch == (duk_small_int_t) 0) {
			DUK_DDD(DUK_DDDPRINT("NUL termination"));
			break;
		} else if ((flags & DUK_S2N_FLAG_ALLOW_EXP) && dig_expt < 0 &&
		           (ch == (duk_small_int_t) 'e' || ch == (duk_small_int_t) 'E')) {
			/* Note: we don't parse back exponent notation for anything else
			 * than radix 10, so this is not an ambiguous check (e.g. hex
			 * exponent values may have 'e' either as a significand digit
			 * or as an exponent separator).
			 *
			 * If the exponent separator occurs twice, 'e' will be interpreted
			 * as a digit (= 14) and will be rejected as an invalid decimal
			 * digit.
			 */

			DUK_DDD(DUK_DDDPRINT("start exponent part"));

			/* Exponent without a sign or with a +/- sign is accepted
			 * by all call sites (even JSON.parse()).
			 */
			ch = *p;
			if (ch == (duk_small_int_t) '-') {
				expt_neg = 1;
				p++;
			} else if (ch == (duk_small_int_t) '+') {
				p++;
			}
			dig_expt = 0;
			continue;
		} else if (ch >= (duk_small_int_t) 'a' && ch <= (duk_small_int_t) 'z') {
			dig = (duk_small_int_t) (ch - (duk_small_int_t) 'a' + 0x0a);
		} else if (ch >= (duk_small_int_t) 'A' && ch <= (duk_small_int_t) 'Z') {
			dig = (duk_small_int_t) (ch - (duk_small_int_t) 'A' + 0x0a);
		} else {
			dig = 255; /* triggers garbage digit check below */
		}
		DUK_ASSERT((dig >= 0 && dig <= 35) || dig == 255);

		if (dig >= radix) {
			if (flags & DUK_S2N_FLAG_ALLOW_GARBAGE) {
				DUK_DDD(DUK_DDDPRINT("garbage termination"));
				break;
			} else {
				DUK_DDD(DUK_DDDPRINT("parse failed: trailing garbage or invalid digit"));
				goto parse_fail;
			}
		}

		if (dig_expt < 0) {
			/* whole or fraction digit */

			if (dig_prec < duk__str2num_digits_for_radix[radix - 2]) {
				/* significant from precision perspective */

				duk_small_int_t f_zero = duk__bi_is_zero(&nc_ctx->f);
				if (f_zero && dig == 0) {
					/* Leading zero is not counted towards precision digits; not
					 * in the integer part, nor in the fraction part.
					 */
					if (dig_frac < 0) {
						dig_lzero++;
					}
				} else {
					/* XXX: join these ops (multiply-accumulate), but only if
					 * code footprint decreases.
					 */
					duk__bi_mul_small(&nc_ctx->t1, &nc_ctx->f, (duk_uint32_t) radix);
					duk__bi_add_small(&nc_ctx->f, &nc_ctx->t1, (duk_uint32_t) dig);
					dig_prec++;
				}
			} else {
				/* Ignore digits beyond a radix-specific limit, but note them
				 * in expt_adj.
				 */
				expt_adj++;
			}

			if (dig_frac >= 0) {
				dig_frac++;
				expt_adj--;
			} else {
				dig_whole++;
			}
		} else {
			/* exponent digit */

			DUK_ASSERT(radix == 10);
			expt = expt * radix + dig;
			if (expt > DUK_S2N_MAX_EXPONENT) {
				/* Impose a reasonable exponent limit, so that exp
				 * doesn't need to get tracked using a bigint.
				 */
				DUK_DDD(DUK_DDDPRINT("parse failed: exponent too large"));
				goto parse_explimit_error;
			}
			dig_expt++;
		}
	}

	/* Leading zero. */

	if (dig_lzero > 0 && dig_whole > 1) {
		if ((flags & DUK_S2N_FLAG_ALLOW_LEADING_ZERO) == 0) {
			DUK_DDD(DUK_DDDPRINT("parse failed: leading zeroes not allowed in integer part"));
			goto parse_fail;
		}
	}

	/* Validity checks for various fraction formats ("0.1", ".1", "1.", "."). */

	if (dig_whole == 0) {
		if (dig_frac == 0) {
			/* "." is not accepted in any format */
			DUK_DDD(DUK_DDDPRINT("parse failed: plain period without leading or trailing digits"));
			goto parse_fail;
		} else if (dig_frac > 0) {
			/* ".123" */
			if ((flags & DUK_S2N_FLAG_ALLOW_NAKED_FRAC) == 0) {
				DUK_DDD(DUK_DDDPRINT("parse failed: fraction part not allowed without "
				                     "leading integer digit(s)"));
				goto parse_fail;
			}
		} else {
			/* Empty ("") is allowed in some formats (e.g. Number(''), as zero,
			 * but it must not have a leading +/- sign (GH-2019).  Note that
			 * for Number(), h_str is already trimmed so we can check for zero
			 * length and still get Number('  +  ') == NaN.
			 */
			if ((flags & DUK_S2N_FLAG_ALLOW_EMPTY_AS_ZERO) == 0) {
				DUK_DDD(DUK_DDDPRINT("parse failed: empty string not allowed (as zero)"));
				goto parse_fail;
			} else if (DUK_HSTRING_GET_BYTELEN(h_str) != 0) {
				DUK_DDD(DUK_DDDPRINT("parse failed: no digits, but not empty (had a +/- sign)"));
				goto parse_fail;
			}
		}
	} else {
		if (dig_frac == 0) {
			/* "123." is allowed in some formats */
			if ((flags & DUK_S2N_FLAG_ALLOW_EMPTY_FRAC) == 0) {
				DUK_DDD(DUK_DDDPRINT("parse failed: empty fractions"));
				goto parse_fail;
			}
		} else if (dig_frac > 0) {
			/* "123.456" */
			;
		} else {
			/* "123" */
			;
		}
	}

	/* Exponent without digits (e.g. "1e" or "1e+").  If trailing garbage is
	 * allowed, ignore exponent part as garbage (= parse as "1", i.e. exp 0).
	 */

	if (dig_expt == 0) {
		if ((flags & DUK_S2N_FLAG_ALLOW_GARBAGE) == 0) {
			DUK_DDD(DUK_DDDPRINT("parse failed: empty exponent"));
			goto parse_fail;
		}
		DUK_ASSERT(expt == 0);
	}

	if (expt_neg) {
		expt = -expt;
	}
	DUK_DDD(
	    DUK_DDDPRINT("expt=%ld, expt_adj=%ld, net exponent -> %ld", (long) expt, (long) expt_adj, (long) (expt + expt_adj)));
	expt += expt_adj;

	/* Fast path check. */

	if (nc_ctx->f.n <= 1 && /* 32-bit value */
	    expt == 0 /* no net exponent */) {
		/* Fast path is triggered for no exponent and also for balanced exponent
		 * and fraction parts, e.g. for "1.23e2" == "123".  Remember to respect
		 * zero sign.
		 */

		/* XXX: could accept numbers larger than 32 bits, e.g. up to 53 bits? */
		DUK_DDD(DUK_DDDPRINT("fast path number parse"));
		if (nc_ctx->f.n == 1) {
			res = (double) nc_ctx->f.v[0];
		} else {
			res = 0.0;
		}
		goto negcheck_and_ret;
	}

	/* Significand ('f') padding. */

	while (dig_prec < duk__str2num_digits_for_radix[radix - 2]) {
		/* Pad significand with "virtual" zero digits so that Dragon4 will
		 * have enough (apparent) precision to work with.
		 */
		DUK_DDD(DUK_DDDPRINT("dig_prec=%ld, pad significand with zero", (long) dig_prec));
		duk__bi_mul_small_copy(&nc_ctx->f, (duk_uint32_t) radix, &nc_ctx->t1);
		DUK__BI_PRINT("f", &nc_ctx->f);
		expt--;
		dig_prec++;
	}

	DUK_DDD(DUK_DDDPRINT("final exponent: %ld", (long) expt));

	/* Detect zero special case. */

	if (nc_ctx->f.n == 0) {
		/* This may happen even after the fast path check, if exponent is
		 * not balanced (e.g. "0e1").  Remember to respect zero sign.
		 */
		DUK_DDD(DUK_DDDPRINT("significand is zero"));
		res = 0.0;
		goto negcheck_and_ret;
	}

	/* Quick reject of too large or too small exponents.  This check
	 * would be incorrect for zero (e.g. "0e1000" is zero, not Infinity)
	 * so zero check must be above.
	 */

	explim = &duk__str2num_exp_limits[radix - 2];
	if (expt > explim->upper) {
		DUK_DDD(DUK_DDDPRINT("exponent too large -> infinite"));
		res = (duk_double_t) DUK_DOUBLE_INFINITY;
		goto negcheck_and_ret;
	} else if (expt < explim->lower) {
		DUK_DDD(DUK_DDDPRINT("exponent too small -> zero"));
		res = (duk_double_t) 0.0;
		goto negcheck_and_ret;
	}

	nc_ctx->is_s2n = 1;
	nc_ctx->e = expt;
	nc_ctx->b = radix;
	nc_ctx->B = 2;
	nc_ctx->is_fixed = 1;
	nc_ctx->abs_pos = 0;
	nc_ctx->req_digits = 53 + 1;

	DUK__BI_PRINT("f", &nc_ctx->f);
	DUK_DDD(DUK_DDDPRINT("e=%ld", (long) nc_ctx->e));

	/*
	 *  Dragon4 slow path (binary) digit generation.
	 *  An extra digit is generated for rounding.
	 */

	duk__dragon4_prepare(nc_ctx); /* setup many variables in nc_ctx */

	DUK_DDD(DUK_DDDPRINT("after prepare:"));
	DUK__BI_PRINT("r", &nc_ctx->r);
	DUK__BI_PRINT("s", &nc_ctx->s);
	DUK__BI_PRINT("mp", &nc_ctx->mp);
	DUK__BI_PRINT("mm", &nc_ctx->mm);

	duk__dragon4_scale(nc_ctx);

	DUK_DDD(DUK_DDDPRINT("after scale; k=%ld", (long) nc_ctx->k));
	DUK__BI_PRINT("r", &nc_ctx->r);
	DUK__BI_PRINT("s", &nc_ctx->s);
	DUK__BI_PRINT("mp", &nc_ctx->mp);
	DUK__BI_PRINT("mm", &nc_ctx->mm);

	duk__dragon4_generate(nc_ctx);

	DUK_ASSERT(nc_ctx->count == 53 + 1);

	/*
	 *  Convert binary digits into an IEEE double.  Need to handle
	 *  denormals and rounding correctly.
	 *
	 *  Some call sites currently assume the result is always a
	 *  non-fastint double.  If this is changed, check all call
	 *  sites.
	 */

	duk__dragon4_ctx_to_double(nc_ctx, &res);
	goto negcheck_and_ret;

negcheck_and_ret:
	if (neg) {
		res = -res;
	}
	duk_pop(thr);
	duk_push_number(thr, (double) res);
	DUK_DDD(DUK_DDDPRINT("result: %!T", (duk_tval *) duk_get_tval(thr, -1)));
	return;

parse_fail:
	DUK_DDD(DUK_DDDPRINT("parse failed"));
	duk_pop(thr);
	duk_push_nan(thr);
	return;

parse_explimit_error:
	DUK_DDD(DUK_DDDPRINT("parse failed, internal error, can't return a value"));
	DUK_ERROR_RANGE(thr, "exponent too large");
	DUK_WO_NORETURN(return;);
}

DUK_INTERNAL void duk_numconv_parse(duk_hthread *thr, duk_small_int_t radix, duk_small_uint_t flags) {
	duk_native_stack_check(thr);
	duk__numconv_parse_raw(thr, radix, flags);
}
