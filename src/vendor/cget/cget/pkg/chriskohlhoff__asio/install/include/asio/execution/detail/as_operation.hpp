//
// execution/detail/as_operation.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_DETAIL_AS_OPERATION_HPP
#define ASIO_EXECUTION_DETAIL_AS_OPERATION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/execution/detail/as_invocable.hpp"
#include "asio/execution/execute.hpp"
#include "asio/execution/set_error.hpp"
#include "asio/traits/start_member.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace execution {
namespace detail {

template <typename Executor, typename Receiver>
struct as_operation
{
  typename remove_cvref<Executor>::type ex_;
  typename remove_cvref<Receiver>::type receiver_;
#if !defined(ASIO_HAS_MOVE)
  asio::detail::shared_ptr<asio::detail::atomic_count> ref_count_;
#endif // !defined(ASIO_HAS_MOVE)

  template <typename E, typename R>
  explicit as_operation(ASIO_MOVE_ARG(E) e, ASIO_MOVE_ARG(R) r)
    : ex_(ASIO_MOVE_CAST(E)(e)),
      receiver_(ASIO_MOVE_CAST(R)(r))
#if !defined(ASIO_HAS_MOVE)
      , ref_count_(new asio::detail::atomic_count(1))
#endif // !defined(ASIO_HAS_MOVE)
  {
  }

  void start() ASIO_NOEXCEPT
  {
#if !defined(ASIO_NO_EXCEPTIONS)
    try
    {
#endif // !defined(ASIO_NO_EXCEPTIONS)
      execution::execute(
          ASIO_MOVE_CAST(typename remove_cvref<Executor>::type)(ex_),
          as_invocable<typename remove_cvref<Receiver>::type,
              Executor>(receiver_
#if !defined(ASIO_HAS_MOVE)
                , ref_count_
#endif // !defined(ASIO_HAS_MOVE)
              ));
#if !defined(ASIO_NO_EXCEPTIONS)
    }
    catch (...)
    {
#if defined(ASIO_HAS_STD_EXCEPTION_PTR)
      execution::set_error(
          ASIO_MOVE_OR_LVALUE(
            typename remove_cvref<Receiver>::type)(
              receiver_),
          std::current_exception());
#else // defined(ASIO_HAS_STD_EXCEPTION_PTR)
      std::terminate();
#endif // defined(ASIO_HAS_STD_EXCEPTION_PTR)
    }
#endif // !defined(ASIO_NO_EXCEPTIONS)
  }
};

} // namespace detail
} // namespace execution
namespace traits {

#if !defined(ASIO_HAS_DEDUCED_START_MEMBER_TRAIT)

template <typename Executor, typename Receiver>
struct start_member<
    asio::execution::detail::as_operation<Executor, Receiver> >
{
  ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef void result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_START_MEMBER_TRAIT)

} // namespace traits
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXECUTION_DETAIL_AS_OPERATION_HPP
