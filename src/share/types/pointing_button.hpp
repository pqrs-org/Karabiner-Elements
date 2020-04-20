#pragma once

#include "stream_utility.hpp"
#include <cstdint>
#include <mapbox/eternal.hpp>
#include <pqrs/osx/iokit_hid_value.hpp>

namespace krbn {
enum class pointing_button : uint32_t {
  zero,

  button1,
  button2,
  button3,
  button4,
  button5,
  button6,
  button7,
  button8,

  button9,
  button10,
  button11,
  button12,
  button13,
  button14,
  button15,
  button16,

  button17,
  button18,
  button19,
  button20,
  button21,
  button22,
  button23,
  button24,

  button25,
  button26,
  button27,
  button28,
  button29,
  button30,
  button31,
  button32,

  end_,
};

namespace impl {
constexpr std::pair<const mapbox::eternal::string, const pointing_button> pointing_button_name_value_pairs[] = {
    {"button1", pointing_button::button1},
    {"button2", pointing_button::button2},
    {"button3", pointing_button::button3},
    {"button4", pointing_button::button4},
    {"button5", pointing_button::button5},
    {"button6", pointing_button::button6},
    {"button7", pointing_button::button7},
    {"button8", pointing_button::button8},

    {"button9", pointing_button::button9},
    {"button10", pointing_button::button10},
    {"button11", pointing_button::button11},
    {"button12", pointing_button::button12},
    {"button13", pointing_button::button13},
    {"button14", pointing_button::button14},
    {"button15", pointing_button::button15},
    {"button16", pointing_button::button16},

    {"button17", pointing_button::button17},
    {"button18", pointing_button::button18},
    {"button19", pointing_button::button19},
    {"button20", pointing_button::button20},
    {"button21", pointing_button::button21},
    {"button22", pointing_button::button22},
    {"button23", pointing_button::button23},
    {"button24", pointing_button::button24},

    {"button25", pointing_button::button25},
    {"button26", pointing_button::button26},
    {"button27", pointing_button::button27},
    {"button28", pointing_button::button28},
    {"button29", pointing_button::button29},
    {"button30", pointing_button::button30},
    {"button31", pointing_button::button31},
    {"button32", pointing_button::button32},
};

constexpr auto pointing_button_name_value_map = mapbox::eternal::hash_map<mapbox::eternal::string, pointing_button>(pointing_button_name_value_pairs);
} // namespace impl

inline std::optional<pointing_button> make_pointing_button(const std::string& name) {
  auto& map = impl::pointing_button_name_value_map;
  auto it = map.find(name.c_str());
  if (it == map.end()) {
    return std::nullopt;
  }
  return it->second;
}

inline std::string make_pointing_button_name(pointing_button pointing_button) {
  for (const auto& pair : impl::pointing_button_name_value_pairs) {
    if (pair.second == pointing_button) {
      return pair.first.c_str();
    }
  }
  return fmt::format("(number:{0})", static_cast<uint32_t>(pointing_button));
}

inline std::optional<pointing_button> make_pointing_button(pqrs::hid::usage_page::value_t usage_page,
                                                           pqrs::hid::usage::value_t usage) {
  if (usage_page == pqrs::hid::usage_page::button) {
    return pointing_button(type_safe::get(usage));
  }
  return std::nullopt;
}

inline std::optional<pointing_button> make_pointing_button(const pqrs::osx::iokit_hid_value& hid_value) {
  if (auto usage_page = hid_value.get_usage_page()) {
    if (auto usage = hid_value.get_usage()) {
      return make_pointing_button(*usage_page,
                                  *usage);
    }
  }
  return std::nullopt;
}

inline void from_json(const nlohmann::json& json, pointing_button& value) {
  if (json.is_string()) {
    if (auto v = make_pointing_button(json.get<std::string>())) {
      value = *v;
    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown pointing_button: `{0}`", json.dump()));
    }
  } else if (json.is_number()) {
    value = pointing_button(json.get<uint32_t>());
  } else {
    throw pqrs::json::unmarshal_error(fmt::format("json must be string or number, but is `{0}`", json.dump()));
  }
}

inline std::ostream& operator<<(std::ostream& stream, const pointing_button& value) {
  return stream_utility::output_enum(stream, value);
}

template <template <class T, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<pointing_button, std::allocator<pointing_button>>& values) {
  return stream_utility::output_enums(stream, values);
}

template <template <class T, class H, class K, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<pointing_button, std::hash<pointing_button>, std::equal_to<pointing_button>, std::allocator<pointing_button>>& values) {
  return stream_utility::output_enums(stream, values);
}
} // namespace krbn
