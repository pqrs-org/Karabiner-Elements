#pragma once

#include "stream_utility.hpp"
#include "types/modifier_flag.hpp"
#include <unordered_set>

namespace krbn {
namespace manipulator {
namespace details {
class modifier_definition final {
public:
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

  static std::unordered_set<modifier> make_modifiers(const nlohmann::json& json) {
    std::unordered_set<modifier> modifiers;

    for (const auto& j : json) {
      if (!j.is_string()) {
        logger::get_logger()->error("complex_modifications json error: modifier should be string form: {0}", j.dump());

      } else {
        const std::string& name = j;
        if (name == "any") {
          modifiers.insert(modifier::any);
        } else if (name == "caps_lock") {
          modifiers.insert(modifier::caps_lock);
        } else if (name == "command") {
          modifiers.insert(modifier::command);
        } else if (name == "control") {
          modifiers.insert(modifier::control);
        } else if (name == "fn") {
          modifiers.insert(modifier::fn);
        } else if (name == "left_command") {
          modifiers.insert(modifier::left_command);
        } else if (name == "left_control") {
          modifiers.insert(modifier::left_control);
        } else if (name == "left_option") {
          modifiers.insert(modifier::left_option);
        } else if (name == "left_shift") {
          modifiers.insert(modifier::left_shift);
        } else if (name == "option") {
          modifiers.insert(modifier::option);
        } else if (name == "right_command") {
          modifiers.insert(modifier::right_command);
        } else if (name == "right_control") {
          modifiers.insert(modifier::right_control);
        } else if (name == "right_option") {
          modifiers.insert(modifier::right_option);
        } else if (name == "right_shift") {
          modifiers.insert(modifier::right_shift);
        } else if (name == "shift") {
          modifiers.insert(modifier::shift);
        } else {
          logger::get_logger()->error("complex_modifications json error: Unknown modifier: {0}", name);
        }
      }
    }

    return modifiers;
  }

  static std::vector<modifier_flag> get_modifier_flags(modifier modifier) {
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

  static modifier get_modifier(modifier_flag modifier_flag) {
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
};

inline std::ostream& operator<<(std::ostream& stream, const modifier_definition::modifier& value) {
#define KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(MODIFIER) \
  case modifier_definition::modifier::MODIFIER:            \
    stream << #MODIFIER;                                   \
    break;

  switch (value) {
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(any);
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(caps_lock);
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(command);
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(control);
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(fn);
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(left_command);
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(left_control);
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(left_option);
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(left_shift);
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(option);
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(right_command);
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(right_control);
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(right_option);
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(right_shift);
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(shift);
    KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(end_);
  }

#undef KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT

  return stream;
}

template <template <class T, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<modifier_definition::modifier, std::allocator<modifier_definition::modifier>>& values) {
  return stream_utility::output_enums(stream, values);
}

template <template <class T, class H, class K, class A> class container>
inline std::ostream& operator<<(std::ostream& stream,
                                const container<modifier_definition::modifier,
                                                std::hash<modifier_definition::modifier>,
                                                std::equal_to<modifier_definition::modifier>,
                                                std::allocator<modifier_definition::modifier>>& values) {
  return stream_utility::output_enums(stream, values);
}
} // namespace details
} // namespace manipulator
} // namespace krbn
