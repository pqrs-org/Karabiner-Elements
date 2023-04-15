//
// execution/receiver_invocation_error.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_RECEIVER_INVOCATION_ERROR_HPP
#define ASIO_EXECUTION_RECEIVER_INVOCATION_ERROR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <stdexcept>

#include "asio/detail/push_options.hpp"

namespace asio {
namespace execution {

/// Exception reported via @c set_error when an exception escapes from
/// @c set_value.
class receiver_invocation_error
  : public std::runtime_error
#if defined(ASIO_HAS_STD_NESTED_EXCEPTION)
  , public std::nested_exception
#endif // defined(ASIO_HAS_STD_NESTED_EXCEPTION)
{
public:
  /// Constructor.
  ASIO_DECL receiver_invocation_error();
};

} // namespace execution
} // namespace asio

#include "asio/detail/pop_options.hpp"

#if defined(ASIO_HEADER_ONLY)
# include "asio/execution/impl/receiver_invocation_error.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // ASIO_EXECUTION_RECEIVER_INVOCATION_ERROR_HPP
