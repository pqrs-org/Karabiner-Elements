#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/hid.hpp>
#include <pqrs/json.hpp>
#include <type_traits>

namespace pqrs::hid {
namespace detail {
template <typename>
inline constexpr bool always_false_v = false;

template <typename T>
  requires type_safe::is_strong_typedef<T>::value
inline void to_json(nlohmann::json& j, const T& value) {
  j = type_safe::get(value);
}

template <typename T>
  requires type_safe::is_strong_typedef<T>::value
inline void from_json(const nlohmann::json& j, T& value) {
  using underlying_type = type_safe::underlying_type<T>;

  if constexpr (std::is_arithmetic_v<underlying_type>) {
    json::requires_number(j, "json");

  } else if constexpr (std::is_same_v<underlying_type, std::string>) {
    json::requires_string(j, "json");

  } else {
    static_assert(always_false_v<underlying_type>, "unsupported strong_typedef underlying type");
  }

  value = T(j.get<underlying_type>());
}
} // namespace detail

//
// number values
//

namespace country_code {
using detail::from_json;
using detail::to_json;
} // namespace country_code

namespace product_id {
using detail::from_json;
using detail::to_json;
} // namespace product_id

namespace report_id {
using detail::from_json;
using detail::to_json;
} // namespace report_id

namespace usage_page {
using detail::from_json;
using detail::to_json;
} // namespace usage_page

namespace usage {
using detail::from_json;
using detail::to_json;
} // namespace usage

namespace vendor_id {
using detail::from_json;
using detail::to_json;
} // namespace vendor_id

//
// string values
//

namespace manufacturer_string {
using detail::from_json;
using detail::to_json;
} // namespace manufacturer_string

namespace product_string {
using detail::from_json;
using detail::to_json;
} // namespace product_string

//
// usage_pair
//

inline void to_json(nlohmann::json& j, const usage_pair& usage_pair) {
  j["usage_page"] = usage_pair.get_usage_page();
  j["usage"] = usage_pair.get_usage();
}

inline void from_json(const nlohmann::json& j, usage_pair& usage_pair) {
  using namespace std::string_literals;

  json::requires_object(j, "json");

  for (const auto& [key, value] : j.items()) {
    if (key == "usage_page") {
      usage_pair.set_usage_page(value.get<usage_page::value_t>());

    } else if (key == "usage") {
      usage_pair.set_usage(value.get<usage::value_t>());

    } else {
      throw json::unmarshal_error("unknown key: `"s + key + "`"s);
    }
  }
}

} // namespace pqrs::hid
