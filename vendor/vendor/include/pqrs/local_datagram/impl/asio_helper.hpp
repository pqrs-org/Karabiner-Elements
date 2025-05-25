#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#ifdef ASIO_STANDALONE
#include <asio.hpp>
#else
#define ASIO_STANDALONE
#include <asio.hpp>
#undef ASIO_STANDALONE
#endif

namespace pqrs {
namespace local_datagram {
namespace impl {
namespace asio_helper {

namespace time_point {
inline asio::steady_timer::time_point now() {
  return asio::steady_timer::clock_type::now();
}

inline asio::steady_timer::time_point pos_infin() {
  return asio::steady_timer::time_point::max();
}

inline asio::steady_timer::time_point neg_infin() {
  return asio::steady_timer::time_point::min();
}

} // namespace time_point
} // namespace asio_helper
} // namespace impl
} // namespace local_datagram
} // namespace pqrs
