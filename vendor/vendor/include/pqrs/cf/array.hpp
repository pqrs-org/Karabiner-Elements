#pragma once

// pqrs::cf::array v2.1.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/cf/cf_ptr.hpp>

namespace pqrs::cf {
[[nodiscard]] inline cf_ptr<CFArrayRef> make_empty_cf_array() noexcept {
  return adopt_cf_ptr(CFArrayCreate(kCFAllocatorDefault,
                                    nullptr,
                                    0,
                                    &kCFTypeArrayCallBacks));
}

[[nodiscard]] inline cf_ptr<CFMutableArrayRef> make_cf_mutable_array(CFIndex capacity = 0) noexcept {
  return adopt_cf_ptr(CFArrayCreateMutable(kCFAllocatorDefault,
                                           capacity,
                                           &kCFTypeArrayCallBacks));
}

template <typename T>
[[nodiscard]] inline T get_cf_array_value(CFArrayRef array, CFIndex index) noexcept {
  if (array && index >= 0 && index < CFArrayGetCount(array)) {
    return static_cast<T>(const_cast<void*>(CFArrayGetValueAtIndex(array, index)));
  }
  return nullptr;
}
} // namespace pqrs::cf
