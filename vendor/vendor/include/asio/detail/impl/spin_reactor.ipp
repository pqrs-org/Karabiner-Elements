//
// detail/impl/spin_reactor.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_SPIN_REACTOR_IPP
#define ASIO_DETAIL_IMPL_SPIN_REACTOR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_ENABLE_SPIN_REACTOR) \
  && !defined(ASIO_HAS_IOCP) \
  && !defined(ASIO_WINDOWS_RUNTIME) \
  && !defined(ASIO_HAS_IO_URING_AS_DEFAULT)

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include "asio/config.hpp"
#include "asio/detail/assert.hpp"
#include "asio/detail/io_control.hpp"
#include "asio/detail/scheduler.hpp"
#include "asio/detail/spin_reactor.hpp"
#include "asio/detail/throw_exception.hpp"
#include "asio/error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

struct spin_reactor::perform_io_cleanup_on_block_exit
{
  explicit perform_io_cleanup_on_block_exit(spin_reactor* r)
    : reactor_(r), first_op_(0)
  {
  }

  ~perform_io_cleanup_on_block_exit()
  {
    if (first_op_)
    {
      // Post the remaining completed operations for invocation.
      if (!ops_.empty())
        reactor_->scheduler_.post_deferred_completions(ops_);

      // A user-initiated operation has completed, but there's no need to
      // explicitly call work_finished() here. Instead, we'll take advantage of
      // the fact that the scheduler will call work_finished() once we return.
    }
    else
    {
      // No user-initiated operations have completed, so we need to compensate
      // for the work_finished() call that the scheduler will make once this
      // operation returns.
      reactor_->scheduler_.compensating_work_started();
    }
  }

  spin_reactor* reactor_;
  op_queue<operation> ops_;
  operation* first_op_;
};

bool spin_reactor::spin_order_is_newest(asio::execution_context& ctx)
{
  char buf[16];
  const char* order = config(ctx).get("reactor", "spin_order", "oldest", buf);

  if (std::strcmp(order, "oldest") == 0)
    return false;
  if (std::strcmp(order, "newest") == 0)
    return true;

  asio::detail::throw_exception(
      std::invalid_argument("unknown reactor spin order"));
  return false;
}

std::size_t spin_reactor::positive_config(
    asio::execution_context& ctx, const char* key)
{
  const int value = config(ctx).get("reactor", key, 1);

  if (value < 1)
  {
    asio::detail::throw_exception(
        std::invalid_argument(
          "reactor scheduling interval must be at least 1"));
  }

  return static_cast<std::size_t>(value);
}

int spin_reactor::non_negative_config(
    asio::execution_context& ctx, const char* key)
{
  const int value = config(ctx).get("reactor", key, 0);

  if (value < 0)
  {
    asio::detail::throw_exception(
        std::invalid_argument(
          "reactor scheduling offset must not be negative"));
  }

  return value;
}

std::size_t spin_reactor::due_after(std::size_t sequence,
    std::size_t interval, int offset)
{
  const std::size_t wanted = static_cast<std::size_t>(offset) % interval;
  const std::size_t have = sequence % interval;
  return sequence
    + (wanted >= have ? wanted - have : interval - have + wanted);
}

spin_reactor::entry::entry(operation::func_type complete_func, bool is_timer)
  : operation(complete_func),
    next_due_(0),
    interval_(1),
    priority_(0),
    seq_(0),
    is_timer_(is_timer)
{
}

spin_reactor::queue_op::queue_op()
  : entry(&spin_reactor::queue_op::do_complete, false),
    descriptor_state_(0),
    op_type_(0)
{
}

spin_reactor::timer_op::timer_op()
  : entry(&spin_reactor::timer_op::do_complete, true),
    reactor_(0)
{
}

bool spin_reactor::precedes(entry* a, entry* b, bool newest_first)
{
  if (a->priority_ != b->priority_)
    return a->priority_ > b->priority_;

  // At the same priority the timers come after every descriptor.
  if (a->is_timer_ != b->is_timer_)
    return b->is_timer_;
  if (a->is_timer_)
    return false;

  const std::size_t a_seq = a->seq_;
  const std::size_t b_seq = b->seq_;
  if (a_seq != b_seq)
    return newest_first ? a_seq > b_seq : a_seq < b_seq;

  // Exception operations must be processed first to ensure that any
  // out-of-band data is read before normal data.
  return static_cast<queue_op*>(a)->op_type_
    > static_cast<queue_op*>(b)->op_type_;
}

operation* spin_reactor::timer_op::perform_io()
{
  perform_io_cleanup_on_block_exit io_cleanup(reactor_);

  mutex::scoped_lock lock(reactor_->mutex_);
  reactor_->timer_queues_.get_ready_timers(io_cleanup.ops_);
  lock.unlock();

  io_cleanup.first_op_ = io_cleanup.ops_.front();
  io_cleanup.ops_.pop();
  return io_cleanup.first_op_;
}

void spin_reactor::timer_op::do_complete(void* owner, operation* base,
    int /*error_value*/, std::size_t /*bytes_transferred*/)
{
  if (owner)
  {
    timer_op* t = static_cast<timer_op*>(base);
    if (operation* op = t->perform_io())
      op->complete(owner, 0, 0);
  }
}

operation* spin_reactor::queue_op::perform_io()
{
  descriptor_state* s = descriptor_state_;
  perform_io_cleanup_on_block_exit io_cleanup(s->reactor_);
  mutex::scoped_lock descriptor_lock(s->mutex_);

  while (reactor_op* op = s->op_queue_[op_type_].front())
  {
    if (reactor_op::status status = op->perform())
    {
      s->op_queue_[op_type_].pop();
      io_cleanup.ops_.push(op);

      if (status == reactor_op::done_and_exhausted)
        break;
    }
    else
      break;
  }

  // The first operation will be returned for completion now. The others will
  // be posted for later by the io_cleanup object's destructor.
  io_cleanup.first_op_ = io_cleanup.ops_.front();
  io_cleanup.ops_.pop();
  return io_cleanup.first_op_;
}

void spin_reactor::queue_op::do_complete(void* owner, operation* base,
    int /*error_value*/, std::size_t /*bytes_transferred*/)
{
  if (owner)
  {
    queue_op* q = static_cast<queue_op*>(base);
    if (operation* op = q->perform_io())
      op->complete(owner, 0, 0);
  }
}

spin_reactor::descriptor_state::descriptor_state(bool locking, int spin_count)
  : next_(0),
    prev_(0),
    mutex_(locking, spin_count),
    reactor_(0),
    descriptor_(-1),
    shutdown_(false),
    reg_seq_(0),
    offset_(0)
{
  for (int i = 0; i < max_ops; ++i)
    linked_[i] = false;
}

spin_reactor::spin_reactor(asio::execution_context& ctx)
  : execution_context_service_base<spin_reactor>(ctx),
    scheduler_(use_service<scheduler>(ctx)),
    mutex_(config(ctx).get("reactor", "registration_locking", true),
        config(ctx).get("reactor", "registration_locking_spin_count", 0)),
    shutdown_(false),
    io_locking_(config(ctx).get("reactor", "io_locking", true)),
    io_locking_spin_count_(
        config(ctx).get("reactor", "io_locking_spin_count", 0)),
    registered_descriptors_mutex_(mutex_.enabled(), mutex_.spin_count()),
    queues_(execution_context::allocator<entry*>(ctx)),
    next_seq_(0),
    newest_first_(spin_order_is_newest(ctx)),
    sequence_(0),
    descriptor_state_pool_(execution_context::allocator<void>(ctx),
        config(ctx).get("reactor", "preallocated_io_objects", 0U),
        io_locking_, io_locking_spin_count_)
{
  // Sized so that an ordered insertion never reallocates.
  if (std::size_t n = config(ctx).get("reactor",
        "preallocated_io_objects", 0U))
    queues_.reserve(n * max_ops + 1);

  timer_op_.reactor_ = this;
  timer_op_.priority_ =
    config(ctx).get("reactor", "timer_scheduling_priority", 0);
  timer_op_.interval_ = positive_config(ctx, "timer_scheduling_interval");
  timer_op_.seq_ = 0;
  link_entry(&timer_op_,
      non_negative_config(ctx, "timer_scheduling_offset"));
}

spin_reactor::~spin_reactor()
{
}

void spin_reactor::shutdown()
{
  mutex::scoped_lock lock(mutex_);
  shutdown_ = true;
  lock.unlock();

  op_queue<operation> ops;

  mutex::scoped_lock descriptors_lock(registered_descriptors_mutex_);

  while (descriptor_state* s = descriptor_state_pool_.first())
  {
    for (int i = 0; i < max_ops; ++i)
      ops.push(s->op_queue_[i]);
    s->shutdown_ = true;
    descriptor_state_pool_.free(s);
  }

  queues_.clear();

  descriptors_lock.unlock();

  timer_queues_.get_all_timers(ops);

  scheduler_.abandon_operations(ops);
}

void spin_reactor::notify_fork(asio::execution_context::fork_event)
{
  // No descriptors to recreate.
}

void spin_reactor::init_task()
{
  scheduler_.init_task();
}

int spin_reactor::register_descriptor(socket_type descriptor,
    spin_reactor::per_descriptor_data& descriptor_data)
{
  descriptor_state* s = allocate_descriptor_state();
  descriptor_data = s;

  ASIO_HANDLER_REACTOR_REGISTRATION((
        context(), static_cast<uintmax_t>(descriptor),
        reinterpret_cast<uintmax_t>(descriptor_data)));

  s->reactor_ = this;
  s->descriptor_ = descriptor;
  s->shutdown_ = false;
  s->offset_ = 0;
  for (int i = 0; i < max_ops; ++i)
  {
    s->linked_[i] = false;
    s->queues_[i].descriptor_state_ = s;
    s->queues_[i].op_type_ = i;
    s->queues_[i].priority_ = 0;
    s->queues_[i].interval_ = 1;
    s->queues_[i].seq_ = s->reg_seq_;
  }

  return 0;
}

int spin_reactor::register_internal_descriptor(
    int op_type, socket_type descriptor,
    spin_reactor::per_descriptor_data& descriptor_data, reactor_op* op)
{
  descriptor_state* s = allocate_descriptor_state();
  descriptor_data = s;

  ASIO_HANDLER_REACTOR_REGISTRATION((
        context(), static_cast<uintmax_t>(descriptor),
        reinterpret_cast<uintmax_t>(descriptor_data)));

  s->reactor_ = this;
  s->descriptor_ = descriptor;
  s->shutdown_ = false;
  s->offset_ = 0;
  for (int i = 0; i < max_ops; ++i)
  {
    s->linked_[i] = false;
    s->queues_[i].descriptor_state_ = s;
    s->queues_[i].op_type_ = i;
    s->queues_[i].priority_ = 0;
    s->queues_[i].interval_ = 1;
    s->queues_[i].seq_ = s->reg_seq_;
  }
  s->op_queue_[op_type].push(op);

  s->linked_[op_type] = true;
  link_entry(&s->queues_[op_type], s->offset_);

  return 0;
}

bool spin_reactor::descriptor_config(const char* name, const char* setting,
    int default_value, int min_value, int& value, asio::error_code& ec)
{
  // The two-level accessor takes the whole assembled string as its key.
  char key[max_descriptor_name_len + 32];
  const int n = std::snprintf(key, sizeof(key),
      "descriptors.%s.%s", name, setting);
  if (n < 0 || static_cast<std::size_t>(n) >= sizeof(key))
  {
    ec = asio::error::invalid_argument;
    return false;
  }

  value = config(context()).get("reactor", key, default_value);

  if (value < min_value)
  {
    ec = asio::error::invalid_argument;
    return false;
  }

  return true;
}

void spin_reactor::set_descriptor_name(
    spin_reactor::per_descriptor_data& descriptor_data, const char* name,
    asio::error_code& ec)
{
  descriptor_state* s = descriptor_data;
  if (!s)
  {
    asio::error::clear(ec);
    return;
  }

  // All three are read before anything changes, so a bad value leaves the
  // descriptor as it was.
  int priority = 0;
  int interval = 1;
  int offset = 0;
  const int no_minimum = (std::numeric_limits<int>::min)();
  if (!descriptor_config(name, "priority", 0, no_minimum, priority, ec)
      || !descriptor_config(name, "interval", 1, 1, interval, ec)
      || !descriptor_config(name, "offset", 0, 0, offset, ec))
    return;

  mutex::scoped_lock descriptors_lock(registered_descriptors_mutex_);

  s->offset_ = offset;

  for (int i = 0; i < max_ops; ++i)
  {
    s->queues_[i].priority_ = priority;
    s->queues_[i].interval_ = static_cast<std::size_t>(interval);
    s->queues_[i].next_due_ =
      due_after(sequence_, s->queues_[i].interval_, offset);
  }

  const bool newest_first = newest_first_;
  std::sort(queues_.begin(), queues_.end(),
      [newest_first](entry* a, entry* b)
      {
        return precedes(a, b, newest_first);
      });

  asio::error::clear(ec);
}

void spin_reactor::move_descriptor(socket_type,
    spin_reactor::per_descriptor_data& target_descriptor_data,
    spin_reactor::per_descriptor_data& source_descriptor_data)
{
  target_descriptor_data = source_descriptor_data;
  source_descriptor_data = 0;
}

void spin_reactor::call_post_immediate_completion(
    operation* op, bool is_continuation, const void* self)
{
  static_cast<const spin_reactor*>(self)->post_immediate_completion(
      op, is_continuation);
}

bool spin_reactor::start_bad_descriptor_op(reactor_op* op)
{
  op->ec_ = asio::error::bad_descriptor;
  return true;
}

bool spin_reactor::start_unready_op(int op_type,
    spin_reactor::per_descriptor_data& descriptor_data,
    mutex::scoped_lock& descriptor_lock, reactor_op* op,
    reactor_op::perform_func_type)
{
  ASIO_ASSERT(descriptor_lock.locked()
      || !descriptor_data->mutex_.enabled());

  descriptor_state* s = descriptor_data;

  if (s->op_queue_[op_type].empty() && ASIO_UNLIKELY(s->shutdown_))
  {
    descriptor_lock.unlock();
    return true;
  }

  s->op_queue_[op_type].push(op);
  scheduler_.work_started();

  const bool link = !s->linked_[op_type];
  if (link)
    s->linked_[op_type] = true;

  descriptor_lock.unlock();

  if (link)
    link_entry(&s->queues_[op_type], s->offset_);

  return false;
}

void spin_reactor::cancel_ops(socket_type,
    spin_reactor::per_descriptor_data& descriptor_data)
{
  if (!descriptor_data)
    return;

  mutex::scoped_lock descriptor_lock(descriptor_data->mutex_);

  op_queue<operation> ops;
  for (int i = 0; i < max_ops; ++i)
  {
    while (reactor_op* op = descriptor_data->op_queue_[i].front())
    {
      op->ec_ = asio::error::operation_aborted;
      descriptor_data->op_queue_[i].pop();
      ops.push(op);
    }
  }

  descriptor_lock.unlock();

  scheduler_.post_deferred_completions(ops);
}

void spin_reactor::cancel_ops_by_key(socket_type,
    spin_reactor::per_descriptor_data& descriptor_data,
    int op_type, void* cancellation_key)
{
  if (!descriptor_data)
    return;

  mutex::scoped_lock descriptor_lock(descriptor_data->mutex_);

  op_queue<operation> ops;
  op_queue<reactor_op> other_ops;
  while (reactor_op* op = descriptor_data->op_queue_[op_type].front())
  {
    descriptor_data->op_queue_[op_type].pop();
    if (op->cancellation_key_ == cancellation_key)
    {
      op->ec_ = asio::error::operation_aborted;
      ops.push(op);
    }
    else
      other_ops.push(op);
  }
  descriptor_data->op_queue_[op_type].push(other_ops);

  descriptor_lock.unlock();

  scheduler_.post_deferred_completions(ops);
}

void spin_reactor::deregister_descriptor(socket_type descriptor,
    spin_reactor::per_descriptor_data& descriptor_data, bool /*closing*/)
{
  (void)descriptor;

  if (!descriptor_data)
    return;

  mutex::scoped_lock descriptor_lock(descriptor_data->mutex_);

  if (!descriptor_data->shutdown_)
  {
    op_queue<operation> ops;
    for (int i = 0; i < max_ops; ++i)
    {
      while (reactor_op* op = descriptor_data->op_queue_[i].front())
      {
        op->ec_ = asio::error::operation_aborted;
        descriptor_data->op_queue_[i].pop();
        ops.push(op);
      }
    }

    descriptor_data->descriptor_ = -1;
    descriptor_data->shutdown_ = true;

    descriptor_lock.unlock();

    ASIO_HANDLER_REACTOR_DEREGISTRATION((
          context(), static_cast<uintmax_t>(descriptor),
          reinterpret_cast<uintmax_t>(descriptor_data)));

    scheduler_.post_deferred_completions(ops);

    // Leave descriptor_data set so that it will be freed by the subsequent
    // call to cleanup_descriptor_data.
  }
  else
  {
    // We are shutting down, so prevent cleanup_descriptor_data from freeing
    // the descriptor_data object and let the destructor free it instead.
    descriptor_data = 0;
  }
}

void spin_reactor::deregister_internal_descriptor(socket_type descriptor,
    spin_reactor::per_descriptor_data& descriptor_data)
{
  (void)descriptor;

  if (!descriptor_data)
    return;

  mutex::scoped_lock descriptor_lock(descriptor_data->mutex_);

  if (!descriptor_data->shutdown_)
  {
    op_queue<operation> ops;
    for (int i = 0; i < max_ops; ++i)
      ops.push(descriptor_data->op_queue_[i]);

    descriptor_data->descriptor_ = -1;
    descriptor_data->shutdown_ = true;

    descriptor_lock.unlock();

    ASIO_HANDLER_REACTOR_DEREGISTRATION((
          context(), static_cast<uintmax_t>(descriptor),
          reinterpret_cast<uintmax_t>(descriptor_data)));

    // Leave descriptor_data set so that it will be freed by the subsequent
    // call to cleanup_descriptor_data.
  }
  else
  {
    // We are shutting down, so prevent cleanup_descriptor_data from freeing
    // the descriptor_data object and let the destructor free it instead.
    descriptor_data = 0;
  }
}

void spin_reactor::cleanup_descriptor_data(
    spin_reactor::per_descriptor_data& descriptor_data)
{
  if (descriptor_data)
  {
    free_descriptor_state(descriptor_data);
    descriptor_data = 0;
  }
}

void spin_reactor::get_pending_ops(op_queue<operation>& ops)
{
  mutex::scoped_lock descriptors_lock(registered_descriptors_mutex_);

  const std::size_t sequence = sequence_++;

  for (std::size_t i = 0, n = queues_.size(); i < n; ++i)
  {
    entry* e = queues_[i];

    if (sequence != e->next_due_)
      continue;

    // Advanced whether or not the entry has anything to offer: one left behind
    // the counter never compares equal again.
    e->next_due_ += e->interval_;

    if (e->is_timer_)
    {
      mutex::scoped_lock lock(mutex_);
      if (timer_queues_.wait_duration_usec(1) == 0)
        ops.push(e);
    }
    else
    {
      queue_op* q = static_cast<queue_op*>(e);
      descriptor_state* s = q->descriptor_state_;
      mutex::scoped_lock descriptor_lock(s->mutex_);
      if (!s->op_queue_[q->op_type_].empty())
        ops.push(e);
    }
  }
}

void spin_reactor::run(long /*usec*/, op_queue<operation>& ops)
{
  get_pending_ops(ops);
}

void spin_reactor::interrupt()
{
  // Nothing to interrupt.
}

spin_reactor::descriptor_state* spin_reactor::allocate_descriptor_state()
{
  mutex::scoped_lock descriptors_lock(registered_descriptors_mutex_);

  descriptor_state* s = descriptor_state_pool_.alloc(
      io_locking_, io_locking_spin_count_);

  s->reg_seq_ = next_seq_++;

  return s;
}

void spin_reactor::link_entry(entry* e, int offset)
{
  const bool newest_first = newest_first_;

  mutex::scoped_lock descriptors_lock(registered_descriptors_mutex_);

  e->next_due_ = due_after(sequence_, e->interval_, offset);

  queue_vector::iterator pos = std::upper_bound(
      queues_.begin(), queues_.end(), e,
      [newest_first](entry* a, entry* b)
      {
        return precedes(a, b, newest_first);
      });

  queues_.insert(pos, e);
}

void spin_reactor::free_descriptor_state(spin_reactor::descriptor_state* s)
{
  mutex::scoped_lock descriptors_lock(registered_descriptors_mutex_);

  std::size_t out = 0;
  for (std::size_t i = 0, n = queues_.size(); i < n; ++i)
  {
    if (queues_[i]->is_timer_
        || static_cast<queue_op*>(queues_[i])->descriptor_state_ != s)
      queues_[out++] = queues_[i];
  }
  queues_.resize(out);

  descriptor_state_pool_.free(s);
}

void spin_reactor::do_add_timer_queue(timer_queue_base& queue)
{
  mutex::scoped_lock lock(mutex_);
  timer_queues_.insert(&queue);
}

void spin_reactor::do_remove_timer_queue(timer_queue_base& queue)
{
  mutex::scoped_lock lock(mutex_);
  timer_queues_.erase(&queue);
}

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_ENABLE_SPIN_REACTOR)
       //   && !defined(ASIO_HAS_IOCP)
       //   && !defined(ASIO_WINDOWS_RUNTIME)
       //   && !defined(ASIO_HAS_IO_URING_AS_DEFAULT)

#endif // ASIO_DETAIL_IMPL_SPIN_REACTOR_IPP
