//
// exection/impl/receiver_invocation_error.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_IMPL_RECEIVER_INVOCATION_ERROR_IPP
#define ASIO_EXECUTION_IMPL_RECEIVER_INVOCATION_ERROR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/execution/receiver_invocation_error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace execution {

receiver_invocation_error::receiver_invocation_error()
  : std::runtime_error("receiver invocation error")
{
}

} // namespace execution
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_EXECUTION_IMPL_RECEIVER_INVOCATION_ERROR_IPP
