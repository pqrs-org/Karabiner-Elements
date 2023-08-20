/*
 *  ECMAScript bytecode
 */

#if !defined(DUK_JS_BYTECODE_H_INCLUDED)
#define DUK_JS_BYTECODE_H_INCLUDED

/*
 *  Bytecode instruction layout
 *  ===========================
 *
 *  Instructions are unsigned 32-bit integers divided as follows:
 *
 *  !3!3!2!2!2!2!2!2!2!2!2!2!1!1!1!1!1!1!1!1!1!1! ! ! ! ! ! ! ! ! ! !
 *  !1!0!9!8!7!6!5!4!3!2!1!0!9!8!7!6!5!4!3!2!1!0!9!8!7!6!5!4!3!2!1!0!
 *  +-----------------------------------------------+---------------+
 *  !       C       !       B       !       A       !       OP      !
 *  +-----------------------------------------------+---------------+
 *
 *  OP (8 bits):  opcode (DUK_OP_*), access should be fastest
 *                consecutive opcodes allocated when opcode needs flags
 *   A (8 bits):  typically a target register number
 *   B (8 bits):  typically first source register/constant number
 *   C (8 bits):  typically second source register/constant number
 *
 *  Some instructions combine BC or ABC together for larger parameter values.
 *  Signed integers (e.g. jump offsets) are encoded as unsigned, with an
 *  opcode specific bias.
 *
 *  Some opcodes have flags which are handled by allocating consecutive
 *  opcodes to make space for 1-N flags.  Flags can also be e.g. in the 'A'
 *  field when there's room for the specific opcode.
 *
 *  For example, if three flags were needed, they could be allocated from
 *  the opcode field as follows:
 *
 *  !3!3!2!2!2!2!2!2!2!2!2!2!1!1!1!1!1!1!1!1!1!1! ! ! ! ! ! ! ! ! ! !
 *  !1!0!9!8!7!6!5!4!3!2!1!0!9!8!7!6!5!4!3!2!1!0!9!8!7!6!5!4!3!2!1!0!
 *  +-----------------------------------------------+---------------+
 *  !       C       !       B       !       A       !    OP   !Z!Y!X!
 *  +-----------------------------------------------+---------------+
 *
 *  Some opcodes accept a reg/const argument which is handled by allocating
 *  flags in the OP field, see DUK_BC_ISREG() and DUK_BC_ISCONST().  The
 *  following convention is shared by most opcodes, so that the compiler
 *  can handle reg/const flagging without opcode specific code paths:
 *
 *  !3!3!2!2!2!2!2!2!2!2!2!2!1!1!1!1!1!1!1!1!1!1! ! ! ! ! ! ! ! ! ! !
 *  !1!0!9!8!7!6!5!4!3!2!1!0!9!8!7!6!5!4!3!2!1!0!9!8!7!6!5!4!3!2!1!0!
 *  +-----------------------------------------------+---------------+
 *  !       C       !       B       !       A       !     OP    !Y!X!
 *  +-----------------------------------------------+---------------+
 *
 *    X  1=B is const, 0=B is reg
 *    Y  1=C is const, 0=C is reg
 *
 *    In effect OP, OP + 1, OP + 2, and OP + 3 are allocated from the
 *    8-bit opcode space for a single logical opcode.  The base opcode
 *    number should be divisible by 4.  If the opcode is called 'FOO'
 *    the following opcode constants would be defined:
 *
 *      DUK_OP_FOO     100       // base opcode number
 *      DUK_OP_FOO_RR  100       // FOO, B=reg, C=reg
 *      DUK_OP_FOO_CR  101       // FOO, B=const, C=reg
 *      DUK_OP_FOO_RC  102       // FOO, B=reg, C=const
 *      DUK_OP_FOO_CC  103       // FOO, B=const, C=const
 *
 *  If only B or C is a reg/const, the unused opcode combinations can be
 *  used for other opcodes (which take no reg/const argument).  However,
 *  such opcode values are initially reserved, at least while opcode space
 *  is available.  For example, if 'BAR' uses B for a register field and
 *  C is a reg/const:
 *
 *      DUK_OP_BAR            116    // base opcode number
 *      DUK_OP_BAR_RR         116    // BAR, B=reg, C=reg
 *      DUK_OP_BAR_CR_UNUSED  117    // unused, could be repurposed
 *      DUK_OP_BAR_RC         118    // BAR, B=reg, C=const
 *      DUK_OP_BAR_CC_UNUSED  119    // unused, could be repurposed
 *
 *  Macro naming is a bit misleading, e.g. "ABC" in macro name but the
 *  field layout is concretely "CBA" in the register.
 */

typedef duk_uint32_t duk_instr_t;

#define DUK_BC_SHIFT_OP  0
#define DUK_BC_SHIFT_A   8
#define DUK_BC_SHIFT_B   16
#define DUK_BC_SHIFT_C   24
#define DUK_BC_SHIFT_BC  DUK_BC_SHIFT_B
#define DUK_BC_SHIFT_ABC DUK_BC_SHIFT_A

#define DUK_BC_UNSHIFTED_MASK_OP  0xffUL
#define DUK_BC_UNSHIFTED_MASK_A   0xffUL
#define DUK_BC_UNSHIFTED_MASK_B   0xffUL
#define DUK_BC_UNSHIFTED_MASK_C   0xffUL
#define DUK_BC_UNSHIFTED_MASK_BC  0xffffUL
#define DUK_BC_UNSHIFTED_MASK_ABC 0xffffffUL

#define DUK_BC_SHIFTED_MASK_OP  (DUK_BC_UNSHIFTED_MASK_OP << DUK_BC_SHIFT_OP)
#define DUK_BC_SHIFTED_MASK_A   (DUK_BC_UNSHIFTED_MASK_A << DUK_BC_SHIFT_A)
#define DUK_BC_SHIFTED_MASK_B   (DUK_BC_UNSHIFTED_MASK_B << DUK_BC_SHIFT_B)
#define DUK_BC_SHIFTED_MASK_C   (DUK_BC_UNSHIFTED_MASK_C << DUK_BC_SHIFT_C)
#define DUK_BC_SHIFTED_MASK_BC  (DUK_BC_UNSHIFTED_MASK_BC << DUK_BC_SHIFT_BC)
#define DUK_BC_SHIFTED_MASK_ABC (DUK_BC_UNSHIFTED_MASK_ABC << DUK_BC_SHIFT_ABC)

#define DUK_DEC_OP(x)  ((x) &0xffUL)
#define DUK_DEC_A(x)   (((x) >> 8) & 0xffUL)
#define DUK_DEC_B(x)   (((x) >> 16) & 0xffUL)
#define DUK_DEC_C(x)   (((x) >> 24) & 0xffUL)
#define DUK_DEC_BC(x)  (((x) >> 16) & 0xffffUL)
#define DUK_DEC_ABC(x) (((x) >> 8) & 0xffffffUL)

#define DUK_ENC_OP(op)          ((duk_instr_t) (op))
#define DUK_ENC_OP_ABC(op, abc) ((duk_instr_t) ((((duk_instr_t) (abc)) << 8) | ((duk_instr_t) (op))))
#define DUK_ENC_OP_A_BC(op, a, bc) \
	((duk_instr_t) ((((duk_instr_t) (bc)) << 16) | (((duk_instr_t) (a)) << 8) | ((duk_instr_t) (op))))
#define DUK_ENC_OP_A_B_C(op, a, b, c) \
	((duk_instr_t) ((((duk_instr_t) (c)) << 24) | (((duk_instr_t) (b)) << 16) | (((duk_instr_t) (a)) << 8) | \
	                ((duk_instr_t) (op))))
#define DUK_ENC_OP_A_B(op, a, b) DUK_ENC_OP_A_B_C((op), (a), (b), 0)
#define DUK_ENC_OP_A(op, a)      DUK_ENC_OP_A_B_C((op), (a), 0, 0)
#define DUK_ENC_OP_BC(op, bc)    DUK_ENC_OP_A_BC((op), 0, (bc))

/* Get opcode base value with B/C reg/const flags cleared. */
#define DUK_BC_NOREGCONST_OP(op) ((op) &0xfc)

/* Constants should be signed so that signed arithmetic involving them
 * won't cause values to be coerced accidentally to unsigned.
 */
#define DUK_BC_OP_MIN  0
#define DUK_BC_OP_MAX  0xffL
#define DUK_BC_A_MIN   0
#define DUK_BC_A_MAX   0xffL
#define DUK_BC_B_MIN   0
#define DUK_BC_B_MAX   0xffL
#define DUK_BC_C_MIN   0
#define DUK_BC_C_MAX   0xffL
#define DUK_BC_BC_MIN  0
#define DUK_BC_BC_MAX  0xffffL
#define DUK_BC_ABC_MIN 0
#define DUK_BC_ABC_MAX 0xffffffL

/* Masks for B/C reg/const indicator in opcode field. */
#define DUK_BC_REGCONST_B (0x01UL)
#define DUK_BC_REGCONST_C (0x02UL)

/* Misc. masks for opcode field. */
#define DUK_BC_INCDECP_FLAG_DEC  (0x04UL)
#define DUK_BC_INCDECP_FLAG_POST (0x08UL)

/* Opcodes. */
#define DUK_OP_LDREG             0
#define DUK_OP_STREG             1
#define DUK_OP_JUMP              2
#define DUK_OP_LDCONST           3
#define DUK_OP_LDINT             4
#define DUK_OP_LDINTX            5
#define DUK_OP_LDTHIS            6
#define DUK_OP_LDUNDEF           7
#define DUK_OP_LDNULL            8
#define DUK_OP_LDTRUE            9
#define DUK_OP_LDFALSE           10
#define DUK_OP_GETVAR            11
#define DUK_OP_BNOT              12
#define DUK_OP_LNOT              13
#define DUK_OP_UNM               14
#define DUK_OP_UNP               15
#define DUK_OP_EQ                16
#define DUK_OP_EQ_RR             16
#define DUK_OP_EQ_CR             17
#define DUK_OP_EQ_RC             18
#define DUK_OP_EQ_CC             19
#define DUK_OP_NEQ               20
#define DUK_OP_NEQ_RR            20
#define DUK_OP_NEQ_CR            21
#define DUK_OP_NEQ_RC            22
#define DUK_OP_NEQ_CC            23
#define DUK_OP_SEQ               24
#define DUK_OP_SEQ_RR            24
#define DUK_OP_SEQ_CR            25
#define DUK_OP_SEQ_RC            26
#define DUK_OP_SEQ_CC            27
#define DUK_OP_SNEQ              28
#define DUK_OP_SNEQ_RR           28
#define DUK_OP_SNEQ_CR           29
#define DUK_OP_SNEQ_RC           30
#define DUK_OP_SNEQ_CC           31
#define DUK_OP_GT                32
#define DUK_OP_GT_RR             32
#define DUK_OP_GT_CR             33
#define DUK_OP_GT_RC             34
#define DUK_OP_GT_CC             35
#define DUK_OP_GE                36
#define DUK_OP_GE_RR             36
#define DUK_OP_GE_CR             37
#define DUK_OP_GE_RC             38
#define DUK_OP_GE_CC             39
#define DUK_OP_LT                40
#define DUK_OP_LT_RR             40
#define DUK_OP_LT_CR             41
#define DUK_OP_LT_RC             42
#define DUK_OP_LT_CC             43
#define DUK_OP_LE                44
#define DUK_OP_LE_RR             44
#define DUK_OP_LE_CR             45
#define DUK_OP_LE_RC             46
#define DUK_OP_LE_CC             47
#define DUK_OP_IFTRUE            48
#define DUK_OP_IFTRUE_R          48
#define DUK_OP_IFTRUE_C          49
#define DUK_OP_IFFALSE           50
#define DUK_OP_IFFALSE_R         50
#define DUK_OP_IFFALSE_C         51
#define DUK_OP_ADD               52
#define DUK_OP_ADD_RR            52
#define DUK_OP_ADD_CR            53
#define DUK_OP_ADD_RC            54
#define DUK_OP_ADD_CC            55
#define DUK_OP_SUB               56
#define DUK_OP_SUB_RR            56
#define DUK_OP_SUB_CR            57
#define DUK_OP_SUB_RC            58
#define DUK_OP_SUB_CC            59
#define DUK_OP_MUL               60
#define DUK_OP_MUL_RR            60
#define DUK_OP_MUL_CR            61
#define DUK_OP_MUL_RC            62
#define DUK_OP_MUL_CC            63
#define DUK_OP_DIV               64
#define DUK_OP_DIV_RR            64
#define DUK_OP_DIV_CR            65
#define DUK_OP_DIV_RC            66
#define DUK_OP_DIV_CC            67
#define DUK_OP_MOD               68
#define DUK_OP_MOD_RR            68
#define DUK_OP_MOD_CR            69
#define DUK_OP_MOD_RC            70
#define DUK_OP_MOD_CC            71
#define DUK_OP_EXP               72
#define DUK_OP_EXP_RR            72
#define DUK_OP_EXP_CR            73
#define DUK_OP_EXP_RC            74
#define DUK_OP_EXP_CC            75
#define DUK_OP_BAND              76
#define DUK_OP_BAND_RR           76
#define DUK_OP_BAND_CR           77
#define DUK_OP_BAND_RC           78
#define DUK_OP_BAND_CC           79
#define DUK_OP_BOR               80
#define DUK_OP_BOR_RR            80
#define DUK_OP_BOR_CR            81
#define DUK_OP_BOR_RC            82
#define DUK_OP_BOR_CC            83
#define DUK_OP_BXOR              84
#define DUK_OP_BXOR_RR           84
#define DUK_OP_BXOR_CR           85
#define DUK_OP_BXOR_RC           86
#define DUK_OP_BXOR_CC           87
#define DUK_OP_BASL              88
#define DUK_OP_BASL_RR           88
#define DUK_OP_BASL_CR           89
#define DUK_OP_BASL_RC           90
#define DUK_OP_BASL_CC           91
#define DUK_OP_BLSR              92
#define DUK_OP_BLSR_RR           92
#define DUK_OP_BLSR_CR           93
#define DUK_OP_BLSR_RC           94
#define DUK_OP_BLSR_CC           95
#define DUK_OP_BASR              96
#define DUK_OP_BASR_RR           96
#define DUK_OP_BASR_CR           97
#define DUK_OP_BASR_RC           98
#define DUK_OP_BASR_CC           99
#define DUK_OP_INSTOF            100
#define DUK_OP_INSTOF_RR         100
#define DUK_OP_INSTOF_CR         101
#define DUK_OP_INSTOF_RC         102
#define DUK_OP_INSTOF_CC         103
#define DUK_OP_IN                104
#define DUK_OP_IN_RR             104
#define DUK_OP_IN_CR             105
#define DUK_OP_IN_RC             106
#define DUK_OP_IN_CC             107
#define DUK_OP_GETPROP           108
#define DUK_OP_GETPROP_RR        108
#define DUK_OP_GETPROP_CR        109
#define DUK_OP_GETPROP_RC        110
#define DUK_OP_GETPROP_CC        111
#define DUK_OP_PUTPROP           112
#define DUK_OP_PUTPROP_RR        112
#define DUK_OP_PUTPROP_CR        113
#define DUK_OP_PUTPROP_RC        114
#define DUK_OP_PUTPROP_CC        115
#define DUK_OP_DELPROP           116
#define DUK_OP_DELPROP_RR        116
#define DUK_OP_DELPROP_CR_UNUSED 117 /* unused now */
#define DUK_OP_DELPROP_RC        118
#define DUK_OP_DELPROP_CC_UNUSED 119 /* unused now */
#define DUK_OP_PREINCR           120 /* pre/post opcode values have constraints, */
#define DUK_OP_PREDECR           121 /* see duk_js_executor.c and duk_js_compiler.c. */
#define DUK_OP_POSTINCR          122
#define DUK_OP_POSTDECR          123
#define DUK_OP_PREINCV           124
#define DUK_OP_PREDECV           125
#define DUK_OP_POSTINCV          126
#define DUK_OP_POSTDECV          127
#define DUK_OP_PREINCP           128 /* pre/post inc/dec prop opcodes have constraints */
#define DUK_OP_PREINCP_RR        128
#define DUK_OP_PREINCP_CR        129
#define DUK_OP_PREINCP_RC        130
#define DUK_OP_PREINCP_CC        131
#define DUK_OP_PREDECP           132
#define DUK_OP_PREDECP_RR        132
#define DUK_OP_PREDECP_CR        133
#define DUK_OP_PREDECP_RC        134
#define DUK_OP_PREDECP_CC        135
#define DUK_OP_POSTINCP          136
#define DUK_OP_POSTINCP_RR       136
#define DUK_OP_POSTINCP_CR       137
#define DUK_OP_POSTINCP_RC       138
#define DUK_OP_POSTINCP_CC       139
#define DUK_OP_POSTDECP          140
#define DUK_OP_POSTDECP_RR       140
#define DUK_OP_POSTDECP_CR       141
#define DUK_OP_POSTDECP_RC       142
#define DUK_OP_POSTDECP_CC       143
#define DUK_OP_DECLVAR           144
#define DUK_OP_DECLVAR_RR        144
#define DUK_OP_DECLVAR_CR        145
#define DUK_OP_DECLVAR_RC        146
#define DUK_OP_DECLVAR_CC        147
#define DUK_OP_REGEXP            148
#define DUK_OP_REGEXP_RR         148
#define DUK_OP_REGEXP_CR         149
#define DUK_OP_REGEXP_RC         150
#define DUK_OP_REGEXP_CC         151
#define DUK_OP_CLOSURE           152
#define DUK_OP_TYPEOF            153
#define DUK_OP_TYPEOFID          154
#define DUK_OP_PUTVAR            155
#define DUK_OP_DELVAR            156
#define DUK_OP_RETREG            157
#define DUK_OP_RETUNDEF          158
#define DUK_OP_RETCONST          159
#define DUK_OP_RETCONSTN         160 /* return const without incref (e.g. number) */
#define DUK_OP_LABEL             161
#define DUK_OP_ENDLABEL          162
#define DUK_OP_BREAK             163
#define DUK_OP_CONTINUE          164
#define DUK_OP_TRYCATCH          165
#define DUK_OP_ENDTRY            166
#define DUK_OP_ENDCATCH          167
#define DUK_OP_ENDFIN            168
#define DUK_OP_THROW             169
#define DUK_OP_INVLHS            170
#define DUK_OP_CSREG             171
#define DUK_OP_CSVAR             172
#define DUK_OP_CSVAR_RR          172
#define DUK_OP_CSVAR_CR          173
#define DUK_OP_CSVAR_RC          174
#define DUK_OP_CSVAR_CC          175
#define DUK_OP_CALL0             176 /* DUK_OP_CALL0 & 0x0F must be zero. */
#define DUK_OP_CALL1             177
#define DUK_OP_CALL2             178
#define DUK_OP_CALL3             179
#define DUK_OP_CALL4             180
#define DUK_OP_CALL5             181
#define DUK_OP_CALL6             182
#define DUK_OP_CALL7             183
#define DUK_OP_CALL8             184
#define DUK_OP_CALL9             185
#define DUK_OP_CALL10            186
#define DUK_OP_CALL11            187
#define DUK_OP_CALL12            188
#define DUK_OP_CALL13            189
#define DUK_OP_CALL14            190
#define DUK_OP_CALL15            191
#define DUK_OP_NEWOBJ            192
#define DUK_OP_NEWARR            193
#define DUK_OP_MPUTOBJ           194
#define DUK_OP_MPUTOBJI          195
#define DUK_OP_INITSET           196
#define DUK_OP_INITGET           197
#define DUK_OP_MPUTARR           198
#define DUK_OP_MPUTARRI          199
#define DUK_OP_SETALEN           200
#define DUK_OP_INITENUM          201
#define DUK_OP_NEXTENUM          202
#define DUK_OP_NEWTARGET         203
#define DUK_OP_DEBUGGER          204
#define DUK_OP_NOP               205
#define DUK_OP_INVALID           206
#define DUK_OP_UNUSED207         207
#define DUK_OP_GETPROPC          208
#define DUK_OP_GETPROPC_RR       208
#define DUK_OP_GETPROPC_CR       209
#define DUK_OP_GETPROPC_RC       210
#define DUK_OP_GETPROPC_CC       211
#define DUK_OP_UNUSED212         212
#define DUK_OP_UNUSED213         213
#define DUK_OP_UNUSED214         214
#define DUK_OP_UNUSED215         215
#define DUK_OP_UNUSED216         216
#define DUK_OP_UNUSED217         217
#define DUK_OP_UNUSED218         218
#define DUK_OP_UNUSED219         219
#define DUK_OP_UNUSED220         220
#define DUK_OP_UNUSED221         221
#define DUK_OP_UNUSED222         222
#define DUK_OP_UNUSED223         223
#define DUK_OP_UNUSED224         224
#define DUK_OP_UNUSED225         225
#define DUK_OP_UNUSED226         226
#define DUK_OP_UNUSED227         227
#define DUK_OP_UNUSED228         228
#define DUK_OP_UNUSED229         229
#define DUK_OP_UNUSED230         230
#define DUK_OP_UNUSED231         231
#define DUK_OP_UNUSED232         232
#define DUK_OP_UNUSED233         233
#define DUK_OP_UNUSED234         234
#define DUK_OP_UNUSED235         235
#define DUK_OP_UNUSED236         236
#define DUK_OP_UNUSED237         237
#define DUK_OP_UNUSED238         238
#define DUK_OP_UNUSED239         239
#define DUK_OP_UNUSED240         240
#define DUK_OP_UNUSED241         241
#define DUK_OP_UNUSED242         242
#define DUK_OP_UNUSED243         243
#define DUK_OP_UNUSED244         244
#define DUK_OP_UNUSED245         245
#define DUK_OP_UNUSED246         246
#define DUK_OP_UNUSED247         247
#define DUK_OP_UNUSED248         248
#define DUK_OP_UNUSED249         249
#define DUK_OP_UNUSED250         250
#define DUK_OP_UNUSED251         251
#define DUK_OP_UNUSED252         252
#define DUK_OP_UNUSED253         253
#define DUK_OP_UNUSED254         254
#define DUK_OP_UNUSED255         255
#define DUK_OP_NONE              256 /* dummy value used as marker (doesn't fit in 8-bit field) */

/* XXX: Allocate flags from opcode field?  Would take 16 opcode slots
 * but avoids shuffling in more cases.  Maybe not worth it.
 */
/* DUK_OP_TRYCATCH flags in A. */
#define DUK_BC_TRYCATCH_FLAG_HAVE_CATCH    (1U << 0)
#define DUK_BC_TRYCATCH_FLAG_HAVE_FINALLY  (1U << 1)
#define DUK_BC_TRYCATCH_FLAG_CATCH_BINDING (1U << 2)
#define DUK_BC_TRYCATCH_FLAG_WITH_BINDING  (1U << 3)

/* DUK_OP_DECLVAR flags in A; bottom bits are reserved for propdesc flags
 * (DUK_PROPDESC_FLAG_XXX).
 */
#define DUK_BC_DECLVAR_FLAG_FUNC_DECL (1U << 4) /* function declaration */

/* DUK_OP_CALLn flags, part of opcode field.  Three lowest bits must match
 * DUK_CALL_FLAG_xxx directly.
 */
#define DUK_BC_CALL_FLAG_TAILCALL       (1U << 0)
#define DUK_BC_CALL_FLAG_CONSTRUCT      (1U << 1)
#define DUK_BC_CALL_FLAG_CALLED_AS_EVAL (1U << 2)
#define DUK_BC_CALL_FLAG_INDIRECT       (1U << 3)

/* Misc constants and helper macros. */
#define DUK_BC_LDINT_BIAS   (1L << 15)
#define DUK_BC_LDINTX_SHIFT 16
#define DUK_BC_JUMP_BIAS    (1L << 23)

#endif /* DUK_JS_BYTECODE_H_INCLUDED */
