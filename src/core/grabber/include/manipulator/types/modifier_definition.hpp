#pragma once

#include "types/json_unmarshal_error.hpp"
#include "types/modifier_flag.hpp"
#include <unordered_set>

namespace krbn {
namespace manipulator {
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
      try {
        modifiers.insert(j.get<modifier>());
      } catch (const json_unmarshal_error& e) {
        logger::get_logger()->error(e.what());
      } catch (...) {}
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

inline void to_json(nlohmann::json& json, const modifier_definition::modifier& value) {
  switch (value) {
    case modifier_definition::modifier::any:
      json = "any";
      break;
    case modifier_definition::modifier::caps_lock:
      json = "caps_lock";
      break;
    case modifier_definition::modifier::command:
      json = "command";
      break;
    case modifier_definition::modifier::control:
      json = "control";
      break;
    case modifier_definition::modifier::fn:
      json = "fn";
      break;
    case modifier_definition::modifier::left_command:
      json = "left_command";
      break;
    case modifier_definition::modifier::left_control:
      json = "left_control";
      break;
    case modifier_definition::modifier::left_option:
      json = "left_option";
      break;
    case modifier_definition::modifier::left_shift:
      json = "left_shift";
      break;
    case modifier_definition::modifier::option:
      json = "option";
      break;
    case modifier_definition::modifier::right_command:
      json = "right_command";
      break;
    case modifier_definition::modifier::right_control:
      json = "right_control";
      break;
    case modifier_definition::modifier::right_option:
      json = "right_option";
      break;
    case modifier_definition::modifier::right_shift:
      json = "right_shift";
      break;
    case modifier_definition::modifier::shift:
      json = "shift";
      break;
    case modifier_definition::modifier::end_:
      json = "end_";
      break;
    default:
      json = nullptr;
  }
}

inline void from_json(const nlohmann::json& json, modifier_definition::modifier& value) {
  if (!json.is_string()) {
    throw json_unmarshal_error(fmt::format("complex_modifications json error: modifier should be string form: {0}", json.dump()));
  }

  auto name = json.get<std::string>();
  if (name == "any") {
    value = modifier_definition::modifier::any;
  } else if (name == "caps_lock") {
    value = modifier_definition::modifier::caps_lock;
  } else if (name == "command") {
    value = modifier_definition::modifier::command;
  } else if (name == "control") {
    value = modifier_definition::modifier::control;
  } else if (name == "fn") {
    value = modifier_definition::modifier::fn;
  } else if (name == "left_command") {
    value = modifier_definition::modifier::left_command;
  } else if (name == "left_control") {
    value = modifier_definition::modifier::left_control;
  } else if (name == "left_option") {
    value = modifier_definition::modifier::left_option;
  } else if (name == "left_shift") {
    value = modifier_definition::modifier::left_shift;
  } else if (name == "option") {
    value = modifier_definition::modifier::option;
  } else if (name == "right_command") {
    value = modifier_definition::modifier::right_command;
  } else if (name == "right_control") {
    value = modifier_definition::modifier::right_control;
  } else if (name == "right_option") {
    value = modifier_definition::modifier::right_option;
  } else if (name == "right_shift") {
    value = modifier_definition::modifier::right_shift;
  } else if (name == "shift") {
    value = modifier_definition::modifier::shift;
  } else if (name == "end_") {
    value = modifier_definition::modifier::end_;
  } else {
    throw json_unmarshal_error(fmt::format("complex_modifications json error: unknown modifier: {0}", name));
  }
}

inline std::ostream& operator<<(std::ostream& stream, const modifier_definition::modifier& value) {
  return stream << nlohmann::json(value);
}

template <template <class T, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<modifier_definition::modifier, std::allocator<modifier_definition::modifier>>& values) {
  return stream << nlohmann::json(values);
}

template <template <class T, class H, class K, class A> class container>
inline std::ostream& operator<<(std::ostream& stream,
                                const container<modifier_definition::modifier,
                                                std::hash<modifier_definition::modifier>,
                                                std::equal_to<modifier_definition::modifier>,
                                                std::allocator<modifier_definition::modifier>>& values) {
  return stream << nlohmann::json(values);
}
} // namespace manipulator
} // namespace krbn
