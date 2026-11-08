//
// detail/impl/epoll_reactor.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_EPOLL_REACTOR_HPP
#define ASIO_DETAIL_IMPL_EPOLL_REACTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if defined(ASIO_HAS_EPOLL)

#include "asio/detail/scheduler.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

inline void epoll_reactor::post_immediate_completion(
    operation* op, bool is_continuation) const
{
  scheduler_.post_immediate_completion(op, is_continuation);
}

template <int OpType, reactor_op::perform_func_type Perform>
inline bool epoll_reactor::start_op(socket_type descriptor,
    epoll_reactor::per_descriptor_data& descriptor_data, reactor_op* op)
{
  (void)descriptor;

  if (ASIO_UNLIKELY(!descriptor_data))
    return start_bad_descriptor_op(op);

  descriptor_data->mutex_.lock();
  mutex::scoped_lock descriptor_lock(descriptor_data->mutex_,
      mutex::scoped_lock::adopt_lock);

  if (Perform != nullptr
      && descriptor_data->try_speculative_[OpType]
      && descriptor_data->op_queue_[OpType].empty()
      && (OpType != read_op
        || descriptor_data->op_queue_[except_op].empty()))
  {
    if (reactor_op::status status = Perform(op))
    {
      if (ASIO_UNLIKELY(status == reactor_op::done_and_exhausted))
        record_done_and_exhausted(OpType, descriptor_data);
      descriptor_lock.unlock();
      return true;
    }
  }

  return start_unready_op(OpType, descriptor_data,
      descriptor_lock, op, Perform);
}

template <typename TimeTraits, typename Allocator>
void epoll_reactor::add_timer_queue(
    timer_queue<TimeTraits, Allocator>& queue)
{
  do_add_timer_queue(queue);
}

template <typename TimeTraits, typename Allocator>
void epoll_reactor::remove_timer_queue(
    timer_queue<TimeTraits, Allocator>& queue)
{
  do_remove_timer_queue(queue);
}

template <typename TimeTraits, typename Allocator>
void epoll_reactor::schedule_timer(timer_queue<TimeTraits, Allocator>& queue,
    const typename TimeTraits::time_type& time,
    typename timer_queue<TimeTraits, Allocator>::per_timer_data& timer,
    wait_op* op)
{
  mutex::scoped_lock lock(mutex_);

  if (shutdown_)
  {
    scheduler_.post_immediate_completion(op, false);
    return;
  }

  bool earliest = queue.enqueue_timer(time, timer, op);
  scheduler_.work_started();
  if (earliest)
    update_timeout();
}

template <typename TimeTraits, typename Allocator>
std::size_t epoll_reactor::cancel_timer(
    timer_queue<TimeTraits, Allocator>& queue,
    typename timer_queue<TimeTraits, Allocator>::per_timer_data& timer,
    std::size_t max_cancelled)
{
  mutex::scoped_lock lock(mutex_);
  op_queue<operation> ops;
  std::size_t n = queue.cancel_timer(timer, ops, max_cancelled);
  lock.unlock();
  scheduler_.post_deferred_completions(ops);
  return n;
}

template <typename TimeTraits, typename Allocator>
void epoll_reactor::cancel_timer_by_key(
    timer_queue<TimeTraits, Allocator>& queue,
    typename timer_queue<TimeTraits, Allocator>::per_timer_data* timer,
    void* cancellation_key)
{
  mutex::scoped_lock lock(mutex_);
  op_queue<operation> ops;
  queue.cancel_timer_by_key(timer, ops, cancellation_key);
  lock.unlock();
  scheduler_.post_deferred_completions(ops);
}

template <typename TimeTraits, typename Allocator>
void epoll_reactor::move_timer(timer_queue<TimeTraits, Allocator>& queue,
    typename timer_queue<TimeTraits, Allocator>::per_timer_data& target,
    typename timer_queue<TimeTraits, Allocator>::per_timer_data& source)
{
  mutex::scoped_lock lock(mutex_);
  op_queue<operation> ops;
  queue.cancel_timer(target, ops);
  queue.move_timer(target, source);
  lock.unlock();
  scheduler_.post_deferred_completions(ops);
}

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_EPOLL)

#endif // ASIO_DETAIL_IMPL_EPOLL_REACTOR_HPP
