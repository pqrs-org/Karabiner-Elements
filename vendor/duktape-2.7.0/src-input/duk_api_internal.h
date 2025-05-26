/*
 *  Internal API calls which have (stack and other) semantics similar
 *  to the public API.
 */

#if !defined(DUK_API_INTERNAL_H_INCLUDED)
#define DUK_API_INTERNAL_H_INCLUDED

/* Inline macro helpers. */
#if defined(DUK_USE_PREFER_SIZE)
#define DUK_INLINE_PERF
#define DUK_ALWAYS_INLINE_PERF
#define DUK_NOINLINE_PERF
#else
#define DUK_INLINE_PERF        DUK_INLINE
#define DUK_ALWAYS_INLINE_PERF DUK_ALWAYS_INLINE
#define DUK_NOINLINE_PERF      DUK_NOINLINE
#endif

/* Inline macro helpers, for bytecode executor. */
#if defined(DUK_USE_EXEC_PREFER_SIZE)
#define DUK_EXEC_INLINE_PERF
#define DUK_EXEC_ALWAYS_INLINE_PERF
#define DUK_EXEC_NOINLINE_PERF
#else
#define DUK_EXEC_INLINE_PERF        DUK_INLINE
#define DUK_EXEC_ALWAYS_INLINE_PERF DUK_ALWAYS_INLINE
#define DUK_EXEC_NOINLINE_PERF      DUK_NOINLINE
#endif

/* duk_push_sprintf constants */
#define DUK_PUSH_SPRINTF_INITIAL_SIZE 256L
#define DUK_PUSH_SPRINTF_SANITY_LIMIT (1L * 1024L * 1024L * 1024L)

/* Flag ORed to err_code to indicate __FILE__ / __LINE__ is not
 * blamed as source of error for error fileName / lineNumber.
 */
#define DUK_ERRCODE_FLAG_NOBLAME_FILELINE (1L << 24)

/* Current convention is to use duk_size_t for value stack sizes and global indices,
 * and duk_idx_t for local frame indices.
 */
DUK_INTERNAL_DECL void duk_valstack_grow_check_throw(duk_hthread *thr, duk_size_t min_bytes);
DUK_INTERNAL_DECL duk_bool_t duk_valstack_grow_check_nothrow(duk_hthread *thr, duk_size_t min_bytes);
DUK_INTERNAL_DECL void duk_valstack_shrink_check_nothrow(duk_hthread *thr, duk_bool_t snug);

DUK_INTERNAL_DECL void duk_copy_tvals_incref(duk_hthread *thr, duk_tval *tv_dst, duk_tval *tv_src, duk_size_t count);

DUK_INTERNAL_DECL duk_tval *duk_reserve_gap(duk_hthread *thr, duk_idx_t idx_base, duk_idx_t count);

DUK_INTERNAL_DECL void duk_set_top_unsafe(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL void duk_set_top_and_wipe(duk_hthread *thr, duk_idx_t top, duk_idx_t idx_wipe_start);

DUK_INTERNAL_DECL void duk_dup_0(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_dup_1(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_dup_2(duk_hthread *thr);
/* duk_dup_m1() would be same as duk_dup_top() */
DUK_INTERNAL_DECL void duk_dup_m2(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_dup_m3(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_dup_m4(duk_hthread *thr);

DUK_INTERNAL_DECL void duk_remove_unsafe(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL void duk_remove_m2(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_remove_n(duk_hthread *thr, duk_idx_t idx, duk_idx_t count);
DUK_INTERNAL_DECL void duk_remove_n_unsafe(duk_hthread *thr, duk_idx_t idx, duk_idx_t count);

DUK_INTERNAL_DECL duk_int_t duk_get_type_tval(duk_tval *tv);
DUK_INTERNAL_DECL duk_uint_t duk_get_type_mask_tval(duk_tval *tv);

#if defined(DUK_USE_VERBOSE_ERRORS) && defined(DUK_USE_PARANOID_ERRORS)
DUK_INTERNAL_DECL const char *duk_get_type_name(duk_hthread *thr, duk_idx_t idx);
#endif
DUK_INTERNAL_DECL duk_small_uint_t duk_get_class_number(duk_hthread *thr, duk_idx_t idx);

DUK_INTERNAL_DECL duk_tval *duk_get_tval(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_tval *duk_get_tval_or_unused(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_tval *duk_require_tval(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL void duk_push_tval(duk_hthread *thr, duk_tval *tv);

/* Push the current 'this' binding; throw TypeError if binding is not object
 * coercible (CheckObjectCoercible).
 */
DUK_INTERNAL_DECL void duk_push_this_check_object_coercible(duk_hthread *thr);

/* duk_push_this() + CheckObjectCoercible() + duk_to_object() */
DUK_INTERNAL_DECL duk_hobject *duk_push_this_coercible_to_object(duk_hthread *thr);

/* duk_push_this() + CheckObjectCoercible() + duk_to_string() */
DUK_INTERNAL_DECL duk_hstring *duk_push_this_coercible_to_string(duk_hthread *thr);

DUK_INTERNAL_DECL duk_hstring *duk_push_uint_to_hstring(duk_hthread *thr, duk_uint_t i);

/* Get a borrowed duk_tval pointer to the current 'this' binding.  Caller must
 * make sure there's an active callstack entry.  Note that the returned pointer
 * is unstable with regards to side effects.
 */
DUK_INTERNAL_DECL duk_tval *duk_get_borrowed_this_tval(duk_hthread *thr);

/* XXX: add fastint support? */
#define duk_push_u64(thr, val) duk_push_number((thr), (duk_double_t) (val))
#define duk_push_i64(thr, val) duk_push_number((thr), (duk_double_t) (val))

/* duk_push_(u)int() is guaranteed to support at least (un)signed 32-bit range */
#define duk_push_u32(thr, val) duk_push_uint((thr), (duk_uint_t) (val))
#define duk_push_i32(thr, val) duk_push_int((thr), (duk_int_t) (val))

/* sometimes stack and array indices need to go on the stack */
#define duk_push_idx(thr, val)     duk_push_int((thr), (duk_int_t) (val))
#define duk_push_uarridx(thr, val) duk_push_uint((thr), (duk_uint_t) (val))
#define duk_push_size_t(thr, val)  duk_push_uint((thr), (duk_uint_t) (val)) /* XXX: assumed to fit for now */

DUK_INTERNAL_DECL duk_bool_t duk_is_string_notsymbol(duk_hthread *thr, duk_idx_t idx);

DUK_INTERNAL_DECL duk_bool_t duk_is_callable_tval(duk_hthread *thr, duk_tval *tv);

DUK_INTERNAL_DECL duk_bool_t duk_is_bare_object(duk_hthread *thr, duk_idx_t idx);

DUK_INTERNAL_DECL duk_hstring *duk_get_hstring(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hstring *duk_get_hstring_notsymbol(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL const char *duk_get_string_notsymbol(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hobject *duk_get_hobject(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hbuffer *duk_get_hbuffer(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hthread *duk_get_hthread(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hcompfunc *duk_get_hcompfunc(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hnatfunc *duk_get_hnatfunc(duk_hthread *thr, duk_idx_t idx);

DUK_INTERNAL_DECL void *duk_get_buffer_data_raw(duk_hthread *thr,
                                                duk_idx_t idx,
                                                duk_size_t *out_size,
                                                void *def_ptr,
                                                duk_size_t def_len,
                                                duk_bool_t throw_flag,
                                                duk_bool_t *out_isbuffer);

DUK_INTERNAL_DECL duk_hobject *duk_get_hobject_with_class(duk_hthread *thr, duk_idx_t idx, duk_small_uint_t classnum);

DUK_INTERNAL_DECL duk_hobject *duk_get_hobject_promote_mask(duk_hthread *thr, duk_idx_t idx, duk_uint_t type_mask);
DUK_INTERNAL_DECL duk_hobject *duk_require_hobject_promote_mask(duk_hthread *thr, duk_idx_t idx, duk_uint_t type_mask);
DUK_INTERNAL_DECL duk_hobject *duk_require_hobject_accept_mask(duk_hthread *thr, duk_idx_t idx, duk_uint_t type_mask);
#define duk_require_hobject_promote_lfunc(thr, idx) duk_require_hobject_promote_mask((thr), (idx), DUK_TYPE_MASK_LIGHTFUNC)
#define duk_get_hobject_promote_lfunc(thr, idx)     duk_get_hobject_promote_mask((thr), (idx), DUK_TYPE_MASK_LIGHTFUNC)

#if 0 /*unused*/
DUK_INTERNAL_DECL void *duk_get_voidptr(duk_hthread *thr, duk_idx_t idx);
#endif

DUK_INTERNAL_DECL duk_hstring *duk_known_hstring(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hobject *duk_known_hobject(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hbuffer *duk_known_hbuffer(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hcompfunc *duk_known_hcompfunc(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hnatfunc *duk_known_hnatfunc(duk_hthread *thr, duk_idx_t idx);

DUK_INTERNAL_DECL duk_double_t duk_to_number_tval(duk_hthread *thr, duk_tval *tv);

DUK_INTERNAL_DECL duk_hstring *duk_to_hstring(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hstring *duk_to_hstring_m1(duk_hthread *thr);
DUK_INTERNAL_DECL duk_hstring *duk_to_hstring_acceptsymbol(duk_hthread *thr, duk_idx_t idx);

DUK_INTERNAL_DECL duk_hobject *duk_to_hobject(duk_hthread *thr, duk_idx_t idx);

DUK_INTERNAL_DECL duk_double_t duk_to_number_m1(duk_hthread *thr);
DUK_INTERNAL_DECL duk_double_t duk_to_number_m2(duk_hthread *thr);

DUK_INTERNAL_DECL duk_bool_t duk_to_boolean_top_pop(duk_hthread *thr);

#if defined(DUK_USE_DEBUGGER_SUPPORT) /* only needed by debugger for now */
DUK_INTERNAL_DECL duk_hstring *duk_safe_to_hstring(duk_hthread *thr, duk_idx_t idx);
#endif
DUK_INTERNAL_DECL void duk_push_class_string_tval(duk_hthread *thr, duk_tval *tv, duk_bool_t avoid_side_effects);

DUK_INTERNAL_DECL duk_int_t duk_to_int_clamped_raw(duk_hthread *thr,
                                                   duk_idx_t idx,
                                                   duk_int_t minval,
                                                   duk_int_t maxval,
                                                   duk_bool_t *out_clamped); /* out_clamped=NULL, RangeError if outside range */
DUK_INTERNAL_DECL duk_int_t duk_to_int_clamped(duk_hthread *thr, duk_idx_t idx, duk_int_t minval, duk_int_t maxval);
DUK_INTERNAL_DECL duk_int_t duk_to_int_check_range(duk_hthread *thr, duk_idx_t idx, duk_int_t minval, duk_int_t maxval);
#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL_DECL duk_uint8_t duk_to_uint8clamped(duk_hthread *thr, duk_idx_t idx);
#endif
DUK_INTERNAL_DECL duk_hstring *duk_to_property_key_hstring(duk_hthread *thr, duk_idx_t idx);

DUK_INTERNAL_DECL duk_hstring *duk_require_hstring(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hstring *duk_require_hstring_notsymbol(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL const char *duk_require_lstring_notsymbol(duk_hthread *thr, duk_idx_t idx, duk_size_t *out_len);
DUK_INTERNAL_DECL const char *duk_require_string_notsymbol(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hobject *duk_require_hobject(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hbuffer *duk_require_hbuffer(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hthread *duk_require_hthread(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hcompfunc *duk_require_hcompfunc(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL duk_hnatfunc *duk_require_hnatfunc(duk_hthread *thr, duk_idx_t idx);

DUK_INTERNAL_DECL duk_hobject *duk_require_hobject_with_class(duk_hthread *thr, duk_idx_t idx, duk_small_uint_t classnum);

DUK_INTERNAL_DECL void duk_push_hstring(duk_hthread *thr, duk_hstring *h);
DUK_INTERNAL_DECL void duk_push_hstring_stridx(duk_hthread *thr, duk_small_uint_t stridx);
DUK_INTERNAL_DECL void duk_push_hstring_empty(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_push_hobject(duk_hthread *thr, duk_hobject *h);
DUK_INTERNAL_DECL void duk_push_hbuffer(duk_hthread *thr, duk_hbuffer *h);
#define duk_push_hthread(thr, h)  duk_push_hobject((thr), (duk_hobject *) (h))
#define duk_push_hnatfunc(thr, h) duk_push_hobject((thr), (duk_hobject *) (h))
DUK_INTERNAL_DECL void duk_push_hobject_bidx(duk_hthread *thr, duk_small_int_t builtin_idx);
DUK_INTERNAL_DECL duk_hobject *duk_push_object_helper(duk_hthread *thr,
                                                      duk_uint_t hobject_flags_and_class,
                                                      duk_small_int_t prototype_bidx);
DUK_INTERNAL_DECL duk_hobject *duk_push_object_helper_proto(duk_hthread *thr,
                                                            duk_uint_t hobject_flags_and_class,
                                                            duk_hobject *proto);
DUK_INTERNAL_DECL duk_hcompfunc *duk_push_hcompfunc(duk_hthread *thr);
DUK_INTERNAL_DECL duk_hboundfunc *duk_push_hboundfunc(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_push_c_function_builtin(duk_hthread *thr, duk_c_function func, duk_int_t nargs);
DUK_INTERNAL_DECL void duk_push_c_function_builtin_noconstruct(duk_hthread *thr, duk_c_function func, duk_int_t nargs);

/* XXX: duk_push_harray() and duk_push_hcompfunc() are inconsistent with
 * duk_push_hobject() etc which don't create a new value.
 */
DUK_INTERNAL_DECL duk_harray *duk_push_harray(duk_hthread *thr);
DUK_INTERNAL_DECL duk_harray *duk_push_harray_with_size(duk_hthread *thr, duk_uint32_t size);
DUK_INTERNAL_DECL duk_tval *duk_push_harray_with_size_outptr(duk_hthread *thr, duk_uint32_t size);

DUK_INTERNAL_DECL void duk_push_string_funcptr(duk_hthread *thr, duk_uint8_t *ptr, duk_size_t sz);
DUK_INTERNAL_DECL void duk_push_lightfunc_name_raw(duk_hthread *thr, duk_c_function func, duk_small_uint_t lf_flags);
DUK_INTERNAL_DECL void duk_push_lightfunc_name(duk_hthread *thr, duk_tval *tv);
DUK_INTERNAL_DECL void duk_push_lightfunc_tostring(duk_hthread *thr, duk_tval *tv);
#if 0 /* not used yet */
DUK_INTERNAL_DECL void duk_push_hnatfunc_name(duk_hthread *thr, duk_hnatfunc *h);
#endif
#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
DUK_INTERNAL_DECL duk_hbufobj *duk_push_bufobj_raw(duk_hthread *thr,
                                                   duk_uint_t hobject_flags_and_class,
                                                   duk_small_int_t prototype_bidx);
#endif

DUK_INTERNAL_DECL void *duk_push_fixed_buffer_nozero(duk_hthread *thr, duk_size_t len);
DUK_INTERNAL_DECL void *duk_push_fixed_buffer_zero(duk_hthread *thr, duk_size_t len);

DUK_INTERNAL_DECL const char *duk_push_string_readable(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL const char *duk_push_string_tval_readable(duk_hthread *thr, duk_tval *tv);
DUK_INTERNAL_DECL const char *duk_push_string_tval_readable_error(duk_hthread *thr, duk_tval *tv);

/* The duk_xxx_prop_stridx_short() variants expect their arguments to be short
 * enough to be packed into a single 32-bit integer argument.  Argument limits
 * vary per call; typically 16 bits are assigned to the signed value stack index
 * and the stridx.  In practice these work well for footprint with constant
 * arguments and such call sites are also easiest to verify to be correct.
 */

DUK_INTERNAL_DECL duk_bool_t duk_get_prop_stridx(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t stridx); /* [] -> [val] */
DUK_INTERNAL_DECL duk_bool_t duk_get_prop_stridx_short_raw(duk_hthread *thr, duk_uint_t packed_args);
#define duk_get_prop_stridx_short(thr, obj_idx, stridx) \
	(DUK_ASSERT_EXPR((duk_int_t) (obj_idx) >= -0x8000L && (duk_int_t) (obj_idx) <= 0x7fffL), \
	 DUK_ASSERT_EXPR((duk_int_t) (stridx) >= 0 && (duk_int_t) (stridx) <= 0xffffL), \
	 duk_get_prop_stridx_short_raw((thr), (((duk_uint_t) (obj_idx)) << 16) + ((duk_uint_t) (stridx))))
DUK_INTERNAL_DECL duk_bool_t duk_get_prop_stridx_boolean(duk_hthread *thr,
                                                         duk_idx_t obj_idx,
                                                         duk_small_uint_t stridx,
                                                         duk_bool_t *out_has_prop); /* [] -> [] */

DUK_INTERNAL_DECL duk_bool_t duk_xget_owndataprop(duk_hthread *thr, duk_idx_t obj_idx);
DUK_INTERNAL_DECL duk_bool_t duk_xget_owndataprop_stridx(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t stridx);
DUK_INTERNAL_DECL duk_bool_t duk_xget_owndataprop_stridx_short_raw(duk_hthread *thr, duk_uint_t packed_args);
#define duk_xget_owndataprop_stridx_short(thr, obj_idx, stridx) \
	(DUK_ASSERT_EXPR((duk_int_t) (obj_idx) >= -0x8000L && (duk_int_t) (obj_idx) <= 0x7fffL), \
	 DUK_ASSERT_EXPR((duk_int_t) (stridx) >= 0 && (duk_int_t) (stridx) <= 0xffffL), \
	 duk_xget_owndataprop_stridx_short_raw((thr), (((duk_uint_t) (obj_idx)) << 16) + ((duk_uint_t) (stridx))))

DUK_INTERNAL_DECL duk_bool_t duk_put_prop_stridx(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t stridx); /* [val] -> [] */
DUK_INTERNAL_DECL duk_bool_t duk_put_prop_stridx_short_raw(duk_hthread *thr, duk_uint_t packed_args);
#define duk_put_prop_stridx_short(thr, obj_idx, stridx) \
	(DUK_ASSERT_EXPR((duk_int_t) (obj_idx) >= -0x8000L && (duk_int_t) (obj_idx) <= 0x7fffL), \
	 DUK_ASSERT_EXPR((duk_int_t) (stridx) >= 0 && (duk_int_t) (stridx) <= 0xffffL), \
	 duk_put_prop_stridx_short_raw((thr), (((duk_uint_t) (obj_idx)) << 16) + ((duk_uint_t) (stridx))))

DUK_INTERNAL_DECL duk_bool_t duk_del_prop_stridx(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t stridx); /* [] -> [] */
#if 0 /* Too few call sites to be useful. */
DUK_INTERNAL_DECL duk_bool_t duk_del_prop_stridx_short_raw(duk_hthread *thr, duk_uint_t packed_args);
#define duk_del_prop_stridx_short(thr, obj_idx, stridx) \
	(DUK_ASSERT_EXPR((obj_idx) >= -0x8000L && (obj_idx) <= 0x7fffL), \
	 DUK_ASSERT_EXPR((stridx) >= 0 && (stridx) <= 0xffffL), \
	 duk_del_prop_stridx_short_raw((thr), (((duk_uint_t) (obj_idx)) << 16) + ((duk_uint_t) (stridx))))
#endif
#define duk_del_prop_stridx_short(thr, obj_idx, stridx) duk_del_prop_stridx((thr), (obj_idx), (stridx))

DUK_INTERNAL_DECL duk_bool_t duk_has_prop_stridx(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t stridx); /* [] -> [] */
#if 0 /* Too few call sites to be useful. */
DUK_INTERNAL_DECL duk_bool_t duk_has_prop_stridx_short_raw(duk_hthread *thr, duk_uint_t packed_args);
#define duk_has_prop_stridx_short(thr, obj_idx, stridx) \
	(DUK_ASSERT_EXPR((obj_idx) >= -0x8000L && (obj_idx) <= 0x7fffL), \
	 DUK_ASSERT_EXPR((stridx) >= 0 && (stridx) <= 0xffffL), \
	 duk_has_prop_stridx_short_raw((thr), (((duk_uint_t) (obj_idx)) << 16) + ((duk_uint_t) (stridx))))
#endif
#define duk_has_prop_stridx_short(thr, obj_idx, stridx) duk_has_prop_stridx((thr), (obj_idx), (stridx))

DUK_INTERNAL_DECL void duk_xdef_prop(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t desc_flags); /* [key val] -> [] */

DUK_INTERNAL_DECL void duk_xdef_prop_index(duk_hthread *thr,
                                           duk_idx_t obj_idx,
                                           duk_uarridx_t arr_idx,
                                           duk_small_uint_t desc_flags); /* [val] -> [] */

/* XXX: Because stridx and desc_flags have a limited range, this call could
 * always pack stridx and desc_flags into a single argument.
 */
DUK_INTERNAL_DECL void duk_xdef_prop_stridx(duk_hthread *thr,
                                            duk_idx_t obj_idx,
                                            duk_small_uint_t stridx,
                                            duk_small_uint_t desc_flags); /* [val] -> [] */
DUK_INTERNAL_DECL void duk_xdef_prop_stridx_short_raw(duk_hthread *thr, duk_uint_t packed_args);
#define duk_xdef_prop_stridx_short(thr, obj_idx, stridx, desc_flags) \
	(DUK_ASSERT_EXPR((duk_int_t) (obj_idx) >= -0x80L && (duk_int_t) (obj_idx) <= 0x7fL), \
	 DUK_ASSERT_EXPR((duk_int_t) (stridx) >= 0 && (duk_int_t) (stridx) <= 0xffffL), \
	 DUK_ASSERT_EXPR((duk_int_t) (desc_flags) >= 0 && (duk_int_t) (desc_flags) <= 0xffL), \
	 duk_xdef_prop_stridx_short_raw((thr), \
	                                (((duk_uint_t) (obj_idx)) << 24) + (((duk_uint_t) (stridx)) << 8) + \
	                                    (duk_uint_t) (desc_flags)))

#define duk_xdef_prop_wec(thr, obj_idx)                duk_xdef_prop((thr), (obj_idx), DUK_PROPDESC_FLAGS_WEC)
#define duk_xdef_prop_index_wec(thr, obj_idx, arr_idx) duk_xdef_prop_index((thr), (obj_idx), (arr_idx), DUK_PROPDESC_FLAGS_WEC)
#define duk_xdef_prop_stridx_wec(thr, obj_idx, stridx) duk_xdef_prop_stridx((thr), (obj_idx), (stridx), DUK_PROPDESC_FLAGS_WEC)
#define duk_xdef_prop_stridx_short_wec(thr, obj_idx, stridx) \
	duk_xdef_prop_stridx_short((thr), (obj_idx), (stridx), DUK_PROPDESC_FLAGS_WEC)

#if 0 /*unused*/
DUK_INTERNAL_DECL void duk_xdef_prop_stridx_builtin(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t stridx, duk_small_int_t builtin_idx, duk_small_uint_t desc_flags);  /* [] -> [] */
#endif

DUK_INTERNAL_DECL void duk_xdef_prop_stridx_thrower(duk_hthread *thr, duk_idx_t obj_idx, duk_small_uint_t stridx); /* [] -> [] */

DUK_INTERNAL_DECL duk_bool_t duk_get_method_stridx(duk_hthread *thr, duk_idx_t idx, duk_small_uint_t stridx);

DUK_INTERNAL_DECL void duk_pack(duk_hthread *thr, duk_idx_t count);
DUK_INTERNAL_DECL duk_idx_t duk_unpack_array_like(duk_hthread *thr, duk_idx_t idx);
#if 0
DUK_INTERNAL_DECL void duk_unpack(duk_hthread *thr);
#endif

DUK_INTERNAL_DECL void duk_push_symbol_descriptive_string(duk_hthread *thr, duk_hstring *h);

DUK_INTERNAL_DECL void duk_resolve_nonbound_function(duk_hthread *thr);

DUK_INTERNAL_DECL duk_idx_t duk_get_top_require_min(duk_hthread *thr, duk_idx_t min_top);
DUK_INTERNAL_DECL duk_idx_t duk_get_top_index_unsafe(duk_hthread *thr);

DUK_INTERNAL_DECL void duk_pop_n_unsafe(duk_hthread *thr, duk_idx_t count);
DUK_INTERNAL_DECL void duk_pop_unsafe(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_pop_2_unsafe(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_pop_3_unsafe(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_pop_n_nodecref_unsafe(duk_hthread *thr, duk_idx_t count);
DUK_INTERNAL_DECL void duk_pop_nodecref_unsafe(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_pop_2_nodecref_unsafe(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_pop_3_nodecref_unsafe(duk_hthread *thr);
DUK_INTERNAL_DECL void duk_pop_undefined(duk_hthread *thr);

DUK_INTERNAL_DECL void duk_compact_m1(duk_hthread *thr);

DUK_INTERNAL_DECL void duk_seal_freeze_raw(duk_hthread *thr, duk_idx_t obj_idx, duk_bool_t is_freeze);

DUK_INTERNAL_DECL void duk_insert_undefined(duk_hthread *thr, duk_idx_t idx);
DUK_INTERNAL_DECL void duk_insert_undefined_n(duk_hthread *thr, duk_idx_t idx, duk_idx_t count);

DUK_INTERNAL_DECL void duk_concat_2(duk_hthread *thr);

DUK_INTERNAL_DECL duk_int_t duk_pcall_method_flags(duk_hthread *thr, duk_idx_t nargs, duk_small_uint_t call_flags);

#if defined(DUK_USE_SYMBOL_BUILTIN)
DUK_INTERNAL_DECL void duk_to_primitive_ordinary(duk_hthread *thr, duk_idx_t idx, duk_int_t hint);
#endif

DUK_INTERNAL_DECL void duk_clear_prototype(duk_hthread *thr, duk_idx_t idx);

/* Raw internal valstack access macros: access is unsafe so call site
 * must have a guarantee that the index is valid.  When that is the case,
 * using these macro results in faster and smaller code than duk_get_tval().
 * Both 'ctx' and 'idx' are evaluted multiple times, but only for asserts.
 */
#define DUK_ASSERT_VALID_NEGIDX(thr, idx) \
	(DUK_ASSERT_EXPR((duk_int_t) (idx) < 0), DUK_ASSERT_EXPR(duk_is_valid_index((thr), (idx))))
#define DUK_ASSERT_VALID_POSIDX(thr, idx) \
	(DUK_ASSERT_EXPR((duk_int_t) (idx) >= 0), DUK_ASSERT_EXPR(duk_is_valid_index((thr), (idx))))
#define DUK_GET_TVAL_NEGIDX(thr, idx) (DUK_ASSERT_VALID_NEGIDX((thr), (idx)), ((duk_hthread *) (thr))->valstack_top + (idx))
#define DUK_GET_TVAL_POSIDX(thr, idx) (DUK_ASSERT_VALID_POSIDX((thr), (idx)), ((duk_hthread *) (thr))->valstack_bottom + (idx))
#define DUK_GET_HOBJECT_NEGIDX(thr, idx) \
	(DUK_ASSERT_VALID_NEGIDX((thr), (idx)), DUK_TVAL_GET_OBJECT(((duk_hthread *) (thr))->valstack_top + (idx)))
#define DUK_GET_HOBJECT_POSIDX(thr, idx) \
	(DUK_ASSERT_VALID_POSIDX((thr), (idx)), DUK_TVAL_GET_OBJECT(((duk_hthread *) (thr))->valstack_bottom + (idx)))

#define DUK_GET_THIS_TVAL_PTR(thr) (DUK_ASSERT_EXPR((thr)->valstack_bottom > (thr)->valstack), (thr)->valstack_bottom - 1)

DUK_INTERNAL_DECL duk_double_t duk_time_get_ecmascript_time(duk_hthread *thr);
DUK_INTERNAL_DECL duk_double_t duk_time_get_ecmascript_time_nofrac(duk_hthread *thr);
DUK_INTERNAL_DECL duk_double_t duk_time_get_monotonic_time(duk_hthread *thr);

#endif /* DUK_API_INTERNAL_H_INCLUDED */
