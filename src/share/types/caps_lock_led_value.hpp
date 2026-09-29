#pragma once

#include <nlohmann/json.hpp>
#include <pqrs/json.hpp>
#include <spdlog/fmt/fmt.h>

namespace krbn {
enum class caps_lock_led_value {
  on,
  off,
  // Stop overriding the caps lock LED so that it follows the caps lock state again.
  automatic,
};

inline void to_json(nlohmann::json& json, const caps_lock_led_value& value) {
  switch (value) {
    case caps_lock_led_value::on:
      json = "on";
      break;
    case caps_lock_led_value::off:
      json = "off";
      break;
    case caps_lock_led_value::automatic:
      json = "auto";
      break;
  }
}

inline void from_json(const nlohmann::json& json, caps_lock_led_value& value) {
  pqrs::json::requires_string(json, "json");

  auto s = json.get<std::string>();

  if (s == "on") {
    value = caps_lock_led_value::on;
  } else if (s == "off") {
    value = caps_lock_led_value::off;
  } else if (s == "auto") {
    value = caps_lock_led_value::automatic;
  } else {
    throw pqrs::json::unmarshal_error(fmt::format("unknown value: `{0}`", s));
  }
}
} // namespace krbn
