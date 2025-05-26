/*
 *  Example torture memory allocator with memory wiping and check for
 *  out-of-bounds writes.
 *
 *  Allocation structure:
 *
 *    [ alloc_hdr | red zone before | user area | red zone after ]
 *
 *     ^                             ^
 *     |                             `--- pointer returned to Duktape
 *     `--- underlying malloc ptr
 */

#include "duktape.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "duk_alloc_torture.h"

#define  RED_ZONE_SIZE  16
#define  RED_ZONE_BYTE  0x5a
#define  INIT_BYTE      0xa5
#define  WIPE_BYTE      0x27

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

static void check_red_zone(alloc_hdr *hdr) {
	size_t size;
	int i;
	int err;
	unsigned char *p;
	unsigned char *userptr;

	size = hdr->u.sz;
	userptr = (unsigned char *) hdr + sizeof(alloc_hdr) + RED_ZONE_SIZE;

	err = 0;
	p = (unsigned char *) hdr + sizeof(alloc_hdr);
	for (i = 0; i < RED_ZONE_SIZE; i++) {
		if (p[i] != RED_ZONE_BYTE) {
			err = 1;
		}
	}
	if (err) {
		fprintf(stderr, "RED ZONE CORRUPTED BEFORE ALLOC: hdr=%p ptr=%p size=%ld\n",
		        (void *) hdr, (void *) userptr, (long) size);
		fflush(stderr);
	}

	err = 0;
	p = (unsigned char *) hdr + sizeof(alloc_hdr) + RED_ZONE_SIZE + size;
	for (i = 0; i < RED_ZONE_SIZE; i++) {
		if (p[i] != RED_ZONE_BYTE) {
			err = 1;
		}
	}
	if (err) {
		fprintf(stderr, "RED ZONE CORRUPTED AFTER ALLOC: hdr=%p ptr=%p size=%ld\n",
		        (void *) hdr, (void *) userptr, (long) size);
		fflush(stderr);
	}
}

void *duk_alloc_torture(void *udata, duk_size_t size) {
	unsigned char *p;

	(void) udata;  /* Suppress warning. */

	if (size == 0) {
		return NULL;
	}

	p = (unsigned char *) malloc(size + sizeof(alloc_hdr) + 2 * RED_ZONE_SIZE);
	if (!p) {
		return NULL;
	}

	((alloc_hdr *) (void *) p)->u.sz = size;
	p += sizeof(alloc_hdr);
	memset((void *) p, RED_ZONE_BYTE, RED_ZONE_SIZE);
	p += RED_ZONE_SIZE;
	memset((void *) p, INIT_BYTE, size);
	p += size;
	memset((void *) p, RED_ZONE_BYTE, RED_ZONE_SIZE);
	p -= size;
	return (void *) p;
}

void *duk_realloc_torture(void *udata, void *ptr, duk_size_t size) {
	unsigned char *p, *old_p;
	size_t old_size;

	(void) udata;  /* Suppress warning. */

	/* Handle the ptr-NULL vs. size-zero cases explicitly to minimize
	 * platform assumptions.  You can get away with much less in specific
	 * well-behaving environments.
	 */

	if (ptr) {
		old_p = (unsigned char *) ptr - sizeof(alloc_hdr) - RED_ZONE_SIZE;
		old_size = ((alloc_hdr *) (void *) old_p)->u.sz;
		check_red_zone((alloc_hdr *) (void *) old_p);

		if (size == 0) {
			memset((void *) old_p, WIPE_BYTE, old_size + sizeof(alloc_hdr) + 2 * RED_ZONE_SIZE);
			free((void *) old_p);
			return NULL;
		} else {
			/* Force address change on every realloc. */
			p = (unsigned char *) malloc(size + sizeof(alloc_hdr) + 2 * RED_ZONE_SIZE);
			if (!p) {
				return NULL;
			}

			((alloc_hdr *) (void *) p)->u.sz = size;
			p += sizeof(alloc_hdr);
			memset((void *) p, RED_ZONE_BYTE, RED_ZONE_SIZE);
			p += RED_ZONE_SIZE;
			if (size > old_size) {
				memcpy((void *) p, (void *) (old_p + sizeof(alloc_hdr) + RED_ZONE_SIZE), old_size);
				memset((void *) (p + old_size), INIT_BYTE, size - old_size);
			} else {
				memcpy((void *) p, (void *) (old_p + sizeof(alloc_hdr) + RED_ZONE_SIZE), size);
			}
			p += size;
			memset((void *) p, RED_ZONE_BYTE, RED_ZONE_SIZE);
			p -= size;

			memset((void *) old_p, WIPE_BYTE, old_size + sizeof(alloc_hdr) + 2 * RED_ZONE_SIZE);
			free((void *) old_p);

			return (void *) p;
		}
	} else {
		if (size == 0) {
			return NULL;
		} else {
			p = (unsigned char *) malloc(size + sizeof(alloc_hdr) + 2 * RED_ZONE_SIZE);
			if (!p) {
				return NULL;
			}

			((alloc_hdr *) (void *) p)->u.sz = size;
			p += sizeof(alloc_hdr);
			memset((void *) p, RED_ZONE_BYTE, RED_ZONE_SIZE);
			p += RED_ZONE_SIZE;
			memset((void *) p, INIT_BYTE, size);
			p += size;
			memset((void *) p, RED_ZONE_BYTE, RED_ZONE_SIZE);
			p -= size;
			return (void *) p;
		}
	}
}

void duk_free_torture(void *udata, void *ptr) {
	unsigned char *p;
	size_t old_size;

	(void) udata;  /* Suppress warning. */

	if (!ptr) {
		return;
	}

	p = (unsigned char *) ptr - sizeof(alloc_hdr) - RED_ZONE_SIZE;
	old_size = ((alloc_hdr *) (void *) p)->u.sz;

	check_red_zone((alloc_hdr *) (void *) p);
	memset((void *) p, WIPE_BYTE, old_size + sizeof(alloc_hdr) + 2 * RED_ZONE_SIZE);
	free((void *) p);
}
