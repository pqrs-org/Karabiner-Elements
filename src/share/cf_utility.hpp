#pragma once

#include "boost_defs.hpp"

#include "logger.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace krbn {
class cf_utility final {
public:
  // ========================================
  // CFDictionary, CFMutableDictionary
  // ========================================

  static CFMutableDictionaryRef _Nonnull create_cfmutabledictionary(CFIndex capacity = 0) {
    return CFDictionaryCreateMutable(nullptr,
                                     capacity,
                                     &kCFTypeDictionaryKeyCallBacks,
                                     &kCFTypeDictionaryValueCallBacks);
  }
};
} // namespace krbn
