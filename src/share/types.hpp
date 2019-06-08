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
    if (auto v = make_consumer_key_code(json.get<std::string>())) {
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
    if (auto v = make_pointing_button(json.get<std::string>())) {
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
