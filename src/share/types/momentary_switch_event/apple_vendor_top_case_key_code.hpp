#pragma once

#include <cstdint>
#include <mapbox/eternal.hpp>
#include <pqrs/osx/iokit_hid_value.hpp>
#include <spdlog/fmt/fmt.h>

namespace krbn {
namespace momentary_switch_event_details {
namespace apple_vendor_top_case_key_code {
constexpr std::pair<const mapbox::eternal::string, const pqrs::hid::usage::value_t> name_value_pairs[] = {
    {"keyboard_fn", pqrs::hid::usage::apple_vendor_top_case::keyboard_fn},
    {"brightness_up", pqrs::hid::usage::apple_vendor_top_case::brightness_up},
    {"brightness_down", pqrs::hid::usage::apple_vendor_top_case::brightness_down},
    {"video_mirror", pqrs::hid::usage::apple_vendor_top_case::video_mirror},
    {"illumination_toggle", pqrs::hid::usage::apple_vendor_top_case::illumination_toggle},
    {"illumination_up", pqrs::hid::usage::apple_vendor_top_case::illumination_up},
    {"illumination_down", pqrs::hid::usage::apple_vendor_top_case::illumination_down},
    // pqrs::hid::usage::apple_vendor_top_case::clamshell_latched
    // pqrs::hid::usage::apple_vendor_top_case::reserved_mouse_data
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
                               pqrs::hid::usage_page::apple_vendor_top_case,
                               key,
                               json);
}
} // namespace apple_vendor_top_case_key_code
} // namespace momentary_switch_event_details
} // namespace krbn
