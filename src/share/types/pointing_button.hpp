#pragma once

#include <cstdint>
#include <iostream>
#include <mapbox/eternal.hpp>
#include <pqrs/hid.hpp>
#include <pqrs/osx/iokit_hid_value.hpp>

namespace krbn {
namespace pointing_button {
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

constexpr value_t zero(0);
constexpr value_t button1(type_safe::get(pqrs::hid::usage::button::button_1));
constexpr value_t button2(type_safe::get(pqrs::hid::usage::button::button_2));
constexpr value_t button3(type_safe::get(pqrs::hid::usage::button::button_3));
constexpr value_t button4(type_safe::get(pqrs::hid::usage::button::button_4));
constexpr value_t button5(type_safe::get(pqrs::hid::usage::button::button_5));
constexpr value_t button6(type_safe::get(pqrs::hid::usage::button::button_6));
constexpr value_t button7(type_safe::get(pqrs::hid::usage::button::button_7));
constexpr value_t button8(type_safe::get(pqrs::hid::usage::button::button_8));
constexpr value_t button9(type_safe::get(pqrs::hid::usage::button::button_9));
constexpr value_t button10(type_safe::get(pqrs::hid::usage::button::button_10));
constexpr value_t button11(type_safe::get(pqrs::hid::usage::button::button_11));
constexpr value_t button12(type_safe::get(pqrs::hid::usage::button::button_12));
constexpr value_t button13(type_safe::get(pqrs::hid::usage::button::button_13));
constexpr value_t button14(type_safe::get(pqrs::hid::usage::button::button_14));
constexpr value_t button15(type_safe::get(pqrs::hid::usage::button::button_15));
constexpr value_t button16(type_safe::get(pqrs::hid::usage::button::button_16));
constexpr value_t button17(type_safe::get(pqrs::hid::usage::button::button_17));
constexpr value_t button18(type_safe::get(pqrs::hid::usage::button::button_18));
constexpr value_t button19(type_safe::get(pqrs::hid::usage::button::button_19));
constexpr value_t button20(type_safe::get(pqrs::hid::usage::button::button_20));
constexpr value_t button21(type_safe::get(pqrs::hid::usage::button::button_21));
constexpr value_t button22(type_safe::get(pqrs::hid::usage::button::button_22));
constexpr value_t button23(type_safe::get(pqrs::hid::usage::button::button_23));
constexpr value_t button24(type_safe::get(pqrs::hid::usage::button::button_24));
constexpr value_t button25(type_safe::get(pqrs::hid::usage::button::button_25));
constexpr value_t button26(type_safe::get(pqrs::hid::usage::button::button_26));
constexpr value_t button27(type_safe::get(pqrs::hid::usage::button::button_27));
constexpr value_t button28(type_safe::get(pqrs::hid::usage::button::button_28));
constexpr value_t button29(type_safe::get(pqrs::hid::usage::button::button_29));
constexpr value_t button30(type_safe::get(pqrs::hid::usage::button::button_30));
constexpr value_t button31(type_safe::get(pqrs::hid::usage::button::button_31));
constexpr value_t button32(type_safe::get(pqrs::hid::usage::button::button_32));

namespace impl {
constexpr std::pair<const mapbox::eternal::string, const value_t> name_value_pairs[] = {
    {"button1", button1},
    {"button2", button2},
    {"button3", button3},
    {"button4", button4},
    {"button5", button5},
    {"button6", button6},
    {"button7", button7},
    {"button8", button8},
    {"button9", button9},
    {"button10", button10},
    {"button11", button11},
    {"button12", button12},
    {"button13", button13},
    {"button14", button14},
    {"button15", button15},
    {"button16", button16},
    {"button17", button17},
    {"button18", button18},
    {"button19", button19},
    {"button20", button20},
    {"button21", button21},
    {"button22", button22},
    {"button23", button23},
    {"button24", button24},
    {"button25", button25},
    {"button26", button26},
    {"button27", button27},
    {"button28", button28},
    {"button29", button29},
    {"button30", button30},
    {"button31", button31},
    {"button32", button32},
};

constexpr auto name_value_map = mapbox::eternal::hash_map<mapbox::eternal::string, value_t>(name_value_pairs);
} // namespace impl
} // namespace pointing_button

inline std::optional<pointing_button::value_t> make_pointing_button(const std::string& name) {
  auto& map = pointing_button::impl::name_value_map;
  auto it = map.find(name.c_str());
  if (it == map.end()) {
    return std::nullopt;
  }
  return it->second;
}

inline std::string make_pointing_button_name(pointing_button::value_t pointing_button) {
  for (const auto& pair : pointing_button::impl::name_value_pairs) {
    if (pair.second == pointing_button) {
      return pair.first.c_str();
    }
  }
  return fmt::format("(number:{0})", type_safe::get(pointing_button));
}

inline std::optional<pointing_button::value_t> make_pointing_button(pqrs::hid::usage_page::value_t usage_page,
                                                                    pqrs::hid::usage::value_t usage) {
  if (usage_page == pqrs::hid::usage_page::button) {
    return pointing_button::value_t(type_safe::get(usage));
  }
  return std::nullopt;
}

inline std::optional<pointing_button::value_t> make_pointing_button(const pqrs::osx::iokit_hid_value& hid_value) {
  if (auto usage_page = hid_value.get_usage_page()) {
    if (auto usage = hid_value.get_usage()) {
      return make_pointing_button(*usage_page,
                                  *usage);
    }
  }
  return std::nullopt;
}

namespace pointing_button {
inline void from_json(const nlohmann::json& json, value_t& value) {
  if (json.is_string()) {
    if (auto v = make_pointing_button(json.get<std::string>())) {
      value = *v;
    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown pointing_button: `{0}`", pqrs::json::dump_for_error_message(json)));
    }
  } else if (json.is_number()) {
    value = pointing_button::value_t(json.get<type_safe::underlying_type<value_t>>());
  } else {
    throw pqrs::json::unmarshal_error(fmt::format("json must be string or number, but is `{0}`", pqrs::json::dump_for_error_message(json)));
  }
}
} // namespace pointing_button
} // namespace krbn

namespace std {
template <>
struct hash<krbn::pointing_button::value_t> : type_safe::hashable<krbn::pointing_button::value_t> {
};
} // namespace std
