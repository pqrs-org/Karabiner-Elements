#if !defined(DUK_CODEPAGE_CONV_H_INCLUDED)
#define DUK_CODEPAGE_CONV_H_INCLUDED

#include "duktape.h"

void duk_decode_string_codepage(duk_context *ctx, const char *str, size_t len, unsigned int *codepage);

#endif  /* DUK_CODEPAGE_CONV_H_INCLUDED */
