//
// detail/size_sequence_adapter.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_SIZE_SEQUENCE_ADAPTER_HPP
#define ASIO_DETAIL_SIZE_SEQUENCE_ADAPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if !defined(ASIO_WINDOWS_RUNTIME)

#include <cstddef>
#include "asio/detail/empty_range.hpp"
#include "asio/detail/socket_types.hpp"
#include "asio/detail/unwrap_ref.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

// Helper class to record the size of each received message.
template <typename SizeSequence>
class size_sequence_adapter
{
public:
  // Set each size to the number of bytes in the corresponding message.
  static void set_sizes(const mmsghdr* msgs,
      std::size_t count, SizeSequence& sizes)
  {
    auto it = unwrap_ref(sizes).begin();
    for (std::size_t i = 0; i < count; ++i, ++it)
      unwrap_ref(*it) = ASIO_MMSGHDR_LEN(msgs[i]);
  }
};

template <>
class size_sequence_adapter<empty_range>
{
public:
  static void set_sizes(const mmsghdr*, std::size_t, const empty_range&)
  {
  }
};

template <>
class size_sequence_adapter<const empty_range>
  : public size_sequence_adapter<empty_range>
{
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // !defined(ASIO_WINDOWS_RUNTIME)

#endif // ASIO_DETAIL_SIZE_SEQUENCE_ADAPTER_HPP
