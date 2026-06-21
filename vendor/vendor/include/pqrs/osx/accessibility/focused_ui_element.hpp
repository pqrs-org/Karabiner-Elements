#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <optional>
#include <string>

namespace pqrs::osx::accessibility {
class focused_ui_element final {
public:
  [[nodiscard]] const std::optional<std::string>& get_role() const noexcept {
    return role_;
  }

  focused_ui_element& set_role(const std::optional<std::string>& value) {
    role_ = value;
    return *this;
  }

  [[nodiscard]] const std::optional<std::string>& get_subrole() const noexcept {
    return subrole_;
  }

  focused_ui_element& set_subrole(const std::optional<std::string>& value) {
    subrole_ = value;
    return *this;
  }

  [[nodiscard]] const std::optional<std::string>& get_role_description() const noexcept {
    return role_description_;
  }

  focused_ui_element& set_role_description(const std::optional<std::string>& value) {
    role_description_ = value;
    return *this;
  }

  [[nodiscard]] const std::optional<std::string>& get_title() const noexcept {
    return title_;
  }

  focused_ui_element& set_title(const std::optional<std::string>& value) {
    title_ = value;
    return *this;
  }

  [[nodiscard]] const std::optional<std::string>& get_description() const noexcept {
    return description_;
  }

  focused_ui_element& set_description(const std::optional<std::string>& value) {
    description_ = value;
    return *this;
  }

  [[nodiscard]] const std::optional<std::string>& get_identifier() const noexcept {
    return identifier_;
  }

  focused_ui_element& set_identifier(const std::optional<std::string>& value) {
    identifier_ = value;
    return *this;
  }

  [[nodiscard]] const std::optional<double>& get_window_position_x() const noexcept {
    return window_position_x_;
  }

  focused_ui_element& set_window_position_x(const std::optional<double>& value) {
    window_position_x_ = value;
    return *this;
  }

  [[nodiscard]] const std::optional<double>& get_window_position_y() const noexcept {
    return window_position_y_;
  }

  focused_ui_element& set_window_position_y(const std::optional<double>& value) {
    window_position_y_ = value;
    return *this;
  }

  [[nodiscard]] const std::optional<double>& get_window_size_width() const noexcept {
    return window_size_width_;
  }

  focused_ui_element& set_window_size_width(const std::optional<double>& value) {
    window_size_width_ = value;
    return *this;
  }

  [[nodiscard]] const std::optional<double>& get_window_size_height() const noexcept {
    return window_size_height_;
  }

  focused_ui_element& set_window_size_height(const std::optional<double>& value) {
    window_size_height_ = value;
    return *this;
  }

  [[nodiscard]] bool operator==(const focused_ui_element& other) const = default;

private:
  std::optional<std::string> role_;
  std::optional<std::string> subrole_;
  std::optional<std::string> role_description_;
  std::optional<std::string> title_;
  std::optional<std::string> description_;
  std::optional<std::string> identifier_;
  std::optional<double> window_position_x_;
  std::optional<double> window_position_y_;
  std::optional<double> window_size_width_;
  std::optional<double> window_size_height_;
};
} // namespace pqrs::osx::accessibility
