//
// impl/trampoline.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_TRAMPOLINE_HPP
#define ASIO_IMPL_TRAMPOLINE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <limits>
#include <memory>
#include <stdexcept>
#include "asio/associator.hpp"
#include "asio/detail/executor_function.hpp"
#include "asio/detail/executor_op.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/handler_tracking.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/op_queue.hpp"
#include "asio/detail/operation.hpp"
#include "asio/detail/throw_exception.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

const int trampoline_depth_bits = 8;

const std::uint64_t trampoline_id_mask =
  (std::uint64_t(1) << (64 - trampoline_depth_bits)) - 1;

inline std::uint64_t make_trampoline_id(
    std::uint64_t id, std::size_t max_inline_depth)
{
  if (max_inline_depth >> trampoline_depth_bits)
  {
    detail::throw_exception(
        std::out_of_range("trampoline max_inline_depth out of range"));
  }

  return (id & trampoline_id_mask)
    | (std::uint64_t(max_inline_depth) << (64 - trampoline_depth_bits));
}

inline std::size_t trampoline_depth(std::uint64_t id) noexcept
{
  return static_cast<std::size_t>(id >> (64 - trampoline_depth_bits));
}

struct trampoline_frame
{
  ASIO_DECL static trampoline_frame* current() noexcept;
  ASIO_DECL static void current(trampoline_frame* f) noexcept;
  ASIO_DECL static bool install_pending() noexcept;
  ASIO_DECL static void install_pending(bool v) noexcept;
  ASIO_DECL void drain();

  op_queue<operation> queue;
  std::size_t num_calls;
  std::size_t max_calls;
  std::size_t inline_depth;
  std::uint64_t installing_id;
};

// Helper class used to mark an installation that the target has not yet run.
// Prevents reentrant requests to install, which would never terminate.
class trampoline_pending_guard
{
public:
  trampoline_pending_guard()
  {
    trampoline_frame::install_pending(true);
  }

  ~trampoline_pending_guard()
  {
    trampoline_frame::install_pending(false);
  }
};

// Helper class used to prevent reentrant use of the trampoline.
class trampoline_bypass
{
public:
  explicit trampoline_bypass(trampoline_frame* f)
    : frame_(f)
  {
    trampoline_frame::current(0);
  }

  ~trampoline_bypass()
  {
    trampoline_frame::current(frame_);
  }

private:
  trampoline_frame* frame_;
};

// Helper class to count how many recursive invocations have been performed.
class trampoline_inline_depth_guard
{
public:
  explicit trampoline_inline_depth_guard(trampoline_frame* f)
    : frame_(f)
  {
    ++frame_->inline_depth;
  }

  ~trampoline_inline_depth_guard()
  {
    --frame_->inline_depth;
  }

private:
  trampoline_frame* frame_;
};

template <typename Executor, typename Property,
    bool = can_prefer<const Executor&, Property>::value>
struct trampoline_prefer_impl
{
  static const Executor& get(const Executor& ex, const Property&) noexcept
  {
    return ex;
  }
};

template <typename Executor, typename Property>
struct trampoline_prefer_impl<Executor, Property, true>
{
  static prefer_result_t<const Executor&, Property> get(
      const Executor& ex, const Property& p)
  {
    return asio::prefer(ex, p);
  }
};

template <typename Executor, typename Property>
inline auto trampoline_prefer(const Executor& ex, const Property& p)
  -> decltype(trampoline_prefer_impl<Executor, Property>::get(ex, p))
{
  return trampoline_prefer_impl<Executor, Property>::get(ex, p);
}

template <typename Executor, typename Function>
inline enable_if_t<
  can_require<const Executor&, execution::blocking_t::never_t>::value
> trampoline_submit_drained(const Executor& ex, Function&& f)
{
  trampoline_prefer(asio::require(ex, execution::blocking.never),
      execution::relationship.continuation).execute(
        static_cast<Function&&>(f));
}

#if !defined(ASIO_NO_TS_EXECUTORS)
template <typename Executor, typename Function>
inline enable_if_t<
  !can_require<const Executor&, execution::blocking_t::never_t>::value
> trampoline_submit_drained(const Executor& ex, Function&& f)
{
  trampoline_prefer(ex, execution::relationship.continuation).post(
      static_cast<Function&&>(f), std::allocator<void>());
}
#endif // !defined(ASIO_NO_TS_EXECUTORS)

// Transfer operators from the trampoline to the trampoline to the target
// executors. Used when the trampoline is unable to run its remaining queued
// functions, when an exception has been thrown.
template <typename Executor>
void trampoline_drain_to_target(
    trampoline_frame& frame, const Executor& ex) noexcept
{
  while (operation* next = frame.queue.front())
  {
    frame.queue.pop();
    trampoline_submit_drained(ex, executor_function(next));
  }
}

// Run the function object according to the exception propagation property.
template <typename InlineExceptionHandling>
struct trampoline_run_immediately
{
  template <typename Executor, typename Function>
  static void call(trampoline_frame*,
      const Executor&, Function&& f) noexcept
  {
    decay_t<Function>(static_cast<Function&&>(f))();
  }
};

template <>
struct trampoline_run_immediately<
    execution::inline_exception_handling_t::propagate_t>
{
  template <typename Executor, typename Function>
  static void call(trampoline_frame* current,
      const Executor& ex, Function&& f)
  {
#if defined(ASIO_NO_EXCEPTIONS)
    (void)current;
    (void)ex;
    decay_t<Function>(static_cast<Function&&>(f))();
#else // defined(ASIO_NO_EXCEPTIONS)
    try
    {
      decay_t<Function>(static_cast<Function&&>(f))();
    }
    catch (...)
    {
      trampoline_bypass bypass(current);
      trampoline_drain_to_target(*current, ex);
      throw;
    }
#endif // defined(ASIO_NO_EXCEPTIONS)
  }
};

// Install a trampoline on this thread and execute the function object inside
// it. Only the outermost call on the thread installs. Any exception results
// in termination.
template <typename Function>
inline void trampoline_install_and_execute(std::uint64_t id,
    Function&& f, std::size_t max_calls) noexcept
{
  if (trampoline_frame::current() != 0)
  {
    static_cast<Function&&>(f)();
    return;
  }

  trampoline_frame frame;
  frame.num_calls = 0;
  frame.max_calls = max_calls;
  frame.inline_depth = 0;
  frame.installing_id = id;
  trampoline_frame::current(&frame);

  static_cast<Function&&>(f)();
  frame.drain();

  trampoline_frame::current(0);
}

// Install a trampoline on this thread and execute the operation object inside
// it. Only the outermost call on the thread installs. Any exception results
// in termination.
ASIO_ALWAYS_INLINE void trampoline_install_and_run(std::uint64_t id,
    operation* target, std::size_t max_calls) noexcept
{
  if (trampoline_frame::current() != 0)
  {
    target->complete(target, 0, 0);
    return;
  }

  trampoline_frame frame;
  frame.num_calls = 0;
  frame.max_calls = max_calls;
  frame.inline_depth = 0;
  frame.installing_id = id;
  trampoline_frame::current(&frame);

  target->complete(target, 0, 0);
  frame.drain();

  trampoline_frame::current(0);
}

// Install a trampoline on this thread and execute the function object inside
// it. Only the outermost call on the thread installs. Exceptions are propagated
// to the caller.
template <typename Executor, typename Function>
void trampoline_install_and_execute_propagating(std::uint64_t id,
    const Executor& ex, Function&& f, std::size_t max_calls)
{
#if defined(ASIO_NO_EXCEPTIONS)
  (void)ex;
#endif // defined(ASIO_NO_EXCEPTIONS)

  if (trampoline_frame::current() != 0)
  {
    static_cast<Function&&>(f)();
    return;
  }

  trampoline_frame frame;
  frame.num_calls = 0;
  frame.max_calls = max_calls;
  frame.inline_depth = 0;
  frame.installing_id = id;
  trampoline_frame::current(&frame);

#if !defined(ASIO_NO_EXCEPTIONS)
  try
#endif // !defined(ASIO_NO_EXCEPTIONS)
  {
    static_cast<Function&&>(f)();
    frame.drain();
  }
#if !defined(ASIO_NO_EXCEPTIONS)
  catch (...)
  {
    trampoline_frame::current(0);
    trampoline_drain_to_target(frame, ex);
    throw;
  }
#endif // !defined(ASIO_NO_EXCEPTIONS)

  trampoline_frame::current(0);
}

// Install a trampoline on this thread and execute the operation object inside
// it. Only the outermost call on the thread installs. Exceptions are propagated
// to the caller.
template <typename Executor>
ASIO_ALWAYS_INLINE void trampoline_install_and_run_propagating(
    std::uint64_t id, const Executor& ex, operation* target,
    std::size_t max_calls)
{
#if defined(ASIO_NO_EXCEPTIONS)
  (void)ex;
#endif // defined(ASIO_NO_EXCEPTIONS)

  if (trampoline_frame::current() != 0)
  {
    target->complete(target, 0, 0);
    return;
  }

  trampoline_frame frame;
  frame.num_calls = 0;
  frame.max_calls = max_calls;
  frame.inline_depth = 0;
  frame.installing_id = id;
  trampoline_frame::current(&frame);

#if !defined(ASIO_NO_EXCEPTIONS)
  try
#endif // !defined(ASIO_NO_EXCEPTIONS)
  {
    target->complete(target, 0, 0);
    frame.drain();
  }
#if !defined(ASIO_NO_EXCEPTIONS)
  catch (...)
  {
    trampoline_frame::current(0);
    trampoline_drain_to_target(frame, ex);
    throw;
  }
#endif // !defined(ASIO_NO_EXCEPTIONS)

  trampoline_frame::current(0);
}

template <typename Executor, typename InlineExceptionHandling>
class trampoline_installer_state
{
public:
  explicit trampoline_installer_state(const Executor&) noexcept
  {
  }

  template <typename Function>
  void install_and_execute(std::uint64_t id,
      Function&& f, std::size_t max_calls) const noexcept
  {
    trampoline_install_and_execute(id,
        static_cast<Function&&>(f), max_calls);
  }

  void install_and_execute(std::uint64_t id,
      executor_function&& f, std::size_t max_calls) const noexcept
  {
    trampoline_install_and_run(id, f.release(), max_calls);
  }

  ASIO_ALWAYS_INLINE void install_and_run(std::uint64_t id,
      operation* target, std::size_t max_calls) const noexcept
  {
    trampoline_install_and_run(id, target, max_calls);
  }
};

template <typename Executor>
class trampoline_installer_state<Executor,
    execution::inline_exception_handling_t::propagate_t>
{
public:
  explicit trampoline_installer_state(const Executor& ex)
    : executor_(ex)
  {
  }

  template <typename Function>
  void install_and_execute(std::uint64_t id,
      Function&& f, std::size_t max_calls) const
  {
    trampoline_install_and_execute_propagating(id, executor_,
        static_cast<Function&&>(f), max_calls);
  }

  void install_and_execute(std::uint64_t id,
      executor_function&& f, std::size_t max_calls) const
  {
    trampoline_install_and_run_propagating(id, executor_,
        f.release(), max_calls);
  }

  ASIO_ALWAYS_INLINE void install_and_run(std::uint64_t id,
      operation* target, std::size_t max_calls) const
  {
    trampoline_install_and_run_propagating(id, executor_, target, max_calls);
  }

private:
  Executor executor_;
};

// Installs a trampoline on this thread and runs a ready-made operation inside
// it. Used when the function object submitted to the trampoline already
// contains an operation.
template <typename Alloc, typename Executor, typename InlineExceptionHandling>
class trampoline_install_op : public operation,
  private trampoline_installer_state<Executor, InlineExceptionHandling>
{
public:
  ASIO_DEFINE_TAGGED_HANDLER_ALLOCATOR_PTR(
      thread_info_base::default_tag, trampoline_install_op);

  trampoline_install_op(std::uint64_t id, std::size_t max_calls,
      operation* target, const Executor& ex, const Alloc& allocator)
    : operation(&trampoline_install_op::do_complete),
      trampoline_installer_state<Executor, InlineExceptionHandling>(ex),
      id_(id),
      max_calls_(max_calls),
      target_(target),
      allocator_(allocator)
  {
  }

  static void do_complete(void* owner, operation* base,
      int /*error_value*/, std::size_t /*bytes_transferred*/)
  {
    // Take ownership of the operation.
    ASIO_ASSUME(base != 0);
    trampoline_install_op* o(static_cast<trampoline_install_op*>(base));
    Alloc allocator(o->allocator_);
    ptr p = { detail::addressof(allocator), o, o };

    ASIO_HANDLER_COMPLETION((*o));

    // Copy out the state so that the memory can be deallocated before the
    // trampoline is installed, exactly as executor_op copies out its handler.
    const std::uint64_t id = o->id_;
    const std::size_t max_calls = o->max_calls_;
    operation* const target = o->target_;
    const trampoline_installer_state<Executor,
        InlineExceptionHandling> policy(*o);
    p.reset();

    if (!owner)
    {
      target->destroy();
      return;
    }

    fenced_block b(fenced_block::half);
    ASIO_HANDLER_INVOCATION_BEGIN(());
    policy.install_and_run(id, target, max_calls);
    ASIO_HANDLER_INVOCATION_END;
  }

private:
  std::uint64_t id_;
  std::size_t max_calls_;
  operation* target_;
  Alloc allocator_;
};

// Installs a trampoline on this thread and runs the function object.
template <typename Function, typename Executor,
    typename InlineExceptionHandling>
class trampoline_installer :
  private trampoline_installer_state<Executor, InlineExceptionHandling>
{
public:
  template <typename F>
  trampoline_installer(std::uint64_t id, std::size_t calls,
      const Executor& ex, F&& f)
    : trampoline_installer_state<Executor, InlineExceptionHandling>(ex),
      id_(id),
      calls_(calls),
      function_(static_cast<F&&>(f))
  {
  }

  void operator()()
  {
    this->install_and_execute(id_, static_cast<Function&&>(function_), calls_);
  }

  std::uint64_t id_;
  std::size_t calls_;
  Function function_;
};

// Installs a trampoline on this thread and runs the function object by
// reference. Used when the execution is always blocking, which means that we
// know that the function outlives the execute call.
template <typename Function, typename Executor,
    typename InlineExceptionHandling>
class trampoline_installer_ref :
  private trampoline_installer_state<Executor, InlineExceptionHandling>
{
public:
  trampoline_installer_ref(std::uint64_t id, std::size_t calls,
      const Executor& ex, Function& f)
    : trampoline_installer_state<Executor, InlineExceptionHandling>(ex),
      id_(id),
      calls_(calls),
      function_(f)
  {
  }

  void operator()()
  {
    this->install_and_execute(id_,
        static_cast<Function&&>(function_), calls_);
  }

  std::uint64_t id_;
  std::size_t calls_;
  Function& function_;
};

template <typename Executor>
inline constraint_t<
  can_require<const Executor&, execution::blocking_t::always_t>::value
    && can_query<const Executor&, execution::blocking_t>::value,
  bool
> trampoline_is_always_blocking(const Executor& ex)
{
  return asio::query(ex, execution::blocking)
    == execution::blocking.always;
}

template <typename Executor>
inline constraint_t<
  !(can_require<const Executor&, execution::blocking_t::always_t>::value
    && can_query<const Executor&, execution::blocking_t>::value),
  bool
> trampoline_is_always_blocking(const Executor&)
{
  return false;
}

inline void trampoline_count_call(trampoline_frame* current) noexcept
{
  if (current->max_calls != (std::numeric_limits<std::size_t>::max)())
    ++current->num_calls;
}

template <typename Function, typename Allocator>
inline void trampoline_enqueue(trampoline_frame* current,
    Function&& f, const Allocator& a, const char* op_name)
{
#if !defined(ASIO_ENABLE_HANDLER_TRACKING)
  (void)op_name;
#endif // !defined(ASIO_ENABLE_HANDLER_TRACKING)

  using function_type = decay_t<Function>;
  using op = executor_op<function_type, Allocator>;

  typename op::ptr p = {
    detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(static_cast<Function&&>(f), a);

  ASIO_HANDLER_CREATION((*p.p, "trampoline",
        current, 0, op_name));

  current->queue.push(p.p);
  p.v = p.p = 0;

  trampoline_count_call(current);
}

template <typename Allocator>
inline void trampoline_enqueue(trampoline_frame* current,
    executor_function&& f, const Allocator&, const char* op_name)
{
#if !defined(ASIO_ENABLE_HANDLER_TRACKING)
  (void)op_name;
#endif // !defined(ASIO_ENABLE_HANDLER_TRACKING)

  ASIO_HANDLER_CREATION((*f.get(), "trampoline",
        current, 0, op_name));

  current->queue.push(f.release());

  trampoline_count_call(current);
}

template <typename Executor>
inline enable_if_t<
    can_query<const Executor&, execution::allocator_t<void>>::value,
    query_result_t<const Executor&, execution::allocator_t<void>>>
trampoline_query_allocator(const Executor& ex)
{
  return asio::query(ex, execution::allocator);
}

template <typename Executor>
inline enable_if_t<
    !can_query<const Executor&, execution::allocator_t<void>>::value,
    std::allocator<void>>
trampoline_query_allocator(const Executor&) noexcept
{
  return std::allocator<void>();
}

// Submit a trampoline installer to a target executor that will not run inline.
template <typename InlineExceptionHandling,
    typename TargetExecutor, typename Function, typename Allocator>
inline void trampoline_submit_installer(const TargetExecutor& ex,
    std::uint64_t id, std::size_t max_calls, Function&& f, const Allocator&)
{
  trampoline_prefer(asio::require(ex, execution::blocking.never),
      execution::relationship.continuation).execute(
        trampoline_installer<decay_t<Function>, TargetExecutor,
          InlineExceptionHandling>(id, max_calls, ex,
            static_cast<Function&&>(f)));
}

template <typename InlineExceptionHandling,
    typename TargetExecutor, typename Allocator>
inline void trampoline_submit_installer(const TargetExecutor& ex,
    std::uint64_t id, std::size_t max_calls, executor_function&& f,
    const Allocator& a)
{
  typedef trampoline_install_op<Allocator, TargetExecutor,
      InlineExceptionHandling> op;
  typename op::ptr p = {
    detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(id, max_calls, f.get(), ex, a);

  f.release();
  operation* const installer = p.p;
  p.v = p.p = 0;

  trampoline_prefer(asio::require(ex, execution::blocking.never),
      execution::relationship.continuation).execute(
        executor_function(installer));
}

template <typename Executor, typename Allocator, typename Function,
    typename Blocking, typename InlineExceptionHandling>
void execute_via_trampoline(std::uint64_t id, const Executor& ex,
    Function&& f, std::size_t max_calls, std::size_t max_inline_depth,
    Blocking, InlineExceptionHandling, const Allocator& a)
{
  using function_type = decay_t<Function>;
  using installer_type = trampoline_installer<function_type,
      Executor, InlineExceptionHandling>;

  const bool is_blocking_never =
    is_same<Blocking, execution::blocking_t::never_t>::value;
  (void)max_inline_depth;

  trampoline_frame* current = trampoline_frame::current();

  if (current != 0)
  {
    if (current->installing_id != id)
    {
      trampoline_submit_installer<InlineExceptionHandling>(
          ex, id, max_calls, static_cast<Function&&>(f), a);
      return;
    }

    if (current->num_calls >= current->max_calls)
    {
      trampoline_bypass bypass(current);
      trampoline_submit_installer<InlineExceptionHandling>(
          ex, id, max_calls, static_cast<Function&&>(f), a);
      return;
    }

    if (!is_blocking_never && max_inline_depth != 0
        && current->inline_depth < max_inline_depth)
    {
      trampoline_count_call(current);
      trampoline_inline_depth_guard depth_guard(current);
      trampoline_run_immediately<InlineExceptionHandling>::call(
          current, ex, static_cast<Function&&>(f));
      return;
    }

    trampoline_enqueue(current, static_cast<Function&&>(f), a, "execute");
    return;
  }

  if (is_blocking_never)
  {
    trampoline_submit_installer<InlineExceptionHandling>(
        ex, id, max_calls, static_cast<Function&&>(f), a);
  }
  else if (!trampoline_frame::install_pending())
  {
    trampoline_pending_guard guard;
    if (trampoline_is_always_blocking(ex))
    {
      trampoline_prefer(ex, execution::relationship.continuation).execute(
          trampoline_installer_ref<function_type, Executor,
            InlineExceptionHandling>(id, max_calls, ex, f));
    }
    else
    {
      trampoline_prefer(ex, execution::relationship.continuation).execute(
          installer_type(id, max_calls, ex, static_cast<Function&&>(f)));
    }
  }
  else
  {
    trampoline_installer_state<Executor, InlineExceptionHandling>(ex)
      .install_and_execute(id, static_cast<Function&&>(f), max_calls);
  }
}

} // namespace detail

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
template <typename Executor1, typename>
trampoline<Executor, Blocking, InlineExceptionHandling>::trampoline(
    const Executor1& e, std::size_t calls, std::size_t max_inline_depth)
  : max_trampolined_calls_(calls),
    executor_(e),
    id_(detail::make_trampoline_id(
          detail::make_trampoline_id(), max_inline_depth))
{
}

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
trampoline<Executor, Blocking, InlineExceptionHandling>::trampoline(carry_id,
    const Executor& e, std::size_t calls, std::uint64_t id)
  : max_trampolined_calls_(calls),
    executor_(e),
    id_(id)
{
}

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
inline std::size_t trampoline<Executor, Blocking,
    InlineExceptionHandling>::max_inline_depth() const noexcept
{
  return detail::trampoline_depth(id_);
}

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
inline bool trampoline<Executor, Blocking,
    InlineExceptionHandling>::running_in_this_thread() const noexcept
{
  const detail::trampoline_frame* current = detail::trampoline_frame::current();
  return current != 0 && current->installing_id == id_;
}

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
inline trampoline<Executor, execution::blocking_t::possibly_t,
    InlineExceptionHandling>
trampoline<Executor, Blocking, InlineExceptionHandling>::require(
    const execution::blocking_t::possibly_t&) const noexcept
{
  typedef trampoline<Executor, execution::blocking_t::possibly_t,
      InlineExceptionHandling> result_type;
  return result_type(typename result_type::carry_id(),
      executor_, max_trampolined_calls_, id_);
}

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
inline trampoline<Executor, execution::blocking_t::never_t,
    InlineExceptionHandling>
trampoline<Executor, Blocking, InlineExceptionHandling>::require(
    const execution::blocking_t::never_t&) const noexcept
{
  typedef trampoline<Executor, execution::blocking_t::never_t,
      InlineExceptionHandling> result_type;
  return result_type(typename result_type::carry_id(),
      executor_, max_trampolined_calls_, id_);
}

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
inline trampoline<Executor, Blocking,
    execution::inline_exception_handling_t::propagate_t>
trampoline<Executor, Blocking, InlineExceptionHandling>::require(
    const execution::inline_exception_handling_t::propagate_t&) const noexcept
{
  typedef trampoline<Executor, Blocking,
      execution::inline_exception_handling_t::propagate_t> result_type;
  return result_type(typename result_type::carry_id(),
      executor_, max_trampolined_calls_, id_);
}

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
inline trampoline<Executor, Blocking,
    execution::inline_exception_handling_t::terminate_t>
trampoline<Executor, Blocking, InlineExceptionHandling>::require(
    const execution::inline_exception_handling_t::terminate_t&) const noexcept
{
  typedef trampoline<Executor, Blocking,
      execution::inline_exception_handling_t::terminate_t> result_type;
  return result_type(typename result_type::carry_id(),
      executor_, max_trampolined_calls_, id_);
}

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
template <typename Property>
trampoline<decay_t<require_result_t<const Executor&, Property>>,
    Blocking, InlineExceptionHandling>
trampoline<Executor, Blocking, InlineExceptionHandling>::require(
    const Property& p,
    constraint_t<can_require<const Executor&, Property>::value>,
    constraint_t<!is_convertible<Property, execution::blocking_t>::value>,
    constraint_t<
      !is_convertible<Property, execution::relationship_t>::value>,
    constraint_t<
      !is_convertible<Property,
        execution::inline_exception_handling_t>::value>) const
  noexcept(is_nothrow_require<const Executor&, Property>::value)
{
  typedef trampoline<
      decay_t<require_result_t<const Executor&, Property>>,
        Blocking, InlineExceptionHandling> result_type;
  return result_type(typename result_type::carry_id(),
      asio::require(executor_, p), max_trampolined_calls_, id_);
}

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
template <typename Property>
trampoline<decay_t<prefer_result_t<const Executor&, Property>>,
    Blocking, InlineExceptionHandling>
trampoline<Executor, Blocking, InlineExceptionHandling>::prefer(
    const Property& p,
    constraint_t<can_prefer<const Executor&, Property>::value>,
    constraint_t<
      !is_identity_prefer<const Executor&, Property>::value>,
    constraint_t<!is_convertible<Property, execution::blocking_t>::value>,
    constraint_t<
      !is_convertible<Property, execution::relationship_t>::value>,
    constraint_t<
      !is_convertible<Property,
        execution::inline_exception_handling_t>::value>) const
  noexcept(is_nothrow_prefer<const Executor&, Property>::value)
{
  typedef trampoline<
      decay_t<prefer_result_t<const Executor&, Property>>,
        Blocking, InlineExceptionHandling> result_type;
  return result_type(typename result_type::carry_id(),
      asio::prefer(executor_, p), max_trampolined_calls_, id_);
}

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
template <typename Function>
constraint_t<
  traits::execute_member<const Executor&, Function>::is_valid,
  void
> trampoline<Executor, Blocking, InlineExceptionHandling>::execute(
    Function&& f) const
{
  ASIO_INVOKE_INLINE detail::execute_via_trampoline(id_, executor_,
      static_cast<Function&&>(f), max_trampolined_calls_, max_inline_depth(),
      Blocking(), InlineExceptionHandling(),
      detail::trampoline_query_allocator(executor_));
}

#if !defined(ASIO_NO_TS_EXECUTORS)

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
template <typename Function, typename OtherAllocator>
void trampoline<Executor, Blocking, InlineExceptionHandling>::dispatch(
    Function&& f, const OtherAllocator& a) const
{
  detail::trampoline_frame* current
    = detail::trampoline_frame::current();

  if (current != 0)
  {
    if (current->installing_id != id_)
    {
      detail::trampoline_prefer(executor_,
          execution::relationship.continuation).dispatch(
            static_cast<Function&&>(f), a);
      return;
    }

    if (current->num_calls >= current->max_calls)
    {
      detail::trampoline_bypass bypass(current);
      detail::trampoline_prefer(executor_,
          execution::relationship.continuation).dispatch(
            static_cast<Function&&>(f), a);
      return;
    }

    detail::trampoline_enqueue(current,
        static_cast<Function&&>(f), a, "dispatch");
    return;
  }

  if (!detail::trampoline_frame::install_pending())
  {
    detail::trampoline_pending_guard guard;
    detail::trampoline_prefer(executor_,
        execution::relationship.continuation).dispatch(
          detail::trampoline_installer<decay_t<Function>, Executor,
            InlineExceptionHandling>(id_, max_trampolined_calls_,
              executor_, static_cast<Function&&>(f)), a);
    return;
  }

  detail::trampoline_installer_state<Executor, InlineExceptionHandling>(
      executor_).install_and_execute(id_, static_cast<Function&&>(f),
        max_trampolined_calls_);
}

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
template <typename Function, typename OtherAllocator>
void trampoline<Executor, Blocking, InlineExceptionHandling>::post(
    Function&& f, const OtherAllocator& a) const
{
  detail::trampoline_frame* current
    = detail::trampoline_frame::current();

  if (current != 0 && current->installing_id == id_)
  {
    if (current->num_calls < current->max_calls)
    {
      detail::trampoline_enqueue(current,
          static_cast<Function&&>(f), a, "post");
      return;
    }

    detail::trampoline_bypass bypass(current);
    detail::trampoline_prefer(executor_,
        execution::relationship.continuation).post(
          static_cast<Function&&>(f), a);
    return;
  }

  detail::trampoline_prefer(executor_,
      execution::relationship.continuation).post(
        static_cast<Function&&>(f), a);
}

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
template <typename Function, typename OtherAllocator>
void trampoline<Executor, Blocking, InlineExceptionHandling>::defer(
    Function&& f, const OtherAllocator& a) const
{
  detail::trampoline_frame* current
    = detail::trampoline_frame::current();

  if (current != 0 && current->installing_id == id_)
  {
    if (current->num_calls < current->max_calls)
    {
      detail::trampoline_enqueue(current,
          static_cast<Function&&>(f), a, "defer");
      return;
    }

    detail::trampoline_bypass bypass(current);
    detail::trampoline_prefer(executor_,
        execution::relationship.continuation).defer(
          static_cast<Function&&>(f), a);
    return;
  }

  detail::trampoline_prefer(executor_,
      execution::relationship.continuation).defer(
        static_cast<Function&&>(f), a);
}

#endif // !defined(ASIO_NO_TS_EXECUTORS)

#if !defined(GENERATING_DOCUMENTATION)

template <template <typename, typename> class Associator,
    typename Function, typename Executor,
    typename InlineExceptionHandling, typename DefaultCandidate>
struct associator<Associator,
    detail::trampoline_installer<Function, Executor, InlineExceptionHandling>,
    DefaultCandidate>
  : Associator<Function, DefaultCandidate>
{
  static typename Associator<Function, DefaultCandidate>::type get(
      const detail::trampoline_installer<Function, Executor,
        InlineExceptionHandling>& w) noexcept
  {
    return Associator<Function, DefaultCandidate>::get(w.function_);
  }

  static auto get(
      const detail::trampoline_installer<Function, Executor,
        InlineExceptionHandling>& w,
      const DefaultCandidate& c) noexcept
    -> decltype(Associator<Function, DefaultCandidate>::get(w.function_, c))
  {
    return Associator<Function, DefaultCandidate>::get(w.function_, c);
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_TRAMPOLINE_HPP
