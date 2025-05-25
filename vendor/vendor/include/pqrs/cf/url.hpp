#pragma once

// pqrs::cf::url v1.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/cf/cf_ptr.hpp>
#include <pqrs/cf/string.hpp>
#include <string>

namespace pqrs {
namespace cf {
inline std::optional<std::string> make_string(CFURLRef url) {
  return pqrs::cf::make_string(CFURLGetString(url));
}

inline cf_ptr<CFURLRef> make_url(const std::string& file_path) {
  cf_ptr<CFURLRef> result;

  if (auto s = make_cf_string(file_path)) {
    if (auto url = CFURLCreateWithString(kCFAllocatorDefault,
                                         *s,
                                         nullptr)) {
      result = url;
      CFRelease(url);
    }
  }

  return result;
}

inline cf_ptr<CFURLRef> make_file_path_url(const std::string& file_path, bool is_directory) {
  cf_ptr<CFURLRef> result;

  if (auto s = make_cf_string(file_path)) {
    if (auto url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                 *s,
                                                 kCFURLPOSIXPathStyle,
                                                 is_directory)) {
      result = url;
      CFRelease(url);
    }
  }

  return result;
}
} // namespace cf
} // namespace pqrs
