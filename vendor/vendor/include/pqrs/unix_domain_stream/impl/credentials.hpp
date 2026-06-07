#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "../peer_credentials.hpp"
#include "asio_helper.hpp"

#if defined(__APPLE__)
#include <sys/socket.h>
#include <unistd.h>
#elif defined(__linux__)
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace pqrs::unix_domain_stream::impl {

inline peer_credentials make_peer_credentials(asio::local::stream_protocol::socket& socket) {
  peer_credentials result;

#if defined(__APPLE__)
  {
    pid_t pid{};
    socklen_t len = sizeof(pid);
    if (getsockopt(socket.native_handle(),
                   SOL_LOCAL,
                   LOCAL_PEERPID,
                   &pid,
                   &len) == 0) {
      result.pid = pid;
    }
  }

  {
    uid_t uid{};
    gid_t gid{};
    if (getpeereid(socket.native_handle(),
                   &uid,
                   &gid) == 0) {
      result.uid = uid;
      result.gid = gid;
    }
  }
#elif defined(__linux__)
  {
    struct ucred credentials{};
    socklen_t len = sizeof(credentials);
    if (getsockopt(socket.native_handle(),
                   SOL_SOCKET,
                   SO_PEERCRED,
                   &credentials,
                   &len) == 0) {
      result.pid = credentials.pid;
      result.uid = credentials.uid;
      result.gid = credentials.gid;
    }
  }
#endif

  return result;
}

} // namespace pqrs::unix_domain_stream::impl
