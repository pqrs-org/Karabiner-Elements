//
// detail/impl/reactive_socket_service_base.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_REACTIVE_SOCKET_SERVICE_BASE_HPP
#define ASIO_DETAIL_IMPL_REACTIVE_SOCKET_SERVICE_BASE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

template <typename Handler, typename IoExecutor>
void reactive_socket_service_base::async_wait(
    base_implementation_type& impl, socket_base::wait_type w,
    Handler& handler, const IoExecutor& io_ex)
{
  bool is_continuation =
    ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

  associated_cancellation_slot_t<Handler> slot
    = asio::get_associated_cancellation_slot(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef reactive_wait_op<Handler, IoExecutor> op;
  typename op::ptr p = { asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  p.p = new (p.v) op(success_ec_, handler, io_ex);

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "socket",
        &impl, impl.socket_, "async_wait"));

  int op_type;
  switch (w)
  {
  case socket_base::wait_read:
    op_type = reactor::read_op;
    break;
  case socket_base::wait_write:
    op_type = reactor::write_op;
    break;
  case socket_base::wait_error:
    op_type = reactor::except_op;
    break;
  default:
    p.p->ec_ = asio::error::invalid_argument;
    start_op<nullptr, reactor::read_op>(impl, p.p,
        is_continuation, true, false, &io_ex, 0);
    p.v = p.p = 0;
    return;
  }

  // Optionally register for per-operation cancellation.
  if (slot.is_connected())
  {
    p.p->cancellation_key_ =
      &slot.template emplace<reactor_op_cancellation>(
          &reactor_, &impl.reactor_data_, impl.socket_, op_type);
  }

  switch (op_type)
  {
  case reactor::read_op:
    start_op<nullptr, reactor::read_op>(impl, p.p,
        is_continuation, false, false, &io_ex, 0);
    break;
  case reactor::write_op:
    start_op<nullptr, reactor::write_op>(impl, p.p,
        is_continuation, false, false, &io_ex, 0);
    break;
  default:
    start_op<nullptr, reactor::except_op>(impl, p.p,
        is_continuation, false, false, &io_ex, 0);
    break;
  }
  p.v = p.p = 0;
}

template <typename ConstBufferSequence, typename Handler, typename IoExecutor>
void reactive_socket_service_base::async_send(
    base_implementation_type& impl, const ConstBufferSequence& buffers,
    socket_base::message_flags flags, Handler& handler,
    const IoExecutor& io_ex)
{
  bool is_continuation =
    ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

  associated_cancellation_slot_t<Handler> slot
    = asio::get_associated_cancellation_slot(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef reactive_socket_send_op<
      ConstBufferSequence, Handler, IoExecutor> op;
  typename op::ptr p = { asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  p.p = new (p.v) op(success_ec_, impl.socket_,
      impl.state_, buffers, flags, handler, io_ex);

  // Optionally register for per-operation cancellation.
  if (slot.is_connected())
  {
    p.p->cancellation_key_ =
      &slot.template emplace<reactor_op_cancellation>(
          &reactor_, &impl.reactor_data_, impl.socket_, reactor::write_op);
  }

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "socket",
        &impl, impl.socket_, "async_send"));

  ASIO_INVOKE_INLINE start_op<&op::do_perform, reactor::write_op>(
      impl, p.p, is_continuation,
      ((impl.state_ & socket_ops::stream_oriented)
        && buffer_sequence_adapter<asio::const_buffer,
          ConstBufferSequence>::all_empty(buffers)),
      ASIO_OS_DEF(MSG_DONTWAIT) == 0, &io_ex, 0);
  p.v = p.p = 0;
}

template <typename Handler, typename IoExecutor>
void reactive_socket_service_base::async_send(
    base_implementation_type& impl, const null_buffers&,
    socket_base::message_flags, Handler& handler,
    const IoExecutor& io_ex)
{
  bool is_continuation =
    ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

  associated_cancellation_slot_t<Handler> slot
    = asio::get_associated_cancellation_slot(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef reactive_null_buffers_op<Handler, IoExecutor> op;
  typename op::ptr p = { asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  p.p = new (p.v) op(success_ec_, handler, io_ex);

  // Optionally register for per-operation cancellation.
  if (slot.is_connected())
  {
    p.p->cancellation_key_ =
      &slot.template emplace<reactor_op_cancellation>(
          &reactor_, &impl.reactor_data_, impl.socket_, reactor::write_op);
  }

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "socket",
        &impl, impl.socket_, "async_send(null_buffers)"));

  start_op<nullptr, reactor::write_op>(impl, p.p,
      is_continuation, false, false, &io_ex, 0);
  p.v = p.p = 0;
}

template <typename MutableBufferSequence,
    typename Handler, typename IoExecutor>
void reactive_socket_service_base::async_receive(
    base_implementation_type& impl, const MutableBufferSequence& buffers,
    socket_base::message_flags flags, Handler& handler,
    const IoExecutor& io_ex)
{
  bool is_continuation =
    ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

  associated_cancellation_slot_t<Handler> slot
    = asio::get_associated_cancellation_slot(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef reactive_socket_recv_op<
      MutableBufferSequence, Handler, IoExecutor> op;
  typename op::ptr p = { asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  p.p = new (p.v) op(success_ec_, impl.socket_,
      impl.state_, buffers, flags, handler, io_ex);

  // Optionally register for per-operation cancellation.
  if (slot.is_connected())
  {
    p.p->cancellation_key_ =
      &slot.template emplace<reactor_op_cancellation>(
          &reactor_, &impl.reactor_data_, impl.socket_, reactor::read_op);
  }

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "socket",
        &impl, impl.socket_, "async_receive"));

  const bool noop = (impl.state_ & socket_ops::stream_oriented)
    && buffer_sequence_adapter<asio::mutable_buffer,
      MutableBufferSequence>::all_empty(buffers);

  if (flags & socket_base::message_out_of_band)
  {
    start_op<nullptr, reactor::except_op>(impl, p.p, is_continuation,
        noop, ASIO_OS_DEF(MSG_DONTWAIT) == 0, &io_ex, 0);
  }
  else
  {
    ASIO_INVOKE_INLINE start_op<&op::do_perform, reactor::read_op>(
        impl, p.p, is_continuation, noop,
        ASIO_OS_DEF(MSG_DONTWAIT) == 0, &io_ex, 0);
  }
  p.v = p.p = 0;
}

template <typename Handler, typename IoExecutor>
void reactive_socket_service_base::async_receive(
    base_implementation_type& impl, const null_buffers&,
    socket_base::message_flags flags, Handler& handler,
    const IoExecutor& io_ex)
{
  bool is_continuation =
    ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

  associated_cancellation_slot_t<Handler> slot
    = asio::get_associated_cancellation_slot(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef reactive_null_buffers_op<Handler, IoExecutor> op;
  typename op::ptr p = { asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  p.p = new (p.v) op(success_ec_, handler, io_ex);

  // Optionally register for per-operation cancellation.
  if (slot.is_connected())
  {
    p.p->cancellation_key_ =
      &slot.template emplace<reactor_op_cancellation>(
          &reactor_, &impl.reactor_data_, impl.socket_, reactor::read_op);
  }

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "socket",
        &impl, impl.socket_, "async_receive(null_buffers)"));

  if (flags & socket_base::message_out_of_band)
  {
    start_op<nullptr, reactor::except_op>(impl, p.p,
        is_continuation, false, false, &io_ex, 0);
  }
  else
  {
    start_op<nullptr, reactor::read_op>(impl, p.p,
        is_continuation, false, false, &io_ex, 0);
  }
  p.v = p.p = 0;
}

template <typename MutableBufferSequence,
    typename Handler, typename IoExecutor>
void reactive_socket_service_base::async_receive_with_flags(
    base_implementation_type& impl, const MutableBufferSequence& buffers,
    socket_base::message_flags in_flags,
    socket_base::message_flags& out_flags, Handler& handler,
    const IoExecutor& io_ex)
{
  bool is_continuation =
    ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

  associated_cancellation_slot_t<Handler> slot
    = asio::get_associated_cancellation_slot(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef reactive_socket_recvmsg_op<
      MutableBufferSequence, Handler, IoExecutor> op;
  typename op::ptr p = { asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  p.p = new (p.v) op(success_ec_, impl.socket_,
      buffers, in_flags, out_flags, handler, io_ex);

  // Optionally register for per-operation cancellation.
  if (slot.is_connected())
  {
    p.p->cancellation_key_ =
      &slot.template emplace<reactor_op_cancellation>(
          &reactor_, &impl.reactor_data_, impl.socket_, reactor::read_op);
  }

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "socket",
        &impl, impl.socket_, "async_receive_with_flags"));

  // Out-of-band data is never received speculatively.
  if (in_flags & socket_base::message_out_of_band)
  {
    start_op<nullptr, reactor::except_op>(impl, p.p, is_continuation,
        false, ASIO_OS_DEF(MSG_DONTWAIT) == 0, &io_ex, 0);
  }
  else
  {
    start_op<&op::do_perform, reactor::read_op>(impl, p.p, is_continuation,
        false, ASIO_OS_DEF(MSG_DONTWAIT) == 0, &io_ex, 0);
  }
  p.v = p.p = 0;
}

template <typename Handler, typename IoExecutor>
void reactive_socket_service_base::async_receive_with_flags(
    base_implementation_type& impl, const null_buffers&,
    socket_base::message_flags in_flags,
    socket_base::message_flags& out_flags, Handler& handler,
    const IoExecutor& io_ex)
{
  bool is_continuation =
    ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

  associated_cancellation_slot_t<Handler> slot
    = asio::get_associated_cancellation_slot(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef reactive_null_buffers_op<Handler, IoExecutor> op;
  typename op::ptr p = { asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  p.p = new (p.v) op(success_ec_, handler, io_ex);

  // Optionally register for per-operation cancellation.
  if (slot.is_connected())
  {
    p.p->cancellation_key_ =
      &slot.template emplace<reactor_op_cancellation>(
          &reactor_, &impl.reactor_data_, impl.socket_, reactor::read_op);
  }

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "socket",
        &impl, impl.socket_, "async_receive_with_flags(null_buffers)"));

  // Clear out_flags, since we cannot give it any other sensible value when
  // performing a null_buffers operation.
  out_flags = 0;

  if (in_flags & socket_base::message_out_of_band)
  {
    start_op<nullptr, reactor::except_op>(impl, p.p,
        is_continuation, false, false, &io_ex, 0);
  }
  else
  {
    start_op<nullptr, reactor::read_op>(impl, p.p,
        is_continuation, false, false, &io_ex, 0);
  }
  p.v = p.p = 0;
}

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_IMPL_REACTIVE_SOCKET_SERVICE_BASE_HPP
