//
// execution/schedule.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_SCHEDULE_HPP
#define ASIO_EXECUTION_SCHEDULE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/execution/executor.hpp"
#include "asio/traits/schedule_member.hpp"
#include "asio/traits/schedule_free.hpp"

#include "asio/detail/push_options.hpp"

#if defined(GENERATING_DOCUMENTATION)

namespace asio {
namespace execution {

/// A customisation point that is used to obtain a sender from a scheduler.
/**
 * The name <tt>execution::schedule</tt> denotes a customisation point object.
 * For some subexpression <tt>s</tt>, let <tt>S</tt> be a type such that
 * <tt>decltype((s))</tt> is <tt>S</tt>. The expression
 * <tt>execution::schedule(s)</tt> is expression-equivalent to:
 *
 * @li <tt>s.schedule()</tt>, if that expression is valid and its type models
 *   <tt>sender</tt>.
 *
 * @li Otherwise, <tt>schedule(s)</tt>, if that expression is valid and its
 *   type models <tt>sender</tt> with overload resolution performed in a context
 *   that includes the declaration <tt>void schedule();</tt> and that does not
 *   include a declaration of <tt>execution::schedule</tt>.
 *
 * @li Otherwise, <tt>S</tt> if <tt>S</tt> satisfies <tt>executor</tt>.
 *
 * @li Otherwise, <tt>execution::schedule(s)</tt> is ill-formed.
 */
inline constexpr unspecified schedule = unspecified;

/// A type trait that determines whether a @c schedule expression is
/// well-formed.
/**
 * Class template @c can_schedule is a trait that is derived from @c true_type
 * if the expression <tt>execution::schedule(std::declval<S>())</tt> is well
 * formed; otherwise @c false_type.
 */
template <typename S>
struct can_schedule :
  integral_constant<bool, automatically_determined>
{
};

} // namespace execution
} // namespace asio

#else // defined(GENERATING_DOCUMENTATION)

namespace asio_execution_schedule_fn {

using asio::decay;
using asio::declval;
using asio::enable_if;
using asio::execution::is_executor;
using asio::traits::schedule_free;
using asio::traits::schedule_member;

void schedule();

enum overload_type
{
  identity,
  call_member,
  call_free,
  ill_formed
};

template <typename S, typename = void>
struct call_traits
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = ill_formed);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

template <typename S>
struct call_traits<S,
  typename enable_if<
    (
      schedule_member<S>::is_valid
    )
  >::type> :
  schedule_member<S>
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = call_member);
};

template <typename S>
struct call_traits<S,
  typename enable_if<
    (
      !schedule_member<S>::is_valid
      &&
      schedule_free<S>::is_valid
    )
  >::type> :
  schedule_free<S>
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = call_free);
};

template <typename S>
struct call_traits<S,
  typename enable_if<
    (
      !schedule_member<S>::is_valid
      &&
      !schedule_free<S>::is_valid
      &&
      is_executor<typename decay<S>::type>::value
    )
  >::type>
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = identity);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);

#if defined(ASIO_HAS_MOVE)
  typedef ASIO_MOVE_ARG(S) result_type;
#else // defined(ASIO_HAS_MOVE)
  typedef ASIO_MOVE_ARG(typename decay<S>::type) result_type;
#endif // defined(ASIO_HAS_MOVE)
};

struct impl
{
  template <typename S>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<S>::overload == identity,
    typename call_traits<S>::result_type
  >::type
  operator()(ASIO_MOVE_ARG(S) s) const
    ASIO_NOEXCEPT_IF((
      call_traits<S>::is_noexcept))
  {
    return ASIO_MOVE_CAST(S)(s);
  }

#if defined(ASIO_HAS_MOVE)
  template <typename S>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<S>::overload == call_member,
    typename call_traits<S>::result_type
  >::type
  operator()(S&& s) const
    ASIO_NOEXCEPT_IF((
      call_traits<S>::is_noexcept))
  {
    return ASIO_MOVE_CAST(S)(s).schedule();
  }

  template <typename S>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<S>::overload == call_free,
    typename call_traits<S>::result_type
  >::type
  operator()(S&& s) const
    ASIO_NOEXCEPT_IF((
      call_traits<S>::is_noexcept))
  {
    return schedule(ASIO_MOVE_CAST(S)(s));
  }
#else // defined(ASIO_HAS_MOVE)
  template <typename S>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<S&>::overload == call_member,
    typename call_traits<S&>::result_type
  >::type
  operator()(S& s) const
    ASIO_NOEXCEPT_IF((
      call_traits<S&>::is_noexcept))
  {
    return s.schedule();
  }

  template <typename S>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<const S&>::overload == call_member,
    typename call_traits<const S&>::result_type
  >::type
  operator()(const S& s) const
    ASIO_NOEXCEPT_IF((
      call_traits<const S&>::is_noexcept))
  {
    return s.schedule();
  }

  template <typename S>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<S&>::overload == call_free,
    typename call_traits<S&>::result_type
  >::type
  operator()(S& s) const
    ASIO_NOEXCEPT_IF((
      call_traits<S&>::is_noexcept))
  {
    return schedule(s);
  }

  template <typename S>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<const S&>::overload == call_free,
    typename call_traits<const S&>::result_type
  >::type
  operator()(const S& s) const
    ASIO_NOEXCEPT_IF((
      call_traits<const S&>::is_noexcept))
  {
    return schedule(s);
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

} // namespace asio_execution_schedule_fn
namespace asio {
namespace execution {
namespace {

static ASIO_CONSTEXPR const asio_execution_schedule_fn::impl&
  schedule = asio_execution_schedule_fn::static_instance<>::instance;

} // namespace

template <typename S>
struct can_schedule :
  integral_constant<bool,
    asio_execution_schedule_fn::call_traits<S>::overload !=
      asio_execution_schedule_fn::ill_formed>
{
};

#if defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename S>
constexpr bool can_schedule_v = can_schedule<S>::value;

#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename S>
struct is_nothrow_schedule :
  integral_constant<bool,
    asio_execution_schedule_fn::call_traits<S>::is_noexcept>
{
};

#if defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename S>
constexpr bool is_nothrow_schedule_v
  = is_nothrow_schedule<S>::value;

#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

} // namespace execution
} // namespace asio

#endif // defined(GENERATING_DOCUMENTATION)

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXECUTION_SCHEDULE_HPP
