#pragma once

#include <cstdint>
#include <pqrs/karabiner/driverkit/virtual_hid_device_driver.hpp>
#include <string_view>

namespace krbn {
enum class modifier_flag : uint32_t {
  zero,
  caps_lock,
  left_control,
  left_shift,
  left_option,
  left_command,
  right_control,
  right_shift,
  right_option,
  right_command,
  fn,
  end_,
};

inline std::optional<pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::modifier> make_hid_report_modifier(modifier_flag modifier_flag) {
  namespace hid_report = pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report;

  switch (modifier_flag) {
    case modifier_flag::left_control:
      return hid_report::modifier::left_control;
    case modifier_flag::left_shift:
      return hid_report::modifier::left_shift;
    case modifier_flag::left_option:
      return hid_report::modifier::left_option;
    case modifier_flag::left_command:
      return hid_report::modifier::left_command;
    case modifier_flag::right_control:
      return hid_report::modifier::right_control;
    case modifier_flag::right_shift:
      return hid_report::modifier::right_shift;
    case modifier_flag::right_option:
      return hid_report::modifier::right_option;
    case modifier_flag::right_command:
      return hid_report::modifier::right_command;
    default:
      return std::nullopt;
  }
}

inline std::optional<std::string_view> get_modifier_flag_name(modifier_flag value) {
  switch (value) {
    case modifier_flag::zero:
      return "zero";
    case modifier_flag::caps_lock:
      return "caps_lock";
    case modifier_flag::left_control:
      return "left_control";
    case modifier_flag::left_shift:
      return "left_shift";
    case modifier_flag::left_option:
      return "left_option";
    case modifier_flag::left_command:
      return "left_command";
    case modifier_flag::right_control:
      return "right_control";
    case modifier_flag::right_shift:
      return "right_shift";
    case modifier_flag::right_option:
      return "right_option";
    case modifier_flag::right_command:
      return "right_command";
    case modifier_flag::fn:
      return "fn";
    case modifier_flag::end_:
      return std::nullopt;
  }
}

inline void to_json(nlohmann::json& json, const modifier_flag& value) {
  if (auto name = get_modifier_flag_name(value)) {
    json = *name;
  }
}

inline void from_json(const nlohmann::json& json, modifier_flag& value) {
  using namespace std::string_literals;

  pqrs::json::requires_string(json, "json");

  auto name = json.get<std::string>();
  if (name == "zero") {
    value = modifier_flag::zero;
  } else if (name == "caps_lock") {
    value = modifier_flag::caps_lock;
  } else if (name == "left_control") {
    value = modifier_flag::left_control;
  } else if (name == "left_shift") {
    value = modifier_flag::left_shift;
  } else if (name == "left_option") {
    value = modifier_flag::left_option;
  } else if (name == "left_command") {
    value = modifier_flag::left_command;
  } else if (name == "right_control") {
    value = modifier_flag::right_control;
  } else if (name == "right_shift") {
    value = modifier_flag::right_shift;
  } else if (name == "right_option") {
    value = modifier_flag::right_option;
  } else if (name == "right_command") {
    value = modifier_flag::right_command;
  } else if (name == "fn") {
    value = modifier_flag::fn;
  } else {
    throw pqrs::json::unmarshal_error("unknown modifier_flag: `" + name + "`");
  }
}
} // namespace krbn
