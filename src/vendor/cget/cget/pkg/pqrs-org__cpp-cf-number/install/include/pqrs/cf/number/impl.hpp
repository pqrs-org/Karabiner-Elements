#pragma once

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <optional>
#include <pqrs/cf/cf_ptr.hpp>

namespace pqrs {
namespace cf {
namespace impl {
template <typename T>
inline cf_ptr<CFNumberRef> make_cf_number(T value, CFNumberType type) {
  cf_ptr<CFNumberRef> result;

  if (auto number = CFNumberCreate(kCFAllocatorDefault, type, &value)) {
    result = number;
    CFRelease(number);
  }

  return result;
}

template <typename T>
inline std::optional<T> make_number(CFTypeRef value, CFNumberType type) {
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
} // namespace impl
} // namespace cf
} // namespace pqrs
