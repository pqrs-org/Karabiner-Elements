//
// execution/execute.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_EXECUTE_HPP
#define ASIO_EXECUTION_EXECUTE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/execution/detail/as_invocable.hpp"
#include "asio/execution/detail/as_receiver.hpp"
#include "asio/traits/execute_member.hpp"
#include "asio/traits/execute_free.hpp"

#include "asio/detail/push_options.hpp"

#if defined(GENERATING_DOCUMENTATION)

namespace asio {
namespace execution {

/// A customisation point that executes a function on an executor.
/**
 * The name <tt>execution::execute</tt> denotes a customisation point object.
 *
 * For some subexpressions <tt>e</tt> and <tt>f</tt>, let <tt>E</tt> be a type
 * such that <tt>decltype((e))</tt> is <tt>E</tt> and let <tt>F</tt> be a type
 * such that <tt>decltype((f))</tt> is <tt>F</tt>. The expression
 * <tt>execution::execute(e, f)</tt> is ill-formed if <tt>F</tt> does not model
 * <tt>invocable</tt>, or if <tt>E</tt> does not model either <tt>executor</tt>
 * or <tt>sender</tt>. Otherwise, it is expression-equivalent to:
 *
 * @li <tt>e.execute(f)</tt>, if that expression is valid. If the function
 *   selected does not execute the function object <tt>f</tt> on the executor
 *   <tt>e</tt>, the program is ill-formed with no diagnostic required.
 *
 * @li Otherwise, <tt>execute(e, f)</tt>, if that expression is valid, with
 *   overload resolution performed in a context that includes the declaration
 *   <tt>void execute();</tt> and that does not include a declaration of
 *   <tt>execution::execute</tt>. If the function selected by overload
 *   resolution does not execute the function object <tt>f</tt> on the executor
 *   <tt>e</tt>, the program is ill-formed with no diagnostic required.
 */
inline constexpr unspecified execute = unspecified;

/// A type trait that determines whether a @c execute expression is well-formed.
/**
 * Class template @c can_execute is a trait that is derived from
 * @c true_type if the expression <tt>execution::execute(std::declval<T>(),
 * std::declval<F>())</tt> is well formed; otherwise @c false_type.
 */
template <typename T, typename F>
struct can_execute :
  integral_constant<bool, automatically_determined>
{
};

} // namespace execution
} // namespace asio

#else // defined(GENERATING_DOCUMENTATION)

namespace asio {
namespace execution {

template <typename T, typename R>
struct is_sender_to;

namespace detail {

template <typename S, typename R>
void submit_helper(ASIO_MOVE_ARG(S) s, ASIO_MOVE_ARG(R) r);

} // namespace detail
} // namespace execution
} // namespace asio
namespace asio_execution_execute_fn {

using asio::conditional;
using asio::decay;
using asio::declval;
using asio::enable_if;
using asio::execution::detail::as_receiver;
using asio::execution::detail::is_as_invocable;
using asio::execution::is_sender_to;
using asio::false_type;
using asio::result_of;
using asio::traits::execute_free;
using asio::traits::execute_member;
using asio::true_type;
using asio::void_type;

void execute();

enum overload_type
{
  call_member,
  call_free,
  adapter,
  ill_formed
};

template <typename Impl, typename T, typename F, typename = void,
    typename = void, typename = void, typename = void, typename = void>
struct call_traits
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = ill_formed);
};

template <typename Impl, typename T, typename F>
struct call_traits<Impl, T, void(F),
  typename enable_if<
    execute_member<typename Impl::template proxy<T>::type, F>::is_valid
  >::type> :
  execute_member<typename Impl::template proxy<T>::type, F>
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = call_member);
};

template <typename Impl, typename T, typename F>
struct call_traits<Impl, T, void(F),
  typename enable_if<
    !execute_member<typename Impl::template proxy<T>, F>::is_valid
  >::type,
  typename enable_if<
    execute_free<T, F>::is_valid
  >::type> :
  execute_free<T, F>
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = call_free);
};

template <typename Impl, typename T, typename F>
struct call_traits<Impl, T, void(F),
  typename enable_if<
    !execute_member<typename Impl::template proxy<T>::type, F>::is_valid
  >::type,
  typename enable_if<
    !execute_free<T, F>::is_valid
  >::type,
  typename void_type<
   typename result_of<typename decay<F>::type&()>::type
  >::type,
  typename enable_if<
    !is_as_invocable<typename decay<F>::type>::value
  >::type,
  typename enable_if<
    is_sender_to<T, as_receiver<typename decay<F>::type, T> >::value
  >::type>
{
  ASIO_STATIC_CONSTEXPR(overload_type, overload = adapter);
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

struct impl
{
  template <typename T>
  struct proxy
  {
#if defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
    struct type
    {
      template <typename F>
      auto execute(ASIO_MOVE_ARG(F) f)
        noexcept(
          noexcept(
            declval<typename conditional<true, T, F>::type>().execute(
              ASIO_MOVE_CAST(F)(f))
          )
        )
        -> decltype(
          declval<typename conditional<true, T, F>::type>().execute(
            ASIO_MOVE_CAST(F)(f))
        );
    };
#else // defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
    typedef T type;
#endif // defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)
  };

  template <typename T, typename F>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<impl, T, void(F)>::overload == call_member,
    typename call_traits<impl, T, void(F)>::result_type
  >::type
  operator()(
      ASIO_MOVE_ARG(T) t,
      ASIO_MOVE_ARG(F) f) const
    ASIO_NOEXCEPT_IF((
      call_traits<impl, T, void(F)>::is_noexcept))
  {
    return ASIO_MOVE_CAST(T)(t).execute(ASIO_MOVE_CAST(F)(f));
  }

  template <typename T, typename F>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<impl, T, void(F)>::overload == call_free,
    typename call_traits<impl, T, void(F)>::result_type
  >::type
  operator()(
      ASIO_MOVE_ARG(T) t,
      ASIO_MOVE_ARG(F) f) const
    ASIO_NOEXCEPT_IF((
      call_traits<impl, T, void(F)>::is_noexcept))
  {
    return execute(ASIO_MOVE_CAST(T)(t), ASIO_MOVE_CAST(F)(f));
  }

  template <typename T, typename F>
  ASIO_CONSTEXPR typename enable_if<
    call_traits<impl, T, void(F)>::overload == adapter,
    typename call_traits<impl, T, void(F)>::result_type
  >::type
  operator()(
      ASIO_MOVE_ARG(T) t,
      ASIO_MOVE_ARG(F) f) const
    ASIO_NOEXCEPT_IF((
      call_traits<impl, T, void(F)>::is_noexcept))
  {
    return asio::execution::detail::submit_helper(
        ASIO_MOVE_CAST(T)(t),
        as_receiver<typename decay<F>::type, T>(
          ASIO_MOVE_CAST(F)(f), 0));
  }
};

template <typename T = impl>
struct static_instance
{
  static const T instance;
};

template <typename T>
const T static_instance<T>::instance = {};

} // namespace asio_execution_execute_fn
namespace asio {
namespace execution {
namespace {

static ASIO_CONSTEXPR const asio_execution_execute_fn::impl&
  execute = asio_execution_execute_fn::static_instance<>::instance;

} // namespace

typedef asio_execution_execute_fn::impl execute_t;

template <typename T, typename F>
struct can_execute :
  integral_constant<bool,
    asio_execution_execute_fn::call_traits<
      execute_t, T, void(F)>::overload !=
        asio_execution_execute_fn::ill_formed>
{
};

#if defined(ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T, typename F>
constexpr bool can_execute_v = can_execute<T, F>::value;

#endif // defined(ASIO_HAS_VARIABLE_TEMPLATES)

} // namespace execution
} // namespace asio

#endif // defined(GENERATING_DOCUMENTATION)

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXECUTION_EXECUTE_HPP
