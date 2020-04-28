#pragma once

#include "event_definition.hpp"
#include "event_queue.hpp"
#include "modifier_definition.hpp"
#include <set>

namespace krbn {
namespace manipulator {
class to_event_definition final {
public:
  to_event_definition(void) : lazy_(false),
                              repeat_(true),
                              halt_(false),
                              hold_down_milliseconds_(0) {
  }

  virtual ~to_event_definition(void) {
  }

  const event_definition& get_event_definition(void) const {
    return event_definition_;
  }

  event_definition& get_event_definition(void) {
    return const_cast<event_definition&>(
        static_cast<const to_event_definition&>(*this).get_event_definition());
  }

  const std::set<modifier_definition::modifier>& get_modifiers(void) const {
    return modifiers_;
  }

  void set_modifiers(const std::set<modifier_definition::modifier>& value) {
    modifiers_ = value;
  }

  bool get_lazy(void) const {
    return lazy_;
  }

  void set_lazy(bool value) {
    lazy_ = value;
  }

  bool get_repeat(void) const {
    return repeat_;
  }

  void set_repeat(bool value) {
    repeat_ = value;
  }

  bool get_halt(void) const {
    return halt_;
  }

  void set_halt(bool value) {
    halt_ = value;
  }

  const std::chrono::milliseconds& get_hold_down_milliseconds(void) const {
    return hold_down_milliseconds_;
  }

  void set_hold_down_milliseconds(const std::chrono::milliseconds& value) {
    hold_down_milliseconds_ = value;
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
        if (auto key_code = make_key_code(modifier_flag)) {
          events.emplace_back(*key_code);
        }
      }
    }

    return events;
  }

private:
  event_definition event_definition_;
  std::set<modifier_definition::modifier> modifiers_;
  bool lazy_;
  bool repeat_;
  bool halt_;
  std::chrono::milliseconds hold_down_milliseconds_;
};

inline void from_json(const nlohmann::json& json, to_event_definition& d) {
  pqrs::json::requires_object(json, "json");

  for (const auto& [key, value] : json.items()) {
    if (d.get_event_definition().handle_json(key, value, json)) {
      // Do nothing

    } else if (key == "modifiers") {
      try {
        d.set_modifiers(modifier_definition::make_modifiers(value));
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
      }

    } else if (key == "lazy") {
      pqrs::json::requires_boolean(value, "`" + key + "`");

      d.set_lazy(value.get<bool>());

    } else if (key == "repeat") {
      pqrs::json::requires_boolean(value, "`" + key + "`");

      d.set_repeat(value.get<bool>());

    } else if (key == "halt") {
      pqrs::json::requires_boolean(value, "`" + key + "`");

      d.set_halt(value.get<bool>());

    } else if (key == "hold_down_milliseconds" ||
               key == "held_down_milliseconds") {
      pqrs::json::requires_number(value, "`" + key + "`");

      d.set_hold_down_milliseconds(std::chrono::milliseconds(value.get<int>()));

    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, pqrs::json::dump_for_error_message(json)));
    }
  }

  // ----------------------------------------

  switch (d.get_event_definition().get_type()) {
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
      throw pqrs::json::unmarshal_error(fmt::format("event type is invalid: `{0}`", pqrs::json::dump_for_error_message(json)));
  }
}
} // namespace manipulator
} // namespace krbn
