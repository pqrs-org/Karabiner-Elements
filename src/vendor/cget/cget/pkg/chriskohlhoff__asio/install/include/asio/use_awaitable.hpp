//
// use_awaitable.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_USE_AWAITABLE_HPP
#define ASIO_USE_AWAITABLE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_CO_AWAIT) || defined(GENERATING_DOCUMENTATION)

#include "asio/awaitable.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

/// A completion token that represents the currently executing coroutine.
/**
 * The @c use_awaitable_t class, with its value @c use_awaitable, is used to
 * represent the currently executing coroutine. This completion token may be
 * passed as a handler to an asynchronous operation. For example:
 *
 * @code awaitable<void> my_coroutine()
 * {
 *   std::size_t n = co_await my_socket.async_read_some(buffer, use_awaitable);
 *   ...
 * } @endcode
 *
 * When used with co_await, the initiating function (@c async_read_some in the
 * above example) suspends the current coroutine. The coroutine is resumed when
 * the asynchronous operation completes, and the result of the operation is
 * returned.
 */
template <typename Executor = executor>
struct use_awaitable_t
{
  ASIO_CONSTEXPR use_awaitable_t()
  {
  }
};

/// A completion token object that represents the currently executing coroutine.
/**
 * See the documentation for asio::use_awaitable_t for a usage example.
 */
#if defined(ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
constexpr use_awaitable_t<> use_awaitable;
#elif defined(ASIO_MSVC)
__declspec(selectany) use_awaitable_t<> use_awaitable;
#endif

} // namespace asio

#include "asio/detail/pop_options.hpp"

#include "asio/impl/use_awaitable.hpp"

#endif // defined(ASIO_HAS_CO_AWAIT) || defined(GENERATING_DOCUMENTATION)

#endif // ASIO_USE_AWAITABLE_HPP
