#pragma once

#include "boost_defs.hpp"

#include "json_utility.hpp"
#include "manipulator_environment.hpp"
#include "types.hpp"
#include <boost/variant.hpp>

namespace krbn {
namespace event_queue {
class event {
public:
  enum class type {
    none,
    key_code,
    consumer_key_code,
    pointing_button,
    pointing_motion,
    // virtual events
    shell_command,
    select_input_source,
    set_variable,
    mouse_key,
    stop_keyboard_repeat,
    // virtual events (passive)
    device_keys_and_pointing_buttons_are_released,
    device_ungrabbed,
    caps_lock_state_changed,
    pointing_device_event_from_event_tap,
    frontmost_application_changed,
    input_source_changed,
    keyboard_type_changed,
  };

  event(void) : type_(type::none),
                value_(boost::blank()) {
  }

  static event make_from_json(const nlohmann::json& json) {
    event result;

    if (auto v = json_utility::find_optional<std::string>(json, "type")) {
      result.type_ = to_type(*v);
    }

    switch (result.type_) {
      case type::none:
        break;

      case type::key_code:
        if (auto s = json_utility::find_optional<std::string>(json, "key_code")) {
          if (auto v = types::make_key_code(*s)) {
            result.value_ = *v;
          }
        }
        break;

      case type::consumer_key_code:
        if (auto s = json_utility::find_optional<std::string>(json, "consumer_key_code")) {
          if (auto v = types::make_consumer_key_code(*s)) {
            result.value_ = *v;
          }
        }
        break;

      case type::pointing_button:
        if (auto s = json_utility::find_optional<std::string>(json, "pointing_button")) {
          if (auto v = types::make_pointing_button(*s)) {
            result.value_ = *v;
          }
        }
        break;

      case type::pointing_motion:
        if (auto v = json_utility::find_object(json, "pointing_motion")) {
          result.value_ = pointing_motion(*v);
        }
        break;

      case type::caps_lock_state_changed:
        if (auto v = json_utility::find_optional<int>(json, "integer_value")) {
          result.value_ = *v;
        }
        break;

      case type::shell_command:
        if (auto v = json_utility::find_optional<std::string>(json, "shell_command")) {
          result.value_ = *v;
        }
        break;

      case type::select_input_source:
        if (auto v = json_utility::find_array(json, "input_source_selectors")) {
          std::vector<input_source_selector> input_source_selectors;
          for (const auto& j : *v) {
            input_source_selectors.emplace_back(j);
          }
          result.value_ = input_source_selectors;
        }
        break;

      case type::set_variable:
        if (auto o = json_utility::find_object(json, "set_variable")) {
          std::pair<std::string, int> pair;
          if (auto v = json_utility::find_optional<std::string>(*o, "name")) {
            pair.first = *v;
          }
          if (auto v = json_utility::find_optional<int>(*o, "value")) {
            pair.second = *v;
          }
          result.value_ = pair;
        }
        break;

      case type::mouse_key:
        if (auto v = json_utility::find_json(json, "mouse_key")) {
          result.value_ = mouse_key(*v);
        }
        break;

      case type::frontmost_application_changed:
        if (auto v = json_utility::find_json(json, "frontmost_application")) {
          result.value_ = manipulator_environment::frontmost_application(*v);
        }
        break;

      case type::input_source_changed:
        if (auto v = json_utility::find_json(json, "input_source_identifiers")) {
          result.value_ = input_source_identifiers(*v);
        }
        break;

      case type::keyboard_type_changed:
        if (auto v = json_utility::find_optional<std::string>(json, "keyboard_type")) {
          result.value_ = *v;
        }
        break;

      case type::stop_keyboard_repeat:
      case type::device_keys_and_pointing_buttons_are_released:
      case type::device_ungrabbed:
      case type::pointing_device_event_from_event_tap:
        break;
    }

    return result;
  }

  nlohmann::json to_json(void) const {
    nlohmann::json json;
    json["type"] = to_c_string(type_);

    switch (type_) {
      case type::none:
        break;

      case type::key_code:
        if (auto v = get_key_code()) {
          if (auto s = types::make_key_code_name(*v)) {
            json["key_code"] = *s;
          }
        }
        break;

      case type::consumer_key_code:
        if (auto v = get_consumer_key_code()) {
          if (auto s = types::make_consumer_key_code_name(*v)) {
            json["consumer_key_code"] = *s;
          }
        }
        break;

      case type::pointing_button:
        if (auto v = get_pointing_button()) {
          if (auto s = types::make_pointing_button_name(*v)) {
            json["pointing_button"] = *s;
          }
        }
        break;

      case type::pointing_motion:
        if (auto v = get_pointing_motion()) {
          json["pointing_motion"] = *v;
        }
        break;

      case type::caps_lock_state_changed:
        if (auto v = get_integer_value()) {
          json["integer_value"] = *v;
        }
        break;

      case type::shell_command:
        if (auto v = get_shell_command()) {
          json["shell_command"] = *v;
        }
        break;

      case type::select_input_source:
        if (auto v = get_input_source_selectors()) {
          json["input_source_selectors"] = *v;
        }
        break;

      case type::set_variable:
        if (auto v = get_set_variable()) {
          json["set_variable"]["name"] = v->first;
          json["set_variable"]["value"] = v->second;
        }
        break;

      case type::mouse_key:
        if (auto v = get_mouse_key()) {
          json["mouse_key"] = v->to_json();
        }
        break;

      case type::frontmost_application_changed:
        if (auto v = get_frontmost_application()) {
          json["frontmost_application"] = v->to_json();
        }
        break;

      case type::input_source_changed:
        if (auto v = get_input_source_identifiers()) {
          json["input_source_identifiers"] = v->to_json();
        }
        break;

      case type::keyboard_type_changed:
        if (auto v = get_keyboard_type()) {
          json["keyboard_type"] = *v;
        }
        break;

      case type::stop_keyboard_repeat:
      case type::device_keys_and_pointing_buttons_are_released:
      case type::device_ungrabbed:
      case type::pointing_device_event_from_event_tap:
        break;
    }

    return json;
  }

  explicit event(key_code key_code) : type_(type::key_code),
                                      value_(key_code) {
  }

  explicit event(consumer_key_code consumer_key_code) : type_(type::consumer_key_code),
                                                        value_(consumer_key_code) {
  }

  explicit event(pointing_button pointing_button) : type_(type::pointing_button),
                                                    value_(pointing_button) {
  }

  explicit event(const pointing_motion& pointing_motion) : type_(type::pointing_motion),
                                                           value_(pointing_motion) {
  }

  explicit event(type type,
                 int64_t integer_value) : type_(type),
                                          value_(integer_value) {
  }

  static event make_shell_command_event(const std::string& shell_command) {
    event e;
    e.type_ = type::shell_command;
    e.value_ = shell_command;
    return e;
  }

  static event make_select_input_source_event(const std::vector<input_source_selector>& input_source_selector) {
    event e;
    e.type_ = type::select_input_source;
    e.value_ = input_source_selector;
    return e;
  }

  static event make_set_variable_event(const std::pair<std::string, int>& pair) {
    event e;
    e.type_ = type::set_variable;
    e.value_ = pair;
    return e;
  }

  static event make_mouse_key_event(const mouse_key& mouse_key) {
    event e;
    e.type_ = type::mouse_key;
    e.value_ = mouse_key;
    return e;
  }

  static event make_stop_keyboard_repeat_event(void) {
    return make_virtual_event(type::stop_keyboard_repeat);
  }

  static event make_device_keys_and_pointing_buttons_are_released_event(void) {
    return make_virtual_event(type::device_keys_and_pointing_buttons_are_released);
  }

  static event make_device_ungrabbed_event(void) {
    return make_virtual_event(type::device_ungrabbed);
  }

  static event make_pointing_device_event_from_event_tap_event(void) {
    return make_virtual_event(type::pointing_device_event_from_event_tap);
  }

  static event make_frontmost_application_changed_event(const std::string& bundle_identifier,
                                                        const std::string& file_path) {
    event e;
    e.type_ = type::frontmost_application_changed;
    e.value_ = manipulator_environment::frontmost_application(bundle_identifier,
                                                              file_path);
    return e;
  }

  static event make_input_source_changed_event(const input_source_identifiers& input_source_identifiers) {
    event e;
    e.type_ = type::input_source_changed;
    e.value_ = input_source_identifiers;
    return e;
  }

  static event make_keyboard_type_changed_event(const std::string& keyboard_type) {
    event e;
    e.type_ = type::keyboard_type_changed;
    e.value_ = keyboard_type;
    return e;
  }

  type get_type(void) const {
    return type_;
  }

  boost::optional<key_code> get_key_code(void) const {
    try {
      if (type_ == type::key_code) {
        return boost::get<key_code>(value_);
      }
    } catch (boost::bad_get&) {
    }
    return boost::none;
  }

  boost::optional<consumer_key_code> get_consumer_key_code(void) const {
    try {
      if (type_ == type::consumer_key_code) {
        return boost::get<consumer_key_code>(value_);
      }
    } catch (boost::bad_get&) {
    }
    return boost::none;
  }

  boost::optional<pointing_button> get_pointing_button(void) const {
    try {
      if (type_ == type::pointing_button) {
        return boost::get<pointing_button>(value_);
      }
    } catch (boost::bad_get&) {
    }
    return boost::none;
  }

  boost::optional<pointing_motion> get_pointing_motion(void) const {
    try {
      if (type_ == type::pointing_motion) {
        return boost::get<pointing_motion>(value_);
      }
    } catch (boost::bad_get&) {
    }
    return boost::none;
  }

  boost::optional<int64_t> get_integer_value(void) const {
    try {
      if (type_ == type::caps_lock_state_changed) {
        return boost::get<int64_t>(value_);
      }
    } catch (boost::bad_get&) {
    }
    return boost::none;
  }

  boost::optional<std::string> get_shell_command(void) const {
    try {
      if (type_ == type::shell_command) {
        return boost::get<std::string>(value_);
      }
    } catch (boost::bad_get&) {
    }
    return boost::none;
  }

  boost::optional<std::vector<input_source_selector>> get_input_source_selectors(void) const {
    try {
      if (type_ == type::select_input_source) {
        return boost::get<std::vector<input_source_selector>>(value_);
      }
    } catch (boost::bad_get&) {
    }
    return boost::none;
  }

  boost::optional<std::pair<std::string, int>> get_set_variable(void) const {
    try {
      if (type_ == type::set_variable) {
        return boost::get<std::pair<std::string, int>>(value_);
      }
    } catch (boost::bad_get&) {
    }
    return boost::none;
  }

  boost::optional<mouse_key> get_mouse_key(void) const {
    try {
      if (type_ == type::mouse_key) {
        return boost::get<mouse_key>(value_);
      }
    } catch (boost::bad_get&) {
    }
    return boost::none;
  }

  boost::optional<manipulator_environment::frontmost_application> get_frontmost_application(void) const {
    try {
      if (type_ == type::frontmost_application_changed) {
        return boost::get<manipulator_environment::frontmost_application>(value_);
      }
    } catch (boost::bad_get&) {
    }
    return boost::none;
  }

  boost::optional<input_source_identifiers> get_input_source_identifiers(void) const {
    try {
      if (type_ == type::input_source_changed) {
        return boost::get<input_source_identifiers>(value_);
      }
    } catch (boost::bad_get&) {
    }
    return boost::none;
  }

  boost::optional<std::string> get_keyboard_type(void) const {
    try {
      if (type_ == type::keyboard_type_changed) {
        return boost::get<std::string>(value_);
      }
    } catch (boost::bad_get&) {
    }
    return boost::none;
  }

  bool operator==(const event& other) const {
    return get_type() == other.get_type() &&
           value_ == other.value_;
  }

  friend size_t hash_value(const event& value) {
    size_t h = 0;
    boost::hash_combine(h, value.type_);
    boost::hash_combine(h, value.value_);
    return h;
  }

private:
  static event make_virtual_event(type type) {
    event e;
    e.type_ = type;
    e.value_ = boost::blank();
    return e;
  }

  static const char* to_c_string(type t) {
#define TO_C_STRING(TYPE) \
  case type::TYPE:        \
    return #TYPE;

    switch (t) {
      TO_C_STRING(none);
      TO_C_STRING(key_code);
      TO_C_STRING(consumer_key_code);
      TO_C_STRING(pointing_button);
      TO_C_STRING(pointing_motion);
      TO_C_STRING(shell_command);
      TO_C_STRING(select_input_source);
      TO_C_STRING(set_variable);
      TO_C_STRING(mouse_key);
      TO_C_STRING(stop_keyboard_repeat);
      TO_C_STRING(device_keys_and_pointing_buttons_are_released);
      TO_C_STRING(device_ungrabbed);
      TO_C_STRING(caps_lock_state_changed);
      TO_C_STRING(pointing_device_event_from_event_tap);
      TO_C_STRING(frontmost_application_changed);
      TO_C_STRING(input_source_changed);
      TO_C_STRING(keyboard_type_changed);
    }

#undef TO_C_STRING

    return nullptr;
  }

  static type to_type(const std::string& t) {
#define TO_TYPE(TYPE)    \
  {                      \
    if (t == #TYPE) {    \
      return type::TYPE; \
    }                    \
  }

    TO_TYPE(key_code);
    TO_TYPE(consumer_key_code);
    TO_TYPE(pointing_button);
    TO_TYPE(pointing_motion);
    TO_TYPE(shell_command);
    TO_TYPE(select_input_source);
    TO_TYPE(set_variable);
    TO_TYPE(mouse_key);
    TO_TYPE(stop_keyboard_repeat);
    TO_TYPE(device_keys_and_pointing_buttons_are_released);
    TO_TYPE(device_ungrabbed);
    TO_TYPE(caps_lock_state_changed);
    TO_TYPE(pointing_device_event_from_event_tap);
    TO_TYPE(frontmost_application_changed);
    TO_TYPE(input_source_changed);
    TO_TYPE(keyboard_type_changed);

#undef TO_TYPE

    return type::none;
  }

  type type_;

  boost::variant<key_code,                                       // For type::key_code
                 consumer_key_code,                              // For type::consumer_key_code
                 pointing_button,                                // For type::pointing_button
                 pointing_motion,                                // For type::pointing_motion
                 int64_t,                                        // For type::caps_lock_state_changed
                 std::string,                                    // For shell_command, keyboard_type_changed
                 std::vector<input_source_selector>,             // For select_input_source
                 std::pair<std::string, int>,                    // For set_variable
                 mouse_key,                                      // For mouse_key
                 manipulator_environment::frontmost_application, // For frontmost_application_changed
                 input_source_identifiers,                       // For input_source_changed
                 boost::blank>                                   // For virtual events
      value_;
};

inline std::ostream& operator<<(std::ostream& stream, const event::type& value) {
  return stream_utility::output_enum(stream, value);
}

inline std::ostream& operator<<(std::ostream& stream, const event& event) {
  stream << "{"
         << "\"type\":";
  stream_utility::output_enum(stream, event.get_type());

  if (auto key_code = event.get_key_code()) {
    stream << ",\"key_code\":" << *key_code;
  }

  if (auto pointing_button = event.get_pointing_button()) {
    stream << ",\"pointing_button\":" << *pointing_button;
  }

  if (auto pointing_motion = event.get_pointing_motion()) {
    stream << ",\"pointing_motion\":" << pointing_motion->to_json();
  }

  stream << "}";

  return stream;
}

inline void to_json(nlohmann::json& json, const event& value) {
  json = value.to_json();
}
} // namespace event_queue
} // namespace krbn

namespace std {
template <>
struct hash<krbn::event_queue::event> final {
  std::size_t operator()(const krbn::event_queue::event& v) const {
    return hash_value(v);
  }
};
} // namespace std
