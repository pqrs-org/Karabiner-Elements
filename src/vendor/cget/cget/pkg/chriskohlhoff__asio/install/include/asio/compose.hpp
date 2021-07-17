//
// compose.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_COMPOSE_HPP
#define ASIO_COMPOSE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/async_result.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

#if defined(ASIO_HAS_VARIADIC_TEMPLATES) \
  || defined(GENERATING_DOCUMENTATION)

/// Launch an asynchronous operation with a stateful implementation.
/**
 * The async_compose function simplifies the implementation of composed
 * asynchronous operations automatically wrapping a stateful function object
 * with a conforming intermediate completion handler.
 *
 * @param implementation A function object that contains the implementation of
 * the composed asynchronous operation. The first argument to the function
 * object is a non-const reference to the enclosing intermediate completion
 * handler. The remaining arguments are any arguments that originate from the
 * completion handlers of any asynchronous operations performed by the
 * implementation.

 * @param token The completion token.
 *
 * @param io_objects_or_executors Zero or more I/O objects or I/O executors for
 * which outstanding work must be maintained.
 *
 * @par Example:
 *
 * @code struct async_echo_implementation
 * {
 *   tcp::socket& socket_;
 *   asio::mutable_buffer buffer_;
 *   enum { starting, reading, writing } state_;
 *
 *   template <typename Self>
 *   void operator()(Self& self,
 *       asio::error_code error = {},
 *       std::size_t n = 0)
 *   {
 *     switch (state_)
 *     {
 *     case starting:
 *       state_ = reading;
 *       socket_.async_read_some(
 *           buffer_, std::move(self));
 *       break;
 *     case reading:
 *       if (error)
 *       {
 *         self.complete(error, 0);
 *       }
 *       else
 *       {
 *         state_ = writing;
 *         asio::async_write(socket_, buffer_,
 *             asio::transfer_exactly(n),
 *             std::move(self));
 *       }
 *       break;
 *     case writing:
 *       self.complete(error, n);
 *       break;
 *     }
 *   }
 * };
 *
 * template <typename CompletionToken>
 * auto async_echo(tcp::socket& socket,
 *     asio::mutable_buffer buffer,
 *     CompletionToken&& token) ->
 *   typename asio::async_result<
 *     typename std::decay<CompletionToken>::type,
 *       void(asio::error_code, std::size_t)>::return_type
 * {
 *   return asio::async_compose<CompletionToken,
 *     void(asio::error_code, std::size_t)>(
 *       async_echo_implementation{socket, buffer,
 *         async_echo_implementation::starting},
 *       token, socket);
 * } @endcode
 */
template <typename CompletionToken, typename Signature,
    typename Implementation, typename... IoObjectsOrExecutors>
ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken, Signature)
async_compose(ASIO_MOVE_ARG(Implementation) implementation,
    ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token,
    ASIO_MOVE_ARG(IoObjectsOrExecutors)... io_objects_or_executors);

#else // defined(ASIO_HAS_VARIADIC_TEMPLATES)
      //   || defined(GENERATING_DOCUMENTATION)

template <typename CompletionToken, typename Signature, typename Implementation>
ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken, Signature)
async_compose(ASIO_MOVE_ARG(Implementation) implementation,
    ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token);

#define ASIO_PRIVATE_ASYNC_COMPOSE_DEF(n) \
  template <typename CompletionToken, typename Signature, \
      typename Implementation, ASIO_VARIADIC_TPARAMS(n)> \
  ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken, Signature) \
  async_compose(ASIO_MOVE_ARG(Implementation) implementation, \
      ASIO_NONDEDUCED_MOVE_ARG(CompletionToken) token, \
      ASIO_VARIADIC_MOVE_PARAMS(n));
  /**/
  ASIO_VARIADIC_GENERATE(ASIO_PRIVATE_ASYNC_COMPOSE_DEF)
#undef ASIO_PRIVATE_ASYNC_COMPOSE_DEF

#endif // defined(ASIO_HAS_VARIADIC_TEMPLATES)
       //   || defined(GENERATING_DOCUMENTATION)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#include "asio/impl/compose.hpp"

#endif // ASIO_COMPOSE_HPP
