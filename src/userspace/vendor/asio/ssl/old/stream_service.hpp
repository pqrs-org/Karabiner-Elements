//
// ssl/old/stream_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2005 Voipster / Indrek dot Juhani at voipster dot com
// Copyright (c) 2005-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_SSL_OLD_STREAM_SERVICE_HPP
#define ASIO_SSL_OLD_STREAM_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <cstddef>
#include <boost/noncopyable.hpp>
#include "asio/io_service.hpp"
#include "asio/ssl/basic_context.hpp"
#include "asio/ssl/old/detail/openssl_stream_service.hpp"
#include "asio/ssl/stream_base.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace ssl {
namespace old {

/// Default service implementation for an SSL stream.
class stream_service
#if defined(GENERATING_DOCUMENTATION)
  : public asio::io_service::service
#else
  : public asio::detail::service_base<stream_service>
#endif
{
private:
  // The type of the platform-specific implementation.
  typedef old::detail::openssl_stream_service service_impl_type;

public:
#if defined(GENERATING_DOCUMENTATION)
  /// The unique service identifier.
  static asio::io_service::id id;
#endif

  /// The type of a stream implementation.
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined impl_type;
#else
  typedef service_impl_type::impl_type impl_type;
#endif

  /// Construct a new stream service for the specified io_service.
  explicit stream_service(asio::io_service& io_service)
    : asio::detail::service_base<stream_service>(io_service),
      service_impl_(asio::use_service<service_impl_type>(io_service))
  {
  }

  /// Return a null stream implementation.
  impl_type null() const
  {
    return service_impl_.null();
  }

  /// Create a new stream implementation.
  template <typename Stream, typename Context_Service>
  void create(impl_type& impl, Stream& next_layer,
      basic_context<Context_Service>& context)
  {
    service_impl_.create(impl, next_layer, context);
  }

  /// Destroy a stream implementation.
  template <typename Stream>
  void destroy(impl_type& impl, Stream& next_layer)
  {
    service_impl_.destroy(impl, next_layer);
  }

  /// Perform SSL handshaking.
  template <typename Stream>
  asio::error_code handshake(impl_type& impl, Stream& next_layer,
      stream_base::handshake_type type, asio::error_code& ec)
  {
    return service_impl_.handshake(impl, next_layer, type, ec);
  }

  /// Start an asynchronous SSL handshake.
  template <typename Stream, typename HandshakeHandler>
  void async_handshake(impl_type& impl, Stream& next_layer,
      stream_base::handshake_type type, HandshakeHandler handler)
  {
    service_impl_.async_handshake(impl, next_layer, type, handler);
  }

  /// Shut down SSL on the stream.
  template <typename Stream>
  asio::error_code shutdown(impl_type& impl, Stream& next_layer,
      asio::error_code& ec)
  {
    return service_impl_.shutdown(impl, next_layer, ec);
  }

  /// Asynchronously shut down SSL on the stream.
  template <typename Stream, typename ShutdownHandler>
  void async_shutdown(impl_type& impl, Stream& next_layer,
      ShutdownHandler handler)
  {
    service_impl_.async_shutdown(impl, next_layer, handler);
  }

  /// Write some data to the stream.
  template <typename Stream, typename ConstBufferSequence>
  std::size_t write_some(impl_type& impl, Stream& next_layer,
      const ConstBufferSequence& buffers, asio::error_code& ec)
  {
    return service_impl_.write_some(impl, next_layer, buffers, ec);
  }

  /// Start an asynchronous write.
  template <typename Stream, typename ConstBufferSequence,
      typename WriteHandler>
  void async_write_some(impl_type& impl, Stream& next_layer,
      const ConstBufferSequence& buffers, WriteHandler handler)
  {
    service_impl_.async_write_some(impl, next_layer, buffers, handler);
  }

  /// Read some data from the stream.
  template <typename Stream, typename MutableBufferSequence>
  std::size_t read_some(impl_type& impl, Stream& next_layer,
      const MutableBufferSequence& buffers, asio::error_code& ec)
  {
    return service_impl_.read_some(impl, next_layer, buffers, ec);
  }

  /// Start an asynchronous read.
  template <typename Stream, typename MutableBufferSequence,
      typename ReadHandler>
  void async_read_some(impl_type& impl, Stream& next_layer,
      const MutableBufferSequence& buffers, ReadHandler handler)
  {
    service_impl_.async_read_some(impl, next_layer, buffers, handler);
  }

  /// Peek at the incoming data on the stream.
  template <typename Stream, typename MutableBufferSequence>
  std::size_t peek(impl_type& impl, Stream& next_layer,
      const MutableBufferSequence& buffers, asio::error_code& ec)
  {
    return service_impl_.peek(impl, next_layer, buffers, ec);
  }

  /// Determine the amount of data that may be read without blocking.
  template <typename Stream>
  std::size_t in_avail(impl_type& impl, Stream& next_layer,
      asio::error_code& ec)
  {
    return service_impl_.in_avail(impl, next_layer, ec);
  }

private:
  // Destroy all user-defined handler objects owned by the service.
  void shutdown_service()
  {
  }

  // The service that provides the platform-specific implementation.
  service_impl_type& service_impl_;
};

} // namespace old
} // namespace ssl
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_SSL_OLD_STREAM_SERVICE_HPP
