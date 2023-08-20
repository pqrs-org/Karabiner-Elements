/*
 *  Regexp executor.
 *
 *  Safety: the ECMAScript executor should prevent user from reading and
 *  replacing regexp bytecode.  Even so, the executor must validate all
 *  memory accesses etc.  When an invalid access is detected (e.g. a 'save'
 *  opcode to invalid, unallocated index) it should fail with an internal
 *  error but not cause a segmentation fault.
 *
 *  Notes:
 *
 *    - Backtrack counts are limited to unsigned 32 bits but should
 *      technically be duk_size_t for strings longer than 4G chars.
 *      This also requires a regexp bytecode change.
 */

#include "duk_internal.h"

#if defined(DUK_USE_REGEXP_SUPPORT)

/*
 *  Helpers for UTF-8 handling
 *
 *  For bytecode readers the duk_uint32_t and duk_int32_t types are correct
 *  because they're used for more than just codepoints.
 */

DUK_LOCAL duk_uint32_t duk__bc_get_u32(duk_re_matcher_ctx *re_ctx, const duk_uint8_t **pc) {
	return (duk_uint32_t) duk_unicode_decode_xutf8_checked(re_ctx->thr, pc, re_ctx->bytecode, re_ctx->bytecode_end);
}

DUK_LOCAL duk_int32_t duk__bc_get_i32(duk_re_matcher_ctx *re_ctx, const duk_uint8_t **pc) {
	duk_uint32_t t;

	/* signed integer encoding needed to work with UTF-8 */
	t = (duk_uint32_t) duk_unicode_decode_xutf8_checked(re_ctx->thr, pc, re_ctx->bytecode, re_ctx->bytecode_end);
	if (t & 1) {
		return -((duk_int32_t) (t >> 1));
	} else {
		return (duk_int32_t) (t >> 1);
	}
}

DUK_LOCAL const duk_uint8_t *duk__utf8_backtrack(duk_hthread *thr,
                                                 const duk_uint8_t **ptr,
                                                 const duk_uint8_t *ptr_start,
                                                 const duk_uint8_t *ptr_end,
                                                 duk_uint_fast32_t count) {
	const duk_uint8_t *p;

	/* Note: allow backtracking from p == ptr_end */
	p = *ptr;
	if (p < ptr_start || p > ptr_end) {
		goto fail;
	}

	while (count > 0) {
		for (;;) {
			p--;
			if (p < ptr_start) {
				goto fail;
			}
			if ((*p & 0xc0) != 0x80) {
				/* utf-8 continuation bytes have the form 10xx xxxx */
				break;
			}
		}
		count--;
	}
	*ptr = p;
	return p;

fail:
	DUK_ERROR_INTERNAL(thr);
	DUK_WO_NORETURN(return NULL;);
}

DUK_LOCAL const duk_uint8_t *duk__utf8_advance(duk_hthread *thr,
                                               const duk_uint8_t **ptr,
                                               const duk_uint8_t *ptr_start,
                                               const duk_uint8_t *ptr_end,
                                               duk_uint_fast32_t count) {
	const duk_uint8_t *p;

	p = *ptr;
	if (p < ptr_start || p >= ptr_end) {
		goto fail;
	}

	while (count > 0) {
		for (;;) {
			p++;

			/* Note: if encoding ends by hitting end of input, we don't check that
			 * the encoding is valid, we just assume it is.
			 */
			if (p >= ptr_end || ((*p & 0xc0) != 0x80)) {
				/* utf-8 continuation bytes have the form 10xx xxxx */
				break;
			}
		}
		count--;
	}

	*ptr = p;
	return p;

fail:
	DUK_ERROR_INTERNAL(thr);
	DUK_WO_NORETURN(return NULL;);
}

/*
 *  Helpers for dealing with the input string
 */

/* Get a (possibly canonicalized) input character from current sp.  The input
 * itself is never modified, and captures always record non-canonicalized
 * characters even in case-insensitive matching.  Return <0 if out of input.
 */
DUK_LOCAL duk_codepoint_t duk__inp_get_cp(duk_re_matcher_ctx *re_ctx, const duk_uint8_t **sp) {
	duk_codepoint_t res;

	if (*sp >= re_ctx->input_end) {
		return -1;
	}
	res = (duk_codepoint_t) duk_unicode_decode_xutf8_checked(re_ctx->thr, sp, re_ctx->input, re_ctx->input_end);
	if (re_ctx->re_flags & DUK_RE_FLAG_IGNORE_CASE) {
		res = duk_unicode_re_canonicalize_char(re_ctx->thr, res);
	}
	return res;
}

DUK_LOCAL const duk_uint8_t *duk__inp_backtrack(duk_re_matcher_ctx *re_ctx, const duk_uint8_t **sp, duk_uint_fast32_t count) {
	return duk__utf8_backtrack(re_ctx->thr, sp, re_ctx->input, re_ctx->input_end, count);
}

/* Backtrack utf-8 input and return a (possibly canonicalized) input character. */
DUK_LOCAL duk_codepoint_t duk__inp_get_prev_cp(duk_re_matcher_ctx *re_ctx, const duk_uint8_t *sp) {
	/* note: caller 'sp' is intentionally not updated here */
	(void) duk__inp_backtrack(re_ctx, &sp, (duk_uint_fast32_t) 1);
	return duk__inp_get_cp(re_ctx, &sp);
}

/*
 *  Regexp recursive matching function.
 *
 *  Returns 'sp' on successful match (points to character after last matched one),
 *  NULL otherwise.
 *
 *  The C recursion depth limit check is only performed in this function, this
 *  suffices because the function is present in all true recursion required by
 *  regexp execution.
 */

DUK_LOCAL const duk_uint8_t *duk__match_regexp(duk_re_matcher_ctx *re_ctx, const duk_uint8_t *pc, const duk_uint8_t *sp) {
	duk_native_stack_check(re_ctx->thr);
	if (re_ctx->recursion_depth >= re_ctx->recursion_limit) {
		DUK_ERROR_RANGE(re_ctx->thr, DUK_STR_REGEXP_EXECUTOR_RECURSION_LIMIT);
		DUK_WO_NORETURN(return NULL;);
	}
	re_ctx->recursion_depth++;

	for (;;) {
		duk_small_int_t op;

		if (re_ctx->steps_count >= re_ctx->steps_limit) {
			DUK_ERROR_RANGE(re_ctx->thr, DUK_STR_REGEXP_EXECUTOR_STEP_LIMIT);
			DUK_WO_NORETURN(return NULL;);
		}
		re_ctx->steps_count++;

		/* Opcodes are at most 7 bits now so they encode to one byte.  If this
		 * were not the case or 'pc' is invalid here (due to a bug etc) we'll
		 * still fail safely through the switch default case.
		 */
		DUK_ASSERT(pc[0] <= 0x7fU);
#if 0
		op = (duk_small_int_t) duk__bc_get_u32(re_ctx, &pc);
#endif
		op = *pc++;

		DUK_DDD(DUK_DDDPRINT("match: rec=%ld, steps=%ld, pc (after op)=%ld, sp=%ld, op=%ld",
		                     (long) re_ctx->recursion_depth,
		                     (long) re_ctx->steps_count,
		                     (long) (pc - re_ctx->bytecode),
		                     (long) (sp - re_ctx->input),
		                     (long) op));

		switch (op) {
		case DUK_REOP_MATCH: {
			goto match;
		}
		case DUK_REOP_CHAR: {
			/*
			 *  Byte-based matching would be possible for case-sensitive
			 *  matching but not for case-insensitive matching.  So, we
			 *  match by decoding the input and bytecode character normally.
			 *
			 *  Bytecode characters are assumed to be already canonicalized.
			 *  Input characters are canonicalized automatically by
			 *  duk__inp_get_cp() if necessary.
			 *
			 *  There is no opcode for matching multiple characters.  The
			 *  regexp compiler has trouble joining strings efficiently
			 *  during compilation.  See doc/regexp.rst for more discussion.
			 */
			duk_codepoint_t c1, c2;

			c1 = (duk_codepoint_t) duk__bc_get_u32(re_ctx, &pc);
			DUK_ASSERT(!(re_ctx->re_flags & DUK_RE_FLAG_IGNORE_CASE) ||
			           c1 == duk_unicode_re_canonicalize_char(re_ctx->thr, c1)); /* canonicalized by compiler */
			c2 = duk__inp_get_cp(re_ctx, &sp);
			/* No need to check for c2 < 0 (end of input): because c1 >= 0, it
			 * will fail the match below automatically and cause goto fail.
			 */
#if 0
			if (c2 < 0) {
				goto fail;
			}
#endif
			DUK_ASSERT(c1 >= 0);

			DUK_DDD(DUK_DDDPRINT("char match, c1=%ld, c2=%ld", (long) c1, (long) c2));
			if (c1 != c2) {
				goto fail;
			}
			break;
		}
		case DUK_REOP_PERIOD: {
			duk_codepoint_t c;

			c = duk__inp_get_cp(re_ctx, &sp);
			if (c < 0 || duk_unicode_is_line_terminator(c)) {
				/* E5 Sections 15.10.2.8, 7.3 */
				goto fail;
			}
			break;
		}
		case DUK_REOP_RANGES:
		case DUK_REOP_INVRANGES: {
			duk_uint32_t n;
			duk_codepoint_t c;
			duk_small_int_t match;

			n = duk__bc_get_u32(re_ctx, &pc);
			c = duk__inp_get_cp(re_ctx, &sp);
			if (c < 0) {
				goto fail;
			}

			match = 0;
			while (n) {
				duk_codepoint_t r1, r2;
				r1 = (duk_codepoint_t) duk__bc_get_u32(re_ctx, &pc);
				r2 = (duk_codepoint_t) duk__bc_get_u32(re_ctx, &pc);
				DUK_DDD(DUK_DDDPRINT("matching ranges/invranges, n=%ld, r1=%ld, r2=%ld, c=%ld",
				                     (long) n,
				                     (long) r1,
				                     (long) r2,
				                     (long) c));
				if (c >= r1 && c <= r2) {
					/* Note: don't bail out early, we must read all the ranges from
					 * bytecode.  Another option is to skip them efficiently after
					 * breaking out of here.  Prefer smallest code.
					 */
					match = 1;
				}
				n--;
			}

			if (op == DUK_REOP_RANGES) {
				if (!match) {
					goto fail;
				}
			} else {
				DUK_ASSERT(op == DUK_REOP_INVRANGES);
				if (match) {
					goto fail;
				}
			}
			break;
		}
		case DUK_REOP_ASSERT_START: {
			duk_codepoint_t c;

			if (sp <= re_ctx->input) {
				break;
			}
			if (!(re_ctx->re_flags & DUK_RE_FLAG_MULTILINE)) {
				goto fail;
			}
			c = duk__inp_get_prev_cp(re_ctx, sp);
			if (duk_unicode_is_line_terminator(c)) {
				/* E5 Sections 15.10.2.8, 7.3 */
				break;
			}
			goto fail;
		}
		case DUK_REOP_ASSERT_END: {
			duk_codepoint_t c;
			const duk_uint8_t *tmp_sp;

			tmp_sp = sp;
			c = duk__inp_get_cp(re_ctx, &tmp_sp);
			if (c < 0) {
				break;
			}
			if (!(re_ctx->re_flags & DUK_RE_FLAG_MULTILINE)) {
				goto fail;
			}
			if (duk_unicode_is_line_terminator(c)) {
				/* E5 Sections 15.10.2.8, 7.3 */
				break;
			}
			goto fail;
		}
		case DUK_REOP_ASSERT_WORD_BOUNDARY:
		case DUK_REOP_ASSERT_NOT_WORD_BOUNDARY: {
			/*
			 *  E5 Section 15.10.2.6.  The previous and current character
			 *  should -not- be canonicalized as they are now.  However,
			 *  canonicalization does not affect the result of IsWordChar()
			 *  (which depends on Unicode characters never canonicalizing
			 *  into ASCII characters) so this does not matter.
			 */
			duk_small_int_t w1, w2;

			if (sp <= re_ctx->input) {
				w1 = 0; /* not a wordchar */
			} else {
				duk_codepoint_t c;
				c = duk__inp_get_prev_cp(re_ctx, sp);
				w1 = duk_unicode_re_is_wordchar(c);
			}
			if (sp >= re_ctx->input_end) {
				w2 = 0; /* not a wordchar */
			} else {
				const duk_uint8_t *tmp_sp = sp; /* dummy so sp won't get updated */
				duk_codepoint_t c;
				c = duk__inp_get_cp(re_ctx, &tmp_sp);
				w2 = duk_unicode_re_is_wordchar(c);
			}

			if (op == DUK_REOP_ASSERT_WORD_BOUNDARY) {
				if (w1 == w2) {
					goto fail;
				}
			} else {
				DUK_ASSERT(op == DUK_REOP_ASSERT_NOT_WORD_BOUNDARY);
				if (w1 != w2) {
					goto fail;
				}
			}
			break;
		}
		case DUK_REOP_JUMP: {
			duk_int32_t skip;

			skip = duk__bc_get_i32(re_ctx, &pc);
			pc += skip;
			break;
		}
		case DUK_REOP_SPLIT1: {
			/* split1: prefer direct execution (no jump) */
			const duk_uint8_t *sub_sp;
			duk_int32_t skip;

			skip = duk__bc_get_i32(re_ctx, &pc);
			sub_sp = duk__match_regexp(re_ctx, pc, sp);
			if (sub_sp) {
				sp = sub_sp;
				goto match;
			}
			pc += skip;
			break;
		}
		case DUK_REOP_SPLIT2: {
			/* split2: prefer jump execution (not direct) */
			const duk_uint8_t *sub_sp;
			duk_int32_t skip;

			skip = duk__bc_get_i32(re_ctx, &pc);
			sub_sp = duk__match_regexp(re_ctx, pc + skip, sp);
			if (sub_sp) {
				sp = sub_sp;
				goto match;
			}
			break;
		}
		case DUK_REOP_SQMINIMAL: {
			duk_uint32_t q, qmin, qmax;
			duk_int32_t skip;
			const duk_uint8_t *sub_sp;

			qmin = duk__bc_get_u32(re_ctx, &pc);
			qmax = duk__bc_get_u32(re_ctx, &pc);
			skip = duk__bc_get_i32(re_ctx, &pc);
			DUK_DDD(DUK_DDDPRINT("minimal quantifier, qmin=%lu, qmax=%lu, skip=%ld",
			                     (unsigned long) qmin,
			                     (unsigned long) qmax,
			                     (long) skip));

			q = 0;
			while (q <= qmax) {
				if (q >= qmin) {
					sub_sp = duk__match_regexp(re_ctx, pc + skip, sp);
					if (sub_sp) {
						sp = sub_sp;
						goto match;
					}
				}
				sub_sp = duk__match_regexp(re_ctx, pc, sp);
				if (!sub_sp) {
					break;
				}
				sp = sub_sp;
				q++;
			}
			goto fail;
		}
		case DUK_REOP_SQGREEDY: {
			duk_uint32_t q, qmin, qmax, atomlen;
			duk_int32_t skip;
			const duk_uint8_t *sub_sp;

			qmin = duk__bc_get_u32(re_ctx, &pc);
			qmax = duk__bc_get_u32(re_ctx, &pc);
			atomlen = duk__bc_get_u32(re_ctx, &pc);
			skip = duk__bc_get_i32(re_ctx, &pc);
			DUK_DDD(DUK_DDDPRINT("greedy quantifier, qmin=%lu, qmax=%lu, atomlen=%lu, skip=%ld",
			                     (unsigned long) qmin,
			                     (unsigned long) qmax,
			                     (unsigned long) atomlen,
			                     (long) skip));

			q = 0;
			while (q < qmax) {
				sub_sp = duk__match_regexp(re_ctx, pc, sp);
				if (!sub_sp) {
					break;
				}
				sp = sub_sp;
				q++;
			}
			while (q >= qmin) {
				sub_sp = duk__match_regexp(re_ctx, pc + skip, sp);
				if (sub_sp) {
					sp = sub_sp;
					goto match;
				}
				if (q == qmin) {
					break;
				}

				/* Note: if atom were to contain e.g. captures, we would need to
				 * re-match the atom to get correct captures.  Simply quantifiers
				 * do not allow captures in their atom now, so this is not an issue.
				 */

				DUK_DDD(DUK_DDDPRINT("greedy quantifier, backtrack %ld characters (atomlen)", (long) atomlen));
				sp = duk__inp_backtrack(re_ctx, &sp, (duk_uint_fast32_t) atomlen);
				q--;
			}
			goto fail;
		}
		case DUK_REOP_SAVE: {
			duk_uint32_t idx;
			const duk_uint8_t *old;
			const duk_uint8_t *sub_sp;

			idx = duk__bc_get_u32(re_ctx, &pc);
			if (idx >= re_ctx->nsaved) {
				/* idx is unsigned, < 0 check is not necessary */
				DUK_D(DUK_DPRINT("internal error, regexp save index insane: idx=%ld", (long) idx));
				goto internal_error;
			}
			old = re_ctx->saved[idx];
			re_ctx->saved[idx] = sp;
			sub_sp = duk__match_regexp(re_ctx, pc, sp);
			if (sub_sp) {
				sp = sub_sp;
				goto match;
			}
			re_ctx->saved[idx] = old;
			goto fail;
		}
		case DUK_REOP_WIPERANGE: {
			/* Wipe capture range and save old values for backtracking.
			 *
			 * XXX: this typically happens with a relatively small idx_count.
			 * It might be useful to handle cases where the count is small
			 * (say <= 8) by saving the values in stack instead.  This would
			 * reduce memory churn and improve performance, at the cost of a
			 * slightly higher code footprint.
			 */
			duk_uint32_t idx_start, idx_count;
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
			duk_uint32_t idx_end, idx;
#endif
			duk_uint8_t **range_save;
			const duk_uint8_t *sub_sp;

			idx_start = duk__bc_get_u32(re_ctx, &pc);
			idx_count = duk__bc_get_u32(re_ctx, &pc);
			DUK_DDD(DUK_DDDPRINT("wipe saved range: start=%ld, count=%ld -> [%ld,%ld] (captures [%ld,%ld])",
			                     (long) idx_start,
			                     (long) idx_count,
			                     (long) idx_start,
			                     (long) (idx_start + idx_count - 1),
			                     (long) (idx_start / 2),
			                     (long) ((idx_start + idx_count - 1) / 2)));
			if (idx_start + idx_count > re_ctx->nsaved || idx_count == 0) {
				/* idx is unsigned, < 0 check is not necessary */
				DUK_D(DUK_DPRINT("internal error, regexp wipe indices insane: idx_start=%ld, idx_count=%ld",
				                 (long) idx_start,
				                 (long) idx_count));
				goto internal_error;
			}
			DUK_ASSERT(idx_count > 0);

			duk_require_stack(re_ctx->thr, 1);
			range_save = (duk_uint8_t **) duk_push_fixed_buffer_nozero(re_ctx->thr, sizeof(duk_uint8_t *) * idx_count);
			DUK_ASSERT(range_save != NULL);
			duk_memcpy(range_save, re_ctx->saved + idx_start, sizeof(duk_uint8_t *) * idx_count);
#if defined(DUK_USE_EXPLICIT_NULL_INIT)
			idx_end = idx_start + idx_count;
			for (idx = idx_start; idx < idx_end; idx++) {
				re_ctx->saved[idx] = NULL;
			}
#else
			duk_memzero((void *) (re_ctx->saved + idx_start), sizeof(duk_uint8_t *) * idx_count);
#endif

			sub_sp = duk__match_regexp(re_ctx, pc, sp);
			if (sub_sp) {
				/* match: keep wiped/resaved values */
				DUK_DDD(DUK_DDDPRINT("match: keep wiped/resaved values [%ld,%ld] (captures [%ld,%ld])",
				                     (long) idx_start,
				                     (long) (idx_start + idx_count - 1),
				                     (long) (idx_start / 2),
				                     (long) ((idx_start + idx_count - 1) / 2)));
				duk_pop_unsafe(re_ctx->thr);
				sp = sub_sp;
				goto match;
			}

			/* fail: restore saves */
			DUK_DDD(DUK_DDDPRINT("fail: restore wiped/resaved values [%ld,%ld] (captures [%ld,%ld])",
			                     (long) idx_start,
			                     (long) (idx_start + idx_count - 1),
			                     (long) (idx_start / 2),
			                     (long) ((idx_start + idx_count - 1) / 2)));
			duk_memcpy((void *) (re_ctx->saved + idx_start),
			           (const void *) range_save,
			           sizeof(duk_uint8_t *) * idx_count);
			duk_pop_unsafe(re_ctx->thr);
			goto fail;
		}
		case DUK_REOP_LOOKPOS:
		case DUK_REOP_LOOKNEG: {
			/*
			 *  Needs a save of multiple saved[] entries depending on what range
			 *  may be overwritten.  Because the regexp parser does no such analysis,
			 *  we currently save the entire saved array here.  Lookaheads are thus
			 *  a bit expensive.  Note that the saved array is not needed for just
			 *  the lookahead sub-match, but for the matching of the entire sequel.
			 *
			 *  The temporary save buffer is pushed on to the valstack to handle
			 *  errors correctly.  Each lookahead causes a C recursion and pushes
			 *  more stuff on the value stack.  If the C recursion limit is less
			 *  than the value stack slack, there is no need to check the stack.
			 *  We do so regardless, just in case.
			 */

			duk_int32_t skip;
			duk_uint8_t **full_save;
			const duk_uint8_t *sub_sp;

			DUK_ASSERT(re_ctx->nsaved > 0);

			duk_require_stack(re_ctx->thr, 1);
			full_save =
			    (duk_uint8_t **) duk_push_fixed_buffer_nozero(re_ctx->thr, sizeof(duk_uint8_t *) * re_ctx->nsaved);
			DUK_ASSERT(full_save != NULL);
			duk_memcpy(full_save, re_ctx->saved, sizeof(duk_uint8_t *) * re_ctx->nsaved);

			skip = duk__bc_get_i32(re_ctx, &pc);
			sub_sp = duk__match_regexp(re_ctx, pc, sp);
			if (op == DUK_REOP_LOOKPOS) {
				if (!sub_sp) {
					goto lookahead_fail;
				}
			} else {
				if (sub_sp) {
					goto lookahead_fail;
				}
			}
			sub_sp = duk__match_regexp(re_ctx, pc + skip, sp);
			if (sub_sp) {
				/* match: keep saves */
				duk_pop_unsafe(re_ctx->thr);
				sp = sub_sp;
				goto match;
			}

			/* fall through */

		lookahead_fail:
			/* fail: restore saves */
			duk_memcpy((void *) re_ctx->saved, (const void *) full_save, sizeof(duk_uint8_t *) * re_ctx->nsaved);
			duk_pop_unsafe(re_ctx->thr);
			goto fail;
		}
		case DUK_REOP_BACKREFERENCE: {
			/*
			 *  Byte matching for back-references would be OK in case-
			 *  sensitive matching.  In case-insensitive matching we need
			 *  to canonicalize characters, so back-reference matching needs
			 *  to be done with codepoints instead.  So, we just decode
			 *  everything normally here, too.
			 *
			 *  Note: back-reference index which is 0 or higher than
			 *  NCapturingParens (= number of capturing parens in the
			 *  -entire- regexp) is a compile time error.  However, a
			 *  backreference referring to a valid capture which has
			 *  not matched anything always succeeds!  See E5 Section
			 *  15.10.2.9, step 5, sub-step 3.
			 */
			duk_uint32_t idx;
			const duk_uint8_t *p;

			idx = duk__bc_get_u32(re_ctx, &pc);
			idx = idx << 1; /* backref n -> saved indices [n*2, n*2+1] */
			if (idx < 2 || idx + 1 >= re_ctx->nsaved) {
				/* regexp compiler should catch these */
				DUK_D(DUK_DPRINT("internal error, backreference index insane"));
				goto internal_error;
			}
			if (!re_ctx->saved[idx] || !re_ctx->saved[idx + 1]) {
				/* capture is 'undefined', always matches! */
				DUK_DDD(DUK_DDDPRINT("backreference: saved[%ld,%ld] not complete, always match",
				                     (long) idx,
				                     (long) (idx + 1)));
				break;
			}
			DUK_DDD(DUK_DDDPRINT("backreference: match saved[%ld,%ld]", (long) idx, (long) (idx + 1)));

			p = re_ctx->saved[idx];
			while (p < re_ctx->saved[idx + 1]) {
				duk_codepoint_t c1, c2;

				/* Note: not necessary to check p against re_ctx->input_end:
				 * the memory access is checked by duk__inp_get_cp(), while
				 * valid compiled regexps cannot write a saved[] entry
				 * which points to outside the string.
				 */
				c1 = duk__inp_get_cp(re_ctx, &p);
				DUK_ASSERT(c1 >= 0);
				c2 = duk__inp_get_cp(re_ctx, &sp);
				/* No need for an explicit c2 < 0 check: because c1 >= 0,
				 * the comparison will always fail if c2 < 0.
				 */
#if 0
				if (c2 < 0) {
					goto fail;
				}
#endif
				if (c1 != c2) {
					goto fail;
				}
			}
			break;
		}
		default: {
			DUK_D(DUK_DPRINT("internal error, regexp opcode error: %ld", (long) op));
			goto internal_error;
		}
		}
	}

match:
	re_ctx->recursion_depth--;
	return sp;

fail:
	re_ctx->recursion_depth--;
	return NULL;

internal_error:
	DUK_ERROR_INTERNAL(re_ctx->thr);
	DUK_WO_NORETURN(return NULL;);
}

/*
 *  Exposed matcher function which provides the semantics of RegExp.prototype.exec().
 *
 *  RegExp.prototype.test() has the same semantics as exec() but does not return the
 *  result object (which contains the matching string and capture groups).  Currently
 *  there is no separate test() helper, so a temporary result object is created and
 *  discarded if test() is needed.  This is intentional, to save code space.
 *
 *  Input stack:  [ ... re_obj input ]
 *  Output stack: [ ... result ]
 */

DUK_LOCAL void duk__regexp_match_helper(duk_hthread *thr, duk_small_int_t force_global) {
	duk_re_matcher_ctx re_ctx;
	duk_hobject *h_regexp;
	duk_hstring *h_bytecode;
	duk_hstring *h_input;
	duk_uint8_t *p_buf;
	const duk_uint8_t *pc;
	const duk_uint8_t *sp;
	duk_small_int_t match = 0;
	duk_small_int_t global;
	duk_uint_fast32_t i;
	double d;
	duk_uint32_t char_offset;

	DUK_ASSERT(thr != NULL);

	DUK_DD(DUK_DDPRINT("regexp match: regexp=%!T, input=%!T",
	                   (duk_tval *) duk_get_tval(thr, -2),
	                   (duk_tval *) duk_get_tval(thr, -1)));

	/*
	 *  Regexp instance check, bytecode check, input coercion.
	 *
	 *  See E5 Section 15.10.6.
	 */

	/* TypeError if wrong; class check, see E5 Section 15.10.6 */
	h_regexp = duk_require_hobject_with_class(thr, -2, DUK_HOBJECT_CLASS_REGEXP);
	DUK_ASSERT(h_regexp != NULL);
	DUK_ASSERT(DUK_HOBJECT_GET_CLASS_NUMBER(h_regexp) == DUK_HOBJECT_CLASS_REGEXP);
	DUK_UNREF(h_regexp);

	h_input = duk_to_hstring(thr, -1);
	DUK_ASSERT(h_input != NULL);

	duk_xget_owndataprop_stridx_short(thr, -2, DUK_STRIDX_INT_BYTECODE); /* [ ... re_obj input ] -> [ ... re_obj input bc ] */
	h_bytecode =
	    duk_require_hstring(thr, -1); /* no regexp instance should exist without a non-configurable bytecode property */
	DUK_ASSERT(h_bytecode != NULL);

	/*
	 *  Basic context initialization.
	 *
	 *  Some init values are read from the bytecode header
	 *  whose format is (UTF-8 codepoints):
	 *
	 *    uint   flags
	 *    uint   nsaved (even, 2n+2 where n = num captures)
	 */

	/* [ ... re_obj input bc ] */

	duk_memzero(&re_ctx, sizeof(re_ctx));

	re_ctx.thr = thr;
	re_ctx.input = (const duk_uint8_t *) DUK_HSTRING_GET_DATA(h_input);
	re_ctx.input_end = re_ctx.input + DUK_HSTRING_GET_BYTELEN(h_input);
	re_ctx.bytecode = (const duk_uint8_t *) DUK_HSTRING_GET_DATA(h_bytecode);
	re_ctx.bytecode_end = re_ctx.bytecode + DUK_HSTRING_GET_BYTELEN(h_bytecode);
	re_ctx.saved = NULL;
	re_ctx.recursion_limit = DUK_USE_REGEXP_EXECUTOR_RECLIMIT;
	re_ctx.steps_limit = DUK_RE_EXECUTE_STEPS_LIMIT;

	/* read header */
	pc = re_ctx.bytecode;
	re_ctx.re_flags = duk__bc_get_u32(&re_ctx, &pc);
	re_ctx.nsaved = duk__bc_get_u32(&re_ctx, &pc);
	re_ctx.bytecode = pc;

	DUK_ASSERT(DUK_RE_FLAG_GLOBAL < 0x10000UL); /* must fit into duk_small_int_t */
	global = (duk_small_int_t) (force_global | (duk_small_int_t) (re_ctx.re_flags & DUK_RE_FLAG_GLOBAL));

	DUK_ASSERT(re_ctx.nsaved >= 2);
	DUK_ASSERT((re_ctx.nsaved % 2) == 0);

	p_buf = (duk_uint8_t *) duk_push_fixed_buffer(thr, sizeof(duk_uint8_t *) * re_ctx.nsaved); /* rely on zeroing */
	DUK_UNREF(p_buf);
	re_ctx.saved = (const duk_uint8_t **) duk_get_buffer(thr, -1, NULL);
	DUK_ASSERT(re_ctx.saved != NULL);

	/* [ ... re_obj input bc saved_buf ] */

#if defined(DUK_USE_EXPLICIT_NULL_INIT)
	for (i = 0; i < re_ctx.nsaved; i++) {
		re_ctx.saved[i] = (duk_uint8_t *) NULL;
	}
#elif defined(DUK_USE_ZERO_BUFFER_DATA)
	/* buffer is automatically zeroed */
#else
	duk_memzero((void *) p_buf, sizeof(duk_uint8_t *) * re_ctx.nsaved);
#endif

	DUK_DDD(DUK_DDDPRINT("regexp ctx initialized, flags=0x%08lx, nsaved=%ld, recursion_limit=%ld, steps_limit=%ld",
	                     (unsigned long) re_ctx.re_flags,
	                     (long) re_ctx.nsaved,
	                     (long) re_ctx.recursion_limit,
	                     (long) re_ctx.steps_limit));

	/*
	 *  Get starting character offset for match, and initialize 'sp' based on it.
	 *
	 *  Note: lastIndex is non-configurable so it must be present (we check the
	 *  internal class of the object above, so we know it is).  User code can set
	 *  its value to an arbitrary (garbage) value though; E5 requires that lastIndex
	 *  be coerced to a number before using.  The code below works even if the
	 *  property is missing: the value will then be coerced to zero.
	 *
	 *  Note: lastIndex may be outside Uint32 range even after ToInteger() coercion.
	 *  For instance, ToInteger(+Infinity) = +Infinity.  We track the match offset
	 *  as an integer, but pre-check it to be inside the 32-bit range before the loop.
	 *  If not, the check in E5 Section 15.10.6.2, step 9.a applies.
	 */

	/* XXX: lastIndex handling produces a lot of asm */

	/* [ ... re_obj input bc saved_buf ] */

	duk_get_prop_stridx_short(thr, -4, DUK_STRIDX_LAST_INDEX); /* -> [ ... re_obj input bc saved_buf lastIndex ] */
	(void) duk_to_int(thr, -1); /* ToInteger(lastIndex) */
	d = duk_get_number(thr, -1); /* integer, but may be +/- Infinite, +/- zero (not NaN, though) */
	duk_pop_nodecref_unsafe(thr);

	if (global) {
		if (d < 0.0 || d > (double) DUK_HSTRING_GET_CHARLEN(h_input)) {
			/* match fail */
			char_offset = 0; /* not really necessary */
			DUK_ASSERT(match == 0);
			goto match_over;
		}
		char_offset = (duk_uint32_t) d;
	} else {
		/* lastIndex must be ignored for non-global regexps, but get the
		 * value for (theoretical) side effects.  No side effects can
		 * really occur, because lastIndex is a normal property and is
		 * always non-configurable for RegExp instances.
		 */
		char_offset = (duk_uint32_t) 0;
	}

	DUK_ASSERT(char_offset <= DUK_HSTRING_GET_CHARLEN(h_input));
	sp = re_ctx.input + duk_heap_strcache_offset_char2byte(thr, h_input, char_offset);

	/*
	 *  Match loop.
	 *
	 *  Try matching at different offsets until match found or input exhausted.
	 */

	/* [ ... re_obj input bc saved_buf ] */

	DUK_ASSERT(match == 0);

	for (;;) {
		/* char offset in [0, h_input->clen] (both ends inclusive), checked before entry */
		DUK_ASSERT_DISABLE(char_offset >= 0);
		DUK_ASSERT(char_offset <= DUK_HSTRING_GET_CHARLEN(h_input));

		/* Note: re_ctx.steps is intentionally not reset, it applies to the entire unanchored match */
		DUK_ASSERT(re_ctx.recursion_depth == 0);

		DUK_DDD(DUK_DDDPRINT("attempt match at char offset %ld; %p [%p,%p]",
		                     (long) char_offset,
		                     (const void *) sp,
		                     (const void *) re_ctx.input,
		                     (const void *) re_ctx.input_end));

		/*
		 *  Note:
		 *
		 *    - duk__match_regexp() is required not to longjmp() in ordinary "non-match"
		 *      conditions; a longjmp() will terminate the entire matching process.
		 *
		 *    - Clearing saved[] is not necessary because backtracking does it
		 *
		 *    - Backtracking also rewinds re_ctx.recursion back to zero, unless an
		 *      internal/limit error occurs (which causes a longjmp())
		 *
		 *    - If we supported anchored matches, we would break out here
		 *      unconditionally; however, ECMAScript regexps don't have anchored
		 *      matches.  It might make sense to implement a fast bail-out if
		 *      the regexp begins with '^' and sp is not 0: currently we'll just
		 *      run through the entire input string, trivially failing the match
		 *      at every non-zero offset.
		 */

		if (duk__match_regexp(&re_ctx, re_ctx.bytecode, sp) != NULL) {
			DUK_DDD(DUK_DDDPRINT("match at offset %ld", (long) char_offset));
			match = 1;
			break;
		}

		/* advance by one character (code point) and one char_offset */
		char_offset++;
		if (char_offset > DUK_HSTRING_GET_CHARLEN(h_input)) {
			/*
			 *  Note:
			 *
			 *    - Intentionally attempt (empty) match at char_offset == k_input->clen
			 *
			 *    - Negative char_offsets have been eliminated and char_offset is duk_uint32_t
			 *      -> no need or use for a negative check
			 */

			DUK_DDD(DUK_DDDPRINT("no match after trying all sp offsets"));
			break;
		}

		/* avoid calling at end of input, will DUK_ERROR (above check suffices to avoid this) */
		(void) duk__utf8_advance(thr, &sp, re_ctx.input, re_ctx.input_end, (duk_uint_fast32_t) 1);
	}

match_over:

	/*
	 *  Matching complete, create result array or return a 'null'.  Update lastIndex
	 *  if necessary.  See E5 Section 15.10.6.2.
	 *
	 *  Because lastIndex is a character (not byte) offset, we need the character
	 *  length of the match which we conveniently get as a side effect of interning
	 *  the matching substring (0th index of result array).
	 *
	 *  saved[0]         start pointer (~ byte offset) of current match
	 *  saved[1]         end pointer (~ byte offset) of current match (exclusive)
	 *  char_offset      start character offset of current match (-> .index of result)
	 *  char_end_offset  end character offset (computed below)
	 */

	/* [ ... re_obj input bc saved_buf ] */

	if (match) {
#if defined(DUK_USE_ASSERTIONS)
		duk_hobject *h_res;
#endif
		duk_uint32_t char_end_offset = 0;

		DUK_DDD(DUK_DDDPRINT("regexp matches at char_offset %ld", (long) char_offset));

		DUK_ASSERT(re_ctx.nsaved >= 2); /* must have start and end */
		DUK_ASSERT((re_ctx.nsaved % 2) == 0); /* and even number */

		/* XXX: Array size is known before and (2 * re_ctx.nsaved) but not taken
		 * advantage of now.  The array is not compacted either, as regexp match
		 * objects are usually short lived.
		 */

		duk_push_array(thr);

#if defined(DUK_USE_ASSERTIONS)
		h_res = duk_require_hobject(thr, -1);
		DUK_ASSERT(DUK_HOBJECT_HAS_EXTENSIBLE(h_res));
		DUK_ASSERT(DUK_HOBJECT_HAS_EXOTIC_ARRAY(h_res));
		DUK_ASSERT(DUK_HOBJECT_GET_CLASS_NUMBER(h_res) == DUK_HOBJECT_CLASS_ARRAY);
#endif

		/* [ ... re_obj input bc saved_buf res_obj ] */

		duk_push_u32(thr, char_offset);
		duk_xdef_prop_stridx_short_wec(thr, -2, DUK_STRIDX_INDEX);

		duk_dup_m4(thr);
		duk_xdef_prop_stridx_short_wec(thr, -2, DUK_STRIDX_INPUT);

		for (i = 0; i < re_ctx.nsaved; i += 2) {
			/* Captures which are undefined have NULL pointers and are returned
			 * as 'undefined'.  The same is done when saved[] pointers are insane
			 * (this should, of course, never happen in practice).
			 */
			duk_push_uarridx(thr, (duk_uarridx_t) (i / 2));

			if (re_ctx.saved[i] && re_ctx.saved[i + 1] && re_ctx.saved[i + 1] >= re_ctx.saved[i]) {
				duk_push_lstring(thr,
				                 (const char *) re_ctx.saved[i],
				                 (duk_size_t) (re_ctx.saved[i + 1] - re_ctx.saved[i]));
				if (i == 0) {
					/* Assumes that saved[0] and saved[1] are always
					 * set by regexp bytecode (if not, char_end_offset
					 * will be zero).  Also assumes clen reflects the
					 * correct char length.
					 */
					char_end_offset = char_offset + (duk_uint32_t) duk_get_length(thr, -1); /* add charlen */
				}
			} else {
				duk_push_undefined(thr);
			}

			/* [ ... re_obj input bc saved_buf res_obj idx val ] */
			duk_def_prop(thr, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_WEC);
		}

		/* [ ... re_obj input bc saved_buf res_obj ] */

		/* NB: 'length' property is automatically updated by the array setup loop */

		if (global) {
			/* global regexp: lastIndex updated on match */
			duk_push_u32(thr, char_end_offset);
			duk_put_prop_stridx_short(thr, -6, DUK_STRIDX_LAST_INDEX);
		} else {
			/* non-global regexp: lastIndex never updated on match */
			;
		}
	} else {
		/*
		 *  No match, E5 Section 15.10.6.2, step 9.a.i - 9.a.ii apply, regardless
		 *  of 'global' flag of the RegExp.  In particular, if lastIndex is invalid
		 *  initially, it is reset to zero.
		 */

		DUK_DDD(DUK_DDDPRINT("regexp does not match"));

		duk_push_null(thr);

		/* [ ... re_obj input bc saved_buf res_obj ] */

		duk_push_int(thr, 0);
		duk_put_prop_stridx_short(thr, -6, DUK_STRIDX_LAST_INDEX);
	}

	/* [ ... re_obj input bc saved_buf res_obj ] */

	duk_insert(thr, -5);

	/* [ ... res_obj re_obj input bc saved_buf ] */

	duk_pop_n_unsafe(thr, 4);

	/* [ ... res_obj ] */

	/* XXX: these last tricks are unnecessary if the function is made
	 * a genuine native function.
	 */
}

DUK_INTERNAL void duk_regexp_match(duk_hthread *thr) {
	duk__regexp_match_helper(thr, 0 /*force_global*/);
}

/* This variant is needed by String.prototype.split(); it needs to perform
 * global-style matching on a cloned RegExp which is potentially non-global.
 */
DUK_INTERNAL void duk_regexp_match_force_global(duk_hthread *thr) {
	duk__regexp_match_helper(thr, 1 /*force_global*/);
}

#else /* DUK_USE_REGEXP_SUPPORT */

/* regexp support disabled */

#endif /* DUK_USE_REGEXP_SUPPORT */
