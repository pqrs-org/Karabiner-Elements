/*
 *  String cache.
 *
 *  Provides a cache to optimize indexed string lookups.  The cache keeps
 *  track of (byte offset, char offset) states for a fixed number of strings.
 *  Otherwise we'd need to scan from either end of the string, as we store
 *  strings in (extended) UTF-8.
 */

#include "duk_internal.h"

/*
 *  Delete references to given hstring from the heap string cache.
 *
 *  String cache references are 'weak': they are not counted towards
 *  reference counts, nor serve as roots for mark-and-sweep.  When an
 *  object is about to be freed, such references need to be removed.
 */

DUK_INTERNAL void duk_heap_strcache_string_remove(duk_heap *heap, duk_hstring *h) {
	duk_uint_t i;
	for (i = 0; i < DUK_HEAP_STRCACHE_SIZE; i++) {
		duk_strcache_entry *c = heap->strcache + i;
		if (c->h == h) {
			DUK_DD(
			    DUK_DDPRINT("deleting weak strcache reference to hstring %p from heap %p", (void *) h, (void *) heap));
			c->h = NULL;

			/* XXX: the string shouldn't appear twice, but we now loop to the
			 * end anyway; if fixed, add a looping assertion to ensure there
			 * is no duplicate.
			 */
		}
	}
}

/*
 *  String scanning helpers
 *
 *  All bytes other than UTF-8 continuation bytes ([0x80,0xbf]) are
 *  considered to contribute a character.  This must match how string
 *  character length is computed.
 */

DUK_LOCAL const duk_uint8_t *duk__scan_forwards(const duk_uint8_t *p, const duk_uint8_t *q, duk_uint_fast32_t n) {
	while (n > 0) {
		for (;;) {
			p++;
			if (p >= q) {
				return NULL;
			}
			if ((*p & 0xc0) != 0x80) {
				break;
			}
		}
		n--;
	}
	return p;
}

DUK_LOCAL const duk_uint8_t *duk__scan_backwards(const duk_uint8_t *p, const duk_uint8_t *q, duk_uint_fast32_t n) {
	while (n > 0) {
		for (;;) {
			p--;
			if (p < q) {
				return NULL;
			}
			if ((*p & 0xc0) != 0x80) {
				break;
			}
		}
		n--;
	}
	return p;
}

/*
 *  Convert char offset to byte offset
 *
 *  Avoid using the string cache if possible: for ASCII strings byte and
 *  char offsets are equal and for short strings direct scanning may be
 *  better than using the string cache (which may evict a more important
 *  entry).
 *
 *  Typing now assumes 32-bit string byte/char offsets (duk_uint_fast32_t).
 *  Better typing might be to use duk_size_t.
 *
 *  Caller should ensure 'char_offset' is within the string bounds [0,charlen]
 *  (endpoint is inclusive).  If this is not the case, no memory unsafe
 *  behavior will happen but an error will be thrown.
 */

DUK_INTERNAL duk_uint_fast32_t duk_heap_strcache_offset_char2byte(duk_hthread *thr, duk_hstring *h, duk_uint_fast32_t char_offset) {
	duk_heap *heap;
	duk_strcache_entry *sce;
	duk_uint_fast32_t byte_offset;
	duk_uint_t i;
	duk_bool_t use_cache;
	duk_uint_fast32_t dist_start, dist_end, dist_sce;
	duk_uint_fast32_t char_length;
	const duk_uint8_t *p_start;
	const duk_uint8_t *p_end;
	const duk_uint8_t *p_found;

	/*
	 *  For ASCII strings, the answer is simple.
	 */

	if (DUK_LIKELY(DUK_HSTRING_IS_ASCII(h))) {
		return char_offset;
	}

	char_length = (duk_uint_fast32_t) DUK_HSTRING_GET_CHARLEN(h);
	DUK_ASSERT(char_offset <= char_length);

	if (DUK_LIKELY(DUK_HSTRING_IS_ASCII(h))) {
		/* Must recheck because the 'is ascii' flag may be set
		 * lazily.  Alternatively, we could just compare charlen
		 * to bytelen.
		 */
		return char_offset;
	}

	/*
	 *  For non-ASCII strings, we need to scan forwards or backwards
	 *  from some starting point.  The starting point may be the start
	 *  or end of the string, or some cached midpoint in the string
	 *  cache.
	 *
	 *  For "short" strings we simply scan without checking or updating
	 *  the cache.  For longer strings we check and update the cache as
	 *  necessary, inserting a new cache entry if none exists.
	 */

	DUK_DDD(DUK_DDDPRINT("non-ascii string %p, char_offset=%ld, clen=%ld, blen=%ld",
	                     (void *) h,
	                     (long) char_offset,
	                     (long) DUK_HSTRING_GET_CHARLEN(h),
	                     (long) DUK_HSTRING_GET_BYTELEN(h)));

	heap = thr->heap;
	sce = NULL;
	use_cache = (char_length > DUK_HEAP_STRINGCACHE_NOCACHE_LIMIT);

	if (use_cache) {
#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 2)
		DUK_DDD(DUK_DDDPRINT("stringcache before char2byte (using cache):"));
		for (i = 0; i < DUK_HEAP_STRCACHE_SIZE; i++) {
			duk_strcache_entry *c = heap->strcache + i;
			DUK_DDD(DUK_DDDPRINT("  [%ld] -> h=%p, cidx=%ld, bidx=%ld",
			                     (long) i,
			                     (void *) c->h,
			                     (long) c->cidx,
			                     (long) c->bidx));
		}
#endif

		for (i = 0; i < DUK_HEAP_STRCACHE_SIZE; i++) {
			duk_strcache_entry *c = heap->strcache + i;

			if (c->h == h) {
				sce = c;
				break;
			}
		}
	}

	/*
	 *  Scan from shortest distance:
	 *    - start of string
	 *    - end of string
	 *    - cache entry (if exists)
	 */

	DUK_ASSERT(DUK_HSTRING_GET_CHARLEN(h) >= char_offset);
	dist_start = char_offset;
	dist_end = char_length - char_offset;
	dist_sce = 0;
	DUK_UNREF(dist_sce); /* initialize for debug prints, needed if sce==NULL */

	p_start = (const duk_uint8_t *) DUK_HSTRING_GET_DATA(h);
	p_end = (const duk_uint8_t *) (p_start + DUK_HSTRING_GET_BYTELEN(h));
	p_found = NULL;

	if (sce) {
		if (char_offset >= sce->cidx) {
			dist_sce = char_offset - sce->cidx;
			if ((dist_sce <= dist_start) && (dist_sce <= dist_end)) {
				DUK_DDD(DUK_DDDPRINT("non-ascii string, use_cache=%ld, sce=%p:%ld:%ld, "
				                     "dist_start=%ld, dist_end=%ld, dist_sce=%ld => "
				                     "scan forwards from sce",
				                     (long) use_cache,
				                     (void *) (sce ? sce->h : NULL),
				                     (sce ? (long) sce->cidx : (long) -1),
				                     (sce ? (long) sce->bidx : (long) -1),
				                     (long) dist_start,
				                     (long) dist_end,
				                     (long) dist_sce));

				p_found = duk__scan_forwards(p_start + sce->bidx, p_end, dist_sce);
				goto scan_done;
			}
		} else {
			dist_sce = sce->cidx - char_offset;
			if ((dist_sce <= dist_start) && (dist_sce <= dist_end)) {
				DUK_DDD(DUK_DDDPRINT("non-ascii string, use_cache=%ld, sce=%p:%ld:%ld, "
				                     "dist_start=%ld, dist_end=%ld, dist_sce=%ld => "
				                     "scan backwards from sce",
				                     (long) use_cache,
				                     (void *) (sce ? sce->h : NULL),
				                     (sce ? (long) sce->cidx : (long) -1),
				                     (sce ? (long) sce->bidx : (long) -1),
				                     (long) dist_start,
				                     (long) dist_end,
				                     (long) dist_sce));

				p_found = duk__scan_backwards(p_start + sce->bidx, p_start, dist_sce);
				goto scan_done;
			}
		}
	}

	/* no sce, or sce scan not best */

	if (dist_start <= dist_end) {
		DUK_DDD(DUK_DDDPRINT("non-ascii string, use_cache=%ld, sce=%p:%ld:%ld, "
		                     "dist_start=%ld, dist_end=%ld, dist_sce=%ld => "
		                     "scan forwards from string start",
		                     (long) use_cache,
		                     (void *) (sce ? sce->h : NULL),
		                     (sce ? (long) sce->cidx : (long) -1),
		                     (sce ? (long) sce->bidx : (long) -1),
		                     (long) dist_start,
		                     (long) dist_end,
		                     (long) dist_sce));

		p_found = duk__scan_forwards(p_start, p_end, dist_start);
	} else {
		DUK_DDD(DUK_DDDPRINT("non-ascii string, use_cache=%ld, sce=%p:%ld:%ld, "
		                     "dist_start=%ld, dist_end=%ld, dist_sce=%ld => "
		                     "scan backwards from string end",
		                     (long) use_cache,
		                     (void *) (sce ? sce->h : NULL),
		                     (sce ? (long) sce->cidx : (long) -1),
		                     (sce ? (long) sce->bidx : (long) -1),
		                     (long) dist_start,
		                     (long) dist_end,
		                     (long) dist_sce));

		p_found = duk__scan_backwards(p_end, p_start, dist_end);
	}

scan_done:

	if (DUK_UNLIKELY(p_found == NULL)) {
		/* Scan error: this shouldn't normally happen; it could happen if
		 * string is not valid UTF-8 data, and clen/blen are not consistent
		 * with the scanning algorithm.
		 */
		goto scan_error;
	}

	DUK_ASSERT(p_found >= p_start);
	DUK_ASSERT(p_found <= p_end); /* may be equal */
	byte_offset = (duk_uint32_t) (p_found - p_start);

	DUK_DDD(DUK_DDDPRINT("-> string %p, cidx %ld -> bidx %ld", (void *) h, (long) char_offset, (long) byte_offset));

	/*
	 *  Update cache entry (allocating if necessary), and move the
	 *  cache entry to the first place (in an "LRU" policy).
	 */

	if (use_cache) {
		/* update entry, allocating if necessary */
		if (!sce) {
			sce = heap->strcache + DUK_HEAP_STRCACHE_SIZE - 1; /* take last entry */
			sce->h = h;
		}
		DUK_ASSERT(sce != NULL);
		sce->bidx = (duk_uint32_t) (p_found - p_start);
		sce->cidx = (duk_uint32_t) char_offset;

		/* LRU: move our entry to first */
		if (sce > &heap->strcache[0]) {
			/*
			 *   A                  C
			 *   B                  A
			 *   C <- sce    ==>    B
			 *   D                  D
			 */
			duk_strcache_entry tmp;

			tmp = *sce;
			duk_memmove((void *) (&heap->strcache[1]),
			            (const void *) (&heap->strcache[0]),
			            (size_t) (((char *) sce) - ((char *) &heap->strcache[0])));
			heap->strcache[0] = tmp;

			/* 'sce' points to the wrong entry here, but is no longer used */
		}
#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 2)
		DUK_DDD(DUK_DDDPRINT("stringcache after char2byte (using cache):"));
		for (i = 0; i < DUK_HEAP_STRCACHE_SIZE; i++) {
			duk_strcache_entry *c = heap->strcache + i;
			DUK_DDD(DUK_DDDPRINT("  [%ld] -> h=%p, cidx=%ld, bidx=%ld",
			                     (long) i,
			                     (void *) c->h,
			                     (long) c->cidx,
			                     (long) c->bidx));
		}
#endif
	}

	return byte_offset;

scan_error:
	DUK_ERROR_INTERNAL(thr);
	DUK_WO_NORETURN(return 0;);
}
