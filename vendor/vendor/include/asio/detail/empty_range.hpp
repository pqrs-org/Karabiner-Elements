//
// detail/empty_range.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_EMPTY_RANGE_HPP
#define ASIO_DETAIL_EMPTY_RANGE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

struct empty_range
{
  struct iterator
  {
    iterator& operator++()
    {
      return *this;
    }

    bool operator!=(const iterator&) const
    {
      return false;
    }
  };

  iterator begin() const
  {
    return iterator();
  }

  iterator end() const
  {
    return {};
  }
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_EMPTY_RANGE_HPP
