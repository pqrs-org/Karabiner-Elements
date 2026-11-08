//
// experimental/channel_error.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXPERIMENTAL_CHANNEL_ERROR_HPP
#define ASIO_EXPERIMENTAL_CHANNEL_ERROR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/channel_error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace experimental {
namespace error {
  using asio::channel_errc::channel_errors;
  using asio::channel_errc::channel_closed;
  using asio::channel_errc::channel_cancelled;
  using asio::channel_errc::get_channel_category;
  ASIO_INLINE_OR_STATIC_VARIABLE const asio::error_category&
    channel_category ASIO_UNUSED_VARIABLE
    = asio::channel_errc::get_channel_category();
  using asio::channel_errc::make_error_code;
} // namespace error
namespace channel_errc {
  using asio::channel_errc::channel_closed;
  using asio::channel_errc::channel_cancelled;
} // namespace channel_errc
} // namespace experimental
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXPERIMENTAL_CHANNEL_ERROR_HPP
