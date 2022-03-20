//
// execution/detail/as_invocable.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_DETAIL_AS_INVOCABLE_HPP
#define ASIO_EXECUTION_DETAIL_AS_INVOCABLE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/atomic_count.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/execution/receiver_invocation_error.hpp"
#include "asio/execution/set_done.hpp"
#include "asio/execution/set_error.hpp"
#include "asio/execution/set_value.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace execution {
namespace detail {

#if defined(ASIO_HAS_MOVE)

template <typename Receiver, typename>
struct as_invocable
{
  Receiver* receiver_;

  explicit as_invocable(Receiver& r) ASIO_NOEXCEPT
    : receiver_(asio::detail::addressof(r))
  {
  }

  as_invocable(as_invocable&& other) ASIO_NOEXCEPT
    : receiver_(other.receiver_)
  {
    other.receiver_ = 0;
  }

  ~as_invocable()
  {
    if (receiver_)
      execution::set_done(ASIO_MOVE_OR_LVALUE(Receiver)(*receiver_));
  }

  void operator()() ASIO_LVALUE_REF_QUAL ASIO_NOEXCEPT
  {
#if !defined(ASIO_NO_EXCEPTIONS)
    try
    {
#endif // !defined(ASIO_NO_EXCEPTIONS)
      execution::set_value(ASIO_MOVE_CAST(Receiver)(*receiver_));
      receiver_ = 0;
#if !defined(ASIO_NO_EXCEPTIONS)
    }
    catch (...)
    {
#if defined(ASIO_HAS_STD_EXCEPTION_PTR)
      execution::set_error(ASIO_MOVE_CAST(Receiver)(*receiver_),
          std::make_exception_ptr(receiver_invocation_error()));
      receiver_ = 0;
#else // defined(ASIO_HAS_STD_EXCEPTION_PTR)
      std::terminate();
#endif // defined(ASIO_HAS_STD_EXCEPTION_PTR)
    }
#endif // !defined(ASIO_NO_EXCEPTIONS)
  }
};

#else // defined(ASIO_HAS_MOVE)

template <typename Receiver, typename>
struct as_invocable
{
  Receiver* receiver_;
  asio::detail::shared_ptr<asio::detail::atomic_count> ref_count_;

  explicit as_invocable(Receiver& r,
      const asio::detail::shared_ptr<
        asio::detail::atomic_count>& c) ASIO_NOEXCEPT
    : receiver_(asio::detail::addressof(r)),
      ref_count_(c)
  {
  }

  as_invocable(const as_invocable& other) ASIO_NOEXCEPT
    : receiver_(other.receiver_),
      ref_count_(other.ref_count_)
  {
    ++(*ref_count_);
  }

  ~as_invocable()
  {
    if (--(*ref_count_) == 0)
      execution::set_done(*receiver_);
  }

  void operator()() ASIO_LVALUE_REF_QUAL ASIO_NOEXCEPT
  {
#if !defined(ASIO_NO_EXCEPTIONS)
    try
    {
#endif // !defined(ASIO_NO_EXCEPTIONS)
      execution::set_value(*receiver_);
      ++(*ref_count_);
    }
#if !defined(ASIO_NO_EXCEPTIONS)
    catch (...)
    {
#if defined(ASIO_HAS_STD_EXCEPTION_PTR)
      execution::set_error(*receiver_,
          std::make_exception_ptr(receiver_invocation_error()));
      ++(*ref_count_);
#else // defined(ASIO_HAS_STD_EXCEPTION_PTR)
      std::terminate();
#endif // defined(ASIO_HAS_STD_EXCEPTION_PTR)
    }
#endif // !defined(ASIO_NO_EXCEPTIONS)
  }
};

#endif // defined(ASIO_HAS_MOVE)

template <typename T>
struct is_as_invocable : false_type
{
};

template <typename Function, typename T>
struct is_as_invocable<as_invocable<Function, T> > : true_type
{
};

} // namespace detail
} // namespace execution
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXECUTION_DETAIL_AS_INVOCABLE_HPP
