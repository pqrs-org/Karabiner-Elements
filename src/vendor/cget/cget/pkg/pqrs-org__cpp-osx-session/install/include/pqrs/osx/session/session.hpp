#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "attributes.hpp"
#include "cg_attributes.hpp"
#include <SystemConfiguration/SystemConfiguration.h>
#include <optional>

namespace pqrs {
namespace osx {
namespace session {
inline std::shared_ptr<cg_attributes> find_cg_session_current_attributes(void) {
  std::shared_ptr<cg_attributes> result;

  if (auto d = CGSessionCopyCurrentDictionary()) {
    result = std::make_shared<cg_attributes>(d);
    CFRelease(d);
  }

  return result;
}
} // namespace session
} // namespace osx
} // namespace pqrs
