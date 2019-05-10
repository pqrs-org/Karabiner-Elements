#pragma once

// `krbn::event_queue::event` can be used safely in a multi-threaded environment.

#include "core_configuration/core_configuration.hpp"
#include "device_properties.hpp"
#include "hash.hpp"
#include "manipulator/manipulator_environment.hpp"
#include "types.hpp"
#include <mpark/variant.hpp>
#include <optional>
#include <pqrs/hash.hpp>
#include <pqrs/osx/system_preferences.hpp>
#include <pqrs/osx/system_preferences/extra/nlohmann_json.hpp>

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
    device_grabbed,
    device_ungrabbed,
    caps_lock_state_changed,
    pointing_device_event_from_event_tap,
    frontmost_application_changed,
    input_source_changed,
    system_preferences_properties_changed,
    virtual_hid_keyboard_configuration_changed,
  };

  using value_t = mpark::variant<key_code,                                                 // For type::key_code
                                 consumer_key_code,                                        // For type::consumer_key_code
                                 pointing_button,                                          // For type::pointing_button
                                 pointing_motion,                                          // For type::pointing_motion
                                 int64_t,                                                  // For type::caps_lock_state_changed
                                 std::string,                                              // For shell_command
                                 std::vector<pqrs::osx::input_source_selector::specifier>, // For select_input_source
                                 std::pair<std::string, int>,                              // For set_variable
                                 mouse_key,                                                // For mouse_key
                                 pqrs::osx::frontmost_application_monitor::application,    // For frontmost_application_changed
                                 pqrs::osx::input_source::properties,                      // For input_source_changed
                                 device_properties,                                        // For device_grabbed
                                 pqrs::osx::system_preferences::properties,                // For system_preferences_properties_changed
                                 core_configuration::details::virtual_hid_keyboard,        // For virtual_hid_keyboard_configuration_changed
                                 mpark::monostate>;                                        // For virtual events

  event(void) : type_(type::none),
                value_(mpark::monostate()) {
  }

  static event make_from_json(const nlohmann::json& json) {
    event result;

    try {
      if (json.is_object()) {
        for (const auto& [key, value] : json.items()) {
          if (key == "type") {
            result.type_ = to_type(value.get<std::string>());
          } else if (key == "key_code") {
            result.value_ = value.get<key_code>();
          } else if (key == "consumer_key_code") {
            result.value_ = value.get<consumer_key_code>();
          } else if (key == "pointing_button") {
            result.value_ = value.get<pointing_button>();
          } else if (key == "pointing_motion") {
            result.value_ = value.get<pointing_motion>();
          } else if (key == "caps_lock_state_changed") {
            result.value_ = value.get<int64_t>();
          } else if (key == "shell_command") {
            result.value_ = value.get<std::string>();
          } else if (key == "input_source_specifiers") {
            result.value_ = value.get<std::vector<pqrs::osx::input_source_selector::specifier>>();
          } else if (key == "set_variable") {
            result.value_ = value.get<std::pair<std::string, int>>();
          } else if (key == "mouse_key") {
            result.value_ = value.get<mouse_key>();
          } else if (key == "frontmost_application") {
            result.value_ = value.get<pqrs::osx::frontmost_application_monitor::application>();
          } else if (key == "input_source_properties") {
            result.value_ = value.get<pqrs::osx::input_source::properties>();
          } else if (key == "system_preferences_properties") {
            result.value_ = value.get<pqrs::osx::system_preferences::properties>();
          } else if (key == "virtual_hid_keyboard_configuration") {
            result.value_ = value.get<core_configuration::details::virtual_hid_keyboard>();
          }
        }
      }
    } catch (...) {
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
          json["key_code"] = types::make_key_code_name(*v);
        }
        break;

      case type::consumer_key_code:
        if (auto v = get_consumer_key_code()) {
          json["consumer_key_code"] = types::make_consumer_key_code_name(*v);
        }
        break;

      case type::pointing_button:
        if (auto v = get_pointing_button()) {
          json["pointing_button"] = types::make_pointing_button_name(*v);
        }
        break;

      case type::pointing_motion:
        if (auto v = get_pointing_motion()) {
          json["pointing_motion"] = *v;
        }
        break;

      case type::caps_lock_state_changed:
        if (auto v = get_integer_value()) {
          json["caps_lock_state_changed"] = *v;
        }
        break;

      case type::shell_command:
        if (auto v = get_shell_command()) {
          json["shell_command"] = *v;
        }
        break;

      case type::select_input_source:
        if (auto v = get_input_source_specifiers()) {
          json["input_source_specifiers"] = *v;
        }
        break;

      case type::set_variable:
        if (auto v = get_set_variable()) {
          json["set_variable"] = *v;
        }
        break;

      case type::mouse_key:
        if (auto v = get_mouse_key()) {
          json["mouse_key"] = *v;
        }
        break;

      case type::frontmost_application_changed:
        if (auto v = get_frontmost_application()) {
          json["frontmost_application"] = *v;
        }
        break;

      case type::input_source_changed:
        if (auto v = get_input_source_properties()) {
          json["input_source_properties"] = *v;
        }
        break;

      case type::system_preferences_properties_changed:
        if (auto v = mpark::get_if<pqrs::osx::system_preferences::properties>(&value_)) {
          json["system_preferences_properties"] = *v;
        }
        break;

      case type::virtual_hid_keyboard_configuration_changed:
        if (auto v = mpark::get_if<core_configuration::details::virtual_hid_keyboard>(&value_)) {
          json["virtual_hid_keyboard_configuration"] = *v;
        }
        break;

      case type::stop_keyboard_repeat:
      case type::device_keys_and_pointing_buttons_are_released:
      case type::device_grabbed:
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

  static event make_select_input_source_event(const std::vector<pqrs::osx::input_source_selector::specifier>& input_source_specifiers) {
    event e;
    e.type_ = type::select_input_source;
    e.value_ = input_source_specifiers;
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

  static event make_device_grabbed_event(const device_properties& device_properties) {
    event e;
    e.type_ = type::device_grabbed;
    e.value_ = device_properties;
    return e;
  }

  static event make_device_ungrabbed_event(void) {
    return make_virtual_event(type::device_ungrabbed);
  }

  static event make_pointing_device_event_from_event_tap_event(void) {
    return make_virtual_event(type::pointing_device_event_from_event_tap);
  }

  static event make_frontmost_application_changed_event(const pqrs::osx::frontmost_application_monitor::application& application) {
    event e;
    e.type_ = type::frontmost_application_changed;
    e.value_ = application;
    return e;
  }

  static event make_input_source_changed_event(const pqrs::osx::input_source::properties& properties) {
    event e;
    e.type_ = type::input_source_changed;
    e.value_ = properties;
    return e;
  }

  static event make_system_preferences_properties_changed_event(const pqrs::osx::system_preferences::properties& properties) {
    event e;
    e.type_ = type::system_preferences_properties_changed;
    e.value_ = properties;
    return e;
  }

  static event make_virtual_hid_keyboard_configuration_changed_event(const core_configuration::details::virtual_hid_keyboard& configuration) {
    event e;
    e.type_ = type::virtual_hid_keyboard_configuration_changed;
    e.value_ = configuration;
    return e;
  }

  type get_type(void) const {
    return type_;
  }

  const value_t& get_value(void) const {
    return value_;
  }

  template <typename T>
  const T* find(void) const {
    return mpark::get_if<T>(&value_);
  }

  std::optional<key_code> get_key_code(void) const {
    try {
      if (type_ == type::key_code) {
        return mpark::get<key_code>(value_);
      }
    } catch (mpark::bad_variant_access&) {
    }
    return std::nullopt;
  }

  std::optional<consumer_key_code> get_consumer_key_code(void) const {
    try {
      if (type_ == type::consumer_key_code) {
        return mpark::get<consumer_key_code>(value_);
      }
    } catch (mpark::bad_variant_access&) {
    }
    return std::nullopt;
  }

  std::optional<pointing_button> get_pointing_button(void) const {
    try {
      if (type_ == type::pointing_button) {
        return mpark::get<pointing_button>(value_);
      }
    } catch (mpark::bad_variant_access&) {
    }
    return std::nullopt;
  }

  std::optional<pointing_motion> get_pointing_motion(void) const {
    try {
      if (type_ == type::pointing_motion) {
        return mpark::get<pointing_motion>(value_);
      }
    } catch (mpark::bad_variant_access&) {
    }
    return std::nullopt;
  }

  std::optional<int64_t> get_integer_value(void) const {
    try {
      if (type_ == type::caps_lock_state_changed) {
        return mpark::get<int64_t>(value_);
      }
    } catch (mpark::bad_variant_access&) {
    }
    return std::nullopt;
  }

  std::optional<std::string> get_shell_command(void) const {
    try {
      if (type_ == type::shell_command) {
        return mpark::get<std::string>(value_);
      }
    } catch (mpark::bad_variant_access&) {
    }
    return std::nullopt;
  }

  std::optional<std::vector<pqrs::osx::input_source_selector::specifier>> get_input_source_specifiers(void) const {
    try {
      if (type_ == type::select_input_source) {
        return mpark::get<std::vector<pqrs::osx::input_source_selector::specifier>>(value_);
      }
    } catch (mpark::bad_variant_access&) {
    }
    return std::nullopt;
  }

  std::optional<std::pair<std::string, int>> get_set_variable(void) const {
    try {
      if (type_ == type::set_variable) {
        return mpark::get<std::pair<std::string, int>>(value_);
      }
    } catch (mpark::bad_variant_access&) {
    }
    return std::nullopt;
  }

  std::optional<mouse_key> get_mouse_key(void) const {
    try {
      if (type_ == type::mouse_key) {
        return mpark::get<mouse_key>(value_);
      }
    } catch (mpark::bad_variant_access&) {
    }
    return std::nullopt;
  }

  std::optional<pqrs::osx::frontmost_application_monitor::application> get_frontmost_application(void) const {
    try {
      if (type_ == type::frontmost_application_changed) {
        return mpark::get<pqrs::osx::frontmost_application_monitor::application>(value_);
      }
    } catch (mpark::bad_variant_access&) {
    }
    return std::nullopt;
  }

  std::optional<pqrs::osx::input_source::properties> get_input_source_properties(void) const {
    try {
      if (type_ == type::input_source_changed) {
        return mpark::get<pqrs::osx::input_source::properties>(value_);
      }
    } catch (mpark::bad_variant_access&) {
    }
    return std::nullopt;
  }

  std::shared_ptr<key_down_up_valued_event> make_key_down_up_valued_event(void) const {
    if (auto value = find<key_code>()) {
      return std::make_shared<key_down_up_valued_event>(*value);

    } else if (auto value = find<consumer_key_code>()) {
      return std::make_shared<key_down_up_valued_event>(*value);

    } else if (auto value = find<pointing_button>()) {
      return std::make_shared<key_down_up_valued_event>(*value);
    }

    return nullptr;
  }

  bool operator==(const event& other) const {
    return get_type() == other.get_type() &&
           value_ == other.value_;
  }

private:
  static event make_virtual_event(type type) {
    event e;
    e.type_ = type;
    e.value_ = mpark::monostate();
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
      TO_C_STRING(device_grabbed);
      TO_C_STRING(device_ungrabbed);
      TO_C_STRING(caps_lock_state_changed);
      TO_C_STRING(pointing_device_event_from_event_tap);
      TO_C_STRING(frontmost_application_changed);
      TO_C_STRING(input_source_changed);
      TO_C_STRING(system_preferences_properties_changed);
      TO_C_STRING(virtual_hid_keyboard_configuration_changed);
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
    TO_TYPE(device_grabbed);
    TO_TYPE(device_ungrabbed);
    TO_TYPE(caps_lock_state_changed);
    TO_TYPE(pointing_device_event_from_event_tap);
    TO_TYPE(frontmost_application_changed);
    TO_TYPE(input_source_changed);
    TO_TYPE(system_preferences_properties_changed);
    TO_TYPE(virtual_hid_keyboard_configuration_changed);

#undef TO_TYPE

    return type::none;
  }

  type type_;
  value_t value_;
};

inline void to_json(nlohmann::json& json, const event& value) {
  json = value.to_json();
}
} // namespace event_queue
} // namespace krbn

namespace std {
template <>
struct hash<krbn::event_queue::event> final {
  std::size_t operator()(const krbn::event_queue::event& value) const {
    std::size_t h = 0;

    pqrs::hash_combine(h, value.get_type());
    pqrs::hash_combine(h, value.get_value());

    return h;
  }
};
} // namespace std
