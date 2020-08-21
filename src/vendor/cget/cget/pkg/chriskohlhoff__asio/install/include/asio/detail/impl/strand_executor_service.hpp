//
// detail/impl/strand_executor_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_STRAND_EXECUTOR_SERVICE_HPP
#define ASIO_DETAIL_IMPL_STRAND_EXECUTOR_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/call_stack.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/handler_invoke_helpers.hpp"
#include "asio/detail/recycling_allocator.hpp"
#include "asio/executor_work_guard.hpp"
#include "asio/defer.hpp"
#include "asio/dispatch.hpp"
#include "asio/post.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

template <typename F, typename Allocator>
class strand_executor_service::allocator_binder
{
public:
  typedef Allocator allocator_type;

  allocator_binder(ASIO_MOVE_ARG(F) f, const Allocator& a)
    : f_(ASIO_MOVE_CAST(F)(f)),
      allocator_(a)
  {
  }

  allocator_binder(const allocator_binder& other)
    : f_(other.f_),
      allocator_(other.allocator_)
  {
  }

#if defined(ASIO_HAS_MOVE)
  allocator_binder(allocator_binder&& other)
    : f_(ASIO_MOVE_CAST(F)(other.f_)),
      allocator_(ASIO_MOVE_CAST(allocator_type)(other.allocator_))
  {
  }
#endif // defined(ASIO_HAS_MOVE)

  allocator_type get_allocator() const ASIO_NOEXCEPT
  {
    return allocator_;
  }

  void operator()()
  {
    f_();
  }

private:
  F f_;
  allocator_type allocator_;
};

template <typename Executor>
class strand_executor_service::invoker<Executor,
    typename enable_if<
      execution::is_executor<Executor>::value
    >::type>
{
public:
  invoker(const implementation_type& impl, Executor& ex)
    : impl_(impl),
      executor_(asio::prefer(ex, execution::outstanding_work.tracked))
  {
  }

  invoker(const invoker& other)
    : impl_(other.impl_),
      executor_(other.executor_)
  {
  }

#if defined(ASIO_HAS_MOVE)
  invoker(invoker&& other)
    : impl_(ASIO_MOVE_CAST(implementation_type)(other.impl_)),
      executor_(ASIO_MOVE_CAST(executor_type)(other.executor_))
  {
  }
#endif // defined(ASIO_HAS_MOVE)

  struct on_invoker_exit
  {
    invoker* this_;

    ~on_invoker_exit()
    {
      this_->impl_->mutex_->lock();
      this_->impl_->ready_queue_.push(this_->impl_->waiting_queue_);
      bool more_handlers = this_->impl_->locked_ =
        !this_->impl_->ready_queue_.empty();
      this_->impl_->mutex_->unlock();

      if (more_handlers)
      {
        recycling_allocator<void> allocator;
        execution::execute(
            asio::prefer(
              asio::require(this_->executor_,
                execution::blocking.never),
            execution::allocator(allocator)),
            ASIO_MOVE_CAST(invoker)(*this_));
      }
    }
  };

  void operator()()
  {
    // Indicate that this strand is executing on the current thread.
    call_stack<strand_impl>::context ctx(impl_.get());

    // Ensure the next handler, if any, is scheduled on block exit.
    on_invoker_exit on_exit = { this };
    (void)on_exit;

    // Run all ready handlers. No lock is required since the ready queue is
    // accessed only within the strand.
    asio::error_code ec;
    while (scheduler_operation* o = impl_->ready_queue_.front())
    {
      impl_->ready_queue_.pop();
      o->complete(impl_.get(), ec, 0);
    }
  }

private:
  typedef typename decay<
      typename prefer_result<
        Executor,
        execution::outstanding_work_t::tracked_t
      >::type
    >::type executor_type;

  implementation_type impl_;
  executor_type executor_;
};

#if !defined(ASIO_NO_TS_EXECUTORS)

template <typename Executor>
class strand_executor_service::invoker<Executor,
    typename enable_if<
      !execution::is_executor<Executor>::value
    >::type>
{
public:
  invoker(const implementation_type& impl, Executor& ex)
    : impl_(impl),
      work_(ex)
  {
  }

  invoker(const invoker& other)
    : impl_(other.impl_),
      work_(other.work_)
  {
  }

#if defined(ASIO_HAS_MOVE)
  invoker(invoker&& other)
    : impl_(ASIO_MOVE_CAST(implementation_type)(other.impl_)),
      work_(ASIO_MOVE_CAST(executor_work_guard<Executor>)(other.work_))
  {
  }
#endif // defined(ASIO_HAS_MOVE)

  struct on_invoker_exit
  {
    invoker* this_;

    ~on_invoker_exit()
    {
      this_->impl_->mutex_->lock();
      this_->impl_->ready_queue_.push(this_->impl_->waiting_queue_);
      bool more_handlers = this_->impl_->locked_ =
        !this_->impl_->ready_queue_.empty();
      this_->impl_->mutex_->unlock();

      if (more_handlers)
      {
        Executor ex(this_->work_.get_executor());
        recycling_allocator<void> allocator;
        ex.post(ASIO_MOVE_CAST(invoker)(*this_), allocator);
      }
    }
  };

  void operator()()
  {
    // Indicate that this strand is executing on the current thread.
    call_stack<strand_impl>::context ctx(impl_.get());

    // Ensure the next handler, if any, is scheduled on block exit.
    on_invoker_exit on_exit = { this };
    (void)on_exit;

    // Run all ready handlers. No lock is required since the ready queue is
    // accessed only within the strand.
    asio::error_code ec;
    while (scheduler_operation* o = impl_->ready_queue_.front())
    {
      impl_->ready_queue_.pop();
      o->complete(impl_.get(), ec, 0);
    }
  }

private:
  implementation_type impl_;
  executor_work_guard<Executor> work_;
};

#endif // !defined(ASIO_NO_TS_EXECUTORS)

template <typename Executor, typename Function>
inline void strand_executor_service::execute(const implementation_type& impl,
    Executor& ex, ASIO_MOVE_ARG(Function) function,
    typename enable_if<
      can_query<Executor, execution::allocator_t<void> >::value
    >::type*)
{
  return strand_executor_service::do_execute(impl, ex,
      ASIO_MOVE_CAST(Function)(function),
      asio::query(ex, execution::allocator));
}

template <typename Executor, typename Function>
inline void strand_executor_service::execute(const implementation_type& impl,
    Executor& ex, ASIO_MOVE_ARG(Function) function,
    typename enable_if<
      !can_query<Executor, execution::allocator_t<void> >::value
    >::type*)
{
  return strand_executor_service::do_execute(impl, ex,
      ASIO_MOVE_CAST(Function)(function),
      std::allocator<void>());
}

template <typename Executor, typename Function, typename Allocator>
void strand_executor_service::do_execute(const implementation_type& impl,
    Executor& ex, ASIO_MOVE_ARG(Function) function, const Allocator& a)
{
  typedef typename decay<Function>::type function_type;

  // If the executor is not never-blocking, and we are already in the strand,
  // then the function can run immediately.
  if (asio::query(ex, execution::blocking) != execution::blocking.never
      && call_stack<strand_impl>::contains(impl.get()))
  {
    // Make a local, non-const copy of the function.
    function_type tmp(ASIO_MOVE_CAST(Function)(function));

    fenced_block b(fenced_block::full);
    asio_handler_invoke_helpers::invoke(tmp, tmp);
    return;
  }

  // Allocate and construct an operation to wrap the function.
  typedef executor_op<function_type, Allocator> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(ASIO_MOVE_CAST(Function)(function), a);

  ASIO_HANDLER_CREATION((impl->service_->context(), *p.p,
        "strand_executor", impl.get(), 0, "execute"));

  // Add the function to the strand and schedule the strand if required.
  bool first = enqueue(impl, p.p);
  p.v = p.p = 0;
  if (first)
  {
    execution::execute(ex, invoker<Executor>(impl, ex));
  }
}

template <typename Executor, typename Function, typename Allocator>
void strand_executor_service::dispatch(const implementation_type& impl,
    Executor& ex, ASIO_MOVE_ARG(Function) function, const Allocator& a)
{
  typedef typename decay<Function>::type function_type;

  // If we are already in the strand then the function can run immediately.
  if (call_stack<strand_impl>::contains(impl.get()))
  {
    // Make a local, non-const copy of the function.
    function_type tmp(ASIO_MOVE_CAST(Function)(function));

    fenced_block b(fenced_block::full);
    asio_handler_invoke_helpers::invoke(tmp, tmp);
    return;
  }

  // Allocate and construct an operation to wrap the function.
  typedef executor_op<function_type, Allocator> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(ASIO_MOVE_CAST(Function)(function), a);

  ASIO_HANDLER_CREATION((impl->service_->context(), *p.p,
        "strand_executor", impl.get(), 0, "dispatch"));

  // Add the function to the strand and schedule the strand if required.
  bool first = enqueue(impl, p.p);
  p.v = p.p = 0;
  if (first)
  {
    asio::dispatch(ex,
        allocator_binder<invoker<Executor>, Allocator>(
          invoker<Executor>(impl, ex), a));
  }
}

// Request invocation of the given function and return immediately.
template <typename Executor, typename Function, typename Allocator>
void strand_executor_service::post(const implementation_type& impl,
    Executor& ex, ASIO_MOVE_ARG(Function) function, const Allocator& a)
{
  typedef typename decay<Function>::type function_type;

  // Allocate and construct an operation to wrap the function.
  typedef executor_op<function_type, Allocator> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(ASIO_MOVE_CAST(Function)(function), a);

  ASIO_HANDLER_CREATION((impl->service_->context(), *p.p,
        "strand_executor", impl.get(), 0, "post"));

  // Add the function to the strand and schedule the strand if required.
  bool first = enqueue(impl, p.p);
  p.v = p.p = 0;
  if (first)
  {
    asio::post(ex,
        allocator_binder<invoker<Executor>, Allocator>(
          invoker<Executor>(impl, ex), a));
  }
}

// Request invocation of the given function and return immediately.
template <typename Executor, typename Function, typename Allocator>
void strand_executor_service::defer(const implementation_type& impl,
    Executor& ex, ASIO_MOVE_ARG(Function) function, const Allocator& a)
{
  typedef typename decay<Function>::type function_type;

  // Allocate and construct an operation to wrap the function.
  typedef executor_op<function_type, Allocator> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(ASIO_MOVE_CAST(Function)(function), a);

  ASIO_HANDLER_CREATION((impl->service_->context(), *p.p,
        "strand_executor", impl.get(), 0, "defer"));

  // Add the function to the strand and schedule the strand if required.
  bool first = enqueue(impl, p.p);
  p.v = p.p = 0;
  if (first)
  {
    asio::defer(ex,
        allocator_binder<invoker<Executor>, Allocator>(
          invoker<Executor>(impl, ex), a));
  }
}

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_IMPL_STRAND_EXECUTOR_SERVICE_HPP
