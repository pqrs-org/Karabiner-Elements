#pragma once

// pqrs::cf::boolean v1.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <CoreFoundation/CoreFoundation.h>
#include <optional>

namespace pqrs {
namespace cf {
inline CFBooleanRef make_cf_boolean(bool value) {
  return value ? kCFBooleanTrue : kCFBooleanFalse;
}

inline std::optional<bool> make_bool(CFTypeRef value) {
  if (!value) {
    return std::nullopt;
  }

  if (CFBooleanGetTypeID() != CFGetTypeID(value)) {
    return std::nullopt;
  }

  return CFBooleanGetValue(static_cast<CFBooleanRef>(value));
}
} // namespace cf
} // namespace pqrs
