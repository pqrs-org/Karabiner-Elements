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
  from_event_definition(void) {
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

  const simultaneous_options& get_simultaneous_options(void) const {
    return simultaneous_options_;
  }

  void set_simultaneous_options(const simultaneous_options& value) {
    simultaneous_options_ = value;
  }

  static bool test_event(const event_queue::event& event,
                         const event_definition& event_definition) {
    if (auto key_code = event.get_key_code()) {
      if (event_definition.get_key_code() == key_code ||
          event_definition.get_any_type() == event_definition::type::key_code) {
        return true;
      }
    }

    if (auto consumer_key_code = event.get_consumer_key_code()) {
      if (event_definition.get_consumer_key_code() == consumer_key_code ||
          event_definition.get_any_type() == event_definition::type::consumer_key_code) {
        return true;
      }
    }

    if (auto pointing_button = event.get_pointing_button()) {
      if (event_definition.get_pointing_button() == pointing_button ||
          event_definition.get_any_type() == event_definition::type::pointing_button) {
        return true;
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
  simultaneous_options simultaneous_options_;
};

inline void from_json(const nlohmann::json& json, from_event_definition& d) {
  if (!json.is_object()) {
    throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
  }

  std::vector<event_definition> event_definitions;
  event_definition default_event_definition;
  from_modifiers_definition from_modifiers_definition;

  for (const auto& [key, value] : json.items()) {
    // key is always std::string.

    if (default_event_definition.handle_json(key, value, json)) {
      // Do nothing

    } else if (from_modifiers_definition.handle_json(key, value, json)) {
      // Do nothing

    } else if (key == "simultaneous") {
      if (!value.is_array()) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be array, but is `{1}`", key, value.dump()));
      }

      for (const auto& j : value) {
        if (!j.is_object()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` entry must be object, but is `{1}`", key, j.dump()));
        }

        event_definition d;

        for (const auto& [k, v] : j.items()) {
          // k is always std::string.

          if (d.handle_json(k, v, j)) {
            // Do nothing
          } else {
            throw pqrs::json::unmarshal_error(fmt::format("unknown key: `{0}` in `{1}", k, j.dump()));
          }
        }

        if (d.get_type() == event_definition::type::none) {
          throw pqrs::json::unmarshal_error(fmt::format("event type is invalid: `{0}`", json.dump()));
        }

        event_definitions.push_back(d);
      }

    } else if (key == "simultaneous_options") {
      try {
        d.set_simultaneous_options(value.get<simultaneous_options>());
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
      }

    } else {
      throw pqrs::json::unmarshal_error(fmt::format("`from` error: unknown key `{0}` in `{1}`", key, json.dump()));
    }
  }

  if (event_definitions.empty() &&
      default_event_definition.get_type() != event_definition::type::none) {
    event_definitions.push_back(default_event_definition);
  }

  // ----------------------------------------

  if (event_definitions.empty()) {
    throw pqrs::json::unmarshal_error(fmt::format("event is not specified: `{0}`", json.dump()));
  }

  for (const auto& d : event_definitions) {
    switch (d.get_type()) {
      case event_definition::type::key_code:
      case event_definition::type::consumer_key_code:
      case event_definition::type::pointing_button:
      case event_definition::type::any:
        break;

      case event_definition::type::none:
      case event_definition::type::shell_command:
      case event_definition::type::select_input_source:
      case event_definition::type::set_variable:
      case event_definition::type::mouse_key:
        throw pqrs::json::unmarshal_error(fmt::format("event type is invalid: `{0}`", json.dump()));
    }
  }

  d.set_event_definitions(event_definitions);
  d.set_from_modifiers_definition(from_modifiers_definition);
}
} // namespace basic
} // namespace manipulators
} // namespace manipulator
} // namespace krbn
