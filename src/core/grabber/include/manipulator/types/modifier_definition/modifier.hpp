#pragma once

#include "types/json_unmarshal_error.hpp"
#include <nlohmann/json.hpp>
#include <unordered_set>

namespace krbn {
namespace manipulator {
namespace modifier_definition {
enum class modifier {
  any,
  caps_lock,
  command,
  control,
  fn,
  left_command,
  left_control,
  left_option,
  left_shift,
  option,
  right_command,
  right_control,
  right_option,
  right_shift,
  shift,
  end_,
};

inline void to_json(nlohmann::json& json, const modifier& value) {
  switch (value) {
    case modifier::any:
      json = "any";
      break;
    case modifier::caps_lock:
      json = "caps_lock";
      break;
    case modifier::command:
      json = "command";
      break;
    case modifier::control:
      json = "control";
      break;
    case modifier::fn:
      json = "fn";
      break;
    case modifier::left_command:
      json = "left_command";
      break;
    case modifier::left_control:
      json = "left_control";
      break;
    case modifier::left_option:
      json = "left_option";
      break;
    case modifier::left_shift:
      json = "left_shift";
      break;
    case modifier::option:
      json = "option";
      break;
    case modifier::right_command:
      json = "right_command";
      break;
    case modifier::right_control:
      json = "right_control";
      break;
    case modifier::right_option:
      json = "right_option";
      break;
    case modifier::right_shift:
      json = "right_shift";
      break;
    case modifier::shift:
      json = "shift";
      break;
    case modifier::end_:
      json = "end_";
      break;
    default:
      json = nullptr;
  }
}

inline void from_json(const nlohmann::json& json, modifier& value) {
  if (!json.is_string()) {
    throw json_unmarshal_error(fmt::format("modifier must be string, but is `{0}`", json.dump()));
  }

  auto name = json.get<std::string>();
  if (name == "any") {
    value = modifier::any;
  } else if (name == "caps_lock") {
    value = modifier::caps_lock;
  } else if (name == "command") {
    value = modifier::command;
  } else if (name == "control") {
    value = modifier::control;
  } else if (name == "fn") {
    value = modifier::fn;
  } else if (name == "left_command") {
    value = modifier::left_command;
  } else if (name == "left_control") {
    value = modifier::left_control;
  } else if (name == "left_option") {
    value = modifier::left_option;
  } else if (name == "left_shift") {
    value = modifier::left_shift;
  } else if (name == "option") {
    value = modifier::option;
  } else if (name == "right_command") {
    value = modifier::right_command;
  } else if (name == "right_control") {
    value = modifier::right_control;
  } else if (name == "right_option") {
    value = modifier::right_option;
  } else if (name == "right_shift") {
    value = modifier::right_shift;
  } else if (name == "shift") {
    value = modifier::shift;
  } else if (name == "end_") {
    value = modifier::end_;
  } else {
    throw json_unmarshal_error(fmt::format("unknown modifier: `{0}`", name));
  }
}

inline std::ostream& operator<<(std::ostream& stream, const modifier& value) {
  return stream << nlohmann::json(value);
}

template <template <class T, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<modifier, std::allocator<modifier>>& values) {
  return stream << nlohmann::json(values);
}

template <template <class T, class H, class K, class A> class container>
inline std::ostream& operator<<(std::ostream& stream,
                                const container<modifier,
                                                std::hash<modifier>,
                                                std::equal_to<modifier>,
                                                std::allocator<modifier>>& values) {
  return stream << nlohmann::json(values);
}
} // namespace modifier_definition
} // namespace manipulator
} // namespace krbn
