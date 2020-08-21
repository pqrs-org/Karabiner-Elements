//
// execution/start.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_START_HPP
#define ASIO_EXECUTION_START_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/traits/start_member.hpp"
#include "asio/traits/start_free.hpp"

#include "asio/detail/push_options.hpp"

#if defined(GENERATING_DOCUMENTATION)

namespace asio {
namespace execution {

/// A customisation point that notifies an operation state object to start
/// its associated operation.
/**
 * The name <tt>execution::start</tt> denotes a customisation point object.
 * The expression <tt>execution::start(R)</tt> for some subexpression
 * <tt>R</tt> is expression-equivalent to:
 *
 * @li <tt>R.start()</tt>, if that expression is valid.
 *
 * @li Otherwise, <tt>start(R)</tt>, if that expression is valid, with
 * overload resolution performed in a context that includes the declaration
 * <tt>void start();</tt> and that does not include a declaration of
 * <tt>execution::start</tt>.
 *
 * @li Otherwise, <tt>execution::start(R)</tt> is ill-formed.
 */
inline constexpr unspecified start = unspecified;

/// A type trait that determines whether a @c start expression is
/// well-formed.
/**
 * Class template @c can_start is a trait that is derived from
 * @c true_type if the expression <tt>execution::start(std::declval<R>(),
 * std::declval<E>())</tt> is well formed; otherwise @c false_type.
 */
template <typename R>
struct can_start :
  integral_constant<bool, automatically_determined>
{
};

} // namespace execution
} // namespace asio

#else // defined(GENERATING_DOCUMENTATION)

namespace asio_execution_start_fn {

using asio::decay;
using asio::declval;
using asio::enable_if;
using asio::traits::start_free;
using asio::traits::start_member;

void start();

enum overload_type
{
  call_member,
  call_free,
  ill_formed
};

template <typename R, typename = void>
struct call_traits
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = ill_formed);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

template <typename R>
struct call_traits<R,
  typename enable_if<
    (
      start_member<R>::is_valid
    )
  >::type> :
  start_member<R>
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = call_member);
};

template <typename R>
struct call_traits<R,
  typename enable_if<
    (
      !start_member<R>::is_valid
      &&
      start_free<R>::is_valid
    )
  >::type> :
  start_free<R>
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = call_free);
};

struct impl
{
#if defined(ASIO_HAS_MOVE)
  template <typename R>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R>::overload == call_member,
    typename call_traits<R>::result_type
  >::type
  operator()(R&& r) const
    ASIO_NOEXCEPT_IF((
      call_traits<R>::is_noexcept))
  {
    return ASIO_MOVE_CAST(R)(r).start();
  }

  template <typename R>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R>::overload == call_free,
    typename call_traits<R>::result_type
  >::type
  operator()(R&& r) const
    ASIO_NOEXCEPT_IF((
      call_traits<R>::is_noexcept))
  {
    return start(ASIO_MOVE_CAST(R)(r));
  }
#else // defined(ASIO_HAS_MOVE)
  template <typename R>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R&>::overload == call_member,
    typename call_traits<R&>::result_type
  >::type
  operator()(R& r) const
    ASIO_NOEXCEPT_IF((
      call_traits<R&>::is_noexcept))
  {
    return r.start();
  }

  template <typename R>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<const R&>::overload == call_member,
    typename call_traits<const R&>::result_type
  >::type
  operator()(const R& r) const
    ASIO_NOEXCEPT_IF((
      call_traits<const R&>::is_noexcept))
  {
    return r.start();
  }

  template <typename R>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R&>::overload == call_free,
    typename call_traits<R&>::result_type
  >::type
  operator()(R& r) const
    ASIO_NOEXCEPT_IF((
      call_traits<R&>::is_noexcept))
  {
    return start(r);
  }

  template <typename R>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<const R&>::overload == call_free,
    typename call_traits<const R&>::result_type
  >::type
  operator()(const R& r) const
    ASIO_NOEXCEPT_IF((
      call_traits<const R&>::is_noexcept))
  {
    return start(r);
  }
#endif // defined(ASIO_HAS_MOVE)
};

template <typename T = impl>
struct static_instance
{
  static const T instance;
};

template <typename T>
const T static_instance<T>::instance = {};

} // namespace asio_execution_start_fn
namespace asio {
namespace execution {
namespace {

static ASIO_CONSTEXPR const asio_execution_start_fn::impl&
  start = asio_execution_start_fn::static_instance<>::instance;

} // namespace

template <typename R>
struct can_start :
  integral_constant<bool,
    asio_execution_start_fn::call_traits<R>::overload !=
      asio_execution_start_fn::ill_formed>
{
};

#if defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename R>
constexpr bool can_start_v = can_start<R>::value;

#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename R>
struct is_nothrow_start :
  integral_constant<bool,
    asio_execution_start_fn::call_traits<R>::is_noexcept>
{
};

#if defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename R>
constexpr bool is_nothrow_start_v
  = is_nothrow_start<R>::value;

#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

} // namespace execution
} // namespace asio

#endif // defined(GENERATING_DOCUMENTATION)

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXECUTION_START_HPP
