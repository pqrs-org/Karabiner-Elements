#pragma once

#include "momentary_switch_event/apple_vendor_keyboard_key_code.hpp"
#include "momentary_switch_event/apple_vendor_top_case_key_code.hpp"
#include "momentary_switch_event/consumer_key_code.hpp"
#include "momentary_switch_event/key_code.hpp"
#include "momentary_switch_event/pointing_button.hpp"
#include <nlohmann/json.hpp>
#include <pqrs/hash.hpp>
#include <variant>

namespace krbn {
/// Events from momentary switch hardwares such as key, consumer, pointing_button.
class momentary_switch_event final {
public:
  using value_t = std::variant<key_code::value_t,
                               consumer_key_code::value_t,
                               std::monostate>;

  momentary_switch_event(void) : value_(std::monostate()) {
  }

  template <typename T>
  explicit momentary_switch_event(const T& value) : value_(value) {
  }

  explicit momentary_switch_event(pqrs::hid::usage_page::value_t usage_page,
                                  pqrs::hid::usage::value_t usage) : value_(std::monostate()) {
    if (auto key_code = make_key_code(usage_page, usage)) {
      value_ = *key_code;
    } else if (auto consumer_key_code = make_consumer_key_code(usage_page, usage)) {
      value_ = *consumer_key_code;

    } else if (usage_page == pqrs::hid::usage_page::apple_vendor_keyboard &&
               momentary_switch_event_details::apple_vendor_keyboard_key_code::target(usage)) {
      usage_pair_ = pqrs::hid::usage_pair(usage_page, usage);

    } else if (usage_page == pqrs::hid::usage_page::apple_vendor_top_case &&
               momentary_switch_event_details::apple_vendor_top_case_key_code::target(usage)) {
      usage_pair_ = pqrs::hid::usage_pair(usage_page, usage);

    } else if (usage_page == pqrs::hid::usage_page::button &&
               momentary_switch_event_details::pointing_button::target(usage)) {
      usage_pair_ = pqrs::hid::usage_pair(usage_page, usage);
    }
  }

  explicit momentary_switch_event(const pqrs::hid::usage_pair& usage_pair) : momentary_switch_event(usage_pair.get_usage_page(), usage_pair.get_usage()) {
  }

  const value_t& get_value(void) const {
    return value_;
  }

  template <typename T>
  void set_value(T value) {
    value_ = value;
  }

  const pqrs::hid::usage_pair& get_usage_pair(void) const {
    return usage_pair_;
  }

  momentary_switch_event& set_usage_pair(const pqrs::hid::usage_pair& usage_pair) {
    usage_pair_ = usage_pair;

    return *this;
  }

  template <typename T>
  const T* get_if(void) const {
    return std::get_if<T>(&value_);
  }

  std::optional<pqrs::hid::usage_pair> make_usage_pair(void) const {
    std::optional<pqrs::hid::usage_page::value_t> usage_page;
    std::optional<pqrs::hid::usage::value_t> usage;

    if (auto value = get_if<key_code::value_t>()) {
      usage_page = make_hid_usage_page(*value);
      usage = make_hid_usage(*value);
    } else if (auto value = get_if<consumer_key_code::value_t>()) {
      usage_page = make_hid_usage_page(*value);
      usage = make_hid_usage(*value);
    }

    if (usage_page && usage) {
      return pqrs::hid::usage_pair(*usage_page, *usage);
    }

    if (usage_pair_.get_usage_page() == pqrs::hid::usage_page::undefined ||
        usage_pair_.get_usage() == pqrs::hid::usage::undefined) {
      return std::nullopt;
    }

    return usage_pair_;
  }

  std::optional<krbn::modifier_flag> make_modifier_flag(void) const {
    if (auto usage_pair = make_usage_pair()) {
      auto usage_page = usage_pair->get_usage_page();
      auto usage = usage_pair->get_usage();

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
    }

    return std::nullopt;
  }

  bool modifier_flag(void) const {
    return make_modifier_flag() != std::nullopt;
  }

  bool pointing_button(void) const {
    return usage_pair_.get_usage_page() == pqrs::hid::usage_page::button;
  }

  bool operator==(const momentary_switch_event& other) const {
    return value_ == other.value_ && usage_pair_ == other.usage_pair_;
  }

  bool operator<(const momentary_switch_event& other) const {
    if (value_ != other.value_) {
      return value_ < other.value_;
    }
    return usage_pair_ < other.usage_pair_;
  }

private:
  value_t value_;
  pqrs::hid::usage_pair usage_pair_;
};

inline void to_json(nlohmann::json& json, const momentary_switch_event& value) {
  auto usage_page = value.get_usage_pair().get_usage_page();
  auto usage = value.get_usage_pair().get_usage();

  if (auto v = value.get_if<key_code::value_t>()) {
    json["key_code"] = make_key_code_name(*v);

  } else if (auto v = value.get_if<consumer_key_code::value_t>()) {
    json["consumer_key_code"] = make_consumer_key_code_name(*v);

  } else if (usage_page == pqrs::hid::usage_page::apple_vendor_keyboard) {
    json["apple_vendor_keyboard_key_code"] = momentary_switch_event_details::apple_vendor_keyboard_key_code::make_name(usage);

  } else if (usage_page == pqrs::hid::usage_page::apple_vendor_top_case) {
    json["apple_vendor_top_case_key_code"] = momentary_switch_event_details::apple_vendor_top_case_key_code::make_name(usage);

  } else if (usage_page == pqrs::hid::usage_page::button) {
    json["pointing_button"] = momentary_switch_event_details::pointing_button::make_name(usage);
  }
}

inline void from_json(const nlohmann::json& json, momentary_switch_event& value) {
  pqrs::json::requires_object(json, "json");

  for (const auto& [k, v] : json.items()) {
    if (k == "key_code") {
      value.set_value(v.get<key_code::value_t>());

    } else if (k == "consumer_key_code") {
      value.set_value(v.get<consumer_key_code::value_t>());

    } else if (k == "apple_vendor_keyboard_key_code") {
      value.set_usage_pair(momentary_switch_event_details::apple_vendor_keyboard_key_code::make_usage_pair(k, v));

    } else if (k == "apple_vendor_top_case_key_code") {
      value.set_usage_pair(momentary_switch_event_details::apple_vendor_top_case_key_code::make_usage_pair(k, v));

    } else if (k == "pointing_button") {
      value.set_usage_pair(momentary_switch_event_details::pointing_button::make_usage_pair(k, v));

    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown key: `{0}`", k));
    }
  }
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::momentary_switch_event> final {
  std::size_t operator()(const krbn::momentary_switch_event& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_value());
    pqrs::hash::combine(h, value.get_usage_pair());

    return h;
  }
};
} // namespace std
