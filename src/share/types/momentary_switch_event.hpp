#pragma once

#include "momentary_switch_event_details/apple_vendor_keyboard_key_code.hpp"
#include "momentary_switch_event_details/apple_vendor_top_case_key_code.hpp"
#include "momentary_switch_event_details/consumer_key_code.hpp"
#include "momentary_switch_event_details/generic_desktop.hpp"
#include "momentary_switch_event_details/key_code.hpp"
#include "momentary_switch_event_details/pointing_button.hpp"
#include <nlohmann/json.hpp>
#include <ostream>
#include <pqrs/hash.hpp>
#include <variant>

namespace krbn {
/// Events from momentary switch hardwares such as key, consumer, pointing_button.
class momentary_switch_event final {
public:
  static bool target(pqrs::hid::usage_page::value_t usage_page,
                     pqrs::hid::usage::value_t usage) {
    return momentary_switch_event_details::key_code::target(usage_page, usage) ||
           momentary_switch_event_details::consumer_key_code::target(usage_page, usage) ||
           momentary_switch_event_details::apple_vendor_keyboard_key_code::target(usage_page, usage) ||
           momentary_switch_event_details::apple_vendor_top_case_key_code::target(usage_page, usage) ||
           momentary_switch_event_details::pointing_button::target(usage_page, usage) ||
           momentary_switch_event_details::generic_desktop::target(usage_page, usage);
  }

  momentary_switch_event(void) {
  }

  momentary_switch_event(pqrs::hid::usage_page::value_t usage_page,
                         pqrs::hid::usage::value_t usage)
      : usage_pair_(usage_page, usage) {
  }

  explicit momentary_switch_event(const pqrs::hid::usage_pair& usage_pair)
      : momentary_switch_event(usage_pair.get_usage_page(), usage_pair.get_usage()) {
  }

  explicit momentary_switch_event(const krbn::modifier_flag& modifier_flag) {
    switch (modifier_flag) {
      case modifier_flag::zero:
        break;
      case modifier_flag::caps_lock:
        usage_pair_ = pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_caps_lock);
        break;
      case modifier_flag::left_control:
        usage_pair_ = pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_left_control);
        break;
      case modifier_flag::left_shift:
        usage_pair_ = pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift);
        break;
      case modifier_flag::left_option:
        usage_pair_ = pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_left_alt);
        break;
      case modifier_flag::left_command:
        usage_pair_ = pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_left_gui);
        break;
      case modifier_flag::right_control:
        usage_pair_ = pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_right_control);
        break;
      case modifier_flag::right_shift:
        usage_pair_ = pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_right_shift);
        break;
      case modifier_flag::right_option:
        usage_pair_ = pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_right_alt);
        break;
      case modifier_flag::right_command:
        usage_pair_ = pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_right_gui);
        break;
      case modifier_flag::fn:
        usage_pair_ = pqrs::hid::usage_pair(pqrs::hid::usage_page::apple_vendor_top_case,
                                            pqrs::hid::usage::apple_vendor_top_case::keyboard_fn);
        break;
      case modifier_flag::end_:
        break;
    }
  }

  const pqrs::hid::usage_pair& get_usage_pair(void) const {
    return usage_pair_;
  }

  momentary_switch_event& set_usage_pair(const pqrs::hid::usage_pair& usage_pair) {
    usage_pair_ = usage_pair;

    return *this;
  }

  std::optional<krbn::modifier_flag> make_modifier_flag(void) const {
    auto usage_page = usage_pair_.get_usage_page();
    auto usage = usage_pair_.get_usage();

    if (usage_page == pqrs::hid::usage_page::keyboard_or_keypad) {
      if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_left_control) {
        return krbn::modifier_flag::left_control;
      } else if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift) {
        return krbn::modifier_flag::left_shift;
      } else if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_left_alt) {
        return krbn::modifier_flag::left_option;
      } else if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_left_gui) {
        return krbn::modifier_flag::left_command;
      } else if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_right_control) {
        return krbn::modifier_flag::right_control;
      } else if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_right_shift) {
        return krbn::modifier_flag::right_shift;
      } else if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_right_alt) {
        return krbn::modifier_flag::right_option;
      } else if (usage == pqrs::hid::usage::keyboard_or_keypad::keyboard_right_gui) {
        return krbn::modifier_flag::right_command;
      }
    } else if (usage_page == pqrs::hid::usage_page::apple_vendor_keyboard) {
      if (usage == pqrs::hid::usage::apple_vendor_keyboard::function) {
        return krbn::modifier_flag::fn;
      }
    } else if (usage_page == pqrs::hid::usage_page::apple_vendor_top_case) {
      if (usage == pqrs::hid::usage::apple_vendor_top_case::keyboard_fn) {
        return krbn::modifier_flag::fn;
      }
    }

    return std::nullopt;
  }

  bool valid(void) const {
    return usage_pair_.get_usage_page() != pqrs::hid::usage_page::undefined &&
           usage_pair_.get_usage() != pqrs::hid::usage::undefined;
  }

  bool modifier_flag(void) const {
    return make_modifier_flag() != std::nullopt;
  }

  bool caps_lock(void) const {
    return usage_pair_.get_usage_page() == pqrs::hid::usage_page::keyboard_or_keypad &&
           usage_pair_.get_usage() == pqrs::hid::usage::keyboard_or_keypad::keyboard_caps_lock;
  }

  bool pointing_button(void) const {
    return usage_pair_.get_usage_page() == pqrs::hid::usage_page::button;
  }

  // Return true if this event interrupts the current key repeat.
  // - true: Letter keys
  // - true: Number keys
  // - true: Controls and symbol keys (escape, return, tab, etc)
  // - true: Media control keys
  // - true: Arrow keys
  // - false: Modifier keys
  // - false: Pointing Buttons
  // - false: D-pad
  bool interrupts_key_repeat(void) const {
    if (!valid() ||
        modifier_flag() ||
        caps_lock() ||
        pointing_button() ||
        usage_pair_.get_usage_page() == pqrs::hid::usage_page::generic_desktop) {
      return false;
    }

    return true;
  }

  auto operator<=>(const momentary_switch_event&) const = default;

private:
  pqrs::hid::usage_pair usage_pair_;
};

inline void to_json(nlohmann::json& json, const momentary_switch_event& value) {
  auto usage_page = value.get_usage_pair().get_usage_page();
  auto usage = value.get_usage_pair().get_usage();

  if (momentary_switch_event_details::key_code::target(usage_page, usage)) {
    json["key_code"] = momentary_switch_event_details::key_code::make_name(usage);

  } else if (momentary_switch_event_details::consumer_key_code::target(usage_page, usage)) {
    json["consumer_key_code"] = momentary_switch_event_details::consumer_key_code::make_name(usage);

  } else if (momentary_switch_event_details::apple_vendor_keyboard_key_code::target(usage_page, usage)) {
    json["apple_vendor_keyboard_key_code"] = momentary_switch_event_details::apple_vendor_keyboard_key_code::make_name(usage);

  } else if (momentary_switch_event_details::apple_vendor_top_case_key_code::target(usage_page, usage)) {
    json["apple_vendor_top_case_key_code"] = momentary_switch_event_details::apple_vendor_top_case_key_code::make_name(usage);

  } else if (momentary_switch_event_details::pointing_button::target(usage_page, usage)) {
    json["pointing_button"] = momentary_switch_event_details::pointing_button::make_name(usage);

  } else if (momentary_switch_event_details::generic_desktop::target(usage_page, usage)) {
    json["generic_desktop"] = momentary_switch_event_details::generic_desktop::make_name(usage);

  } else {
    json = "unsupported";
  }
}

inline void from_json(const nlohmann::json& json, momentary_switch_event& value) {
  pqrs::json::requires_object(json, "json");

  for (const auto& [k, v] : json.items()) {
    if (k == "key_code") {
      value.set_usage_pair(momentary_switch_event_details::key_code::make_usage_pair(k, v));

    } else if (k == "consumer_key_code") {
      value.set_usage_pair(momentary_switch_event_details::consumer_key_code::make_usage_pair(k, v));

    } else if (k == "apple_vendor_keyboard_key_code") {
      value.set_usage_pair(momentary_switch_event_details::apple_vendor_keyboard_key_code::make_usage_pair(k, v));

    } else if (k == "apple_vendor_top_case_key_code") {
      value.set_usage_pair(momentary_switch_event_details::apple_vendor_top_case_key_code::make_usage_pair(k, v));

    } else if (k == "pointing_button") {
      value.set_usage_pair(momentary_switch_event_details::pointing_button::make_usage_pair(k, v));

    } else if (k == "generic_desktop") {
      value.set_usage_pair(momentary_switch_event_details::generic_desktop::make_usage_pair(k, v));

    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown key: `{0}`", k));
    }
  }
}

inline std::ostream& operator<<(std::ostream& stream, const momentary_switch_event& value) {
  stream << nlohmann::json(value);
  return stream;
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::momentary_switch_event> final {
  std::size_t operator()(const krbn::momentary_switch_event& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_usage_pair());

    return h;
  }
};
} // namespace std
