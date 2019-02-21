#pragma once

#include "event_definition.hpp"
#include "event_queue.hpp"
#include "modifier_definition.hpp"

namespace krbn {
namespace manipulator {
class to_event_definition final {
public:
  explicit to_event_definition(const nlohmann::json& json) : lazy_(false),
                                                             repeat_(true),
                                                             halt_(false),
                                                             hold_down_milliseconds_(0) {
    if (!json.is_object()) {
      throw pqrs::json::unmarshal_error(fmt::format("to_event_definition must be object, but is `{0}`", json.dump()));
    }

    for (const auto& [key, value] : json.items()) {
      // key is always std::string.

      if (event_definition_.handle_json(key, value, json)) {
        continue;
      }

      if (key == "modifiers") {
        modifiers_ = modifier_definition::make_modifiers(value, "modifiers");
        continue;
      }

      if (key == "lazy") {
        if (!value.is_boolean()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be boolean, but is `{1}`", key, value.dump()));
        }

        lazy_ = value;

        continue;
      }

      if (key == "repeat") {
        if (!value.is_boolean()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be boolean, but is `{1}`", key, value.dump()));
        }

        repeat_ = value;

        continue;
      }

      if (key == "halt") {
        if (!value.is_boolean()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be boolean, but is `{1}`", key, value.dump()));
        }

        halt_ = value;

        continue;
      }

      if (key == "hold_down_milliseconds" ||
          key == "held_down_milliseconds") {
        if (!value.is_number()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be number, but is `{1}`", key, value.dump()));
        }

        hold_down_milliseconds_ = std::chrono::milliseconds(value);

        continue;
      }

      throw pqrs::json::unmarshal_error(fmt::format("to_event_definition error: unknown key `{0}` in `{1}`", key, json.dump()));
    }

    // ----------------------------------------

    switch (event_definition_.get_type()) {
      case event_definition::type::key_code:
      case event_definition::type::consumer_key_code:
      case event_definition::type::pointing_button:
      case event_definition::type::shell_command:
      case event_definition::type::select_input_source:
      case event_definition::type::set_variable:
      case event_definition::type::mouse_key:
        break;

      case event_definition::type::none:
      case event_definition::type::any:
        throw pqrs::json::unmarshal_error(fmt::format("to_event_definition error: event type is invalid: `{0}`", json.dump()));
        break;
    }
  }

  virtual ~to_event_definition(void) {
  }

  const event_definition& get_event_definition(void) const {
    return event_definition_;
  }

  const std::unordered_set<modifier_definition::modifier>& get_modifiers(void) const {
    return modifiers_;
  }

  bool get_lazy(void) const {
    return lazy_;
  }

  bool get_repeat(void) const {
    return repeat_;
  }

  bool get_halt(void) const {
    return halt_;
  }

  std::chrono::milliseconds get_hold_down_milliseconds(void) const {
    return hold_down_milliseconds_;
  }

  bool needs_virtual_hid_pointing(void) const {
    if (event_definition_.get_type() == event_definition::type::pointing_button ||
        event_definition_.get_type() == event_definition::type::mouse_key) {
      return true;
    }
    return false;
  }

  std::vector<event_queue::event> make_modifier_events(void) const {
    std::vector<event_queue::event> events;

    for (const auto& modifier : modifiers_) {
      // `event_definition::get_modifiers` might return two modifier_flags.
      // (eg. `modifier_flag::left_shift` and `modifier_flag::right_shift` for `modifier::shift`.)
      // We use the first modifier_flag.

      auto modifier_flags = modifier_definition::get_modifier_flags(modifier);
      if (!modifier_flags.empty()) {
        auto modifier_flag = modifier_flags.front();
        if (auto key_code = types::make_key_code(modifier_flag)) {
          events.emplace_back(*key_code);
        }
      }
    }

    return events;
  }

private:
  event_definition event_definition_;
  std::unordered_set<modifier_definition::modifier> modifiers_;
  bool lazy_;
  bool repeat_;
  bool halt_;
  std::chrono::milliseconds hold_down_milliseconds_;
};
} // namespace manipulator
} // namespace krbn
