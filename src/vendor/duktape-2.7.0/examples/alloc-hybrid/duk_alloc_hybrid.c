/*
 *  Example memory allocator with pool allocation for small sizes and
 *  fallback into malloc/realloc/free for larger sizes or when the pools
 *  are exhausted.
 *
 *  Useful to reduce memory churn or work around a platform allocator
 *  that doesn't handle a lot of small allocations efficiently.
 */

#include "duktape.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "duk_alloc_hybrid.h"

/* Define to enable some debug printfs. */
/* #define DUK_ALLOC_HYBRID_DEBUG */

typedef struct {
	size_t size;
	int count;
} pool_size_spec;

static pool_size_spec pool_sizes[] = {
	{ 32, 1024 },
	{ 48, 2048 },
	{ 64, 2048 },
	{ 128, 2048 },
	{ 256, 512 },
	{ 1024, 64 },
	{ 2048, 32 }
};

#define  NUM_POOLS  (sizeof(pool_sizes) / sizeof(pool_size_spec))

/* This must fit into the smallest pool entry. */
struct pool_free_entry;
typedef struct pool_free_entry pool_free_entry;
struct pool_free_entry {
	pool_free_entry *next;
};

typedef struct {
	pool_free_entry *free;
	char *alloc_start;
	char *alloc_end;
	size_t size;
	int count;
} pool_header;

typedef struct {
	pool_header headers[NUM_POOLS];
	size_t pool_max_size;
	char *alloc_start;
	char *alloc_end;
} pool_state;

#define ADDR_IN_STATE_ALLOC(st,p) \
	((char *) (p) >= (st)->alloc_start && (char *) (p) < (st)->alloc_end)
#define ADDR_IN_HEADER_ALLOC(hdr,p) \
	((char *) (p) >= (hdr)->alloc_start && (char *) (p) < (hdr)->alloc_end)

#if defined(DUK_ALLOC_HYBRID_DEBUG)
static void dump_pool_state(pool_state *st) {
	pool_free_entry *free;
	int free_len;
	int i;

	printf("=== Pool state: st=%p\n", (void *) st);
	for (i = 0; i < (int) NUM_POOLS; i++) {
		pool_header *hdr = st->headers + i;

		for (free = hdr->free, free_len = 0; free != NULL; free = free->next) {
			free_len++;
		}

		printf("[%d]: size %ld, count %ld, used %ld, free list len %ld\n",
		       i, (long) hdr->size, (long) hdr->count,
		       (long) (hdr->count - free_len),
		       (long) free_len);
	}
}
#else
static void dump_pool_state(pool_state *st) {
	(void) st;
}
#endif

void *duk_alloc_hybrid_init(void) {
	pool_state *st;
	size_t total_size, max_size;
	int i, j;
	char *p;

	st = (pool_state *) malloc(sizeof(pool_state));
	if (!st) {
		return NULL;
	}
	memset((void *) st, 0, sizeof(pool_state));
	st->alloc_start = NULL;
	st->alloc_end = NULL;

	for (i = 0, total_size = 0, max_size = 0; i < (int) NUM_POOLS; i++) {
#if defined(DUK_ALLOC_HYBRID_DEBUG)
		printf("Pool %d: size %ld, count %ld\n", i, (long) pool_sizes[i].size, (long) pool_sizes[i].count);
#endif
		total_size += pool_sizes[i].size * (size_t) pool_sizes[i].count;
		if (pool_sizes[i].size > max_size) {
			max_size = pool_sizes[i].size;
		}
	}
#if defined(DUK_ALLOC_HYBRID_DEBUG)
	printf("Total size %ld, max pool size %ld\n", (long) total_size, (long) max_size);
#endif

	st->alloc_start = (char *) malloc(total_size);
	if (!st->alloc_start) {
		free(st);
		return NULL;
	}
	st->alloc_end = st->alloc_start + total_size;
	st->pool_max_size = max_size;
	memset((void *) st->alloc_start, 0, total_size);

	for (i = 0, p = st->alloc_start; i < (int) NUM_POOLS; i++) {
		pool_header *hdr = st->headers + i;

		hdr->alloc_start = p;
		hdr->alloc_end = p + pool_sizes[i].size * (size_t) pool_sizes[i].count;
		hdr->free = (pool_free_entry *) (void *) p;
		hdr->size = pool_sizes[i].size;
		hdr->count = pool_sizes[i].count;

		for (j = 0; j < pool_sizes[i].count; j++) {
			pool_free_entry *ent = (pool_free_entry *) (void *) p;
			if (j == pool_sizes[i].count - 1) {
				ent->next = (pool_free_entry *) NULL;
			} else {
				ent->next = (pool_free_entry *) (void *) (p + pool_sizes[i].size);
			}
			p += pool_sizes[i].size;
		}
	}

	dump_pool_state(st);

	/* Use 'st' as udata. */
	return (void *) st;
}

void *duk_alloc_hybrid(void *udata, duk_size_t size) {
	pool_state *st = (pool_state *) udata;
	int i;
	void *new_ptr;

#if 0
	dump_pool_state(st);
#endif

	if (size == 0) {
		return NULL;
	}
	if (size > st->pool_max_size) {
#if defined(DUK_ALLOC_HYBRID_DEBUG)
		printf("alloc fallback: %ld\n", (long) size);
#endif
		return malloc(size);
	}

	for (i = 0; i < (int) NUM_POOLS; i++) {
		pool_header *hdr = st->headers + i;
		if (hdr->size < size) {
			continue;
		}

		if (hdr->free) {
#if 0
			printf("alloc from pool: %ld -> pool size %ld\n", (long) size, (long) hdr->size);
#endif
			new_ptr = (void *) hdr->free;
			hdr->free = hdr->free->next;
			return new_ptr;
		} else {
#if defined(DUK_ALLOC_HYBRID_DEBUG)
			printf("alloc out of pool entries: %ld -> pool size %ld\n", (long) size, (long) hdr->size);
#endif
			break;
		}
	}

#if defined(DUK_ALLOC_HYBRID_DEBUG)
	printf("alloc fallback (out of pool): %ld\n", (long) size);
#endif
	return malloc(size);
}

void *duk_realloc_hybrid(void *udata, void *ptr, duk_size_t size) {
	pool_state *st = (pool_state *) udata;
	void *new_ptr;
	int i;

#if 0
	dump_pool_state(st);
#endif

	if (ADDR_IN_STATE_ALLOC(st, ptr)) {
		/* 'ptr' cannot be NULL. */
		for (i = 0; i < (int) NUM_POOLS; i++) {
			pool_header *hdr = st->headers + i;
			if (ADDR_IN_HEADER_ALLOC(hdr, ptr)) {
				if (size <= hdr->size) {
					/* Still fits, no shrink support. */
#if 0
					printf("realloc original from pool: still fits, size %ld, pool size %ld\n",
					       (long) size, (long) hdr->size);
#endif
					return ptr;
				}

				new_ptr = duk_alloc_hybrid(udata, size);
				if (!new_ptr) {
#if defined(DUK_ALLOC_HYBRID_DEBUG)
					printf("realloc original from pool: needed larger size, failed to alloc\n");
#endif
					return NULL;
				}
				memcpy(new_ptr, ptr, hdr->size);

				((pool_free_entry *) ptr)->next = hdr->free;
				hdr->free = (pool_free_entry *) ptr;
#if 0
				printf("realloc original from pool: size %ld, pool size %ld\n", (long) size, (long) hdr->size);
#endif
				return new_ptr;
			}
		}
#if defined(DUK_ALLOC_HYBRID_DEBUG)
		printf("NEVER HERE\n");
#endif
		return NULL;
	} else if (ptr != NULL) {
		if (size == 0) {
			free(ptr);
			return NULL;
		} else {
#if defined(DUK_ALLOC_HYBRID_DEBUG)
			printf("realloc fallback: size %ld\n", (long) size);
#endif
			return realloc(ptr, size);
		}
	} else {
#if 0
		printf("realloc NULL ptr, call alloc: %ld\n", (long) size);
#endif
		return duk_alloc_hybrid(udata, size);
	}
}

void duk_free_hybrid(void *udata, void *ptr) {
	pool_state *st = (pool_state *) udata;
	int i;

#if 0
	dump_pool_state(st);
#endif

	if (!ADDR_IN_STATE_ALLOC(st, ptr)) {
		if (ptr == NULL) {
			return;
		}
#if 0
		printf("free out of pool: %p\n", (void *) ptr);
#endif
		free(ptr);
		return;
	}

	for (i = 0; i < (int) NUM_POOLS; i++) {
		pool_header *hdr = st->headers + i;
		if (ADDR_IN_HEADER_ALLOC(hdr, ptr)) {
			((pool_free_entry *) ptr)->next = hdr->free;
			hdr->free = (pool_free_entry *) ptr;
#if 0
			printf("free from pool: %p\n", ptr);
#endif
			return;
		}
	}

#if defined(DUK_ALLOC_HYBRID_DEBUG)
	printf("NEVER HERE\n");
#endif
}
