#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "peer_context.hpp"
#include "reply_token.hpp"

namespace pqrs::osx::xpc {

// Describes the remote peer and the single-use reply context associated
// with one incoming request.
struct request_context final {
  peer_context peer;
  reply_token reply;
};

} // namespace pqrs::osx::xpc
