#pragma once

#include "impl.hpp"
#include <mapbox/eternal.hpp>

namespace krbn {
namespace momentary_switch_event_details {
namespace generic_desktop {
constexpr std::pair<const mapbox::eternal::string, const pqrs::hid::usage::value_t> name_value_pairs[] = {
    {"system_sleep", pqrs::hid::usage::generic_desktop::system_sleep},
    {"dpad_up", pqrs::hid::usage::generic_desktop::dpad_up},
    {"dpad_down", pqrs::hid::usage::generic_desktop::dpad_down},
    {"dpad_right", pqrs::hid::usage::generic_desktop::dpad_right},
    {"dpad_left", pqrs::hid::usage::generic_desktop::dpad_left},
};

constexpr auto name_value_map = mapbox::eternal::hash_map<mapbox::eternal::string, pqrs::hid::usage::value_t>(name_value_pairs);

inline bool target(pqrs::hid::usage_page::value_t usage_page,
                   pqrs::hid::usage::value_t usage) {
  if (usage_page == pqrs::hid::usage_page::generic_desktop) {
    return impl::find_pair(name_value_pairs, usage) != std::end(name_value_pairs);
  }

  return false;
}

inline std::string make_name(pqrs::hid::usage::value_t usage) {
  return impl::make_name(name_value_pairs, usage);
}

inline pqrs::hid::usage_pair make_usage_pair(const std::string& key,
                                             const nlohmann::json& json) {
  return impl::make_usage_pair(name_value_map,
                               pqrs::hid::usage_page::generic_desktop,
                               key,
                               json);
}
} // namespace generic_desktop
} // namespace momentary_switch_event_details
} // namespace krbn
