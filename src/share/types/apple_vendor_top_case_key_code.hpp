#pragma once

#include <cstdint>
#include <mapbox/eternal.hpp>
#include <pqrs/osx/iokit_hid_value.hpp>
#include <spdlog/fmt/fmt.h>

namespace krbn {
namespace apple_vendor_top_case_key_code {
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

constexpr value_t keyboard_fn(type_safe::get(pqrs::hid::usage::apple_vendor_top_case::keyboard_fn));
constexpr value_t brightness_up(type_safe::get(pqrs::hid::usage::apple_vendor_top_case::brightness_up));
constexpr value_t brightness_down(type_safe::get(pqrs::hid::usage::apple_vendor_top_case::brightness_down));
constexpr value_t video_mirror(type_safe::get(pqrs::hid::usage::apple_vendor_top_case::video_mirror));
constexpr value_t illumination_toggle(type_safe::get(pqrs::hid::usage::apple_vendor_top_case::illumination_toggle));
constexpr value_t illumination_up(type_safe::get(pqrs::hid::usage::apple_vendor_top_case::illumination_up));
constexpr value_t illumination_down(type_safe::get(pqrs::hid::usage::apple_vendor_top_case::illumination_down));
constexpr value_t clamshell_latched(type_safe::get(pqrs::hid::usage::apple_vendor_top_case::clamshell_latched));
constexpr value_t reserved_mouse_data(type_safe::get(pqrs::hid::usage::apple_vendor_top_case::reserved_mouse_data));

namespace impl {
constexpr std::pair<const mapbox::eternal::string, const value_t> name_value_pairs[] = {
    {"keyboard_fn", keyboard_fn},
    {"brightness_up", brightness_up},
    {"brightness_down", brightness_down},
    {"video_mirror", video_mirror},
    {"illumination_toggle", illumination_toggle},
    {"illumination_up", illumination_up},
    {"illumination_down", illumination_down},
    {"clamshell_latched", clamshell_latched},
    {"reserved_mouse_data", reserved_mouse_data},
};

constexpr auto name_value_map = mapbox::eternal::hash_map<mapbox::eternal::string, value_t>(name_value_pairs);

#define USAGE_VALUE_PAIR(name) \
  { pqrs::hid::usage::apple_vendor_top_case::name, apple_vendor_top_case_key_code::name }

constexpr auto usage_page_apple_vendor_top_case_map = mapbox::eternal::map<pqrs::hid::usage::value_t, value_t>({
    USAGE_VALUE_PAIR(keyboard_fn),
    USAGE_VALUE_PAIR(brightness_up),
    USAGE_VALUE_PAIR(brightness_down),
    USAGE_VALUE_PAIR(video_mirror),
    USAGE_VALUE_PAIR(illumination_toggle),
    USAGE_VALUE_PAIR(illumination_up),
    USAGE_VALUE_PAIR(illumination_down),
    USAGE_VALUE_PAIR(clamshell_latched),
    USAGE_VALUE_PAIR(reserved_mouse_data),
});

#undef USAGE_VALUE_PAIR
} // namespace impl
} // namespace apple_vendor_top_case_key_code

inline std::optional<apple_vendor_top_case_key_code::value_t> make_apple_vendor_top_case_key_code(const std::string& name) {
  auto& map = apple_vendor_top_case_key_code::impl::name_value_map;
  auto it = map.find(name.c_str());
  if (it == map.end()) {
    return std::nullopt;
  }
  return it->second;
}

inline std::string make_apple_vendor_top_case_key_code_name(apple_vendor_top_case_key_code::value_t apple_vendor_top_case_key_code) {
  for (const auto& pair : apple_vendor_top_case_key_code::impl::name_value_pairs) {
    if (pair.second == apple_vendor_top_case_key_code) {
      return pair.first.c_str();
    }
  }
  return fmt::format("(number:{0})", type_safe::get(apple_vendor_top_case_key_code));
}

inline std::optional<apple_vendor_top_case_key_code::value_t> find_unnamed_apple_vendor_top_case_key_code_number(const std::string& name) {
  std::string_view prefix("(number:");

  if (pqrs::string::starts_with(name, prefix)) {
    try {
      return apple_vendor_top_case_key_code::value_t(std::stoi(name.substr(prefix.size())));
    } catch (const std::exception& e) {
    }
  }

  return std::nullopt;
}

inline std::optional<apple_vendor_top_case_key_code::value_t> make_apple_vendor_top_case_key_code(pqrs::hid::usage_page::value_t usage_page,
                                                                                                  pqrs::hid::usage::value_t usage) {
  if (usage_page == pqrs::hid::usage_page::apple_vendor_top_case) {
    // Note:
    // We need to check usage value since some usages are not apple_vendor_top_case key event.
    // (e.g., pqrs::hid::usage::apple_vendor_top_case::ac_pan is horizontal wheel.)

    auto& map = apple_vendor_top_case_key_code::impl::usage_page_apple_vendor_top_case_map;
    auto it = map.find(usage);
    if (it != map.end()) {
      return it->second;
    }
  }

  return std::nullopt;
}

inline std::optional<apple_vendor_top_case_key_code::value_t> make_apple_vendor_top_case_key_code(const pqrs::osx::iokit_hid_value& hid_value) {
  if (auto usage_page = hid_value.get_usage_page()) {
    if (auto usage = hid_value.get_usage()) {
      return make_apple_vendor_top_case_key_code(*usage_page,
                                                 *usage);
    }
  }
  return std::nullopt;
}

inline std::optional<pqrs::hid::usage_page::value_t> make_hid_usage_page(apple_vendor_top_case_key_code::value_t apple_vendor_top_case_key_code) {
  return pqrs::hid::usage_page::apple_vendor_top_case;
}

inline std::optional<pqrs::hid::usage::value_t> make_hid_usage(apple_vendor_top_case_key_code::value_t apple_vendor_top_case_key_code) {
  return pqrs::hid::usage::value_t(type_safe::get(apple_vendor_top_case_key_code));
}

namespace apple_vendor_top_case_key_code {
inline void from_json(const nlohmann::json& json, apple_vendor_top_case_key_code::value_t& value) {
  if (json.is_string()) {
    if (auto v = make_apple_vendor_top_case_key_code(json.get<std::string>())) {
      value = *v;
    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown apple_vendor_top_case_key_code: `{0}`", pqrs::json::dump_for_error_message(json)));
    }
  } else if (json.is_number()) {
    value = apple_vendor_top_case_key_code::value_t(json.get<type_safe::underlying_type<value_t>>());
  } else {
    throw pqrs::json::unmarshal_error(fmt::format("json must be string or number, but is `{0}`", pqrs::json::dump_for_error_message(json)));
  }
}
} // namespace apple_vendor_top_case_key_code
} // namespace krbn

namespace std {
template <>
struct hash<krbn::apple_vendor_top_case_key_code::value_t> : type_safe::hashable<krbn::apple_vendor_top_case_key_code::value_t> {
};
} // namespace std
