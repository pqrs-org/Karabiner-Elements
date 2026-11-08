//
// detail/impl/reactive_socket_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_REACTIVE_SOCKET_SERVICE_HPP
#define ASIO_DETAIL_IMPL_REACTIVE_SOCKET_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/associated_cancellation_slot.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

template <typename Protocol>
template <typename ConstBufferSequence, typename Handler, typename IoExecutor>
void reactive_socket_service<Protocol>::async_send_to(
    implementation_type& impl, const ConstBufferSequence& buffers,
    const endpoint_type& destination, socket_base::message_flags flags,
    Handler& handler, const IoExecutor& io_ex)
{
  bool is_continuation =
    ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

  associated_cancellation_slot_t<Handler> slot
    = asio::get_associated_cancellation_slot(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef reactive_socket_sendto_op<ConstBufferSequence,
      endpoint_type, Handler, IoExecutor> op;
  typename op::ptr p = { asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  p.p = new (p.v) op(success_ec_, impl.socket_,
      buffers, destination, flags, handler, io_ex);

  // Optionally register for per-operation cancellation.
  if (slot.is_connected())
  {
    p.p->cancellation_key_ =
      &slot.template emplace<reactor_op_cancellation>(
          &reactor_, &impl.reactor_data_, impl.socket_, reactor::write_op);
  }

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "socket",
        &impl, impl.socket_, "async_send_to"));

  start_op<&op::do_perform, reactor::write_op>(impl, p.p, is_continuation,
      false, ASIO_OS_DEF(MSG_DONTWAIT) == 0, &io_ex, 0);
  p.v = p.p = 0;
}

template <typename Protocol>
template <typename Handler, typename IoExecutor>
void reactive_socket_service<Protocol>::async_send_to(
    implementation_type& impl, const null_buffers&,
    const endpoint_type&, socket_base::message_flags,
    Handler& handler, const IoExecutor& io_ex)
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
        &impl, impl.socket_, "async_send_to(null_buffers)"));

  start_op<nullptr, reactor::write_op>(impl, p.p,
      is_continuation, false, false, &io_ex, 0);
  p.v = p.p = 0;
}

template <typename Protocol>
template <typename MutableBufferSequence,
    typename Handler, typename IoExecutor>
void reactive_socket_service<Protocol>::async_receive_from(
    implementation_type& impl, const MutableBufferSequence& buffers,
    endpoint_type& sender_endpoint, socket_base::message_flags flags,
    Handler& handler, const IoExecutor& io_ex)
{
  bool is_continuation =
    ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

  associated_cancellation_slot_t<Handler> slot
    = asio::get_associated_cancellation_slot(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef reactive_socket_recvfrom_op<MutableBufferSequence,
      endpoint_type, Handler, IoExecutor> op;
  typename op::ptr p = { asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  int protocol = impl.protocol_.type();
  p.p = new (p.v) op(success_ec_, impl.socket_, protocol,
      buffers, sender_endpoint, flags, handler, io_ex);

  // Optionally register for per-operation cancellation.
  if (slot.is_connected())
  {
    p.p->cancellation_key_ =
      &slot.template emplace<reactor_op_cancellation>(
          &reactor_, &impl.reactor_data_, impl.socket_, reactor::read_op);
  }

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "socket",
        &impl, impl.socket_, "async_receive_from"));

  if (flags & socket_base::message_out_of_band)
  {
    start_op<&op::do_perform, reactor::except_op>(impl, p.p, is_continuation,
        false, ASIO_OS_DEF(MSG_DONTWAIT) == 0, &io_ex, 0);
  }
  else
  {
    start_op<&op::do_perform, reactor::read_op>(impl, p.p, is_continuation,
        false, ASIO_OS_DEF(MSG_DONTWAIT) == 0, &io_ex, 0);
  }
  p.v = p.p = 0;
}

template <typename Protocol>
template <typename Handler, typename IoExecutor>
void reactive_socket_service<Protocol>::async_receive_from(
    implementation_type& impl, const null_buffers&,
    endpoint_type& sender_endpoint, socket_base::message_flags flags,
    Handler& handler, const IoExecutor& io_ex)
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
        &impl, impl.socket_, "async_receive_from(null_buffers)"));

  // Reset endpoint since it can be given no sensible value at this time.
  sender_endpoint = endpoint_type();

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

template <typename Protocol>
template <typename Socket, typename Handler, typename IoExecutor>
void reactive_socket_service<Protocol>::async_accept(
    implementation_type& impl, Socket& peer,
    endpoint_type* peer_endpoint, Handler& handler,
    const IoExecutor& io_ex)
{
  bool is_continuation =
    ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

  associated_cancellation_slot_t<Handler> slot
    = asio::get_associated_cancellation_slot(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef reactive_socket_accept_op<Socket, Protocol, Handler, IoExecutor> op;
  typename op::ptr p = { asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  p.p = new (p.v) op(success_ec_, impl.socket_, impl.state_,
      peer, impl.protocol_, peer_endpoint, handler, io_ex);

  // Optionally register for per-operation cancellation.
  if (slot.is_connected() && !peer.is_open())
  {
    p.p->cancellation_key_ =
      &slot.template emplace<reactor_op_cancellation>(
          &reactor_, &impl.reactor_data_, impl.socket_, reactor::read_op);
  }

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "socket",
        &impl, impl.socket_, "async_accept"));

  start_accept_op<&op::do_perform>(impl, p.p,
      is_continuation, peer.is_open(), &io_ex, 0);
  p.v = p.p = 0;
}

template <typename Protocol>
template <typename PeerIoExecutor, typename Handler, typename IoExecutor>
void reactive_socket_service<Protocol>::async_move_accept(
    implementation_type& impl, const PeerIoExecutor& peer_io_ex,
    endpoint_type* peer_endpoint, Handler& handler,
    const IoExecutor& io_ex)
{
  bool is_continuation =
    ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

  associated_cancellation_slot_t<Handler> slot
    = asio::get_associated_cancellation_slot(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef reactive_socket_move_accept_op<Protocol,
      PeerIoExecutor, Handler, IoExecutor> op;
  typename op::ptr p = { asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  p.p = new (p.v) op(success_ec_, peer_io_ex, impl.socket_,
      impl.state_, impl.protocol_, peer_endpoint, handler, io_ex);

  // Optionally register for per-operation cancellation.
  if (slot.is_connected())
  {
    p.p->cancellation_key_ =
      &slot.template emplace<reactor_op_cancellation>(
          &reactor_, &impl.reactor_data_, impl.socket_, reactor::read_op);
  }

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "socket",
        &impl, impl.socket_, "async_accept"));

  start_accept_op<&op::do_perform>(impl, p.p,
      is_continuation, false, &io_ex, 0);
  p.v = p.p = 0;
}

template <typename Protocol>
template <typename Handler, typename IoExecutor>
void reactive_socket_service<Protocol>::async_connect(
    implementation_type& impl, const endpoint_type& peer_endpoint,
    Handler& handler, const IoExecutor& io_ex)
{
  bool is_continuation =
    ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

  associated_cancellation_slot_t<Handler> slot
    = asio::get_associated_cancellation_slot(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef reactive_socket_connect_op<Handler, IoExecutor> op;
  typename op::ptr p = { asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  p.p = new (p.v) op(success_ec_, impl.socket_, handler, io_ex);

  // Optionally register for per-operation cancellation.
  if (slot.is_connected())
  {
    p.p->cancellation_key_ =
      &slot.template emplace<reactor_op_cancellation>(
          &reactor_, &impl.reactor_data_, impl.socket_, reactor::connect_op);
  }

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "socket",
        &impl, impl.socket_, "async_connect"));

  start_connect_op(impl, p.p, is_continuation,
      peer_endpoint.data(), peer_endpoint.size(), &io_ex, 0);
  p.v = p.p = 0;
}

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_IMPL_REACTIVE_SOCKET_SERVICE_HPP
