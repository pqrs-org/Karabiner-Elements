#pragma once

// pqrs::osx::process_info v1.0

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::osx::process_info` can be used safely in a multi-threaded environment.

#include "process_info/objc.h"
#include <pqrs/cf/string.hpp>

namespace pqrs {
namespace osx {
namespace process_info {
inline std::string globally_unique_string(void) {
  char buffer[256];

  pqrs_osx_process_info_create_globally_unique_string(buffer, sizeof(buffer));

  return buffer;
}

inline int process_identifier(void) {
  return pqrs_osx_process_info_process_identifier();
}

void disable_sudden_termination(void) {
  pqrs_osx_process_info_disable_sudden_termination();
}

void enable_sudden_termination(void) {
  pqrs_osx_process_info_enable_sudden_termination();
}
} // namespace process_info
} // namespace osx
} // namespace pqrs
