//
// impl/io_context.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_IO_CONTEXT_HPP
#define ASIO_IMPL_IO_CONTEXT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/config.hpp"
#include "asio/detail/completion_handler.hpp"
#include "asio/detail/executor_function.hpp"
#include "asio/detail/executor_op.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/handler_type_requirements.hpp"
#include "asio/detail/non_const_lvalue.hpp"
#include "asio/detail/service_registry.hpp"
#include "asio/detail/type_traits.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN

template <typename Allocator>
io_context::io_context(allocator_arg_t, const Allocator& a)
  : execution_context(std::allocator_arg, a, config_from_concurrency_hint()),
    impl_(asio::make_service<impl_type>(*this, false))
{
}

template <typename Allocator>
io_context::io_context(allocator_arg_t,
    const Allocator& a, int concurrency_hint)
  : execution_context(std::allocator_arg, a,
      config_from_concurrency_hint(concurrency_hint)),
    impl_(asio::make_service<impl_type>(*this, false))
{
}

template <typename Allocator>
io_context::io_context(allocator_arg_t, const Allocator& a,
    const execution_context::service_maker& initial_services)
  : execution_context(std::allocator_arg, a, initial_services),
    impl_(asio::make_service<impl_type>(*this, false))
{
}

#if !defined(GENERATING_DOCUMENTATION)

template <typename Service>
inline Service& use_service(io_context& ioc)
{
  // Check that Service meets the necessary type requirements.
  (void)static_cast<execution_context::service*>(static_cast<Service*>(0));
  (void)static_cast<const execution_context::id*>(&Service::id);

  return ioc.service_registry_->template use_service<Service>(ioc);
}

template <>
inline detail::io_context_impl& use_service<detail::io_context_impl>(
    io_context& ioc)
{
  return ioc.impl_;
}

#endif // !defined(GENERATING_DOCUMENTATION)

inline io_context::executor_type
io_context::get_executor() noexcept
{
  return executor_type(*this);
}

template <typename Rep, typename Period>
std::size_t io_context::run_for(
    const chrono::duration<Rep, Period>& rel_time)
{
  return this->run_until(chrono::steady_clock::now() + rel_time);
}

template <typename Clock, typename Duration>
std::size_t io_context::run_until(
    const chrono::time_point<Clock, Duration>& abs_time)
{
  std::size_t n = 0;
  while (this->run_one_until(abs_time))
    if (n != (std::numeric_limits<std::size_t>::max)())
      ++n;
  return n;
}

template <typename Rep, typename Period>
std::size_t io_context::run_one_for(
    const chrono::duration<Rep, Period>& rel_time)
{
  return this->run_one_until(chrono::steady_clock::now() + rel_time);
}

template <typename Clock, typename Duration>
std::size_t io_context::run_one_until(
    const chrono::time_point<Clock, Duration>& abs_time)
{
  typename Clock::time_point now = Clock::now();
  while (now < abs_time)
  {
    typename Clock::duration rel_time = abs_time - now;
    if (rel_time > chrono::seconds(1))
      rel_time = chrono::seconds(1);

    std::size_t s = impl_.wait_one(
        static_cast<long>(chrono::duration_cast<
          chrono::microseconds>(rel_time).count()));

    if (s || impl_.stopped())
      return s;

    now = Clock::now();
  }

  return 0;
}

#if !defined(ASIO_NO_DEPRECATED)

template <typename Handler>
#if defined(GENERATING_DOCUMENTATION)
unspecified
#else
inline detail::wrapped_handler<io_context&, Handler>
#endif
io_context::wrap(Handler handler)
{
  return detail::wrapped_handler<io_context&, Handler>(*this, handler);
}

#endif // !defined(ASIO_NO_DEPRECATED)

template <typename Allocator, uintptr_t Bits>
io_context::basic_executor_type<Allocator, Bits>&
io_context::basic_executor_type<Allocator, Bits>::operator=(
    const basic_executor_type& other) noexcept
{
  if (this != &other)
  {
    this->allocator_ref() = other.allocator_ref();
    io_context* old_io_context = context_ptr();
    target_ = other.target_;
    if (Bits & outstanding_work_tracked)
    {
      if (context_ptr())
        context_ptr()->impl_.work_started();
      if (old_io_context)
        old_io_context->impl_.work_finished();
    }
  }
  return *this;
}

template <typename Allocator, uintptr_t Bits>
io_context::basic_executor_type<Allocator, Bits>&
io_context::basic_executor_type<Allocator, Bits>::operator=(
    basic_executor_type&& other) noexcept
{
  if (this != &other)
  {
    this->allocator_ref() =
      static_cast<Allocator&&>(other.allocator_ref());
    io_context* old_io_context = context_ptr();
    target_ = other.target_;
    if (Bits & outstanding_work_tracked)
    {
      other.target_ = 0;
      if (old_io_context)
        old_io_context->impl_.work_finished();
    }
  }
  return *this;
}

template <typename Allocator, uintptr_t Bits>
inline bool io_context::basic_executor_type<Allocator,
    Bits>::running_in_this_thread() const noexcept
{
  return context_ptr()->impl_.can_dispatch();
}

template <typename Function>
void io_context::execute_immediately(io_context* ctx, Function&& f)
{
  typedef decay_t<Function> function_type;

  // Obtain a non-const instance of the function.
  detail::non_const_lvalue<Function> f2(f);

#if defined(ASIO_NO_EXCEPTIONS)
  (void)ctx;
  detail::fenced_block b(detail::fenced_block::full);
  static_cast<function_type&&>(f2.value)();
#else // defined(ASIO_NO_EXCEPTIONS)
  try
  {
    detail::fenced_block b(detail::fenced_block::full);
    static_cast<function_type&&>(f2.value)();
  }
  catch (...)
  {
    ctx->impl_.capture_current_exception();
  }
#endif // defined(ASIO_NO_EXCEPTIONS)
}

template <typename Allocator, uintptr_t Bits>
template <typename Function>
ASIO_ALWAYS_INLINE
void io_context::basic_executor_type<Allocator, Bits>::do_execute(
    Function&& f) const
{
  typedef decay_t<Function> function_type;

  // Invoke immediately if the blocking.possibly property is enabled and we are
  // already inside the thread pool.
  if ((bits() & blocking_never) == 0 && context_ptr()->impl_.can_dispatch())
  {
    io_context::execute_immediately(context_ptr(), static_cast<Function&&>(f));
    return;
  }

  // Allocate and construct an operation to wrap the function.
  typedef detail::executor_op<function_type, Allocator> op;
  typename op::ptr p = {
      detail::addressof(this->allocator_ref()),
      op::ptr::allocate(this->allocator_ref()), 0 };
  p.p = new (p.v) op(static_cast<Function&&>(f),
      this->allocator_ref());

  ASIO_HANDLER_CREATION((*context_ptr(), *p.p,
        "io_context", context_ptr(), 0, "execute"));

  if ((bits() & relationship_continuation) != 0)
    context_ptr()->impl_.post_immediate_completion(p.p, true);
  else
    context_ptr()->impl_.post_immediate_completion(p.p, false);
  p.v = p.p = 0;
}

template <typename Allocator, uintptr_t Bits>
void io_context::basic_executor_type<Allocator, Bits>::do_execute(
    detail::executor_function&& f) const
{
  // Invoke immediately if the blocking.possibly property is enabled and we are
  // already inside the thread pool.
  if ((bits() & blocking_never) == 0 && context_ptr()->impl_.can_dispatch())
  {
    io_context::execute_immediately(context_ptr(),
        static_cast<detail::executor_function&&>(f));
    return;
  }

  ASIO_HANDLER_CREATION((*context_ptr(), *f.get(),
        "io_context", context_ptr(), 0, "execute"));

  if ((bits() & relationship_continuation) != 0)
    context_ptr()->impl_.post_immediate_completion(f.release(), true);
  else
    context_ptr()->impl_.post_immediate_completion(f.release(), false);
}

#if !defined(ASIO_NO_TS_EXECUTORS)
template <typename Allocator, uintptr_t Bits>
inline io_context& io_context::basic_executor_type<
    Allocator, Bits>::context() const noexcept
{
  return *context_ptr();
}

template <typename Allocator, uintptr_t Bits>
inline void io_context::basic_executor_type<Allocator,
    Bits>::on_work_started() const noexcept
{
  context_ptr()->impl_.work_started();
}

template <typename Allocator, uintptr_t Bits>
inline void io_context::basic_executor_type<Allocator,
    Bits>::on_work_finished() const noexcept
{
  context_ptr()->impl_.work_finished();
}

template <typename Allocator, uintptr_t Bits>
template <typename Function, typename OtherAllocator>
void io_context::basic_executor_type<Allocator, Bits>::dispatch(
    Function&& f, const OtherAllocator& a) const
{
  typedef decay_t<Function> function_type;

  // Invoke immediately if we are already inside the thread pool.
  if (context_ptr()->impl_.can_dispatch())
  {
    // Make a local, non-const copy of the function.
    function_type tmp(static_cast<Function&&>(f));

    detail::fenced_block b(detail::fenced_block::full);
    static_cast<function_type&&>(tmp)();
    return;
  }

  // Allocate and construct an operation to wrap the function.
  typedef detail::executor_op<function_type, OtherAllocator> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(static_cast<Function&&>(f), a);

  ASIO_HANDLER_CREATION((*context_ptr(), *p.p,
        "io_context", context_ptr(), 0, "dispatch"));

  context_ptr()->impl_.post_immediate_completion(p.p, false);
  p.v = p.p = 0;
}

template <typename Allocator, uintptr_t Bits>
template <typename Function, typename OtherAllocator>
void io_context::basic_executor_type<Allocator, Bits>::post(
    Function&& f, const OtherAllocator& a) const
{
  // Allocate and construct an operation to wrap the function.
  typedef detail::executor_op<decay_t<Function>, OtherAllocator> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(static_cast<Function&&>(f), a);

  ASIO_HANDLER_CREATION((*context_ptr(), *p.p,
        "io_context", context_ptr(), 0, "post"));

  context_ptr()->impl_.post_immediate_completion(p.p, false);
  p.v = p.p = 0;
}

template <typename Allocator, uintptr_t Bits>
template <typename Function, typename OtherAllocator>
void io_context::basic_executor_type<Allocator, Bits>::defer(
    Function&& f, const OtherAllocator& a) const
{
  // Allocate and construct an operation to wrap the function.
  typedef detail::executor_op<decay_t<Function>, OtherAllocator> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(static_cast<Function&&>(f), a);

  ASIO_HANDLER_CREATION((*context_ptr(), *p.p,
        "io_context", context_ptr(), 0, "defer"));

  context_ptr()->impl_.post_immediate_completion(p.p, true);
  p.v = p.p = 0;
}
#endif // !defined(ASIO_NO_TS_EXECUTORS)

inline asio::io_context& io_context::service::get_io_context()
{
  return static_cast<asio::io_context&>(context());
}

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_IO_CONTEXT_HPP
