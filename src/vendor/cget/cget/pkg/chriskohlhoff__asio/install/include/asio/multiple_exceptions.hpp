//
// multiple_exceptions.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
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

#if defined(ASIO_HAS_STD_EXCEPTION_PTR) \
  || defined(GENERATING_DOCUMENTATION)

/// Exception thrown when there are multiple pending exceptions to rethrow.
class multiple_exceptions
  : public std::exception
{
public:
  /// Constructor.
  ASIO_DECL multiple_exceptions(
      std::exception_ptr first) ASIO_NOEXCEPT;

  /// Obtain message associated with exception.
  ASIO_DECL virtual const char* what() const
    ASIO_NOEXCEPT_OR_NOTHROW;

  /// Obtain a pointer to the first exception.
  ASIO_DECL std::exception_ptr first_exception() const;

private:
  std::exception_ptr first_;
};

#endif // defined(ASIO_HAS_STD_EXCEPTION_PTR)
       //   || defined(GENERATING_DOCUMENTATION)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#if defined(ASIO_HEADER_ONLY)
# include "asio/impl/multiple_exceptions.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // ASIO_MULTIPLE_EXCEPTIONS_HPP
