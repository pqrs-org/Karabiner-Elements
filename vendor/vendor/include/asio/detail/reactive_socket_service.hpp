//
// detail/reactive_socket_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_REACTIVE_SOCKET_SERVICE_HPP
#define ASIO_DETAIL_REACTIVE_SOCKET_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if !defined(ASIO_HAS_IOCP) \
  && !defined(ASIO_HAS_IO_URING_AS_DEFAULT)

#include "asio/buffer.hpp"
#include "asio/error.hpp"
#include "asio/execution_context.hpp"
#include "asio/socket_base.hpp"
#include "asio/detail/buffer_sequence_adapter.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/reactive_null_buffers_op.hpp"
#include "asio/detail/reactive_socket_accept_op.hpp"
#include "asio/detail/reactive_socket_connect_op.hpp"
#include "asio/detail/reactive_socket_recvfrom_op.hpp"
#include "asio/detail/reactive_socket_recvmmsg_op.hpp"
#include "asio/detail/reactive_socket_sendmmsg_op.hpp"
#include "asio/detail/reactive_socket_sendto_op.hpp"
#include "asio/detail/reactive_socket_service_base.hpp"
#include "asio/detail/reactor.hpp"
#include "asio/detail/reactor_op.hpp"
#include "asio/detail/socket_holder.hpp"
#include "asio/detail/socket_ops.hpp"
#include "asio/detail/socket_types.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

template <typename Protocol>
class reactive_socket_service :
  public execution_context_service_base<reactive_socket_service<Protocol>>,
  public reactive_socket_service_base
{
public:
  // The protocol type.
  typedef Protocol protocol_type;

  // The endpoint type.
  typedef typename Protocol::endpoint endpoint_type;

  // The native type of a socket.
  typedef socket_type native_handle_type;

  // The implementation type of the socket.
  struct implementation_type :
    reactive_socket_service_base::base_implementation_type
  {
    // Default constructor.
    implementation_type()
      : protocol_(endpoint_type().protocol())
    {
    }

    // The protocol associated with the socket.
    protocol_type protocol_;
  };

  // Constructor.
  reactive_socket_service(execution_context& context)
    : execution_context_service_base<
        reactive_socket_service<Protocol>>(context),
      reactive_socket_service_base(context)
  {
  }

  // Destroy all user-defined handler objects owned by the service.
  void shutdown()
  {
    this->base_shutdown();
  }

  // Move-construct a new socket implementation.
  void move_construct(implementation_type& impl,
      implementation_type& other_impl) noexcept
  {
    this->base_move_construct(impl, other_impl);

    impl.protocol_ = other_impl.protocol_;
    other_impl.protocol_ = endpoint_type().protocol();
  }

  // Move-assign from another socket implementation.
  void move_assign(implementation_type& impl,
      reactive_socket_service_base& other_service,
      implementation_type& other_impl)
  {
    this->base_move_assign(impl, other_service, other_impl);

    impl.protocol_ = other_impl.protocol_;
    other_impl.protocol_ = endpoint_type().protocol();
  }

  // Move-construct a new socket implementation from another protocol type.
  template <typename Protocol1>
  void converting_move_construct(implementation_type& impl,
      reactive_socket_service<Protocol1>&,
      typename reactive_socket_service<
        Protocol1>::implementation_type& other_impl)
  {
    this->base_move_construct(impl, other_impl);

    impl.protocol_ = protocol_type(other_impl.protocol_);
    other_impl.protocol_ = typename Protocol1::endpoint().protocol();
  }

  // Open a new socket implementation.
  asio::error_code open(implementation_type& impl,
      const protocol_type& protocol, asio::error_code& ec)
  {
    if (!do_open(impl, protocol.family(),
          protocol.type(), protocol.protocol(), ec))
      impl.protocol_ = protocol;

    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Assign a native socket to a socket implementation.
  asio::error_code assign(implementation_type& impl,
      const protocol_type& protocol, const native_handle_type& native_socket,
      asio::error_code& ec)
  {
    if (!do_assign(impl, protocol.type(), native_socket, ec))
      impl.protocol_ = protocol;

    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Get the native socket representation.
  native_handle_type native_handle(implementation_type& impl)
  {
    return impl.socket_;
  }

  // Bind the socket to the specified local endpoint.
  asio::error_code bind(implementation_type& impl,
      const endpoint_type& endpoint, asio::error_code& ec)
  {
    socket_ops::bind(impl.socket_, endpoint.data(), endpoint.size(), ec);

    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Set a socket option.
  template <typename Option>
  asio::error_code set_option(implementation_type& impl,
      const Option& option, asio::error_code& ec)
  {
    socket_ops::setsockopt(impl.socket_, impl.state_,
        option.level(impl.protocol_), option.name(impl.protocol_),
        option.data(impl.protocol_), option.size(impl.protocol_), ec);

    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Set a socket option.
  template <typename Option>
  asio::error_code get_option(const implementation_type& impl,
      Option& option, asio::error_code& ec) const
  {
    std::size_t size = option.size(impl.protocol_);
    socket_ops::getsockopt(impl.socket_, impl.state_,
        option.level(impl.protocol_), option.name(impl.protocol_),
        option.data(impl.protocol_), &size, ec);
    if (!ec)
      option.resize(impl.protocol_, size);

    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Get the local endpoint.
  endpoint_type local_endpoint(const implementation_type& impl,
      asio::error_code& ec) const
  {
    endpoint_type endpoint;
    std::size_t addr_len = endpoint.capacity();
    if (socket_ops::getsockname(impl.socket_, endpoint.data(), &addr_len, ec))
    {
      ASIO_ERROR_LOCATION(ec);
      return endpoint_type();
    }
    endpoint.resize(addr_len);
    return endpoint;
  }

  // Get the remote endpoint.
  endpoint_type remote_endpoint(const implementation_type& impl,
      asio::error_code& ec) const
  {
    endpoint_type endpoint;
    std::size_t addr_len = endpoint.capacity();
    if (socket_ops::getpeername(impl.socket_,
          endpoint.data(), &addr_len, false, ec))
    {
      ASIO_ERROR_LOCATION(ec);
      return endpoint_type();
    }
    endpoint.resize(addr_len);
    return endpoint;
  }

  // Disable sends or receives on the socket.
  asio::error_code shutdown(base_implementation_type& impl,
      socket_base::shutdown_type what, asio::error_code& ec)
  {
    socket_ops::shutdown(impl.socket_, what, ec);

    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Send a datagram to the specified endpoint. Returns the number of bytes
  // sent.
  template <typename ConstBufferSequence>
  size_t send_to(implementation_type& impl, const ConstBufferSequence& buffers,
      const endpoint_type& destination, socket_base::message_flags flags,
      asio::error_code& ec)
  {
    typedef buffer_sequence_adapter<asio::const_buffer,
        ConstBufferSequence> bufs_type;

    size_t n;
    if (bufs_type::is_single_buffer)
    {
      n = socket_ops::sync_sendto1(impl.socket_, impl.state_,
          bufs_type::first(buffers).data(),
          bufs_type::first(buffers).size(), flags,
          destination.data(), destination.size(), ec);
    }
    else
    {
      bufs_type bufs(buffers);
      n = socket_ops::sync_sendto(impl.socket_, impl.state_,
          bufs.buffers(), bufs.count(), flags,
          destination.data(), destination.size(), ec);
    }

    ASIO_ERROR_LOCATION(ec);
    return n;
  }

  // Wait until data can be sent without blocking.
  size_t send_to(implementation_type& impl, const null_buffers&,
      const endpoint_type&, socket_base::message_flags,
      asio::error_code& ec)
  {
    // Wait for socket to become ready.
    socket_ops::poll_write(impl.socket_, impl.state_, -1, ec);

    ASIO_ERROR_LOCATION(ec);
    return 0;
  }

  // Start an asynchronous send. The data being sent must be valid for the
  // lifetime of the asynchronous operation.
  template <typename ConstBufferSequence, typename Handler, typename IoExecutor>
  void async_send_to(implementation_type& impl,
      const ConstBufferSequence& buffers,
      const endpoint_type& destination, socket_base::message_flags flags,
      Handler& handler, const IoExecutor& io_ex);

  // Start an asynchronous wait until data can be sent without blocking.
  template <typename Handler, typename IoExecutor>
  void async_send_to(implementation_type& impl, const null_buffers&,
      const endpoint_type&, socket_base::message_flags,
      Handler& handler, const IoExecutor& io_ex);

  // Receive a datagram with the endpoint of the sender. Returns the number of
  // bytes received.
  template <typename MutableBufferSequence>
  size_t receive_from(implementation_type& impl,
      const MutableBufferSequence& buffers,
      endpoint_type& sender_endpoint, socket_base::message_flags flags,
      asio::error_code& ec)
  {
    typedef buffer_sequence_adapter<asio::mutable_buffer,
        MutableBufferSequence> bufs_type;

    std::size_t addr_len = sender_endpoint.capacity();
    std::size_t n;
    if (bufs_type::is_single_buffer)
    {
      n = socket_ops::sync_recvfrom1(impl.socket_, impl.state_,
          bufs_type::first(buffers).data(), bufs_type::first(buffers).size(),
          flags, sender_endpoint.data(), &addr_len, ec);
    }
    else
    {
      bufs_type bufs(buffers);
      n = socket_ops::sync_recvfrom(impl.socket_, impl.state_, bufs.buffers(),
          bufs.count(), flags, sender_endpoint.data(), &addr_len, ec);
    }

    if (!ec)
      sender_endpoint.resize(addr_len);

    ASIO_ERROR_LOCATION(ec);
    return n;
  }

  // Wait until data can be received without blocking.
  size_t receive_from(implementation_type& impl, const null_buffers&,
      endpoint_type& sender_endpoint, socket_base::message_flags,
      asio::error_code& ec)
  {
    // Wait for socket to become ready.
    socket_ops::poll_read(impl.socket_, impl.state_, -1, ec);

    // Reset endpoint since it can be given no sensible value at this time.
    sender_endpoint = endpoint_type();

    ASIO_ERROR_LOCATION(ec);
    return 0;
  }

  // Start an asynchronous receive. The buffer for the data being received and
  // the sender_endpoint object must both be valid for the lifetime of the
  // asynchronous operation.
  template <typename MutableBufferSequence,
      typename Handler, typename IoExecutor>
  void async_receive_from(implementation_type& impl,
      const MutableBufferSequence& buffers, endpoint_type& sender_endpoint,
      socket_base::message_flags flags, Handler& handler,
      const IoExecutor& io_ex);

  // Wait until data can be received without blocking.
  template <typename Handler, typename IoExecutor>
  void async_receive_from(implementation_type& impl, const null_buffers&,
      endpoint_type& sender_endpoint, socket_base::message_flags flags,
      Handler& handler, const IoExecutor& io_ex);

  // Send multiple datagrams to the specified endpoints. Returns the number of
  // datagrams sent.
  template <typename BufferSequences, typename EndpointSequence>
  size_t send_to_multiple(implementation_type& impl,
      BufferSequences&& buffer_sequences, EndpointSequence&& endpoints,
      socket_base::message_flags flags, asio::error_code& ec)
  {
    size_t n = detail::send_multiple<endpoint_type>(impl.socket_,
        impl.state_, buffer_sequences, endpoints, flags, ec);
    ASIO_ERROR_LOCATION(ec);
    return n;
  }

  // Receive multiple datagrams with the endpoints of the senders. Returns the
  // number of datagrams received.
  template <typename BufferSequences, typename EndpointSequence,
      typename SizeSequence, typename FlagsSequence>
  size_t receive_from_multiple(implementation_type& impl,
      BufferSequences&& buffer_sequences, EndpointSequence&& endpoints,
      SizeSequence&& sizes, FlagsSequence&& out_flags,
      socket_base::message_flags flags, asio::error_code& ec)
  {
    size_t n = detail::receive_multiple(impl.socket_, impl.state_,
        buffer_sequences, endpoints, sizes, out_flags, flags, ec);
    ASIO_ERROR_LOCATION(ec);
    return n;
  }

  // Start an asynchronous send of multiple datagrams to the specified
  // endpoints.
  template <typename BufferSequences, typename EndpointSequence,
      typename Handler, typename IoExecutor>
  void async_send_to_multiple(implementation_type& impl,
      BufferSequences&& buffer_sequences, EndpointSequence&& endpoints,
      socket_base::message_flags flags, Handler& handler,
      const IoExecutor& io_ex)
  {
    this->template start_send_multiple_op<endpoint_type>(impl,
        static_cast<BufferSequences&&>(buffer_sequences),
        static_cast<EndpointSequence&&>(endpoints), flags, handler, io_ex,
        "async_send_to_multiple");
  }

  // Start an asynchronous receive of multiple datagrams with the endpoints of
  // the senders.
  template <typename BufferSequences, typename EndpointSequence,
      typename SizeSequence, typename FlagsSequence,
      typename Handler, typename IoExecutor>
  void async_receive_from_multiple(implementation_type& impl,
      BufferSequences&& buffer_sequences, EndpointSequence&& endpoints,
      SizeSequence&& sizes, FlagsSequence&& out_flags,
      socket_base::message_flags flags, Handler& handler,
      const IoExecutor& io_ex)
  {
    this->start_receive_multiple_op(impl,
        static_cast<BufferSequences&&>(buffer_sequences),
        static_cast<EndpointSequence&&>(endpoints),
        static_cast<SizeSequence&&>(sizes),
        static_cast<FlagsSequence&&>(out_flags), flags, handler, io_ex,
        "async_receive_from_multiple");
  }

  // Accept a new connection.
  template <typename Socket>
  asio::error_code accept(implementation_type& impl,
      Socket& peer, endpoint_type* peer_endpoint, asio::error_code& ec)
  {
    // We cannot accept a socket that is already open.
    if (peer.is_open())
    {
      ec = asio::error::already_open;
      ASIO_ERROR_LOCATION(ec);
      return ec;
    }

    std::size_t addr_len = peer_endpoint ? peer_endpoint->capacity() : 0;
    socket_holder new_socket(socket_ops::sync_accept(impl.socket_,
          impl.state_, peer_endpoint ? peer_endpoint->data() : 0,
          peer_endpoint ? &addr_len : 0, ec));

    // On success, assign new connection to peer socket object.
    if (new_socket.get() != invalid_socket)
    {
      if (peer_endpoint)
        peer_endpoint->resize(addr_len);
      peer.assign(impl.protocol_, new_socket.get(), ec);
      if (!ec)
        new_socket.release();
    }

    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Start an asynchronous accept. The peer and peer_endpoint objects must be
  // valid until the accept's handler is invoked.
  template <typename Socket, typename Handler, typename IoExecutor>
  void async_accept(implementation_type& impl, Socket& peer,
      endpoint_type* peer_endpoint, Handler& handler, const IoExecutor& io_ex);

  // Start an asynchronous accept. The peer_endpoint object must be valid until
  // the accept's handler is invoked.
  template <typename PeerIoExecutor, typename Handler, typename IoExecutor>
  void async_move_accept(implementation_type& impl,
      const PeerIoExecutor& peer_io_ex, endpoint_type* peer_endpoint,
      Handler& handler, const IoExecutor& io_ex);

  // Connect the socket to the specified endpoint.
  asio::error_code connect(implementation_type& impl,
      const endpoint_type& peer_endpoint, asio::error_code& ec)
  {
    socket_ops::sync_connect(impl.socket_,
        peer_endpoint.data(), peer_endpoint.size(), ec);
    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Start an asynchronous connect.
  template <typename Handler, typename IoExecutor>
  void async_connect(implementation_type& impl,
      const endpoint_type& peer_endpoint,
      Handler& handler, const IoExecutor& io_ex);
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#include "asio/detail/impl/reactive_socket_service.hpp"

#endif // !defined(ASIO_HAS_IOCP)
       //   && !defined(ASIO_HAS_IO_URING_AS_DEFAULT)

#endif // ASIO_DETAIL_REACTIVE_SOCKET_SERVICE_HPP
