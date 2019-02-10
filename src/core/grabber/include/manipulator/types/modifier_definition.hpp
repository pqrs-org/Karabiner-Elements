#pragma once

#include "modifier_definition/modifier.hpp"
#include "types/json_unmarshal_error.hpp"
#include "types/modifier_flag.hpp"
#include <unordered_set>

namespace krbn {
namespace manipulator {
namespace modifier_definition {
inline std::unordered_set<modifier> make_modifiers(const nlohmann::json& json) {
  std::unordered_set<modifier> modifiers;

  for (const auto& j : json) {
    modifiers.insert(j.get<modifier>());
  }

  return modifiers;
}

inline std::vector<modifier_flag> get_modifier_flags(modifier modifier) {
  switch (modifier) {
    case modifier::any:
      return {};

    case modifier::caps_lock:
      return {modifier_flag::caps_lock};

    case modifier::command:
      return {modifier_flag::left_command, modifier_flag::right_command};

    case modifier::control:
      return {modifier_flag::left_control, modifier_flag::right_control};

    case modifier::fn:
      return {modifier_flag::fn};

    case modifier::left_command:
      return {modifier_flag::left_command};

    case modifier::left_control:
      return {modifier_flag::left_control};

    case modifier::left_option:
      return {modifier_flag::left_option};

    case modifier::left_shift:
      return {modifier_flag::left_shift};

    case modifier::option:
      return {modifier_flag::left_option, modifier_flag::right_option};

    case modifier::right_command:
      return {modifier_flag::right_command};

    case modifier::right_control:
      return {modifier_flag::right_control};

    case modifier::right_option:
      return {modifier_flag::right_option};

    case modifier::right_shift:
      return {modifier_flag::right_shift};

    case modifier::shift:
      return {modifier_flag::left_shift, modifier_flag::right_shift};

    case modifier::end_:
      return {};
  }
}

inline modifier get_modifier(modifier_flag modifier_flag) {
  switch (modifier_flag) {
    case modifier_flag::caps_lock:
      return modifier::caps_lock;

    case modifier_flag::fn:
      return modifier::fn;

    case modifier_flag::left_command:
      return modifier::left_command;

    case modifier_flag::left_control:
      return modifier::left_control;

    case modifier_flag::left_option:
      return modifier::left_option;

    case modifier_flag::left_shift:
      return modifier::left_shift;

    case modifier_flag::right_command:
      return modifier::right_command;

    case modifier_flag::right_control:
      return modifier::right_control;

    case modifier_flag::right_option:
      return modifier::right_option;

    case modifier_flag::right_shift:
      return modifier::right_shift;

    case modifier_flag::zero:
    case modifier_flag::end_:
      return modifier::end_;
  }
}
} // namespace modifier_definition
} // namespace manipulator
} // namespace krbn
