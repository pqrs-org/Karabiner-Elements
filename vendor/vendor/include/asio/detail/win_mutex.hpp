//
// detail/win_mutex.hpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_WIN_MUTEX_HPP
#define ASIO_DETAIL_WIN_MUTEX_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_WINDOWS_SRWLOCK)

#include "asio/detail/noncopyable.hpp"
#include "asio/detail/scoped_lock.hpp"
#include "asio/detail/socket_types.hpp"
#include <synchapi.h>

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

class win_mutex
  : private noncopyable
{
public:
  typedef asio::detail::scoped_lock<win_mutex> scoped_lock;

  // Constructor.
  win_mutex()
  {
    ::InitializeSRWLock(&srw_lock_);
  }

  // Destructor. SRWLock does not require explicit cleanup.
  ~win_mutex()
  {
  }

  // Try to lock the mutex.
  bool try_lock()
  {
    return ::TryAcquireSRWLockExclusive(&srw_lock_) != 0;
  }

  // Lock the mutex.
  void lock()
  {
    ::AcquireSRWLockExclusive(&srw_lock_);
  }

  // Unlock the mutex.
  void unlock()
  {
    ::ReleaseSRWLockExclusive(&srw_lock_);
  }

private:
  ::SRWLOCK srw_lock_;
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_WINDOWS_SRWLOCK)

#endif // ASIO_DETAIL_WIN_MUTEX_HPP
