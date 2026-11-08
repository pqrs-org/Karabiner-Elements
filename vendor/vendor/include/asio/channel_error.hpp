//
// channel_error.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_CHANNEL_ERROR_HPP
#define ASIO_CHANNEL_ERROR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/error_code.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace channel_errc {

enum channel_errors
{
  /// The channel was closed.
  channel_closed = 1,

  /// The channel was cancelled.
  channel_cancelled = 2
};

extern ASIO_DECL
const asio::error_category& get_channel_category();

ASIO_INLINE_OR_STATIC_VARIABLE const asio::error_category&
  channel_category ASIO_UNUSED_VARIABLE
  = asio::channel_errc::get_channel_category();

} // namespace channel_errc
ASIO_INLINE_NAMESPACE_END
} // namespace asio

namespace std {

template<> struct is_error_code_enum<
    asio::channel_errc::channel_errors>
{
  static const bool value = true;
};

} // namespace std

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace channel_errc {

inline asio::error_code make_error_code(channel_errors e)
{
  return asio::error_code(
      static_cast<int>(e), get_channel_category());
}

} // namespace channel_errc
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#if defined(ASIO_HEADER_ONLY)
# include "asio/impl/channel_error.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // ASIO_CHANNEL_ERROR_HPP
