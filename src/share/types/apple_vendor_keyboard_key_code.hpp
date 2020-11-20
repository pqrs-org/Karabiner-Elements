#pragma once

#include <cstdint>
#include <mapbox/eternal.hpp>
#include <pqrs/osx/iokit_hid_value.hpp>
#include <spdlog/fmt/fmt.h>

namespace krbn {
namespace apple_vendor_keyboard_key_code {
struct value_t : type_safe::strong_typedef<value_t, uint32_t>,
                 type_safe::strong_typedef_op::equality_comparison<value_t>,
                 type_safe::strong_typedef_op::relational_comparison<value_t>,
                 type_safe::strong_typedef_op::integer_arithmetic<value_t> {
  using strong_typedef::strong_typedef;
};

inline std::ostream& operator<<(std::ostream& stream, const value_t& value) {
  return stream << type_safe::get(value);
}

//
// Values
//

constexpr value_t spotlight(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::spotlight));
constexpr value_t dashboard(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::dashboard));
constexpr value_t function(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::function));
constexpr value_t launchpad(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::launchpad));
constexpr value_t reserved(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::reserved));
constexpr value_t caps_lock_delay_enable(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::caps_lock_delay_enable));
constexpr value_t power_state(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::power_state));
constexpr value_t expose_all(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::expose_all));
constexpr value_t expose_desktop(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::expose_desktop));
constexpr value_t brightness_up(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::brightness_up));
constexpr value_t brightness_down(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::brightness_down));
constexpr value_t language(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::language));

namespace impl {
constexpr std::pair<const mapbox::eternal::string, const value_t> name_value_pairs[] = {
    {"spotlight", spotlight},
    {"dashboard", dashboard},
    {"function", function},
    {"launchpad", launchpad},
    {"reserved", reserved},
    {"caps_lock_delay_enable", caps_lock_delay_enable},
    {"power_state", power_state},
    {"expose_all", expose_all},
    {"expose_desktop", expose_desktop},
    {"brightness_up", brightness_up},
    {"brightness_down", brightness_down},
    {"language", language},
};

constexpr auto name_value_map = mapbox::eternal::hash_map<mapbox::eternal::string, value_t>(name_value_pairs);

#define USAGE_VALUE_PAIR(name) \
  { pqrs::hid::usage::apple_vendor_keyboard::name, apple_vendor_keyboard_key_code::name }

constexpr auto usage_page_apple_vendor_keyboard_map = mapbox::eternal::map<pqrs::hid::usage::value_t, value_t>({
    USAGE_VALUE_PAIR(spotlight),
    USAGE_VALUE_PAIR(dashboard),
    USAGE_VALUE_PAIR(function),
    USAGE_VALUE_PAIR(launchpad),
    USAGE_VALUE_PAIR(reserved),
    USAGE_VALUE_PAIR(caps_lock_delay_enable),
    USAGE_VALUE_PAIR(power_state),
    USAGE_VALUE_PAIR(expose_all),
    USAGE_VALUE_PAIR(expose_desktop),
    USAGE_VALUE_PAIR(brightness_up),
    USAGE_VALUE_PAIR(brightness_down),
    USAGE_VALUE_PAIR(language),
});

#undef USAGE_VALUE_PAIR
} // namespace impl
} // namespace apple_vendor_keyboard_key_code

inline std::optional<apple_vendor_keyboard_key_code::value_t> make_apple_vendor_keyboard_key_code(const std::string& name) {
  auto& map = apple_vendor_keyboard_key_code::impl::name_value_map;
  auto it = map.find(name.c_str());
  if (it == map.end()) {
    return std::nullopt;
  }
  return it->second;
}

inline std::string make_apple_vendor_keyboard_key_code_name(apple_vendor_keyboard_key_code::value_t apple_vendor_keyboard_key_code) {
  for (const auto& pair : apple_vendor_keyboard_key_code::impl::name_value_pairs) {
    if (pair.second == apple_vendor_keyboard_key_code) {
      return pair.first.c_str();
    }
  }
  return fmt::format("(number:{0})", type_safe::get(apple_vendor_keyboard_key_code));
}

inline std::optional<apple_vendor_keyboard_key_code::value_t> find_unnamed_apple_vendor_keyboard_key_code_number(const std::string& name) {
  std::string_view prefix("(number:");

  if (pqrs::string::starts_with(name, prefix)) {
    try {
      return apple_vendor_keyboard_key_code::value_t(std::stoi(name.substr(prefix.size())));
    } catch (const std::exception& e) {
    }
  }

  return std::nullopt;
}

inline std::optional<apple_vendor_keyboard_key_code::value_t> make_apple_vendor_keyboard_key_code(pqrs::hid::usage_page::value_t usage_page,
                                                                                                  pqrs::hid::usage::value_t usage) {
  if (usage_page == pqrs::hid::usage_page::apple_vendor_keyboard) {
    // Note:
    // We need to check usage value since some usages are not apple_vendor_keyboard key event.
    // (e.g., pqrs::hid::usage::apple_vendor_keyboard::ac_pan is horizontal wheel.)

    auto& map = apple_vendor_keyboard_key_code::impl::usage_page_apple_vendor_keyboard_map;
    auto it = map.find(usage);
    if (it != map.end()) {
      return it->second;
    }
  }

  return std::nullopt;
}

inline std::optional<apple_vendor_keyboard_key_code::value_t> make_apple_vendor_keyboard_key_code(const pqrs::osx::iokit_hid_value& hid_value) {
  if (auto usage_page = hid_value.get_usage_page()) {
    if (auto usage = hid_value.get_usage()) {
      return make_apple_vendor_keyboard_key_code(*usage_page,
                                                 *usage);
    }
  }
  return std::nullopt;
}

inline std::optional<pqrs::hid::usage_page::value_t> make_hid_usage_page(apple_vendor_keyboard_key_code::value_t apple_vendor_keyboard_key_code) {
  return pqrs::hid::usage_page::apple_vendor_keyboard;
}

inline std::optional<pqrs::hid::usage::value_t> make_hid_usage(apple_vendor_keyboard_key_code::value_t apple_vendor_keyboard_key_code) {
  return pqrs::hid::usage::value_t(type_safe::get(apple_vendor_keyboard_key_code));
}

namespace apple_vendor_keyboard_key_code {
inline void from_json(const nlohmann::json& json, apple_vendor_keyboard_key_code::value_t& value) {
  if (json.is_string()) {
    if (auto v = make_apple_vendor_keyboard_key_code(json.get<std::string>())) {
      value = *v;
    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown apple_vendor_keyboard_key_code: `{0}`", pqrs::json::dump_for_error_message(json)));
    }
  } else if (json.is_number()) {
    value = apple_vendor_keyboard_key_code::value_t(json.get<type_safe::underlying_type<value_t>>());
  } else {
    throw pqrs::json::unmarshal_error(fmt::format("json must be string or number, but is `{0}`", pqrs::json::dump_for_error_message(json)));
  }
}
} // namespace apple_vendor_keyboard_key_code
} // namespace krbn

namespace std {
template <>
struct hash<krbn::apple_vendor_keyboard_key_code::value_t> : type_safe::hashable<krbn::apple_vendor_keyboard_key_code::value_t> {
};
} // namespace std
