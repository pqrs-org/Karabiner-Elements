#pragma once

#include "impl.hpp"
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

inline bool target(pqrs::hid::usage::value_t usage) {
  return impl::find_pair(name_value_pairs, usage) != std::end(name_value_pairs);
}

inline std::string make_name(pqrs::hid::usage::value_t usage) {
  return impl::make_name(name_value_pairs, usage);
}

inline pqrs::hid::usage_pair make_usage_pair(const std::string& key,
                                             const nlohmann::json& json) {
  return impl::make_usage_pair(name_value_map,
                               pqrs::hid::usage_page::apple_vendor_keyboard,
                               key,
                               json);
}
} // namespace apple_vendor_keyboard_key_code
} // namespace momentary_switch_event_details
} // namespace krbn
