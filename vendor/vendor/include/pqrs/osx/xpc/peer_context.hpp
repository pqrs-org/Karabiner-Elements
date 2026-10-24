#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "peer_identity.hpp"
#include "types.hpp"
#include <optional>

namespace pqrs::osx::xpc {

// Identifies a peer within one transport. Identity is available after the
// transport handshake succeeds; failures before that point carry no identity.
struct peer_context final {
  peer_id id;
  std::optional<peer_identity> identity;
};

} // namespace pqrs::osx::xpc
