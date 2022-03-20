//
// impl/system_executor.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_SYSTEM_EXECUTOR_HPP
#define ASIO_IMPL_SYSTEM_EXECUTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/executor_op.hpp"
#include "asio/detail/global.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/system_context.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

template <typename Blocking, typename Relationship, typename Allocator>
inline system_context&
basic_system_executor<Blocking, Relationship, Allocator>::query(
    execution::context_t) ASIO_NOEXCEPT
{
  return detail::global<system_context>();
}

template <typename Blocking, typename Relationship, typename Allocator>
inline std::size_t
basic_system_executor<Blocking, Relationship, Allocator>::query(
    execution::occupancy_t) const ASIO_NOEXCEPT
{
  return detail::global<system_context>().num_threads_;
}

template <typename Blocking, typename Relationship, typename Allocator>
template <typename Function>
inline void
basic_system_executor<Blocking, Relationship, Allocator>::do_execute(
    ASIO_MOVE_ARG(Function) f, execution::blocking_t::possibly_t) const
{
  // Obtain a non-const instance of the function.
  detail::non_const_lvalue<Function> f2(f);

#if !defined(ASIO_NO_EXCEPTIONS)
  try
  {
#endif// !defined(ASIO_NO_EXCEPTIONS)
    detail::fenced_block b(detail::fenced_block::full);
    asio_handler_invoke_helpers::invoke(f2.value, f2.value);
#if !defined(ASIO_NO_EXCEPTIONS)
  }
  catch (...)
  {
    std::terminate();
  }
#endif// !defined(ASIO_NO_EXCEPTIONS)
}

template <typename Blocking, typename Relationship, typename Allocator>
template <typename Function>
inline void
basic_system_executor<Blocking, Relationship, Allocator>::do_execute(
    ASIO_MOVE_ARG(Function) f, execution::blocking_t::always_t) const
{
  // Obtain a non-const instance of the function.
  detail::non_const_lvalue<Function> f2(f);

#if !defined(ASIO_NO_EXCEPTIONS)
  try
  {
#endif// !defined(ASIO_NO_EXCEPTIONS)
    detail::fenced_block b(detail::fenced_block::full);
    asio_handler_invoke_helpers::invoke(f2.value, f2.value);
#if !defined(ASIO_NO_EXCEPTIONS)
  }
  catch (...)
  {
    std::terminate();
  }
#endif// !defined(ASIO_NO_EXCEPTIONS)
}

template <typename Blocking, typename Relationship, typename Allocator>
template <typename Function>
void basic_system_executor<Blocking, Relationship, Allocator>::do_execute(
    ASIO_MOVE_ARG(Function) f, execution::blocking_t::never_t) const
{
  system_context& ctx = detail::global<system_context>();

  // Allocate and construct an operation to wrap the function.
  typedef typename decay<Function>::type function_type;
  typedef detail::executor_op<function_type, Allocator> op;
  typename op::ptr p = { detail::addressof(allocator_),
      op::ptr::allocate(allocator_), 0 };
  p.p = new (p.v) op(ASIO_MOVE_CAST(Function)(f), allocator_);

  if (is_same<Relationship, execution::relationship_t::continuation_t>::value)
  {
    ASIO_HANDLER_CREATION((ctx, *p.p,
          "system_executor", &ctx, 0, "execute(blk=never,rel=cont)"));
  }
  else
  {
    ASIO_HANDLER_CREATION((ctx, *p.p,
          "system_executor", &ctx, 0, "execute(blk=never,rel=fork)"));
  }

  ctx.scheduler_.post_immediate_completion(p.p,
      is_same<Relationship, execution::relationship_t::continuation_t>::value);
  p.v = p.p = 0;
}

#if !defined(ASIO_NO_TS_EXECUTORS)
template <typename Blocking, typename Relationship, typename Allocator>
inline system_context& basic_system_executor<
    Blocking, Relationship, Allocator>::context() const ASIO_NOEXCEPT
{
  return detail::global<system_context>();
}

template <typename Blocking, typename Relationship, typename Allocator>
template <typename Function, typename OtherAllocator>
void basic_system_executor<Blocking, Relationship, Allocator>::dispatch(
    ASIO_MOVE_ARG(Function) f, const OtherAllocator&) const
{
  typename decay<Function>::type tmp(ASIO_MOVE_CAST(Function)(f));
  asio_handler_invoke_helpers::invoke(tmp, tmp);
}

template <typename Blocking, typename Relationship, typename Allocator>
template <typename Function, typename OtherAllocator>
void basic_system_executor<Blocking, Relationship, Allocator>::post(
    ASIO_MOVE_ARG(Function) f, const OtherAllocator& a) const
{
  typedef typename decay<Function>::type function_type;

  system_context& ctx = detail::global<system_context>();

  // Allocate and construct an operation to wrap the function.
  typedef detail::executor_op<function_type, OtherAllocator> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(ASIO_MOVE_CAST(Function)(f), a);

  ASIO_HANDLER_CREATION((ctx, *p.p,
        "system_executor", &this->context(), 0, "post"));

  ctx.scheduler_.post_immediate_completion(p.p, false);
  p.v = p.p = 0;
}

template <typename Blocking, typename Relationship, typename Allocator>
template <typename Function, typename OtherAllocator>
void basic_system_executor<Blocking, Relationship, Allocator>::defer(
    ASIO_MOVE_ARG(Function) f, const OtherAllocator& a) const
{
  typedef typename decay<Function>::type function_type;

  system_context& ctx = detail::global<system_context>();

  // Allocate and construct an operation to wrap the function.
  typedef detail::executor_op<function_type, OtherAllocator> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(ASIO_MOVE_CAST(Function)(f), a);

  ASIO_HANDLER_CREATION((ctx, *p.p,
        "system_executor", &this->context(), 0, "defer"));

  ctx.scheduler_.post_immediate_completion(p.p, true);
  p.v = p.p = 0;
}
#endif // !defined(ASIO_NO_TS_EXECUTORS)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_SYSTEM_EXECUTOR_HPP
