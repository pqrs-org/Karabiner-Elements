//
// detail/impl/io_uring_service.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_IO_URING_SERVICE_IPP
#define ASIO_DETAIL_IMPL_IO_URING_SERVICE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_IO_URING)

#include <cstddef>
#include <sys/eventfd.h>
#include "asio/detail/cstdint.hpp"
#include "asio/detail/io_uring_service.hpp"
#include "asio/detail/reactor_op.hpp"
#include "asio/detail/scheduler.hpp"
#include "asio/detail/throw_error.hpp"
#include "asio/error.hpp"

#if defined(ASIO_HAS_THREAD_SANITIZER)
# include <sanitizer/tsan_interface.h>
#endif // defined(ASIO_HAS_THREAD_SANITIZER)

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

struct io_uring_service::ring
{
  mutex mutex_;
  ::io_uring ring_;
  submit_sqes_op submit_sqes_op_;
  int pending_sqes_;
  bool pending_submit_sqes_op_;
  bool ready_;

  ring(io_uring_service* owner, std::size_t index, bool locking, int spin_count)
    : mutex_(locking, spin_count),
      submit_sqes_op_(owner, index),
      pending_sqes_(0),
      pending_submit_sqes_op_(false),
      ready_(false)
  {
    ring_.ring_fd = -1;
  }

  ring(ring&& other) noexcept
    : mutex_(other.mutex_.enabled(), other.mutex_.spin_count()),
      submit_sqes_op_(other.submit_sqes_op_.service_,
          other.submit_sqes_op_.ring_index_),
      pending_sqes_(0),
      pending_submit_sqes_op_(false),
      ready_(false)
  {
    ring_.ring_fd = -1;
  }
};

io_uring_service::io_uring_service(asio::execution_context& ctx)
  : execution_context_service_base<io_uring_service>(ctx),
    scheduler_(use_service<scheduler>(ctx)),
    rings_(execution_context::allocator<ring>(ctx)),
    next_ring_(0),
    outstanding_work_(0),
    pollfd_buf_(execution_context::allocator< ::pollfd>(ctx)),
    shutdown_(false),
    io_locking_(config(ctx).get("reactor", "io_locking", true)),
    io_locking_spin_count_(
        config(ctx).get("reactor", "io_locking_spin_count", 0)),
    iowait_(config(ctx).get("reactor", "io_uring_iowait", true)),
    submit_batch_size_(
        config(ctx).get("reactor", "io_uring_submit_batch_size", 128)),
    ring_size_(config(ctx).get("reactor",
          "io_uring_ring_size", static_cast<unsigned int>(default_ring_size))),
    unflushed_submits_(0),
    timeout_(),
    registration_mutex_(
        config(ctx).get("reactor", "registration_locking", true)),
    registered_io_objects_(execution_context::allocator<void>(ctx),
        config(ctx).get("reactor", "preallocated_io_objects", 0U),
        io_locking_, io_locking_spin_count_),
    reactor_(use_service<reactor>(ctx)),
    reactor_data_(),
    event_fd_(-1)
{
  int n = config(ctx).get("reactor", "io_uring_ring_count", 1);
  std::size_t num_rings = static_cast<std::size_t>(n < 1 ? 1 : n);
  bool reg_locking = config(ctx).get("reactor", "registration_locking", true);
  int reg_spin = config(ctx).get(
      "reactor", "registration_locking_spin_count", 0);
  rings_.reserve(num_rings);
  for (std::size_t i = 0; i < num_rings; ++i)
    rings_.emplace_back(this, i, reg_locking, reg_spin);
  pollfd_buf_.resize(rings_.size());
  reactor_.init_task();
  init_ring();
  register_with_reactor();
}

io_uring_service::~io_uring_service()
{
  for (std::size_t i = 0; i < rings_.size(); ++i)
    if (rings_[i].ring_.ring_fd != -1)
      ::io_uring_queue_exit(&rings_[i].ring_);
  if (event_fd_ != -1)
    ::close(event_fd_);
}

void io_uring_service::shutdown()
{
  mutex::scoped_lock lock(rings_[0].mutex_);
  shutdown_ = true;
  lock.unlock();

  op_queue<operation> ops;

  // Cancel all outstanding operations.
  while (io_object* io_obj = registered_io_objects_.first())
  {
    for (int i = 0; i < max_ops; ++i)
    {
      if (!io_obj->queues_[i].op_queue_.empty())
      {
        ops.push(io_obj->queues_[i].op_queue_);
        std::size_t ring_index = io_obj->queues_[i].first_op_ring_index_;
        mutex::scoped_lock cancel_lock(rings_[ring_index].mutex_);
        if (::io_uring_sqe* sqe = get_sqe(ring_index))
          ::io_uring_prep_cancel(sqe, &io_obj->queues_[i], 0);
        submit_sqes(ring_index);
      }
    }
    io_obj->shutdown_ = true;
    registered_io_objects_.free(io_obj);
  }

  // Cancel the timeout operation.
  if (::io_uring_sqe* sqe = get_sqe(0))
    ::io_uring_prep_cancel(sqe, &timeout_, 0);
  submit_sqes(0);

  // Wait for all completions to come back, across all rings.
  for (; outstanding_work_ > 0; --outstanding_work_)
  {
    bool got = false;
    for (std::size_t s = 0; s < rings_.size() && !got; ++s)
    {
      ::io_uring_cqe* cqe = 0;
      if (::io_uring_peek_cqe(&rings_[s].ring_, &cqe) == 0)
      {
        ::io_uring_cqe_seen(&rings_[s].ring_, cqe);
        got = true;
      }
    }
    if (!got)
    {
      ::io_uring_cqe* cqe = 0;
      if (::io_uring_wait_cqe(&rings_[0].ring_, &cqe) != 0)
        break;
      ::io_uring_cqe_seen(&rings_[0].ring_, cqe);
    }
  }

  timer_queues_.get_all_timers(ops);

  scheduler_.abandon_operations(ops);
}

void io_uring_service::notify_fork(
    asio::execution_context::fork_event fork_ev)
{
  switch (fork_ev)
  {
  case asio::execution_context::fork_prepare:
    {
      // Cancel all outstanding operations. They will be restarted after the
      // fork completes. Each queue's in-flight operation may be on any ring
      // and io_uring_prep_cancel() is ring-scoped, so target that same ring.
      mutex::scoped_lock registration_lock(registration_mutex_);
      for (io_object* io_obj = registered_io_objects_.first();
          io_obj != 0; io_obj = io_obj->next_)
      {
        mutex::scoped_lock io_object_lock(io_obj->mutex_);
        for (int i = 0; i < max_ops; ++i)
        {
          if (!io_obj->queues_[i].op_queue_.empty()
              && !io_obj->queues_[i].cancel_requested_)
          {
            std::size_t ring_index = io_obj->queues_[i].first_op_ring_index_;
            mutex::scoped_lock lock(rings_[ring_index].mutex_);
            if (::io_uring_sqe* sqe = get_sqe(ring_index))
              ::io_uring_prep_cancel(sqe, &io_obj->queues_[i], 0);
            submit_sqes(ring_index);
          }
        }
      }

      // Cancel the timeout operation.
      {
        mutex::scoped_lock lock(rings_[0].mutex_);
        if (::io_uring_sqe* sqe = get_sqe(0))
          ::io_uring_prep_cancel(sqe, &timeout_, 0);
        submit_sqes(0);
      }

      // Wait for all completions to come back, and post all completed I/O
      // queues to the scheduler. Note that some operations may have already
      // completed, or were explicitly cancelled. All others will be
      // automatically restarted.
      op_queue<operation> ops;
      const std::size_t n = rings_.size();
      for (; outstanding_work_ > 0; --outstanding_work_)
      {
        std::size_t s = 0;
        ::io_uring_cqe* cqe = 0;
        for (; s < n; ++s)
          if (::io_uring_peek_cqe(&rings_[s].ring_, &cqe) == 0)
            break;
        if (s == n)
        {
          s = 0;
          if (::io_uring_wait_cqe(&rings_[0].ring_, &cqe) != 0)
            break;
        }
        if (void* ptr = ::io_uring_cqe_get_data(cqe))
        {
#if defined(ASIO_HAS_THREAD_SANITIZER)
          __tsan_acquire(ptr);
#endif // defined(ASIO_HAS_THREAD_SANITIZER)
          if (ptr != this && ptr != &timer_queues_ && ptr != &timeout_)
          {
            io_queue* io_q = static_cast<io_queue*>(ptr);
            io_q->set_result(cqe->res);
            ops.push(io_q);
          }
        }
        ::io_uring_cqe_seen(&rings_[s].ring_, cqe);
      }
      scheduler_.post_deferred_completions(ops);

      // Restart and eventfd operation.
      register_with_reactor();
    }
    break;

  case asio::execution_context::fork_parent:
    // Restart the timeout and eventfd operations.
    update_timeout();
    register_with_reactor();
    break;

  case asio::execution_context::fork_child:
    {
      // The child process gets a new io_uring instance.
      for (std::size_t i = 0; i < rings_.size(); ++i)
        ::io_uring_queue_exit(&rings_[i].ring_);
      init_ring();
      register_with_reactor();
    }
    break;
  default:
    break;
  }
}

void io_uring_service::init_task()
{
  scheduler_.init_task();
}

void io_uring_service::register_io_object(
    io_uring_service::per_io_object_data& io_obj)
{
  io_obj = allocate_io_object();

  mutex::scoped_lock io_object_lock(io_obj->mutex_);

  io_obj->service_ = this;
  io_obj->shutdown_ = false;
  for (int i = 0; i < max_ops; ++i)
  {
    io_obj->queues_[i].io_object_ = io_obj;
    io_obj->queues_[i].cancel_requested_ = false;
    io_obj->queues_[i].first_op_ring_index_ = 0;
  }
}

void io_uring_service::register_internal_io_object(
    io_uring_service::per_io_object_data& io_obj,
    int op_type, io_uring_operation* op)
{
  io_obj = allocate_io_object();

  mutex::scoped_lock io_object_lock(io_obj->mutex_);

  io_obj->service_ = this;
  io_obj->shutdown_ = false;
  for (int i = 0; i < max_ops; ++i)
  {
    io_obj->queues_[i].io_object_ = io_obj;
    io_obj->queues_[i].cancel_requested_ = false;
    io_obj->queues_[i].first_op_ring_index_ = 0;
  }

  io_obj->queues_[op_type].op_queue_.push(op);
  io_object_lock.unlock();
  mutex::scoped_lock lock(rings_[0].mutex_);
  if (::io_uring_sqe* sqe = get_sqe(0))
  {
    op->prepare(sqe);
    ::io_uring_sqe_set_data(sqe, &io_obj->queues_[op_type]);
#if defined(ASIO_HAS_THREAD_SANITIZER)
    __tsan_release(&io_obj->queues_[op_type]);
#endif // defined(ASIO_HAS_THREAD_SANITIZER)
    post_submit_sqes_op(lock, 0);
  }
  else
  {
    asio::error_code ec(ENOBUFS,
        asio::error::get_system_category());
    asio::detail::throw_error(ec, "io_uring_get_sqe");
  }
}

void io_uring_service::register_buffers(const ::iovec* v, unsigned n)
{
  for (std::size_t i = 0; i < rings_.size(); ++i)
  {
    int result = ::io_uring_register_buffers(&rings_[i].ring_, v, n);
    if (result < 0)
    {
      asio::error_code ec(-result,
          asio::error::get_system_category());
      asio::detail::throw_error(ec, "io_uring_register_buffers");
    }
  }
}

void io_uring_service::unregister_buffers()
{
  for (std::size_t i = 0; i < rings_.size(); ++i)
    (void)::io_uring_unregister_buffers(&rings_[i].ring_);
}

void io_uring_service::start_op(int op_type,
    io_uring_service::per_io_object_data& io_obj,
    io_uring_operation* op, bool is_continuation)
{
  if (!io_obj)
  {
    op->ec_ = asio::error::bad_descriptor;
    post_immediate_completion(op, is_continuation);
    return;
  }

  mutex::scoped_lock io_object_lock(io_obj->mutex_);

  if (io_obj->shutdown_)
  {
    io_object_lock.unlock();
    post_immediate_completion(op, is_continuation);
    return;
  }

  if (io_obj->queues_[op_type].op_queue_.empty())
  {
    if (op->perform(false))
    {
      io_object_lock.unlock();
      scheduler_.post_immediate_completion(op, is_continuation);
    }
    else
    {
      std::size_t ring_index = current_ring_index();
      io_obj->queues_[op_type].op_queue_.push(op);
      io_obj->queues_[op_type].first_op_ring_index_ = ring_index;
      io_object_lock.unlock();
      mutex::scoped_lock lock(rings_[ring_index].mutex_);
      if (::io_uring_sqe* sqe = get_sqe(ring_index))
      {
        op->prepare(sqe);
        ::io_uring_sqe_set_data(sqe, &io_obj->queues_[op_type]);
#if defined(ASIO_HAS_THREAD_SANITIZER)
        __tsan_release(&io_obj->queues_[op_type]);
#endif // defined(ASIO_HAS_THREAD_SANITIZER)
        scheduler_.work_started();
        post_submit_sqes_op(lock, ring_index);
      }
      else
      {
        lock.unlock();
        io_obj->queues_[op_type].set_result(-ENOBUFS);
        post_immediate_completion(&io_obj->queues_[op_type], is_continuation);
      }
    }
  }
  else
  {
    io_obj->queues_[op_type].op_queue_.push(op);
    scheduler_.work_started();
  }
}

void io_uring_service::cancel_ops(io_uring_service::per_io_object_data& io_obj)
{
  if (!io_obj)
    return;

  mutex::scoped_lock io_object_lock(io_obj->mutex_);
  op_queue<operation> ops;
  do_cancel_ops(io_obj, ops);
  io_object_lock.unlock();
  scheduler_.post_deferred_completions(ops);
}

void io_uring_service::cancel_ops_by_key(
    io_uring_service::per_io_object_data& io_obj,
    int op_type, void* cancellation_key)
{
  if (!io_obj)
    return;

  mutex::scoped_lock io_object_lock(io_obj->mutex_);

  bool first = true;
  op_queue<operation> ops;
  op_queue<io_uring_operation> other_ops;
  while (io_uring_operation* op = io_obj->queues_[op_type].op_queue_.front())
  {
    io_obj->queues_[op_type].op_queue_.pop();
    if (op->cancellation_key_ == cancellation_key)
    {
      if (first)
      {
        other_ops.push(op);
        if (!io_obj->queues_[op_type].cancel_requested_)
        {
          io_obj->queues_[op_type].cancel_requested_ = true;
          std::size_t ring_index =
            io_obj->queues_[op_type].first_op_ring_index_;
          mutex::scoped_lock lock(rings_[ring_index].mutex_);
          if (::io_uring_sqe* sqe = get_sqe(ring_index))
          {
            ::io_uring_prep_cancel(sqe, &io_obj->queues_[op_type], 0);
            submit_sqes(ring_index);
          }
        }
      }
      else
      {
        op->ec_ = asio::error::operation_aborted;
        ops.push(op);
      }
    }
    else
      other_ops.push(op);
    first = false;
  }
  io_obj->queues_[op_type].op_queue_.push(other_ops);

  io_object_lock.unlock();

  scheduler_.post_deferred_completions(ops);
}

void io_uring_service::deregister_io_object(
    io_uring_service::per_io_object_data& io_obj)
{
  if (!io_obj)
    return;

  mutex::scoped_lock io_object_lock(io_obj->mutex_);
  if (!io_obj->shutdown_)
  {
    op_queue<operation> ops;
    bool pending_cancelled_ops = do_cancel_ops(io_obj, ops);
    io_obj->shutdown_ = true;
    io_object_lock.unlock();
    scheduler_.post_deferred_completions(ops);
    if (pending_cancelled_ops)
    {
      // There are still pending operations. Prevent cleanup_io_object from
      // freeing the I/O object and let the last operation to complete free it.
      io_obj = 0;
    }
    else
    {
      // Leave io_obj set so that it will be freed by the subsequent call to
      // cleanup_io_object.
    }
  }
  else
  {
    // We are shutting down, so prevent cleanup_io_object from freeing
    // the I/O object and let the destructor free it instead.
    io_obj = 0;
  }
}

void io_uring_service::cleanup_io_object(
    io_uring_service::per_io_object_data& io_obj)
{
  if (io_obj)
  {
    free_io_object(io_obj);
    io_obj = 0;
  }
}

void io_uring_service::run(long usec, op_queue<operation>& ops)
{
  if (usec != 0 && unflushed_submits_ != 0)
    usec = 0;
  if (rings_.size() == 1)
    run_single_ring(usec, ops);
  else
    run_multi_ring(usec, ops);
}

void io_uring_service::run_single_ring(long usec, op_queue<operation>& ops)
{
  __kernel_timespec ts;
  int local_ops = 0;
  ::io_uring* ring = &rings_[0].ring_;

  if (usec > 0)
  {
    ts.tv_sec = usec / 1000000;
    ts.tv_nsec = (usec % 1000000) * 1000;
    mutex::scoped_lock lock(rings_[0].mutex_);
    if (::io_uring_sqe* sqe = get_sqe(0))
    {
      ++local_ops;
      ::io_uring_prep_timeout(sqe, &ts, 0, 0);
      ::io_uring_sqe_set_data(sqe, &ts);
#if defined(ASIO_HAS_THREAD_SANITIZER)
      __tsan_release(&ts);
#endif // defined(ASIO_HAS_THREAD_SANITIZER)
      submit_sqes(0);
    }
  }

  ::io_uring_cqe* cqe = 0;
  int result = (usec == 0)
    ? ::io_uring_peek_cqe(ring, &cqe)
    : ::io_uring_wait_cqe(ring, &cqe);

  if (local_ops > 0)
  {
    if (result != 0 || ::io_uring_cqe_get_data(cqe) != &ts)
    {
      mutex::scoped_lock lock(rings_[0].mutex_);
      if (::io_uring_sqe* sqe = get_sqe(0))
      {
        ++local_ops;
        ::io_uring_prep_timeout_remove(sqe, reinterpret_cast<__u64>(&ts), 0);
        ::io_uring_sqe_set_data(sqe, &ts);
#if defined(ASIO_HAS_THREAD_SANITIZER)
        __tsan_release(&ts);
#endif // defined(ASIO_HAS_THREAD_SANITIZER)
        submit_sqes(0);
      }
    }
  }

  bool check_timers = false;
  int count = 0;
  while (result == 0 || local_ops > 0)
  {
    if (result == 0)
    {
      if (void* ptr = ::io_uring_cqe_get_data(cqe))
      {
#if defined(ASIO_HAS_THREAD_SANITIZER)
        __tsan_acquire(ptr);
#endif // defined(ASIO_HAS_THREAD_SANITIZER)
        if (ptr == this)
        {
          // The io_uring service was interrupted.
        }
        else if (ptr == &timer_queues_)
        {
          check_timers = true;
        }
        else if (ptr == &timeout_)
        {
          check_timers = true;
          timeout_.tv_sec = 0;
          timeout_.tv_nsec = 0;
        }
        else if (ptr == &ts)
        {
          --local_ops;
        }
        else
        {
          io_queue* io_q = static_cast<io_queue*>(ptr);
          io_q->set_result(cqe->res);
          ops.push(io_q);
        }
      }
      ::io_uring_cqe_seen(ring, cqe);
      ++count;
    }
    result = (count < complete_batch_size || local_ops > 0)
      ? ::io_uring_peek_cqe(ring, &cqe) : -EAGAIN;
  }

  decrement(outstanding_work_, count);

  if (check_timers)
  {
    mutex::scoped_lock lock(rings_[0].mutex_);
    timer_queues_.get_ready_timers(ops);
    if (timeout_.tv_sec == 0 && timeout_.tv_nsec == 0)
    {
      timeout_ = get_timeout();
      if (::io_uring_sqe* sqe = get_sqe(0))
      {
        ::io_uring_prep_timeout(sqe, &timeout_, 0, 0);
        ::io_uring_sqe_set_data(sqe, &timeout_);
#if defined(ASIO_HAS_THREAD_SANITIZER)
        __tsan_release(&timeout_);
#endif // defined(ASIO_HAS_THREAD_SANITIZER)
        push_submit_sqes_op(ops, 0);
      }
    }
  }
}

void io_uring_service::run_multi_ring(long usec, op_queue<operation>& ops)
{
  __kernel_timespec ts;
  int local_ops = 0;

  if (usec > 0)
  {
    ts.tv_sec = usec / 1000000;
    ts.tv_nsec = (usec % 1000000) * 1000;
    mutex::scoped_lock lock(rings_[0].mutex_);
    if (::io_uring_sqe* sqe = get_sqe(0))
    {
      ++local_ops;
      ::io_uring_prep_timeout(sqe, &ts, 0, 0);
      ::io_uring_sqe_set_data(sqe, &ts);
#if defined(ASIO_HAS_THREAD_SANITIZER)
      __tsan_release(&ts);
#endif // defined(ASIO_HAS_THREAD_SANITIZER)
      submit_sqes(0);
    }
  }

  // rings_[s].ready_ records whether ring s had a completion ready the
  // last time it was checked below.
  if (usec == 0)
  {
    for (std::size_t s = 0; s < rings_.size(); ++s)
      rings_[s].ready_ = true;
  }
  else
  {
    ::pollfd* pfds = pollfd_buf_.data();
    for (std::size_t s = 0; s < rings_.size(); ++s)
    {
      pfds[s].fd = rings_[s].ring_.ring_fd;
      pfds[s].events = POLLIN;
      pfds[s].revents = 0;
    }
    int pres;
    do
    {
      pres = ::poll(pfds, rings_.size(), -1);
    } while (pres < 0 && errno == EINTR);
    for (std::size_t s = 0; s < rings_.size(); ++s)
      rings_[s].ready_ = pres > 0 && (pfds[s].revents & POLLIN) != 0;
  }

  bool check_timers = false;
  int count = 0;
  for (std::size_t s = 0; s < rings_.size(); ++s)
  {
    if (!rings_[s].ready_)
      continue;

    ::io_uring* ring = &rings_[s].ring_;
    ::io_uring_cqe* cqe = 0;
    int result = ::io_uring_peek_cqe(ring, &cqe);

    if (s == 0 && local_ops > 0)
    {
      if (result != 0 || ::io_uring_cqe_get_data(cqe) != &ts)
      {
        mutex::scoped_lock lock(rings_[0].mutex_);
        if (::io_uring_sqe* sqe = get_sqe(0))
        {
          ++local_ops;
          ::io_uring_prep_timeout_remove(
              sqe, reinterpret_cast<__u64>(&ts), 0);
          ::io_uring_sqe_set_data(sqe, &ts);
#if defined(ASIO_HAS_THREAD_SANITIZER)
          __tsan_release(&ts);
#endif // defined(ASIO_HAS_THREAD_SANITIZER)
          submit_sqes(0);
        }
      }
    }

    int ring_count = 0;
    while (result == 0 || (s == 0 && local_ops > 0))
    {
      if (result == 0)
      {
        if (void* ptr = ::io_uring_cqe_get_data(cqe))
        {
#if defined(ASIO_HAS_THREAD_SANITIZER)
          __tsan_acquire(ptr);
#endif // defined(ASIO_HAS_THREAD_SANITIZER)
          if (ptr == this)
          {
            // The io_uring service was interrupted.
          }
          else if (ptr == &timer_queues_)
          {
            check_timers = true;
          }
          else if (ptr == &timeout_)
          {
            check_timers = true;
            timeout_.tv_sec = 0;
            timeout_.tv_nsec = 0;
          }
          else if (ptr == &ts)
          {
            --local_ops;
          }
          else
          {
            io_queue* io_q = static_cast<io_queue*>(ptr);
            io_q->set_result(cqe->res);
            ops.push(io_q);
          }
        }
        ::io_uring_cqe_seen(ring, cqe);
        ++ring_count;
      }
      result = (ring_count < complete_batch_size || (s == 0 && local_ops > 0))
        ? ::io_uring_peek_cqe(ring, &cqe) : -EAGAIN;
    }
    count += ring_count;
  }

  decrement(outstanding_work_, count);

  if (check_timers)
  {
    mutex::scoped_lock lock(rings_[0].mutex_);
    timer_queues_.get_ready_timers(ops);
    if (timeout_.tv_sec == 0 && timeout_.tv_nsec == 0)
    {
      timeout_ = get_timeout();
      if (::io_uring_sqe* sqe = get_sqe(0))
      {
        ::io_uring_prep_timeout(sqe, &timeout_, 0, 0);
        ::io_uring_sqe_set_data(sqe, &timeout_);
#if defined(ASIO_HAS_THREAD_SANITIZER)
        __tsan_release(&timeout_);
#endif // defined(ASIO_HAS_THREAD_SANITIZER)
        push_submit_sqes_op(ops, 0);
      }
    }
  }
}

void io_uring_service::interrupt()
{
  mutex::scoped_lock lock(rings_[0].mutex_);
  if (::io_uring_sqe* sqe = get_sqe(0))
  {
    ::io_uring_prep_nop(sqe);
    ::io_uring_sqe_set_data(sqe, this);
#if defined(ASIO_HAS_THREAD_SANITIZER)
    __tsan_release(this);
#endif // defined(ASIO_HAS_THREAD_SANITIZER)
  }
  submit_sqes(0);
}

void io_uring_service::init_ring()
{
  const unsigned int entries = ring_size_ != 0
    ? ring_size_ : static_cast<unsigned int>(default_ring_size);
  for (std::size_t i = 0; i < rings_.size(); ++i)
  {
    // All rings share the same kernel worker pool.
    ::io_uring_params params = {};
    if (i > 0)
    {
      params.flags = IORING_SETUP_ATTACH_WQ;
      params.wq_fd = static_cast<__u32>(rings_[0].ring_.ring_fd);
    }

    int result =
      ::io_uring_queue_init_params(entries, &rings_[i].ring_, &params);
    if (result < 0)
    {
      rings_[i].ring_.ring_fd = -1;
      for (std::size_t j = 0; j < i; ++j)
        ::io_uring_queue_exit(&rings_[j].ring_);
      asio::error_code ec(-result,
          asio::error::get_system_category());
      asio::detail::throw_error(ec, "io_uring_queue_init");
    }

#if defined(IORING_FEAT_NO_IOWAIT)
    if (!iowait_)
      static_cast<void>(::io_uring_set_iowait(&rings_[i].ring_, false));
#endif // defined(IORING_FEAT_NO_IOWAIT)
  }

#if !defined(ASIO_HAS_IO_URING_AS_DEFAULT)
  event_fd_ = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (event_fd_ < 0)
  {
    asio::error_code ec(errno,
        asio::error::get_system_category());
    for (std::size_t i = 0; i < rings_.size(); ++i)
      ::io_uring_queue_exit(&rings_[i].ring_);
    asio::detail::throw_error(ec, "eventfd");
  }

  int result = ::io_uring_register_eventfd(&rings_[0].ring_, event_fd_);
  if (result < 0)
  {
    ::close(event_fd_);
    for (std::size_t i = 0; i < rings_.size(); ++i)
      ::io_uring_queue_exit(&rings_[i].ring_);
    asio::error_code ec(-result,
        asio::error::get_system_category());
    asio::detail::throw_error(ec, "io_uring_queue_init");
  }
#endif // !defined(ASIO_HAS_IO_URING_AS_DEFAULT)
}

#if !defined(ASIO_HAS_IO_URING_AS_DEFAULT)
class io_uring_service::event_fd_read_op :
  public reactor_op
{
public:
  event_fd_read_op(io_uring_service* s)
    : reactor_op(asio::error_code(),
        &event_fd_read_op::do_perform, event_fd_read_op::do_complete),
      service_(s)
  {
  }

  static status do_perform(reactor_op* base)
  {
    event_fd_read_op* o(static_cast<event_fd_read_op*>(base));

    for (;;)
    {
      // Only perform one read. The kernel maintains an atomic counter.
      uint64_t counter(0);
      errno = 0;
      int bytes_read = ::read(o->service_->event_fd_,
          &counter, sizeof(uint64_t));
      if (bytes_read < 0 && errno == EINTR)
        continue;
      break;
    }

    op_queue<operation> ops;
    o->service_->run(0, ops);
    o->service_->scheduler_.post_deferred_completions(ops);

    return not_done;
  }

  static void do_complete(void* /*owner*/, operation* base,
      const asio::error_code& /*ec*/,
      std::size_t /*bytes_transferred*/)
  {
    event_fd_read_op* o(static_cast<event_fd_read_op*>(base));
    delete o;
  }

private:
  io_uring_service* service_;
};
#endif // !defined(ASIO_HAS_IO_URING_AS_DEFAULT)

void io_uring_service::register_with_reactor()
{
#if !defined(ASIO_HAS_IO_URING_AS_DEFAULT)
  reactor_.register_internal_descriptor(reactor::read_op,
      event_fd_, reactor_data_, new event_fd_read_op(this));
#endif // !defined(ASIO_HAS_IO_URING_AS_DEFAULT)
}

io_uring_service::io_object* io_uring_service::allocate_io_object()
{
  mutex::scoped_lock registration_lock(registration_mutex_);
  return registered_io_objects_.alloc(io_locking_, io_locking_spin_count_);
}

void io_uring_service::free_io_object(io_uring_service::io_object* io_obj)
{
  mutex::scoped_lock registration_lock(registration_mutex_);
  registered_io_objects_.free(io_obj);
}

bool io_uring_service::do_cancel_ops(
    per_io_object_data& io_obj, op_queue<operation>& ops)
{
  bool cancel_op = false;

  for (int i = 0; i < max_ops; ++i)
  {
    if (io_uring_operation* first_op = io_obj->queues_[i].op_queue_.front())
    {
      cancel_op = true;
      io_obj->queues_[i].op_queue_.pop();
      while (io_uring_operation* op = io_obj->queues_[i].op_queue_.front())
      {
        op->ec_ = asio::error::operation_aborted;
        io_obj->queues_[i].op_queue_.pop();
        ops.push(op);
      }
      io_obj->queues_[i].op_queue_.push(first_op);
    }
  }

  if (cancel_op)
  {
    for (int i = 0; i < max_ops; ++i)
    {
      if (!io_obj->queues_[i].op_queue_.empty()
          && !io_obj->queues_[i].cancel_requested_)
      {
        io_obj->queues_[i].cancel_requested_ = true;
        std::size_t ring_index = io_obj->queues_[i].first_op_ring_index_;
        mutex::scoped_lock lock(rings_[ring_index].mutex_);
        if (::io_uring_sqe* sqe = get_sqe(ring_index))
          ::io_uring_prep_cancel(sqe, &io_obj->queues_[i], 0);
        submit_sqes(ring_index);
      }
    }
  }

  return cancel_op;
}

void io_uring_service::do_add_timer_queue(timer_queue_base& queue)
{
  mutex::scoped_lock lock(rings_[0].mutex_);
  timer_queues_.insert(&queue);
}

void io_uring_service::do_remove_timer_queue(timer_queue_base& queue)
{
  mutex::scoped_lock lock(rings_[0].mutex_);
  timer_queues_.erase(&queue);
}

void io_uring_service::update_timeout()
{
  // Timers always live on ring 0.
  if (::io_uring_sqe* sqe = get_sqe(0))
  {
    ::io_uring_prep_timeout_remove(sqe, reinterpret_cast<__u64>(&timeout_), 0);
    ::io_uring_sqe_set_data(sqe, &timer_queues_);
#if defined(ASIO_HAS_THREAD_SANITIZER)
    __tsan_release(&timer_queues_);
#endif // defined(ASIO_HAS_THREAD_SANITIZER)
  }
}

__kernel_timespec io_uring_service::get_timeout() const
{
  __kernel_timespec ts;
  long usec = timer_queues_.wait_duration_usec(5 * 60 * 1000 * 1000);
  ts.tv_sec = usec / 1000000;
  ts.tv_nsec = usec ? (usec % 1000000) * 1000 : 1;
  return ts;
}

std::size_t io_uring_service::current_ring_index()
{
  // tss_ring_index_ is one slot shared by every io_uring_service instance
  // in the process, so what's cached is a raw, unmodded per-thread index,
  // not a ring index for any particular instance - the modulo by
  // rings_.size() is applied fresh on every call so it's always in bounds
  // for this instance, regardless of which instance assigned the raw value.
  uintptr_t raw;
  if (void* cached = tss_ring_index_)
    raw = reinterpret_cast<uintptr_t>(cached) - 1;
  else
  {
    raw = next_ring_.fetch_add(1, std::memory_order_relaxed);
    tss_ring_index_ = reinterpret_cast<void*>(raw + 1);
  }
  return static_cast<std::size_t>(raw % rings_.size());
}

io_uring_service::mutex& io_uring_service::timer_mutex()
{
  return rings_[0].mutex_;
}

::io_uring_sqe* io_uring_service::get_sqe(std::size_t ring_index)
{
  ::io_uring* ring = &rings_[ring_index].ring_;
  ::io_uring_sqe* sqe = ::io_uring_get_sqe(ring);
  if (!sqe)
  {
    submit_sqes(ring_index);
    sqe = ::io_uring_get_sqe(ring);
  }
  if (sqe)
  {
    ::io_uring_sqe_set_data(sqe, 0);
    ++rings_[ring_index].pending_sqes_;
  }
  return sqe;
}

void io_uring_service::submit_sqes(std::size_t ring_index)
{
  ring& r = rings_[ring_index];
  if (r.pending_sqes_ != 0)
  {
    int result = ::io_uring_submit(&r.ring_);
    if (result > 0)
    {
      r.pending_sqes_ -= result;
      increment(outstanding_work_, result);
    }
  }
}

void io_uring_service::post_submit_sqes_op(
    mutex::scoped_lock& lock, std::size_t ring_index)
{
  ring& r = rings_[ring_index];
  if (r.pending_sqes_ >= submit_batch_size_)
  {
    submit_sqes(ring_index);
  }
  else if (r.pending_sqes_ != 0 && !r.pending_submit_sqes_op_)
  {
    r.pending_submit_sqes_op_ = true;
    increment(unflushed_submits_, 1);
    lock.unlock();
    scheduler_.post_immediate_completion(&r.submit_sqes_op_, false);
  }
}

void io_uring_service::push_submit_sqes_op(
    op_queue<operation>& ops, std::size_t ring_index)
{
  ring& r = rings_[ring_index];
  if (r.pending_sqes_ != 0 && !r.pending_submit_sqes_op_)
  {
    r.pending_submit_sqes_op_ = true;
    increment(unflushed_submits_, 1);
    ops.push(&r.submit_sqes_op_);
    scheduler_.compensating_work_started();
  }
}

io_uring_service::submit_sqes_op::submit_sqes_op(
    io_uring_service* s, std::size_t ring_index)
  : operation(&io_uring_service::submit_sqes_op::do_complete),
    service_(s),
    ring_index_(ring_index)
{
}

void io_uring_service::submit_sqes_op::do_complete(void* owner, operation* base,
    const asio::error_code& /*ec*/, std::size_t /*bytes_transferred*/)
{
  if (owner)
  {
    submit_sqes_op* o = static_cast<submit_sqes_op*>(base);
    ring& r = o->service_->rings_[o->ring_index_];
    mutex::scoped_lock lock(r.mutex_);
    o->service_->submit_sqes(o->ring_index_);
    if (r.pending_sqes_ != 0)
    {
      o->service_->scheduler_.post_immediate_completion(o, true);
    }
    else
    {
      r.pending_submit_sqes_op_ = false;
      decrement(o->service_->unflushed_submits_, 1);
    }
  }
}

io_uring_service::io_queue::io_queue()
  : operation(&io_uring_service::io_queue::do_complete)
{
}

struct io_uring_service::perform_io_cleanup_on_block_exit
{
  explicit perform_io_cleanup_on_block_exit(io_uring_service* s)
    : service_(s), io_object_to_free_(0), first_op_(0)
  {
  }

  ~perform_io_cleanup_on_block_exit()
  {
    if (io_object_to_free_)
    {
      mutex::scoped_lock lock(service_->rings_[0].mutex_);
      service_->free_io_object(io_object_to_free_);
    }

    if (first_op_)
    {
      // Post the remaining completed operations for invocation.
      if (!ops_.empty())
        service_->scheduler_.post_deferred_completions(ops_);

      // A user-initiated operation has completed, but there's no need to
      // explicitly call work_finished() here. Instead, we'll take advantage of
      // the fact that the scheduler will call work_finished() once we return.
    }
    else
    {
      // No user-initiated operations have completed, so we need to compensate
      // for the work_finished() call that the scheduler will make once this
      // operation returns.
      service_->scheduler_.compensating_work_started();
    }
  }

  io_uring_service* service_;
  io_object* io_object_to_free_;
  op_queue<operation> ops_;
  operation* first_op_;
};

operation* io_uring_service::io_queue::perform_io(int result)
{
  perform_io_cleanup_on_block_exit io_cleanup(io_object_->service_);
  mutex::scoped_lock io_object_lock(io_object_->mutex_);

  if (result != -ECANCELED || cancel_requested_)
  {
    if (io_uring_operation* op = op_queue_.front())
    {
      if (result < 0)
      {
        op->ec_.assign(-result, asio::error::get_system_category());
        op->bytes_transferred_ = 0;
      }
      else
      {
        op->ec_.assign(0, op->ec_.category());
        op->bytes_transferred_ = static_cast<std::size_t>(result);
      }
    }

    while (io_uring_operation* op = op_queue_.front())
    {
      if (op->perform(io_cleanup.ops_.empty()))
      {
        op_queue_.pop();
        io_cleanup.ops_.push(op);
      }
      else
        break;
    }
  }

  cancel_requested_ = false;

  if (!op_queue_.empty())
  {
    io_uring_service* service = io_object_->service_;
    std::size_t ring_index = service->current_ring_index();
    first_op_ring_index_ = ring_index;
    mutex::scoped_lock lock(service->rings_[ring_index].mutex_);
    if (::io_uring_sqe* sqe = service->get_sqe(ring_index))
    {
      op_queue_.front()->prepare(sqe);
      ::io_uring_sqe_set_data(sqe, this);
#if defined(ASIO_HAS_THREAD_SANITIZER)
      __tsan_release(this);
#endif // defined(ASIO_HAS_THREAD_SANITIZER)
      service->post_submit_sqes_op(lock, ring_index);
    }
    else
    {
      lock.unlock();
      while (io_uring_operation* op = op_queue_.front())
      {
        op->ec_ = asio::error::no_buffer_space;
        op_queue_.pop();
        io_cleanup.ops_.push(op);
      }
    }
  }

  // The last operation to complete on a shut down object must free it.
  if (io_object_->shutdown_)
  {
    io_cleanup.io_object_to_free_ = io_object_;
    for (int i = 0; i < max_ops; ++i)
      if (!io_object_->queues_[i].op_queue_.empty())
        io_cleanup.io_object_to_free_ = 0;
  }

  // The first operation will be returned for completion now. The others will
  // be posted for later by the io_cleanup object's destructor.
  io_cleanup.first_op_ = io_cleanup.ops_.front();
  io_cleanup.ops_.pop();
  return io_cleanup.first_op_;
}

void io_uring_service::io_queue::do_complete(void* owner, operation* base,
    const asio::error_code& ec, std::size_t bytes_transferred)
{
  if (owner)
  {
    io_queue* io_q = static_cast<io_queue*>(base);
    int result = static_cast<int>(bytes_transferred);
    if (operation* op = io_q->perform_io(result))
    {
      op->complete(owner, ec, 0);
    }
  }
}

io_uring_service::io_object::io_object(bool locking, int spin_count)
  : mutex_(locking, spin_count)
{
}

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_IO_URING)

#endif // ASIO_DETAIL_IMPL_IO_URING_SERVICE_IPP
