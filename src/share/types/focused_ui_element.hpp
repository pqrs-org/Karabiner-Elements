#pragma once

#include <optional>
#include <pqrs/json.hpp>
#include <string>

namespace krbn {
class focused_ui_element final {
public:
  focused_ui_element() {
  }

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

  const std::optional<std::string>& get_title() const {
    return title_;
  }

  focused_ui_element& set_title(const std::optional<std::string>& value) {
    title_ = value;
    return *this;
  }

  const std::optional<double>& get_window_position_x() const {
    return window_position_x_;
  }

  focused_ui_element& set_window_position_x(const std::optional<double>& value) {
    window_position_x_ = value;
    return *this;
  }

  const std::optional<double>& get_window_position_y() const {
    return window_position_y_;
  }

  focused_ui_element& set_window_position_y(const std::optional<double>& value) {
    window_position_y_ = value;
    return *this;
  }

  const std::optional<double>& get_window_size_width() const {
    return window_size_width_;
  }

  focused_ui_element& set_window_size_width(const std::optional<double>& value) {
    window_size_width_ = value;
    return *this;
  }

  const std::optional<double>& get_window_size_height() const {
    return window_size_height_;
  }

  focused_ui_element& set_window_size_height(const std::optional<double>& value) {
    window_size_height_ = value;
    return *this;
  }

  bool operator==(const focused_ui_element& other) const = default;

private:
  std::optional<std::string> role_;
  std::optional<std::string> subrole_;
  std::optional<std::string> title_;
  std::optional<double> window_position_x_;
  std::optional<double> window_position_y_;
  std::optional<double> window_size_width_;
  std::optional<double> window_size_height_;
};

inline void to_json(nlohmann::json& j, const focused_ui_element& s) {
  j = nlohmann::json::object();

  if (auto& v = s.get_role()) {
    j["role"] = *v;
  }

  if (auto& v = s.get_subrole()) {
    j["subrole"] = *v;
  }

  if (auto& v = s.get_title()) {
    j["title"] = *v;
  }

  if (auto& v = s.get_window_position_x()) {
    j["window_position_x"] = *v;
  }

  if (auto& v = s.get_window_position_y()) {
    j["window_position_y"] = *v;
  }

  if (auto& v = s.get_window_size_width()) {
    j["window_size_width"] = *v;
  }

  if (auto& v = s.get_window_size_height()) {
    j["window_size_height"] = *v;
  }
}

inline void from_json(const nlohmann::json& j, focused_ui_element& s) {
  using namespace std::string_literals;

  pqrs::json::requires_object(j, "json");

  for (const auto& [key, value] : j.items()) {
    if (key == "role") {
      pqrs::json::requires_string(value, "`"s + key + "`");

      s.set_role(value.get<std::string>());

    } else if (key == "subrole") {
      pqrs::json::requires_string(value, "`"s + key + "`");

      s.set_subrole(value.get<std::string>());

    } else if (key == "title") {
      pqrs::json::requires_string(value, "`"s + key + "`");

      s.set_title(value.get<std::string>());

    } else if (key == "window_position_x") {
      pqrs::json::requires_number(value, "`"s + key + "`");

      s.set_window_position_x(value.get<double>());

    } else if (key == "window_position_y") {
      pqrs::json::requires_number(value, "`"s + key + "`");

      s.set_window_position_y(value.get<double>());

    } else if (key == "window_size_width") {
      pqrs::json::requires_number(value, "`"s + key + "`");

      s.set_window_size_width(value.get<double>());

    } else if (key == "window_size_height") {
      pqrs::json::requires_number(value, "`"s + key + "`");

      s.set_window_size_height(value.get<double>());

    } else {
      throw pqrs::json::unmarshal_error("unknown key: `"s + key + "`"s);
    }
  }
}
} // namespace krbn
