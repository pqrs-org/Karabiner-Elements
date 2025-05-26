/*
 *  Self tests to ensure execution environment is sane.  Intended to catch
 *  compiler/platform problems which cannot be detected at compile time.
 */

#include "duk_internal.h"

#if defined(DUK_USE_SELF_TESTS)

/*
 *  Unions and structs for self tests
 */

typedef union {
	double d;
	duk_uint8_t x[8];
} duk__test_double_union;

/* Self test failed.  Expects a local variable 'error_count' to exist. */
#define DUK__FAILED(msg) \
	do { \
		DUK_D(DUK_DPRINT("self test failed: " #msg " at " DUK_FILE_MACRO ":" DUK_MACRO_STRINGIFY(DUK_LINE_MACRO))); \
		error_count++; \
	} while (0)

#define DUK__DBLUNION_CMP_TRUE(a, b) \
	do { \
		if (duk_memcmp((const void *) (a), (const void *) (b), sizeof(duk__test_double_union)) != 0) { \
			DUK__FAILED("double union compares false (expected true)"); \
		} \
	} while (0)

#define DUK__DBLUNION_CMP_FALSE(a, b) \
	do { \
		if (duk_memcmp((const void *) (a), (const void *) (b), sizeof(duk__test_double_union)) == 0) { \
			DUK__FAILED("double union compares true (expected false)"); \
		} \
	} while (0)

typedef union {
	duk_uint32_t i;
	duk_uint8_t x[8];
} duk__test_u32_union;

#if defined(DUK_USE_INTEGER_LE)
#define DUK__U32_INIT(u, a, b, c, d) \
	do { \
		(u)->x[0] = (d); \
		(u)->x[1] = (c); \
		(u)->x[2] = (b); \
		(u)->x[3] = (a); \
	} while (0)
#elif defined(DUK_USE_INTEGER_ME)
#error integer mixed endian not supported now
#elif defined(DUK_USE_INTEGER_BE)
#define DUK__U32_INIT(u, a, b, c, d) \
	do { \
		(u)->x[0] = (a); \
		(u)->x[1] = (b); \
		(u)->x[2] = (c); \
		(u)->x[3] = (d); \
	} while (0)
#else
#error unknown integer endianness
#endif

#if defined(DUK_USE_DOUBLE_LE)
#define DUK__DOUBLE_INIT(u, a, b, c, d, e, f, g, h) \
	do { \
		(u)->x[0] = (h); \
		(u)->x[1] = (g); \
		(u)->x[2] = (f); \
		(u)->x[3] = (e); \
		(u)->x[4] = (d); \
		(u)->x[5] = (c); \
		(u)->x[6] = (b); \
		(u)->x[7] = (a); \
	} while (0)
#define DUK__DOUBLE_COMPARE(u, a, b, c, d, e, f, g, h) \
	((u)->x[0] == (h) && (u)->x[1] == (g) && (u)->x[2] == (f) && (u)->x[3] == (e) && (u)->x[4] == (d) && (u)->x[5] == (c) && \
	 (u)->x[6] == (b) && (u)->x[7] == (a))
#elif defined(DUK_USE_DOUBLE_ME)
#define DUK__DOUBLE_INIT(u, a, b, c, d, e, f, g, h) \
	do { \
		(u)->x[0] = (d); \
		(u)->x[1] = (c); \
		(u)->x[2] = (b); \
		(u)->x[3] = (a); \
		(u)->x[4] = (h); \
		(u)->x[5] = (g); \
		(u)->x[6] = (f); \
		(u)->x[7] = (e); \
	} while (0)
#define DUK__DOUBLE_COMPARE(u, a, b, c, d, e, f, g, h) \
	((u)->x[0] == (d) && (u)->x[1] == (c) && (u)->x[2] == (b) && (u)->x[3] == (a) && (u)->x[4] == (h) && (u)->x[5] == (g) && \
	 (u)->x[6] == (f) && (u)->x[7] == (e))
#elif defined(DUK_USE_DOUBLE_BE)
#define DUK__DOUBLE_INIT(u, a, b, c, d, e, f, g, h) \
	do { \
		(u)->x[0] = (a); \
		(u)->x[1] = (b); \
		(u)->x[2] = (c); \
		(u)->x[3] = (d); \
		(u)->x[4] = (e); \
		(u)->x[5] = (f); \
		(u)->x[6] = (g); \
		(u)->x[7] = (h); \
	} while (0)
#define DUK__DOUBLE_COMPARE(u, a, b, c, d, e, f, g, h) \
	((u)->x[0] == (a) && (u)->x[1] == (b) && (u)->x[2] == (c) && (u)->x[3] == (d) && (u)->x[4] == (e) && (u)->x[5] == (f) && \
	 (u)->x[6] == (g) && (u)->x[7] == (h))
#else
#error unknown double endianness
#endif

/*
 *  Various sanity checks for typing
 */

DUK_LOCAL duk_uint_t duk__selftest_types(void) {
	duk_uint_t error_count = 0;

	if (!(sizeof(duk_int8_t) == 1 && sizeof(duk_uint8_t) == 1 && sizeof(duk_int16_t) == 2 && sizeof(duk_uint16_t) == 2 &&
	      sizeof(duk_int32_t) == 4 && sizeof(duk_uint32_t) == 4)) {
		DUK__FAILED("duk_(u)int{8,16,32}_t size");
	}
#if defined(DUK_USE_64BIT_OPS)
	if (!(sizeof(duk_int64_t) == 8 && sizeof(duk_uint64_t) == 8)) {
		DUK__FAILED("duk_(u)int64_t size");
	}
#endif

	if (!(sizeof(duk_size_t) >= sizeof(duk_uint_t))) {
		/* Some internal code now assumes that all duk_uint_t values
		 * can be expressed with a duk_size_t.
		 */
		DUK__FAILED("duk_size_t is smaller than duk_uint_t");
	}
	if (!(sizeof(duk_int_t) >= 4)) {
		DUK__FAILED("duk_int_t is not 32 bits");
	}

	return error_count;
}

/*
 *  Packed tval sanity
 */

DUK_LOCAL duk_uint_t duk__selftest_packed_tval(void) {
	duk_uint_t error_count = 0;

#if defined(DUK_USE_PACKED_TVAL)
	if (sizeof(void *) > 4) {
		DUK__FAILED("packed duk_tval in use but sizeof(void *) > 4");
	}
#endif

	return error_count;
}

/*
 *  Two's complement arithmetic.
 */

DUK_LOCAL duk_uint_t duk__selftest_twos_complement(void) {
	duk_uint_t error_count = 0;
	volatile int test;
	test = -1;

	/* Note that byte order doesn't affect this test: all bytes in
	 * 'test' will be 0xFF for two's complement.
	 */
	if (((volatile duk_uint8_t *) &test)[0] != (duk_uint8_t) 0xff) {
		DUK__FAILED("two's complement arithmetic");
	}

	return error_count;
}

/*
 *  Byte order.  Important to self check, because on some exotic platforms
 *  there is no actual detection but rather assumption based on platform
 *  defines.
 */

DUK_LOCAL duk_uint_t duk__selftest_byte_order(void) {
	duk_uint_t error_count = 0;
	duk__test_u32_union u1;
	duk__test_double_union u2;

	/*
	 *  >>> struct.pack('>d', 102030405060).encode('hex')
	 *  '4237c17c6dc40000'
	 */

	DUK__U32_INIT(&u1, 0xde, 0xad, 0xbe, 0xef);
	DUK__DOUBLE_INIT(&u2, 0x42, 0x37, 0xc1, 0x7c, 0x6d, 0xc4, 0x00, 0x00);

	if (u1.i != (duk_uint32_t) 0xdeadbeefUL) {
		DUK__FAILED("duk_uint32_t byte order");
	}

	if (!duk_double_equals(u2.d, 102030405060.0)) {
		DUK__FAILED("double byte order");
	}

	return error_count;
}

/*
 *  DUK_BSWAP macros
 */

DUK_LOCAL duk_uint_t duk__selftest_bswap_macros(void) {
	duk_uint_t error_count = 0;
	volatile duk_uint32_t x32_input, x32_output;
	duk_uint32_t x32;
	volatile duk_uint16_t x16_input, x16_output;
	duk_uint16_t x16;
	duk_double_union du;
	duk_double_t du_diff;
#if defined(DUK_BSWAP64)
	volatile duk_uint64_t x64_input, x64_output;
	duk_uint64_t x64;
#endif

	/* Cover both compile time and runtime bswap operations, as these
	 * may have different bugs.
	 */

	x16_input = 0xbeefUL;
	x16 = x16_input;
	x16 = DUK_BSWAP16(x16);
	x16_output = x16;
	if (x16_output != (duk_uint16_t) 0xefbeUL) {
		DUK__FAILED("DUK_BSWAP16");
	}

	x16 = 0xbeefUL;
	x16 = DUK_BSWAP16(x16);
	if (x16 != (duk_uint16_t) 0xefbeUL) {
		DUK__FAILED("DUK_BSWAP16");
	}

	x32_input = 0xdeadbeefUL;
	x32 = x32_input;
	x32 = DUK_BSWAP32(x32);
	x32_output = x32;
	if (x32_output != (duk_uint32_t) 0xefbeaddeUL) {
		DUK__FAILED("DUK_BSWAP32");
	}

	x32 = 0xdeadbeefUL;
	x32 = DUK_BSWAP32(x32);
	if (x32 != (duk_uint32_t) 0xefbeaddeUL) {
		DUK__FAILED("DUK_BSWAP32");
	}

#if defined(DUK_BSWAP64)
	x64_input = DUK_U64_CONSTANT(0x8899aabbccddeeff);
	x64 = x64_input;
	x64 = DUK_BSWAP64(x64);
	x64_output = x64;
	if (x64_output != (duk_uint64_t) DUK_U64_CONSTANT(0xffeeddccbbaa9988)) {
		DUK__FAILED("DUK_BSWAP64");
	}

	x64 = DUK_U64_CONSTANT(0x8899aabbccddeeff);
	x64 = DUK_BSWAP64(x64);
	if (x64 != (duk_uint64_t) DUK_U64_CONSTANT(0xffeeddccbbaa9988)) {
		DUK__FAILED("DUK_BSWAP64");
	}
#endif

	/* >>> struct.unpack('>d', '4000112233445566'.decode('hex'))
	 * (2.008366013071895,)
	 */

	du.uc[0] = 0x40;
	du.uc[1] = 0x00;
	du.uc[2] = 0x11;
	du.uc[3] = 0x22;
	du.uc[4] = 0x33;
	du.uc[5] = 0x44;
	du.uc[6] = 0x55;
	du.uc[7] = 0x66;
	DUK_DBLUNION_DOUBLE_NTOH(&du);
	du_diff = du.d - 2.008366013071895;
#if 0
	DUK_D(DUK_DPRINT("du_diff: %lg\n", (double) du_diff));
#endif
	if (du_diff > 1e-15) {
		/* Allow very small lenience because some compilers won't parse
		 * exact IEEE double constants (happened in matrix testing with
		 * Linux gcc-4.8 -m32 at least).
		 */
#if 0
		DUK_D(DUK_DPRINT("Result of DUK_DBLUNION_DOUBLE_NTOH: %02x %02x %02x %02x %02x %02x %02x %02x\n",
		            (unsigned int) du.uc[0], (unsigned int) du.uc[1],
		            (unsigned int) du.uc[2], (unsigned int) du.uc[3],
		            (unsigned int) du.uc[4], (unsigned int) du.uc[5],
		            (unsigned int) du.uc[6], (unsigned int) du.uc[7]));
#endif
		DUK__FAILED("DUK_DBLUNION_DOUBLE_NTOH");
	}

	return error_count;
}

/*
 *  Basic double / byte union memory layout.
 */

DUK_LOCAL duk_uint_t duk__selftest_double_union_size(void) {
	duk_uint_t error_count = 0;

	if (sizeof(duk__test_double_union) != 8) {
		DUK__FAILED("invalid union size");
	}

	return error_count;
}

/*
 *  Union aliasing, see misc/clang_aliasing.c.
 */

DUK_LOCAL duk_uint_t duk__selftest_double_aliasing(void) {
	/* This testcase fails when Emscripten-generated code runs on Firefox.
	 * It's not an issue because the failure should only affect packed
	 * duk_tval representation, which is not used with Emscripten.
	 */
#if defined(DUK_USE_PACKED_TVAL)
	duk_uint_t error_count = 0;
	duk__test_double_union a, b;

	/* Test signaling NaN and alias assignment in all endianness combinations.
	 */

	/* little endian */
	a.x[0] = 0x11;
	a.x[1] = 0x22;
	a.x[2] = 0x33;
	a.x[3] = 0x44;
	a.x[4] = 0x00;
	a.x[5] = 0x00;
	a.x[6] = 0xf1;
	a.x[7] = 0xff;
	b = a;
	DUK__DBLUNION_CMP_TRUE(&a, &b);

	/* big endian */
	a.x[0] = 0xff;
	a.x[1] = 0xf1;
	a.x[2] = 0x00;
	a.x[3] = 0x00;
	a.x[4] = 0x44;
	a.x[5] = 0x33;
	a.x[6] = 0x22;
	a.x[7] = 0x11;
	b = a;
	DUK__DBLUNION_CMP_TRUE(&a, &b);

	/* mixed endian */
	a.x[0] = 0x00;
	a.x[1] = 0x00;
	a.x[2] = 0xf1;
	a.x[3] = 0xff;
	a.x[4] = 0x11;
	a.x[5] = 0x22;
	a.x[6] = 0x33;
	a.x[7] = 0x44;
	b = a;
	DUK__DBLUNION_CMP_TRUE(&a, &b);

	return error_count;
#else
	DUK_D(DUK_DPRINT("skip double aliasing self test when duk_tval is not packed"));
	return 0;
#endif
}

/*
 *  Zero sign, see misc/tcc_zerosign2.c.
 */

DUK_LOCAL duk_uint_t duk__selftest_double_zero_sign(void) {
	duk_uint_t error_count = 0;
	duk__test_double_union a, b;

	a.d = 0.0;
	b.d = -a.d;
	DUK__DBLUNION_CMP_FALSE(&a, &b);

	return error_count;
}

/*
 *  Rounding mode: Duktape assumes round-to-nearest, check that this is true.
 *  If we had C99 fenv.h we could check that fegetround() == FE_TONEAREST,
 *  but we don't want to rely on that header; and even if we did, it's good
 *  to ensure the rounding actually works.
 */

DUK_LOCAL duk_uint_t duk__selftest_double_rounding(void) {
	duk_uint_t error_count = 0;
	duk__test_double_union a, b, c;

#if 0
	/* Include <fenv.h> and test manually; these trigger failures: */
	fesetround(FE_UPWARD);
	fesetround(FE_DOWNWARD);
	fesetround(FE_TOWARDZERO);

	/* This is the default and passes. */
	fesetround(FE_TONEAREST);
#endif

	/* Rounding tests check that none of the other modes (round to
	 * +Inf, round to -Inf, round to zero) can be active:
	 * http://www.gnu.org/software/libc/manual/html_node/Rounding.html
	 */

	/* 1.0 + 2^(-53): result is midway between 1.0 and 1.0 + ulp.
	 * Round to nearest: 1.0
	 * Round to +Inf:    1.0 + ulp
	 * Round to -Inf:    1.0
	 * Round to zero:    1.0
	 * => Correct result eliminates round to +Inf.
	 */
	DUK__DOUBLE_INIT(&a, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
	DUK__DOUBLE_INIT(&b, 0x3c, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
	duk_memset((void *) &c, 0, sizeof(c));
	c.d = a.d + b.d;
	if (!DUK__DOUBLE_COMPARE(&c, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)) {
		DUK_D(DUK_DPRINT("broken result (native endiannesss): %02x %02x %02x %02x %02x %02x %02x %02x",
		                 (unsigned int) c.x[0],
		                 (unsigned int) c.x[1],
		                 (unsigned int) c.x[2],
		                 (unsigned int) c.x[3],
		                 (unsigned int) c.x[4],
		                 (unsigned int) c.x[5],
		                 (unsigned int) c.x[6],
		                 (unsigned int) c.x[7]));
		DUK__FAILED("invalid result from 1.0 + 0.5ulp");
	}

	/* (1.0 + ulp) + 2^(-53): result is midway between 1.0 + ulp and 1.0 + 2*ulp.
	 * Round to nearest: 1.0 + 2*ulp (round to even mantissa)
	 * Round to +Inf:    1.0 + 2*ulp
	 * Round to -Inf:    1.0 + ulp
	 * Round to zero:    1.0 + ulp
	 * => Correct result eliminates round to -Inf and round to zero.
	 */
	DUK__DOUBLE_INIT(&a, 0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01);
	DUK__DOUBLE_INIT(&b, 0x3c, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
	duk_memset((void *) &c, 0, sizeof(c));
	c.d = a.d + b.d;
	if (!DUK__DOUBLE_COMPARE(&c, 0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02)) {
		DUK_D(DUK_DPRINT("broken result (native endiannesss): %02x %02x %02x %02x %02x %02x %02x %02x",
		                 (unsigned int) c.x[0],
		                 (unsigned int) c.x[1],
		                 (unsigned int) c.x[2],
		                 (unsigned int) c.x[3],
		                 (unsigned int) c.x[4],
		                 (unsigned int) c.x[5],
		                 (unsigned int) c.x[6],
		                 (unsigned int) c.x[7]));
		DUK__FAILED("invalid result from (1.0 + ulp) + 0.5ulp");
	}

	/* Could do negative number testing too, but the tests above should
	 * differentiate between IEEE 754 rounding modes.
	 */
	return error_count;
}

/*
 *  fmod(): often a portability issue in embedded or bare platform targets.
 *  Check for at least minimally correct behavior.  Unlike some other math
 *  functions (like cos()) Duktape relies on fmod() internally too.
 */

DUK_LOCAL duk_uint_t duk__selftest_fmod(void) {
	duk_uint_t error_count = 0;
	duk__test_double_union u1, u2;
	volatile duk_double_t t1, t2, t3;

	/* fmod() with integer argument and exponent 2^32 is used by e.g.
	 * ToUint32() and some Duktape internals.
	 */
	u1.d = DUK_FMOD(10.0, 4294967296.0);
	u2.d = 10.0;
	DUK__DBLUNION_CMP_TRUE(&u1, &u2);

	u1.d = DUK_FMOD(4294967306.0, 4294967296.0);
	u2.d = 10.0;
	DUK__DBLUNION_CMP_TRUE(&u1, &u2);

	u1.d = DUK_FMOD(73014444042.0, 4294967296.0);
	u2.d = 10.0;
	DUK__DBLUNION_CMP_TRUE(&u1, &u2);

	/* 52-bit integer split into two parts:
	 * >>> 0x1fedcba9876543
	 * 8987183256397123
	 * >>> float(0x1fedcba9876543) / float(2**53)
	 * 0.9977777777777778
	 */
	u1.d = DUK_FMOD(8987183256397123.0, 4294967296.0);
	u2.d = (duk_double_t) 0xa9876543UL;
	DUK__DBLUNION_CMP_TRUE(&u1, &u2);
	t1 = 8987183256397123.0;
	t2 = 4294967296.0;
	t3 = t1 / t2;
	u1.d = DUK_FLOOR(t3);
	u2.d = (duk_double_t) 0x1fedcbUL;
	DUK__DBLUNION_CMP_TRUE(&u1, &u2);

	/* C99 behavior is for fmod() result sign to mathc argument sign. */
	u1.d = DUK_FMOD(-10.0, 4294967296.0);
	u2.d = -10.0;
	DUK__DBLUNION_CMP_TRUE(&u1, &u2);

	u1.d = DUK_FMOD(-4294967306.0, 4294967296.0);
	u2.d = -10.0;
	DUK__DBLUNION_CMP_TRUE(&u1, &u2);

	u1.d = DUK_FMOD(-73014444042.0, 4294967296.0);
	u2.d = -10.0;
	DUK__DBLUNION_CMP_TRUE(&u1, &u2);

	return error_count;
}

/*
 *  Struct size/alignment if platform requires it
 *
 *  There are some compiler specific struct padding pragmas etc in use, this
 *  selftest ensures they're correctly detected and used.
 */

DUK_LOCAL duk_uint_t duk__selftest_struct_align(void) {
	duk_uint_t error_count = 0;

#if (DUK_USE_ALIGN_BY == 4)
	if ((sizeof(duk_hbuffer_fixed) % 4) != 0) {
		DUK__FAILED("sizeof(duk_hbuffer_fixed) not aligned to 4");
	}
#elif (DUK_USE_ALIGN_BY == 8)
	if ((sizeof(duk_hbuffer_fixed) % 8) != 0) {
		DUK__FAILED("sizeof(duk_hbuffer_fixed) not aligned to 8");
	}
#elif (DUK_USE_ALIGN_BY == 1)
	/* no check */
#else
#error invalid DUK_USE_ALIGN_BY
#endif
	return error_count;
}

/*
 *  64-bit arithmetic
 *
 *  There are some platforms/compilers where 64-bit types are available
 *  but don't work correctly.  Test for known cases.
 */

DUK_LOCAL duk_uint_t duk__selftest_64bit_arithmetic(void) {
	duk_uint_t error_count = 0;
#if defined(DUK_USE_64BIT_OPS)
	volatile duk_int64_t i;
	volatile duk_double_t d;

	/* Catch a double-to-int64 cast issue encountered in practice. */
	d = 2147483648.0;
	i = (duk_int64_t) d;
	if (i != DUK_I64_CONSTANT(0x80000000)) {
		DUK__FAILED("casting 2147483648.0 to duk_int64_t failed");
	}
#else
	/* nop */
#endif
	return error_count;
}

/*
 *  Casting
 */

DUK_LOCAL duk_uint_t duk__selftest_cast_double_to_small_uint(void) {
	/*
	 *  https://github.com/svaarala/duktape/issues/127#issuecomment-77863473
	 */

	duk_uint_t error_count = 0;

	duk_double_t d1, d2;
	duk_small_uint_t u;

	duk_double_t d1v, d2v;
	duk_small_uint_t uv;

	/* Test without volatiles */

	d1 = 1.0;
	u = (duk_small_uint_t) d1;
	d2 = (duk_double_t) u;

	if (!(duk_double_equals(d1, 1.0) && u == 1 && duk_double_equals(d2, 1.0) && duk_double_equals(d1, d2))) {
		DUK__FAILED("double to duk_small_uint_t cast failed");
	}

	/* Same test with volatiles */

	d1v = 1.0;
	uv = (duk_small_uint_t) d1v;
	d2v = (duk_double_t) uv;

	if (!(duk_double_equals(d1v, 1.0) && uv == 1 && duk_double_equals(d2v, 1.0) && duk_double_equals(d1v, d2v))) {
		DUK__FAILED("double to duk_small_uint_t cast failed");
	}

	return error_count;
}

DUK_LOCAL duk_uint_t duk__selftest_cast_double_to_uint32(void) {
	/*
	 *  This test fails on an exotic ARM target; double-to-uint
	 *  cast is incorrectly clamped to -signed- int highest value.
	 *
	 *  https://github.com/svaarala/duktape/issues/336
	 */

	duk_uint_t error_count = 0;
	duk_double_t dv;
	duk_uint32_t uv;

	dv = 3735928559.0; /* 0xdeadbeef in decimal */
	uv = (duk_uint32_t) dv;

	if (uv != 0xdeadbeefUL) {
		DUK__FAILED("double to duk_uint32_t cast failed");
	}

	return error_count;
}

/*
 *  Minimal test of user supplied allocation functions
 *
 *    - Basic alloc + realloc + free cycle
 *
 *    - Realloc to significantly larger size to (hopefully) trigger a
 *      relocation and check that relocation copying works
 */

DUK_LOCAL duk_uint_t duk__selftest_alloc_funcs(duk_alloc_function alloc_func,
                                               duk_realloc_function realloc_func,
                                               duk_free_function free_func,
                                               void *udata) {
	duk_uint_t error_count = 0;
	void *ptr;
	void *new_ptr;
	duk_small_int_t i, j;
	unsigned char x;

	if (alloc_func == NULL || realloc_func == NULL || free_func == NULL) {
		return 0;
	}

	for (i = 1; i <= 256; i++) {
		ptr = alloc_func(udata, (duk_size_t) i);
		if (ptr == NULL) {
			DUK_D(DUK_DPRINT("alloc failed, ignore"));
			continue; /* alloc failed, ignore */
		}
		for (j = 0; j < i; j++) {
			((unsigned char *) ptr)[j] = (unsigned char) (0x80 + j);
		}
		new_ptr = realloc_func(udata, ptr, 1024);
		if (new_ptr == NULL) {
			DUK_D(DUK_DPRINT("realloc failed, ignore"));
			free_func(udata, ptr);
			continue; /* realloc failed, ignore */
		}
		ptr = new_ptr;
		for (j = 0; j < i; j++) {
			x = ((unsigned char *) ptr)[j];
			if (x != (unsigned char) (0x80 + j)) {
				DUK_D(DUK_DPRINT("byte at index %ld doesn't match after realloc: %02lx",
				                 (long) j,
				                 (unsigned long) x));
				DUK__FAILED("byte compare after realloc");
				break;
			}
		}
		free_func(udata, ptr);
	}

	return error_count;
}

/*
 *  Self test main
 */

DUK_INTERNAL duk_uint_t duk_selftest_run_tests(duk_alloc_function alloc_func,
                                               duk_realloc_function realloc_func,
                                               duk_free_function free_func,
                                               void *udata) {
	duk_uint_t error_count = 0;

	DUK_D(DUK_DPRINT("self test starting"));

	error_count += duk__selftest_types();
	error_count += duk__selftest_packed_tval();
	error_count += duk__selftest_twos_complement();
	error_count += duk__selftest_byte_order();
	error_count += duk__selftest_bswap_macros();
	error_count += duk__selftest_double_union_size();
	error_count += duk__selftest_double_aliasing();
	error_count += duk__selftest_double_zero_sign();
	error_count += duk__selftest_double_rounding();
	error_count += duk__selftest_fmod();
	error_count += duk__selftest_struct_align();
	error_count += duk__selftest_64bit_arithmetic();
	error_count += duk__selftest_cast_double_to_small_uint();
	error_count += duk__selftest_cast_double_to_uint32();
	error_count += duk__selftest_alloc_funcs(alloc_func, realloc_func, free_func, udata);

	DUK_D(DUK_DPRINT("self test complete, total error count: %ld", (long) error_count));

	return error_count;
}

#endif /* DUK_USE_SELF_TESTS */
