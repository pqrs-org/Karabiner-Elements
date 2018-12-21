#pragma once

// pqrs::cf::string v2.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <optional>
#include <pqrs/cf/cf_ptr.hpp>
#include <string>
#include <vector>

namespace pqrs {
namespace cf {
inline std::optional<std::string> make_string(CFTypeRef value) {
  if (value) {
    if (CFGetTypeID(value) == CFStringGetTypeID()) {
      auto cf_string = static_cast<CFStringRef>(value);

      if (auto p = CFStringGetCStringPtr(cf_string, kCFStringEncodingUTF8)) {
        return p;
      } else {
        // When cf_string contains unicode character such as `こんにちは`.
        auto length = CFStringGetLength(cf_string);
        auto max_size = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
        std::vector<char> buffer(max_size);
        if (CFStringGetCString(cf_string, &(buffer[0]), max_size, kCFStringEncodingUTF8)) {
          return &(buffer[0]);
        }
      }
    }
  }

  return std::nullopt;
}

inline cf_ptr<CFStringRef> make_cf_string(const std::string& string) {
  cf_ptr<CFStringRef> result;

  if (auto cf_string = CFStringCreateWithCString(kCFAllocatorDefault,
                                                 string.c_str(),
                                                 kCFStringEncodingUTF8)) {
    result = cf_string;
    CFRelease(cf_string);
  }

  return result;
}
} // namespace cf
} // namespace pqrs
