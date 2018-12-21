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
  // Converts
  // ========================================

  static std::optional<int64_t> to_int64_t(CFTypeRef _Nullable value) {
    if (!value) {
      return std::nullopt;
    }

    if (CFNumberGetTypeID() != CFGetTypeID(value)) {
      return std::nullopt;
    }

    auto cfnumber = static_cast<CFNumberRef>(value);

    int64_t result;
    if (CFNumberGetValue(cfnumber, kCFNumberSInt64Type, &result)) {
      return result;
    }

    return std::nullopt;
  }

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
