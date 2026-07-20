//
// ssl/detail/stream_core.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_SSL_DETAIL_STREAM_CORE_HPP
#define ASIO_SSL_DETAIL_STREAM_CORE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#include "asio/ssl/detail/engine.hpp"
#include "asio/buffer.hpp"
#include "asio/detail/memory.hpp"
#include "asio/steady_timer.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace ssl {
namespace detail {

struct stream_core
{
  // According to the OpenSSL documentation, this is the buffer size that is
  // sufficient to hold the largest possible TLS record.
  enum { max_tls_record_size = 17 * 1024 };

  template <typename Executor>
  stream_core(SSL_CTX* context, const Executor& ex,
      std::size_t output_buffer_size, std::size_t input_buffer_size)
    : engine_(context, output_buffer_size, input_buffer_size),
      pending_read_(ex),
      pending_write_(ex),
      output_buffer_size_(clamp_buffer_size(output_buffer_size)),
      input_buffer_size_(clamp_buffer_size(input_buffer_size)),
      output_buffer_space_(new unsigned char[output_buffer_size_]()),
      input_buffer_space_(new unsigned char[input_buffer_size_]())
  {
    pending_read_.expires_at(neg_infin());
    pending_write_.expires_at(neg_infin());
  }

  template <typename Executor>
  stream_core(SSL* ssl_impl, const Executor& ex,
      std::size_t output_buffer_size, std::size_t input_buffer_size)
    : engine_(ssl_impl, output_buffer_size, input_buffer_size),
      pending_read_(ex),
      pending_write_(ex),
      output_buffer_size_(clamp_buffer_size(output_buffer_size)),
      input_buffer_size_(clamp_buffer_size(input_buffer_size)),
      output_buffer_space_(new unsigned char[output_buffer_size_]()),
      input_buffer_space_(new unsigned char[input_buffer_size_]())
  {
    pending_read_.expires_at(neg_infin());
    pending_write_.expires_at(neg_infin());
  }

  stream_core(stream_core&& other)
    : engine_(static_cast<engine&&>(other.engine_)),
      pending_read_(
         static_cast<asio::steady_timer&&>(
           other.pending_read_)),
      pending_write_(
         static_cast<asio::steady_timer&&>(
           other.pending_write_)),
      output_buffer_size_(other.output_buffer_size_),
      input_buffer_size_(other.input_buffer_size_),
      output_buffer_space_(
          static_cast<std::unique_ptr<unsigned char[]>&&>(
            other.output_buffer_space_)),
      input_buffer_space_(
          static_cast<std::unique_ptr<unsigned char[]>&&>(
            other.input_buffer_space_)),
      input_(other.input_)
  {
    other.input_ = asio::const_buffer(0, 0);
  }

  ~stream_core()
  {
  }

  stream_core& operator=(stream_core&& other)
  {
    if (this != &other)
    {
      engine_ = static_cast<engine&&>(other.engine_);
      pending_read_ =
        static_cast<asio::steady_timer&&>(
          other.pending_read_);
      pending_write_ =
        static_cast<asio::steady_timer&&>(
          other.pending_write_);
      output_buffer_size_ = other.output_buffer_size_;
      input_buffer_size_ = other.input_buffer_size_;
      output_buffer_space_ =
        static_cast<std::unique_ptr<unsigned char[]>&&>(
          other.output_buffer_space_);
      input_buffer_space_ =
        static_cast<std::unique_ptr<unsigned char[]>&&>(
          other.input_buffer_space_);
      input_ = other.input_;
      other.input_ = asio::const_buffer(0, 0);
    }
    return *this;
  }

  // The SSL engine.
  engine engine_;

  // Timer used for storing queued read operations.
  asio::steady_timer pending_read_;

  // Timer used for storing queued write operations.
  asio::steady_timer pending_write_;

  // Helper function for obtaining a time value that always fires.
  static asio::steady_timer::time_point neg_infin()
  {
    return (asio::steady_timer::time_point::min)();
  }

  // Helper function for obtaining a time value that never fires.
  static asio::steady_timer::time_point pos_infin()
  {
    return (asio::steady_timer::time_point::max)();
  }

  // Helper function to get a timer's expiry time.
  static asio::steady_timer::time_point expiry(
      const asio::steady_timer& timer)
  {
    return timer.expiry();
  }

  // A buffer that may be used to prepare output intended for the transport.
  asio::mutable_buffer output_buffer()
  {
    return asio::buffer(output_buffer_space_.get(), output_buffer_size_);
  }

  // A buffer that may be used to read input intended for the engine.
  asio::mutable_buffer input_buffer()
  {
    return asio::buffer(input_buffer_space_.get(), input_buffer_size_);
  }

  // Ensure a requested buffer size is at least large enough to hold the
  // largest possible TLS record. A size of zero requests the default.
  static std::size_t clamp_buffer_size(std::size_t size)
  {
    return size < std::size_t(max_tls_record_size)
      ? std::size_t(max_tls_record_size) : size;
  }

  // The size of the buffer used to prepare output intended for the transport.
  std::size_t output_buffer_size_;

  // The size of the buffer used to read input intended for the engine.
  std::size_t input_buffer_size_;

  // Buffer space used to prepare output intended for the transport.
  std::unique_ptr<unsigned char[]> output_buffer_space_;

  // Buffer space used to read input intended for the engine.
  std::unique_ptr<unsigned char[]> input_buffer_space_;

  // The buffer pointing to the engine's unconsumed input.
  asio::const_buffer input_;
};

} // namespace detail
} // namespace ssl
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_SSL_DETAIL_STREAM_CORE_HPP
