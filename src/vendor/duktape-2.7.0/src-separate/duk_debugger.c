/*
 *  Duktape debugger
 */

#include "duk_internal.h"

#if defined(DUK_USE_DEBUGGER_SUPPORT)

/*
 *  Assert helpers
 */

#if defined(DUK_USE_ASSERTIONS)
#define DUK__DBG_TPORT_ENTER() \
	do { \
		DUK_ASSERT(heap->dbg_calling_transport == 0); \
		heap->dbg_calling_transport = 1; \
	} while (0)
#define DUK__DBG_TPORT_EXIT() \
	do { \
		DUK_ASSERT(heap->dbg_calling_transport == 1); \
		heap->dbg_calling_transport = 0; \
	} while (0)
#else
#define DUK__DBG_TPORT_ENTER() \
	do { \
	} while (0)
#define DUK__DBG_TPORT_EXIT() \
	do { \
	} while (0)
#endif

/*
 *  Helper structs
 */

typedef union {
	void *p;
	duk_uint_t b[1];
	/* Use b[] to access the size of the union, which is strictly not
	 * correct.  Can't use fixed size unless there's feature detection
	 * for pointer byte size.
	 */
} duk__ptr_union;

/*
 *  Detach handling
 */

#define DUK__SET_CONN_BROKEN(thr, reason) \
	do { \
		/* For now shared handler is fine. */ \
		duk__debug_do_detach1((thr)->heap, (reason)); \
	} while (0)

DUK_LOCAL void duk__debug_do_detach1(duk_heap *heap, duk_int_t reason) {
	/* Can be called multiple times with no harm.  Mark the transport
	 * bad (dbg_read_cb == NULL) and clear state except for the detached
	 * callback and the udata field.  The detached callback is delayed
	 * to the message loop so that it can be called between messages;
	 * this avoids corner cases related to immediate debugger reattach
	 * inside the detached callback.
	 */

	if (heap->dbg_detaching) {
		DUK_D(DUK_DPRINT("debugger already detaching, ignore detach1"));
		return;
	}

	DUK_D(DUK_DPRINT("debugger transport detaching, marking transport broken"));

	heap->dbg_detaching = 1; /* prevent multiple in-progress detaches */

	if (heap->dbg_write_cb != NULL) {
		duk_hthread *thr;

		thr = heap->heap_thread;
		DUK_ASSERT(thr != NULL);

		duk_debug_write_notify(thr, DUK_DBG_CMD_DETACHING);
		duk_debug_write_int(thr, reason);
		duk_debug_write_eom(thr);
	}

	heap->dbg_read_cb = NULL;
	heap->dbg_write_cb = NULL;
	heap->dbg_peek_cb = NULL;
	heap->dbg_read_flush_cb = NULL;
	heap->dbg_write_flush_cb = NULL;
	heap->dbg_request_cb = NULL;
	/* heap->dbg_detached_cb: keep */
	/* heap->dbg_udata: keep */
	/* heap->dbg_processing: keep on purpose to avoid debugger re-entry in detaching state */
	heap->dbg_state_dirty = 0;
	heap->dbg_force_restart = 0;
	heap->dbg_pause_flags = 0;
	heap->dbg_pause_act = NULL;
	heap->dbg_pause_startline = 0;
	heap->dbg_have_next_byte = 0;
	duk_debug_clear_paused(heap); /* XXX: some overlap with field inits above */
	heap->dbg_state_dirty = 0; /* XXX: clear_paused sets dirty; rework? */

	/* Ensure there are no stale active breakpoint pointers.
	 * Breakpoint list is currently kept - we could empty it
	 * here but we'd need to handle refcounts correctly, and
	 * we'd need a 'thr' reference for that.
	 *
	 * XXX: clear breakpoint on either attach or detach?
	 */
	heap->dbg_breakpoints_active[0] = (duk_breakpoint *) NULL;
}

DUK_LOCAL void duk__debug_do_detach2(duk_heap *heap) {
	duk_debug_detached_function detached_cb;
	void *detached_udata;
	duk_hthread *thr;

	thr = heap->heap_thread;
	if (thr == NULL) {
		DUK_ASSERT(heap->dbg_detached_cb == NULL);
		return;
	}

	/* Safe to call multiple times. */

	detached_cb = heap->dbg_detached_cb;
	detached_udata = heap->dbg_udata;
	heap->dbg_detached_cb = NULL;
	heap->dbg_udata = NULL;

	if (detached_cb) {
		/* Careful here: state must be wiped before the call
		 * so that we can cleanly handle a re-attach from
		 * inside the callback.
		 */
		DUK_D(DUK_DPRINT("detached during message loop, delayed call to detached_cb"));
		detached_cb(thr, detached_udata);
	}

	heap->dbg_detaching = 0;
}

DUK_INTERNAL void duk_debug_do_detach(duk_heap *heap) {
	duk__debug_do_detach1(heap, 0);
	duk__debug_do_detach2(heap);
}

/* Called on a read/write error: NULL all callbacks except the detached
 * callback so that we never accidentally call them after a read/write
 * error has been indicated.  This is especially important for the transport
 * I/O callbacks to fulfill guaranteed callback semantics.
 */
DUK_LOCAL void duk__debug_null_most_callbacks(duk_hthread *thr) {
	duk_heap *heap;

	DUK_ASSERT(thr != NULL);

	heap = thr->heap;
	DUK_D(DUK_DPRINT("transport read/write error, NULL all callbacks expected detached"));
	heap->dbg_read_cb = NULL;
	heap->dbg_write_cb = NULL; /* this is especially critical to avoid another write call in detach1() */
	heap->dbg_peek_cb = NULL;
	heap->dbg_read_flush_cb = NULL;
	heap->dbg_write_flush_cb = NULL;
	heap->dbg_request_cb = NULL;
	/* keep heap->dbg_detached_cb */
}

/*
 *  Pause handling
 */

DUK_LOCAL void duk__debug_set_pause_state(duk_hthread *thr, duk_heap *heap, duk_small_uint_t pause_flags) {
	duk_uint_fast32_t line;

	line = duk_debug_curr_line(thr);
	if (line == 0) {
		/* No line info for current function. */
		duk_small_uint_t updated_flags;

		updated_flags = pause_flags & ~(DUK_PAUSE_FLAG_LINE_CHANGE);
		DUK_D(DUK_DPRINT("no line info for current activation, disable line-based pause flags: 0x%08lx -> 0x%08lx",
		                 (long) pause_flags,
		                 (long) updated_flags));
		pause_flags = updated_flags;
	}

	heap->dbg_pause_flags = pause_flags;
	heap->dbg_pause_act = thr->callstack_curr;
	heap->dbg_pause_startline = (duk_uint32_t) line;
	heap->dbg_state_dirty = 1;

	DUK_D(DUK_DPRINT("set state for automatic pause triggers, flags=0x%08lx, act=%p, startline=%ld",
	                 (long) heap->dbg_pause_flags,
	                 (void *) heap->dbg_pause_act,
	                 (long) heap->dbg_pause_startline));
}

/*
 *  Debug connection peek and flush primitives
 */

DUK_INTERNAL duk_bool_t duk_debug_read_peek(duk_hthread *thr) {
	duk_heap *heap;
	duk_bool_t ret;

	DUK_ASSERT(thr != NULL);
	heap = thr->heap;
	DUK_ASSERT(heap != NULL);

	if (heap->dbg_read_cb == NULL) {
		DUK_D(DUK_DPRINT("attempt to peek in detached state, return zero (= no data)"));
		return 0;
	}
	if (heap->dbg_peek_cb == NULL) {
		DUK_DD(DUK_DDPRINT("no peek callback, return zero (= no data)"));
		return 0;
	}

	DUK__DBG_TPORT_ENTER();
	ret = (duk_bool_t) (heap->dbg_peek_cb(heap->dbg_udata) > 0);
	DUK__DBG_TPORT_EXIT();
	return ret;
}

DUK_INTERNAL void duk_debug_read_flush(duk_hthread *thr) {
	duk_heap *heap;

	DUK_ASSERT(thr != NULL);
	heap = thr->heap;
	DUK_ASSERT(heap != NULL);

	if (heap->dbg_read_cb == NULL) {
		DUK_D(DUK_DPRINT("attempt to read flush in detached state, ignore"));
		return;
	}
	if (heap->dbg_read_flush_cb == NULL) {
		DUK_DD(DUK_DDPRINT("no read flush callback, ignore"));
		return;
	}

	DUK__DBG_TPORT_ENTER();
	heap->dbg_read_flush_cb(heap->dbg_udata);
	DUK__DBG_TPORT_EXIT();
}

DUK_INTERNAL void duk_debug_write_flush(duk_hthread *thr) {
	duk_heap *heap;

	DUK_ASSERT(thr != NULL);
	heap = thr->heap;
	DUK_ASSERT(heap != NULL);

	if (heap->dbg_read_cb == NULL) {
		DUK_D(DUK_DPRINT("attempt to write flush in detached state, ignore"));
		return;
	}
	if (heap->dbg_write_flush_cb == NULL) {
		DUK_DD(DUK_DDPRINT("no write flush callback, ignore"));
		return;
	}

	DUK__DBG_TPORT_ENTER();
	heap->dbg_write_flush_cb(heap->dbg_udata);
	DUK__DBG_TPORT_EXIT();
}

/*
 *  Debug connection skip primitives
 */

/* Skip fully. */
DUK_INTERNAL void duk_debug_skip_bytes(duk_hthread *thr, duk_size_t length) {
	duk_uint8_t dummy[64];
	duk_size_t now;

	DUK_ASSERT(thr != NULL);

	while (length > 0) {
		now = (length > sizeof(dummy) ? sizeof(dummy) : length);
		duk_debug_read_bytes(thr, dummy, now);
		length -= now;
	}
}

DUK_INTERNAL void duk_debug_skip_byte(duk_hthread *thr) {
	DUK_ASSERT(thr != NULL);

	(void) duk_debug_read_byte(thr);
}

/*
 *  Debug connection read primitives
 */

/* Peek ahead in the stream one byte. */
DUK_INTERNAL uint8_t duk_debug_peek_byte(duk_hthread *thr) {
	/* It is important not to call this if the last byte read was an EOM.
	 * Reading ahead in this scenario would cause unnecessary blocking if
	 * another message is not available.
	 */

	duk_uint8_t x;

	x = duk_debug_read_byte(thr);
	thr->heap->dbg_have_next_byte = 1;
	thr->heap->dbg_next_byte = x;
	return x;
}

/* Read fully. */
DUK_INTERNAL void duk_debug_read_bytes(duk_hthread *thr, duk_uint8_t *data, duk_size_t length) {
	duk_heap *heap;
	duk_uint8_t *p;
	duk_size_t left;
	duk_size_t got;

	DUK_ASSERT(thr != NULL);
	heap = thr->heap;
	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(data != NULL);

	if (heap->dbg_read_cb == NULL) {
		DUK_D(DUK_DPRINT("attempt to read %ld bytes in detached state, return zero data", (long) length));
		goto fail;
	}

	/* NOTE: length may be zero */
	p = data;
	if (length >= 1 && heap->dbg_have_next_byte) {
		heap->dbg_have_next_byte = 0;
		*p++ = heap->dbg_next_byte;
	}
	for (;;) {
		left = (duk_size_t) ((data + length) - p);
		if (left == 0) {
			break;
		}
		DUK_ASSERT(heap->dbg_read_cb != NULL);
		DUK_ASSERT(left >= 1);
#if defined(DUK_USE_DEBUGGER_TRANSPORT_TORTURE)
		left = 1;
#endif
		DUK__DBG_TPORT_ENTER();
		got = heap->dbg_read_cb(heap->dbg_udata, (char *) p, left);
		DUK__DBG_TPORT_EXIT();

		if (got == 0 || got > left) {
			DUK_D(DUK_DPRINT("connection error during read, return zero data"));
			duk__debug_null_most_callbacks(thr); /* avoid calling write callback in detach1() */
			DUK__SET_CONN_BROKEN(thr, 1);
			goto fail;
		}
		p += got;
	}
	return;

fail:
	duk_memzero((void *) data, (size_t) length);
}

DUK_INTERNAL duk_uint8_t duk_debug_read_byte(duk_hthread *thr) {
	duk_uint8_t x;

	x = 0; /* just in case callback is broken and won't write 'x' */
	duk_debug_read_bytes(thr, &x, 1);
	return x;
}

DUK_LOCAL duk_uint32_t duk__debug_read_uint32_raw(duk_hthread *thr) {
	duk_uint8_t buf[4];

	DUK_ASSERT(thr != NULL);

	duk_debug_read_bytes(thr, buf, 4);
	return ((duk_uint32_t) buf[0] << 24) | ((duk_uint32_t) buf[1] << 16) | ((duk_uint32_t) buf[2] << 8) | (duk_uint32_t) buf[3];
}

DUK_LOCAL duk_int32_t duk__debug_read_int32_raw(duk_hthread *thr) {
	return (duk_int32_t) duk__debug_read_uint32_raw(thr);
}

DUK_LOCAL duk_uint16_t duk__debug_read_uint16_raw(duk_hthread *thr) {
	duk_uint8_t buf[2];

	DUK_ASSERT(thr != NULL);

	duk_debug_read_bytes(thr, buf, 2);
	return ((duk_uint16_t) buf[0] << 8) | (duk_uint16_t) buf[1];
}

DUK_INTERNAL duk_int32_t duk_debug_read_int(duk_hthread *thr) {
	duk_small_uint_t x;
	duk_small_uint_t t;

	DUK_ASSERT(thr != NULL);

	x = duk_debug_read_byte(thr);
	if (x >= 0xc0) {
		t = duk_debug_read_byte(thr);
		return (duk_int32_t) (((x - 0xc0) << 8) + t);
	} else if (x >= 0x80) {
		return (duk_int32_t) (x - 0x80);
	} else if (x == DUK_DBG_IB_INT4) {
		return (duk_int32_t) duk__debug_read_uint32_raw(thr);
	}

	DUK_D(DUK_DPRINT("debug connection error: failed to decode int"));
	DUK__SET_CONN_BROKEN(thr, 1);
	return 0;
}

DUK_LOCAL duk_hstring *duk__debug_read_hstring_raw(duk_hthread *thr, duk_uint32_t len) {
	duk_uint8_t buf[31];
	duk_uint8_t *p;

	if (len <= sizeof(buf)) {
		duk_debug_read_bytes(thr, buf, (duk_size_t) len);
		duk_push_lstring(thr, (const char *) buf, (duk_size_t) len);
	} else {
		p = (duk_uint8_t *) duk_push_fixed_buffer(thr, (duk_size_t) len); /* zero for paranoia */
		DUK_ASSERT(p != NULL);
		duk_debug_read_bytes(thr, p, (duk_size_t) len);
		(void) duk_buffer_to_string(thr, -1); /* Safety relies on debug client, which is OK. */
	}

	return duk_require_hstring(thr, -1);
}

DUK_INTERNAL duk_hstring *duk_debug_read_hstring(duk_hthread *thr) {
	duk_small_uint_t x;
	duk_uint32_t len;

	DUK_ASSERT(thr != NULL);

	x = duk_debug_read_byte(thr);
	if (x >= 0x60 && x <= 0x7f) {
		/* For short strings, use a fixed temp buffer. */
		len = (duk_uint32_t) (x - 0x60);
	} else if (x == DUK_DBG_IB_STR2) {
		len = (duk_uint32_t) duk__debug_read_uint16_raw(thr);
	} else if (x == DUK_DBG_IB_STR4) {
		len = (duk_uint32_t) duk__debug_read_uint32_raw(thr);
	} else {
		goto fail;
	}

	return duk__debug_read_hstring_raw(thr, len);

fail:
	DUK_D(DUK_DPRINT("debug connection error: failed to decode int"));
	DUK__SET_CONN_BROKEN(thr, 1);
	duk_push_hstring_empty(thr); /* always push some string */
	return duk_require_hstring(thr, -1);
}

DUK_LOCAL duk_hbuffer *duk__debug_read_hbuffer_raw(duk_hthread *thr, duk_uint32_t len) {
	duk_uint8_t *p;

	p = (duk_uint8_t *) duk_push_fixed_buffer(thr, (duk_size_t) len); /* zero for paranoia */
	DUK_ASSERT(p != NULL);
	duk_debug_read_bytes(thr, p, (duk_size_t) len);

	return duk_require_hbuffer(thr, -1);
}

DUK_LOCAL void *duk__debug_read_pointer_raw(duk_hthread *thr) {
	duk_small_uint_t x;
	duk__ptr_union pu;

	DUK_ASSERT(thr != NULL);

	x = duk_debug_read_byte(thr);
	if (x != sizeof(pu)) {
		goto fail;
	}
	duk_debug_read_bytes(thr, (duk_uint8_t *) &pu.p, sizeof(pu));
#if defined(DUK_USE_INTEGER_LE)
	duk_byteswap_bytes((duk_uint8_t *) pu.b, sizeof(pu));
#endif
	return (void *) pu.p;

fail:
	DUK_D(DUK_DPRINT("debug connection error: failed to decode pointer"));
	DUK__SET_CONN_BROKEN(thr, 1);
	return (void *) NULL;
}

DUK_LOCAL duk_double_t duk__debug_read_double_raw(duk_hthread *thr) {
	duk_double_union du;

	DUK_ASSERT(sizeof(du.uc) == 8);
	duk_debug_read_bytes(thr, (duk_uint8_t *) du.uc, sizeof(du.uc));
	DUK_DBLUNION_DOUBLE_NTOH(&du);
	return du.d;
}

#if 0
DUK_INTERNAL duk_heaphdr *duk_debug_read_heapptr(duk_hthread *thr) {
	duk_small_uint_t x;

	DUK_ASSERT(thr != NULL);

	x = duk_debug_read_byte(thr);
	if (x != DUK_DBG_IB_HEAPPTR) {
		goto fail;
	}

	return (duk_heaphdr *) duk__debug_read_pointer_raw(thr);

 fail:
	DUK_D(DUK_DPRINT("debug connection error: failed to decode heapptr"));
	DUK__SET_CONN_BROKEN(thr, 1);
	return NULL;
}
#endif

DUK_INTERNAL duk_heaphdr *duk_debug_read_any_ptr(duk_hthread *thr) {
	duk_small_uint_t x;

	DUK_ASSERT(thr != NULL);

	x = duk_debug_read_byte(thr);
	switch (x) {
	case DUK_DBG_IB_OBJECT:
	case DUK_DBG_IB_POINTER:
	case DUK_DBG_IB_HEAPPTR:
		/* Accept any pointer-like value; for 'object' dvalue, read
		 * and ignore the class number.
		 */
		if (x == DUK_DBG_IB_OBJECT) {
			duk_debug_skip_byte(thr);
		}
		break;
	default:
		goto fail;
	}

	return (duk_heaphdr *) duk__debug_read_pointer_raw(thr);

fail:
	DUK_D(DUK_DPRINT("debug connection error: failed to decode any pointer (object, pointer, heapptr)"));
	DUK__SET_CONN_BROKEN(thr, 1);
	return NULL;
}

DUK_INTERNAL duk_tval *duk_debug_read_tval(duk_hthread *thr) {
	duk_uint8_t x;
	duk_uint_t t;
	duk_uint32_t len;

	DUK_ASSERT(thr != NULL);

	x = duk_debug_read_byte(thr);

	if (x >= 0xc0) {
		t = (duk_uint_t) (x - 0xc0);
		t = (t << 8) + duk_debug_read_byte(thr);
		duk_push_uint(thr, (duk_uint_t) t);
		goto return_ptr;
	}
	if (x >= 0x80) {
		duk_push_uint(thr, (duk_uint_t) (x - 0x80));
		goto return_ptr;
	}
	if (x >= 0x60) {
		len = (duk_uint32_t) (x - 0x60);
		duk__debug_read_hstring_raw(thr, len);
		goto return_ptr;
	}

	switch (x) {
	case DUK_DBG_IB_INT4: {
		duk_int32_t i = duk__debug_read_int32_raw(thr);
		duk_push_i32(thr, i);
		break;
	}
	case DUK_DBG_IB_STR4: {
		len = duk__debug_read_uint32_raw(thr);
		duk__debug_read_hstring_raw(thr, len);
		break;
	}
	case DUK_DBG_IB_STR2: {
		len = duk__debug_read_uint16_raw(thr);
		duk__debug_read_hstring_raw(thr, len);
		break;
	}
	case DUK_DBG_IB_BUF4: {
		len = duk__debug_read_uint32_raw(thr);
		duk__debug_read_hbuffer_raw(thr, len);
		break;
	}
	case DUK_DBG_IB_BUF2: {
		len = duk__debug_read_uint16_raw(thr);
		duk__debug_read_hbuffer_raw(thr, len);
		break;
	}
	case DUK_DBG_IB_UNDEFINED: {
		duk_push_undefined(thr);
		break;
	}
	case DUK_DBG_IB_NULL: {
		duk_push_null(thr);
		break;
	}
	case DUK_DBG_IB_TRUE: {
		duk_push_true(thr);
		break;
	}
	case DUK_DBG_IB_FALSE: {
		duk_push_false(thr);
		break;
	}
	case DUK_DBG_IB_NUMBER: {
		duk_double_t d;
		d = duk__debug_read_double_raw(thr);
		duk_push_number(thr, d);
		break;
	}
	case DUK_DBG_IB_OBJECT: {
		duk_heaphdr *h;
		duk_debug_skip_byte(thr);
		h = (duk_heaphdr *) duk__debug_read_pointer_raw(thr);
		duk_push_heapptr(thr, (void *) h);
		break;
	}
	case DUK_DBG_IB_POINTER: {
		void *ptr;
		ptr = duk__debug_read_pointer_raw(thr);
		duk_push_pointer(thr, ptr);
		break;
	}
	case DUK_DBG_IB_LIGHTFUNC: {
		/* XXX: Not needed for now, so not implemented.  Note that
		 * function pointers may have different size/layout than
		 * a void pointer.
		 */
		DUK_D(DUK_DPRINT("reading lightfunc values unimplemented"));
		goto fail;
	}
	case DUK_DBG_IB_HEAPPTR: {
		duk_heaphdr *h;
		h = (duk_heaphdr *) duk__debug_read_pointer_raw(thr);
		duk_push_heapptr(thr, (void *) h);
		break;
	}
	case DUK_DBG_IB_UNUSED: /* unused: not accepted in inbound messages */
	default:
		goto fail;
	}

return_ptr:
	return DUK_GET_TVAL_NEGIDX(thr, -1);

fail:
	DUK_D(DUK_DPRINT("debug connection error: failed to decode tval"));
	DUK__SET_CONN_BROKEN(thr, 1);
	return NULL;
}

/*
 *  Debug connection write primitives
 */

/* Write fully. */
DUK_INTERNAL void duk_debug_write_bytes(duk_hthread *thr, const duk_uint8_t *data, duk_size_t length) {
	duk_heap *heap;
	const duk_uint8_t *p;
	duk_size_t left;
	duk_size_t got;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(length == 0 || data != NULL);
	heap = thr->heap;
	DUK_ASSERT(heap != NULL);

	if (heap->dbg_write_cb == NULL) {
		DUK_D(DUK_DPRINT("attempt to write %ld bytes in detached state, ignore", (long) length));
		return;
	}
	if (length == 0) {
		/* Avoid doing an actual write callback with length == 0,
		 * because that's reserved for a write flush.
		 */
		return;
	}
	DUK_ASSERT(data != NULL);

	p = data;
	for (;;) {
		left = (duk_size_t) ((data + length) - p);
		if (left == 0) {
			break;
		}
		DUK_ASSERT(heap->dbg_write_cb != NULL);
		DUK_ASSERT(left >= 1);
#if defined(DUK_USE_DEBUGGER_TRANSPORT_TORTURE)
		left = 1;
#endif
		DUK__DBG_TPORT_ENTER();
		got = heap->dbg_write_cb(heap->dbg_udata, (const char *) p, left);
		DUK__DBG_TPORT_EXIT();

		if (got == 0 || got > left) {
			duk__debug_null_most_callbacks(thr); /* avoid calling write callback in detach1() */
			DUK_D(DUK_DPRINT("connection error during write"));
			DUK__SET_CONN_BROKEN(thr, 1);
			return;
		}
		p += got;
	}
}

DUK_INTERNAL void duk_debug_write_byte(duk_hthread *thr, duk_uint8_t x) {
	duk_debug_write_bytes(thr, (const duk_uint8_t *) &x, 1);
}

DUK_INTERNAL void duk_debug_write_unused(duk_hthread *thr) {
	duk_debug_write_byte(thr, DUK_DBG_IB_UNUSED);
}

DUK_INTERNAL void duk_debug_write_undefined(duk_hthread *thr) {
	duk_debug_write_byte(thr, DUK_DBG_IB_UNDEFINED);
}

#if defined(DUK_USE_DEBUGGER_INSPECT)
DUK_INTERNAL void duk_debug_write_null(duk_hthread *thr) {
	duk_debug_write_byte(thr, DUK_DBG_IB_NULL);
}
#endif

DUK_INTERNAL void duk_debug_write_boolean(duk_hthread *thr, duk_uint_t val) {
	duk_debug_write_byte(thr, val ? DUK_DBG_IB_TRUE : DUK_DBG_IB_FALSE);
}

/* Write signed 32-bit integer. */
DUK_INTERNAL void duk_debug_write_int(duk_hthread *thr, duk_int32_t x) {
	duk_uint8_t buf[5];
	duk_size_t len;

	DUK_ASSERT(thr != NULL);

	if (x >= 0 && x <= 0x3fL) {
		buf[0] = (duk_uint8_t) (0x80 + x);
		len = 1;
	} else if (x >= 0 && x <= 0x3fffL) {
		buf[0] = (duk_uint8_t) (0xc0 + (x >> 8));
		buf[1] = (duk_uint8_t) (x & 0xff);
		len = 2;
	} else {
		/* Signed integers always map to 4 bytes now. */
		buf[0] = (duk_uint8_t) DUK_DBG_IB_INT4;
		buf[1] = (duk_uint8_t) ((x >> 24) & 0xff);
		buf[2] = (duk_uint8_t) ((x >> 16) & 0xff);
		buf[3] = (duk_uint8_t) ((x >> 8) & 0xff);
		buf[4] = (duk_uint8_t) (x & 0xff);
		len = 5;
	}
	duk_debug_write_bytes(thr, buf, len);
}

/* Write unsigned 32-bit integer. */
DUK_INTERNAL void duk_debug_write_uint(duk_hthread *thr, duk_uint32_t x) {
	/* The debugger protocol doesn't support a plain integer encoding for
	 * the full 32-bit unsigned range (only 32-bit signed).  For now,
	 * unsigned 32-bit values simply written as signed ones.  This is not
	 * a concrete issue except for 32-bit heaphdr fields.  Proper solutions
	 * would be to (a) write such integers as IEEE doubles or (b) add an
	 * unsigned 32-bit dvalue.
	 */
	if (x >= 0x80000000UL) {
		DUK_D(DUK_DPRINT("writing unsigned integer 0x%08lx as signed integer", (long) x));
	}
	duk_debug_write_int(thr, (duk_int32_t) x);
}

DUK_INTERNAL void duk_debug_write_strbuf(duk_hthread *thr, const char *data, duk_size_t length, duk_uint8_t marker_base) {
	duk_uint8_t buf[5];
	duk_size_t buflen;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(length == 0 || data != NULL);

	if (length <= 0x1fUL && marker_base == DUK_DBG_IB_STR4) {
		/* For strings, special form for short lengths. */
		buf[0] = (duk_uint8_t) (0x60 + length);
		buflen = 1;
	} else if (length <= 0xffffUL) {
		buf[0] = (duk_uint8_t) (marker_base + 1);
		buf[1] = (duk_uint8_t) (length >> 8);
		buf[2] = (duk_uint8_t) (length & 0xff);
		buflen = 3;
	} else {
		buf[0] = (duk_uint8_t) marker_base;
		buf[1] = (duk_uint8_t) (length >> 24);
		buf[2] = (duk_uint8_t) ((length >> 16) & 0xff);
		buf[3] = (duk_uint8_t) ((length >> 8) & 0xff);
		buf[4] = (duk_uint8_t) (length & 0xff);
		buflen = 5;
	}

	duk_debug_write_bytes(thr, (const duk_uint8_t *) buf, buflen);
	duk_debug_write_bytes(thr, (const duk_uint8_t *) data, length);
}

DUK_INTERNAL void duk_debug_write_string(duk_hthread *thr, const char *data, duk_size_t length) {
	duk_debug_write_strbuf(thr, data, length, DUK_DBG_IB_STR4);
}

DUK_INTERNAL void duk_debug_write_cstring(duk_hthread *thr, const char *data) {
	DUK_ASSERT(thr != NULL);

	duk_debug_write_string(thr, data, data ? DUK_STRLEN(data) : 0);
}

DUK_INTERNAL void duk_debug_write_hstring(duk_hthread *thr, duk_hstring *h) {
	DUK_ASSERT(thr != NULL);

	/* XXX: differentiate null pointer from empty string? */
	duk_debug_write_string(thr,
	                       (h != NULL ? (const char *) DUK_HSTRING_GET_DATA(h) : NULL),
	                       (h != NULL ? (duk_size_t) DUK_HSTRING_GET_BYTELEN(h) : 0));
}

DUK_LOCAL void duk__debug_write_hstring_safe_top(duk_hthread *thr) {
	duk_debug_write_hstring(thr, duk_safe_to_hstring(thr, -1));
}

DUK_INTERNAL void duk_debug_write_buffer(duk_hthread *thr, const char *data, duk_size_t length) {
	duk_debug_write_strbuf(thr, data, length, DUK_DBG_IB_BUF4);
}

DUK_INTERNAL void duk_debug_write_hbuffer(duk_hthread *thr, duk_hbuffer *h) {
	DUK_ASSERT(thr != NULL);

	duk_debug_write_buffer(thr,
	                       (h != NULL ? (const char *) DUK_HBUFFER_GET_DATA_PTR(thr->heap, h) : NULL),
	                       (h != NULL ? (duk_size_t) DUK_HBUFFER_GET_SIZE(h) : 0));
}

DUK_LOCAL void duk__debug_write_pointer_raw(duk_hthread *thr, void *ptr, duk_uint8_t ibyte) {
	duk_uint8_t buf[2];
	duk__ptr_union pu;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(sizeof(ptr) >= 1 && sizeof(ptr) <= 16);
	/* ptr may be NULL */

	buf[0] = ibyte;
	buf[1] = sizeof(pu);
	duk_debug_write_bytes(thr, buf, 2);
	pu.p = (void *) ptr;
#if defined(DUK_USE_INTEGER_LE)
	duk_byteswap_bytes((duk_uint8_t *) pu.b, sizeof(pu));
#endif
	duk_debug_write_bytes(thr, (const duk_uint8_t *) &pu.p, (duk_size_t) sizeof(pu));
}

DUK_INTERNAL void duk_debug_write_pointer(duk_hthread *thr, void *ptr) {
	duk__debug_write_pointer_raw(thr, ptr, DUK_DBG_IB_POINTER);
}

#if defined(DUK_USE_DEBUGGER_DUMPHEAP) || defined(DUK_USE_DEBUGGER_INSPECT)
DUK_INTERNAL void duk_debug_write_heapptr(duk_hthread *thr, duk_heaphdr *h) {
	duk__debug_write_pointer_raw(thr, (void *) h, DUK_DBG_IB_HEAPPTR);
}
#endif /* DUK_USE_DEBUGGER_DUMPHEAP || DUK_USE_DEBUGGER_INSPECT */

DUK_INTERNAL void duk_debug_write_hobject(duk_hthread *thr, duk_hobject *obj) {
	duk_uint8_t buf[3];
	duk__ptr_union pu;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(sizeof(obj) >= 1 && sizeof(obj) <= 16);
	DUK_ASSERT(obj != NULL);

	buf[0] = DUK_DBG_IB_OBJECT;
	buf[1] = (duk_uint8_t) DUK_HOBJECT_GET_CLASS_NUMBER(obj);
	buf[2] = sizeof(pu);
	duk_debug_write_bytes(thr, buf, 3);
	pu.p = (void *) obj;
#if defined(DUK_USE_INTEGER_LE)
	duk_byteswap_bytes((duk_uint8_t *) pu.b, sizeof(pu));
#endif
	duk_debug_write_bytes(thr, (const duk_uint8_t *) &pu.p, (duk_size_t) sizeof(pu));
}

DUK_INTERNAL void duk_debug_write_tval(duk_hthread *thr, duk_tval *tv) {
	duk_c_function lf_func;
	duk_small_uint_t lf_flags;
	duk_uint8_t buf[4];
	duk_double_union du1;
	duk_double_union du2;
	duk_int32_t i32;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(tv != NULL);

	switch (DUK_TVAL_GET_TAG(tv)) {
	case DUK_TAG_UNDEFINED:
		duk_debug_write_byte(thr, DUK_DBG_IB_UNDEFINED);
		break;
	case DUK_TAG_UNUSED:
		duk_debug_write_byte(thr, DUK_DBG_IB_UNUSED);
		break;
	case DUK_TAG_NULL:
		duk_debug_write_byte(thr, DUK_DBG_IB_NULL);
		break;
	case DUK_TAG_BOOLEAN:
		DUK_ASSERT(DUK_TVAL_GET_BOOLEAN(tv) == 0 || DUK_TVAL_GET_BOOLEAN(tv) == 1);
		duk_debug_write_boolean(thr, DUK_TVAL_GET_BOOLEAN(tv));
		break;
	case DUK_TAG_POINTER:
		duk_debug_write_pointer(thr, (void *) DUK_TVAL_GET_POINTER(tv));
		break;
	case DUK_TAG_LIGHTFUNC:
		DUK_TVAL_GET_LIGHTFUNC(tv, lf_func, lf_flags);
		buf[0] = DUK_DBG_IB_LIGHTFUNC;
		buf[1] = (duk_uint8_t) (lf_flags >> 8);
		buf[2] = (duk_uint8_t) (lf_flags & 0xff);
		buf[3] = sizeof(lf_func);
		duk_debug_write_bytes(thr, buf, 4);
		duk_debug_write_bytes(thr, (const duk_uint8_t *) &lf_func, sizeof(lf_func));
		break;
	case DUK_TAG_STRING:
		duk_debug_write_hstring(thr, DUK_TVAL_GET_STRING(tv));
		break;
	case DUK_TAG_OBJECT:
		duk_debug_write_hobject(thr, DUK_TVAL_GET_OBJECT(tv));
		break;
	case DUK_TAG_BUFFER:
		duk_debug_write_hbuffer(thr, DUK_TVAL_GET_BUFFER(tv));
		break;
#if defined(DUK_USE_FASTINT)
	case DUK_TAG_FASTINT:
#endif
	default:
		/* Numbers are normalized to big (network) endian.  We can
		 * (but are not required) to use integer dvalues when there's
		 * no loss of precision.
		 *
		 * XXX: share check with other code; this check is slow but
		 * reliable and doesn't require careful exponent/mantissa
		 * mask tricks as in the fastint downgrade code.
		 */
		DUK_ASSERT(!DUK_TVAL_IS_UNUSED(tv));
		DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv));
		du1.d = DUK_TVAL_GET_NUMBER(tv);
		i32 = (duk_int32_t) du1.d;
		du2.d = (duk_double_t) i32;

		DUK_DD(DUK_DDPRINT("i32=%ld du1=%02x%02x%02x%02x%02x%02x%02x%02x "
		                   "du2=%02x%02x%02x%02x%02x%02x%02x%02x",
		                   (long) i32,
		                   (unsigned int) du1.uc[0],
		                   (unsigned int) du1.uc[1],
		                   (unsigned int) du1.uc[2],
		                   (unsigned int) du1.uc[3],
		                   (unsigned int) du1.uc[4],
		                   (unsigned int) du1.uc[5],
		                   (unsigned int) du1.uc[6],
		                   (unsigned int) du1.uc[7],
		                   (unsigned int) du2.uc[0],
		                   (unsigned int) du2.uc[1],
		                   (unsigned int) du2.uc[2],
		                   (unsigned int) du2.uc[3],
		                   (unsigned int) du2.uc[4],
		                   (unsigned int) du2.uc[5],
		                   (unsigned int) du2.uc[6],
		                   (unsigned int) du2.uc[7]));

		if (duk_memcmp((const void *) du1.uc, (const void *) du2.uc, sizeof(du1.uc)) == 0) {
			duk_debug_write_int(thr, i32);
		} else {
			DUK_DBLUNION_DOUBLE_HTON(&du1);
			duk_debug_write_byte(thr, DUK_DBG_IB_NUMBER);
			duk_debug_write_bytes(thr, (const duk_uint8_t *) du1.uc, sizeof(du1.uc));
		}
	}
}

#if defined(DUK_USE_DEBUGGER_DUMPHEAP)
/* Variant for writing duk_tvals so that any heap allocated values are
 * written out as tagged heap pointers.
 */
DUK_LOCAL void duk__debug_write_tval_heapptr(duk_hthread *thr, duk_tval *tv) {
	if (DUK_TVAL_IS_HEAP_ALLOCATED(tv)) {
		duk_heaphdr *h = DUK_TVAL_GET_HEAPHDR(tv);
		duk_debug_write_heapptr(thr, h);
	} else {
		duk_debug_write_tval(thr, tv);
	}
}
#endif /* DUK_USE_DEBUGGER_DUMPHEAP */

/*
 *  Debug connection message write helpers
 */

#if 0 /* unused */
DUK_INTERNAL void duk_debug_write_request(duk_hthread *thr, duk_small_uint_t command) {
	duk_debug_write_byte(thr, DUK_DBG_IB_REQUEST);
	duk_debug_write_int(thr, command);
}
#endif

DUK_INTERNAL void duk_debug_write_reply(duk_hthread *thr) {
	duk_debug_write_byte(thr, DUK_DBG_IB_REPLY);
}

DUK_INTERNAL void duk_debug_write_error_eom(duk_hthread *thr, duk_small_uint_t err_code, const char *msg) {
	/* Allow NULL 'msg' */
	duk_debug_write_byte(thr, DUK_DBG_IB_ERROR);
	duk_debug_write_int(thr, (duk_int32_t) err_code);
	duk_debug_write_cstring(thr, msg);
	duk_debug_write_eom(thr);
}

DUK_INTERNAL void duk_debug_write_notify(duk_hthread *thr, duk_small_uint_t command) {
	duk_debug_write_byte(thr, DUK_DBG_IB_NOTIFY);
	duk_debug_write_int(thr, (duk_int32_t) command);
}

DUK_INTERNAL void duk_debug_write_eom(duk_hthread *thr) {
	duk_debug_write_byte(thr, DUK_DBG_IB_EOM);

	/* As an initial implementation, write flush after every EOM (and the
	 * version identifier).  A better implementation would flush only when
	 * Duktape is finished processing messages so that a flush only happens
	 * after all outbound messages are finished on that occasion.
	 */
	duk_debug_write_flush(thr);
}

/*
 *  Status message and helpers
 */

DUK_INTERNAL duk_uint_fast32_t duk_debug_curr_line(duk_hthread *thr) {
	duk_activation *act;
	duk_uint_fast32_t line;
	duk_uint_fast32_t pc;

	act = thr->callstack_curr;
	if (act == NULL) {
		return 0;
	}

	/* We're conceptually between two opcodes; act->pc indicates the next
	 * instruction to be executed.  This is usually the correct pc/line to
	 * indicate in Status.  (For the 'debugger' statement this now reports
	 * the pc/line after the debugger statement because the debugger opcode
	 * has already been executed.)
	 */

	pc = duk_hthread_get_act_curr_pc(thr, act);

	/* XXX: this should be optimized to be a raw query and avoid valstack
	 * operations if possible.
	 */
	duk_push_tval(thr, &act->tv_func);
	line = duk_hobject_pc2line_query(thr, -1, pc);
	duk_pop(thr);
	return line;
}

DUK_INTERNAL void duk_debug_send_status(duk_hthread *thr) {
	duk_activation *act;

	duk_debug_write_notify(thr, DUK_DBG_CMD_STATUS);
	duk_debug_write_int(thr, (DUK_HEAP_HAS_DEBUGGER_PAUSED(thr->heap) ? 1 : 0));

	act = thr->callstack_curr;
	if (act == NULL) {
		duk_debug_write_undefined(thr);
		duk_debug_write_undefined(thr);
		duk_debug_write_int(thr, 0);
		duk_debug_write_int(thr, 0);
	} else {
		duk_push_tval(thr, &act->tv_func);
		duk_get_prop_literal(thr, -1, "fileName");
		duk__debug_write_hstring_safe_top(thr);
		duk_get_prop_literal(thr, -2, "name");
		duk__debug_write_hstring_safe_top(thr);
		duk_pop_3(thr);
		/* Report next pc/line to be executed. */
		duk_debug_write_uint(thr, (duk_uint32_t) duk_debug_curr_line(thr));
		duk_debug_write_uint(thr, (duk_uint32_t) duk_hthread_get_act_curr_pc(thr, act));
	}

	duk_debug_write_eom(thr);
}

#if defined(DUK_USE_DEBUGGER_THROW_NOTIFY)
DUK_INTERNAL void duk_debug_send_throw(duk_hthread *thr, duk_bool_t fatal) {
	/*
	 *  NFY <int: 5> <int: fatal> <str: msg> <str: filename> <int: linenumber> EOM
	 */

	duk_activation *act;
	duk_uint32_t pc;

	DUK_ASSERT(thr->valstack_top > thr->valstack); /* At least: ... [err] */

	duk_debug_write_notify(thr, DUK_DBG_CMD_THROW);
	duk_debug_write_int(thr, (duk_int32_t) fatal);

	/* Report thrown value to client coerced to string */
	duk_dup_top(thr);
	duk__debug_write_hstring_safe_top(thr);
	duk_pop(thr);

	if (duk_is_error(thr, -1)) {
		/* Error instance, use augmented error data directly */
		duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_FILE_NAME);
		duk__debug_write_hstring_safe_top(thr);
		duk_get_prop_stridx_short(thr, -2, DUK_STRIDX_LINE_NUMBER);
		duk_debug_write_uint(thr, duk_get_uint(thr, -1));
		duk_pop_2(thr);
	} else {
		/* For anything other than an Error instance, we calculate the
		 * error location directly from the current activation if one
		 * exists.
		 */
		act = thr->callstack_curr;
		if (act != NULL) {
			duk_push_tval(thr, &act->tv_func);
			duk_get_prop_literal(thr, -1, "fileName");
			duk__debug_write_hstring_safe_top(thr);
			pc = (duk_uint32_t) duk_hthread_get_act_prev_pc(thr, act);
			duk_debug_write_uint(thr, (duk_uint32_t) duk_hobject_pc2line_query(thr, -2, pc));
			duk_pop_2(thr);
		} else {
			/* Can happen if duk_throw() is called on an empty
			 * callstack.
			 */
			duk_debug_write_cstring(thr, "");
			duk_debug_write_uint(thr, 0);
		}
	}

	duk_debug_write_eom(thr);
}
#endif /* DUK_USE_DEBUGGER_THROW_NOTIFY */

/*
 *  Debug message processing
 */

/* Skip dvalue. */
DUK_LOCAL duk_bool_t duk__debug_skip_dvalue(duk_hthread *thr) {
	duk_uint8_t x;
	duk_uint32_t len;

	x = duk_debug_read_byte(thr);

	if (x >= 0xc0) {
		duk_debug_skip_byte(thr);
		return 0;
	}
	if (x >= 0x80) {
		return 0;
	}
	if (x >= 0x60) {
		duk_debug_skip_bytes(thr, (duk_size_t) (x - 0x60));
		return 0;
	}
	switch (x) {
	case DUK_DBG_IB_EOM:
		return 1; /* Return 1: got EOM */
	case DUK_DBG_IB_REQUEST:
	case DUK_DBG_IB_REPLY:
	case DUK_DBG_IB_ERROR:
	case DUK_DBG_IB_NOTIFY:
		break;
	case DUK_DBG_IB_INT4:
		(void) duk__debug_read_uint32_raw(thr);
		break;
	case DUK_DBG_IB_STR4:
	case DUK_DBG_IB_BUF4:
		len = duk__debug_read_uint32_raw(thr);
		duk_debug_skip_bytes(thr, len);
		break;
	case DUK_DBG_IB_STR2:
	case DUK_DBG_IB_BUF2:
		len = duk__debug_read_uint16_raw(thr);
		duk_debug_skip_bytes(thr, len);
		break;
	case DUK_DBG_IB_UNUSED:
	case DUK_DBG_IB_UNDEFINED:
	case DUK_DBG_IB_NULL:
	case DUK_DBG_IB_TRUE:
	case DUK_DBG_IB_FALSE:
		break;
	case DUK_DBG_IB_NUMBER:
		duk_debug_skip_bytes(thr, 8);
		break;
	case DUK_DBG_IB_OBJECT:
		duk_debug_skip_byte(thr);
		len = duk_debug_read_byte(thr);
		duk_debug_skip_bytes(thr, len);
		break;
	case DUK_DBG_IB_POINTER:
	case DUK_DBG_IB_HEAPPTR:
		len = duk_debug_read_byte(thr);
		duk_debug_skip_bytes(thr, len);
		break;
	case DUK_DBG_IB_LIGHTFUNC:
		duk_debug_skip_bytes(thr, 2);
		len = duk_debug_read_byte(thr);
		duk_debug_skip_bytes(thr, len);
		break;
	default:
		goto fail;
	}

	return 0;

fail:
	DUK__SET_CONN_BROKEN(thr, 1);
	return 1; /* Pretend like we got EOM */
}

/* Skip dvalues to EOM. */
DUK_LOCAL void duk__debug_skip_to_eom(duk_hthread *thr) {
	for (;;) {
		if (duk__debug_skip_dvalue(thr)) {
			break;
		}
	}
}

/* Read and validate a call stack index.  If index is invalid, write out an
 * error message and return zero.
 */
DUK_LOCAL duk_int32_t duk__debug_read_validate_csindex(duk_hthread *thr) {
	duk_int32_t level;
	level = duk_debug_read_int(thr);
	if (level >= 0 || -level > (duk_int32_t) thr->callstack_top) {
		duk_debug_write_error_eom(thr, DUK_DBG_ERR_NOTFOUND, "invalid callstack index");
		return 0; /* zero indicates failure */
	}
	return level;
}

/* Read a call stack index and lookup the corresponding duk_activation.
 * If index is invalid, write out an error message and return NULL.
 */
DUK_LOCAL duk_activation *duk__debug_read_level_get_activation(duk_hthread *thr) {
	duk_activation *act;
	duk_int32_t level;

	level = duk_debug_read_int(thr);
	act = duk_hthread_get_activation_for_level(thr, level);
	if (act == NULL) {
		duk_debug_write_error_eom(thr, DUK_DBG_ERR_NOTFOUND, "invalid callstack index");
	}
	return act;
}

/*
 *  Simple commands
 */

DUK_LOCAL void duk__debug_handle_basic_info(duk_hthread *thr, duk_heap *heap) {
	DUK_UNREF(heap);
	DUK_D(DUK_DPRINT("debug command Version"));

	duk_debug_write_reply(thr);
	duk_debug_write_int(thr, DUK_VERSION);
	duk_debug_write_cstring(thr, DUK_GIT_DESCRIBE);
	duk_debug_write_cstring(thr, DUK_USE_TARGET_INFO);
#if defined(DUK_USE_DOUBLE_LE)
	duk_debug_write_int(thr, 1);
#elif defined(DUK_USE_DOUBLE_ME)
	duk_debug_write_int(thr, 2);
#elif defined(DUK_USE_DOUBLE_BE)
	duk_debug_write_int(thr, 3);
#else
	duk_debug_write_int(thr, 0);
#endif
	duk_debug_write_int(thr, (duk_int_t) sizeof(void *));
	duk_debug_write_eom(thr);
}

DUK_LOCAL void duk__debug_handle_trigger_status(duk_hthread *thr, duk_heap *heap) {
	DUK_UNREF(heap);
	DUK_D(DUK_DPRINT("debug command TriggerStatus"));

	duk_debug_write_reply(thr);
	duk_debug_write_eom(thr);
	heap->dbg_state_dirty = 1;
}

DUK_LOCAL void duk__debug_handle_pause(duk_hthread *thr, duk_heap *heap) {
	DUK_D(DUK_DPRINT("debug command Pause"));
	duk_debug_set_paused(heap);
	duk_debug_write_reply(thr);
	duk_debug_write_eom(thr);
}

DUK_LOCAL void duk__debug_handle_resume(duk_hthread *thr, duk_heap *heap) {
	duk_small_uint_t pause_flags;

	DUK_D(DUK_DPRINT("debug command Resume"));

	duk_debug_clear_paused(heap);

	pause_flags = 0;
#if 0 /* manual testing */
	pause_flags |= DUK_PAUSE_FLAG_ONE_OPCODE;
	pause_flags |= DUK_PAUSE_FLAG_CAUGHT_ERROR;
	pause_flags |= DUK_PAUSE_FLAG_UNCAUGHT_ERROR;
#endif
#if defined(DUK_USE_DEBUGGER_PAUSE_UNCAUGHT)
	pause_flags |= DUK_PAUSE_FLAG_UNCAUGHT_ERROR;
#endif

	duk__debug_set_pause_state(thr, heap, pause_flags);

	duk_debug_write_reply(thr);
	duk_debug_write_eom(thr);
}

DUK_LOCAL void duk__debug_handle_step(duk_hthread *thr, duk_heap *heap, duk_int32_t cmd) {
	duk_small_uint_t pause_flags;

	DUK_D(DUK_DPRINT("debug command StepInto/StepOver/StepOut: %d", (int) cmd));

	if (cmd == DUK_DBG_CMD_STEPINTO) {
		pause_flags = DUK_PAUSE_FLAG_LINE_CHANGE | DUK_PAUSE_FLAG_FUNC_ENTRY | DUK_PAUSE_FLAG_FUNC_EXIT;
	} else if (cmd == DUK_DBG_CMD_STEPOVER) {
		pause_flags = DUK_PAUSE_FLAG_LINE_CHANGE | DUK_PAUSE_FLAG_FUNC_EXIT;
	} else {
		DUK_ASSERT(cmd == DUK_DBG_CMD_STEPOUT);
		pause_flags = DUK_PAUSE_FLAG_FUNC_EXIT;
	}
#if defined(DUK_USE_DEBUGGER_PAUSE_UNCAUGHT)
	pause_flags |= DUK_PAUSE_FLAG_UNCAUGHT_ERROR;
#endif

	/* If current activation doesn't have line information, line-based
	 * pause flags are automatically disabled.  As a result, e.g.
	 * StepInto will then pause on (native) function entry or exit.
	 */
	duk_debug_clear_paused(heap);
	duk__debug_set_pause_state(thr, heap, pause_flags);

	duk_debug_write_reply(thr);
	duk_debug_write_eom(thr);
}

DUK_LOCAL void duk__debug_handle_list_break(duk_hthread *thr, duk_heap *heap) {
	duk_small_int_t i;

	DUK_D(DUK_DPRINT("debug command ListBreak"));
	duk_debug_write_reply(thr);
	for (i = 0; i < (duk_small_int_t) heap->dbg_breakpoint_count; i++) {
		duk_debug_write_hstring(thr, heap->dbg_breakpoints[i].filename);
		duk_debug_write_uint(thr, (duk_uint32_t) heap->dbg_breakpoints[i].line);
	}
	duk_debug_write_eom(thr);
}

DUK_LOCAL void duk__debug_handle_add_break(duk_hthread *thr, duk_heap *heap) {
	duk_hstring *filename;
	duk_uint32_t linenumber;
	duk_small_int_t idx;

	DUK_UNREF(heap);

	filename = duk_debug_read_hstring(thr);
	linenumber = (duk_uint32_t) duk_debug_read_int(thr);
	DUK_D(DUK_DPRINT("debug command AddBreak: %!O:%ld", (duk_hobject *) filename, (long) linenumber));
	idx = duk_debug_add_breakpoint(thr, filename, linenumber);
	if (idx >= 0) {
		duk_debug_write_reply(thr);
		duk_debug_write_int(thr, (duk_int32_t) idx);
		duk_debug_write_eom(thr);
	} else {
		duk_debug_write_error_eom(thr, DUK_DBG_ERR_TOOMANY, "no space for breakpoint");
	}
}

DUK_LOCAL void duk__debug_handle_del_break(duk_hthread *thr, duk_heap *heap) {
	duk_small_uint_t idx;

	DUK_UNREF(heap);

	DUK_D(DUK_DPRINT("debug command DelBreak"));
	idx = (duk_small_uint_t) duk_debug_read_int(thr);
	if (duk_debug_remove_breakpoint(thr, idx)) {
		duk_debug_write_reply(thr);
		duk_debug_write_eom(thr);
	} else {
		duk_debug_write_error_eom(thr, DUK_DBG_ERR_NOTFOUND, "invalid breakpoint index");
	}
}

DUK_LOCAL void duk__debug_handle_get_var(duk_hthread *thr, duk_heap *heap) {
	duk_activation *act;
	duk_hstring *str;
	duk_bool_t rc;

	DUK_UNREF(heap);
	DUK_D(DUK_DPRINT("debug command GetVar"));

	act = duk__debug_read_level_get_activation(thr);
	if (act == NULL) {
		return;
	}
	str = duk_debug_read_hstring(thr); /* push to stack */
	DUK_ASSERT(str != NULL);

	rc = duk_js_getvar_activation(thr, act, str, 0);

	duk_debug_write_reply(thr);
	if (rc) {
		duk_debug_write_int(thr, 1);
		DUK_ASSERT(duk_get_tval(thr, -2) != NULL);
		duk_debug_write_tval(thr, duk_get_tval(thr, -2));
	} else {
		duk_debug_write_int(thr, 0);
		duk_debug_write_unused(thr);
	}
	duk_debug_write_eom(thr);
}

DUK_LOCAL void duk__debug_handle_put_var(duk_hthread *thr, duk_heap *heap) {
	duk_activation *act;
	duk_hstring *str;
	duk_tval *tv;

	DUK_UNREF(heap);
	DUK_D(DUK_DPRINT("debug command PutVar"));

	act = duk__debug_read_level_get_activation(thr);
	if (act == NULL) {
		return;
	}
	str = duk_debug_read_hstring(thr); /* push to stack */
	DUK_ASSERT(str != NULL);
	tv = duk_debug_read_tval(thr);
	if (tv == NULL) {
		/* detached */
		return;
	}

	duk_js_putvar_activation(thr, act, str, tv, 0);

	/* XXX: Current putvar implementation doesn't have a success flag,
	 * add one and send to debug client?
	 */
	duk_debug_write_reply(thr);
	duk_debug_write_eom(thr);
}

DUK_LOCAL void duk__debug_handle_get_call_stack(duk_hthread *thr, duk_heap *heap) {
	duk_hthread *curr_thr = thr;
	duk_activation *curr_act;
	duk_uint_fast32_t pc;
	duk_uint_fast32_t line;

	DUK_ASSERT(thr != NULL);
	DUK_UNREF(heap);

	duk_debug_write_reply(thr);
	while (curr_thr != NULL) {
		for (curr_act = curr_thr->callstack_curr; curr_act != NULL; curr_act = curr_act->parent) {
			/* PC/line semantics here are:
			 *   - For callstack top we're conceptually between two
			 *     opcodes and current PC indicates next line to
			 *     execute, so report that (matches Status).
			 *   - For other activations we're conceptually still
			 *     executing the instruction at PC-1, so report that
			 *     (matches error stacktrace behavior).
			 *   - See: https://github.com/svaarala/duktape/issues/281
			 */

			/* XXX: optimize to use direct reads, i.e. avoid
			 * value stack operations.
			 */
			duk_push_tval(thr, &curr_act->tv_func);
			duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_FILE_NAME);
			duk__debug_write_hstring_safe_top(thr);
			duk_get_prop_stridx_short(thr, -2, DUK_STRIDX_NAME);
			duk__debug_write_hstring_safe_top(thr);
			pc = duk_hthread_get_act_curr_pc(thr, curr_act);
			if (curr_act != curr_thr->callstack_curr && pc > 0) {
				pc--;
			}
			line = duk_hobject_pc2line_query(thr, -3, pc);
			duk_debug_write_uint(thr, (duk_uint32_t) line);
			duk_debug_write_uint(thr, (duk_uint32_t) pc);
			duk_pop_3(thr);
		}
		curr_thr = curr_thr->resumer;
	}
	/* SCANBUILD: warning about 'thr' potentially being NULL here,
	 * warning is incorrect because thr != NULL always here.
	 */
	duk_debug_write_eom(thr);
}

DUK_LOCAL void duk__debug_handle_get_locals(duk_hthread *thr, duk_heap *heap) {
	duk_activation *act;
	duk_hstring *varname;

	DUK_UNREF(heap);

	act = duk__debug_read_level_get_activation(thr);
	if (act == NULL) {
		return;
	}

	duk_debug_write_reply(thr);

	/* XXX: several nice-to-have improvements here:
	 *   - Use direct reads avoiding value stack operations
	 *   - Avoid triggering getters, indicate getter values to debug client
	 *   - If side effects are possible, add error catching
	 */

	if (DUK_TVAL_IS_OBJECT(&act->tv_func)) {
		duk_hobject *h_func = DUK_TVAL_GET_OBJECT(&act->tv_func);
		duk_hobject *h_varmap;

		h_varmap = duk_hobject_get_varmap(thr, h_func);
		if (h_varmap != NULL) {
			duk_push_hobject(thr, h_varmap);
			duk_enum(thr, -1, 0 /*enum_flags*/);
			while (duk_next(thr, -1 /*enum_index*/, 0 /*get_value*/)) {
				varname = duk_known_hstring(thr, -1);

				duk_js_getvar_activation(thr, act, varname, 0 /*throw_flag*/);
				/* [ ... func varmap enum key value this ] */
				duk_debug_write_hstring(thr, duk_get_hstring(thr, -3));
				duk_debug_write_tval(thr, duk_get_tval(thr, -2));
				duk_pop_3(thr); /* -> [ ... func varmap enum ] */
			}
		} else {
			DUK_D(DUK_DPRINT("varmap missing in GetLocals, ignore"));
		}
	} else {
		DUK_D(DUK_DPRINT("varmap is not an object in GetLocals, ignore"));
	}

	duk_debug_write_eom(thr);
}

DUK_LOCAL void duk__debug_handle_eval(duk_hthread *thr, duk_heap *heap) {
	duk_small_uint_t call_flags;
	duk_int_t call_ret;
	duk_small_int_t eval_err;
	duk_bool_t direct_eval;
	duk_int32_t level;
	duk_idx_t idx_func;

	DUK_UNREF(heap);

	DUK_D(DUK_DPRINT("debug command Eval"));

	/* The eval code is executed within the lexical environment of a specified
	 * activation.  For now, use global object eval() function, with the eval
	 * considered a 'direct call to eval'.
	 *
	 * Callstack index for debug commands only affects scope -- the callstack
	 * as seen by, e.g. Duktape.act() will be the same regardless.
	 */

	/* nargs == 2 so we can pass a callstack index to eval(). */
	idx_func = duk_get_top(thr);
	duk_push_c_function(thr, duk_bi_global_object_eval, 2 /*nargs*/);
	duk_push_undefined(thr); /* 'this' binding shouldn't matter here */

	/* Read callstack index, if non-null. */
	if (duk_debug_peek_byte(thr) == DUK_DBG_IB_NULL) {
		direct_eval = 0;
		level = -1; /* Not needed, but silences warning. */
		(void) duk_debug_read_byte(thr);
	} else {
		direct_eval = 1;
		level = duk__debug_read_validate_csindex(thr);
		if (level == 0) {
			return;
		}
	}

	DUK_ASSERT(!direct_eval || (level < 0 && -level <= (duk_int32_t) thr->callstack_top));

	(void) duk_debug_read_hstring(thr);
	if (direct_eval) {
		duk_push_int(thr, level - 1); /* compensate for eval() call */
	}

	/* [ ... eval "eval" eval_input level? ] */

	call_flags = 0;
	if (direct_eval) {
		duk_activation *act;
		duk_hobject *fun;

		act = duk_hthread_get_activation_for_level(thr, level);
		if (act != NULL) {
			fun = DUK_ACT_GET_FUNC(act);
			if (fun != NULL && DUK_HOBJECT_IS_COMPFUNC(fun)) {
				/* Direct eval requires that there's a current
				 * activation and it is an ECMAScript function.
				 * When Eval is executed from e.g. cooperate API
				 * call we'll need to do an indirect eval instead.
				 */
				call_flags |= DUK_CALL_FLAG_DIRECT_EVAL;
			}
		}
	}

	call_ret = duk_pcall_method_flags(thr, duk_get_top(thr) - (idx_func + 2), call_flags);

	if (call_ret == DUK_EXEC_SUCCESS) {
		eval_err = 0;
		/* Use result value as is. */
	} else {
		/* For errors a string coerced result is most informative
		 * right now, as the debug client doesn't have the capability
		 * to traverse the error object.
		 */
		eval_err = 1;
		duk_safe_to_string(thr, -1);
	}

	/* [ ... result ] */

	duk_debug_write_reply(thr);
	duk_debug_write_int(thr, (duk_int32_t) eval_err);
	DUK_ASSERT(duk_get_tval(thr, -1) != NULL);
	duk_debug_write_tval(thr, duk_get_tval(thr, -1));
	duk_debug_write_eom(thr);
}

DUK_LOCAL void duk__debug_handle_detach(duk_hthread *thr, duk_heap *heap) {
	DUK_UNREF(heap);
	DUK_D(DUK_DPRINT("debug command Detach"));

	duk_debug_write_reply(thr);
	duk_debug_write_eom(thr);

	DUK_D(DUK_DPRINT("debug connection detached, mark broken"));
	DUK__SET_CONN_BROKEN(thr, 0); /* not an error */
}

DUK_LOCAL void duk__debug_handle_apprequest(duk_hthread *thr, duk_heap *heap) {
	duk_idx_t old_top;

	DUK_D(DUK_DPRINT("debug command AppRequest"));

	old_top = duk_get_top(thr); /* save stack top */

	if (heap->dbg_request_cb != NULL) {
		duk_idx_t nrets;
		duk_idx_t nvalues = 0;
		duk_idx_t top, idx;

		/* Read tvals from the message and push them onto the valstack,
		 * then call the request callback to process the request.
		 */
		while (duk_debug_peek_byte(thr) != DUK_DBG_IB_EOM) {
			duk_tval *tv;
			if (!duk_check_stack(thr, 1)) {
				DUK_D(DUK_DPRINT("failed to allocate space for request dvalue(s)"));
				goto fail;
			}
			tv = duk_debug_read_tval(thr); /* push to stack */
			if (tv == NULL) {
				/* detached */
				return;
			}
			nvalues++;
		}
		DUK_ASSERT(duk_get_top(thr) == old_top + nvalues);

		/* Request callback should push values for reply to client onto valstack */
		DUK_D(DUK_DPRINT("calling into AppRequest request_cb with nvalues=%ld, old_top=%ld, top=%ld",
		                 (long) nvalues,
		                 (long) old_top,
		                 (long) duk_get_top(thr)));
		nrets = heap->dbg_request_cb(thr, heap->dbg_udata, nvalues);
		DUK_D(DUK_DPRINT("returned from AppRequest request_cb; nvalues=%ld -> nrets=%ld, old_top=%ld, top=%ld",
		                 (long) nvalues,
		                 (long) nrets,
		                 (long) old_top,
		                 (long) duk_get_top(thr)));
		if (nrets >= 0) {
			DUK_ASSERT(duk_get_top(thr) >= old_top + nrets);
			if (duk_get_top(thr) < old_top + nrets) {
				DUK_D(DUK_DPRINT("AppRequest callback doesn't match value stack configuration, "
				                 "top=%ld < old_top=%ld + nrets=%ld; "
				                 "this might mean it's unsafe to continue!",
				                 (long) duk_get_top(thr),
				                 (long) old_top,
				                 (long) nrets));
				goto fail;
			}

			/* Reply with tvals pushed by request callback */
			duk_debug_write_byte(thr, DUK_DBG_IB_REPLY);
			top = duk_get_top(thr);
			for (idx = top - nrets; idx < top; idx++) {
				duk_debug_write_tval(thr, DUK_GET_TVAL_POSIDX(thr, idx));
			}
			duk_debug_write_eom(thr);
		} else {
			DUK_ASSERT(duk_get_top(thr) >= old_top + 1);
			if (duk_get_top(thr) < old_top + 1) {
				DUK_D(DUK_DPRINT("request callback return value doesn't match value stack configuration"));
				goto fail;
			}
			duk_debug_write_error_eom(thr, DUK_DBG_ERR_APPLICATION, duk_get_string(thr, -1));
		}

		duk_set_top(thr, old_top); /* restore stack top */
	} else {
		DUK_D(DUK_DPRINT("no request callback, treat AppRequest as unsupported"));
		duk_debug_write_error_eom(thr, DUK_DBG_ERR_UNSUPPORTED, "AppRequest unsupported by target");
	}

	return;

fail:
	duk_set_top(thr, old_top); /* restore stack top */
	DUK__SET_CONN_BROKEN(thr, 1);
}

/*
 *  DumpHeap command
 */

#if defined(DUK_USE_DEBUGGER_DUMPHEAP)
/* XXX: this has some overlap with object inspection; remove this and make
 * DumpHeap return lists of heapptrs instead?
 */
DUK_LOCAL void duk__debug_dump_heaphdr(duk_hthread *thr, duk_heap *heap, duk_heaphdr *hdr) {
	DUK_UNREF(heap);

	duk_debug_write_heapptr(thr, hdr);
	duk_debug_write_uint(thr, (duk_uint32_t) DUK_HEAPHDR_GET_TYPE(hdr));
	duk_debug_write_uint(thr, (duk_uint32_t) DUK_HEAPHDR_GET_FLAGS_RAW(hdr));
#if defined(DUK_USE_REFERENCE_COUNTING)
	duk_debug_write_uint(thr, (duk_uint32_t) DUK_HEAPHDR_GET_REFCOUNT(hdr));
#else
	duk_debug_write_int(thr, (duk_int32_t) -1);
#endif

	switch (DUK_HEAPHDR_GET_TYPE(hdr)) {
	case DUK_HTYPE_STRING: {
		duk_hstring *h = (duk_hstring *) hdr;

		duk_debug_write_uint(thr, (duk_uint32_t) DUK_HSTRING_GET_BYTELEN(h));
		duk_debug_write_uint(thr, (duk_uint32_t) DUK_HSTRING_GET_CHARLEN(h));
		duk_debug_write_uint(thr, (duk_uint32_t) DUK_HSTRING_GET_HASH(h));
		duk_debug_write_hstring(thr, h);
		break;
	}
	case DUK_HTYPE_OBJECT: {
		duk_hobject *h = (duk_hobject *) hdr;
		duk_hstring *k;
		duk_uint_fast32_t i;

		duk_debug_write_uint(thr, (duk_uint32_t) DUK_HOBJECT_GET_CLASS_NUMBER(h));
		duk_debug_write_heapptr(thr, (duk_heaphdr *) DUK_HOBJECT_GET_PROTOTYPE(heap, h));
		duk_debug_write_uint(thr, (duk_uint32_t) DUK_HOBJECT_GET_ESIZE(h));
		duk_debug_write_uint(thr, (duk_uint32_t) DUK_HOBJECT_GET_ENEXT(h));
		duk_debug_write_uint(thr, (duk_uint32_t) DUK_HOBJECT_GET_ASIZE(h));
		duk_debug_write_uint(thr, (duk_uint32_t) DUK_HOBJECT_GET_HSIZE(h));

		for (i = 0; i < (duk_uint_fast32_t) DUK_HOBJECT_GET_ENEXT(h); i++) {
			duk_debug_write_uint(thr, (duk_uint32_t) DUK_HOBJECT_E_GET_FLAGS(heap, h, i));
			k = DUK_HOBJECT_E_GET_KEY(heap, h, i);
			duk_debug_write_heapptr(thr, (duk_heaphdr *) k);
			if (k == NULL) {
				duk_debug_write_int(thr, 0); /* isAccessor */
				duk_debug_write_unused(thr);
				continue;
			}
			if (DUK_HOBJECT_E_SLOT_IS_ACCESSOR(heap, h, i)) {
				duk_debug_write_int(thr, 1); /* isAccessor */
				duk_debug_write_heapptr(thr, (duk_heaphdr *) DUK_HOBJECT_E_GET_VALUE_PTR(heap, h, i)->a.get);
				duk_debug_write_heapptr(thr, (duk_heaphdr *) DUK_HOBJECT_E_GET_VALUE_PTR(heap, h, i)->a.set);
			} else {
				duk_debug_write_int(thr, 0); /* isAccessor */

				duk__debug_write_tval_heapptr(thr, &DUK_HOBJECT_E_GET_VALUE_PTR(heap, h, i)->v);
			}
		}

		for (i = 0; i < (duk_uint_fast32_t) DUK_HOBJECT_GET_ASIZE(h); i++) {
			/* Note: array dump will include elements beyond
			 * 'length'.
			 */
			duk__debug_write_tval_heapptr(thr, DUK_HOBJECT_A_GET_VALUE_PTR(heap, h, i));
		}
		break;
	}
	case DUK_HTYPE_BUFFER: {
		duk_hbuffer *h = (duk_hbuffer *) hdr;

		duk_debug_write_uint(thr, (duk_uint32_t) DUK_HBUFFER_GET_SIZE(h));
		duk_debug_write_buffer(thr, (const char *) DUK_HBUFFER_GET_DATA_PTR(heap, h), (duk_size_t) DUK_HBUFFER_GET_SIZE(h));
		break;
	}
	default: {
		DUK_D(DUK_DPRINT("invalid htype: %d", (int) DUK_HEAPHDR_GET_TYPE(hdr)));
	}
	}
}

DUK_LOCAL void duk__debug_dump_heap_allocated(duk_hthread *thr, duk_heap *heap) {
	duk_heaphdr *hdr;

	hdr = heap->heap_allocated;
	while (hdr != NULL) {
		duk__debug_dump_heaphdr(thr, heap, hdr);
		hdr = DUK_HEAPHDR_GET_NEXT(heap, hdr);
	}
}

DUK_LOCAL void duk__debug_dump_strtab(duk_hthread *thr, duk_heap *heap) {
	duk_uint32_t i;
	duk_hstring *h;

	for (i = 0; i < heap->st_size; i++) {
#if defined(DUK_USE_STRTAB_PTRCOMP)
		h = DUK_USE_HEAPPTR_DEC16((heap)->heap_udata, heap->strtable16[i]);
#else
		h = heap->strtable[i];
#endif
		while (h != NULL) {
			duk__debug_dump_heaphdr(thr, heap, (duk_heaphdr *) h);
			h = h->hdr.h_next;
		}
	}
}

DUK_LOCAL void duk__debug_handle_dump_heap(duk_hthread *thr, duk_heap *heap) {
	DUK_D(DUK_DPRINT("debug command DumpHeap"));

	duk_debug_write_reply(thr);
	duk__debug_dump_heap_allocated(thr, heap);
	duk__debug_dump_strtab(thr, heap);
	duk_debug_write_eom(thr);
}
#endif /* DUK_USE_DEBUGGER_DUMPHEAP */

DUK_LOCAL void duk__debug_handle_get_bytecode(duk_hthread *thr, duk_heap *heap) {
	duk_activation *act;
	duk_hcompfunc *fun = NULL;
	duk_size_t i, n;
	duk_tval *tv;
	duk_hobject **fn;
	duk_int32_t level = -1;
	duk_uint8_t ibyte;

	DUK_UNREF(heap);

	DUK_D(DUK_DPRINT("debug command GetBytecode"));

	ibyte = duk_debug_peek_byte(thr);
	if (ibyte != DUK_DBG_IB_EOM) {
		tv = duk_debug_read_tval(thr);
		if (tv == NULL) {
			/* detached */
			return;
		}
		if (DUK_TVAL_IS_OBJECT(tv)) {
			/* tentative, checked later */
			fun = (duk_hcompfunc *) DUK_TVAL_GET_OBJECT(tv);
			DUK_ASSERT(fun != NULL);
		} else if (DUK_TVAL_IS_NUMBER(tv)) {
			level = (duk_int32_t) DUK_TVAL_GET_NUMBER(tv);
		} else {
			DUK_D(DUK_DPRINT("invalid argument to GetBytecode: %!T", tv));
			goto fail_args;
		}
	}

	if (fun == NULL) {
		act = duk_hthread_get_activation_for_level(thr, level);
		if (act == NULL) {
			goto fail_index;
		}
		fun = (duk_hcompfunc *) DUK_ACT_GET_FUNC(act);
	}

	if (fun == NULL || !DUK_HOBJECT_IS_COMPFUNC((duk_hobject *) fun)) {
		DUK_D(DUK_DPRINT("invalid argument to GetBytecode: %!O", fun));
		goto fail_args;
	}
	DUK_ASSERT(fun != NULL && DUK_HOBJECT_IS_COMPFUNC((duk_hobject *) fun));

	duk_debug_write_reply(thr);
	n = DUK_HCOMPFUNC_GET_CONSTS_COUNT(heap, fun);
	duk_debug_write_int(thr, (duk_int32_t) n);
	tv = DUK_HCOMPFUNC_GET_CONSTS_BASE(heap, fun);
	for (i = 0; i < n; i++) {
		duk_debug_write_tval(thr, tv);
		tv++;
	}
	n = DUK_HCOMPFUNC_GET_FUNCS_COUNT(heap, fun);
	duk_debug_write_int(thr, (duk_int32_t) n);
	fn = DUK_HCOMPFUNC_GET_FUNCS_BASE(heap, fun);
	for (i = 0; i < n; i++) {
		duk_debug_write_hobject(thr, *fn);
		fn++;
	}
	duk_debug_write_string(thr,
	                       (const char *) DUK_HCOMPFUNC_GET_CODE_BASE(heap, fun),
	                       (duk_size_t) DUK_HCOMPFUNC_GET_CODE_SIZE(heap, fun));
	duk_debug_write_eom(thr);
	return;

fail_args:
	duk_debug_write_error_eom(thr, DUK_DBG_ERR_UNKNOWN, "invalid argument");
	return;

fail_index:
	duk_debug_write_error_eom(thr, DUK_DBG_ERR_NOTFOUND, "invalid callstack index");
	return;
}

/*
 *  Object inspection commands: GetHeapObjInfo, GetObjPropDesc,
 *  GetObjPropDescRange
 */

#if defined(DUK_USE_DEBUGGER_INSPECT)

#if 0 /* pruned */
DUK_LOCAL const char * const duk__debug_getinfo_heaphdr_keys[] = {
	"reachable",
	"temproot",
	"finalizable",
	"finalized",
	"readonly"
	/* NULL not needed here */
};
DUK_LOCAL duk_uint_t duk__debug_getinfo_heaphdr_masks[] = {
	DUK_HEAPHDR_FLAG_REACHABLE,
	DUK_HEAPHDR_FLAG_TEMPROOT,
	DUK_HEAPHDR_FLAG_FINALIZABLE,
	DUK_HEAPHDR_FLAG_FINALIZED,
	DUK_HEAPHDR_FLAG_READONLY,
	0  /* terminator */
};
#endif
DUK_LOCAL const char * const duk__debug_getinfo_hstring_keys[] = {
#if 0
	"arridx",
	"symbol",
	"hidden",
	"reserved_word",
	"strict_reserved_word",
	"eval_or_arguments",
#endif
	"extdata"
	/* NULL not needed here */
};
DUK_LOCAL duk_uint_t duk__debug_getinfo_hstring_masks[] = {
#if 0
	DUK_HSTRING_FLAG_ARRIDX,
	DUK_HSTRING_FLAG_SYMBOL,
	DUK_HSTRING_FLAG_HIDDEN,
	DUK_HSTRING_FLAG_RESERVED_WORD,
	DUK_HSTRING_FLAG_STRICT_RESERVED_WORD,
	DUK_HSTRING_FLAG_EVAL_OR_ARGUMENTS,
#endif
	DUK_HSTRING_FLAG_EXTDATA,
	0 /* terminator */
};
DUK_LOCAL const char * const duk__debug_getinfo_hobject_keys[] = {
	"extensible",     "constructable", "callable",         "boundfunc",        "compfunc",        "natfunc",     "bufobj",
	"fastrefs",       "array_part",    "strict",           "notail",           "newenv",          "namebinding", "createargs",
	"have_finalizer", "exotic_array",  "exotic_stringobj", "exotic_arguments", "exotic_proxyobj", "special_call"
	/* NULL not needed here */
};
DUK_LOCAL duk_uint_t duk__debug_getinfo_hobject_masks[] = {
	DUK_HOBJECT_FLAG_EXTENSIBLE,      DUK_HOBJECT_FLAG_CONSTRUCTABLE,    DUK_HOBJECT_FLAG_CALLABLE,
	DUK_HOBJECT_FLAG_BOUNDFUNC,       DUK_HOBJECT_FLAG_COMPFUNC,         DUK_HOBJECT_FLAG_NATFUNC,
	DUK_HOBJECT_FLAG_BUFOBJ,          DUK_HOBJECT_FLAG_FASTREFS,         DUK_HOBJECT_FLAG_ARRAY_PART,
	DUK_HOBJECT_FLAG_STRICT,          DUK_HOBJECT_FLAG_NOTAIL,           DUK_HOBJECT_FLAG_NEWENV,
	DUK_HOBJECT_FLAG_NAMEBINDING,     DUK_HOBJECT_FLAG_CREATEARGS,       DUK_HOBJECT_FLAG_HAVE_FINALIZER,
	DUK_HOBJECT_FLAG_EXOTIC_ARRAY,    DUK_HOBJECT_FLAG_EXOTIC_STRINGOBJ, DUK_HOBJECT_FLAG_EXOTIC_ARGUMENTS,
	DUK_HOBJECT_FLAG_EXOTIC_PROXYOBJ, DUK_HOBJECT_FLAG_SPECIAL_CALL,     0 /* terminator */
};
DUK_LOCAL const char * const duk__debug_getinfo_hbuffer_keys[] = {
	"dynamic",
	"external"
	/* NULL not needed here */
};
DUK_LOCAL duk_uint_t duk__debug_getinfo_hbuffer_masks[] = {
	DUK_HBUFFER_FLAG_DYNAMIC,
	DUK_HBUFFER_FLAG_EXTERNAL,
	0 /* terminator */
};

DUK_LOCAL void duk__debug_getinfo_flags_key(duk_hthread *thr, const char *key) {
	duk_debug_write_uint(thr, 0);
	duk_debug_write_cstring(thr, key);
}

DUK_LOCAL void duk__debug_getinfo_prop_uint(duk_hthread *thr, const char *key, duk_uint_t val) {
	duk_debug_write_uint(thr, 0);
	duk_debug_write_cstring(thr, key);
	duk_debug_write_uint(thr, val);
}

DUK_LOCAL void duk__debug_getinfo_prop_int(duk_hthread *thr, const char *key, duk_int_t val) {
	duk_debug_write_uint(thr, 0);
	duk_debug_write_cstring(thr, key);
	duk_debug_write_int(thr, val);
}

DUK_LOCAL void duk__debug_getinfo_prop_bool(duk_hthread *thr, const char *key, duk_bool_t val) {
	duk_debug_write_uint(thr, 0);
	duk_debug_write_cstring(thr, key);
	duk_debug_write_boolean(thr, val);
}

DUK_LOCAL void duk__debug_getinfo_bitmask(duk_hthread *thr, const char * const *keys, duk_uint_t *masks, duk_uint_t flags) {
	const char *key;
	duk_uint_t mask;

	for (;;) {
		mask = *masks++;
		if (mask == 0) {
			break;
		}
		key = *keys++;
		DUK_ASSERT(key != NULL);

		DUK_DD(DUK_DDPRINT("inspect bitmask: key=%s, mask=0x%08lx, flags=0x%08lx",
		                   key,
		                   (unsigned long) mask,
		                   (unsigned long) flags));
		duk__debug_getinfo_prop_bool(thr, key, flags & mask);
	}
}

/* Inspect a property using a virtual index into a conceptual property list
 * consisting of (1) all array part items from [0,a_size[ (even when above
 * .length) and (2) all entry part items from [0,e_next[.  Unused slots are
 * indicated using dvalue 'unused'.
 */
DUK_LOCAL duk_bool_t duk__debug_getprop_index(duk_hthread *thr, duk_heap *heap, duk_hobject *h_obj, duk_uint_t idx) {
	duk_uint_t a_size;
	duk_tval *tv;
	duk_hstring *h_key;
	duk_hobject *h_getset;
	duk_uint_t flags;

	DUK_UNREF(heap);

	a_size = DUK_HOBJECT_GET_ASIZE(h_obj);
	if (idx < a_size) {
		duk_debug_write_uint(thr, DUK_PROPDESC_FLAGS_WEC);
		duk_debug_write_uint(thr, idx);
		tv = DUK_HOBJECT_A_GET_VALUE_PTR(heap, h_obj, idx);
		duk_debug_write_tval(thr, tv);
		return 1;
	}

	idx -= a_size;
	if (idx >= DUK_HOBJECT_GET_ENEXT(h_obj)) {
		return 0;
	}

	h_key = DUK_HOBJECT_E_GET_KEY(heap, h_obj, idx);
	if (h_key == NULL) {
		duk_debug_write_uint(thr, 0);
		duk_debug_write_null(thr);
		duk_debug_write_unused(thr);
		return 1;
	}

	flags = DUK_HOBJECT_E_GET_FLAGS(heap, h_obj, idx);
	if (DUK_HSTRING_HAS_SYMBOL(h_key)) {
		flags |= DUK_DBG_PROPFLAG_SYMBOL;
	}
	if (DUK_HSTRING_HAS_HIDDEN(h_key)) {
		flags |= DUK_DBG_PROPFLAG_HIDDEN;
	}
	duk_debug_write_uint(thr, flags);
	duk_debug_write_hstring(thr, h_key);
	if (flags & DUK_PROPDESC_FLAG_ACCESSOR) {
		h_getset = DUK_HOBJECT_E_GET_VALUE_GETTER(heap, h_obj, idx);
		if (h_getset) {
			duk_debug_write_hobject(thr, h_getset);
		} else {
			duk_debug_write_null(thr);
		}
		h_getset = DUK_HOBJECT_E_GET_VALUE_SETTER(heap, h_obj, idx);
		if (h_getset) {
			duk_debug_write_hobject(thr, h_getset);
		} else {
			duk_debug_write_null(thr);
		}
	} else {
		tv = DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(heap, h_obj, idx);
		duk_debug_write_tval(thr, tv);
	}
	return 1;
}

DUK_LOCAL void duk__debug_handle_get_heap_obj_info(duk_hthread *thr, duk_heap *heap) {
	duk_heaphdr *h;

	DUK_D(DUK_DPRINT("debug command GetHeapObjInfo"));
	DUK_UNREF(heap);

	DUK_ASSERT(sizeof(duk__debug_getinfo_hstring_keys) / sizeof(const char *) ==
	           sizeof(duk__debug_getinfo_hstring_masks) / sizeof(duk_uint_t) - 1);
	DUK_ASSERT(sizeof(duk__debug_getinfo_hobject_keys) / sizeof(const char *) ==
	           sizeof(duk__debug_getinfo_hobject_masks) / sizeof(duk_uint_t) - 1);
	DUK_ASSERT(sizeof(duk__debug_getinfo_hbuffer_keys) / sizeof(const char *) ==
	           sizeof(duk__debug_getinfo_hbuffer_masks) / sizeof(duk_uint_t) - 1);

	h = duk_debug_read_any_ptr(thr);
	if (!h) {
		duk_debug_write_error_eom(thr, DUK_DBG_ERR_UNKNOWN, "invalid target");
		return;
	}

	duk_debug_write_reply(thr);

	/* As with all inspection code, we rely on the debug client providing
	 * a valid, non-stale pointer: there's no portable way to safely
	 * validate the pointer here.
	 */

	duk__debug_getinfo_flags_key(thr, "heapptr");
	duk_debug_write_heapptr(thr, h);

	/* XXX: comes out as signed now */
	duk__debug_getinfo_prop_uint(thr, "heaphdr_flags", (duk_uint_t) DUK_HEAPHDR_GET_FLAGS(h));
	duk__debug_getinfo_prop_uint(thr, "heaphdr_type", (duk_uint_t) DUK_HEAPHDR_GET_TYPE(h));
#if defined(DUK_USE_REFERENCE_COUNTING)
	duk__debug_getinfo_prop_uint(thr, "refcount", (duk_uint_t) DUK_HEAPHDR_GET_REFCOUNT(h));
#endif
#if 0 /* pruned */
	duk__debug_getinfo_bitmask(thr,
	                           duk__debug_getinfo_heaphdr_keys,
	                           duk__debug_getinfo_heaphdr_masks,
	                           DUK_HEAPHDR_GET_FLAGS_RAW(h));
#endif

	switch (DUK_HEAPHDR_GET_TYPE(h)) {
	case DUK_HTYPE_STRING: {
		duk_hstring *h_str;

		h_str = (duk_hstring *) h;
		duk__debug_getinfo_bitmask(thr,
		                           duk__debug_getinfo_hstring_keys,
		                           duk__debug_getinfo_hstring_masks,
		                           DUK_HEAPHDR_GET_FLAGS_RAW(h));
		duk__debug_getinfo_prop_uint(thr, "bytelen", (duk_uint_t) DUK_HSTRING_GET_BYTELEN(h_str));
		duk__debug_getinfo_prop_uint(thr, "charlen", (duk_uint_t) DUK_HSTRING_GET_CHARLEN(h_str));
		duk__debug_getinfo_prop_uint(thr, "hash", (duk_uint_t) DUK_HSTRING_GET_HASH(h_str));
		duk__debug_getinfo_flags_key(thr, "data");
		duk_debug_write_hstring(thr, h_str);
		break;
	}
	case DUK_HTYPE_OBJECT: {
		duk_hobject *h_obj;
		duk_hobject *h_proto;

		h_obj = (duk_hobject *) h;
		h_proto = DUK_HOBJECT_GET_PROTOTYPE(heap, h_obj);

		/* duk_hobject specific fields. */
		duk__debug_getinfo_bitmask(thr,
		                           duk__debug_getinfo_hobject_keys,
		                           duk__debug_getinfo_hobject_masks,
		                           DUK_HEAPHDR_GET_FLAGS_RAW(h));
		duk__debug_getinfo_prop_uint(thr, "class_number", DUK_HOBJECT_GET_CLASS_NUMBER(h_obj));
		duk__debug_getinfo_flags_key(thr, "class_name");
		duk_debug_write_hstring(thr, DUK_HOBJECT_GET_CLASS_STRING(heap, h_obj));
		duk__debug_getinfo_flags_key(thr, "prototype");
		if (h_proto != NULL) {
			duk_debug_write_hobject(thr, h_proto);
		} else {
			duk_debug_write_null(thr);
		}
		duk__debug_getinfo_flags_key(thr, "props");
		duk_debug_write_pointer(thr, (void *) DUK_HOBJECT_GET_PROPS(heap, h_obj));
		duk__debug_getinfo_prop_uint(thr, "e_size", (duk_uint_t) DUK_HOBJECT_GET_ESIZE(h_obj));
		duk__debug_getinfo_prop_uint(thr, "e_next", (duk_uint_t) DUK_HOBJECT_GET_ENEXT(h_obj));
		duk__debug_getinfo_prop_uint(thr, "a_size", (duk_uint_t) DUK_HOBJECT_GET_ASIZE(h_obj));
		duk__debug_getinfo_prop_uint(thr, "h_size", (duk_uint_t) DUK_HOBJECT_GET_HSIZE(h_obj));

		if (DUK_HOBJECT_IS_ARRAY(h_obj)) {
			duk_harray *h_arr;
			h_arr = (duk_harray *) h_obj;

			duk__debug_getinfo_prop_uint(thr, "length", (duk_uint_t) h_arr->length);
			duk__debug_getinfo_prop_bool(thr, "length_nonwritable", h_arr->length_nonwritable);
		}

		if (DUK_HOBJECT_IS_NATFUNC(h_obj)) {
			duk_hnatfunc *h_fun;
			h_fun = (duk_hnatfunc *) h_obj;

			duk__debug_getinfo_prop_int(thr, "nargs", h_fun->nargs);
			duk__debug_getinfo_prop_int(thr, "magic", h_fun->magic);
			duk__debug_getinfo_prop_bool(thr, "varargs", h_fun->magic == DUK_HNATFUNC_NARGS_VARARGS);
			/* Native function pointer may be different from a void pointer,
			 * and we serialize it from memory directly now (no byte swapping etc).
			 */
			duk__debug_getinfo_flags_key(thr, "funcptr");
			duk_debug_write_buffer(thr, (const char *) &h_fun->func, sizeof(h_fun->func));
		}

		if (DUK_HOBJECT_IS_COMPFUNC(h_obj)) {
			duk_hcompfunc *h_fun;
			duk_hbuffer *h_buf;
			duk_hobject *h_lexenv;
			duk_hobject *h_varenv;
			h_fun = (duk_hcompfunc *) h_obj;

			duk__debug_getinfo_prop_int(thr, "nregs", h_fun->nregs);
			duk__debug_getinfo_prop_int(thr, "nargs", h_fun->nargs);

			duk__debug_getinfo_flags_key(thr, "lex_env");
			h_lexenv = DUK_HCOMPFUNC_GET_LEXENV(thr->heap, h_fun);
			if (h_lexenv != NULL) {
				duk_debug_write_hobject(thr, h_lexenv);
			} else {
				duk_debug_write_null(thr);
			}
			duk__debug_getinfo_flags_key(thr, "var_env");
			h_varenv = DUK_HCOMPFUNC_GET_VARENV(thr->heap, h_fun);
			if (h_varenv != NULL) {
				duk_debug_write_hobject(thr, h_varenv);
			} else {
				duk_debug_write_null(thr);
			}

			duk__debug_getinfo_prop_uint(thr, "start_line", h_fun->start_line);
			duk__debug_getinfo_prop_uint(thr, "end_line", h_fun->end_line);
			h_buf = (duk_hbuffer *) DUK_HCOMPFUNC_GET_DATA(thr->heap, h_fun);
			if (h_buf != NULL) {
				duk__debug_getinfo_flags_key(thr, "data");
				duk_debug_write_heapptr(thr, (duk_heaphdr *) h_buf);
			}
		}

		if (DUK_HOBJECT_IS_BOUNDFUNC(h_obj)) {
			duk_hboundfunc *h_bfun;
			h_bfun = (duk_hboundfunc *) (void *) h_obj;

			duk__debug_getinfo_flags_key(thr, "target");
			duk_debug_write_tval(thr, &h_bfun->target);
			duk__debug_getinfo_flags_key(thr, "this_binding");
			duk_debug_write_tval(thr, &h_bfun->this_binding);
			duk__debug_getinfo_flags_key(thr, "nargs");
			duk_debug_write_int(thr, h_bfun->nargs);
			/* h_bfun->args not exposed now */
		}

		if (DUK_HOBJECT_IS_THREAD(h_obj)) {
			/* XXX: Currently no inspection of threads, e.g. value stack, call
			 * stack, catch stack, etc.
			 */
			duk_hthread *h_thr;
			h_thr = (duk_hthread *) h_obj;
			DUK_UNREF(h_thr);
		}

		if (DUK_HOBJECT_IS_DECENV(h_obj)) {
			duk_hdecenv *h_env;
			h_env = (duk_hdecenv *) h_obj;

			duk__debug_getinfo_flags_key(thr, "thread");
			duk_debug_write_heapptr(thr, (duk_heaphdr *) (h_env->thread));
			duk__debug_getinfo_flags_key(thr, "varmap");
			duk_debug_write_heapptr(thr, (duk_heaphdr *) (h_env->varmap));
			duk__debug_getinfo_prop_uint(thr, "regbase", (duk_uint_t) h_env->regbase_byteoff);
		}

		if (DUK_HOBJECT_IS_OBJENV(h_obj)) {
			duk_hobjenv *h_env;
			h_env = (duk_hobjenv *) h_obj;

			duk__debug_getinfo_flags_key(thr, "target");
			duk_debug_write_heapptr(thr, (duk_heaphdr *) (h_env->target));
			duk__debug_getinfo_prop_bool(thr, "has_this", h_env->has_this);
		}

#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
		if (DUK_HOBJECT_IS_BUFOBJ(h_obj)) {
			duk_hbufobj *h_bufobj;
			h_bufobj = (duk_hbufobj *) h_obj;

			duk__debug_getinfo_prop_uint(thr, "slice_offset", h_bufobj->offset);
			duk__debug_getinfo_prop_uint(thr, "slice_length", h_bufobj->length);
			duk__debug_getinfo_prop_uint(thr, "elem_shift", (duk_uint_t) h_bufobj->shift);
			duk__debug_getinfo_prop_uint(thr, "elem_type", (duk_uint_t) h_bufobj->elem_type);
			duk__debug_getinfo_prop_bool(thr, "is_typedarray", (duk_uint_t) h_bufobj->is_typedarray);
			if (h_bufobj->buf != NULL) {
				duk__debug_getinfo_flags_key(thr, "buffer");
				duk_debug_write_heapptr(thr, (duk_heaphdr *) h_bufobj->buf);
			}
		}
#endif /* DUK_USE_BUFFEROBJECT_SUPPORT */
		break;
	}
	case DUK_HTYPE_BUFFER: {
		duk_hbuffer *h_buf;

		h_buf = (duk_hbuffer *) h;
		duk__debug_getinfo_bitmask(thr,
		                           duk__debug_getinfo_hbuffer_keys,
		                           duk__debug_getinfo_hbuffer_masks,
		                           DUK_HEAPHDR_GET_FLAGS_RAW(h));
		duk__debug_getinfo_prop_uint(thr, "size", (duk_uint_t) DUK_HBUFFER_GET_SIZE(h_buf));
		duk__debug_getinfo_flags_key(thr, "dataptr");
		duk_debug_write_pointer(thr, (void *) DUK_HBUFFER_GET_DATA_PTR(thr->heap, h_buf));
		duk__debug_getinfo_flags_key(thr, "data");
		duk_debug_write_hbuffer(thr, h_buf); /* tolerates NULL h_buf */
		break;
	}
	default: {
		/* Since we already started writing the reply, just emit nothing. */
		DUK_D(DUK_DPRINT("inspect target pointer has invalid heaphdr type"));
	}
	}

	duk_debug_write_eom(thr);
}

DUK_LOCAL void duk__debug_handle_get_obj_prop_desc(duk_hthread *thr, duk_heap *heap) {
	duk_heaphdr *h;
	duk_hobject *h_obj;
	duk_hstring *h_key;
	duk_propdesc desc;

	DUK_D(DUK_DPRINT("debug command GetObjPropDesc"));
	DUK_UNREF(heap);

	h = duk_debug_read_any_ptr(thr);
	if (!h) {
		duk_debug_write_error_eom(thr, DUK_DBG_ERR_UNKNOWN, "invalid target");
		return;
	}
	h_key = duk_debug_read_hstring(thr);
	if (h == NULL || DUK_HEAPHDR_GET_TYPE(h) != DUK_HTYPE_OBJECT || h_key == NULL) {
		goto fail_args;
	}
	h_obj = (duk_hobject *) h;

	if (duk_hobject_get_own_propdesc(thr, h_obj, h_key, &desc, 0 /*flags*/)) {
		duk_int_t virtual_idx;
		duk_bool_t rc;

		/* To use the shared helper need the virtual index. */
		DUK_ASSERT(desc.e_idx >= 0 || desc.a_idx >= 0);
		virtual_idx = (desc.a_idx >= 0 ? desc.a_idx : (duk_int_t) DUK_HOBJECT_GET_ASIZE(h_obj) + desc.e_idx);

		duk_debug_write_reply(thr);
		rc = duk__debug_getprop_index(thr, heap, h_obj, (duk_uint_t) virtual_idx);
		DUK_ASSERT(rc == 1);
		DUK_UNREF(rc);
		duk_debug_write_eom(thr);
	} else {
		duk_debug_write_error_eom(thr, DUK_DBG_ERR_NOTFOUND, "not found");
	}
	return;

fail_args:
	duk_debug_write_error_eom(thr, DUK_DBG_ERR_UNKNOWN, "invalid args");
}

DUK_LOCAL void duk__debug_handle_get_obj_prop_desc_range(duk_hthread *thr, duk_heap *heap) {
	duk_heaphdr *h;
	duk_hobject *h_obj;
	duk_uint_t idx, idx_start, idx_end;

	DUK_D(DUK_DPRINT("debug command GetObjPropDescRange"));
	DUK_UNREF(heap);

	h = duk_debug_read_any_ptr(thr);
	idx_start = (duk_uint_t) duk_debug_read_int(thr);
	idx_end = (duk_uint_t) duk_debug_read_int(thr);
	if (h == NULL || DUK_HEAPHDR_GET_TYPE(h) != DUK_HTYPE_OBJECT) {
		goto fail_args;
	}
	h_obj = (duk_hobject *) h;

	/* The index range space is conceptually the array part followed by the
	 * entry part.  Unlike normal enumeration all slots are exposed here as
	 * is and return 'unused' if the slots are not in active use.  In particular
	 * the array part is included for the full a_size regardless of what the
	 * array .length is.
	 */

	duk_debug_write_reply(thr);
	for (idx = idx_start; idx < idx_end; idx++) {
		if (!duk__debug_getprop_index(thr, heap, h_obj, idx)) {
			break;
		}
	}
	duk_debug_write_eom(thr);
	return;

fail_args:
	duk_debug_write_error_eom(thr, DUK_DBG_ERR_UNKNOWN, "invalid args");
}

#endif /* DUK_USE_DEBUGGER_INSPECT */

/*
 *  Process incoming debug requests
 *
 *  Individual request handlers can push temporaries on the value stack and
 *  rely on duk__debug_process_message() to restore the value stack top
 *  automatically.
 */

/* Process one debug message.  Automatically restore value stack top to its
 * entry value, so that individual message handlers don't need exact value
 * stack handling which is convenient.
 */
DUK_LOCAL void duk__debug_process_message(duk_hthread *thr) {
	duk_heap *heap;
	duk_uint8_t x;
	duk_int32_t cmd;
	duk_idx_t entry_top;

	DUK_ASSERT(thr != NULL);
	heap = thr->heap;
	DUK_ASSERT(heap != NULL);

	entry_top = duk_get_top(thr);

	x = duk_debug_read_byte(thr);
	switch (x) {
	case DUK_DBG_IB_REQUEST: {
		cmd = duk_debug_read_int(thr);
		switch (cmd) {
		case DUK_DBG_CMD_BASICINFO: {
			duk__debug_handle_basic_info(thr, heap);
			break;
		}
		case DUK_DBG_CMD_TRIGGERSTATUS: {
			duk__debug_handle_trigger_status(thr, heap);
			break;
		}
		case DUK_DBG_CMD_PAUSE: {
			duk__debug_handle_pause(thr, heap);
			break;
		}
		case DUK_DBG_CMD_RESUME: {
			duk__debug_handle_resume(thr, heap);
			break;
		}
		case DUK_DBG_CMD_STEPINTO:
		case DUK_DBG_CMD_STEPOVER:
		case DUK_DBG_CMD_STEPOUT: {
			duk__debug_handle_step(thr, heap, cmd);
			break;
		}
		case DUK_DBG_CMD_LISTBREAK: {
			duk__debug_handle_list_break(thr, heap);
			break;
		}
		case DUK_DBG_CMD_ADDBREAK: {
			duk__debug_handle_add_break(thr, heap);
			break;
		}
		case DUK_DBG_CMD_DELBREAK: {
			duk__debug_handle_del_break(thr, heap);
			break;
		}
		case DUK_DBG_CMD_GETVAR: {
			duk__debug_handle_get_var(thr, heap);
			break;
		}
		case DUK_DBG_CMD_PUTVAR: {
			duk__debug_handle_put_var(thr, heap);
			break;
		}
		case DUK_DBG_CMD_GETCALLSTACK: {
			duk__debug_handle_get_call_stack(thr, heap);
			break;
		}
		case DUK_DBG_CMD_GETLOCALS: {
			duk__debug_handle_get_locals(thr, heap);
			break;
		}
		case DUK_DBG_CMD_EVAL: {
			duk__debug_handle_eval(thr, heap);
			break;
		}
		case DUK_DBG_CMD_DETACH: {
			/* The actual detached_cb call is postponed to message loop so
			 * we don't need any special precautions here (just skip to EOM
			 * on the already closed connection).
			 */
			duk__debug_handle_detach(thr, heap);
			break;
		}
#if defined(DUK_USE_DEBUGGER_DUMPHEAP)
		case DUK_DBG_CMD_DUMPHEAP: {
			duk__debug_handle_dump_heap(thr, heap);
			break;
		}
#endif /* DUK_USE_DEBUGGER_DUMPHEAP */
		case DUK_DBG_CMD_GETBYTECODE: {
			duk__debug_handle_get_bytecode(thr, heap);
			break;
		}
		case DUK_DBG_CMD_APPREQUEST: {
			duk__debug_handle_apprequest(thr, heap);
			break;
		}
#if defined(DUK_USE_DEBUGGER_INSPECT)
		case DUK_DBG_CMD_GETHEAPOBJINFO: {
			duk__debug_handle_get_heap_obj_info(thr, heap);
			break;
		}
		case DUK_DBG_CMD_GETOBJPROPDESC: {
			duk__debug_handle_get_obj_prop_desc(thr, heap);
			break;
		}
		case DUK_DBG_CMD_GETOBJPROPDESCRANGE: {
			duk__debug_handle_get_obj_prop_desc_range(thr, heap);
			break;
		}
#endif /* DUK_USE_DEBUGGER_INSPECT */
		default: {
			DUK_D(DUK_DPRINT("debug command unsupported: %d", (int) cmd));
			duk_debug_write_error_eom(thr, DUK_DBG_ERR_UNSUPPORTED, "unsupported command");
		}
		} /* switch cmd */
		break;
	}
	case DUK_DBG_IB_REPLY: {
		DUK_D(DUK_DPRINT("debug reply, skipping"));
		break;
	}
	case DUK_DBG_IB_ERROR: {
		DUK_D(DUK_DPRINT("debug error, skipping"));
		break;
	}
	case DUK_DBG_IB_NOTIFY: {
		DUK_D(DUK_DPRINT("debug notify, skipping"));
		break;
	}
	default: {
		DUK_D(DUK_DPRINT("invalid initial byte, drop connection: %d", (int) x));
		goto fail;
	}
	} /* switch initial byte */

	DUK_ASSERT(duk_get_top(thr) >= entry_top);
	duk_set_top(thr, entry_top);
	duk__debug_skip_to_eom(thr);
	return;

fail:
	DUK_ASSERT(duk_get_top(thr) >= entry_top);
	duk_set_top(thr, entry_top);
	DUK__SET_CONN_BROKEN(thr, 1);
	return;
}

DUK_LOCAL void duk__check_resend_status(duk_hthread *thr) {
	if (thr->heap->dbg_read_cb != NULL && thr->heap->dbg_state_dirty) {
		duk_debug_send_status(thr);
		thr->heap->dbg_state_dirty = 0;
	}
}

DUK_INTERNAL duk_bool_t duk_debug_process_messages(duk_hthread *thr, duk_bool_t no_block) {
#if defined(DUK_USE_ASSERTIONS)
	duk_idx_t entry_top;
#endif
	duk_bool_t retval = 0;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
#if defined(DUK_USE_ASSERTIONS)
	entry_top = duk_get_top(thr);
#endif

	DUK_D(DUK_DPRINT("process debug messages: read_cb=%s, no_block=%ld, detaching=%ld, processing=%ld",
	                 thr->heap->dbg_read_cb ? "not NULL" : "NULL",
	                 (long) no_block,
	                 (long) thr->heap->dbg_detaching,
	                 (long) thr->heap->dbg_processing));
	DUK_DD(DUK_DDPRINT("top at entry: %ld", (long) duk_get_top(thr)));

	/* thr->heap->dbg_detaching may be != 0 if a debugger write outside
	 * the message loop caused a transport error and detach1() to run.
	 */
	DUK_ASSERT(thr->heap->dbg_detaching == 0 || thr->heap->dbg_detaching == 1);
	DUK_ASSERT(thr->heap->dbg_processing == 0);
	thr->heap->dbg_processing = 1;

	/* Ensure dirty state causes a Status even if never process any
	 * messages.  This is expected by the bytecode executor when in
	 * the running state.
	 */
	duk__check_resend_status(thr);

	for (;;) {
		/* Process messages until we're no longer paused or we peek
		 * and see there's nothing to read right now.
		 */
		DUK_DD(DUK_DDPRINT("top at loop top: %ld", (long) duk_get_top(thr)));
		DUK_ASSERT(thr->heap->dbg_processing == 1);

		while (thr->heap->dbg_read_cb == NULL && thr->heap->dbg_detaching) {
			/* Detach is pending; can be triggered from outside the
			 * debugger loop (e.g. Status notify write error) or by
			 * previous message handling.  Call detached callback
			 * here, in a controlled state, to ensure a possible
			 * reattach inside the detached_cb is handled correctly.
			 *
			 * Recheck for detach in a while loop: an immediate
			 * reattach involves a call to duk_debugger_attach()
			 * which writes a debugger handshake line immediately
			 * inside the API call.  If the transport write fails
			 * for that handshake, we can immediately end up in a
			 * "transport broken, detaching" case several times here.
			 * Loop back until we're either cleanly attached or
			 * fully detached.
			 *
			 * NOTE: Reset dbg_processing = 1 forcibly, in case we
			 * re-attached; duk_debugger_attach() sets dbg_processing
			 * to 0 at the moment.
			 */

			DUK_D(DUK_DPRINT("detach pending (dbg_read_cb == NULL, dbg_detaching != 0), call detach2"));

			duk__debug_do_detach2(thr->heap);
			thr->heap->dbg_processing = 1; /* may be set to 0 by duk_debugger_attach() inside callback */

			DUK_D(DUK_DPRINT("after detach2 (and possible reattach): dbg_read_cb=%s, dbg_detaching=%ld",
			                 thr->heap->dbg_read_cb ? "not NULL" : "NULL",
			                 (long) thr->heap->dbg_detaching));
		}
		DUK_ASSERT(thr->heap->dbg_detaching == 0); /* true even with reattach */
		DUK_ASSERT(thr->heap->dbg_processing == 1); /* even after a detach and possible reattach */

		if (thr->heap->dbg_read_cb == NULL) {
			DUK_D(DUK_DPRINT("debug connection broken (and not detaching), stop processing messages"));
			break;
		}

		if (!DUK_HEAP_HAS_DEBUGGER_PAUSED(thr->heap) || no_block) {
			if (!duk_debug_read_peek(thr)) {
				/* Note: peek cannot currently trigger a detach
				 * so the dbg_detaching == 0 assert outside the
				 * loop is correct.
				 */
				DUK_D(DUK_DPRINT("processing debug message, peek indicated no data, stop processing messages"));
				break;
			}
			DUK_D(DUK_DPRINT("processing debug message, peek indicated there is data, handle it"));
		} else {
			DUK_D(DUK_DPRINT("paused, process debug message, blocking if necessary"));
		}

		duk__check_resend_status(thr);
		duk__debug_process_message(thr);
		duk__check_resend_status(thr);

		retval = 1; /* processed one or more messages */
	}

	DUK_ASSERT(thr->heap->dbg_detaching == 0);
	DUK_ASSERT(thr->heap->dbg_processing == 1);
	thr->heap->dbg_processing = 0;

	/* As an initial implementation, read flush after exiting the message
	 * loop.  If transport is broken, this is a no-op (with debug logs).
	 */
	duk_debug_read_flush(thr); /* this cannot initiate a detach */
	DUK_ASSERT(thr->heap->dbg_detaching == 0);

	DUK_DD(DUK_DDPRINT("top at exit: %ld", (long) duk_get_top(thr)));

#if defined(DUK_USE_ASSERTIONS)
	/* Easy to get wrong, so assert for it. */
	DUK_ASSERT(entry_top == duk_get_top(thr));
#endif

	return retval;
}

/*
 *  Halt execution helper
 */

/* Halt execution and enter a debugger message loop until execution is resumed
 * by the client.  PC for the current activation may be temporarily decremented
 * so that the "current" instruction will be shown by the client.  This helper
 * is callable from anywhere, also outside bytecode executor.
 */

DUK_INTERNAL void duk_debug_halt_execution(duk_hthread *thr, duk_bool_t use_prev_pc) {
	duk_activation *act;
	duk_hcompfunc *fun;
	duk_instr_t *old_pc = NULL;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	DUK_ASSERT(duk_debug_is_attached(thr->heap));
	DUK_ASSERT(thr->heap->dbg_processing == 0);
	DUK_ASSERT(!duk_debug_is_paused(thr->heap));

	duk_debug_set_paused(thr->heap);

	act = thr->callstack_curr;

	/* NOTE: act may be NULL if an error is thrown outside of any activation,
	 * which may happen in the case of, e.g. syntax errors.
	 */

	/* Decrement PC if that was requested, this requires a PC sync. */
	if (act != NULL) {
		duk_hthread_sync_currpc(thr);
		old_pc = act->curr_pc;
		fun = (duk_hcompfunc *) DUK_ACT_GET_FUNC(act);

		/* Short circuit if is safe: if act->curr_pc != NULL, 'fun' is
		 * guaranteed to be a non-NULL ECMAScript function.
		 */
		DUK_ASSERT(act->curr_pc == NULL || (fun != NULL && DUK_HOBJECT_IS_COMPFUNC((duk_hobject *) fun)));
		if (use_prev_pc && act->curr_pc != NULL && act->curr_pc > DUK_HCOMPFUNC_GET_CODE_BASE(thr->heap, fun)) {
			act->curr_pc--;
		}
	}

	/* Process debug messages until we are no longer paused. */

	/* NOTE: This is a bit fragile.  It's important to ensure that
	 * duk_debug_process_messages() never throws an error or
	 * act->curr_pc will never be reset.
	 */

	thr->heap->dbg_state_dirty = 1;
	while (DUK_HEAP_HAS_DEBUGGER_PAUSED(thr->heap)) {
		DUK_ASSERT(duk_debug_is_attached(thr->heap));
		DUK_ASSERT(thr->heap->dbg_processing == 0);
		duk_debug_process_messages(thr, 0 /*no_block*/);
	}

	/* XXX: Decrementing and restoring act->curr_pc works now, but if the
	 * debugger message loop gains the ability to adjust the current PC
	 * (e.g. a forced jump) restoring the PC here will break.  Another
	 * approach would be to use a state flag for the "decrement 1 from
	 * topmost activation's PC" and take it into account whenever dealing
	 * with PC values.
	 */
	if (act != NULL) {
		act->curr_pc = old_pc; /* restore PC */
	}
}

/*
 *  Breakpoint management
 */

DUK_INTERNAL duk_small_int_t duk_debug_add_breakpoint(duk_hthread *thr, duk_hstring *filename, duk_uint32_t line) {
	duk_heap *heap;
	duk_breakpoint *b;

	/* Caller must trigger recomputation of active breakpoint list.  To
	 * ensure stale values are not used if that doesn't happen, clear the
	 * active breakpoint list here.
	 */

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(filename != NULL);
	heap = thr->heap;
	DUK_ASSERT(heap != NULL);

	if (heap->dbg_breakpoint_count >= DUK_HEAP_MAX_BREAKPOINTS) {
		DUK_D(DUK_DPRINT("failed to add breakpoint for %O:%ld, all breakpoint slots used",
		                 (duk_heaphdr *) filename,
		                 (long) line));
		return -1;
	}
	heap->dbg_breakpoints_active[0] = (duk_breakpoint *) NULL;
	b = heap->dbg_breakpoints + (heap->dbg_breakpoint_count++);
	b->filename = filename;
	b->line = line;
	DUK_HSTRING_INCREF(thr, filename);

	return (duk_small_int_t) (heap->dbg_breakpoint_count - 1); /* index */
}

DUK_INTERNAL duk_bool_t duk_debug_remove_breakpoint(duk_hthread *thr, duk_small_uint_t breakpoint_index) {
	duk_heap *heap;
	duk_hstring *h;
	duk_breakpoint *b;
	duk_size_t move_size;

	/* Caller must trigger recomputation of active breakpoint list.  To
	 * ensure stale values are not used if that doesn't happen, clear the
	 * active breakpoint list here.
	 */

	DUK_ASSERT(thr != NULL);
	heap = thr->heap;
	DUK_ASSERT(heap != NULL);
	DUK_ASSERT(duk_debug_is_attached(thr->heap));
	DUK_ASSERT_DISABLE(breakpoint_index >= 0); /* unsigned */

	if (breakpoint_index >= heap->dbg_breakpoint_count) {
		DUK_D(DUK_DPRINT("invalid breakpoint index: %ld", (long) breakpoint_index));
		return 0;
	}
	b = heap->dbg_breakpoints + breakpoint_index;

	h = b->filename;
	DUK_ASSERT(h != NULL);

	move_size = sizeof(duk_breakpoint) * (heap->dbg_breakpoint_count - breakpoint_index - 1);
	duk_memmove((void *) b, (const void *) (b + 1), (size_t) move_size);

	heap->dbg_breakpoint_count--;
	heap->dbg_breakpoints_active[0] = (duk_breakpoint *) NULL;

	DUK_HSTRING_DECREF(thr, h); /* side effects */
	DUK_UNREF(h); /* w/o refcounting */

	/* Breakpoint entries above the used area are left as garbage. */

	return 1;
}

/*
 *  Misc state management
 */

DUK_INTERNAL duk_bool_t duk_debug_is_attached(duk_heap *heap) {
	return (heap->dbg_read_cb != NULL);
}

DUK_INTERNAL duk_bool_t duk_debug_is_paused(duk_heap *heap) {
	return (DUK_HEAP_HAS_DEBUGGER_PAUSED(heap) != 0);
}

DUK_INTERNAL void duk_debug_set_paused(duk_heap *heap) {
	if (duk_debug_is_paused(heap)) {
		DUK_D(DUK_DPRINT("trying to set paused state when already paused, ignoring"));
	} else {
		DUK_HEAP_SET_DEBUGGER_PAUSED(heap);
		heap->dbg_state_dirty = 1;
		duk_debug_clear_pause_state(heap);
		DUK_ASSERT(heap->ms_running == 0); /* debugger can't be triggered within mark-and-sweep */
		heap->ms_running = 2; /* prevent mark-and-sweep, prevent refzero queueing */
		heap->ms_prevent_count++;
		DUK_ASSERT(heap->ms_prevent_count != 0); /* Wrap. */
		DUK_ASSERT(heap->heap_thread != NULL);
	}
}

DUK_INTERNAL void duk_debug_clear_paused(duk_heap *heap) {
	if (duk_debug_is_paused(heap)) {
		DUK_HEAP_CLEAR_DEBUGGER_PAUSED(heap);
		heap->dbg_state_dirty = 1;
		duk_debug_clear_pause_state(heap);
		DUK_ASSERT(heap->ms_running == 2);
		DUK_ASSERT(heap->ms_prevent_count > 0);
		heap->ms_prevent_count--;
		heap->ms_running = 0;
		DUK_ASSERT(heap->heap_thread != NULL);
	} else {
		DUK_D(DUK_DPRINT("trying to clear paused state when not paused, ignoring"));
	}
}

DUK_INTERNAL void duk_debug_clear_pause_state(duk_heap *heap) {
	heap->dbg_pause_flags = 0;
	heap->dbg_pause_act = NULL;
	heap->dbg_pause_startline = 0;
}

#else /* DUK_USE_DEBUGGER_SUPPORT */

/* No debugger support. */

#endif /* DUK_USE_DEBUGGER_SUPPORT */
