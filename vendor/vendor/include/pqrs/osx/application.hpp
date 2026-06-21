#pragma once

// pqrs::osx::application v1.3.0

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/osx/application/impl/impl.h>

namespace pqrs::osx::application {

enum class activation_policy {
  regular = pqrs_osx_application_activation_policy_regular,
  accessory = pqrs_osx_application_activation_policy_accessory,
  prohibited = pqrs_osx_application_activation_policy_prohibited,
};

inline void set_activation_policy(activation_policy policy) noexcept {
  pqrs_osx_application_set_activation_policy(static_cast<pqrs_osx_application_activation_policy_t>(policy));
}

inline void finish_launching() noexcept {
  pqrs_osx_application_finish_launching();
}

inline void run() noexcept {
  pqrs_osx_application_run();
}

inline void stop() noexcept {
  pqrs_osx_application_stop();
}

// Prevents NSApplication.shared.terminate from calling exit internally.
// When a terminate request is received, stop is called instead, allowing the run loop to exit.
// (applicationShouldTerminate returns .terminateCancel.)
inline void enable_stop_on_terminate() noexcept {
  pqrs_osx_application_enable_stop_on_terminate();
}

} // namespace pqrs::osx::application
