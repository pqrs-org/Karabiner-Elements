#pragma once

// (C) Copyright Takayama Fumihiko 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

namespace pqrs {
namespace local_datagram {

inline bool non_empty_filesystem_endpoint_path(const std::string& path) {
  if (path.size() == 0) {
    return false;
  }

  // The abstract socket namespace starts '\0'.
  if (path.c_str()[0] == '\0') {
    return false;
  }

  return true;
}

inline bool non_empty_filesystem_endpoint_path(const asio::local::datagram_protocol::endpoint& endpoint) {
  return non_empty_filesystem_endpoint_path(endpoint.path());
}

} // namespace local_datagram
} // namespace pqrs
