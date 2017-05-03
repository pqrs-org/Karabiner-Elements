#pragma once

#include "boost_defs.hpp"

#include "event_queue.hpp"
#include <boost/optional.hpp>
#include <json/json.hpp>
#include <unordered_set>

namespace krbn {
namespace manipulator {
namespace details {

class event_definition final {
public:
  enum class type {
    none,
    key_code,
    pointing_button,
  };

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
  };

  event_definition(const nlohmann::json& json) : type_(type::none) {
    // Set type_ and values.
    do {
      {
        const std::string key = "key_code";
        if (json.find(key) != std::end(json) && json[key].is_string()) {
          if (auto key_code = types::get_key_code(json[key])) {
            type_ = type::key_code;
            key_code_ = *key_code;

            // if key_code is modifier, push it into modifiers_.
            switch (types::get_modifier_flag(*key_code)) {
              case modifier_flag::caps_lock:
                modifiers_.insert(modifier::caps_lock);
                break;
              case modifier_flag::fn:
                modifiers_.insert(modifier::fn);
                break;
              case modifier_flag::left_command:
                modifiers_.insert(modifier::left_command);
                break;
              case modifier_flag::left_control:
                modifiers_.insert(modifier::left_control);
                break;
              case modifier_flag::left_option:
                modifiers_.insert(modifier::left_option);
                break;
              case modifier_flag::left_shift:
                modifiers_.insert(modifier::left_shift);
                break;
              case modifier_flag::right_command:
                modifiers_.insert(modifier::right_command);
                break;
              case modifier_flag::right_control:
                modifiers_.insert(modifier::right_control);
                break;
              case modifier_flag::right_option:
                modifiers_.insert(modifier::right_option);
                break;
              case modifier_flag::right_shift:
                modifiers_.insert(modifier::right_shift);
                break;
              case modifier_flag::zero:
              case modifier_flag::end_:
                break;
            }
          }
        }
      }
      {
        const std::string key = "pointing_button";
        if (json.find(key) != std::end(json) && json[key].is_string()) {
          if (auto pointing_button = types::get_pointing_button(json[key])) {
            type_ = type::pointing_button;
            pointing_button_ = *pointing_button;
            break;
          }
        }
      }
    } while (false);

    // Set modifiers_.
    {
      const std::string key = "modifiers";
      if (json.find(key) != std::end(json) && json[key].is_array()) {
        for (const auto& j : json[key]) {
          if (j.is_string()) {
            if (j == "any") { modifiers_.insert(modifier::any); }
            if (j == "command") { modifiers_.insert(modifier::command); }
            if (j == "control") { modifiers_.insert(modifier::control); }
            if (j == "fn") { modifiers_.insert(modifier::fn); }
            if (j == "left_command") { modifiers_.insert(modifier::left_command); }
            if (j == "left_control") { modifiers_.insert(modifier::left_control); }
            if (j == "left_option") { modifiers_.insert(modifier::left_option); }
            if (j == "left_shift") { modifiers_.insert(modifier::left_shift); }
            if (j == "option") { modifiers_.insert(modifier::option); }
            if (j == "right_command") { modifiers_.insert(modifier::right_command); }
            if (j == "right_control") { modifiers_.insert(modifier::right_control); }
            if (j == "right_option") { modifiers_.insert(modifier::right_option); }
            if (j == "right_shift") { modifiers_.insert(modifier::right_shift); }
            if (j == "shift") { modifiers_.insert(modifier::shift); }
          }
        }
      }
    }
  }

  event_definition(key_code key_code) : type_(type::key_code),
                                        key_code_(key_code) {
  }

  event_definition(pointing_button pointing_button) : type_(type::pointing_button),
                                                      pointing_button_(pointing_button) {
  }

  type get_type(void) const {
    return type_;
  }

  boost::optional<key_code> get_key_code(void) const {
    if (type_ == type::key_code) {
      return key_code_;
    }
    return boost::none;
  }

  boost::optional<pointing_button> get_pointing_button(void) const {
    if (type_ == type::pointing_button) {
      return pointing_button_;
    }
    return boost::none;
  }

  const std::unordered_set<modifier>& get_modifiers(void) const {
    return modifiers_;
  }

  boost::optional<event_queue::queued_event::event> to_event(void) const {
    switch (type_) {
      case type::none:
        return boost::none;
      case type::key_code:
        return event_queue::queued_event::event(key_code_);
      case type::pointing_button:
        return event_queue::queued_event::event(pointing_button_);
    }
  }

private:
  type type_;
  union {
    key_code key_code_;
    pointing_button pointing_button_;
  };
  std::unordered_set<modifier> modifiers_;
};
} // namespace details
} // namespace manipulator
} // namespace krbn
