#pragma once

#include "boost_defs.hpp"

#include "event_queue.hpp"
#include "modifier_flag_manager.hpp"
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
    end_,
  };

  event_definition(const nlohmann::json& json) : type_(type::none) {
    // Set type_ and values.
    do {
      {
        const std::string key = "key_code";
        if (json.find(key) != std::end(json) && json[key].is_string()) {
          const std::string& name = json[key];
          if (auto key_code = types::get_key_code(name)) {
            type_ = type::key_code;
            key_code_ = *key_code;
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

  event_definition(key_code key_code,
                   const std::unordered_set<modifier> modifiers) : type_(type::key_code),
                                                                   key_code_(key_code),
                                                                   modifiers_(modifiers) {
  }

  event_definition(key_code key_code) : event_definition(key_code, std::unordered_set<modifier>()) {
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

  boost::optional<std::unordered_set<modifier_flag>> test_modifiers(const modifier_flag_manager& modifier_flag_manager) const {
    bool has_any = (modifiers_.find(modifier::any) != std::end(modifiers_));
    bool has_command = (modifiers_.find(modifier::command) != std::end(modifiers_));
    bool has_control = (modifiers_.find(modifier::control) != std::end(modifiers_));
    bool has_option = (modifiers_.find(modifier::option) != std::end(modifiers_));
    bool has_shift = (modifiers_.find(modifier::shift) != std::end(modifiers_));

    std::unordered_set<modifier_flag> modifier_flags;

    for (int i = 0; i < static_cast<int>(modifier::end_); ++i) {
      auto m = modifier(i);

      if (m == modifier::any) {
        continue;
      }

      if (modifiers_.find(m) != std::end(modifiers_)) {
        auto pair = test_modifier(modifier_flag_manager, m);
        if (!pair.first) {
          return boost::none;
        }
        if (pair.second != modifier_flag::zero) {
          modifier_flags.insert(pair.second);
        }

      } else {
        // m is not a member of modifiers_.
        if (!has_any) {
          // Strict matching

          if (m == modifier::command ||
              m == modifier::control ||
              m == modifier::option ||
              m == modifier::shift ||
              (has_command && (m == modifier::left_command || m == modifier::right_command)) ||
              (has_control && (m == modifier::left_control || m == modifier::right_control)) ||
              (has_option && (m == modifier::left_option || m == modifier::right_option)) ||
              (has_shift && (m == modifier::left_shift || m == modifier::right_shift))) {
            continue;
          }

          auto pair = test_modifier(modifier_flag_manager, m);
          if (pair.first) {
            return boost::none;
          }
        }
      }
    }

    return modifier_flags;
  }

  static std::pair<bool, modifier_flag> test_modifier(const modifier_flag_manager& modifier_flag_manager,
                                                      modifier modifier) {
    if (modifier == modifier::any) {
      return std::make_pair(true, modifier_flag::zero);
    }

    auto modifier_flags = get_modifier_flags(modifier);
    if (!modifier_flags.empty()) {
      for (const auto& m : modifier_flags) {
        if (modifier_flag_manager.is_pressed(m)) {
          return std::make_pair(true, m);
        }
      }
    }

    return std::make_pair(false, modifier_flag::zero);
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
