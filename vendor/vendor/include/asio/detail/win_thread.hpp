//
// detail/win_thread.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_WIN_THREAD_HPP
#define ASIO_DETAIL_WIN_THREAD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_WINDOWS) \
  && !defined(ASIO_WINDOWS_APP) \
  && !defined(UNDER_CE)

#include <cstddef>
#include "asio/detail/memory.hpp"
#include "asio/detail/socket_types.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

ASIO_DECL unsigned int __stdcall win_thread_function(void* arg);

#if defined(WINVER) && (WINVER < 0x0500)
ASIO_DECL void __stdcall apc_function(ULONG data);
#else
ASIO_DECL void __stdcall apc_function(ULONG_PTR data);
#endif

template <typename T>
class win_thread_base
{
public:
  static bool terminate_threads()
  {
    return ::InterlockedExchangeAdd(&terminate_threads_, 0) != 0;
  }

  static void set_terminate_threads(bool b)
  {
    ::InterlockedExchange(&terminate_threads_, b ? 1 : 0);
  }

private:
  static long terminate_threads_;
};

template <typename T>
long win_thread_base<T>::terminate_threads_ = 0;

class win_thread
  : public win_thread_base<win_thread>
{
public:
  // Construct in a non-joinable state.
  win_thread() noexcept
    : arg_(0)
  {
  }

  // Constructor.
  template <typename Function>
  win_thread(Function f, unsigned int stack_size = 0)
    : win_thread(std::allocator_arg, std::allocator<void>(), f, stack_size)
  {
  }

  // Construct with custom allocator.
  template <typename Allocator, typename Function>
  win_thread(allocator_arg_t, const Allocator& a,
      Function f, unsigned int stack_size = 0)
    : arg_(start_thread(allocate_object<func<Function, Allocator>>(a, f, a),
          stack_size))
  {
  }

  // Move constructor.
  win_thread(win_thread&& other) noexcept
    : arg_(other.arg_)
  {
    other.arg_ = 0;
  }

  // Destructor.
  ASIO_DECL ~win_thread();

  // Move assignment.
  win_thread& operator=(win_thread&& other) noexcept
  {
    arg_ = other.arg_;
    other.arg_ = 0;
    return *this;
  }

  // Whether the thread can be joined.
  bool joinable() const
  {
    return !!arg_;
  }

  // Wait for the thread to exit.
  ASIO_DECL void join();

  // Get number of CPUs.
  ASIO_DECL static std::size_t hardware_concurrency();

private:
  friend ASIO_DECL unsigned int __stdcall win_thread_function(void* arg);

#if defined(WINVER) && (WINVER < 0x0500)
  friend ASIO_DECL void __stdcall apc_function(ULONG);
#else
  friend ASIO_DECL void __stdcall apc_function(ULONG_PTR);
#endif

  class func_base
  {
  public:
    virtual ~func_base() {}
    virtual void run() = 0;
    virtual void destroy() = 0;
    ::HANDLE thread_;
    ::HANDLE entry_event_;
    ::HANDLE exit_event_;
  };

  template <typename Function, typename Allocator>
  class func
    : public func_base
  {
  public:
    func(Function f, const Allocator& a)
      : f_(f),
        allocator_(a)
    {
    }

    virtual void run()
    {
      f_();
    }

    virtual void destroy()
    {
      deallocate_object(allocator_, this);
    }

  private:
    Function f_;
    Allocator allocator_;
  };

  ASIO_DECL func_base* start_thread(
      func_base* arg, unsigned int stack_size);

  func_base* arg_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#if defined(ASIO_HEADER_ONLY)
# include "asio/detail/impl/win_thread.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // defined(ASIO_WINDOWS)
       // && !defined(ASIO_WINDOWS_APP)
       // && !defined(UNDER_CE)

#endif // ASIO_DETAIL_WIN_THREAD_HPP
