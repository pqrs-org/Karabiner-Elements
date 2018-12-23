#pragma once

#include <Security/CodeSigning.h>
#include <optional>
#include <pqrs/cf/array.hpp>
#include <pqrs/cf/dictionary.hpp>
#include <pqrs/cf/number.hpp>
#include <pqrs/cf/string.hpp>
#include <string>

namespace krbn {
class codesign {
public:
  static std::optional<std::string> get_common_name_of_process(pid_t pid) {
    std::optional<std::string> common_name;

    if (auto attributes = pqrs::cf::make_cf_mutable_dictionary()) {
      if (auto pid_number = pqrs::cf::make_cf_number(static_cast<int64_t>(pid))) {
        CFDictionarySetValue(*attributes, kSecGuestAttributePid, *pid_number);

        SecCodeRef guest;
        if (SecCodeCopyGuestWithAttributes(nullptr, *attributes, kSecCSDefaultFlags, &guest) == errSecSuccess) {
          CFDictionaryRef information;
          if (SecCodeCopySigningInformation(guest, kSecCSSigningInformation, &information) == errSecSuccess) {
            if (auto certificates = static_cast<CFArrayRef>(CFDictionaryGetValue(information, kSecCodeInfoCertificates))) {
              if (CFArrayGetCount(certificates) > 0) {
                auto certificate = pqrs::cf::get_cf_array_value<SecCertificateRef>(certificates, 0);
                CFStringRef common_name_string;
                if (SecCertificateCopyCommonName(certificate, &common_name_string) == errSecSuccess) {
                  common_name = pqrs::cf::make_string(common_name_string);
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
};
} // namespace krbn
