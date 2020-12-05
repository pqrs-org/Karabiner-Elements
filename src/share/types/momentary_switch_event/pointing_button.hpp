#pragma once

#include "impl.hpp"
#include <mapbox/eternal.hpp>

namespace krbn {
namespace momentary_switch_event_details {
namespace pointing_button {
constexpr std::pair<const mapbox::eternal::string, const pqrs::hid::usage::value_t> name_value_pairs[] = {
    {"button1", pqrs::hid::usage::button::button_1},
    {"button2", pqrs::hid::usage::button::button_2},
    {"button3", pqrs::hid::usage::button::button_3},
    {"button4", pqrs::hid::usage::button::button_4},
    {"button5", pqrs::hid::usage::button::button_5},
    {"button6", pqrs::hid::usage::button::button_6},
    {"button7", pqrs::hid::usage::button::button_7},
    {"button8", pqrs::hid::usage::button::button_8},
    {"button9", pqrs::hid::usage::button::button_9},
    {"button10", pqrs::hid::usage::button::button_10},
    {"button11", pqrs::hid::usage::button::button_11},
    {"button12", pqrs::hid::usage::button::button_12},
    {"button13", pqrs::hid::usage::button::button_13},
    {"button14", pqrs::hid::usage::button::button_14},
    {"button15", pqrs::hid::usage::button::button_15},
    {"button16", pqrs::hid::usage::button::button_16},
    {"button17", pqrs::hid::usage::button::button_17},
    {"button18", pqrs::hid::usage::button::button_18},
    {"button19", pqrs::hid::usage::button::button_19},
    {"button20", pqrs::hid::usage::button::button_20},
    {"button21", pqrs::hid::usage::button::button_21},
    {"button22", pqrs::hid::usage::button::button_22},
    {"button23", pqrs::hid::usage::button::button_23},
    {"button24", pqrs::hid::usage::button::button_24},
    {"button25", pqrs::hid::usage::button::button_25},
    {"button26", pqrs::hid::usage::button::button_26},
    {"button27", pqrs::hid::usage::button::button_27},
    {"button28", pqrs::hid::usage::button::button_28},
    {"button29", pqrs::hid::usage::button::button_29},
    {"button30", pqrs::hid::usage::button::button_30},
    {"button31", pqrs::hid::usage::button::button_31},
    {"button32", pqrs::hid::usage::button::button_32},
};

constexpr auto name_value_map = mapbox::eternal::hash_map<mapbox::eternal::string, pqrs::hid::usage::value_t>(name_value_pairs);

inline bool target(pqrs::hid::usage::value_t usage) {
  return true;
}

inline std::string make_name(pqrs::hid::usage::value_t usage) {
  return impl::make_name(name_value_pairs, usage);
}

inline std::optional<pqrs::hid::usage::value_t> find_usage(const std::string& name) {
  return impl::find_usage(name_value_map, name);
}
} // namespace pointing_button
} // namespace momentary_switch_event_details
} // namespace krbn
