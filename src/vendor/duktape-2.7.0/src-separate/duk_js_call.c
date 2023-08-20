/*
 *  Call handling.
 *
 *  duk_handle_call_unprotected():
 *
 *    - Unprotected call to ECMAScript or Duktape/C function, from native
 *      code or bytecode executor.
 *
 *    - Also handles Ecma-to-Ecma calls which reuses a currently running
 *      executor instance to avoid native recursion.  Call setup is done
 *      normally, but just before calling the bytecode executor a special
 *      return code is used to indicate that a calling executor is reused.
 *
 *    - Also handles tailcalls, i.e. reuse of current duk_activation.
 *
 *    - Also handles setup for initial Duktape.Thread.resume().
 *
 *  duk_handle_safe_call():
 *
 *    - Protected C call within current activation.
 *
 *  setjmp() and local variables have a nasty interaction, see execution.rst;
 *  non-volatile locals modified after setjmp() call are not guaranteed to
 *  keep their value and can cause compiler or compiler version specific
 *  difficult to replicate issues.
 *
 *  See 'execution.rst'.
 */

#include "duk_internal.h"

/* XXX: heap->error_not_allowed for success path too? */

/*
 *  Limit check helpers.
 */

/* Check native stack space if DUK_USE_NATIVE_STACK_CHECK() defined. */
DUK_INTERNAL void duk_native_stack_check(duk_hthread *thr) {
#if defined(DUK_USE_NATIVE_STACK_CHECK)
	if (DUK_USE_NATIVE_STACK_CHECK() != 0) {
		DUK_ERROR_RANGE(thr, DUK_STR_NATIVE_STACK_LIMIT);
	}
#else
	DUK_UNREF(thr);
#endif
}

/* Allow headroom for calls during error augmentation (see GH-191).
 * We allow space for 10 additional recursions, with one extra
 * for, e.g. a print() call at the deepest level, and an extra
 * +1 for protected call wrapping.
 */
#define DUK__AUGMENT_CALL_RELAX_COUNT (10 + 2)

/* Stack space required by call handling entry. */
#define DUK__CALL_HANDLING_REQUIRE_STACK 8

DUK_LOCAL DUK_NOINLINE void duk__call_c_recursion_limit_check_slowpath(duk_hthread *thr) {
	/* When augmenting an error, the effective limit is a bit higher.
	 * Check for it only if the fast path check fails.
	 */
#if defined(DUK_USE_AUGMENT_ERROR_THROW) || defined(DUK_USE_AUGMENT_ERROR_CREATE)
	if (thr->heap->augmenting_error) {
		if (thr->heap->call_recursion_depth < thr->heap->call_recursion_limit + DUK__AUGMENT_CALL_RELAX_COUNT) {
			DUK_D(DUK_DPRINT("C recursion limit reached but augmenting error and within relaxed limit"));
			return;
		}
	}
#endif

	DUK_D(DUK_DPRINT("call prevented because C recursion limit reached"));
	DUK_ERROR_RANGE(thr, DUK_STR_NATIVE_STACK_LIMIT);
	DUK_WO_NORETURN(return;);
}

DUK_LOCAL DUK_ALWAYS_INLINE void duk__call_c_recursion_limit_check(duk_hthread *thr) {
	DUK_ASSERT(thr->heap->call_recursion_depth >= 0);
	DUK_ASSERT(thr->heap->call_recursion_depth <= thr->heap->call_recursion_limit);

	duk_native_stack_check(thr);

	/* This check is forcibly inlined because it's very cheap and almost
	 * always passes.  The slow path is forcibly noinline.
	 */
	if (DUK_LIKELY(thr->heap->call_recursion_depth < thr->heap->call_recursion_limit)) {
		return;
	}

	duk__call_c_recursion_limit_check_slowpath(thr);
}

DUK_LOCAL DUK_NOINLINE void duk__call_callstack_limit_check_slowpath(duk_hthread *thr) {
	/* When augmenting an error, the effective limit is a bit higher.
	 * Check for it only if the fast path check fails.
	 */
#if defined(DUK_USE_AUGMENT_ERROR_THROW) || defined(DUK_USE_AUGMENT_ERROR_CREATE)
	if (thr->heap->augmenting_error) {
		if (thr->callstack_top < DUK_USE_CALLSTACK_LIMIT + DUK__AUGMENT_CALL_RELAX_COUNT) {
			DUK_D(DUK_DPRINT("call stack limit reached but augmenting error and within relaxed limit"));
			return;
		}
	}
#endif

	/* XXX: error message is a bit misleading: we reached a recursion
	 * limit which is also essentially the same as a C callstack limit
	 * (except perhaps with some relaxed threading assumptions).
	 */
	DUK_D(DUK_DPRINT("call prevented because call stack limit reached"));
	DUK_ERROR_RANGE(thr, DUK_STR_CALLSTACK_LIMIT);
	DUK_WO_NORETURN(return;);
}

DUK_LOCAL DUK_ALWAYS_INLINE void duk__call_callstack_limit_check(duk_hthread *thr) {
	/* This check is forcibly inlined because it's very cheap and almost
	 * always passes.  The slow path is forcibly noinline.
	 */
	if (DUK_LIKELY(thr->callstack_top < DUK_USE_CALLSTACK_LIMIT)) {
		return;
	}

	duk__call_callstack_limit_check_slowpath(thr);
}

/*
 *  Interrupt counter fixup (for development only).
 */

#if defined(DUK_USE_INTERRUPT_COUNTER) && defined(DUK_USE_DEBUG)
DUK_LOCAL void duk__interrupt_fixup(duk_hthread *thr, duk_hthread *entry_curr_thread) {
	/* Currently the bytecode executor and executor interrupt
	 * instruction counts are off because we don't execute the
	 * interrupt handler when we're about to exit from the initial
	 * user call into Duktape.
	 *
	 * If we were to execute the interrupt handler here, the counts
	 * would match.  You can enable this block manually to check
	 * that this is the case.
	 */

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);

#if defined(DUK_USE_INTERRUPT_DEBUG_FIXUP)
	if (entry_curr_thread == NULL) {
		thr->interrupt_init = thr->interrupt_init - thr->interrupt_counter;
		thr->heap->inst_count_interrupt += thr->interrupt_init;
		DUK_DD(DUK_DDPRINT("debug test: updated interrupt count on exit to "
		                   "user code, instruction counts: executor=%ld, interrupt=%ld",
		                   (long) thr->heap->inst_count_exec,
		                   (long) thr->heap->inst_count_interrupt));
		DUK_ASSERT(thr->heap->inst_count_exec == thr->heap->inst_count_interrupt);
	}
#else
	DUK_UNREF(thr);
	DUK_UNREF(entry_curr_thread);
#endif
}
#endif

/*
 *  Arguments object creation.
 *
 *  Creating arguments objects involves many small details, see E5 Section
 *  10.6 for the specific requirements.  Much of the arguments object exotic
 *  behavior is implemented in duk_hobject_props.c, and is enabled by the
 *  object flag DUK_HOBJECT_FLAG_EXOTIC_ARGUMENTS.
 */

DUK_LOCAL void duk__create_arguments_object(duk_hthread *thr, duk_hobject *func, duk_hobject *varenv, duk_idx_t idx_args) {
	duk_hobject *arg; /* 'arguments' */
	duk_hobject *formals; /* formals for 'func' (may be NULL if func is a C function) */
	duk_idx_t i_arg;
	duk_idx_t i_map;
	duk_idx_t i_mappednames;
	duk_idx_t i_formals;
	duk_idx_t i_argbase;
	duk_idx_t n_formals;
	duk_idx_t idx;
	duk_idx_t num_stack_args;
	duk_bool_t need_map;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(func != NULL);
	DUK_ASSERT(DUK_HOBJECT_IS_NONBOUND_FUNCTION(func));
	DUK_ASSERT(varenv != NULL);

	/* [ ... func this arg1(@idx_args) ... argN envobj ]
	 * [ arg1(@idx_args) ... argN envobj ] (for tailcalls)
	 */

	need_map = 0;

	i_argbase = idx_args;
	num_stack_args = duk_get_top(thr) - i_argbase - 1;
	DUK_ASSERT(i_argbase >= 0);
	DUK_ASSERT(num_stack_args >= 0);

	formals = (duk_hobject *) duk_hobject_get_formals(thr, (duk_hobject *) func);
	if (formals) {
		n_formals = (duk_idx_t) ((duk_harray *) formals)->length;
		duk_push_hobject(thr, formals);
	} else {
		/* This shouldn't happen without tampering of internal
		 * properties: if a function accesses 'arguments', _Formals
		 * is kept.  Check for the case anyway in case internal
		 * properties have been modified manually.
		 */
		DUK_D(DUK_DPRINT("_Formals is undefined when creating arguments, use n_formals == 0"));
		n_formals = 0;
		duk_push_undefined(thr);
	}
	i_formals = duk_require_top_index(thr);

	DUK_ASSERT(n_formals >= 0);
	DUK_ASSERT(formals != NULL || n_formals == 0);

	DUK_DDD(
	    DUK_DDDPRINT("func=%!O, formals=%!O, n_formals=%ld", (duk_heaphdr *) func, (duk_heaphdr *) formals, (long) n_formals));

	/* [ ... formals ] */

	/*
	 *  Create required objects:
	 *    - 'arguments' object: array-like, but not an array
	 *    - 'map' object: internal object, tied to 'arguments' (bare)
	 *    - 'mappedNames' object: temporary value used during construction (bare)
	 */

	arg = duk_push_object_helper(thr,
	                             DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_FASTREFS | DUK_HOBJECT_FLAG_ARRAY_PART |
	                                 DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_ARGUMENTS),
	                             DUK_BIDX_OBJECT_PROTOTYPE);
	DUK_ASSERT(arg != NULL);
	(void) duk_push_object_helper(thr,
	                              DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_FASTREFS |
	                                  DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_OBJECT),
	                              -1); /* no prototype */
	(void) duk_push_object_helper(thr,
	                              DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_FASTREFS |
	                                  DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_OBJECT),
	                              -1); /* no prototype */
	i_arg = duk_get_top(thr) - 3;
	i_map = i_arg + 1;
	i_mappednames = i_arg + 2;
	DUK_ASSERT(!duk_is_bare_object(thr, -3)); /* arguments */
	DUK_ASSERT(duk_is_bare_object(thr, -2)); /* map */
	DUK_ASSERT(duk_is_bare_object(thr, -1)); /* mappedNames */

	/* [ ... formals arguments map mappedNames ] */

	DUK_DDD(DUK_DDDPRINT("created arguments related objects: "
	                     "arguments at index %ld -> %!O "
	                     "map at index %ld -> %!O "
	                     "mappednames at index %ld -> %!O",
	                     (long) i_arg,
	                     (duk_heaphdr *) duk_get_hobject(thr, i_arg),
	                     (long) i_map,
	                     (duk_heaphdr *) duk_get_hobject(thr, i_map),
	                     (long) i_mappednames,
	                     (duk_heaphdr *) duk_get_hobject(thr, i_mappednames)));

	/*
	 *  Init arguments properties, map, etc.
	 */

	duk_push_int(thr, num_stack_args);
	duk_xdef_prop_stridx(thr, i_arg, DUK_STRIDX_LENGTH, DUK_PROPDESC_FLAGS_WC);

	/*
	 *  Init argument related properties.
	 */

	/* step 11 */
	idx = num_stack_args - 1;
	while (idx >= 0) {
		DUK_DDD(
		    DUK_DDDPRINT("arg idx %ld, argbase=%ld, argidx=%ld", (long) idx, (long) i_argbase, (long) (i_argbase + idx)));

		DUK_DDD(DUK_DDDPRINT("define arguments[%ld]=arg", (long) idx));
		duk_dup(thr, i_argbase + idx);
		duk_xdef_prop_index_wec(thr, i_arg, (duk_uarridx_t) idx);
		DUK_DDD(DUK_DDDPRINT("defined arguments[%ld]=arg", (long) idx));

		/* step 11.c is relevant only if non-strict (checked in 11.c.ii) */
		if (!DUK_HOBJECT_HAS_STRICT(func) && idx < n_formals) {
			DUK_ASSERT(formals != NULL);

			DUK_DDD(DUK_DDDPRINT("strict function, index within formals (%ld < %ld)", (long) idx, (long) n_formals));

			duk_get_prop_index(thr, i_formals, (duk_uarridx_t) idx);
			DUK_ASSERT(duk_is_string(thr, -1));

			duk_dup_top(thr); /* [ ... name name ] */

			if (!duk_has_prop(thr, i_mappednames)) {
				/* steps 11.c.ii.1 - 11.c.ii.4, but our internal book-keeping
				 * differs from the reference model
				 */

				/* [ ... name ] */

				need_map = 1;

				DUK_DDD(
				    DUK_DDDPRINT("set mappednames[%s]=%ld", (const char *) duk_get_string(thr, -1), (long) idx));
				duk_dup_top(thr); /* name */
				(void) duk_push_uint_to_hstring(thr, (duk_uint_t) idx); /* index */
				duk_xdef_prop_wec(thr, i_mappednames); /* out of spec, must be configurable */

				DUK_DDD(DUK_DDDPRINT("set map[%ld]=%s", (long) idx, duk_get_string(thr, -1)));
				duk_dup_top(thr); /* name */
				duk_xdef_prop_index_wec(thr, i_map, (duk_uarridx_t) idx); /* out of spec, must be configurable */
			} else {
				/* duk_has_prop() popped the second 'name' */
			}

			/* [ ... name ] */
			duk_pop(thr); /* pop 'name' */
		}

		idx--;
	}

	DUK_DDD(DUK_DDDPRINT("actual arguments processed"));

	/* step 12 */
	if (need_map) {
		DUK_DDD(DUK_DDDPRINT("adding 'map' and 'varenv' to arguments object"));

		/* should never happen for a strict callee */
		DUK_ASSERT(!DUK_HOBJECT_HAS_STRICT(func));

		duk_dup(thr, i_map);
		duk_xdef_prop_stridx(thr, i_arg, DUK_STRIDX_INT_MAP, DUK_PROPDESC_FLAGS_NONE); /* out of spec, don't care */

		/* The variable environment for magic variable bindings needs to be
		 * given by the caller and recorded in the arguments object.
		 *
		 * See E5 Section 10.6, the creation of setters/getters.
		 *
		 * The variable environment also provides access to the callee, so
		 * an explicit (internal) callee property is not needed.
		 */

		duk_push_hobject(thr, varenv);
		duk_xdef_prop_stridx(thr, i_arg, DUK_STRIDX_INT_VARENV, DUK_PROPDESC_FLAGS_NONE); /* out of spec, don't care */
	}

	/* steps 13-14 */
	if (DUK_HOBJECT_HAS_STRICT(func)) {
		/* Callee/caller are throwers and are not deletable etc.  They
		 * could be implemented as virtual properties, but currently
		 * there is no support for virtual properties which are accessors
		 * (only plain virtual properties).  This would not be difficult
		 * to change in duk_hobject_props, but we can make the throwers
		 * normal, concrete properties just as easily.
		 *
		 * Note that the specification requires that the *same* thrower
		 * built-in object is used here!  See E5 Section 10.6 main
		 * algoritm, step 14, and Section 13.2.3 which describes the
		 * thrower.  See test case test-arguments-throwers.js.
		 */

		DUK_DDD(DUK_DDDPRINT("strict function, setting caller/callee to throwers"));

		/* In ES2017 .caller is no longer set at all. */
		duk_xdef_prop_stridx_thrower(thr, i_arg, DUK_STRIDX_CALLEE);
	} else {
		DUK_DDD(DUK_DDDPRINT("non-strict function, setting callee to actual value"));
		duk_push_hobject(thr, func);
		duk_xdef_prop_stridx(thr, i_arg, DUK_STRIDX_CALLEE, DUK_PROPDESC_FLAGS_WC);
	}

	/* set exotic behavior only after we're done */
	if (need_map) {
		/* Exotic behaviors are only enabled for arguments objects
		 * which have a parameter map (see E5 Section 10.6 main
		 * algorithm, step 12).
		 *
		 * In particular, a non-strict arguments object with no
		 * mapped formals does *NOT* get exotic behavior, even
		 * for e.g. "caller" property.  This seems counterintuitive
		 * but seems to be the case.
		 */

		/* cannot be strict (never mapped variables) */
		DUK_ASSERT(!DUK_HOBJECT_HAS_STRICT(func));

		DUK_DDD(DUK_DDDPRINT("enabling exotic behavior for arguments object"));
		DUK_HOBJECT_SET_EXOTIC_ARGUMENTS(arg);
	} else {
		DUK_DDD(DUK_DDDPRINT("not enabling exotic behavior for arguments object"));
	}

	DUK_DDD(DUK_DDDPRINT("final arguments related objects: "
	                     "arguments at index %ld -> %!O "
	                     "map at index %ld -> %!O "
	                     "mappednames at index %ld -> %!O",
	                     (long) i_arg,
	                     (duk_heaphdr *) duk_get_hobject(thr, i_arg),
	                     (long) i_map,
	                     (duk_heaphdr *) duk_get_hobject(thr, i_map),
	                     (long) i_mappednames,
	                     (duk_heaphdr *) duk_get_hobject(thr, i_mappednames)));

	/* [ args(n) envobj formals arguments map mappednames ] */

	duk_pop_2(thr);
	duk_remove_m2(thr);

	/* [ args(n) envobj arguments ] */
}

/* Helper for creating the arguments object and adding it to the env record
 * on top of the value stack.
 */
DUK_LOCAL void duk__handle_createargs_for_call(duk_hthread *thr, duk_hobject *func, duk_hobject *env, duk_idx_t idx_args) {
	DUK_DDD(DUK_DDDPRINT("creating arguments object for function call"));

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(func != NULL);
	DUK_ASSERT(env != NULL);
	DUK_ASSERT(DUK_HOBJECT_HAS_CREATEARGS(func));

	/* [ ... arg1 ... argN envobj ] */

	duk__create_arguments_object(thr, func, env, idx_args);

	/* [ ... arg1 ... argN envobj argobj ] */

	duk_xdef_prop_stridx_short(thr,
	                           -2,
	                           DUK_STRIDX_LC_ARGUMENTS,
	                           DUK_HOBJECT_HAS_STRICT(func) ? DUK_PROPDESC_FLAGS_E : /* strict: non-deletable, non-writable */
                                                                  DUK_PROPDESC_FLAGS_WE); /* non-strict: non-deletable, writable */
	/* [ ... arg1 ... argN envobj ] */
}

/*
 *  Helpers for constructor call handling.
 *
 *  There are two [[Construct]] operations in the specification:
 *
 *    - E5 Section 13.2.2: for Function objects
 *    - E5 Section 15.3.4.5.2: for "bound" Function objects
 *
 *  The chain of bound functions is resolved in Section 15.3.4.5.2,
 *  with arguments "piling up" until the [[Construct]] internal
 *  method is called on the final, actual Function object.  Note
 *  that the "prototype" property is looked up *only* from the
 *  final object, *before* calling the constructor.
 *
 *  Since Duktape 2.2 bound functions are represented with the
 *  duk_hboundfunc internal type, and bound function chains are
 *  collapsed when a bound function is created.  As a result, the
 *  direct target of a duk_hboundfunc is always non-bound and the
 *  this/argument lists have been resolved.
 *
 *  When constructing new Array instances, an unnecessary object is
 *  created and discarded now: the standard [[Construct]] creates an
 *  object, and calls the Array constructor.  The Array constructor
 *  returns an Array instance, which is used as the result value for
 *  the "new" operation; the object created before the Array constructor
 *  call is discarded.
 *
 *  This would be easy to fix, e.g. by knowing that the Array constructor
 *  will always create a replacement object and skip creating the fallback
 *  object in that case.
 */

/* Update default instance prototype for constructor call. */
DUK_LOCAL void duk__update_default_instance_proto(duk_hthread *thr, duk_idx_t idx_func) {
	duk_hobject *proto;
	duk_hobject *fallback;

	DUK_ASSERT(duk_is_constructable(thr, idx_func));

	duk_get_prop_stridx_short(thr, idx_func, DUK_STRIDX_PROTOTYPE);
	proto = duk_get_hobject(thr, -1);
	if (proto == NULL) {
		DUK_DDD(DUK_DDDPRINT("constructor has no 'prototype' property, or value not an object "
		                     "-> leave standard Object prototype as fallback prototype"));
	} else {
		DUK_DDD(DUK_DDDPRINT("constructor has 'prototype' property with object value "
		                     "-> set fallback prototype to that value: %!iO",
		                     (duk_heaphdr *) proto));
		/* Original fallback (default instance) is untouched when
		 * resolving bound functions etc.
		 */
		fallback = duk_known_hobject(thr, idx_func + 1);
		DUK_ASSERT(fallback != NULL);
		DUK_HOBJECT_SET_PROTOTYPE_UPDREF(thr, fallback, proto);
	}
	duk_pop(thr);
}

/* Postprocess: return value special handling, error augmentation. */
DUK_INTERNAL void duk_call_construct_postprocess(duk_hthread *thr, duk_small_uint_t proxy_invariant) {
	/* Use either fallback (default instance) or retval depending
	 * on retval type.  Needs to be called before unwind because
	 * the default instance is read from the current (immutable)
	 * 'this' binding.
	 *
	 * For Proxy 'construct' calls the return value must be an
	 * Object (we accept object-like values like buffers and
	 * lightfuncs too).  If not, TypeError.
	 */
	if (duk_check_type_mask(thr, -1, DUK_TYPE_MASK_OBJECT | DUK_TYPE_MASK_BUFFER | DUK_TYPE_MASK_LIGHTFUNC)) {
		DUK_DDD(DUK_DDDPRINT("replacement value"));
	} else {
		if (DUK_UNLIKELY(proxy_invariant != 0U)) {
			/* Proxy 'construct' return value invariant violated. */
			DUK_ERROR_TYPE_INVALID_TRAP_RESULT(thr);
			DUK_WO_NORETURN(return;);
		}
		/* XXX: direct value stack access */
		duk_pop(thr);
		duk_push_this(thr);
	}

#if defined(DUK_USE_AUGMENT_ERROR_CREATE)
	/* Augment created errors upon creation, not when they are thrown or
	 * rethrown.  __FILE__ and __LINE__ are not desirable here; the call
	 * stack reflects the caller which is correct.  Skip topmost, unwound
	 * activation when creating a traceback.  If thr->ptr_curr_pc was !=
	 * NULL we'd need to sync the current PC so that the traceback comes
	 * out right; however it is always synced here so just assert for it.
	 */
	DUK_ASSERT(thr->ptr_curr_pc == NULL);
	duk_err_augment_error_create(thr, thr, NULL, 0, DUK_AUGMENT_FLAG_NOBLAME_FILELINE | DUK_AUGMENT_FLAG_SKIP_ONE);
#endif
}

/*
 *  Helper for handling a bound function when a call is being made.
 *
 *  Assumes that bound function chains have been "collapsed" so that either
 *  the target is non-bound or there is one bound function that points to a
 *  nonbound target.
 *
 *  Prepends the bound arguments to the value stack (at idx_func + 2).
 *  The 'this' binding is also updated if necessary (at idx_func + 1).
 *  Note that for constructor calls the 'this' binding is never updated by
 *  [[BoundThis]].
 */

DUK_LOCAL void duk__handle_bound_chain_for_call(duk_hthread *thr, duk_idx_t idx_func, duk_bool_t is_constructor_call) {
	duk_tval *tv_func;
	duk_hobject *func;
	duk_idx_t len;

	DUK_ASSERT(thr != NULL);

	/* On entry, item at idx_func is a bound, non-lightweight function,
	 * but we don't rely on that below.
	 */

	DUK_ASSERT(duk_get_top(thr) >= idx_func + 2);

	tv_func = duk_require_tval(thr, idx_func);
	DUK_ASSERT(tv_func != NULL);

	if (DUK_TVAL_IS_OBJECT(tv_func)) {
		func = DUK_TVAL_GET_OBJECT(tv_func);

		/* XXX: separate helper function, out of fast path? */
		if (DUK_HOBJECT_HAS_BOUNDFUNC(func)) {
			duk_hboundfunc *h_bound;
			duk_tval *tv_args;
			duk_tval *tv_gap;

			h_bound = (duk_hboundfunc *) (void *) func;
			tv_args = h_bound->args;
			len = h_bound->nargs;
			DUK_ASSERT(len == 0 || tv_args != NULL);

			DUK_DDD(DUK_DDDPRINT("bound function encountered, ptr=%p: %!T",
			                     (void *) DUK_TVAL_GET_OBJECT(tv_func),
			                     tv_func));

			/* [ ... func this arg1 ... argN ] */

			if (is_constructor_call) {
				/* See: tests/ecmascript/test-spec-bound-constructor.js */
				DUK_DDD(DUK_DDDPRINT("constructor call: don't update this binding"));
			} else {
				/* XXX: duk_replace_tval */
				duk_push_tval(thr, &h_bound->this_binding);
				duk_replace(thr, idx_func + 1); /* idx_this = idx_func + 1 */
			}

			/* [ ... func this arg1 ... argN ] */

			duk_require_stack(thr, len);

			tv_gap = duk_reserve_gap(thr, idx_func + 2, len);
			duk_copy_tvals_incref(thr, tv_gap, tv_args, (duk_size_t) len);

			/* [ ... func this <bound args> arg1 ... argN ] */

			duk_push_tval(thr, &h_bound->target);
			duk_replace(thr, idx_func); /* replace in stack */

			DUK_DDD(DUK_DDDPRINT("bound function handled, idx_func=%ld, curr func=%!T",
			                     (long) idx_func,
			                     duk_get_tval(thr, idx_func)));
		}
	} else if (DUK_TVAL_IS_LIGHTFUNC(tv_func)) {
		/* Lightweight function: never bound, so terminate. */
		;
	} else {
		/* Shouldn't happen, so ugly error is enough. */
		DUK_ERROR_INTERNAL(thr);
		DUK_WO_NORETURN(return;);
	}

	DUK_ASSERT(duk_get_top(thr) >= idx_func + 2);

	DUK_DDD(DUK_DDDPRINT("final non-bound function is: %!T", duk_get_tval(thr, idx_func)));

#if defined(DUK_USE_ASSERTIONS)
	tv_func = duk_require_tval(thr, idx_func);
	DUK_ASSERT(DUK_TVAL_IS_LIGHTFUNC(tv_func) || DUK_TVAL_IS_OBJECT(tv_func));
	if (DUK_TVAL_IS_OBJECT(tv_func)) {
		func = DUK_TVAL_GET_OBJECT(tv_func);
		DUK_ASSERT(func != NULL);
		DUK_ASSERT(!DUK_HOBJECT_HAS_BOUNDFUNC(func));
		DUK_ASSERT(DUK_HOBJECT_HAS_COMPFUNC(func) || DUK_HOBJECT_HAS_NATFUNC(func) || DUK_HOBJECT_IS_PROXY(func));
	}
#endif
}

/*
 *  Helper for inline handling of .call(), .apply(), and .construct().
 */

DUK_LOCAL duk_bool_t duk__handle_specialfuncs_for_call(duk_hthread *thr,
                                                       duk_idx_t idx_func,
                                                       duk_hobject *func,
                                                       duk_small_uint_t *call_flags,
                                                       duk_bool_t first) {
#if defined(DUK_USE_ASSERTIONS)
	duk_c_function natfunc;
#endif
	duk_tval *tv_args;

	DUK_ASSERT(func != NULL);
	DUK_ASSERT((*call_flags & DUK_CALL_FLAG_CONSTRUCT) == 0); /* Caller. */

#if defined(DUK_USE_ASSERTIONS)
	natfunc = ((duk_hnatfunc *) func)->func;
	DUK_ASSERT(natfunc != NULL);
#endif

	/* On every round of function resolution at least target function and
	 * 'this' binding are set.  We can assume that here, and must guarantee
	 * it on exit.  Value stack reserve is extended for bound function and
	 * .apply() unpacking so we don't need to extend it here when we need a
	 * few slots.
	 */
	DUK_ASSERT(duk_get_top(thr) >= idx_func + 2);

	/* Handle native 'eval' specially.  A direct eval check is only made
	 * for the first resolution attempt; e.g. a bound eval call is -not-
	 * a direct eval call.
	 */
	if (DUK_UNLIKELY(((duk_hnatfunc *) func)->magic == 15)) {
		/* For now no special handling except for direct eval
		 * detection.
		 */
		DUK_ASSERT(((duk_hnatfunc *) func)->func == duk_bi_global_object_eval);
		if (first && (*call_flags & DUK_CALL_FLAG_CALLED_AS_EVAL)) {
			*call_flags = (*call_flags & ~DUK_CALL_FLAG_CALLED_AS_EVAL) | DUK_CALL_FLAG_DIRECT_EVAL;
		}
		DUK_ASSERT(duk_get_top(thr) >= idx_func + 2);
		return 1; /* stop resolving */
	}

	/* Handle special functions based on the DUK_HOBJECT_FLAG_SPECIAL_CALL
	 * flag; their magic value is used for switch-case.
	 *
	 * NOTE: duk_unpack_array_like() reserves value stack space
	 * for the result values (unlike most other value stack calls).
	 */
	switch (((duk_hnatfunc *) func)->magic) {
	case 0: { /* 0=Function.prototype.call() */
		/* Value stack:
		 * idx_func + 0: Function.prototype.call()  [removed]
		 * idx_func + 1: this binding for .call (target function)
		 * idx_func + 2: 1st argument to .call, desired 'this' binding
		 * idx_func + 3: 2nd argument to .call, desired 1st argument for ultimate target
		 * ...
		 *
		 * Remove idx_func + 0 to get:
		 * idx_func + 0: target function
		 * idx_func + 1: this binding
		 * idx_func + 2: call arguments
		 * ...
		 */
		DUK_ASSERT(natfunc == duk_bi_function_prototype_call);
		duk_remove_unsafe(thr, idx_func);
		tv_args = thr->valstack_bottom + idx_func + 2;
		if (thr->valstack_top < tv_args) {
			DUK_ASSERT(tv_args <= thr->valstack_end);
			thr->valstack_top = tv_args; /* at least target function and 'this' binding present */
		}
		break;
	}
	case 1: { /* 1=Function.prototype.apply() */
		/* Value stack:
		 * idx_func + 0: Function.prototype.apply()  [removed]
		 * idx_func + 1: this binding for .apply (target function)
		 * idx_func + 2: 1st argument to .apply, desired 'this' binding
		 * idx_func + 3: 2nd argument to .apply, argArray
		 * [anything after this MUST be ignored]
		 *
		 * Remove idx_func + 0 and unpack the argArray to get:
		 * idx_func + 0: target function
		 * idx_func + 1: this binding
		 * idx_func + 2: call arguments
		 * ...
		 */
		DUK_ASSERT(natfunc == duk_bi_function_prototype_apply);
		duk_remove_unsafe(thr, idx_func);
		goto apply_shared;
	}
#if defined(DUK_USE_REFLECT_BUILTIN)
	case 2: { /* 2=Reflect.apply() */
		/* Value stack:
		 * idx_func + 0: Reflect.apply()  [removed]
		 * idx_func + 1: this binding for .apply (ignored, usually Reflect)  [removed]
		 * idx_func + 2: 1st argument to .apply, target function
		 * idx_func + 3: 2nd argument to .apply, desired 'this' binding
		 * idx_func + 4: 3rd argument to .apply, argArray
		 * [anything after this MUST be ignored]
		 *
		 * Remove idx_func + 0 and idx_func + 1, and unpack the argArray to get:
		 * idx_func + 0: target function
		 * idx_func + 1: this binding
		 * idx_func + 2: call arguments
		 * ...
		 */
		DUK_ASSERT(natfunc == duk_bi_reflect_apply);
		duk_remove_n_unsafe(thr, idx_func, 2);
		goto apply_shared;
	}
	case 3: { /* 3=Reflect.construct() */
		/* Value stack:
		 * idx_func + 0: Reflect.construct()  [removed]
		 * idx_func + 1: this binding for .construct (ignored, usually Reflect)  [removed]
		 * idx_func + 2: 1st argument to .construct, target function
		 * idx_func + 3: 2nd argument to .construct, argArray
		 * idx_func + 4: 3rd argument to .construct, newTarget
		 * [anything after this MUST be ignored]
		 *
		 * Remove idx_func + 0 and idx_func + 1, unpack the argArray,
		 * and insert default instance (prototype not yet updated), to get:
		 * idx_func + 0: target function
		 * idx_func + 1: this binding (default instance)
		 * idx_func + 2: constructor call arguments
		 * ...
		 *
		 * Call flags must be updated to reflect the fact that we're
		 * now dealing with a constructor call, and e.g. the 'this'
		 * binding cannot be overwritten if the target is bound.
		 *
		 * newTarget is checked but not yet passed onwards.
		 */

		duk_idx_t top;

		DUK_ASSERT(natfunc == duk_bi_reflect_construct);
		*call_flags |= DUK_CALL_FLAG_CONSTRUCT;
		duk_remove_n_unsafe(thr, idx_func, 2);
		top = duk_get_top(thr);
		if (!duk_is_constructable(thr, idx_func)) {
			/* Target constructability must be checked before
			 * unpacking argArray (which may cause side effects).
			 * Just return; caller will throw the error.
			 */
			duk_set_top_unsafe(thr, idx_func + 2); /* satisfy asserts */
			break;
		}
		duk_push_object(thr);
		duk_insert(thr, idx_func + 1); /* default instance */

		/* [ ... func default_instance argArray newTarget? ] */

		top = duk_get_top(thr);
		if (top < idx_func + 3) {
			/* argArray is a mandatory argument for Reflect.construct(). */
			DUK_ERROR_TYPE_INVALID_ARGS(thr);
			DUK_WO_NORETURN(return 0;);
		}
		if (top > idx_func + 3) {
			if (!duk_strict_equals(thr, idx_func, idx_func + 3)) {
				/* XXX: [[Construct]] newTarget currently unsupported */
				DUK_ERROR_UNSUPPORTED(thr);
				DUK_WO_NORETURN(return 0;);
			}
			duk_set_top_unsafe(thr, idx_func + 3); /* remove any args beyond argArray */
		}
		DUK_ASSERT(duk_get_top(thr) == idx_func + 3);
		DUK_ASSERT(duk_is_valid_index(thr, idx_func + 2));
		(void) duk_unpack_array_like(thr,
		                             idx_func + 2); /* XXX: should also remove target to be symmetric with duk_pack()? */
		duk_remove(thr, idx_func + 2);
		DUK_ASSERT(duk_get_top(thr) >= idx_func + 2);
		break;
	}
#endif /* DUK_USE_REFLECT_BUILTIN */
	default: {
		DUK_ASSERT(0);
		DUK_UNREACHABLE();
	}
	}

	DUK_ASSERT(duk_get_top(thr) >= idx_func + 2);
	return 0; /* keep resolving */

apply_shared:
	tv_args = thr->valstack_bottom + idx_func + 2;
	if (thr->valstack_top <= tv_args) {
		DUK_ASSERT(tv_args <= thr->valstack_end);
		thr->valstack_top = tv_args; /* at least target func and 'this' binding present */
		/* No need to check for argArray. */
	} else {
		DUK_ASSERT(duk_get_top(thr) >= idx_func + 3); /* idx_func + 2 covered above */
		if (thr->valstack_top > tv_args + 1) {
			duk_set_top_unsafe(thr, idx_func + 3); /* remove any args beyond argArray */
		}
		DUK_ASSERT(duk_is_valid_index(thr, idx_func + 2));
		if (!duk_is_callable(thr, idx_func)) {
			/* Avoid unpack side effects if the target isn't callable.
			 * Calling code will throw the actual error.
			 */
		} else {
			(void) duk_unpack_array_like(thr, idx_func + 2);
			duk_remove(thr, idx_func + 2);
		}
	}
	DUK_ASSERT(duk_get_top(thr) >= idx_func + 2);
	return 0; /* keep resolving */
}

/*
 *  Helper for Proxy handling.
 */

#if defined(DUK_USE_ES6_PROXY)
DUK_LOCAL void duk__handle_proxy_for_call(duk_hthread *thr, duk_idx_t idx_func, duk_hproxy *h_proxy, duk_small_uint_t *call_flags) {
	duk_bool_t rc;

	/* Value stack:
	 * idx_func + 0: Proxy object
	 * idx_func + 1: this binding for call
	 * idx_func + 2: 1st argument for call
	 * idx_func + 3: 2nd argument for call
	 * ...
	 *
	 * If Proxy doesn't have a trap for the call ('apply' or 'construct'),
	 * replace Proxy object with target object.
	 *
	 * If we're dealing with a normal call and the Proxy has an 'apply'
	 * trap, manipulate value stack to:
	 *
	 * idx_func + 0: trap
	 * idx_func + 1: Proxy's handler
	 * idx_func + 2: Proxy's target
	 * idx_func + 3: this binding for call (from idx_func + 1)
	 * idx_func + 4: call arguments packed to an array
	 *
	 * If we're dealing with a constructor call and the Proxy has a
	 * 'construct' trap, manipulate value stack to:
	 *
	 * idx_func + 0: trap
	 * idx_func + 1: Proxy's handler
	 * idx_func + 2: Proxy's target
	 * idx_func + 3: call arguments packed to an array
	 * idx_func + 4: newTarget == Proxy object here
	 *
	 * As we don't yet have proper newTarget support, the newTarget at
	 * idx_func + 3 is just the original constructor being called, i.e.
	 * the Proxy object (not the target).  Note that the default instance
	 * (original 'this' binding) is dropped and ignored.
	 */

	duk_push_hobject(thr, h_proxy->handler);
	rc = duk_get_prop_stridx_short(thr, -1, (*call_flags & DUK_CALL_FLAG_CONSTRUCT) ? DUK_STRIDX_CONSTRUCT : DUK_STRIDX_APPLY);
	if (rc == 0) {
		/* Not found, continue to target.  If this is a construct
		 * call, update default instance prototype using the Proxy,
		 * not the target.
		 */
		if (*call_flags & DUK_CALL_FLAG_CONSTRUCT) {
			if (!(*call_flags & DUK_CALL_FLAG_DEFAULT_INSTANCE_UPDATED)) {
				*call_flags |= DUK_CALL_FLAG_DEFAULT_INSTANCE_UPDATED;
				duk__update_default_instance_proto(thr, idx_func);
			}
		}
		duk_pop_2(thr);
		duk_push_hobject(thr, h_proxy->target);
		duk_replace(thr, idx_func);
		return;
	}

	/* Here we must be careful not to replace idx_func while
	 * h_proxy is still needed, otherwise h_proxy may become
	 * dangling.  This could be improved e.g. using a
	 * duk_pack_slice() with a freeform slice.
	 */

	/* Here:
	 * idx_func + 0: Proxy object
	 * idx_func + 1: this binding for call
	 * idx_func + 2: 1st argument for call
	 * idx_func + 3: 2nd argument for call
	 * ...
	 * idx_func + N: handler
	 * idx_func + N + 1: trap
	 */

	duk_insert(thr, idx_func + 1);
	duk_insert(thr, idx_func + 2);
	duk_push_hobject(thr, h_proxy->target);
	duk_insert(thr, idx_func + 3);
	duk_pack(thr, duk_get_top(thr) - (idx_func + 5));
	DUK_ASSERT(!duk_is_bare_object(thr, -1));

	/* Here:
	 * idx_func + 0: Proxy object
	 * idx_func + 1: trap
	 * idx_func + 2: Proxy's handler
	 * idx_func + 3: Proxy's target
	 * idx_func + 4: this binding for call
	 * idx_func + 5: arguments array
	 */
	DUK_ASSERT(duk_get_top(thr) == idx_func + 6);

	if (*call_flags & DUK_CALL_FLAG_CONSTRUCT) {
		*call_flags |= DUK_CALL_FLAG_CONSTRUCT_PROXY; /* Enable 'construct' trap return invariant check. */
		*call_flags &= ~(DUK_CALL_FLAG_CONSTRUCT); /* Resume as non-constructor call to the trap. */

		/* 'apply' args: target, thisArg, argArray
		 * 'construct' args: target, argArray, newTarget
		 */
		duk_remove(thr, idx_func + 4);
		duk_push_hobject(thr, (duk_hobject *) h_proxy);
	}

	/* Finalize value stack layout by removing Proxy reference. */
	duk_remove(thr, idx_func);
	h_proxy = NULL; /* invalidated */
	DUK_ASSERT(duk_get_top(thr) == idx_func + 5);
}
#endif /* DUK_USE_ES6_PROXY */

/*
 *  Helper for setting up var_env and lex_env of an activation,
 *  assuming it does NOT have the DUK_HOBJECT_FLAG_NEWENV flag.
 */

DUK_LOCAL void duk__handle_oldenv_for_call(duk_hthread *thr, duk_hobject *func, duk_activation *act) {
	duk_hcompfunc *f;
	duk_hobject *h_lex;
	duk_hobject *h_var;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(func != NULL);
	DUK_ASSERT(act != NULL);
	DUK_ASSERT(!DUK_HOBJECT_HAS_NEWENV(func));
	DUK_ASSERT(!DUK_HOBJECT_HAS_CREATEARGS(func));
	DUK_ASSERT(DUK_HOBJECT_IS_COMPFUNC(func));
	DUK_UNREF(thr);

	f = (duk_hcompfunc *) func;
	h_lex = DUK_HCOMPFUNC_GET_LEXENV(thr->heap, f);
	h_var = DUK_HCOMPFUNC_GET_VARENV(thr->heap, f);
	DUK_ASSERT(h_lex != NULL); /* Always true for closures (not for templates) */
	DUK_ASSERT(h_var != NULL);
	act->lex_env = h_lex;
	act->var_env = h_var;
	DUK_HOBJECT_INCREF(thr, h_lex);
	DUK_HOBJECT_INCREF(thr, h_var);
}

/*
 *  Helper for updating callee 'caller' property.
 */

#if defined(DUK_USE_NONSTD_FUNC_CALLER_PROPERTY)
DUK_LOCAL void duk__update_func_caller_prop(duk_hthread *thr, duk_hobject *func) {
	duk_tval *tv_caller;
	duk_hobject *h_tmp;
	duk_activation *act_callee;
	duk_activation *act_caller;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(func != NULL);
	DUK_ASSERT(!DUK_HOBJECT_HAS_BOUNDFUNC(func)); /* bound chain resolved */
	DUK_ASSERT(thr->callstack_top >= 1);

	if (DUK_HOBJECT_HAS_STRICT(func)) {
		/* Strict functions don't get their 'caller' updated. */
		return;
	}

	DUK_ASSERT(thr->callstack_top > 0);
	act_callee = thr->callstack_curr;
	DUK_ASSERT(act_callee != NULL);
	act_caller = (thr->callstack_top >= 2 ? act_callee->parent : NULL);

	/* XXX: check .caller writability? */

	/* Backup 'caller' property and update its value. */
	tv_caller = duk_hobject_find_entry_tval_ptr_stridx(thr->heap, func, DUK_STRIDX_CALLER);
	if (tv_caller) {
		/* If caller is global/eval code, 'caller' should be set to
		 * 'null'.
		 *
		 * XXX: there is no exotic flag to infer this correctly now.
		 * The NEWENV flag is used now which works as intended for
		 * everything (global code, non-strict eval code, and functions)
		 * except strict eval code.  Bound functions are never an issue
		 * because 'func' has been resolved to a non-bound function.
		 */

		if (act_caller != NULL) {
			/* act_caller->func may be NULL in some finalization cases,
			 * just treat like we don't know the caller.
			 */
			if (act_caller->func && !DUK_HOBJECT_HAS_NEWENV(act_caller->func)) {
				/* Setting to NULL causes 'caller' to be set to
				 * 'null' as desired.
				 */
				act_caller = NULL;
			}
		}

		if (DUK_TVAL_IS_OBJECT(tv_caller)) {
			h_tmp = DUK_TVAL_GET_OBJECT(tv_caller);
			DUK_ASSERT(h_tmp != NULL);
			act_callee->prev_caller = h_tmp;

			/* Previous value doesn't need refcount changes because its ownership
			 * is transferred to prev_caller.
			 */

			if (act_caller != NULL) {
				DUK_ASSERT(act_caller->func != NULL);
				DUK_TVAL_SET_OBJECT(tv_caller, act_caller->func);
				DUK_TVAL_INCREF(thr, tv_caller);
			} else {
				DUK_TVAL_SET_NULL(tv_caller); /* no incref */
			}
		} else {
			/* 'caller' must only take on 'null' or function value */
			DUK_ASSERT(!DUK_TVAL_IS_HEAP_ALLOCATED(tv_caller));
			DUK_ASSERT(act_callee->prev_caller == NULL);
			if (act_caller != NULL && act_caller->func) {
				/* Tolerate act_caller->func == NULL which happens in
				 * some finalization cases; treat like unknown caller.
				 */
				DUK_TVAL_SET_OBJECT(tv_caller, act_caller->func);
				DUK_TVAL_INCREF(thr, tv_caller);
			} else {
				DUK_TVAL_SET_NULL(tv_caller); /* no incref */
			}
		}
	}
}
#endif /* DUK_USE_NONSTD_FUNC_CALLER_PROPERTY */

/*
 *  Shared helpers for resolving the final, non-bound target function of the
 *  call and the effective 'this' binding.  Resolves bound functions and
 *  applies .call(), .apply(), and .construct() inline.
 *
 *  Proxy traps are also handled inline so that if the target is a Proxy with
 *  a 'call' or 'construct' trap, the trap handler is called with a modified
 *  argument list.
 *
 *  Once the bound function / .call() / .apply() / .construct() sequence has
 *  been resolved, the value at idx_func + 1 may need coercion described in
 *  E5 Section 10.4.3.
 *
 *  A call that begins as a non-constructor call may be converted into a
 *  constructor call during the resolution process if Reflect.construct()
 *  is invoked.  This is handled by updating the caller's call_flags.
 *
 *  For global and eval code (E5 Sections 10.4.1 and 10.4.2), we assume
 *  that the caller has provided the correct 'this' binding explicitly
 *  when calling, i.e.:
 *
 *    - global code: this=global object
 *    - direct eval: this=copy from eval() caller's this binding
 *    - other eval:  this=global object
 *
 *  The 'this' coercion may cause a recursive function call with arbitrary
 *  side effects, because ToObject() may be called.
 */

DUK_LOCAL DUK_INLINE void duk__coerce_nonstrict_this_binding(duk_hthread *thr, duk_idx_t idx_this) {
	duk_tval *tv_this;
	duk_hobject *obj_global;

	tv_this = thr->valstack_bottom + idx_this;
	switch (DUK_TVAL_GET_TAG(tv_this)) {
	case DUK_TAG_OBJECT:
		DUK_DDD(DUK_DDDPRINT("this binding: non-strict, object -> use directly"));
		break;
	case DUK_TAG_UNDEFINED:
	case DUK_TAG_NULL:
		DUK_DDD(DUK_DDDPRINT("this binding: non-strict, undefined/null -> use global object"));
		obj_global = thr->builtins[DUK_BIDX_GLOBAL];
		/* XXX: avoid this check somehow */
		if (DUK_LIKELY(obj_global != NULL)) {
			DUK_ASSERT(!DUK_TVAL_IS_HEAP_ALLOCATED(tv_this)); /* no need to decref previous value */
			DUK_TVAL_SET_OBJECT(tv_this, obj_global);
			DUK_HOBJECT_INCREF(thr, obj_global);
		} else {
			/* This may only happen if built-ins are being "torn down".
			 * This behavior is out of specification scope.
			 */
			DUK_D(DUK_DPRINT("this binding: wanted to use global object, but it is NULL -> using undefined instead"));
			DUK_ASSERT(!DUK_TVAL_IS_HEAP_ALLOCATED(tv_this)); /* no need to decref previous value */
			DUK_TVAL_SET_UNDEFINED(tv_this); /* nothing to incref */
		}
		break;
	default:
		/* Plain buffers and lightfuncs are object coerced.  Lightfuncs
		 * very rarely come here however, because the call target would
		 * need to be a non-strict non-lightfunc (lightfuncs are considered
		 * strict) with an explicit lightfunc 'this' binding.
		 */
		DUK_ASSERT(!DUK_TVAL_IS_UNUSED(tv_this));
		DUK_DDD(DUK_DDDPRINT("this binding: non-strict, not object/undefined/null -> use ToObject(value)"));
		duk_to_object(thr, idx_this); /* may have side effects */
		break;
	}
}

DUK_LOCAL DUK_ALWAYS_INLINE duk_bool_t duk__resolve_target_fastpath_check(duk_hthread *thr,
                                                                          duk_idx_t idx_func,
                                                                          duk_hobject **out_func,
                                                                          duk_small_uint_t call_flags) {
#if defined(DUK_USE_PREFER_SIZE)
	DUK_UNREF(thr);
	DUK_UNREF(idx_func);
	DUK_UNREF(out_func);
	DUK_UNREF(call_flags);
#else /* DUK_USE_PREFER_SIZE */
	duk_tval *tv_func;
	duk_hobject *func;

	if (DUK_UNLIKELY(call_flags & DUK_CALL_FLAG_CONSTRUCT)) {
		return 0;
	}

	tv_func = DUK_GET_TVAL_POSIDX(thr, idx_func);
	DUK_ASSERT(tv_func != NULL);

	if (DUK_LIKELY(DUK_TVAL_IS_OBJECT(tv_func))) {
		func = DUK_TVAL_GET_OBJECT(tv_func);
		if (DUK_HOBJECT_IS_CALLABLE(func) && !DUK_HOBJECT_HAS_BOUNDFUNC(func) && !DUK_HOBJECT_HAS_SPECIAL_CALL(func)) {
			*out_func = func;

			if (DUK_HOBJECT_HAS_STRICT(func)) {
				/* Strict function: no 'this' coercion. */
				return 1;
			}

			duk__coerce_nonstrict_this_binding(thr, idx_func + 1);
			return 1;
		}
	} else if (DUK_TVAL_IS_LIGHTFUNC(tv_func)) {
		*out_func = NULL;

		/* Lightfuncs are considered strict, so 'this' binding is
		 * used as is.  They're never bound, always constructable,
		 * and never special functions.
		 */
		return 1;
	}
#endif /* DUK_USE_PREFER_SIZE */
	return 0; /* let slow path deal with it */
}

DUK_LOCAL duk_hobject *duk__resolve_target_func_and_this_binding(duk_hthread *thr,
                                                                 duk_idx_t idx_func,
                                                                 duk_small_uint_t *call_flags) {
	duk_tval *tv_func;
	duk_hobject *func;
	duk_bool_t first;

	DUK_ASSERT(duk_get_top(thr) >= idx_func + 2);

	for (first = 1;; first = 0) {
		DUK_ASSERT(duk_get_top(thr) >= idx_func + 2);

		tv_func = DUK_GET_TVAL_POSIDX(thr, idx_func);
		DUK_ASSERT(tv_func != NULL);

		DUK_DD(DUK_DDPRINT("target func: %!iT", tv_func));

		if (DUK_TVAL_IS_OBJECT(tv_func)) {
			func = DUK_TVAL_GET_OBJECT(tv_func);

			if (*call_flags & DUK_CALL_FLAG_CONSTRUCT) {
				if (DUK_UNLIKELY(!DUK_HOBJECT_HAS_CONSTRUCTABLE(func))) {
					goto not_constructable;
				}
			} else {
				if (DUK_UNLIKELY(!DUK_HOBJECT_IS_CALLABLE(func))) {
					goto not_callable;
				}
			}

			if (DUK_LIKELY(!DUK_HOBJECT_HAS_BOUNDFUNC(func) && !DUK_HOBJECT_HAS_SPECIAL_CALL(func) &&
			               !DUK_HOBJECT_HAS_EXOTIC_PROXYOBJ(func))) {
				/* Common case, so test for using a single bitfield test.
				 * Break out to handle this coercion etc.
				 */
				break;
			}

			/* XXX: could set specialcall for boundfuncs too, simplify check above */

			if (DUK_HOBJECT_HAS_BOUNDFUNC(func)) {
				DUK_ASSERT(!DUK_HOBJECT_HAS_SPECIAL_CALL(func));
				DUK_ASSERT(!DUK_HOBJECT_IS_NATFUNC(func));

				/* Callable/constructable flags are the same
				 * for the bound function and its target, so
				 * we don't need to check them here, we can
				 * check them from the target only.
				 */
				duk__handle_bound_chain_for_call(thr, idx_func, *call_flags & DUK_CALL_FLAG_CONSTRUCT);

				DUK_ASSERT(DUK_TVAL_IS_OBJECT(duk_require_tval(thr, idx_func)) ||
				           DUK_TVAL_IS_LIGHTFUNC(duk_require_tval(thr, idx_func)));
			} else {
				DUK_ASSERT(DUK_HOBJECT_HAS_SPECIAL_CALL(func));

#if defined(DUK_USE_ES6_PROXY)
				if (DUK_HOBJECT_HAS_EXOTIC_PROXYOBJ(func)) {
					/* If no trap, resume processing from Proxy trap.
					 * If trap exists, helper converts call into a trap
					 * call; this may change a constructor call into a
					 * normal (non-constructor) trap call.  We must
					 * continue processing even when a trap is found as
					 * the trap may be bound.
					 */
					duk__handle_proxy_for_call(thr, idx_func, (duk_hproxy *) func, call_flags);
				} else
#endif
				{
					DUK_ASSERT(DUK_HOBJECT_IS_NATFUNC(func));
					DUK_ASSERT(DUK_HOBJECT_HAS_CALLABLE(func));
					DUK_ASSERT(!DUK_HOBJECT_HAS_CONSTRUCTABLE(func));
					/* Constructable check already done above. */

					if (duk__handle_specialfuncs_for_call(thr, idx_func, func, call_flags, first) != 0) {
						/* Encountered native eval call, normal call
						 * context.  Break out, handle this coercion etc.
						 */
						break;
					}
				}
			}
			/* Retry loop. */
		} else if (DUK_TVAL_IS_LIGHTFUNC(tv_func)) {
			/* Lightfuncs are:
			 *   - Always strict, so no 'this' coercion.
			 *   - Always callable.
			 *   - Always constructable.
			 *   - Never specialfuncs.
			 */
			func = NULL;
			goto finished;
		} else {
			goto not_callable;
		}
	}

	DUK_ASSERT(func != NULL);

	if (!DUK_HOBJECT_HAS_STRICT(func)) {
		/* Non-strict target needs 'this' coercion.
		 * This has potential side effects invalidating
		 * 'tv_func'.
		 */
		duk__coerce_nonstrict_this_binding(thr, idx_func + 1);
	}
	if (*call_flags & DUK_CALL_FLAG_CONSTRUCT) {
		if (!(*call_flags & DUK_CALL_FLAG_DEFAULT_INSTANCE_UPDATED)) {
			*call_flags |= DUK_CALL_FLAG_DEFAULT_INSTANCE_UPDATED;
			duk__update_default_instance_proto(thr, idx_func);
		}
	}

finished :
#if defined(DUK_USE_ASSERTIONS)
{
	duk_tval *tv_tmp;

	tv_tmp = duk_get_tval(thr, idx_func);
	DUK_ASSERT(tv_tmp != NULL);

	DUK_ASSERT((DUK_TVAL_IS_OBJECT(tv_tmp) && DUK_HOBJECT_IS_CALLABLE(DUK_TVAL_GET_OBJECT(tv_tmp))) ||
	           DUK_TVAL_IS_LIGHTFUNC(tv_tmp));
	DUK_ASSERT(func == NULL || !DUK_HOBJECT_HAS_BOUNDFUNC(func));
	DUK_ASSERT(func == NULL || (DUK_HOBJECT_IS_COMPFUNC(func) || DUK_HOBJECT_IS_NATFUNC(func)));
	DUK_ASSERT(func == NULL || (DUK_HOBJECT_HAS_CONSTRUCTABLE(func) || (*call_flags & DUK_CALL_FLAG_CONSTRUCT) == 0));
}
#endif

	return func;

not_callable:
	DUK_ASSERT(tv_func != NULL);

#if defined(DUK_USE_VERBOSE_ERRORS)
	/* GETPROPC delayed error handling: when target is not callable,
	 * GETPROPC replaces idx_func+0 with a non-callable wrapper object
	 * with a hidden Symbol to signify it's to be handled here.  If
	 * found, unwrap the original Error and throw it as is here.  The
	 * hidden Symbol is only checked as an own property, not inherited
	 * (which would be dangerous).
	 */
	if (DUK_TVAL_IS_OBJECT(tv_func)) {
		duk_tval *tv_wrap =
		    duk_hobject_find_entry_tval_ptr_stridx(thr->heap, DUK_TVAL_GET_OBJECT(tv_func), DUK_STRIDX_INT_TARGET);
		if (tv_wrap != NULL) {
			DUK_DD(DUK_DDPRINT("delayed error from GETPROPC: %!T", tv_wrap));
			duk_push_tval(thr, tv_wrap);
			(void) duk_throw(thr);
			DUK_WO_NORETURN(return NULL;);
		}
	}
#endif

#if defined(DUK_USE_VERBOSE_ERRORS)
#if defined(DUK_USE_PARANOID_ERRORS)
	DUK_ERROR_FMT1(thr, DUK_ERR_TYPE_ERROR, "%s not callable", duk_get_type_name(thr, idx_func));
#else
	DUK_ERROR_FMT1(thr, DUK_ERR_TYPE_ERROR, "%s not callable", duk_push_string_tval_readable(thr, tv_func));
#endif
#else
	DUK_ERROR_TYPE(thr, DUK_STR_NOT_CALLABLE);
#endif
	DUK_WO_NORETURN(return NULL;);

not_constructable:
	/* For now GETPROPC delayed error not needed for constructor calls. */
#if defined(DUK_USE_VERBOSE_ERRORS)
#if defined(DUK_USE_PARANOID_ERRORS)
	DUK_ERROR_FMT1(thr, DUK_ERR_TYPE_ERROR, "%s not constructable", duk_get_type_name(thr, idx_func));
#else
	DUK_ERROR_FMT1(thr, DUK_ERR_TYPE_ERROR, "%s not constructable", duk_push_string_tval_readable(thr, tv_func));
#endif
#else
	DUK_ERROR_TYPE(thr, DUK_STR_NOT_CONSTRUCTABLE);
#endif
	DUK_WO_NORETURN(return NULL;);
}

/*
 *  Manipulate value stack so that exactly 'num_stack_rets' return
 *  values are at 'idx_retbase' in every case, assuming there are
 *  'rc' return values on top of stack.
 *
 *  This is a bit tricky, because the called C function operates in
 *  the same activation record and may have e.g. popped the stack
 *  empty (below idx_retbase).
 */

DUK_LOCAL void duk__safe_call_adjust_valstack(duk_hthread *thr,
                                              duk_idx_t idx_retbase,
                                              duk_idx_t num_stack_rets,
                                              duk_idx_t num_actual_rets) {
	duk_idx_t idx_rcbase;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(idx_retbase >= 0);
	DUK_ASSERT(num_stack_rets >= 0);
	DUK_ASSERT(num_actual_rets >= 0);

	idx_rcbase = duk_get_top(thr) - num_actual_rets; /* base of known return values */
	if (DUK_UNLIKELY(idx_rcbase < 0)) {
		DUK_ERROR_TYPE(thr, DUK_STR_INVALID_CFUNC_RC);
		DUK_WO_NORETURN(return;);
	}

	DUK_DDD(DUK_DDDPRINT("adjust valstack after func call: "
	                     "num_stack_rets=%ld, num_actual_rets=%ld, stack_top=%ld, idx_retbase=%ld, idx_rcbase=%ld",
	                     (long) num_stack_rets,
	                     (long) num_actual_rets,
	                     (long) duk_get_top(thr),
	                     (long) idx_retbase,
	                     (long) idx_rcbase));

	DUK_ASSERT(idx_rcbase >= 0); /* caller must check */

	/* Space for num_stack_rets was reserved before the safe call.
	 * Because value stack reserve cannot shrink except in call returns,
	 * the reserve is still in place.  Adjust valstack, carefully
	 * ensuring we don't overstep the reserve.
	 */

	/* Match idx_rcbase with idx_retbase so that the return values
	 * start at the correct index.
	 */
	if (idx_rcbase > idx_retbase) {
		duk_idx_t count = idx_rcbase - idx_retbase;

		DUK_DDD(DUK_DDDPRINT("elements at/after idx_retbase have enough to cover func retvals "
		                     "(idx_retbase=%ld, idx_rcbase=%ld)",
		                     (long) idx_retbase,
		                     (long) idx_rcbase));

		/* Remove values between irc_rcbase (start of intended return
		 * values) and idx_retbase to lower return values to idx_retbase.
		 */
		DUK_ASSERT(count > 0);
		duk_remove_n(thr, idx_retbase, count); /* may be NORZ */
	} else {
		duk_idx_t count = idx_retbase - idx_rcbase;

		DUK_DDD(DUK_DDDPRINT("not enough elements at/after idx_retbase to cover func retvals "
		                     "(idx_retbase=%ld, idx_rcbase=%ld)",
		                     (long) idx_retbase,
		                     (long) idx_rcbase));

		/* Insert 'undefined' at idx_rcbase (start of intended return
		 * values) to lift return values to idx_retbase.
		 */
		DUK_ASSERT(count >= 0);
		DUK_ASSERT(thr->valstack_end - thr->valstack_top >= count); /* reserve cannot shrink */
		duk_insert_undefined_n(thr, idx_rcbase, count);
	}

	/* Chop extra retvals away / extend with undefined. */
	duk_set_top_unsafe(thr, idx_retbase + num_stack_rets);
}

/*
 *  Activation setup for tailcalls and non-tailcalls.
 */

#if defined(DUK_USE_TAILCALL)
DUK_LOCAL duk_small_uint_t duk__call_setup_act_attempt_tailcall(duk_hthread *thr,
                                                                duk_small_uint_t call_flags,
                                                                duk_idx_t idx_func,
                                                                duk_hobject *func,
                                                                duk_size_t entry_valstack_bottom_byteoff,
                                                                duk_size_t entry_valstack_end_byteoff,
                                                                duk_idx_t *out_nargs,
                                                                duk_idx_t *out_nregs,
                                                                duk_size_t *out_vs_min_bytes,
                                                                duk_activation **out_act) {
	duk_activation *act;
	duk_tval *tv1, *tv2;
	duk_idx_t idx_args;
	duk_small_uint_t flags1, flags2;
#if defined(DUK_USE_DEBUGGER_SUPPORT)
	duk_activation *prev_pause_act;
#endif

	DUK_UNREF(entry_valstack_end_byteoff);

	/* Tailcall cannot be flagged to resume calls, and a
	 * previous frame must exist.
	 */
	DUK_ASSERT(thr->callstack_top >= 1);

	act = thr->callstack_curr;
	DUK_ASSERT(act != NULL);
	*out_act = act;

	if (func == NULL || !DUK_HOBJECT_IS_COMPFUNC(func)) {
		DUK_DDD(DUK_DDDPRINT("tail call prevented by target not being ecma function"));
		return 0;
	}
	if (act->flags & DUK_ACT_FLAG_PREVENT_YIELD) {
		DUK_DDD(DUK_DDDPRINT("tail call prevented by current activation having DUK_ACT_FLAG_PREVENT_YIELD"));
		return 0;
	}
	/* Tailcall is only allowed if current and candidate
	 * function have identical return value handling.  There
	 * are three possible return value handling cases:
	 *   1. Normal function call, no special return value handling.
	 *   2. Constructor call, return value replacement object check.
	 *   3. Proxy 'construct' trap call, return value invariant check.
	 */
	flags1 = (duk_small_uint_t) ((act->flags & DUK_ACT_FLAG_CONSTRUCT) ? 1 : 0)
#if defined(DUK_USE_ES6_PROXY)
	         | (duk_small_uint_t) ((act->flags & DUK_ACT_FLAG_CONSTRUCT_PROXY) ? 2 : 0)
#endif
	    ;
	flags2 = (duk_small_uint_t) ((call_flags & DUK_CALL_FLAG_CONSTRUCT) ? 1 : 0)
#if defined(DUK_USE_ES6_PROXY)
	         | (duk_small_uint_t) ((call_flags & DUK_CALL_FLAG_CONSTRUCT_PROXY) ? 2 : 0);
#endif
	;
	if (flags1 != flags2) {
		DUK_DDD(DUK_DDDPRINT("tail call prevented by incompatible return value handling"));
		return 0;
	}
	DUK_ASSERT(((act->flags & DUK_ACT_FLAG_CONSTRUCT) && (call_flags & DUK_CALL_FLAG_CONSTRUCT)) ||
	           (!(act->flags & DUK_ACT_FLAG_CONSTRUCT) && !(call_flags & DUK_CALL_FLAG_CONSTRUCT)));
	DUK_ASSERT(((act->flags & DUK_ACT_FLAG_CONSTRUCT_PROXY) && (call_flags & DUK_CALL_FLAG_CONSTRUCT_PROXY)) ||
	           (!(act->flags & DUK_ACT_FLAG_CONSTRUCT_PROXY) && !(call_flags & DUK_CALL_FLAG_CONSTRUCT_PROXY)));
	if (DUK_HOBJECT_HAS_NOTAIL(func)) {
		/* See: test-bug-tailcall-preventyield-assert.c. */
		DUK_DDD(DUK_DDDPRINT("tail call prevented by function having a notail flag"));
		return 0;
	}

	/*
	 *  Tailcall handling
	 *
	 *  Although the callstack entry is reused, we need to explicitly unwind
	 *  the current activation (or simulate an unwind).  In particular, the
	 *  current activation must be closed, otherwise something like
	 *  test-bug-reduce-judofyr.js results.  Also catchers need to be unwound
	 *  because there may be non-error-catching label entries in valid tail calls.
	 *
	 *  Special attention is needed for debugger and pause behavior when
	 *  reusing an activation.
	 *    - Disable StepOut processing for the activation unwind because
	 *      we reuse the activation, see:
	 *      https://github.com/svaarala/duktape/issues/1684.
	 *    - Disable line change pause flag permanently if act == dbg_pause_act
	 *      (if set) because it would no longer be relevant, see:
	 *      https://github.com/svaarala/duktape/issues/1726,
	 *      https://github.com/svaarala/duktape/issues/1786.
	 *    - Check for function entry (e.g. StepInto) pause flag here, because
	 *      the executor pause check won't trigger due to shared activation, see:
	 *      https://github.com/svaarala/duktape/issues/1726.
	 */

	DUK_DDD(DUK_DDDPRINT("is tail call, reusing activation at callstack top, at index %ld", (long) (thr->callstack_top - 1)));

	DUK_ASSERT(!DUK_HOBJECT_HAS_BOUNDFUNC(func));
	DUK_ASSERT(!DUK_HOBJECT_HAS_NATFUNC(func));
	DUK_ASSERT(DUK_HOBJECT_HAS_COMPFUNC(func));
	DUK_ASSERT((act->flags & DUK_ACT_FLAG_PREVENT_YIELD) == 0);
	DUK_ASSERT(call_flags & DUK_CALL_FLAG_ALLOW_ECMATOECMA);

	/* Unwind the topmost callstack entry before reusing it.  This
	 * also unwinds the catchers related to the topmost entry.
	 */
	DUK_ASSERT(thr->callstack_top > 0);
	DUK_ASSERT(thr->callstack_curr != NULL);
#if defined(DUK_USE_DEBUGGER_SUPPORT)
	if (act == thr->heap->dbg_pause_act) {
		thr->heap->dbg_pause_flags &= ~DUK_PAUSE_FLAG_LINE_CHANGE;
	}

	prev_pause_act = thr->heap->dbg_pause_act;
	thr->heap->dbg_pause_act = NULL;
	if (thr->heap->dbg_pause_flags & DUK_PAUSE_FLAG_FUNC_ENTRY) {
		DUK_D(DUK_DPRINT("PAUSE TRIGGERED by function entry (tailcall)"));
		duk_debug_set_paused(thr->heap);
	}
#endif
	duk_hthread_activation_unwind_reuse_norz(thr);
#if defined(DUK_USE_DEBUGGER_SUPPORT)
	thr->heap->dbg_pause_act = prev_pause_act;
#endif
	DUK_ASSERT(act == thr->callstack_curr);

	/* XXX: We could restore the caller's value stack reserve
	 * here, as if we did an actual unwind-and-call.  Without
	 * the restoration, value stack reserve may remain higher
	 * than would otherwise be possible until we return to a
	 * non-tailcall.
	 */

	/* Then reuse the unwound activation. */
	act->cat = NULL;
	act->var_env = NULL;
	act->lex_env = NULL;
	DUK_ASSERT(func != NULL);
	DUK_ASSERT(DUK_HOBJECT_HAS_COMPFUNC(func));
	act->func = func; /* don't want an intermediate exposed state with func == NULL */
#if defined(DUK_USE_NONSTD_FUNC_CALLER_PROPERTY)
	act->prev_caller = NULL;
#endif
	/* don't want an intermediate exposed state with invalid pc */
	act->curr_pc = DUK_HCOMPFUNC_GET_CODE_BASE(thr->heap, (duk_hcompfunc *) func);
#if defined(DUK_USE_DEBUGGER_SUPPORT)
	act->prev_line = 0;
#endif
	DUK_TVAL_SET_OBJECT(&act->tv_func, func); /* borrowed, no refcount */
	DUK_HOBJECT_INCREF(thr, func);

	act->flags = DUK_ACT_FLAG_TAILCALLED;
	if (DUK_HOBJECT_HAS_STRICT(func)) {
		act->flags |= DUK_ACT_FLAG_STRICT;
	}
	if (call_flags & DUK_CALL_FLAG_CONSTRUCT) {
		act->flags |= DUK_ACT_FLAG_CONSTRUCT;
	}
#if defined(DUK_USE_ES6_PROXY)
	if (call_flags & DUK_CALL_FLAG_CONSTRUCT_PROXY) {
		act->flags |= DUK_ACT_FLAG_CONSTRUCT_PROXY;
	}
#endif

	DUK_ASSERT(DUK_ACT_GET_FUNC(act) == func); /* already updated */
	DUK_ASSERT(act->var_env == NULL);
	DUK_ASSERT(act->lex_env == NULL);
	act->bottom_byteoff = entry_valstack_bottom_byteoff; /* tail call -> reuse current "frame" */
#if 0
	/* Topmost activation retval_byteoff is considered garbage, no need to init. */
	act->retval_byteoff = 0;
#endif
	/* Filled in when final reserve is known, dummy value doesn't matter
	 * even in error unwind because reserve_byteoff is only used when
	 * returning to -this- activation.
	 */
	act->reserve_byteoff = 0;

	/*
	 *  Manipulate valstack so that args are on the current bottom and the
	 *  previous caller's 'this' binding (which is the value preceding the
	 *  current bottom) is replaced with the new 'this' binding:
	 *
	 *       [ ... this_old | (crud) func this_new arg1 ... argN ]
	 *  -->  [ ... this_new | arg1 ... argN ]
	 *
	 *  For tail calling to work properly, the valstack bottom must not grow
	 *  here; otherwise crud would accumulate on the valstack.
	 */

	tv1 = thr->valstack_bottom - 1;
	tv2 = thr->valstack_bottom + idx_func + 1;
	DUK_ASSERT(tv1 >= thr->valstack && tv1 < thr->valstack_top); /* tv1 is -below- valstack_bottom */
	DUK_ASSERT(tv2 >= thr->valstack_bottom && tv2 < thr->valstack_top);
	DUK_TVAL_SET_TVAL_UPDREF(thr, tv1, tv2); /* side effects */

	idx_args = idx_func + 2;
	duk_remove_n(thr, 0, idx_args); /* may be NORZ */

	idx_func = 0;
	DUK_UNREF(idx_func); /* really 'not applicable' anymore, should not be referenced after this */
	idx_args = 0;

	*out_nargs = ((duk_hcompfunc *) func)->nargs;
	*out_nregs = ((duk_hcompfunc *) func)->nregs;
	DUK_ASSERT(*out_nregs >= 0);
	DUK_ASSERT(*out_nregs >= *out_nargs);
	*out_vs_min_bytes =
	    entry_valstack_bottom_byteoff + sizeof(duk_tval) * ((duk_size_t) *out_nregs + DUK_VALSTACK_INTERNAL_EXTRA);

#if defined(DUK_USE_NONSTD_FUNC_CALLER_PROPERTY)
#if defined(DUK_USE_TAILCALL)
#error incorrect options: tail calls enabled with function caller property
#endif
	/* XXX: This doesn't actually work properly for tail calls, so
	 * tail calls are disabled when DUK_USE_NONSTD_FUNC_CALLER_PROPERTY
	 * is in use.
	 */
	duk__update_func_caller_prop(thr, func);
#endif

	/* [ ... this_new | arg1 ... argN ] */

	return 1;
}
#endif /* DUK_USE_TAILCALL */

DUK_LOCAL void duk__call_setup_act_not_tailcall(duk_hthread *thr,
                                                duk_small_uint_t call_flags,
                                                duk_idx_t idx_func,
                                                duk_hobject *func,
                                                duk_size_t entry_valstack_bottom_byteoff,
                                                duk_size_t entry_valstack_end_byteoff,
                                                duk_idx_t *out_nargs,
                                                duk_idx_t *out_nregs,
                                                duk_size_t *out_vs_min_bytes,
                                                duk_activation **out_act) {
	duk_activation *act;
	duk_activation *new_act;

	DUK_UNREF(entry_valstack_end_byteoff);

	DUK_DDD(DUK_DDDPRINT("not a tail call, pushing a new activation to callstack, to index %ld", (long) (thr->callstack_top)));

	duk__call_callstack_limit_check(thr);
	new_act = duk_hthread_activation_alloc(thr);
	DUK_ASSERT(new_act != NULL);

	act = thr->callstack_curr;
	if (act != NULL) {
		/*
		 *  Update return value stack index of current activation (if any).
		 *
		 *  Although it might seem this is not necessary (bytecode executor
		 *  does this for ECMAScript-to-ECMAScript calls; other calls are
		 *  handled here), this turns out to be necessary for handling yield
		 *  and resume.  For them, an ECMAScript-to-native call happens, and
		 *  the ECMAScript call's retval_byteoff must be set for things to work.
		 */

		act->retval_byteoff = entry_valstack_bottom_byteoff + (duk_size_t) idx_func * sizeof(duk_tval);
	}

	new_act->parent = act;
	thr->callstack_curr = new_act;
	thr->callstack_top++;
	act = new_act;
	*out_act = act;

	DUK_ASSERT(thr->valstack_top > thr->valstack_bottom); /* at least effective 'this' */
	DUK_ASSERT(func == NULL || !DUK_HOBJECT_HAS_BOUNDFUNC(func));

	act->cat = NULL;

	act->flags = 0;
	if (call_flags & DUK_CALL_FLAG_CONSTRUCT) {
		act->flags |= DUK_ACT_FLAG_CONSTRUCT;
	}
#if defined(DUK_USE_ES6_PROXY)
	if (call_flags & DUK_CALL_FLAG_CONSTRUCT_PROXY) {
		act->flags |= DUK_ACT_FLAG_CONSTRUCT_PROXY;
	}
#endif
	if (call_flags & DUK_CALL_FLAG_DIRECT_EVAL) {
		act->flags |= DUK_ACT_FLAG_DIRECT_EVAL;
	}

	/* start of arguments: idx_func + 2. */
	act->func = func; /* NULL for lightfunc */
	if (DUK_LIKELY(func != NULL)) {
		DUK_TVAL_SET_OBJECT(&act->tv_func, func); /* borrowed, no refcount */
		if (DUK_HOBJECT_HAS_STRICT(func)) {
			act->flags |= DUK_ACT_FLAG_STRICT;
		}
		if (DUK_HOBJECT_IS_COMPFUNC(func)) {
			*out_nargs = ((duk_hcompfunc *) func)->nargs;
			*out_nregs = ((duk_hcompfunc *) func)->nregs;
			DUK_ASSERT(*out_nregs >= 0);
			DUK_ASSERT(*out_nregs >= *out_nargs);
			*out_vs_min_bytes =
			    entry_valstack_bottom_byteoff +
			    sizeof(duk_tval) * ((duk_size_t) idx_func + 2U + (duk_size_t) *out_nregs + DUK_VALSTACK_INTERNAL_EXTRA);
		} else {
			/* True because of call target lookup checks. */
			DUK_ASSERT(DUK_HOBJECT_IS_NATFUNC(func));

			*out_nargs = ((duk_hnatfunc *) func)->nargs;
			*out_nregs = *out_nargs;
			if (*out_nargs >= 0) {
				*out_vs_min_bytes =
				    entry_valstack_bottom_byteoff +
				    sizeof(duk_tval) * ((duk_size_t) idx_func + 2U + (duk_size_t) *out_nregs +
				                        DUK_VALSTACK_API_ENTRY_MINIMUM + DUK_VALSTACK_INTERNAL_EXTRA);
			} else {
				/* Vararg function. */
				duk_size_t valstack_top_byteoff =
				    (duk_size_t) ((duk_uint8_t *) thr->valstack_top - ((duk_uint8_t *) thr->valstack));
				*out_vs_min_bytes = valstack_top_byteoff + sizeof(duk_tval) * (DUK_VALSTACK_API_ENTRY_MINIMUM +
				                                                               DUK_VALSTACK_INTERNAL_EXTRA);
			}
		}
	} else {
		duk_small_uint_t lf_flags;
		duk_tval *tv_func;

		act->flags |= DUK_ACT_FLAG_STRICT;

		tv_func = DUK_GET_TVAL_POSIDX(thr, idx_func);
		DUK_ASSERT(DUK_TVAL_IS_LIGHTFUNC(tv_func));
		DUK_TVAL_SET_TVAL(&act->tv_func, tv_func); /* borrowed, no refcount */

		lf_flags = DUK_TVAL_GET_LIGHTFUNC_FLAGS(tv_func);
		*out_nargs = DUK_LFUNC_FLAGS_GET_NARGS(lf_flags);
		if (*out_nargs != DUK_LFUNC_NARGS_VARARGS) {
			*out_vs_min_bytes = entry_valstack_bottom_byteoff +
			                    sizeof(duk_tval) * ((duk_size_t) idx_func + 2U + (duk_size_t) *out_nargs +
			                                        DUK_VALSTACK_API_ENTRY_MINIMUM + DUK_VALSTACK_INTERNAL_EXTRA);
		} else {
			duk_size_t valstack_top_byteoff =
			    (duk_size_t) ((duk_uint8_t *) thr->valstack_top - ((duk_uint8_t *) thr->valstack));
			*out_vs_min_bytes = valstack_top_byteoff +
			                    sizeof(duk_tval) * (DUK_VALSTACK_API_ENTRY_MINIMUM + DUK_VALSTACK_INTERNAL_EXTRA);
			*out_nargs = -1; /* vararg */
		}
		*out_nregs = *out_nargs;
	}

	act->var_env = NULL;
	act->lex_env = NULL;
#if defined(DUK_USE_NONSTD_FUNC_CALLER_PROPERTY)
	act->prev_caller = NULL;
#endif
	act->curr_pc = NULL;
#if defined(DUK_USE_DEBUGGER_SUPPORT)
	act->prev_line = 0;
#endif
	act->bottom_byteoff = entry_valstack_bottom_byteoff + sizeof(duk_tval) * ((duk_size_t) idx_func + 2U);
#if 0
	act->retval_byteoff = 0;   /* topmost activation retval_byteoff is considered garbage, no need to init */
#endif
	/* Filled in when final reserve is known, dummy value doesn't matter
	 * even in error unwind because reserve_byteoff is only used when
	 * returning to -this- activation.
	 */
	act->reserve_byteoff = 0; /* filled in by caller */

	/* XXX: Is this INCREF necessary? 'func' is always a borrowed
	 * reference reachable through the value stack?  If changed, stack
	 * unwind code also needs to be fixed to match.
	 */
	DUK_HOBJECT_INCREF_ALLOWNULL(thr, func); /* act->func */

#if defined(DUK_USE_NONSTD_FUNC_CALLER_PROPERTY)
	if (func) {
		duk__update_func_caller_prop(thr, func);
	}
#endif
}

/*
 *  Environment setup.
 */

DUK_LOCAL void duk__call_env_setup(duk_hthread *thr, duk_hobject *func, duk_activation *act, duk_idx_t idx_args) {
	duk_hobject *env;

	DUK_ASSERT(func == NULL || !DUK_HOBJECT_HAS_BOUNDFUNC(func)); /* bound function has already been resolved */

	if (DUK_LIKELY(func != NULL)) {
		if (DUK_LIKELY(DUK_HOBJECT_HAS_NEWENV(func))) {
			DUK_STATS_INC(thr->heap, stats_envrec_newenv);
			if (DUK_LIKELY(!DUK_HOBJECT_HAS_CREATEARGS(func))) {
				/* Use a new environment but there's no 'arguments' object;
				 * delayed environment initialization.  This is the most
				 * common case.
				 */
				DUK_ASSERT(act->lex_env == NULL);
				DUK_ASSERT(act->var_env == NULL);
			} else {
				/* Use a new environment and there's an 'arguments' object.
				 * We need to initialize it right now.
				 */

				/* third arg: absolute index (to entire valstack) of bottom_byteoff of new activation */
				env = duk_create_activation_environment_record(thr, func, act->bottom_byteoff);
				DUK_ASSERT(env != NULL);

				/* [ ... func this arg1 ... argN envobj ] */

				DUK_ASSERT(DUK_HOBJECT_HAS_CREATEARGS(func));
				duk__handle_createargs_for_call(thr, func, env, idx_args);

				/* [ ... func this arg1 ... argN envobj ] */

				act->lex_env = env;
				act->var_env = env;
				DUK_HOBJECT_INCREF(thr, env);
				DUK_HOBJECT_INCREF(thr, env); /* XXX: incref by count (2) directly */
				duk_pop(thr);
			}
		} else {
			/* Use existing env (e.g. for non-strict eval); cannot have
			 * an own 'arguments' object (but can refer to an existing one).
			 */

			DUK_ASSERT(!DUK_HOBJECT_HAS_CREATEARGS(func));

			DUK_STATS_INC(thr->heap, stats_envrec_oldenv);
			duk__handle_oldenv_for_call(thr, func, act);

			DUK_ASSERT(act->lex_env != NULL);
			DUK_ASSERT(act->var_env != NULL);
		}
	} else {
		/* Lightfuncs are always native functions and have "newenv". */
		DUK_ASSERT(act->lex_env == NULL);
		DUK_ASSERT(act->var_env == NULL);
		DUK_STATS_INC(thr->heap, stats_envrec_newenv);
	}
}

/*
 *  Misc shared helpers.
 */

/* Check thread state, update current thread. */
DUK_LOCAL void duk__call_thread_state_update(duk_hthread *thr) {
	DUK_ASSERT(thr != NULL);

	if (DUK_LIKELY(thr == thr->heap->curr_thread)) {
		if (DUK_UNLIKELY(thr->state != DUK_HTHREAD_STATE_RUNNING)) {
			/* Should actually never happen, but check anyway. */
			goto thread_state_error;
		}
	} else {
		DUK_ASSERT(thr->heap->curr_thread == NULL || thr->heap->curr_thread->state == DUK_HTHREAD_STATE_RUNNING);
		if (DUK_UNLIKELY(thr->state != DUK_HTHREAD_STATE_INACTIVE)) {
			goto thread_state_error;
		}
		DUK_HEAP_SWITCH_THREAD(thr->heap, thr);
		thr->state = DUK_HTHREAD_STATE_RUNNING;

		/* Multiple threads may be simultaneously in the RUNNING
		 * state, but not in the same "resume chain".
		 */
	}
	DUK_ASSERT(thr->heap->curr_thread == thr);
	DUK_ASSERT(thr->state == DUK_HTHREAD_STATE_RUNNING);
	return;

thread_state_error:
	DUK_ERROR_FMT1(thr, DUK_ERR_TYPE_ERROR, "invalid thread state (%ld)", (long) thr->state);
	DUK_WO_NORETURN(return;);
}

/*
 *  Main unprotected call handler, handles:
 *
 *    - All combinations of native/ECMAScript caller and native/ECMAScript
 *      target.
 *
 *    - Optimized ECMAScript-to-ECMAScript call where call handling only
 *      sets up a new duk_activation but reuses an existing bytecode executor
 *      (the caller) without native recursion.
 *
 *    - Tailcalls, where an activation is reused without increasing call
 *      stack (duk_activation) depth.
 *
 *    - Setup for an initial Duktape.Thread.resume().
 *
 *  The call handler doesn't provide any protection guarantees, protected calls
 *  must be implemented e.g. by wrapping the call in a duk_safe_call().
 *  Call setup may fail at any stage, even when the new activation is in
 *  place; the only guarantee is that the state is consistent for unwinding.
 */

DUK_LOCAL duk_int_t duk__handle_call_raw(duk_hthread *thr, duk_idx_t idx_func, duk_small_uint_t call_flags) {
#if defined(DUK_USE_ASSERTIONS)
	duk_activation *entry_act;
	duk_size_t entry_callstack_top;
#endif
	duk_size_t entry_valstack_bottom_byteoff;
	duk_size_t entry_valstack_end_byteoff;
	duk_int_t entry_call_recursion_depth;
	duk_hthread *entry_curr_thread;
	duk_uint_fast8_t entry_thread_state;
	duk_instr_t **entry_ptr_curr_pc;
	duk_idx_t idx_args;
	duk_idx_t nargs; /* # argument registers target function wants (< 0 => "as is") */
	duk_idx_t nregs; /* # total registers target function wants on entry (< 0 => "as is") */
	duk_size_t vs_min_bytes; /* minimum value stack size (bytes) for handling call */
	duk_hobject *func; /* 'func' on stack (borrowed reference) */
	duk_activation *act;
	duk_ret_t rc;
	duk_small_uint_t use_tailcall;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(thr->heap != NULL);
	/* Asserts for heap->curr_thread omitted: it may be NULL, 'thr', or
	 * any other thread (e.g. when heap thread is used to run finalizers).
	 */
	DUK_CTX_ASSERT_VALID(thr);
	DUK_ASSERT(duk_is_valid_index(thr, idx_func));
	DUK_ASSERT(idx_func >= 0);

	DUK_STATS_INC(thr->heap, stats_call_all);

	/* If a tail call:
	 *   - an ECMAScript activation must be on top of the callstack
	 *   - there cannot be any catch stack entries that would catch
	 *     a return
	 */
#if defined(DUK_USE_ASSERTIONS)
	if (call_flags & DUK_CALL_FLAG_TAILCALL) {
		duk_activation *tmp_act;
		duk_catcher *tmp_cat;

		DUK_ASSERT(thr->callstack_top >= 1);
		DUK_ASSERT(DUK_ACT_GET_FUNC(thr->callstack_curr) != NULL);
		DUK_ASSERT(DUK_HOBJECT_IS_COMPFUNC(DUK_ACT_GET_FUNC(thr->callstack_curr)));

		/* No entry in the catch stack which would actually catch a
		 * throw can refer to the callstack entry being reused.
		 * There *can* be catch stack entries referring to the current
		 * callstack entry as long as they don't catch (e.g. label sites).
		 */

		tmp_act = thr->callstack_curr;
		for (tmp_cat = tmp_act->cat; tmp_cat != NULL; tmp_cat = tmp_cat->parent) {
			DUK_ASSERT(DUK_CAT_GET_TYPE(tmp_cat) == DUK_CAT_TYPE_LABEL); /* a non-catching entry */
		}
	}
#endif /* DUK_USE_ASSERTIONS */

	/*
	 *  Store entry state.
	 */

#if defined(DUK_USE_ASSERTIONS)
	entry_act = thr->callstack_curr;
	entry_callstack_top = thr->callstack_top;
#endif
	entry_valstack_bottom_byteoff = (duk_size_t) ((duk_uint8_t *) thr->valstack_bottom - (duk_uint8_t *) thr->valstack);
	entry_valstack_end_byteoff = (duk_size_t) ((duk_uint8_t *) thr->valstack_end - (duk_uint8_t *) thr->valstack);
	entry_call_recursion_depth = thr->heap->call_recursion_depth;
	entry_curr_thread = thr->heap->curr_thread; /* may be NULL if first call */
	entry_thread_state = thr->state;
	entry_ptr_curr_pc = thr->ptr_curr_pc; /* may be NULL */

	/* If thr->ptr_curr_pc is set, sync curr_pc to act->pc.  Then NULL
	 * thr->ptr_curr_pc so that it's not accidentally used with an incorrect
	 * activation when side effects occur.
	 */
	duk_hthread_sync_and_null_currpc(thr);
	DUK_ASSERT(thr->ptr_curr_pc == NULL);

	DUK_DD(DUK_DDPRINT("duk__handle_call_raw: thr=%p, idx_func=%ld, "
	                   "call_flags=0x%08lx (constructor=%ld), "
	                   "valstack_top=%ld, idx_func=%ld, idx_args=%ld, rec_depth=%ld/%ld, "
	                   "entry_valstack_bottom_byteoff=%ld, entry_valstack_end_byteoff=%ld, "
	                   "entry_call_recursion_depth=%ld, "
	                   "entry_curr_thread=%p, entry_thread_state=%ld",
	                   (void *) thr,
	                   (long) idx_func,
	                   (unsigned long) call_flags,
	                   (long) ((call_flags & DUK_CALL_FLAG_CONSTRUCT) != 0 ? 1 : 0),
	                   (long) duk_get_top(thr),
	                   (long) idx_func,
	                   (long) (idx_func + 2),
	                   (long) thr->heap->call_recursion_depth,
	                   (long) thr->heap->call_recursion_limit,
	                   (long) entry_valstack_bottom_byteoff,
	                   (long) entry_valstack_end_byteoff,
	                   (long) entry_call_recursion_depth,
	                   (void *) entry_curr_thread,
	                   (long) entry_thread_state));

	/*
	 *  Thread state check and book-keeping.
	 */

	duk__call_thread_state_update(thr);

	/*
	 *  Increase call recursion depth as early as possible so that if we
	 *  enter a recursive call for any reason there's a backstop to native
	 *  recursion.  This can happen e.g. for almost any property read
	 *  because it may cause a getter call or a Proxy trap (GC and finalizers
	 *  are not an issue because they are not recursive).  If we end up
	 *  doing an Ecma-to-Ecma call, revert the increase.  (See GH-2032.)
	 *
	 *  For similar reasons, ensure there is a known value stack spare
	 *  even before we actually prepare the value stack for the target
	 *  function.  If this isn't done, early recursion may consume the
	 *  value stack space.
	 *
	 *  XXX: Should bump yield preventcount early, for the same reason.
	 */

	duk__call_c_recursion_limit_check(thr);
	thr->heap->call_recursion_depth++;
	duk_require_stack(thr, DUK__CALL_HANDLING_REQUIRE_STACK);

	/*
	 *  Resolve final target function; handle bound functions and special
	 *  functions like .call() and .apply().  Also figure out the effective
	 *  'this' binding, which replaces the current value at idx_func + 1.
	 */

	if (DUK_LIKELY(duk__resolve_target_fastpath_check(thr, idx_func, &func, call_flags) != 0U)) {
		DUK_DDD(DUK_DDDPRINT("fast path target resolve"));
	} else {
		DUK_DDD(DUK_DDDPRINT("slow path target resolve"));
		func = duk__resolve_target_func_and_this_binding(thr, idx_func, &call_flags);
	}
	DUK_ASSERT(duk_get_top(thr) - idx_func >= 2); /* at least func and this present */

	DUK_ASSERT(func == NULL || !DUK_HOBJECT_HAS_BOUNDFUNC(func));
	DUK_ASSERT(func == NULL || (DUK_HOBJECT_IS_COMPFUNC(func) || DUK_HOBJECT_IS_NATFUNC(func)));

	/* [ ... func this arg1 ... argN ] */

	/*
	 *  Setup a preliminary activation and figure out nargs/nregs and
	 *  value stack minimum size.
	 *
	 *  Don't touch valstack_bottom or valstack_top yet so that Duktape API
	 *  calls work normally.
	 *
	 *  Because 'act' is not zeroed, all fields must be filled in.
	 */

	/* Should not be necessary, but initialize to silence warnings. */
	act = NULL;
	nargs = 0;
	nregs = 0;
	vs_min_bytes = 0;

#if defined(DUK_USE_TAILCALL)
	use_tailcall = (call_flags & DUK_CALL_FLAG_TAILCALL);
	if (use_tailcall) {
		use_tailcall = duk__call_setup_act_attempt_tailcall(thr,
		                                                    call_flags,
		                                                    idx_func,
		                                                    func,
		                                                    entry_valstack_bottom_byteoff,
		                                                    entry_valstack_end_byteoff,
		                                                    &nargs,
		                                                    &nregs,
		                                                    &vs_min_bytes,
		                                                    &act);
	}
#else
	DUK_ASSERT((call_flags & DUK_CALL_FLAG_TAILCALL) == 0); /* compiler ensures this */
	use_tailcall = 0;
#endif

	if (use_tailcall) {
		idx_args = 0;
		DUK_STATS_INC(thr->heap, stats_call_tailcall);
	} else {
		duk__call_setup_act_not_tailcall(thr,
		                                 call_flags,
		                                 idx_func,
		                                 func,
		                                 entry_valstack_bottom_byteoff,
		                                 entry_valstack_end_byteoff,
		                                 &nargs,
		                                 &nregs,
		                                 &vs_min_bytes,
		                                 &act);
		idx_args = idx_func + 2;
	}
	/* After this point idx_func is no longer valid for tailcalls. */

	DUK_ASSERT(act != NULL);

	/* [ ... func this arg1 ... argN ] */

	/*
	 *  Grow value stack to required size before env setup.  This
	 *  must happen before env setup to handle some corner cases
	 *  correctly, e.g. test-bug-scope-segv-gh2448.js.
	 */

	duk_valstack_grow_check_throw(thr, vs_min_bytes);
	act->reserve_byteoff = (duk_size_t) ((duk_uint8_t *) thr->valstack_end - (duk_uint8_t *) thr->valstack);

	/*
	 *  Environment record creation and 'arguments' object creation.
	 *  Named function expression name binding is handled by the
	 *  compiler; the compiled function's parent env will contain
	 *  the (immutable) binding already.
	 *
	 *  This handling is now identical for C and ECMAScript functions.
	 *  C functions always have the 'NEWENV' flag set, so their
	 *  environment record initialization is delayed (which is good).
	 *
	 *  Delayed creation (on demand) is handled in duk_js_var.c.
	 */

	duk__call_env_setup(thr, func, act, idx_args);

	/* [ ... func this arg1 ... argN ] */

	/*
	 *  Setup value stack: clamp to 'nargs', fill up to 'nregs',
	 *  ensure value stack size matches target requirements, and
	 *  switch value stack bottom.  Valstack top is kept.
	 */

	if (use_tailcall) {
		DUK_ASSERT(nregs >= 0);
		DUK_ASSERT(nregs >= nargs);
		duk_set_top_and_wipe(thr, nregs, nargs);
	} else {
		if (nregs >= 0) {
			DUK_ASSERT(nregs >= nargs);
			duk_set_top_and_wipe(thr, idx_func + 2 + nregs, idx_func + 2 + nargs);
		} else {
			;
		}
		thr->valstack_bottom = thr->valstack_bottom + idx_func + 2;
	}
	DUK_ASSERT(thr->valstack_bottom >= thr->valstack);
	DUK_ASSERT(thr->valstack_top >= thr->valstack_bottom);
	DUK_ASSERT(thr->valstack_end >= thr->valstack_top);

	/*
	 *  Make the actual call.  For Ecma-to-Ecma calls detect that
	 *  setup is complete, then return with a status code that allows
	 *  the caller to reuse the running executor.
	 */

	if (func != NULL && DUK_HOBJECT_IS_COMPFUNC(func)) {
		/*
		 *  ECMAScript call.
		 */

		DUK_ASSERT(func != NULL);
		DUK_ASSERT(DUK_HOBJECT_HAS_COMPFUNC(func));
		act->curr_pc = DUK_HCOMPFUNC_GET_CODE_BASE(thr->heap, (duk_hcompfunc *) func);

		if (call_flags & DUK_CALL_FLAG_ALLOW_ECMATOECMA) {
			DUK_DD(DUK_DDPRINT("avoid native call, use existing executor"));
			DUK_STATS_INC(thr->heap, stats_call_ecmatoecma);
			DUK_ASSERT((act->flags & DUK_ACT_FLAG_PREVENT_YIELD) == 0);
			DUK_REFZERO_CHECK_FAST(thr);
			DUK_ASSERT(thr->ptr_curr_pc == NULL);
			thr->heap->call_recursion_depth--; /* No recursion increase for this case. */
			return 1; /* 1=reuse executor */
		}
		DUK_ASSERT(use_tailcall == 0);

		/* duk_hthread_activation_unwind_norz() will decrease this on unwind */
		DUK_ASSERT((act->flags & DUK_ACT_FLAG_PREVENT_YIELD) == 0);
		act->flags |= DUK_ACT_FLAG_PREVENT_YIELD;
		thr->callstack_preventcount++;

		/* [ ... func this | arg1 ... argN ] ('this' must precede new bottom) */

		/*
		 *  Bytecode executor call.
		 *
		 *  Execute bytecode, handling any recursive function calls and
		 *  thread resumptions.  Returns when execution would return from
		 *  the entry level activation.  When the executor returns, a
		 *  single return value is left on the stack top.
		 *
		 *  The only possible longjmp() is an error (DUK_LJ_TYPE_THROW),
		 *  other types are handled internally by the executor.
		 */

		/* thr->ptr_curr_pc is set by bytecode executor early on entry */
		DUK_ASSERT(thr->ptr_curr_pc == NULL);
		DUK_DDD(DUK_DDDPRINT("entering bytecode execution"));
		duk_js_execute_bytecode(thr);
		DUK_DDD(DUK_DDDPRINT("returned from bytecode execution"));
	} else {
		/*
		 *  Native call.
		 */

		DUK_ASSERT(func == NULL || ((duk_hnatfunc *) func)->func != NULL);
		DUK_ASSERT(use_tailcall == 0);

		/* [ ... func this | arg1 ... argN ] ('this' must precede new bottom) */

		/* duk_hthread_activation_unwind_norz() will decrease this on unwind */
		DUK_ASSERT((act->flags & DUK_ACT_FLAG_PREVENT_YIELD) == 0);
		act->flags |= DUK_ACT_FLAG_PREVENT_YIELD;
		thr->callstack_preventcount++;

		/* For native calls must be NULL so we don't sync back */
		DUK_ASSERT(thr->ptr_curr_pc == NULL);

		/* XXX: native funcptr could come out of call setup. */
		if (func) {
			rc = ((duk_hnatfunc *) func)->func(thr);
		} else {
			duk_tval *tv_func;
			duk_c_function funcptr;

			tv_func = &act->tv_func;
			DUK_ASSERT(DUK_TVAL_IS_LIGHTFUNC(tv_func));
			funcptr = DUK_TVAL_GET_LIGHTFUNC_FUNCPTR(tv_func);
			rc = funcptr(thr);
		}

		/* Automatic error throwing, retval check. */

		if (rc == 0) {
			DUK_ASSERT(thr->valstack < thr->valstack_end);
			DUK_ASSERT(DUK_TVAL_IS_UNDEFINED(thr->valstack_top));
			thr->valstack_top++;
		} else if (rc == 1) {
			;
		} else if (rc < 0) {
			duk_error_throw_from_negative_rc(thr, rc);
			DUK_WO_NORETURN(return 0;);
		} else {
			DUK_ERROR_TYPE(thr, DUK_STR_INVALID_CFUNC_RC);
			DUK_WO_NORETURN(return 0;);
		}
	}
	DUK_ASSERT(thr->ptr_curr_pc == NULL);
	DUK_ASSERT(use_tailcall == 0);

	/*
	 *  Constructor call post processing.
	 */

#if defined(DUK_USE_ES6_PROXY)
	if (call_flags & (DUK_CALL_FLAG_CONSTRUCT | DUK_CALL_FLAG_CONSTRUCT_PROXY)) {
		duk_call_construct_postprocess(thr, call_flags & DUK_CALL_FLAG_CONSTRUCT_PROXY);
	}
#else
	if (call_flags & DUK_CALL_FLAG_CONSTRUCT) {
		duk_call_construct_postprocess(thr, 0);
	}
#endif

	/*
	 *  Unwind, restore valstack bottom and other book-keeping.
	 */

	DUK_ASSERT(thr->callstack_curr != NULL);
	DUK_ASSERT(thr->callstack_curr->parent == entry_act);
	DUK_ASSERT(thr->callstack_top == entry_callstack_top + 1);
	duk_hthread_activation_unwind_norz(thr);
	DUK_ASSERT(thr->callstack_curr == entry_act);
	DUK_ASSERT(thr->callstack_top == entry_callstack_top);

	thr->valstack_bottom = (duk_tval *) (void *) ((duk_uint8_t *) thr->valstack + entry_valstack_bottom_byteoff);
	/* keep current valstack_top */
	DUK_ASSERT(thr->valstack_bottom >= thr->valstack);
	DUK_ASSERT(thr->valstack_top >= thr->valstack_bottom);
	DUK_ASSERT(thr->valstack_end >= thr->valstack_top);
	DUK_ASSERT(thr->valstack_top - thr->valstack_bottom >= idx_func + 1);

	/* Return value handling. */

	/* [ ... func this (crud) retval ] */

	{
		duk_tval *tv_ret;
		duk_tval *tv_funret;

		tv_ret = thr->valstack_bottom + idx_func;
		tv_funret = thr->valstack_top - 1;
#if defined(DUK_USE_FASTINT)
		/* Explicit check for fastint downgrade. */
		DUK_TVAL_CHKFAST_INPLACE_FAST(tv_funret);
#endif
		DUK_TVAL_SET_TVAL_UPDREF(thr, tv_ret, tv_funret); /* side effects */
	}

	duk_set_top_unsafe(thr, idx_func + 1);

	/* [ ... retval ] */

	/* Restore caller's value stack reserve (cannot fail). */
	DUK_ASSERT((duk_uint8_t *) thr->valstack + entry_valstack_end_byteoff >= (duk_uint8_t *) thr->valstack_top);
	DUK_ASSERT((duk_uint8_t *) thr->valstack + entry_valstack_end_byteoff <= (duk_uint8_t *) thr->valstack_alloc_end);
	thr->valstack_end = (duk_tval *) (void *) ((duk_uint8_t *) thr->valstack + entry_valstack_end_byteoff);

	/* XXX: Trial value stack shrink would be OK here, but we'd need
	 * to prevent side effects of the potential realloc.
	 */

	/* Restore entry thread executor curr_pc stack frame pointer. */
	thr->ptr_curr_pc = entry_ptr_curr_pc;

	DUK_HEAP_SWITCH_THREAD(thr->heap, entry_curr_thread); /* may be NULL */
	thr->state = (duk_uint8_t) entry_thread_state;

	/* Disabled assert: triggered with some torture tests. */
#if 0
	DUK_ASSERT((thr->state == DUK_HTHREAD_STATE_INACTIVE && thr->heap->curr_thread == NULL) ||  /* first call */
	           (thr->state == DUK_HTHREAD_STATE_INACTIVE && thr->heap->curr_thread != NULL) ||  /* other call */
	           (thr->state == DUK_HTHREAD_STATE_RUNNING && thr->heap->curr_thread == thr));     /* current thread */
#endif

	thr->heap->call_recursion_depth = entry_call_recursion_depth;

	/* If the debugger is active we need to force an interrupt so that
	 * debugger breakpoints are rechecked.  This is important for function
	 * calls caused by side effects (e.g. when doing a DUK_OP_GETPROP), see
	 * GH-303.  Only needed for success path, error path always causes a
	 * breakpoint recheck in the executor.  It would be enough to set this
	 * only when returning to an ECMAScript activation, but setting the flag
	 * on every return should have no ill effect.
	 */
#if defined(DUK_USE_DEBUGGER_SUPPORT)
	if (duk_debug_is_attached(thr->heap)) {
		DUK_DD(DUK_DDPRINT("returning with debugger enabled, force interrupt"));
		DUK_ASSERT(thr->interrupt_counter <= thr->interrupt_init);
		thr->interrupt_init -= thr->interrupt_counter;
		thr->interrupt_counter = 0;
		thr->heap->dbg_force_restart = 1;
	}
#endif

#if defined(DUK_USE_INTERRUPT_COUNTER) && defined(DUK_USE_DEBUG)
	duk__interrupt_fixup(thr, entry_curr_thread);
#endif

	/* Restored by success path. */
	DUK_ASSERT(thr->heap->call_recursion_depth == entry_call_recursion_depth);
	DUK_ASSERT(thr->ptr_curr_pc == entry_ptr_curr_pc);
	DUK_ASSERT_LJSTATE_UNSET(thr->heap);

	DUK_REFZERO_CHECK_FAST(thr);

	return 0; /* 0=call handled inline */
}

DUK_INTERNAL duk_int_t duk_handle_call_unprotected_nargs(duk_hthread *thr, duk_idx_t nargs, duk_small_uint_t call_flags) {
	duk_idx_t idx_func;
	DUK_ASSERT(duk_get_top(thr) >= nargs + 2);
	idx_func = duk_get_top(thr) - (nargs + 2);
	DUK_ASSERT(idx_func >= 0);
	return duk_handle_call_unprotected(thr, idx_func, call_flags);
}

DUK_INTERNAL duk_int_t duk_handle_call_unprotected(duk_hthread *thr, duk_idx_t idx_func, duk_small_uint_t call_flags) {
	DUK_ASSERT(duk_is_valid_index(thr, idx_func));
	DUK_ASSERT(idx_func >= 0);
	return duk__handle_call_raw(thr, idx_func, call_flags);
}

/*
 *  duk_handle_safe_call(): make a "C protected call" within the
 *  current activation.
 *
 *  The allowed thread states for making a call are the same as for
 *  duk_handle_call_protected().
 *
 *  Even though this call is protected, errors are thrown for insane arguments
 *  and may result in a fatal error unless there's another protected call which
 *  catches such errors.
 *
 *  The error handling path should be error free, even for out-of-memory
 *  errors, to ensure safe sandboxing.  (As of Duktape 2.2.0 this is not
 *  yet the case for environment closing which may run out of memory, see
 *  XXX notes below.)
 */

DUK_LOCAL void duk__handle_safe_call_inner(duk_hthread *thr,
                                           duk_safe_call_function func,
                                           void *udata,
#if defined(DUK_USE_ASSERTIONS)
                                           duk_size_t entry_valstack_bottom_byteoff,
                                           duk_size_t entry_callstack_top,
#endif
                                           duk_hthread *entry_curr_thread,
                                           duk_uint_fast8_t entry_thread_state,
                                           duk_idx_t idx_retbase,
                                           duk_idx_t num_stack_rets) {
	duk_ret_t rc;

	DUK_ASSERT(thr != NULL);
	DUK_CTX_ASSERT_VALID(thr);

	/*
	 *  Thread state check and book-keeping.
	 */

	duk__call_thread_state_update(thr);

	/*
	 *  Recursion limit check.
	 */

	duk__call_c_recursion_limit_check(thr);
	thr->heap->call_recursion_depth++;

	/*
	 *  Make the C call.
	 */

	rc = func(thr, udata);

	DUK_DDD(DUK_DDDPRINT("safe_call, func rc=%ld", (long) rc));

	/*
	 *  Valstack manipulation for results.
	 */

	/* we're running inside the caller's activation, so no change in call/catch stack or valstack bottom */
	DUK_ASSERT(thr->callstack_top == entry_callstack_top);
	DUK_ASSERT(thr->valstack_bottom >= thr->valstack);
	DUK_ASSERT((duk_size_t) ((duk_uint8_t *) thr->valstack_bottom - (duk_uint8_t *) thr->valstack) ==
	           entry_valstack_bottom_byteoff);
	DUK_ASSERT(thr->valstack_top >= thr->valstack_bottom);
	DUK_ASSERT(thr->valstack_end >= thr->valstack_top);

	if (DUK_UNLIKELY(rc < 0)) {
		duk_error_throw_from_negative_rc(thr, rc);
		DUK_WO_NORETURN(return;);
	}
	DUK_ASSERT(rc >= 0);

	duk__safe_call_adjust_valstack(thr, idx_retbase, num_stack_rets, rc); /* throws for insane rc */

	DUK_HEAP_SWITCH_THREAD(thr->heap, entry_curr_thread); /* may be NULL */
	thr->state = (duk_uint8_t) entry_thread_state;
}

DUK_LOCAL void duk__handle_safe_call_error(duk_hthread *thr,
                                           duk_activation *entry_act,
#if defined(DUK_USE_ASSERTIONS)
                                           duk_size_t entry_callstack_top,
#endif
                                           duk_hthread *entry_curr_thread,
                                           duk_uint_fast8_t entry_thread_state,
                                           duk_idx_t idx_retbase,
                                           duk_idx_t num_stack_rets,
                                           duk_size_t entry_valstack_bottom_byteoff,
                                           duk_jmpbuf *old_jmpbuf_ptr) {
	DUK_ASSERT(thr != NULL);
	DUK_CTX_ASSERT_VALID(thr);

	/*
	 *  Error during call.  The error value is at heap->lj.value1.
	 *
	 *  The very first thing we do is restore the previous setjmp catcher.
	 *  This means that any error in error handling will propagate outwards
	 *  instead of causing a setjmp() re-entry above.
	 */

	DUK_DDD(DUK_DDDPRINT("error caught during protected duk_handle_safe_call()"));

	/* Other longjmp types are handled by executor before propagating
	 * the error here.
	 */
	DUK_ASSERT(thr->heap->lj.type == DUK_LJ_TYPE_THROW);
	DUK_ASSERT_LJSTATE_SET(thr->heap);

	/* Either pointer may be NULL (at entry), so don't assert. */
	thr->heap->lj.jmpbuf_ptr = old_jmpbuf_ptr;

	/* XXX: callstack unwind may now throw an error when closing
	 * scopes; this is a sandboxing issue, described in:
	 * https://github.com/svaarala/duktape/issues/476
	 */
	/* XXX: "unwind to" primitive? */

	DUK_ASSERT(thr->callstack_top >= entry_callstack_top);
	while (thr->callstack_curr != entry_act) {
		DUK_ASSERT(thr->callstack_curr != NULL);
		duk_hthread_activation_unwind_norz(thr);
	}
	DUK_ASSERT(thr->callstack_top == entry_callstack_top);

	/* Switch active thread before any side effects to avoid a
	 * dangling curr_thread pointer.
	 */
	DUK_HEAP_SWITCH_THREAD(thr->heap, entry_curr_thread); /* may be NULL */
	thr->state = (duk_uint8_t) entry_thread_state;

	DUK_ASSERT(thr->heap->curr_thread == entry_curr_thread);
	DUK_ASSERT(thr->state == entry_thread_state);

	/* Restore valstack bottom. */
	thr->valstack_bottom = (duk_tval *) (void *) ((duk_uint8_t *) thr->valstack + entry_valstack_bottom_byteoff);

	/* [ ... | (crud) ] */

	/* XXX: ensure space in valstack (now relies on internal reserve)? */
	duk_push_tval(thr, &thr->heap->lj.value1);

	/* [ ... | (crud) errobj ] */

	DUK_ASSERT(duk_get_top(thr) >= 1); /* at least errobj must be on stack */

	duk__safe_call_adjust_valstack(thr, idx_retbase, num_stack_rets, 1); /* 1 = num actual 'return values' */

	/* [ ... | ] or [ ... | errobj (M * undefined)] where M = num_stack_rets - 1 */

	/* Reset longjmp state. */
	thr->heap->lj.type = DUK_LJ_TYPE_UNKNOWN;
	thr->heap->lj.iserror = 0;
	DUK_TVAL_SET_UNDEFINED_UPDREF_NORZ(thr, &thr->heap->lj.value1);
	DUK_TVAL_SET_UNDEFINED_UPDREF_NORZ(thr, &thr->heap->lj.value2);

	/* Error handling complete, remove side effect protections.  Caller
	 * will process pending finalizers.
	 */
#if defined(DUK_USE_ASSERTIONS)
	DUK_ASSERT(thr->heap->error_not_allowed == 1);
	thr->heap->error_not_allowed = 0;
#endif
	DUK_ASSERT(thr->heap->pf_prevent_count > 0);
	thr->heap->pf_prevent_count--;
	DUK_DD(DUK_DDPRINT("safe call error handled, pf_prevent_count updated to %ld", (long) thr->heap->pf_prevent_count));

	/* thr->ptr_curr_pc is restored by
	 * duk__handle_safe_call_shared_unwind() which is also used for
	 * success path.
	 */
}

DUK_LOCAL void duk__handle_safe_call_shared_unwind(duk_hthread *thr,
                                                   duk_idx_t idx_retbase,
                                                   duk_idx_t num_stack_rets,
#if defined(DUK_USE_ASSERTIONS)
                                                   duk_size_t entry_callstack_top,
#endif
                                                   duk_int_t entry_call_recursion_depth,
                                                   duk_hthread *entry_curr_thread,
                                                   duk_instr_t **entry_ptr_curr_pc) {
	DUK_ASSERT(thr != NULL);
	DUK_CTX_ASSERT_VALID(thr);
	DUK_UNREF(idx_retbase);
	DUK_UNREF(num_stack_rets);
	DUK_UNREF(entry_curr_thread);

	DUK_ASSERT(thr->callstack_top == entry_callstack_top);

	/* Restore entry thread executor curr_pc stack frame pointer.
	 * XXX: would be enough to do in error path only, should nest
	 * cleanly in success path.
	 */
	thr->ptr_curr_pc = entry_ptr_curr_pc;

	thr->heap->call_recursion_depth = entry_call_recursion_depth;

	/* stack discipline consistency check */
	DUK_ASSERT(duk_get_top(thr) == idx_retbase + num_stack_rets);

	/* A debugger forced interrupt check is not needed here, as
	 * problematic safe calls are not caused by side effects.
	 */

#if defined(DUK_USE_INTERRUPT_COUNTER) && defined(DUK_USE_DEBUG)
	duk__interrupt_fixup(thr, entry_curr_thread);
#endif
}

DUK_INTERNAL duk_int_t duk_handle_safe_call(duk_hthread *thr,
                                            duk_safe_call_function func,
                                            void *udata,
                                            duk_idx_t num_stack_args,
                                            duk_idx_t num_stack_rets) {
	duk_activation *entry_act;
	duk_size_t entry_valstack_bottom_byteoff;
#if defined(DUK_USE_ASSERTIONS)
	duk_size_t entry_valstack_end_byteoff;
	duk_size_t entry_callstack_top;
	duk_size_t entry_callstack_preventcount;
#endif
	duk_int_t entry_call_recursion_depth;
	duk_hthread *entry_curr_thread;
	duk_uint_fast8_t entry_thread_state;
	duk_instr_t **entry_ptr_curr_pc;
	duk_jmpbuf *old_jmpbuf_ptr = NULL;
	duk_jmpbuf our_jmpbuf;
	duk_idx_t idx_retbase;
	duk_int_t retval;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(duk_get_top(thr) >= num_stack_args); /* Caller ensures. */

	DUK_STATS_INC(thr->heap, stats_safecall_all);

	/* Value stack reserve handling: safe call assumes caller has reserved
	 * space for nrets (assuming optimal unwind processing).  Value stack
	 * reserve is not stored/restored as for normal calls because a safe
	 * call conceptually happens in the same activation.
	 */

	/* Careful with indices like '-x'; if 'x' is zero, it refers to bottom */
	entry_act = thr->callstack_curr;
	entry_valstack_bottom_byteoff = (duk_size_t) ((duk_uint8_t *) thr->valstack_bottom - (duk_uint8_t *) thr->valstack);
#if defined(DUK_USE_ASSERTIONS)
	entry_valstack_end_byteoff = (duk_size_t) ((duk_uint8_t *) thr->valstack_end - (duk_uint8_t *) thr->valstack);
	entry_callstack_top = thr->callstack_top;
	entry_callstack_preventcount = thr->callstack_preventcount;
#endif
	entry_call_recursion_depth = thr->heap->call_recursion_depth;
	entry_curr_thread = thr->heap->curr_thread; /* may be NULL if first call */
	entry_thread_state = thr->state;
	entry_ptr_curr_pc = thr->ptr_curr_pc; /* may be NULL */
	idx_retbase = duk_get_top(thr) - num_stack_args; /* not a valid stack index if num_stack_args == 0 */
	DUK_ASSERT(idx_retbase >= 0);

	DUK_ASSERT((duk_idx_t) (thr->valstack_top - thr->valstack_bottom) >= num_stack_args); /* Caller ensures. */
	DUK_ASSERT((duk_idx_t) (thr->valstack_end - (thr->valstack_bottom + idx_retbase)) >= num_stack_rets); /* Caller ensures. */

	/* Cannot portably debug print a function pointer, hence 'func' not printed! */
	DUK_DD(DUK_DDPRINT("duk_handle_safe_call: thr=%p, num_stack_args=%ld, num_stack_rets=%ld, "
	                   "valstack_top=%ld, idx_retbase=%ld, rec_depth=%ld/%ld, "
	                   "entry_act=%p, entry_valstack_bottom_byteoff=%ld, entry_call_recursion_depth=%ld, "
	                   "entry_curr_thread=%p, entry_thread_state=%ld",
	                   (void *) thr,
	                   (long) num_stack_args,
	                   (long) num_stack_rets,
	                   (long) duk_get_top(thr),
	                   (long) idx_retbase,
	                   (long) thr->heap->call_recursion_depth,
	                   (long) thr->heap->call_recursion_limit,
	                   (void *) entry_act,
	                   (long) entry_valstack_bottom_byteoff,
	                   (long) entry_call_recursion_depth,
	                   (void *) entry_curr_thread,
	                   (long) entry_thread_state));

	/* Setjmp catchpoint setup. */
	old_jmpbuf_ptr = thr->heap->lj.jmpbuf_ptr;
	thr->heap->lj.jmpbuf_ptr = &our_jmpbuf;

	/* Prevent yields for the duration of the safe call.  This only
	 * matters if the executor makes safe calls to functions that
	 * yield, this doesn't currently happen.
	 */
	thr->callstack_preventcount++;

#if defined(DUK_USE_CPP_EXCEPTIONS)
	try {
#else
	DUK_ASSERT(thr->heap->lj.jmpbuf_ptr == &our_jmpbuf);
	if (DUK_SETJMP(our_jmpbuf.jb) == 0) {
		/* Success path. */
#endif
		DUK_DDD(DUK_DDDPRINT("safe_call setjmp catchpoint setup complete"));

		duk__handle_safe_call_inner(thr,
		                            func,
		                            udata,
#if defined(DUK_USE_ASSERTIONS)
		                            entry_valstack_bottom_byteoff,
		                            entry_callstack_top,
#endif
		                            entry_curr_thread,
		                            entry_thread_state,
		                            idx_retbase,
		                            num_stack_rets);

		DUK_STATS_INC(thr->heap, stats_safecall_nothrow);

		/* Either pointer may be NULL (at entry), so don't assert */
		thr->heap->lj.jmpbuf_ptr = old_jmpbuf_ptr;

		/* If calls happen inside the safe call, these are restored by
		 * whatever calls are made.  Reserve cannot decrease.
		 */
		DUK_ASSERT(thr->callstack_curr == entry_act);
		DUK_ASSERT((duk_size_t) ((duk_uint8_t *) thr->valstack_end - (duk_uint8_t *) thr->valstack) >=
		           entry_valstack_end_byteoff);

		retval = DUK_EXEC_SUCCESS;
#if defined(DUK_USE_CPP_EXCEPTIONS)
	} catch (duk_internal_exception &exc) {
		DUK_UNREF(exc);
#else
	} else {
		/* Error path. */
#endif
		DUK_ASSERT((duk_size_t) ((duk_uint8_t *) thr->valstack_end - (duk_uint8_t *) thr->valstack) >=
		           entry_valstack_end_byteoff);

		DUK_STATS_INC(thr->heap, stats_safecall_throw);

		duk__handle_safe_call_error(thr,
		                            entry_act,
#if defined(DUK_USE_ASSERTIONS)
		                            entry_callstack_top,
#endif
		                            entry_curr_thread,
		                            entry_thread_state,
		                            idx_retbase,
		                            num_stack_rets,
		                            entry_valstack_bottom_byteoff,
		                            old_jmpbuf_ptr);

		retval = DUK_EXEC_ERROR;
	}
#if defined(DUK_USE_CPP_EXCEPTIONS)
	catch (duk_fatal_exception &exc) {
		DUK_D(DUK_DPRINT("rethrow duk_fatal_exception"));
		DUK_UNREF(exc);
		throw;
	} catch (std::exception &exc) {
		const char *what = exc.what();
		DUK_ASSERT((duk_size_t) ((duk_uint8_t *) thr->valstack_end - (duk_uint8_t *) thr->valstack) >=
		           entry_valstack_end_byteoff);
		DUK_STATS_INC(thr->heap, stats_safecall_throw);
		if (!what) {
			what = "unknown";
		}
		DUK_D(DUK_DPRINT("unexpected c++ std::exception (perhaps thrown by user code)"));
		try {
			DUK_ERROR_FMT1(thr,
			               DUK_ERR_TYPE_ERROR,
			               "caught invalid c++ std::exception '%s' (perhaps thrown by user code)",
			               what);
			DUK_WO_NORETURN(return 0;);
		} catch (duk_internal_exception exc) {
			DUK_D(DUK_DPRINT("caught api error thrown from unexpected c++ std::exception"));
			DUK_UNREF(exc);
			duk__handle_safe_call_error(thr,
			                            entry_act,
#if defined(DUK_USE_ASSERTIONS)
			                            entry_callstack_top,
#endif
			                            entry_curr_thread,
			                            entry_thread_state,
			                            idx_retbase,
			                            num_stack_rets,
			                            entry_valstack_bottom_byteoff,
			                            old_jmpbuf_ptr);
			retval = DUK_EXEC_ERROR;
		}
	} catch (...) {
		DUK_D(DUK_DPRINT("unexpected c++ exception (perhaps thrown by user code)"));
		DUK_ASSERT((duk_size_t) ((duk_uint8_t *) thr->valstack_end - (duk_uint8_t *) thr->valstack) >=
		           entry_valstack_end_byteoff);
		DUK_STATS_INC(thr->heap, stats_safecall_throw);
		try {
			DUK_ERROR_TYPE(thr, "caught invalid c++ exception (perhaps thrown by user code)");
			DUK_WO_NORETURN(return 0;);
		} catch (duk_internal_exception exc) {
			DUK_D(DUK_DPRINT("caught api error thrown from unexpected c++ exception"));
			DUK_UNREF(exc);
			duk__handle_safe_call_error(thr,
			                            entry_act,
#if defined(DUK_USE_ASSERTIONS)
			                            entry_callstack_top,
#endif
			                            entry_curr_thread,
			                            entry_thread_state,
			                            idx_retbase,
			                            num_stack_rets,
			                            entry_valstack_bottom_byteoff,
			                            old_jmpbuf_ptr);
			retval = DUK_EXEC_ERROR;
		}
	}
#endif

	DUK_ASSERT(thr->heap->lj.jmpbuf_ptr == old_jmpbuf_ptr); /* success/error path both do this */

	DUK_ASSERT_LJSTATE_UNSET(thr->heap);

	DUK_ASSERT((duk_size_t) ((duk_uint8_t *) thr->valstack_end - (duk_uint8_t *) thr->valstack) >= entry_valstack_end_byteoff);
	duk__handle_safe_call_shared_unwind(thr,
	                                    idx_retbase,
	                                    num_stack_rets,
#if defined(DUK_USE_ASSERTIONS)
	                                    entry_callstack_top,
#endif
	                                    entry_call_recursion_depth,
	                                    entry_curr_thread,
	                                    entry_ptr_curr_pc);

	/* Restore preventcount. */
	thr->callstack_preventcount--;
	DUK_ASSERT(thr->callstack_preventcount == entry_callstack_preventcount);

	/* Final asserts. */
	DUK_ASSERT(thr->callstack_curr == entry_act);
	DUK_ASSERT((duk_size_t) ((duk_uint8_t *) thr->valstack_bottom - (duk_uint8_t *) thr->valstack) ==
	           entry_valstack_bottom_byteoff);
	DUK_ASSERT((duk_size_t) ((duk_uint8_t *) thr->valstack_end - (duk_uint8_t *) thr->valstack) >= entry_valstack_end_byteoff);
	DUK_ASSERT(thr->callstack_top == entry_callstack_top);
	DUK_ASSERT(thr->heap->call_recursion_depth == entry_call_recursion_depth);
	DUK_ASSERT(thr->heap->curr_thread == entry_curr_thread);
	DUK_ASSERT(thr->state == entry_thread_state);
	DUK_ASSERT(thr->ptr_curr_pc == entry_ptr_curr_pc);
	DUK_ASSERT(duk_get_top(thr) == idx_retbase + num_stack_rets);
	DUK_ASSERT_LJSTATE_UNSET(thr->heap);

	/* Pending side effects. */
	DUK_REFZERO_CHECK_FAST(thr);

	return retval;
}

/*
 *  Property-based call (foo.noSuch()) error setup: replace target function
 *  on stack top with a hidden Symbol tagged non-callable wrapper object
 *  holding the error.  The error gets thrown in call handling at the
 *  proper spot to follow ECMAScript semantics.
 */

#if defined(DUK_USE_VERBOSE_ERRORS)
DUK_INTERNAL DUK_NOINLINE DUK_COLD void duk_call_setup_propcall_error(duk_hthread *thr, duk_tval *tv_base, duk_tval *tv_key) {
	const char *str_targ, *str_key, *str_base;
	duk_idx_t entry_top;

	entry_top = duk_get_top(thr);

	/* [ <nargs> target ] */

	/* Must stabilize pointers first.  tv_targ is already on stack top. */
	duk_push_tval(thr, tv_base);
	duk_push_tval(thr, tv_key);

	DUK_GC_TORTURE(thr->heap);

	duk_push_bare_object(thr);

	/* [ <nargs> target base key {} ] */

	/* We only push a wrapped error, replacing the call target (at
	 * idx_func) with the error to ensure side effects come out
	 * correctly:
	 * - Property read
	 * - Call argument evaluation
	 * - Callability check and error thrown
	 *
	 * A hidden Symbol on the wrapper object pushed above is used by
	 * call handling to figure out the error is to be thrown as is.
	 * It is CRITICAL that the hidden Symbol can never occur on a
	 * user visible object that may get thrown.
	 */

#if defined(DUK_USE_PARANOID_ERRORS)
	str_targ = duk_get_type_name(thr, -4);
	str_key = duk_get_type_name(thr, -2);
	str_base = duk_get_type_name(thr, -3);
	duk_push_error_object(thr,
	                      DUK_ERR_TYPE_ERROR | DUK_ERRCODE_FLAG_NOBLAME_FILELINE,
	                      "%s not callable (property %s of %s)",
	                      str_targ,
	                      str_key,
	                      str_base);
	duk_xdef_prop_stridx(thr, -2, DUK_STRIDX_INT_TARGET, DUK_PROPDESC_FLAGS_NONE); /* Marker property, reuse _Target. */
	/* [ <nargs> target base key { _Target: error } ] */
	duk_replace(thr, entry_top - 1);
#else
	str_targ = duk_push_string_readable(thr, -4);
	str_key = duk_push_string_readable(thr, -3);
	str_base = duk_push_string_readable(thr, -5);
	duk_push_error_object(thr,
	                      DUK_ERR_TYPE_ERROR | DUK_ERRCODE_FLAG_NOBLAME_FILELINE,
	                      "%s not callable (property %s of %s)",
	                      str_targ,
	                      str_key,
	                      str_base);
	/* [ <nargs> target base key {} str_targ str_key str_base error ] */
	duk_xdef_prop_stridx(thr, -5, DUK_STRIDX_INT_TARGET, DUK_PROPDESC_FLAGS_NONE); /* Marker property, reuse _Target. */
	/* [ <nargs> target base key { _Target: error } str_targ str_key str_base ] */
	duk_swap(thr, -4, entry_top - 1);
	/* [ <nargs> { _Target: error } base key target str_targ str_key str_base ] */
#endif

	/* [ <nregs> { _Target: error } <variable> */
	duk_set_top(thr, entry_top);

	/* [ <nregs> { _Target: error } */
	DUK_ASSERT(!duk_is_callable(thr, -1)); /* Critical so that call handling will throw the error. */
}
#endif /* DUK_USE_VERBOSE_ERRORS */
