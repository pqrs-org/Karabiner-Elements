#pragma once

// pqrs::osx::application v1.0

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/osx/application/impl/impl.h>

namespace pqrs {
namespace osx {
namespace application {

enum class activation_policy {
  regular = pqrs_osx_application_activation_policy_regular,
  accessory = pqrs_osx_application_activation_policy_accessory,
  prohibited = pqrs_osx_application_activation_policy_prohibited,
};

inline void set_activation_policy(activation_policy policy) {
  pqrs_osx_application_set_activation_policy(static_cast<pqrs_osx_application_activation_policy_t>(policy));
}

inline void finish_launching(void) {
  pqrs_osx_application_finish_launching();
}

inline void run(void) {
  pqrs_osx_application_run();
}

} // namespace application
} // namespace osx
} // namespace pqrs
