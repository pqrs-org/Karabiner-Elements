#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <nlohmann/json.hpp>
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
  try {
    s.set_bundle_identifier(j.at("bundle_identifier").get<std::string>());
  } catch (...) {}

  try {
    s.set_file_path(j.at("file_path").get<std::string>());
  } catch (...) {}
}
} // namespace frontmost_application_monitor
} // namespace osx
} // namespace pqrs
