#pragma once

// pqrs::osx::codesign v2.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <Security/CodeSigning.h>
#include <optional>
#include <pqrs/cf/array.hpp>
#include <pqrs/cf/dictionary.hpp>
#include <pqrs/cf/number.hpp>
#include <pqrs/cf/string.hpp>
#include <string>

namespace pqrs {
namespace osx {
namespace codesign {

class signing_information final {
public:
  signing_information(void) {
  }

  signing_information(CFDictionaryRef information) {
    if (auto value = static_cast<CFStringRef>(CFDictionaryGetValue(information, kSecCodeInfoTeamIdentifier))) {
      team_id_ = cf::make_string(value);
    }

    if (auto value = static_cast<CFStringRef>(CFDictionaryGetValue(information, kSecCodeInfoIdentifier))) {
      identifier_ = cf::make_string(value);
    }
  }

  const std::optional<std::string>& get_team_id(void) const {
    return team_id_;
  }

  const std::optional<std::string>& get_identifier(void) const {
    return identifier_;
  }

private:
  std::optional<std::string> team_id_;
  std::optional<std::string> identifier_;
};

inline signing_information get_signing_information_of_process(pid_t pid) {
  signing_information result;

  if (auto attributes = cf::make_cf_mutable_dictionary()) {
    if (auto pid_number = cf::make_cf_number(static_cast<int64_t>(pid))) {
      CFDictionarySetValue(*attributes, kSecGuestAttributePid, *pid_number);

      SecCodeRef guest;
      if (SecCodeCopyGuestWithAttributes(nullptr, *attributes, kSecCSDefaultFlags, &guest) == errSecSuccess) {
        CFDictionaryRef information;
        if (SecCodeCopySigningInformation(guest, kSecCSSigningInformation, &information) == errSecSuccess) {
          result = signing_information(information);

          CFRelease(information);
        }

        CFRelease(guest);
      }
    }
  }

  return result;
}

inline std::optional<std::string> find_common_name_of_process(pid_t pid) {
  std::optional<std::string> common_name;

  if (auto attributes = cf::make_cf_mutable_dictionary()) {
    if (auto pid_number = cf::make_cf_number(static_cast<int64_t>(pid))) {
      CFDictionarySetValue(*attributes, kSecGuestAttributePid, *pid_number);

      SecCodeRef guest;
      if (SecCodeCopyGuestWithAttributes(nullptr, *attributes, kSecCSDefaultFlags, &guest) == errSecSuccess) {
        CFDictionaryRef information;
        if (SecCodeCopySigningInformation(guest, kSecCSSigningInformation, &information) == errSecSuccess) {
          if (auto certificates = static_cast<CFArrayRef>(CFDictionaryGetValue(information, kSecCodeInfoCertificates))) {
            if (CFArrayGetCount(certificates) > 0) {
              auto certificate = cf::get_cf_array_value<SecCertificateRef>(certificates, 0);
              CFStringRef common_name_string;
              if (SecCertificateCopyCommonName(certificate, &common_name_string) == errSecSuccess) {
                common_name = cf::make_string(common_name_string);
                CFRelease(common_name_string);
              }
            }
          }

          CFRelease(information);
        }

        CFRelease(guest);
      }
    }
  }

  return common_name;
}

} // namespace codesign
} // namespace osx
} // namespace pqrs
