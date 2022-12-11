#pragma once

// pqrs::osx::workspace v1.0

// (C) Copyright Takayama Fumihiko 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "workspace/impl/objc.h"
#include <string_view>

namespace pqrs {
namespace osx {
namespace workspace {

inline std::string find_application_url_by_bundle_identifier(const std::string_view& bundle_identifier) {
  auto url = pqrs_osx_workspace_find_application_url_by_bundle_identifier(bundle_identifier.data());
  if (url == nullptr) {
    return "";
  }

  std::string result(url);
  free(url);
  return result;
}

} // namespace workspace
} // namespace osx
} // namespace pqrs
