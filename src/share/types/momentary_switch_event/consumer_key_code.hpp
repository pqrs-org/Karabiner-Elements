#pragma once

#include "impl.hpp"
#include <mapbox/eternal.hpp>

namespace krbn {
namespace momentary_switch_event_details {
namespace consumer_key_code {
constexpr std::pair<const mapbox::eternal::string, const pqrs::hid::usage::value_t> name_value_pairs[] = {
    {"power", pqrs::hid::usage::consumer::power},
    {"display_brightness_increment", pqrs::hid::usage::consumer::display_brightness_increment},
    {"display_brightness_decrement", pqrs::hid::usage::consumer::display_brightness_decrement},
    {"fast_forward", pqrs::hid::usage::consumer::fast_forward},
    {"rewind", pqrs::hid::usage::consumer::rewind},
    {"scan_next_track", pqrs::hid::usage::consumer::scan_next_track},
    {"scan_previous_track", pqrs::hid::usage::consumer::scan_previous_track},
    {"eject", pqrs::hid::usage::consumer::eject},
    {"play_or_pause", pqrs::hid::usage::consumer::play_or_pause},
    {"mute", pqrs::hid::usage::consumer::mute},
    {"volume_increment", pqrs::hid::usage::consumer::volume_increment},
    {"volume_decrement", pqrs::hid::usage::consumer::volume_decrement},

    // Aliases
    {"fastforward", pqrs::hid::usage::consumer::fast_forward},
};

constexpr auto name_value_map = mapbox::eternal::hash_map<mapbox::eternal::string, pqrs::hid::usage::value_t>(name_value_pairs);

inline bool target(pqrs::hid::usage::value_t usage) {
  return impl::find_pair(name_value_pairs, usage) != std::end(name_value_pairs);
}

inline std::string make_name(pqrs::hid::usage::value_t usage) {
  return impl::make_name(name_value_pairs, usage);
}

inline std::optional<pqrs::hid::usage::value_t> find_usage(const std::string& name) {
  return impl::find_usage(name_value_map, name);
}

inline pqrs::hid::usage_pair make_usage_pair(const std::string& key,
                                             const nlohmann::json& json) {
  return impl::make_usage_pair(name_value_map,
                               pqrs::hid::usage_page::consumer,
                               key,
                               json);
}
} // namespace consumer_key_code
} // namespace momentary_switch_event_details
} // namespace krbn
