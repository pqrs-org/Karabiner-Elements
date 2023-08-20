/*
 *  Union to access IEEE float memory representation.
 */

#if !defined(DUK_FLTUNION_H_INCLUDED)
#define DUK_FLTUNION_H_INCLUDED

#include "duk_internal.h"

union duk_float_union {
	float f;
	duk_uint32_t ui[1];
	duk_uint16_t us[2];
	duk_uint8_t uc[4];
};

typedef union duk_float_union duk_float_union;

#if defined(DUK_USE_DOUBLE_LE) || defined(DUK_USE_DOUBLE_ME)
#define DUK_FLT_IDX_UI0 0
#define DUK_FLT_IDX_US0 1
#define DUK_FLT_IDX_US1 0
#define DUK_FLT_IDX_UC0 3
#define DUK_FLT_IDX_UC1 2
#define DUK_FLT_IDX_UC2 1
#define DUK_FLT_IDX_UC3 0
#elif defined(DUK_USE_DOUBLE_BE)
#define DUK_FLT_IDX_UI0 0
#define DUK_FLT_IDX_US0 0
#define DUK_FLT_IDX_US1 1
#define DUK_FLT_IDX_UC0 0
#define DUK_FLT_IDX_UC1 1
#define DUK_FLT_IDX_UC2 2
#define DUK_FLT_IDX_UC3 3
#else
#error internal error
#endif

#endif /* DUK_FLTUNION_H_INCLUDED */
