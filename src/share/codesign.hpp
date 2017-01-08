#pragma once

#include "boost_defs.hpp"

#include "cf_utility.hpp"
#include <Security/CodeSigning.h>
#include <boost/optional.hpp>
#include <string>

class codesign {
public:
  static boost::optional<std::string> get_common_name_of_process(pid_t pid) {
    boost::optional<std::string> common_name;

    if (auto attributes = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                    0,
                                                    &kCFTypeDictionaryKeyCallBacks,
                                                    &kCFTypeDictionaryValueCallBacks)) {
      if (auto pid_number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &pid)) {
        CFDictionarySetValue(attributes, kSecGuestAttributePid, pid_number);

        SecCodeRef guest;
        if (SecCodeCopyGuestWithAttributes(nullptr, attributes, kSecCSDefaultFlags, &guest) == errSecSuccess) {
          CFDictionaryRef information;
          if (SecCodeCopySigningInformation(guest, kSecCSSigningInformation, &information) == errSecSuccess) {
            if (auto certificates = static_cast<CFArrayRef>(CFDictionaryGetValue(information, kSecCodeInfoCertificates))) {
              if (CFArrayGetCount(certificates) > 0) {
                auto certificate = static_cast<SecCertificateRef>(const_cast<void*>(CFArrayGetValueAtIndex(certificates, 0)));
                CFStringRef common_name_string;
                if (SecCertificateCopyCommonName(certificate, &common_name_string) == errSecSuccess) {
                  common_name = cf_utility::to_string(common_name_string);
                  CFRelease(common_name_string);
                }
              }
            }

            CFRelease(information);
          }

          CFRelease(guest);
        }

        CFRelease(pid_number);
      }

      CFRelease(attributes);
    }

    return common_name;
  }
};
