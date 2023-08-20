/*
 *  Initialize built-in objects.  Current thread must have a valstack
 *  and initialization errors may longjmp, so a setjmp() catch point
 *  must exist.
 */

#include "duk_internal.h"

/*
 *  Encoding constants, must match genbuiltins.py
 */

#define DUK__PROP_FLAGS_BITS  3
#define DUK__LENGTH_PROP_BITS 3
#define DUK__NARGS_BITS       3
#define DUK__PROP_TYPE_BITS   3

#define DUK__NARGS_VARARGS_MARKER 0x07

#define DUK__PROP_TYPE_DOUBLE        0
#define DUK__PROP_TYPE_STRING        1
#define DUK__PROP_TYPE_STRIDX        2
#define DUK__PROP_TYPE_BUILTIN       3
#define DUK__PROP_TYPE_UNDEFINED     4
#define DUK__PROP_TYPE_BOOLEAN_TRUE  5
#define DUK__PROP_TYPE_BOOLEAN_FALSE 6
#define DUK__PROP_TYPE_ACCESSOR      7

/*
 *  Create built-in objects by parsing an init bitstream generated
 *  by genbuiltins.py.
 */

#if defined(DUK_USE_ROM_OBJECTS)
#if defined(DUK_USE_ROM_GLOBAL_CLONE) || defined(DUK_USE_ROM_GLOBAL_INHERIT)
DUK_LOCAL void duk__duplicate_ram_global_object(duk_hthread *thr) {
	duk_hobject *h_global;
#if defined(DUK_USE_ROM_GLOBAL_CLONE)
	duk_hobject *h_oldglobal;
	duk_uint8_t *props;
	duk_size_t alloc_size;
#endif
	duk_hobject *h_objenv;

	/* XXX: refactor into internal helper, duk_clone_hobject() */

#if defined(DUK_USE_ROM_GLOBAL_INHERIT)
	/* Inherit from ROM-based global object: less RAM usage, less transparent. */
	h_global = duk_push_object_helper(thr,
	                                  DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_FASTREFS |
	                                      DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_GLOBAL),
	                                  DUK_BIDX_GLOBAL);
	DUK_ASSERT(h_global != NULL);
#elif defined(DUK_USE_ROM_GLOBAL_CLONE)
	/* Clone the properties of the ROM-based global object to create a
	 * fully RAM-based global object.  Uses more memory than the inherit
	 * model but more compliant.
	 */
	h_global = duk_push_object_helper(thr,
	                                  DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_FLAG_FASTREFS |
	                                      DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_GLOBAL),
	                                  DUK_BIDX_OBJECT_PROTOTYPE);
	DUK_ASSERT(h_global != NULL);
	h_oldglobal = thr->builtins[DUK_BIDX_GLOBAL];
	DUK_ASSERT(h_oldglobal != NULL);

	/* Copy the property table verbatim; this handles attributes etc.
	 * For ROM objects it's not necessary (or possible) to update
	 * refcounts so leave them as is.
	 */
	alloc_size = DUK_HOBJECT_P_ALLOC_SIZE(h_oldglobal);
	DUK_ASSERT(alloc_size > 0);
	props = DUK_ALLOC_CHECKED(thr, alloc_size);
	DUK_ASSERT(props != NULL);
	DUK_ASSERT(DUK_HOBJECT_GET_PROPS(thr->heap, h_oldglobal) != NULL);
	duk_memcpy((void *) props, (const void *) DUK_HOBJECT_GET_PROPS(thr->heap, h_oldglobal), alloc_size);

	/* XXX: keep property attributes or tweak them here?
	 * Properties will now be non-configurable even when they're
	 * normally configurable for the global object.
	 */

	DUK_ASSERT(DUK_HOBJECT_GET_PROPS(thr->heap, h_global) == NULL);
	DUK_HOBJECT_SET_PROPS(thr->heap, h_global, props);
	DUK_HOBJECT_SET_ESIZE(h_global, DUK_HOBJECT_GET_ESIZE(h_oldglobal));
	DUK_HOBJECT_SET_ENEXT(h_global, DUK_HOBJECT_GET_ENEXT(h_oldglobal));
	DUK_HOBJECT_SET_ASIZE(h_global, DUK_HOBJECT_GET_ASIZE(h_oldglobal));
	DUK_HOBJECT_SET_HSIZE(h_global, DUK_HOBJECT_GET_HSIZE(h_oldglobal));
#else
#error internal error in config defines
#endif

	duk_hobject_compact_props(thr, h_global);
	DUK_ASSERT(thr->builtins[DUK_BIDX_GLOBAL] != NULL);
	DUK_ASSERT(
	    !DUK_HEAPHDR_NEEDS_REFCOUNT_UPDATE((duk_heaphdr *) thr->builtins[DUK_BIDX_GLOBAL])); /* no need to decref: ROM object */
	thr->builtins[DUK_BIDX_GLOBAL] = h_global;
	DUK_HOBJECT_INCREF(thr, h_global);
	DUK_D(DUK_DPRINT("duplicated global object: %!O", h_global));

	/* Create a fresh object environment for the global scope.  This is
	 * needed so that the global scope points to the newly created RAM-based
	 * global object.
	 */
	h_objenv =
	    (duk_hobject *) duk_hobjenv_alloc(thr,
	                                      DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_OBJENV));
	DUK_ASSERT(h_objenv != NULL);
	DUK_ASSERT(DUK_HOBJECT_GET_PROTOTYPE(thr->heap, h_objenv) == NULL);
	duk_push_hobject(thr, h_objenv);

	DUK_ASSERT(h_global != NULL);
	((duk_hobjenv *) h_objenv)->target = h_global;
	DUK_HOBJECT_INCREF(thr, h_global);
	DUK_ASSERT(((duk_hobjenv *) h_objenv)->has_this == 0);

	DUK_ASSERT(thr->builtins[DUK_BIDX_GLOBAL_ENV] != NULL);
	DUK_ASSERT(!DUK_HEAPHDR_NEEDS_REFCOUNT_UPDATE(
	    (duk_heaphdr *) thr->builtins[DUK_BIDX_GLOBAL_ENV])); /* no need to decref: ROM object */
	thr->builtins[DUK_BIDX_GLOBAL_ENV] = h_objenv;
	DUK_HOBJECT_INCREF(thr, h_objenv);
	DUK_D(DUK_DPRINT("duplicated global env: %!O", h_objenv));

	DUK_HOBJENV_ASSERT_VALID((duk_hobjenv *) h_objenv);

	duk_pop_2(thr); /* Pop global object and global env. */
}
#endif /* DUK_USE_ROM_GLOBAL_CLONE || DUK_USE_ROM_GLOBAL_INHERIT */

DUK_INTERNAL void duk_hthread_create_builtin_objects(duk_hthread *thr) {
	/* Setup builtins from ROM objects.  All heaps/threads will share
	 * the same readonly objects.
	 */
	duk_small_uint_t i;

	for (i = 0; i < DUK_NUM_BUILTINS; i++) {
		duk_hobject *h;
		h = (duk_hobject *) DUK_LOSE_CONST(duk_rom_builtins_bidx[i]);
		DUK_ASSERT(h != NULL);
		thr->builtins[i] = h;
	}

#if defined(DUK_USE_ROM_GLOBAL_CLONE) || defined(DUK_USE_ROM_GLOBAL_INHERIT)
	/* By default the global object is read-only which is often much
	 * more of an issue than having read-only built-in objects (like
	 * RegExp, Date, etc).  Use a RAM-based copy of the global object
	 * and the global environment object for convenience.
	 */
	duk__duplicate_ram_global_object(thr);
#endif
}
#else /* DUK_USE_ROM_OBJECTS */
DUK_LOCAL void duk__push_stridx(duk_hthread *thr, duk_bitdecoder_ctx *bd) {
	duk_small_uint_t n;

	n = (duk_small_uint_t) duk_bd_decode_varuint(bd);
	DUK_ASSERT_DISABLE(n >= 0); /* unsigned */
	DUK_ASSERT(n < DUK_HEAP_NUM_STRINGS);
	duk_push_hstring_stridx(thr, n);
}
DUK_LOCAL void duk__push_string(duk_hthread *thr, duk_bitdecoder_ctx *bd) {
	/* XXX: built-ins data could provide a maximum length that is
	 * actually needed; bitpacked max length is now 256 bytes.
	 */
	duk_uint8_t tmp[DUK_BD_BITPACKED_STRING_MAXLEN];
	duk_small_uint_t len;

	len = duk_bd_decode_bitpacked_string(bd, tmp);
	duk_push_lstring(thr, (const char *) tmp, (duk_size_t) len);
}
DUK_LOCAL void duk__push_stridx_or_string(duk_hthread *thr, duk_bitdecoder_ctx *bd) {
	duk_small_uint_t n;

	n = (duk_small_uint_t) duk_bd_decode_varuint(bd);
	if (n == 0) {
		duk__push_string(thr, bd);
	} else {
		n--;
		DUK_ASSERT(n < DUK_HEAP_NUM_STRINGS);
		duk_push_hstring_stridx(thr, n);
	}
}
DUK_LOCAL void duk__push_double(duk_hthread *thr, duk_bitdecoder_ctx *bd) {
	duk_double_union du;
	duk_small_uint_t i;

	for (i = 0; i < 8; i++) {
		/* Encoding endianness must match target memory layout,
		 * build scripts and genbuiltins.py must ensure this.
		 */
		du.uc[i] = (duk_uint8_t) duk_bd_decode(bd, 8);
	}

	duk_push_number(thr, du.d); /* push operation normalizes NaNs */
}

DUK_INTERNAL void duk_hthread_create_builtin_objects(duk_hthread *thr) {
	duk_bitdecoder_ctx bd_ctx;
	duk_bitdecoder_ctx *bd = &bd_ctx; /* convenience */
	duk_hobject *h;
	duk_small_uint_t i, j;

	DUK_D(DUK_DPRINT("INITBUILTINS BEGIN: DUK_NUM_BUILTINS=%d, DUK_NUM_BUILTINS_ALL=%d",
	                 (int) DUK_NUM_BUILTINS,
	                 (int) DUK_NUM_ALL_BUILTINS));

	duk_memzero(&bd_ctx, sizeof(bd_ctx));
	bd->data = (const duk_uint8_t *) duk_builtins_data;
	bd->length = (duk_size_t) DUK_BUILTINS_DATA_LENGTH;

	/*
	 *  First create all built-in bare objects on the empty valstack.
	 *
	 *  Built-ins in the index range [0,DUK_NUM_BUILTINS-1] have value
	 *  stack indices matching their eventual thr->builtins[] index.
	 *
	 *  Built-ins in the index range [DUK_NUM_BUILTINS,DUK_NUM_ALL_BUILTINS]
	 *  will exist on the value stack during init but won't be placed
	 *  into thr->builtins[].  These are objects referenced in some way
	 *  from thr->builtins[] roots but which don't need to be indexed by
	 *  Duktape through thr->builtins[] (e.g. user custom objects).
	 *
	 *  Internal prototypes will be incorrect (NULL) at this stage.
	 */

	duk_require_stack(thr, DUK_NUM_ALL_BUILTINS);

	DUK_DD(DUK_DDPRINT("create empty built-ins"));
	DUK_ASSERT_TOP(thr, 0);
	for (i = 0; i < DUK_NUM_ALL_BUILTINS; i++) {
		duk_small_uint_t class_num;
		duk_small_int_t len = -1; /* must be signed */

		class_num = (duk_small_uint_t) duk_bd_decode_varuint(bd);
		len = (duk_small_int_t) duk_bd_decode_flagged_signed(bd, DUK__LENGTH_PROP_BITS, (duk_int32_t) -1 /*def_value*/);

		if (class_num == DUK_HOBJECT_CLASS_FUNCTION) {
			duk_small_uint_t natidx;
			duk_small_int_t c_nargs; /* must hold DUK_VARARGS */
			duk_c_function c_func;
			duk_int16_t magic;

			DUK_DDD(DUK_DDDPRINT("len=%ld", (long) len));
			DUK_ASSERT(len >= 0);

			natidx = (duk_small_uint_t) duk_bd_decode_varuint(bd);
			DUK_ASSERT(natidx != 0);
			c_func = duk_bi_native_functions[natidx];
			DUK_ASSERT(c_func != NULL);

			c_nargs = (duk_small_int_t) duk_bd_decode_flagged_signed(bd, DUK__NARGS_BITS, len /*def_value*/);
			if (c_nargs == DUK__NARGS_VARARGS_MARKER) {
				c_nargs = DUK_VARARGS;
			}

			/* XXX: set magic directly here? (it could share the c_nargs arg) */
			(void) duk_push_c_function_builtin(thr, c_func, c_nargs);
			h = duk_known_hobject(thr, -1);

			/* Currently all built-in native functions are strict.
			 * duk_push_c_function() now sets strict flag, so
			 * assert for it.
			 */
			DUK_ASSERT(DUK_HOBJECT_HAS_STRICT(h));

			/* XXX: function properties */

			duk__push_stridx_or_string(thr, bd);
#if defined(DUK_USE_FUNC_NAME_PROPERTY)
			duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_NAME, DUK_PROPDESC_FLAGS_C);
#else
			duk_pop(thr); /* Not very ideal but good enough for now. */
#endif

			/* Almost all global level Function objects are constructable
			 * but not all: Function.prototype is a non-constructable,
			 * callable Function.
			 */
			if (duk_bd_decode_flag(bd)) {
				DUK_ASSERT(DUK_HOBJECT_HAS_CONSTRUCTABLE(h));
			} else {
				DUK_HOBJECT_CLEAR_CONSTRUCTABLE(h);
			}

			/* Cast converts magic to 16-bit signed value */
			magic = (duk_int16_t) duk_bd_decode_varuint(bd);
			((duk_hnatfunc *) h)->magic = magic;
		} else if (class_num == DUK_HOBJECT_CLASS_ARRAY) {
			duk_push_array(thr);
		} else if (class_num == DUK_HOBJECT_CLASS_OBJENV) {
			duk_hobjenv *env;
			duk_hobject *global;

			DUK_ASSERT(i == DUK_BIDX_GLOBAL_ENV);
			DUK_ASSERT(DUK_BIDX_GLOBAL_ENV > DUK_BIDX_GLOBAL);

			env = duk_hobjenv_alloc(thr,
			                        DUK_HOBJECT_FLAG_EXTENSIBLE | DUK_HOBJECT_CLASS_AS_FLAGS(DUK_HOBJECT_CLASS_OBJENV));
			DUK_ASSERT(env->target == NULL);
			duk_push_hobject(thr, (duk_hobject *) env);

			global = duk_known_hobject(thr, DUK_BIDX_GLOBAL);
			DUK_ASSERT(global != NULL);
			env->target = global;
			DUK_HOBJECT_INCREF(thr, global);
			DUK_ASSERT(env->has_this == 0);

			DUK_HOBJENV_ASSERT_VALID(env);
		} else {
			DUK_ASSERT(class_num != DUK_HOBJECT_CLASS_DECENV);

			(void) duk_push_object_helper(thr,
			                              DUK_HOBJECT_FLAG_FASTREFS | DUK_HOBJECT_FLAG_EXTENSIBLE,
			                              -1); /* no prototype or class yet */
		}

		h = duk_known_hobject(thr, -1);
		DUK_HOBJECT_SET_CLASS_NUMBER(h, class_num);

		if (i < DUK_NUM_BUILTINS) {
			thr->builtins[i] = h;
			DUK_HOBJECT_INCREF(thr, &h->hdr);
		}

		if (len >= 0) {
			/* In ES2015+ built-in function object .length property
			 * has property attributes C (configurable only):
			 * http://www.ecma-international.org/ecma-262/7.0/#sec-ecmascript-standard-built-in-objects
			 *
			 * Array.prototype remains an Array instance in ES2015+
			 * and its length has attributes W (writable only).
			 * Because .length is now virtual for duk_harray, it is
			 * not encoded explicitly in init data.
			 */

			DUK_ASSERT(class_num != DUK_HOBJECT_CLASS_ARRAY); /* .length is virtual */
			duk_push_int(thr, len);
			duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_LENGTH, DUK_PROPDESC_FLAGS_C);
		}

		/* enable exotic behaviors last */

		if (class_num == DUK_HOBJECT_CLASS_ARRAY) {
			DUK_ASSERT(DUK_HOBJECT_HAS_EXOTIC_ARRAY(h)); /* set by duk_push_array() */
		}
		if (class_num == DUK_HOBJECT_CLASS_STRING) {
			DUK_HOBJECT_SET_EXOTIC_STRINGOBJ(h);
		}

		/* some assertions */

		DUK_ASSERT(DUK_HOBJECT_HAS_EXTENSIBLE(h));
		/* DUK_HOBJECT_FLAG_CONSTRUCTABLE varies */
		DUK_ASSERT(!DUK_HOBJECT_HAS_BOUNDFUNC(h));
		DUK_ASSERT(!DUK_HOBJECT_HAS_COMPFUNC(h));
		/* DUK_HOBJECT_FLAG_NATFUNC varies */
		DUK_ASSERT(!DUK_HOBJECT_IS_THREAD(h));
		DUK_ASSERT(!DUK_HOBJECT_IS_PROXY(h));
		DUK_ASSERT(!DUK_HOBJECT_HAS_ARRAY_PART(h) || class_num == DUK_HOBJECT_CLASS_ARRAY);
		/* DUK_HOBJECT_FLAG_STRICT varies */
		DUK_ASSERT(!DUK_HOBJECT_HAS_NATFUNC(h) || /* all native functions have NEWENV */
		           DUK_HOBJECT_HAS_NEWENV(h));
		DUK_ASSERT(!DUK_HOBJECT_HAS_NAMEBINDING(h));
		DUK_ASSERT(!DUK_HOBJECT_HAS_CREATEARGS(h));
		/* DUK_HOBJECT_FLAG_EXOTIC_ARRAY varies */
		/* DUK_HOBJECT_FLAG_EXOTIC_STRINGOBJ varies */
		DUK_ASSERT(!DUK_HOBJECT_HAS_EXOTIC_ARGUMENTS(h));

		DUK_DDD(DUK_DDDPRINT("created built-in %ld, class=%ld, length=%ld", (long) i, (long) class_num, (long) len));
	}

	/*
	 *  Then decode the builtins init data (see genbuiltins.py) to
	 *  init objects.  Internal prototypes are set at this stage,
	 *  with thr->builtins[] populated.
	 */

	DUK_DD(DUK_DDPRINT("initialize built-in object properties"));
	for (i = 0; i < DUK_NUM_ALL_BUILTINS; i++) {
		duk_small_uint_t t;
		duk_small_uint_t num;

		DUK_DDD(DUK_DDDPRINT("initializing built-in object at index %ld", (long) i));
		h = duk_known_hobject(thr, (duk_idx_t) i);

		t = (duk_small_uint_t) duk_bd_decode_varuint(bd);
		if (t > 0) {
			t--;
			DUK_DDD(DUK_DDDPRINT("set internal prototype: built-in %ld", (long) t));
			DUK_HOBJECT_SET_PROTOTYPE_UPDREF(thr, h, duk_known_hobject(thr, (duk_idx_t) t));
		} else if (DUK_HOBJECT_IS_NATFUNC(h)) {
			/* Standard native built-ins cannot inherit from
			 * %NativeFunctionPrototype%, they are required to
			 * inherit from Function.prototype directly.
			 */
			DUK_ASSERT(thr->builtins[DUK_BIDX_FUNCTION_PROTOTYPE] != NULL);
			DUK_HOBJECT_SET_PROTOTYPE_UPDREF(thr, h, thr->builtins[DUK_BIDX_FUNCTION_PROTOTYPE]);
		}

		t = (duk_small_uint_t) duk_bd_decode_varuint(bd);
		if (t > 0) {
			/* 'prototype' property for all built-in objects (which have it) has attributes:
			 *  [[Writable]] = false,
			 *  [[Enumerable]] = false,
			 *  [[Configurable]] = false
			 */
			t--;
			DUK_DDD(DUK_DDDPRINT("set external prototype: built-in %ld", (long) t));
			duk_dup(thr, (duk_idx_t) t);
			duk_xdef_prop_stridx(thr, (duk_idx_t) i, DUK_STRIDX_PROTOTYPE, DUK_PROPDESC_FLAGS_NONE);
		}

		t = (duk_small_uint_t) duk_bd_decode_varuint(bd);
		if (t > 0) {
			/* 'constructor' property for all built-in objects (which have it) has attributes:
			 *  [[Writable]] = true,
			 *  [[Enumerable]] = false,
			 *  [[Configurable]] = true
			 */
			t--;
			DUK_DDD(DUK_DDDPRINT("set external constructor: built-in %ld", (long) t));
			duk_dup(thr, (duk_idx_t) t);
			duk_xdef_prop_stridx(thr, (duk_idx_t) i, DUK_STRIDX_CONSTRUCTOR, DUK_PROPDESC_FLAGS_WC);
		}

		/* normal valued properties */
		num = (duk_small_uint_t) duk_bd_decode_varuint(bd);
		DUK_DDD(DUK_DDDPRINT("built-in object %ld, %ld normal valued properties", (long) i, (long) num));
		for (j = 0; j < num; j++) {
			duk_small_uint_t defprop_flags;

			duk__push_stridx_or_string(thr, bd);

			/*
			 *  Property attribute defaults are defined in E5 Section 15 (first
			 *  few pages); there is a default for all properties and a special
			 *  default for 'length' properties.  Variation from the defaults is
			 *  signaled using a single flag bit in the bitstream.
			 */

			defprop_flags = (duk_small_uint_t) duk_bd_decode_flagged(bd,
			                                                         DUK__PROP_FLAGS_BITS,
			                                                         (duk_uint32_t) DUK_PROPDESC_FLAGS_WC);
			defprop_flags |= DUK_DEFPROP_FORCE | DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_HAVE_WRITABLE |
			                 DUK_DEFPROP_HAVE_ENUMERABLE |
			                 DUK_DEFPROP_HAVE_CONFIGURABLE; /* Defaults for data properties. */

			/* The writable, enumerable, configurable flags in prop_flags
			 * match both duk_def_prop() and internal property flags.
			 */
			DUK_ASSERT(DUK_PROPDESC_FLAG_WRITABLE == DUK_DEFPROP_WRITABLE);
			DUK_ASSERT(DUK_PROPDESC_FLAG_ENUMERABLE == DUK_DEFPROP_ENUMERABLE);
			DUK_ASSERT(DUK_PROPDESC_FLAG_CONFIGURABLE == DUK_DEFPROP_CONFIGURABLE);

			t = (duk_small_uint_t) duk_bd_decode(bd, DUK__PROP_TYPE_BITS);

			DUK_DDD(DUK_DDDPRINT("built-in %ld, normal-valued property %ld, key %!T, flags 0x%02lx, type %ld",
			                     (long) i,
			                     (long) j,
			                     duk_get_tval(thr, -1),
			                     (unsigned long) defprop_flags,
			                     (long) t));

			switch (t) {
			case DUK__PROP_TYPE_DOUBLE: {
				duk__push_double(thr, bd);
				break;
			}
			case DUK__PROP_TYPE_STRING: {
				duk__push_string(thr, bd);
				break;
			}
			case DUK__PROP_TYPE_STRIDX: {
				duk__push_stridx(thr, bd);
				break;
			}
			case DUK__PROP_TYPE_BUILTIN: {
				duk_small_uint_t bidx;

				bidx = (duk_small_uint_t) duk_bd_decode_varuint(bd);
				duk_dup(thr, (duk_idx_t) bidx);
				break;
			}
			case DUK__PROP_TYPE_UNDEFINED: {
				duk_push_undefined(thr);
				break;
			}
			case DUK__PROP_TYPE_BOOLEAN_TRUE: {
				duk_push_true(thr);
				break;
			}
			case DUK__PROP_TYPE_BOOLEAN_FALSE: {
				duk_push_false(thr);
				break;
			}
			case DUK__PROP_TYPE_ACCESSOR: {
				duk_small_uint_t natidx_getter = (duk_small_uint_t) duk_bd_decode_varuint(bd);
				duk_small_uint_t natidx_setter = (duk_small_uint_t) duk_bd_decode_varuint(bd);
				duk_small_uint_t accessor_magic = (duk_small_uint_t) duk_bd_decode_varuint(bd);
				duk_c_function c_func_getter;
				duk_c_function c_func_setter;

				DUK_DDD(DUK_DDDPRINT(
				    "built-in accessor property: objidx=%ld, key=%!T, getteridx=%ld, setteridx=%ld, flags=0x%04lx",
				    (long) i,
				    duk_get_tval(thr, -1),
				    (long) natidx_getter,
				    (long) natidx_setter,
				    (unsigned long) defprop_flags));

				c_func_getter = duk_bi_native_functions[natidx_getter];
				if (c_func_getter != NULL) {
					duk_push_c_function_builtin_noconstruct(thr, c_func_getter, 0); /* always 0 args */
					duk_set_magic(thr, -1, (duk_int_t) accessor_magic);
					defprop_flags |= DUK_DEFPROP_HAVE_GETTER;
				}
				c_func_setter = duk_bi_native_functions[natidx_setter];
				if (c_func_setter != NULL) {
					duk_push_c_function_builtin_noconstruct(thr, c_func_setter, 1); /* always 1 arg */
					duk_set_magic(thr, -1, (duk_int_t) accessor_magic);
					defprop_flags |= DUK_DEFPROP_HAVE_SETTER;
				}

				/* Writable flag doesn't make sense for an accessor. */
				DUK_ASSERT((defprop_flags & DUK_PROPDESC_FLAG_WRITABLE) == 0); /* genbuiltins.py ensures */

				defprop_flags &= ~(DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_HAVE_WRITABLE);
				defprop_flags |= DUK_DEFPROP_HAVE_ENUMERABLE | DUK_DEFPROP_HAVE_CONFIGURABLE;
				break;
			}
			default: {
				/* exhaustive */
				DUK_UNREACHABLE();
			}
			}

			duk_def_prop(thr, (duk_idx_t) i, defprop_flags);
			DUK_ASSERT_TOP(thr, DUK_NUM_ALL_BUILTINS);
		}

		/* native function properties */
		num = (duk_small_uint_t) duk_bd_decode_varuint(bd);
		DUK_DDD(DUK_DDDPRINT("built-in object %ld, %ld function valued properties", (long) i, (long) num));
		for (j = 0; j < num; j++) {
			duk_hstring *h_key;
			duk_small_uint_t natidx;
			duk_int_t c_nargs; /* must hold DUK_VARARGS */
			duk_small_uint_t c_length;
			duk_int16_t magic;
			duk_c_function c_func;
			duk_hnatfunc *h_func;
#if defined(DUK_USE_LIGHTFUNC_BUILTINS)
			duk_small_int_t lightfunc_eligible;
#endif
			duk_small_uint_t defprop_flags;

			duk__push_stridx_or_string(thr, bd);
			h_key = duk_known_hstring(thr, -1);
			DUK_UNREF(h_key);
			natidx = (duk_small_uint_t) duk_bd_decode_varuint(bd);

			c_length = (duk_small_uint_t) duk_bd_decode(bd, DUK__LENGTH_PROP_BITS);
			c_nargs = (duk_int_t) duk_bd_decode_flagged(bd, DUK__NARGS_BITS, (duk_uint32_t) c_length /*def_value*/);
			if (c_nargs == DUK__NARGS_VARARGS_MARKER) {
				c_nargs = DUK_VARARGS;
			}

			c_func = duk_bi_native_functions[natidx];

			DUK_DDD(
			    DUK_DDDPRINT("built-in %ld, function-valued property %ld, key %!O, natidx %ld, length %ld, nargs %ld",
			                 (long) i,
			                 (long) j,
			                 (duk_heaphdr *) h_key,
			                 (long) natidx,
			                 (long) c_length,
			                 (c_nargs == DUK_VARARGS ? (long) -1 : (long) c_nargs)));

			/* Cast converts magic to 16-bit signed value */
			magic = (duk_int16_t) duk_bd_decode_varuint(bd);

#if defined(DUK_USE_LIGHTFUNC_BUILTINS)
			lightfunc_eligible =
			    ((c_nargs >= DUK_LFUNC_NARGS_MIN && c_nargs <= DUK_LFUNC_NARGS_MAX) || (c_nargs == DUK_VARARGS)) &&
			    (c_length <= DUK_LFUNC_LENGTH_MAX) && (magic >= DUK_LFUNC_MAGIC_MIN && magic <= DUK_LFUNC_MAGIC_MAX);

			/* These functions have trouble working as lightfuncs.
			 * Some of them have specific asserts and some may have
			 * additional properties (e.g. 'require.id' may be written).
			 */
			if (c_func == duk_bi_global_object_eval) {
				lightfunc_eligible = 0;
			}
#if defined(DUK_USE_COROUTINE_SUPPORT)
			if (c_func == duk_bi_thread_yield || c_func == duk_bi_thread_resume) {
				lightfunc_eligible = 0;
			}
#endif
			if (c_func == duk_bi_function_prototype_call || c_func == duk_bi_function_prototype_apply ||
			    c_func == duk_bi_reflect_apply || c_func == duk_bi_reflect_construct) {
				lightfunc_eligible = 0;
			}

			if (lightfunc_eligible) {
				duk_tval tv_lfunc;
				duk_small_uint_t lf_nargs =
				    (duk_small_uint_t) (c_nargs == DUK_VARARGS ? DUK_LFUNC_NARGS_VARARGS : c_nargs);
				duk_small_uint_t lf_flags = DUK_LFUNC_FLAGS_PACK(magic, c_length, lf_nargs);
				DUK_TVAL_SET_LIGHTFUNC(&tv_lfunc, c_func, lf_flags);
				duk_push_tval(thr, &tv_lfunc);
				DUK_D(DUK_DPRINT("built-in function eligible as light function: i=%d, j=%d c_length=%ld, "
				                 "c_nargs=%ld, magic=%ld -> %!iT",
				                 (int) i,
				                 (int) j,
				                 (long) c_length,
				                 (long) c_nargs,
				                 (long) magic,
				                 duk_get_tval(thr, -1)));
				goto lightfunc_skip;
			}

			DUK_D(DUK_DPRINT(
			    "built-in function NOT ELIGIBLE as light function: i=%d, j=%d c_length=%ld, c_nargs=%ld, magic=%ld",
			    (int) i,
			    (int) j,
			    (long) c_length,
			    (long) c_nargs,
			    (long) magic));
#endif /* DUK_USE_LIGHTFUNC_BUILTINS */

			/* [ (builtin objects) name ] */

			duk_push_c_function_builtin_noconstruct(thr, c_func, c_nargs);
			h_func = duk_known_hnatfunc(thr, -1);
			DUK_UNREF(h_func);

			/* XXX: add into init data? */

			/* Special call handling, not described in init data. */
			if (c_func == duk_bi_global_object_eval || c_func == duk_bi_function_prototype_call ||
			    c_func == duk_bi_function_prototype_apply || c_func == duk_bi_reflect_apply ||
			    c_func == duk_bi_reflect_construct) {
				DUK_HOBJECT_SET_SPECIAL_CALL((duk_hobject *) h_func);
			}

			/* Currently all built-in native functions are strict.
			 * This doesn't matter for many functions, but e.g.
			 * String.prototype.charAt (and other string functions)
			 * rely on being strict so that their 'this' binding is
			 * not automatically coerced.
			 */
			DUK_HOBJECT_SET_STRICT((duk_hobject *) h_func);

			/* No built-in functions are constructable except the top
			 * level ones (Number, etc).
			 */
			DUK_ASSERT(!DUK_HOBJECT_HAS_CONSTRUCTABLE((duk_hobject *) h_func));

			/* XXX: any way to avoid decoding magic bit; there are quite
			 * many function properties and relatively few with magic values.
			 */
			h_func->magic = magic;

			/* [ (builtin objects) name func ] */

			duk_push_uint(thr, c_length);
			duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_LENGTH, DUK_PROPDESC_FLAGS_C);

			duk_dup_m2(thr);
			duk_xdef_prop_stridx_short(thr, -2, DUK_STRIDX_NAME, DUK_PROPDESC_FLAGS_C);

			/* XXX: other properties of function instances; 'arguments', 'caller'. */

			DUK_DD(DUK_DDPRINT("built-in object %ld, function property %ld -> %!T",
			                   (long) i,
			                   (long) j,
			                   (duk_tval *) duk_get_tval(thr, -1)));

			/* [ (builtin objects) name func ] */

			/*
			 *  The default property attributes are correct for all
			 *  function valued properties of built-in objects now.
			 */

#if defined(DUK_USE_LIGHTFUNC_BUILTINS)
		lightfunc_skip:
#endif

			defprop_flags = (duk_small_uint_t) duk_bd_decode_flagged(bd,
			                                                         DUK__PROP_FLAGS_BITS,
			                                                         (duk_uint32_t) DUK_PROPDESC_FLAGS_WC);
			defprop_flags |= DUK_DEFPROP_FORCE | DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_HAVE_WRITABLE |
			                 DUK_DEFPROP_HAVE_ENUMERABLE | DUK_DEFPROP_HAVE_CONFIGURABLE;
			DUK_ASSERT(DUK_PROPDESC_FLAG_WRITABLE == DUK_DEFPROP_WRITABLE);
			DUK_ASSERT(DUK_PROPDESC_FLAG_ENUMERABLE == DUK_DEFPROP_ENUMERABLE);
			DUK_ASSERT(DUK_PROPDESC_FLAG_CONFIGURABLE == DUK_DEFPROP_CONFIGURABLE);

			duk_def_prop(thr, (duk_idx_t) i, defprop_flags);

			/* [ (builtin objects) ] */
		}
	}

	/*
	 *  Special post-tweaks, for cases not covered by the init data format.
	 *
	 *  - Set Date.prototype.toGMTString to Date.prototype.toUTCString.
	 *    toGMTString is required to have the same Function object as
	 *    toUTCString in E5 Section B.2.6.  Note that while Smjs respects
	 *    this, V8 does not (the Function objects are distinct).
	 *
	 *  - Make DoubleError non-extensible.
	 *
	 *  - Add info about most important effective compile options to Duktape.
	 *
	 *  - Possibly remove some properties (values or methods) which are not
	 *    desirable with current feature options but are not currently
	 *    conditional in init data.
	 */

#if defined(DUK_USE_DATE_BUILTIN)
	duk_get_prop_stridx_short(thr, DUK_BIDX_DATE_PROTOTYPE, DUK_STRIDX_TO_UTC_STRING);
	duk_xdef_prop_stridx_short(thr, DUK_BIDX_DATE_PROTOTYPE, DUK_STRIDX_TO_GMT_STRING, DUK_PROPDESC_FLAGS_WC);
#endif

	h = duk_known_hobject(thr, DUK_BIDX_DOUBLE_ERROR);
	DUK_HOBJECT_CLEAR_EXTENSIBLE(h);

#if !defined(DUK_USE_ES6_OBJECT_PROTO_PROPERTY)
	DUK_DD(DUK_DDPRINT("delete Object.prototype.__proto__ built-in which is not enabled in features"));
	(void) duk_hobject_delprop_raw(thr,
	                               thr->builtins[DUK_BIDX_OBJECT_PROTOTYPE],
	                               DUK_HTHREAD_STRING___PROTO__(thr),
	                               DUK_DELPROP_FLAG_THROW);
#endif

#if !defined(DUK_USE_ES6_OBJECT_SETPROTOTYPEOF)
	DUK_DD(DUK_DDPRINT("delete Object.setPrototypeOf built-in which is not enabled in features"));
	(void) duk_hobject_delprop_raw(thr,
	                               thr->builtins[DUK_BIDX_OBJECT_CONSTRUCTOR],
	                               DUK_HTHREAD_STRING_SET_PROTOTYPE_OF(thr),
	                               DUK_DELPROP_FLAG_THROW);
#endif

	/* XXX: relocate */
	duk_push_string(thr,
	/* Endianness indicator */
#if defined(DUK_USE_INTEGER_LE)
	                "l"
#elif defined(DUK_USE_INTEGER_BE)
	                "b"
#elif defined(DUK_USE_INTEGER_ME) /* integer mixed endian not really used now */
	                "m"
#else
	                "?"
#endif
#if defined(DUK_USE_DOUBLE_LE)
	                "l"
#elif defined(DUK_USE_DOUBLE_BE)
	                "b"
#elif defined(DUK_USE_DOUBLE_ME)
	                "m"
#else
	                "?"
#endif
	                " "
	/* Packed or unpacked tval */
#if defined(DUK_USE_PACKED_TVAL)
	                "p"
#else
	                "u"
#endif
#if defined(DUK_USE_FASTINT)
	                "f"
#endif
	                " "
	/* Low memory/performance options */
#if defined(DUK_USE_STRTAB_PTRCOMP)
	                "s"
#endif
#if !defined(DUK_USE_HEAPPTR16) && !defined(DUK_DATAPTR16) && !defined(DUK_FUNCPTR16)
	                "n"
#endif
#if defined(DUK_USE_HEAPPTR16)
	                "h"
#endif
#if defined(DUK_USE_DATAPTR16)
	                "d"
#endif
#if defined(DUK_USE_FUNCPTR16)
	                "f"
#endif
#if defined(DUK_USE_REFCOUNT16)
	                "R"
#endif
#if defined(DUK_USE_STRHASH16)
	                "H"
#endif
#if defined(DUK_USE_STRLEN16)
	                "S"
#endif
#if defined(DUK_USE_BUFLEN16)
	                "B"
#endif
#if defined(DUK_USE_OBJSIZES16)
	                "O"
#endif
#if defined(DUK_USE_LIGHTFUNC_BUILTINS)
	                "L"
#endif
#if defined(DUK_USE_ROM_STRINGS) || defined(DUK_USE_ROM_OBJECTS)
	                /* XXX: This won't be shown in practice now
	                 * because this code is not run when builtins
	                 * are in ROM.
	                 */
	                "Z"
#endif
#if defined(DUK_USE_LITCACHE_SIZE)
	                "l"
#endif
	                " "
	/* Object property allocation layout */
#if defined(DUK_USE_HOBJECT_LAYOUT_1)
	                "p1"
#elif defined(DUK_USE_HOBJECT_LAYOUT_2)
	                "p2"
#elif defined(DUK_USE_HOBJECT_LAYOUT_3)
	                "p3"
#else
	                "p?"
#endif
	                " "
	/* Alignment guarantee */
#if (DUK_USE_ALIGN_BY == 4)
	                "a4"
#elif (DUK_USE_ALIGN_BY == 8)
	                "a8"
#elif (DUK_USE_ALIGN_BY == 1)
	                "a1"
#else
#error invalid DUK_USE_ALIGN_BY
#endif
	                " "
	                /* Architecture, OS, and compiler strings */
	                DUK_USE_ARCH_STRING " " DUK_USE_OS_STRING " " DUK_USE_COMPILER_STRING);
	duk_xdef_prop_stridx_short(thr, DUK_BIDX_DUKTAPE, DUK_STRIDX_ENV, DUK_PROPDESC_FLAGS_WC);

	/*
	 *  Since built-ins are not often extended, compact them.
	 */

	DUK_DD(DUK_DDPRINT("compact built-ins"));
	for (i = 0; i < DUK_NUM_ALL_BUILTINS; i++) {
		duk_hobject_compact_props(thr, duk_known_hobject(thr, (duk_idx_t) i));
	}

	DUK_D(DUK_DPRINT("INITBUILTINS END"));

#if defined(DUK_USE_DEBUG_LEVEL) && (DUK_USE_DEBUG_LEVEL >= 1)
	for (i = 0; i < DUK_NUM_ALL_BUILTINS; i++) {
		DUK_DD(DUK_DDPRINT("built-in object %ld after initialization and compacting: %!@iO",
		                   (long) i,
		                   (duk_heaphdr *) duk_require_hobject(thr, (duk_idx_t) i)));
	}
#endif

	/*
	 *  Pop built-ins from stack: they are now INCREF'd and
	 *  reachable from the builtins[] array or indirectly
	 *  through builtins[].
	 */

	duk_set_top(thr, 0);
	DUK_ASSERT_TOP(thr, 0);
}
#endif /* DUK_USE_ROM_OBJECTS */

DUK_INTERNAL void duk_hthread_copy_builtin_objects(duk_hthread *thr_from, duk_hthread *thr_to) {
	duk_small_uint_t i;

	for (i = 0; i < DUK_NUM_BUILTINS; i++) {
		thr_to->builtins[i] = thr_from->builtins[i];
		DUK_HOBJECT_INCREF_ALLOWNULL(thr_to, thr_to->builtins[i]); /* side effect free */
	}
}
