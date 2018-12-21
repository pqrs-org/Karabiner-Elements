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
  // CFArray, CFMutableArray
  // ========================================

  template <typename T>
  static T _Nullable get_value(CFArrayRef _Nonnull array, CFIndex index) {
    if (array && index < CFArrayGetCount(array)) {
      return static_cast<T>(const_cast<void*>(CFArrayGetValueAtIndex(array, index)));
    }
    return nullptr;
  }

  template <typename T>
  static bool exists(CFArrayRef _Nonnull array, T _Nonnull value) {
    if (array) {
      CFRange range = {0, CFArrayGetCount(array)};
      if (CFArrayContainsValue(array, range, value)) {
        return true;
      }
    }
    return false;
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

  static void set_cfmutabledictionary_value(CFMutableDictionaryRef _Nonnull dictionary,
                                            CFStringRef _Nonnull key,
                                            int64_t value) {
    if (auto number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &value)) {
      CFDictionarySetValue(dictionary, key, number);
      CFRelease(number);
    }
  }
};
} // namespace krbn
