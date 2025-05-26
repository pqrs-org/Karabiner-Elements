/*
 *  Bitstream encoder.
 */

#include "duk_internal.h"

DUK_INTERNAL void duk_be_encode(duk_bitencoder_ctx *ctx, duk_uint32_t data, duk_small_int_t bits) {
	duk_uint8_t tmp;

	DUK_ASSERT(ctx != NULL);
	DUK_ASSERT(ctx->currbits < 8);

	/* This limitation would be fixable but adds unnecessary complexity. */
	DUK_ASSERT(bits >= 1 && bits <= 24);

	ctx->currval = (ctx->currval << bits) | data;
	ctx->currbits += bits;

	while (ctx->currbits >= 8) {
		if (ctx->offset < ctx->length) {
			tmp = (duk_uint8_t) ((ctx->currval >> (ctx->currbits - 8)) & 0xff);
			ctx->data[ctx->offset++] = tmp;
		} else {
			/* If buffer has been exhausted, truncate bitstream */
			ctx->truncated = 1;
		}

		ctx->currbits -= 8;
	}
}

DUK_INTERNAL void duk_be_finish(duk_bitencoder_ctx *ctx) {
	duk_small_int_t npad;

	DUK_ASSERT(ctx != NULL);
	DUK_ASSERT(ctx->currbits < 8);

	npad = (duk_small_int_t) (8 - ctx->currbits);
	if (npad > 0) {
		duk_be_encode(ctx, 0, npad);
	}
	DUK_ASSERT(ctx->currbits == 0);
}
