#pragma once

#include "event_queue.hpp"
#include <mpark/variant.hpp>
#include <nlohmann/json.hpp>
#include <optional>

namespace krbn {
namespace manipulator {
class event_definition {
public:
  enum class type {
    none,
    key_code,
    consumer_key_code,
    pointing_button,
    any,
    shell_command,
    select_input_source,
    set_variable,
    mouse_key,
  };

  using value_t = mpark::variant<mpark::monostate,
                                 key_code,
                                 consumer_key_code,
                                 pointing_button,
                                 type,                                                     // For any
                                 std::string,                                              // For shell_command
                                 std::vector<pqrs::osx::input_source_selector::specifier>, // For select_input_source
                                 std::pair<std::string, int>,                              // For set_variable
                                 mouse_key                                                 // For mouse_key
                                 >;

  event_definition(void) : type_(type::none),
                           value_(mpark::monostate()) {
  }

  virtual ~event_definition(void) {
  }

  type get_type(void) const {
    return type_;
  }

  const value_t& get_value(void) const {
    return value_;
  }

  std::optional<key_code> get_key_code(void) const {
    if (type_ == type::key_code) {
      return mpark::get<key_code>(value_);
    }
    return std::nullopt;
  }

  std::optional<consumer_key_code> get_consumer_key_code(void) const {
    if (type_ == type::consumer_key_code) {
      return mpark::get<consumer_key_code>(value_);
    }
    return std::nullopt;
  }

  std::optional<pointing_button> get_pointing_button(void) const {
    if (type_ == type::pointing_button) {
      return mpark::get<pointing_button>(value_);
    }
    return std::nullopt;
  }

  std::optional<type> get_any_type(void) const {
    if (type_ == type::any) {
      return mpark::get<type>(value_);
    }
    return std::nullopt;
  }

  std::optional<std::string> get_shell_command(void) const {
    if (type_ == type::shell_command) {
      return mpark::get<std::string>(value_);
    }
    return std::nullopt;
  }

  std::optional<std::vector<pqrs::osx::input_source_selector::specifier>> get_input_source_specifiers(void) const {
    if (type_ == type::select_input_source) {
      return mpark::get<std::vector<pqrs::osx::input_source_selector::specifier>>(value_);
    }
    return std::nullopt;
  }

  std::optional<std::pair<std::string, int>> get_set_variable(void) const {
    if (type_ == type::set_variable) {
      return mpark::get<std::pair<std::string, int>>(value_);
    }
    return std::nullopt;
  }

  std::optional<mouse_key> get_mouse_key(void) const {
    if (type_ == type::mouse_key) {
      return mpark::get<mouse_key>(value_);
    }
    return std::nullopt;
  }

  std::optional<event_queue::event> to_event(void) const {
    switch (type_) {
      case type::none:
        return std::nullopt;
      case type::key_code:
        return event_queue::event(mpark::get<key_code>(value_));
      case type::consumer_key_code:
        return event_queue::event(mpark::get<consumer_key_code>(value_));
      case type::pointing_button:
        return event_queue::event(mpark::get<pointing_button>(value_));
      case type::any:
        return std::nullopt;
      case type::shell_command:
        return event_queue::event::make_shell_command_event(mpark::get<std::string>(value_));
      case type::select_input_source:
        return event_queue::event::make_select_input_source_event(mpark::get<std::vector<pqrs::osx::input_source_selector::specifier>>(value_));
      case type::set_variable:
        return event_queue::event::make_set_variable_event(mpark::get<std::pair<std::string, int>>(value_));
      case type::mouse_key:
        return event_queue::event::make_mouse_key_event(mpark::get<mouse_key>(value_));
    }
  }

  bool handle_json(const std::string& key,
                   const nlohmann::json& value,
                   const nlohmann::json& json) {
    // Set type_ and values.

    // ----------------------------------------
    // key_code

    if (key == "key_code") {
      if (type_ != type::none) {
        logger::get_logger()->error("complex_modifications json error: Duplicated type definition: {0}", json.dump());
        return true;
      }
      if (!value.is_string()) {
        logger::get_logger()->error("complex_modifications json error: Invalid form of key_code: {0}", json.dump());
        return true;
      }

      if (auto key_code = types::make_key_code(value.get<std::string>())) {
        type_ = type::key_code;
        value_ = *key_code;
      }

      return true;
    }

    // ----------------------------------------
    // consumer_key_code

    if (key == "consumer_key_code") {
      if (type_ != type::none) {
        logger::get_logger()->error("complex_modifications json error: Duplicated type definition: {0}", json.dump());
        return true;
      }
      if (!value.is_string()) {
        logger::get_logger()->error("complex_modifications json error: Invalid form of consumer_key_code: {0}", json.dump());
        return true;
      }

      if (auto consumer_key_code = types::make_consumer_key_code(value.get<std::string>())) {
        type_ = type::consumer_key_code;
        value_ = *consumer_key_code;
      }

      return true;
    }

    // ----------------------------------------
    // pointing_button

    if (key == "pointing_button") {
      if (type_ != type::none) {
        logger::get_logger()->error("complex_modifications json error: Duplicated type definition: {0}", json.dump());
        return true;
      }
      if (!value.is_string()) {
        logger::get_logger()->error("complex_modifications json error: Invalid form of pointing_button: {0}", json.dump());
        return true;
      }

      if (auto pointing_button = types::make_pointing_button(value.get<std::string>())) {
        type_ = type::pointing_button;
        value_ = *pointing_button;
      }

      return true;
    }

    // ----------------------------------------
    // any

    if (key == "any") {
      if (type_ != type::none) {
        logger::get_logger()->error("complex_modifications json error: Duplicated type definition: {0}", json.dump());
        return true;
      }
      if (!value.is_string()) {
        logger::get_logger()->error("complex_modifications json error: Invalid form of any: {0}", json.dump());
        return true;
      }

      if (value == "key_code") {
        type_ = type::any;
        value_ = type::key_code;
      } else if (value == "consumer_key_code") {
        type_ = type::any;
        value_ = type::consumer_key_code;
      } else if (value == "pointing_button") {
        type_ = type::any;
        value_ = type::pointing_button;
      } else {
        logger::get_logger()->error("complex_modifications json error: Unknown value of any: {0}", json.dump());
      }

      return true;
    }

    // ----------------------------------------
    // shell_command

    if (key == "shell_command") {
      if (type_ != type::none) {
        logger::get_logger()->error("complex_modifications json error: Duplicated type definition: {0}", json.dump());
        return true;
      }
      if (!value.is_string()) {
        logger::get_logger()->error("complex_modifications json error: Invalid form of shell_command: {0}", json.dump());
        return true;
      }

      type_ = type::shell_command;
      value_ = value.get<std::string>();

      return true;
    }

    // ----------------------------------------
    // select_input_source

    if (key == "select_input_source") {
      if (type_ != type::none) {
        logger::get_logger()->error("complex_modifications json error: Duplicated type definition: {0}", json.dump());
        return true;
      }

      std::vector<pqrs::osx::input_source_selector::specifier> input_source_specifiers;

      if (value.is_object()) {
        input_source_specifiers.emplace_back(value);
      } else if (value.is_array()) {
        for (const auto& v : value) {
          input_source_specifiers.emplace_back(v);
        }
      } else {
        logger::get_logger()->error("complex_modifications json error: Invalid form of select_input_source: {0}", json.dump());
        return true;
      }

      type_ = type::select_input_source;
      value_ = input_source_specifiers;

      return true;
    }

    // ----------------------------------------
    // set_variable

    if (key == "set_variable") {
      if (type_ != type::none) {
        logger::get_logger()->error("complex_modifications json error: Duplicated type definition: {0}", json.dump());
        return true;
      }
      if (!value.is_object()) {
        logger::get_logger()->error("complex_modifications json error: Invalid form of set_variable: {0}", json.dump());
        return true;
      }

      if (auto n = json_utility::find_optional<std::string>(value, "name")) {
        if (auto v = json_utility::find_optional<int>(value, "value")) {
          type_ = type::set_variable;
          value_ = std::make_pair(*n, *v);
        } else {
          logger::get_logger()->error("complex_modifications json error: valid `value` is not found in set_variable: {0}", json.dump());
        }
      } else {
        logger::get_logger()->error("complex_modifications json error: valid `name` is not found in set_variable: {0}", json.dump());
      }

      return true;
    }

    // ----------------------------------------
    // mouse_key

    if (key == "mouse_key") {
      if (type_ != type::none) {
        logger::get_logger()->error("complex_modifications json error: Duplicated type definition: {0}", json.dump());
        return true;
      }
      if (!value.is_object()) {
        logger::get_logger()->error("complex_modifications json error: Invalid form of mouse_key: {0}", json.dump());
        return true;
      }

      type_ = type::mouse_key;
      value_ = mouse_key(value);

      return true;
    }

    if (key == "description") {
      // Do nothing

      return true;
    }

    return false;
  }

protected:
  type type_;
  value_t value_;
};
} // namespace manipulator
} // namespace krbn
