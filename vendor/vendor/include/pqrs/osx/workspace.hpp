#pragma once

// pqrs::osx::workspace v2.5

// (C) Copyright Takayama Fumihiko 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "workspace/impl/impl.h"
#include <string>

namespace pqrs {
namespace osx {
namespace workspace {

inline void open_application_by_bundle_identifier(const std::string& bundle_identifier) {
  pqrs_osx_workspace_open_application_by_bundle_identifier(bundle_identifier.c_str());
}

inline void open_application_by_file_path(const std::string& file_path) {
  pqrs_osx_workspace_open_application_by_file_path(file_path.c_str());
}

inline std::string find_application_url_by_bundle_identifier(const std::string& bundle_identifier) {
  char buffer[512];

  pqrs_osx_workspace_find_application_url_by_bundle_identifier(bundle_identifier.c_str(),
                                                               buffer,
                                                               sizeof(buffer));

  return buffer;
}

inline bool application_running_by_bundle_identifier(const std::string& bundle_identifier) {
  return pqrs_osx_workspace_application_running_by_bundle_identifier(bundle_identifier.c_str());
}

inline bool application_running_by_file_path(const std::string& file_path) {
  return pqrs_osx_workspace_application_running_by_file_path(file_path.c_str());
}

} // namespace workspace
} // namespace osx
} // namespace pqrs
