#pragma once

// pqrs::osx::codesign v3.1.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "codesign/anchor_type.hpp"
#include "codesign/team_id.hpp"
#include <Security/CodeSigning.h>
#include <filesystem>
#include <optional>
#include <pqrs/cf/array.hpp>
#include <pqrs/cf/cf_ptr.hpp>
#include <pqrs/cf/dictionary.hpp>
#include <pqrs/cf/number.hpp>
#include <pqrs/cf/string.hpp>
#include <pqrs/cf/url.hpp>
#include <string>

namespace pqrs::osx::codesign {

class signing_information final {
public:
  signing_information() {
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

  [[nodiscard]] anchor_type get_verified_anchor_type() const noexcept {
    return verified_anchor_type_;
  }

  [[nodiscard]] const std::optional<team_id>& get_verified_team_id() const noexcept {
    return verified_team_id_;
  }

  [[nodiscard]] const std::optional<std::string>& get_identifier() const noexcept {
    return identifier_;
  }

private:
  anchor_type verified_anchor_type_{anchor_type::none};
  std::optional<team_id> verified_team_id_;
  std::optional<std::string> identifier_;
};

[[nodiscard]] inline cf::cf_ptr<SecRequirementRef> get_anchor_apple_requirement() {
  cf::cf_ptr<SecRequirementRef> result;

  SecRequirementRef req = nullptr;
  auto status = SecRequirementCreateWithString(CFSTR("anchor apple"),
                                               kSecCSDefaultFlags,
                                               &req);
  if (status == errSecSuccess) {
    result = cf::adopt_cf_ptr(req);
  }

  return result;
}

[[nodiscard]] inline cf::cf_ptr<SecRequirementRef> get_anchor_apple_generic_requirement() {
  cf::cf_ptr<SecRequirementRef> result;

  SecRequirementRef req = nullptr;
  auto status = SecRequirementCreateWithString(CFSTR("anchor apple generic"),
                                               kSecCSDefaultFlags,
                                               &req);
  if (status == errSecSuccess) {
    result = cf::adopt_cf_ptr(req);
  }

  return result;
}

[[nodiscard]] inline cf::cf_ptr<SecCodeRef> get_code_of_process(pid_t pid) {
  cf::cf_ptr<SecCodeRef> result;

  if (auto attributes = cf::make_cf_mutable_dictionary()) {
    if (auto pid_number = cf::make_cf_number(static_cast<int64_t>(pid))) {
      CFDictionarySetValue(*attributes, kSecGuestAttributePid, *pid_number);

      SecCodeRef guest;
      if (SecCodeCopyGuestWithAttributes(nullptr, *attributes, kSecCSDefaultFlags, &guest) == errSecSuccess) {
        result = cf::adopt_cf_ptr(guest);
      }
    }
  }

  return result;
}

[[nodiscard]] inline cf::cf_ptr<SecStaticCodeRef> get_code_of_file(const std::filesystem::path& file_path) {
  cf::cf_ptr<SecStaticCodeRef> result;

  if (auto url = cf::make_file_path_url(file_path, false)) {
    SecStaticCodeRef static_code;
    if (SecStaticCodeCreateWithPath(*url, kSecCSDefaultFlags, &static_code) == errSecSuccess) {
      result = cf::adopt_cf_ptr(static_code);
    }
  }

  return result;
}

[[nodiscard]] inline OSStatus verify_code(SecCodeRef code, SecRequirementRef requirement) noexcept {
  return SecCodeCheckValidity(code, kSecCSStrictValidate, requirement);
}

[[nodiscard]] inline OSStatus verify_code(SecStaticCodeRef code, SecRequirementRef requirement) noexcept {
  return SecStaticCodeCheckValidity(code, kSecCSStrictValidate, requirement);
}

struct code_signing_information final {
  cf::cf_ptr<CFDictionaryRef> information;
  anchor_type anchor = anchor_type::none;
};

template <typename CodeRef>
[[nodiscard]] std::optional<code_signing_information> get_signing_information_of_code(CodeRef code) {
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
    result.information = cf::adopt_cf_ptr(information);
    return result;
  }

  return std::nullopt;
}

[[nodiscard]] inline std::optional<std::string> get_common_name_of_signing_information(CFDictionaryRef information) {
  std::optional<std::string> result;

  if (information) {
    if (auto certificates = static_cast<CFArrayRef>(CFDictionaryGetValue(information, kSecCodeInfoCertificates))) {
      if (CFArrayGetCount(certificates) > 0) {
        auto certificate = cf::get_cf_array_value<SecCertificateRef>(certificates, 0);
        CFStringRef common_name_string;
        if (SecCertificateCopyCommonName(certificate, &common_name_string) == errSecSuccess) {
          auto common_name = cf::adopt_cf_ptr(common_name_string);
          result = cf::make_string(*common_name);
        }
      }
    }
  }

  return result;
}

[[nodiscard]] inline signing_information get_signing_information_of_process(pid_t pid) {
  signing_information result;

  if (auto code = get_code_of_process(pid)) {
    if (auto information = get_signing_information_of_code(*code)) {
      result = signing_information(*information->information, information->anchor);
    }
  }

  return result;
}

[[nodiscard]] inline signing_information get_signing_information_of_file(const std::filesystem::path& file_path) {
  signing_information result;

  if (auto code = get_code_of_file(file_path)) {
    if (auto information = get_signing_information_of_code(*code)) {
      result = signing_information(*information->information, information->anchor);
    }
  }

  return result;
}

[[nodiscard]] inline std::optional<std::string> find_common_name_of_process(pid_t pid) {
  std::optional<std::string> result;

  if (auto code = get_code_of_process(pid)) {
    if (auto information = get_signing_information_of_code(*code)) {
      result = get_common_name_of_signing_information(*information->information);
    }
  }

  return result;
}

[[nodiscard]] inline std::optional<std::string> find_common_name_of_file(const std::filesystem::path& file_path) {
  std::optional<std::string> result;

  if (auto code = get_code_of_file(file_path)) {
    if (auto information = get_signing_information_of_code(*code)) {
      result = get_common_name_of_signing_information(*information->information);
    }
  }

  return result;
}

} // namespace pqrs::osx::codesign
