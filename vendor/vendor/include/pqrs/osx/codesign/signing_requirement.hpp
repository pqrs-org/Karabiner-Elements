#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "anchor_type.hpp"
#include "team_id.hpp"
#include <expected>
#include <optional>
#include <string>

namespace pqrs::osx::codesign {

struct signing_requirement_parameters final {
  std::optional<anchor_type> anchor;
  std::optional<pqrs::osx::codesign::team_id> team_id;
  std::optional<std::string> identifier;
  std::optional<std::string> certificate_common_name;
  std::optional<std::string> leaf_certificate_sha1;
  std::optional<std::string> cdhash;
};

enum class signing_requirement_error {
  missing_anchor,
  invalid_anchor,
  empty_string,
  invalid_string_character,
  invalid_hash,
};

namespace detail {
inline std::expected<std::string, signing_requirement_error> quote_requirement_string(const std::string &value) {
  if (value.empty()) {
    return std::unexpected(signing_requirement_error::empty_string);
  }

  std::string result = "\"";

  for (unsigned char c : value) {
    if (c < 0x20 || c == 0x7f || c == '\\') {
      return std::unexpected(signing_requirement_error::invalid_string_character);
    }

    if (c == '"') {
      result += '\\';
    }

    result += static_cast<char>(c);
  }

  return result + '"';
}

inline std::expected<std::string, signing_requirement_error> quote_requirement_hash(const std::string &value) {
  if (value.size() != 40 ||
      value.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
    return std::unexpected(signing_requirement_error::invalid_hash);
  }

  return "H\"" + value + '"';
}
} // namespace detail

// The trust anchor is required. Other explicitly supplied conditions are
// included. Invalid inputs are returned as errors.
[[nodiscard]] inline std::expected<std::string, signing_requirement_error>
make_signing_requirement(const signing_requirement_parameters &parameters) {
  std::string result;

  auto append = [&](const std::string &condition) {
    if (!result.empty()) {
      result += " and ";
    }
    result += condition;
  };

  if (!parameters.anchor) {
    return std::unexpected(signing_requirement_error::missing_anchor);
  }

  switch (*parameters.anchor) {
    case anchor_type::apple:
      append("anchor apple");
      break;

    case anchor_type::apple_generic:
      append("anchor apple generic");
      break;

    default:
      return std::unexpected(signing_requirement_error::invalid_anchor);
  }

  if (parameters.team_id) {
    auto quoted = detail::quote_requirement_string(type_safe::get(*parameters.team_id));
    if (!quoted) {
      return std::unexpected(quoted.error());
    }

    append("certificate leaf[subject.OU] = " + *quoted);
  }

  if (parameters.identifier) {
    auto quoted = detail::quote_requirement_string(*parameters.identifier);
    if (!quoted) {
      return std::unexpected(quoted.error());
    }

    append("identifier " + *quoted);
  }

  if (parameters.certificate_common_name) {
    auto quoted = detail::quote_requirement_string(*parameters.certificate_common_name);
    if (!quoted) {
      return std::unexpected(quoted.error());
    }

    append("certificate leaf[subject.CN] = " + *quoted);
  }

  if (parameters.leaf_certificate_sha1) {
    auto quoted = detail::quote_requirement_hash(*parameters.leaf_certificate_sha1);
    if (!quoted) {
      return std::unexpected(quoted.error());
    }

    append("certificate leaf = " + *quoted);
  }

  if (parameters.cdhash) {
    auto quoted = detail::quote_requirement_hash(*parameters.cdhash);
    if (!quoted) {
      return std::unexpected(quoted.error());
    }

    append("cdhash " + *quoted);
  }

  return result;
}

} // namespace pqrs::osx::codesign
