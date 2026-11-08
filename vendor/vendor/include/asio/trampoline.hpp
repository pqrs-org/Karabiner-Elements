//
// trampoline.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_TRAMPOLINE_HPP
#define ASIO_TRAMPOLINE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <cstddef>
#include <cstdint>
#include "asio/detail/native_executors.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/execution.hpp"
#include "asio/execution/inline_exception_handling.hpp"
#include "asio/execution_context.hpp"
#include "asio/is_executor.hpp"
#include "asio/prefer.hpp"
#include "asio/require.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

// Create an identity for a new trampoline.
ASIO_DECL std::uint64_t make_trampoline_id() noexcept;

} // namespace detail

/// Performs trampoline execution within the adapted executor, establishing the
/// trampoline from inside that executor's own execution.
/**
 * The @c trampoline class template adapts an existing executor such that:
 *
 * @li Posted function objects (or when the @c blocking property is set to @c
 * blocking.never) are submitted to the target executor, unless a trampoline
 * with this executor's identity and with sufficient capacity is already
 * installed on the current thread, in which case they are queued on it.
 *
 * @li Dispatched function objects (or when @c blocking is @c blocking.possibly)
 * are submitted to the target executor if there is already a trampoline with
 * this executor's identity on the current thread, but it has already reached
 * its maximum number of calls. If there is such a trampoline with sufficient
 * capacity, the function object is queued to it. Otherwise the target executor
 * is given a wrapper that installs a new trampoline, and the function object
 * runs within it.
 *
 * A trampoline's identity is assigned once, when it is constructed directly or
 * via @c make_trampoline(). This identity is carried through copies,
 * conversions and require/prefer results. The identity is used to determine
 * whether a submitted function object is permitted to join an already-installed
 * trampoline.
 */
template <typename Executor,
    typename Blocking = execution::blocking_t::possibly_t,
    typename InlineExceptionHandling
      = execution::inline_exception_handling_t::propagate_t>
class trampoline
{
public:
  /// The type of the underlying executor.
  typedef Executor inner_executor_type;

  /// Default constructor.
  /**
   * This constructor is only valid if the underlying executor type is default
   * constructible. If this executor installs the trampoline, no additional
   * calls may be run through the trampoline. Allocates a fresh identity.
   */
  trampoline()
    : max_trampolined_calls_(0),
      executor_(),
      id_(detail::make_trampoline_id())
  {
  }

  /// Construct an executor for the specified target executor.
  /**
   * @param e The target executor.
   *
   * @param calls The maximum number of additional calls (i.e. excluding the
   * call that installs the trampoline) that can be run on the trampoline, if
   * this executor installs the trampoline. Pass
   * <tt>(std::numeric_limits<std::size_t>::max)()</tt> to specify that the
   * trampoline places no bound on the number of calls.
   *
   * @param max_inline_depth The deepest a @c blocking.possibly @c execute(), or
   * @c dispatch(), made from inside this trampoline's own frame may run
   * immediately rather than queue.
   *
   * @throws std::out_of_range if @c max_inline_depth is greater than 255.
   */
  template <typename Executor1,
      typename = constraint_t<
        conditional_t<
          !is_same<Executor1, trampoline>::value,
          is_convertible<Executor1, Executor>,
          false_type
        >::value
      >>
  trampoline(const Executor1& e, std::size_t calls,
      std::size_t max_inline_depth = 0);

  /// Copy constructor.
  trampoline(const trampoline& other) noexcept
    : max_trampolined_calls_(other.max_trampolined_calls_),
      executor_(other.executor_),
      id_(other.id_)
  {
  }

  /// Converting copy constructor.
  /**
   * This constructor is only valid if the @c OtherExecutor type is convertible
   * to @c Executor.
   */
  template <typename OtherExecutor>
  trampoline(
      const trampoline<OtherExecutor, Blocking,
        InlineExceptionHandling>& other) noexcept
    : max_trampolined_calls_(other.max_trampolined_calls_),
      executor_(other.executor_),
      id_(other.id_)
  {
  }

  /// Assignment operator.
  trampoline& operator=(
      const trampoline& other) noexcept
  {
    max_trampolined_calls_ = other.max_trampolined_calls_;
    executor_ = other.executor_;
    id_ = other.id_;
    return *this;
  }

  /// Converting assignment operator.
  /**
   * This assignment operator is only valid if the @c OtherExecutor type is
   * convertible to @c Executor.
   */
  template <typename OtherExecutor>
  trampoline& operator=(
      const trampoline<OtherExecutor, Blocking,
        InlineExceptionHandling>& other) noexcept
  {
    max_trampolined_calls_ = other.max_trampolined_calls_;
    executor_ = other.executor_;
    id_ = other.id_;
    return *this;
  }

  /// Move constructor.
  trampoline(trampoline&& other) noexcept
    : max_trampolined_calls_(other.max_trampolined_calls_),
      executor_(static_cast<Executor&&>(other.executor_)),
      id_(other.id_)
  {
  }

  /// Converting move constructor.
  /**
   * This constructor is only valid if the @c OtherExecutor type is convertible
   * to @c Executor.
   */
  template <typename OtherExecutor>
  trampoline(trampoline<OtherExecutor, Blocking,
      InlineExceptionHandling>&& other) noexcept
    : max_trampolined_calls_(other.max_trampolined_calls_),
      executor_(static_cast<OtherExecutor&&>(other.executor_)),
      id_(other.id_)
  {
  }

  /// Move assignment operator.
  trampoline& operator=(trampoline&& other) noexcept
  {
    max_trampolined_calls_ = other.max_trampolined_calls_;
    executor_ = static_cast<Executor&&>(other.executor_);
    id_ = other.id_;
    return *this;
  }

  /// Converting move assignment operator.
  /**
   * This assignment operator is only valid if the @c OtherExecutor type is
   * convertible to @c Executor.
   */
  template <typename OtherExecutor>
  trampoline& operator=(trampoline<OtherExecutor, Blocking,
      InlineExceptionHandling>&& other) noexcept
  {
    max_trampolined_calls_ = other.max_trampolined_calls_;
    executor_ = static_cast<OtherExecutor&&>(other.executor_);
    id_ = other.id_;
    return *this;
  }

  /// Destructor.
  ~trampoline() noexcept
  {
  }

  /// Obtain the underlying executor.
  inner_executor_type get_inner_executor() const noexcept
  {
    return executor_;
  }

  /// Obtain the number of calls that may be queued to a trampoline this
  /// executor installs.
  /**
   * The count is the number of function objects the trampoline runs in addition
   * to the one that installs the trampoline. A count of
   * <tt>(std::numeric_limits<std::size_t>::max)()</tt> means unbounded.
   *
   * The count applies only when this executor installs the trampoline. A
   * submission that joins a trampoline already installed on the current thread
   * runs under that trampoline's count instead, which may be larger or smaller
   * than this one.
   */
  std::size_t max_trampolined_calls() const noexcept
  {
    return max_trampolined_calls_;
  }

  /// Obtain the deepest a @c blocking.possibly @c execute() made from inside
  /// this trampoline's own frame may run immediately rather than queue.
  /**
   * A value of 0 means every dispatched function queues, exactly as if this
   * trampoline had never been given a @c max_inline_depth.
   */
  std::size_t max_inline_depth() const noexcept;

#if !defined(ASIO_NO_TS_EXECUTORS)
  /// Obtain the underlying execution context.
  execution_context& context() const noexcept
  {
    return executor_.context();
  }

  /// Inform the executor that it has some outstanding work.
  /**
   * The executor delegates this call to its underlying executor.
   */
  void on_work_started() const noexcept
  {
    executor_.on_work_started();
  }

  /// Inform the executor that some work is no longer outstanding.
  /**
   * The executor delegates this call to its underlying executor.
   */
  void on_work_finished() const noexcept
  {
    executor_.on_work_finished();
  }
#endif // !defined(ASIO_NO_TS_EXECUTORS)

  /// Determine whether a trampoline with this executor's identity is running in
  /// the current thread.
  /**
   * @return @c true if a trampoline frame is resident on this thread and its
   * identity equals this executor's identity. Otherwise returns @c false.
   */
  bool running_in_this_thread() const noexcept;

#if !defined(GENERATING_DOCUMENTATION)
private:
  friend struct ASIO_VERSIONED_NAME(require_fn)::impl;
  friend struct ASIO_VERSIONED_NAME(prefer_fn)::impl;
#endif // !defined(GENERATING_DOCUMENTATION)

  /// Obtain an executor with the @c blocking.possibly property.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::require customisation point.
   */
  trampoline<Executor, execution::blocking_t::possibly_t,
      InlineExceptionHandling>
  require(const execution::blocking_t::possibly_t&) const noexcept;

  /// Obtain an executor with the @c blocking.never property.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::require customisation point.
   */
  trampoline<Executor, execution::blocking_t::never_t,
      InlineExceptionHandling>
  require(const execution::blocking_t::never_t&) const noexcept;

  /// Obtain an executor with the @c inline_exception_handling.propagate
  /// property.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::require customisation point.
   *
   * For example:
   * @code auto ex1 = asio::make_trampoline(ioc, 4);
   * auto ex2 = asio::require(ex1,
   *     asio::execution::inline_exception_handling.propagate); @endcode
   */
  trampoline<Executor, Blocking,
      execution::inline_exception_handling_t::propagate_t>
  require(const execution::inline_exception_handling_t::propagate_t&)
    const noexcept;

  /// Obtain an executor with the @c inline_exception_handling.terminate
  /// property.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::require customisation point.
   *
   * For example:
   * @code auto ex1 = asio::make_trampoline(ioc, 4);
   * auto ex2 = asio::require(ex1,
   *     asio::execution::inline_exception_handling.terminate); @endcode
   */
  trampoline<Executor, Blocking,
      execution::inline_exception_handling_t::terminate_t>
  require(const execution::inline_exception_handling_t::terminate_t&)
    const noexcept;

  /// Forward a requirement to the underlying executor.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::require customisation point.
   */
  template <typename Property>
  trampoline<decay_t<require_result_t<const Executor&, Property>>,
      Blocking, InlineExceptionHandling>
  require(const Property& p,
      constraint_t<
        can_require<const Executor&, Property>::value
      > = 0,
      constraint_t<
        !is_convertible<Property, execution::blocking_t>::value
      > = 0,
      constraint_t<
        !is_convertible<Property, execution::relationship_t>::value
      > = 0,
      constraint_t<
        !is_convertible<Property,
          execution::inline_exception_handling_t>::value
      > = 0) const
    noexcept(is_nothrow_require<const Executor&, Property>::value);

  /// Forward a preference to the underlying executor.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::prefer customisation point.
   */
  template <typename Property>
  trampoline<decay_t<prefer_result_t<const Executor&, Property>>,
      Blocking, InlineExceptionHandling>
  prefer(const Property& p,
      constraint_t<
        can_prefer<const Executor&, Property>::value
      > = 0,
      constraint_t<
        !is_identity_prefer<const Executor&, Property>::value
      > = 0,
      constraint_t<
        !is_convertible<Property, execution::blocking_t>::value
      > = 0,
      constraint_t<
        !is_convertible<Property, execution::relationship_t>::value
      > = 0,
      constraint_t<
        !is_convertible<Property,
          execution::inline_exception_handling_t>::value
      > = 0) const
    noexcept(is_nothrow_prefer<const Executor&, Property>::value);

#if !defined(GENERATING_DOCUMENTATION)
private:
  friend struct ASIO_VERSIONED_NAME(query_fn)::impl;
  friend struct asio::execution::detail::blocking_t<0>;
  friend struct asio::execution::detail::relationship_t<0>;
  friend struct asio::execution::detail::inline_exception_handling_t<0>;
#endif // !defined(GENERATING_DOCUMENTATION)

  /// Query the current value of the @c blocking property.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::query customisation point.
   */
  static constexpr execution::blocking_t query(
      execution::blocking_t) noexcept
  {
    return Blocking();
  }

  /// Query the current value of the @c relationship property.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::query customisation point.
   *
   * Always returns @c relationship.continuation.
   */
  static constexpr execution::relationship_t::continuation_t query(
      execution::relationship_t) noexcept
  {
    return execution::relationship_t::continuation_t();
  }

  /// Query the current value of the @c inline_exception_handling property.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::query customisation point.
   *
   * For example:
   * @code auto ex = asio::make_trampoline(ioc, 4);
   * if (asio::query(ex,
   *       asio::execution::inline_exception_handling)
   *     == asio::execution::inline_exception_handling.propagate)
   *   ... @endcode
   */
  static constexpr execution::inline_exception_handling_t query(
      execution::inline_exception_handling_t) noexcept
  {
    return InlineExceptionHandling();
  }

  /// Forward a query to the underlying executor.
  /**
   * Do not call this function directly. It is intended for use with the
   * asio::query customisation point.
   */
  template <typename Property>
  query_result_t<const Executor&, Property> query(const Property& p,
      constraint_t<
        can_query<const Executor&, Property>::value
      > = 0,
      constraint_t<
        !is_convertible<Property, execution::blocking_t>::value
      > = 0,
      constraint_t<
        !is_convertible<Property, execution::relationship_t>::value
      > = 0,
      constraint_t<
        !is_convertible<Property,
          execution::inline_exception_handling_t>::value
      > = 0) const
    noexcept(is_nothrow_query<const Executor&, Property>::value)
  {
    return asio::query(executor_, p);
  }

public:
  /// Request the executor to invoke the given function object.
  /**
   * If there is a trampoline with this executor's identity installed on the
   * current thread, and it has sufficient capacity, the function object is
   * queued on it and invoked as the trampoline is drained. If the trampoline
   * has no more capacity, or has a different identity, the function object is
   * wrapped and submitted to the target executor, so that a trampoline is
   * installed within the target executor's context.
   *
   * If there is no trampoline, the function object wrapper is submitted using
   * the @c blocking.never property when this executor's blocking property is @c
   * blocking.never, and is otherwise handed to the target executor as it is,
   * which establishes the trampoline from within the target's own execution.
   *
   * The trampoline's @c inline_exception_handling property governs an
   * exception from every function object submitted through this function,
   * including one that the trampoline chooses to run immediately under @c
   * max_inline_depth. Whether a submitted function is queued or run
   * immediately is an optimisation that the caller cannot observe, and it
   * does not alter the handling of an exception that function throws.
   *
   * @param f The function object to be called. The function signature must be:
   * @code void function(); @endcode
   */
  template <typename Function>
  constraint_t<
    traits::execute_member<const Executor&, Function>::is_valid,
    void
  > execute(Function&& f) const;

#if !defined(ASIO_NO_TS_EXECUTORS)
  /// Request the executor to invoke the given function object.
  /**
   * If there is a trampoline with this executor's identity installed on the
   * current thread, and it has sufficient capacity, the function object is
   * queued on it. If the trampoline has no more capacity, or has a different
   * identity, the function object is submitted to the target executor's
   * dispatch function. If there is no trampoline, the target executor's
   * dispatch function is given a wrapper that establishes one from within the
   * target's own execution.
   *
   * @param f The function object to be called.
   * @param a An allocator that may be used by the executor.
   */
  template <typename Function, typename OtherAllocator>
  void dispatch(Function&& f, const OtherAllocator& a) const;

  /// Request the executor to invoke the given function object.
  /**
   * If there is a trampoline with this executor's identity installed on the
   * current thread, and it has sufficient capacity, the function object is
   * queued on it. Otherwise the function object is submitted to the target
   * executor's post function.
   *
   * @param f The function object to be called.
   * @param a An allocator that may be used by the executor.
   */
  template <typename Function, typename OtherAllocator>
  void post(Function&& f, const OtherAllocator& a) const;

  /// Request the executor to invoke the given function object.
  /**
   * If there is a trampoline with this executor's identity installed on the
   * current thread, and it has sufficient capacity, the function object is
   * queued on it. Otherwise the function object is submitted to the target
   * executor's defer function.
   *
   * @param f The function object to be called.
   * @param a An allocator that may be used by the executor.
   */
  template <typename Function, typename OtherAllocator>
  void defer(Function&& f, const OtherAllocator& a) const;
#endif // !defined(ASIO_NO_TS_EXECUTORS)

  /// Compare two executors for equality.
  /**
   * Two trampoline executors are equal if they have the same identity and the
   * same underlying executor.
   */
  friend bool operator==(const trampoline& a,
      const trampoline& b) noexcept
  {
    return a.id_ == b.id_ && a.executor_ == b.executor_;
  }

  /// Compare two executors for inequality.
  /**
   * Two trampoline executors are unequal if they have different identities or
   * different underlying executors.
   */
  friend bool operator!=(const trampoline& a,
      const trampoline& b) noexcept
  {
    return !(a == b);
  }

private:
  template <typename, typename, typename>
  friend class trampoline;

  // Tag type used to for constructor disambiguation.
  struct carry_id
  {
  };

  // Construct an executor for the specified target executor and identity.
  trampoline(carry_id, const Executor& e, std::size_t calls, std::uint64_t id);

  std::size_t max_trampolined_calls_;
  Executor executor_;
  std::uint64_t id_;
};

/** @defgroup make_trampoline asio::make_trampoline
 *
 * @brief The asio::make_trampoline function creates a @ref trampoline
 * object for an executor or execution context.
 */
/*@{*/

/// Create a @ref trampoline object for an executor.
/**
 * @param ex The target executor.
 *
 * @param calls The maximum number of additional calls (i.e. excluding the call
 * that installs the trampoline) that can be run on the trampoline, if the
 * returned executor installs the trampoline. Pass
 * <tt>(std::numeric_limits<std::size_t>::max)()</tt> to specify that the
 * trampoline places no bound on the number of calls.
 *
 * @param max_inline_depth The deepest a @c blocking.possibly @c execute()
 * made from inside this trampoline's own frame may run immediately rather
 * than queue, counting the nesting of such immediate calls already
 * underway. A nested function that runs this way runs before the remainder
 * of its caller, ahead of whatever is already queued on the frame. It still
 * spends one of @c calls, the same as a queued call. A value of 0, the
 * default, preserves today's behaviour: every dispatched function queues
 * and runs only as the frame is drained.
 *
 * @throws std::out_of_range if @c max_inline_depth is greater than 255.
 *
 * @returns A @c trampoline constructed with the specified
 * executor, and a freshly allocated identity. Passing an existing @c
 * trampoline as @c ex is not a special case. As with @c make_strand
 * wrapping an existing strand, it creates a new trampoline, with its own
 * fresh identity, over that one as its target. The two do not join each
 * other's trampolines.
 */
template <typename Executor,
    typename = constraint_t<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    >>
inline trampoline<Executor> make_trampoline(
    const Executor& ex, std::size_t calls, std::size_t max_inline_depth = 0)
{
  return trampoline<Executor>(ex, calls, max_inline_depth);
}

/// Create a @ref trampoline object for an execution context.
/**
 * @param ctx An execution context, from which an executor will be obtained.
 *
 * @param calls The maximum number of additional calls (i.e. excluding the call
 * that installs the trampoline) that can be run on the trampoline, if the
 * returned executor installs the trampoline. Pass
 * <tt>(std::numeric_limits<std::size_t>::max)()</tt> to specify that the
 * trampoline places no bound on the number of calls.
 *
 * @param max_inline_depth The deepest a @c blocking.possibly @c execute()
 * made from inside this trampoline's own frame may run immediately rather
 * than queue, counting the nesting of such immediate calls already
 * underway. A nested function that runs this way runs before the remainder
 * of its caller, ahead of whatever is already queued on the frame. It still
 * spends one of @c calls, the same as a queued call. A value of 0, the
 * default, preserves today's behaviour: every dispatched function queues
 * and runs only as the frame is drained.
 *
 * @throws std::out_of_range if @c max_inline_depth is greater than 255.
 *
 * @returns A trampoline constructed with the execution context's
 * executor, obtained by performing <tt>ctx.get_executor()</tt>, and a freshly
 * allocated identity.
 */
template <typename ExecutionContext,
    typename = constraint_t<
      is_convertible<ExecutionContext&, execution_context&>::value
    >>
inline trampoline<typename ExecutionContext::executor_type>
make_trampoline(ExecutionContext& ctx,
    std::size_t calls, std::size_t max_inline_depth = 0)
{
  return trampoline<typename ExecutionContext::executor_type>(
      ctx.get_executor(), calls, max_inline_depth);
}

/*@}*/

#if !defined(GENERATING_DOCUMENTATION)

namespace detail {

// A trampoline forwards every property it is given on to its target
// executor, so its outstanding work is counted natively exactly when the
// target's is. See the identical specialisation for strand in strand.hpp.
// Any other answer, polymorphic included, collapses to none: the run-time
// test for a polymorphic executor calls target_type(), which a trampoline
// lacks. A trampoline is never natively executed: a completion handler must
// always be run through the trampoline itself, whatever the trampoline
// wraps.
template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
struct native_outstanding_work_kind<
    trampoline<Executor, Blocking, InlineExceptionHandling>>
{
  using type = conditional_t<
      is_same<
        native_outstanding_work_kind_t<Executor>,
        native_outstanding_work_kinds::native
      >::value,
      native_outstanding_work_kinds::native,
      native_outstanding_work_kinds::none
    >;
};

} // namespace detail

namespace traits {

#if !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
struct equality_comparable<
    trampoline<Executor, Blocking, InlineExceptionHandling>>
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;
};

#endif // !defined(ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling, typename Function>
struct execute_member<
    trampoline<Executor, Blocking, InlineExceptionHandling>,
    Function,
    enable_if_t<
      traits::execute_member<const Executor&, Function>::is_valid
    >
  >
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = false;
  typedef void result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling, typename Property>
struct query_static_constexpr_member<
    trampoline<Executor, Blocking, InlineExceptionHandling>,
    Property,
    enable_if_t<
      is_convertible<Property, execution::blocking_t>::value
    >
  >
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;
  typedef Blocking result_type;

  static constexpr result_type value() noexcept
  {
    return result_type();
  }
};

// A trampoline's relationship is always relationship.continuation: see the
// class documentation.
template <typename Executor, typename Blocking,
    typename InlineExceptionHandling, typename Property>
struct query_static_constexpr_member<
    trampoline<Executor, Blocking, InlineExceptionHandling>,
    Property,
    enable_if_t<
      is_convertible<Property, execution::relationship_t>::value
    >
  >
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;
  typedef execution::relationship_t::continuation_t result_type;

  static constexpr result_type value() noexcept
  {
    return result_type();
  }
};

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling, typename Property>
struct query_static_constexpr_member<
    trampoline<Executor, Blocking, InlineExceptionHandling>,
    Property,
    enable_if_t<
      is_convertible<Property, execution::inline_exception_handling_t>::value
    >
  >
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;
  typedef InlineExceptionHandling result_type;

  static constexpr result_type value() noexcept
  {
    return result_type();
  }
};

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling, typename Property>
struct query_member<
    trampoline<Executor, Blocking, InlineExceptionHandling>,
    Property,
    enable_if_t<
      can_query<const Executor&, Property>::value
        && !is_convertible<Property, execution::blocking_t>::value
        && !is_convertible<Property, execution::relationship_t>::value
        && !is_convertible<Property,
          execution::inline_exception_handling_t>::value
    >
  >
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept =
    is_nothrow_query<Executor, Property>::value;
  typedef query_result_t<Executor, Property> result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
struct require_member<
    trampoline<Executor, Blocking, InlineExceptionHandling>,
    execution::blocking_t::possibly_t
  >
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;
  typedef trampoline<Executor, execution::blocking_t::possibly_t,
      InlineExceptionHandling> result_type;
};

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
struct require_member<
    trampoline<Executor, Blocking, InlineExceptionHandling>,
    execution::blocking_t::never_t
  >
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;
  typedef trampoline<Executor, execution::blocking_t::never_t,
      InlineExceptionHandling> result_type;
};

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
struct require_member<
    trampoline<Executor, Blocking, InlineExceptionHandling>,
    execution::inline_exception_handling_t::propagate_t
  >
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;
  typedef trampoline<Executor, Blocking,
      execution::inline_exception_handling_t::propagate_t> result_type;
};

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling>
struct require_member<
    trampoline<Executor, Blocking, InlineExceptionHandling>,
    execution::inline_exception_handling_t::terminate_t
  >
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept = true;
  typedef trampoline<Executor, Blocking,
      execution::inline_exception_handling_t::terminate_t> result_type;
};

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling, typename Property>
struct require_member<
    trampoline<Executor, Blocking, InlineExceptionHandling>,
    Property,
    enable_if_t<
      can_require<const Executor&, Property>::value
        && !is_convertible<Property, execution::blocking_t>::value
        && !is_convertible<Property, execution::relationship_t>::value
        && !is_convertible<Property,
          execution::inline_exception_handling_t>::value
    >
  >
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept =
    is_nothrow_require<const Executor&, Property>::value;
  typedef trampoline<
      decay_t<require_result_t<const Executor&, Property>>,
        Blocking, InlineExceptionHandling> result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

#if !defined(ASIO_HAS_DEDUCED_PREFER_MEMBER_TRAIT)

template <typename Executor, typename Blocking,
    typename InlineExceptionHandling, typename Property>
struct prefer_member<
    trampoline<Executor, Blocking, InlineExceptionHandling>,
    Property,
    enable_if_t<
      can_prefer<const Executor&, Property>::value
        && !is_identity_prefer<const Executor&, Property>::value
        && !is_convertible<Property, execution::blocking_t>::value
        && !is_convertible<Property, execution::relationship_t>::value
        && !is_convertible<Property,
          execution::inline_exception_handling_t>::value
    >
  >
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept =
    is_nothrow_prefer<const Executor&, Property>::value;
  typedef trampoline<
      decay_t<prefer_result_t<const Executor&, Property>>,
        Blocking, InlineExceptionHandling> result_type;
};

#endif // !defined(ASIO_HAS_DEDUCED_PREFER_MEMBER_TRAIT)

} // namespace traits

#endif // !defined(GENERATING_DOCUMENTATION)

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#include "asio/impl/trampoline.hpp"
#if defined(ASIO_HEADER_ONLY)
# include "asio/impl/trampoline.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // ASIO_TRAMPOLINE_HPP
