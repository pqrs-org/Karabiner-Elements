/*
 *  Selftest code
 */

#if !defined(DUK_SELFTEST_H_INCLUDED)
#define DUK_SELFTEST_H_INCLUDED

#if defined(DUK_USE_SELF_TESTS)
DUK_INTERNAL_DECL duk_uint_t duk_selftest_run_tests(duk_alloc_function alloc_func,
                                                    duk_realloc_function realloc_func,
                                                    duk_free_function free_func,
                                                    void *udata);
#endif

#endif /* DUK_SELFTEST_H_INCLUDED */
