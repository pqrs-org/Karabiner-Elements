//
// detail/eager_wait_op.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_EAGER_WAIT_OP_HPP
#define ASIO_DETAIL_EAGER_WAIT_OP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/handler_tracking.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

template <typename Payload>
class eager_wait_op ASIO_INHERIT_TRACKED_HANDLER
{
public:
  void complete(Payload&& payload)
  {
    func_(this, complete_action, static_cast<void*>(&payload));
  }

  void destroy()
  {
    func_(this, destroy_action, 0);
  }

protected:
  enum action
  {
    destroy_action = 0,
    complete_action = 1
  };

  typedef void (*func_type)(eager_wait_op*, action, void*);

  eager_wait_op(func_type func)
    : func_(func)
  {
  }

  // Prevents deletion through this type.
  ~eager_wait_op()
  {
  }

  func_type func_;
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_EAGER_WAIT_OP_HPP
