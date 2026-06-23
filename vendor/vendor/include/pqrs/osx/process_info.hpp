#pragma once

// pqrs::osx::process_info v2.4.0

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::process_info` can be used safely in a multi-threaded environment.

#include "process_info/impl/impl.h"
#include <string>

namespace pqrs::osx::process_info {
[[nodiscard]] inline std::string globally_unique_string() {
  // ProcessInfo.globallyUniqueString is UUID-like in current Foundation
  // implementations, but the API does not document a maximum length. Use a
  // comfortably larger buffer to leave room for future format changes.
  char buffer[256];

  pqrs_osx_process_info_create_globally_unique_string(buffer, sizeof(buffer));

  return buffer;
}

[[nodiscard]] inline int process_identifier() noexcept {
  return pqrs_osx_process_info_process_identifier();
}

inline void disable_sudden_termination() noexcept {
  pqrs_osx_process_info_disable_sudden_termination();
}

inline void enable_sudden_termination() noexcept {
  pqrs_osx_process_info_enable_sudden_termination();
}

class scoped_sudden_termination_blocker final {
public:
  scoped_sudden_termination_blocker() noexcept {
    disable_sudden_termination();
  }

  ~scoped_sudden_termination_blocker() noexcept {
    enable_sudden_termination();
  }
};
} // namespace pqrs::osx::process_info
