//
// detail/slim_mutex.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_SLIM_MUTEX_HPP
#define ASIO_DETAIL_SLIM_MUTEX_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if !defined(ASIO_HAS_THREADS)
# include "asio/detail/null_mutex.hpp"
#elif defined(ASIO_HAS_STD_ATOMIC_WAIT)
# include "asio/detail/atomic_slim_mutex.hpp"
#else
# include "asio/detail/mutex.hpp"
#endif

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

#if !defined(ASIO_HAS_THREADS)
typedef null_mutex slim_mutex;
#elif defined(ASIO_HAS_STD_ATOMIC_WAIT)
typedef atomic_slim_mutex slim_mutex;
#else
typedef mutex slim_mutex;
#endif

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#endif // ASIO_DETAIL_SLIM_MUTEX_HPP
