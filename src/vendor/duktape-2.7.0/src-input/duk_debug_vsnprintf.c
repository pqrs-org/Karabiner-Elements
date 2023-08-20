/*
 *  Custom formatter for debug printing, allowing Duktape specific data
 *  structures (such as tagged values and heap objects) to be printed with
 *  a nice format string.  Because debug printing should not affect execution
 *  state, formatting here must be independent of execution (see implications
 *  below) and must not allocate memory.
 *
 *  Custom format tags begin with a '%!' to safely distinguish them from
 *  standard format tags.  The following conversions are supported:
 *
 *     %!T    tagged value (duk_tval *)
 *     %!O    heap object (duk_heaphdr *)
 *     %!I    decoded bytecode instruction
 *     %!X    bytecode instruction opcode name (arg is long)
 *     %!C    catcher (duk_catcher *)
 *     %!A    activation (duk_activation *)
 *
 *  Everything is serialized in a JSON-like manner.  The default depth is one
 *  level, internal prototype is not followed, and internal properties are not
 *  serialized.  The following modifiers change this behavior:
 *
 *     @      print pointers
 *     #      print binary representations (where applicable)
 *     d      deep traversal of own properties (not prototype)
 *     p      follow prototype chain (useless without 'd')
 *     i      include internal properties (other than prototype)
 *     x      hexdump buffers
 *     h      heavy formatting
 *
 *  For instance, the following serializes objects recursively, but does not
 *  follow the prototype chain nor print internal properties: "%!dO".
 *
 *  Notes:
 *
 *    * Standard snprintf return value semantics seem to vary.  This
 *      implementation returns the number of bytes it actually wrote
 *      (excluding the null terminator).  If retval == buffer size,
 *      output was truncated (except for corner cases).
 *
 *    * Output format is intentionally different from ECMAScript
 *      formatting requirements, as formatting here serves debugging
 *      of internals.
 *
 *    * Depth checking (and updating) is done in each type printer
 *      separately, to allow them to call each other freely.
 *
 *    * Some pathological structures might take ages to print (e.g.
 *      self recursion with 100 properties pointing to the object
 *      itself).  To guard against these, each printer also checks
 *      whether the output buffer is full; if so, early exit.
 *
 *    * Reference loops are detected using a loop stack.
 */

#include "duk_internal.h"

#if defined(DUK_USE_DEBUG)

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* list of conversion specifiers that terminate a format tag;
 * this is unfortunately guesswork.
 */
#define DUK__ALLOWED_STANDARD_SPECIFIERS "diouxXeEfFgGaAcsCSpnm"

/* maximum length of standard format tag that we support */
#define DUK__MAX_FORMAT_TAG_LENGTH 32

/* heapobj recursion depth when deep printing is selected */
#define DUK__DEEP_DEPTH_LIMIT 8

/* maximum recursion depth for loop detection stacks */
#define DUK__LOOP_STACK_DEPTH 256

/* must match bytecode defines now; build autogenerate? */
DUK_LOCAL const char * const duk__bc_optab[256] = {
	"LDREG",       "STREG",       "JUMP",        "LDCONST",     "LDINT",       "LDINTX",      "LDTHIS",      "LDUNDEF",
	"LDNULL",      "LDTRUE",      "LDFALSE",     "GETVAR",      "BNOT",        "LNOT",        "UNM",         "UNP",
	"EQ_RR",       "EQ_CR",       "EQ_RC",       "EQ_CC",       "NEQ_RR",      "NEQ_CR",      "NEQ_RC",      "NEQ_CC",
	"SEQ_RR",      "SEQ_CR",      "SEQ_RC",      "SEQ_CC",      "SNEQ_RR",     "SNEQ_CR",     "SNEQ_RC",     "SNEQ_CC",

	"GT_RR",       "GT_CR",       "GT_RC",       "GT_CC",       "GE_RR",       "GE_CR",       "GE_RC",       "GE_CC",
	"LT_RR",       "LT_CR",       "LT_RC",       "LT_CC",       "LE_RR",       "LE_CR",       "LE_RC",       "LE_CC",
	"IFTRUE_R",    "IFTRUE_C",    "IFFALSE_R",   "IFFALSE_C",   "ADD_RR",      "ADD_CR",      "ADD_RC",      "ADD_CC",
	"SUB_RR",      "SUB_CR",      "SUB_RC",      "SUB_CC",      "MUL_RR",      "MUL_CR",      "MUL_RC",      "MUL_CC",

	"DIV_RR",      "DIV_CR",      "DIV_RC",      "DIV_CC",      "MOD_RR",      "MOD_CR",      "MOD_RC",      "MOD_CC",
	"EXP_RR",      "EXP_CR",      "EXP_RC",      "EXP_CC",      "BAND_RR",     "BAND_CR",     "BAND_RC",     "BAND_CC",
	"BOR_RR",      "BOR_CR",      "BOR_RC",      "BOR_CC",      "BXOR_RR",     "BXOR_CR",     "BXOR_RC",     "BXOR_CC",
	"BASL_RR",     "BASL_CR",     "BASL_RC",     "BASL_CC",     "BLSR_RR",     "BLSR_CR",     "BLSR_RC",     "BLSR_CC",

	"BASR_RR",     "BASR_CR",     "BASR_RC",     "BASR_CC",     "INSTOF_RR",   "INSTOF_CR",   "INSTOF_RC",   "INSTOF_CC",
	"IN_RR",       "IN_CR",       "IN_RC",       "IN_CC",       "GETPROP_RR",  "GETPROP_CR",  "GETPROP_RC",  "GETPROP_CC",
	"PUTPROP_RR",  "PUTPROP_CR",  "PUTPROP_RC",  "PUTPROP_CC",  "DELPROP_RR",  "DELPROP_CR",  "DELPROP_RC",  "DELPROP_CC",
	"PREINCR",     "PREDECR",     "POSTINCR",    "POSTDECR",    "PREINCV",     "PREDECV",     "POSTINCV",    "POSTDECV",

	"PREINCP_RR",  "PREINCP_CR",  "PREINCP_RC",  "PREINCP_CC",  "PREDECP_RR",  "PREDECP_CR",  "PREDECP_RC",  "PREDECP_CC",
	"POSTINCP_RR", "POSTINCP_CR", "POSTINCP_RC", "POSTINCP_CC", "POSTDECP_RR", "POSTDECP_CR", "POSTDECP_RC", "POSTDECP_CC",
	"DECLVAR_RR",  "DECLVAR_CR",  "DECLVAR_RC",  "DECLVAR_CC",  "REGEXP_RR",   "REGEXP_RC",   "REGEXP_CR",   "REGEXP_CC",
	"CLOSURE",     "TYPEOF",      "TYPEOFID",    "PUTVAR",      "DELVAR",      "RETREG",      "RETUNDEF",    "RETCONST",

	"RETCONSTN",   "LABEL",       "ENDLABEL",    "BREAK",       "CONTINUE",    "TRYCATCH",    "ENDTRY",      "ENDCATCH",
	"ENDFIN",      "THROW",       "INVLHS",      "CSREG",       "CSVAR_RR",    "CSVAR_CR",    "CSVAR_RC",    "CSVAR_CC",
	"CALL0",       "CALL1",       "CALL2",       "CALL3",       "CALL4",       "CALL5",       "CALL6",       "CALL7",
	"CALL8",       "CALL9",       "CALL10",      "CALL11",      "CALL12",      "CALL13",      "CALL14",      "CALL15",

	"NEWOBJ",      "NEWARR",      "MPUTOBJ",     "MPUTOBJI",    "INITSET",     "INITGET",     "MPUTARR",     "MPUTARRI",
	"SETALEN",     "INITENUM",    "NEXTENUM",    "NEWTARGET",   "DEBUGGER",    "NOP",         "INVALID",     "UNUSED207",
	"GETPROPC_RR", "GETPROPC_CR", "GETPROPC_RC", "GETPROPC_CC", "UNUSED212",   "UNUSED213",   "UNUSED214",   "UNUSED215",
	"UNUSED216",   "UNUSED217",   "UNUSED218",   "UNUSED219",   "UNUSED220",   "UNUSED221",   "UNUSED222",   "UNUSED223",

	"UNUSED224",   "UNUSED225",   "UNUSED226",   "UNUSED227",   "UNUSED228",   "UNUSED229",   "UNUSED230",   "UNUSED231",
	"UNUSED232",   "UNUSED233",   "UNUSED234",   "UNUSED235",   "UNUSED236",   "UNUSED237",   "UNUSED238",   "UNUSED239",
	"UNUSED240",   "UNUSED241",   "UNUSED242",   "UNUSED243",   "UNUSED244",   "UNUSED245",   "UNUSED246",   "UNUSED247",
	"UNUSED248",   "UNUSED249",   "UNUSED250",   "UNUSED251",   "UNUSED252",   "UNUSED253",   "UNUSED254",   "UNUSED255"
};

typedef struct duk__dprint_state duk__dprint_state;
struct duk__dprint_state {
	duk_fixedbuffer *fb;

	/* loop_stack_index could be perhaps be replaced by 'depth', but it's nice
	 * to not couple these two mechanisms unnecessarily.
	 */
	duk_hobject *loop_stack[DUK__LOOP_STACK_DEPTH];
	duk_int_t loop_stack_index;
	duk_int_t loop_stack_limit;

	duk_int_t depth;
	duk_int_t depth_limit;

	duk_bool_t pointer;
	duk_bool_t heavy;
	duk_bool_t binary;
	duk_bool_t follow_proto;
	duk_bool_t internal;
	duk_bool_t hexdump;
};

/* helpers */
DUK_LOCAL_DECL void duk__print_hstring(duk__dprint_state *st, duk_hstring *k, duk_bool_t quotes);
DUK_LOCAL_DECL void duk__print_hobject(duk__dprint_state *st, duk_hobject *h);
DUK_LOCAL_DECL void duk__print_hbuffer(duk__dprint_state *st, duk_hbuffer *h);
DUK_LOCAL_DECL void duk__print_tval(duk__dprint_state *st, duk_tval *tv);
DUK_LOCAL_DECL void duk__print_instr(duk__dprint_state *st, duk_instr_t ins);
DUK_LOCAL_DECL void duk__print_heaphdr(duk__dprint_state *st, duk_heaphdr *h);
DUK_LOCAL_DECL void duk__print_shared_heaphdr(duk__dprint_state *st, duk_heaphdr *h);
DUK_LOCAL_DECL void duk__print_shared_heaphdr_string(duk__dprint_state *st, duk_heaphdr_string *h);

DUK_LOCAL void duk__print_shared_heaphdr(duk__dprint_state *st, duk_heaphdr *h) {
	duk_fixedbuffer *fb = st->fb;

	if (st->heavy) {
		duk_fb_sprintf(fb, "(%p)", (void *) h);
	}

	if (!h) {
		return;
	}

	if (st->binary) {
		duk_size_t i;
		duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_LBRACKET);
		for (i = 0; i < (duk_size_t) sizeof(*h); i++) {
			duk_fb_sprintf(fb, "%02lx", (unsigned long) ((duk_uint8_t *) h)[i]);
		}
		duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_RBRACKET);
	}

#if defined(DUK_USE_REFERENCE_COUNTING) /* currently implicitly also DUK_USE_DOUBLE_LINKED_HEAP */
	if (st->heavy) {
		duk_fb_sprintf(fb,
		               "[h_next=%p,h_prev=%p,h_refcount=%lu,h_flags=%08lx,type=%ld,"
		               "reachable=%ld,temproot=%ld,finalizable=%ld,finalized=%ld]",
		               (void *) DUK_HEAPHDR_GET_NEXT(NULL, h),
		               (void *) DUK_HEAPHDR_GET_PREV(NULL, h),
		               (unsigned long) DUK_HEAPHDR_GET_REFCOUNT(h),
		               (unsigned long) DUK_HEAPHDR_GET_FLAGS(h),
		               (long) DUK_HEAPHDR_GET_TYPE(h),
		               (long) (DUK_HEAPHDR_HAS_REACHABLE(h) ? 1 : 0),
		               (long) (DUK_HEAPHDR_HAS_TEMPROOT(h) ? 1 : 0),
		               (long) (DUK_HEAPHDR_HAS_FINALIZABLE(h) ? 1 : 0),
		               (long) (DUK_HEAPHDR_HAS_FINALIZED(h) ? 1 : 0));
	}
#else
	if (st->heavy) {
		duk_fb_sprintf(fb,
		               "[h_next=%p,h_flags=%08lx,type=%ld,reachable=%ld,temproot=%ld,finalizable=%ld,finalized=%ld]",
		               (void *) DUK_HEAPHDR_GET_NEXT(NULL, h),
		               (unsigned long) DUK_HEAPHDR_GET_FLAGS(h),
		               (long) DUK_HEAPHDR_GET_TYPE(h),
		               (long) (DUK_HEAPHDR_HAS_REACHABLE(h) ? 1 : 0),
		               (long) (DUK_HEAPHDR_HAS_TEMPROOT(h) ? 1 : 0),
		               (long) (DUK_HEAPHDR_HAS_FINALIZABLE(h) ? 1 : 0),
		               (long) (DUK_HEAPHDR_HAS_FINALIZED(h) ? 1 : 0));
	}
#endif
}

DUK_LOCAL void duk__print_shared_heaphdr_string(duk__dprint_state *st, duk_heaphdr_string *h) {
	duk_fixedbuffer *fb = st->fb;

	if (st->heavy) {
		duk_fb_sprintf(fb, "(%p)", (void *) h);
	}

	if (!h) {
		return;
	}

	if (st->binary) {
		duk_size_t i;
		duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_LBRACKET);
		for (i = 0; i < (duk_size_t) sizeof(*h); i++) {
			duk_fb_sprintf(fb, "%02lx", (unsigned long) ((duk_uint8_t *) h)[i]);
		}
		duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_RBRACKET);
	}

#if defined(DUK_USE_REFERENCE_COUNTING)
	if (st->heavy) {
		duk_fb_sprintf(fb,
		               "[h_refcount=%lu,h_flags=%08lx,type=%ld,reachable=%ld,temproot=%ld,finalizable=%ld,finalized=%ld]",
		               (unsigned long) DUK_HEAPHDR_GET_REFCOUNT((duk_heaphdr *) h),
		               (unsigned long) DUK_HEAPHDR_GET_FLAGS((duk_heaphdr *) h),
		               (long) DUK_HEAPHDR_GET_TYPE((duk_heaphdr *) h),
		               (long) (DUK_HEAPHDR_HAS_REACHABLE((duk_heaphdr *) h) ? 1 : 0),
		               (long) (DUK_HEAPHDR_HAS_TEMPROOT((duk_heaphdr *) h) ? 1 : 0),
		               (long) (DUK_HEAPHDR_HAS_FINALIZABLE((duk_heaphdr *) h) ? 1 : 0),
		               (long) (DUK_HEAPHDR_HAS_FINALIZED((duk_heaphdr *) h) ? 1 : 0));
	}
#else
	if (st->heavy) {
		duk_fb_sprintf(fb,
		               "[h_flags=%08lx,type=%ld,reachable=%ld,temproot=%ld,finalizable=%ld,finalized=%ld]",
		               (unsigned long) DUK_HEAPHDR_GET_FLAGS((duk_heaphdr *) h),
		               (long) DUK_HEAPHDR_GET_TYPE((duk_heaphdr *) h),
		               (long) (DUK_HEAPHDR_HAS_REACHABLE((duk_heaphdr *) h) ? 1 : 0),
		               (long) (DUK_HEAPHDR_HAS_TEMPROOT((duk_heaphdr *) h) ? 1 : 0),
		               (long) (DUK_HEAPHDR_HAS_FINALIZABLE((duk_heaphdr *) h) ? 1 : 0),
		               (long) (DUK_HEAPHDR_HAS_FINALIZED((duk_heaphdr *) h) ? 1 : 0));
	}
#endif
}

DUK_LOCAL void duk__print_hstring(duk__dprint_state *st, duk_hstring *h, duk_bool_t quotes) {
	duk_fixedbuffer *fb = st->fb;
	const duk_uint8_t *p;
	const duk_uint8_t *p_end;

	/* terminal type: no depth check */

	if (duk_fb_is_full(fb)) {
		return;
	}

	duk__print_shared_heaphdr_string(st, &h->hdr);

	if (!h) {
		duk_fb_put_cstring(fb, "NULL");
		return;
	}

	p = DUK_HSTRING_GET_DATA(h);
	p_end = p + DUK_HSTRING_GET_BYTELEN(h);

	if (p_end > p && p[0] == DUK_ASC_UNDERSCORE) {
		/* If property key begins with underscore, encode it with
		 * forced quotes (e.g. "_Foo") to distinguish it from encoded
		 * internal properties (e.g. \x82Bar -> _Bar).
		 */
		quotes = 1;
	}

	if (quotes) {
		duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_DOUBLEQUOTE);
	}
	while (p < p_end) {
		duk_uint8_t ch = *p++;

		/* two special escapes: '\' and '"', other printables as is */
		if (ch == '\\') {
			duk_fb_sprintf(fb, "\\\\");
		} else if (ch == '"') {
			duk_fb_sprintf(fb, "\\\"");
		} else if (ch >= 0x20 && ch <= 0x7e) {
			duk_fb_put_byte(fb, ch);
		} else if (ch == 0x82 && !quotes) {
			/* encode \x82Bar as _Bar if no quotes are
			 * applied, this is for readable internal keys.
			 */
			duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_UNDERSCORE);
		} else {
			duk_fb_sprintf(fb, "\\x%02lx", (unsigned long) ch);
		}
	}
	if (quotes) {
		duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_DOUBLEQUOTE);
	}
#if defined(DUK_USE_REFERENCE_COUNTING)
	/* XXX: limit to quoted strings only, to save keys from being cluttered? */
	duk_fb_sprintf(fb, "/%lu", (unsigned long) DUK_HEAPHDR_GET_REFCOUNT(&h->hdr));
#endif
}

#define DUK__COMMA() \
	do { \
		if (first) { \
			first = 0; \
		} else { \
			duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_COMMA); \
		} \
	} while (0)

DUK_LOCAL void duk__print_hobject(duk__dprint_state *st, duk_hobject *h) {
	duk_fixedbuffer *fb = st->fb;
	duk_uint_fast32_t i;
	duk_tval *tv;
	duk_hstring *key;
	duk_bool_t first = 1;
	const char *brace1 = "{";
	const char *brace2 = "}";
	duk_bool_t pushed_loopstack = 0;

	if (duk_fb_is_full(fb)) {
		return;
	}

	duk__print_shared_heaphdr(st, &h->hdr);

	if (h && DUK_HOBJECT_HAS_ARRAY_PART(h)) {
		brace1 = "[";
		brace2 = "]";
	}

	if (!h) {
		duk_fb_put_cstring(fb, "NULL");
		goto finished;
	}

	if (st->depth >= st->depth_limit) {
		const char *subtype = "generic";

		if (DUK_HOBJECT_IS_COMPFUNC(h)) {
			subtype = "compfunc";
		} else if (DUK_HOBJECT_IS_NATFUNC(h)) {
			subtype = "natfunc";
		} else if (DUK_HOBJECT_IS_THREAD(h)) {
			subtype = "thread";
		} else if (DUK_HOBJECT_IS_BUFOBJ(h)) {
			subtype = "bufobj";
		} else if (DUK_HOBJECT_IS_ARRAY(h)) {
			subtype = "array";
		}
		duk_fb_sprintf(fb, "%sobject/%s %p%s", (const char *) brace1, subtype, (void *) h, (const char *) brace2);
		return;
	}

	for (i = 0; i < (duk_uint_fast32_t) st->loop_stack_index; i++) {
		if (st->loop_stack[i] == h) {
			duk_fb_sprintf(fb, "%sLOOP:%p%s", (const char *) brace1, (void *) h, (const char *) brace2);
			return;
		}
	}

	/* after this, return paths should 'goto finished' for decrement */
	st->depth++;

	if (st->loop_stack_index >= st->loop_stack_limit) {
		duk_fb_sprintf(fb, "%sOUT-OF-LOOP-STACK%s", (const char *) brace1, (const char *) brace2);
		goto finished;
	}
	st->loop_stack[st->loop_stack_index++] = h;
	pushed_loopstack = 1;

	/*
	 *  Notation: double underscore used for internal properties which are not
	 *  stored in the property allocation (e.g. '__valstack').
	 */

	duk_fb_put_cstring(fb, brace1);

	if (DUK_HOBJECT_GET_PROPS(NULL, h)) {
		duk_uint32_t a_limit;

		a_limit = DUK_HOBJECT_GET_ASIZE(h);
		if (st->internal) {
			/* dump all allocated entries, unused entries print as 'unused',
			 * note that these may extend beyond current 'length' and look
			 * a bit funny.
			 */
		} else {
			/* leave out trailing 'unused' elements */
			while (a_limit > 0) {
				tv = DUK_HOBJECT_A_GET_VALUE_PTR(NULL, h, a_limit - 1);
				if (!DUK_TVAL_IS_UNUSED(tv)) {
					break;
				}
				a_limit--;
			}
		}

		for (i = 0; i < a_limit; i++) {
			tv = DUK_HOBJECT_A_GET_VALUE_PTR(NULL, h, i);
			DUK__COMMA();
			duk__print_tval(st, tv);
		}
		for (i = 0; i < DUK_HOBJECT_GET_ENEXT(h); i++) {
			key = DUK_HOBJECT_E_GET_KEY(NULL, h, i);
			if (!key) {
				continue;
			}
			if (!st->internal && DUK_HSTRING_HAS_HIDDEN(key)) {
				continue;
			}
			DUK__COMMA();
			duk__print_hstring(st, key, 0);
			duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_COLON);
			if (DUK_HOBJECT_E_SLOT_IS_ACCESSOR(NULL, h, i)) {
				duk_fb_sprintf(fb,
				               "[get:%p,set:%p]",
				               (void *) DUK_HOBJECT_E_GET_VALUE(NULL, h, i).a.get,
				               (void *) DUK_HOBJECT_E_GET_VALUE(NULL, h, i).a.set);
			} else {
				tv = &DUK_HOBJECT_E_GET_VALUE(NULL, h, i).v;
				duk__print_tval(st, tv);
			}
			if (st->heavy) {
				duk_fb_sprintf(fb, "<%02lx>", (unsigned long) DUK_HOBJECT_E_GET_FLAGS(NULL, h, i));
			}
		}
	}
	if (st->internal) {
		if (DUK_HOBJECT_IS_ARRAY(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__array:true");
		}
		if (DUK_HOBJECT_HAS_EXTENSIBLE(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__extensible:true");
		}
		if (DUK_HOBJECT_HAS_CONSTRUCTABLE(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__constructable:true");
		}
		if (DUK_HOBJECT_HAS_BOUNDFUNC(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__boundfunc:true");
		}
		if (DUK_HOBJECT_HAS_COMPFUNC(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__compfunc:true");
		}
		if (DUK_HOBJECT_HAS_NATFUNC(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__natfunc:true");
		}
		if (DUK_HOBJECT_HAS_BUFOBJ(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__bufobj:true");
		}
		if (DUK_HOBJECT_IS_THREAD(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__thread:true");
		}
		if (DUK_HOBJECT_HAS_ARRAY_PART(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__array_part:true");
		}
		if (DUK_HOBJECT_HAS_STRICT(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__strict:true");
		}
		if (DUK_HOBJECT_HAS_NOTAIL(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__notail:true");
		}
		if (DUK_HOBJECT_HAS_NEWENV(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__newenv:true");
		}
		if (DUK_HOBJECT_HAS_NAMEBINDING(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__namebinding:true");
		}
		if (DUK_HOBJECT_HAS_CREATEARGS(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__createargs:true");
		}
		if (DUK_HOBJECT_HAS_EXOTIC_ARRAY(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__exotic_array:true");
		}
		if (DUK_HOBJECT_HAS_EXOTIC_STRINGOBJ(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__exotic_stringobj:true");
		}
		if (DUK_HOBJECT_HAS_EXOTIC_ARGUMENTS(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__exotic_arguments:true");
		}
		if (DUK_HOBJECT_IS_BUFOBJ(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__exotic_bufobj:true");
		}
		if (DUK_HOBJECT_HAS_EXOTIC_PROXYOBJ(h)) {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__exotic_proxyobj:true");
		}
	}

	if (st->internal && DUK_HOBJECT_IS_ARRAY(h)) {
		duk_harray *a = (duk_harray *) h;
		DUK__COMMA();
		duk_fb_sprintf(fb, "__length:%ld", (long) a->length);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__length_nonwritable:%ld", (long) a->length_nonwritable);
	} else if (st->internal && DUK_HOBJECT_IS_COMPFUNC(h)) {
		duk_hcompfunc *f = (duk_hcompfunc *) h;
		DUK__COMMA();
		duk_fb_put_cstring(fb, "__data:");
		duk__print_hbuffer(st, (duk_hbuffer *) DUK_HCOMPFUNC_GET_DATA(NULL, f));
		DUK__COMMA();
		duk_fb_put_cstring(fb, "__lexenv:");
		duk__print_hobject(st, DUK_HCOMPFUNC_GET_LEXENV(NULL, f));
		DUK__COMMA();
		duk_fb_put_cstring(fb, "__varenv:");
		duk__print_hobject(st, DUK_HCOMPFUNC_GET_VARENV(NULL, f));
		DUK__COMMA();
		duk_fb_sprintf(fb, "__nregs:%ld", (long) f->nregs);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__nargs:%ld", (long) f->nargs);
#if defined(DUK_USE_DEBUGGER_SUPPORT)
		DUK__COMMA();
		duk_fb_sprintf(fb, "__start_line:%ld", (long) f->start_line);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__end_line:%ld", (long) f->end_line);
#endif
		DUK__COMMA();
		duk_fb_put_cstring(fb, "__data:");
		duk__print_hbuffer(st, (duk_hbuffer *) DUK_HCOMPFUNC_GET_DATA(NULL, f));
	} else if (st->internal && DUK_HOBJECT_IS_NATFUNC(h)) {
		duk_hnatfunc *f = (duk_hnatfunc *) h;
		DUK__COMMA();
		duk_fb_sprintf(fb, "__func:");
		duk_fb_put_funcptr(fb, (duk_uint8_t *) &f->func, sizeof(f->func));
		DUK__COMMA();
		duk_fb_sprintf(fb, "__nargs:%ld", (long) f->nargs);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__magic:%ld", (long) f->magic);
	} else if (st->internal && DUK_HOBJECT_IS_DECENV(h)) {
		duk_hdecenv *e = (duk_hdecenv *) h;
		DUK__COMMA();
		duk_fb_sprintf(fb, "__thread:");
		duk__print_hobject(st, (duk_hobject *) e->thread);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__varmap:");
		duk__print_hobject(st, (duk_hobject *) e->varmap);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__regbase_byteoff:%ld", (long) e->regbase_byteoff);
	} else if (st->internal && DUK_HOBJECT_IS_OBJENV(h)) {
		duk_hobjenv *e = (duk_hobjenv *) h;
		DUK__COMMA();
		duk_fb_sprintf(fb, "__target:");
		duk__print_hobject(st, (duk_hobject *) e->target);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__has_this:%ld", (long) e->has_this);
#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
	} else if (st->internal && DUK_HOBJECT_IS_BUFOBJ(h)) {
		duk_hbufobj *b = (duk_hbufobj *) h;
		DUK__COMMA();
		duk_fb_sprintf(fb, "__buf:");
		duk__print_hbuffer(st, (duk_hbuffer *) b->buf);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__buf_prop:");
		duk__print_hobject(st, (duk_hobject *) b->buf_prop);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__offset:%ld", (long) b->offset);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__length:%ld", (long) b->length);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__shift:%ld", (long) b->shift);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__elemtype:%ld", (long) b->elem_type);
#endif
	} else if (st->internal && DUK_HOBJECT_IS_PROXY(h)) {
		duk_hproxy *p = (duk_hproxy *) h;
		DUK__COMMA();
		duk_fb_sprintf(fb, "__target:");
		duk__print_hobject(st, p->target);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__handler:");
		duk__print_hobject(st, p->handler);
	} else if (st->internal && DUK_HOBJECT_IS_THREAD(h)) {
		duk_hthread *t = (duk_hthread *) h;
		DUK__COMMA();
		duk_fb_sprintf(fb, "__ptr_curr_pc:%p", (void *) t->ptr_curr_pc);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__heap:%p", (void *) t->heap);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__strict:%ld", (long) t->strict);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__state:%ld", (long) t->state);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__unused1:%ld", (long) t->unused1);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__unused2:%ld", (long) t->unused2);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__valstack:%p", (void *) t->valstack);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__valstack_end:%p/%ld", (void *) t->valstack_end, (long) (t->valstack_end - t->valstack));
		DUK__COMMA();
		duk_fb_sprintf(fb,
		               "__valstack_alloc_end:%p/%ld",
		               (void *) t->valstack_alloc_end,
		               (long) (t->valstack_alloc_end - t->valstack));
		DUK__COMMA();
		duk_fb_sprintf(fb,
		               "__valstack_bottom:%p/%ld",
		               (void *) t->valstack_bottom,
		               (long) (t->valstack_bottom - t->valstack));
		DUK__COMMA();
		duk_fb_sprintf(fb, "__valstack_top:%p/%ld", (void *) t->valstack_top, (long) (t->valstack_top - t->valstack));
		DUK__COMMA();
		duk_fb_sprintf(fb, "__callstack_curr:%p", (void *) t->callstack_curr);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__callstack_top:%ld", (long) t->callstack_top);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__callstack_preventcount:%ld", (long) t->callstack_preventcount);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__resumer:");
		duk__print_hobject(st, (duk_hobject *) t->resumer);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__compile_ctx:%p", (void *) t->compile_ctx);
#if defined(DUK_USE_INTERRUPT_COUNTER)
		DUK__COMMA();
		duk_fb_sprintf(fb, "__interrupt_counter:%ld", (long) t->interrupt_counter);
		DUK__COMMA();
		duk_fb_sprintf(fb, "__interrupt_init:%ld", (long) t->interrupt_init);
#endif

		/* XXX: print built-ins array? */
	}
#if defined(DUK_USE_REFERENCE_COUNTING)
	if (st->internal) {
		DUK__COMMA();
		duk_fb_sprintf(fb, "__refcount:%lu", (unsigned long) DUK_HEAPHDR_GET_REFCOUNT((duk_heaphdr *) h));
	}
#endif
	if (st->internal) {
		DUK__COMMA();
		duk_fb_sprintf(fb, "__class:%ld", (long) DUK_HOBJECT_GET_CLASS_NUMBER(h));
	}

	DUK__COMMA();
	duk_fb_sprintf(fb, "__heapptr:%p", (void *) h); /* own pointer */

	/* prototype should be last, for readability */
	if (DUK_HOBJECT_GET_PROTOTYPE(NULL, h)) {
		if (st->follow_proto) {
			DUK__COMMA();
			duk_fb_put_cstring(fb, "__prototype:");
			duk__print_hobject(st, DUK_HOBJECT_GET_PROTOTYPE(NULL, h));
		} else {
			DUK__COMMA();
			duk_fb_sprintf(fb, "__prototype:%p", (void *) DUK_HOBJECT_GET_PROTOTYPE(NULL, h));
		}
	}

	duk_fb_put_cstring(fb, brace2);

#if defined(DUK_USE_HOBJECT_HASH_PART)
	if (st->heavy && DUK_HOBJECT_GET_HSIZE(h) > 0) {
		duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_LANGLE);
		for (i = 0; i < DUK_HOBJECT_GET_HSIZE(h); i++) {
			duk_uint_t h_idx = DUK_HOBJECT_H_GET_INDEX(NULL, h, i);
			if (i > 0) {
				duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_COMMA);
			}
			if (h_idx == DUK_HOBJECT_HASHIDX_UNUSED) {
				duk_fb_sprintf(fb, "u");
			} else if (h_idx == DUK_HOBJECT_HASHIDX_DELETED) {
				duk_fb_sprintf(fb, "d");
			} else {
				duk_fb_sprintf(fb, "%ld", (long) h_idx);
			}
		}
		duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_RANGLE);
	}
#endif

finished:
	st->depth--;
	if (pushed_loopstack) {
		st->loop_stack_index--;
		st->loop_stack[st->loop_stack_index] = NULL;
	}
}

DUK_LOCAL void duk__print_hbuffer(duk__dprint_state *st, duk_hbuffer *h) {
	duk_fixedbuffer *fb = st->fb;
	duk_size_t i, n;
	duk_uint8_t *p;

	if (duk_fb_is_full(fb)) {
		return;
	}

	/* terminal type: no depth check */

	if (!h) {
		duk_fb_put_cstring(fb, "NULL");
		return;
	}

	if (DUK_HBUFFER_HAS_DYNAMIC(h)) {
		if (DUK_HBUFFER_HAS_EXTERNAL(h)) {
			duk_hbuffer_external *g = (duk_hbuffer_external *) h;
			duk_fb_sprintf(fb,
			               "buffer:external:%p:%ld",
			               (void *) DUK_HBUFFER_EXTERNAL_GET_DATA_PTR(NULL, g),
			               (long) DUK_HBUFFER_EXTERNAL_GET_SIZE(g));
		} else {
			duk_hbuffer_dynamic *g = (duk_hbuffer_dynamic *) h;
			duk_fb_sprintf(fb,
			               "buffer:dynamic:%p:%ld",
			               (void *) DUK_HBUFFER_DYNAMIC_GET_DATA_PTR(NULL, g),
			               (long) DUK_HBUFFER_DYNAMIC_GET_SIZE(g));
		}
	} else {
		duk_fb_sprintf(fb, "buffer:fixed:%ld", (long) DUK_HBUFFER_GET_SIZE(h));
	}

#if defined(DUK_USE_REFERENCE_COUNTING)
	duk_fb_sprintf(fb, "/%lu", (unsigned long) DUK_HEAPHDR_GET_REFCOUNT(&h->hdr));
#endif

	if (st->hexdump) {
		duk_fb_sprintf(fb, "=[");
		n = DUK_HBUFFER_GET_SIZE(h);
		p = (duk_uint8_t *) DUK_HBUFFER_GET_DATA_PTR(NULL, h);
		for (i = 0; i < n; i++) {
			duk_fb_sprintf(fb, "%02lx", (unsigned long) p[i]);
		}
		duk_fb_sprintf(fb, "]");
	}
}

DUK_LOCAL void duk__print_heaphdr(duk__dprint_state *st, duk_heaphdr *h) {
	duk_fixedbuffer *fb = st->fb;

	if (duk_fb_is_full(fb)) {
		return;
	}

	if (!h) {
		duk_fb_put_cstring(fb, "NULL");
		return;
	}

	switch (DUK_HEAPHDR_GET_TYPE(h)) {
	case DUK_HTYPE_STRING:
		duk__print_hstring(st, (duk_hstring *) h, 1);
		break;
	case DUK_HTYPE_OBJECT:
		duk__print_hobject(st, (duk_hobject *) h);
		break;
	case DUK_HTYPE_BUFFER:
		duk__print_hbuffer(st, (duk_hbuffer *) h);
		break;
	default:
		duk_fb_sprintf(fb, "[unknown htype %ld]", (long) DUK_HEAPHDR_GET_TYPE(h));
		break;
	}
}

DUK_LOCAL void duk__print_tval(duk__dprint_state *st, duk_tval *tv) {
	duk_fixedbuffer *fb = st->fb;

	if (duk_fb_is_full(fb)) {
		return;
	}

	/* depth check is done when printing an actual type */

	if (st->heavy) {
		duk_fb_sprintf(fb, "(%p)", (void *) tv);
	}

	if (!tv) {
		duk_fb_put_cstring(fb, "NULL");
		return;
	}

	if (st->binary) {
		duk_size_t i;
		duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_LBRACKET);
		for (i = 0; i < (duk_size_t) sizeof(*tv); i++) {
			duk_fb_sprintf(fb, "%02lx", (unsigned long) ((duk_uint8_t *) tv)[i]);
		}
		duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_RBRACKET);
	}

	if (st->heavy) {
		duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_LANGLE);
	}
	switch (DUK_TVAL_GET_TAG(tv)) {
	case DUK_TAG_UNDEFINED: {
		duk_fb_put_cstring(fb, "undefined");
		break;
	}
	case DUK_TAG_UNUSED: {
		duk_fb_put_cstring(fb, "unused");
		break;
	}
	case DUK_TAG_NULL: {
		duk_fb_put_cstring(fb, "null");
		break;
	}
	case DUK_TAG_BOOLEAN: {
		duk_fb_put_cstring(fb, DUK_TVAL_GET_BOOLEAN(tv) ? "true" : "false");
		break;
	}
	case DUK_TAG_STRING: {
		/* Note: string is a terminal heap object, so no depth check here */
		duk__print_hstring(st, DUK_TVAL_GET_STRING(tv), 1);
		break;
	}
	case DUK_TAG_OBJECT: {
		duk__print_hobject(st, DUK_TVAL_GET_OBJECT(tv));
		break;
	}
	case DUK_TAG_BUFFER: {
		duk__print_hbuffer(st, DUK_TVAL_GET_BUFFER(tv));
		break;
	}
	case DUK_TAG_POINTER: {
		duk_fb_sprintf(fb, "pointer:%p", (void *) DUK_TVAL_GET_POINTER(tv));
		break;
	}
	case DUK_TAG_LIGHTFUNC: {
		duk_c_function func;
		duk_small_uint_t lf_flags;

		DUK_TVAL_GET_LIGHTFUNC(tv, func, lf_flags);
		duk_fb_sprintf(fb, "lightfunc:");
		duk_fb_put_funcptr(fb, (duk_uint8_t *) &func, sizeof(func));
		duk_fb_sprintf(fb, ":%04lx", (long) lf_flags);
		break;
	}
#if defined(DUK_USE_FASTINT)
	case DUK_TAG_FASTINT:
		DUK_ASSERT(!DUK_TVAL_IS_UNUSED(tv));
		DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv));
		duk_fb_sprintf(fb, "%.18g_F", (double) DUK_TVAL_GET_NUMBER(tv));
		break;
#endif
	default: {
		/* IEEE double is approximately 16 decimal digits; print a couple extra */
		DUK_ASSERT(!DUK_TVAL_IS_UNUSED(tv));
		DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv));
		duk_fb_sprintf(fb, "%.18g", (double) DUK_TVAL_GET_NUMBER(tv));
		break;
	}
	}
	if (st->heavy) {
		duk_fb_put_byte(fb, (duk_uint8_t) DUK_ASC_RANGLE);
	}
}

DUK_LOCAL void duk__print_instr(duk__dprint_state *st, duk_instr_t ins) {
	duk_fixedbuffer *fb = st->fb;
	duk_small_int_t op;
	const char *op_name;

	op = (duk_small_int_t) DUK_DEC_OP(ins);
	op_name = duk__bc_optab[op];

	/* XXX: option to fix opcode length so it lines up nicely */

	if (op == DUK_OP_JUMP) {
		duk_int_t diff1 = (duk_int_t) (DUK_DEC_ABC(ins) - DUK_BC_JUMP_BIAS); /* from next pc */
		duk_int_t diff2 = diff1 + 1; /* from curr pc */

		duk_fb_sprintf(fb,
		               "%s %ld (to pc%c%ld)",
		               (const char *) op_name,
		               (long) diff1,
		               (int) (diff2 >= 0 ? '+' : '-'), /* char format: use int */
		               (long) (diff2 >= 0 ? diff2 : -diff2));
	} else {
		duk_fb_sprintf(fb,
		               "%s %ld, %ld, %ld",
		               (const char *) op_name,
		               (long) DUK_DEC_A(ins),
		               (long) DUK_DEC_B(ins),
		               (long) DUK_DEC_C(ins));
	}
}

DUK_LOCAL void duk__print_opcode(duk__dprint_state *st, duk_small_int_t opcode) {
	duk_fixedbuffer *fb = st->fb;

	if (opcode < DUK_BC_OP_MIN || opcode > DUK_BC_OP_MAX) {
		duk_fb_sprintf(fb, "?(%ld)", (long) opcode);
	} else {
		duk_fb_sprintf(fb, "%s", (const char *) duk__bc_optab[opcode]);
	}
}

DUK_LOCAL void duk__print_catcher(duk__dprint_state *st, duk_catcher *cat) {
	duk_fixedbuffer *fb = st->fb;

	if (duk_fb_is_full(fb)) {
		return;
	}

	if (!cat) {
		duk_fb_put_cstring(fb, "NULL");
		return;
	}

	duk_fb_sprintf(fb,
	               "[catcher ptr=%p parent=%p varname=%p pc_base=%p, idx_base=%ld, flags=0x%08lx]",
	               (void *) cat,
	               (void *) cat->parent,
	               (void *) cat->h_varname,
	               (void *) cat->pc_base,
	               (long) cat->idx_base,
	               (unsigned long) cat->flags);
}

DUK_LOCAL void duk__print_activation(duk__dprint_state *st, duk_activation *act) {
	duk_fixedbuffer *fb = st->fb;

	if (duk_fb_is_full(fb)) {
		return;
	}

	if (!act) {
		duk_fb_put_cstring(fb, "NULL");
		return;
	}

	/* prev_caller: conditional, omitted on purpose, it's rarely used. */
	/* prev_line: conditional, omitted on purpose (but would be nice). */
	duk_fb_sprintf(fb,
	               "[activation ptr=%p tv_func=<omit> func=%p parent=%p var_env=%p lex_env=%p cat=%p curr_pc=%p "
	               "bottom_byteoff=%ld retval_byteoff=%ld reserve_byteoff=%ld flags=%ld]",
	               (void *) act,
	               (void *) act->func,
	               (void *) act->parent,
	               (void *) act->var_env,
	               (void *) act->lex_env,
	               (void *) act->cat,
	               (void *) act->curr_pc,
	               (long) act->bottom_byteoff,
	               (long) act->retval_byteoff,
	               (long) act->reserve_byteoff,
	               (long) act->flags);
}

DUK_INTERNAL duk_int_t duk_debug_vsnprintf(char *str, duk_size_t size, const char *format, va_list ap) {
	duk_fixedbuffer fb;
	const char *p = format;
	const char *p_end = p + DUK_STRLEN(format);
	duk_int_t retval;

	duk_memzero(&fb, sizeof(fb));
	fb.buffer = (duk_uint8_t *) str;
	fb.length = size;
	fb.offset = 0;
	fb.truncated = 0;

	while (p < p_end) {
		char ch = *p++;
		const char *p_begfmt = NULL;
		duk_bool_t got_exclamation = 0;
		duk_bool_t got_long = 0; /* %lf, %ld etc */
		duk__dprint_state st;

		if (ch != DUK_ASC_PERCENT) {
			duk_fb_put_byte(&fb, (duk_uint8_t) ch);
			continue;
		}

		/*
		 *  Format tag parsing.  Since we don't understand all the
		 *  possible format tags allowed, we just scan for a terminating
		 *  specifier and keep track of relevant modifiers that we do
		 *  understand.  See man 3 printf.
		 */

		duk_memzero(&st, sizeof(st));
		st.fb = &fb;
		st.depth = 0;
		st.depth_limit = 1;
		st.loop_stack_index = 0;
		st.loop_stack_limit = DUK__LOOP_STACK_DEPTH;

		p_begfmt = p - 1;
		while (p < p_end) {
			ch = *p++;

			if (ch == DUK_ASC_STAR) {
				/* unsupported: would consume multiple args */
				goto format_error;
			} else if (ch == DUK_ASC_PERCENT) {
				duk_fb_put_byte(&fb, (duk_uint8_t) DUK_ASC_PERCENT);
				break;
			} else if (ch == DUK_ASC_EXCLAMATION) {
				got_exclamation = 1;
			} else if (!got_exclamation && ch == DUK_ASC_LC_L) {
				got_long = 1;
			} else if (got_exclamation && ch == DUK_ASC_LC_D) {
				st.depth_limit = DUK__DEEP_DEPTH_LIMIT;
			} else if (got_exclamation && ch == DUK_ASC_LC_P) {
				st.follow_proto = 1;
			} else if (got_exclamation && ch == DUK_ASC_LC_I) {
				st.internal = 1;
			} else if (got_exclamation && ch == DUK_ASC_LC_X) {
				st.hexdump = 1;
			} else if (got_exclamation && ch == DUK_ASC_LC_H) {
				st.heavy = 1;
			} else if (got_exclamation && ch == DUK_ASC_ATSIGN) {
				st.pointer = 1;
			} else if (got_exclamation && ch == DUK_ASC_HASH) {
				st.binary = 1;
			} else if (got_exclamation && ch == DUK_ASC_UC_T) {
				duk_tval *t = va_arg(ap, duk_tval *);
				if (st.pointer && !st.heavy) {
					duk_fb_sprintf(&fb, "(%p)", (void *) t);
				}
				duk__print_tval(&st, t);
				break;
			} else if (got_exclamation && ch == DUK_ASC_UC_O) {
				duk_heaphdr *t = va_arg(ap, duk_heaphdr *);
				if (st.pointer && !st.heavy) {
					duk_fb_sprintf(&fb, "(%p)", (void *) t);
				}
				duk__print_heaphdr(&st, t);
				break;
			} else if (got_exclamation && ch == DUK_ASC_UC_I) {
				duk_instr_t t = va_arg(ap, duk_instr_t);
				duk__print_instr(&st, t);
				break;
			} else if (got_exclamation && ch == DUK_ASC_UC_X) {
				long t = va_arg(ap, long);
				duk__print_opcode(&st, (duk_small_int_t) t);
				break;
			} else if (got_exclamation && ch == DUK_ASC_UC_C) {
				duk_catcher *t = va_arg(ap, duk_catcher *);
				duk__print_catcher(&st, t);
				break;
			} else if (got_exclamation && ch == DUK_ASC_UC_A) {
				duk_activation *t = va_arg(ap, duk_activation *);
				duk__print_activation(&st, t);
				break;
			} else if (!got_exclamation && strchr(DUK__ALLOWED_STANDARD_SPECIFIERS, (int) ch)) {
				char fmtbuf[DUK__MAX_FORMAT_TAG_LENGTH];
				duk_size_t fmtlen;

				DUK_ASSERT(p >= p_begfmt);
				fmtlen = (duk_size_t) (p - p_begfmt);
				if (fmtlen >= sizeof(fmtbuf)) {
					/* format is too large, abort */
					goto format_error;
				}
				duk_memzero(fmtbuf, sizeof(fmtbuf));
				duk_memcpy(fmtbuf, p_begfmt, fmtlen);

				/* assume exactly 1 arg, which is why '*' is forbidden; arg size still
				 * depends on type though.
				 */

				if (ch == DUK_ASC_LC_F || ch == DUK_ASC_LC_G || ch == DUK_ASC_LC_E) {
					/* %f and %lf both consume a 'long' */
					double arg = va_arg(ap, double);
					duk_fb_sprintf(&fb, fmtbuf, arg);
				} else if (ch == DUK_ASC_LC_D && got_long) {
					/* %ld */
					long arg = va_arg(ap, long);
					duk_fb_sprintf(&fb, fmtbuf, arg);
				} else if (ch == DUK_ASC_LC_D) {
					/* %d; only 16 bits are guaranteed */
					int arg = va_arg(ap, int);
					duk_fb_sprintf(&fb, fmtbuf, arg);
				} else if (ch == DUK_ASC_LC_U && got_long) {
					/* %lu */
					unsigned long arg = va_arg(ap, unsigned long);
					duk_fb_sprintf(&fb, fmtbuf, arg);
				} else if (ch == DUK_ASC_LC_U) {
					/* %u; only 16 bits are guaranteed */
					unsigned int arg = va_arg(ap, unsigned int);
					duk_fb_sprintf(&fb, fmtbuf, arg);
				} else if (ch == DUK_ASC_LC_X && got_long) {
					/* %lx */
					unsigned long arg = va_arg(ap, unsigned long);
					duk_fb_sprintf(&fb, fmtbuf, arg);
				} else if (ch == DUK_ASC_LC_X) {
					/* %x; only 16 bits are guaranteed */
					unsigned int arg = va_arg(ap, unsigned int);
					duk_fb_sprintf(&fb, fmtbuf, arg);
				} else if (ch == DUK_ASC_LC_S) {
					/* %s */
					const char *arg = va_arg(ap, const char *);
					if (arg == NULL) {
						/* '%s' and NULL is not portable, so special case
						 * it for debug printing.
						 */
						duk_fb_sprintf(&fb, "NULL");
					} else {
						duk_fb_sprintf(&fb, fmtbuf, arg);
					}
				} else if (ch == DUK_ASC_LC_P) {
					/* %p */
					void *arg = va_arg(ap, void *);
					if (arg == NULL) {
						/* '%p' and NULL is portable, but special case it
						 * anyway to get a standard NULL marker in logs.
						 */
						duk_fb_sprintf(&fb, "NULL");
					} else {
						duk_fb_sprintf(&fb, fmtbuf, arg);
					}
				} else if (ch == DUK_ASC_LC_C) {
					/* '%c', passed concretely as int */
					int arg = va_arg(ap, int);
					duk_fb_sprintf(&fb, fmtbuf, arg);
				} else {
					/* Should not happen. */
					duk_fb_sprintf(&fb, "INVALID-FORMAT(%s)", (const char *) fmtbuf);
				}
				break;
			} else {
				/* ignore */
			}
		}
	}
	goto done;

format_error:
	duk_fb_put_cstring(&fb, "FMTERR");
	/* fall through */

done:
	retval = (duk_int_t) fb.offset;
	duk_fb_put_byte(&fb, (duk_uint8_t) 0);

	/* return total chars written excluding terminator */
	return retval;
}

#if 0 /*unused*/
DUK_INTERNAL duk_int_t duk_debug_snprintf(char *str, duk_size_t size, const char *format, ...) {
	duk_int_t retval;
	va_list ap;
	va_start(ap, format);
	retval = duk_debug_vsnprintf(str, size, format, ap);
	va_end(ap);
	return retval;
}
#endif

/* Formatting function pointers is tricky: there is no standard pointer for
 * function pointers and the size of a function pointer may depend on the
 * specific pointer type.  This helper formats a function pointer based on
 * its memory layout to get something useful on most platforms.
 */
DUK_INTERNAL void duk_debug_format_funcptr(char *buf, duk_size_t buf_size, duk_uint8_t *fptr, duk_size_t fptr_size) {
	duk_size_t i;
	duk_uint8_t *p = (duk_uint8_t *) buf;
	duk_uint8_t *p_end = (duk_uint8_t *) (buf + buf_size - 1);

	DUK_ASSERT(buf != NULL);
	duk_memzero(buf, buf_size);

	for (i = 0; i < fptr_size; i++) {
		duk_int_t left = (duk_int_t) (p_end - p);
		duk_uint8_t ch;
		if (left <= 0) {
			break;
		}

		/* Quite approximate but should be useful for little and big endian. */
#if defined(DUK_USE_INTEGER_BE)
		ch = fptr[i];
#else
		ch = fptr[fptr_size - 1 - i];
#endif
		p += DUK_SNPRINTF((char *) p, (duk_size_t) left, "%02lx", (unsigned long) ch);
	}
}

#endif /* DUK_USE_DEBUG */
