#pragma once

// pqrs::cf::boolean v1.1.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <CoreFoundation/CoreFoundation.h>
#include <optional>

namespace pqrs::cf {

[[nodiscard]] inline CFBooleanRef make_cf_boolean(bool value) noexcept {
  return value ? kCFBooleanTrue : kCFBooleanFalse;
}

[[nodiscard]] inline std::optional<bool> make_bool(CFTypeRef value) noexcept {
  if (!value) {
    return std::nullopt;
  }

  if (CFBooleanGetTypeID() != CFGetTypeID(value)) {
    return std::nullopt;
  }

  return CFBooleanGetValue(static_cast<CFBooleanRef>(value));
}

} // namespace pqrs::cf
