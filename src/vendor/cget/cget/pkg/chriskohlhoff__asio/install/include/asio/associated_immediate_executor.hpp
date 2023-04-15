//
// associated_immediate_executor.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_ASSOCIATED_IMMEDIATE_EXECUTOR_HPP
#define ASIO_ASSOCIATED_IMMEDIATE_EXECUTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/associator.hpp"
#include "asio/detail/functional.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/execution/blocking.hpp"
#include "asio/execution/executor.hpp"
#include "asio/execution_context.hpp"
#include "asio/is_executor.hpp"
#include "asio/require.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

template <typename T, typename Executor>
struct associated_immediate_executor;

namespace detail {

template <typename T, typename = void>
struct has_immediate_executor_type : false_type
{
};

template <typename T>
struct has_immediate_executor_type<T,
  typename void_type<typename T::immediate_executor_type>::type>
    : true_type
{
};

template <typename E, typename = void, typename = void>
struct default_immediate_executor
{
  typedef typename require_result<E, execution::blocking_t::never_t>::type type;

  static type get(const E& e) ASIO_NOEXCEPT
  {
    return asio::require(e, execution::blocking.never);
  }
};

template <typename E>
struct default_immediate_executor<E,
  typename enable_if<
    !execution::is_executor<E>::value
  >::type,
  typename enable_if<
    is_executor<E>::value
  >::type>
{
  class type : public E
  {
  public:
    template <typename Executor1>
    explicit type(const Executor1& e,
        typename constraint<
          conditional<
            !is_same<Executor1, type>::value,
            is_convertible<Executor1, E>,
            false_type
          >::type::value
        >::type = 0) ASIO_NOEXCEPT
      : E(e)
    {
    }

    type(const type& other) ASIO_NOEXCEPT
      : E(static_cast<const E&>(other))
    {
    }

#if defined(ASIO_HAS_MOVE)
    type(type&& other) ASIO_NOEXCEPT
      : E(ASIO_MOVE_CAST(E)(other))
    {
    }
#endif // defined(ASIO_HAS_MOVE)

    template <typename Function, typename Allocator>
    void dispatch(ASIO_MOVE_ARG(Function) f, const Allocator& a) const
    {
      this->post(ASIO_MOVE_CAST(Function)(f), a);
    }

    friend bool operator==(const type& a, const type& b) ASIO_NOEXCEPT
    {
      return static_cast<const E&>(a) == static_cast<const E&>(b);
    }

    friend bool operator!=(const type& a, const type& b) ASIO_NOEXCEPT
    {
      return static_cast<const E&>(a) != static_cast<const E&>(b);
    }
  };

  static type get(const E& e) ASIO_NOEXCEPT
  {
    return type(e);
  }
};

template <typename T, typename E, typename = void, typename = void>
struct associated_immediate_executor_impl
{
  typedef void asio_associated_immediate_executor_is_unspecialised;

  typedef typename default_immediate_executor<E>::type type;

  static ASIO_AUTO_RETURN_TYPE_PREFIX(type) get(
      const T&, const E& e) ASIO_NOEXCEPT
    ASIO_AUTO_RETURN_TYPE_SUFFIX((default_immediate_executor<E>::get(e)))
  {
    return default_immediate_executor<E>::get(e);
  }
};

template <typename T, typename E>
struct associated_immediate_executor_impl<T, E,
  typename void_type<typename T::immediate_executor_type>::type>
{
  typedef typename T::immediate_executor_type type;

  static ASIO_AUTO_RETURN_TYPE_PREFIX(type) get(
      const T& t, const E&) ASIO_NOEXCEPT
    ASIO_AUTO_RETURN_TYPE_SUFFIX((t.get_immediate_executor()))
  {
    return t.get_immediate_executor();
  }
};

template <typename T, typename E>
struct associated_immediate_executor_impl<T, E,
  typename enable_if<
    !has_immediate_executor_type<T>::value
  >::type,
  typename void_type<
    typename associator<associated_immediate_executor, T, E>::type
  >::type> : associator<associated_immediate_executor, T, E>
{
};

} // namespace detail

/// Traits type used to obtain the immediate executor associated with an object.
/**
 * A program may specialise this traits type if the @c T template parameter in
 * the specialisation is a user-defined type. The template parameter @c
 * Executor shall be a type meeting the Executor requirements.
 *
 * Specialisations shall meet the following requirements, where @c t is a const
 * reference to an object of type @c T, and @c e is an object of type @c
 * Executor.
 *
 * @li Provide a nested typedef @c type that identifies a type meeting the
 * Executor requirements.
 *
 * @li Provide a noexcept static member function named @c get, callable as @c
 * get(t) and with return type @c type or a (possibly const) reference to @c
 * type.
 *
 * @li Provide a noexcept static member function named @c get, callable as @c
 * get(t,e) and with return type @c type or a (possibly const) reference to @c
 * type.
 */
template <typename T, typename Executor>
struct associated_immediate_executor
#if !defined(GENERATING_DOCUMENTATION)
  : detail::associated_immediate_executor_impl<T, Executor>
#endif // !defined(GENERATING_DOCUMENTATION)
{
#if defined(GENERATING_DOCUMENTATION)
  /// If @c T has a nested type @c immediate_executor_type,
  // <tt>T::immediate_executor_type</tt>. Otherwise @c Executor.
  typedef see_below type;

  /// If @c T has a nested type @c immediate_executor_type, returns
  /// <tt>t.get_immediate_executor()</tt>. Otherwise returns
  /// <tt>asio::require(ex, asio::execution::blocking.never)</tt>.
  static decltype(auto) get(const T& t, const Executor& ex) ASIO_NOEXCEPT;
#endif // defined(GENERATING_DOCUMENTATION)
};

/// Helper function to obtain an object's associated executor.
/**
 * @returns <tt>associated_immediate_executor<T, Executor>::get(t, ex)</tt>
 */
template <typename T, typename Executor>
ASIO_NODISCARD inline ASIO_AUTO_RETURN_TYPE_PREFIX2(
    typename associated_immediate_executor<T, Executor>::type)
get_associated_immediate_executor(const T& t, const Executor& ex,
    typename constraint<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    >::type = 0) ASIO_NOEXCEPT
  ASIO_AUTO_RETURN_TYPE_SUFFIX((
    associated_immediate_executor<T, Executor>::get(t, ex)))
{
  return associated_immediate_executor<T, Executor>::get(t, ex);
}

/// Helper function to obtain an object's associated executor.
/**
 * @returns <tt>associated_immediate_executor<T, typename
 * ExecutionContext::executor_type>::get(t, ctx.get_executor())</tt>
 */
template <typename T, typename ExecutionContext>
ASIO_NODISCARD inline typename associated_immediate_executor<T,
    typename ExecutionContext::executor_type>::type
get_associated_immediate_executor(const T& t, ExecutionContext& ctx,
    typename constraint<is_convertible<ExecutionContext&,
      execution_context&>::value>::type = 0) ASIO_NOEXCEPT
{
  return associated_immediate_executor<T,
    typename ExecutionContext::executor_type>::get(t, ctx.get_executor());
}

#if defined(ASIO_HAS_ALIAS_TEMPLATES)

template <typename T, typename Executor>
using associated_immediate_executor_t =
  typename associated_immediate_executor<T, Executor>::type;

#endif // defined(ASIO_HAS_ALIAS_TEMPLATES)

namespace detail {

template <typename T, typename E, typename = void>
struct associated_immediate_executor_forwarding_base
{
};

template <typename T, typename E>
struct associated_immediate_executor_forwarding_base<T, E,
    typename enable_if<
      is_same<
        typename associated_immediate_executor<T,
          E>::asio_associated_immediate_executor_is_unspecialised,
        void
      >::value
    >::type>
{
  typedef void asio_associated_immediate_executor_is_unspecialised;
};

} // namespace detail

#if defined(ASIO_HAS_STD_REFERENCE_WRAPPER) \
  || defined(GENERATING_DOCUMENTATION)

/// Specialisation of associated_immediate_executor for
/// @c std::reference_wrapper.
template <typename T, typename Executor>
struct associated_immediate_executor<reference_wrapper<T>, Executor>
#if !defined(GENERATING_DOCUMENTATION)
  : detail::associated_immediate_executor_forwarding_base<T, Executor>
#endif // !defined(GENERATING_DOCUMENTATION)
{
  /// Forwards @c type to the associator specialisation for the unwrapped type
  /// @c T.
  typedef typename associated_immediate_executor<T, Executor>::type type;

  /// Forwards the request to get the executor to the associator specialisation
  /// for the unwrapped type @c T.
  static ASIO_AUTO_RETURN_TYPE_PREFIX(type) get(
      reference_wrapper<T> t, const Executor& ex) ASIO_NOEXCEPT
    ASIO_AUTO_RETURN_TYPE_SUFFIX((
      associated_immediate_executor<T, Executor>::get(t.get(), ex)))
  {
    return associated_immediate_executor<T, Executor>::get(t.get(), ex);
  }
};

#endif // defined(ASIO_HAS_STD_REFERENCE_WRAPPER)
       //   || defined(GENERATING_DOCUMENTATION)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_ASSOCIATED_IMMEDIATE_EXECUTOR_HPP
