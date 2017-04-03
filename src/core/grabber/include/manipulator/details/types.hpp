#pragma once

#include <json/json.hpp>

namespace krbn {
namespace manipulator {
namespace detail {

class event_definition final {
public:
  enum class type {
    none,
    key,
    pointing_button,
  };

  enum class modifier {
    any,
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
  };

  event_definition(const nlohmann::json& json) : type_(type::none) {
    // Set type_ and value_.
    do {
      {
        const std::string key = "key";
        if (json.find(key) != std::end(json) && json[key].is_string()) {
          type_ = type::key;
          value_ = json[key];
          break;
        }
      }
      {
        const std::string key = "pointing_button";
        if (json.find(key) != std::end(json) && json[key].is_string()) {
          type_ = type::pointing_button;
          value_ = json[key];
          break;
        }
      }
    } while (false);

    // Set modifiers_.
    {
      const std::string key = "modifiers";
      if (json.find(key) != std::end(json) && json[key].is_array()) {
        for (const auto& j : json[key]) {
          if (j.is_string()) {
            if (j == "any") { modifiers_.push_back(modifier::any); }
            if (j == "command") { modifiers_.push_back(modifier::command); }
            if (j == "control") { modifiers_.push_back(modifier::control); }
            if (j == "fn") { modifiers_.push_back(modifier::fn); }
            if (j == "left_command") { modifiers_.push_back(modifier::left_command); }
            if (j == "left_control") { modifiers_.push_back(modifier::left_control); }
            if (j == "left_option") { modifiers_.push_back(modifier::left_option); }
            if (j == "left_shift") { modifiers_.push_back(modifier::left_shift); }
            if (j == "option") { modifiers_.push_back(modifier::option); }
            if (j == "right_command") { modifiers_.push_back(modifier::right_command); }
            if (j == "right_control") { modifiers_.push_back(modifier::right_control); }
            if (j == "right_option") { modifiers_.push_back(modifier::right_option); }
            if (j == "right_shift") { modifiers_.push_back(modifier::right_shift); }
            if (j == "shift") { modifiers_.push_back(modifier::shift); }
          }
        }
      }
    }
  }

  type get_type(void) const {
    return type_;
  }

  const std::string& get_value(void) const {
    return value_;
  }

  const std::vector<modifier>& get_modifiers(void) const {
    return modifiers_;
  }

private:
  type type_;
  std::string value_;
  std::vector<modifier> modifiers_;
};
} // namespace detail
} // namespace manipulator
} // namespace krbn
