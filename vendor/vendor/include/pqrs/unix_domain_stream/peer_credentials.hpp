#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <optional>
#include <sys/types.h>

namespace pqrs::unix_domain_stream {

struct peer_credentials final {
  std::optional<pid_t> pid;
  std::optional<uid_t> uid;
  std::optional<gid_t> gid;
};

} // namespace pqrs::unix_domain_stream
