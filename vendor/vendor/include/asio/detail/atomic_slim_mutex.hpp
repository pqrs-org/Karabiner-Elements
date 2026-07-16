//
// detail/atomic_slim_mutex.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_ATOMIC_SLIM_MUTEX_HPP
#define ASIO_DETAIL_ATOMIC_SLIM_MUTEX_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_STD_ATOMIC_WAIT)

#include <atomic>
#include "asio/detail/conditionally_enabled_mutex.hpp"
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/scoped_lock.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

class atomic_slim_mutex
  : private noncopyable
{
public:
  typedef asio::detail::scoped_lock<atomic_slim_mutex> scoped_lock;

  // Constructor.
  atomic_slim_mutex()
    : state_(0)
  {
  }

  // Destructor.
  ~atomic_slim_mutex()
  {
  }

  // Try to lock the mutex.
  bool try_lock()
  {
    int expected = 0;
    return state_.compare_exchange_strong(expected, 1,
        std::memory_order_acquire, std::memory_order_relaxed);
  }

  // Lock the mutex.
  void lock()
  {
    int expected = 0;
    if (state_.compare_exchange_strong(expected, 1,
          std::memory_order_acquire, std::memory_order_relaxed))
      return;
    if (expected != 2)
      expected = state_.exchange(2, std::memory_order_acquire);
    while (expected != 0)
    {
      state_.wait(2, std::memory_order_relaxed);
      expected = state_.exchange(2, std::memory_order_acquire);
    }
  }

  // Unlock the mutex.
  void unlock()
  {
    if (state_.fetch_sub(1, std::memory_order_release) != 1)
    {
      state_.store(0, std::memory_order_release);
      state_.notify_one();
    }
  }

private:
  friend class conditionally_enabled_mutex<atomic_slim_mutex>;

  // 0 = unlocked, 1 = locked with no waiters, 2 = locked with waiters.
  // -1 is reserved for conditionally_enabled_mutex<atomic_slim_mutex> to
  // represent the disabled state.
  std::atomic<int> state_;
};

template <>
class conditionally_enabled_mutex<atomic_slim_mutex>
  : private noncopyable
{
public:
  // Helper class to lock and unlock a mutex automatically.
  class scoped_lock
    : private noncopyable
  {
  public:
    // Tag type used to distinguish constructors.
    enum adopt_lock_t { adopt_lock };

    // Constructor adopts a lock that is already held.
    scoped_lock(conditionally_enabled_mutex& m, adopt_lock_t)
      : mutex_(m),
        locked_(m.enabled())
    {
    }

    // Constructor acquires the lock.
    explicit scoped_lock(conditionally_enabled_mutex& m)
      : mutex_(m),
        locked_(false)
    {
      if (m.enabled())
      {
        for (int n = mutex_.spin_count_; n != 0; n -= (n > 0) ? 1 : 0)
        {
          if (mutex_.try_lock())
          {
            locked_ = true;
            return;
          }
        }
        mutex_.lock();
        locked_ = true;
      }
    }

    // Destructor releases the lock.
    ~scoped_lock()
    {
      if (locked_)
        mutex_.mutex_.unlock();
    }

    // Explicitly acquire the lock.
    void lock()
    {
      if (!locked_ && mutex_.enabled())
      {
        for (int n = mutex_.spin_count_; n != 0; n -= (n > 0) ? 1 : 0)
        {
          if (mutex_.try_lock())
          {
            locked_ = true;
            return;
          }
        }
        mutex_.mutex_.lock();
        locked_ = true;
      }
    }

    // Explicitly release the lock.
    void unlock()
    {
      if (locked_)
      {
        mutex_.mutex_.unlock();
        locked_ = false;
      }
    }

    // Test whether the lock is held.
    bool locked() const
    {
      return locked_;
    }

  private:
    friend class conditionally_enabled_mutex;
    conditionally_enabled_mutex& mutex_;
    bool locked_;
  };

  // Constructor.
  explicit conditionally_enabled_mutex(bool enabled, int spin_count = 0)
    : spin_count_(spin_count)
  {
    if (!enabled)
      mutex_.state_.store(-1, std::memory_order_relaxed);
  }

  // Destructor.
  ~conditionally_enabled_mutex()
  {
  }

  // Determine whether locking is enabled.
  bool enabled() const
  {
    return mutex_.state_.load(std::memory_order_relaxed) != -1;
  }

  // Get the spin count.
  int spin_count() const
  {
    return spin_count_;
  }

  // Lock the mutex.
  void lock()
  {
    if (enabled())
    {
      for (int n = spin_count_; n != 0; n -= (n > 0) ? 1 : 0)
        if (mutex_.try_lock())
          return;
      mutex_.lock();
    }
  }

  // Unlock the mutex.
  void unlock()
  {
    if (enabled())
      mutex_.unlock();
  }

  // Try to lock the mutex.
  bool try_lock()
  {
    if (!enabled())
      return true;
    return mutex_.try_lock();
  }

private:
  friend class scoped_lock;
  atomic_slim_mutex mutex_;
  const int spin_count_;
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_STD_ATOMIC_WAIT)

#endif // ASIO_DETAIL_ATOMIC_SLIM_MUTEX_HPP
