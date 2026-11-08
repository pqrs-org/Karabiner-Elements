//
// spawn.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_SPAWN_HPP
#define ASIO_SPAWN_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_BOOST_CONTEXT_FIBER) \
  || defined(ASIO_HAS_ASM_FIBER) \
  || defined(GENERATING_DOCUMENTATION)

#include "asio/any_io_executor.hpp"
#include "asio/cancellation_signal.hpp"
#include "asio/cancellation_state.hpp"
#include "asio/detail/exception.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/io_context.hpp"
#include "asio/is_executor.hpp"
#include "asio/strand.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

class spawned_thread;

template <typename T>
struct spawn_signature
{
  typedef void type(exception_ptr, T);
};

template <>
struct spawn_signature<void>
{
  typedef void type(exception_ptr);
};

template <typename Executor>
class initiate_spawn;

} // namespace detail

/// A @ref completion_token that represents the currently executing coroutine.
/**
 * The basic_yield_context class is a completion token type that is used to
 * represent the currently executing stackful coroutine. A basic_yield_context
 * object may be passed as a completion token to an asynchronous operation. For
 * example:
 *
 * @code template <typename Executor>
 * void my_coroutine(basic_yield_context<Executor> yield)
 * {
 *   ...
 *   std::size_t n = my_socket.async_read_some(buffer, yield);
 *   ...
 * } @endcode
 *
 * The initiating function (async_read_some in the above example) suspends the
 * current coroutine. The coroutine is resumed when the asynchronous operation
 * completes, and the result of the operation is returned.
 */
template <typename Executor>
class basic_yield_context
{
public:
  /// The executor type associated with the yield context.
  typedef Executor executor_type;

  /// The cancellation slot type associated with the yield context.
  typedef cancellation_slot cancellation_slot_type;

  /// Construct a yield context from another yield context type.
  /**
   * Requires that OtherExecutor be convertible to Executor.
   */
  template <typename OtherExecutor>
  basic_yield_context(const basic_yield_context<OtherExecutor>& other,
      constraint_t<
        is_convertible<OtherExecutor, Executor>::value
      > = 0)
    : spawned_thread_(other.spawned_thread_),
      executor_(other.executor_),
      ec_(other.ec_)
  {
  }

  /// Get the executor associated with the yield context.
  executor_type get_executor() const noexcept
  {
    return executor_;
  }

  /// Get the cancellation slot associated with the coroutine.
  cancellation_slot_type get_cancellation_slot() const noexcept;

  /// Get the cancellation state associated with the coroutine.
  cancellation_state get_cancellation_state() const noexcept;

  /// Reset the cancellation state associated with the coroutine.
  /**
   * Let <tt>P</tt> be the cancellation slot associated with the current
   * coroutine's @ref spawn completion handler. Assigns a new
   * asio::cancellation_state object <tt>S</tt>, constructed as
   * <tt>S(P)</tt>, into the current coroutine's cancellation state object.
   */
  void reset_cancellation_state() const;

  /// Reset the cancellation state associated with the coroutine.
  /**
   * Let <tt>P</tt> be the cancellation slot associated with the current
   * coroutine's @ref spawn completion handler. Assigns a new
   * asio::cancellation_state object <tt>S</tt>, constructed as <tt>S(P,
   * std::forward<Filter>(filter))</tt>, into the current coroutine's
   * cancellation state object.
   */
  template <typename Filter>
  void reset_cancellation_state(Filter&& filter) const;

  /// Reset the cancellation state associated with the coroutine.
  /**
   * Let <tt>P</tt> be the cancellation slot associated with the current
   * coroutine's @ref spawn completion handler. Assigns a new
   * asio::cancellation_state object <tt>S</tt>, constructed as <tt>S(P,
   * std::forward<InFilter>(in_filter),
   * std::forward<OutFilter>(out_filter))</tt>, into the current coroutine's
   * cancellation state object.
   */
  template <typename InFilter, typename OutFilter>
  void reset_cancellation_state(InFilter&& in_filter,
      OutFilter&& out_filter) const;

  /// Determine whether the current coroutine has been cancelled.
  cancellation_type_t cancelled() const noexcept;

  /// Determine whether the coroutine throws if trying to suspend when it has
  /// been cancelled.
  bool throw_if_cancelled() const noexcept;

  /// Set whether the coroutine throws if trying to suspend when it has been
  /// cancelled.
  void throw_if_cancelled(bool value) const noexcept;

  /// Return a yield context that sets the specified error_code.
  /**
   * By default, when a yield context is used with an asynchronous operation, a
   * non-success error_code is converted to system_error and thrown. This
   * operator may be used to specify an error_code object that should instead be
   * set with the asynchronous operation's result. For example:
   *
   * @code template <typename Executor>
   * void my_coroutine(basic_yield_context<Executor> yield)
   * {
   *   ...
   *   std::size_t n = my_socket.async_read_some(buffer, yield[ec]);
   *   if (ec)
   *   {
   *     // An error occurred.
   *   }
   *   ...
   * } @endcode
   */
  basic_yield_context operator[](asio::error_code& ec) const
  {
    basic_yield_context tmp(*this);
    tmp.ec_ = &ec;
    return tmp;
  }

#if !defined(GENERATING_DOCUMENTATION)
//private:
  basic_yield_context(detail::spawned_thread* t, const Executor& ex)
    : spawned_thread_(t),
      executor_(ex),
      ec_(0)
  {
  }

  detail::spawned_thread* spawned_thread_;
  Executor executor_;
  asio::error_code* ec_;
#endif // !defined(GENERATING_DOCUMENTATION)
};

/// A @ref completion_token object that represents the currently executing
/// coroutine.
typedef basic_yield_context<any_io_executor> yield_context;

/**
 * @defgroup spawn asio::spawn
 *
 * @brief Start a new stackful coroutine.
 *
 * @sa @ref overview_spawn "Stackful coroutines"
 *
 * The spawn() function is a high-level wrapper over the Boost.Coroutine
 * library. This function enables programs to implement asynchronous logic in a
 * synchronous manner, as illustrated by the following example:
 *
 * @code asio::spawn(my_strand, do_echo, asio::detached);
 *
 * // ...
 *
 * void do_echo(asio::yield_context yield)
 * {
 *   try
 *   {
 *     char data[128];
 *     for (;;)
 *     {
 *       std::size_t length =
 *         my_socket.async_read_some(
 *           asio::buffer(data), yield);
 *
 *       asio::async_write(my_socket,
 *           asio::buffer(data, length), yield);
 *     }
 *   }
 *   catch (std::exception& e)
 *   {
 *     // ...
 *   }
 * } @endcode
 */
/*@{*/

/// Start a new stackful coroutine that executes on a given executor.
/**
 * This function is used to launch a new stackful coroutine.
 *
 * @param ex Identifies the executor that will run the stackful coroutine.
 *
 * @param function The coroutine function. The function must be callable the
 * signature:
 * @code void function(basic_yield_context<Executor> yield); @endcode
 *
 * @param token The @ref completion_token that will handle the notification
 * that the coroutine has completed. If the return type @c R of @c function is
 * @c void, the function signature of the completion handler must be:
 *
 * @code void handler(std::exception_ptr); @endcode
 * Otherwise, the function signature of the completion handler must be:
 * @code void handler(std::exception_ptr, R); @endcode
 *
 * @par Completion Signature
 * @code void(std::exception_ptr, R) @endcode
 * where @c R is the return type of the function object.
 *
 * @par Per-Operation Cancellation
 * The new thread of execution is created with a cancellation state that
 * supports @c cancellation_type::terminal values only. To change the
 * cancellation state, call the basic_yield_context member function
 * @c reset_cancellation_state.
 */
template <typename Executor, typename F,
    ASIO_COMPLETION_TOKEN_FOR(typename detail::spawn_signature<
      result_of_t<F(basic_yield_context<Executor>)>>::type)
        CompletionToken = default_completion_token_t<Executor>>
auto spawn(const Executor& ex, F&& function,
    CompletionToken&& token = default_completion_token_t<Executor>(),
    constraint_t<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    > = 0)
  -> decltype(
    async_initiate<CompletionToken,
      typename detail::spawn_signature<
        result_of_t<F(basic_yield_context<Executor>)>>::type>(
          declval<detail::initiate_spawn<Executor>>(),
          token, static_cast<F&&>(function)));

/// Start a new stackful coroutine that executes on a given execution context.
/**
 * This function is used to launch a new stackful coroutine.
 *
 * @param ctx Identifies the execution context that will run the stackful
 * coroutine.
 *
 * @param function The coroutine function. The function must be callable the
 * signature:
 * @code void function(basic_yield_context<Executor> yield); @endcode
 *
 * @param token The @ref completion_token that will handle the notification
 * that the coroutine has completed. If the return type @c R of @c function is
 * @c void, the function signature of the completion handler must be:
 *
 * @code void handler(std::exception_ptr); @endcode
 * Otherwise, the function signature of the completion handler must be:
 * @code void handler(std::exception_ptr, R); @endcode
 *
 * @par Completion Signature
 * @code void(std::exception_ptr, R) @endcode
 * where @c R is the return type of the function object.
 *
 * @par Per-Operation Cancellation
 * The new thread of execution is created with a cancellation state that
 * supports @c cancellation_type::terminal values only. To change the
 * cancellation state, call the basic_yield_context member function
 * @c reset_cancellation_state.
 */
template <typename ExecutionContext, typename F,
    ASIO_COMPLETION_TOKEN_FOR(typename detail::spawn_signature<
      result_of_t<F(basic_yield_context<
        typename ExecutionContext::executor_type>)>>::type)
          CompletionToken = default_completion_token_t<
            typename ExecutionContext::executor_type>>
auto spawn(ExecutionContext& ctx, F&& function,
    CompletionToken&& token
      = default_completion_token_t<typename ExecutionContext::executor_type>(),
    constraint_t<
      is_convertible<ExecutionContext&, execution_context&>::value
    > = 0)
  -> decltype(
    async_initiate<CompletionToken,
      typename detail::spawn_signature<
        result_of_t<F(basic_yield_context<
          typename ExecutionContext::executor_type>)>>::type>(
            declval<detail::initiate_spawn<
              typename ExecutionContext::executor_type>>(),
            token, static_cast<F&&>(function)));

/// Start a new stackful coroutine, inheriting the executor of another.
/**
 * This function is used to launch a new stackful coroutine.
 *
 * @param ctx Identifies the current coroutine as a parent of the new
 * coroutine. This specifies that the new coroutine should inherit the executor
 * of the parent. For example, if the parent coroutine is executing in a
 * particular strand, then the new coroutine will execute in the same strand.
 *
 * @param function The coroutine function. The function must be callable the
 * signature:
 * @code void function(basic_yield_context<Executor> yield); @endcode
 *
 * @param token The @ref completion_token that will handle the notification
 * that the coroutine has completed. If the return type @c R of @c function is
 * @c void, the function signature of the completion handler must be:
 *
 * @code void handler(std::exception_ptr); @endcode
 * Otherwise, the function signature of the completion handler must be:
 * @code void handler(std::exception_ptr, R); @endcode
 *
 * @par Completion Signature
 * @code void(std::exception_ptr, R) @endcode
 * where @c R is the return type of the function object.
 *
 * @par Per-Operation Cancellation
 * The new thread of execution is created with a cancellation state that
 * supports @c cancellation_type::terminal values only. To change the
 * cancellation state, call the basic_yield_context member function
 * @c reset_cancellation_state.
 */
template <typename Executor, typename F,
    ASIO_COMPLETION_TOKEN_FOR(typename detail::spawn_signature<
      result_of_t<F(basic_yield_context<Executor>)>>::type)
        CompletionToken = default_completion_token_t<Executor>>
auto spawn(const basic_yield_context<Executor>& ctx, F&& function,
    CompletionToken&& token = default_completion_token_t<Executor>(),
    constraint_t<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    > = 0)
  -> decltype(
    async_initiate<CompletionToken,
      typename detail::spawn_signature<
        result_of_t<F(basic_yield_context<Executor>)>>::type>(
          declval<detail::initiate_spawn<Executor>>(),
          token, static_cast<F&&>(function)));

/// Start a new stackful coroutine that executes on a given executor.
/**
 * This function is used to launch a new stackful coroutine using the
 * specified stack allocator.
 *
 * @param ex Identifies the executor that will run the stackful coroutine.
 *
 * @param stack_allocator Denotes the allocator to be used to allocate the
 * underlying coroutine's stack. The type must meet the StackAllocator
 * requirements.
 *
 * @param function The coroutine function. The function must be callable the
 * signature:
 * @code void function(basic_yield_context<Executor> yield); @endcode
 *
 * @param token The @ref completion_token that will handle the notification
 * that the coroutine has completed. If the return type @c R of @c function is
 * @c void, the function signature of the completion handler must be:
 *
 * @code void handler(std::exception_ptr); @endcode
 * Otherwise, the function signature of the completion handler must be:
 * @code void handler(std::exception_ptr, R); @endcode
 *
 * @par Completion Signature
 * @code void(std::exception_ptr, R) @endcode
 * where @c R is the return type of the function object.
 *
 * @par Per-Operation Cancellation
 * The new thread of execution is created with a cancellation state that
 * supports @c cancellation_type::terminal values only. To change the
 * cancellation state, call the basic_yield_context member function
 * @c reset_cancellation_state.
 */
template <typename Executor, typename StackAllocator, typename F,
    ASIO_COMPLETION_TOKEN_FOR(typename detail::spawn_signature<
      result_of_t<F(basic_yield_context<Executor>)>>::type)
        CompletionToken = default_completion_token_t<Executor>>
auto spawn(const Executor& ex, allocator_arg_t,
    StackAllocator&& stack_allocator, F&& function,
    CompletionToken&& token = default_completion_token_t<Executor>(),
    constraint_t<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    > = 0)
  -> decltype(
    async_initiate<CompletionToken,
      typename detail::spawn_signature<
        result_of_t<F(basic_yield_context<Executor>)>>::type>(
          declval<detail::initiate_spawn<Executor>>(),
          token, allocator_arg_t(),
          static_cast<StackAllocator&&>(stack_allocator),
          static_cast<F&&>(function)));

/// Start a new stackful coroutine that executes on a given execution context.
/**
 * This function is used to launch a new stackful coroutine.
 *
 * @param ctx Identifies the execution context that will run the stackful
 * coroutine.
 *
 * @param stack_allocator Denotes the allocator to be used to allocate the
 * underlying coroutine's stack. The type must meet the StackAllocator
 * requirements.
 *
 * @param function The coroutine function. The function must be callable the
 * signature:
 * @code void function(basic_yield_context<Executor> yield); @endcode
 *
 * @param token The @ref completion_token that will handle the notification
 * that the coroutine has completed. If the return type @c R of @c function is
 * @c void, the function signature of the completion handler must be:
 *
 * @code void handler(std::exception_ptr); @endcode
 * Otherwise, the function signature of the completion handler must be:
 * @code void handler(std::exception_ptr, R); @endcode
 *
 * @par Completion Signature
 * @code void(std::exception_ptr, R) @endcode
 * where @c R is the return type of the function object.
 *
 * @par Per-Operation Cancellation
 * The new thread of execution is created with a cancellation state that
 * supports @c cancellation_type::terminal values only. To change the
 * cancellation state, call the basic_yield_context member function
 * @c reset_cancellation_state.
 */
template <typename ExecutionContext, typename StackAllocator, typename F,
    ASIO_COMPLETION_TOKEN_FOR(typename detail::spawn_signature<
      result_of_t<F(basic_yield_context<
        typename ExecutionContext::executor_type>)>>::type)
          CompletionToken = default_completion_token_t<
            typename ExecutionContext::executor_type>>
auto spawn(ExecutionContext& ctx, allocator_arg_t,
    StackAllocator&& stack_allocator, F&& function,
    CompletionToken&& token
      = default_completion_token_t<typename ExecutionContext::executor_type>(),
    constraint_t<
      is_convertible<ExecutionContext&, execution_context&>::value
    > = 0)
  -> decltype(
    async_initiate<CompletionToken,
      typename detail::spawn_signature<
        result_of_t<F(basic_yield_context<
          typename ExecutionContext::executor_type>)>>::type>(
            declval<detail::initiate_spawn<
              typename ExecutionContext::executor_type>>(),
            token, allocator_arg_t(),
            static_cast<StackAllocator&&>(stack_allocator),
            static_cast<F&&>(function)));

/// Start a new stackful coroutine, inheriting the executor of another.
/**
 * This function is used to launch a new stackful coroutine using the
 * specified stack allocator.
 *
 * @param ctx Identifies the current coroutine as a parent of the new
 * coroutine. This specifies that the new coroutine should inherit the
 * executor of the parent. For example, if the parent coroutine is executing
 * in a particular strand, then the new coroutine will execute in the same
 * strand.
 *
 * @param stack_allocator Denotes the allocator to be used to allocate the
 * underlying coroutine's stack. The type must meet the StackAllocator
 * requirements.
 *
 * @param function The coroutine function. The function must be callable the
 * signature:
 * @code void function(basic_yield_context<Executor> yield); @endcode
 *
 * @param token The @ref completion_token that will handle the notification
 * that the coroutine has completed. If the return type @c R of @c function is
 * @c void, the function signature of the completion handler must be:
 *
 * @code void handler(std::exception_ptr); @endcode
 * Otherwise, the function signature of the completion handler must be:
 * @code void handler(std::exception_ptr, R); @endcode
 *
 * @par Completion Signature
 * @code void(std::exception_ptr, R) @endcode
 * where @c R is the return type of the function object.
 *
 * @par Per-Operation Cancellation
 * The new thread of execution is created with a cancellation state that
 * supports @c cancellation_type::terminal values only. To change the
 * cancellation state, call the basic_yield_context member function
 * @c reset_cancellation_state.
 */
template <typename Executor, typename StackAllocator, typename F,
    ASIO_COMPLETION_TOKEN_FOR(typename detail::spawn_signature<
      result_of_t<F(basic_yield_context<Executor>)>>::type)
        CompletionToken = default_completion_token_t<Executor>>
auto spawn(const basic_yield_context<Executor>& ctx, allocator_arg_t,
    StackAllocator&& stack_allocator, F&& function,
    CompletionToken&& token = default_completion_token_t<Executor>(),
    constraint_t<
      is_executor<Executor>::value || execution::is_executor<Executor>::value
    > = 0)
  -> decltype(
    async_initiate<CompletionToken,
      typename detail::spawn_signature<
        result_of_t<F(basic_yield_context<Executor>)>>::type>(
          declval<detail::initiate_spawn<Executor>>(),
          token, allocator_arg_t(),
          static_cast<StackAllocator&&>(stack_allocator),
          static_cast<F&&>(function)));

/*@}*/

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#include "asio/impl/spawn.hpp"

#else // defined(ASIO_HAS_BOOST_CONTEXT_FIBER)
      //   || defined(ASIO_HAS_ASM_FIBER)
      //   || defined(GENERATING_DOCUMENTATION)
# error No spawn() implementation available
#endif // defined(ASIO_HAS_BOOST_CONTEXT_FIBER)
       //   || defined(ASIO_HAS_ASM_FIBER)
       //   || defined(GENERATING_DOCUMENTATION)

#endif // ASIO_SPAWN_HPP
