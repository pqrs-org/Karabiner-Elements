//
// execution/set_value.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_SET_VALUE_HPP
#define ASIO_EXECUTION_SET_VALUE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/detail/variadic_templates.hpp"
#include "asio/traits/set_value_member.hpp"
#include "asio/traits/set_value_free.hpp"

#include "asio/detail/push_options.hpp"

#if defined(GENERATING_DOCUMENTATION)

namespace asio {
namespace execution {

/// A customisation point that delivers a value to a receiver.
/**
 * The name <tt>execution::set_value</tt> denotes a customisation point object.
 * The expression <tt>execution::set_value(R, Vs...)</tt> for some
 * subexpressions <tt>R</tt> and <tt>Vs...</tt> is expression-equivalent to:
 *
 * @li <tt>R.set_value(Vs...)</tt>, if that expression is valid. If the
 *   function selected does not send the value(s) <tt>Vs...</tt> to the receiver
 *   <tt>R</tt>'s value channel, the program is ill-formed with no diagnostic
 *   required.
 *
 * @li Otherwise, <tt>set_value(R, Vs...)</tt>, if that expression is valid,
 *   with overload resolution performed in a context that includes the
 *   declaration <tt>void set_value();</tt> and that does not include a
 *   declaration of <tt>execution::set_value</tt>. If the function selected by
 *   overload resolution does not send the value(s) <tt>Vs...</tt> to the
 *   receiver <tt>R</tt>'s value channel, the program is ill-formed with no
 *   diagnostic required.
 *
 * @li Otherwise, <tt>execution::set_value(R, Vs...)</tt> is ill-formed.
 */
inline constexpr unspecified set_value = unspecified;

/// A type trait that determines whether a @c set_value expression is
/// well-formed.
/**
 * Class template @c can_set_value is a trait that is derived from
 * @c true_type if the expression <tt>execution::set_value(std::declval<R>(),
 * std::declval<Vs>()...)</tt> is well formed; otherwise @c false_type.
 */
template <typename R, typename... Vs>
struct can_set_value :
  integral_constant<bool, automatically_determined>
{
};

} // namespace execution
} // namespace asio

#else // defined(GENERATING_DOCUMENTATION)

namespace asio_execution_set_value_fn {

using asio::decay;
using asio::declval;
using asio::enable_if;
using asio::traits::set_value_free;
using asio::traits::set_value_member;

void set_value();

enum overload_type
{
  call_member,
  call_free,
  ill_formed
};

template <typename R, typename Vs, typename = void, typename = void>
struct call_traits
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = ill_formed);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

template <typename R, typename Vs>
struct call_traits<R, Vs,
  typename enable_if<
    set_value_member<R, Vs>::is_valid
  >::type> :
  set_value_member<R, Vs>
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = call_member);
};

template <typename R, typename Vs>
struct call_traits<R, Vs,
  typename enable_if<
    !set_value_member<R, Vs>::is_valid
  >::type,
  typename enable_if<
    set_value_free<R, Vs>::is_valid
  >::type> :
  set_value_free<R, Vs>
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = call_free);
};

struct impl
{
#if defined(ASIO_HAS_MOVE)

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename R, typename... Vs>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R, void(Vs...)>::overload == call_member,
    typename call_traits<R, void(Vs...)>::result_type
  >::type
  operator()(R&& r, Vs&&... v) const
    ASIO_NOEXCEPT_IF((
      call_traits<R, void(Vs...)>::is_noexcept))
  {
    return ASIO_MOVE_CAST(R)(r).set_value(ASIO_MOVE_CAST(Vs)(v)...);
  }

  template <typename R, typename... Vs>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R, void(Vs...)>::overload == call_free,
    typename call_traits<R, void(Vs...)>::result_type
  >::type
  operator()(R&& r, Vs&&... v) const
    ASIO_NOEXCEPT_IF((
      call_traits<R, void(Vs...)>::is_noexcept))
  {
    return set_value(ASIO_MOVE_CAST(R)(r),
        ASIO_MOVE_CAST(Vs)(v)...);
  }

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename R>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R, void()>::overload == call_member,
    typename call_traits<R, void()>::result_type
  >::type
  operator()(R&& r) const
    ASIO_NOEXCEPT_IF((
      call_traits<R, void()>::is_noexcept))
  {
    return ASIO_MOVE_CAST(R)(r).set_value();
  }

  template <typename R>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R, void()>::overload == call_free,
    typename call_traits<R, void()>::result_type
  >::type
  operator()(R&& r) const
    ASIO_NOEXCEPT_IF((
      call_traits<R, void()>::is_noexcept))
  {
    return set_value(ASIO_MOVE_CAST(R)(r));
  }

#define ASIO_PRIVATE_SET_VALUE_CALL_DEF(n) \
  template <typename R, ASIO_VARIADIC_TPARAMS(n)> \
  ASIO_CONSTEXPR typename enable_if< \
    call_traits<R, \
      void(ASIO_VARIADIC_TARGS(n))>::overload == call_member, \
    typename call_traits<R, void(ASIO_VARIADIC_TARGS(n))>::result_type \
  >::type \
  operator()(R&& r, ASIO_VARIADIC_MOVE_PARAMS(n)) const \
    ASIO_NOEXCEPT_IF(( \
      call_traits<R, void(ASIO_VARIADIC_TARGS(n))>::is_noexcept)) \
  { \
    return ASIO_MOVE_CAST(R)(r).set_value( \
        ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  \
  template <typename R, ASIO_VARIADIC_TPARAMS(n)> \
  ASIO_CONSTEXPR typename enable_if< \
    call_traits<R, void(ASIO_VARIADIC_TARGS(n))>::overload == call_free, \
    typename call_traits<R, void(ASIO_VARIADIC_TARGS(n))>::result_type \
  >::type \
  operator()(R&& r, ASIO_VARIADIC_MOVE_PARAMS(n)) const \
    ASIO_NOEXCEPT_IF(( \
      call_traits<R, void(ASIO_VARIADIC_TARGS(n))>::is_noexcept)) \
  { \
    return set_value(ASIO_MOVE_CAST(R)(r), \
        ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  /**/
ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_SET_VALUE_CALL_DEF)
#undef ASIO_PRIVATE_SET_VALUE_CALL_DEF

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

#else // defined(ASIO_HAS_MOVE)

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename R, typename... Vs>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R&, void(const Vs&...)>::overload == call_member,
    typename call_traits<R&, void(const Vs&...)>::result_type
  >::type
  operator()(R& r, const Vs&... v) const
    ASIO_NOEXCEPT_IF((
      call_traits<R&, void(const Vs&...)>::is_noexcept))
  {
    return r.set_value(v...);
  }

  template <typename R, typename... Vs>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<const R&, void(const Vs&...)>::overload == call_member,
    typename call_traits<const R&, void(const Vs&...)>::result_type
  >::type
  operator()(const R& r, const Vs&... v) const
    ASIO_NOEXCEPT_IF((
      call_traits<const R&, void(const Vs&...)>::is_noexcept))
  {
    return r.set_value(v...);
  }

  template <typename R, typename... Vs>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R&, void(const Vs&...)>::overload == call_free,
    typename call_traits<R&, void(const Vs&...)>::result_type
  >::type
  operator()(R& r, const Vs&... v) const
    ASIO_NOEXCEPT_IF((
      call_traits<R&, void(const Vs&...)>::is_noexcept))
  {
    return set_value(r, v...);
  }

  template <typename R, typename... Vs>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<const R&, void(const Vs&...)>::overload == call_free,
    typename call_traits<const R&, void(const Vs&...)>::result_type
  >::type
  operator()(const R& r, const Vs&... v) const
    ASIO_NOEXCEPT_IF((
      call_traits<const R&, void(const Vs&...)>::is_noexcept))
  {
    return set_value(r, v...);
  }

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename R>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R&, void()>::overload == call_member,
    typename call_traits<R&, void()>::result_type
  >::type
  operator()(R& r) const
    ASIO_NOEXCEPT_IF((
      call_traits<R&, void()>::is_noexcept))
  {
    return r.set_value();
  }

  template <typename R>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<const R&, void()>::overload == call_member,
    typename call_traits<const R&, void()>::result_type
  >::type
  operator()(const R& r) const
    ASIO_NOEXCEPT_IF((
      call_traits<const R&, void()>::is_noexcept))
  {
    return r.set_value();
  }

  template <typename R>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R&, void()>::overload == call_free,
    typename call_traits<R&, void()>::result_type
  >::type
  operator()(R& r) const
    ASIO_NOEXCEPT_IF((
      call_traits<R&, void()>::is_noexcept))
  {
    return set_value(r);
  }

  template <typename R>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<const R&, void()>::overload == call_free,
    typename call_traits<const R&, void()>::result_type
  >::type
  operator()(const R& r) const
    ASIO_NOEXCEPT_IF((
      call_traits<const R&, void()>::is_noexcept))
  {
    return set_value(r);
  }

#define ASIO_PRIVATE_SET_VALUE_CALL_DEF(n) \
  template <typename R, ASIO_VARIADIC_TPARAMS(n)> \
  ASIO_CONSTEXPR typename enable_if< \
    call_traits<R&, \
      void(ASIO_VARIADIC_TARGS(n))>::overload == call_member, \
    typename call_traits<R&, void(ASIO_VARIADIC_TARGS(n))>::result_type \
  >::type \
  operator()(R& r, ASIO_VARIADIC_MOVE_PARAMS(n)) const \
    ASIO_NOEXCEPT_IF(( \
      call_traits<R&, void(ASIO_VARIADIC_TARGS(n))>::is_noexcept)) \
  { \
    return r.set_value(ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  \
  template <typename R, ASIO_VARIADIC_TPARAMS(n)> \
  ASIO_CONSTEXPR typename enable_if< \
    call_traits<const R&, \
      void(ASIO_VARIADIC_TARGS(n))>::overload == call_member, \
    typename call_traits<const R&, \
      void(ASIO_VARIADIC_TARGS(n))>::result_type \
  >::type \
  operator()(const R& r, ASIO_VARIADIC_MOVE_PARAMS(n)) const \
    ASIO_NOEXCEPT_IF(( \
      call_traits<const R&, void(ASIO_VARIADIC_TARGS(n))>::is_noexcept)) \
  { \
    return r.set_value(ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  \
  template <typename R, ASIO_VARIADIC_TPARAMS(n)> \
  ASIO_CONSTEXPR typename enable_if< \
    call_traits<R&, \
      void(ASIO_VARIADIC_TARGS(n))>::overload == call_free, \
    typename call_traits<R&, void(ASIO_VARIADIC_TARGS(n))>::result_type \
  >::type \
  operator()(R& r, ASIO_VARIADIC_MOVE_PARAMS(n)) const \
    ASIO_NOEXCEPT_IF(( \
      call_traits<R&, void(ASIO_VARIADIC_TARGS(n))>::is_noexcept)) \
  { \
    return set_value(r, ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  \
  template <typename R, ASIO_VARIADIC_TPARAMS(n)> \
  ASIO_CONSTEXPR typename enable_if< \
    call_traits<const R&, \
      void(ASIO_VARIADIC_TARGS(n))>::overload == call_free, \
    typename call_traits<const R&, \
      void(ASIO_VARIADIC_TARGS(n))>::result_type \
  >::type \
  operator()(const R& r, ASIO_VARIADIC_MOVE_PARAMS(n)) const \
    ASIO_NOEXCEPT_IF(( \
      call_traits<const R&, void(ASIO_VARIADIC_TARGS(n))>::is_noexcept)) \
  { \
    return set_value(r, ASIO_VARIADIC_MOVE_ARGS(n)); \
  } \
  /**/
ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_SET_VALUE_CALL_DEF)
#undef ASIO_PRIVATE_SET_VALUE_CALL_DEF

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

#endif // defined(ASIO_HAS_MOVE)
};

template <typename T = impl>
struct static_instance
{
  static const T instance;
};

template <typename T>
const T static_instance<T>::instance = {};

} // namespace asio_execution_set_value_fn
namespace asio {
namespace execution {
namespace {

static ASIO_CONSTEXPR const asio_execution_set_value_fn::impl&
  set_value = asio_execution_set_value_fn::static_instance<>::instance;

} // namespace

#if defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename R, typename... Vs>
struct can_set_value :
  integral_constant<bool,
    asio_execution_set_value_fn::call_traits<R, void(Vs...)>::overload !=
      asio_execution_set_value_fn::ill_formed>
{
};

#if defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename R, typename... Vs>
constexpr bool can_set_value_v = can_set_value<R, Vs...>::value;

#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename R, typename... Vs>
struct is_nothrow_set_value :
  integral_constant<bool,
    asio_execution_set_value_fn::call_traits<R, void(Vs...)>::is_noexcept>
{
};

#if defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename R, typename... Vs>
constexpr bool is_nothrow_set_value_v
  = is_nothrow_set_value<R, Vs...>::value;

#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)

template <typename R, typename = void,
    typename = void, typename = void, typename = void, typename = void,
    typename = void, typename = void, typename = void, typename = void>
struct can_set_value;

template <typename R, typename = void,
    typename = void, typename = void, typename = void, typename = void,
    typename = void, typename = void, typename = void, typename = void>
struct is_nothrow_set_value;

template <typename R>
struct can_set_value<R> :
  integral_constant<bool,
    asio_execution_set_value_fn::call_traits<R, void()>::overload !=
      asio_execution_set_value_fn::ill_formed>
{
};

template <typename R>
struct is_nothrow_set_value<R> :
  integral_constant<bool,
    asio_execution_set_value_fn::call_traits<R, void()>::is_noexcept>
{
};

#define ASIO_PRIVATE_SET_VALUE_TRAITS_DEF(n) \
  template <typename R, ASIO_VARIADIC_TPARAMS(n)> \
  struct can_set_value<R, ASIO_VARIADIC_TARGS(n)> : \
    integral_constant<bool, \
      asio_execution_set_value_fn::call_traits<R, \
        void(ASIO_VARIADIC_TARGS(n))>::overload != \
          asio_execution_set_value_fn::ill_formed> \
  { \
  }; \
  \
  template <typename R, ASIO_VARIADIC_TPARAMS(n)> \
  struct is_nothrow_set_value<R, ASIO_VARIADIC_TARGS(n)> : \
    integral_constant<bool, \
      asio_execution_set_value_fn::call_traits<R, \
        void(ASIO_VARIADIC_TARGS(n))>::is_noexcept> \
  { \
  }; \
  /**/
ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_SET_VALUE_TRAITS_DEF)
#undef ASIO_PRIVATE_SET_VALUE_TRAITS_DEF

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)

} // namespace execution
} // namespace asio

#endif // defined(GENERATING_DOCUMENTATION)

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXECUTION_SET_VALUE_HPP
