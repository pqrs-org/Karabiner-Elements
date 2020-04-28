#pragma once

#include "modifier_definition/modifier.hpp"
#include "types/modifier_flag.hpp"
#include <pqrs/json.hpp>
#include <set>

namespace krbn {
namespace manipulator {
namespace modifier_definition {
// Note:
// We use `std::set` instead of `std::unordered_set` in order to use modifiers with to_event_definition.
inline std::set<modifier> make_modifiers(const nlohmann::json& json) {
  if (json.is_string()) {
    return std::set<modifier>{
        json.get<modifier>(),
    };
  }

  if (json.is_array()) {
    return json.get<std::set<modifier>>();
  }

  throw pqrs::json::unmarshal_error(fmt::format("json must be array or string, but is `{0}`", pqrs::json::dump_for_error_message(json)));
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
