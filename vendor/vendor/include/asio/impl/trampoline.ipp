//
// impl/trampoline.ipp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_TRAMPOLINE_IPP
#define ASIO_IMPL_TRAMPOLINE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <cstdint>
#include <exception>
#include "asio/detail/atomic_count.hpp"
#include "asio/detail/static_mutex.hpp"
#include "asio/detail/tss_ptr.hpp"
#include "asio/trampoline.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

// Used only to define a thread-local slot that we test for null vs non-null.
struct trampoline_pending
{
};

template <typename T = void>
struct trampoline_tss
{
  static tss_ptr<trampoline_frame> current;
  static tss_ptr<trampoline_pending> install_pending;
  static trampoline_pending pending_value;

#if !defined(ASIO_HAS_THREADS)
  static std::uint64_t next_id;
#elif defined(ATOMIC_LLONG_LOCK_FREE) && (ATOMIC_LLONG_LOCK_FREE == 2)
  static std::atomic<std::uint64_t> next_id;
#else
  static std::uint64_t next_id;
  static static_mutex next_id_mutex;
#endif
};

template <typename T>
tss_ptr<trampoline_frame> trampoline_tss<T>::current;

template <typename T>
tss_ptr<trampoline_pending> trampoline_tss<T>::install_pending;

template <typename T>
trampoline_pending trampoline_tss<T>::pending_value;

#if !defined(ASIO_HAS_THREADS)
template <typename T>
std::uint64_t trampoline_tss<T>::next_id = 0;
#elif defined(ATOMIC_LLONG_LOCK_FREE) && (ATOMIC_LLONG_LOCK_FREE == 2)
template <typename T>
std::atomic<std::uint64_t> trampoline_tss<T>::next_id(0);
#else
template <typename T>
std::uint64_t trampoline_tss<T>::next_id = 0;
template <typename T>
static_mutex trampoline_tss<T>::next_id_mutex = ASIO_STATIC_MUTEX_INIT;
#endif

trampoline_frame* trampoline_frame::current() noexcept
{
  return trampoline_tss<>::current;
}

void trampoline_frame::current(trampoline_frame* f) noexcept
{
  trampoline_tss<>::current = f;
}

bool trampoline_frame::install_pending() noexcept
{
  return trampoline_tss<>::install_pending != 0;
}

void trampoline_frame::install_pending(bool v) noexcept
{
  trampoline_tss<>::install_pending =
    v ? &trampoline_tss<>::pending_value : 0;
}

void trampoline_frame::drain()
{
  while (operation* next = queue.front())
  {
    queue.pop();
    next->complete(this, 0, 0);
  }
}

#if !defined(ASIO_HAS_THREADS)

std::uint64_t make_trampoline_id() noexcept
{
  std::uint64_t id = ++trampoline_tss<>::next_id;
  if (id == 0)
    std::terminate();
  return id;
}

#elif defined(ATOMIC_LLONG_LOCK_FREE) && (ATOMIC_LLONG_LOCK_FREE == 2)

std::uint64_t make_trampoline_id() noexcept
{
  std::uint64_t id = trampoline_tss<>::next_id.fetch_add(
      1, std::memory_order_relaxed) + 1;
  if (id == 0)
    std::terminate();
  return id;
}

#else

std::uint64_t make_trampoline_id() noexcept
{
  std::uint64_t id;
  {
    trampoline_tss<>::next_id_mutex.init();
    static_mutex::scoped_lock lock(trampoline_tss<>::next_id_mutex);
    id = ++trampoline_tss<>::next_id;
  }
  if (id == 0)
    std::terminate();
  return id;
}

#endif

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_TRAMPOLINE_IPP
