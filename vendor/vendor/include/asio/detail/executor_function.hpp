//
// detail/executor_function.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_EXECUTOR_FUNCTION_HPP
#define ASIO_DETAIL_EXECUTOR_FUNCTION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/executor_op.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/operation.hpp"
#include "asio/detail/type_traits.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

// Lightweight, move-only function object wrapper.
class executor_function
{
public:
  template <typename F, typename Alloc>
  explicit executor_function(F f, const Alloc& a)
    : op_(executor_function::make_operation(static_cast<F&&>(f), a))
  {
  }

  // Adopts an operation that already holds the function.
  explicit executor_function(operation* op) noexcept
    : op_(op)
  {
  }

  executor_function(executor_function&& other) noexcept
    : op_(other.op_)
  {
    other.op_ = 0;
  }

  ~executor_function()
  {
    if (op_)
      op_->destroy();
  }

  void operator()()
  {
    if (operation* o = op_)
    {
      op_ = 0;
      o->complete(o, 0, 0);
    }
  }

  // Obtains the operation that holds the function, retaining ownership.
  operation* get() const noexcept
  {
    return op_;
  }

  // Relinquishes ownership of the operation that holds the function.
  operation* release() noexcept
  {
    operation* o = op_;
    op_ = 0;
    return o;
  }

  // Allocates an operation to hold the function.
  template <typename F, typename Alloc>
  static operation* make_operation(F&& f, const Alloc& a)
  {
    typedef executor_op<decay_t<F>, Alloc,
        thread_info_base::executor_function_tag> op_type;
    typename op_type::ptr p = {
      detail::addressof(a), op_type::ptr::allocate(a), 0 };
    op_type* op = new (p.v) op_type(static_cast<F&&>(f), a);
    p.v = 0;
    return op;
  }

private:
  executor_function(const executor_function&);
  executor_function& operator=(const executor_function&);

  operation* op_;
};

// Lightweight, non-owning, copyable function object wrapper.
class executor_function_view
{
public:
  template <typename F>
  explicit executor_function_view(F& f) noexcept
    : complete_(&executor_function_view::complete<F>),
      function_(&f)
  {
  }

  void operator()()
  {
    complete_(function_);
  }

private:
  // Helper to complete function invocation.
  template <typename F>
  static void complete(void* f)
  {
    (*static_cast<F*>(f))();
  }

  void (*complete_)(void*);
  void* function_;
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_EXECUTOR_FUNCTION_HPP
