//
// impl/awaitable.ipp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_AWAITABLE_IPP
#define ASIO_IMPL_AWAITABLE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_CO_AWAIT)

#include "asio/awaitable.hpp"
#include "asio/detail/call_stack.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

void awaitable_launch_context::launch(void (*pump_fn)(void*), void* arg)
{
  call_stack<awaitable_launch_context>::context ctx(this);
  pump_fn(arg);
}

bool awaitable_launch_context::is_launching()
{
  return !!call_stack<awaitable_launch_context>::contains(this);
}

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_CO_AWAIT)

#endif // ASIO_IMPL_AWAITABLE_IPP
