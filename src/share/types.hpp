#pragma once

#include "Karabiner-VirtualHIDDevice/dist/include/karabiner_virtual_hid_device_methods.hpp"
#include "apple_hid_usage_tables.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "stream_utility.hpp"
#include "types/absolute_time_duration.hpp"
#include "types/absolute_time_point.hpp"
#include "types/consumer_key_code.hpp"
#include "types/device_id.hpp"
#include "types/device_identifiers.hpp"
#include "types/device_state.hpp"
#include "types/event_type.hpp"
#include "types/grabbable_state.hpp"
#include "types/hid_country_code.hpp"
#include "types/hid_usage.hpp"
#include "types/hid_usage_page.hpp"
#include "types/hid_value.hpp"
#include "types/key_code.hpp"
#include "types/key_down_up_valued_event.hpp"
#include "types/led_state.hpp"
#include "types/location_id.hpp"
#include "types/modifier_flag.hpp"
#include "types/mouse_key.hpp"
#include "types/operation_type.hpp"
#include "types/pointing_button.hpp"
#include "types/pointing_motion.hpp"
#include "types/product_id.hpp"
#include "types/vendor_id.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <pqrs/osx/frontmost_application_monitor.hpp>
#include <pqrs/osx/frontmost_application_monitor/extra/nlohmann_json.hpp>
#include <pqrs/osx/input_source.hpp>
#include <pqrs/osx/input_source/extra/nlohmann_json.hpp>
#include <pqrs/osx/input_source_selector.hpp>
#include <pqrs/osx/input_source_selector/extra/nlohmann_json.hpp>
#include <string>
#include <thread>
#include <type_safe/strong_typedef.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace krbn {
namespace types {
inline std::optional<hid_usage_page> make_hid_usage_page(key_code key_code) {
  switch (key_code) {
    case key_code::fn:
    case key_code::illumination_decrement:
    case key_code::illumination_increment:
    case key_code::apple_top_case_display_brightness_decrement:
    case key_code::apple_top_case_display_brightness_increment:
      return hid_usage_page::apple_vendor_top_case;

    case key_code::dashboard:
    case key_code::launchpad:
    case key_code::mission_control:
    case key_code::apple_display_brightness_decrement:
    case key_code::apple_display_brightness_increment:
      return hid_usage_page::apple_vendor_keyboard;

    case key_code::mute:
    case key_code::volume_decrement:
    case key_code::volume_increment:
    case key_code::display_brightness_decrement:
    case key_code::display_brightness_increment:
    case key_code::rewind:
    case key_code::play_or_pause:
    case key_code::fastforward:
    case key_code::eject:
      return hid_usage_page::consumer;

    case key_code::vk_none:
      return std::nullopt;

    default:
      return hid_usage_page::keyboard_or_keypad;
  }
}

inline std::optional<hid_usage> make_hid_usage(key_code key_code) {
  switch (key_code) {
    case key_code::fn:
      return hid_usage::av_top_case_keyboard_fn;

    case key_code::illumination_decrement:
      return hid_usage::av_top_case_illumination_down;

    case key_code::illumination_increment:
      return hid_usage::av_top_case_illumination_up;

    case key_code::apple_top_case_display_brightness_decrement:
      return hid_usage::av_top_case_brightness_down;

    case key_code::apple_top_case_display_brightness_increment:
      return hid_usage::av_top_case_brightness_up;

    case key_code::dashboard:
      return hid_usage::apple_vendor_keyboard_dashboard;

    case key_code::launchpad:
      return hid_usage::apple_vendor_keyboard_launchpad;

    case key_code::mission_control:
      return hid_usage::apple_vendor_keyboard_expose_all;

    case key_code::apple_display_brightness_decrement:
      return hid_usage::apple_vendor_keyboard_brightness_down;

    case key_code::apple_display_brightness_increment:
      return hid_usage::apple_vendor_keyboard_brightness_up;

    case key_code::mute:
      return hid_usage::csmr_mute;

    case key_code::volume_decrement:
      return hid_usage::csmr_volume_decrement;

    case key_code::volume_increment:
      return hid_usage::csmr_volume_increment;

    case key_code::display_brightness_decrement:
      return hid_usage::csmr_display_brightness_decrement;

    case key_code::display_brightness_increment:
      return hid_usage::csmr_display_brightness_increment;

    case key_code::rewind:
      return hid_usage::csmr_rewind;

    case key_code::play_or_pause:
      return hid_usage::csmr_play_or_pause;

    case key_code::fastforward:
      return hid_usage::csmr_fastforward;

    case key_code::eject:
      return hid_usage::csmr_eject;

    case key_code::vk_none:
      return std::nullopt;

    default:
      return hid_usage(key_code);
  }
}

inline std::optional<pqrs::karabiner_virtual_hid_device::hid_report::modifier> make_hid_report_modifier(modifier_flag modifier_flag) {
  switch (modifier_flag) {
    case modifier_flag::left_control:
      return pqrs::karabiner_virtual_hid_device::hid_report::modifier::left_control;
    case modifier_flag::left_shift:
      return pqrs::karabiner_virtual_hid_device::hid_report::modifier::left_shift;
    case modifier_flag::left_option:
      return pqrs::karabiner_virtual_hid_device::hid_report::modifier::left_option;
    case modifier_flag::left_command:
      return pqrs::karabiner_virtual_hid_device::hid_report::modifier::left_command;
    case modifier_flag::right_control:
      return pqrs::karabiner_virtual_hid_device::hid_report::modifier::right_control;
    case modifier_flag::right_shift:
      return pqrs::karabiner_virtual_hid_device::hid_report::modifier::right_shift;
    case modifier_flag::right_option:
      return pqrs::karabiner_virtual_hid_device::hid_report::modifier::right_option;
    case modifier_flag::right_command:
      return pqrs::karabiner_virtual_hid_device::hid_report::modifier::right_command;
    default:
      return std::nullopt;
  }
}

inline const std::vector<std::pair<std::string, consumer_key_code>>& get_consumer_key_code_name_value_pairs(void) {
  static std::mutex mutex;
  std::lock_guard<std::mutex> guard(mutex);

  static std::vector<std::pair<std::string, consumer_key_code>> pairs({
      {"power", consumer_key_code::power},
      {"display_brightness_increment", consumer_key_code::display_brightness_increment},
      {"display_brightness_decrement", consumer_key_code::display_brightness_decrement},
      {"fastforward", consumer_key_code::fastforward},
      {"rewind", consumer_key_code::rewind},
      {"scan_next_track", consumer_key_code::scan_next_track},
      {"scan_previous_track", consumer_key_code::scan_previous_track},
      {"eject", consumer_key_code::eject},
      {"play_or_pause", consumer_key_code::play_or_pause},
      {"mute", consumer_key_code::mute},
      {"volume_increment", consumer_key_code::volume_increment},
      {"volume_decrement", consumer_key_code::volume_decrement},
  });

  return pairs;
}

inline const std::unordered_map<std::string, consumer_key_code>& get_consumer_key_code_name_value_map(void) {
  static std::mutex mutex;
  std::lock_guard<std::mutex> guard(mutex);

  static std::unordered_map<std::string, consumer_key_code> map;

  if (map.empty()) {
    for (const auto& pair : get_consumer_key_code_name_value_pairs()) {
      auto it = map.find(pair.first);
      if (it != std::end(map)) {
        logger::get_logger()->error("duplicate entry in get_consumer_key_code_name_value_pairs: {0}", pair.first);
      } else {
        map.emplace(pair.first, pair.second);
      }
    }
  }

  return map;
}

inline std::optional<consumer_key_code> make_consumer_key_code(const std::string& name) {
  auto& map = get_consumer_key_code_name_value_map();
  auto it = map.find(name);
  if (it == map.end()) {
    return std::nullopt;
  }
  return it->second;
}

inline std::string make_consumer_key_code_name(consumer_key_code consumer_key_code) {
  for (const auto& pair : get_consumer_key_code_name_value_pairs()) {
    if (pair.second == consumer_key_code) {
      return pair.first;
    }
  }
  return fmt::format("(number:{0})", static_cast<uint32_t>(consumer_key_code));
}

inline std::optional<consumer_key_code> make_consumer_key_code(hid_usage_page usage_page, hid_usage usage) {
  auto u = static_cast<uint32_t>(usage);

  switch (usage_page) {
    case hid_usage_page::consumer:
      switch (consumer_key_code(u)) {
        case consumer_key_code::power:
        case consumer_key_code::display_brightness_increment:
        case consumer_key_code::display_brightness_decrement:
        case consumer_key_code::fastforward:
        case consumer_key_code::rewind:
        case consumer_key_code::scan_next_track:
        case consumer_key_code::scan_previous_track:
        case consumer_key_code::eject:
        case consumer_key_code::play_or_pause:
        case consumer_key_code::mute:
        case consumer_key_code::volume_increment:
        case consumer_key_code::volume_decrement:
          return consumer_key_code(u);
      }

    default:
      break;
  }

  return std::nullopt;
}

inline std::optional<consumer_key_code> make_consumer_key_code(const hid_value& hid_value) {
  if (auto hid_usage_page = hid_value.get_hid_usage_page()) {
    if (auto hid_usage = hid_value.get_hid_usage()) {
      return make_consumer_key_code(*hid_usage_page,
                                    *hid_usage);
    }
  }
  return std::nullopt;
}

inline std::optional<hid_usage_page> make_hid_usage_page(consumer_key_code consumer_key_code) {
  return hid_usage_page::consumer;
}

inline std::optional<hid_usage> make_hid_usage(consumer_key_code consumer_key_code) {
  return hid_usage(static_cast<uint32_t>(consumer_key_code));
}

inline const std::vector<std::pair<std::string, pointing_button>>& get_pointing_button_name_value_pairs(void) {
  static std::mutex mutex;
  std::lock_guard<std::mutex> guard(mutex);

  static std::vector<std::pair<std::string, pointing_button>> pairs({
      // From IOHIDUsageTables.h

      {"button1", pointing_button::button1},
      {"button2", pointing_button::button2},
      {"button3", pointing_button::button3},
      {"button4", pointing_button::button4},
      {"button5", pointing_button::button5},
      {"button6", pointing_button::button6},
      {"button7", pointing_button::button7},
      {"button8", pointing_button::button8},

      {"button9", pointing_button::button9},
      {"button10", pointing_button::button10},
      {"button11", pointing_button::button11},
      {"button12", pointing_button::button12},
      {"button13", pointing_button::button13},
      {"button14", pointing_button::button14},
      {"button15", pointing_button::button15},
      {"button16", pointing_button::button16},

      {"button17", pointing_button::button17},
      {"button18", pointing_button::button18},
      {"button19", pointing_button::button19},
      {"button20", pointing_button::button20},
      {"button21", pointing_button::button21},
      {"button22", pointing_button::button22},
      {"button23", pointing_button::button23},
      {"button24", pointing_button::button24},

      {"button25", pointing_button::button25},
      {"button26", pointing_button::button26},
      {"button27", pointing_button::button27},
      {"button28", pointing_button::button28},
      {"button29", pointing_button::button29},
      {"button30", pointing_button::button30},
      {"button31", pointing_button::button31},
      {"button32", pointing_button::button32},
  });

  return pairs;
}

inline const std::unordered_map<std::string, pointing_button>& get_pointing_button_name_value_map(void) {
  static std::mutex mutex;
  std::lock_guard<std::mutex> guard(mutex);

  static std::unordered_map<std::string, pointing_button> map;

  if (map.empty()) {
    for (const auto& pair : get_pointing_button_name_value_pairs()) {
      auto it = map.find(pair.first);
      if (it != std::end(map)) {
        logger::get_logger()->error("duplicate entry in get_pointing_button_name_value_pairs: {0}", pair.first);
      } else {
        map.emplace(pair.first, pair.second);
      }
    }
  }

  return map;
}

inline std::optional<pointing_button> make_pointing_button(const std::string& name) {
  auto& map = get_pointing_button_name_value_map();
  auto it = map.find(name);
  if (it == map.end()) {
    return std::nullopt;
  }
  return it->second;
}

inline std::string make_pointing_button_name(pointing_button pointing_button) {
  for (const auto& pair : get_pointing_button_name_value_pairs()) {
    if (pair.second == pointing_button) {
      return pair.first;
    }
  }
  return fmt::format("(number:{0})", static_cast<uint32_t>(pointing_button));
}

inline std::optional<pointing_button> make_pointing_button(hid_usage_page usage_page, hid_usage usage) {
  if (usage_page == hid_usage_page::button) {
    return pointing_button(usage);
  }
  return std::nullopt;
}

inline std::optional<pointing_button> make_pointing_button(const hid_value& hid_value) {
  if (auto hid_usage_page = hid_value.get_hid_usage_page()) {
    if (auto hid_usage = hid_value.get_hid_usage()) {
      return make_pointing_button(*hid_usage_page,
                                  *hid_usage);
    }
  }
  return std::nullopt;
}

inline std::string to_string(const key_down_up_valued_event& event) {
  if (auto value = event.find<key_code>()) {
    auto json = nlohmann::json::object({
        {"key_code", make_key_code_name(*value)},
    });
    return json.dump();

  } else if (auto value = event.find<consumer_key_code>()) {
    auto json = nlohmann::json::object({
        {"consumer_key_code", make_consumer_key_code_name(*value)},
    });
    return json.dump();

  } else if (auto value = event.find<pointing_button>()) {
    auto json = nlohmann::json::object({
        {"pointing_button", make_pointing_button_name(*value)},
    });
    return json.dump();
  }

  return "";
}
} // namespace types

inline void from_json(const nlohmann::json& json, key_code& value) {
  if (json.is_string()) {
    if (auto v = make_key_code(json.get<std::string>())) {
      value = *v;
    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown key_code: `{0}`", json.dump()));
    }
  } else if (json.is_number()) {
    value = key_code(json.get<uint32_t>());
  } else {
    throw pqrs::json::unmarshal_error(fmt::format("json must be string or number, but is `{0}`", json.dump()));
  }
}

inline void from_json(const nlohmann::json& json, consumer_key_code& value) {
  if (json.is_string()) {
    if (auto v = types::make_consumer_key_code(json.get<std::string>())) {
      value = *v;
    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown consumer_key_code: `{0}`", json.dump()));
    }
  } else if (json.is_number()) {
    value = consumer_key_code(json.get<uint32_t>());
  } else {
    throw pqrs::json::unmarshal_error(fmt::format("json must be string or number, but is `{0}`", json.dump()));
  }
}

inline void from_json(const nlohmann::json& json, pointing_button& value) {
  if (json.is_string()) {
    if (auto v = types::make_pointing_button(json.get<std::string>())) {
      value = *v;
    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown pointing_button: `{0}`", json.dump()));
    }
  } else if (json.is_number()) {
    value = pointing_button(json.get<uint32_t>());
  } else {
    throw pqrs::json::unmarshal_error(fmt::format("json must be string or number, but is `{0}`", json.dump()));
  }
}
} // namespace krbn
