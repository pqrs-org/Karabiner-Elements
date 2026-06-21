#pragma once

// pqrs::cf::url v1.2.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <optional>
#include <pqrs/cf/cf_ptr.hpp>
#include <pqrs/cf/string.hpp>
#include <string>

namespace pqrs::cf {
[[nodiscard]] inline std::optional<std::string> make_string(CFURLRef url) {
  if (url) {
    return make_string(CFURLGetString(url));
  }

  return std::nullopt;
}

[[nodiscard]] inline cf_ptr<CFURLRef> make_url(const std::string& url_string) {
  if (auto s = make_cf_string(url_string)) {
    if (auto url = CFURLCreateWithString(kCFAllocatorDefault,
                                         *s,
                                         nullptr)) {
      return adopt_cf_ptr(url);
    }
  }

  return {};
}

[[nodiscard]] inline cf_ptr<CFURLRef> make_file_path_url(const std::string& file_path, bool is_directory) {
  if (auto s = make_cf_string(file_path)) {
    if (auto url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                 *s,
                                                 kCFURLPOSIXPathStyle,
                                                 is_directory)) {
      return adopt_cf_ptr(url);
    }
  }

  return {};
}
} // namespace pqrs::cf
