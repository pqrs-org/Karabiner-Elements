#pragma once

// pqrs::cf::dictionary v1.1

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/cf/cf_ptr.hpp>

namespace pqrs {
namespace cf {
inline cf_ptr<CFMutableDictionaryRef> make_cf_mutable_dictionary(CFIndex capacity = 0) {
  cf_ptr<CFMutableDictionaryRef> result;

  if (auto cf_mutable_dictionary = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                             capacity,
                                                             &kCFTypeDictionaryKeyCallBacks,
                                                             &kCFTypeDictionaryValueCallBacks)) {
    result = cf_mutable_dictionary;
    CFRelease(cf_mutable_dictionary);
  }

  return result;
}

inline cf_ptr<CFMutableDictionaryRef> make_cf_mutable_dictionary_copy(CFDictionaryRef dictionary,
                                                                      CFIndex capacity = 0) {
  cf_ptr<CFMutableDictionaryRef> result;

  if (auto cf_mutable_dictionary = CFDictionaryCreateMutableCopy(kCFAllocatorDefault,
                                                                 capacity,
                                                                 dictionary)) {
    result = cf_mutable_dictionary;
    CFRelease(cf_mutable_dictionary);
  }

  return result;
}
} // namespace cf
} // namespace pqrs
