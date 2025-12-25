#pragma once

// pqrs::cf::bundle v2.2

// (C) Copyright Takayama Fumihiko 2025.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <CoreFoundation/CoreFoundation.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <optional>
#include <pqrs/cf/url.hpp>
#include <string>
#include <string_view>

namespace pqrs {
namespace cf {
namespace bundle {

constexpr std::string_view package_type_application("APPL");
constexpr std::string_view package_type_framework("FMWK");
constexpr std::string_view package_type_bundle("BNDL");
constexpr std::string_view package_type_finder("FNDR");

std::optional<std::string> get_package_type(const std::filesystem::path& bundle_path,
                                            bool guess_if_missing_package_type = false) {
  if (auto url = make_file_path_url(bundle_path.string(), true)) {
    if (auto bundle = cf_ptr(CFBundleCreate(nullptr, *url))) {
      if (auto info_dictionary = CFBundleGetInfoDictionary(*bundle)) {
        if (auto type = CFDictionaryGetValue(info_dictionary,
                                             CFSTR("CFBundlePackageType"))) {
          return make_string(type);
        }

        if (guess_if_missing_package_type) {
          auto extension = bundle_path.extension().string();
          std::transform(std::cbegin(extension),
                         std::cend(extension),
                         std::begin(extension),
                         [](unsigned char c) { return std::tolower(c); });

          if (extension == ".app") {
            return std::string(package_type_application);
          }

          if (extension == ".framework") {
            return std::string(package_type_framework);
          }

          return std::string(package_type_bundle);
        }
      }
    }
  }

  return std::nullopt;
}

bool application(const std::filesystem::path& bundle_path) {
  auto package_type = get_package_type(bundle_path,
                                       true);
  return package_type == package_type_application ||
         package_type == package_type_finder;
}

} // namespace bundle
} // namespace cf
} // namespace pqrs
