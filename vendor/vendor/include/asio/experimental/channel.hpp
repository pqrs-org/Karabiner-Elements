//
// experimental/channel.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXPERIMENTAL_CHANNEL_HPP
#define ASIO_EXPERIMENTAL_CHANNEL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/channel.hpp"
#include "asio/experimental/channel_error.hpp"
#include "asio/experimental/channel_traits.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace experimental {

template <typename ExecutorOrSignature, typename... Signatures>
using channel = asio::channel<ExecutorOrSignature, Signatures...>;

} // namespace experimental
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXPERIMENTAL_CHANNEL_HPP
