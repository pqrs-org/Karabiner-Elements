//
// detail/reactive_socket_service_base.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_REACTIVE_SOCKET_SERVICE_BASE_HPP
#define ASIO_DETAIL_REACTIVE_SOCKET_SERVICE_BASE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/io_control.hpp"

#if !defined(ASIO_HAS_IOCP) \
  && !defined(ASIO_WINDOWS_RUNTIME) \
  && !defined(ASIO_HAS_IO_URING_AS_DEFAULT)

#include "asio/associated_cancellation_slot.hpp"
#include "asio/buffer.hpp"
#include "asio/cancellation_type.hpp"
#include "asio/error.hpp"
#include "asio/execution_context.hpp"
#include "asio/socket_base.hpp"
#include "asio/detail/buffer_sequence_adapter.hpp"
#include "asio/detail/buffer_sequences_adapter.hpp"
#include "asio/detail/native_executors.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/reactive_null_buffers_op.hpp"
#include "asio/detail/reactive_socket_recv_op.hpp"
#include "asio/detail/reactive_socket_recvmmsg_op.hpp"
#include "asio/detail/reactive_socket_recvmsg_op.hpp"
#include "asio/detail/reactive_socket_send_op.hpp"
#include "asio/detail/reactive_socket_sendmmsg_op.hpp"
#include "asio/detail/reactive_wait_op.hpp"
#include "asio/detail/reactor.hpp"
#include "asio/detail/reactor_op.hpp"
#include "asio/detail/socket_holder.hpp"
#include "asio/detail/socket_ops.hpp"
#include "asio/detail/socket_types.hpp"
#include "asio/detail/type_traits.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

class reactive_socket_service_base
{
public:
  // The native type of a socket.
  typedef socket_type native_handle_type;

  // The implementation type of the socket.
  struct base_implementation_type
  {
    // The native socket representation.
    socket_type socket_;

    // The current state of the socket.
    socket_ops::state_type state_;

    // Per-descriptor data used by the reactor.
    reactor::per_descriptor_data reactor_data_;
  };

  // Constructor.
  ASIO_DECL reactive_socket_service_base(execution_context& context);

  // Destroy all user-defined handler objects owned by the service.
  ASIO_DECL void base_shutdown();

  // Construct a new socket implementation.
  ASIO_DECL void construct(base_implementation_type& impl);

  // Move-construct a new socket implementation.
  ASIO_DECL void base_move_construct(base_implementation_type& impl,
      base_implementation_type& other_impl) noexcept;

  // Move-assign from another socket implementation.
  ASIO_DECL void base_move_assign(base_implementation_type& impl,
      reactive_socket_service_base& other_service,
      base_implementation_type& other_impl);

  // Destroy a socket implementation.
  ASIO_DECL void destroy(base_implementation_type& impl);

  // Determine whether the socket is open.
  bool is_open(const base_implementation_type& impl) const
  {
    return impl.socket_ != invalid_socket;
  }

  // Destroy a socket implementation.
  ASIO_DECL asio::error_code close(
      base_implementation_type& impl, asio::error_code& ec);

  // Release ownership of the socket.
  ASIO_DECL socket_type release(
      base_implementation_type& impl, asio::error_code& ec);

  // Get the native socket representation.
  native_handle_type native_handle(base_implementation_type& impl)
  {
    return impl.socket_;
  }

  // Cancel all operations associated with the socket.
  ASIO_DECL asio::error_code cancel(
      base_implementation_type& impl, asio::error_code& ec);

  // Determine whether the socket is at the out-of-band data mark.
  bool at_mark(const base_implementation_type& impl,
      asio::error_code& ec) const
  {
    return socket_ops::sockatmark(impl.socket_, ec);
  }

  // Determine the number of bytes available for reading.
  std::size_t available(const base_implementation_type& impl,
      asio::error_code& ec) const
  {
    return socket_ops::available(impl.socket_, ec);
  }

  // Place the socket into the state where it will listen for new connections.
  asio::error_code listen(base_implementation_type& impl,
      int backlog, asio::error_code& ec)
  {
    socket_ops::listen(impl.socket_, backlog, ec);
    return ec;
  }

  // Perform an IO control command on the socket.
  template <typename IO_Control_Command>
  asio::error_code io_control(base_implementation_type& impl,
      IO_Control_Command& command, asio::error_code& ec)
  {
    const char* descriptor_name = 0;
    if (get_descriptor_name(command, descriptor_name, ec))
    {
      if (!ec)
        reactor_.set_descriptor_name(impl.reactor_data_, descriptor_name, ec);
      return ec;
    }

    socket_ops::ioctl(impl.socket_, impl.state_, command.name(),
        static_cast<ioctl_arg_type*>(command.data()), ec);
    return ec;
  }

  // Gets the non-blocking mode of the socket.
  bool non_blocking(const base_implementation_type& impl) const
  {
    return (impl.state_ & socket_ops::user_set_non_blocking) != 0;
  }

  // Sets the non-blocking mode of the socket.
  asio::error_code non_blocking(base_implementation_type& impl,
      bool mode, asio::error_code& ec)
  {
    socket_ops::set_user_non_blocking(impl.socket_, impl.state_, mode, ec);
    return ec;
  }

  // Gets the non-blocking mode of the native socket implementation.
  bool native_non_blocking(const base_implementation_type& impl) const
  {
    return (impl.state_ & socket_ops::internal_non_blocking) != 0;
  }

  // Sets the non-blocking mode of the native socket implementation.
  asio::error_code native_non_blocking(base_implementation_type& impl,
      bool mode, asio::error_code& ec)
  {
    socket_ops::set_internal_non_blocking(impl.socket_, impl.state_, mode, ec);
    return ec;
  }

  // Wait for the socket to become ready to read, ready to write, or to have
  // pending error conditions.
  asio::error_code wait(base_implementation_type& impl,
      socket_base::wait_type w, asio::error_code& ec)
  {
    switch (w)
    {
    case socket_base::wait_read:
      socket_ops::poll_read(impl.socket_, impl.state_, -1, ec);
      break;
    case socket_base::wait_write:
      socket_ops::poll_write(impl.socket_, impl.state_, -1, ec);
      break;
    case socket_base::wait_error:
      socket_ops::poll_error(impl.socket_, impl.state_, -1, ec);
      break;
    default:
      ec = asio::error::invalid_argument;
      break;
    }

    return ec;
  }

  // Asynchronously wait for the socket to become ready to read, ready to
  // write, or to have pending error conditions.
  template <typename Handler, typename IoExecutor>
  void async_wait(base_implementation_type& impl,
      socket_base::wait_type w, Handler& handler, const IoExecutor& io_ex);

  // Send the given data to the peer.
  template <typename ConstBufferSequence>
  size_t send(base_implementation_type& impl,
      const ConstBufferSequence& buffers,
      socket_base::message_flags flags, asio::error_code& ec)
  {
    typedef buffer_sequence_adapter<asio::const_buffer,
        ConstBufferSequence> bufs_type;

    if (bufs_type::is_single_buffer)
    {
      return socket_ops::sync_send1(impl.socket_,
          impl.state_, bufs_type::first(buffers).data(),
          bufs_type::first(buffers).size(), flags, ec);
    }
    else
    {
      bufs_type bufs(buffers);
      return socket_ops::sync_send(impl.socket_, impl.state_,
          bufs.buffers(), bufs.count(), flags, bufs.all_empty(), ec);
    }
  }

  // Wait until data can be sent without blocking.
  size_t send(base_implementation_type& impl, const null_buffers&,
      socket_base::message_flags, asio::error_code& ec)
  {
    // Wait for socket to become ready.
    socket_ops::poll_write(impl.socket_, impl.state_, -1, ec);

    return 0;
  }

  // Start an asynchronous send. The data being sent must be valid for the
  // lifetime of the asynchronous operation.
  template <typename ConstBufferSequence, typename Handler, typename IoExecutor>
  void async_send(base_implementation_type& impl,
      const ConstBufferSequence& buffers, socket_base::message_flags flags,
      Handler& handler, const IoExecutor& io_ex);

  // Start an asynchronous wait until data can be sent without blocking.
  template <typename Handler, typename IoExecutor>
  void async_send(base_implementation_type& impl, const null_buffers&,
      socket_base::message_flags, Handler& handler, const IoExecutor& io_ex);

  // Receive some data from the peer. Returns the number of bytes received.
  template <typename MutableBufferSequence>
  size_t receive(base_implementation_type& impl,
      const MutableBufferSequence& buffers,
      socket_base::message_flags flags, asio::error_code& ec)
  {
    typedef buffer_sequence_adapter<asio::mutable_buffer,
        MutableBufferSequence> bufs_type;

    if (bufs_type::is_single_buffer)
    {
      return socket_ops::sync_recv1(impl.socket_,
          impl.state_, bufs_type::first(buffers).data(),
          bufs_type::first(buffers).size(), flags, ec);
    }
    else
    {
      bufs_type bufs(buffers);
      return socket_ops::sync_recv(impl.socket_, impl.state_,
          bufs.buffers(), bufs.count(), flags, bufs.all_empty(), ec);
    }
  }

  // Wait until data can be received without blocking.
  size_t receive(base_implementation_type& impl, const null_buffers&,
      socket_base::message_flags, asio::error_code& ec)
  {
    // Wait for socket to become ready.
    socket_ops::poll_read(impl.socket_, impl.state_, -1, ec);

    return 0;
  }

  // Start an asynchronous receive. The buffer for the data being received
  // must be valid for the lifetime of the asynchronous operation.
  template <typename MutableBufferSequence,
      typename Handler, typename IoExecutor>
  void async_receive(base_implementation_type& impl,
      const MutableBufferSequence& buffers, socket_base::message_flags flags,
      Handler& handler, const IoExecutor& io_ex);

  // Wait until data can be received without blocking.
  template <typename Handler, typename IoExecutor>
  void async_receive(base_implementation_type& impl,
      const null_buffers&, socket_base::message_flags flags,
      Handler& handler, const IoExecutor& io_ex);

  // Receive some data with associated flags. Returns the number of bytes
  // received.
  template <typename MutableBufferSequence>
  size_t receive_with_flags(base_implementation_type& impl,
      const MutableBufferSequence& buffers,
      socket_base::message_flags in_flags,
      socket_base::message_flags& out_flags, asio::error_code& ec)
  {
    buffer_sequence_adapter<asio::mutable_buffer,
        MutableBufferSequence> bufs(buffers);

    return socket_ops::sync_recvmsg(impl.socket_, impl.state_,
        bufs.buffers(), bufs.count(), in_flags, out_flags, ec);
  }

  // Wait until data can be received without blocking.
  size_t receive_with_flags(base_implementation_type& impl,
      const null_buffers&, socket_base::message_flags,
      socket_base::message_flags& out_flags, asio::error_code& ec)
  {
    // Wait for socket to become ready.
    socket_ops::poll_read(impl.socket_, impl.state_, -1, ec);

    // Clear out_flags, since we cannot give it any other sensible value when
    // performing a null_buffers operation.
    out_flags = 0;

    return 0;
  }

  // Start an asynchronous receive. The buffer for the data being received
  // must be valid for the lifetime of the asynchronous operation.
  template <typename MutableBufferSequence,
      typename Handler, typename IoExecutor>
  void async_receive_with_flags(base_implementation_type& impl,
      const MutableBufferSequence& buffers, socket_base::message_flags in_flags,
      socket_base::message_flags& out_flags, Handler& handler,
      const IoExecutor& io_ex);

  // Wait until data can be received without blocking.
  template <typename Handler, typename IoExecutor>
  void async_receive_with_flags(base_implementation_type& impl,
      const null_buffers&, socket_base::message_flags in_flags,
      socket_base::message_flags& out_flags, Handler& handler,
      const IoExecutor& io_ex);

  // Send multiple datagrams to the peer. Returns the number of datagrams sent.
  template <typename BufferSequences>
  size_t send_multiple(base_implementation_type& impl,
      BufferSequences&& buffer_sequences,
      socket_base::message_flags flags, asio::error_code& ec)
  {
    empty_range endpoints;
    size_t n = detail::send_multiple(impl.socket_, impl.state_,
        buffer_sequences, endpoints, flags, ec);
    ASIO_ERROR_LOCATION(ec);
    return n;
  }

  // Receive multiple datagrams from the peer. Returns the number of datagrams
  // received.
  template <typename BufferSequences,
      typename SizeSequence, typename FlagsSequence>
  size_t receive_multiple(base_implementation_type& impl,
      BufferSequences&& buffer_sequences, SizeSequence&& sizes,
      FlagsSequence&& out_flags, socket_base::message_flags flags,
      asio::error_code& ec)
  {
    empty_range endpoints;
    size_t n = detail::receive_multiple(impl.socket_, impl.state_,
        buffer_sequences, endpoints, sizes, out_flags, flags, ec);
    ASIO_ERROR_LOCATION(ec);
    return n;
  }

  // Start an asynchronous send of multiple datagrams.
  template <typename BufferSequences, typename Handler, typename IoExecutor>
  void async_send_multiple(base_implementation_type& impl,
      BufferSequences&& buffer_sequences, socket_base::message_flags flags,
      Handler& handler, const IoExecutor& io_ex)
  {
    start_send_multiple_op<void>(impl,
        static_cast<BufferSequences&&>(buffer_sequences), empty_range(),
        flags, handler, io_ex, "async_send_multiple");
  }

  // Start an asynchronous receive of multiple datagrams.
  template <typename BufferSequences, typename SizeSequence,
      typename FlagsSequence, typename Handler, typename IoExecutor>
  void async_receive_multiple(base_implementation_type& impl,
      BufferSequences&& buffer_sequences, SizeSequence&& sizes,
      FlagsSequence&& out_flags, socket_base::message_flags flags,
      Handler& handler, const IoExecutor& io_ex)
  {
    start_receive_multiple_op(impl,
        static_cast<BufferSequences&&>(buffer_sequences), empty_range(),
        static_cast<SizeSequence&&>(sizes),
        static_cast<FlagsSequence&&>(out_flags), flags, handler, io_ex,
        "async_receive_multiple");
  }

protected:
  // Start the asynchronous send of multiple datagrams.
  template <typename Endpoint, typename BufferSequences,
      typename EndpointSequence, typename Handler, typename IoExecutor>
  void start_send_multiple_op(base_implementation_type& impl,
      BufferSequences&& buffer_sequences, EndpointSequence&& endpoints,
      socket_base::message_flags flags, Handler& handler,
      const IoExecutor& io_ex, const char* op_name)
  {
    bool is_continuation =
      ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

    associated_cancellation_slot_t<Handler> slot
      = asio::get_associated_cancellation_slot(handler);

    typedef reactive_socket_sendmmsg_op<decay_t<BufferSequences>,
        decay_t<EndpointSequence>, Endpoint, Handler, IoExecutor> op;
    typename op::ptr p = { asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(success_ec_, impl.socket_,
        static_cast<BufferSequences&&>(buffer_sequences),
        static_cast<EndpointSequence&&>(endpoints), flags, handler, io_ex);

    if (slot.is_connected())
    {
      p.p->cancellation_key_ =
        &slot.template emplace<reactor_op_cancellation>(
            &reactor_, &impl.reactor_data_, impl.socket_, reactor::write_op);
    }

    ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "socket",
          &impl, impl.socket_, op_name));
    (void)op_name;

    start_op<&op::do_perform, reactor::write_op>(impl, p.p, is_continuation,
        false, true, &io_ex, 0);
    p.v = p.p = 0;
  }

  // Start the asynchronous receive of multiple datagrams.
  template <typename BufferSequences, typename EndpointSequence,
      typename SizeSequence, typename FlagsSequence,
      typename Handler, typename IoExecutor>
  void start_receive_multiple_op(base_implementation_type& impl,
      BufferSequences&& buffer_sequences, EndpointSequence&& endpoints,
      SizeSequence&& sizes, FlagsSequence&& out_flags,
      socket_base::message_flags flags, Handler& handler,
      const IoExecutor& io_ex, const char* op_name)
  {
    bool is_continuation =
      ASIO_VERSIONED_NAME(handler_cont_helpers)::is_continuation(handler);

    associated_cancellation_slot_t<Handler> slot
      = asio::get_associated_cancellation_slot(handler);

    typedef reactive_socket_recvmmsg_op<decay_t<BufferSequences>,
        decay_t<EndpointSequence>, decay_t<SizeSequence>,
        decay_t<FlagsSequence>, Handler, IoExecutor> op;
    typename op::ptr p = { asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(success_ec_, impl.socket_,
        static_cast<BufferSequences&&>(buffer_sequences),
        static_cast<EndpointSequence&&>(endpoints),
        static_cast<SizeSequence&&>(sizes),
        static_cast<FlagsSequence&&>(out_flags), flags, handler, io_ex);

    if (slot.is_connected())
    {
      p.p->cancellation_key_ =
        &slot.template emplace<reactor_op_cancellation>(
            &reactor_, &impl.reactor_data_, impl.socket_, reactor::read_op);
    }

    ASIO_HANDLER_CREATION((reactor_.context(), *p.p, "socket",
          &impl, impl.socket_, op_name));
    (void)op_name;

    start_op<&op::do_perform, reactor::read_op>(impl, p.p, is_continuation,
        false, true, &io_ex, 0);
    p.v = p.p = 0;
  }

  // Open a new socket implementation.
  ASIO_DECL asio::error_code do_open(
      base_implementation_type& impl, int af,
      int type, int protocol, asio::error_code& ec);

  // Assign a native socket to a socket implementation.
  ASIO_DECL asio::error_code do_assign(
      base_implementation_type& impl, int type,
      const native_handle_type& native_socket, asio::error_code& ec);

  // Start the asynchronous read or write operation. Returns true if the
  // operation is ready for its immediate completion.
  template <reactor_op::perform_func_type Perform, int OpType>
  inline bool do_start_op(base_implementation_type& impl,
      reactor_op* op, bool noop, bool needs_non_blocking)
  {
    if (!noop)
    {
      if ((impl.state_ & socket_ops::non_blocking)
          || !needs_non_blocking
          || socket_ops::set_internal_non_blocking(
            impl.socket_, impl.state_, true, op->ec_))
      {
        ASIO_INVOKE_INLINE return reactor_.start_op<OpType, Perform>(
            impl.socket_, impl.reactor_data_, op);
      }
    }

    return true;
  }

  // Start the asynchronous operation for handlers that are specialised for
  // immediate completion.
  template <reactor_op::perform_func_type Perform, int OpType, typename Op>
  void start_op(base_implementation_type& impl, Op* op, bool is_continuation,
      bool noop, bool needs_non_blocking, const void* io_ex, ...)
  {
    ASIO_INVOKE_INLINE if (do_start_op<Perform, OpType>(impl, op,
          noop, needs_non_blocking))
      Op::do_immediate(op, is_continuation, io_ex);
  }

  // Start the asynchronous operation for handlers that are not specialised for
  // immediate completion.
  template <reactor_op::perform_func_type Perform, int OpType, typename Op>
  void start_op(base_implementation_type& impl, Op* op, bool is_continuation,
      bool noop, bool needs_non_blocking, const void* io_ex,
      enable_if_t<
        !has_associated_immediate_executor<
          typename Op::handler_type,
          typename Op::io_executor_type
        >::value
      >*)
  {
    ASIO_INVOKE_INLINE return start_op<Perform, OpType>(impl, op,
        is_continuation, noop, needs_non_blocking, io_ex,
        native_execute_kind_t<typename Op::io_executor_type>());
  }

  template <reactor_op::perform_func_type Perform, int OpType, typename Op>
  void start_op(base_implementation_type& impl, Op* op,
      bool is_continuation, bool noop, bool needs_non_blocking,
      const void*, native_execute_kinds::native)
  {
    ASIO_INVOKE_INLINE if (do_start_op<Perform, OpType>(impl, op,
          noop, needs_non_blocking))
      reactor::call_post_immediate_completion(op, is_continuation, &reactor_);
  }

  template <reactor_op::perform_func_type Perform, int OpType, typename Op>
  void start_op(base_implementation_type& impl, Op* op,
      bool is_continuation, bool noop, bool needs_non_blocking,
      const void* io_ex, native_execute_kinds::polymorphic)
  {
    ASIO_INVOKE_INLINE if (do_start_op<Perform, OpType>(
          impl, op, noop, needs_non_blocking))
    {
      if ((has_native_execute)(
            *static_cast<const typename Op::io_executor_type*>(io_ex)))
      {
        reactor::call_post_immediate_completion(op, is_continuation,
            &reactor_);
      }
      else
        Op::do_immediate(op, is_continuation, io_ex);
    }
  }

  template <reactor_op::perform_func_type Perform, int OpType, typename Op>
  void start_op(base_implementation_type& impl, Op* op,
      bool is_continuation, bool noop, bool needs_non_blocking,
      const void* io_ex, native_execute_kinds::none)
  {
    ASIO_INVOKE_INLINE if (do_start_op<Perform, OpType>(impl, op,
          noop, needs_non_blocking))
      Op::do_immediate(op, is_continuation, io_ex);
  }

  // Set the result of an accept operation that was started on an open peer.
  ASIO_DECL ASIO_NEVER_INLINE static bool start_already_open_op(
      reactor_op* op);

  template <reactor_op::perform_func_type Perform>
  bool do_start_accept_op(base_implementation_type& impl,
      reactor_op* op, bool peer_is_open)
  {
    if (!peer_is_open)
      return do_start_op<Perform, reactor::read_op>(impl, op, false, true);

    return start_already_open_op(op);
  }

  // Start the asynchronous accept operation for handlers that are specialised
  // for immediate completion.
  template <reactor_op::perform_func_type Perform, typename Op>
  void start_accept_op(base_implementation_type& impl, Op* op,
      bool is_continuation, bool peer_is_open, const void* io_ex, ...)
  {
    if (do_start_accept_op<Perform>(impl, op, peer_is_open))
      Op::do_immediate(op, is_continuation, io_ex);
  }

  // Start the asynchronous operation for handlers that are not specialised for
  // immediate completion.
  template <reactor_op::perform_func_type Perform, typename Op>
  void start_accept_op(base_implementation_type& impl, Op* op,
      bool is_continuation, bool peer_is_open, const void* io_ex,
      enable_if_t<
        !has_associated_immediate_executor<
          typename Op::handler_type,
          typename Op::io_executor_type
        >::value
      >*)
  {
    return start_accept_op<Perform>(impl, op, is_continuation, peer_is_open,
        io_ex, native_execute_kind_t<typename Op::io_executor_type>());
  }

  template <reactor_op::perform_func_type Perform, typename Op>
  void start_accept_op(base_implementation_type& impl, Op* op,
      bool is_continuation, bool peer_is_open, const void*,
      native_execute_kinds::native)
  {
    if (do_start_accept_op<Perform>(impl, op, peer_is_open))
      reactor::call_post_immediate_completion(op, is_continuation, &reactor_);
  }

  template <reactor_op::perform_func_type Perform, typename Op>
  void start_accept_op(base_implementation_type& impl, Op* op,
      bool is_continuation, bool peer_is_open, const void* io_ex,
      native_execute_kinds::polymorphic)
  {
    if (do_start_accept_op<Perform>(impl, op, peer_is_open))
    {
      if ((has_native_execute)(
            *static_cast<const typename Op::io_executor_type*>(io_ex)))
      {
        reactor::call_post_immediate_completion(op, is_continuation,
            &reactor_);
      }
      else
        Op::do_immediate(op, is_continuation, io_ex);
    }
  }

  template <reactor_op::perform_func_type Perform, typename Op>
  void start_accept_op(base_implementation_type& impl, Op* op,
      bool is_continuation, bool peer_is_open, const void* io_ex,
      native_execute_kinds::none)
  {
    if (do_start_accept_op<Perform>(impl, op, peer_is_open))
      Op::do_immediate(op, is_continuation, io_ex);
  }

  // Attempt the non-blocking connect. Returns true if the operation is ready
  // for immediate completion.
  ASIO_DECL bool do_connect(base_implementation_type& impl,
      reactor_op* op, const void* addr, size_t addrlen);

  // Start the asynchronous connect operation. A connect is never attempted
  // speculatively. Returns true if the operation is ready for its immediate
  // completion, which the caller makes.
  bool do_start_connect_op(base_implementation_type& impl,
      reactor_op* op, const void* addr, size_t addrlen)
  {
    if (do_connect(impl, op, addr, addrlen))
      return true;

    return reactor_.start_op<reactor::connect_op, nullptr>(impl.socket_,
        impl.reactor_data_, op);
  }

  // Start the asynchronous operation for handlers that are specialised for
  // immediate completion.
  template <typename Op>
  void start_connect_op(base_implementation_type& impl,
      Op* op, bool is_continuation, const void* addr,
      size_t addrlen, const void* io_ex, ...)
  {
    if (do_start_connect_op(impl, op, addr, addrlen))
      Op::do_immediate(op, is_continuation, io_ex);
  }

  // Start the asynchronous operation for handlers that are not specialised for
  // immediate completion.
  template <typename Op>
  void start_connect_op(base_implementation_type& impl, Op* op,
      bool is_continuation, const void* addr, size_t addrlen,
      const void* io_ex,
      enable_if_t<
        !has_associated_immediate_executor<
          typename Op::handler_type,
          typename Op::io_executor_type
        >::value
      >*)
  {
    return start_connect_op(impl, op, is_continuation, addr, addrlen,
        io_ex, native_execute_kind_t<typename Op::io_executor_type>());
  }

  template <typename Op>
  void start_connect_op(base_implementation_type& impl, Op* op,
      bool is_continuation, const void* addr, size_t addrlen, const void*,
      native_execute_kinds::native)
  {
    if (do_start_connect_op(impl, op, addr, addrlen))
      reactor::call_post_immediate_completion(op, is_continuation, &reactor_);
  }

  template <typename Op>
  void start_connect_op(base_implementation_type& impl, Op* op,
      bool is_continuation, const void* addr, size_t addrlen,
      const void* io_ex, native_execute_kinds::polymorphic)
  {
    if (do_start_connect_op(impl, op, addr, addrlen))
    {
      if ((has_native_execute)(
            *static_cast<const typename Op::io_executor_type*>(io_ex)))
      {
        reactor::call_post_immediate_completion(op, is_continuation,
            &reactor_);
      }
      else
        Op::do_immediate(op, is_continuation, io_ex);
    }
  }

  template <typename Op>
  void start_connect_op(base_implementation_type& impl, Op* op,
      bool is_continuation, const void* addr, size_t addrlen,
      const void* io_ex, native_execute_kinds::none)
  {
    if (do_start_connect_op(impl, op, addr, addrlen))
      Op::do_immediate(op, is_continuation, io_ex);
  }

  // Helper class used to implement per-operation cancellation
  class reactor_op_cancellation
  {
  public:
    reactor_op_cancellation(reactor* r,
        reactor::per_descriptor_data* p, socket_type d, int o)
      : reactor_(r),
        reactor_data_(p),
        descriptor_(d),
        op_type_(o)
    {
    }

    void operator()(cancellation_type_t type)
    {
      if (!!(type &
            (cancellation_type::terminal
              | cancellation_type::partial
              | cancellation_type::total)))
      {
        reactor_->cancel_ops_by_key(descriptor_,
            *reactor_data_, op_type_, this);
      }
    }

  private:
    reactor* reactor_;
    reactor::per_descriptor_data* reactor_data_;
    socket_type descriptor_;
    int op_type_;
  };

  // The selector that performs event demultiplexing for the service.
  reactor& reactor_;

  // Cached success value to avoid accessing category singleton.
  const asio::error_code success_ec_;

  // Extra state flags to be applied to newly opened sockets.
  socket_ops::state_type extra_state_;
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#include "asio/detail/impl/reactive_socket_service_base.hpp"

#if defined(ASIO_HEADER_ONLY)
# include "asio/detail/impl/reactive_socket_service_base.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // !defined(ASIO_HAS_IOCP)
       //   && !defined(ASIO_WINDOWS_RUNTIME)
       //   && !defined(ASIO_HAS_IO_URING_AS_DEFAULT)

#endif // ASIO_DETAIL_REACTIVE_SOCKET_SERVICE_BASE_HPP
