#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/hid/extra/nlohmann_json.hpp>
#include <pqrs/json.hpp>
#include <pqrs/osx/iokit_hid_value.hpp>

namespace pqrs {
namespace osx {
// iokit_hid_value

inline void to_json(nlohmann::json& j, const iokit_hid_value& value) {
  j = nlohmann::json::object();

  j["time_stamp"] = type_safe::get(value.get_time_stamp());

  j["integer_value"] = value.get_integer_value();

  if (auto usage_page = value.get_usage_page()) {
    j["usage_page"] = *usage_page;
  }

  if (auto usage = value.get_usage()) {
    j["usage"] = *usage;
  }
}

inline void from_json(const nlohmann::json& j, iokit_hid_value& hid_value) {
  using namespace std::string_literals;

  if (!j.is_object()) {
    throw json::unmarshal_error("json must be object, but is `"s + j.dump() + "`"s);
  }

  for (const auto& [key, value] : j.items()) {
    if (key == "time_stamp") {
      if (!value.is_number()) {
        throw json::unmarshal_error("`"s + key + "` must be number, but is `"s + value.dump() + "`"s);
      }
      hid_value.set_time_stamp(chrono::absolute_time_point(value.get<uint64_t>()));

    } else if (key == "integer_value") {
      if (!value.is_number()) {
        throw json::unmarshal_error("`"s + key + "` must be number, but is `"s + value.dump() + "`"s);
      }
      hid_value.set_integer_value(value.get<CFIndex>());

    } else if (key == "usage_page") {
      if (!value.is_number()) {
        throw json::unmarshal_error("`"s + key + "` must be number, but is `"s + value.dump() + "`"s);
      }
      hid_value.set_usage_page(value.get<hid::usage_page::value_t>());

    } else if (key == "usage") {
      if (!value.is_number()) {
        throw json::unmarshal_error("`"s + key + "` must be number, but is `"s + value.dump() + "`"s);
      }
      hid_value.set_usage(value.get<hid::usage::value_t>());

    } else {
      throw json::unmarshal_error("unknown key: `"s + key + "`"s);
    }
  }
}
} // namespace osx
} // namespace pqrs
