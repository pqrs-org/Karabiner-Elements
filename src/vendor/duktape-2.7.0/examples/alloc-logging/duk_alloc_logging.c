/*
 *  Example memory allocator with machine parseable logging.
 *
 *  Also sizes for reallocs and frees are logged so that the memory
 *  behavior can be essentially replayed to accurately determine e.g.
 *  optimal pool sizes for a pooled allocator.
 *
 *  Allocation structure:
 *
 *    [ alloc_hdr | user area ]
 *
 *     ^           ^
 *     |           `--- pointer returned to Duktape
 *     `--- underlying malloc ptr
 */

#include "duktape.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "duk_alloc_logging.h"

#define  ALLOC_LOG_FILE  "/tmp/duk-alloc-log.txt"

typedef struct {
	/* The double value in the union is there to ensure alignment is
	 * good for IEEE doubles too.  In many 32-bit environments 4 bytes
	 * would be sufficiently aligned and the double value is unnecessary.
	 */
	union {
		size_t sz;
		double d;
	} u;
} alloc_hdr;

static FILE *log_file = NULL;

static void write_log(const char *fmt, ...) {
	va_list ap;

	if (!log_file) {
		log_file = fopen(ALLOC_LOG_FILE, "wb");
		if (!log_file) {
			return;
		}
	}

	va_start(ap, fmt);
	vfprintf(log_file, fmt, ap);
	va_end(ap);
}

void *duk_alloc_logging(void *udata, duk_size_t size) {
	alloc_hdr *hdr;
	void *ret;

	(void) udata;  /* Suppress warning. */

	if (size == 0) {
		write_log("A NULL %ld\n", (long) size);
		return NULL;
	}

	hdr = (alloc_hdr *) malloc(size + sizeof(alloc_hdr));
	if (!hdr) {
		write_log("A FAIL %ld\n", (long) size);
		return NULL;
	}
	hdr->u.sz = size;
	ret = (void *) (hdr + 1);
	write_log("A %p %ld\n", ret, (long) size);
	return ret;
}

void *duk_realloc_logging(void *udata, void *ptr, duk_size_t size) {
	alloc_hdr *hdr;
	size_t old_size;
	void *t;
	void *ret;

	(void) udata;  /* Suppress warning. */

	/* Handle the ptr-NULL vs. size-zero cases explicitly to minimize
	 * platform assumptions.  You can get away with much less in specific
	 * well-behaving environments.
	 */

	if (ptr) {
		hdr = (alloc_hdr *) (void *) ((unsigned char *) ptr - sizeof(alloc_hdr));
		old_size = hdr->u.sz;

		if (size == 0) {
			write_log("R %p %ld NULL 0\n", ptr, (long) old_size);
			free((void *) hdr);
			return NULL;
		} else {
			t = realloc((void *) hdr, size + sizeof(alloc_hdr));
			if (!t) {
				write_log("R %p %ld FAIL %ld\n", ptr, (long) old_size, (long) size);
				return NULL;
			}
			hdr = (alloc_hdr *) t;
			hdr->u.sz = size;
			ret = (void *) (hdr + 1);
			write_log("R %p %ld %p %ld\n", ptr, (long) old_size, ret, (long) size);
			return ret;
		}
	} else {
		if (size == 0) {
			write_log("R NULL 0 NULL 0\n");
			return NULL;
		} else {
			hdr = (alloc_hdr *) malloc(size + sizeof(alloc_hdr));
			if (!hdr) {
				write_log("R NULL 0 FAIL %ld\n", (long) size);
				return NULL;
			}
			hdr->u.sz = size;
			ret = (void *) (hdr + 1);
			write_log("R NULL 0 %p %ld\n", ret, (long) size);
			return ret;
		}
	}
}

void duk_free_logging(void *udata, void *ptr) {
	alloc_hdr *hdr;

	(void) udata;  /* Suppress warning. */

	if (!ptr) {
		write_log("F NULL 0\n");
		return;
	}
	hdr = (alloc_hdr *) (void *) ((unsigned char *) ptr - sizeof(alloc_hdr));
	write_log("F %p %ld\n", ptr, (long) hdr->u.sz);
	free((void *) hdr);
}
