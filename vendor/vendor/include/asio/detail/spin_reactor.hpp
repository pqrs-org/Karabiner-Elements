//
// detail/spin_reactor.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_SPIN_REACTOR_HPP
#define ASIO_DETAIL_SPIN_REACTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_ENABLE_SPIN_REACTOR) \
  && !defined(ASIO_HAS_IOCP) \
  && !defined(ASIO_WINDOWS_RUNTIME) \
  && !defined(ASIO_HAS_IO_URING_AS_DEFAULT)

#include <cstddef>
#include <vector>
#include "asio/detail/conditionally_enabled_mutex.hpp"
#include "asio/detail/limits.hpp"
#include "asio/detail/object_pool.hpp"
#include "asio/detail/op_queue.hpp"
#include "asio/detail/reactor_op.hpp"
#include "asio/detail/scheduler_task.hpp"
#include "asio/detail/slim_mutex.hpp"
#include "asio/detail/socket_types.hpp"
#include "asio/detail/timer_queue_base.hpp"
#include "asio/detail/timer_queue_set.hpp"
#include "asio/detail/wait_op.hpp"
#include "asio/error.hpp"
#include "asio/execution_context.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

class spin_reactor
  : public execution_context_service_base<spin_reactor>,
    public scheduler_task
{
public:
  // The mutex type used by this reactor.
  typedef conditionally_enabled_mutex<slim_mutex> mutex;

#if defined(ASIO_WINDOWS) || defined(ASIO_CYGWIN_W32_SOCKETS)
  enum op_types { read_op = 0, write_op = 1, except_op = 2,
    connect_op = 3, max_ops = 4 };
#else // defined(ASIO_WINDOWS) || defined(ASIO_CYGWIN_W32_SOCKETS)
  enum op_types { read_op = 0, write_op = 1,
    connect_op = 1, except_op = 2, max_ops = 3 };
#endif // defined(ASIO_WINDOWS) || defined(ASIO_CYGWIN_W32_SOCKETS)

  struct descriptor_state;

  // Something the reactor offers to the scheduler, in the order it is offered.
  class entry : operation
  {
    friend class spin_reactor;

    // The pass this entry is next due on, guarded by the queues' mutex.
    std::size_t next_due_;

    // How many passes go by between offers of this entry.
    std::size_t interval_;

    // Where this entry comes relative to the others. Higher is offered first.
    int priority_;

    // Registration order. Breaks a tie at the same priority.
    std::size_t seq_;

    // Which kind of entry this is.
    bool is_timer_;

    ASIO_DECL entry(operation::func_type complete_func, bool is_timer);
  };

  // The operations queued against one type for one descriptor.
  class queue_op : public entry
  {
    friend class spin_reactor;

    descriptor_state* descriptor_state_;
    int op_type_;

    ASIO_DECL queue_op();
    ASIO_DECL operation* perform_io();
    ASIO_DECL static void do_complete(void* owner, operation* base,
        int error_value, std::size_t bytes_transferred);
  };

  // The reactor's ready timers, as one entry in the same order.
  class timer_op : public entry
  {
    friend class spin_reactor;

    spin_reactor* reactor_;

    ASIO_DECL timer_op();
    ASIO_DECL operation* perform_io();
    ASIO_DECL static void do_complete(void* owner, operation* base,
        int error_value, std::size_t bytes_transferred);
  };

  // Per-descriptor queues, with one completable entry per queue.
  struct descriptor_state
  {
    ASIO_DECL descriptor_state(bool locking, int spin_count);

    // Links used by the object pool to recycle this object.
    descriptor_state* next_;
    descriptor_state* prev_;

    mutex mutex_;
    spin_reactor* reactor_;
    int descriptor_;
    op_queue<reactor_op> op_queue_[max_ops];
    bool shutdown_;

    // Registration order, ordering this descriptor's queues against others.
    std::size_t reg_seq_;

    // The configured offset, kept so the phase can be recomputed from it.
    int offset_;

    // Whether each queue is registered, which it stays until deregistration.
    bool linked_[max_ops];

    queue_op queues_[max_ops];
  };

  // Per-descriptor data.
  typedef descriptor_state* per_descriptor_data;

  // Constructor.
  ASIO_DECL spin_reactor(asio::execution_context& ctx);

  // Destructor.
  ASIO_DECL ~spin_reactor();

  // Destroy all user-defined handler objects owned by the service.
  ASIO_DECL void shutdown();

  // Recreate internal descriptors following a fork.
  ASIO_DECL void notify_fork(
      asio::execution_context::fork_event fork_ev);

  // Initialise the task.
  ASIO_DECL void init_task();

  // Register a descriptor with the reactor. Always succeeds.
  ASIO_DECL int register_descriptor(socket_type descriptor,
      per_descriptor_data& descriptor_data);

  // Register a descriptor with an associated single operation. Always
  // succeeds.
  ASIO_DECL int register_internal_descriptor(
      int op_type, socket_type descriptor,
      per_descriptor_data& descriptor_data, reactor_op* op);

  // Give a descriptor a name and apply the priority, interval and offset
  // configured for it.
  ASIO_DECL void set_descriptor_name(per_descriptor_data& descriptor_data,
      const char* name, asio::error_code& ec);

  // Move descriptor registration from one descriptor_data object to another.
  ASIO_DECL void move_descriptor(socket_type descriptor,
      per_descriptor_data& target_descriptor_data,
      per_descriptor_data& source_descriptor_data);

  // Post a reactor operation for immediate completion.
  void post_immediate_completion(operation* op, bool is_continuation) const;

  // Post a reactor operation for immediate completion.
  ASIO_DECL static void call_post_immediate_completion(
      operation* op, bool is_continuation, const void* self);

  // Start a new operation with the operation type and perform function
  // specified at compile time. Returns true if the operation is ready for
  // immediate completion. Otherwise, the reactor operation will be attempted
  // again on a later pass.
  template <int OpType, reactor_op::perform_func_type Perform>
  inline bool start_op(socket_type descriptor,
      per_descriptor_data& descriptor_data, reactor_op* op);

  // Set the result of an operation that was started on an unopen descriptor.
  ASIO_DECL ASIO_NEVER_INLINE static bool start_bad_descriptor_op(
      reactor_op* op);

  // Queue an operation that was not completed speculatively. It is attempted
  // again on the next pass.
  ASIO_DECL ASIO_NEVER_INLINE bool start_unready_op(int op_type,
      per_descriptor_data& descriptor_data, mutex::scoped_lock& descriptor_lock,
      reactor_op* op, reactor_op::perform_func_type perform_func);

  // Cancel all operations associated with the given descriptor. The
  // handlers associated with the descriptor will be invoked with the
  // operation_aborted error.
  ASIO_DECL void cancel_ops(socket_type descriptor,
      per_descriptor_data& descriptor_data);

  // Cancel all operations associated with the given descriptor and key. The
  // handlers associated with the descriptor will be invoked with the
  // operation_aborted error.
  ASIO_DECL void cancel_ops_by_key(socket_type descriptor,
      per_descriptor_data& descriptor_data,
      int op_type, void* cancellation_key);

  // Cancel any operations that are running against the descriptor and remove
  // its registration from the reactor. The reactor resources associated with
  // the descriptor must be released by calling cleanup_descriptor_data.
  ASIO_DECL void deregister_descriptor(socket_type descriptor,
      per_descriptor_data& descriptor_data, bool closing);

  // Remove the descriptor's registration from the reactor. The reactor
  // resources associated with the descriptor must be released by calling
  // cleanup_descriptor_data.
  ASIO_DECL void deregister_internal_descriptor(
      socket_type descriptor, per_descriptor_data& descriptor_data);

  // Perform any post-deregistration cleanup tasks associated with the
  // descriptor data.
  ASIO_DECL void cleanup_descriptor_data(
      per_descriptor_data& descriptor_data);

  // Add a new timer queue to the reactor.
  template <typename TimeTraits, typename Allocator>
  void add_timer_queue(timer_queue<TimeTraits, Allocator>& timer_queue);

  // Remove a timer queue from the reactor.
  template <typename TimeTraits, typename Allocator>
  void remove_timer_queue(timer_queue<TimeTraits, Allocator>& timer_queue);

  // Schedule a new operation in the given timer queue to expire at the
  // specified absolute time.
  template <typename TimeTraits, typename Allocator>
  void schedule_timer(timer_queue<TimeTraits, Allocator>& queue,
      const typename TimeTraits::time_type& time,
      typename timer_queue<TimeTraits, Allocator>::per_timer_data& timer,
      wait_op* op);

  // Cancel the timer operations associated with the given token. Returns the
  // number of operations that have been posted or dispatched.
  template <typename TimeTraits, typename Allocator>
  std::size_t cancel_timer(timer_queue<TimeTraits, Allocator>& queue,
      typename timer_queue<TimeTraits, Allocator>::per_timer_data& timer,
      std::size_t max_cancelled = (std::numeric_limits<std::size_t>::max)());

  // Cancel the timer operations associated with the given key.
  template <typename TimeTraits, typename Allocator>
  void cancel_timer_by_key(timer_queue<TimeTraits, Allocator>& queue,
      typename timer_queue<TimeTraits, Allocator>::per_timer_data* timer,
      void* cancellation_key);

  // Move the timer operations associated with the given timer.
  template <typename TimeTraits, typename Allocator>
  void move_timer(timer_queue<TimeTraits, Allocator>& queue,
      typename timer_queue<TimeTraits, Allocator>::per_timer_data& target,
      typename timer_queue<TimeTraits, Allocator>::per_timer_data& source);

  // Hand back every operation queue that is not empty. Never waits, whatever
  // usec says.
  ASIO_DECL void run(long usec, op_queue<operation>& ops);

  // Interrupt the reactor.
  ASIO_DECL void interrupt();

private:
  // The registered queues, in the order they are offered in.
  typedef std::vector<entry*,
    execution_context::allocator<entry*>> queue_vector;

  // Read the "reactor" / "spin_order" configuration value.
  ASIO_DECL static bool spin_order_is_newest(
      asio::execution_context& ctx);

  // Read a "reactor" / "descriptors.<name>.<setting>" value. Returns false,
  // with ec set, if the key does not fit or the value is below min_value.
  ASIO_DECL bool descriptor_config(const char* name, const char* setting,
      int default_value, int min_value, int& value,
      asio::error_code& ec);

  // Read a positive "reactor" configuration value, throwing otherwise. Used
  // for the timer interval.
  ASIO_DECL static std::size_t positive_config(
      asio::execution_context& ctx, const char* key);

  // Read a non-negative "reactor" configuration value, throwing otherwise.
  // Used for the timer offset.
  ASIO_DECL static int non_negative_config(
      asio::execution_context& ctx, const char* key);

  // The first pass at or after the given one on which a queue with this
  // interval and offset is due. Must be given a pass not yet taken.
  ASIO_DECL static std::size_t due_after(std::size_t sequence,
      std::size_t interval, int offset);

  // Order two queues by priority, highest first, then by registration order,
  // whichever way round the configuration asks, then by operation type.
  ASIO_DECL static bool precedes(entry* a, entry* b, bool newest_first);

  // Hand back every registered queue that is due this pass and has operations
  // waiting, and any ready timers.
  ASIO_DECL void get_pending_ops(op_queue<operation>& ops);

  // Allocate a new descriptor state object.
  ASIO_DECL descriptor_state* allocate_descriptor_state();

  // Add an entry in its ordered position and set its first due pass.
  // Must not be called with a descriptor's mutex held.
  ASIO_DECL void link_entry(entry* e, int offset);

  // Free an existing descriptor state object and unregister its queues.
  ASIO_DECL void free_descriptor_state(descriptor_state* s);

  // Helper function to add a new timer queue.
  ASIO_DECL void do_add_timer_queue(timer_queue_base& queue);

  // Helper function to remove a timer queue.
  ASIO_DECL void do_remove_timer_queue(timer_queue_base& queue);

  // The scheduler implementation used to post completions.
  scheduler& scheduler_;

  // Mutex to protect access to internal data.
  mutex mutex_;

  // The timer queues.
  timer_queue_set timer_queues_;

  // Whether the service has been shut down.
  bool shutdown_;

  // Whether I/O locking is enabled.
  const bool io_locking_;

  // How many times to spin waiting for the I/O mutex.
  const int io_locking_spin_count_;

  // Mutex to protect access to the registered queues.
  mutex registered_descriptors_mutex_;

  // The registered operation queues, kept sorted.
  queue_vector queues_;

  // The order to assign to the next descriptor registered.
  std::size_t next_seq_;

  // Which way round registration order compares, from "spin_order".
  const bool newest_first_;

  // The timers' own entry, carrying their scheduling configuration.
  timer_op timer_op_;

  // Names the pass about to be taken. Left to wrap, so due-ness must stay an
  // equality test against it and not a residue.
  std::size_t sequence_;

  // Recycles the per-descriptor state objects.
  object_pool<descriptor_state, execution_context::allocator<void>>
    descriptor_state_pool_;

  // Helper class to do post-perform_io cleanup.
  struct perform_io_cleanup_on_block_exit;
  friend struct perform_io_cleanup_on_block_exit;
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#include "asio/detail/impl/spin_reactor.hpp"
#if defined(ASIO_HEADER_ONLY)
# include "asio/detail/impl/spin_reactor.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // defined(ASIO_ENABLE_SPIN_REACTOR)
       //   && !defined(ASIO_HAS_IOCP)
       //   && !defined(ASIO_WINDOWS_RUNTIME)
       //   && !defined(ASIO_HAS_IO_URING_AS_DEFAULT)

#endif // ASIO_DETAIL_SPIN_REACTOR_HPP
