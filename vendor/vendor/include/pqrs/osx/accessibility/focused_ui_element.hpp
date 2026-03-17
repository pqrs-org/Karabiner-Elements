#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <optional>
#include <string>

namespace pqrs {
namespace osx {
namespace accessibility {
class focused_ui_element final {
public:
  const std::optional<std::string>& get_role() const {
    return role_;
  }

  focused_ui_element& set_role(const std::optional<std::string>& value) {
    role_ = value;
    return *this;
  }

  const std::optional<std::string>& get_subrole() const {
    return subrole_;
  }

  focused_ui_element& set_subrole(const std::optional<std::string>& value) {
    subrole_ = value;
    return *this;
  }

  const std::optional<std::string>& get_role_description() const {
    return role_description_;
  }

  focused_ui_element& set_role_description(const std::optional<std::string>& value) {
    role_description_ = value;
    return *this;
  }

  const std::optional<std::string>& get_title() const {
    return title_;
  }

  focused_ui_element& set_title(const std::optional<std::string>& value) {
    title_ = value;
    return *this;
  }

  const std::optional<std::string>& get_description() const {
    return description_;
  }

  focused_ui_element& set_description(const std::optional<std::string>& value) {
    description_ = value;
    return *this;
  }

  const std::optional<std::string>& get_identifier() const {
    return identifier_;
  }

  focused_ui_element& set_identifier(const std::optional<std::string>& value) {
    identifier_ = value;
    return *this;
  }

  bool operator==(const focused_ui_element& other) const = default;

private:
  std::optional<std::string> role_;
  std::optional<std::string> subrole_;
  std::optional<std::string> role_description_;
  std::optional<std::string> title_;
  std::optional<std::string> description_;
  std::optional<std::string> identifier_;
};
} // namespace accessibility
} // namespace osx
} // namespace pqrs
