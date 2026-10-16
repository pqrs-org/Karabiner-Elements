#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <system_error>

namespace pqrs::osx::xpc {

enum class listener_recovery_action {
  // Destroy this listener after signal delivery and create a new instance.
  recreate_listener,

  // Exit the process so its service manager can launch a fresh instance.
  // Used when a named listener loses its Mach service registration.
  restart_process,

  // Correct the listener options, then create a new listener instance.
  correct_configuration,
};

struct listener_recovery_request final {
  // The error that made this listener instance unusable.
  std::error_code error;

  // The recovery action recommended from the listener type and terminal error.
  listener_recovery_action recommended_action;
};

} // namespace pqrs::osx::xpc
