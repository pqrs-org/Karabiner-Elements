#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::local_datagram::impl::buffer` can be used safely in a multi-threaded environment.

#include "../request_id.hpp"
#include <optional>
#include <vector>

namespace pqrs {
namespace local_datagram {
namespace impl {
class buffer final {
public:
  // Sending empty data causes `No buffer space available` error after wake up on macOS.
  // We append `type` into the beginning of data in order to avoid this issue.

  enum class type : uint8_t {
    server_check,
    user_data,
  };

  buffer(type t,
         std::optional<request_id> request_id) : request_id_(request_id) {
    v_.push_back(static_cast<uint8_t>(t));
  }

  buffer(type t,
         std::optional<request_id> request_id,
         const std::vector<uint8_t>& v) : request_id_(request_id) {
    v_.push_back(static_cast<uint8_t>(t));

    std::copy(std::begin(v),
              std::end(v),
              std::back_inserter(v_));
  }

  buffer(type t,
         std::optional<request_id> request_id,
         const uint8_t* p,
         size_t length) : request_id_(request_id) {
    v_.push_back(static_cast<uint8_t>(t));

    if (p && length > 0) {
      std::copy(p,
                p + length,
                std::back_inserter(v_));
    }
  }

  const std::optional<request_id>& get_request_id(void) const {
    return request_id_;
  }

  const std::vector<uint8_t>& get_vector(void) const {
    return v_;
  }

private:
  std::optional<request_id> request_id_;
  std::vector<uint8_t> v_;
};
} // namespace impl
} // namespace local_datagram
} // namespace pqrs
