#pragma once

// pqrs::cf::bundle v1.0

// (C) Copyright Takayama Fumihiko 2025.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <CoreFoundation/CoreFoundation.h>
#include <filesystem>
#include <optional>
#include <pqrs/cf/url.hpp>
#include <string>

namespace pqrs {
namespace cf {
namespace bundle {

constexpr const char* package_type_application = "APPL";
constexpr const char* package_type_framework = "FMWK";
constexpr const char* package_type_bundle = "BNDL";

std::optional<std::string> get_package_type(const std::filesystem::path& bundle_path) {
  if (auto url = pqrs::cf::make_url(bundle_path.string())) {
    if (auto bundle = pqrs::cf::cf_ptr(CFBundleCreate(nullptr, *url))) {
      if (auto info_dictionary = CFBundleGetInfoDictionary(*bundle)) {
        if (auto type = CFDictionaryGetValue(info_dictionary,
                                             CFSTR("CFBundlePackageType"))) {
          return pqrs::cf::make_string(type);
        }
      }
    }
  }

  return std::nullopt;
}

bool application(const std::filesystem::path& bundle_path) {
  return get_package_type(bundle_path) == package_type_application;
}

} // namespace bundle
} // namespace cf
} // namespace pqrs
