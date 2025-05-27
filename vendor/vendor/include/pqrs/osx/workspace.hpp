#pragma once

// pqrs::osx::workspace v2.6

// (C) Copyright Takayama Fumihiko 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "workspace/impl/impl.h"
#include <string>
#include <vector>

namespace pqrs {
namespace osx {
namespace workspace {

struct open_configuration final {
  bool activates = true;
  bool adds_to_recent_items = true;
  bool allows_running_application_substitution = true;
  bool creates_new_application_instance = false;
  bool hides = false;
  bool hides_others = false;
  std::vector<std::string> arguments;
  // All data members must be public to use designated initializers,
  // so even the temporary variable for the `c_struct` method is exposed as public.
  mutable std::vector<const char*> internal_c_str_arguments_;

  pqrs_osx_workspace_open_configuration c_struct(void) const {
    internal_c_str_arguments_.clear();
    for (const auto& arg : arguments) {
      internal_c_str_arguments_.push_back(arg.c_str());
    }
    internal_c_str_arguments_.push_back(nullptr);

    return pqrs_osx_workspace_open_configuration{
        .activates = activates,
        .adds_to_recent_items = adds_to_recent_items,
        .allows_running_application_substitution = allows_running_application_substitution,
        .creates_new_application_instance = creates_new_application_instance,
        .hides = hides,
        .hides_others = hides_others,
        .arguments = internal_c_str_arguments_.data(),
    };
  }
};

inline void open_application_by_bundle_identifier(const std::string& bundle_identifier,
                                                  const open_configuration& configuration) {
  pqrs_osx_workspace_open_application_by_bundle_identifier(bundle_identifier.c_str(),
                                                           &(configuration.c_struct()));
}

inline void open_application_by_bundle_identifier(const std::string& bundle_identifier) {
  open_application_by_bundle_identifier(bundle_identifier,
                                        open_configuration());
}

inline void open_application_by_file_path(const std::string& file_path,
                                          const open_configuration& configuration) {
  pqrs_osx_workspace_open_application_by_file_path(file_path.c_str(),
                                                   &(configuration.c_struct()));
}

inline void open_application_by_file_path(const std::string& file_path) {
  open_application_by_file_path(file_path,
                                open_configuration());
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
