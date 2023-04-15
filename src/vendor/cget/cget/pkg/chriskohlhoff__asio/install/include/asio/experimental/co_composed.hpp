//
// experimental/co_composed.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXPERIMENTAL_CO_COMPOSED_HPP
#define ASIO_EXPERIMENTAL_CO_COMPOSED_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/async_result.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace experimental {

/// Creates an initiation function object that may be used to launch a
/// coroutine-based composed asynchronous operation.
/**
 * The experimental::co_composed utility simplifies the implementation of
 * composed asynchronous operations by automatically adapting a coroutine to be
 * an initiation function object for use with @c async_initiate. When awaiting
 * asynchronous operations, the coroutine automatically uses a conforming
 * intermediate completion handler.
 *
 * @param implementation A function object that contains the coroutine-based
 * implementation of the composed asynchronous operation. The first argument to
 * the function object represents the state of the operation, and may be used
 * to test for cancellation. The remaining arguments are those passed to @c
 * async_initiate after the completion token.
 *
 * @param io_objects_or_executors Zero or more I/O objects or I/O executors for
 * which outstanding work must be maintained while the operation is incomplete.
 *
 * @par Per-Operation Cancellation
 * By default, per-operation cancellation is disabled for composed operations
 * that use experimental::co_composed. It must be explicitly enabled by calling
 * the state's @c reset_cancellation_state function.
 *
 * @par Examples
 * The following example illustrates manual error handling and explicit checks
 * for cancellation. The completion handler is invoked via a @c co_yield to the
 * state's @c complete function, which never returns.
 *
 * @code template <typename CompletionToken>
 * auto async_echo(tcp::socket& socket,
 *     CompletionToken&& token)
 * {
 *   return asio::async_initiate<
 *     CompletionToken, void(std::error_code)>(
 *       asio::experimental::co_composed(
 *         [](auto state, tcp::socket& socket) -> void
 *         {
 *           state.reset_cancellation_state(
 *             asio::enable_terminal_cancellation());
 *
 *           while (!state.cancelled())
 *           {
 *             char data[1024];
 *             auto [e1, n1] =
 *               co_await socket.async_read_some(
 *                 asio::buffer(data),
 *                 asio::as_tuple(asio::deferred));
 *
 *             if (e1)
 *               co_yield state.complete(e1);
 *
 *             if (!!state.cancelled())
 *               co_yield state.complete(
 *                 make_error_code(asio::error::operation_aborted));
 *
 *             auto [e2, n2] =
 *               co_await asio::async_write(socket,
 *                 asio::buffer(data, n1),
 *                 asio::as_tuple(asio::deferred));
 *
 *             if (e2)
 *               co_yield state.complete(e2);
 *           }
 *         }, socket),
 *       token, std::ref(socket));
 * } @endcode
 *
 * This next example shows exception-based error handling and implicit checks
 * for cancellation. The completion handler is invoked after returning from the
 * coroutine via @c co_return. Valid @c co_return values are specified using
 * completion signatures passed to the @c co_composed function.
 *
 * @code template <typename CompletionToken>
 * auto async_echo(tcp::socket& socket,
 *     CompletionToken&& token)
 * {
 *   return asio::async_initiate<
 *     CompletionToken, void(std::error_code)>(
 *       asio::experimental::co_composed<
 *         void(std::error_code)>(
 *           [](auto state, tcp::socket& socket) -> void
 *           {
 *             try
 *             {
 *               state.throw_if_cancelled(true);
 *               state.reset_cancellation_state(
 *                 asio::enable_terminal_cancellation());
 *
 *               for (;;)
 *               {
 *                 char data[1024];
 *                 std::size_t n = co_await socket.async_read_some(
 *                     asio::buffer(data), asio::deferred);
 *
 *                 co_await asio::async_write(socket,
 *                     asio::buffer(data, n), asio::deferred);
 *               }
 *             }
 *             catch (const std::system_error& e)
 *             {
 *               co_return {e.code()};
 *             }
 *           }, socket),
 *       token, std::ref(socket));
 * } @endcode
 */
template <completion_signature... Signatures,
    typename Implementation, typename... IoObjectsOrExecutors>
auto co_composed(Implementation&& implementation,
    IoObjectsOrExecutors&&... io_objects_or_executors);

} // namespace experimental
} // namespace asio

#include "asio/detail/pop_options.hpp"

#include "asio/experimental/impl/co_composed.hpp"

#endif // ASIO_EXPERIMENTAL_CO_COMPOSED_HPP
