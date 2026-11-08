//
// detail/eager_wait_handler.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_EAGER_WAIT_HANDLER_HPP
#define ASIO_DETAIL_EAGER_WAIT_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/bind_handler.hpp"
#include "asio/detail/completion_payload_handler.hpp"
#include "asio/detail/eager_wait_op.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/handler_work.hpp"
#include "asio/detail/memory.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

template <typename Payload, typename Handler, typename IoExecutor>
class eager_wait_handler : public eager_wait_op<Payload>
{
public:
  ASIO_DEFINE_HANDLER_PTR(eager_wait_handler);

  template <typename Handler_>
  eager_wait_handler(Handler_&& h, const IoExecutor& io_ex)
    : eager_wait_op<Payload>(&eager_wait_handler::do_complete),
      handler_(static_cast<Handler_&&>(h)),
      work_(handler_, io_ex)
  {
  }

  static void do_complete(eager_wait_op<Payload>* base,
      typename eager_wait_op<Payload>::action act, void* payload)
  {
    // Take ownership of the handler object.
    eager_wait_handler* h(static_cast<eager_wait_handler*>(base));
    ptr p = { detail::addressof(h->handler_), h, h };

    ASIO_HANDLER_COMPLETION((*h));

    // Take ownership of the operation's outstanding work.
    handler_work<Handler, IoExecutor> w(
        static_cast<handler_work<Handler, IoExecutor>&&>(h->work_));

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    if (act == eager_wait_op<Payload>::complete_action)
    {
      completion_payload_handler<Payload, Handler> handler(
          static_cast<Payload&&>(*static_cast<Payload*>(payload)),
          h->handler_);
      p.h = detail::addressof(handler.handler_);
      p.reset();

      // Make the upcall if required.
      fenced_block b(fenced_block::half);
      ASIO_HANDLER_INVOCATION_BEGIN(());
      w.complete(handler, handler.handler_);
      ASIO_HANDLER_INVOCATION_END;
    }
    else
    {
      binder0<Handler> handler(h->handler_);
      p.h = detail::addressof(handler.handler_);
      p.reset();
    }
  }

  void immediate(Payload&& payload, const IoExecutor& io_ex)
  {
    // Take ownership of the handler object.
    ptr p = { detail::addressof(handler_), this, this };

    ASIO_HANDLER_COMPLETION((*this));

    // Take ownership of the operation's outstanding work.
    immediate_handler_work<Handler, IoExecutor> w(
        static_cast<handler_work<Handler, IoExecutor>&&>(work_));

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    completion_payload_handler<Payload, Handler> handler(
        static_cast<Payload&&>(payload), handler_);
    p.h = detail::addressof(handler.handler_);
    p.reset();

    ASIO_HANDLER_INVOCATION_BEGIN(());
    w.complete(handler, handler.handler_, &io_ex);
    ASIO_HANDLER_INVOCATION_END;
  }

private:
  Handler handler_;
  handler_work<Handler, IoExecutor> work_;
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_EAGER_WAIT_HANDLER_HPP
