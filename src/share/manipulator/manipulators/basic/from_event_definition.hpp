#pragma once

#include "../../types.hpp"
#include "simultaneous_options.hpp"
#include <pqrs/json.hpp>
#include <set>
#include <vector>

namespace krbn {
namespace manipulator {
namespace manipulators {
namespace basic {
class from_event_definition final {
public:
  from_event_definition(void)
      : simultaneous_options_(std::make_shared<simultaneous_options>()) {
  }

  virtual ~from_event_definition(void) {
  }

  const std::vector<event_definition>& get_event_definitions(void) const {
    return event_definitions_;
  }

  void set_event_definitions(const std::vector<event_definition>& value) {
    event_definitions_ = value;
  }

  const from_modifiers_definition& get_from_modifiers_definition(void) const {
    return from_modifiers_definition_;
  }

  void set_from_modifiers_definition(const from_modifiers_definition& value) {
    from_modifiers_definition_ = value;
  }

  const std::optional<event_integer_value::value_t>& get_event_integer_value(void) const {
    return event_integer_value_;
  }

  void set_event_integer_value(const std::optional<event_integer_value::value_t>& value) {
    event_integer_value_ = value;
  }

  const pqrs::not_null_shared_ptr_t<simultaneous_options>& get_simultaneous_options(void) const {
    return simultaneous_options_;
  }

  void set_simultaneous_options(const pqrs::not_null_shared_ptr_t<simultaneous_options>& value) {
    simultaneous_options_ = value;
  }

  static bool test_event(const event_queue::event& event,
                         const event_definition& event_definition) {
    if (auto event_momentary_switch_event = event.get_if<momentary_switch_event>()) {
      if (auto event_definition_momentary_switch_event = event_definition.get_if<momentary_switch_event>()) {
        if (*event_momentary_switch_event == *event_definition_momentary_switch_event) {
          return true;
        }
      }
    }

    if (auto any_type = event_definition.get_if<event_definition::any_type>()) {
      if (auto e = event.get_if<momentary_switch_event>()) {
        if (e->valid()) {
          auto usage_page = e->get_usage_pair().get_usage_page();

          switch (*any_type) {
            case event_definition::any_type::key_code:
              if (usage_page == pqrs::hid::usage_page::keyboard_or_keypad) {
                return true;
              }
              break;

            case event_definition::any_type::consumer_key_code:
              if (usage_page == pqrs::hid::usage_page::consumer) {
                return true;
              }
              break;

            case event_definition::any_type::apple_vendor_keyboard_key_code:
              if (usage_page == pqrs::hid::usage_page::apple_vendor_keyboard) {
                return true;
              }
              break;

            case event_definition::any_type::apple_vendor_top_case_key_code:
              if (usage_page == pqrs::hid::usage_page::apple_vendor_top_case) {
                return true;
              }
              break;

            case event_definition::any_type::pointing_button:
              if (usage_page == pqrs::hid::usage_page::button) {
                return true;
              }
              break;
          }
        }
      }
    }

    return false;
  }

  static bool test_event(const event_queue::event& event,
                         const from_event_definition& from_event_definition) {
    for (const auto& d : from_event_definition.get_event_definitions()) {
      if (test_event(event, d)) {
        return true;
      }
    }

    return false;
  }

  static bool test_key_order(const std::vector<event_queue::event>& events,
                             simultaneous_options::key_order key_order,
                             const std::vector<event_definition>& event_definitions) {
    switch (key_order) {
      case simultaneous_options::key_order::insensitive:
        // Do nothing
        break;

      case simultaneous_options::key_order::strict:
        for (auto events_it = std::begin(events); events_it != std::end(events); std::advance(events_it, 1)) {
          auto event_definitions_index = static_cast<size_t>(std::distance(std::begin(events), events_it));
          if (event_definitions_index < event_definitions.size()) {
            if (!test_event(*events_it, event_definitions[event_definitions_index])) {
              return false;
            }
          }
        }
        break;

      case simultaneous_options::key_order::strict_inverse:
        for (auto events_it = std::begin(events); events_it != std::end(events); std::advance(events_it, 1)) {
          auto event_definitions_index = static_cast<size_t>(std::distance(std::begin(events), events_it));
          if (event_definitions_index < event_definitions.size()) {
            if (!test_event(*events_it, event_definitions[event_definitions.size() - 1 - event_definitions_index])) {
              return false;
            }
          }
        }
        break;
    }

    return true;
  }

private:
  std::vector<event_definition> event_definitions_;
  from_modifiers_definition from_modifiers_definition_;
  std::optional<event_integer_value::value_t> event_integer_value_;
  pqrs::not_null_shared_ptr_t<simultaneous_options> simultaneous_options_;
};

inline void from_json(const nlohmann::json& json, from_event_definition& d) {
  pqrs::json::requires_object(json, "json");

  std::vector<event_definition> event_definitions;
  event_definition default_event_definition;

  for (const auto& [key, value] : json.items()) {
    // key is always std::string.

    if (default_event_definition.handle_json(key, value, json)) {
      // Do nothing

    } else if (key == "modifiers") {
      try {
        d.set_from_modifiers_definition(value.get<from_modifiers_definition>());
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
      }

    } else if (key == "integer_value") {
      try {
        d.set_event_integer_value(value.get<event_integer_value::value_t>());
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
      }

    } else if (key == "simultaneous") {
      pqrs::json::requires_array(value, "`" + key + "`");

      for (const auto& j : value) {
        pqrs::json::requires_object(j, "`" + key + "` entry");

        event_definition d;

        for (const auto& [k, v] : j.items()) {
          // k is always std::string.

          if (d.handle_json(k, v, j)) {
            // Do nothing
          } else {
            throw pqrs::json::unmarshal_error(fmt::format("unknown key: `{0}` in `{1}", k, pqrs::json::dump_for_error_message(j)));
          }
        }

        if (d.get_type() == event_definition::type::none) {
          throw pqrs::json::unmarshal_error(fmt::format("event type is invalid: `{0}`", pqrs::json::dump_for_error_message(json)));
        }

        event_definitions.push_back(d);
      }

    } else if (key == "simultaneous_options") {
      try {
        d.set_simultaneous_options(std::make_shared<simultaneous_options>(value));
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
      }

    } else {
      throw pqrs::json::unmarshal_error(fmt::format("`from` error: unknown key `{0}` in `{1}`", key, pqrs::json::dump_for_error_message(json)));
    }
  }

  if (event_definitions.empty() &&
      default_event_definition.get_type() != event_definition::type::none) {
    event_definitions.push_back(default_event_definition);
  }

  // ----------------------------------------

  if (event_definitions.empty()) {
    throw pqrs::json::unmarshal_error(fmt::format("event is not specified: `{0}`", pqrs::json::dump_for_error_message(json)));
  }

  for (const auto& d : event_definitions) {
    switch (d.get_type()) {
      case event_definition::type::momentary_switch_event:
      case event_definition::type::any:
        break;

      case event_definition::type::none:
      case event_definition::type::shell_command:
      case event_definition::type::select_input_source:
      case event_definition::type::set_variable:
      case event_definition::type::set_notification_message:
      case event_definition::type::mouse_key:
      case event_definition::type::sticky_modifier:
      case event_definition::type::software_function:
        throw pqrs::json::unmarshal_error(fmt::format("event type is invalid: `{0}`", pqrs::json::dump_for_error_message(json)));
    }
  }

  d.set_event_definitions(event_definitions);
}
} // namespace basic
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
