#pragma once

#include "boost_defs.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <boost/optional.hpp>
#include <string>

namespace krbn {
class cf_utility final {
public:
  template <typename T>
  class deleter final {
  public:
    using pointer = T;
    void operator()(T _Nullable ref) {
      if (ref) {
        CFRelease(ref);
      }
    }
  };

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

  static CFMutableDictionaryRef _Nonnull create_cfmutabledictionary(CFIndex capacity = 0) {
    return CFDictionaryCreateMutable(nullptr,
                                     capacity,
                                     &kCFTypeDictionaryKeyCallBacks,
                                     &kCFTypeDictionaryValueCallBacks);
  }
};
  
class cf_string final {
public:
  cf_string(): value_(nullptr) {
  }

  cf_string(const _Nullable CFStringRef& s, bool retain) {
    value_ = s;
    if (retain && value_ != nullptr)
      CFRetain(value_);
  }

  cf_string(const std::string& s) {
    value_ = CFStringCreateWithCString(kCFAllocatorDefault, s.c_str(), kCFStringEncodingUTF8);
  }
  
  cf_string(const cf_string& s) {
    value_ = s.value_;
    CFRetain(value_);
  }
  
  ~cf_string() {
    if (value_ != nullptr)
      CFRelease(value_);
  }
  
  cf_string& operator=(const cf_string& s) {
    if (value_ != nullptr)
      CFRelease(value_);

    value_ = s.value_;
    if (value_ != nullptr)
      CFRetain(value_);
    
    return *this;
  }
  
  cf_string& operator=(const std::string& s) {
    if (value_ != nullptr)
      CFRelease(value_);
    
    value_ = CFStringCreateWithCString(kCFAllocatorDefault, s.c_str(), kCFStringEncodingUTF8);
    
    return *this;
  }
  
  inline _Nullable CFStringRef get() const {
    return value_;
  }
  
private:
  _Nullable CFStringRef value_;
};
  
inline bool operator==(const cf_string& lhs, const cf_string& rhs) {
  if (lhs.get() == nullptr || rhs.get() == nullptr)
    return lhs.get() == rhs.get();
  return CFStringCompare(lhs.get(), rhs.get(), 0) == 0;
}

inline bool operator==(const _Nullable CFStringRef& lhs, const cf_string& rhs) {
  if (lhs == nullptr || rhs.get() == nullptr)
    return lhs == rhs.get();
  return CFStringCompare(lhs, rhs.get(), 0) == 0;
}

inline bool operator==(const cf_string& lhs, const _Nullable CFStringRef& rhs) {
  if (lhs.get() == nullptr || rhs == nullptr)
    return lhs.get() == rhs;
  return CFStringCompare(lhs.get(), rhs, 0) == 0;
}

inline bool operator!=(const cf_string& lhs, const cf_string& rhs) {
  if (lhs.get() == nullptr || rhs.get() == nullptr)
    return lhs.get() != rhs.get();
  return CFStringCompare(lhs.get(), rhs.get(), 0) != 0;
}

inline bool operator!=(const _Nullable CFStringRef& lhs, const cf_string& rhs) {
  if (lhs == nullptr || rhs.get() == nullptr)
    return lhs != rhs.get();
  return CFStringCompare(lhs, rhs.get(), 0) != 0;
}

inline bool operator!=(const cf_string& lhs, const _Nullable CFStringRef& rhs) {
  if (lhs.get() == nullptr || rhs == NULL)
    return lhs.get() != rhs;
  return CFStringCompare(lhs.get(), rhs, 0) != 0;
}
} // namespace krbn
