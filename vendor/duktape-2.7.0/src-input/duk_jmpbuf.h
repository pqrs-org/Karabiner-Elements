/*
 *  Wrapper for jmp_buf.
 *
 *  This is used because jmp_buf is an array type for backward compatibility.
 *  Wrapping jmp_buf in a struct makes pointer references, sizeof, etc,
 *  behave more intuitively.
 *
 *  http://en.wikipedia.org/wiki/Setjmp.h#Member_types
 */

#if !defined(DUK_JMPBUF_H_INCLUDED)
#define DUK_JMPBUF_H_INCLUDED

#if defined(DUK_USE_CPP_EXCEPTIONS)
struct duk_jmpbuf {
	duk_small_int_t dummy; /* unused */
};
#else
struct duk_jmpbuf {
	DUK_JMPBUF_TYPE jb;
};
#endif

#endif /* DUK_JMPBUF_H_INCLUDED */
