//
// impl/multiple_exceptions.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_MULTIPLE_EXCEPTIONS_IPP
#define ASIO_IMPL_MULTIPLE_EXCEPTIONS_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/multiple_exceptions.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

#if defined(ASIO_HAS_STD_EXCEPTION_PTR)

multiple_exceptions::multiple_exceptions(
    std::exception_ptr first) ASIO_NOEXCEPT
  : first_(ASIO_MOVE_CAST(std::exception_ptr)(first))
{
}

const char* multiple_exceptions::what() const ASIO_NOEXCEPT_OR_NOTHROW
{
  return "multiple exceptions";
}

std::exception_ptr multiple_exceptions::first_exception() const
{
  return first_;
}

#endif // defined(ASIO_HAS_STD_EXCEPTION_PTR)

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_MULTIPLE_EXCEPTIONS_IPP
