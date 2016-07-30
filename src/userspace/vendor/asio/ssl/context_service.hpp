//
// ssl/context_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_SSL_CONTEXT_SERVICE_HPP
#define ASIO_SSL_CONTEXT_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
  
#if defined(ASIO_ENABLE_OLD_SSL)
# include "asio/ssl/old/context_service.hpp"
#endif // defined(ASIO_ENABLE_OLD_SSL)

#include "asio/detail/push_options.hpp"

namespace asio {
namespace ssl {

#if defined(ASIO_ENABLE_OLD_SSL)

using asio::ssl::old::context_service;

#endif // defined(ASIO_ENABLE_OLD_SSL)

} // namespace ssl
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_SSL_CONTEXT_SERVICE_HPP
