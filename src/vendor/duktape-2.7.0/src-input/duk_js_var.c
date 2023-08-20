/*
 *  Identifier access and function closure handling.
 *
 *  Provides the primitives for slow path identifier accesses: GETVAR,
 *  PUTVAR, DELVAR, etc.  The fast path, direct register accesses, should
 *  be used for most identifier accesses.  Consequently, these slow path
 *  primitives should be optimized for maximum compactness.
 *
 *  ECMAScript environment records (declarative and object) are represented
 *  as internal objects with control keys.  Environment records have a
 *  parent record ("outer environment reference") which is represented by
 *  the implicit prototype for technical reasons (in other words, it is a
 *  convenient field).  The prototype chain is not followed in the ordinary
 *  sense for variable lookups.
 *
 *  See identifier-handling.rst for more details on the identifier algorithms
 *  and the internal representation.  See function-objects.rst for details on
 *  what function templates and instances are expected to look like.
 *
 *  Care must be taken to avoid duk_tval pointer invalidation caused by
 *  e.g. value stack or object resizing.
 *
 *  TODO: properties for function instances could be initialized much more
 *  efficiently by creating a property allocation for a certain size and
 *  filling in keys and values directly (and INCREFing both with "bulk incref"
 *  primitives.
 *
 *  XXX: duk_hobject_getprop() and duk_hobject_putprop() calls are a bit
 *  awkward (especially because they follow the prototype chain); rework
 *  if "raw" own property helpers are added.
 */

#include "duk_internal.h"

/*
 *  Local result type for duk__get_identifier_reference() lookup.
 */

typedef struct {
	duk_hobject *env;
	duk_hobject *holder; /* for object-bound identifiers */
	duk_tval *value; /* for register-bound and declarative env identifiers */
	duk_uint_t attrs; /* property attributes for identifier (relevant if value != NULL) */
	duk_bool_t has_this; /* for object-bound identifiers: provide 'this' binding */
} duk__id_lookup_result;

/*
 *  Create a new function object based on a "template function" which contains
 *  compiled bytecode, constants, etc, but lacks a lexical environment.
 *
 *  ECMAScript requires that each created closure is a separate object, with
 *  its own set of editable properties.  However, structured property values
 *  (such as the formal arguments list and the variable map) are shared.
 *  Also the bytecode, constants, and inner functions are shared.
 *
 *  See E5 Section 13.2 for detailed requirements on the function objects;
 *  there are no similar requirements for function "templates" which are an
 *  implementation dependent internal feature.  Also see function-objects.rst
 *  for a discussion on the function instance properties provided by this
 *  implementation.
 *
 *  Notes:
 *
 *   * Order of internal properties should match frequency of use, since the
 *     properties will be linearly scanned on lookup (functions usually don't
 *     have enough properties to warrant a hash part).
 *
 *   * The created closure is independent of its template; they do share the
 *     same 'data' buffer object, but the template object itself can be freed
 *     even if the closure object remains reachable.
 */

DUK_LOCAL void duk__inc_data_inner_refcounts(duk_hthread *thr, duk_hcompfunc *f) {
	duk_tval *tv, *tv_end;
	duk_hobject **funcs, **funcs_end;

	DUK_UNREF(thr);

	/* If function creation fails due to out-of-memory, the data buffer
	 * pointer may be NULL in some cases.  That's actually possible for
	 * GC code, but shouldn't be possible here because the incomplete
	 * function will be unwound from the value stack and never instantiated.
	 */
	DUK_ASSERT(DUK_HCOMPFUNC_GET_DATA(thr->heap, f) != NULL);

	tv = DUK_HCOMPFUNC_GET_CONSTS_BASE(thr->heap, f);
	tv_end = DUK_HCOMPFUNC_GET_CONSTS_END(thr->heap, f);
	while (tv < tv_end) {
		DUK_TVAL_INCREF(thr, tv);
		tv++;
	}

	funcs = DUK_HCOMPFUNC_GET_FUNCS_BASE(thr->heap, f);
	funcs_end = DUK_HCOMPFUNC_GET_FUNCS_END(thr->heap, f);
	while (funcs < funcs_end) {
		DUK_HEAPHDR_INCREF(thr, (duk_heaphdr *) *funcs);
		funcs++;
	}
}

/* Push a new closure on the stack.
 *
 * Note: if fun_temp has NEWENV, i.e. a new lexical and variable declaration
 * is created when the function is called, only outer_lex_env matters
 * (outer_var_env is ignored and may or may not be same as outer_lex_env).
 */

DUK_LOCAL const duk_uint16_t duk__closure_copy_proplist[] = {
	/* order: most frequent to least frequent */
	DUK_STRIDX_INT_VARMAP,
	DUK_STRIDX_INT_FORMALS,
#if defined(DUK_USE_PC2LINE)
	DUK_STRIDX_INT_PC2LINE,
#endif
#if defined(DUK_USE_FUNC_FILENAME_PROPERTY)
	DUK_STRIDX_FILE_NAME,
#endif
#if defined(DUK_USE_NONSTD_FUNC_SOURCE_PROPERTY)
	DUK_STRIDX_INT_SOURCE
#endif
};

DUK_INTERNAL
void duk_js_push_closure(duk_hthread *thr,
                         duk_hcompfunc *fun_temp,
                         duk_hobject *outer_var_env,
                         duk_hobject *outer_lex_env,
                         duk_bool_t add_auto_proto) {
	duk_hcompfunc *fun_clos;
	duk_harray *formals;
	duk_small_uint_t i;
	duk_uint_t len_value;

	DUK_ASSERT(fun_temp != NULL);
	DUK_ASSERT(DUK_HCOMPFUNC_GET_DATA(thr->heap, fun_temp) != NULL);
	DUK_ASSERT(DUK_HCOMPFUNC_GET_FUNCS(thr->heap, fun_temp) != NULL);
	DUK_ASSERT(DUK_HCOMPFUNC_GET_BYTECODE(thr->heap, fun_temp) != NULL);
	DUK_ASSERT(outer_var_env != NULL);
	DUK_ASSERT(outer_lex_env != NULL);
	DUK_UNREF(len_value);

	DUK_STATS_INC(thr->heap, stats_envrec_pushclosure);

	fun_clos = duk_push_hcompfunc(thr);
	DUK_ASSERT(fun_clos != NULL);
	DUK_ASSERT(DUK_HOBJECT_GET_PROTOTYPE(thr->heap, (duk_hobject *) fun_clos) == thr->builtins[DUK_BIDX_FUNCTION_PROTOTYPE]);

	duk_push_hobject(thr, &fun_temp->obj); /* -> [ ... closure template ] */

	DUK_ASSERT(DUK_HOBJECT_IS_COMPFUNC((duk_hobject *) fun_clos));
	DUK_ASSERT(DUK_HCOMPFUNC_GET_DATA(thr->heap, fun_clos) == NULL);
	DUK_ASSERT(DUK_HCOMPFUNC_GET_FUNCS(thr->heap, fun_clos) == NULL);
	DUK_ASSERT(DUK_HCOMPFUNC_GET_BYTECODE(thr->heap, fun_clos) == NULL);

	DUK_HCOMPFUNC_SET_DATA(thr->heap, fun_clos, DUK_HCOMPFUNC_GET_DATA(thr->heap, fun_temp));
	DUK_HCOMPFUNC_SET_FUNCS(thr->heap, fun_clos, DUK_HCOMPFUNC_GET_FUNCS(thr->heap, fun_temp));
	DUK_HCOMPFUNC_SET_BYTECODE(thr->heap, fun_clos, DUK_HCOMPFUNC_GET_BYTECODE(thr->heap, fun_temp));

	/* Note: all references inside 'data' need to get their refcounts
	 * upped too.  This is the case because refcounts are decreased
	 * through every function referencing 'data' independently.
	 */

	DUK_HBUFFER_INCREF(thr, DUK_HCOMPFUNC_GET_DATA(thr->heap, fun_clos));
	duk__inc_data_inner_refcounts(thr, fun_temp);

	fun_clos->nregs = fun_temp->nregs;
	fun_clos->nargs = fun_temp->nargs;
#if defined(DUK_USE_DEBUGGER_SUPPORT)
	fun_clos->start_line = fun_temp->start_line;
	fun_clos->end_line = fun_temp->end_line;
#endif

	DUK_ASSERT(DUK_HCOMPFUNC_GET_DATA(thr->heap, fun_clos) != NULL);
	DUK_ASSERT(DUK_HCOMPFUNC_GET_FUNCS(thr->heap, fun_clos) != NULL);
	DUK_ASSERT(DUK_HCOMPFUNC_GET_BYTECODE(thr->heap, fun_clos) != NULL);

	/* XXX: Could also copy from template, but there's no way to have any
	 * other value here now (used code has no access to the template).
	 * Prototype is set by duk_push_hcompfunc().
	 */
	DUK_ASSERT(DUK_HOBJECT_GET_PROTOTYPE(thr->heap, &fun_clos->obj) == thr->builtins[DUK_BIDX_FUNCTION_PROTOTYPE]);
#if 0
	DUK_HOBJECT_SET_PROTOTYPE_UPDREF(thr, &fun_clos->obj, thr->builtins[DUK_BIDX_FUNCTION_PROTOTYPE]);
#endif

	/* Copy duk_hobject flags as is from the template using a mask.
	 * Leave out duk_heaphdr owned flags just in case (e.g. if there's
	 * some GC flag or similar).  Some flags can then be adjusted
	 * separately if necessary.
	 */

	/* DUK_HEAPHDR_SET_FLAGS() masks changes to non-duk_heaphdr flags only. */
	DUK_HEAPHDR_SET_FLAGS((duk_heaphdr *) fun_clos, DUK_HEAPHDR_GET_FLAGS_RAW((duk_heaphdr *) fun_temp));
	DUK_DD(DUK_DDPRINT("fun_temp heaphdr flags: 0x%08lx, fun_clos heaphdr flags: 0x%08lx",
	                   (unsigned long) DUK_HEAPHDR_GET_FLAGS_RAW((duk_heaphdr *) fun_temp),
	                   (unsigned long) DUK_HEAPHDR_GET_FLAGS_RAW((duk_heaphdr *) fun_clos)));

	DUK_ASSERT(DUK_HOBJECT_HAS_EXTENSIBLE(&fun_clos->obj));
	DUK_ASSERT(!DUK_HOBJECT_HAS_BOUNDFUNC(&fun_clos->obj));
	DUK_ASSERT(DUK_HOBJECT_HAS_COMPFUNC(&fun_clos->obj));
	DUK_ASSERT(!DUK_HOBJECT_HAS_NATFUNC(&fun_clos->obj));
	DUK_ASSERT(!DUK_HOBJECT_IS_THREAD(&fun_clos->obj));
	/* DUK_HOBJECT_FLAG_ARRAY_PART: don't care */
	/* DUK_HOBJECT_FLAG_NEWENV: handled below */
	DUK_ASSERT(!DUK_HOBJECT_HAS_EXOTIC_ARRAY(&fun_clos->obj));
	DUK_ASSERT(!DUK_HOBJECT_HAS_EXOTIC_STRINGOBJ(&fun_clos->obj));
	DUK_ASSERT(!DUK_HOBJECT_HAS_EXOTIC_ARGUMENTS(&fun_clos->obj));

	if (!DUK_HOBJECT_HAS_CONSTRUCTABLE(&fun_clos->obj)) {
		/* If the template is not constructable don't add an automatic
		 * .prototype property.  This is the case for e.g. ES2015 object
		 * literal getters/setters and method definitions.
		 */
		add_auto_proto = 0;
	}

	/*
	 *  Setup environment record properties based on the template and
	 *  its flags.
	 *
	 *  If DUK_HOBJECT_HAS_NEWENV(fun_temp) is true, the environment
	 *  records represent identifiers "outside" the function; the
	 *  "inner" environment records are created on demand.  Otherwise,
	 *  the environment records are those that will be directly used
	 *  (e.g. for declarations).
	 *
	 *  _Lexenv is always set; _Varenv defaults to _Lexenv if missing,
	 *  so _Varenv is only set if _Lexenv != _Varenv.
	 *
	 *  This is relatively complex, see doc/identifier-handling.rst.
	 */

	if (DUK_HOBJECT_HAS_NEWENV(&fun_clos->obj)) {
#if defined(DUK_USE_FUNC_NAME_PROPERTY)
		if (DUK_HOBJECT_HAS_NAMEBINDING(&fun_clos->obj)) {
			duk_hobject *proto;
			duk_hdecenv *new_env;

			/*
			 *  Named function expression, name needs to be bound
			 *  in an intermediate environment record.  The "outer"
			 *  lexical/variable environment will thus be:
			 *
			 *  a) { funcname: <func>, __prototype: outer_lex_env }
			 *  b) { funcname: <func>, __prototype:  <globalenv> }  (if outer_lex_env missing)
			 */

			if (outer_lex_env) {
				proto = outer_lex_env;
			} else {
				proto = thr->builtins[DUK_BIDX_GLOBAL_ENV];
			}

			/* -> [ ... closure template env ] */
			new_env =
			    duk_hdecenv_alloc(thr,
			                      DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_DECENV));
			DUK_ASSERT(new_env != NULL);
			duk_push_hobject(thr, (duk_hobject *) new_env);

			DUK_ASSERT(DUK_HOBJECT_GET_PROTOTYPE(thr->heap, (duk_hobject *) new_env) == NULL);
			DUK_HOBJECT_SET_PROTOTYPE(thr->heap, (duk_hobject *) new_env, proto);
			DUK_HOBJECT_INCREF_ALLOWNULL(thr, proto);

			DUK_ASSERT(new_env->thread == NULL); /* Closed. */
			DUK_ASSERT(new_env->varmap == NULL);

			/* It's important that duk_xdef_prop() is a 'raw define' so that any
			 * properties in an ancestor are never an issue (they should never be
			 * e.g. non-writable, but just in case).
			 *
			 * Because template objects are not visible to user code, the case
			 * where .name is missing shouldn't happen in practice.  It it does,
			 * the name 'undefined' gets bound and maps to the closure (which is
			 * a bit odd, but safe).
			 */
			(void) duk_get_prop_stridx_short(thr, -2, DUK_STRIDX_NAME);
			/* -> [ ... closure template env funcname ] */
			duk_dup_m4(thr); /* -> [ ... closure template env funcname closure ] */
			duk_xdef_prop(thr, -3, DUK_PROPDESC_FLAGS_NONE); /* -> [ ... closure template env ] */
			/* env[funcname] = closure */

			/* [ ... closure template env ] */

			DUK_HCOMPFUNC_SET_LEXENV(thr->heap, fun_clos, (duk_hobject *) new_env);
			DUK_HCOMPFUNC_SET_VARENV(thr->heap, fun_clos, (duk_hobject *) new_env);
			DUK_HOBJECT_INCREF(thr, (duk_hobject *) new_env);
			DUK_HOBJECT_INCREF(thr, (duk_hobject *) new_env);
			duk_pop_unsafe(thr);

			/* [ ... closure template ] */
		} else
#endif /* DUK_USE_FUNC_NAME_PROPERTY */
		{
			/*
			 *  Other cases (function declaration, anonymous function expression,
			 *  strict direct eval code).  The "outer" environment will be whatever
			 *  the caller gave us.
			 */

			DUK_HCOMPFUNC_SET_LEXENV(thr->heap, fun_clos, outer_lex_env);
			DUK_HCOMPFUNC_SET_VARENV(thr->heap, fun_clos, outer_lex_env);
			DUK_HOBJECT_INCREF(thr, outer_lex_env);
			DUK_HOBJECT_INCREF(thr, outer_lex_env);

			/* [ ... closure template ] */
		}
	} else {
		/*
		 *  Function gets no new environment when called.  This is the
		 *  case for global code, indirect eval code, and non-strict
		 *  direct eval code.  There is no direct correspondence to the
		 *  E5 specification, as global/eval code is not exposed as a
		 *  function.
		 */

		DUK_ASSERT(!DUK_HOBJECT_HAS_NAMEBINDING(&fun_temp->obj));

		DUK_HCOMPFUNC_SET_LEXENV(thr->heap, fun_clos, outer_lex_env);
		DUK_HCOMPFUNC_SET_VARENV(thr->heap, fun_clos, outer_var_env);
		DUK_HOBJECT_INCREF(thr, outer_lex_env); /* NULLs not allowed; asserted on entry */
		DUK_HOBJECT_INCREF(thr, outer_var_env);
	}
	DUK_DDD(DUK_DDDPRINT("closure varenv -> %!ipO, lexenv -> %!ipO",
	                     (duk_heaphdr *) fun_clos->var_env,
	                     (duk_heaphdr *) fun_clos->lex_env));

	/* Call handling assumes this for all callable closures. */
	DUK_ASSERT(DUK_HCOMPFUNC_GET_LEXENV(thr->heap, fun_clos) != NULL);
	DUK_ASSERT(DUK_HCOMPFUNC_GET_VARENV(thr->heap, fun_clos) != NULL);

	/*
	 *  Copy some internal properties directly
	 *
	 *  The properties will be non-writable and non-enumerable, but
	 *  configurable.
	 *
	 *  Function templates are bare objects, so inheritance of internal
	 *  Symbols is not an issue here even when using ordinary property
	 *  reads.  The function instance created is not bare, so internal
	 *  Symbols must be defined without inheritance checks.
	 */

	/* [ ... closure template ] */

	DUK_DDD(DUK_DDDPRINT("copying properties: closure=%!iT, template=%!iT",
	                     (duk_tval *) duk_get_tval(thr, -2),
	                     (duk_tval *) duk_get_tval(thr, -1)));

	for (i = 0; i < (duk_small_uint_t) (sizeof(duk__closure_copy_proplist) / sizeof(duk_uint16_t)); i++) {
		duk_small_int_t stridx = (duk_small_int_t) duk__closure_copy_proplist[i];
		if (duk_xget_owndataprop_stridx_short(thr, -1, stridx)) {
			/* [ ... closure template val ] */
			DUK_DDD(DUK_DDDPRINT("copying property, stridx=%ld -> found", (long) stridx));
			duk_xdef_prop_stridx_short(thr, -3, stridx, DUK_PROPDESC_FLAGS_C);
		} else {
			DUK_DDD(DUK_DDDPRINT("copying property, stridx=%ld -> not found", (long) stridx));
			duk_pop_unsafe(thr);
		}
	}

	/*
	 *  "length" maps to number of formals (E5 Section 13.2) for function
	 *  declarations/expressions (non-bound functions).  Note that 'nargs'
	 *  is NOT necessarily equal to the number of arguments.  Use length
	 *  of _Formals; if missing, assume nargs matches .length.
	 */

	/* [ ... closure template ] */

	formals = duk_hobject_get_formals(thr, (duk_hobject *) fun_temp);
	if (formals) {
		len_value = (duk_uint_t) formals->length;
		DUK_DD(DUK_DDPRINT("closure length from _Formals -> %ld", (long) len_value));
	} else {
		len_value = fun_temp->nargs;
		DUK_DD(DUK_DDPRINT("closure length defaulted from nargs -> %ld", (long) len_value));
	}

	duk_push_uint(thr, len_value); /* [ ... closure template len_value ] */
	duk_xdef_prop_stridx_short(thr, -3, DUK_STRIDX_LENGTH, DUK_PROPDESC_FLAGS_C);

	/*
	 *  "prototype" is, by default, a fresh object with the "constructor"
	 *  property.
	 *
	 *  Note that this creates a circular reference for every function
	 *  instance (closure) which prevents refcount-based collection of
	 *  function instances.
	 *
	 *  XXX: Try to avoid creating the default prototype object, because
	 *  many functions are not used as constructors and the default
	 *  prototype is unnecessary.  Perhaps it could be created on-demand
	 *  when it is first accessed?
	 */

	/* [ ... closure template ] */

	if (add_auto_proto) {
		duk_push_object(thr); /* -> [ ... closure template newobj ] */
		duk_dup_m3(thr); /* -> [ ... closure template newobj closure ] */
		duk_xdef_prop_stridx_short(thr,
		                           -2,
		                           DUK_STRIDX_CONSTRUCTOR,
		                           DUK_PROPDESC_FLAGS_WC); /* -> [ ... closure template newobj ] */
		duk_compact(thr, -1); /* compact the prototype */
		duk_xdef_prop_stridx_short(thr, -3, DUK_STRIDX_PROTOTYPE, DUK_PROPDESC_FLAGS_W); /* -> [ ... closure template ] */
	}

	/*
	 *  "arguments" and "caller" must be mapped to throwers for strict
	 *  mode and bound functions (E5 Section 15.3.5).
	 *
	 *  XXX: This is expensive to have for every strict function instance.
	 *  Try to implement as virtual properties or on-demand created properties.
	 */

	/* [ ... closure template ] */

	if (DUK_HOBJECT_HAS_STRICT(&fun_clos->obj)) {
		duk_xdef_prop_stridx_thrower(thr, -2, DUK_STRIDX_CALLER);
		duk_xdef_prop_stridx_thrower(thr, -2, DUK_STRIDX_LC_ARGUMENTS);
	} else {
#if defined(DUK_USE_NONSTD_FUNC_CALLER_PROPERTY)
		DUK_DDD(DUK_DDDPRINT("function is non-strict and non-standard 'caller' property in use, add initial 'null' value"));
		duk_push_null(thr);
		duk_xdef_prop_stridx_short(thr, -3, DUK_STRIDX_CALLER, DUK_PROPDESC_FLAGS_NONE);
#else
		DUK_DDD(DUK_DDDPRINT("function is non-strict and non-standard 'caller' property not used"));
#endif
	}

	/*
	 *  "name" used to be non-standard but is now defined by ES2015.
	 *  In ES2015/ES2016 the .name property is configurable.
	 */

	/* [ ... closure template ] */

#if defined(DUK_USE_FUNC_NAME_PROPERTY)
	/* XXX: Look for own property only; doesn't matter much because
	 * templates are bare objects.
	 */
	if (duk_get_prop_stridx_short(thr, -1, DUK_STRIDX_NAME)) {
		/* [ ... closure template name ] */
		DUK_ASSERT(duk_is_string(thr, -1));
		DUK_DD(DUK_DDPRINT("setting function instance name to %!T", duk_get_tval(thr, -1)));
		duk_xdef_prop_stridx_short(thr, -3, DUK_STRIDX_NAME, DUK_PROPDESC_FLAGS_C); /* -> [ ... closure template ] */
	} else {
		/* Anonymous functions don't have a .name in ES2015, so don't set
		 * it on the instance either.  The instance will then inherit
		 * it from Function.prototype.name.
		 */
		DUK_DD(DUK_DDPRINT("not setting function instance .name"));
		duk_pop_unsafe(thr);
	}
#endif

	/*
	 *  Compact the closure, in most cases no properties will be added later.
	 *  Also, without this the closures end up having unused property slots
	 *  (e.g. in Duktape 0.9.0, 8 slots would be allocated and only 7 used).
	 *  A better future solution would be to allocate the closure directly
	 *  to correct size (and setup the properties directly without going
	 *  through the API).
	 */

	duk_compact(thr, -2);

	/*
	 *  Some assertions (E5 Section 13.2).
	 */

	DUK_ASSERT(DUK_HOBJECT_GET_CLASS_NUMBER(&fun_clos->obj) == DUK_HOBJECT_CLASS_FUNCTION);
	DUK_ASSERT(DUK_HOBJECT_GET_PROTOTYPE(thr->heap, &fun_clos->obj) == thr->builtins[DUK_BIDX_FUNCTION_PROTOTYPE]);
	DUK_ASSERT(DUK_HOBJECT_HAS_EXTENSIBLE(&fun_clos->obj));
	DUK_ASSERT(duk_has_prop_stridx(thr, -2, DUK_STRIDX_LENGTH) != 0);
	DUK_ASSERT(add_auto_proto == 0 || duk_has_prop_stridx(thr, -2, DUK_STRIDX_PROTOTYPE) != 0);
	/* May be missing .name */
	DUK_ASSERT(!DUK_HOBJECT_HAS_STRICT(&fun_clos->obj) || duk_has_prop_stridx(thr, -2, DUK_STRIDX_CALLER) != 0);
	DUK_ASSERT(!DUK_HOBJECT_HAS_STRICT(&fun_clos->obj) || duk_has_prop_stridx(thr, -2, DUK_STRIDX_LC_ARGUMENTS) != 0);

	/*
	 *  Finish
	 */

	/* [ ... closure template ] */

	DUK_DDD(DUK_DDDPRINT("created function instance: template=%!iT -> closure=%!iT",
	                     (duk_tval *) duk_get_tval(thr, -1),
	                     (duk_tval *) duk_get_tval(thr, -2)));

	duk_pop_unsafe(thr);

	/* [ ... closure ] */
}

/*
 *  Delayed activation environment record initialization (for functions
 *  with NEWENV).
 *
 *  The non-delayed initialization is handled by duk_handle_call().
 */

DUK_LOCAL void duk__preallocate_env_entries(duk_hthread *thr, duk_hobject *varmap, duk_hobject *env) {
	duk_uint_fast32_t i;

	for (i = 0; i < (duk_uint_fast32_t) DUK_HOBJECT_GET_ENEXT(varmap); i++) {
		duk_hstring *key;

		key = DUK_HOBJECT_E_GET_KEY(thr->heap, varmap, i);
		DUK_ASSERT(key != NULL); /* assume keys are compact in _Varmap */
		DUK_ASSERT(!DUK_HOBJECT_E_SLOT_IS_ACCESSOR(thr->heap, varmap, i)); /* assume plain values */

		/* Predefine as 'undefined' to reserve a property slot.
		 * This makes the unwind process (where register values
		 * are copied to the env object) safe against throwing.
		 *
		 * XXX: This could be made much faster by creating the
		 * property table directly.
		 */
		duk_push_undefined(thr);
		DUK_DDD(DUK_DDDPRINT("preallocate env entry for key %!O", key));
		duk_hobject_define_property_internal(thr, env, key, DUK_PROPDESC_FLAGS_WE);
	}
}

/* shared helper */
DUK_INTERNAL
duk_hobject *duk_create_activation_environment_record(duk_hthread *thr, duk_hobject *func, duk_size_t bottom_byteoff) {
	duk_hdecenv *env;
	duk_hobject *parent;
	duk_hcompfunc *f;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(func != NULL);

	DUK_STATS_INC(thr->heap, stats_envrec_create);

	f = (duk_hcompfunc *) func;
	parent = DUK_HCOMPFUNC_GET_LEXENV(thr->heap, f);
	if (!parent) {
		parent = thr->builtins[DUK_BIDX_GLOBAL_ENV];
	}

	env = duk_hdecenv_alloc(thr, DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_DECENV));
	DUK_ASSERT(env != NULL);
	duk_push_hobject(thr, (duk_hobject *) env);

	DUK_ASSERT(DUK_HOBJECT_GET_PROTOTYPE(thr->heap, (duk_hobject *) env) == NULL);
	DUK_HOBJECT_SET_PROTOTYPE(thr->heap, (duk_hobject *) env, parent);
	DUK_HOBJECT_INCREF_ALLOWNULL(thr, parent); /* parent env is the prototype */

	/* open scope information, for compiled functions only */

	DUK_ASSERT(env->thread == NULL);
	DUK_ASSERT(env->varmap == NULL);
	DUK_ASSERT(env->regbase_byteoff == 0);
	if (DUK_HOBJECT_IS_COMPFUNC(func)) {
		duk_hobject *varmap;

		varmap = duk_hobject_get_varmap(thr, func);
		if (varmap != NULL) {
			env->varmap = varmap;
			DUK_HOBJECT_INCREF(thr, varmap);
			env->thread = thr;
			DUK_HTHREAD_INCREF(thr, thr);
			env->regbase_byteoff = bottom_byteoff;

			/* Preallocate env property table to avoid potential
			 * for out-of-memory on unwind when the env is closed.
			 */
			duk__preallocate_env_entries(thr, varmap, (duk_hobject *) env);
		} else {
			/* If function has no _Varmap, leave the environment closed. */
			DUK_ASSERT(env->thread == NULL);
			DUK_ASSERT(env->varmap == NULL);
			DUK_ASSERT(env->regbase_byteoff == 0);
		}
	}

	return (duk_hobject *) env;
}

DUK_INTERNAL
void duk_js_init_activation_environment_records_delayed(duk_hthread *thr, duk_activation *act) {
	duk_hobject *func;
	duk_hobject *env;

	DUK_ASSERT(thr != NULL);
	func = DUK_ACT_GET_FUNC(act);
	DUK_ASSERT(func != NULL);
	DUK_ASSERT(!DUK_HOBJECT_HAS_BOUNDFUNC(func)); /* bound functions are never in act 'func' */

	/*
	 *  Delayed initialization only occurs for 'NEWENV' functions.
	 */

	DUK_ASSERT(DUK_HOBJECT_HAS_NEWENV(func));
	DUK_ASSERT(act->lex_env == NULL);
	DUK_ASSERT(act->var_env == NULL);

	DUK_STATS_INC(thr->heap, stats_envrec_delayedcreate);

	env = duk_create_activation_environment_record(thr, func, act->bottom_byteoff);
	DUK_ASSERT(env != NULL);
	/* 'act' is a stable pointer, so still OK. */

	DUK_DDD(DUK_DDDPRINT("created delayed fresh env: %!ipO", (duk_heaphdr *) env));
#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 2)
	{
		duk_hobject *p = env;
		while (p) {
			DUK_DDD(DUK_DDDPRINT("  -> %!ipO", (duk_heaphdr *) p));
			p = DUK_HOBJECT_GET_PROTOTYPE(thr->heap, p);
		}
	}
#endif

	act->lex_env = env;
	act->var_env = env;
	DUK_HOBJECT_INCREF(thr, env); /* XXX: incref by count (here 2 times) */
	DUK_HOBJECT_INCREF(thr, env);

	duk_pop_unsafe(thr);
}

/*
 *  Closing environment records.
 *
 *  The environment record MUST be closed with the thread where its activation
 *  is; i.e. if 'env' is open, 'thr' must match env->thread, and the regbase
 *  and varmap must still be valid.  On entry, 'env' must be reachable.
 */

DUK_INTERNAL void duk_js_close_environment_record(duk_hthread *thr, duk_hobject *env) {
	duk_uint_fast32_t i;
	duk_hobject *varmap;
	duk_hstring *key;
	duk_tval *tv;
	duk_uint_t regnum;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(env != NULL);

	if (DUK_UNLIKELY(!DUK_HOBJECT_IS_DECENV(env))) {
		DUK_DDD(DUK_DDDPRINT("env not a declarative record: %!iO", (duk_heaphdr *) env));
		return;
	}

	varmap = ((duk_hdecenv *) env)->varmap;
	if (varmap == NULL) {
		DUK_DDD(DUK_DDDPRINT("env already closed: %!iO", (duk_heaphdr *) env));

		return;
	}
	DUK_ASSERT(((duk_hdecenv *) env)->thread != NULL);
	DUK_HDECENV_ASSERT_VALID((duk_hdecenv *) env);

	DUK_DDD(DUK_DDDPRINT("closing env: %!iO", (duk_heaphdr *) env));
	DUK_DDD(DUK_DDDPRINT("varmap: %!O", (duk_heaphdr *) varmap));

	/* Env must be closed in the same thread as where it runs. */
	DUK_ASSERT(((duk_hdecenv *) env)->thread == thr);

	/* XXX: additional conditions when to close variables? we don't want to do it
	 * unless the environment may have "escaped" (referenced in a function closure).
	 * With delayed environments, the existence is probably good enough of a check.
	 */

	/* Note: we rely on the _Varmap having a bunch of nice properties, like:
	 *  - being compacted and unmodified during this process
	 *  - not containing an array part
	 *  - having correct value types
	 */

	DUK_DDD(DUK_DDDPRINT("copying bound register values, %ld bound regs", (long) DUK_HOBJECT_GET_ENEXT(varmap)));

	/* Copy over current variable values from value stack to the
	 * environment record.  The scope object is empty but may
	 * inherit from another scope which has conflicting names.
	 */

	/* XXX: Do this using a once allocated entry area, no side effects.
	 * Hash part would need special treatment however (maybe copy, and
	 * then realloc with hash part if large enough).
	 */
	for (i = 0; i < (duk_uint_fast32_t) DUK_HOBJECT_GET_ENEXT(varmap); i++) {
		duk_size_t regbase_byteoff;

		key = DUK_HOBJECT_E_GET_KEY(thr->heap, varmap, i);
		DUK_ASSERT(key != NULL); /* assume keys are compact in _Varmap */
		DUK_ASSERT(!DUK_HOBJECT_E_SLOT_IS_ACCESSOR(thr->heap, varmap, i)); /* assume plain values */

		tv = DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(thr->heap, varmap, i);
		DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv));
		DUK_ASSERT(DUK_TVAL_GET_NUMBER(tv) <= (duk_double_t) DUK_UINT32_MAX); /* limits */
#if defined(DUK_USE_FASTINT)
		DUK_ASSERT(DUK_TVAL_IS_FASTINT(tv));
		regnum = (duk_uint_t) DUK_TVAL_GET_FASTINT_U32(tv);
#else
		regnum = (duk_uint_t) DUK_TVAL_GET_NUMBER(tv);
#endif

		regbase_byteoff = ((duk_hdecenv *) env)->regbase_byteoff;
		DUK_ASSERT((duk_uint8_t *) thr->valstack + regbase_byteoff + sizeof(duk_tval) * regnum >=
		           (duk_uint8_t *) thr->valstack);
		DUK_ASSERT((duk_uint8_t *) thr->valstack + regbase_byteoff + sizeof(duk_tval) * regnum <
		           (duk_uint8_t *) thr->valstack_top);

		/* Write register value into env as named properties.
		 * If property already exists, overwrites silently.
		 * Property is writable, but not deletable (not configurable
		 * in terms of property attributes).
		 *
		 * This property write must not throw because we're unwinding
		 * and unwind code is not allowed to throw at present.  The
		 * call itself has no such guarantees, but we've preallocated
		 * entries for each property when the env was created, so no
		 * out-of-memory error should be possible.  If this guarantee
		 * is not provided, problems like GH-476 may happen.
		 */
		duk_push_tval(thr,
		              (duk_tval *) (void *) ((duk_uint8_t *) thr->valstack + regbase_byteoff + sizeof(duk_tval) * regnum));
		DUK_DDD(DUK_DDDPRINT("closing identifier %!O -> reg %ld, value %!T",
		                     (duk_heaphdr *) key,
		                     (long) regnum,
		                     (duk_tval *) duk_get_tval(thr, -1)));
		duk_hobject_define_property_internal(thr, env, key, DUK_PROPDESC_FLAGS_WE);
	}

	/* NULL atomically to avoid inconsistent state + side effects. */
	DUK_HOBJECT_DECREF_NORZ(thr, ((duk_hdecenv *) env)->thread);
	DUK_HOBJECT_DECREF_NORZ(thr, ((duk_hdecenv *) env)->varmap);
	((duk_hdecenv *) env)->thread = NULL;
	((duk_hdecenv *) env)->varmap = NULL;

	DUK_DDD(DUK_DDDPRINT("env after closing: %!O", (duk_heaphdr *) env));
}

/*
 *  GETIDREF: a GetIdentifierReference-like helper.
 *
 *  Provides a parent traversing lookup and a single level lookup
 *  (for HasBinding).
 *
 *  Instead of returning the value, returns a bunch of values allowing
 *  the caller to read, write, or delete the binding.  Value pointers
 *  are duk_tval pointers which can be mutated directly as long as
 *  refcounts are properly updated.  Note that any operation which may
 *  reallocate valstacks or compact objects may invalidate the returned
 *  duk_tval (but not object) pointers, so caller must be very careful.
 *
 *  If starting environment record 'env' is given, 'act' is ignored.
 *  However, if 'env' is NULL, the caller may identify, in 'act', an
 *  activation which hasn't had its declarative environment initialized
 *  yet.  The activation registers are then looked up, and its parent
 *  traversed normally.
 *
 *  The 'out' structure values are only valid if the function returns
 *  success (non-zero).
 */

/* lookup name from an open declarative record's registers */
DUK_LOCAL
duk_bool_t duk__getid_open_decl_env_regs(duk_hthread *thr, duk_hstring *name, duk_hdecenv *env, duk__id_lookup_result *out) {
	duk_tval *tv;
	duk_size_t reg_rel;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(name != NULL);
	DUK_ASSERT(env != NULL);
	DUK_ASSERT(out != NULL);

	DUK_ASSERT(DUK_HOBJECT_IS_DECENV((duk_hobject *) env));
	DUK_HDECENV_ASSERT_VALID(env);

	if (env->thread == NULL) {
		/* already closed */
		return 0;
	}
	DUK_ASSERT(env->varmap != NULL);

	tv = duk_hobject_find_entry_tval_ptr(thr->heap, env->varmap, name);
	if (DUK_UNLIKELY(tv == NULL)) {
		return 0;
	}

	DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv));
	DUK_ASSERT(DUK_TVAL_GET_NUMBER(tv) <= (duk_double_t) DUK_UINT32_MAX); /* limits */
#if defined(DUK_USE_FASTINT)
	DUK_ASSERT(DUK_TVAL_IS_FASTINT(tv));
	reg_rel = (duk_size_t) DUK_TVAL_GET_FASTINT_U32(tv);
#else
	reg_rel = (duk_size_t) DUK_TVAL_GET_NUMBER(tv);
#endif
	DUK_ASSERT_DISABLE(reg_rel >= 0); /* unsigned */

	tv = (duk_tval *) (void *) ((duk_uint8_t *) env->thread->valstack + env->regbase_byteoff + sizeof(duk_tval) * reg_rel);
	DUK_ASSERT(tv >= env->thread->valstack && tv < env->thread->valstack_end); /* XXX: more accurate? */

	out->value = tv;
	out->attrs = DUK_PROPDESC_FLAGS_W; /* registers are mutable, non-deletable */
	out->env = (duk_hobject *) env;
	out->holder = NULL;
	out->has_this = 0;
	return 1;
}

/* lookup name from current activation record's functions' registers */
DUK_LOCAL
duk_bool_t duk__getid_activation_regs(duk_hthread *thr, duk_hstring *name, duk_activation *act, duk__id_lookup_result *out) {
	duk_tval *tv;
	duk_hobject *func;
	duk_hobject *varmap;
	duk_size_t reg_rel;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(name != NULL);
	DUK_ASSERT(act != NULL);
	DUK_ASSERT(out != NULL);

	func = DUK_ACT_GET_FUNC(act);
	DUK_ASSERT(func != NULL);
	DUK_ASSERT(DUK_HOBJECT_HAS_NEWENV(func));

	if (!DUK_HOBJECT_IS_COMPFUNC(func)) {
		return 0;
	}

	/* XXX: move varmap to duk_hcompfunc struct field? */
	varmap = duk_hobject_get_varmap(thr, func);
	if (!varmap) {
		return 0;
	}

	tv = duk_hobject_find_entry_tval_ptr(thr->heap, varmap, name);
	if (!tv) {
		return 0;
	}
	DUK_ASSERT(DUK_TVAL_IS_NUMBER(tv));
	reg_rel = (duk_size_t) DUK_TVAL_GET_NUMBER(tv);
	DUK_ASSERT_DISABLE(reg_rel >= 0);
	DUK_ASSERT(reg_rel < ((duk_hcompfunc *) func)->nregs);

	tv = (duk_tval *) (void *) ((duk_uint8_t *) thr->valstack + act->bottom_byteoff);
	tv += reg_rel;

	out->value = tv;
	out->attrs = DUK_PROPDESC_FLAGS_W; /* registers are mutable, non-deletable */
	out->env = NULL;
	out->holder = NULL;
	out->has_this = 0;
	return 1;
}

DUK_LOCAL
duk_bool_t duk__get_identifier_reference(duk_hthread *thr,
                                         duk_hobject *env,
                                         duk_hstring *name,
                                         duk_activation *act,
                                         duk_bool_t parents,
                                         duk__id_lookup_result *out) {
	duk_tval *tv;
	duk_uint_t sanity;

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(env != NULL || act != NULL);
	DUK_ASSERT(name != NULL);
	DUK_ASSERT(out != NULL);

	DUK_ASSERT(!env || DUK_HOBJECT_IS_ENV(env));
	DUK_ASSERT(!env || !DUK_HOBJECT_HAS_ARRAY_PART(env));

	/*
	 *  Conceptually, we look for the identifier binding by starting from
	 *  'env' and following to chain of environment records (represented
	 *  by the prototype chain).
	 *
	 *  If 'env' is NULL, the current activation does not yet have an
	 *  allocated declarative environment record; this should be treated
	 *  exactly as if the environment record existed but had no bindings
	 *  other than register bindings.
	 *
	 *  Note: we assume that with the DUK_HOBJECT_FLAG_NEWENV cleared
	 *  the environment will always be initialized immediately; hence
	 *  a NULL 'env' should only happen with the flag set.  This is the
	 *  case for: (1) function calls, and (2) strict, direct eval calls.
	 */

	if (env == NULL && act != NULL) {
		duk_hobject *func;
		duk_hcompfunc *f;

		DUK_DDD(DUK_DDDPRINT("duk__get_identifier_reference: env is NULL, activation is non-NULL -> "
		                     "delayed env case, look up activation regs first"));

		/*
		 *  Try registers
		 */

		if (duk__getid_activation_regs(thr, name, act, out)) {
			DUK_DDD(DUK_DDDPRINT("duk__get_identifier_reference successful: "
			                     "name=%!O -> value=%!T, attrs=%ld, has_this=%ld, env=%!O, holder=%!O "
			                     "(found from register bindings when env=NULL)",
			                     (duk_heaphdr *) name,
			                     (duk_tval *) out->value,
			                     (long) out->attrs,
			                     (long) out->has_this,
			                     (duk_heaphdr *) out->env,
			                     (duk_heaphdr *) out->holder));
			return 1;
		}

		DUK_DDD(DUK_DDDPRINT("not found in current activation regs"));

		/*
		 *  Not found in registers, proceed to the parent record.
		 *  Here we need to determine what the parent would be,
		 *  if 'env' was not NULL (i.e. same logic as when initializing
		 *  the record).
		 *
		 *  Note that environment initialization is only deferred when
		 *  DUK_HOBJECT_HAS_NEWENV is set, and this only happens for:
		 *    - Function code
		 *    - Strict eval code
		 *
		 *  We only need to check _Lexenv here; _Varenv exists only if it
		 *  differs from _Lexenv (and thus _Lexenv will also be present).
		 */

		if (!parents) {
			DUK_DDD(DUK_DDDPRINT("duk__get_identifier_reference failed, no parent traversal "
			                     "(not found from register bindings when env=NULL)"));
			goto fail_not_found;
		}

		func = DUK_ACT_GET_FUNC(act);
		DUK_ASSERT(func != NULL);
		DUK_ASSERT(DUK_HOBJECT_HAS_NEWENV(func));
		f = (duk_hcompfunc *) func;

		env = DUK_HCOMPFUNC_GET_LEXENV(thr->heap, f);
		if (!env) {
			env = thr->builtins[DUK_BIDX_GLOBAL_ENV];
		}

		DUK_DDD(DUK_DDDPRINT("continue lookup from env: %!iO", (duk_heaphdr *) env));
	}

	/*
	 *  Prototype walking starting from 'env'.
	 *
	 *  ('act' is not needed anywhere here.)
	 */

	sanity = DUK_HOBJECT_PROTOTYPE_CHAIN_SANITY;
	while (env != NULL) {
		duk_small_uint_t cl;
		duk_uint_t attrs;

		DUK_DDD(DUK_DDDPRINT("duk__get_identifier_reference, name=%!O, considering env=%p -> %!iO",
		                     (duk_heaphdr *) name,
		                     (void *) env,
		                     (duk_heaphdr *) env));

		DUK_ASSERT(env != NULL);
		DUK_ASSERT(DUK_HOBJECT_IS_ENV(env));
		DUK_ASSERT(!DUK_HOBJECT_HAS_ARRAY_PART(env));

		cl = DUK_HOBJECT_GET_CLASS_NUMBER(env);
		DUK_ASSERT(cl == DUK_HOBJECT_CLASS_OBJENV || cl == DUK_HOBJECT_CLASS_DECENV);
		if (cl == DUK_HOBJECT_CLASS_DECENV) {
			/*
			 *  Declarative environment record.
			 *
			 *  Identifiers can never be stored in ancestors and are
			 *  always plain values, so we can use an internal helper
			 *  and access the value directly with an duk_tval ptr.
			 *
			 *  A closed environment is only indicated by it missing
			 *  the "book-keeping" properties required for accessing
			 *  register-bound variables.
			 */

			DUK_HDECENV_ASSERT_VALID((duk_hdecenv *) env);
			if (duk__getid_open_decl_env_regs(thr, name, (duk_hdecenv *) env, out)) {
				DUK_DDD(DUK_DDDPRINT("duk__get_identifier_reference successful: "
				                     "name=%!O -> value=%!T, attrs=%ld, has_this=%ld, env=%!O, holder=%!O "
				                     "(declarative environment record, scope open, found in regs)",
				                     (duk_heaphdr *) name,
				                     (duk_tval *) out->value,
				                     (long) out->attrs,
				                     (long) out->has_this,
				                     (duk_heaphdr *) out->env,
				                     (duk_heaphdr *) out->holder));
				return 1;
			}

			tv = duk_hobject_find_entry_tval_ptr_and_attrs(thr->heap, env, name, &attrs);
			if (tv) {
				out->value = tv;
				out->attrs = attrs;
				out->env = env;
				out->holder = env;
				out->has_this = 0;

				DUK_DDD(DUK_DDDPRINT("duk__get_identifier_reference successful: "
				                     "name=%!O -> value=%!T, attrs=%ld, has_this=%ld, env=%!O, holder=%!O "
				                     "(declarative environment record, found in properties)",
				                     (duk_heaphdr *) name,
				                     (duk_tval *) out->value,
				                     (long) out->attrs,
				                     (long) out->has_this,
				                     (duk_heaphdr *) out->env,
				                     (duk_heaphdr *) out->holder));
				return 1;
			}
		} else {
			/*
			 *  Object environment record.
			 *
			 *  Binding (target) object is an external, uncontrolled object.
			 *  Identifier may be bound in an ancestor property, and may be
			 *  an accessor.  Target can also be a Proxy which we must support
			 *  here.
			 */

			/* XXX: we could save space by using _Target OR _This.  If _Target, assume
			 * this binding is undefined.  If _This, assumes this binding is _This, and
			 * target is also _This.  One property would then be enough.
			 */

			duk_hobject *target;
			duk_bool_t found;

			DUK_ASSERT(cl == DUK_HOBJECT_CLASS_OBJENV);
			DUK_HOBJENV_ASSERT_VALID((duk_hobjenv *) env);

			target = ((duk_hobjenv *) env)->target;
			DUK_ASSERT(target != NULL);

			/* Target may be a Proxy or property may be an accessor, so we must
			 * use an actual, Proxy-aware hasprop check here.
			 *
			 * out->holder is NOT set to the actual duk_hobject where the
			 * property is found, but rather the object binding target object.
			 */

#if defined(DUK_USE_ES6_PROXY)
			if (DUK_UNLIKELY(DUK_HOBJECT_IS_PROXY(target))) {
				duk_tval tv_name;
				duk_tval tv_target_tmp;

				DUK_ASSERT(name != NULL);
				DUK_TVAL_SET_STRING(&tv_name, name);
				DUK_TVAL_SET_OBJECT(&tv_target_tmp, target);

				found = duk_hobject_hasprop(thr, &tv_target_tmp, &tv_name);
			} else
#endif /* DUK_USE_ES6_PROXY */
			{
				/* XXX: duk_hobject_hasprop() would be correct for
				 * non-Proxy objects too, but it is about ~20-25%
				 * slower at present so separate code paths for
				 * Proxy and non-Proxy now.
				 */
				found = duk_hobject_hasprop_raw(thr, target, name);
			}

			if (found) {
				out->value = NULL; /* can't get value, may be accessor */
				out->attrs = 0; /* irrelevant when out->value == NULL */
				out->env = env;
				out->holder = target;
				out->has_this = ((duk_hobjenv *) env)->has_this;

				DUK_DDD(DUK_DDDPRINT("duk__get_identifier_reference successful: "
				                     "name=%!O -> value=%!T, attrs=%ld, has_this=%ld, env=%!O, holder=%!O "
				                     "(object environment record)",
				                     (duk_heaphdr *) name,
				                     (duk_tval *) out->value,
				                     (long) out->attrs,
				                     (long) out->has_this,
				                     (duk_heaphdr *) out->env,
				                     (duk_heaphdr *) out->holder));
				return 1;
			}
		}

		if (!parents) {
			DUK_DDD(DUK_DDDPRINT("duk__get_identifier_reference failed, no parent traversal "
			                     "(not found from first traversed env)"));
			goto fail_not_found;
		}

		if (DUK_UNLIKELY(sanity-- == 0)) {
			DUK_ERROR_RANGE(thr, DUK_STR_PROTOTYPE_CHAIN_LIMIT);
			DUK_WO_NORETURN(return 0;);
		}
		env = DUK_HOBJECT_GET_PROTOTYPE(thr->heap, env);
	}

	/*
	 *  Not found (even in global object)
	 */

fail_not_found:
	return 0;
}

/*
 *  HASVAR: check identifier binding from a given environment record
 *  without traversing its parents.
 *
 *  This primitive is not exposed to user code as such, but is used
 *  internally for e.g. declaration binding instantiation.
 *
 *  See E5 Sections:
 *    10.2.1.1.1 HasBinding(N)
 *    10.2.1.2.1 HasBinding(N)
 *
 *  Note: strictness has no bearing on this check.  Hence we don't take
 *  a 'strict' parameter.
 */

#if 0 /*unused*/
DUK_INTERNAL
duk_bool_t duk_js_hasvar_envrec(duk_hthread *thr,
                                duk_hobject *env,
                                duk_hstring *name) {
	duk__id_lookup_result ref;
	duk_bool_t parents;

	DUK_DDD(DUK_DDDPRINT("hasvar: thr=%p, env=%p, name=%!O "
	                     "(env -> %!dO)",
	                     (void *) thr, (void *) env, (duk_heaphdr *) name,
	                     (duk_heaphdr *) env));

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(env != NULL);
	DUK_ASSERT(name != NULL);

        DUK_ASSERT_REFCOUNT_NONZERO_HEAPHDR(env);
        DUK_ASSERT_REFCOUNT_NONZERO_HEAPHDR(name);

	DUK_ASSERT(DUK_HOBJECT_IS_ENV(env));
	DUK_ASSERT(!DUK_HOBJECT_HAS_ARRAY_PART(env));

	/* lookup results is ignored */
	parents = 0;
	return duk__get_identifier_reference(thr, env, name, NULL, parents, &ref);
}
#endif

/*
 *  GETVAR
 *
 *  See E5 Sections:
 *    11.1.2 Identifier Reference
 *    10.3.1 Identifier Resolution
 *    11.13.1 Simple Assignment  [example of where the Reference is GetValue'd]
 *    8.7.1 GetValue (V)
 *    8.12.1 [[GetOwnProperty]] (P)
 *    8.12.2 [[GetProperty]] (P)
 *    8.12.3 [[Get]] (P)
 *
 *  If 'throw' is true, always leaves two values on top of stack: [val this].
 *
 *  If 'throw' is false, returns 0 if identifier cannot be resolved, and the
 *  stack will be unaffected in this case.  If identifier is resolved, returns
 *  1 and leaves [val this] on top of stack.
 *
 *  Note: the 'strict' flag of a reference returned by GetIdentifierReference
 *  is ignored by GetValue.  Hence we don't take a 'strict' parameter.
 *
 *  The 'throw' flag is needed for implementing 'typeof' for an unreferenced
 *  identifier.  An unreference identifier in other contexts generates a
 *  ReferenceError.
 */

DUK_LOCAL
duk_bool_t duk__getvar_helper(duk_hthread *thr, duk_hobject *env, duk_activation *act, duk_hstring *name, duk_bool_t throw_flag) {
	duk__id_lookup_result ref;
	duk_tval tv_tmp_obj;
	duk_tval tv_tmp_key;
	duk_bool_t parents;

	DUK_DDD(DUK_DDDPRINT("getvar: thr=%p, env=%p, act=%p, name=%!O "
	                     "(env -> %!dO)",
	                     (void *) thr,
	                     (void *) env,
	                     (void *) act,
	                     (duk_heaphdr *) name,
	                     (duk_heaphdr *) env));

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(name != NULL);
	/* env and act may be NULL */

	DUK_STATS_INC(thr->heap, stats_getvar_all);

	DUK_ASSERT_REFCOUNT_NONZERO_HEAPHDR(env);
	DUK_ASSERT_REFCOUNT_NONZERO_HEAPHDR(name);

	parents = 1; /* follow parent chain */
	if (duk__get_identifier_reference(thr, env, name, act, parents, &ref)) {
		if (ref.value) {
			duk_push_tval(thr, ref.value);
			duk_push_undefined(thr);
		} else {
			DUK_ASSERT(ref.holder != NULL);

			/* ref.holder is safe across the getprop call (even
			 * with side effects) because 'env' is reachable and
			 * ref.holder is a direct heap pointer.
			 */

			DUK_TVAL_SET_OBJECT(&tv_tmp_obj, ref.holder);
			DUK_TVAL_SET_STRING(&tv_tmp_key, name);
			(void) duk_hobject_getprop(thr, &tv_tmp_obj, &tv_tmp_key); /* [value] */

			if (ref.has_this) {
				duk_push_hobject(thr, ref.holder);
			} else {
				duk_push_undefined(thr);
			}

			/* [value this] */
		}

		return 1;
	} else {
		if (throw_flag) {
			DUK_ERROR_FMT1(thr,
			               DUK_ERR_REFERENCE_ERROR,
			               "identifier '%s' undefined",
			               (const char *) DUK_HSTRING_GET_DATA(name));
			DUK_WO_NORETURN(return 0;);
		}

		return 0;
	}
}

DUK_INTERNAL
duk_bool_t duk_js_getvar_envrec(duk_hthread *thr, duk_hobject *env, duk_hstring *name, duk_bool_t throw_flag) {
	return duk__getvar_helper(thr, env, NULL, name, throw_flag);
}

DUK_INTERNAL
duk_bool_t duk_js_getvar_activation(duk_hthread *thr, duk_activation *act, duk_hstring *name, duk_bool_t throw_flag) {
	DUK_ASSERT(act != NULL);
	return duk__getvar_helper(thr, act->lex_env, act, name, throw_flag);
}

/*
 *  PUTVAR
 *
 *  See E5 Sections:
 *    11.1.2 Identifier Reference
 *    10.3.1 Identifier Resolution
 *    11.13.1 Simple Assignment  [example of where the Reference is PutValue'd]
 *    8.7.2 PutValue (V,W)  [see especially step 3.b, undefined -> automatic global in non-strict mode]
 *    8.12.4 [[CanPut]] (P)
 *    8.12.5 [[Put]] (P)
 *
 *  Note: may invalidate any valstack (or object) duk_tval pointers because
 *  putting a value may reallocate any object or any valstack.  Caller beware.
 */

DUK_LOCAL
void duk__putvar_helper(duk_hthread *thr,
                        duk_hobject *env,
                        duk_activation *act,
                        duk_hstring *name,
                        duk_tval *val,
                        duk_bool_t strict) {
	duk__id_lookup_result ref;
	duk_tval tv_tmp_val;
	duk_tval tv_tmp_obj;
	duk_tval tv_tmp_key;
	duk_bool_t parents;

	DUK_STATS_INC(thr->heap, stats_putvar_all);

	DUK_DDD(DUK_DDDPRINT("putvar: thr=%p, env=%p, act=%p, name=%!O, val=%p, strict=%ld "
	                     "(env -> %!dO, val -> %!T)",
	                     (void *) thr,
	                     (void *) env,
	                     (void *) act,
	                     (duk_heaphdr *) name,
	                     (void *) val,
	                     (long) strict,
	                     (duk_heaphdr *) env,
	                     (duk_tval *) val));

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(name != NULL);
	DUK_ASSERT(val != NULL);
	/* env and act may be NULL */

	DUK_ASSERT_REFCOUNT_NONZERO_HEAPHDR(env);
	DUK_ASSERT_REFCOUNT_NONZERO_HEAPHDR(name);
	DUK_ASSERT_REFCOUNT_NONZERO_TVAL(val);

	DUK_TVAL_SET_TVAL(&tv_tmp_val, val); /* Stabilize. */
	val = NULL;

	/*
	 *  In strict mode E5 protects 'eval' and 'arguments' from being
	 *  assigned to (or even declared anywhere).  Attempt to do so
	 *  should result in a compile time SyntaxError.  See the internal
	 *  design documentation for details.
	 *
	 *  Thus, we should never come here, run-time, for strict code,
	 *  and name 'eval' or 'arguments'.
	 */

	DUK_ASSERT(!strict || (name != DUK_HTHREAD_STRING_EVAL(thr) && name != DUK_HTHREAD_STRING_LC_ARGUMENTS(thr)));

	/*
	 *  Lookup variable and update in-place if found.
	 */

	parents = 1; /* follow parent chain */

	if (duk__get_identifier_reference(thr, env, name, act, parents, &ref)) {
		if (ref.value && (ref.attrs & DUK_PROPDESC_FLAG_WRITABLE)) {
			/* Update duk_tval in-place if pointer provided and the
			 * property is writable.  If the property is not writable
			 * (immutable binding), use duk_hobject_putprop() which
			 * will respect mutability.
			 */
			duk_tval *tv_val;

			tv_val = ref.value;
			DUK_ASSERT(tv_val != NULL);
			DUK_TVAL_SET_TVAL_UPDREF(thr, tv_val, &tv_tmp_val); /* side effects */

			/* ref.value invalidated here */
		} else {
			DUK_ASSERT(ref.holder != NULL);

			DUK_TVAL_SET_OBJECT(&tv_tmp_obj, ref.holder);
			DUK_TVAL_SET_STRING(&tv_tmp_key, name);
			(void) duk_hobject_putprop(thr, &tv_tmp_obj, &tv_tmp_key, &tv_tmp_val, strict);

			/* ref.value invalidated here */
		}

		return;
	}

	/*
	 *  Not found: write to global object (non-strict) or ReferenceError
	 *  (strict); see E5 Section 8.7.2, step 3.
	 */

	if (strict) {
		DUK_DDD(DUK_DDDPRINT("identifier binding not found, strict => reference error"));
		DUK_ERROR_FMT1(thr,
		               DUK_ERR_REFERENCE_ERROR,
		               "identifier '%s' undefined",
		               (const char *) DUK_HSTRING_GET_DATA(name));
		DUK_WO_NORETURN(return;);
	}

	DUK_DDD(DUK_DDDPRINT("identifier binding not found, not strict => set to global"));

	DUK_TVAL_SET_OBJECT(&tv_tmp_obj, thr->builtins[DUK_BIDX_GLOBAL]);
	DUK_TVAL_SET_STRING(&tv_tmp_key, name);
	(void) duk_hobject_putprop(thr, &tv_tmp_obj, &tv_tmp_key, &tv_tmp_val, 0); /* 0 = no throw */

	/* NB: 'val' may be invalidated here because put_value may realloc valstack,
	 * caller beware.
	 */
}

DUK_INTERNAL
void duk_js_putvar_envrec(duk_hthread *thr, duk_hobject *env, duk_hstring *name, duk_tval *val, duk_bool_t strict) {
	duk__putvar_helper(thr, env, NULL, name, val, strict);
}

DUK_INTERNAL
void duk_js_putvar_activation(duk_hthread *thr, duk_activation *act, duk_hstring *name, duk_tval *val, duk_bool_t strict) {
	DUK_ASSERT(act != NULL);
	duk__putvar_helper(thr, act->lex_env, act, name, val, strict);
}

/*
 *  DELVAR
 *
 *  See E5 Sections:
 *    11.4.1 The delete operator
 *    10.2.1.1.5 DeleteBinding (N)  [declarative environment record]
 *    10.2.1.2.5 DeleteBinding (N)  [object environment record]
 *
 *  Variable bindings established inside eval() are deletable (configurable),
 *  other bindings are not, including variables declared in global level.
 *  Registers are always non-deletable, and the deletion of other bindings
 *  is controlled by the configurable flag.
 *
 *  For strict mode code, the 'delete' operator should fail with a compile
 *  time SyntaxError if applied to identifiers.  Hence, no strict mode
 *  run-time deletion of identifiers should ever happen.  This function
 *  should never be called from strict mode code!
 */

DUK_LOCAL
duk_bool_t duk__delvar_helper(duk_hthread *thr, duk_hobject *env, duk_activation *act, duk_hstring *name) {
	duk__id_lookup_result ref;
	duk_bool_t parents;

	DUK_DDD(DUK_DDDPRINT("delvar: thr=%p, env=%p, act=%p, name=%!O "
	                     "(env -> %!dO)",
	                     (void *) thr,
	                     (void *) env,
	                     (void *) act,
	                     (duk_heaphdr *) name,
	                     (duk_heaphdr *) env));

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(name != NULL);
	/* env and act may be NULL */

	DUK_ASSERT_REFCOUNT_NONZERO_HEAPHDR(name);

	parents = 1; /* follow parent chain */

	if (duk__get_identifier_reference(thr, env, name, act, parents, &ref)) {
		if (ref.value && !(ref.attrs & DUK_PROPDESC_FLAG_CONFIGURABLE)) {
			/* Identifier found in registers (always non-deletable)
			 * or declarative environment record and non-configurable.
			 */
			return 0;
		}
		DUK_ASSERT(ref.holder != NULL);

		return duk_hobject_delprop_raw(thr, ref.holder, name, 0);
	}

	/*
	 *  Not found (even in global object).
	 *
	 *  In non-strict mode this is a silent SUCCESS (!), see E5 Section 11.4.1,
	 *  step 3.b.  In strict mode this case is a compile time SyntaxError so
	 *  we should not come here.
	 */

	DUK_DDD(DUK_DDDPRINT("identifier to be deleted not found: name=%!O "
	                     "(treated as silent success)",
	                     (duk_heaphdr *) name));
	return 1;
}

#if 0 /*unused*/
DUK_INTERNAL
duk_bool_t duk_js_delvar_envrec(duk_hthread *thr,
                                duk_hobject *env,
                                duk_hstring *name) {
	return duk__delvar_helper(thr, env, NULL, name);
}
#endif

DUK_INTERNAL
duk_bool_t duk_js_delvar_activation(duk_hthread *thr, duk_activation *act, duk_hstring *name) {
	DUK_ASSERT(act != NULL);
	return duk__delvar_helper(thr, act->lex_env, act, name);
}

/*
 *  DECLVAR
 *
 *  See E5 Sections:
 *    10.4.3 Entering Function Code
 *    10.5 Declaration Binding Instantion
 *    12.2 Variable Statement
 *    11.1.2 Identifier Reference
 *    10.3.1 Identifier Resolution
 *
 *  Variable declaration behavior is mainly discussed in Section 10.5,
 *  and is not discussed in the execution semantics (Sections 11-13).
 *
 *  Conceptually declarations happen when code (global, eval, function)
 *  is entered, before any user code is executed.  In practice, register-
 *  bound identifiers are 'declared' automatically (by virtue of being
 *  allocated to registers with the initial value 'undefined').  Other
 *  identifiers are declared in the function prologue with this primitive.
 *
 *  Since non-register bindings eventually back to an internal object's
 *  properties, the 'prop_flags' argument is used to specify binding
 *  type:
 *
 *    - Immutable binding: set DUK_PROPDESC_FLAG_WRITABLE to false
 *    - Non-deletable binding: set DUK_PROPDESC_FLAG_CONFIGURABLE to false
 *    - The flag DUK_PROPDESC_FLAG_ENUMERABLE should be set, although it
 *      doesn't really matter for internal objects
 *
 *  All bindings are non-deletable mutable bindings except:
 *
 *    - Declarations in eval code (mutable, deletable)
 *    - 'arguments' binding in strict function code (immutable)
 *    - Function name binding of a function expression (immutable)
 *
 *  Declarations may go to declarative environment records (always
 *  so for functions), but may also go to object environment records
 *  (e.g. global code).  The global object environment has special
 *  behavior when re-declaring a function (but not a variable); see
 *  E5.1 specification, Section 10.5, step 5.e.
 *
 *  Declarations always go to the 'top-most' environment record, i.e.
 *  we never check the record chain.  It's not an error even if a
 *  property (even an immutable or non-deletable one) of the same name
 *  already exists.
 *
 *  If a declared variable already exists, its value needs to be updated
 *  (if possible).  Returns 1 if a PUTVAR needs to be done by the caller;
 *  otherwise returns 0.
 */

DUK_LOCAL
duk_bool_t duk__declvar_helper(duk_hthread *thr,
                               duk_hobject *env,
                               duk_hstring *name,
                               duk_tval *val,
                               duk_small_uint_t prop_flags,
                               duk_bool_t is_func_decl) {
	duk_hobject *holder;
	duk_bool_t parents;
	duk__id_lookup_result ref;
	duk_tval *tv;

	DUK_DDD(DUK_DDDPRINT("declvar: thr=%p, env=%p, name=%!O, val=%!T, prop_flags=0x%08lx, is_func_decl=%ld "
	                     "(env -> %!iO)",
	                     (void *) thr,
	                     (void *) env,
	                     (duk_heaphdr *) name,
	                     (duk_tval *) val,
	                     (unsigned long) prop_flags,
	                     (unsigned int) is_func_decl,
	                     (duk_heaphdr *) env));

	DUK_ASSERT(thr != NULL);
	DUK_ASSERT(env != NULL);
	DUK_ASSERT(name != NULL);
	DUK_ASSERT(val != NULL);

	/* Note: in strict mode the compiler should reject explicit
	 * declaration of 'eval' or 'arguments'.  However, internal
	 * bytecode may declare 'arguments' in the function prologue.
	 * We don't bother checking (or asserting) for these now.
	 */

	/* Note: val is a stable duk_tval pointer.  The caller makes
	 * a value copy into its stack frame, so 'tv_val' is not subject
	 * to side effects here.
	 */

	/*
	 *  Check whether already declared.
	 *
	 *  We need to check whether the binding exists in the environment
	 *  without walking its parents.  However, we still need to check
	 *  register-bound identifiers and the prototype chain of an object
	 *  environment target object.
	 */

	parents = 0; /* just check 'env' */
	if (duk__get_identifier_reference(thr, env, name, NULL, parents, &ref)) {
		duk_int_t e_idx;
		duk_int_t h_idx;
		duk_small_uint_t flags;

		/*
		 *  Variable already declared, ignore re-declaration.
		 *  The only exception is the updated behavior of E5.1 for
		 *  global function declarations, E5.1 Section 10.5, step 5.e.
		 *  This behavior does not apply to global variable declarations.
		 */

		if (!(is_func_decl && env == thr->builtins[DUK_BIDX_GLOBAL_ENV])) {
			DUK_DDD(DUK_DDDPRINT("re-declare a binding, ignoring"));
			return 1; /* 1 -> needs a PUTVAR */
		}

		/*
		 *  Special behavior in E5.1.
		 *
		 *  Note that even though parents == 0, the conflicting property
		 *  may be an inherited property (currently our global object's
		 *  prototype is Object.prototype).  Step 5.e first operates on
		 *  the existing property (which is potentially in an ancestor)
		 *  and then defines a new property in the global object (and
		 *  never modifies the ancestor).
		 *
		 *  Also note that this logic would become even more complicated
		 *  if the conflicting property might be a virtual one.  Object
		 *  prototype has no virtual properties, though.
		 *
		 *  XXX: this is now very awkward, rework.
		 */

		DUK_DDD(DUK_DDDPRINT("re-declare a function binding in global object, "
		                     "updated E5.1 processing"));

		DUK_ASSERT(ref.holder != NULL);
		holder = ref.holder;

		/* holder will be set to the target object, not the actual object
		 * where the property was found (see duk__get_identifier_reference()).
		 */
		DUK_ASSERT(DUK_HOBJECT_GET_CLASS_NUMBER(holder) == DUK_HOBJECT_CLASS_GLOBAL);
		DUK_ASSERT(!DUK_HOBJECT_HAS_EXOTIC_ARRAY(holder)); /* global object doesn't have array part */

		/* XXX: use a helper for prototype traversal; no loop check here */
		/* must be found: was found earlier, and cannot be inherited */
		for (;;) {
			DUK_ASSERT(holder != NULL);
			if (duk_hobject_find_entry(thr->heap, holder, name, &e_idx, &h_idx)) {
				DUK_ASSERT(e_idx >= 0);
				break;
			}
			/* SCANBUILD: NULL pointer dereference, doesn't actually trigger,
			 * asserted above.
			 */
			holder = DUK_HOBJECT_GET_PROTOTYPE(thr->heap, holder);
		}
		DUK_ASSERT(holder != NULL);
		DUK_ASSERT(e_idx >= 0);
		/* SCANBUILD: scan-build produces a NULL pointer dereference warning
		 * below; it never actually triggers because holder is actually never
		 * NULL.
		 */

		/* ref.holder is global object, holder is the object with the
		 * conflicting property.
		 */

		flags = DUK_HOBJECT_E_GET_FLAGS(thr->heap, holder, e_idx);
		if (!(flags & DUK_PROPDESC_FLAG_CONFIGURABLE)) {
			if (flags & DUK_PROPDESC_FLAG_ACCESSOR) {
				DUK_DDD(DUK_DDDPRINT("existing property is a non-configurable "
				                     "accessor -> reject"));
				goto fail_existing_attributes;
			}
			if (!((flags & DUK_PROPDESC_FLAG_WRITABLE) && (flags & DUK_PROPDESC_FLAG_ENUMERABLE))) {
				DUK_DDD(DUK_DDDPRINT("existing property is a non-configurable "
				                     "plain property which is not writable and "
				                     "enumerable -> reject"));
				goto fail_existing_attributes;
			}

			DUK_DDD(DUK_DDDPRINT("existing property is not configurable but "
			                     "is plain, enumerable, and writable -> "
			                     "allow redeclaration"));
		}

		if (holder == ref.holder) {
			/* XXX: if duk_hobject_define_property_internal() was updated
			 * to handle a pre-existing accessor property, this would be
			 * a simple call (like for the ancestor case).
			 */
			DUK_DDD(DUK_DDDPRINT("redefine, offending property in global object itself"));

			if (flags & DUK_PROPDESC_FLAG_ACCESSOR) {
				duk_hobject *tmp;

				tmp = DUK_HOBJECT_E_GET_VALUE_GETTER(thr->heap, holder, e_idx);
				DUK_HOBJECT_E_SET_VALUE_GETTER(thr->heap, holder, e_idx, NULL);
				DUK_HOBJECT_DECREF_ALLOWNULL(thr, tmp);
				DUK_UNREF(tmp);
				tmp = DUK_HOBJECT_E_GET_VALUE_SETTER(thr->heap, holder, e_idx);
				DUK_HOBJECT_E_SET_VALUE_SETTER(thr->heap, holder, e_idx, NULL);
				DUK_HOBJECT_DECREF_ALLOWNULL(thr, tmp);
				DUK_UNREF(tmp);
			} else {
				tv = DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(thr->heap, holder, e_idx);
				DUK_TVAL_SET_UNDEFINED_UPDREF(thr, tv);
			}

			/* Here val would be potentially invalid if we didn't make
			 * a value copy at the caller.
			 */

			tv = DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(thr->heap, holder, e_idx);
			DUK_TVAL_SET_TVAL(tv, val);
			DUK_TVAL_INCREF(thr, tv);
			DUK_HOBJECT_E_SET_FLAGS(thr->heap, holder, e_idx, prop_flags);

			DUK_DDD(DUK_DDDPRINT("updated global binding, final result: "
			                     "value -> %!T, prop_flags=0x%08lx",
			                     (duk_tval *) DUK_HOBJECT_E_GET_VALUE_TVAL_PTR(thr->heap, holder, e_idx),
			                     (unsigned long) prop_flags));
		} else {
			DUK_DDD(DUK_DDDPRINT("redefine, offending property in ancestor"));

			DUK_ASSERT(ref.holder == thr->builtins[DUK_BIDX_GLOBAL]);
			duk_push_tval(thr, val);
			duk_hobject_define_property_internal(thr, ref.holder, name, prop_flags);
		}

		return 0;
	}

	/*
	 *  Not found (in registers or record objects).  Declare
	 *  to current variable environment.
	 */

	/*
	 *  Get holder object
	 */

	if (DUK_HOBJECT_IS_DECENV(env)) {
		DUK_HDECENV_ASSERT_VALID((duk_hdecenv *) env);
		holder = env;
	} else {
		DUK_HOBJENV_ASSERT_VALID((duk_hobjenv *) env);
		holder = ((duk_hobjenv *) env)->target;
		DUK_ASSERT(holder != NULL);
	}

	/*
	 *  Define new property
	 *
	 *  Note: this may fail if the holder is not extensible.
	 */

	/* XXX: this is awkward as we use an internal method which doesn't handle
	 * extensibility etc correctly.  Basically we'd want to do a [[DefineOwnProperty]]
	 * or Object.defineProperty() here.
	 */

	if (!DUK_HOBJECT_HAS_EXTENSIBLE(holder)) {
		goto fail_not_extensible;
	}

	duk_push_hobject(thr, holder);
	duk_push_hstring(thr, name);
	duk_push_tval(thr, val);
	duk_xdef_prop(thr, -3, prop_flags); /* [holder name val] -> [holder] */
	duk_pop_unsafe(thr);

	return 0;

fail_existing_attributes:
fail_not_extensible:
	DUK_ERROR_TYPE(thr, "declaration failed");
	DUK_WO_NORETURN(return 0;);
}

DUK_INTERNAL
duk_bool_t duk_js_declvar_activation(duk_hthread *thr,
                                     duk_activation *act,
                                     duk_hstring *name,
                                     duk_tval *val,
                                     duk_small_uint_t prop_flags,
                                     duk_bool_t is_func_decl) {
	duk_hobject *env;
	duk_tval tv_val_copy;

	DUK_ASSERT(act != NULL);

	/*
	 *  Make a value copy of the input val.  This ensures that
	 *  side effects cannot invalidate the pointer.
	 */

	DUK_TVAL_SET_TVAL(&tv_val_copy, val);
	val = &tv_val_copy;

	/*
	 *  Delayed env creation check
	 */

	if (!act->var_env) {
		DUK_ASSERT(act->lex_env == NULL);
		duk_js_init_activation_environment_records_delayed(thr, act);
		/* 'act' is a stable pointer, so still OK. */
	}
	DUK_ASSERT(act->lex_env != NULL);
	DUK_ASSERT(act->var_env != NULL);

	env = act->var_env;
	DUK_ASSERT(env != NULL);
	DUK_ASSERT(DUK_HOBJECT_IS_ENV(env));

	return duk__declvar_helper(thr, env, name, val, prop_flags, is_func_decl);
}
