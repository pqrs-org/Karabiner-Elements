#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "attributes.hpp"
#include "cg_attributes.hpp"
#include <SystemConfiguration/SystemConfiguration.h>
#include <memory>
#include <pqrs/cf/cf_ptr.hpp>

namespace pqrs::osx::session {
[[nodiscard]] inline std::shared_ptr<cg_attributes> find_cg_session_current_attributes() {
  std::shared_ptr<cg_attributes> result;

  if (auto dictionary = pqrs::cf::adopt_cf_ptr(CGSessionCopyCurrentDictionary())) {
    result = std::make_shared<cg_attributes>(*dictionary);
  }

  return result;
}
} // namespace pqrs::osx::session
