//
// detail/impl/throw_error.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_THROW_ERROR_IPP
#define ASIO_DETAIL_IMPL_THROW_ERROR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/throw_error.hpp"
#include "asio/system_error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

void do_throw_error(
    const asio::error_code& err
    ASIO_SOURCE_LOCATION_PARAM)
{
  asio::system_error e(err);
  asio::detail::throw_exception(e ASIO_SOURCE_LOCATION_ARG);
}

void do_throw_error(
    const asio::error_code& err,
    const char* location
    ASIO_SOURCE_LOCATION_PARAM)
{
  // boostify: non-boost code starts here
#if defined(ASIO_MSVC) \
  && defined(ASIO_HAS_STD_SYSTEM_ERROR) \
  && (_MSC_VER < 1800)
  // Microsoft's implementation of std::system_error is non-conformant in that
  // it ignores the error code's message when a "what" string is supplied. We'll
  // work around this by explicitly formatting the "what" string.
  std::string what_msg = location;
  what_msg += ": ";
  what_msg += err.message();
  asio::system_error e(err, what_msg);
  asio::detail::throw_exception(e ASIO_SOURCE_LOCATION_ARG);
#else // defined(ASIO_MSVC)
      //   && defined(ASIO_HAS_STD_SYSTEM_ERROR)
      //   && (_MSC_VER < 1928)
  // boostify: non-boost code ends here
  asio::system_error e(err, location);
  asio::detail::throw_exception(e ASIO_SOURCE_LOCATION_ARG);
  // boostify: non-boost code starts here
#endif // defined(ASIO_MSVC)
       //   && defined(ASIO_HAS_STD_SYSTEM_ERROR)
       //   && (_MSC_VER < 1800)
  // boostify: non-boost code ends here
}

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_IMPL_THROW_ERROR_IPP
