#pragma once

#include "event_queue.hpp"
#include <mpark/variant.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <pqrs/json.hpp>

namespace krbn {
namespace manipulator {
class event_definition final {
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
                                 key_code::value_t,
                                 consumer_key_code::value_t,
                                 pointing_button::value_t,
                                 type,                                                     // For any
                                 std::string,                                              // For shell_command
                                 std::vector<pqrs::osx::input_source_selector::specifier>, // For select_input_source
                                 std::pair<std::string, int>,                              // For set_variable
                                 mouse_key                                                 // For mouse_key
                                 >;

  event_definition(void) : type_(type::none),
                           value_(mpark::monostate()) {
  }

  ~event_definition(void) {
  }

  type get_type(void) const {
    return type_;
  }

  const value_t& get_value(void) const {
    return value_;
  }

  std::optional<key_code::value_t> get_key_code(void) const {
    if (type_ == type::key_code) {
      return mpark::get<key_code::value_t>(value_);
    }
    return std::nullopt;
  }

  std::optional<consumer_key_code::value_t> get_consumer_key_code(void) const {
    if (type_ == type::consumer_key_code) {
      return mpark::get<consumer_key_code::value_t>(value_);
    }
    return std::nullopt;
  }

  std::optional<pointing_button::value_t> get_pointing_button(void) const {
    if (type_ == type::pointing_button) {
      return mpark::get<pointing_button::value_t>(value_);
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
        return event_queue::event(mpark::get<key_code::value_t>(value_));
      case type::consumer_key_code:
        return event_queue::event(mpark::get<consumer_key_code::value_t>(value_));
      case type::pointing_button:
        return event_queue::event(mpark::get<pointing_button::value_t>(value_));
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
      check_type(json);

      try {
        type_ = type::key_code;
        value_ = value.get<key_code::value_t>();
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
      }

      return true;
    }

    // ----------------------------------------
    // consumer_key_code

    if (key == "consumer_key_code") {
      check_type(json);

      try {
        type_ = type::consumer_key_code;
        value_ = value.get<consumer_key_code::value_t>();
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
      }

      return true;
    }

    // ----------------------------------------
    // pointing_button

    if (key == "pointing_button") {
      check_type(json);

      try {
        type_ = type::pointing_button;
        value_ = value.get<pointing_button::value_t>();
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
      }

      return true;
    }

    // ----------------------------------------
    // any

    if (key == "any") {
      check_type(json);

      if (!value.is_string()) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be string, but is `{1}`", key, value.dump()));
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
        throw pqrs::json::unmarshal_error(fmt::format("unknown `{0}`: `{1}`", key, value.dump()));
      }

      return true;
    }

    // ----------------------------------------
    // shell_command

    if (key == "shell_command") {
      check_type(json);

      if (!value.is_string()) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be string, but is `{1}`", key, value.dump()));
      }

      type_ = type::shell_command;
      value_ = value.get<std::string>();

      return true;
    }

    // ----------------------------------------
    // select_input_source

    if (key == "select_input_source") {
      check_type(json);

      std::vector<pqrs::osx::input_source_selector::specifier> input_source_specifiers;

      if (value.is_object()) {
        try {
          input_source_specifiers.push_back(value.get<pqrs::osx::input_source_selector::specifier>());
        } catch (const pqrs::json::unmarshal_error& e) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
        }
      } else if (value.is_array()) {
        try {
          input_source_specifiers = value.get<std::vector<pqrs::osx::input_source_selector::specifier>>();
        } catch (const pqrs::json::unmarshal_error& e) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
        }
      } else {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be object or array of objects, but is `{1}`", key, value.dump()));
      }

      type_ = type::select_input_source;
      value_ = input_source_specifiers;

      return true;
    }

    // ----------------------------------------
    // set_variable

    if (key == "set_variable") {
      check_type(json);

      if (!value.is_object()) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be object, but is `{1}`", key, value.dump()));
      }

      std::optional<std::string> variable_name;
      std::optional<int> variable_value;

      for (const auto& [k, v] : value.items()) {
        // k is always std::string.

        if (k == "name") {
          if (!v.is_string()) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}.name` must be string, but is `{1}`", key, v.dump()));
          }
          variable_name = v.get<std::string>();

        } else if (k == "value") {
          if (!v.is_number()) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}.value` must be number, but is `{1}`", key, v.dump()));
          }
          variable_value = v.get<int>();

        } else if (k == "description") {
          // Do nothing

        } else {
          throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", k, value.dump()));
        }
      }

      if (!variable_name) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}.name` is not found in `{1}`", key, value.dump()));
      }

      if (!variable_value) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}.value` is not found in `{1}`", key, value.dump()));
      }

      type_ = type::set_variable;
      value_ = std::make_pair(*variable_name, *variable_value);

      return true;
    }

    // ----------------------------------------
    // mouse_key

    if (key == "mouse_key") {
      check_type(json);

      if (!value.is_object()) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be object, but is `{1}`", key, value.dump()));
      }

      type_ = type::mouse_key;

      try {
        value_ = value.get<mouse_key>();
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
      }

      return true;
    }

    if (key == "description") {
      // Do nothing

      return true;
    }

    return false;
  }

private:
  void check_type(const nlohmann::json& json) const {
    if (type_ != type::none) {
      throw pqrs::json::unmarshal_error(fmt::format("multiple types are specified: `{0}`", json.dump()));
    }
  }

  type type_;
  value_t value_;
};
} // namespace manipulator
} // namespace krbn
