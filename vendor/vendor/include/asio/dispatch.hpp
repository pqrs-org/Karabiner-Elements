//
// dispatch.hpp
// ~~~~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DISPATCH_HPP
#define ASIO_DISPATCH_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/async_result.hpp"
#include "asio/detail/initiate_dispatch.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/execution_context.hpp"
#include "asio/execution/executor.hpp"
#include "asio/is_executor.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

/// Submits a completion token or function object for execution.
/**
 * This function submits an object for execution using the object's associated
 * executor. The function object may be called from the current thread prior to
 * returning from <tt>dispatch()</tt>. Otherwise, it is queued for execution.
 *
 * @param token The @ref completion_token that will be used to produce a
 * completion handler. The function signature of the completion handler must be:
 * @code void handler(); @endcode
 *
 * @returns This function returns <tt>async_initiate<NullaryToken,
 * void()>(Init{}, token)</tt>, where @c Init is a function object type defined
 * as:
 *
 * @code class Init
 * {
 * public:
 *   template <typename CompletionHandler>
 *     void operator()(CompletionHandler&& completion_handler) const;
 * }; @endcode
 *
 * The function call operator of @c Init:
 *
 * @li Obtains the handler's associated executor object @c ex of type @c Ex by
 * performing
 * @code auto ex = get_associated_executor(completion_handler); @endcode
 *
 * @li Obtains the handler's associated allocator object @c alloc by performing
 * @code auto alloc = get_associated_allocator(completion_handler); @endcode
 *
 * @li If <tt>execution::is_executor<Ex>::value</tt> is true, performs
 * @code prefer(ex, execution::allocator(alloc)).execute(
 *     std::forward<CompletionHandler>(completion_handler)); @endcode
 *
 * @li If <tt>execution::is_executor<Ex>::value</tt> is false, performs
 * @code ex.dispatch(
 *     std::forward<CompletionHandler>(completion_handler),
 *     alloc); @endcode
 *
 * @par Completion Signature
 * @code void() @endcode
 */
template <ASIO_COMPLETION_TOKEN_FOR(void()) NullaryToken = deferred_t>
inline auto dispatch(NullaryToken&& token = deferred_t())
  -> decltype(
    async_initiate<NullaryToken, void()>(
      declval<detail::initiate_dispatch>(), token))
{
  return async_initiate<NullaryToken, void()>(
      detail::initiate_dispatch(), token);
}

/// Submits a completion token or function object for execution.
/**
 * This function submits an object for execution using the specified executor.
 * The function object may be called from the current thread prior to returning
 * from <tt>dispatch()</tt>. Otherwise, it is queued for execution.
 *
 * @param ex The target executor.
 *
 * @param token The @ref completion_token that will be used to produce a
 * completion handler. The function signature of the completion handler must be:
 * @code void handler(); @endcode
 *
 * @returns This function returns <tt>async_initiate<NullaryToken,
 * void()>(Init{ex}, token)</tt>, where @c Init is a function object type
 * defined as:
 *
 * @code class Init
 * {
 * public:
 *   using executor_type = Executor;
 *   explicit Init(const Executor& ex) : ex_(ex) {}
 *   executor_type get_executor() const noexcept { return ex_; }
 *   template <typename CompletionHandler>
 *     void operator()(CompletionHandler&& completion_handler) const;
 * private:
 *   Executor ex_; // exposition only
 * }; @endcode
 *
 * The function call operator of @c Init:
 *
 * @li Obtains the handler's associated executor object @c ex1 of type @c Ex1 by
 * performing
 * @code auto ex1 = get_associated_executor(completion_handler, ex); @endcode
 *
 * @li Obtains the handler's associated allocator object @c alloc by performing
 * @code auto alloc = get_associated_allocator(completion_handler); @endcode
 *
 * @li If <tt>execution::is_executor<Ex1>::value</tt> is true, constructs a
 * function object @c f with a member @c executor_ that is initialised with
 * <tt>prefer(ex1, execution::outstanding_work.tracked)</tt>, a member @c
 * handler_ that is a decay-copy of @c completion_handler, and a function call
 * operator that performs:
 * @code auto a = get_associated_allocator(handler_);
 * prefer(executor_, execution::allocator(a)).execute(std::move(handler_));
 * @endcode
 *
 * @li If <tt>execution::is_executor<Ex1>::value</tt> is false, constructs a
 * function object @c f with a member @c work_ that is initialised with
 * <tt>make_work_guard(ex1)</tt>, a member @c handler_ that is a decay-copy of
 * @c completion_handler, and a function call operator that performs:
 * @code auto a = get_associated_allocator(handler_);
 * work_.get_executor().dispatch(std::move(handler_), a);
 * work_.reset(); @endcode
 *
 * @li If <tt>execution::is_executor<Executor>::value</tt> is true, performs
 * @code prefer(ex, execution::allocator(alloc)).execute(std::move(f)); @endcode
 *
 * @li If <tt>execution::is_executor<Executor>::value</tt> is false, performs
 * @code ex.dispatch(std::move(f), alloc); @endcode
 *
 * @par Completion Signature
 * @code void() @endcode
 */
template <typename Executor,
    ASIO_COMPLETION_TOKEN_FOR(void()) NullaryToken
      = default_completion_token_t<Executor>>
inline auto dispatch(const Executor& ex,
    NullaryToken&& token = default_completion_token_t<Executor>(),
    constraint_t<
      execution::is_executor<Executor>::value || is_executor<Executor>::value
    > = 0)
  -> decltype(
    async_initiate<NullaryToken, void()>(
      declval<detail::initiate_dispatch_with_executor<Executor>>(),
      token, detail::empty_work_function()))
{
  return async_initiate<NullaryToken, void()>(
      detail::initiate_dispatch_with_executor<Executor>(ex),
      token, detail::empty_work_function());
}

/// Submits a completion token or function object for execution.
/**
 * @param ctx An execution context, from which the target executor is obtained.
 *
 * @param token The @ref completion_token that will be used to produce a
 * completion handler. The function signature of the completion handler must be:
 * @code void handler(); @endcode
 *
 * @returns <tt>dispatch(ctx.get_executor(), forward<NullaryToken>(token))</tt>.
 *
 * @par Completion Signature
 * @code void() @endcode
 */
template <typename ExecutionContext,
    ASIO_COMPLETION_TOKEN_FOR(void()) NullaryToken
      = default_completion_token_t<typename ExecutionContext::executor_type>>
inline auto dispatch(ExecutionContext& ctx,
    NullaryToken&& token = default_completion_token_t<
      typename ExecutionContext::executor_type>(),
    constraint_t<
      is_convertible<ExecutionContext&, execution_context&>::value
    > = 0)
  -> decltype(
    async_initiate<NullaryToken, void()>(
      declval<detail::initiate_dispatch_with_executor<
        typename ExecutionContext::executor_type>>(),
      token, detail::empty_work_function()))
{
  return async_initiate<NullaryToken, void()>(
      detail::initiate_dispatch_with_executor<
        typename ExecutionContext::executor_type>(ctx.get_executor()),
      token, detail::empty_work_function());
}

/// Submits a function to be run on a specified target executor, and after
/// completion submits the completion handler.
/**
 * This function submits a function object for execution using the specified
 * executor. The function object may be called from the current thread prior to
 * returning from <tt>dispatch()</tt>. Otherwise, it is queued for execution.
 * After the submitted function completes, the completion handler is dispatched
 * to run on its associated executor.
 *
 * @param function A nullary function to be executed on the target executor.
 *
 * @param ex The target executor.
 *
 * @param token The @ref completion_token that will be used to produce a
 * completion handler. The function signature of the completion handler must be:
 * @code void handler(); @endcode
 *
 * @returns This function returns <tt>async_initiate<NullaryToken,
 * void()>(Init{ex}, token, forward<Function>(function))</tt>, where @c Init is
 * a function object type defined as:
 *
 * @code class Init
 * {
 * public:
 *   using executor_type = Executor;
 *   explicit Init(const Executor& ex) : ex_(ex) {}
 *   executor_type get_executor() const noexcept { return ex_; }
 *   template <typename CompletionHandler>
 *     void operator()(CompletionHandler&& completion_handler,
 *       Function&& function) const;
 * private:
 *   Executor ex_; // exposition only
 * }; @endcode
 *
 * The function call operator of @c Init:
 *
 * @li Obtains the handler's associated executor object @c ex1 of type @c Ex1 by
 * performing
 * @code auto ex1 = get_associated_executor(completion_handler, ex); @endcode
 *
 * @li Obtains the handler's associated allocator object @c alloc by performing
 * @code auto alloc = get_associated_allocator(completion_handler); @endcode
 *
 * @li If <tt>execution::is_executor<Ex1>::value</tt> is true, constructs a
 * function object wrapper @c f with a member @c executor_ that is initialised
 * with <tt>prefer(ex1, execution::outstanding_work.tracked)</tt>, a member @c
 * function_ that is a decay-copy of @c function, a member @c handler_ that is a
 * decay-copy of @c completion_handler, and a function call operator that
 * performs:
 * @code std::move(function_)();
 * auto a = get_associated_allocator(handler_);
 * prefer(executor_, execution::allocator(a)).execute(std::move(handler_));
 * @endcode
 *
 * @li If <tt>execution::is_executor<Ex1>::value</tt> is false, constructs a
 * function object wrapper @c f with a member @c work_ that is initialised with
 * <tt>make_work_guard(ex1)</tt>, a member @c function_ that is a decay-copy of
 * @c function, a member @c handler_ that is a decay-copy of @c
 * completion_handler, and a function call operator that performs:
 * @code std::move(function_)();
 * auto a = get_associated_allocator(handler_);
 * work_.get_executor().dispatch(std::move(handler_), a);
 * work_.reset(); @endcode
 *
 * @li If <tt>execution::is_executor<Executor>::value</tt> is true, performs
 * @code prefer(
 *     require(ex, execution::blocking.never),
 *     execution::relationship.fork,
 *     execution::allocator(alloc)
 *   ).execute(std::move(f)); @endcode
 *
 * @li If <tt>execution::is_executor<Executor>::value</tt> is false, performs
 * @code ex.dispatch(std::move(f), alloc); @endcode
 *
 * @note If the function object throws an exception, that exception is allowed
 * to propagate to the target executor. The behaviour in this case is dependent
 * on the executor. For example, asio::io_context will allow the
 * exception to propagate to the caller that runs the @c io_context, whereas
 * asio::thread_pool will call @c std::terminate.
 *
 * @par Completion Signature
 * @code void() @endcode
 */
template <typename Function, typename Executor,
    ASIO_COMPLETION_TOKEN_FOR(void()) NullaryToken
      = default_completion_token_t<Executor>>
inline auto dispatch(Function&& function, const Executor& ex,
    NullaryToken&& token = default_completion_token_t<Executor>(),
    constraint_t<
      is_void<result_of_t<decay_t<Function>()>>::value
    > = 0,
    constraint_t<
      (execution::is_executor<Executor>::value
          && can_require<Executor, execution::blocking_t::never_t>::value)
        || is_executor<Executor>::value
    > = 0)
  -> decltype(
    async_initiate<NullaryToken, void()>(
        declval<detail::initiate_dispatch_with_executor<Executor>>(),
        token, static_cast<Function&&>(function)))
{
  return async_initiate<NullaryToken, void()>(
      detail::initiate_dispatch_with_executor<Executor>(ex),
      token, static_cast<Function&&>(function));
}

/// Submits a function to be run on a specified target executor, and passes the
/// result to a completion handler.
/**
 * This function submits a function object for execution using the specified
 * executor. The function object may be called from the current thread prior to
 * returning from <tt>dispatch()</tt>. Otherwise, it is queued for execution.
 * After the submitted function completes, the completion handler is dispatched
 * along with the function's result, to run on its associated executor.
 *
 * @param function A nullary function to be executed on the target executor.
 *
 * @param ex The target executor.
 *
 * @param token The @ref completion_token that will be used to produce a
 * completion handler. The function signature of the completion handler must be:
 * @code void handler(decay_t<result_of_t<decay_t<Function>()>>); @endcode
 *
 * @returns This function returns <tt>async_initiate<CompletionToken,
 * void()>(Init{ex}, token)</tt>, where @c Init is a function object type
 * defined as:
 *
 * @code class Init
 * {
 * public:
 *   using executor_type = Executor;
 *   explicit Init(const Executor& ex) : ex_(ex) {}
 *   executor_type get_executor() const noexcept { return ex_; }
 *   template <typename CompletionHandler>
 *     void operator()(CompletionHandler&& completion_handler,
 *       Function&& function) const;
 * private:
 *   Executor ex_; // exposition only
 * }; @endcode
 *
 * The function call operator of @c Init:
 *
 * @li Obtains the handler's associated executor object @c ex1 of type @c Ex1 by
 * performing
 * @code auto ex1 = get_associated_executor(completion_handler, ex); @endcode
 *
 * @li Obtains the handler's associated allocator object @c alloc by performing
 * @code auto alloc = get_associated_allocator(completion_handler); @endcode
 *
 * @li If <tt>execution::is_executor<Ex1>::value</tt> is true, constructs a
 * function object wrapper @c f with a member @c executor_ that is initialised
 * with <tt>prefer(ex1, execution::outstanding_work.tracked)</tt>, a member @c
 * function_ that is a decay-copy of @c function, a member @c handler_ that is a
 * decay-copy of @c completion_handler, and a function call operator that
 * performs:
 * @code auto result = std::move(function_)();
 * auto a = get_associated_allocator(handler_);
 * prefer(executor_, execution::allocator(a)).execute(
 *     std::bind(std::move(handler_), std::move(result)));
 * @endcode
 *
 * @li If <tt>execution::is_executor<Ex1>::value</tt> is false, constructs a
 * function object wrapper @c f with a member @c work_ that is initialised with
 * <tt>make_work_guard(ex1)</tt>, a member @c function_ that is a decay-copy of
 * @c function, a member @c handler_ that is a decay-copy of @c
 * completion_handler, and a function call operator that performs:
 * @code auto result = std::move(function_)();
 * auto a = get_associated_allocator(handler_);
 * work_.get_executor().dispatch(
 *     std::bind(std::move(handler_), std::move(result)), a);
 * work_.reset(); @endcode
 *
 * @li If <tt>execution::is_executor<Executor>::value</tt> is true, performs
 * @code prefer(
 *     require(ex, execution::blocking.never),
 *     execution::relationship.fork,
 *     execution::allocator(alloc)
 *   ).execute(std::move(f)); @endcode
 *
 * @li If <tt>execution::is_executor<Executor>::value</tt> is false, performs
 * @code ex.dispatch(std::move(f), alloc); @endcode
 *
 * @note If the function object throws an exception, that exception is allowed
 * to propagate to the target executor. The behaviour in this case is dependent
 * on the executor. For example, asio::io_context will allow the
 * exception to propagate to the caller that runs the @c io_context, whereas
 * asio::thread_pool will call @c std::terminate.
 *
 * @par Completion Signature
 * @code void(decay_t<result_of_t<decay_t<Function>()>>) @endcode
 */
template <typename Function, typename Executor,
    ASIO_COMPLETION_TOKEN_FOR(
      void(decay_t<result_of_t<decay_t<Function>()>>)) CompletionToken
        = default_completion_token_t<Executor>>
inline auto dispatch(Function&& function, const Executor& ex,
    CompletionToken&& token = default_completion_token_t<Executor>(),
    constraint_t<
      !is_void<result_of_t<decay_t<Function>()>>::value
    > = 0,
    constraint_t<
      (execution::is_executor<Executor>::value
          && can_require<Executor, execution::blocking_t::never_t>::value)
        || is_executor<Executor>::value
    > = 0)
  -> decltype(
    async_initiate<CompletionToken, void(detail::work_result_t<Function>)>(
      declval<detail::initiate_dispatch_with_executor<Executor>>(),
      token, static_cast<Function&&>(function)))
{
  return async_initiate<CompletionToken, void(detail::work_result_t<Function>)>(
      detail::initiate_dispatch_with_executor<Executor>(ex),
      token, static_cast<Function&&>(function));
}

/// Submits a function to be run on a specified execution context, and after
/// completion submits the completion handler.
/**
 * @param function A nullary function to be executed on the target executor.
 *
 * @param ctx An execution context, from which the target executor is obtained.
 *
 * @param token The @ref completion_token that will be used to produce a
 * completion handler. The function signature of the completion handler must be:
 * @code void handler(); @endcode
 *
 * @returns <tt>dispatch(forward<Function>(function), ctx.get_executor(),
 * forward<NullaryToken>(token))</tt>.
 *
 * @note If the function object throws an exception, that exception is allowed
 * to propagate to the target executor. The behaviour in this case is dependent
 * on the executor. For example, asio::io_context will allow the
 * exception to propagate to the caller that runs the @c io_context, whereas
 * asio::thread_pool will call @c std::terminate.
 *
 * @par Completion Signature
 * @code void() @endcode
 */
template <typename Function, typename ExecutionContext,
    ASIO_COMPLETION_TOKEN_FOR(void()) NullaryToken
      = default_completion_token_t<typename ExecutionContext::executor_type>>
inline auto dispatch(Function&& function, ExecutionContext& ctx,
    NullaryToken&& token = default_completion_token_t<
      typename ExecutionContext::executor_type>(),
    constraint_t<
      is_void<result_of_t<decay_t<Function>()>>::value
    > = 0,
    constraint_t<
      is_convertible<ExecutionContext&, execution_context&>::value
    > = 0)
  -> decltype(
    async_initiate<NullaryToken, void()>(
      declval<detail::initiate_dispatch_with_executor<
        typename ExecutionContext::executor_type>>(),
        token, static_cast<Function&&>(function)))
{
  return async_initiate<NullaryToken, void()>(
      detail::initiate_dispatch_with_executor<
        typename ExecutionContext::executor_type>(ctx.get_executor()),
        token, static_cast<Function&&>(function));
}

/// Submits a function to be run on a specified execution context, and passes
/// the result to a completion handler.
/**
 * @param function A nullary function to be executed on the target executor.
 *
 * @param ctx An execution context, from which the target executor is obtained.
 *
 * @param token The @ref completion_token that will be used to produce a
 * completion handler. The function signature of the completion handler must be:
 * @code void handler(); @endcode
 *
 * @returns <tt>dispatch(forward<Function>(function), ctx.get_executor(),
 * forward<CompletionToken>(token))</tt>.
 *
 * @note If the function object throws an exception, that exception is allowed
 * to propagate to the target executor. The behaviour in this case is dependent
 * on the executor. For example, asio::io_context will allow the
 * exception to propagate to the caller that runs the @c io_context, whereas
 * asio::thread_pool will call @c std::terminate.
 *
 * @par Completion Signature
 * @code void(decay_t<result_of_t<decay_t<Function>()>>) @endcode
 */
template <typename Function, typename ExecutionContext,
    ASIO_COMPLETION_TOKEN_FOR(
      void(decay_t<result_of_t<decay_t<Function>()>>)) CompletionToken
        = default_completion_token_t<typename ExecutionContext::executor_type>>
inline auto dispatch(Function&& function, ExecutionContext& ctx,
    CompletionToken&& token = default_completion_token_t<
      typename ExecutionContext::executor_type>(),
    constraint_t<
      !is_void<result_of_t<decay_t<Function>()>>::value
    > = 0,
    constraint_t<
      is_convertible<ExecutionContext&, execution_context&>::value
    > = 0)
  -> decltype(
    async_initiate<CompletionToken, void(detail::work_result_t<Function>)>(
      declval<detail::initiate_dispatch_with_executor<
        typename ExecutionContext::executor_type>>(),
        token, static_cast<Function&&>(function)))
{
  return async_initiate<CompletionToken, void(detail::work_result_t<Function>)>(
      detail::initiate_dispatch_with_executor<
        typename ExecutionContext::executor_type>(ctx.get_executor()),
        token, static_cast<Function&&>(function));
}

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DISPATCH_HPP
