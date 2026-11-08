//
// detail/scheduler_operation.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_SCHEDULER_OPERATION_HPP
#define ASIO_DETAIL_SCHEDULER_OPERATION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/error_code.hpp"
#include "asio/detail/handler_tracking.hpp"
#include "asio/detail/op_queue.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

class scheduler;

// Base class for all operations. A function pointer is used instead of virtual
// functions to avoid the associated overhead.
class scheduler_operation ASIO_INHERIT_TRACKED_HANDLER
{
public:
  typedef scheduler_operation operation_type;

  void complete(void* owner, int error_value, std::size_t bytes_transferred)
  {
    func_(owner, this, error_value, bytes_transferred);
  }

  void destroy()
  {
    func_(0, this, 0, 0);
  }

protected:
  typedef void (*func_type)(void*, scheduler_operation*, int, std::size_t);

  scheduler_operation(func_type func)
    : next_(0),
      func_(func),
      task_result_(0)
  {
  }

  // Prevents deletion through this type.
  ~scheduler_operation()
  {
  }

private:
  friend class op_queue_access;
  scheduler_operation* next_ ASIO_UNUSED_MEMBER;
  func_type func_;
protected:
  friend class scheduler;
  unsigned int task_result_; // Passed into bytes transferred.
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_SCHEDULER_OPERATION_HPP
