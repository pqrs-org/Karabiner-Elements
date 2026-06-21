#pragma once

// pqrs::cf::dictionary v1.2.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/cf/cf_ptr.hpp>

namespace pqrs::cf {
[[nodiscard]] inline cf_ptr<CFMutableDictionaryRef> make_cf_mutable_dictionary(CFIndex capacity = 0) noexcept {
  if (auto cf_mutable_dictionary = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                             capacity,
                                                             &kCFTypeDictionaryKeyCallBacks,
                                                             &kCFTypeDictionaryValueCallBacks)) {
    return adopt_cf_ptr(cf_mutable_dictionary);
  }

  return nullptr;
}

[[nodiscard]] inline cf_ptr<CFMutableDictionaryRef> make_cf_mutable_dictionary_copy(CFDictionaryRef dictionary,
                                                                                    CFIndex capacity = 0) noexcept {
  if (auto cf_mutable_dictionary = CFDictionaryCreateMutableCopy(kCFAllocatorDefault,
                                                                 capacity,
                                                                 dictionary)) {
    return adopt_cf_ptr(cf_mutable_dictionary);
  }

  return nullptr;
}
} // namespace pqrs::cf
