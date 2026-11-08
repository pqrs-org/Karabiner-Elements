//
// detail/impl/reactive_descriptor_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_REACTIVE_DESCRIPTOR_SERVICE_HPP
#define ASIO_DETAIL_IMPL_REACTIVE_DESCRIPTOR_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

template <typename Handler, typename IoExecutor>
void reactive_descriptor_service::async_wait(
    implementation_type& impl, posix::descriptor_base::wait_type w,
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

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "descriptor",
        &impl, impl.descriptor_, "async_wait"));

  int op_type;
  switch (w)
  {
  case posix::descriptor_base::wait_read:
    op_type = reactor::read_op;
    break;
  case posix::descriptor_base::wait_write:
    op_type = reactor::write_op;
    break;
  case posix::descriptor_base::wait_error:
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
          &reactor_, &impl.reactor_data_, impl.descriptor_, op_type);
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
void reactive_descriptor_service::async_write_some(
    implementation_type& impl, const ConstBufferSequence& buffers,
    Handler& handler, const IoExecutor& io_ex)
{
  bool is_continuation =
    ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

  associated_cancellation_slot_t<Handler> slot
    = asio::get_associated_cancellation_slot(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef descriptor_write_op<ConstBufferSequence, Handler, IoExecutor> op;
  typename op::ptr p = { asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  p.p = new (p.v) op(success_ec_, impl.descriptor_, buffers, handler, io_ex);

  // Optionally register for per-operation cancellation.
  if (slot.is_connected())
  {
    p.p->cancellation_key_ =
      &slot.template emplace<reactor_op_cancellation>(
          &reactor_, &impl.reactor_data_,
          impl.descriptor_, reactor::write_op);
  }

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "descriptor",
        &impl, impl.descriptor_, "async_write_some"));

  start_op<&op::do_perform, reactor::write_op>(impl, p.p, is_continuation,
      buffer_sequence_adapter<asio::const_buffer,
        ConstBufferSequence>::all_empty(buffers), true, &io_ex, 0);
  p.v = p.p = 0;
}

template <typename Handler, typename IoExecutor>
void reactive_descriptor_service::async_write_some(
    implementation_type& impl, const null_buffers&,
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
          &reactor_, &impl.reactor_data_,
          impl.descriptor_, reactor::write_op);
  }

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "descriptor",
        &impl, impl.descriptor_, "async_write_some(null_buffers)"));

  start_op<nullptr, reactor::write_op>(impl, p.p,
      is_continuation, false, false, &io_ex, 0);
  p.v = p.p = 0;
}

template <typename MutableBufferSequence,
    typename Handler, typename IoExecutor>
void reactive_descriptor_service::async_read_some(
    implementation_type& impl, const MutableBufferSequence& buffers,
    Handler& handler, const IoExecutor& io_ex)
{
  bool is_continuation =
    ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

  associated_cancellation_slot_t<Handler> slot
    = asio::get_associated_cancellation_slot(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef descriptor_read_op<MutableBufferSequence, Handler, IoExecutor> op;
  typename op::ptr p = { asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  p.p = new (p.v) op(success_ec_, impl.descriptor_, buffers, handler, io_ex);

  // Optionally register for per-operation cancellation.
  if (slot.is_connected())
  {
    p.p->cancellation_key_ =
      &slot.template emplace<reactor_op_cancellation>(
          &reactor_, &impl.reactor_data_,
          impl.descriptor_, reactor::read_op);
  }

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "descriptor",
        &impl, impl.descriptor_, "async_read_some"));

  start_op<&op::do_perform, reactor::read_op>(impl, p.p, is_continuation,
      buffer_sequence_adapter<asio::mutable_buffer,
        MutableBufferSequence>::all_empty(buffers), true, &io_ex, 0);
  p.v = p.p = 0;
}

template <typename Handler, typename IoExecutor>
void reactive_descriptor_service::async_read_some(
    implementation_type& impl, const null_buffers&,
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
          &reactor_, &impl.reactor_data_,
          impl.descriptor_, reactor::read_op);
  }

  ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "descriptor",
        &impl, impl.descriptor_, "async_read_some(null_buffers)"));

  start_op<nullptr, reactor::read_op>(impl, p.p,
      is_continuation, false, false, &io_ex, 0);
  p.v = p.p = 0;
}

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_IMPL_REACTIVE_DESCRIPTOR_SERVICE_HPP
