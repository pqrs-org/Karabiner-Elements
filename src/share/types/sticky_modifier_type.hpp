#pragma once

#include <nlohmann/json.hpp>

namespace krbn {
enum class sticky_modifier_type {
  on,
  off,
  toggle,
};

inline void to_json(nlohmann::json& json, const sticky_modifier_type& value) {
  switch (value) {
    case sticky_modifier_type::on:
      json = "on";
      break;
    case sticky_modifier_type::off:
      json = "off";
      break;
    case sticky_modifier_type::toggle:
      json = "toggle";
      break;
  }
}

inline void from_json(const nlohmann::json& json, sticky_modifier_type& value) {
  pqrs::json::requires_string(json, "json");

  auto s = json.get<std::string>();

  if (s == "on") {
    value = sticky_modifier_type::on;
  } else if (s == "off") {
    value = sticky_modifier_type::off;
  } else if (s == "toggle") {
    value = sticky_modifier_type::toggle;
  } else {
    throw pqrs::json::unmarshal_error(fmt::format("unknown value: `{0}`", s));
  }
}
} // namespace krbn
