#if !defined(DUK_ALLOC_HYBRID_H_INCLUDED)
#define DUK_ALLOC_HYBRID_H_INCLUDED

#include "duktape.h"

void *duk_alloc_hybrid_init(void);
void *duk_alloc_hybrid(void *udata, duk_size_t size);
void *duk_realloc_hybrid(void *udata, void *ptr, duk_size_t size);
void duk_free_hybrid(void *udata, void *ptr);

#endif  /* DUK_ALLOC_HYBRID_H_INCLUDED */
