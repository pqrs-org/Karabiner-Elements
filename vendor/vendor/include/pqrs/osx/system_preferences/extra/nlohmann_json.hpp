#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/hid/extra/nlohmann_json.hpp>
#include <pqrs/json.hpp>
#include <pqrs/osx/iokit_types/extra/nlohmann_json.hpp>
#include <pqrs/osx/system_preferences/properties.hpp>

namespace pqrs {
namespace osx {
namespace system_preferences {

//
// properties
//

inline void to_json(nlohmann::json& j, const properties& value) {
  j = nlohmann::json::object({
      {"use_fkeys_as_standard_function_keys", value.get_use_fkeys_as_standard_function_keys()},
      {"scroll_direction_is_natural", value.get_scroll_direction_is_natural()},
  });
}

inline void from_json(const nlohmann::json& j, properties& value) {
  using namespace std::string_literals;

  json::requires_object(j, "json");

  for (const auto& [k, v] : j.items()) {
    if (k == "use_fkeys_as_standard_function_keys") {
      json::requires_boolean(v, "`"s + k + "`");

      value.set_use_fkeys_as_standard_function_keys(v.get<bool>());

    } else if (k == "scroll_direction_is_natural") {
      json::requires_boolean(v, "`"s + k + "`");

      value.set_scroll_direction_is_natural(v.get<bool>());

    } else {
      throw json::unmarshal_error("unknown key: `"s + k + "`"s);
    }
  }
}
} // namespace system_preferences
} // namespace osx
} // namespace pqrs
