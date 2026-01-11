#pragma once

// pqrs::osx::codesign v3.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "codesign/anchor_type.hpp"
#include "codesign/team_id.hpp"
#include <Security/CodeSigning.h>
#include <filesystem>
#include <optional>
#include <pqrs/cf/array.hpp>
#include <pqrs/cf/dictionary.hpp>
#include <pqrs/cf/number.hpp>
#include <pqrs/cf/string.hpp>
#include <pqrs/cf/url.hpp>
#include <string>

namespace pqrs {
namespace osx {
namespace codesign {

class signing_information final {
public:
  signing_information(void) {
  }

  signing_information(CFDictionaryRef information, anchor_type anchor_type)
      : verified_anchor_type_(anchor_type) {
    if (auto value = static_cast<CFStringRef>(CFDictionaryGetValue(information, kSecCodeInfoTeamIdentifier))) {
      if (auto s = cf::make_string(value)) {
        verified_team_id_ = team_id(*s);
      }
    }

    if (auto value = static_cast<CFStringRef>(CFDictionaryGetValue(information, kSecCodeInfoIdentifier))) {
      identifier_ = cf::make_string(value);
    }
  }

  anchor_type get_verified_anchor_type(void) const {
    return verified_anchor_type_;
  }

  const std::optional<team_id>& get_verified_team_id(void) const {
    return verified_team_id_;
  }

  const std::optional<std::string>& get_identifier(void) const {
    return identifier_;
  }

private:
  anchor_type verified_anchor_type_{anchor_type::none};
  std::optional<team_id> verified_team_id_;
  std::optional<std::string> identifier_;
};

inline cf::cf_ptr<SecRequirementRef> get_anchor_apple_requirement(void) {
  cf::cf_ptr<SecRequirementRef> result;

  SecRequirementRef req = nullptr;
  auto status = SecRequirementCreateWithString(CFSTR("anchor apple"),
                                               kSecCSDefaultFlags,
                                               &req);
  if (status == errSecSuccess) {
    result = req;

    CFRelease(req);
  }

  return result;
}

inline cf::cf_ptr<SecRequirementRef> get_anchor_apple_generic_requirement(void) {
  cf::cf_ptr<SecRequirementRef> result;

  SecRequirementRef req = nullptr;
  auto status = SecRequirementCreateWithString(CFSTR("anchor apple generic"),
                                               kSecCSDefaultFlags,
                                               &req);
  if (status == errSecSuccess) {
    result = req;

    CFRelease(req);
  }

  return result;
}

inline cf::cf_ptr<SecCodeRef> get_code_of_process(pid_t pid) {
  cf::cf_ptr<SecCodeRef> result;

  if (auto attributes = cf::make_cf_mutable_dictionary()) {
    if (auto pid_number = cf::make_cf_number(static_cast<int64_t>(pid))) {
      CFDictionarySetValue(*attributes, kSecGuestAttributePid, *pid_number);

      SecCodeRef guest;
      if (SecCodeCopyGuestWithAttributes(nullptr, *attributes, kSecCSDefaultFlags, &guest) == errSecSuccess) {
        result = guest;

        CFRelease(guest);
      }
    }
  }

  return result;
}

inline cf::cf_ptr<SecStaticCodeRef> get_code_of_file(std::filesystem::path file_path) {
  cf::cf_ptr<SecStaticCodeRef> result;

  if (auto url = cf::make_file_path_url(file_path, false)) {
    SecStaticCodeRef static_code;
    if (SecStaticCodeCreateWithPath(*url, kSecCSDefaultFlags, &static_code) == errSecSuccess) {
      result = static_code;

      CFRelease(static_code);
    }
  }

  return result;
}

inline OSStatus verify_code(SecCodeRef code, SecRequirementRef requirement) {
  return SecCodeCheckValidity(code, kSecCSStrictValidate, requirement);
}

inline OSStatus verify_code(SecStaticCodeRef code, SecRequirementRef requirement) {
  return SecStaticCodeCheckValidity(code, kSecCSStrictValidate, requirement);
}

struct code_signing_information final {
  cf::cf_ptr<CFDictionaryRef> information;
  anchor_type anchor = anchor_type::none;
};

template <typename CodeRef>
std::optional<code_signing_information> get_signing_information_of_code(CodeRef code) {
  code_signing_information result;

  // We need to check the narrower anchor_apple_requirement first.
  if (auto requirement = get_anchor_apple_requirement()) {
    if (verify_code(code, *requirement) == errSecSuccess) {
      result.anchor = anchor_type::apple;
    }
  }

  if (result.anchor == anchor_type::none) {
    if (auto requirement = get_anchor_apple_generic_requirement()) {
      if (verify_code(code, *requirement) == errSecSuccess) {
        result.anchor = anchor_type::apple_generic;
      }
    }
  }

  if (result.anchor == anchor_type::none) {
    return std::nullopt;
  }

  CFDictionaryRef information;
  if (SecCodeCopySigningInformation(code, kSecCSSigningInformation, &information) == errSecSuccess) {
    result.information = information;
    CFRelease(information);
    return result;
  }

  return std::nullopt;
}

inline std::optional<std::string> get_common_name_of_signing_information(CFDictionaryRef information) {
  std::optional<std::string> result;

  if (information) {
    if (auto certificates = static_cast<CFArrayRef>(CFDictionaryGetValue(information, kSecCodeInfoCertificates))) {
      if (CFArrayGetCount(certificates) > 0) {
        auto certificate = cf::get_cf_array_value<SecCertificateRef>(certificates, 0);
        CFStringRef common_name_string;
        if (SecCertificateCopyCommonName(certificate, &common_name_string) == errSecSuccess) {
          result = cf::make_string(common_name_string);

          CFRelease(common_name_string);
        }
      }
    }
  }

  return result;
}

inline signing_information get_signing_information_of_process(pid_t pid) {
  signing_information result;

  if (auto code = get_code_of_process(pid)) {
    if (auto information = get_signing_information_of_code(*code)) {
      result = signing_information(*information->information, information->anchor);
    }
  }

  return result;
}

inline signing_information get_signing_information_of_file(std::filesystem::path file_path) {
  signing_information result;

  if (auto code = get_code_of_file(file_path)) {
    if (auto information = get_signing_information_of_code(*code)) {
      result = signing_information(*information->information, information->anchor);
    }
  }

  return result;
}

inline std::optional<std::string> find_common_name_of_process(pid_t pid) {
  std::optional<std::string> result;

  if (auto code = get_code_of_process(pid)) {
    if (auto information = get_signing_information_of_code(*code)) {
      result = get_common_name_of_signing_information(*information->information);
    }
  }

  return result;
}

inline std::optional<std::string> find_common_name_of_file(std::filesystem::path file_path) {
  std::optional<std::string> result;

  if (auto code = get_code_of_file(file_path)) {
    if (auto information = get_signing_information_of_code(*code)) {
      result = get_common_name_of_signing_information(*information->information);
    }
  }

  return result;
}

} // namespace codesign
} // namespace osx
} // namespace pqrs
