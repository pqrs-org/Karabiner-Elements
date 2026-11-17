#pragma once

#include "event_definition.hpp"
#include "event_queue.hpp"
#include "event_variable_interpolation.hpp"
#include "manipulator/condition_factory.hpp"
#include "manipulator/condition_manager.hpp"
#include "modifier_definition.hpp"
#include <algorithm>
#include <optional>
#include <set>

namespace krbn::manipulator {
class to_event_definition final {
public:
  to_event_definition(const to_event_definition&) = delete;

  to_event_definition() = default;

  to_event_definition(const nlohmann::json& json)
      : to_event_definition() {
    pqrs::json::requires_object(json, "json");

    for (const auto& [key, value] : json.items()) {
      if (event_definition_.handle_json(key, value, json)) {
        // Do nothing

      } else if (key == "modifiers") {
        try {
          modifiers_ = modifier_definition::make_modifiers(value);
        } catch (const pqrs::json::unmarshal_error& e) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", key, e.what()));
        }

      } else if (key == "lazy") {
        pqrs::json::requires_boolean(value, "`" + key + "`");

        lazy_ = value.get<bool>();

      } else if (key == "repeat") {
        pqrs::json::requires_boolean(value, "`" + key + "`");

        repeat_ = value.get<bool>();

      } else if (key == "halt") {
        pqrs::json::requires_boolean(value, "`" + key + "`");

        halt_ = value.get<bool>();

      } else if (key == "hold_down_milliseconds" ||
                 key == "held_down_milliseconds") {
        pqrs::json::requires_number(value, "`" + key + "`");

        hold_down_milliseconds_ = std::chrono::milliseconds(value.get<int>());

      } else if (key == "conditions") {
        pqrs::json::requires_array(value, "`" + key + "`");

        for (const auto& j : value) {
          condition_manager_.push_back_condition(condition_factory::make_condition(j));
        }

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, pqrs::json::dump_for_error_message(json)));
      }
    }

    // ----------------------------------------

    switch (event_definition_.get_type()) {
      case event_definition::type::momentary_switch_event:
      case event_definition::type::from_event:
      case event_definition::type::shell_command:
      case event_definition::type::send_user_command:
      case event_definition::type::select_input_source:
      case event_definition::type::set_variable:
      case event_definition::type::set_notification_message:
      case event_definition::type::set_caps_lock_led:
      case event_definition::type::mouse_key:
      case event_definition::type::sticky_modifier:
      case event_definition::type::software_function:
        break;

      case event_definition::type::none:
      case event_definition::type::any:
        throw pqrs::json::unmarshal_error(fmt::format("event type is invalid: `{0}`", pqrs::json::dump_for_error_message(json)));
    }

    if (auto event = event_definition_.get_if<momentary_switch_event>();
        event && event->logitech_hidpp_button()) {
      throw pqrs::json::unmarshal_error("`logitech_hidpp_button` is available only in `from`");
    }
  }

  ~to_event_definition() {
  }

  static pqrs::not_null_shared_ptr_t<to_event_definition> make_from_momentary_switch_event(const momentary_switch_event& value,
                                                                                           const to_event_definition& base) {
    auto result = pqrs::not_null_shared_ptr_t<to_event_definition>(std::make_shared<to_event_definition>());

    result->event_definition_ = event_definition::make_from_momentary_switch_event(value);
    result->set_modifiers(base.get_modifiers());
    result->set_lazy(base.get_lazy());
    result->set_repeat(base.get_repeat());
    result->set_halt(base.get_halt());
    result->set_hold_down_milliseconds(base.get_hold_down_milliseconds());
    for (const auto& condition : base.get_condition_manager().get_conditions()) {
      result->get_condition_manager().push_back_condition(condition);
    }

    return result;
  }

  // Returns a copy of `base` in which `${key_code}` in `shell_command` or `set_notification_message.text`
  // is replaced with the name of `pressed_event` (e.g. `tab`, `f12`, `button1`).
  //
  // Returns std::nullopt if `base` is of another type or if nothing was replaced,
  // in which case the caller should keep using `base` as it is.
  [[nodiscard]] static std::optional<pqrs::not_null_shared_ptr_t<to_event_definition>> make_with_expanded_variables(
      const to_event_definition& base,
      const momentary_switch_event& pressed_event) {
    // The name is the value of the single key of the json form of the event,
    // e.g. `{"key_code": "tab"}` -> `tab`.
    // Events without a name (e.g. `logitech_hidpp_button`, which is a number) are not expanded.
    std::string key_code_name;
    {
      nlohmann::json json = pressed_event;
      if (json.is_object() && json.size() == 1) {
        const auto& value = json.begin().value();
        if (value.is_string()) {
          key_code_name = value.get<std::string>();
        }
      }
    }

    if (key_code_name.empty()) {
      return std::nullopt;
    }

    const event_variable_interpolation::variables variables{
        {"key_code", key_code_name},
    };

    std::optional<event_definition> expanded;

    if (auto s = base.event_definition_.get_if<std::string>();
        s && base.event_definition_.get_type() == event_definition::type::shell_command) {
      auto text = event_variable_interpolation::expand(*s, variables);
      if (text != *s) {
        expanded = event_definition::make_shell_command(text);
      }

    } else if (auto m = base.event_definition_.get_if<notification_message>();
               m && base.event_definition_.get_type() == event_definition::type::set_notification_message) {
      auto text = event_variable_interpolation::expand(m->get_text(), variables);
      if (text != m->get_text()) {
        auto copy = *m;
        copy.set_text(text);
        expanded = event_definition::make_set_notification_message(copy);
      }
    }

    if (!expanded) {
      return std::nullopt;
    }

    auto result = pqrs::not_null_shared_ptr_t<to_event_definition>(std::make_shared<to_event_definition>());

    result->event_definition_ = *expanded;
    result->set_modifiers(base.get_modifiers());
    result->set_lazy(base.get_lazy());
    result->set_repeat(base.get_repeat());
    result->set_halt(base.get_halt());
    result->set_hold_down_milliseconds(base.get_hold_down_milliseconds());
    for (const auto& condition : base.get_condition_manager().get_conditions()) {
      result->get_condition_manager().push_back_condition(condition);
    }

    return result;
  }

  [[nodiscard]] const event_definition& get_event_definition() const {
    return event_definition_;
  }

  [[nodiscard]] event_definition& get_event_definition() {
    return const_cast<event_definition&>(
        static_cast<const to_event_definition&>(*this).get_event_definition());
  }

  [[nodiscard]] const std::set<modifier_definition::modifier>& get_modifiers() const {
    return modifiers_;
  }

  void set_modifiers(const std::set<modifier_definition::modifier>& value) {
    modifiers_ = value;
  }

  [[nodiscard]] bool get_lazy() const {
    return lazy_;
  }

  void set_lazy(bool value) {
    lazy_ = value;
  }

  [[nodiscard]] bool get_repeat() const {
    return repeat_;
  }

  void set_repeat(bool value) {
    repeat_ = value;
  }

  [[nodiscard]] bool get_halt() const {
    return halt_;
  }

  void set_halt(bool value) {
    halt_ = value;
  }

  [[nodiscard]] const std::chrono::milliseconds& get_hold_down_milliseconds() const {
    return hold_down_milliseconds_;
  }

  void set_hold_down_milliseconds(const std::chrono::milliseconds& value) {
    hold_down_milliseconds_ = value;
  }

  [[nodiscard]] const condition_manager& get_condition_manager() const {
    return condition_manager_;
  }
  [[nodiscard]] condition_manager& get_condition_manager() {
    return const_cast<condition_manager&>(static_cast<const to_event_definition&>(*this).get_condition_manager());
  }

  [[nodiscard]] bool needs_virtual_hid_pointing() const {
    if (auto e = event_definition_.get_if<momentary_switch_event>()) {
      if (e->pointing_button()) {
        return true;
      }
    }

    if (event_definition_.get_type() == event_definition::type::mouse_key) {
      return true;
    }

    return false;
  }

  [[nodiscard]] bool uses_caps_lock_led_override() const {
    return event_definition_.get_type() == event_definition::type::set_caps_lock_led;
  }

  std::vector<momentary_switch_event> make_modifier_events() const {
    std::vector<momentary_switch_event> events;

    for (const auto& modifier : modifiers_) {
      // `event_definition::get_modifiers` might return two modifier_flags.
      // (eg. `modifier_flag::left_shift` and `modifier_flag::right_shift` for `modifier::shift`.)
      // We use the first modifier_flag.

      auto modifier_flags = modifier_definition::get_modifier_flags(modifier);
      if (!modifier_flags.empty()) {
        auto modifier_flag = modifier_flags.front();
        momentary_switch_event e(modifier_flag);
        if (e.valid()) {
          events.push_back(e);
        }
      }
    }

    return events;
  }

private:
  event_definition event_definition_;
  std::set<modifier_definition::modifier> modifiers_;
  bool lazy_{false};
  bool repeat_{true};
  bool halt_{false};
  std::chrono::milliseconds hold_down_milliseconds_{0};
  condition_manager condition_manager_;
};

typedef std::vector<pqrs::not_null_shared_ptr_t<to_event_definition>> to_event_definitions;

template <typename Predicate, typename... EventDefinitions>
[[nodiscard]] bool any_event_definition_matches(const Predicate& predicate,
                                                const EventDefinitions&... event_definitions) {
  return (std::ranges::any_of(event_definitions,
                              [&](const auto& definition) {
                                return predicate(*definition);
                              }) ||
          ...);
}
} // namespace krbn::manipulator
