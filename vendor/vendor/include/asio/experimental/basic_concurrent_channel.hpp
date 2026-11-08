//
// experimental/basic_concurrent_channel.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXPERIMENTAL_BASIC_CONCURRENT_CHANNEL_HPP
#define ASIO_EXPERIMENTAL_BASIC_CONCURRENT_CHANNEL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/basic_concurrent_channel.hpp"
#include "asio/channel_traits.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace experimental {

template <typename Executor,
    typename Traits = asio::channel_traits<>,
    typename... Signatures>
using basic_concurrent_channel =
  asio::basic_concurrent_channel<Executor, Traits, Signatures...>;

} // namespace experimental
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXPERIMENTAL_BASIC_CONCURRENT_CHANNEL_HPP
