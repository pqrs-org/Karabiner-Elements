//
// impl/thread_pool.hpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_THREAD_POOL_HPP
#define ASIO_IMPL_THREAD_POOL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/blocking_executor_op.hpp"
#include "asio/detail/bulk_executor_op.hpp"
#include "asio/detail/executor_op.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/non_const_lvalue.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/execution_context.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

inline thread_pool::executor_type
thread_pool::get_executor() ASIO_NOEXCEPT
{
  return executor_type(*this);
}

inline thread_pool::executor_type
thread_pool::executor() ASIO_NOEXCEPT
{
  return executor_type(*this);
}

inline thread_pool::scheduler_type
thread_pool::scheduler() ASIO_NOEXCEPT
{
  return scheduler_type(*this);
}

template <typename Allocator, unsigned int Bits>
thread_pool::basic_executor_type<Allocator, Bits>&
thread_pool::basic_executor_type<Allocator, Bits>::operator=(
    const basic_executor_type& other) ASIO_NOEXCEPT
{
  if (this != &other)
  {
    thread_pool* old_thread_pool = pool_;
    pool_ = other.pool_;
    allocator_ = other.allocator_;
    bits_ = other.bits_;
    if (Bits & outstanding_work_tracked)
    {
      if (pool_)
        pool_->scheduler_.work_started();
      if (old_thread_pool)
        old_thread_pool->scheduler_.work_finished();
    }
  }
  return *this;
}

#if defined(ASIO_HAS_MOVE)
template <typename Allocator, unsigned int Bits>
thread_pool::basic_executor_type<Allocator, Bits>&
thread_pool::basic_executor_type<Allocator, Bits>::operator=(
    basic_executor_type&& other) ASIO_NOEXCEPT
{
  if (this != &other)
  {
    thread_pool* old_thread_pool = pool_;
    pool_ = other.pool_;
    allocator_ = std::move(other.allocator_);
    bits_ = other.bits_;
    if (Bits & outstanding_work_tracked)
    {
      other.pool_ = 0;
      if (old_thread_pool)
        old_thread_pool->scheduler_.work_finished();
    }
  }
  return *this;
}
#endif // defined(ASIO_HAS_MOVE)

template <typename Allocator, unsigned int Bits>
inline bool thread_pool::basic_executor_type<Allocator,
    Bits>::running_in_this_thread() const ASIO_NOEXCEPT
{
  return pool_->scheduler_.can_dispatch();
}

template <typename Allocator, unsigned int Bits>
template <typename Function>
void thread_pool::basic_executor_type<Allocator,
    Bits>::do_execute(ASIO_MOVE_ARG(Function) f, false_type) const
{
  typedef typename decay<Function>::type function_type;

  // Invoke immediately if the blocking.possibly property is enabled and we are
  // already inside the thread pool.
  if ((bits_ & blocking_never) == 0 && pool_->scheduler_.can_dispatch())
  {
    // Make a local, non-const copy of the function.
    function_type tmp(ASIO_MOVE_CAST(Function)(f));

#if defined(ASIO_HAS_STD_EXCEPTION_PTR) \
  && !defined(ASIO_NO_EXCEPTIONS)
    try
    {
#endif // defined(ASIO_HAS_STD_EXCEPTION_PTR)
       //   && !defined(ASIO_NO_EXCEPTIONS)
      detail::fenced_block b(detail::fenced_block::full);
      asio_handler_invoke_helpers::invoke(tmp, tmp);
      return;
#if defined(ASIO_HAS_STD_EXCEPTION_PTR) \
  && !defined(ASIO_NO_EXCEPTIONS)
    }
    catch (...)
    {
      pool_->scheduler_.capture_current_exception();
      return;
    }
#endif // defined(ASIO_HAS_STD_EXCEPTION_PTR)
       //   && !defined(ASIO_NO_EXCEPTIONS)
  }

  // Allocate and construct an operation to wrap the function.
  typedef detail::executor_op<function_type, Allocator> op;
  typename op::ptr p = { detail::addressof(allocator_),
      op::ptr::allocate(allocator_), 0 };
  p.p = new (p.v) op(ASIO_MOVE_CAST(Function)(f), allocator_);

  if ((bits_ & relationship_continuation) != 0)
  {
    ASIO_HANDLER_CREATION((*pool_, *p.p,
          "thread_pool", pool_, 0, "execute(blk=never,rel=cont)"));
  }
  else
  {
    ASIO_HANDLER_CREATION((*pool_, *p.p,
          "thread_pool", pool_, 0, "execute(blk=never,rel=fork)"));
  }

  pool_->scheduler_.post_immediate_completion(p.p,
      (bits_ & relationship_continuation) != 0);
  p.v = p.p = 0;
}

template <typename Allocator, unsigned int Bits>
template <typename Function>
void thread_pool::basic_executor_type<Allocator,
    Bits>::do_execute(ASIO_MOVE_ARG(Function) f, true_type) const
{
  // Obtain a non-const instance of the function.
  detail::non_const_lvalue<Function> f2(f);

  // Invoke immediately if we are already inside the thread pool.
  if (pool_->scheduler_.can_dispatch())
  {
#if !defined(ASIO_NO_EXCEPTIONS)
    try
    {
#endif // !defined(ASIO_NO_EXCEPTIONS)
      detail::fenced_block b(detail::fenced_block::full);
      asio_handler_invoke_helpers::invoke(f2.value, f2.value);
      return;
#if !defined(ASIO_NO_EXCEPTIONS)
    }
    catch (...)
    {
      std::terminate();
    }
#endif // !defined(ASIO_NO_EXCEPTIONS)
  }

  // Construct an operation to wrap the function.
  typedef typename decay<Function>::type function_type;
  detail::blocking_executor_op<function_type> op(f2.value);

  ASIO_HANDLER_CREATION((*pool_, op,
        "thread_pool", pool_, 0, "execute(blk=always)"));

  pool_->scheduler_.post_immediate_completion(&op, false);
  op.wait();
}

template <typename Allocator, unsigned int Bits>
template <typename Function>
void thread_pool::basic_executor_type<Allocator, Bits>::do_bulk_execute(
    ASIO_MOVE_ARG(Function) f, std::size_t n, false_type) const
{
  typedef typename decay<Function>::type function_type;
  typedef detail::bulk_executor_op<function_type, Allocator> op;

  // Allocate and construct operations to wrap the function.
  detail::op_queue<detail::scheduler_operation> ops;
  for (std::size_t i = 0; i < n; ++i)
  {
    typename op::ptr p = { detail::addressof(allocator_),
        op::ptr::allocate(allocator_), 0 };
    p.p = new (p.v) op(ASIO_MOVE_CAST(Function)(f), allocator_, i);
    ops.push(p.p);

    if ((bits_ & relationship_continuation) != 0)
    {
      ASIO_HANDLER_CREATION((*pool_, *p.p,
            "thread_pool", pool_, 0, "bulk_execute(blk=never,rel=cont)"));
    }
    else
    {
      ASIO_HANDLER_CREATION((*pool_, *p.p,
            "thread_pool", pool_, 0, "bulk)execute(blk=never,rel=fork)"));
    }

    p.v = p.p = 0;
  }

  pool_->scheduler_.post_immediate_completions(n,
      ops, (bits_ & relationship_continuation) != 0);
}

template <typename Function>
struct thread_pool_always_blocking_function_adapter
{
  typename decay<Function>::type* f;
  std::size_t n;

  void operator()()
  {
    for (std::size_t i = 0; i < n; ++i)
    {
      (*f)(i);
    }
  }
};

template <typename Allocator, unsigned int Bits>
template <typename Function>
void thread_pool::basic_executor_type<Allocator, Bits>::do_bulk_execute(
    ASIO_MOVE_ARG(Function) f, std::size_t n, true_type) const
{
  // Obtain a non-const instance of the function.
  detail::non_const_lvalue<Function> f2(f);

  thread_pool_always_blocking_function_adapter<Function>
    adapter = { detail::addressof(f2.value), n };

  this->do_execute(adapter, true_type());
}

#if !defined(ASIO_NO_TS_EXECUTORS)
template <typename Allocator, unsigned int Bits>
inline thread_pool& thread_pool::basic_executor_type<
    Allocator, Bits>::context() const ASIO_NOEXCEPT
{
  return *pool_;
}

template <typename Allocator, unsigned int Bits>
inline void thread_pool::basic_executor_type<Allocator,
    Bits>::on_work_started() const ASIO_NOEXCEPT
{
  pool_->scheduler_.work_started();
}

template <typename Allocator, unsigned int Bits>
inline void thread_pool::basic_executor_type<Allocator,
    Bits>::on_work_finished() const ASIO_NOEXCEPT
{
  pool_->scheduler_.work_finished();
}

template <typename Allocator, unsigned int Bits>
template <typename Function, typename OtherAllocator>
void thread_pool::basic_executor_type<Allocator, Bits>::dispatch(
    ASIO_MOVE_ARG(Function) f, const OtherAllocator& a) const
{
  typedef typename decay<Function>::type function_type;

  // Invoke immediately if we are already inside the thread pool.
  if (pool_->scheduler_.can_dispatch())
  {
    // Make a local, non-const copy of the function.
    function_type tmp(ASIO_MOVE_CAST(Function)(f));

    detail::fenced_block b(detail::fenced_block::full);
    asio_handler_invoke_helpers::invoke(tmp, tmp);
    return;
  }

  // Allocate and construct an operation to wrap the function.
  typedef detail::executor_op<function_type, OtherAllocator> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(ASIO_MOVE_CAST(Function)(f), a);

  ASIO_HANDLER_CREATION((*pool_, *p.p,
        "thread_pool", pool_, 0, "dispatch"));

  pool_->scheduler_.post_immediate_completion(p.p, false);
  p.v = p.p = 0;
}

template <typename Allocator, unsigned int Bits>
template <typename Function, typename OtherAllocator>
void thread_pool::basic_executor_type<Allocator, Bits>::post(
    ASIO_MOVE_ARG(Function) f, const OtherAllocator& a) const
{
  typedef typename decay<Function>::type function_type;

  // Allocate and construct an operation to wrap the function.
  typedef detail::executor_op<function_type, OtherAllocator> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(ASIO_MOVE_CAST(Function)(f), a);

  ASIO_HANDLER_CREATION((*pool_, *p.p,
        "thread_pool", pool_, 0, "post"));

  pool_->scheduler_.post_immediate_completion(p.p, false);
  p.v = p.p = 0;
}

template <typename Allocator, unsigned int Bits>
template <typename Function, typename OtherAllocator>
void thread_pool::basic_executor_type<Allocator, Bits>::defer(
    ASIO_MOVE_ARG(Function) f, const OtherAllocator& a) const
{
  typedef typename decay<Function>::type function_type;

  // Allocate and construct an operation to wrap the function.
  typedef detail::executor_op<function_type, OtherAllocator> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(ASIO_MOVE_CAST(Function)(f), a);

  ASIO_HANDLER_CREATION((*pool_, *p.p,
        "thread_pool", pool_, 0, "defer"));

  pool_->scheduler_.post_immediate_completion(p.p, true);
  p.v = p.p = 0;
}
#endif // !defined(ASIO_NO_TS_EXECUTORS)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_THREAD_POOL_HPP
