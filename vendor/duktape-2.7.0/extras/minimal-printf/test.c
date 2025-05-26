#include <stdio.h>
#include <string.h>
#include "duk_minimal_printf.h"

char buffer[32];

static void init_buffer(void) {
	int i;

	for (i = 0; i < (int) sizeof(buffer); i++) {
		buffer[i] = 0xff;
	}
}

static void dump_buffer(void) {
	int i;
	unsigned char c;

	printf("Buffer: '");
	for (i = 0; i < (int) sizeof(buffer); i++) {
		c = (unsigned char) buffer[i];
		if (c < 0x20 || c >= 0x7e) {
			printf("<%02x>", (unsigned int) c);
		} else {
			printf("%c", (int) c);
		}
	}
	printf("'");
#if 0
	printf(" -> ");
	printf("Buffer:");
	for (i = 0; i < sizeof(buffer); i++) {
		c = (unsigned char) buffer[i];
		if (c <= 0x20 || c >= 0x7e) {
			printf(" <%02x>", (unsigned int) c);
		} else {
			printf(" %c", (char) c);
		}
	}
#endif
	printf("\n");
}

int main(int argc, char *argv[]) {
	int ret;
	void *voidptr;
	int i;

	(void) argc; (void) argv;

	/* Char format. */
	init_buffer();
	duk_minimal_snprintf(buffer, sizeof(buffer), "foo %c bar", 'Z');
	dump_buffer();

	/* Signed long format. */
	init_buffer();
	duk_minimal_snprintf(buffer, sizeof(buffer), "%ld %9ld", (long) 123, (long) 4321);
	dump_buffer();

	/* Signed long with zero padding. */
	init_buffer();
	duk_minimal_snprintf(buffer, sizeof(buffer), "%09ld", (long) 4321);
	dump_buffer();
	init_buffer();
	duk_minimal_snprintf(buffer, sizeof(buffer), "%03ld %03ld %03ld", (long) -4321, (long) -432, (long) -43);
	dump_buffer();

	/* Unsigned long with zero padding. */
	init_buffer();
	duk_minimal_snprintf(buffer, sizeof(buffer), "%03lu %03lu %03lu", (long) -4321, (long) -432, (long) -43);
	dump_buffer();

	/* Signed integer. */
	init_buffer();
	duk_minimal_snprintf(buffer, sizeof(buffer), "%d %9d", (int) 0, (int) 4321);
	dump_buffer();

	/* Signed negative integer, fixed field width. */
	init_buffer();
	duk_minimal_snprintf(buffer, sizeof(buffer), "%9d", (int) -321);
	dump_buffer();
	init_buffer();
	duk_minimal_snprintf(buffer, sizeof(buffer), "%09d", (int) -321);
	dump_buffer();
	printf("  -- printf comparison: %9d %09d\n", -321, -321);

	/* Hex formatting. */
	init_buffer();
	duk_minimal_snprintf(buffer, sizeof(buffer), "%03x %03lx 0x%08lx", (int) 510, (long) 5105, (long) 0xdeadbeef);
	dump_buffer();

	/* Pointer formatting, NULL and non-NULL. */
	init_buffer();
	duk_minimal_snprintf(buffer, sizeof(buffer), "%p %p", (void *) NULL, (void *) buffer);
	dump_buffer();

	/* File/line like format test. */
	init_buffer();
	duk_minimal_snprintf(buffer, sizeof(buffer), "%s:%d", "foo bar quux", 123);
	dump_buffer();

	/* Zero size output buffer. */
	init_buffer();
	duk_minimal_snprintf(buffer, 0, "%s:%d", "foo bar quux", 123);
	dump_buffer();
	init_buffer();
	duk_minimal_snprintf(buffer, 0, "");
	dump_buffer();

	/* NUL terminator boundary test. */
	init_buffer();
	duk_minimal_snprintf(buffer, 7, "foo: %s", "bar");
	dump_buffer();
	init_buffer();
	duk_minimal_snprintf(buffer, 8, "foo: %s", "bar");
	dump_buffer();
	init_buffer();
	duk_minimal_snprintf(buffer, 9, "foo: %s", "bar");
	dump_buffer();

	/* sprintf() binding, uses SIZE_MAX internally. */
	init_buffer();
	duk_minimal_sprintf(buffer, "unbounded print %s", "foo");
	dump_buffer();

	/* Pointer formatting; non-NULL and NULL. */
	init_buffer();
	duk_minimal_snprintf(buffer, sizeof(buffer), "%p %p", (void *) NULL, (void *) 0xdeadbeef);
	dump_buffer();

	/* Pointer parsing, non-NULL (32-bit) pointer. */
	voidptr = (void *) 123;
	ret = duk_minimal_sscanf("0xdeadbeef", "%p", &voidptr);
	printf("ret=%d, void pointer: %p\n", ret, voidptr);

	/* Pointer parsing, NULL (32-bit) pointer. */
	voidptr = (void *) 123;
	ret = duk_minimal_sscanf("0x00000000", "%p", &voidptr);
	printf("ret=%d, void pointer: %p\n", ret, voidptr);

	/* Pointer parsing, non-NULL (32-bit) pointer but garbage follows. */
	voidptr = (void *) 123;
	ret = duk_minimal_sscanf("0xdeadbeefx", "%p", &voidptr);
	printf("ret=%d, void pointer: %p\n", ret, voidptr);

	/* Fixed width test over a range of widths. */
	for (i = 0; i <= 9; i++) {
		char fmtbuf[16];

		printf("--- pos/neg fixed width test, i=%d\n", i);

		/* %0<i>d. %00d makes no sense, but tested anyway. */
		memset((void *) fmtbuf, 0, sizeof(fmtbuf));
		fmtbuf[0] = (char) '%';
		fmtbuf[1] = (char) '0';
		fmtbuf[2] = (char) ('0' + i);
		fmtbuf[3] = 'd';
		init_buffer();
		duk_minimal_sprintf(buffer, (const char *) fmtbuf, 321);
		dump_buffer();
		init_buffer();
		duk_minimal_sprintf(buffer, (const char *) fmtbuf, -321);
		dump_buffer();
		printf("  ==> printf: |");
		printf((const char *) fmtbuf, 321);
		printf("| |");
		printf((const char *) fmtbuf, -321);
		printf("|\n");

		/* %<i>d. */
		memset((void *) fmtbuf, 0, sizeof(fmtbuf));
		fmtbuf[0] = (char) '%';
		fmtbuf[1] = (char) ('0' + i);
		fmtbuf[2] = 'd';
		init_buffer();
		duk_minimal_sprintf(buffer, (const char *) fmtbuf, 321);
		dump_buffer();
		init_buffer();
		duk_minimal_sprintf(buffer, (const char *) fmtbuf, -321);
		dump_buffer();
		printf("  ==> printf: |");
		printf((const char *) fmtbuf, 321);
		printf("| |");
		printf((const char *) fmtbuf, -321);
		printf("|\n");
	}

	return 0;
}
