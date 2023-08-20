/*
 *  Unicode helpers
 */

#if !defined(DUK_UNICODE_H_INCLUDED)
#define DUK_UNICODE_H_INCLUDED

/*
 *  UTF-8 / XUTF-8 / CESU-8 constants
 */

#define DUK_UNICODE_MAX_XUTF8_LENGTH     7 /* up to 36 bit codepoints */
#define DUK_UNICODE_MAX_XUTF8_BMP_LENGTH 3 /* all codepoints up to U+FFFF */
#define DUK_UNICODE_MAX_CESU8_LENGTH     6 /* all codepoints up to U+10FFFF */
#define DUK_UNICODE_MAX_CESU8_BMP_LENGTH 3 /* all codepoints up to U+FFFF */

/*
 *  Useful Unicode codepoints
 *
 *  Integer constants must be signed to avoid unexpected coercions
 *  in comparisons.
 */

#define DUK_UNICODE_CP_ZWNJ 0x200cL /* zero-width non-joiner */
#define DUK_UNICODE_CP_ZWJ  0x200dL /* zero-width joiner */
#define DUK_UNICODE_CP_REPLACEMENT_CHARACTER \
	0xfffdL /* http://en.wikipedia.org/wiki/Replacement_character#Replacement_character \
	         */

/*
 *  ASCII character constants
 *
 *  C character literals like 'x' have a platform specific value and do
 *  not match ASCII (UTF-8) values on e.g. EBCDIC platforms.  So, use
 *  these (admittedly awkward) constants instead.  These constants must
 *  also have signed values to avoid unexpected coercions in comparisons.
 *
 *  http://en.wikipedia.org/wiki/ASCII
 */

#define DUK_ASC_NUL         0x00
#define DUK_ASC_SOH         0x01
#define DUK_ASC_STX         0x02
#define DUK_ASC_ETX         0x03
#define DUK_ASC_EOT         0x04
#define DUK_ASC_ENQ         0x05
#define DUK_ASC_ACK         0x06
#define DUK_ASC_BEL         0x07
#define DUK_ASC_BS          0x08
#define DUK_ASC_HT          0x09
#define DUK_ASC_LF          0x0a
#define DUK_ASC_VT          0x0b
#define DUK_ASC_FF          0x0c
#define DUK_ASC_CR          0x0d
#define DUK_ASC_SO          0x0e
#define DUK_ASC_SI          0x0f
#define DUK_ASC_DLE         0x10
#define DUK_ASC_DC1         0x11
#define DUK_ASC_DC2         0x12
#define DUK_ASC_DC3         0x13
#define DUK_ASC_DC4         0x14
#define DUK_ASC_NAK         0x15
#define DUK_ASC_SYN         0x16
#define DUK_ASC_ETB         0x17
#define DUK_ASC_CAN         0x18
#define DUK_ASC_EM          0x19
#define DUK_ASC_SUB         0x1a
#define DUK_ASC_ESC         0x1b
#define DUK_ASC_FS          0x1c
#define DUK_ASC_GS          0x1d
#define DUK_ASC_RS          0x1e
#define DUK_ASC_US          0x1f
#define DUK_ASC_SPACE       0x20
#define DUK_ASC_EXCLAMATION 0x21
#define DUK_ASC_DOUBLEQUOTE 0x22
#define DUK_ASC_HASH        0x23
#define DUK_ASC_DOLLAR      0x24
#define DUK_ASC_PERCENT     0x25
#define DUK_ASC_AMP         0x26
#define DUK_ASC_SINGLEQUOTE 0x27
#define DUK_ASC_LPAREN      0x28
#define DUK_ASC_RPAREN      0x29
#define DUK_ASC_STAR        0x2a
#define DUK_ASC_PLUS        0x2b
#define DUK_ASC_COMMA       0x2c
#define DUK_ASC_MINUS       0x2d
#define DUK_ASC_PERIOD      0x2e
#define DUK_ASC_SLASH       0x2f
#define DUK_ASC_0           0x30
#define DUK_ASC_1           0x31
#define DUK_ASC_2           0x32
#define DUK_ASC_3           0x33
#define DUK_ASC_4           0x34
#define DUK_ASC_5           0x35
#define DUK_ASC_6           0x36
#define DUK_ASC_7           0x37
#define DUK_ASC_8           0x38
#define DUK_ASC_9           0x39
#define DUK_ASC_COLON       0x3a
#define DUK_ASC_SEMICOLON   0x3b
#define DUK_ASC_LANGLE      0x3c
#define DUK_ASC_EQUALS      0x3d
#define DUK_ASC_RANGLE      0x3e
#define DUK_ASC_QUESTION    0x3f
#define DUK_ASC_ATSIGN      0x40
#define DUK_ASC_UC_A        0x41
#define DUK_ASC_UC_B        0x42
#define DUK_ASC_UC_C        0x43
#define DUK_ASC_UC_D        0x44
#define DUK_ASC_UC_E        0x45
#define DUK_ASC_UC_F        0x46
#define DUK_ASC_UC_G        0x47
#define DUK_ASC_UC_H        0x48
#define DUK_ASC_UC_I        0x49
#define DUK_ASC_UC_J        0x4a
#define DUK_ASC_UC_K        0x4b
#define DUK_ASC_UC_L        0x4c
#define DUK_ASC_UC_M        0x4d
#define DUK_ASC_UC_N        0x4e
#define DUK_ASC_UC_O        0x4f
#define DUK_ASC_UC_P        0x50
#define DUK_ASC_UC_Q        0x51
#define DUK_ASC_UC_R        0x52
#define DUK_ASC_UC_S        0x53
#define DUK_ASC_UC_T        0x54
#define DUK_ASC_UC_U        0x55
#define DUK_ASC_UC_V        0x56
#define DUK_ASC_UC_W        0x57
#define DUK_ASC_UC_X        0x58
#define DUK_ASC_UC_Y        0x59
#define DUK_ASC_UC_Z        0x5a
#define DUK_ASC_LBRACKET    0x5b
#define DUK_ASC_BACKSLASH   0x5c
#define DUK_ASC_RBRACKET    0x5d
#define DUK_ASC_CARET       0x5e
#define DUK_ASC_UNDERSCORE  0x5f
#define DUK_ASC_GRAVE       0x60
#define DUK_ASC_LC_A        0x61
#define DUK_ASC_LC_B        0x62
#define DUK_ASC_LC_C        0x63
#define DUK_ASC_LC_D        0x64
#define DUK_ASC_LC_E        0x65
#define DUK_ASC_LC_F        0x66
#define DUK_ASC_LC_G        0x67
#define DUK_ASC_LC_H        0x68
#define DUK_ASC_LC_I        0x69
#define DUK_ASC_LC_J        0x6a
#define DUK_ASC_LC_K        0x6b
#define DUK_ASC_LC_L        0x6c
#define DUK_ASC_LC_M        0x6d
#define DUK_ASC_LC_N        0x6e
#define DUK_ASC_LC_O        0x6f
#define DUK_ASC_LC_P        0x70
#define DUK_ASC_LC_Q        0x71
#define DUK_ASC_LC_R        0x72
#define DUK_ASC_LC_S        0x73
#define DUK_ASC_LC_T        0x74
#define DUK_ASC_LC_U        0x75
#define DUK_ASC_LC_V        0x76
#define DUK_ASC_LC_W        0x77
#define DUK_ASC_LC_X        0x78
#define DUK_ASC_LC_Y        0x79
#define DUK_ASC_LC_Z        0x7a
#define DUK_ASC_LCURLY      0x7b
#define DUK_ASC_PIPE        0x7c
#define DUK_ASC_RCURLY      0x7d
#define DUK_ASC_TILDE       0x7e
#define DUK_ASC_DEL         0x7f

/*
 *  Miscellaneous
 */

/* Uppercase A is 0x41, lowercase a is 0x61; OR 0x20 to convert uppercase
 * to lowercase.
 */
#define DUK_LOWERCASE_CHAR_ASCII(x) ((x) | 0x20)

/*
 *  Unicode tables
 */

#if defined(DUK_USE_SOURCE_NONBMP)
/*
 *  Automatically generated by extract_chars.py, do not edit!
 */

extern const duk_uint8_t duk_unicode_ids_noa[1116];
#else
/*
 *  Automatically generated by extract_chars.py, do not edit!
 */

extern const duk_uint8_t duk_unicode_ids_noabmp[625];
#endif

#if defined(DUK_USE_SOURCE_NONBMP)
/*
 *  Automatically generated by extract_chars.py, do not edit!
 */

extern const duk_uint8_t duk_unicode_ids_m_let_noa[42];
#else
/*
 *  Automatically generated by extract_chars.py, do not edit!
 */

extern const duk_uint8_t duk_unicode_ids_m_let_noabmp[24];
#endif

#if defined(DUK_USE_SOURCE_NONBMP)
/*
 *  Automatically generated by extract_chars.py, do not edit!
 */

extern const duk_uint8_t duk_unicode_idp_m_ids_noa[576];
#else
/*
 *  Automatically generated by extract_chars.py, do not edit!
 */

extern const duk_uint8_t duk_unicode_idp_m_ids_noabmp[358];
#endif

/*
 *  Automatically generated by extract_caseconv.py, do not edit!
 */

extern const duk_uint8_t duk_unicode_caseconv_uc[1411];
extern const duk_uint8_t duk_unicode_caseconv_lc[706];

#if defined(DUK_USE_REGEXP_CANON_WORKAROUND)
/*
 *  Automatically generated by extract_caseconv.py, do not edit!
 */

extern const duk_uint16_t duk_unicode_re_canon_lookup[65536];
#endif

#if defined(DUK_USE_REGEXP_CANON_BITMAP)
/*
 *  Automatically generated by extract_caseconv.py, do not edit!
 */

#define DUK_CANON_BITMAP_BLKSIZE                                      32
#define DUK_CANON_BITMAP_BLKSHIFT                                     5
#define DUK_CANON_BITMAP_BLKMASK                                      31
extern const duk_uint8_t duk_unicode_re_canon_bitmap[256];
#endif

/*
 *  Extern
 */

/* duk_unicode_support.c */
#if !defined(DUK_SINGLE_FILE)
DUK_INTERNAL_DECL const duk_uint8_t duk_unicode_xutf8_markers[7];
DUK_INTERNAL_DECL const duk_uint16_t duk_unicode_re_ranges_digit[2];
DUK_INTERNAL_DECL const duk_uint16_t duk_unicode_re_ranges_white[22];
DUK_INTERNAL_DECL const duk_uint16_t duk_unicode_re_ranges_wordchar[8];
DUK_INTERNAL_DECL const duk_uint16_t duk_unicode_re_ranges_not_digit[4];
DUK_INTERNAL_DECL const duk_uint16_t duk_unicode_re_ranges_not_white[24];
DUK_INTERNAL_DECL const duk_uint16_t duk_unicode_re_ranges_not_wordchar[10];
DUK_INTERNAL_DECL const duk_int8_t duk_is_idchar_tab[128];
#endif /* !DUK_SINGLE_FILE */

/*
 *  Prototypes
 */

DUK_INTERNAL_DECL duk_small_int_t duk_unicode_get_xutf8_length(duk_ucodepoint_t cp);
#if defined(DUK_USE_ASSERTIONS)
DUK_INTERNAL_DECL duk_small_int_t duk_unicode_get_cesu8_length(duk_ucodepoint_t cp);
#endif
DUK_INTERNAL_DECL duk_small_int_t duk_unicode_encode_xutf8(duk_ucodepoint_t cp, duk_uint8_t *out);
DUK_INTERNAL_DECL duk_small_int_t duk_unicode_encode_cesu8(duk_ucodepoint_t cp, duk_uint8_t *out);
DUK_INTERNAL_DECL duk_small_int_t duk_unicode_decode_xutf8(duk_hthread *thr,
                                                           const duk_uint8_t **ptr,
                                                           const duk_uint8_t *ptr_start,
                                                           const duk_uint8_t *ptr_end,
                                                           duk_ucodepoint_t *out_cp);
DUK_INTERNAL_DECL duk_ucodepoint_t duk_unicode_decode_xutf8_checked(duk_hthread *thr,
                                                                    const duk_uint8_t **ptr,
                                                                    const duk_uint8_t *ptr_start,
                                                                    const duk_uint8_t *ptr_end);
DUK_INTERNAL_DECL duk_size_t duk_unicode_unvalidated_utf8_length(const duk_uint8_t *data, duk_size_t blen);
DUK_INTERNAL_DECL duk_bool_t duk_unicode_is_utf8_compatible(const duk_uint8_t *buf, duk_size_t len);
DUK_INTERNAL_DECL duk_small_int_t duk_unicode_is_whitespace(duk_codepoint_t cp);
DUK_INTERNAL_DECL duk_small_int_t duk_unicode_is_line_terminator(duk_codepoint_t cp);
DUK_INTERNAL_DECL duk_small_int_t duk_unicode_is_identifier_start(duk_codepoint_t cp);
DUK_INTERNAL_DECL duk_small_int_t duk_unicode_is_identifier_part(duk_codepoint_t cp);
DUK_INTERNAL_DECL duk_small_int_t duk_unicode_is_letter(duk_codepoint_t cp);
DUK_INTERNAL_DECL void duk_unicode_case_convert_string(duk_hthread *thr, duk_bool_t uppercase);
#if defined(DUK_USE_REGEXP_SUPPORT)
DUK_INTERNAL_DECL duk_codepoint_t duk_unicode_re_canonicalize_char(duk_hthread *thr, duk_codepoint_t cp);
DUK_INTERNAL_DECL duk_small_int_t duk_unicode_re_is_wordchar(duk_codepoint_t cp);
#endif

#endif /* DUK_UNICODE_H_INCLUDED */
