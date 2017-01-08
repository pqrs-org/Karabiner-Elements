#pragma once

#include "boost_defs.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <boost/optional.hpp>
#include <string>

class cf_utility final {
public:
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
};
