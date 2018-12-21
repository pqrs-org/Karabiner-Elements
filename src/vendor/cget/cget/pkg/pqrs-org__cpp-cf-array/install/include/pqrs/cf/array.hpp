#pragma once

// pqrs::cf::array v2.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/cf/cf_ptr.hpp>

namespace pqrs {
namespace cf {
inline cf_ptr<CFArrayRef> make_empty_cf_array(void) {
  cf_ptr<CFArrayRef> result;

  if (auto cf_array = CFArrayCreate(kCFAllocatorDefault,
                                    nullptr,
                                    0,
                                    &kCFTypeArrayCallBacks)) {
    result = cf_array;
    CFRelease(cf_array);
  }

  return result;
}

inline cf_ptr<CFMutableArrayRef> make_cf_mutable_array(CFIndex capacity = 0) {
  cf_ptr<CFMutableArrayRef> result;

  if (auto cf_mutable_array = CFArrayCreateMutable(kCFAllocatorDefault,
                                                   capacity,
                                                   &kCFTypeArrayCallBacks)) {
    result = cf_mutable_array;
    CFRelease(cf_mutable_array);
  }

  return result;
}

template <typename T>
inline T get_cf_array_value(CFArrayRef array, CFIndex index) {
  if (array && index < CFArrayGetCount(array)) {
    return static_cast<T>(const_cast<void*>(CFArrayGetValueAtIndex(array, index)));
  }
  return nullptr;
}
} // namespace cf
} // namespace pqrs
