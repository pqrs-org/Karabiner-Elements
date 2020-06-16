//
// io_object_executor.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IO_OBJECT_EXECUTOR_HPP
#define ASIO_DETAIL_IO_OBJECT_EXECUTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/handler_invoke_helpers.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/io_context.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

// Wrap the (potentially polymorphic) executor so that we can bypass it when
// dispatching on a target executor that has a native I/O implementation.
template <typename Executor>
class io_object_executor
{
public:
  io_object_executor(const Executor& ex,
      bool native_implementation) ASIO_NOEXCEPT
    : executor_(ex),
      has_native_impl_(native_implementation)
  {
  }

  io_object_executor(const io_object_executor& other) ASIO_NOEXCEPT
    : executor_(other.executor_),
      has_native_impl_(other.has_native_impl_)
  {
  }

  template <typename Executor1>
  io_object_executor(
      const io_object_executor<Executor1>& other) ASIO_NOEXCEPT
    : executor_(other.inner_executor()),
      has_native_impl_(other.has_native_implementation())
  {
  }

#if defined(ASIO_HAS_MOVE)
  io_object_executor(io_object_executor&& other) ASIO_NOEXCEPT
    : executor_(ASIO_MOVE_CAST(Executor)(other.executor_)),
      has_native_impl_(other.has_native_impl_)
  {
  }
#endif // defined(ASIO_HAS_MOVE)

  const Executor& inner_executor() const ASIO_NOEXCEPT
  {
    return executor_;
  }

  bool has_native_implementation() const ASIO_NOEXCEPT
  {
    return has_native_impl_;
  }

  execution_context& context() const ASIO_NOEXCEPT
  {
    return executor_.context();
  }

  void on_work_started() const ASIO_NOEXCEPT
  {
    if (is_same<Executor, io_context::executor_type>::value
        || has_native_impl_)
    {
      // When using a native implementation, work is already counted by the
      // execution context.
    }
    else
    {
      executor_.on_work_started();
    }
  }

  void on_work_finished() const ASIO_NOEXCEPT
  {
    if (is_same<Executor, io_context::executor_type>::value
        || has_native_impl_)
    {
      // When using a native implementation, work is already counted by the
      // execution context.
    }
    else
    {
      executor_.on_work_finished();
    }
  }

  template <typename F, typename A>
  void dispatch(ASIO_MOVE_ARG(F) f, const A& a) const
  {
    if (is_same<Executor, io_context::executor_type>::value
        || has_native_impl_)
    {
      // When using a native implementation, I/O completion handlers are
      // already dispatched according to the execution context's executor's
      // rules. We can call the function directly.
#if defined(ASIO_HAS_MOVE)
      if (is_same<F, typename decay<F>::type>::value)
      {
        asio_handler_invoke_helpers::invoke(f, f);
        return;
      }
#endif // defined(ASIO_HAS_MOVE)
      typename decay<F>::type function(ASIO_MOVE_CAST(F)(f));
      asio_handler_invoke_helpers::invoke(function, function);
    }
    else
    {
      executor_.dispatch(ASIO_MOVE_CAST(F)(f), a);
    }
  }

  template <typename F, typename A>
  void post(ASIO_MOVE_ARG(F) f, const A& a) const
  {
    executor_.post(ASIO_MOVE_CAST(F)(f), a);
  }

  template <typename F, typename A>
  void defer(ASIO_MOVE_ARG(F) f, const A& a) const
  {
    executor_.defer(ASIO_MOVE_CAST(F)(f), a);
  }

  friend bool operator==(const io_object_executor& a,
      const io_object_executor& b) ASIO_NOEXCEPT
  {
    return a.executor_ == b.executor_
      && a.has_native_impl_ == b.has_native_impl_;
  }

  friend bool operator!=(const io_object_executor& a,
      const io_object_executor& b) ASIO_NOEXCEPT
  {
    return a.executor_ != b.executor_
      || a.has_native_impl_ != b.has_native_impl_;
  }

private:
  Executor executor_;
  const bool has_native_impl_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_IO_OBJECT_EXECUTOR_HPP
