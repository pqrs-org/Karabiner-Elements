/*
 *  Example debug transport with a local debug message encoder/decoder.
 *
 *  Provides a "received dvalue" callback for a fully parsed dvalue (user
 *  code frees dvalue) and a "cooperate" callback for e.g. UI integration.
 *  There are a few other callbacks.  See test.c for usage examples.
 *
 *  This transport implementation is not multithreaded which means that:
 *
 *    - Callbacks to "received dvalue" callback come from the Duktape thread,
 *      either during normal execution or from duk_debugger_cooperate().
 *
 *    - Calls into duk_trans_dvalue_send() must be made from the callbacks
 *      provided (e.g. "received dvalue" or "cooperate") which use the active
 *      Duktape thread.
 *
 *    - The only exception to this is when Duktape is idle: you can then call
 *      duk_trans_dvalue_send() from any thread (only one thread at a time).
 *      When you next call into Duktape or call duk_debugger_cooperate(), the
 *      queued data will be read and processed by Duktape.
 *
 *  There are functions for creating and freeing values; internally they use
 *  malloc() and free() for memory management.  Duktape heap alloc functions
 *  are not used to minimize disturbances to the Duktape heap under debugging.
 *
 *  Doesn't depend on C99 types; assumes "int" is at least 32 bits, and makes
 *  a few assumptions about format specifiers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "duktape.h"
#include "duk_trans_dvalue.h"

/* Define to enable debug prints to stderr. */
#if 0
#define DEBUG_PRINTS
#endif

/* Define to enable error prints to stderr. */
#if 1
#define ERROR_PRINTS
#endif

/*
 *  Dvalue handling
 */

duk_dvalue *duk_dvalue_alloc(void) {
	duk_dvalue *dv = (duk_dvalue *) malloc(sizeof(duk_dvalue));
	if (dv) {
		memset((void *) dv, 0, sizeof(duk_dvalue));
		dv->buf = NULL;
	}
	return dv;
}

void duk_dvalue_free(duk_dvalue *dv) {
	if (dv) {
		free(dv->buf);  /* tolerates NULL */
		dv->buf = NULL;
		free(dv);
	}
}

static void duk__dvalue_bufesc(duk_dvalue *dv, char *buf, size_t maxbytes, int stresc) {
	size_t i, limit;

	*buf = (char) 0;
	limit = dv->len > maxbytes ? maxbytes : dv->len;
	for (i = 0; i < limit; i++) {
		unsigned char c = dv->buf[i];
		if (stresc) {
			if (c >= 0x20 && c <= 0x7e && c != (char) '"' && c != (char) '\'') {
				sprintf(buf, "%c", c);
				buf++;
			} else {
				sprintf(buf, "\\x%02x", (unsigned int) c);
				buf += 4;
			}
		} else {
			sprintf(buf, "%02x", (unsigned int) c);
			buf += 2;
		}
	}
	if (dv->len > maxbytes) {
		sprintf(buf, "...");
		buf += 3;
	}
}

/* Caller must provide a buffer at least DUK_DVALUE_TOSTRING_BUFLEN in size. */
void duk_dvalue_to_string(duk_dvalue *dv, char *buf) {
	char hexbuf[32 * 4 + 4];  /* 32 hex encoded or \xXX escaped bytes, possible "...", NUL */

	if (!dv) {
		sprintf(buf, "NULL");
		return;
	}

	switch (dv->tag) {
	case DUK_DVALUE_EOM:
		sprintf(buf, "EOM");
		break;
	case DUK_DVALUE_REQ:
		sprintf(buf, "REQ");
		break;
	case DUK_DVALUE_REP:
		sprintf(buf, "REP");
		break;
	case DUK_DVALUE_ERR:
		sprintf(buf, "ERR");
		break;
	case DUK_DVALUE_NFY:
		sprintf(buf, "NFY");
		break;
	case DUK_DVALUE_INTEGER:
		sprintf(buf, "%d", dv->i);
		break;
	case DUK_DVALUE_STRING:
		duk__dvalue_bufesc(dv, hexbuf, 32, 1);
		sprintf(buf, "str:%ld:\"%s\"", (long) dv->len, hexbuf);
		break;
	case DUK_DVALUE_BUFFER:
		duk__dvalue_bufesc(dv, hexbuf, 32, 0);
		sprintf(buf, "buf:%ld:%s", (long) dv->len, hexbuf);
		break;
	case DUK_DVALUE_UNUSED:
		sprintf(buf, "undefined");
		break;
	case DUK_DVALUE_UNDEFINED:
		sprintf(buf, "undefined");
		break;
	case DUK_DVALUE_NULL:
		sprintf(buf, "null");
		break;
	case DUK_DVALUE_TRUE:
		sprintf(buf, "true");
		break;
	case DUK_DVALUE_FALSE:
		sprintf(buf, "false");
		break;
	case DUK_DVALUE_NUMBER:
		if (fpclassify(dv->d) == FP_ZERO) {
			if (signbit(dv->d)) {
				sprintf(buf, "-0");
			} else {
				sprintf(buf, "0");
			}
		} else {
			sprintf(buf, "%lg", dv->d);
		}
		break;
	case DUK_DVALUE_OBJECT:
		duk__dvalue_bufesc(dv, hexbuf, 32, 0);
		sprintf(buf, "obj:%d:%s", (int) dv->i, hexbuf);
		break;
	case DUK_DVALUE_POINTER:
		duk__dvalue_bufesc(dv, hexbuf, 32, 0);
		sprintf(buf, "ptr:%s", hexbuf);
		break;
	case DUK_DVALUE_LIGHTFUNC:
		duk__dvalue_bufesc(dv, hexbuf, 32, 0);
		sprintf(buf, "lfunc:%04x:%s", (unsigned int) dv->i, hexbuf);
		break;
	case DUK_DVALUE_HEAPPTR:
		duk__dvalue_bufesc(dv, hexbuf, 32, 0);
		sprintf(buf, "heapptr:%s", hexbuf);
		break;
	default:
		sprintf(buf, "unknown:%d", (int) dv->tag);
	}
}

duk_dvalue *duk_dvalue_make_tag(int tag) {
	duk_dvalue *dv = duk_dvalue_alloc();
	if (!dv) { return NULL; }
	dv->tag = tag;
	return dv;
}

duk_dvalue *duk_dvalue_make_tag_int(int tag, int intval) {
	duk_dvalue *dv = duk_dvalue_alloc();
	if (!dv) { return NULL; }
	dv->tag = tag;
	dv->i = intval;
	return dv;
}

duk_dvalue *duk_dvalue_make_tag_double(int tag, double dblval) {
	duk_dvalue *dv = duk_dvalue_alloc();
	if (!dv) { return NULL; }
	dv->tag = tag;
	dv->d = dblval;
	return dv;
}

duk_dvalue *duk_dvalue_make_tag_data(int tag, const char *buf, size_t len) {
	unsigned char *p;
	duk_dvalue *dv = duk_dvalue_alloc();
	if (!dv) { return NULL; }
	/* Alloc size is len + 1 so that a NUL terminator is always
	 * guaranteed which is convenient, e.g. you can printf() the
	 * value safely.
	 */
	p = (unsigned char *) malloc(len + 1);
	if (!p) {
		free(dv);
		return NULL;
	}
	memcpy((void *) p, (const void *) buf, len);
	p[len] = (unsigned char) 0;
	dv->tag = tag;
	dv->buf = p;
	dv->len = len;
	return dv;
}

duk_dvalue *duk_dvalue_make_tag_int_data(int tag, int intval, const char *buf, size_t len) {
	duk_dvalue *dv = duk_dvalue_make_tag_data(tag, buf, len);
	if (!dv) { return NULL; }
	dv->i = intval;
	return dv;
}

/*
 *  Dvalue transport handling
 */

static void duk__trans_dvalue_double_byteswap(duk_trans_dvalue_ctx *ctx, volatile unsigned char *p) {
	unsigned char t;

	/* Portable IEEE double byteswap.  Relies on runtime detection of
	 * host endianness.
	 */

	if (ctx->double_byteorder == 0) {
		/* little endian */
		t = p[0]; p[0] = p[7]; p[7] = t;
		t = p[1]; p[1] = p[6]; p[6] = t;
		t = p[2]; p[2] = p[5]; p[5] = t;
		t = p[3]; p[3] = p[4]; p[4] = t;
	} else if (ctx->double_byteorder == 1) {
		/* big endian: ok as is */
		;
	} else {
		/* mixed endian */
		t = p[0]; p[0] = p[3]; p[3] = t;
		t = p[1]; p[1] = p[2]; p[2] = t;
		t = p[4]; p[4] = p[7]; p[7] = t;
		t = p[5]; p[5] = p[6]; p[6] = t;
	}
}

static unsigned int duk__trans_dvalue_parse_u32(duk_trans_dvalue_ctx *ctx, unsigned char *p) {
	/* Integers are network endian, read back into host format in
	 * a portable manner.
	 */
	(void) ctx;
	return (((unsigned int) p[0]) << 24) +
	       (((unsigned int) p[1]) << 16) +
	       (((unsigned int) p[2]) << 8) +
	       (((unsigned int) p[3]) << 0);
}

static int duk__trans_dvalue_parse_i32(duk_trans_dvalue_ctx *ctx, unsigned char *p) {
	/* Portable sign handling, doesn't assume 'int' is exactly 32 bits
	 * like a direct cast would.
	 */
	unsigned int tmp = duk__trans_dvalue_parse_u32(ctx, p);
	if (tmp & 0x80000000UL) {
		return -((int) ((tmp ^ 0xffffffffUL) + 1UL));
	} else {
		return tmp;
	}
}

static unsigned int duk__trans_dvalue_parse_u16(duk_trans_dvalue_ctx *ctx, unsigned char *p) {
	/* Integers are network endian, read back into host format. */
	(void) ctx;
	return (((unsigned int) p[0]) << 8) +
	       (((unsigned int) p[1]) << 0);
}

static double duk__trans_dvalue_parse_double(duk_trans_dvalue_ctx *ctx, unsigned char *p) {
	/* IEEE doubles are network endian, read back into host format. */
	volatile union {
		double d;
		unsigned char b[8];
	} u;
	memcpy((void *) u.b, (const void *) p, 8);
	duk__trans_dvalue_double_byteswap(ctx, u.b);
	return u.d;
}

static unsigned char *duk__trans_dvalue_encode_u32(duk_trans_dvalue_ctx *ctx, unsigned char *p, unsigned int val) {
	/* Integers are written in network endian format. */
	(void) ctx;
	*p++ = (unsigned char) ((val >> 24) & 0xff);
	*p++ = (unsigned char) ((val >> 16) & 0xff);
	*p++ = (unsigned char) ((val >> 8) & 0xff);
	*p++ = (unsigned char) (val & 0xff);
	return p;
}

static unsigned char *duk__trans_dvalue_encode_i32(duk_trans_dvalue_ctx *ctx, unsigned char *p, int val) {
	return duk__trans_dvalue_encode_u32(ctx, p, (unsigned int) val & 0xffffffffUL);
}

static unsigned char *duk__trans_dvalue_encode_u16(duk_trans_dvalue_ctx *ctx, unsigned char *p, unsigned int val) {
	/* Integers are written in network endian format. */
	(void) ctx;
	*p++ = (unsigned char) ((val >> 8) & 0xff);
	*p++ = (unsigned char) (val & 0xff);
	return p;
}

static unsigned char *duk__trans_dvalue_encode_double(duk_trans_dvalue_ctx *ctx, unsigned char *p, double val) {
	/* IEEE doubles are written in network endian format. */
	volatile union {
		double d;
		unsigned char b[8];
	} u;
	u.d = val;
	duk__trans_dvalue_double_byteswap(ctx, u.b);
	memcpy((void *) p, (const void *) u.b, 8);
	p += 8;
	return p;
}

static unsigned char *duk__trans_buffer_ensure(duk_trans_buffer *dbuf, size_t space) {
	size_t avail;
	size_t used;
	size_t new_size;
	void *new_alloc;

	used = dbuf->write_offset;
	avail = dbuf->alloc_size - dbuf->write_offset;

	if (avail >= space) {
		if (avail - space > 256) {
			/* Too big, resize so that we reclaim memory if we have just
			 * received a large string/buffer value.
			 */
			goto do_realloc;
		}
	} else {
		/* Too small, resize. */
		goto do_realloc;
	}

	return dbuf->base + dbuf->write_offset;

 do_realloc:
	new_size = used + space + 256;  /* some extra to reduce resizes */
	new_alloc = realloc(dbuf->base, new_size);
	if (new_alloc) {
		dbuf->base = (unsigned char *) new_alloc;
		dbuf->alloc_size = new_size;
#if defined(DEBUG_PRINTS)
		fprintf(stderr, "%s: resized buffer %p to %ld bytes, read_offset=%ld, write_offset=%ld\n",
		        __func__, (void *) dbuf, (long) new_size, (long) dbuf->read_offset, (long) dbuf->write_offset);
		fflush(stderr);
#endif
		return dbuf->base + dbuf->write_offset;
	} else {
		return NULL;
	}
}

/* When read_offset is large enough, "rebase" buffer by deleting already
 * read data and updating offsets.
 */
static void duk__trans_buffer_rebase(duk_trans_buffer *dbuf) {
	if (dbuf->read_offset > 64) {
#if defined(DEBUG_PRINTS)
		fprintf(stderr, "%s: rebasing buffer %p, read_offset=%ld, write_offset=%ld\n",
		        __func__, (void *) dbuf, (long) dbuf->read_offset, (long) dbuf->write_offset);
		fflush(stderr);
#endif
		if (dbuf->write_offset > dbuf->read_offset) {
			memmove((void *) dbuf->base, (const void *) (dbuf->base + dbuf->read_offset), dbuf->write_offset - dbuf->read_offset);
		}
		dbuf->write_offset -= dbuf->read_offset;
		dbuf->read_offset = 0;
	}
}

duk_trans_dvalue_ctx *duk_trans_dvalue_init(void) {
	volatile union {
		double d;
		unsigned char b[8];
	} u;
	duk_trans_dvalue_ctx *ctx = NULL;

	ctx = (duk_trans_dvalue_ctx *) malloc(sizeof(duk_trans_dvalue_ctx));
	if (!ctx) { goto fail; }
	memset((void *) ctx, 0, sizeof(duk_trans_dvalue_ctx));
	ctx->received = NULL;
	ctx->cooperate = NULL;
	ctx->handshake = NULL;
	ctx->detached = NULL;
	ctx->send_buf.base = NULL;
	ctx->recv_buf.base = NULL;

	ctx->send_buf.base = malloc(256);
	if (!ctx->send_buf.base) { goto fail; }
	ctx->send_buf.alloc_size = 256;

	ctx->recv_buf.base = malloc(256);
	if (!ctx->recv_buf.base) { goto fail; }
	ctx->recv_buf.alloc_size = 256;

	/* IEEE double byte order, detect at run time (could also use
	 * preprocessor defines but that's verbose to make portable).
	 *
	 * >>> struct.unpack('>d', '1122334455667788'.decode('hex'))
	 * (3.841412024471731e-226,)
	 * >>> struct.unpack('>d', '8877665544332211'.decode('hex'))
	 * (-7.086876636573014e-268,)
	 * >>> struct.unpack('>d', '4433221188776655'.decode('hex'))
	 * (3.5294303071877444e+20,)
	 */
	u.b[0] = 0x11; u.b[1] = 0x22; u.b[2] = 0x33; u.b[3] = 0x44;
	u.b[4] = 0x55; u.b[5] = 0x66; u.b[6] = 0x77; u.b[7] = 0x88;
	if (u.d < 0.0) {
		ctx->double_byteorder = 0;  /* little endian */
	} else if (u.d < 1.0) {
		ctx->double_byteorder = 1;  /* big endian */
	} else {
		ctx->double_byteorder = 2;  /* mixed endian (arm) */
	}
#if defined(DEBUG_PRINTS)
	fprintf(stderr, "double endianness test value is %lg -> byteorder %d\n",
	        u.d, ctx->double_byteorder);
	fflush(stderr);
#endif

	return ctx;

 fail:
	if (ctx) {
		free(ctx->recv_buf.base);  /* tolerates NULL */
		free(ctx->send_buf.base);  /* tolerates NULL */
		free(ctx);
	}
	return NULL;
}

void duk_trans_dvalue_free(duk_trans_dvalue_ctx *ctx) {
	if (ctx) {
		free(ctx->send_buf.base);  /* tolerates NULL */
		free(ctx->recv_buf.base);  /* tolerates NULL */
		free(ctx);
	}
}

void duk_trans_dvalue_send(duk_trans_dvalue_ctx *ctx, duk_dvalue *dv) {
	unsigned char *p;

	/* Convert argument dvalue into Duktape debug protocol format.
	 * Literal constants are used here for the debug protocol,
	 * e.g. initial byte 0x02 is REP, see doc/debugger.rst.
	 */

#if defined(DEBUG_PRINTS)
	{
		char buf[DUK_DVALUE_TOSTRING_BUFLEN];
		duk_dvalue_to_string(dv, buf);
		fprintf(stderr, "%s: sending dvalue: %s\n", __func__, buf);
		fflush(stderr);
	}
#endif

	switch (dv->tag) {
	case DUK_DVALUE_EOM: {
		p = duk__trans_buffer_ensure(&ctx->send_buf, 1);
		if (!p) { goto alloc_error; }
		*p++ = 0x00;
		ctx->send_buf.write_offset += 1;
		break;
	}
	case DUK_DVALUE_REQ: {
		p = duk__trans_buffer_ensure(&ctx->send_buf, 1);
		if (!p) { goto alloc_error; }
		*p++ = 0x01;
		ctx->send_buf.write_offset += 1;
		break;
	}
	case DUK_DVALUE_REP: {
		p = duk__trans_buffer_ensure(&ctx->send_buf, 1);
		if (!p) { goto alloc_error; }
		*p++ = 0x02;
		ctx->send_buf.write_offset += 1;
		break;
	}
	case DUK_DVALUE_ERR: {
		p = duk__trans_buffer_ensure(&ctx->send_buf, 1);
		if (!p) { goto alloc_error; }
		*p++ = 0x03;
		ctx->send_buf.write_offset += 1;
		break;
	}
	case DUK_DVALUE_NFY: {
		p = duk__trans_buffer_ensure(&ctx->send_buf, 1);
		if (!p) { goto alloc_error; }
		*p++ = 0x04;
		ctx->send_buf.write_offset += 1;
		break;
	}
	case DUK_DVALUE_INTEGER: {
		int i = dv->i;
		if (i >= 0 && i <= 63) {
			p = duk__trans_buffer_ensure(&ctx->send_buf, 1);
			if (!p) { goto alloc_error; }
			*p++ = (unsigned char) (0x80 + i);
			ctx->send_buf.write_offset += 1;
		} else if (i >= 0 && i <= 16383L) {
			p = duk__trans_buffer_ensure(&ctx->send_buf, 2);
			if (!p) { goto alloc_error; }
			*p++ = (unsigned char) (0xc0 + (i >> 8));
			*p++ = (unsigned char) (i & 0xff);
			ctx->send_buf.write_offset += 2;
		} else if (i >= -0x80000000L && i <= 0x7fffffffL) {  /* Harmless warning on some platforms (re: range) */
			p = duk__trans_buffer_ensure(&ctx->send_buf, 5);
			if (!p) { goto alloc_error; }
			*p++ = 0x10;
			p = duk__trans_dvalue_encode_i32(ctx, p, i);
			ctx->send_buf.write_offset += 5;
		} else {
			goto dvalue_error;
		}
		break;
	}
	case DUK_DVALUE_STRING: {
		size_t i = dv->len;
		if (i <= 0x1fUL) {
			p = duk__trans_buffer_ensure(&ctx->send_buf, 1 + i);
			if (!p) { goto alloc_error; }
			*p++ = (unsigned char) (0x60 + i);
			memcpy((void *) p, (const void *) dv->buf, i);
			p += i;
			ctx->send_buf.write_offset += 1 + i;
		} else if (i <= 0xffffUL) {
			p = duk__trans_buffer_ensure(&ctx->send_buf, 3 + i);
			if (!p) { goto alloc_error; }
			*p++ = 0x12;
			p = duk__trans_dvalue_encode_u16(ctx, p, (unsigned int) i);
			memcpy((void *) p, (const void *) dv->buf, i);
			p += i;
			ctx->send_buf.write_offset += 3 + i;
		} else if (i <= 0xffffffffUL) {
			p = duk__trans_buffer_ensure(&ctx->send_buf, 5 + i);
			if (!p) { goto alloc_error; }
			*p++ = 0x11;
			p = duk__trans_dvalue_encode_u32(ctx, p, (unsigned int) i);
			memcpy((void *) p, (const void *) dv->buf, i);
			p += i;
			ctx->send_buf.write_offset += 5 + i;
		} else {
			goto dvalue_error;
		}
		break;
	}
	case DUK_DVALUE_BUFFER: {
		size_t i = dv->len;
		if (i <= 0xffffUL) {
			p = duk__trans_buffer_ensure(&ctx->send_buf, 3 + i);
			if (!p) { goto alloc_error; }
			*p++ = 0x14;
			p = duk__trans_dvalue_encode_u16(ctx, p, (unsigned int) i);
			memcpy((void *) p, (const void *) dv->buf, i);
			p += i;
			ctx->send_buf.write_offset += 3 + i;
		} else if (i <= 0xffffffffUL) {
			p = duk__trans_buffer_ensure(&ctx->send_buf, 5 + i);
			if (!p) { goto alloc_error; }
			*p++ = 0x13;
			p = duk__trans_dvalue_encode_u32(ctx, p, (unsigned int) i);
			memcpy((void *) p, (const void *) dv->buf, i);
			p += i;
			ctx->send_buf.write_offset += 5 + i;
		} else {
			goto dvalue_error;
		}
		break;
	}
	case DUK_DVALUE_UNUSED: {
		p = duk__trans_buffer_ensure(&ctx->send_buf, 1);
		if (!p) { goto alloc_error; }
		*p++ = 0x15;
		ctx->send_buf.write_offset += 1;
		break;
	}
	case DUK_DVALUE_UNDEFINED: {
		p = duk__trans_buffer_ensure(&ctx->send_buf, 1);
		if (!p) { goto alloc_error; }
		*p++ = 0x16;
		ctx->send_buf.write_offset += 1;
		break;
	}
	case DUK_DVALUE_NULL: {
		p = duk__trans_buffer_ensure(&ctx->send_buf, 1);
		if (!p) { goto alloc_error; }
		*p++ = 0x17;
		ctx->send_buf.write_offset += 1;
		break;
	}
	case DUK_DVALUE_TRUE: {
		p = duk__trans_buffer_ensure(&ctx->send_buf, 1);
		if (!p) { goto alloc_error; }
		*p++ = 0x18;
		ctx->send_buf.write_offset += 1;
		break;
	}
	case DUK_DVALUE_FALSE: {
		p = duk__trans_buffer_ensure(&ctx->send_buf, 1);
		if (!p) { goto alloc_error; }
		*p++ = 0x19;
		ctx->send_buf.write_offset += 1;
		break;
	}
	case DUK_DVALUE_NUMBER: {
		p = duk__trans_buffer_ensure(&ctx->send_buf, 9);
		if (!p) { goto alloc_error; }
		*p++ = 0x1a;
		p = duk__trans_dvalue_encode_double(ctx, p, dv->d);
		ctx->send_buf.write_offset += 9;
		break;
	}
	case DUK_DVALUE_OBJECT: {
		size_t i = dv->len;
		if (i <= 0xffUL && dv->i >= 0 && dv->i <= 0xffL) {
			p = duk__trans_buffer_ensure(&ctx->send_buf, 3 + i);
			if (!p) { goto alloc_error; }
			*p++ = 0x1b;
			*p++ = (unsigned char) dv->i;
			*p++ = (unsigned char) i;
			memcpy((void *) p, (const void *) dv->buf, i);
			ctx->send_buf.write_offset += 3 + i;
		} else {
			goto dvalue_error;
		}
		break;
	}
	case DUK_DVALUE_POINTER: {
		size_t i = dv->len;
		if (i <= 0xffUL) {
			p = duk__trans_buffer_ensure(&ctx->send_buf, 2 + i);
			if (!p) { goto alloc_error; }
			*p++ = 0x1c;
			*p++ = (unsigned char) i;
			memcpy((void *) p, (const void *) dv->buf, i);
			ctx->send_buf.write_offset += 2 + i;
		} else {
			goto dvalue_error;
		}
		break;
	}
	case DUK_DVALUE_LIGHTFUNC: {
		size_t i = dv->len;
		if (i <= 0xffUL && dv->i >= 0 && dv->i <= 0xffffL) {
			p = duk__trans_buffer_ensure(&ctx->send_buf, 4 + i);
			if (!p) { goto alloc_error; }
			*p++ = 0x1d;
			p = duk__trans_dvalue_encode_u16(ctx, p, (unsigned int) dv->i);
			*p++ = (unsigned char) i;
			memcpy((void *) p, (const void *) dv->buf, i);
			ctx->send_buf.write_offset += 4 + i;
		} else {
			goto dvalue_error;
		}
		break;
	}
	case DUK_DVALUE_HEAPPTR: {
		size_t i = dv->len;
		if (i <= 0xffUL) {
			p = duk__trans_buffer_ensure(&ctx->send_buf, 2 + i);
			if (!p) { goto alloc_error; }
			*p++ = 0x1e;
			*p++ = (unsigned char) i;
			memcpy((void *) p, (const void *) dv->buf, i);
			ctx->send_buf.write_offset += 2 + i;
		} else {
			goto dvalue_error;
		}
		break;
	}
	default: {
		goto dvalue_error;
	}
	}  /* end switch */

	return;

 dvalue_error:
#if defined(ERROR_PRINTS)
	fprintf(stderr, "%s: internal error, argument dvalue is invalid\n", __func__);
	fflush(stdout);
#endif
	return;

 alloc_error:
#if defined(ERROR_PRINTS)
	fprintf(stderr, "%s: internal error, failed to allocate space for write\n", __func__);
	fflush(stdout);
#endif
	return;
}

static void duk__trans_dvalue_send_and_free(duk_trans_dvalue_ctx *ctx, duk_dvalue *dv) {
	if (!dv) { return; }
	duk_trans_dvalue_send(ctx, dv);
	duk_dvalue_free(dv);
}

void duk_trans_dvalue_send_eom(duk_trans_dvalue_ctx *ctx) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag(DUK_DVALUE_EOM));
}

void duk_trans_dvalue_send_req(duk_trans_dvalue_ctx *ctx) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag(DUK_DVALUE_REQ));
}

void duk_trans_dvalue_send_rep(duk_trans_dvalue_ctx *ctx) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag(DUK_DVALUE_REP));
}

void duk_trans_dvalue_send_err(duk_trans_dvalue_ctx *ctx) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag(DUK_DVALUE_ERR));
}

void duk_trans_dvalue_send_nfy(duk_trans_dvalue_ctx *ctx) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag(DUK_DVALUE_NFY));
}

void duk_trans_dvalue_send_integer(duk_trans_dvalue_ctx *ctx, int val) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag_int(DUK_DVALUE_INTEGER, val));
}

void duk_trans_dvalue_send_string(duk_trans_dvalue_ctx *ctx, const char *str) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag_data(DUK_DVALUE_STRING, str, strlen(str)));
}

void duk_trans_dvalue_send_lstring(duk_trans_dvalue_ctx *ctx, const char *str, size_t len) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag_data(DUK_DVALUE_STRING, str, len));
}

void duk_trans_dvalue_send_buffer(duk_trans_dvalue_ctx *ctx, const char *buf, size_t len) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag_data(DUK_DVALUE_BUFFER, buf, len));
}

void duk_trans_dvalue_send_unused(duk_trans_dvalue_ctx *ctx) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag(DUK_DVALUE_UNUSED));
}

void duk_trans_dvalue_send_undefined(duk_trans_dvalue_ctx *ctx) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag(DUK_DVALUE_UNDEFINED));
}

void duk_trans_dvalue_send_null(duk_trans_dvalue_ctx *ctx) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag(DUK_DVALUE_NULL));
}

void duk_trans_dvalue_send_true(duk_trans_dvalue_ctx *ctx) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag(DUK_DVALUE_TRUE));
}

void duk_trans_dvalue_send_false(duk_trans_dvalue_ctx *ctx) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag(DUK_DVALUE_FALSE));
}

void duk_trans_dvalue_send_number(duk_trans_dvalue_ctx *ctx, double val) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag_double(DUK_DVALUE_NUMBER, val));
}

void duk_trans_dvalue_send_object(duk_trans_dvalue_ctx *ctx, int classnum, const char *ptr_data, size_t ptr_len) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag_int_data(DUK_DVALUE_OBJECT, classnum, ptr_data, ptr_len));
}

void duk_trans_dvalue_send_pointer(duk_trans_dvalue_ctx *ctx, const char *ptr_data, size_t ptr_len) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag_data(DUK_DVALUE_POINTER, ptr_data, ptr_len));
}

void duk_trans_dvalue_send_lightfunc(duk_trans_dvalue_ctx *ctx, int lf_flags, const char *ptr_data, size_t ptr_len) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag_int_data(DUK_DVALUE_LIGHTFUNC, lf_flags, ptr_data, ptr_len));
}

void duk_trans_dvalue_send_heapptr(duk_trans_dvalue_ctx *ctx, const char *ptr_data, size_t ptr_len) {
	duk__trans_dvalue_send_and_free(ctx, duk_dvalue_make_tag_data(DUK_DVALUE_HEAPPTR, ptr_data, ptr_len));
}

void duk_trans_dvalue_send_req_cmd(duk_trans_dvalue_ctx *ctx, int cmd) {
	duk_trans_dvalue_send_req(ctx);
	duk_trans_dvalue_send_integer(ctx, cmd);
}

static duk_dvalue *duk__trans_trial_parse_dvalue(duk_trans_dvalue_ctx *ctx) {
	unsigned char *p;
	size_t len;
	unsigned char ib;
	duk_dvalue *dv;
	size_t datalen;

	p = ctx->recv_buf.base + ctx->recv_buf.read_offset;
	len = ctx->recv_buf.write_offset - ctx->recv_buf.read_offset;

	if (len == 0) {
		return NULL;
	}
	ib = p[0];

#if defined(DEBUG_PRINTS)
	{
		size_t i;
		fprintf(stderr, "%s: parsing dvalue, window:", __func__);
		for (i = 0; i < 16; i++) {
			if (i < len) {
				fprintf(stderr, " %02x", (unsigned int) p[i]);
			} else {
				fprintf(stderr, " ??");
			}
		}
		fprintf(stderr, " (length %ld, read_offset %ld, write_offset %ld, alloc_size %ld)\n",
		        (long) len, (long) ctx->recv_buf.read_offset, (long) ctx->recv_buf.write_offset,
		        (long) ctx->recv_buf.alloc_size);
		fflush(stderr);
	}
#endif

	if (ib <= 0x1fU) {
		/* 0x00 ... 0x1f */
		switch (ib) {
		case 0x00: {
			ctx->recv_buf.read_offset += 1;
			dv = duk_dvalue_make_tag(DUK_DVALUE_EOM);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x01: {
			ctx->recv_buf.read_offset += 1;
			dv = duk_dvalue_make_tag(DUK_DVALUE_REQ);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x02: {
			ctx->recv_buf.read_offset += 1;
			dv = duk_dvalue_make_tag(DUK_DVALUE_REP);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x03: {
			ctx->recv_buf.read_offset += 1;
			dv = duk_dvalue_make_tag(DUK_DVALUE_ERR);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x04: {
			ctx->recv_buf.read_offset += 1;
			dv = duk_dvalue_make_tag(DUK_DVALUE_NFY);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x10: {
			int intval;
			if (len < 5) { goto partial; }
			intval = duk__trans_dvalue_parse_i32(ctx, p + 1);
			ctx->recv_buf.read_offset += 5;
			dv = duk_dvalue_make_tag_int(DUK_DVALUE_INTEGER, intval);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x11: {
			if (len < 5) { goto partial; }
			datalen = (size_t) duk__trans_dvalue_parse_u32(ctx, p + 1);
			if (len < 5 + datalen) { goto partial; }
			ctx->recv_buf.read_offset += 5 + datalen;
			dv = duk_dvalue_make_tag_data(DUK_DVALUE_STRING, (const char *) (p + 5), datalen);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x12: {
			if (len < 3) { goto partial; }
			datalen = (size_t) duk__trans_dvalue_parse_u16(ctx, p + 1);
			if (len < 3 + datalen) { goto partial; }
			ctx->recv_buf.read_offset += 3 + datalen;
			dv = duk_dvalue_make_tag_data(DUK_DVALUE_STRING, (const char *) (p + 3), datalen);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x13: {
			if (len < 5) { goto partial; }
			datalen = (size_t) duk__trans_dvalue_parse_u32(ctx, p + 1);
			if (len < 5 + datalen) { goto partial; }
			ctx->recv_buf.read_offset += 5 + datalen;
			dv = duk_dvalue_make_tag_data(DUK_DVALUE_BUFFER, (const char *) (p + 5), datalen);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x14: {
			if (len < 3) { goto partial; }
			datalen = (size_t) duk__trans_dvalue_parse_u16(ctx, p + 1);
			if (len < 3 + datalen) { goto partial; }
			ctx->recv_buf.read_offset += 3 + datalen;
			dv = duk_dvalue_make_tag_data(DUK_DVALUE_BUFFER, (const char *) (p + 3), datalen);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x15: {
			ctx->recv_buf.read_offset += 1;
			dv = duk_dvalue_make_tag(DUK_DVALUE_UNUSED);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x16: {
			ctx->recv_buf.read_offset += 1;
			dv = duk_dvalue_make_tag(DUK_DVALUE_UNDEFINED);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x17: {
			ctx->recv_buf.read_offset += 1;
			dv = duk_dvalue_make_tag(DUK_DVALUE_NULL);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x18: {
			ctx->recv_buf.read_offset += 1;
			dv = duk_dvalue_make_tag(DUK_DVALUE_TRUE);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x19: {
			ctx->recv_buf.read_offset += 1;
			dv = duk_dvalue_make_tag(DUK_DVALUE_FALSE);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x1a: {
			double dblval;
			if (len < 9) { goto partial; }
			dblval = duk__trans_dvalue_parse_double(ctx, p + 1);
			ctx->recv_buf.read_offset += 9;
			dv = duk_dvalue_make_tag_double(DUK_DVALUE_NUMBER, dblval);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x1b: {
			int classnum;
			if (len < 3) { goto partial; }
			datalen = (size_t) p[2];
			if (len < 3 + datalen) { goto partial; }
			classnum = (int) p[1];
			ctx->recv_buf.read_offset += 3 + datalen;
			dv = duk_dvalue_make_tag_int_data(DUK_DVALUE_OBJECT, classnum, (const char *) (p + 3), datalen);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x1c: {
			if (len < 2) { goto partial; }
			datalen = (size_t) p[1];
			if (len < 2 + datalen) { goto partial; }
			ctx->recv_buf.read_offset += 2 + datalen;
			dv = duk_dvalue_make_tag_data(DUK_DVALUE_POINTER, (const char *) (p + 2), datalen);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x1d: {
			int lf_flags;
			if (len < 4) { goto partial; }
			datalen = (size_t) p[3];
			if (len < 4 + datalen) { goto partial; }
			lf_flags = (int) duk__trans_dvalue_parse_u16(ctx, p + 1);
			ctx->recv_buf.read_offset += 4 + datalen;
			dv = duk_dvalue_make_tag_int_data(DUK_DVALUE_LIGHTFUNC, lf_flags, (const char *) (p + 4), datalen);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		case 0x1e: {
			if (len < 2) { goto partial; }
			datalen = (size_t) p[1];
			if (len < 2 + datalen) { goto partial; }
			ctx->recv_buf.read_offset += 2 + datalen;
			dv = duk_dvalue_make_tag_data(DUK_DVALUE_HEAPPTR, (const char *) (p + 2), datalen);
			if (!dv) { goto alloc_error; }
			return dv;
		}
		default: {
			goto format_error;
		}
		}  /* end switch */
	} else if (ib <= 0x5fU) {
		/* 0x20 ... 0x5f */
		goto format_error;
	} else if (ib <= 0x7fU) {
		/* 0x60 ... 0x7f */
		datalen = (size_t) (ib - 0x60U);
		if (len < 1 + datalen) { goto partial; }
		ctx->recv_buf.read_offset += 1 + datalen;
		dv = duk_dvalue_make_tag_data(DUK_DVALUE_STRING, (const char *) (p + 1), datalen);
		if (!dv) { goto alloc_error; }
		return dv;
	} else if (ib <= 0xbfU) {
		/* 0x80 ... 0xbf */
		int intval;
		intval = (int) (ib - 0x80U);
		ctx->recv_buf.read_offset += 1;
		dv = duk_dvalue_make_tag_int(DUK_DVALUE_INTEGER, intval);
		if (!dv) { goto alloc_error; }
		return dv;
	} else {
		/* 0xc0 ... 0xff */
		int intval;
		if (len < 2) { goto partial; }
		intval = (((int) (ib - 0xc0U)) << 8) + (int) p[1];
		ctx->recv_buf.read_offset += 2;
		dv = duk_dvalue_make_tag_int(DUK_DVALUE_INTEGER, intval);
		if (!dv) { goto alloc_error; }
		return dv;
	}

	/* never here */

 partial:
	return NULL;

 alloc_error:
#if defined(ERROR_PRINTS)
	fprintf(stderr, "%s: internal error, cannot allocate space for dvalue\n", __func__);
	fflush(stdout);
#endif
	return NULL;

 format_error:
#if defined(ERROR_PRINTS)
	fprintf(stderr, "%s: internal error, dvalue format error\n", __func__);
	fflush(stdout);
#endif
	return NULL;
}

static duk_dvalue *duk__trans_trial_parse_handshake(duk_trans_dvalue_ctx *ctx) {
	unsigned char *p;
	size_t len;
	duk_dvalue *dv;
	size_t i;

	p = ctx->recv_buf.base + ctx->recv_buf.read_offset;
	len = ctx->recv_buf.write_offset - ctx->recv_buf.read_offset;

	for (i = 0; i < len; i++) {
		if (p[i] == 0x0a) {
			/* Handshake line is returned as a dvalue for convenience; it's
			 * not actually a part of the dvalue phase of the protocol.
			 */
			ctx->recv_buf.read_offset += i + 1;
			dv = duk_dvalue_make_tag_data(DUK_DVALUE_STRING, (const char *) p, i);
			if (!dv) { goto alloc_error; }
			return dv;
		}
	}

	return NULL;

 alloc_error:
#if defined(ERROR_PRINTS)
	fprintf(stderr, "%s: internal error, cannot allocate space for handshake line\n", __func__);
	fflush(stdout);
#endif
	return NULL;
}

static void duk__trans_call_cooperate(duk_trans_dvalue_ctx *ctx, int block) {
	if (ctx->cooperate) {
		ctx->cooperate(ctx, block);
	}
}

static void duk__trans_call_received(duk_trans_dvalue_ctx *ctx, duk_dvalue *dv) {
	if (ctx->received) {
		ctx->received(ctx, dv);
	}
}

static void duk__trans_call_handshake(duk_trans_dvalue_ctx *ctx, const char *line) {
	if (ctx->handshake) {
		ctx->handshake(ctx, line);
	}
}

static void duk__trans_call_detached(duk_trans_dvalue_ctx *ctx) {
	if (ctx->detached) {
		ctx->detached(ctx);
	}
}

/*
 *  Duktape callbacks
 */

duk_size_t duk_trans_dvalue_read_cb(void *udata, char *buffer, duk_size_t length) {
	duk_trans_dvalue_ctx *ctx = (duk_trans_dvalue_ctx *) udata;

#if defined(DEBUG_PRINTS)
	fprintf(stderr, "%s: %p %p %ld\n", __func__, udata, (void *) buffer, (long) length);
	fflush(stderr);
#endif

	duk__trans_call_cooperate(ctx, 0);

	for (;;) {
		size_t avail, now;

		avail = (size_t) (ctx->send_buf.write_offset - ctx->send_buf.read_offset);
		if (avail == 0) {
			/* Must cooperate until user callback provides data.  From
			 * Duktape's perspective we MUST block until data is received.
			 */
			duk__trans_call_cooperate(ctx, 1);
		} else {
			now = avail;
			if (now > length) {
				now = length;
			}
			memcpy((void *) buffer, (const void *) (ctx->send_buf.base + ctx->send_buf.read_offset), now);
			duk__trans_buffer_rebase(&ctx->send_buf);
			ctx->send_buf.read_offset += now;
			return now;
		}
	}
}

duk_size_t duk_trans_dvalue_write_cb(void *udata, const char *buffer, duk_size_t length) {
	duk_trans_dvalue_ctx *ctx = (duk_trans_dvalue_ctx *) udata;
	unsigned char *p;

#if defined(DEBUG_PRINTS)
	fprintf(stderr, "%s: %p %p %ld\n", __func__, udata, (void *) buffer, (long) length);
	fflush(stderr);
#endif

	duk__trans_call_cooperate(ctx, 0);

	/* Append data. */
	duk__trans_buffer_rebase(&ctx->recv_buf);
	p = duk__trans_buffer_ensure(&ctx->recv_buf, length);
	memcpy((void *) p, (const void *) buffer, (size_t) length);
	ctx->recv_buf.write_offset += length;

	/* Trial parse handshake line or dvalue(s). */
	if (!ctx->handshake_done) {
		duk_dvalue *dv = duk__trans_trial_parse_handshake(ctx);
		if (dv) {
			/* Handshake line is available for caller for the
			 * duration of the callback, and must not be freed
			 * by the caller.
			 */
			duk__trans_call_handshake(ctx, (const char *) dv->buf);
#if defined(DEBUG_PRINTS)
			fprintf(stderr, "%s: handshake ok\n", __func__);
			fflush(stderr);
#endif
			duk_dvalue_free(dv);
			ctx->handshake_done = 1;
		}
	}
	if (ctx->handshake_done) {
		for (;;) {
			duk_dvalue *dv = duk__trans_trial_parse_dvalue(ctx);
			if (dv) {
#if defined(DEBUG_PRINTS)
				{
					char buf[DUK_DVALUE_TOSTRING_BUFLEN];
					duk_dvalue_to_string(dv, buf);
					fprintf(stderr, "%s: received dvalue: %s\n", __func__, buf);
					fflush(stderr);
				}
#endif

				duk__trans_call_received(ctx, dv);
			} else {
				break;
			}
		}
	}

	duk__trans_call_cooperate(ctx, 0);  /* just in case, if dvalues changed something */

	return length;
}

duk_size_t duk_trans_dvalue_peek_cb(void *udata) {
	duk_trans_dvalue_ctx *ctx = (duk_trans_dvalue_ctx *) udata;
	size_t avail;

#if defined(DEBUG_PRINTS)
	fprintf(stderr, "%s: %p\n", __func__, udata);
	fflush(stderr);
#endif

	duk__trans_call_cooperate(ctx, 0);
	avail = (size_t) (ctx->send_buf.write_offset - ctx->send_buf.read_offset);
	return (duk_size_t) avail;
}

void duk_trans_dvalue_read_flush_cb(void *udata) {
	duk_trans_dvalue_ctx *ctx = (duk_trans_dvalue_ctx *) udata;

#if defined(DEBUG_PRINTS)
	fprintf(stderr, "%s: %p\n", __func__, udata);
	fflush(stderr);
#endif

	duk__trans_call_cooperate(ctx, 0);
}

void duk_trans_dvalue_write_flush_cb(void *udata) {
	duk_trans_dvalue_ctx *ctx = (duk_trans_dvalue_ctx *) udata;

#if defined(DEBUG_PRINTS)
	fprintf(stderr, "%s: %p\n", __func__, udata);
	fflush(stderr);
#endif

	duk__trans_call_cooperate(ctx, 0);
}

void duk_trans_dvalue_detached_cb(duk_context *duk_ctx, void *udata) {
	duk_trans_dvalue_ctx *ctx = (duk_trans_dvalue_ctx *) udata;

	(void) duk_ctx;

#if defined(DEBUG_PRINTS)
	fprintf(stderr, "%s: %p\n", __func__, udata);
	fflush(stderr);
#endif

	duk__trans_call_detached(ctx);
}
