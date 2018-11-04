#pragma once

#include "boost_defs.hpp"

#include "dispatcher.hpp"
#include "logger.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <boost/optional.hpp>
#include <mutex>
#include <string>
#include <thread>

namespace krbn {
class cf_utility final {
public:
  // ========================================
  // Converts
  // ========================================

  static boost::optional<std::string> to_string(CFTypeRef _Nullable value) {
    if (!value) {
      return boost::none;
    }

    if (CFStringGetTypeID() != CFGetTypeID(value)) {
      return boost::none;
    }

    auto cfstring = static_cast<CFStringRef>(value);

    std::string string;
    if (auto p = CFStringGetCStringPtr(cfstring, kCFStringEncodingUTF8)) {
      string = p;
    } else {
      auto length = CFStringGetLength(cfstring);
      auto max_size = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
      char* buffer = new char[max_size];
      if (CFStringGetCString(cfstring, buffer, max_size, kCFStringEncodingUTF8)) {
        string = buffer;
      }
      delete[] buffer;
    }

    return string;
  }

  static CFStringRef _Nullable create_cfstring(const std::string& string) {
    return CFStringCreateWithCString(kCFAllocatorDefault,
                                     string.c_str(),
                                     kCFStringEncodingUTF8);
  }

  static boost::optional<int64_t> to_int64_t(CFTypeRef _Nullable value) {
    if (!value) {
      return boost::none;
    }

    if (CFNumberGetTypeID() != CFGetTypeID(value)) {
      return boost::none;
    }

    auto cfnumber = static_cast<CFNumberRef>(value);

    int64_t result;
    if (CFNumberGetValue(cfnumber, kCFNumberSInt64Type, &result)) {
      return result;
    }

    return boost::none;
  }

  // ========================================
  // CFArray, CFMutableArray
  // ========================================

  static CFArrayRef _Nonnull create_empty_cfarray(void) {
    return CFArrayCreate(nullptr, nullptr, 0, &kCFTypeArrayCallBacks);
  }

  static CFMutableArrayRef _Nonnull create_cfmutablearray(CFIndex capacity = 0) {
    return CFArrayCreateMutable(nullptr, capacity, &kCFTypeArrayCallBacks);
  }

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
