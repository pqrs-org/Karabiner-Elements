#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/hid.hpp>
#include <pqrs/json.hpp>

namespace pqrs {
namespace hid {
namespace country_code {
inline void to_json(nlohmann::json& j, const value_t& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, value_t& value) {
  json::requires_number(j, "json");

  value = value_t(j.get<type_safe::underlying_type<value_t>>());
}
} // namespace country_code

namespace product_id {
inline void to_json(nlohmann::json& j, const value_t& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, value_t& value) {
  json::requires_number(j, "json");

  value = value_t(j.get<type_safe::underlying_type<value_t>>());
}
} // namespace product_id

namespace report_id {
inline void to_json(nlohmann::json& j, const value_t& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, value_t& value) {
  json::requires_number(j, "json");

  value = value_t(j.get<type_safe::underlying_type<value_t>>());
}
} // namespace report_id

namespace usage_page {
inline void to_json(nlohmann::json& j, const value_t& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, value_t& value) {
  json::requires_number(j, "json");

  value = value_t(j.get<type_safe::underlying_type<value_t>>());
}
} // namespace usage_page

namespace usage {
inline void to_json(nlohmann::json& j, const value_t& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, value_t& value) {
  json::requires_number(j, "json");

  value = value_t(j.get<type_safe::underlying_type<value_t>>());
}
} // namespace usage

namespace vendor_id {
inline void to_json(nlohmann::json& j, const value_t& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, value_t& value) {
  json::requires_number(j, "json");

  value = value_t(j.get<type_safe::underlying_type<value_t>>());
}
} // namespace vendor_id

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

} // namespace hid
} // namespace pqrs
