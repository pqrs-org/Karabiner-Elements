#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <system_error>

namespace pqrs::osx::xpc {

enum class client_recovery_action {
  // Destroy this client after signal delivery and create a new instance.
  recreate_client,

  // Update or restart the peer, then create a new client instance.
  recreate_client_after_peer_update,

  // Correct the client options, then create a new client instance.
  correct_configuration,
};

struct client_recovery_request final {
  // The error that made this client instance unusable.
  std::error_code error;

  // The recovery action recommended from the terminal error.
  client_recovery_action recommended_action;
};

} // namespace pqrs::osx::xpc
