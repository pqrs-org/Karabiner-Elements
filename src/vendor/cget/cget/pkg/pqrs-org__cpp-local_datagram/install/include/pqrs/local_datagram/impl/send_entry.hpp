#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::local_datagram::impl::send_entry` can be used safely in a multi-threaded environment.

#include "asio_helper.hpp"
#include <optional>
#include <vector>

namespace pqrs {
namespace local_datagram {
namespace impl {
class send_entry final {
public:
  // Sending empty data causes `No buffer space available` error after wake up on macOS.
  // We append `type` into the beginning of data in order to avoid this issue.

  enum class type : uint8_t {
    server_check,
    user_data,
    response,
  };

  send_entry(type t,
             std::shared_ptr<asio::local::datagram_protocol::endpoint> destination_endpoint,
             const std::function<void(void)>& processed = nullptr) : destination_endpoint_(destination_endpoint),
                                                                     processed_(processed),
                                                                     bytes_transferred_(0),
                                                                     no_buffer_space_error_count_(0) {
    buffer_.push_back(static_cast<uint8_t>(t));
  }

  send_entry(type t,
             const std::vector<uint8_t>& v,
             std::shared_ptr<asio::local::datagram_protocol::endpoint> destination_endpoint,
             const std::function<void(void)>& processed = nullptr) : destination_endpoint_(destination_endpoint),
                                                                     processed_(processed),
                                                                     bytes_transferred_(0),
                                                                     no_buffer_space_error_count_(0) {
    buffer_.push_back(static_cast<uint8_t>(t));

    std::copy(std::begin(v),
              std::end(v),
              std::back_inserter(buffer_));
  }

  send_entry(type t,
             const uint8_t* p,
             size_t length,
             std::shared_ptr<asio::local::datagram_protocol::endpoint> destination_endpoint,
             const std::function<void(void)>& processed = nullptr) : destination_endpoint_(destination_endpoint),
                                                                     processed_(processed),
                                                                     bytes_transferred_(0),
                                                                     no_buffer_space_error_count_(0) {
    buffer_.push_back(static_cast<uint8_t>(t));

    if (p && length > 0) {
      std::copy(p,
                p + length,
                std::back_inserter(buffer_));
    }
  }

  std::shared_ptr<asio::local::datagram_protocol::endpoint> get_destination_endpoint(void) const {
    return destination_endpoint_;
  }

  const std::function<void(void)>& get_processed(void) const {
    return processed_;
  }

  size_t get_bytes_transferred(void) const {
    return bytes_transferred_;
  }

  size_t get_no_buffer_space_error_count(void) const {
    return no_buffer_space_error_count_;
  }

  void set_no_buffer_space_error_count(size_t value) {
    no_buffer_space_error_count_ = value;
  }

  const asio::const_buffer make_buffer(void) const {
    if (bytes_transferred_ >= buffer_.size()) {
      return asio::const_buffer();
    }

    return asio::const_buffer(
        &(buffer_[0]) + bytes_transferred_,
        buffer_.size() - bytes_transferred_);
  }

  void add_bytes_transferred(size_t value) {
    bytes_transferred_ += value;
  }

  size_t rest_bytes(void) {
    if (bytes_transferred_ >= buffer_.size()) {
      return 0;
    }

    return buffer_.size() - bytes_transferred_;
  }

  bool transfer_complete(void) {
    return bytes_transferred_ >= buffer_.size();
  }

private:
  std::vector<uint8_t> buffer_;
  std::shared_ptr<asio::local::datagram_protocol::endpoint> destination_endpoint_;
  std::function<void(void)> processed_;
  size_t bytes_transferred_;
  size_t no_buffer_space_error_count_;
};
} // namespace impl
} // namespace local_datagram
} // namespace pqrs
