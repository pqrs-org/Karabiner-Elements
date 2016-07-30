//
// detail/addressof.hpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_ADDRESSOF_HPP
#define ASIO_DETAIL_ADDRESSOF_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_STD_ADDRESSOF)
# include <memory>
#else // defined(ASIO_HAS_STD_ADDRESSOF)
# include <boost/utility/addressof.hpp>
#endif // defined(ASIO_HAS_STD_ADDRESSOF)

namespace asio {
namespace detail {

#if defined(ASIO_HAS_STD_ADDRESSOF)
using std::addressof;
#else // defined(ASIO_HAS_STD_ADDRESSOF)
using boost::addressof;
#endif // defined(ASIO_HAS_STD_ADDRESSOF)

} // namespace detail
} // namespace asio

#endif // ASIO_DETAIL_ADDRESSOF_HPP
