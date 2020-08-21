//
// execution/set_error.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_SET_ERROR_HPP
#define ASIO_EXECUTION_SET_ERROR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/traits/set_error_member.hpp"
#include "asio/traits/set_error_free.hpp"

#include "asio/detail/push_options.hpp"

#if defined(GENERATING_DOCUMENTATION)

namespace asio {
namespace execution {

/// A customisation point that delivers an error notification to a receiver.
/**
 * The name <tt>execution::set_error</tt> denotes a customisation point object.
 * The expression <tt>execution::set_error(R, E)</tt> for some subexpressions
 * <tt>R</tt> and <tt>E</tt> are expression-equivalent to:
 *
 * @li <tt>R.set_error(E)</tt>, if that expression is valid. If the function
 *   selected does not send the error <tt>E</tt> to the receiver <tt>R</tt>'s
 *   error channel, the program is ill-formed with no diagnostic required.
 *
 * @li Otherwise, <tt>set_error(R, E)</tt>, if that expression is valid, with
 *   overload resolution performed in a context that includes the declaration
 *   <tt>void set_error();</tt> and that does not include a declaration of
 *   <tt>execution::set_error</tt>. If the function selected by overload
 *   resolution does not send the error <tt>E</tt> to the receiver <tt>R</tt>'s
 *   error channel, the program is ill-formed with no diagnostic required.
 *
 * @li Otherwise, <tt>execution::set_error(R, E)</tt> is ill-formed.
 */
inline constexpr unspecified set_error = unspecified;

/// A type trait that determines whether a @c set_error expression is
/// well-formed.
/**
 * Class template @c can_set_error is a trait that is derived from
 * @c true_type if the expression <tt>execution::set_error(std::declval<R>(),
 * std::declval<E>())</tt> is well formed; otherwise @c false_type.
 */
template <typename R, typename E>
struct can_set_error :
  integral_constant<bool, automatically_determined>
{
};

} // namespace execution
} // namespace asio

#else // defined(GENERATING_DOCUMENTATION)

namespace asio_execution_set_error_fn {

using asio::decay;
using asio::declval;
using asio::enable_if;
using asio::traits::set_error_free;
using asio::traits::set_error_member;

void set_error();

enum overload_type
{
  call_member,
  call_free,
  ill_formed
};

template <typename R, typename E, typename = void>
struct call_traits
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = ill_formed);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

template <typename R, typename E>
struct call_traits<R, void(E),
  typename enable_if<
    (
      set_error_member<R, E>::is_valid
    )
  >::type> :
  set_error_member<R, E>
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = call_member);
};

template <typename R, typename E>
struct call_traits<R, void(E),
  typename enable_if<
    (
      !set_error_member<R, E>::is_valid
      &&
      set_error_free<R, E>::is_valid
    )
  >::type> :
  set_error_free<R, E>
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = call_free);
};

struct impl
{
#if defined(ASIO_HAS_MOVE)
  template <typename R, typename E>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R, void(E)>::overload == call_member,
    typename call_traits<R, void(E)>::result_type
  >::type
  operator()(R&& r, E&& e) const
    ASIO_NOEXCEPT_IF((
      call_traits<R, void(E)>::is_noexcept))
  {
    return ASIO_MOVE_CAST(R)(r).set_error(ASIO_MOVE_CAST(E)(e));
  }

  template <typename R, typename E>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R, void(E)>::overload == call_free,
    typename call_traits<R, void(E)>::result_type
  >::type
  operator()(R&& r, E&& e) const
    ASIO_NOEXCEPT_IF((
      call_traits<R, void(E)>::is_noexcept))
  {
    return set_error(ASIO_MOVE_CAST(R)(r), ASIO_MOVE_CAST(E)(e));
  }
#else // defined(ASIO_HAS_MOVE)
  template <typename R, typename E>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R&, void(const E&)>::overload == call_member,
    typename call_traits<R&, void(const E&)>::result_type
  >::type
  operator()(R& r, const E& e) const
    ASIO_NOEXCEPT_IF((
      call_traits<R&, void(const E&)>::is_noexcept))
  {
    return r.set_error(e);
  }

  template <typename R, typename E>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<const R&, void(const E&)>::overload == call_member,
    typename call_traits<const R&, void(const E&)>::result_type
  >::type
  operator()(const R& r, const E& e) const
    ASIO_NOEXCEPT_IF((
      call_traits<const R&, void(const E&)>::is_noexcept))
  {
    return r.set_error(e);
  }

  template <typename R, typename E>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<R&, void(const E&)>::overload == call_free,
    typename call_traits<R&, void(const E&)>::result_type
  >::type
  operator()(R& r, const E& e) const
    ASIO_NOEXCEPT_IF((
      call_traits<R&, void(const E&)>::is_noexcept))
  {
    return set_error(r, e);
  }

  template <typename R, typename E>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<const R&, void(const E&)>::overload == call_free,
    typename call_traits<const R&, void(const E&)>::result_type
  >::type
  operator()(const R& r, const E& e) const
    ASIO_NOEXCEPT_IF((
      call_traits<const R&, void(const E&)>::is_noexcept))
  {
    return set_error(r, e);
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

} // namespace asio_execution_set_error_fn
namespace asio {
namespace execution {
namespace {

static ASIO_CONSTEXPR const asio_execution_set_error_fn::impl&
  set_error = asio_execution_set_error_fn::static_instance<>::instance;

} // namespace

template <typename R, typename E>
struct can_set_error :
  integral_constant<bool,
    asio_execution_set_error_fn::call_traits<R, void(E)>::overload !=
      asio_execution_set_error_fn::ill_formed>
{
};

#if defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename R, typename E>
constexpr bool can_set_error_v = can_set_error<R, E>::value;

#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename R, typename E>
struct is_nothrow_set_error :
  integral_constant<bool,
    asio_execution_set_error_fn::call_traits<R, void(E)>::is_noexcept>
{
};

#if defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename R, typename E>
constexpr bool is_nothrow_set_error_v
  = is_nothrow_set_error<R, E>::value;

#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

} // namespace execution
} // namespace asio

#endif // defined(GENERATING_DOCUMENTATION)

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXECUTION_SET_ERROR_HPP
