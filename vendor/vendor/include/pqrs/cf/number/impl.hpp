#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <optional>
#include <pqrs/cf/cf_ptr.hpp>

namespace pqrs::cf::impl {
template <typename T>
[[nodiscard]] inline cf_ptr<CFNumberRef> make_cf_number(T value, CFNumberType type) noexcept {
  return adopt_cf_ptr(CFNumberCreate(kCFAllocatorDefault, type, &value));
}

template <typename T>
[[nodiscard]] inline std::optional<T> make_number(CFTypeRef value, CFNumberType type) noexcept {
  if (!value) {
    return std::nullopt;
  }

  if (CFNumberGetTypeID() != CFGetTypeID(value)) {
    return std::nullopt;
  }

  T result;
  if (CFNumberGetValue(static_cast<CFNumberRef>(value), type, &result)) {
    return result;
  }

  return std::nullopt;
}
} // namespace pqrs::cf::impl
