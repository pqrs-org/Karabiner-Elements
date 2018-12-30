#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <SystemConfiguration/SystemConfiguration.h>
#include <optional>

namespace pqrs {
namespace osx {
namespace session {
inline std::optional<uid_t> find_console_user_id(void) {
  uid_t uid;
  if (auto user_name = SCDynamicStoreCopyConsoleUser(nullptr, &uid, nullptr)) {
    CFRelease(user_name);
    return uid;
  }

  return std::nullopt;
}
} // namespace session
} // namespace osx
} // namespace pqrs
