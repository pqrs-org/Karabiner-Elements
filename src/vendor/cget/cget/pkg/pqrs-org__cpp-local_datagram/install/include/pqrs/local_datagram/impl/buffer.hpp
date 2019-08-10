#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::local_datagram::impl::buffer` can be used safely in a multi-threaded environment.

#include <vector>

namespace pqrs {
namespace local_datagram {
namespace impl {
class buffer final {
public:
  buffer(void) {
  }

  buffer(const std::vector<uint8_t>& v) {
    v_ = v;
  }

  buffer(const uint8_t* p, size_t length) {
    if (p && length > 0) {
      v_.resize(length);
      memcpy(&(v_[0]), p, length);
    }
  }

  const std::vector<uint8_t>& get_vector(void) const { return v_; }

private:
  std::vector<uint8_t> v_;
};
} // namespace impl
} // namespace local_datagram
} // namespace pqrs
