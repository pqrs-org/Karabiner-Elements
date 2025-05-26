#if !defined(DUK_ALLOC_LOGGING_H_INCLUDED)
#define DUK_ALLOC_LOGGING_H_INCLUDED

#include "duktape.h"

void *duk_alloc_logging(void *udata, duk_size_t size);
void *duk_realloc_logging(void *udata, void *ptr, duk_size_t size);
void duk_free_logging(void *udata, void *ptr);

#endif  /* DUK_ALLOC_LOGGING_H_INCLUDED */
