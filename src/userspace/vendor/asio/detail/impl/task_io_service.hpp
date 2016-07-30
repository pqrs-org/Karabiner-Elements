//
// detail/impl/task_io_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_TASK_IO_SERVICE_HPP
#define ASIO_DETAIL_IMPL_TASK_IO_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/addressof.hpp"
#include "asio/detail/completion_handler.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/handler_cont_helpers.hpp"
#include "asio/detail/handler_invoke_helpers.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

template <typename Handler>
void task_io_service::dispatch(Handler& handler)
{
  if (thread_call_stack::contains(this))
  {
    fenced_block b(fenced_block::full);
    asio_handler_invoke_helpers::invoke(handler, handler);
  }
  else
  {
    // Allocate and construct an operation to wrap the handler.
    typedef completion_handler<Handler> op;
    typename op::ptr p = { asio::detail::addressof(handler),
      asio_handler_alloc_helpers::allocate(
        sizeof(op), handler), 0 };
    p.p = new (p.v) op(handler);

    ASIO_HANDLER_CREATION((p.p, "io_service", this, "dispatch"));

    do_dispatch(p.p);
    p.v = p.p = 0;
  }
}

template <typename Handler>
void task_io_service::post(Handler& handler)
{
  bool is_continuation =
    asio_handler_cont_helpers::is_continuation(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef completion_handler<Handler> op;
  typename op::ptr p = { asio::detail::addressof(handler),
    asio_handler_alloc_helpers::allocate(
      sizeof(op), handler), 0 };
  p.p = new (p.v) op(handler);

  ASIO_HANDLER_CREATION((p.p, "io_service", this, "post"));

  post_immediate_completion(p.p, is_continuation);
  p.v = p.p = 0;
}

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_IMPL_TASK_IO_SERVICE_HPP
