#if !defined(DUK_REPLACEMENTS_H_INCLUDED)
#define DUK_REPLACEMENTS_H_INCLUDED

#if !defined(DUK_SINGLE_FILE)
#if defined(DUK_USE_COMPUTED_INFINITY)
DUK_INTERNAL_DECL double duk_computed_infinity;
#endif
#if defined(DUK_USE_COMPUTED_NAN)
DUK_INTERNAL_DECL double duk_computed_nan;
#endif
#endif /* !DUK_SINGLE_FILE */

#if defined(DUK_USE_REPL_FPCLASSIFY)
DUK_INTERNAL_DECL int duk_repl_fpclassify(double x);
#endif
#if defined(DUK_USE_REPL_SIGNBIT)
DUK_INTERNAL_DECL int duk_repl_signbit(double x);
#endif
#if defined(DUK_USE_REPL_ISFINITE)
DUK_INTERNAL_DECL int duk_repl_isfinite(double x);
#endif
#if defined(DUK_USE_REPL_ISNAN)
DUK_INTERNAL_DECL int duk_repl_isnan(double x);
#endif
#if defined(DUK_USE_REPL_ISINF)
DUK_INTERNAL_DECL int duk_repl_isinf(double x);
#endif

#endif /* DUK_REPLACEMENTS_H_INCLUDED */
