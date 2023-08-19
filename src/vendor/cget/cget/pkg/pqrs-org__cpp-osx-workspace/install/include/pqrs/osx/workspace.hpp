#pragma once

// pqrs::osx::workspace v2.2

// (C) Copyright Takayama Fumihiko 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "workspace/impl/impl.h"
#include <string>

namespace pqrs {
namespace osx {
namespace workspace {

inline std::string find_application_url_by_bundle_identifier(const std::string& bundle_identifier) {
  char buffer[512];

  pqrs_osx_workspace_find_application_url_by_bundle_identifier(bundle_identifier.c_str(),
                                                               buffer,
                                                               sizeof(buffer));

  return buffer;
}

} // namespace workspace
} // namespace osx
} // namespace pqrs
