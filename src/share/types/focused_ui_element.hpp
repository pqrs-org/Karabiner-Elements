#pragma once

#include <optional>
#include <pqrs/json.hpp>
#include <string>

namespace krbn {
class focused_ui_element final {
public:
  focused_ui_element(void) {
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

  bool operator==(const focused_ui_element& other) const = default;

private:
  std::optional<std::string> role_;
  std::optional<std::string> subrole_;
  std::optional<std::string> title_;
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

    } else {
      throw pqrs::json::unmarshal_error("unknown key: `"s + key + "`"s);
    }
  }
}
} // namespace krbn
