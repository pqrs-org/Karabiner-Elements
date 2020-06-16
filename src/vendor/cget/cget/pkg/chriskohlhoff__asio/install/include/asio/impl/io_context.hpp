//
// impl/io_context.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_IO_CONTEXT_HPP
#define ASIO_IMPL_IO_CONTEXT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/completion_handler.hpp"
#include "asio/detail/executor_op.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/handler_type_requirements.hpp"
#include "asio/detail/non_const_lvalue.hpp"
#include "asio/detail/recycling_allocator.hpp"
#include "asio/detail/service_registry.hpp"
#include "asio/detail/throw_error.hpp"
#include "asio/detail/type_traits.hpp"

#include "asio/detail/push_options.hpp"

#if !defined(GENERATING_DOCUMENTATION)

namespace asio {

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

} // namespace asio

#endif // !defined(GENERATING_DOCUMENTATION)

#include "asio/detail/pop_options.hpp"

#if defined(ASIO_HAS_IOCP)
# include "asio/detail/win_iocp_io_context.hpp"
#else
# include "asio/detail/scheduler.hpp"
#endif

#include "asio/detail/push_options.hpp"

namespace asio {

inline io_context::executor_type
io_context::get_executor() ASIO_NOEXCEPT
{
  return executor_type(*this);
}

#if defined(ASIO_HAS_CHRONO)

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

    asio::error_code ec;
    std::size_t s = impl_.wait_one(
        static_cast<long>(chrono::duration_cast<
          chrono::microseconds>(rel_time).count()), ec);
    asio::detail::throw_error(ec);

    if (s || impl_.stopped())
      return s;

    now = Clock::now();
  }

  return 0;
}

#endif // defined(ASIO_HAS_CHRONO)

#if !defined(ASIO_NO_DEPRECATED)

inline void io_context::reset()
{
  restart();
}

struct io_context::initiate_dispatch
{
  template <typename LegacyCompletionHandler>
  void operator()(ASIO_MOVE_ARG(LegacyCompletionHandler) handler,
      io_context* self) const
  {
    // If you get an error on the following line it means that your handler does
    // not meet the documented type requirements for a LegacyCompletionHandler.
    ASIO_LEGACY_COMPLETION_HANDLER_CHECK(
        LegacyCompletionHandler, handler) type_check;

    detail::non_const_lvalue<LegacyCompletionHandler> handler2(handler);
    if (self->impl_.can_dispatch())
    {
      detail::fenced_block b(detail::fenced_block::full);
      asio_handler_invoke_helpers::invoke(
          handler2.value, handler2.value);
    }
    else
    {
      // Allocate and construct an operation to wrap the handler.
      typedef detail::completion_handler<
        typename decay<LegacyCompletionHandler>::type> op;
      typename op::ptr p = { detail::addressof(handler2.value),
        op::ptr::allocate(handler2.value), 0 };
      p.p = new (p.v) op(handler2.value);

      ASIO_HANDLER_CREATION((*self, *p.p,
            "io_context", self, 0, "dispatch"));

      self->impl_.do_dispatch(p.p);
      p.v = p.p = 0;
    }
  }
};

template <typename LegacyCompletionHandler>
ASIO_INITFN_AUTO_RESULT_TYPE(LegacyCompletionHandler, void ())
io_context::dispatch(ASIO_MOVE_ARG(LegacyCompletionHandler) handler)
{
  return async_initiate<LegacyCompletionHandler, void ()>(
      initiate_dispatch(), handler, this);
}

struct io_context::initiate_post
{
  template <typename LegacyCompletionHandler>
  void operator()(ASIO_MOVE_ARG(LegacyCompletionHandler) handler,
      io_context* self) const
  {
    // If you get an error on the following line it means that your handler does
    // not meet the documented type requirements for a LegacyCompletionHandler.
    ASIO_LEGACY_COMPLETION_HANDLER_CHECK(
        LegacyCompletionHandler, handler) type_check;

    detail::non_const_lvalue<LegacyCompletionHandler> handler2(handler);

    bool is_continuation =
      asio_handler_cont_helpers::is_continuation(handler2.value);

    // Allocate and construct an operation to wrap the handler.
    typedef detail::completion_handler<
      typename decay<LegacyCompletionHandler>::type> op;
    typename op::ptr p = { detail::addressof(handler2.value),
        op::ptr::allocate(handler2.value), 0 };
    p.p = new (p.v) op(handler2.value);

    ASIO_HANDLER_CREATION((*self, *p.p,
          "io_context", self, 0, "post"));

    self->impl_.post_immediate_completion(p.p, is_continuation);
    p.v = p.p = 0;
  }
};

template <typename LegacyCompletionHandler>
ASIO_INITFN_AUTO_RESULT_TYPE(LegacyCompletionHandler, void ())
io_context::post(ASIO_MOVE_ARG(LegacyCompletionHandler) handler)
{
  return async_initiate<LegacyCompletionHandler, void ()>(
      initiate_post(), handler, this);
}

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

inline io_context&
io_context::executor_type::context() const ASIO_NOEXCEPT
{
  return io_context_;
}

inline void
io_context::executor_type::on_work_started() const ASIO_NOEXCEPT
{
  io_context_.impl_.work_started();
}

inline void
io_context::executor_type::on_work_finished() const ASIO_NOEXCEPT
{
  io_context_.impl_.work_finished();
}

template <typename Function, typename Allocator>
void io_context::executor_type::dispatch(
    ASIO_MOVE_ARG(Function) f, const Allocator& a) const
{
  typedef typename decay<Function>::type function_type;

  // Invoke immediately if we are already inside the thread pool.
  if (io_context_.impl_.can_dispatch())
  {
    // Make a local, non-const copy of the function.
    function_type tmp(ASIO_MOVE_CAST(Function)(f));

    detail::fenced_block b(detail::fenced_block::full);
    asio_handler_invoke_helpers::invoke(tmp, tmp);
    return;
  }

  // Allocate and construct an operation to wrap the function.
  typedef detail::executor_op<function_type, Allocator, detail::operation> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(ASIO_MOVE_CAST(Function)(f), a);

  ASIO_HANDLER_CREATION((this->context(), *p.p,
        "io_context", &this->context(), 0, "dispatch"));

  io_context_.impl_.post_immediate_completion(p.p, false);
  p.v = p.p = 0;
}

template <typename Function, typename Allocator>
void io_context::executor_type::post(
    ASIO_MOVE_ARG(Function) f, const Allocator& a) const
{
  typedef typename decay<Function>::type function_type;

  // Allocate and construct an operation to wrap the function.
  typedef detail::executor_op<function_type, Allocator, detail::operation> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(ASIO_MOVE_CAST(Function)(f), a);

  ASIO_HANDLER_CREATION((this->context(), *p.p,
        "io_context", &this->context(), 0, "post"));

  io_context_.impl_.post_immediate_completion(p.p, false);
  p.v = p.p = 0;
}

template <typename Function, typename Allocator>
void io_context::executor_type::defer(
    ASIO_MOVE_ARG(Function) f, const Allocator& a) const
{
  typedef typename decay<Function>::type function_type;

  // Allocate and construct an operation to wrap the function.
  typedef detail::executor_op<function_type, Allocator, detail::operation> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(ASIO_MOVE_CAST(Function)(f), a);

  ASIO_HANDLER_CREATION((this->context(), *p.p,
        "io_context", &this->context(), 0, "defer"));

  io_context_.impl_.post_immediate_completion(p.p, true);
  p.v = p.p = 0;
}

inline bool
io_context::executor_type::running_in_this_thread() const ASIO_NOEXCEPT
{
  return io_context_.impl_.can_dispatch();
}

#if !defined(ASIO_NO_DEPRECATED)
inline io_context::work::work(asio::io_context& io_context)
  : io_context_impl_(io_context.impl_)
{
  io_context_impl_.work_started();
}

inline io_context::work::work(const work& other)
  : io_context_impl_(other.io_context_impl_)
{
  io_context_impl_.work_started();
}

inline io_context::work::~work()
{
  io_context_impl_.work_finished();
}

inline asio::io_context& io_context::work::get_io_context()
{
  return static_cast<asio::io_context&>(io_context_impl_.context());
}
#endif // !defined(ASIO_NO_DEPRECATED)

inline asio::io_context& io_context::service::get_io_context()
{
  return static_cast<asio::io_context&>(context());
}

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_IO_CONTEXT_HPP
