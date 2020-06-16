//
// detached.hpp
// ~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETACHED_HPP
#define ASIO_DETACHED_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <memory>

#include "asio/detail/push_options.hpp"

namespace asio {

/// Class used to specify that an asynchronous operation is detached.
/**

 * The detached_t class is used to indicate that an asynchronous operation is
 * detached. That is, there is no completion handler waiting for the
 * operation's result. A detached_t object may be passed as a handler to an
 * asynchronous operation, typically using the special value
 * @c asio::detached. For example:

 * @code my_socket.async_send(my_buffer, asio::detached);
 * @endcode
 */
class detached_t
{
public:
  /// Constructor. 
  ASIO_CONSTEXPR detached_t()
  {
  }
};

/// A special value, similar to std::nothrow.
/**
 * See the documentation for asio::detached_t for a usage example.
 */
#if defined(ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
constexpr detached_t detached;
#elif defined(ASIO_MSVC)
__declspec(selectany) detached_t detached;
#endif

} // namespace asio

#include "asio/detail/pop_options.hpp"

#include "asio/impl/detached.hpp"

#endif // ASIO_DETACHED_HPP
