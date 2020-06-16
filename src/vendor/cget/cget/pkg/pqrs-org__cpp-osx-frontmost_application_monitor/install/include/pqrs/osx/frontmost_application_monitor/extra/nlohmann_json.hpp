#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/json.hpp>
#include <pqrs/osx/frontmost_application_monitor/application.hpp>

namespace pqrs {
namespace osx {
namespace frontmost_application_monitor {
inline void to_json(nlohmann::json& j, const application& s) {
  j = nlohmann::json::object();

  if (auto& v = s.get_bundle_identifier()) {
    j["bundle_identifier"] = *v;
  }

  if (auto& v = s.get_file_path()) {
    j["file_path"] = *v;
  }
}

inline void from_json(const nlohmann::json& j, application& s) {
  using namespace std::string_literals;

  json::requires_object(j, "json");

  for (const auto& [key, value] : j.items()) {
    if (key == "bundle_identifier") {
      json::requires_string(value, "`"s + key + "`");

      s.set_bundle_identifier(value.get<std::string>());

    } else if (key == "file_path") {
      json::requires_string(value, "`"s + key + "`");

      s.set_file_path(value.get<std::string>());

    } else {
      throw json::unmarshal_error("unknown key: `"s + key + "`"s);
    }
  }
}
} // namespace frontmost_application_monitor
} // namespace osx
} // namespace pqrs
