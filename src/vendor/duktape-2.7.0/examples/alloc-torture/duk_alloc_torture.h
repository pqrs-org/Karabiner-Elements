#if !defined(DUK_ALLOC_TORTURE_H_INCLUDED)
#define DUK_ALLOC_TORTURE_H_INCLUDED

#include "duktape.h"

void *duk_alloc_torture(void *udata, duk_size_t size);
void *duk_realloc_torture(void *udata, void *ptr, duk_size_t size);
void duk_free_torture(void *udata, void *ptr);

#endif  /* DUK_ALLOC_TORTURE_H_INCLUDED */
