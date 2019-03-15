//
// this_coro.hpp
// ~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_THIS_CORO_HPP
#define ASIO_THIS_CORO_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace this_coro {

/// Awaitable type that returns the executor of the current coroutine.
struct executor_t
{
  ASIO_CONSTEXPR executor_t()
  {
  }
};

/// Awaitable object that returns the executor of the current coroutine.
#if defined(ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
constexpr executor_t executor;
#elif defined(ASIO_MSVC)
__declspec(selectany) executor_t executor;
#endif

} // namespace this_coro
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_THIS_CORO_HPP
