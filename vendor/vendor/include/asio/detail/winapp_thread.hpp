//
// detail/winapp_thread.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_WINAPP_THREAD_HPP
#define ASIO_DETAIL_WINAPP_THREAD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_WINDOWS) && defined(ASIO_WINDOWS_APP)

#include "asio/detail/memory.hpp"
#include "asio/detail/socket_types.hpp"
#include "asio/detail/throw_error.hpp"
#include "asio/error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

DWORD WINAPI winapp_thread_function(LPVOID arg);

class winapp_thread
{
public:
  // Construct in a non-joinable state.
  winapp_thread() noexcept
    : arg_(0)
  {
  }

  // Constructor.
  template <typename Function>
  winapp_thread(Function f, unsigned int = 0)
    : winapp_thread(std::allocator_arg, std::allocator<void>(), f)
  {
  }

  // Construct with custom allocator.
  template <typename Allocator, typename Function>
  winapp_thread(allocator_arg_t, const Allocator& a,
      Function f, unsigned int = 0)
    : arg_(start_thread(allocate_object<func<Function, Allocator>>(a, f, a)))
  {
  }

  // Move constructor.
  winapp_thread(winapp_thread&& other) noexcept
    : arg_(other.arg_)
  {
    other.arg_ = 0;
  }

  // Destructor.
  ~winapp_thread()
  {
    if (arg_)
      std::terminate();
  }

  // Move assignment.
  winapp_thread& operator=(winapp_thread&& other) noexcept
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
  void join()
  {
    if (arg_)
      ::WaitForSingleObjectEx(arg_->thread_, INFINITE, false);
  }

  // Get number of CPUs.
  static std::size_t hardware_concurrency()
  {
    SYSTEM_INFO system_info;
    ::GetNativeSystemInfo(&system_info);
    return system_info.dwNumberOfProcessors;
  }

private:
  friend DWORD WINAPI winapp_thread_function(LPVOID arg);

  class func_base
  {
  public:
    virtual ~func_base() {}
    virtual void run() = 0;
    virtual void destroy() = 0;
    ::HANDLE thread_;
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

  func_base* start_thread(func_base* arg)
  {
    DWORD thread_id = 0;
    arg->thread_ = ::CreateThread(0, 0,
        winapp_thread_function, arg, 0, &thread_id);
    if (!arg->thread_)
    {
      arg->destroy();
      DWORD last_error = ::GetLastError();
      asio::error_code ec(last_error,
          asio::error::get_system_category());
      asio::detail::throw_error(ec, "thread");
    }
    return arg;
  }

  func_base* arg_;
};

inline DWORD WINAPI winapp_thread_function(LPVOID arg)
{
  static_cast<winapp_thread::func_base*>(arg)->run();
  return 0;
}

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_WINDOWS) && defined(UNDER_CE)

#endif // ASIO_DETAIL_WINAPP_THREAD_HPP
