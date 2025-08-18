//
// detail/impl/win_thread.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_WIN_THREAD_IPP
#define ASIO_DETAIL_IMPL_WIN_THREAD_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_WINDOWS) \
  && !defined(ASIO_WINDOWS_APP) \
  && !defined(UNDER_CE)

#include <process.h>
#include "asio/detail/throw_error.hpp"
#include "asio/detail/win_thread.hpp"
#include "asio/error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

win_thread::~win_thread()
{
  if (arg_)
    std::terminate();
}

void win_thread::join()
{
  if (arg_)
  {
    HANDLE handles[2] = { arg_->exit_event_, arg_->thread_ };
    ::WaitForMultipleObjects(2, handles, FALSE, INFINITE);
    ::CloseHandle(arg_->exit_event_);
    if (terminate_threads())
    {
      ::TerminateThread(arg_->thread_, 0);
    }
    else
    {
      ::QueueUserAPC(apc_function, arg_->thread_, 0);
      ::WaitForSingleObject(arg_->thread_, INFINITE);
    }
    ::CloseHandle(arg_->thread_);
    arg_->destroy();
    arg_ = 0;
  }
}

std::size_t win_thread::hardware_concurrency()
{
  SYSTEM_INFO system_info;
  ::GetSystemInfo(&system_info);
  return system_info.dwNumberOfProcessors;
}

win_thread::func_base* win_thread::start_thread(
    func_base* arg, unsigned int stack_size)
{
  arg->entry_event_ = ::CreateEventW(0, true, false, 0);
  if (!arg->entry_event_)
  {
    DWORD last_error = ::GetLastError();
    arg->destroy();
    asio::error_code ec(last_error,
        asio::error::get_system_category());
    asio::detail::throw_error(ec, "thread.entry_event");
  }

  arg->exit_event_ = ::CreateEventW(0, true, false, 0);
  if (!arg->exit_event_)
  {
    DWORD last_error = ::GetLastError();
    ::CloseHandle(arg->entry_event_);
    arg->destroy();
    asio::error_code ec(last_error,
        asio::error::get_system_category());
    asio::detail::throw_error(ec, "thread.exit_event");
  }

  unsigned int thread_id = 0;
  arg->thread_ = reinterpret_cast<HANDLE>(::_beginthreadex(0,
        stack_size, win_thread_function, arg, 0, &thread_id));
  if (!arg->thread_)
  {
    DWORD last_error = ::GetLastError();
    ::CloseHandle(arg->entry_event_);
    ::CloseHandle(arg->exit_event_);
    arg->destroy();
    asio::error_code ec(last_error,
        asio::error::get_system_category());
    asio::detail::throw_error(ec, "thread");
  }

  if (arg->entry_event_)
  {
    ::WaitForSingleObject(arg->entry_event_, INFINITE);
    ::CloseHandle(arg->entry_event_);
    arg->entry_event_ = 0;
  }

  return arg;
}

unsigned int __stdcall win_thread_function(void* arg)
{
  win_thread::func_base* func = static_cast<win_thread::func_base*>(arg);
  ::SetEvent(func->entry_event_);

  func->run();

  // Signal that the thread has finished its work, but rather than returning go
  // to sleep to put the thread into a well known state. If the thread is being
  // joined during global object destruction then it may be killed using
  // TerminateThread (to avoid a deadlock in DllMain). Otherwise, the SleepEx
  // call will be interrupted using QueueUserAPC and the thread will shut down
  // cleanly.
  ::SetEvent(func->exit_event_);
  ::SleepEx(INFINITE, TRUE);

  return 0;
}

#if defined(WINVER) && (WINVER < 0x0500)
void __stdcall apc_function(ULONG) {}
#else
void __stdcall apc_function(ULONG_PTR) {}
#endif

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_WINDOWS)
       // && !defined(ASIO_WINDOWS_APP)
       // && !defined(UNDER_CE)

#endif // ASIO_DETAIL_IMPL_WIN_THREAD_IPP
