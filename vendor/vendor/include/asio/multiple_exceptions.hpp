//
// multiple_exceptions.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_MULTIPLE_EXCEPTIONS_HPP
#define ASIO_MULTIPLE_EXCEPTIONS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <exception>
#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN

/// Exception thrown when there are multiple pending exceptions to rethrow.
class multiple_exceptions
  : public std::exception
{
public:
  /// Constructor.
  multiple_exceptions(std::exception_ptr first) noexcept
    : first_(static_cast<std::exception_ptr&&>(first))
  {
  }

  /// Destructor.
  virtual ~multiple_exceptions() noexcept
  {
  }

  /// Obtain message associated with exception.
  virtual const char* what() const noexcept
  {
    return "multiple exceptions";
  }

  /// Obtain a pointer to the first exception.
  std::exception_ptr first_exception() const
  {
    return first_;
  }

private:
  std::exception_ptr first_;
};

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_MULTIPLE_EXCEPTIONS_HPP
