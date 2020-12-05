#pragma once

#include "unnamed.hpp"
#include <mapbox/eternal.hpp>

namespace krbn {
namespace momentary_switch_event_details {
namespace apple_vendor_keyboard_key_code {
constexpr std::pair<const mapbox::eternal::string, const pqrs::hid::usage::value_t> name_value_pairs[] = {
    {"spotlight", pqrs::hid::usage::apple_vendor_keyboard::spotlight},
    {"dashboard", pqrs::hid::usage::apple_vendor_keyboard::dashboard},
    {"function", pqrs::hid::usage::apple_vendor_keyboard::function},
    {"launchpad", pqrs::hid::usage::apple_vendor_keyboard::launchpad},
    // pqrs::hid::usage::apple_vendor_keyboard::reserved
    // pqrs::hid::usage::apple_vendor_keyboard::caps_lock_delay_enable
    // pqrs::hid::usage::apple_vendor_keyboard::power_state
    {"expose_all", pqrs::hid::usage::apple_vendor_keyboard::expose_all},
    {"expose_desktop", pqrs::hid::usage::apple_vendor_keyboard::expose_desktop},
    {"brightness_up", pqrs::hid::usage::apple_vendor_keyboard::brightness_up},
    {"brightness_down", pqrs::hid::usage::apple_vendor_keyboard::brightness_down},
    {"language", pqrs::hid::usage::apple_vendor_keyboard::language},
};

constexpr auto name_value_map = mapbox::eternal::hash_map<mapbox::eternal::string, pqrs::hid::usage::value_t>(name_value_pairs);

inline auto find_pair(pqrs::hid::usage::value_t usage) {
  return std::find_if(std::begin(name_value_pairs),
                      std::end(name_value_pairs),
                      [&](const auto& pair) {
                        return pair.second == usage;
                      });
}

inline bool target(pqrs::hid::usage::value_t usage) {
  return find_pair(usage) != std::end(name_value_pairs);
}

inline std::string make_name(pqrs::hid::usage::value_t usage) {
  auto it = find_pair(usage);
  if (it != std::end(name_value_pairs)) {
    return it->first.c_str();
  }

  // fallback
  return unnamed::make_name(usage);
}

inline std::optional<pqrs::hid::usage::value_t> find_usage(const std::string& name) {
  auto it = name_value_map.find(name.c_str());
  if (it != name_value_map.end()) {
    return it->second;
  }

  // fallback
  return unnamed::find_usage(name);
}
} // namespace apple_vendor_keyboard_key_code
} // namespace momentary_switch_event_details
} // namespace krbn
