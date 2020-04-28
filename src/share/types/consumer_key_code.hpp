#pragma once

#include <cstdint>
#include <mapbox/eternal.hpp>
#include <pqrs/osx/iokit_hid_value.hpp>
#include <spdlog/fmt/fmt.h>

namespace krbn {
namespace consumer_key_code {
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

constexpr value_t power(type_safe::get(pqrs::hid::usage::consumer::power));
constexpr value_t display_brightness_increment(type_safe::get(pqrs::hid::usage::consumer::display_brightness_increment));
constexpr value_t display_brightness_decrement(type_safe::get(pqrs::hid::usage::consumer::display_brightness_decrement));
constexpr value_t fast_forward(type_safe::get(pqrs::hid::usage::consumer::fast_forward));
constexpr value_t rewind(type_safe::get(pqrs::hid::usage::consumer::rewind));
constexpr value_t scan_next_track(type_safe::get(pqrs::hid::usage::consumer::scan_next_track));
constexpr value_t scan_previous_track(type_safe::get(pqrs::hid::usage::consumer::scan_previous_track));
constexpr value_t eject(type_safe::get(pqrs::hid::usage::consumer::eject));
constexpr value_t play_or_pause(type_safe::get(pqrs::hid::usage::consumer::play_or_pause));
constexpr value_t mute(type_safe::get(pqrs::hid::usage::consumer::mute));
constexpr value_t volume_increment(type_safe::get(pqrs::hid::usage::consumer::volume_increment));
constexpr value_t volume_decrement(type_safe::get(pqrs::hid::usage::consumer::volume_decrement));

namespace impl {
constexpr std::pair<const mapbox::eternal::string, const value_t> name_value_pairs[] = {
    {"power", power},
    {"display_brightness_increment", display_brightness_increment},
    {"display_brightness_decrement", display_brightness_decrement},
    {"fast_forward", fast_forward},
    {"rewind", rewind},
    {"scan_next_track", scan_next_track},
    {"scan_previous_track", scan_previous_track},
    {"eject", eject},
    {"play_or_pause", play_or_pause},
    {"mute", mute},
    {"volume_increment", volume_increment},
    {"volume_decrement", volume_decrement},

    // Aliases
    {"fastforward", fast_forward},
};

constexpr auto name_value_map = mapbox::eternal::hash_map<mapbox::eternal::string, value_t>(name_value_pairs);

#define USAGE_VALUE_PAIR(name) \
  { pqrs::hid::usage::consumer::name, consumer_key_code::name }

constexpr auto usage_page_consumer_map = mapbox::eternal::map<pqrs::hid::usage::value_t, value_t>({
    USAGE_VALUE_PAIR(power),
    USAGE_VALUE_PAIR(display_brightness_increment),
    USAGE_VALUE_PAIR(display_brightness_decrement),
    USAGE_VALUE_PAIR(fast_forward),
    USAGE_VALUE_PAIR(rewind),
    USAGE_VALUE_PAIR(scan_next_track),
    USAGE_VALUE_PAIR(scan_previous_track),
    USAGE_VALUE_PAIR(eject),
    USAGE_VALUE_PAIR(play_or_pause),
    USAGE_VALUE_PAIR(mute),
    USAGE_VALUE_PAIR(volume_increment),
    USAGE_VALUE_PAIR(volume_decrement),
});

#undef USAGE_VALUE_PAIR
} // namespace impl
} // namespace consumer_key_code

inline std::optional<consumer_key_code::value_t> make_consumer_key_code(const std::string& name) {
  auto& map = consumer_key_code::impl::name_value_map;
  auto it = map.find(name.c_str());
  if (it == map.end()) {
    return std::nullopt;
  }
  return it->second;
}

inline std::string make_consumer_key_code_name(consumer_key_code::value_t consumer_key_code) {
  for (const auto& pair : consumer_key_code::impl::name_value_pairs) {
    if (pair.second == consumer_key_code) {
      return pair.first.c_str();
    }
  }
  return fmt::format("(number:{0})", type_safe::get(consumer_key_code));
}

inline std::optional<consumer_key_code::value_t> make_consumer_key_code(pqrs::hid::usage_page::value_t usage_page,
                                                                        pqrs::hid::usage::value_t usage) {
  if (usage_page == pqrs::hid::usage_page::consumer) {
    // Note:
    // We need to check usage value since some usages are not consumer key event.
    // (e.g., pqrs::hid::usage::consumer::ac_pan is horizontal wheel.)

    auto& map = consumer_key_code::impl::usage_page_consumer_map;
    auto it = map.find(usage);
    if (it != map.end()) {
      return it->second;
    }
  }

  return std::nullopt;
}

inline std::optional<consumer_key_code::value_t> make_consumer_key_code(const pqrs::osx::iokit_hid_value& hid_value) {
  if (auto usage_page = hid_value.get_usage_page()) {
    if (auto usage = hid_value.get_usage()) {
      return make_consumer_key_code(*usage_page,
                                    *usage);
    }
  }
  return std::nullopt;
}

inline std::optional<pqrs::hid::usage_page::value_t> make_hid_usage_page(consumer_key_code::value_t consumer_key_code) {
  return pqrs::hid::usage_page::consumer;
}

inline std::optional<pqrs::hid::usage::value_t> make_hid_usage(consumer_key_code::value_t consumer_key_code) {
  return pqrs::hid::usage::value_t(type_safe::get(consumer_key_code));
}

namespace consumer_key_code {
inline void from_json(const nlohmann::json& json, consumer_key_code::value_t& value) {
  if (json.is_string()) {
    if (auto v = make_consumer_key_code(json.get<std::string>())) {
      value = *v;
    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown consumer_key_code: `{0}`", pqrs::json::dump_for_error_message(json)));
    }
  } else if (json.is_number()) {
    value = consumer_key_code::value_t(json.get<type_safe::underlying_type<value_t>>());
  } else {
    throw pqrs::json::unmarshal_error(fmt::format("json must be string or number, but is `{0}`", pqrs::json::dump_for_error_message(json)));
  }
}
} // namespace consumer_key_code
} // namespace krbn

namespace std {
template <>
struct hash<krbn::consumer_key_code::value_t> : type_safe::hashable<krbn::consumer_key_code::value_t> {
};
} // namespace std
