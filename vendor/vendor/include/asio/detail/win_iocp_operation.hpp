//
// detail/win_iocp_operation.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_WIN_IOCP_OPERATION_HPP
#define ASIO_DETAIL_WIN_IOCP_OPERATION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_IOCP)

#include "asio/detail/handler_tracking.hpp"
#include "asio/detail/op_queue.hpp"
#include "asio/detail/socket_types.hpp"
#include "asio/error.hpp"
#include "asio/error_code.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

class scheduler;
class win_iocp_io_context;

// Base class for all operations. A function pointer is used instead of virtual
// functions to avoid the associated overhead.
class win_iocp_operation
  : public OVERLAPPED
    ASIO_ALSO_INHERIT_TRACKED_HANDLER
{
public:
  typedef win_iocp_operation operation_type;

  void complete(void* owner, int error_value, std::size_t bytes_transferred)
  {
    func_(owner, this, error_value, bytes_transferred);
  }

  void destroy()
  {
    Internal = reinterpret_cast<ULONG_PTR>(
        &asio::error::get_system_category());
    func_(0, this, 0, 0);
  }

  asio::error_code make_error_code(int error_value) const
  {
    return asio::error_code(error_value,
        *reinterpret_cast<asio::error_category*>(Internal));
  }

  void reset()
  {
    Internal = 0;
    InternalHigh = 0;
    Offset = 0;
    OffsetHigh = 0;
    hEvent = 0;
    ready_ = 0;
  }

protected:
  typedef void (*func_type)(void*, win_iocp_operation*, int, std::size_t);

  win_iocp_operation(func_type func)
    : next_(0),
      func_(func),
      task_result_(0)
  {
    reset();
  }

  // Prevents deletion through this type.
  ~win_iocp_operation()
  {
  }

private:
  friend class op_queue_access;
  friend class win_iocp_io_context;
  friend class scheduler;
  win_iocp_operation* next_;
  func_type func_;
  LONG ready_;
  unsigned int task_result_; // Used by scheduler to pass as bytes_transferred.
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_IOCP)

#endif // ASIO_DETAIL_WIN_IOCP_OPERATION_HPP
