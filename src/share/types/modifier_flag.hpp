#pragma once

#include <cstdint>

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

inline void from_json(const nlohmann::json& json, modifier_flag& value) {
  if (!json.is_string()) {
    throw pqrs::json::unmarshal_error(fmt::format("json must be string, but is `{0}`", json.dump()));
  }

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
    throw pqrs::json::unmarshal_error(fmt::format("unknown modifier_flag: `{0}`", name));
  }
}
} // namespace krbn
