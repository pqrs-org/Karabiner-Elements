/*
 *  Unicode support tables automatically generated during build.
 */

#include "duk_internal.h"

/*
 *  Unicode tables containing ranges of Unicode characters in a
 *  packed format.  These tables are used to match non-ASCII
 *  characters of complex productions by resorting to a linear
 *  range-by-range comparison.  This is very slow, but is expected
 *  to be very rare in practical ECMAScript source code, and thus
 *  compactness is most important.
 *
 *  The tables are matched using uni_range_match() and the format
 *  is described in tools/extract_chars.py.
 */

#if defined(DUK_USE_SOURCE_NONBMP)
/* IdentifierStart production with ASCII excluded */
/* duk_unicode_ids_noa[] */
#include "duk_unicode_ids_noa.c"
#else
/* IdentifierStart production with ASCII and non-BMP excluded */
/* duk_unicode_ids_noabmp[] */
#include "duk_unicode_ids_noabmp.c"
#endif

#if defined(DUK_USE_SOURCE_NONBMP)
/* IdentifierStart production with Letter and ASCII excluded */
/* duk_unicode_ids_m_let_noa[] */
#include "duk_unicode_ids_m_let_noa.c"
#else
/* IdentifierStart production with Letter, ASCII, and non-BMP excluded */
/* duk_unicode_ids_m_let_noabmp[] */
#include "duk_unicode_ids_m_let_noabmp.c"
#endif

#if defined(DUK_USE_SOURCE_NONBMP)
/* IdentifierPart production with IdentifierStart and ASCII excluded */
/* duk_unicode_idp_m_ids_noa[] */
#include "duk_unicode_idp_m_ids_noa.c"
#else
/* IdentifierPart production with IdentifierStart, ASCII, and non-BMP excluded */
/* duk_unicode_idp_m_ids_noabmp[] */
#include "duk_unicode_idp_m_ids_noabmp.c"
#endif

/*
 *  Case conversion tables generated using tools/extract_caseconv.py.
 */

/* duk_unicode_caseconv_uc[] */
/* duk_unicode_caseconv_lc[] */

#include "duk_unicode_caseconv.c"

#if defined(DUK_USE_REGEXP_CANON_WORKAROUND)
#include "duk_unicode_re_canon_lookup.c"
#endif

#if defined(DUK_USE_REGEXP_CANON_BITMAP)
#include "duk_unicode_re_canon_bitmap.c"
#endif
