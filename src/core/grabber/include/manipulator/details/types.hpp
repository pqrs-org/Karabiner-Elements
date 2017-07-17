#pragma once

#include "boost_defs.hpp"

#include "event_queue.hpp"
#include "modifier_flag_manager.hpp"
#include "stream_utility.hpp"
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <json/json.hpp>
#include <unordered_set>

namespace krbn {
namespace manipulator {
namespace details {

class event_definition {
public:
  enum class type {
    none,
    key_code,
    pointing_button,
    shell_command,
    set_variable,
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

  virtual ~event_definition(void) {
  }

  type get_type(void) const {
    return type_;
  }

  boost::optional<key_code> get_key_code(void) const {
    if (type_ == type::key_code) {
      return boost::get<key_code>(value_);
    }
    return boost::none;
  }

  boost::optional<pointing_button> get_pointing_button(void) const {
    if (type_ == type::pointing_button) {
      return boost::get<pointing_button>(value_);
    }
    return boost::none;
  }

  boost::optional<std::string> get_shell_command(void) const {
    if (type_ == type::shell_command) {
      return boost::get<std::string>(value_);
    }
    return boost::none;
  }

  boost::optional<std::pair<std::string, int>> get_set_variable(void) const {
    if (type_ == type::set_variable) {
      return boost::get<std::pair<std::string, int>>(value_);
    }
    return boost::none;
  }

  boost::optional<event_queue::queued_event::event> to_event(void) const {
    switch (type_) {
      case type::none:
        return boost::none;
      case type::key_code:
        return event_queue::queued_event::event(boost::get<key_code>(value_));
      case type::pointing_button:
        return event_queue::queued_event::event(boost::get<pointing_button>(value_));
      case type::shell_command:
        return event_queue::queued_event::event::make_shell_command_event(boost::get<std::string>(value_));
      case type::set_variable:
        return event_queue::queued_event::event::make_set_variable_event(boost::get<std::pair<std::string, int>>(value_));
    }
  }

  static std::unordered_set<modifier> make_modifiers(const nlohmann::json& json) {
    std::unordered_set<modifier> modifiers;

    for (const auto& j : json) {
      if (!j.is_string()) {
        logger::get_logger().error("complex_modifications json error: modifier should be string form: {0}", j.dump());

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
          logger::get_logger().error("complex_modifications json error: Unknown modifier: {0}", name);
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

protected:
  event_definition(void) : type_(type::none) {
  }

  event_definition(key_code key_code) : type_(type::key_code),
                                        value_(key_code) {
  }

  event_definition(pointing_button pointing_button) : type_(type::pointing_button),
                                                      value_(pointing_button) {
  }

  void handle_json(const nlohmann::json& json,
                   std::function<bool(const std::string&, const nlohmann::json&)> extra_json_handler) {
    if (!json.is_object()) {
      logger::get_logger().error("complex_modifications json error: Invalid form of event_definition: {0}", json.dump());
      return;
    }

    for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
      // it.key() is always std::string.
      const auto& key = it.key();
      const auto& value = it.value();

      // Set type_ and values.
      if (key == "key_code") {
        if (type_ != type::none) {
          logger::get_logger().error("complex_modifications json error: Duplicated type definition: {0}", json.dump());
          continue;
        }
        if (!value.is_string()) {
          logger::get_logger().error("complex_modifications json error: Invalid form of key_code: {0}", json.dump());
          continue;
        }

        if (auto key_code = types::get_key_code(value.get<std::string>())) {
          type_ = type::key_code;
          value_ = *key_code;
        }

      } else if (key == "pointing_button") {
        if (type_ != type::none) {
          logger::get_logger().error("complex_modifications json error: Duplicated type definition: {0}", json.dump());
          continue;
        }
        if (!value.is_string()) {
          logger::get_logger().error("complex_modifications json error: Invalid form of pointing_button: {0}", json.dump());
          continue;
        }

        if (auto pointing_button = types::get_pointing_button(value.get<std::string>())) {
          type_ = type::pointing_button;
          value_ = *pointing_button;
        }

      } else if (key == "shell_command") {
        if (type_ != type::none) {
          logger::get_logger().error("complex_modifications json error: Duplicated type definition: {0}", json.dump());
          continue;
        }
        if (!value.is_string()) {
          logger::get_logger().error("complex_modifications json error: Invalid form of shell_command: {0}", json.dump());
          continue;
        }

        type_ = type::shell_command;
        value_ = value.get<std::string>();

      } else if (key == "set_variable") {
        if (type_ != type::none) {
          logger::get_logger().error("complex_modifications json error: Duplicated type definition: {0}", json.dump());
          continue;
        }
        if (!value.is_object()) {
          logger::get_logger().error("complex_modifications json error: Invalid form of set_variable: {0}", json.dump());
          continue;
        }

        const std::string name_key = "name";
        const std::string value_key = "value";
        if (value.find(name_key) == std::end(value)) {
          logger::get_logger().error("complex_modifications json error: name is not found in set_variable: {0}", json.dump());
          continue;
        }
        if (!value[name_key].is_string()) {
          logger::get_logger().error("complex_modifications json error: Invalid form of set_variable.name: {0}", json.dump());
          continue;
        }
        if (value.find(value_key) == std::end(value)) {
          logger::get_logger().error("complex_modifications json error: value is not found in set_variable: {0}", json.dump());
          continue;
        }
        if (!value[value_key].is_number()) {
          logger::get_logger().error("complex_modifications json error: Invalid form of set_variable.value: {0}", json.dump());
          continue;
        }

        std::string variable_name = value[name_key];
        int variable_value = value[value_key];

        type_ = type::set_variable;
        value_ = std::make_pair(variable_name, variable_value);

      } else if (key == "description") {
        // Do nothing

      } else {
        if (!extra_json_handler(key, value)) {
          logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
        }
      }
    }
  }

  type type_;
  boost::variant<key_code,
                 pointing_button,
                 std::string,                // For shell_command
                 std::pair<std::string, int> // For set_variable
                 >
      value_;
}; // namespace details

class from_event_definition final : public event_definition {
public:
  from_event_definition(const nlohmann::json& json) {
    handle_json(json,
                [&](const std::string& key, const nlohmann::json& value) {
                  return extra_json_handler(key, value);
                });
  }

  from_event_definition(key_code key_code,
                        std::unordered_set<modifier> mandatory_modifiers,
                        std::unordered_set<modifier> optional_modifiers) : event_definition(key_code),
                                                                           mandatory_modifiers_(mandatory_modifiers),
                                                                           optional_modifiers_(optional_modifiers) {
  }

  virtual ~from_event_definition(void) {
  }

  const std::unordered_set<modifier>& get_mandatory_modifiers(void) const {
    return mandatory_modifiers_;
  }

  const std::unordered_set<modifier>& get_optional_modifiers(void) const {
    return optional_modifiers_;
  }

  boost::optional<std::unordered_set<modifier_flag>> test_modifiers(const modifier_flag_manager& modifier_flag_manager) const {
    std::unordered_set<modifier_flag> modifier_flags;

    // If mandatory_modifiers_ contains modifier::any, return all active modifier_flags.

    if (mandatory_modifiers_.find(modifier::any) != std::end(mandatory_modifiers_)) {
      for (auto i = static_cast<uint32_t>(modifier_flag::zero) + 1; i != static_cast<uint32_t>(modifier_flag::end_); ++i) {
        auto flag = modifier_flag(i);
        if (modifier_flag_manager.is_pressed(flag)) {
          modifier_flags.insert(flag);
        }
      }
      return modifier_flags;
    }

    // Check modifier_flag state.

    for (int i = 0; i < static_cast<int>(modifier::end_); ++i) {
      auto m = modifier(i);

      if (mandatory_modifiers_.find(m) != std::end(mandatory_modifiers_)) {
        auto pair = test_modifier(modifier_flag_manager, m);
        if (!pair.first) {
          return boost::none;
        }
        if (pair.second != modifier_flag::zero) {
          modifier_flags.insert(pair.second);
        }
      }
    }

    // If optional_modifiers_ does not contain modifier::any, we have to check modifier flags strictly.

    if (optional_modifiers_.find(modifier::any) == std::end(optional_modifiers_)) {
      std::unordered_set<modifier_flag> extra_modifier_flags;
      for (auto m = static_cast<uint32_t>(modifier_flag::zero) + 1; m != static_cast<uint32_t>(modifier_flag::end_); ++m) {
        extra_modifier_flags.insert(modifier_flag(m));
      }

      for (int i = 0; i < static_cast<int>(modifier::end_); ++i) {
        auto m = modifier(i);

        if (mandatory_modifiers_.find(m) != std::end(mandatory_modifiers_) ||
            optional_modifiers_.find(m) != std::end(optional_modifiers_)) {
          for (const auto& flag : get_modifier_flags(m)) {
            extra_modifier_flags.erase(flag);
          }
        }
      }

      for (const auto& flag : extra_modifier_flags) {
        if (modifier_flag_manager.is_pressed(flag)) {
          return boost::none;
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

private:
  bool extra_json_handler(const std::string& key,
                          const nlohmann::json& value) {
    if (key == "modifiers") {
      if (!value.is_object()) {
        logger::get_logger().error("complex_modifications json error: Invalid form of modifiers: {0}", value.dump());
        return true;
      }

      for (auto it = std::begin(value); it != std::end(value); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& k = it.key();
        const auto& v = it.value();

        if (k == "mandatory") {
          mandatory_modifiers_ = make_modifiers(v);
        } else if (k == "optional") {
          optional_modifiers_ = make_modifiers(v);
        } else {
          logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", k, value.dump());
        }
      }
      return true;
    }

    return false;
  }

  std::unordered_set<modifier> mandatory_modifiers_;
  std::unordered_set<modifier> optional_modifiers_;
};

class to_event_definition final : public event_definition {
public:
  to_event_definition(const nlohmann::json& json) {
    handle_json(json,
                [&](const std::string& key, const nlohmann::json& value) {
                  return extra_json_handler(key, value);
                });
  }

  to_event_definition(key_code key_code,
                      std::unordered_set<modifier> modifiers) : event_definition(key_code),
                                                                modifiers_(modifiers) {
  }

  virtual ~to_event_definition(void) {
  }

  const std::unordered_set<modifier>& get_modifiers(void) const {
    return modifiers_;
  }

private:
  bool extra_json_handler(const std::string& key,
                          const nlohmann::json& value) {
    if (key == "modifiers") {
      modifiers_ = make_modifiers(value);
      return true;
    }

    return false;
  }

  std::unordered_set<modifier> modifiers_;
};

inline std::ostream& operator<<(std::ostream& stream, const event_definition::modifier& value) {
#define KRBN_MANIPULATOR_DETAILS_MODIFIER_OUTPUT(MODIFIER) \
  case event_definition::modifier::MODIFIER:               \
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
inline std::ostream& operator<<(std::ostream& stream, const container<event_definition::modifier, std::allocator<event_definition::modifier>>& values) {
  return stream_utility::output_enums(stream, values);
}

template <template <class T, class H, class K, class A> class container>
inline std::ostream& operator<<(std::ostream& stream,
                                const container<event_definition::modifier,
                                                std::hash<event_definition::modifier>,
                                                std::equal_to<event_definition::modifier>,
                                                std::allocator<event_definition::modifier>>& values) {
  return stream_utility::output_enums(stream, values);
}
} // namespace details
} // namespace manipulator
} // namespace krbn
