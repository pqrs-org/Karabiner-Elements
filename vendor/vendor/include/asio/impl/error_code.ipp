//
// impl/error_code.ipp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_ERROR_CODE_IPP
#define ASIO_IMPL_ERROR_CODE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#if defined(ASIO_WINDOWS) || defined(__CYGWIN__)
# include <winerror.h>
#elif defined(ASIO_WINDOWS_RUNTIME)
# include <windows.h>
#else
# include <cerrno>
# include <cstring>
# include <string>
#endif
#include "asio/detail/local_free_on_block_exit.hpp"
#include "asio/detail/socket_types.hpp"
#include "asio/error_code.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace error {
namespace detail {

ASIO_DECL std::error_condition error_number_to_condition(int ev);

} // namespace detail
} // namespace error
namespace detail {

class system_category : public error_category
{
public:
  const char* name() const noexcept
  {
    return "asio.system";
  }

  std::string message(int value) const
  {
#if defined(ASIO_WINDOWS_RUNTIME) || defined(ASIO_WINDOWS_APP)
    std::wstring wmsg(128, wchar_t());
    for (;;)
    {
      DWORD wlength = ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM
          | FORMAT_MESSAGE_IGNORE_INSERTS, 0, value,
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          &wmsg[0], static_cast<DWORD>(wmsg.size()), 0);
      if (wlength == 0 && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
      {
        wmsg.resize(wmsg.size() + wmsg.size() / 2);
        continue;
      }
      if (wlength && wmsg[wlength - 1] == '\n')
        --wlength;
      if (wlength && wmsg[wlength - 1] == '\r')
        --wlength;
      if (wlength)
      {
        std::string msg(wlength * 2, char());
        int length = ::WideCharToMultiByte(CP_ACP, 0,
            wmsg.c_str(), static_cast<int>(wlength),
            &msg[0], static_cast<int>(wlength * 2), 0, 0);
        if (length <= 0)
          return "asio.system error";
        msg.resize(static_cast<std::size_t>(length));
        return msg;
      }
      else
        return "asio.system error";
    }
#elif defined(ASIO_WINDOWS) || defined(__CYGWIN__)
    char* msg = 0;
    DWORD length = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER
        | FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS, 0, value,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&msg, 0, 0);
    detail::local_free_on_block_exit local_free_obj(msg);
    if (length && msg[length - 1] == '\n')
      msg[--length] = '\0';
    if (length && msg[length - 1] == '\r')
      msg[--length] = '\0';
    if (length)
      return msg;
    else
      return "asio.system error";
#else // defined(ASIO_WINDOWS_DESKTOP) || defined(__CYGWIN__)
#if !defined(__sun)
    if (value == ECANCELED)
      return "Operation aborted.";
#endif // !defined(__sun)
#if defined(__sun) || defined(__QNX__) || defined(__SYMBIAN32__)
    using namespace std;
    return strerror(value);
#else
    char buf[256] = "";
    using namespace std;
    return strerror_result(strerror_r(value, buf, sizeof(buf)), buf);
#endif
#endif // defined(ASIO_WINDOWS_DESKTOP) || defined(__CYGWIN__)
  }

  std::error_condition default_error_condition(int ev) const noexcept
  {
    return error::detail::error_number_to_condition(ev);
  }

private:
  // Helper function to adapt the result from glibc's variant of strerror_r.
  static const char* strerror_result(int, const char* s) { return s; }
  static const char* strerror_result(const char* s, const char*) { return s; }
};

} // namespace detail

const error_category& system_category()
{
  static detail::system_category instance;
  return instance;
}

} // namespace asio

#include "asio/detail/pop_options.hpp"

// For implementation of asio::error::detail::error_number_to_condition.
#include "asio/error.hpp"

#endif // ASIO_IMPL_ERROR_CODE_IPP
