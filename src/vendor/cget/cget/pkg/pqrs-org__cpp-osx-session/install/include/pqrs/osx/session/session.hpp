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
inline std::optional<uid_t> find_console_user_id(void) {
  uid_t uid;
  if (auto user_name = SCDynamicStoreCopyConsoleUser(nullptr, &uid, nullptr)) {
    CFRelease(user_name);
    return uid;
  }

  return std::nullopt;
}

inline std::shared_ptr<cg_attributes> find_cg_session_current_attributes(void) {
  std::shared_ptr<cg_attributes> result;

  if (auto d = CGSessionCopyCurrentDictionary()) {
    result = std::make_shared<cg_attributes>(d);
    CFRelease(d);
  }

  return result;
}

inline std::optional<SecuritySessionId> find_caller_security_session(void) {
  SecuritySessionId id = 0;
  SessionAttributeBits attributes = 0;
  if (SessionGetInfo(callerSecuritySession, &id, &attributes) != errSessionSuccess) {
    return std::nullopt;
  }
  return id;
}

inline std::optional<attributes> find_attributes(SecuritySessionId session_id) {
  SecuritySessionId id = 0;
  SessionAttributeBits bits = 0;
  if (SessionGetInfo(session_id, &id, &bits) != errSessionSuccess) {
    return std::nullopt;
  }
  return attributes(bits);
}
} // namespace session
} // namespace osx
} // namespace pqrs
